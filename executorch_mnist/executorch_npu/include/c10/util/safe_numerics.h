#pragma once
#include <cstddef>
#include <limits>

namespace c10 {
template <typename To, typename From>
inline bool check_integercast_overflow(From val) {
    return val < std::numeric_limits<To>::min() || val > std::numeric_limits<To>::max();
}
} // namespace c10
