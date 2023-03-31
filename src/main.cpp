
#include "io.h"
#include <iostream>
using namespace std;

namespace mov {

    enum class MemoryMode : u8 {
        MEMORY_MODE         = 0b00000000,
        MEMORY_MODE_8_BIT   = 0b00000001,
        MEMORY_MODE_16_BIT  = 0b00000010,
        REGISTER_MODE       = 0b00000011,
    };

    const static char* register_names_byte[] = {
            "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
    };

    const static char* register_names_word[] = {
            "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
    };

    const static char* register_pattern[] = {
            "bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"
    };

    void print_mov_register_to_register(
            const bool reg_dest,
            const bool word_data,
            const u8 reg_field,
            const u8 rm_field) {

        cout << "mov ";

        const char* reg_field_register_name;
        const char* rm_field_register_name;

        if (word_data) {
            reg_field_register_name = register_names_word[reg_field];
            rm_field_register_name = register_names_word[rm_field];
        } else {
            reg_field_register_name = register_names_byte[reg_field];
            rm_field_register_name = register_names_byte[rm_field];
        }

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

        const char* register_name;
        if (word_data) register_name = register_names_word[reg_field];
        else register_name = register_names_byte[reg_field];

        if (reg_dest) cout << register_name << ", [" << address << "]";
        else cout << "[" << address << "], " << register_name;

        cout << "\n";
    }

    void print_mov_memory_mode(
            const bool reg_dest,
            const bool word_data,
            const u8 reg_field,
            const u8 rm_field) {
    }

    u8 read_reg_field(const u8 byte) {
        return byte >> 3 & bit::LOW_3BIT;
    }

    u8 read_rm_field(const u8 byte) {
        return byte & bit::LOW_3BIT;
    }

    bool decode(io::input_stream& is) {
        u8 byte = is.byte();

        // register/memory to/from register
        if ((byte >> 2) == 0b00100010) {
            const bool reg_dest = (byte >> 1) & bit::LOW_1BIT;
            const bool word_data = byte & bit::LOW_1BIT;

            byte = is.next_byte();

            const auto memory_mode = static_cast<MemoryMode>(byte >> 6);
            switch (memory_mode) {
                case MemoryMode::REGISTER_MODE: {
                    const u8 reg_field = read_reg_field(byte);
                    const u8 rm_field = read_rm_field(byte);
                    print_mov_register_to_register(reg_dest, word_data, reg_field, rm_field);
                    return true;
                }
                case MemoryMode::MEMORY_MODE: {
                    const u8 reg_field = read_reg_field(byte);
                    const u8 rm_field = read_rm_field(byte);

                    // if R/M field = 110, we should read DIRECT ADDRESS
                    if (rm_field == 0b00000110) {
                        const u8 address_low = is.next_byte();
                        const u8 address_high = is.next_byte();
                        const u16 address = bit::combine(address_high, address_low);
                        print_mov_direct_address(reg_dest, address, reg_field, word_data);
                        return true;
                    }
                    return false;
                }
                default: {}
            }
        }

        return false;
    }
}

int decode(io::input_stream& is) {
    for (is.next(); !is.complete(); is.next()) {
        if (mov::decode(is)) continue;

        const u8 byte = is.byte();
        cerr << "Decoding failed on: byte = ";
        bit::print_bits(cerr, byte);
        cerr << ", position = " << is.pos << "\n";
        return 1;
    }
    return 0;
}

int main() {
    io::input_stream is;

    return decode(is);
}
