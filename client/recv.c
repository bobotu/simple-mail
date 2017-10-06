//
// Created by 李泽钧 on 2017/10/7.
//

#include <glib.h>
#include <gmime/gmime.h>
#include <pop3.h>

#include "common.h"

static
void message_part_callback(GMimeObject *parent, GMimeObject *part, void *user_data) {
    GString *buf = user_data;

    if (GMIME_IS_TEXT_PART(part)) {
        GMimeDataWrapper *content     = g_mime_part_get_content(GMIME_PART(part));
        GMimeStream      *content_buf = g_mime_stream_mem_new();
        g_mime_data_wrapper_write_to_stream(content, content_buf);

        GByteArray *bytes_arr = g_mime_stream_mem_get_byte_array((GMimeStreamMem *) content_buf);
        GBytes     *bytes     = g_byte_array_free_to_bytes(bytes_arr);
        size_t     len        = 0;
        char       *text      = g_bytes_unref_to_data(bytes, &len);

        g_string_append_len(buf, text, len);
        g_string_append(buf, "\n");
    } else if (GMIME_IS_MESSAGE_PART(part)) {
        GMimeMessage *message;

        message = g_mime_message_part_get_message((GMimeMessagePart *) part);
        g_mime_message_foreach(message, message_part_callback, user_data);
    } else {
        g_string_append(buf, "(非文本内容无法显示)\n");
    }
}

static
GString *format_mail(GMimeMessage *mail) {
    GString *buf = g_string_new(NULL);

    InternetAddressList *from    = g_mime_message_get_from(mail);
    char                *encoded = internet_address_list_to_string(from, NULL, false);
    char                *decoded = g_mime_utils_header_decode_text(NULL, encoded);
    g_free(encoded);
    g_string_append_printf(buf, "From: %s\n", decoded);
    g_free(decoded);

    InternetAddressList *to = g_mime_message_get_to(mail);
    encoded = internet_address_list_to_string(to, NULL, false);
    decoded = g_mime_utils_header_decode_text(NULL, encoded);
    g_free(encoded);
    g_string_append_printf(buf, "To: %s\n", decoded);
    g_free(decoded);

    encoded = (char *) g_mime_message_get_subject(mail);
    decoded = g_mime_utils_header_decode_text(NULL, encoded);
    g_string_append_printf(buf, "Subject: %s\n", decoded);
    g_free(decoded);

    GDateTime *time = g_mime_message_get_date(mail);
    decoded = g_date_time_format(time, "%F %T");
    g_string_append_printf(buf, "Date: %s\n\n", decoded);
    g_free(decoded);

    g_mime_message_foreach(mail, message_part_callback, buf);

    g_string_append_c(buf, '\0');

    return buf;
}

static
GString *recv_one_mail(pop_client_t *client, int mid, int top) {
    int    err   = 0;
    char   *data = NULL;
    size_t len;

    if (top > 0) {
        err = pop_top(client, mid, (size_t) top, &data, &len);
    } else {
        err = pop_retr(client, mid, &data, &len);
    }
    if (err < 0) {
        g_free(data);
        fprintf(stderr, "读取邮件 %d 失败，错误内容 %s", mid, pop_err_to_str(err));
        return NULL;
    }

    GMimeParser  *parser;
    GMimeMessage *mail;
    GMimeStream  *stream;

    stream = g_mime_stream_mem_new_with_buffer(data, len);
    g_free(data);
    parser = g_mime_parser_new_with_stream(stream);
    g_object_unref(stream);
    mail = g_mime_parser_construct_message(parser, NULL);
    g_object_unref(parser);

    GString *text = format_mail(mail);
    g_object_unref(mail);
    return text;
}

static
int recv_mail_all(pop_client_t *client, pop_list_t *mails, size_t sz, int top) {
    for (int i = (int) (sz - 1); i >= 0; i--) {
        GString *mail = recv_one_mail(client, mails[i].mid, top);
        if (mail == NULL) return 1;
        printf("\n%s\n", mail->str);
        g_string_free(mail, true);
    }
    return 0;
}

static
int recv_mail_interactive(pop_client_t *client, pop_list_t *mails, size_t sz, int top) {
    for (int i = (int) (sz - 1); i >= 0; i--) {
        GString *mail = recv_one_mail(client, mails[i].mid, top);
        if (mail == NULL) return 1;
        printf("\n%s\n", mail->str);
        g_string_free(mail, true);
        printf("(下一封/退出 - n/q): ");
        fflush(stdout);
        int op = getc(stdin);
        while (getc(stdin) != '\n');
        switch (op) {
            case 'n':
                break;
            default:
                return 0;
        }
    }
    return 0;
}

int app_recv_mail(struct app_recv_options *options) {
    struct app_base_options *bo = options->base;

    pop_client_t *client    = NULL;
    pop_list_t   *mail_list = NULL;

    APP_LOG(bo, "开始连接服务器 %s (%s) \n",
            bo->host,
            bo->use_ssl ? "使用 TLS 连接" : "不使用 TLS");
    int err = pop_connect(bo->host, bo->use_ssl, &client);
    if (err < 0) goto done;

    APP_LOG(bo, "以 %s (%s) 登录\n", bo->username, bo->password);
    err = pop_auth(client, bo->username, bo->password);
    if (err < 0) goto done;
    if (client->conn->is_ssl && !bo->use_ssl) {
        APP_LOG(bo, "出于安全考虑，连接自动升级为 SSL 连接\n");
    }

    APP_LOG(bo, "获取邮箱信息\n");
    size_t mails = 0;
    err = pop_list(client, &mail_list, &mails);
    if (err < 0) goto done;
    printf("有 %ld 封邮件\n", mails);
    if (options->list) {
        printf("所有邮件的 ID \n");
        for (int i = (int) (mails - 1); i >= 0; i--) {
            printf("%d: %d %d bytes\n", i, mail_list[i].mid, mail_list[i].sz);
        }
        goto done;
    }

    if (options->mid >= 0) {
        GString *mail = recv_one_mail(client, options->mid, options->top);
        if (mail == NULL) {
            err = 1;
            goto done;
        }
        printf("\n邮件 %d:\n\n%s\n", options->mid, mail->str);
        g_string_free(mail, true);
        goto done;
    }

    if (options->interactive) {
        err = recv_mail_interactive(client, mail_list, mails, options->top);
    } else {
        err = recv_mail_all(client, mail_list, mails, options->top);
    }

done:
    pop_client_close(client);
    g_free(mail_list);
    if (err == 0) {
        return 0;
    }
    if (err < 0) {
        fprintf(stderr, "失败, 错误信息: %s\n", pop_err_to_str(err));
    }
    return err;
}