//
// Created by Vadim Gush on 02.04.2023.
//

#include "mov.h"
#include <iostream>
using namespace std;

const char* get_register_name(const bool word_data, const u8 reg_field) {
    return word_data ? mov::register_names_word[reg_field] : mov::register_names_byte[reg_field];
}

void print_mov_register_to_register(
        const bool reg_dest,
        const bool word_data,
        const u8 reg_field,
        const u8 rm_field) {
    cout << "mov ";

    const char* reg_field_register_name = get_register_name(word_data, reg_field);
    const char* rm_field_register_name = get_register_name(word_data, rm_field);
    if (reg_dest) cout << reg_field_register_name << ", " << rm_field_register_name;
    else cout << rm_field_register_name << ", " << reg_field_register_name;

    cout << "\n";
}

void print_mov_direct_address(
        const bool reg_dest,
        const u16 address,
        const u8 reg_field,
        const bool word_data) {
    cout << "mov ";

    const char* register_name = get_register_name(word_data, reg_field);
    if (reg_dest) cout << register_name << ", [" << address << "]";
    else cout << "[" << address << "], " << register_name;

    cout << "\n";
}

void print_mov_memory_mode(
        const bool reg_dest,
        const bool word_data,
        const u8 reg_field,
        const u8 rm_field) {
    cout << "mov ";

    const char* register_name = get_register_name(word_data, reg_field);
    const char* memory_pattern = mov::register_pattern[rm_field];
    if (reg_dest) cout << register_name << ", [" << memory_pattern << "]";
    else cout << "[" << memory_pattern << "], " << register_name;

    cout << "\n";
}

u8 read_reg_field(const u8 byte) {
    return byte >> 3 & bit::LOW_3BIT;
}

u8 read_rm_field(const u8 byte) {
    return byte & bit::LOW_3BIT;
}

errors::DecodingError mov::decode(io::input_stream& is) {
    u8 byte = is.byte();

    // register/memory to/from register
    if ((byte >> 2) == 0b00100010) {
        const bool reg_dest = (byte >> 1) & bit::LOW_1BIT;
        const bool word_data = byte & bit::LOW_1BIT;

        byte = is.next_byte();

        const auto memory_mode = static_cast<mov::MemoryMode>(byte >> 6);
        switch (memory_mode) {
            case mov::MemoryMode::REGISTER_MODE: {
                const u8 reg_field = read_reg_field(byte);
                const u8 rm_field = read_rm_field(byte);
                print_mov_register_to_register(reg_dest, word_data, reg_field, rm_field);
                return errors::DecodingError::NONE;
            }
            case mov::MemoryMode::MEMORY_MODE: {
                const u8 reg_field = read_reg_field(byte);
                const u8 rm_field = read_rm_field(byte);

                // if R/M field = 110, we should read DIRECT ADDRESS
                if (rm_field == 0b00000110) {
                    const u8 address_low = is.next_byte();
                    const u8 address_high = is.next_byte();
                    const u16 address = bit::combine(address_high, address_low);
                    print_mov_direct_address(reg_dest, address, reg_field, word_data);
                    return errors::DecodingError::NONE;
                } else {
                    print_mov_memory_mode(reg_dest, word_data, reg_field, rm_field);
                    return errors::DecodingError::NONE;
                }
            }
            default: return errors::DecodingError::UNSUPPORTED_INSTRUCTION_TYPE;
        }
    }

    return errors::DecodingError::UNKNOWN_INSTRUCTION;
}
