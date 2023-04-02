//
// Created by Vadim Gush on 31.03.2023.
//

#include "utils.h"

u16 bit::combine(const u8 high, const u8 low) {
    return (static_cast<u16>(high) << 8) | static_cast<u16>(low);
}
