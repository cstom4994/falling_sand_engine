
#ifndef ME_BASIC_TYPES_H
#define ME_BASIC_TYPES_H

#include <cstdint>
#include <functional>
#include <memory>

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

typedef unsigned char byte;

static_assert(std::numeric_limits<unsigned char>::digits == 8, "unsigned char must be 8 bits");
static_assert(sizeof(float) == 4, "float must be a 32-bit floating point number");
static_assert(sizeof(double) == 8, "double must be a 64-bit floating point number");

static_assert(static_cast<signed char>(128) == -128, "expected two's complement platform");

static_assert(std::numeric_limits<float>::is_iec559, "voxelio depends on IEC 559 floats");
static_assert(std::numeric_limits<double>::is_iec559, "voxelio depends on IEC 559 doubles");

using umax = std::uintmax_t;
using usize = std::size_t;
using pimax = std::intmax_t;
using pfmax = long double;
using argb32 = u32;

template <typename T, size_t size>
class size_checker {
    static_assert(sizeof(T) == size, "Check the size of integral types");
};

template class size_checker<i64, 8>;
template class size_checker<i32, 4>;
template class size_checker<i16, 2>;
template class size_checker<i8, 1>;
template class size_checker<u64, 8>;
template class size_checker<u32, 4>;
template class size_checker<u16, 2>;
template class size_checker<u8, 1>;

namespace ME {

static inline u16 swapuint16(u16 x) { return (x >> 8) | (x << 8); }

static inline u32 swapuint32(u32 x) { return ((x & 0x000000FF) << 24) | ((x & 0x0000FF00) << 8) | ((x & 0x00FF0000) >> 8) | ((x & 0xFF000000) >> 24); }

static inline u64 swapuint64(u64 x) {
    return ((x << 56) & 0xFF00000000000000ULL) | ((x << 40) & 0x00FF000000000000ULL) | ((x << 24) & 0x0000FF0000000000ULL) | ((x << 8) & 0x000000FF00000000ULL) | ((x >> 8) & 0x00000000FF000000ULL) |
           ((x >> 24) & 0x0000000000FF0000ULL) | ((x >> 40) & 0x000000000000FF00ULL) | ((x >> 56) & 0x00000000000000FFULL);
}

template <typename T>
struct ME_remove_reference {
    using type = T;
};

template <typename T>
struct ME_remove_reference<T &> {
    using type = T;
};

template <typename T>
struct ME_remove_reference<T &&> {
    using type = T;
};

template <typename T>
constexpr typename ME_remove_reference<T>::type &&ME_move(T &&arg) noexcept {
    return (typename ME_remove_reference<T>::type &&) arg;
}

template <typename T>
using initializer_list = std::initializer_list<T>;

template <typename T>
using function = std::function<T>;

template <typename T>
using scope = std::unique_ptr<T>;
template <typename T, typename... Args>
constexpr scope<T> create_scope(Args &&...args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T>
using ref = std::shared_ptr<T>;
template <typename T, typename... Args>
constexpr ref<T> create_ref(Args &&...args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}
}  // namespace ME

#endif