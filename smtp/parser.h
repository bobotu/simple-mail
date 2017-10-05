//
// Created by 李泽钧 on 2017/9/25.
//

#ifndef SIMPLE_MAIL_SMTP_PARSER_H
#define SIMPLE_MAIL_SMTP_PARSER_H

#include <stdbool.h>

typedef struct smtp_parse_ctx {
    const char *buf;
    size_t     len;
} smtp_parse_ctx;

// the SMTP reply codes.
#define S_RESP_GREETING 220
#define S_RESP_EHLO     250

typedef struct smtp_reply_t {
    int  code;
    void *payload;
} smtp_reply_t;

// the SMTP replies
typedef struct smtp_reply_greeting {
    char *domain;
    char *text;
    char **str_buf;
} smtp_reply_greeting;

typedef struct smtp_reply_ehlo {
    char *domain;
    char *greeting;
    char *keyword;
    char **args;
    char **str_buf;
} smtp_reply_ehlo;

void smtp_destroy_reply(smtp_reply_t *reply);

int smtp_parse_line_ctx(smtp_parse_ctx *ctx, smtp_reply_t **reply, bool first);

// smtp_parse_line try to parse a line (terminated by null) to smtp_reply.
// return: < 0 means error, 0 means continue, > 0 means done.
int smtp_parse_line(char *buf, smtp_reply_t **reply, bool first);

// smtp_parse_line try to parse a line (terminated by \r\n) to smtp_reply.
// return: < 0 means error, 0 means continue, > 0 means done.
int smtp_parse_line_crlf(char *buf, smtp_reply_t **reply, bool first);

#endif //SIMPLE_MAIL_SMTP_PARSER_H
