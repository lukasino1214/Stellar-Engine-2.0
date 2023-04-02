#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <string_view>
#include <string>
#include <memory>

namespace Stellar {
    inline namespace Types {
        using u8 = std::uint8_t;
        using u16 = std::uint16_t;
        using u32 = std::uint32_t;
        using u64 = std::uint64_t;
        using usize = std::size_t;
        using b32 = u32;

        using i8 = std::int8_t;
        using i16 = std::int16_t;
        using i32 = std::int32_t;
        using i64 = std::int64_t;
        using isize = std::ptrdiff_t;

        using f32 = float;
        using f64 = double;
    }
}