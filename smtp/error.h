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
        case 252:
            return "Cannot VRFY user.";
        case 421:
            return "Service not available, closing transmission channel.";
        case 450:
            return "Requested mail action not taken: mailbox unavailable.";
        case 451:
            return "Requested action aborted: local error in processing.";
        case 452:
            return "Requested action not taken: insufficient system storage.";
        case 500:
            return "Syntax error, command unrecognised.";
        case 501:
            return "Syntax error in parameters or arguments.";
        case 502:
            return "Command not implemented.";
        case 503:
            return "Bad sequence of commands.";
        case 504:
            return "Command parameter not implemented.";
        case 521:
            return "Does not accept mail (see rfc1846).";
        case 530:
            return "Access denied.";
        case 550:
            return "Requested action not taken: mailbox unavailable.";
        case 551:
            return "User not local; please try <forward-path>.";
        case 552:
            return "Requested mail action aborted: exceeded storage allocation.";
        case 553:
            return "Requested action not taken: mailbox name not allowed.";
        case 554:
            return "Transaction failed.";
        default:
            return "Unknown error.";
    }
}

#endif //SIMPLE_MAIL_SMTP_ERROR_H
