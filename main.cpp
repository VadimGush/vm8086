/**

NOTE:

This was an experiment to write an instruction decoder which is able
to switch between decoding multiple streams of instructions on the fly.

It is capable of basically switching between decoding one byte in one instruction
stream, to decoding another byte in another instruction stream. The result:
code is overly complicated and requires saving/reading state of the current
decoding process all the time on every byte read.

BAD IDEA. DO NOT IMPLEMENT DECODERS LIKE THIS.

*/
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
using i8 = char;
using u16 = unsigned short;
using i16 = short;

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

enum class Result {
    SUCCESS = 99,
    ERROR_DECODE_INSTRUCTION_UNKNOWN_INSTRUCTION = 0,
    ERROR_EXECUTE_UNSUPPORTED_INSTRUCTION = 1,
    ERROR_DECODE_ARGUMENTS_UNSUPPORTED_INSTRUCTION = 2,
    ERROR_DECODE_ARGUMENTS_WRONG_POSITION = 3,
};

const static char* errors[] = {
    "Instruction decoding failure: unknown instruction",
    "Execution failure: instruction is not supported for execution",
    "Arguments decoding failure: instruction not supported",
    "Arguments decoding failure: wrong decoder position",
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

namespace mov {

    enum class Type {
        REGISTER_MEMORY_TO_FROM_REGISTER,
        IMMEDIATE_TO_REGISTER_MEMORY,
        IMMEDIATE_TO_REGISTER,
        MEMORY_ACCUMULATOR,
        ACCUMULATOR_TO_MEMORY,
        REGISTER_MEMORY_TO_SEGMENT_REGISTER,
        SEGMENT_REGISTER_TO_REGISTER_MEMORY,
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
        // We have different moves
        Type type;

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

        u16 data = 0;
    };

}

enum class State {
    READ_INSTRUCTION,
    READ_ARGUMENTS,
};

enum class Instruction {
    MOV,
};

/**
* Represents the state of the instruction decoder. When the current instruction
* is decoded, the flag "complete" will be set to "true", which means that 
* instruction is ready to be executed.
*/
struct decoder_state {

    // We always start with reading a new instruction
    State state = State::READ_INSTRUCTION;

    // Instruction that we're currently decoding
    Instruction instruction;

    // Instruction specific states for decoding
    mov::state mov;

    // Position within current instruction
    size_t instruction_pos = 0;

    // Position within stream of data
    size_t pos = 0;

    // If the current instruction is complete
    bool complete = false;
};

namespace mov {

    Result decode_memory_mode(decoder_state& decoder_state, const u8 byte) {
        mov::state& state = decoder_state.mov;

        // Read REG, R/M fields
        const u8 reg = (byte >> 3) & 0b00000111;
        state.register_memory = static_cast<mov::RegisterMemory>(byte & 0b00000111);
        state.memory_mode = static_cast<mov::MemoryMode>(byte >> 6);
        state.displacement = 0;

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
        if (state.memory_mode == MemoryMode::MEMORY_MODE) {
            if (state.register_memory != mov::RegisterMemory::BP) {
                decoder_state.complete = true;
            }
        }

        return Result::SUCCESS;
    }

    Result decode_mod_reg_rm(decoder_state& decoder_state, const u8 byte) {
        mov::state& state = decoder_state.mov;

        // The way we decode REG and R/M fields depends on what
        // data we have in MOD field. So we will decode it first.
        state.memory_mode = static_cast<mov::MemoryMode>(byte >> 6);
        
        switch (state.memory_mode) {

            // In case of register mode (when we're only moving data between registers)
            // we need to just get the registers themselfs and nothing more 
            case mov::MemoryMode::REGISTER_MODE: {
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
            case mov::MemoryMode::MEMORY_MODE: {
                return decode_memory_mode(decoder_state, byte);
            }
            case mov::MemoryMode::MEMORY_MODE_8_BIT: {
                return decode_memory_mode(decoder_state, byte);
            }
            case mov::MemoryMode::MEMORY_MODE_16_BIT: {
                return decode_memory_mode(decoder_state, byte);
            }
            default: {}
        }

        return Result::SUCCESS;
    }

    Result decode_low_disp(decoder_state& decoder_state, const u8 byte) {
        mov::state& state = decoder_state.mov;

        state.displacement = static_cast<u16>(byte);

        return Result::SUCCESS;
    }

    Result decode_high_disp(decoder_state& decoder_state, const u8 byte) {
        mov::state& state = decoder_state.mov;

        const u16 high_d = static_cast<u16>(byte);
        state.displacement = (state.displacement & 0x00FF) | (high_d << 8);

        return Result::SUCCESS;
    }

    Result decode_low_data(decoder_state& decoder_state, const u8 byte) {
        mov::state& state = decoder_state.mov;

        state.data = static_cast<u16>(byte);

        return Result::SUCCESS;
    }

    Result decode_high_data(decoder_state& decoder_state, const u8 byte) {
        mov::state& state = decoder_state.mov;

        const u16 high_d = static_cast<u16>(byte);
        state.data = (state.data & 0x00FF) | (high_d << 8);

        return Result::SUCCESS;
    }

    Result decode_arguments(decoder_state& decoder_state, const u8 byte) {
        state& state = decoder_state.mov;

        switch (state.type) {

            case Type::REGISTER_MEMORY_TO_FROM_REGISTER: {
                switch (decoder_state.instruction_pos) {
                    case 1: {
                        return decode_mod_reg_rm(decoder_state, byte); 
                    }
                    case 2: {
                        const Result result = decode_low_disp(decoder_state, byte);
                        // Complete instruction if we're only required to read
                        // the 8 bit displacement
                        if (state.memory_mode == mov::MemoryMode::MEMORY_MODE_8_BIT) {
                            decoder_state.complete = true;
                        }
                        return result;
                    }
                    case 3: {
                        const Result result = decode_high_disp(decoder_state, byte);
                        decoder_state.complete = true;
                        return result;
                    }
                    default: {
                        return Result::ERROR_DECODE_ARGUMENTS_WRONG_POSITION;
                    }
                }
            }

            case Type::IMMEDIATE_TO_REGISTER: {
                switch (decoder_state.instruction_pos) {
                    case 1: {
                        const Result result = decode_low_data(decoder_state, byte);
                        if (state.operation_mode == OperationMode::BYTE_DATA) {
                            decoder_state.complete = true;
                        }
                        return result;
                    }
                    case 2: {
                        const Result result = decode_high_data(decoder_state, byte);
                        decoder_state.complete = true;
                        return result;
                    }
                    default: {
                        return Result::ERROR_DECODE_ARGUMENTS_WRONG_POSITION;
                    }
                }
            }

            default: {}
        }

        return Result::SUCCESS;
    }

    bool decode_instruction(decoder_state& decoder_state, const u8 byte) {

        // MOV - register/memory to/from register
        if ((byte >> 2) == 0b00100010) {
            decoder_state.instruction = Instruction::MOV;
            decoder_state.state = State::READ_ARGUMENTS;

            mov::state& state = decoder_state.mov;
            state.type = Type::REGISTER_MEMORY_TO_FROM_REGISTER;
            state.operation_mode = static_cast<OperationMode>(byte & 0b00000001);
            state.reg_field_mode = (byte & 0b00000010) ? 
                    mov::RegFieldMode::DESTINATION : mov::RegFieldMode::SOURCE;
            return true;
        }

        // MOV - immediate to register
        if ((byte >> 4) == 0b00001011) {
            decoder_state.instruction = Instruction::MOV;
            decoder_state.state = State::READ_ARGUMENTS;

            mov::state& state = decoder_state.mov;
            state.type = Type::IMMEDIATE_TO_REGISTER;
            state.operation_mode = static_cast<OperationMode>((byte >> 3) & 0b00000001);
            state.dst_register = byte & 0b00000111;
            return true;
        }

        return false;
    }

    const static char* register_names_w0[] = {
        "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
    };

    const static char* register_names_w1[] = {
        "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
    };

    const static char* register_memory_names[] = {
        "bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"
    };

    void execute_memory_mode(
        const decoder_state& decoder_state,
        const char* dst_register_name,
        const char* src_register_name) {
        const state& state = decoder_state.mov;

        const u8 pattern_id = static_cast<u8>(state.register_memory);
        const char* pattern = register_memory_names[pattern_id];
        const int disp = static_cast<int>(state.displacement);

        switch (state.reg_field_mode) {
            case RegFieldMode::DESTINATION: {
                cout << dst_register_name << ", [";
                if (disp != 0) {
                    cout << pattern << " + " << disp << "]";
                } else {
                    cout << pattern << "]";
                }
                break;
            }
            case RegFieldMode::SOURCE: {
                if (disp != 0) {
                    cout << "[" << pattern << " + " << disp <<  "], ";
                } else {
                    cout << "[" << pattern <<  "], ";
                }
                cout << src_register_name;
                break;
            }
        }
    }

    Result execute_register_memory_to_from_register(const decoder_state& decoder_state) {
        const state& state = decoder_state.mov;

        const char* dst_register_name;
        const char* src_register_name;
        switch (state.operation_mode) {

            case OperationMode::BYTE_DATA: {
                dst_register_name = register_names_w0[state.dst_register];
                src_register_name = register_names_w0[state.src_register];
                break;
            }

            case OperationMode::WORD_DATA: {
                dst_register_name = register_names_w1[state.dst_register];
                src_register_name = register_names_w1[state.src_register];
                break;
            }
        }


        switch (state.memory_mode) {

            case mov::MemoryMode::REGISTER_MODE: {
                cout << "mov ";
                cout << dst_register_name << ", ";
                cout << src_register_name;
                cout << endl;
                break;
            }

            case mov::MemoryMode::MEMORY_MODE: {
                cout << "mov ";
                if (state.register_memory == RegisterMemory::BP) {
                    // direct address
                    switch (state.reg_field_mode) {
                        case RegFieldMode::DESTINATION: {
                            cout << dst_register_name << ", [";
                            cout << state.displacement << "]";
                            break;
                        }
                        case RegFieldMode::SOURCE: {
                            cout << "[" << state.displacement << "], ";
                            cout << src_register_name;
                            break;
                        }
                    }
                } else {
                    execute_memory_mode(decoder_state, dst_register_name, src_register_name);
                }
                cout << endl;
                break;
            }

            case mov::MemoryMode::MEMORY_MODE_8_BIT: {
                cout << "mov ";
                execute_memory_mode(decoder_state, dst_register_name, src_register_name);
                cout << endl;
                break;
            }

            case mov::MemoryMode::MEMORY_MODE_16_BIT: {
                cout << "mov ";
                execute_memory_mode(decoder_state, dst_register_name, src_register_name);
                cout << endl;
                break;
            }

            default: {
                return Result::ERROR_EXECUTE_UNSUPPORTED_INSTRUCTION;
            }
        }
        return Result::SUCCESS;
    }

    Result execute_immediate_to_register(const decoder_state& decoder_state) {
        const state& state = decoder_state.mov;

        cout << "mov ";
        switch (state.operation_mode) {
            case (mov::OperationMode::BYTE_DATA): {
                cout << register_names_w0[state.dst_register] << ", ";

                const u8 s = static_cast<u8>(state.data);
                const i8 v = *reinterpret_cast<const i8*>(&s);
                cout << static_cast<int>(v);
                break;
            }
            case (mov::OperationMode::WORD_DATA): {
                cout << register_names_w1[state.dst_register] << ", ";

                const i16 v = *reinterpret_cast<const i16*>(&state.data);
                cout << static_cast<int>(v);
                break;
            }
        }
        cout << endl;

        return Result::SUCCESS;
    }

    Result execute(const decoder_state& decoder_state) {
        const state& state = decoder_state.mov;

        switch (state.type) {
            case Type::REGISTER_MEMORY_TO_FROM_REGISTER: {
                return execute_register_memory_to_from_register(decoder_state);
            }
            case Type::IMMEDIATE_TO_REGISTER: {
                return execute_immediate_to_register(decoder_state);
            }
            default: {
                return Result::ERROR_EXECUTE_UNSUPPORTED_INSTRUCTION;
            }
        }
        return Result::SUCCESS;
    }
}

/**
* Decodes an instruction by reading the current byte in the data stream.
*/
Result decode_instruction(decoder_state& decoder_state, const u8 byte) {

    // Go through every supported instruction
    if (mov::decode_instruction(decoder_state, byte)) { return Result::SUCCESS; }

    cerr << "Unknown instruction: '";
    print_byte(cerr, byte);
    cerr << "' at position: " << decoder_state.pos << endl;
    return Result::ERROR_DECODE_INSTRUCTION_UNKNOWN_INSTRUCTION;
}

/**
* Decodes all arguments (if the instruction has any) by reading the current
* byte in the data stream;
*/
Result decode_arguments(decoder_state& decoder_state, u8 byte) {

    switch (decoder_state.instruction) {
        case (Instruction::MOV): return mov::decode_arguments(decoder_state, byte);
        default: {}
    }

    // this should never happen
    return Result::ERROR_DECODE_ARGUMENTS_UNSUPPORTED_INSTRUCTION;
}


/**
* In our case execute will just print assembly representation
* of the current 8086 instruction.
*/
Result execute(decoder_state& decoder_state) {
    Result result = Result::SUCCESS;

    switch (decoder_state.instruction) {
        case Instruction::MOV: {
            result = mov::execute(decoder_state);
            break;
        }
        default: {
            return Result::ERROR_EXECUTE_UNSUPPORTED_INSTRUCTION;
        }
    }

    if (result != Result::SUCCESS) {
        return result;
    }

    // Set state of the decoder to read a new instruction
    decoder_state.complete = false;
    decoder_state.state = State::READ_INSTRUCTION;
    decoder_state.instruction_pos = 0;
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

        decoder.instruction_pos += 1;
        decoder.pos += 1;

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
