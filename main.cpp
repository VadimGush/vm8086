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
using u16 = unsigned short;

enum class Result {
    SUCCESS = 99,
    ERROR_DECODE_INSTRUCTION_UNKNOWN_INSTRUCTION = 0,
    ERROR_EXECUTE_UNSUPPORTED_INSTRUCTION = 1,
    ERROR_DECODE_ARGUMENTS_UNSUPPORTED_INSTRUCTION = 2,
};

const static char* errors[] = {
    "Instruction decoding failure: unknown instruction",
    "Execution failure: unknown instruction",
    "Arguments decoding failure: instruction not supported",
};

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
    MOV,
};

namespace mov {

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

    enum class RegisterMemory : u8 {
        BX_SI               = 0b00000000,
        BX_DI               = 0b00000001,
        BP_SI               = 0b00000010,
        BP_DI               = 0b00000011,
        SI                  = 0b00000100,
        DI                  = 0b00000101,
        BP                  = 0b00000110,
        BX                  = 0b00000111,
    };

    enum class RegFieldMode {
        DESTINATION,
        SOURCE,
    };

    struct state {
        // W field
        OperationMode operation_mode        = OperationMode::BYTE_DATA;
        // MOD field
        MemoryMode memory_mode              = MemoryMode::MEMORY_MODE;
        // D field
        RegFieldMode reg_field_mode         = RegFieldMode::DESTINATION;
        // R/M field
        RegisterMemory register_memory      = RegisterMemory::BX_SI;

        u8 dst_register = 0;
        u8 src_register = 0;

        u16 displacement = 0;
    };

}

/**
* Represents the state of the instruction decoder. When the current instruction
* is decoded, the flag "complete" will be set to "true", which means that 
* instruction is ready to be executed.
*/
struct decoder_state {
    State state = State::READ_INSTRUCTION;
    Instruction instruction;

    // Instruction specific states for decoding
    mov::state mov;

    size_t bytes_read = 0;
    bool complete = false;
};

template <typename O>
void print_byte(O& out, const u8 byte) {
    for (int i = 0; i < 8; i++) {
        if (byte << i & 0b10000000) {
            out << "1";
        } else {
            out << "0";
        }
    }
}

/**
* Decodes an instruction by reading the current byte in the data stream.
*/
Result decode_instruction(decoder_state& decoder_state, const u8 byte) {

    // MOV - register/memory to/from register
    if ((byte >> 2) == 0b00100010) {
        decoder_state.instruction = Instruction::MOV;
        decoder_state.state = State::READ_ARGUMENTS;

        mov::state& state = decoder_state.mov;
        state.operation_mode = static_cast<mov::OperationMode>(byte & 0b00000001);
        state.reg_field_mode = (byte & 0b00000010) ? mov::RegFieldMode::DESTINATION : mov::RegFieldMode::SOURCE;
        return Result::SUCCESS;
    }
    
    cerr << "Unknown instruction: ";
    print_byte(cerr, byte);
    cerr << endl;
    return Result::ERROR_DECODE_INSTRUCTION_UNKNOWN_INSTRUCTION;
}

/**
* Decodes all arguments (if the instruction has any) by reading the current
* byte in the data stream;
*/
Result decode_arguments(decoder_state& decoder_state, u8 byte) {

    if (decoder_state.instruction == Instruction::MOV) {
        mov::state& state = decoder_state.mov;

        // We need to read MOD field in advance on the second byte
        // of decoded instruction in order to understand what we need
        // to do further
        if (decoder_state.bytes_read == 1) {
            state.memory_mode = static_cast<mov::MemoryMode>(byte >> 6);
        }

        switch (state.memory_mode) {

            // In case of register mode (when we're only moving data between registers)
            // we need to just get the registers themselfs and nothing more 
            case (mov::MemoryMode::REGISTER_MODE): {
                // Read REG, R/M fields
                const u8 reg = (byte >> 3) & 0b00000111;
                const u8 rmf = byte & 0b00000111;

                switch (state.reg_field_mode) {
                    case (mov::RegFieldMode::DESTINATION): {
                        state.dst_register = reg;
                        state.src_register = rmf;
                        break;
                    }
                    case (mov::RegFieldMode::SOURCE): {
                        state.dst_register = rmf;
                        state.src_register = reg;
                        break;
                    }
                }
                // this mod requires only to registers that
                // we already got, so we can complete this instruction
                decoder_state.complete = true;
                return Result::SUCCESS;
            }

            // In case of memory mode (we're moving to/from register to/from memory)
            // we need to understand if we need to read a displacement
            case (mov::MemoryMode::MEMORY_MODE): {

                if (decoder_state.bytes_read == 1) {
                    // Read REG, R/M fields
                    const u8 reg = (byte >> 3) & 0b00000111;
                    state.register_memory = static_cast<mov::RegisterMemory>(byte & 0b00000111);
                    state.memory_mode = static_cast<mov::MemoryMode>(byte >> 6);

                    switch (state.reg_field_mode) {
                        case (mov::RegFieldMode::DESTINATION): {
                            state.dst_register = reg;
                            break;
                        }
                        case (mov::RegFieldMode::SOURCE): {
                            state.src_register = reg;
                            break;
                        }
                    }

                    // If we have memory mode with no displacement
                    // and it is not the case when MOD = 00 and R/M = 110
                    // we should just complete an instruction
                    if (state.memory_mode == mov::MemoryMode::MEMORY_MODE && state.register_memory != mov::RegisterMemory::BP) {
                        decoder_state.complete = true;
                    }

                } else if (decoder_state.bytes_read == 2) {

                    // The third byte is low displacement 8-bit value
                    const u16 low_d = static_cast<u16>(byte);
                    state.displacement = (state.displacement & 0xFF00) | low_d;

                    // Complete instruction if we're only required to read
                    // the 8 bit displacement
                    if (state.memory_mode == mov::MemoryMode::MEMORY_MODE_8_BIT) {
                        decoder_state.complete = true;
                    }

                } else if (decoder_state.bytes_read == 3) {

                    // The forth byte is high displacement 8-bit value
                    const u16 high_d = static_cast<u16>(byte);
                    state.displacement = (state.displacement & 0x00FF) | (high_d << 8);
                    decoder_state.complete = true;
                }
                break;
            }
            default: {}
        }
    }

    return Result::SUCCESS;
}

const char* register_names_w0[] = {
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
};

const char* register_names_w1[] = {
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

/**
* In our case execute will just print assembly representation
* of the current 8086 instruction.
*/
Result execute(decoder_state& decoder_state) {

    switch (decoder_state.instruction) {
        case (Instruction::MOV): {
            cout << "mov ";
            mov::state& state = decoder_state.mov;
            switch (state.memory_mode) {
                case (mov::MemoryMode::REGISTER_MODE): {
                    switch (state.operation_mode) {
                        case (mov::OperationMode::BYTE_DATA): {
                            cout << register_names_w0[state.dst_register] << ", ";
                            cout << register_names_w0[state.src_register];
                            break;
                        }
                        case (mov::OperationMode::WORD_DATA): {
                            cout << register_names_w1[state.dst_register] << ", ";
                            cout << register_names_w1[state.src_register];
                            break;
                        }
                    }
                    break;
                }
                case (mov::MemoryMode::MEMORY_MODE): {

                }
                default: {}
            }
            cout << endl;
            break;
        }
        default: {
            return Result::ERROR_EXECUTE_UNSUPPORTED_INSTRUCTION;
        }
    }

    // Set state of the decoder to read a new instruction
    decoder_state.complete = false;
    decoder_state.state = State::READ_INSTRUCTION;
    decoder_state.bytes_read = 0;
    return Result::SUCCESS;
}

Result decode(const vector<u8>& data) {
    decoder_state decoder{};

    for (const u8 byte : data) {
        // Result of decoding of the current byte
        Result result = Result::SUCCESS;

        // Every instruction is decoded byte by byte and it is
        // devided into 2 parts: instruction and its arguments
        // which byte is considered to be part of instruction
        // and which one part of arguments is up to the decoding code.
        switch (decoder.state) {
            case (State::READ_INSTRUCTION): { 
                result = decode_instruction(decoder, byte);
                break;
            }
            case (State::READ_ARGUMENTS): { 
                result = decode_arguments(decoder, byte); 
                break;
            }
        }
        decoder.bytes_read += 1;

        // As soon as instruction is decoded (meaning it is completed)
        // we will execute it
        if (decoder.complete) {
            result = execute(decoder);
        }

        if (result != Result::SUCCESS) {
            return result;
        }
    }
    return Result::SUCCESS;
}

int main() {

    vector<u8> data = read_data();
    cout << "read: " << data.size() << " bytes" << endl << endl;

    const Result result = decode(data);
    if (result != Result::SUCCESS) {
        cerr << "Error! ";
        const int error_message_id = static_cast<int>(result);
        cerr << errors[error_message_id] << endl;
    }

    return 0;
}
