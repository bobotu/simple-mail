//
// Created by 李泽钧 on 2017/9/25.
//

#ifndef SIMPLE_MAIL_SMTP_ERROR_H
#define SIMPLE_MAIL_SMTP_ERROR_H

// error code.
#define S_ERR_BAD_RESPONSE       (1)
#define S_ERR_REQ_SYNTAX         (2)
#define S_ERR_SEND_COMMAND       (3)
#define S_ERR_NO_SUPPORTED_AUTH  (4)
#define S_ERR_CONNECTION         (5)

static inline
const char *smtp_err_to_str(int err_code) {
    err_code = -err_code;
    switch (err_code) {
        case S_ERR_BAD_RESPONSE:
            return "SMTP response have wrong format.";
        case S_ERR_REQ_SYNTAX:
            return "SMTP request command have syntax error.";
        case S_ERR_SEND_COMMAND:
            return "Network error when sending command.";
        case S_ERR_NO_SUPPORTED_AUTH:
            return "This client doesn't support auth method used by server.";
        case S_ERR_CONNECTION:
            return "Underlay networking error, please see log for more information.";
            // TODO: SMTP protocol error codes.
        default:
            return "Unknown error.";
    }
}

#endif //SIMPLE_MAIL_SMTP_ERROR_H
