#define READ_BUFFER_SIZE 1024

#include <memory>
#include <fcntl.h>
#include <unistd.h>
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

using u8 = unsigned char;

struct data_block {
    u8 buffer[READ_BUFFER_SIZE];
    // TODO: this might result in stack overflow 
    unique_ptr<data_block> next;
};

/**
* Reads a binary data from the standart input.
*/
vector<u8> read_data() {
    data_block block = { .next = nullptr };
    data_block* current_block = &block;

    // Read input in blocks of binary data
    size_t total_bytes = 0;
    while (true) {
        const size_t bytes = read(0, current_block->buffer, READ_BUFFER_SIZE);
        total_bytes += bytes;

        if (bytes == READ_BUFFER_SIZE) {
            current_block->next = make_unique<data_block>();
            current_block = current_block->next.get();
        } else {
            break;
        }
    }

    // Convert blocks of data into one big buffer
    vector<u8> data(total_bytes);
    current_block = &block;
    size_t block_id = 0;
    while (current_block != nullptr) {
        const size_t bytes_to_copy = min(total_bytes, (size_t) READ_BUFFER_SIZE);
        memcpy(&data[block_id * READ_BUFFER_SIZE], current_block->buffer, bytes_to_copy);
        total_bytes -= bytes_to_copy;

        current_block = current_block->next.get();
        block_id += 1;
    }
    return data;
}

enum class State {
    READ_INSTRUCTION,
    READ_ARGUMENTS,
};

enum class Instruction {
    MOVE,
};

enum class OperationMode : u8 {
    BYTE_DATA           = 0b00000000,
    WORD_DATA           = 0b00000001,
};

enum class MemoryMode : u8 {
    MEMORY_MODE         = 0b00000000,
    MEMORY_MODE_8_BIT   = 0b00000001,
    MEMORY_MODE_16_BIT  = 0b00000010,
    REGISTER_MODE       = 0b00000011,
};

/**
* Represents the state of the instruction decoder. When the current instruction
* is decoded, the flag "complete" will be set to "true", which means that 
* instruction is ready to be executed.
*/
struct decoder_state {

    State state = State::READ_INSTRUCTION;
    Instruction instruction = Instruction::MOVE;

    OperationMode operation_mode = OperationMode::BYTE_DATA;
    MemoryMode memory_mode = MemoryMode::MEMORY_MODE;

    bool reg_field_dest = false;
    u8 dst_register = 0;
    u8 src_register = 0;

    bool complete = false;
};


/**
* Decodes an instruction by reading the current byte in the data stream.
*/
int decode_instruction(decoder_state& state, u8 byte) {

    if ((byte >> 2) == 0b00100010) {
        state.state = State::READ_ARGUMENTS;
        state.instruction = Instruction::MOVE;
        state.operation_mode = static_cast<OperationMode>(byte & 0b00000001);
        state.reg_field_dest = (byte & 0b00000010) ? true : false;
        return 0;
    }
    return 1;
}

/**
* Decodes all arguments (if the instruction has any) by reading the current
* byte in the data stream;
*/
int decode_arguments(decoder_state& state, u8 byte) {

    if (state.instruction == Instruction::MOVE) {
        state.memory_mode = static_cast<MemoryMode>(byte >> 6);

        if (state.memory_mode == MemoryMode::REGISTER_MODE) {
            const u8 reg = (byte >> 3) & 0b00000111;
            const u8 rmf = byte & 0b00000111;
            if (state.reg_field_dest) {
                state.dst_register = reg;
                state.src_register = rmf;
            } else {
                state.dst_register = rmf;
                state.src_register = reg;
            }
            state.complete = true;
        }
    }

    return 0;
}

const char* register_names_w0[] = {
    "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"
};

const char* register_names_w1[] = {
    "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"
};

/**
* In our case execute will just print assembly representation
* of the current 8086 instruction.
*/
int execute(decoder_state& state) {
    switch (state.instruction) {
        case (Instruction::MOVE): {
            cout << "MOV ";
            switch (state.memory_mode) {
                case (MemoryMode::REGISTER_MODE): {
                    switch (state.operation_mode) {
                        case (OperationMode::BYTE_DATA): {
                            cout << register_names_w0[state.dst_register] << ", ";
                            cout << register_names_w0[state.src_register];
                            break;
                        }
                        case (OperationMode::WORD_DATA): {
                            cout << register_names_w1[state.dst_register] << ", ";
                            cout << register_names_w1[state.src_register];
                            break;
                        }
                    }
                    break;
                }
                default: {}
            }
            cout << endl;
            break;
        }
    }

    state.complete = false;
    state.state = State::READ_INSTRUCTION;
    return 0;
}

int decode(const vector<u8>& data) {
    decoder_state state{};

    for (u8 byte : data) {
        int error = 0;

        /*
        For debugging purposes
        cout << "Current byte: ";
        for (int i = 0; i < 8; i++) {
            if (byte << i & 0b10000000) {
                cout << "1";
            } else {
                cout << "0";
            }
        }
        cout << endl;
        */

        // Decode parts of instruction
        switch (state.state) {
            case (State::READ_INSTRUCTION): { 
                error = decode_instruction(state, byte);
                break;
            }
            case (State::READ_ARGUMENTS): { 
                error = decode_arguments(state, byte); 
                break;
            }
        }

        // Execute decoded instruction
        if (state.complete) {
            execute(state);
        }

        if (error) return error;
    }
    return 0;
}

int main() {

    vector<u8> data = read_data();
    cout << "read: " << data.size() << " bytes" << endl << endl;
    decode(data);

    return 0;
}
