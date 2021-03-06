#ifndef RASTER_TYPES_UNIQUE_PTR_HH
#define RASTER_TYPES_UNIQUE_PTR_HH

/**
 * Provides an implementation of the Matrix concept for objects managed by C++11 unique_ptrs.
 */

#include <memory>

namespace raster {
    template <typename T>
    auto rows(const std::unique_ptr<T> &ptr) -> decltype(rows(*ptr)) {
        return rows(*ptr);
    }
    
    template <typename T>
    auto columns(const std::unique_ptr<T> &ptr) -> decltype(columns(*ptr)) {
        return columns(*ptr);
    }
    
    template <typename T, typename U>
    auto get(const std::unique_ptr<T> &ptr, U y, U x) -> decltype(get(*ptr, y, x)) {
        return get(*ptr, y, x);
    }
    
    template <typename T, typename U, typename V>
    auto set(const std::unique_ptr<T> &ptr, U y, U x, V v) -> decltype(set(*ptr, y, x, v)) {
        return set(*ptr, y, x, v);
    }
}

#endif /* #ifndef RASTER_TYPES_UNIQUE_PTR_HH */

