//
// Created by 李泽钧 on 2017/9/26.
//

#ifndef SIMPLE_MAIL_CONN_H
#define SIMPLE_MAIL_CONN_H

#include <stdbool.h>
#include <openssl/ssl.h>
#include <glib.h>

#include "error.h"

#define C_DEFAULT_RECV_BUF_SIZE 2048

// read_buf_t is read buffer used in conn_t.
struct read_buf_t {
    size_t     chunk_size;
    const char *term;
    char       *chunk;
    char       *chunk_end;
    char       *chunk_start;
    bool read_more;

    // Put buffer here to reduce memory allocation during transmission.
    GString *result_buf;
};

// conn_t represent a connection.
typedef struct conn_t {
    bool is_ssl;
    int  fd;

    SSL     *ssl_conn;
    SSL_CTX *ssl_ctx;

    struct read_buf_t buf;
} conn_t;

// conn_init_openssl initialize OpenSSL library.
// Just invoke once.
void conn_init_openssl();

// conn_connect_server will establish connection with server
// specified by addr(host:port).
int conn_connect_server(const char *addr, bool ssl, conn_t **conn);

// conn_upgrade_ssl upgrade conn to SSL connection.
// Must call conn_init_openssl before this function.
int conn_upgrade_ssl(conn_t *conn);

// conn_new_conn make a new connection, with read buffer size.
// If buf_size if 0, it will using the default buffer size.
conn_t *conn_new_conn(size_t buf_size);

// conn_close_conn close connection, and do necessary cleanup.
void conn_close_conn(conn_t *conn);

// conn_set_term set the default line terminator used in read line.
#define conn_set_term(c, t) (c)->buf.term = (t)

// conn_read_line_with_term read a line with a specified terminator.
ssize_t conn_read_line_with_term(conn_t *conn, char **line, const char *term);

// conn_read_line read a line with default terminator in conn.
ssize_t conn_read_line(conn_t *conn, char **line);

// conn_send send data via connection.
int conn_send(conn_t *conn, const void *buf, size_t len);

// conn_read read specified size of data via connection.
ssize_t conn_read(conn_t *conn, void *buf, size_t n);

#endif //SIMPLE_MAIL_CONN_H
