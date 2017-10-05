//
// Created by 李泽钧 on 2017/9/26.
//

#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "log.h"
#include <glib.h>

#include "conn.h"

void conn_init_openssl() {
    SSL_load_error_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    SSL_library_init();
}

static
int connect_server(const char *hostname, const char *port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype   = SOCK_STREAM;
    hints.ai_family     = PF_UNSPEC;

    struct addrinfo *res;
    int             err = getaddrinfo(hostname, port, &hints, &res);
    if (err != 0) {
        g_critical("connect to server encounter error: %s", gai_strerror(err));
        return -C_ERR_HOST_CANNOT_FIND;
    }

    int                  fd = 0;
    for (struct addrinfo *r = res; r != NULL; r = r->ai_next) {
        fd = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
        if (fd < 0) {
            g_warning("init socket error: %s", strerror(errno));
            continue;
        }

        if (connect(fd, r->ai_addr, r->ai_addrlen) < 0) {
            g_warning("connect to server encounter error: %s", strerror(errno));
            close(fd);
            fd = -1;
            continue;
        }

        break;
    }
    freeaddrinfo(res);

    if (fd < 0) {
        return -C_ERR_HOST_CANNOT_CONNECT;
    }
    return fd;
}

int conn_connect_server(const char *addr, bool ssl, conn_t **conn) {
    char **hp = g_strsplit(addr, ":", 2);
    if (hp[0] == NULL || hp[1] == NULL) return -C_ERR_HOST_BAD_ADDR;

    int fd = connect_server(hp[0], hp[1]);
    g_strfreev(hp);
    if (fd < 0) return fd;

    conn_t *c = conn_new_conn(0);
    c->is_ssl = ssl;
    c->fd     = fd;
    *conn = c;

    if (ssl) {
        int err = conn_upgrade_ssl(c);
        if (err <= 0) return err;
    }

    return 0;
}

int conn_upgrade_ssl(conn_t *conn) {
    conn->is_ssl  = true;
    conn->ssl_ctx = SSL_CTX_new(SSLv23_method());
    if (conn->ssl_ctx == NULL) {
        g_critical("new ssl ctx error: %s", ERR_error_string(ERR_get_error(), NULL));
        return -C_ERR_SSL;
    }

    conn->ssl_conn = SSL_new(conn->ssl_ctx);
    if (conn->ssl_conn == NULL) {
        g_critical("new ssl error: %s", ERR_error_string(ERR_get_error(), NULL));
        return -C_ERR_SSL;
    }

    int err = SSL_set_fd(conn->ssl_conn, conn->fd);
    if (err == 0) {
        g_critical("ssl set fd error: %s", ERR_error_string(ERR_get_error(), NULL));
        return -C_ERR_SSL;
    }

    err = SSL_connect(conn->ssl_conn);
    if (err <= 0) {
        err = SSL_get_error(conn->ssl_conn, err);
        switch (err) {
            case SSL_ERROR_ZERO_RETURN:
                g_critical("ssl connection closed");
                break;
            case SSL_ERROR_SSL:
                g_critical("ssl connect meets OpenSSL error: %s", ERR_error_string(ERR_get_error(), NULL));
                break;
            default:
                g_critical("ssl connect error");
        }
        return -C_ERR_SSL;
    }

    return 0;
}

conn_t *conn_new_conn(size_t buf_size) {
    conn_t *c = g_new0(conn_t, 1);

    c->is_ssl             = false;
    c->fd                 = -1;
    c->ssl_conn           = NULL;
    c->buf.chunk_size     = buf_size;
    if (buf_size == 0)
        c->buf.chunk_size = C_DEFAULT_RECV_BUF_SIZE;
    c->buf.chunk          = g_malloc0(c->buf.chunk_size);
    c->buf.chunk_start    = c->buf.chunk;
    c->buf.chunk_end      = c->buf.chunk_start;
    c->buf.result_buf     = g_string_new(NULL);
    c->buf.read_more      = true;
    return c;
}

void conn_close_conn(conn_t *c) {
    if (c->is_ssl) {
        SSL_CTX_free(c->ssl_ctx);
        SSL_shutdown(c->ssl_conn);
    } else {
        close(c->fd);
    }

    g_free(c->buf.chunk);
    g_string_free(c->buf.result_buf, true);
}
