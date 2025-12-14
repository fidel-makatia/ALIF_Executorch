#pragma once
#include <cstdint>
#include <cmath>

// Include BFloat16.h first to get c10::detail functions
#include <c10/util/BFloat16.h>

namespace c10 {

struct alignas(2) Half {
    uint16_t x;
    Half() = default;

    explicit Half(float f) {
        x = detail::fp16_ieee_from_fp32_value(f);
    }

    operator float() const {
        return detail::fp16_ieee_to_fp32_value(x);
    }
};

} // namespace c10
