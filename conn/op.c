//
// Created by 李泽钧 on 2017/9/26.
//

#include <unistd.h>

#include "log.h"
#include <glib.h>
#include <errno.h>
#include <sys/socket.h>

#include "conn.h"

ssize_t conn_read_line(conn_t *conn, char **line) {
    return conn_read_line_with_term(conn, line, conn->buf.term);
}

ssize_t conn_read_line_with_term(conn_t *conn, char **line, const char *term) {
    struct read_buf_t *buf = &conn->buf;
    if (term == NULL) return -C_ERR_READ_NO_TERM;

    if (buf->chunk_start >= buf->chunk_end) {
        buf->chunk_start = buf->chunk;
        buf->read_more   = true;
    }

    size_t term_len = strlen(term);

    while (true) {
        if (!buf->read_more) {
            char *end = g_strstr_len(buf->chunk_start,
                                     buf->chunk_end - buf->chunk_start,
                                     term);
            if (end != NULL) {
                g_string_append_len(buf->result_buf,
                                    buf->chunk_start,
                                    (end - buf->chunk_start));
                g_string_append_c(buf->result_buf, '\0');
                buf->chunk_start = end + term_len;
                break;
            } else {
                size_t remain = buf->chunk_end - buf->chunk_start;
                if (remain > term_len) {
                    g_string_append_len(buf->result_buf,
                                        buf->chunk_start,
                                        (remain - term_len));
                    remain = term_len;
                }

                memcpy(buf->chunk, buf->chunk_end - remain, remain);
                buf->chunk_start = buf->chunk + remain;

                buf->read_more = true;
            }
        } else {
            size_t  remain = buf->chunk_start - buf->chunk;
            ssize_t sz     = conn_read(conn, buf->chunk_start, buf->chunk_size - remain);
            if (sz < 0) {
                g_critical("read data encounter error: %s", strerror(errno));
                return -C_ERR_READ_FAIL;
            }
            buf->chunk_end   = buf->chunk_start + sz;
            buf->chunk_start = buf->chunk_start - remain;
            buf->read_more   = false;
        }
    }

    size_t sz = buf->result_buf->len;
    *line = g_new(char, sz);
    memcpy(*line, buf->result_buf->str, sz);
    g_string_truncate(buf->result_buf, 0);
    return sz;
}

int conn_send(conn_t *conn, const void *buf, size_t len) {
    size_t  cnt = 0;
    ssize_t n   = 0;
    while (cnt < len) {
        if (conn->is_ssl) {
            n = SSL_write(conn->ssl_conn, buf + cnt, (int) (len - cnt));
            if (n <= 0) {
                n = SSL_get_error(conn->ssl_conn, (int) n);
                if (n == SSL_ERROR_WANT_READ || n == SSL_ERROR_WANT_WRITE)
                    continue;
                else return -C_ERR_SEND_FAIL;
            }
        } else {
            n = send(conn->fd, buf + cnt, len - cnt, 0);
            if (n < 0) {
                g_critical("send command encounter error: %s", strerror(errno));
                return -C_ERR_SEND_FAIL;
            }
        }
        cnt += n;
    }
    return 0;
}

ssize_t conn_read(conn_t *conn, void *buf, size_t n) {
    while (true) {
        if (conn->is_ssl) {
            int ret = SSL_read(conn->ssl_conn, buf, (int) n);
            if (ret <= 0) {
                ret = SSL_get_error(conn->ssl_conn, ret);
                if (ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE)
                    continue;
            }
            return ret;
        } else {
            return read(conn->fd, buf, n);
        }
    }
}