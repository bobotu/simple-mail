//
// Created by 李泽钧 on 2017/9/26.
//

#ifndef SIMPLE_MAIL_CONN_ERROR_H
#define SIMPLE_MAIL_CONN_ERROR_H

#define C_ERR_HOST_CANNOT_FIND     (1)
#define C_ERR_HOST_CANNOT_CONNECT  (2)
#define C_ERR_HOST_BAD_ADDR        (3)
#define C_ERR_READ_NO_TERM         (4)
#define C_ERR_READ_FAIL            (5)
#define C_ERR_SEND_FAIL            (6)
#define C_ERR_SSL                  (7)

static inline
const char *conn_err_to_str(int err_code) {
    err_code = -err_code;
    switch (err_code) {
        case C_ERR_HOST_CANNOT_FIND:
            return "Server address cannot resolve, please see log for more information.";
        case C_ERR_HOST_CANNOT_CONNECT:
            return "Server cannot connect.";
        case C_ERR_HOST_BAD_ADDR:
            return "Server address have wrong format.";
        case C_ERR_READ_NO_TERM:
            return "No line terminator while read line.";
        case C_ERR_READ_FAIL:
        case C_ERR_SEND_FAIL:
            return "Network error while communicate with remote server.";
        case C_ERR_SSL:
            return "Error in OpenSSL, please see log for more information.";
        default:
            return "Unknown error.";
    }
}

#endif //SIMPLE_MAIL_CONN_ERROR_H
