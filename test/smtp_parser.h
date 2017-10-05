//
// Created by 李泽钧 on 2017/9/25.
//

#ifndef TEST_SIMPLE_MAIL_SMTP_PARSER_H
#define TEST_SIMPLE_MAIL_SMTP_PARSER_H

#include <smtp.h>
#include <glib.h>
#include <greatest.h>

#define ASSERT_STR_EQn(s1, s2) do {         \
    if ((s1) == NULL) ASSERT_EQ(NULL, s2);  \
    else ASSERT(g_str_equal(s1, s2));       \
} while(0)

static greatest_test_res s_check_greeting(smtp_reply_t *reply,
                                          const char *domain,
                                          const char *text,
                                          bool no_payload) {
    ASSERT(reply != NULL);
    smtp_reply_greeting *greeting = reply->payload;
    ASSERT_EQ(S_RESP_GREETING, reply->code);
    if (no_payload) {
        ASSERT_EQ(NULL, reply->payload);
    } else {
        ASSERT(reply->payload != NULL);
        ASSERT_STR_EQn(domain, greeting->domain);
        ASSERT_STR_EQn(text, greeting->text);
    }

    PASS();
}

TEST s_should_parse_greeting() {
#define CASE(s, f, c, a1, a2, a3) do {                  \
    smtp_reply_t *reply = NULL;                         \
    char *str = g_strdup(s);                            \
    int cont = smtp_parse_line(str, &reply, f);         \
    if (c) ASSERT_EQ(0, cont); else ASSERT(cont > 0);   \
    CHECK_CALL(s_check_greeting(reply, a1, a2, a3));    \
    g_free(str);                                        \
    smtp_destroy_reply(reply);                          \
} while (0)

    CASE("220 smtp.qq.com Esmtp QQ Mail Server", true, false,
         "smtp.qq.com", "Esmtp QQ Mail Server", false);

    CASE("220 smtp.qq.com", true, false, "smtp.qq.com", NULL, false);

    CASE("220-smtp.qq.com Esmtp QQ Mail Server", true, true,
         "smtp.qq.com", "Esmtp QQ Mail Server", false);

    CASE("220-hello test", false, true, NULL, "hello test", false);

    CASE("220", false, false, NULL, NULL, true);

    PASS();

#undef CASE
}

static greatest_test_res s_check_ehlo(smtp_reply_t *reply,
                                      const char *s1,
                                      const char *s2,
                                      bool is_ehlo_line) {
    ASSERT(reply != NULL);
    smtp_reply_ehlo *ehlo = reply->payload;
    ASSERT_EQ(S_RESP_EHLO, reply->code);
    ASSERT(reply->payload != NULL);
    if (is_ehlo_line) {
        ASSERT_STR_EQn(s1, ehlo->keyword);
        char *joined = g_strjoinv(" ", ehlo->args);
        ASSERT_STR_EQn(s2, joined);
        g_free(joined);
    } else {
        ASSERT_STR_EQn(s1, ehlo->domain);
        ASSERT_STR_EQn(s2, ehlo->greeting);
    }

    PASS();
}

TEST s_should_parse_ehlo() {
#define CASE(s, f, c, a1, a2, a3) do {                  \
    smtp_reply_t *reply = NULL;                         \
    char *str = g_strdup(s);                            \
    int cont = smtp_parse_line(str, &reply, f);         \
    if (c) ASSERT_EQ(0, cont); else ASSERT(cont > 0);   \
    CHECK_CALL(s_check_ehlo(reply, a1, a2, a3));        \
    g_free(str);                                        \
    smtp_destroy_reply(reply);                          \
} while (0)

    CASE("250 smtp.qq.com test", true, false, "smtp.qq.com", "test", false);

    CASE("250-smtp.qq.com", true, true, "smtp.qq.com", NULL, false);

    CASE("250-PIPELINING", false, true, "PIPELINING", "", true);

    CASE("250-SIZE 73400320", false, true, "SIZE", "73400320", true);

    CASE("250-AUTH LOGIN PLAIN", false, true, "AUTH", "LOGIN PLAIN", true);

    CASE("250 8BITMIME", false, false, "8BITMIME", "", true);

    PASS();

#undef CASE
}

SUITE (smtp_parser_suite) {
    RUN_TEST(s_should_parse_greeting);
    RUN_TEST(s_should_parse_ehlo);
}

#endif //TEST_SIMPLE_MAIL_SMTP_PARSER_H
