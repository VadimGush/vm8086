//
// Created by Vadim Gush on 02.04.2023.
//

#ifndef VM8086_DECODER_H
#define VM8086_DECODER_H

#include "utils.h"
#include "io.h"

namespace decoder {

    enum struct DecodingError {
        NONE = 99,
        UNKNOWN_INSTRUCTION = 0,
        UNSUPPORTED_INSTRUCTION_TYPE = 1,
    };

    const static char* decoding_error_message[] = {
            "Unknown helper",
            "This type of helper is not supported"
    };

    DecodingError decode(io::input_stream &is);

}

#endif //VM8086_DECODER_H
