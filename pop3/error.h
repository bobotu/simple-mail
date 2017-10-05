//
// Created by 李泽钧 on 2017/10/5.
//

#ifndef SIMPLE_MAIL_POP3_ERROR_H
#define SIMPLE_MAIL_POP3_ERROR_H

#define P_ERR_CONNECTION    (1)
#define P_ERR_BAD_RESPONSE  (2)

static inline
const char *pop_err_to_str(int err_code) {
    err_code = -err_code;
    switch (err_code) {
        case P_ERR_CONNECTION:
            return "Underlay networking error. please see log for more information.";
        case P_ERR_BAD_RESPONSE:
            return "Server return a bad response.";
        default:
            return "Unknown error.";

    }
}

#endif //SIMPLE_MAIL_POP3_ERROR_H
