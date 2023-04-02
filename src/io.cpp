//
// Created by Vadim Gush on 31.03.2023.
//

#include "io.h"
#include <unistd.h>

bool io::input_stream::complete() const {
    return buffer_pos == buffer_size;
}

u8 io::input_stream::byte() {
    if (buffer_pos == buffer_size) {
        return 0;
    }
    return buffer[buffer_pos];
}

u8 io::input_stream::read_byte() {
    next();
    return byte();
}

void io::input_stream::next() {
    if (buffer_pos == buffer_size) {
        const size_t bytes = ::read(0, buffer, BUFFER_SIZE);
        buffer_size = bytes;
        buffer_pos = 0;
    } else {
        buffer_pos += 1;
    }
    pos += 1;
}