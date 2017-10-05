//
// Created by 李泽钧 on 2017/9/25.
//

#include <greatest.h>

#include "smtp_parser.h"
#include "conn_op.h"

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(smtp_parser_suite);
    RUN_SUITE(conn_op_suite);
    GREATEST_MAIN_END();
}