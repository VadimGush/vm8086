//
// Created by Vadim Gush on 02.04.2023.
//

#ifndef VM8086_HELPER_H
#define VM8086_HELPER_H

#include "utils.h"
#include "io.h"

namespace helper {

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

        static mod_reg_rm decode(u8 byte);
    };

    mod_reg_rm read_mod_reg_rm(io::input_stream&);

    i32 read_signed_data(bool word_data, io::input_stream&);

    u32 read_unsigned_data(bool word_data, io::input_stream&);

    const char* get_register_name(bool word_data, u8 reg);

    const char* get_register_pattern(u8 pattern);

    // mov ax, bx
    void print_reg_reg(bool invert, const char* dst, const char* src);

    // mov ax, [ bx + si ]
    void print_reg_memory(bool invert, const char* dst, const char* pattern);

    // mov ax, [ 12 ]
    void print_reg_direct_memory(bool invert, const char* dst, u32 address);

    // mov ax, [ bx + si + 10]
    void print_reg_memory_disp(bool invert, const char* dst, const char* pattern, i32 disp);

}

#endif //VM8086_HELPER_H
