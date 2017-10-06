//
// Created by 李泽钧 on 2017/10/6.
//

#ifndef SIMPLE_MAIL_CLIENT_COMMON_H
#define SIMPLE_MAIL_CLIENT_COMMON_H

#include <stdbool.h>
#include <stdio.h>

struct app_base_options {
    const char *username;
    const char *password;
    const char *host;
    bool use_ssl;
    bool verbose;
};

static inline
bool app_base_options_check(struct app_base_options *options) {
    return !(options->username == NULL ||
             options->password == NULL ||
             options->host == NULL);
}

struct app_send_options {
    struct app_base_options *base;

    const char *rcpts;
    const char *subject;
    const char *type;
};

static inline
bool app_send_options_check(struct app_send_options *options) {
    return !(options->subject == NULL || options->rcpts == NULL);
}

struct app_recv_options {
    struct app_base_options *base;

    bool list;
    bool all;
    bool interactive;

    int top;
    int mid;
};

static inline
bool app_recv_options_check(struct app_recv_options *options) {
    if (options->all || options->list || options->mid >= 0) {
        options->interactive = false;
    }
    if (options->all && options->list) return false;
    if (options->top == 0) return false;
    return true;
}

#define APP_LOG(__opts__, __fmt__, ...) do {                         \
    if (__opts__->verbose) fprintf(stdout, __fmt__, ##__VA_ARGS__);  \
} while (0)

int app_send_mail(struct app_send_options *options);

int app_recv_mail(struct app_recv_options *options);

#endif //SIMPLE_MAIL_CLIENT_COMMON_H
