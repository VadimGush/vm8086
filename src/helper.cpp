//
// Created by Vadim Gush on 02.04.2023.
//

#include "helper.h"
#include <iostream>
using namespace helper;

const static char *register_names_byte[] = {
        "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
};

const static char *register_names_word[] = {
        "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

const static char *register_pattern[] = {
        "bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"
};

const char* helper::get_register_name(bool word_data, u8 reg) {
    return word_data ? register_names_word[reg] : register_names_byte[reg];
}

const char* helper::get_register_pattern(u8 reg) {
    return register_pattern[reg];
}

const char* mod_reg_rm::reg_name(const bool word_data) const {
    return get_register_name(word_data, reg);
}

const char* mod_reg_rm::rm_name(const bool word_data) const {
    return get_register_name(word_data, rm);
}

const char* mod_reg_rm::rm_pattern() const {
    return get_register_pattern(rm);
}

void helper::print_reg_reg(bool invert, const char* dst, const char* src) {
    if (!invert) std::cout << dst << ", " << src << '\n';
    else std::cout << src << ", " << dst << '\n';
}

void helper::print_reg_memory(bool invert, const char* dst, const char* pattern) {
    if (!invert) std::cout << dst << ", [" << pattern << "]\n";
    else std::cout << "[" << pattern << "], " << dst << "\n";
}

void helper::print_reg_direct_memory(bool invert, const char* dst, const u32 address) {
    if (!invert) std::cout << dst << ", [" << address << "]\n";
    else std::cout << "[" << address << "], " << dst << "\n";
}

void helper::print_reg_memory_disp(bool invert, const char* dst, const char* pattern, i32 disp) {
    if (disp == 0) {
        print_reg_memory(invert, dst, pattern);
    } else if (disp > 0) {
        if (!invert) std::cout << dst << ", [" << pattern << " + " << disp << "]\n";
        else std::cout << "[" << pattern << " + " << disp << "], " << dst << "\n";
    } else {
        if (!invert) std::cout << dst << ", [" << pattern << " - " << std::abs(disp) << "]\n";
        else std::cout << "[" << pattern << " - " << std::abs(disp) << "], " << dst << "\n";
    }
}

mod_reg_rm helper::read_mod_reg_rm(io::input_stream& is) {
    return mod_reg_rm::decode(is.read_byte());
}

i32 helper::read_signed_data(bool word_data, io::input_stream& is) {
    if (word_data) {
        const u8 low = is.read_byte();
        const u8 high = is.read_byte();
        return bit::reinterpret<i16>(bit::combine(high, low));
    } else {
        return bit::reinterpret<i8>(is.read_byte());
    }
}

u32 helper::read_unsigned_data(bool word_data, io::input_stream& is) {
    if (word_data) {
        const u8 low = is.read_byte();
        const u8 high = is.read_byte();
        return bit::combine(high, low);
    } else {
        return is.read_byte();
    }
}

mod_reg_rm mod_reg_rm::decode(const u8 byte) {
    return mod_reg_rm {
        .mod = static_cast<MemoryMode>(byte >> 6),
        .reg = static_cast<u8>((byte >> 3) & bit::LOW_3BIT),
        .rm = static_cast<u8>(byte & bit::LOW_3BIT)
    };
}
