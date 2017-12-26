//
// Created by 李泽钧 on 2017/10/5.
//

#include <argparse.h>
#include <glib.h>
#include <conn.h>
#include <gmime/gmime.h>
#include "common.h"


static const char *const usage[] = {
"simple-mail send [options]",
"simple-mail recv [options]",
NULL,
};

#define DESCRIPTION_STR "\nsimple-mail 有 发送(SMTP) 和 接收(POP3) 邮件的功能。\n\n"  \
                        "如果需要发送邮件，请按照下述方法指定邮件服务和邮件发送的信息，"   \
                        "simple-mail 将从 stdin 读取邮件的正文文本。\n\n"             \
                        "如果需要接收邮件"                                           \
                        "\n\n以下是配置选项的说明:"
#define EPILOG_STR      ""

static
void init() {
    conn_init_openssl();
    g_mime_init();
}

int main(int argc, const char **argv) {
    init();

    struct app_base_options base_opts = {NULL, NULL, NULL, false, false};
    struct app_send_options send_opts = {&base_opts, NULL, NULL, "plain"};
    struct app_recv_options recv_opts = {&base_opts, false, false, true, -1, -1};

    struct argparse_option options[] = {
    OPT_HELP(),
    OPT_GROUP("邮件服务商信息:"),
    OPT_STRING('u', "username", &base_opts.username, "邮件服务的登录用户名"),
    OPT_STRING('p', "password", &base_opts.password, "邮件服务的登录密码"),
    OPT_STRING('H', "host", &base_opts.host, "邮件服务器，格式为 addr:port"),
    OPT_BOOLEAN('S', "ssl", &base_opts.use_ssl, "邮件服务是否默认使用 TLS"),
    OPT_BOOLEAN('v', "verbose", &base_opts.verbose, "是否显示详细过程日志。(错误信息一定会显示)"),
    OPT_GROUP("发送配置:"),
    OPT_STRING('t', "to", &send_opts.rcpts, "邮件接收地址，多个邮箱使用 ; 分隔"),
    OPT_STRING('s', "subject", &send_opts.subject, "邮件主题"),
    OPT_STRING('x', "mime-type", &send_opts.type, "正文的 MIME text 类型，默认为 plain"),
    OPT_GROUP("接收配置:"),
    OPT_BOOLEAN('l', "list", &recv_opts.list, "仅显示邮箱中所有邮件的 id"),
    OPT_BOOLEAN('a', "all", &recv_opts.all, "将所有的邮件文本输出至 stdin"),
    OPT_BOOLEAN('i', "interactive", &recv_opts.interactive, "交互式读取邮件"),
    OPT_INTEGER('T', "top", &recv_opts.top, "只显示正文的前 top 部分"),
    OPT_INTEGER('m', "id", &recv_opts.mid, "只显示 mid 的内容"),
    OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse, DESCRIPTION_STR, EPILOG_STR);
    argc = argparse_parse(&argparse, argc, argv);

    if (argc < 1 || !app_base_options_check(&base_opts)) {
        argparse_usage(&argparse);
        return 1;
    }

    if (g_str_equal(argv[0], "send")) {
        if (!app_send_options_check(&send_opts)) {
            fputs("发送命令缺少必须参数，请参看帮助文档\n", stderr);
            argparse_usage(&argparse);
            return 1;
        }
        return app_send_mail(&send_opts);
    } else if (g_str_equal(argv[0], "recv")) {
        if (!app_recv_options_check(&recv_opts)) {
            fputs("发送命令缺少必须参数或参数冲突，请参看帮助文档\n", stderr);
            argparse_usage(&argparse);
            return 1;
        }
        return app_recv_mail(&recv_opts);
    } else {
        fputs("请指定 send 或 recv 指令\n", stderr);
        return 1;
    }
}