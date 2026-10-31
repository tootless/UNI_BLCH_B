#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <type_traits>
#include <cstddef>

template <typename T>
void append_bits(std::vector<uint8_t>& out, T value)
{
    for (size_t i = 0; i < sizeof(T); ++i) {
        out.push_back(static_cast<uint8_t>(value & 0xFF));
        value >>= 8;
    }
}

inline void append_hash(
    std::vector<uint8_t>& out,
    const std::array<uint32_t, 8>& hash)
{
    for (uint32_t word : hash) {
        append_bits(out, word);
    }
}