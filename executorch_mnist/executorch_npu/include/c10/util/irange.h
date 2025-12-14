// Minimal stub for c10::irange - provides Python-like range iteration
#pragma once
#include <cstddef>
#include <iterator>

namespace c10 {

template <typename T>
class integer_range {
public:
    class iterator {
    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;
        using iterator_category = std::forward_iterator_tag;

        explicit iterator(T value) : value_(value) {}
        T operator*() const { return value_; }
        iterator& operator++() { ++value_; return *this; }
        iterator operator++(int) { auto tmp = *this; ++value_; return tmp; }
        bool operator==(const iterator& other) const { return value_ == other.value_; }
        bool operator!=(const iterator& other) const { return value_ != other.value_; }
    private:
        T value_;
    };

    integer_range(T begin, T end) : begin_(begin), end_(end) {}
    iterator begin() const { return iterator(begin_); }
    iterator end() const { return iterator(end_); }

private:
    T begin_;
    T end_;
};

template <typename T>
integer_range<T> irange(T end) {
    return integer_range<T>(T(0), end);
}

template <typename T>
integer_range<T> irange(T begin, T end) {
    return integer_range<T>(begin, end);
}

} // namespace c10
