//
// Created by Vadim Gush on 02.04.2023.
//

#ifndef VM8086_ERRORS_H
#define VM8086_ERRORS_H

namespace errors {

    enum struct DecodingError {
        NONE = 99,
        UNKNOWN_INSTRUCTION = 0,
        UNSUPPORTED_INSTRUCTION_TYPE = 1,
    };

    const static char* decoding_error_message[] = {
            "Unknown instruction",
            "This type of instruction is not supported"
    };

}


#endif //VM8086_ERRORS_H
