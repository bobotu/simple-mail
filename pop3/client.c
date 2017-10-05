//
// Created by 李泽钧 on 2017/9/28.
//

#include "log.h"
#include <glib.h>

#include "client.h"
#include "error.h"

static
char *read_response_line(pop_client_t *client) {
    char    *line = NULL;
    ssize_t sz    = conn_read_line(client->conn, &line);
    if (sz < 0) {
        g_critical("read response encounter connection error: %", conn_err_to_str((int) sz));
        return NULL;
    }
    return line;
}

static
int read_multiline_response(pop_client_t *client, GPtrArray **resp) {
    GPtrArray *r = g_ptr_array_new_with_free_func(g_free);
    while (1) {
        char *line = read_response_line(client);
        if (line == NULL) {
            g_ptr_array_unref(r);
            return -P_ERR_CONNECTION;
        }
        if (line[0] == '.') {
            g_free(line);
            break;
        }
        g_ptr_array_add(r, line);
    }

    *resp = r;
    return 0;
}

static
int get_resp_status(pop_client_t *client) {
    char *line = read_response_line(client);
    if (line == NULL) return -P_ERR_CONNECTION;

    char f = line[0];
    g_free(line);
    if (f != '+') return -P_ERR_BAD_RESPONSE;
    return 0;
}

static
int send_cmd_get_resp_status(pop_client_t *client, void *cmd, size_t len) {
    int ret = conn_send(client->conn, cmd, len);
    if (ret < 0) {
        g_critical("send command encounter connection error: %", conn_err_to_str(ret));
        return -P_ERR_CONNECTION;
    }

    return get_resp_status(client);
}

static
char *send_cmd_read_response(pop_client_t *client, void *cmd, size_t len) {
    int ret = conn_send(client->conn, cmd, len);
    if (ret < 0) {
        g_critical("send command encounter connection error: %", conn_err_to_str(ret));
        return NULL;
    }

    return read_response_line(client);
}

int pop_connect(char *host, bool ssl, pop_client_t **client) {
    pop_client_t *c = g_new0(pop_client_t, 1);
    *client = c;
    int ret = conn_connect_server(host, ssl, &c->conn);
    if (ret < 0) {
        g_critical("connect server encounter error: %s", conn_err_to_str(ret));
        return -P_ERR_CONNECTION;
    }
    conn_set_term(c->conn, "\r\n");

    return get_resp_status(c);
}

int pop_use_ssl(pop_client_t *client) {
    if (client->conn->is_ssl) return 0;

    int ret = send_cmd_get_resp_status(client, P_STLS"\r\n", 6);
    if (ret < 0) return ret;

    ret = conn_upgrade_ssl(client->conn);
    if (ret < 0) {
        return -P_ERR_CONNECTION;
    }
    return 0;
}

int pop_auth(pop_client_t *client, const char *username, const char *passwd) {
    GString *cmd = g_string_new(NULL);
    g_string_printf(cmd, P_USER" %s\r\n", username);
    int ret = send_cmd_get_resp_status(client, cmd->str, cmd->len);
    if (ret < 0) {
        g_string_free(cmd, true);
        return ret;
    }

    g_string_printf(cmd, P_PASS" %s\r\n", passwd);
    ret = send_cmd_get_resp_status(client, cmd->str, cmd->len);
    if (ret < 0) {
        g_string_free(cmd, true);
        return ret;
    }

    g_string_free(cmd, true);

    return 0;
}

int pop_status(pop_client_t *client, pop_status_t *stat) {
    char *line = send_cmd_read_response(client, P_STAT"\r\n", 6);
    if (line == NULL) return -P_ERR_CONNECTION;

    if (line[0] != '+') {
        g_free(line);
        return -P_ERR_BAD_RESPONSE;
    }

    char *resp  = line + 4;
    char **digs = g_strsplit(resp, " ", 2);

    stat->num = atoi(digs[0]);
    stat->sz  = atoi(digs[1]);

    g_strfreev(digs);
    g_free(line);

    return 0;
}

int pop_list(pop_client_t *client, pop_list_t **list, size_t *len) {
    int ret = send_cmd_get_resp_status(client, P_LIST"\r\n", 6);
    if (ret < 0) return ret;

    GPtrArray *lines;
    read_multiline_response(client, &lines);

    pop_list_t *l = g_new(pop_list_t, lines->len);
    *list = l;
    *len  = lines->len;
    int i;
    for (i = 0; i < lines->len; i++) {
        char *line  = g_ptr_array_index(lines, i);
        char **digs = g_strsplit(line, " ", 2);
        l[i].mid = atoi(digs[0]);
        l[i].sz  = atoi(digs[1]);
        g_strfreev(digs);
    }

    g_ptr_array_unref(lines);

    return 0;
}

int pop_top(pop_client_t *client, int mid, size_t sz, char **data, size_t *data_len) {
    GString *cmd = g_string_new(NULL);
    g_string_printf(cmd, P_TOP" %d %ld\r\n", mid, sz);
    int ret = send_cmd_get_resp_status(client, cmd->str, cmd->len);
    g_string_free(cmd, true);
    if (ret < 0) return ret;

    ssize_t len = conn_read_line_with_term(client->conn, data, ".\r\n");
    if (len < 0) {
        g_critical("read mail %d encounter connection error: %", mid, conn_err_to_str((int) len));
        return -P_ERR_CONNECTION;
    }
    *data_len = (size_t) len;

    return 0;
}

int pop_retr(pop_client_t *client, int mid, char **data, size_t *data_len) {
    GString *cmd = g_string_new(NULL);
    g_string_printf(cmd, P_RETR" %d\r\n", mid);
    int ret = send_cmd_get_resp_status(client, cmd->str, cmd->len);
    g_string_free(cmd, true);
    if (ret < 0) return ret;

    ssize_t len = conn_read_line_with_term(client->conn, data, ".\r\n");
    if (len < 0) {
        g_critical("read mail %d encounter connection error: %", mid, conn_err_to_str((int) len));
        return -P_ERR_CONNECTION;
    }
    *data_len = (size_t) len;

    return 0;
}

int pop_del(pop_client_t *client, int mid) {
    GString *cmd = g_string_new(NULL);
    g_string_printf(cmd, P_DELE" %d\r\n", mid);
    int ret = send_cmd_get_resp_status(client, cmd->str, cmd->len);
    g_string_free(cmd, true);
    return ret;
}

int pop_quit(pop_client_t *client) {
    conn_send(client->conn, P_QUIT"\r\n", 6);
    return 0;
}

int pop_get_mail_list(pop_client_t *client, const char *username, const char *passwd) {
    int err = pop_use_ssl(client);
    if (err < 0) return err;

    err = pop_auth(client, username, passwd);
    if (err < 0) return err;

    err = pop_list(client, &client->mails, &client->mail_num);
    if (err < 0) return err;

    return 0;
}

void pop_client_close(pop_client_t *client) {
    pop_quit(client);
    conn_close_conn(client->conn);
    g_free(client->mails);
}
