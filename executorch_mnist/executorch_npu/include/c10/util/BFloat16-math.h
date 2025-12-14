#pragma once
#include <c10/util/BFloat16.h>
#include <cmath>
namespace c10 {
inline bool isnan(BFloat16 v) { return std::isnan(static_cast<float>(v)); }
inline bool isinf(BFloat16 v) { return std::isinf(static_cast<float>(v)); }
inline BFloat16 abs(BFloat16 v) { return BFloat16(std::abs(static_cast<float>(v))); }
} // namespace c10
