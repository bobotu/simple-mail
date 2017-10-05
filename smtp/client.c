//
// Created by 李泽钧 on 2017/9/26.
//

#include "log.h"
#include <glib.h>
#include <conn.h>
#include <gmime/gmime.h>

#include "client.h"
#include "error.h"
#include "parser.h"

#define AUTH_OK_CODE       334
#define AUTH_SUCC_CODE     235
#define DATA_START         354
#define COMMAND_READY_CODE 220
#define COMMAND_OK_CODE    250

static
int read_response(conn_t *conn, GPtrArray **resp) {
    GPtrArray *result = g_ptr_array_new_with_free_func((GDestroyNotify) smtp_destroy_reply);

    conn_set_term(conn, "\r\n");
    char         *line;
    smtp_reply_t *reply;
    bool      first   = true;

    while (1) {
        ssize_t len = conn_read_line(conn, &line);
        if (len < 0) {
            g_ptr_array_unref(result);
            g_critical("read response encounter error: %s", conn_err_to_str((int) len));
            return -S_ERR_CONNECTION;
        }

        int status = smtp_parse_line(line, &reply, first);
        if (status < 0) {
            g_ptr_array_unref(result);
            return status;
        }

        first = false;
        g_ptr_array_add(result, reply);
        if (status > 0) {
            break;
        }
    }

    *resp = result;

    return 0;
}

static
int smtp_send_command(conn_t *conn, const char *cmd, size_t len, GPtrArray **resp) {
    int err = conn_send(conn, cmd, len);
    if (err < 0) {
        g_critical("send command encounter error: %s", conn_err_to_str(err));
        return -S_ERR_CONNECTION;
    }

    return read_response(conn, resp);
}

static
int smtp_send_command_get_code(conn_t *conn, const char *cmd, size_t len) {
    GPtrArray *resp;

    int ret = smtp_send_command(conn, cmd, len, &resp);
    if (ret < 0) return ret;
    if (resp->len < 1) return -S_ERR_BAD_RESPONSE;

    smtp_reply_t *r = g_ptr_array_index(resp, 0);
    ret = r->code;
    g_ptr_array_unref(resp);

    return ret;
}

int smtp_connect(char *host, bool ssl, smtp_client_t **client) {
    smtp_client_t *c = g_new0(smtp_client_t, 1);
    *client = c;
    int ret = conn_connect_server(host, ssl, &c->conn);
    if (ret < 0) {
        g_critical("connect server encounter error: %s", conn_err_to_str(ret));
        return -S_ERR_CONNECTION;
    }

    GPtrArray *replies;
    ret = read_response(c->conn, &replies);
    if (ret < 0) {
        g_critical("read greeting text encounter error: %s", conn_err_to_str(ret));
        return -S_ERR_CONNECTION;
    }
    smtp_init_resp_t *resp = &c->init_resp;

    int text_idx = 0;

    for (int i = 0; i < replies->len; i++) {
        smtp_reply_t *r = g_ptr_array_index(replies, i);
        if (r->code != S_RESP_GREETING) {
            ret = -r->code;
            break;
        }

        if (r->payload != NULL) {
            smtp_reply_greeting *g = r->payload;
            if (g->domain != NULL) resp->domain = g_strdup(g->domain);
            if (g->text != NULL) {
                if (resp->texts == NULL) {
                    resp->texts = g_new0(char *, replies->len + 1);
                    resp->buf   = g_string_chunk_new(replies->len);
                }
                resp->texts[text_idx] = g_string_chunk_insert(resp->buf, g->text);
            }
        }
    }

    g_ptr_array_unref(replies);

    return ret;
}

inline static
enum smtp_auth_method resolve_auth_method(char *m) {
    if (g_strcmp0("LOGIN", m)) return login;
    else if (g_strcmp0("PLAIN", m)) return plain;
    else return unknown;
}

static
void handle_auth_methods(char **methods, smtp_ehlo_resp_t *resp) {
    for (int i = 0; methods[i] != NULL; i++) {
        enum smtp_auth_method m = resolve_auth_method(methods[i]);
        if (m == login) resp->auth_methods.login = true;
        else if (m == plain) resp->auth_methods.plain = true;
    }
}

static
void handle_ehlo_reply(smtp_reply_ehlo *ehlo, smtp_ehlo_resp_t *resp) {
    if (ehlo->domain != NULL) {
        resp->domain = g_strdup(ehlo->domain);
        if (ehlo->greeting != NULL) resp->greeting = g_strdup(ehlo->greeting);
    }

    if (ehlo->keyword == NULL) return;

    if (g_str_equal("SIZE", ehlo->keyword)) {
        if (ehlo->args[0] != NULL) resp->size = atoi(ehlo->args[0]);
    } else if (g_str_equal("AUTH", ehlo->keyword)) {
        handle_auth_methods(ehlo->args, resp);
    } else if (g_str_has_prefix(ehlo->keyword, "AUTH=")) {
        resp->auth = resolve_auth_method(ehlo->args[0]);
    } else if (g_str_equal("STARTTLS", ehlo->keyword)) {
        resp->starttls = true;
    }
}

int smtp_ehlo(smtp_client_t *client, const char *msg) {
    GString *data = g_string_new(NULL);
    g_string_printf(data, "%s %s\r\n", S_EHLO, msg);

    int       ret = 0;
    GPtrArray *replies;
    ret = smtp_send_command(client->conn, data->str, data->len, &replies);
    g_string_free(data, true);
    if (ret < 0) {
        g_ptr_array_unref(replies);
        return ret;
    }

    for (int i = 0; i < replies->len; i++) {
        smtp_reply_t *r = g_ptr_array_index(replies, i);
        if (r->code != S_RESP_EHLO) {
            ret = -r->code;
            break;
        }

        if (r->payload != NULL) {
            handle_ehlo_reply(r->payload, &client->ehlo_resp);
        }
    }

    g_ptr_array_unref(replies);

    return ret;
}

static
int smtp_auth_login(smtp_client_t *c, const char *un, const char *pw) {
    int ret = 0;

    char *cmd = S_AUTH " LOGIN" "\r\n";
    ret = smtp_send_command_get_code(c->conn, cmd, strlen(cmd));
    if (ret < 0) return ret;
    if (ret != AUTH_OK_CODE) return -ret;

    char *username = g_base64_encode((const guchar *) un, strlen(un));
    cmd = g_strconcat(username, "\r\n", NULL);
    ret = smtp_send_command_get_code(c->conn, cmd, strlen(cmd));
    g_free(username);
    g_free(cmd);
    if (ret < 0) return ret;
    if (ret != AUTH_OK_CODE) return -ret;

    char *password = g_base64_encode((const guchar *) pw, strlen(pw));
    cmd = g_strconcat(password, "\r\n", NULL);
    ret = smtp_send_command_get_code(c->conn, cmd, strlen(cmd));
    g_free(password);
    g_free(cmd);
    if (ret < 0) return ret;
    if (ret != AUTH_SUCC_CODE) return -ret;

    return 0;
}

static
int smtp_auth_plain(smtp_client_t *c, const char *un, const char *pw) {
    int ret = 0;

    GString *cmd = g_string_new(NULL);
    g_string_append_c(cmd, '\0');
    g_string_append(cmd, un);
    g_string_append_c(cmd, '\0');
    g_string_append(cmd, pw);
    char *encoded = g_base64_encode((const guchar *) cmd->str, cmd->len);
    g_string_printf(cmd, S_AUTH" PLAIN %s\r\n", encoded);
    g_free(encoded);

    ret = smtp_send_command_get_code(c->conn, cmd->str, cmd->len);
    g_string_free(cmd, true);
    if (ret < 0) return ret;
    if (ret != AUTH_SUCC_CODE) return -ret;

    return 0;
}

int smtp_auth(smtp_client_t *client, const char *username, const char *passwd) {
    smtp_ehlo_resp_t *r = &client->ehlo_resp;

    if (r->starttls && !client->conn->is_ssl) {
        g_info("find server support TLS, upgrading to TLS connection");
        char *cmd = S_STARTTLS"\r\n";
        int  ret  = smtp_send_command_get_code(client->conn, cmd, strlen(cmd));
        if (ret < 0) return ret;
        if (ret != COMMAND_READY_CODE) {
            g_error(S_STARTTLS
                    " command fail. response code is: %d", ret);
            return -ret;
        }
        conn_upgrade_ssl(client->conn);
    }

    if (r->auth_methods.login) {
        return smtp_auth_login(client, username, passwd);
    } else if (r->auth_methods.plain) {
        return smtp_auth_plain(client, username, passwd);
    }

    return -S_ERR_NO_SUPPORTED_AUTH;
}

int smtp_mail(smtp_client_t *client, const char *from) {
    g_info("new mail from %s", from);
    GString *cmd = g_string_new(NULL);
    g_string_printf(cmd, S_MAIL" FROM:<%s>\r\n", from);
    int ret = smtp_send_command_get_code(client->conn, cmd->str, cmd->len);
    g_string_free(cmd, true);
    if (ret < 0) return ret;
    if (ret != COMMAND_OK_CODE) return -ret;

    return 0;
}

int smtp_rcpt(smtp_client_t *client, const char **rcpt) {
    GString *cmd = g_string_new(NULL);

    int sent = 0;
    int cnt  = 0;

    for (int i = 0; rcpt[i] != NULL; i++) {
        cnt++;
        g_string_printf(cmd, S_RCPT" TO:<%s>\r\n", rcpt[i]);
        int ret = smtp_send_command_get_code(client->conn, cmd->str, cmd->len);
        if (ret < 0) {
            g_warning("rcpt to: %s fail. error: %d", rcpt[i], smtp_err_to_str(ret));
            continue;
        }
        if (ret != COMMAND_OK_CODE) {
            g_warning("rcpt to: %s fail. error: %d", rcpt[i], smtp_err_to_str(ret));
            continue;
        }
        sent--;
    }
    g_string_free(cmd, true);
    return sent - cnt;
}

int smtp_data(smtp_client_t *client, const void *data, size_t len) {
    int ret = smtp_send_command_get_code(client->conn, S_DATA"\r\n", strlen(S_DATA) + 2);
    if (ret < 0) return ret;
    if (ret != DATA_START) return -ret;

    ret = conn_send(client->conn, data, len);
    if (ret < 0) {
        g_critical("send data encounter connection error: %s", conn_err_to_str(ret));
        return -S_ERR_CONNECTION;
    }

    ret = smtp_send_command_get_code(client->conn, "\r\n.\r\n", 5);
    if (ret < 0) return ret;
    if (ret != COMMAND_OK_CODE) return -ret;

    return 0;
}

char *mail_to_mime_str(smtp_mail_t mail) {
    char         *to  = g_strjoinv(";", mail.rcpts);
    GMimeMessage *msg = g_mime_message_new(true);
    g_mime_object_set_header((GMimeObject *) msg, "From", mail.mail_address, NULL);
    g_mime_object_set_header((GMimeObject *) msg, "To", to, NULL);
    g_mime_message_set_date(msg, g_date_time_new_now_local());
    g_mime_message_set_subject(msg, mail.subject, NULL);
    g_free(to);

    GMimeTextPart *part = g_mime_text_part_new_with_subtype("plain");
    g_mime_text_part_set_text(part, mail.payload);
    g_mime_message_set_mime_part(msg, (GMimeObject *) part);
    g_object_unref(part);

    return g_mime_object_to_string((GMimeObject *) msg, NULL);
}

int smtp_send_mail(smtp_client_t *client, smtp_mail_t mail, bool need_auth) {
    int err = 0;
    if (need_auth) {
        err = smtp_ehlo(client, mail.mail_address);
        if (err < 0) return err;

        err = smtp_auth(client, mail.mail_address, mail.password);
        if (err < 0) return err;
    }

    err = smtp_mail(client, mail.mail_address);
    if (err < 0) return err;

    err = smtp_rcpt(client, (const char **) mail.rcpts);
    if (err < 0) {
        g_critical("%d rcpt failed", -err);
    }

    char *payload = mail_to_mime_str(mail);

    err = smtp_data(client, payload, strlen(payload));
    if (err < 0) return err;

    g_free(payload);
    return 0;
}

void smtp_client_close(smtp_client_t *client) {
    smtp_send_command_get_code(client->conn, S_QUIT"\r\n", 6);
    smtp_init_resp_clear(&client->init_resp);
    smtp_ehlo_resp_clear(&client->ehlo_resp);
    conn_close_conn(client->conn);
    g_free(client);
}

void smtp_init_resp_clear(smtp_init_resp_t *resp) {
    g_free(resp->domain);
    g_free(resp->texts);
    g_string_chunk_free(resp->buf);
}

void smtp_ehlo_resp_clear(smtp_ehlo_resp_t *resp) {
    g_free(resp->domain);
    g_free(resp->greeting);
}

