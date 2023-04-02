//
// Created by Vadim Gush on 31.03.2023.
//

#ifndef VM8086_UTILS_H
#define VM8086_UTILS_H

#include <vector>
#include <memory>

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long;

using i8 = char;
using i16 = short;
using i32 = int;
using i64 = long;
using fd_t = int;

namespace bit {

    constexpr u8 LOW_1BIT = 0b00000001;
    constexpr u8 LOW_2BIT = 0b00000011;
    constexpr u8 LOW_3BIT = 0b00000111;
    constexpr u8 LOW_4BIT = 0b00001111;

    constexpr u8 HIGH_1BIT = 0b10000000;
    constexpr u8 HIGH_2BIT = 0b11000000;
    constexpr u8 HIGH_3BIT = 0b11100000;
    constexpr u8 HIGH_4BIT = 0b11110000;

    u16 combine(u8 high, u8 low);

    template <typename O, typename I>
    O reinterpret(const I& value) {
        return *reinterpret_cast<const O*>(&value);
    }

    template <typename O>
    void print_bits(O& out, const u8 byte) {
        for (size_t i = 0; i < 8; i++) {
            if (byte << i & HIGH_1BIT) out << "1";
            else out << "0";
        }
    }
}

template <typename T>
using vec = std::vector<T>;

template <typename T>
using uptr = std::unique_ptr<T>;

#endif //VM8086_UTILS_H
