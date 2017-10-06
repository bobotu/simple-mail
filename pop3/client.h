//
// Created by 李泽钧 on 2017/9/28.
//

#ifndef SIMPLE_MAIL_POP3_CLIENT_H
#define SIMPLE_MAIL_POP3_CLIENT_H

#include <stdlib.h>
#include <stdbool.h>

#include <conn.h>

// POP3 commands.
#define P_CAPA "CAPA"
#define P_STLS "STLS"
#define P_USER "USER"
#define P_PASS "PASS"
#define P_STAT "STAT"
#define P_LIST "LIST"
#define P_TOP  "TOP"
#define P_RETR "RETR"
#define P_DELE "DELE"
#define P_QUIT "QUIT"

// pop_list_t contains mail id and size pair.
typedef struct pop_list_t {
    int mid, sz;
} pop_list_t;

// pop_client_t represent a pop3 client.
typedef struct pop_client_t {
    conn_t *conn;

    pop_list_t *mails;
    size_t     mail_num;
} pop_client_t;

// pop_connect connect to pop mail server.
int pop_connect(const char *host, bool ssl, pop_client_t **client);

void pop_client_close(pop_client_t *client);

// pop_use_ssl upgrade connection to SSL if it not in SSL mode.
int pop_use_ssl(pop_client_t *client);

// pop_auth do POP3 authentication.
int pop_auth(pop_client_t *client, const char *username, const char *passwd);

// pop_status_t contains number of mails and total size pair.
typedef struct pop_status_t {
    int num, sz;
} pop_status_t;

// pop_status get total number of mails and their size.
int pop_status(pop_client_t *client, pop_status_t *stat);

// pop_list get all mails id and size.
int pop_list(pop_client_t *client, pop_list_t **list, size_t *len);

// pop_top get the header and first `sz` byte of `mid`.
int pop_top(pop_client_t *client, int mid, size_t sz, char **data, size_t *data_len);

// pop_retr get the full text of `mid`.
int pop_retr(pop_client_t *client, int mid, char **data, size_t *data_len);

// pop_del delete `mid`.
int pop_del(pop_client_t *client, int mid);

// pop_quit end the POP3 transaction.
int pop_quit(pop_client_t *client);

#endif //SIMPLE_MAIL_POP3_CLIENT_H
