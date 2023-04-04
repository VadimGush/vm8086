//
// Created by Vadim Gush on 02.04.2023.
//

#ifndef VM8086_PRINTER_H
#define VM8086_PRINTER_H

#include "utils.h"
#include "io.h"

// Ha-ha, funny name ;D
namespace printer {

    // <instr> <arg1: str>, <arg2: str>
    void print_instr_str_str(bool invert, const char* instr, const char* arg1, const char* arg2);

    // <instr> [<arg1: str> + <addr: int>], <arg2: str>
    void print_instr_str_int_str(const char* instr, const char* arg1, i32 addr, const char* arg2);

    // <instr> <arg1: str>, [<arg2: str> + <addr: int>]
    void print_instr_str_str_int(const char* instr, const char* arg1, const char* arg2, i32 addr);

    // <instr> <arg1: str>, <data: int>
    void print_instr_str_int(const char* instr, const char* arg1, i32 data);

    // <instr> [<arg1: str> + <addr: int>], <data: int>
    void print_instr_str_int_int(const char* instr, const char* arg1, i32 addr, i32 data);

    // <instr> <data: int>
    void print_instr_int(const char* instr, i32 data);

    // <instr> <arg1: str>
    void print_instr_str(const char* instr, const char* arg1);

    const char* get_register_name(bool word_data, u8 reg);

    const char* get_register_pattern(u8 pattern);

}

#endif //VM8086_PRINTER_H
