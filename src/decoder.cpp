//
// Created by Vadim Gush on 02.04.2023.
//

#include "decoder.h"
#include "helper.h"
#include <iostream>
using namespace helper;

decoder::DecodingError decoder::decode(io::input_stream& is) {
    const u8 byte = is.byte();

    // MOV - register/memory to/from register
    if ((byte >> 2) == 0b00100010) {
        std::cout << "mov ";

        const bool reg_dest = (byte >> 1) & bit::LOW_1BIT;
        const bool word = byte & bit::LOW_1BIT;
        const mod_reg_rm mrr = read_mod_reg_rm(is);

        switch (mrr.mod) {

            case MemoryMode::REGISTER_MODE: {
                print_reg_reg(!reg_dest, mrr.reg_name(word), mrr.rm_name(word));
                return decoder::DecodingError::NONE;
            }

            case MemoryMode::MEMORY_MODE: {
                // if R/M field = 110, we should read DIRECT ADDRESS
                if (mrr.rm == 0b00000110) {
                    const u32 address = read_unsigned_data(true, is);
                    print_reg_direct_memory(!reg_dest, mrr.reg_name(word), address);
                } else {
                    print_reg_memory(!reg_dest, mrr.reg_name(word), mrr.rm_pattern());
                }
                return decoder::DecodingError::NONE;
            }

            case MemoryMode::MEMORY_MODE_8_BIT: {
                const i32 disp = read_signed_data(false, is);
                print_reg_memory_disp(!reg_dest, mrr.reg_name(word), mrr.rm_pattern(), disp);
                return decoder::DecodingError::NONE;
            }

            case MemoryMode::MEMORY_MODE_16_BIT: {
                const i32 disp = read_signed_data(true, is);
                print_reg_memory_disp(!reg_dest, mrr.reg_name(word), mrr.rm_pattern(), disp);
                return decoder::DecodingError::NONE;
            }
        }
    }

    // MOV - immediate to register
    if ((byte >> 4) == 0b00001011) {
        const bool word_data = ((byte >> 3) & bit::LOW_1BIT) != 0;
        const u8 reg = byte & bit::LOW_3BIT;
        const i32 data = read_signed_data(word_data, is);

        const char* register_name = get_register_name(word_data, reg);
        std::cout << "mov " << register_name << ", " << data << "\n";
        return decoder::DecodingError::NONE;
    }

    return decoder::DecodingError::UNKNOWN_INSTRUCTION;
}
