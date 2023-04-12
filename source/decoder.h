//
// Created by Vadim Gush on 02.04.2023.
//

#ifndef VM8086_DECODER_H
#define VM8086_DECODER_H

#include <utils/types.h>
#include "io.h"

namespace decoder {

    enum class MemoryMode : u8 {
        MEMORY_MODE = 0b00000000,
        MEMORY_MODE_8_BIT = 0b00000001,
        MEMORY_MODE_16_BIT = 0b00000010,
        REGISTER_MODE = 0b00000011,
    };

    struct mod_reg_rm {
        const MemoryMode mod;
        const u8 reg;
        const u8 rm;

        const char* reg_name(bool word_data) const;

        const char* rm_name(bool word_data) const;

        const char* rm_pattern() const;
    };

    enum struct DecodingError {
        NONE = 99,
        UNKNOWN_INSTRUCTION = 0,
        UNSUPPORTED_INSTRUCTION_TYPE = 1,
    };

    const static char* decoding_error_message[] = {
            "Unknown instruction",
            "This type of instruction is not supported"
    };

    DecodingError decode(io::input_stream &is);

}

#endif //VM8086_DECODER_H
