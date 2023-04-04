//
// Created by Vadim Gush on 02.04.2023.
//

#include "decoder.h"
#include "printer.h"
#include <iostream>
using namespace decoder;

// [ data low ] [ data high ]
i32 decode_signed_data(const bool word_data, io::input_stream& is) {
    if (word_data) {
        const u8 low = is.next_byte();
        const u8 high = is.next_byte();
        return bit::reinterpret<i16>(bit::combine(high, low));
    } else {
        const u8 low = is.next_byte();
        return bit::reinterpret<i8>(low);
    }
}

// [ data low ] [ data high ]
u32 decode_unsigned_data(const bool word_data, io::input_stream& is) {
    if (word_data) {
        const u8 low = is.next_byte();
        const u8 high = is.next_byte();
        return bit::combine(high, low);
    } else {
        return is.next_byte();
    }
}

// [ mod reg rm ]
mod_reg_rm decode_mod_reg_rm(io::input_stream& is) {
    const u8 byte = is.next_byte();
    return mod_reg_rm {
            .mod = static_cast<MemoryMode>(byte >> 6),
            .reg = static_cast<u8>((byte >> 3) & bit::LOW_3BIT),
            .rm = static_cast<u8>(byte & bit::LOW_3BIT)
    };
}

// [ mod reg rm ] [ disp low ] [ disp high ]
decoder::DecodingError decode_mod_reg_rm_disp(io::input_stream& is, const char* instr, const bool reg_dest, const bool word) {
    const mod_reg_rm mrr = decode_mod_reg_rm(is);

    switch (mrr.mod) {

        case MemoryMode::REGISTER_MODE: {
            const char* reg1 = printer::get_register_name(word, mrr.reg);
            const char* reg2 = printer::get_register_name(word, mrr.rm);

            // <instr> <reg>, <reg>
            printer::print_instr_str_str(!reg_dest, instr, reg1, reg2);
            return decoder::DecodingError::NONE;
        }

        case MemoryMode::MEMORY_MODE: {
            // if R/M field = 110, we should read DIRECT ADDRESS
            if (mrr.rm == 0b00000110) {
                const i32 address = static_cast<i32>(decode_unsigned_data(true, is));
                const char* reg = printer::get_register_name(word, mrr.reg);

                // <instr> <reg>, [address]
                if (reg_dest) printer::print_instr_str_str_int(instr, reg, nullptr, address);
                // <instr> [address], <reg>
                else printer::print_instr_str_int_str(instr, nullptr, address, reg);
            } else {
                const char* reg_pattern = printer::get_register_pattern(mrr.rm);
                const char* reg = printer::get_register_name(word, mrr.reg);

                // <instr> <reg>, [pattern]
                if (reg_dest) printer::print_instr_str_str_int(instr, reg, reg_pattern, 0);
                // <instr> [pattern], <reg>
                else printer::print_instr_str_int_str(instr, reg_pattern, 0, reg);
            }
            return decoder::DecodingError::NONE;
        }

        case MemoryMode::MEMORY_MODE_8_BIT: {
            const i32 disp = decode_signed_data(false, is);
            const char* reg_pattern = printer::get_register_pattern(mrr.rm);
            const char* reg = printer::get_register_name(word, mrr.reg);

            // <instr> <reg>, [pattern + disp]
            if (reg_dest) printer::print_instr_str_str_int(instr, reg, reg_pattern, disp);
            // <instr> [pattern + disp], <reg>
            else printer::print_instr_str_int_str(instr, reg_pattern, disp, reg);

            return decoder::DecodingError::NONE;
        }

        case MemoryMode::MEMORY_MODE_16_BIT: {
            const i32 disp = decode_signed_data(true, is);
            const char* reg_pattern = printer::get_register_pattern(mrr.rm);
            const char* reg = printer::get_register_name(word, mrr.reg);

            // <instr> <reg>, [pattern + disp]
            if (reg_dest) printer::print_instr_str_str_int(instr, reg, reg_pattern, disp);
            // <instr> [pattern + disp], <reg>
            else printer::print_instr_str_int_str(instr, reg_pattern, disp, reg);

            return decoder::DecodingError::NONE;
        }
    }

    return decoder::DecodingError::UNSUPPORTED_INSTRUCTION_TYPE;
}

// [ mod <opcode> rm ] [ disp low ] [ disp high] [ data low ] [ data high ]
decoder::DecodingError decode_mod_opcode_rm_disp_data(io::input_stream& is,
                                                      const mod_reg_rm& mrr,
                                                      const char* instr,
                                                      const bool sign,
                                                      const bool word) {

    switch(mrr.mod) {

        case MemoryMode::REGISTER_MODE: {
            const char* reg = printer::get_register_name(word, mrr.rm);
            const i32 data = decode_signed_data(!sign & word, is);
            // <instr> <reg>, <data>
            printer::print_instr_str_int(instr, reg, data);
            return DecodingError::NONE;
        }

        case MemoryMode::MEMORY_MODE: {
            // direct address
            if (mrr.rm == 0b00000110) {
                const i32 address = static_cast<i32>(decode_unsigned_data(true, is));
                const i32 data = decode_signed_data(!sign & word, is);
                // <instr> [ <address> ], <data>
                printer::print_instr_str_int_int(instr, nullptr, address, data);
            } else {
                const char* pattern = printer::get_register_pattern(mrr.rm);
                const i32 data = decode_signed_data(!sign & word, is);
                // <instr> [ <pattern> ], <data>
                printer::print_instr_str_int_int(instr, pattern, 0, data);
            }
            return DecodingError::NONE;
        }

        case MemoryMode::MEMORY_MODE_8_BIT: {
            const char* pattern = printer::get_register_pattern(mrr.rm);
            const i32 disp = decode_signed_data(false, is);
            const i32 data = decode_signed_data(!sign & word, is);
            // <instr> [ <pattern> + <disp> ], <data>
            printer::print_instr_str_int_int(instr, pattern, disp, data);
            return DecodingError::NONE;
        }

        case MemoryMode::MEMORY_MODE_16_BIT: {
            const char* pattern = printer::get_register_pattern(mrr.rm);
            const i32 disp = decode_signed_data(true, is);
            const i32 data = decode_signed_data(!sign & word, is);
            // <instr> [ <pattern> + <disp> ], <data>
            printer::print_instr_str_int_int(instr, pattern, disp, data);
            return DecodingError::NONE;
        }
    }

    return decoder::DecodingError::UNSUPPORTED_INSTRUCTION_TYPE;
}


decoder::DecodingError decoder::decode(io::input_stream& is) {
    const u8 byte = is.byte();

    // MOV - register/memory to/from register
    if ((byte >> 2) == 0b00100010) {
        const bool reg_dest = (byte >> 1) & bit::LOW_1BIT;
        const bool word = byte & bit::LOW_1BIT;
        return decode_mod_reg_rm_disp(is, "mov", reg_dest, word);
    }

    // MOV - immediate to register
    if ((byte >> 4) == 0b00001011) {
        const bool word = ((byte >> 3) & bit::LOW_1BIT) != 0;
        const char* reg = printer::get_register_name(word, byte & bit::LOW_3BIT);
        const i32 data = decode_signed_data(word, is);
        printer::print_instr_str_int("mov", reg, data);
        return decoder::DecodingError::NONE;
    }

    // ADD - reg/memory with register to either
    if ((byte >> 2) == 0) {
        const bool reg_dest = (byte >> 1) & bit::LOW_1BIT;
        const bool word = byte & bit::LOW_1BIT;
        return decode_mod_reg_rm_disp(is, "add", reg_dest, word);
    }

    // CMP - reg/memory with register to either
    if ((byte >> 2) == 0b00001110) {
        const bool reg_dest = (byte >> 1) & bit::LOW_1BIT;
        const bool word = byte & bit::LOW_1BIT;
        return decode_mod_reg_rm_disp(is, "cmp", reg_dest, word);
    }

    // ADD - immediate to accumulator
    if ((byte >> 1) == 0b00000010) {
        const bool word = byte & bit::LOW_1BIT;
        const i32 data = decode_signed_data(word, is);
        printer::print_instr_str_int("add", "ax", data);
        return decoder::DecodingError::NONE;
    }

    // SUB - immediate from accumulator
    if ((byte >> 1) == 0b00010110) {
        const bool word = byte & bit::LOW_1BIT;
        const i32 data = decode_signed_data(word, is);
        printer::print_instr_str_int("sub", "ax", data);
        return decoder::DecodingError::NONE;
    }

    // CMP - immediate with accumulator
    if ((byte >> 1) == 0b00011110) {
        const bool word = byte & bit::LOW_1BIT;
        const i32 data = decode_signed_data(word, is);
        printer::print_instr_str_int("cmp", "ax", data);
        return decoder::DecodingError::NONE;
    }

    // SUB - reg/memory and register to either
    if ((byte >> 2) == 0b00001010) {
        const bool reg_dest = (byte >> 1) & bit::LOW_1BIT;
        const bool word = byte & bit::LOW_1BIT;
        return decode_mod_reg_rm_disp(is, "sub", reg_dest, word);
    }

    if ((byte >> 2) == 0b00100000) {
        const bool sign = (byte >> 1) & bit::LOW_1BIT;
        const bool word = byte & bit::LOW_1BIT;
        const mod_reg_rm mrr = decode_mod_reg_rm(is);

        // ADD - immediate to register/memory
        if (mrr.reg == 0b00000000) return decode_mod_opcode_rm_disp_data(is, mrr, "add", sign, word);
        // SUB - immediate from register/memory
        if (mrr.reg == 0b00000101) return decode_mod_opcode_rm_disp_data(is, mrr, "sub", sign, word);
        // CMP - immediate with register/memory
        if (mrr.reg == 0b00000111) return decode_mod_opcode_rm_disp_data(is, mrr, "cmp", sign, word);
    }

    return decoder::DecodingError::UNKNOWN_INSTRUCTION;
}
