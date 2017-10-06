//
// Created by 李泽钧 on 2017/9/26.
//

#ifndef SIMPLE_MAIL_SMTP_CLIENT_H
#define SIMPLE_MAIL_SMTP_CLIENT_H

#include <glib.h>
#include <conn.h>

// the SMTP commands.
#define S_EHLO "EHLO"
#define S_HELP "HELO"
#define S_MAIL "MAIL"
#define S_RCPT "RCPT"
#define S_DATA "DATA"
#define S_RSET "RSET"
#define S_VRFY "VRFY"
#define S_EXPN "EXPN"
#define S_NOOP "NOOP"
#define S_QUIT "QUIT"
#define S_AUTH "AUTH"
#define S_STARTTLS "STARTTLS"

// smtp_init_resp_t represent the SMTP server greeting response.
typedef struct smtp_init_resp_t {
    char         *domain;
    char         **texts;
    GStringChunk *buf;
} smtp_init_resp_t;

// smtp_init_resp_clear clear init response.
void smtp_init_resp_clear(smtp_init_resp_t *resp);

enum smtp_auth_method {
    unknown, login, plain
};

typedef struct smtp_ehlo_resp_t {
    char *domain;
    char *greeting;

    int size;
    bool starttls;

    enum smtp_auth_method auth;
    struct {
        bool login;
        bool plain;
    }                     auth_methods;

    // Missing many SMTP extension support.
} smtp_ehlo_resp_t;

// smtp_ehlo_resp_clear clear ehlo response.
void smtp_ehlo_resp_clear(smtp_ehlo_resp_t *resp);

// smtp_client_t represent SMTP protocol client.
typedef struct smtp_client_t {
    conn_t *conn;

    smtp_init_resp_t init_resp;
    smtp_ehlo_resp_t ehlo_resp;
} smtp_client_t;

// smtp_connect connect to SMTP server.
int smtp_connect(const char *host, bool ssl, smtp_client_t **client);

// smtp_client_close close SMTP client, and do necessary cleanup.
void smtp_client_close(smtp_client_t *client);

// smtp_ehlo send EHLO command to server.
int smtp_ehlo(smtp_client_t *client, const char *msg);

// smtp_use_ssl upgrade connection to SSL if it not in SSL mode.
int smtp_use_ssl(smtp_client_t *client);

// smtp_auth do SMTP authentication.
int smtp_auth(smtp_client_t *client, const char *username, const char *passwd);

// smtp_mail send MAIL FROM command to server.
int smtp_mail(smtp_client_t *client, const char *from);

// smtp_rcpt send RCPT TO command to server.
// rcpt is NULL terminated array of string.
int smtp_rcpt(smtp_client_t *client, char **rcpt);

// smtp_data send data as mail payload to server.
int smtp_data(smtp_client_t *client, const void *data, size_t len);

#endif //SIMPLE_MAIL_SMTP_CLIENT_H
