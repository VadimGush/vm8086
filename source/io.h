//
// Created by Vadim Gush on 31.03.2023.
//

#ifndef VM8086_IO_H
#define VM8086_IO_H

#define BUFFER_SIZE 1024

#include <utils/types.h>
#include <cstdlib>

namespace io {

    /**
     * Represents stream of encoded instructions.
     */
    struct input_stream {
        u8 buffer[BUFFER_SIZE];
        size_t buffer_pos = BUFFER_SIZE;
        size_t buffer_size = BUFFER_SIZE;
        size_t pos = 0;

        u8 byte();

        void next();

        u8 next_byte();

        bool complete() const;
    };

}

#endif //VM8086_IO_H
