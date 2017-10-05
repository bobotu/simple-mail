//
// Created by 李泽钧 on 2017/9/25.
//

#include <string.h>
#include <stdbool.h>
#include <glib.h>

#include "parser.h"
#include "smtp.h"

static
const char *ctx_readn(smtp_parse_ctx *ctx, size_t n) {
    const char *b = ctx->buf;
    ctx->buf += n;
    ctx->len -= n;
    return b;
}

static
smtp_parse_ctx smtp_new_parse_ctx_crlf(char *buf) {
    smtp_parse_ctx result = {buf, 0};

    for (size_t i = 0; !(buf[i] == '\r' && buf[i + 1] == '\n'); i++)
        result.len++;

    buf[result.len] = '\0';
    return result;
}

static
smtp_parse_ctx smtp_new_parse_ctx(char *buf) {
    smtp_parse_ctx result = {buf, 0};
    result.len = strlen(buf);
    return result;
}

static
int parse_code(smtp_parse_ctx *ctx) {
    int ret = S_ERR_BAD_RESPONSE;
    if (ctx->len < 3) return ret;
    const char *str = ctx_readn(ctx, 3);

    for (int i = 0; i < 3; i++) {
        if (!g_ascii_isdigit(str[i])) return -S_ERR_BAD_RESPONSE;
        ret = ret * 10 + str[i] - '0';
    }

    return ret;
}

static
smtp_reply_greeting *parse_payload_greeting(smtp_parse_ctx *ctx, bool first) {
    if (ctx->len <= 0) return NULL;

    smtp_reply_greeting *r = g_new0(smtp_reply_greeting, 1);

    if (first) {
        char **buf = g_strsplit(ctx->buf, " ", 2);
        r->str_buf = buf;
        if (buf[0] == NULL) return r;
        r->domain = buf[0];
        r->text   = buf[1];
    } else {
        r->text    = g_strdup(ctx->buf);
        r->str_buf = g_new0(char *, 2);
        r->str_buf[0] = r->text;
    }
    return r;
}

static
smtp_reply_ehlo *parse_payload_ehlo(smtp_parse_ctx *ctx, bool first) {
    if (ctx->len <= 0) return NULL;

    smtp_reply_ehlo *r = g_new0(smtp_reply_ehlo, 1);

    if (first) {
        r->str_buf = g_strsplit(ctx->buf, " ", 2);
        if (r->str_buf[0] == NULL) return r;
        r->domain   = r->str_buf[0];
        r->greeting = r->str_buf[1];
    } else {
        r->str_buf = g_strsplit(ctx->buf, " ", -1);
        if (r->str_buf[0] == NULL) return r;
        r->keyword = r->str_buf[0];
        r->args    = r->str_buf + 1;
    }
    return r;
}

static
void *parse_payload(smtp_parse_ctx *ctx, int reply_code, bool first) {
    switch (reply_code) {
        case S_RESP_GREETING:
            return parse_payload_greeting(ctx, first);
        case S_RESP_EHLO:
            return parse_payload_ehlo(ctx, first);
        default:
            return NULL;
    }
}

int smtp_parse_line_ctx(smtp_parse_ctx *ctx, smtp_reply_t **reply, bool first) {
    smtp_reply_t *r = g_new0(smtp_reply_t, 1);
    *reply = r;

    if ((r->code = parse_code(ctx)) < 0) return r->code;

    int ret = 1;
    if (ctx->len < 1) return ret;
    if (ctx_readn(ctx, 1)[0] == '-') ret = 0;

    r->payload = parse_payload(ctx, r->code, first);
    return ret;
}

int smtp_parse_line(char *buf, smtp_reply_t **reply, bool first) {
    smtp_parse_ctx ctx = smtp_new_parse_ctx(buf);
    return smtp_parse_line_ctx(&ctx, reply, first);
}

int smtp_parse_line_crlf(char *buf, smtp_reply_t **reply, bool first) {
    smtp_parse_ctx ctx = smtp_new_parse_ctx_crlf(buf);
    return smtp_parse_line_ctx(&ctx, reply, first);
}

static
void d_greeting(smtp_reply_greeting *g) {
    if (g != NULL) {
        g_strfreev(g->str_buf);
    }
}

static
void d_ehlo(smtp_reply_ehlo *g) {
    if (g != NULL) {
        g_strfreev(g->str_buf);
    }
}

void smtp_destroy_reply(smtp_reply_t *reply) {
    switch (reply->code) {
        case S_RESP_GREETING:
            d_greeting(reply->payload);
            break;
        case S_RESP_EHLO:
            d_ehlo(reply->payload);
            break;
        default:;
    }
    g_free(reply);
}
