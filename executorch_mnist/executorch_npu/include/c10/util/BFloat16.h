// BFloat16 stub for embedded systems with required c10::detail functions
#pragma once
#include <cstdint>
#include <cmath>

namespace c10 {
namespace detail {

// Convert raw bits to float32
inline float f32_from_bits(uint32_t bits) {
    union { uint32_t i; float f; } u;
    u.i = bits;
    return u.f;
}

// Convert float32 to raw bits
inline uint32_t fp32_to_bits(float f) {
    union { float f; uint32_t i; } u;
    u.f = f;
    return u.i;
}

inline float fp32_from_bits(uint32_t bits) {
    return f32_from_bits(bits);
}

// Round to nearest even for bfloat16 conversion
inline uint16_t round_to_nearest_even(float f) {
    uint32_t bits = fp32_to_bits(f);
    // Check for NaN
    if ((bits & 0x7FFFFFFF) > 0x7F800000) {
        return static_cast<uint16_t>((bits >> 16) | 0x40);  // Quiet NaN
    }
    // Round to nearest even
    uint32_t rounding_bias = ((bits >> 16) & 1) + 0x7FFF;
    return static_cast<uint16_t>((bits + rounding_bias) >> 16);
}

// FP16 conversion functions
inline float fp16_ieee_to_fp32_value(uint16_t h) {
    uint32_t sign = static_cast<uint32_t>(h & 0x8000) << 16;
    uint32_t exp = (h >> 10) & 0x1F;
    uint32_t mantissa = h & 0x3FF;

    if (exp == 0) {
        if (mantissa == 0) {
            return f32_from_bits(sign);  // Signed zero
        }
        // Denormal
        while ((mantissa & 0x400) == 0) {
            mantissa <<= 1;
            exp--;
        }
        exp++;
        mantissa &= 0x3FF;
    } else if (exp == 31) {
        // Inf or NaN
        return f32_from_bits(sign | 0x7F800000 | (mantissa << 13));
    }

    return f32_from_bits(sign | ((exp + 112) << 23) | (mantissa << 13));
}

inline uint32_t fp16_ieee_to_fp32_bits(uint16_t h) {
    return fp32_to_bits(fp16_ieee_to_fp32_value(h));
}

inline uint16_t fp16_ieee_from_fp32_value(float f) {
    uint32_t bits = fp32_to_bits(f);
    uint32_t sign = (bits >> 16) & 0x8000;
    int32_t exp = ((bits >> 23) & 0xFF) - 127;
    uint32_t mantissa = bits & 0x7FFFFF;

    if (exp > 15) {
        // Overflow to infinity
        return static_cast<uint16_t>(sign | 0x7C00);
    } else if (exp < -14) {
        // Underflow to zero
        return static_cast<uint16_t>(sign);
    } else if (exp < -10) {
        // Denormal
        mantissa = (mantissa | 0x800000) >> (1 - exp - 10);
        return static_cast<uint16_t>(sign | (mantissa >> 13));
    }

    return static_cast<uint16_t>(sign | ((exp + 15) << 10) | (mantissa >> 13));
}

} // namespace detail

struct alignas(2) BFloat16 {
    uint16_t x;

    BFloat16() = default;

    explicit BFloat16(float f) {
        x = detail::round_to_nearest_even(f);
    }

    operator float() const {
        return detail::f32_from_bits(static_cast<uint32_t>(x) << 16);
    }
};

// Overflow checking for safe math
template<typename T>
inline bool mul_overflows(T a, T b, T* result) {
    *result = a * b;
    if (a == 0 || b == 0) return false;
    return (*result / a) != b;
}

} // namespace c10
