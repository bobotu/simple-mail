//
// Created by 李泽钧 on 2017/10/6.
//

#include <string.h>

#include <glib.h>
#include <gmime/gmime.h>
#include <smtp.h>

#include "common.h"

static
char *mail_to_mime_str(struct app_send_options *opts, const char *text) {
    GMimeMessage *msg = g_mime_message_new(true);
    g_mime_object_set_header((GMimeObject *) msg, "From", opts->base->username, NULL);
    g_mime_object_set_header((GMimeObject *) msg, "To", opts->rcpts, NULL);
    g_mime_message_set_date(msg, g_date_time_new_now_local());
    g_mime_message_set_subject(msg, opts->subject, NULL);

    GMimeTextPart *part = g_mime_text_part_new_with_subtype(opts->type);
    g_mime_text_part_set_text(part, text);
    g_mime_message_set_mime_part(msg, (GMimeObject *) part);
    g_object_unref(part);

    return g_mime_object_to_string((GMimeObject *) msg, NULL);
}

int app_send_mail(struct app_send_options *options) {
    int                     err   = 0;
    struct app_base_options *bo   = options->base;
    smtp_client_t           *client;
    GString                 *text = g_string_new(NULL);

    char buf[1024] = {0};
    while (true) {
        char *result = fgets(buf, 1024, stdin);
        if (result == NULL) {
            if (feof(stdin)) {
                g_string_append_c(text, '\0');
                break;
            } else {
                fputs("读取输入错误\n", stderr);
                g_string_free(text, true);
                return -1;
            }
        }
        g_string_append(text, buf);
    }

    APP_LOG(bo, "开始连接服务器 %s (%s) \n",
            bo->host,
            bo->use_ssl ? "使用 TLS 连接" : "不使用 TLS");
    err = smtp_connect(bo->host, bo->use_ssl, &client);
    if (err < 0) goto error;

    APP_LOG(bo, "已连接, 开始 SMTP EHLO\n");
    err = smtp_ehlo(client, bo->username);
    if (err < 0) goto error;

    APP_LOG(bo, "以 %s (%s) 登录\n", bo->username, bo->password);
    err = smtp_auth(client, bo->username, bo->password);
    if (err < 0) goto error;
    if (client->conn->is_ssl && !bo->use_ssl) {
        APP_LOG(bo, "出于安全考虑，连接自动升级为 SSL 连接\n");
    }

    APP_LOG(bo, "发送 SMTP MAIL FROM 指令\n");
    err = smtp_mail(client, bo->username);
    if (err < 0) goto error;

    APP_LOG(bo, "发送 SMTP RCPT TO %s \n", options->rcpts);
    char **rcpts_v = g_strsplit(options->rcpts, ";", -1);
    err = smtp_rcpt(client, rcpts_v);
    g_strfreev(rcpts_v);
    if (err < 0) {
        fprintf(stderr, "%d rcpt failed", -err);
        goto error;
    }

    char *payload = mail_to_mime_str(options, text->str);
    APP_LOG(bo, "MIME 文本:\n%s\n开始发送邮件\n", payload);
    err = smtp_data(client, payload, strlen(payload));
    g_free(payload);
    if (err < 0) goto error;

error:
    smtp_client_close(client);
    g_string_free(text, true);
    if (err == 0) {
        APP_LOG(bo, "发送成功\n");
        return 0;
    }
    fprintf(stderr, "发送失败, 错误信息: %s\n", smtp_err_to_str(err));
    return -1;
}