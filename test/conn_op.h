//
// Created by 李泽钧 on 2017/9/26.
//

#ifndef TEST_SIMPLE_MAIL_CONN_OP_H
#define TEST_SIMPLE_MAIL_CONN_OP_H

#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <conn.h>
#include <greatest.h>

struct write_data_t {
    const char *data;
    useconds_t delay;
};

struct send_thread_arg_t {
    GAsyncQueue *ch;
    int         pipe;
};

static void *send_thread(void *arg) {
    struct send_thread_arg_t *args = arg;
    GAsyncQueue              *ch   = args->ch;
    int                      fd    = args->pipe;

    while (1) {
        struct write_data_t *w = g_async_queue_pop(ch);
        if (w->data == NULL) break;

        if (w->delay != 0) usleep(w->delay);
        write(fd, w->data, strlen(w->data));
    }


    return NULL;
}

TEST c_should_read_line() {
    int pipes[2];
    ASSERT_EQ(0, pipe(pipes));
    GAsyncQueue         *ch = g_async_queue_new();
    struct write_data_t w   = {NULL, 0};
    char                *result;

    struct send_thread_arg_t arg;
    arg.pipe = pipes[1];
    arg.ch   = ch;
    GThread *sender = g_thread_new("sender", send_thread, &arg);

    conn_t *conn = conn_new_conn(3);
    conn->fd = pipes[0];
    conn_set_term(conn, "\r\n");

    w.data = "250-smtp.qq.com\r\n";
    g_async_queue_push(ch, &w);
    conn_read_line(conn, &result);
    ASSERT_STR_EQ("250-smtp.qq.com", result);
    g_free(result);

    w.data = "250-SIZE 73400320\r\n250-AUTH";
    g_async_queue_push(ch, &w);
    conn_read_line(conn, &result);
    ASSERT_STR_EQ("250-SIZE 73400320", result);
    g_free(result);

    w.data  = " LOGIN";
    w.delay = 500;
    g_async_queue_push(ch, &w);
    struct write_data_t w1 = {" PLAIN\r\n", 0};
    g_async_queue_push(ch, &w1);
    conn_read_line(conn, &result);
    ASSERT_STR_EQ("250-AUTH LOGIN PLAIN", result);
    g_free(result);

    w.data = NULL;
    g_async_queue_push(ch, &w);

    g_thread_join(sender);
    conn_close_conn(conn);
    g_async_queue_unref(ch);
    g_thread_unref(sender);

    PASS();
}

SUITE (conn_op_suite) {
    RUN_TEST(c_should_read_line);
}

#endif //TEST_SIMPLE_MAIL_CONN_OP_H
