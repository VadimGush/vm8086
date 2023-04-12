//
// Created by Vadim Gush on 02.04.2023.
//

#include "decoder.h"
#include "printer.h"
#include <iostream>
#include <sstream>
#include <utils/collections.h>
#include <utils/bits.h>
using namespace decoder;

static umap<i32, str> labels{};

const char* get_label(const io::input_stream& is, const i32 displacement) {
    const i32 pos = static_cast<i32>(is.pos) + displacement;
    if (!labels.contains(pos)) {
        std::stringstream ss{};
        ss << "label_" << labels.size();
        labels.emplace(pos, ss.str());
    }
    return labels.at(pos).c_str();
}

// [ data low ] [ data high ]
i32 decode_signed_data(const bool word_data, io::input_stream& is) {
    if (word_data) {
        const u8 low = is.next_byte();
        const u8 high = is.next_byte();
        return bits::reinterpret<i16>(bits::combine(high, low));
    } else {
        const u8 low = is.next_byte();
        return bits::reinterpret<i8>(low);
    }
}

// [ data low ] [ data high ]
u32 decode_unsigned_data(const bool word_data, io::input_stream& is) {
    if (word_data) {
        const u8 low = is.next_byte();
        const u8 high = is.next_byte();
        return bits::combine(high, low);
    } else {
        return is.next_byte();
    }
}

// [ mod reg rm ]
mod_reg_rm decode_mod_reg_rm(io::input_stream& is) {
    const u8 byte = is.next_byte();
    return mod_reg_rm {
            .mod = static_cast<MemoryMode>(byte >> 6),
            .reg = static_cast<u8>((byte >> 3) & bits::LOW_3BIT),
            .rm = static_cast<u8>(byte & bits::LOW_3BIT)
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

    // TODO: brute-forcing the first byte until we get an instruction is not a good solution
    //       we want to know which instructions we should check after we read the very first bit

    // MOV - register/memory to/from register
    if ((byte >> 2) == 0b00100010) {
        const bool reg_dest = (byte >> 1) & bits::LOW_1BIT;
        const bool word = byte & bits::LOW_1BIT;
        return decode_mod_reg_rm_disp(is, "mov", reg_dest, word);
    }

    // MOV - immediate to register
    if ((byte >> 4) == 0b00001011) {
        const bool word = ((byte >> 3) & bits::LOW_1BIT) != 0;
        const char* reg = printer::get_register_name(word, byte & bits::LOW_3BIT);
        const i32 data = decode_signed_data(word, is);
        printer::print_instr_str_int("mov", reg, data);
        return decoder::DecodingError::NONE;
    }

    // ADD - reg/memory with register to either
    if ((byte >> 2) == 0) {
        const bool reg_dest = (byte >> 1) & bits::LOW_1BIT;
        const bool word = byte & bits::LOW_1BIT;
        return decode_mod_reg_rm_disp(is, "add", reg_dest, word);
    }

    // CMP - reg/memory with register to either
    if ((byte >> 2) == 0b00001110) {
        const bool reg_dest = (byte >> 1) & bits::LOW_1BIT;
        const bool word = byte & bits::LOW_1BIT;
        return decode_mod_reg_rm_disp(is, "cmp", reg_dest, word);
    }

    // ADD - immediate to accumulator
    if ((byte >> 1) == 0b00000010) {
        const bool word = byte & bits::LOW_1BIT;
        const i32 data = decode_signed_data(word, is);
        printer::print_instr_str_int("add", printer::get_register_name(word, 0), data);
        return decoder::DecodingError::NONE;
    }

    // SUB - immediate from accumulator
    if ((byte >> 1) == 0b00010110) {
        const bool word = byte & bits::LOW_1BIT;
        const i32 data = decode_signed_data(word, is);
        printer::print_instr_str_int("sub", printer::get_register_name(word, 0), data);
        return decoder::DecodingError::NONE;
    }

    // CMP - immediate with accumulator
    if ((byte >> 1) == 0b00011110) {
        const bool word = byte & bits::LOW_1BIT;
        const i32 data = decode_signed_data(word, is);
        printer::print_instr_str_int("cmp", printer::get_register_name(word, 0), data);
        return decoder::DecodingError::NONE;
    }

    // SUB - reg/memory and register to either
    if ((byte >> 2) == 0b00001010) {
        const bool reg_dest = (byte >> 1) & bits::LOW_1BIT;
        const bool word = byte & bits::LOW_1BIT;
        return decode_mod_reg_rm_disp(is, "sub", reg_dest, word);
    }

    if ((byte >> 2) == 0b00100000) {
        const bool sign = (byte >> 1) & bits::LOW_1BIT;
        const bool word = byte & bits::LOW_1BIT;
        const mod_reg_rm mrr = decode_mod_reg_rm(is);

        // ADD - immediate to register/memory
        if (mrr.reg == 0b00000000) return decode_mod_opcode_rm_disp_data(is, mrr, "add", sign, word);
        // SUB - immediate from register/memory
        if (mrr.reg == 0b00000101) return decode_mod_opcode_rm_disp_data(is, mrr, "sub", sign, word);
        // CMP - immediate with register/memory
        if (mrr.reg == 0b00000111) return decode_mod_opcode_rm_disp_data(is, mrr, "cmp", sign, word);
    }

    // JE/JZ - jump on equal zero
    if (byte == 0b01110100) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("je", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // JL/JNGE - jump on less/not greater or equal
    if (byte == 0b01111100) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("jl", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // JLE/JNG - jump on less or equal/not greater
    if (byte == 0b01111110) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("jle", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // JB/JNAE - jump on below/not above or equal
    if (byte == 0b01110010) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("jb", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // JBE/JNA - jump on below or equal/not above
    if (byte == 0b01110110) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("jbe", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // JP/JPE - jump on parity/parity even
    if (byte == 0b01111010) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("jp", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // JO - jump on overflow
    if (byte == 0b01110000) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("jo", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // JS - jump on sign
    if (byte == 0b01111000) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("js", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // JNE/JNZ - jump on not equal/not zero
    if (byte == 0b01110101) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("jne", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // JNL/JGE - jump on not less/greater or equal
    if (byte == 0b01111101) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("jnl", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // JNLE/JG - jump on not less or equal/greater
    if (byte == 0b01111111) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("jnle", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // JNB/JAE - jump on not below/above or equal
    if (byte == 0b01110011) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("jnb", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // JNBE/JA - jump on not below or equal/above
    if (byte == 0b01110111) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("jnbe", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // JNP/JPO - jump on not par/par odd
    if (byte == 0b01111011) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("jnp", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // JNO - jump on not overflow
    if (byte == 0b01110001) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("jno", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // JNS - jump on not sign
    if (byte == 0b01111001) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("jns", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // LOOP - loop CX times
    if (byte == 0b11100010) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("loop", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // LOOPZ/LOOPE - loop while zero/equal
    if (byte == 0b11100001) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("loopz", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // LOOPNZ/LOOPNE - lopp while zero/equal
    if (byte == 0b11100000) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("loopnz", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    // JCXZ - jump on CX zero
    if (byte == 0b11100011) {
        const i32 label = static_cast<i32>(decode_signed_data(false, is));
        printer::print_instr_str("jcxz", get_label(is, label));
        return decoder::DecodingError::NONE;
    }

    return decoder::DecodingError::UNKNOWN_INSTRUCTION;
}
