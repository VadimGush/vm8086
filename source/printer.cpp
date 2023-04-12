//
// Created by Vadim Gush on 02.04.2023.
//

#include "printer.h"
#include <iostream>
using namespace printer;

const static char *register_names_byte[] = {
        "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
};

const static char *register_names_word[] = {
        "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

const static char *register_pattern[] = {
        "bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"
};

const char* printer::get_register_name(bool word_data, u8 reg) {
    return word_data ? register_names_word[reg] : register_names_byte[reg];
}

const char* printer::get_register_pattern(u8 reg) {
    return register_pattern[reg];
}

// <instr> <arg1: str>, <arg2: str>
void printer::print_instr_str_str(bool invert, const char* instr, const char* arg1, const char* arg2) {
    std::cout << instr << " ";
    if (!invert) std::cout << arg1 << ", " << arg2;
    else std::cout << arg2 << ", " << arg1;
    std::cout << "\n";
}

// [<arg1: str> + <addr: int>]
void print_arg_str_int(const char* arg1, const i32 addr) {
    if (addr == 0) {
        std::cout << "[";
        if (arg1) std::cout << arg1;
        std::cout << "]";
    }
    else if (addr > 0) {
        std::cout << "[";
        if (arg1) std::cout << arg1 << " + ";
        std::cout << addr << "]";
    }
    else {
        std::cout << "[";
        if (arg1) std::cout << arg1 << " - ";
        std::cout << std::abs(addr) << "]";
    }
}

// <instr> [<arg1: str> + <addr: int>], <arg2: str>
void printer::print_instr_str_int_str(const char* instr, const char* arg1, const i32 addr, const char* arg2) {
    std::cout << instr << " ";
    print_arg_str_int(arg1, addr);
    std::cout << ", " << arg2 << "\n";
}

// <instr> <arg1: str>, [<arg2: str> + <addr: int>]
void printer::print_instr_str_str_int(const char* instr, const char* arg1, const char* arg2, const i32 addr) {
    std::cout << instr << " " << arg1 << ", ";
    print_arg_str_int(arg2, addr);
    std::cout << "\n";
}

// <instr> <arg1: str>, <data: int>
void printer::print_instr_str_int(const char* instr, const char* arg1, const i32 data) {
    std::cout << instr << " " << arg1 << ", " << data << "\n";
}

// <instr> [<arg1: str> + <addr: int>], <data: int>
void printer::print_instr_str_int_int(const char* instr, const char* arg1, const i32 addr, const i32 data) {
    std::cout << instr << " ";
    print_arg_str_int(arg1, addr);
    std::cout << ", " << data << "\n";
}

// <instr> <data: int>
void printer::print_instr_int(const char* const instr, const i32 data) {
    std::cout << instr << " " << data << "\n";
}

// <instr> <arg1: str>
void printer::print_instr_str(const char* const instr, const char* const arg1) {
    std::cout << instr << " " << arg1 << "\n";
}
