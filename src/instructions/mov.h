//
// Created by Vadim Gush on 02.04.2023.
//

#ifndef VM8086_MOV_H
#define VM8086_MOV_H

#include "../utils.h"
#include "../errors.h"
#include "../io.h"

namespace mov {

    enum class MemoryMode : u8 {
        MEMORY_MODE = 0b00000000,
        MEMORY_MODE_8_BIT = 0b00000001,
        MEMORY_MODE_16_BIT = 0b00000010,
        REGISTER_MODE = 0b00000011,
    };

    const static char *register_names_byte[] = {
            "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
    };

    const static char *register_names_word[] = {
            "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
    };

    const static char *register_pattern[] = {
            "bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"
    };

    errors::DecodingError decode(io::input_stream& is);

}

#endif //VM8086_MOV_H
