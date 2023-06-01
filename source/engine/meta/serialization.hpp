
#ifndef ME_SERIALIZATION_HPP
#define ME_SERIALIZATION_HPP

#include <stdint.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <exception>
#include <list>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace ME::meta {

namespace EndianSwapper {
class SwapByteBase {
public:
    static bool ShouldSwap() {
        static const uint16_t swapTest = 1;
        return (*((char*)&swapTest) == 1);
    }

    static void SwapBytes(uint8_t& v1, uint8_t& v2) {
        uint8_t tmp = v1;
        v1 = v2;
        v2 = tmp;
    }
};

template <class T, int S>
class SwapByte : public SwapByteBase {
public:
    static T Swap(T v) {
        assert(false);  // Shoud not be here...
        return v;
    }
};

template <class T>
class SwapByte<T, 1> : public SwapByteBase {
public:
    static T Swap(T v) { return v; }
};

template <class T>
class SwapByte<T, 2> : public SwapByteBase {
public:
    static T Swap(T v) {
        if (ShouldSwap()) return ((uint16_t)v >> 8) | ((uint16_t)v << 8);
        return v;
    }
};

template <class T>
class SwapByte<T, 4> : public SwapByteBase {
public:
    static T Swap(T v) {
        if (ShouldSwap()) {
            return (SwapByte<uint16_t, 2>::Swap((uint32_t)v & 0xffff) << 16) | (SwapByte<uint16_t, 2>::Swap(((uint32_t)v & 0xffff0000) >> 16));
        }
        return v;
    }
};

template <class T>
class SwapByte<T, 8> : public SwapByteBase {
public:
    static T Swap(T v) {
        if (ShouldSwap()) return (((uint64_t)SwapByte<uint32_t, 4>::Swap((uint32_t)(v & 0xffffffffull))) << 32) | (SwapByte<uint32_t, 4>::Swap((uint32_t)(v >> 32)));
        return v;
    }
};

template <>
class SwapByte<float, 4> : public SwapByteBase {
public:
    static float Swap(float v) {
        union {
            float f;
            uint8_t c[4];
        };
        f = v;
        if (ShouldSwap()) {
            SwapBytes(c[0], c[3]);
            SwapBytes(c[1], c[2]);
        }
        return f;
    }
};

template <>
class SwapByte<double, 8> : public SwapByteBase {
public:
    static double Swap(double v) {
        union {
            double f;
            uint8_t c[8];
        };
        f = v;
        if (ShouldSwap()) {
            SwapBytes(c[0], c[7]);
            SwapBytes(c[1], c[6]);
            SwapBytes(c[2], c[5]);
            SwapBytes(c[3], c[4]);
        }
        return f;
    }
};
}  // namespace EndianSwapper

template <class STREAM_TYPE>
class archive {
public:
    archive(STREAM_TYPE& stream) : m_stream(stream) {}

public:
    template <class T>
    const archive& operator<<(const T& v) const {
        *this& v;
        return *this;
    }

    template <class T>
    archive& operator>>(T& v) {
        *this& v;
        return *this;
    }

public:
    template <class T>
    archive& operator&(T& v) {
        v.Serialize(*this);
        return *this;
    }

    template <class T>
    const archive& operator&(const T& v) const {
        ((T&)v).Serialize(*this);
        return *this;
    }

    template <class T, size_t N>
    archive& operator&(T (&v)[N]) {
        uint32_t len;
        *this& len;
        for (size_t i = 0; i < N; ++i) *this& v[i];
        return *this;
    }

    template <class T, size_t N>
    const archive& operator&(const T (&v)[N]) const {
        uint32_t len = N;
        *this& len;
        for (size_t i = 0; i < N; ++i) *this& v[i];
        return *this;
    }

#define SERIALIZER_FOR_POD(type)                        \
    archive& operator&(type& v) {                       \
        m_stream.read((char*)&v, sizeof(type));         \
        if (!m_stream) {                                \
            throw std::runtime_error("malformed data"); \
        }                                               \
        v = Swap(v);                                    \
        return *this;                                   \
    }                                                   \
    const archive& operator&(type v) const {            \
        v = Swap(v);                                    \
        m_stream.write((const char*)&v, sizeof(type));  \
        return *this;                                   \
    }

    // Special serializer for floating point values, can't directly swap on floating point variable
    // because it might cause a NaN and the wrong value is read back (the problem occurs on vstudio)
#define SERIALIZER_FOR_POD_FLOATINGPOINT(type)                                                                                \
    archive& operator&(type& v) {                                                                                             \
        union {                                                                                                               \
            type f;                                                                                                           \
            uint8_t c[sizeof(type)];                                                                                          \
        };                                                                                                                    \
        m_stream.read((char*)&c[0], sizeof(type));                                                                            \
        if (!m_stream) {                                                                                                      \
            throw std::runtime_error("malformed data");                                                                       \
        }                                                                                                                     \
        if (EndianSwapper::SwapByteBase::ShouldSwap()) {                                                                      \
            for (int i = 0; i < sizeof(type) / 2; ++i) EndianSwapper::SwapByteBase::SwapBytes(c[i], c[sizeof(type) - 1 - i]); \
        }                                                                                                                     \
        v = f;                                                                                                                \
        return *this;                                                                                                         \
    }                                                                                                                         \
    const archive& operator&(type v) const {                                                                                  \
        union {                                                                                                               \
            type f;                                                                                                           \
            uint8_t c[sizeof(type)];                                                                                          \
        };                                                                                                                    \
        f = v;                                                                                                                \
        if (EndianSwapper::SwapByteBase::ShouldSwap()) {                                                                      \
            for (int i = 0; i < sizeof(type) / 2; ++i) EndianSwapper::SwapByteBase::SwapBytes(c[i], c[sizeof(type) - 1 - i]); \
        }                                                                                                                     \
        m_stream.write((const char*)&c[0], sizeof(type));                                                                     \
        return *this;                                                                                                         \
    }

    SERIALIZER_FOR_POD(bool)
    SERIALIZER_FOR_POD(char)
    SERIALIZER_FOR_POD(unsigned char)
    SERIALIZER_FOR_POD(short)
    SERIALIZER_FOR_POD(unsigned short)
    SERIALIZER_FOR_POD(int)
    SERIALIZER_FOR_POD(unsigned int)
    SERIALIZER_FOR_POD(long)
    SERIALIZER_FOR_POD(unsigned long)
    SERIALIZER_FOR_POD(long long)
    SERIALIZER_FOR_POD(unsigned long long)
    SERIALIZER_FOR_POD_FLOATINGPOINT(float)
    SERIALIZER_FOR_POD_FLOATINGPOINT(double)

#define SERIALIZER_FOR_STL(type)                                                               \
    template <class T>                                                                         \
    archive& operator&(type<T>& v) {                                                           \
        uint32_t len;                                                                          \
        *this& len;                                                                            \
        for (uint32_t i = 0; i < len; ++i) {                                                   \
            T value;                                                                           \
            *this& value;                                                                      \
            v.insert(v.end(), value);                                                          \
        }                                                                                      \
        return *this;                                                                          \
    }                                                                                          \
    template <class T>                                                                         \
    const archive& operator&(const type<T>& v) const {                                         \
        uint32_t len = v.size();                                                               \
        *this& len;                                                                            \
        for (typename type<T>::const_iterator it = v.begin(); it != v.end(); ++it) *this&* it; \
        return *this;                                                                          \
    }

#define SERIALIZER_FOR_STL2(type)                                                                   \
    template <class T1, class T2>                                                                   \
    archive& operator&(type<T1, T2>& v) {                                                           \
        uint32_t len;                                                                               \
        *this& len;                                                                                 \
        for (uint32_t i = 0; i < len; ++i) {                                                        \
            std::pair<T1, T2> value;                                                                \
            *this& value;                                                                           \
            v.insert(v.end(), value);                                                               \
        }                                                                                           \
        return *this;                                                                               \
    }                                                                                               \
    template <class T1, class T2>                                                                   \
    const archive& operator&(const type<T1, T2>& v) const {                                         \
        uint32_t len = v.size();                                                                    \
        *this& len;                                                                                 \
        for (typename type<T1, T2>::const_iterator it = v.begin(); it != v.end(); ++it) *this&* it; \
        return *this;                                                                               \
    }

    SERIALIZER_FOR_STL(std::vector)
    SERIALIZER_FOR_STL(std::deque)
    SERIALIZER_FOR_STL(std::list)
    SERIALIZER_FOR_STL(std::set)
    SERIALIZER_FOR_STL(std::multiset)
    SERIALIZER_FOR_STL2(std::map)
    SERIALIZER_FOR_STL2(std::multimap)

    template <class T1, class T2>
    archive& operator&(std::pair<T1, T2>& v) {
        *this& v.first& v.second;
        return *this;
    }

    template <class T1, class T2>
    const archive& operator&(const std::pair<T1, T2>& v) const {
        *this& v.first& v.second;
        return *this;
    }

    archive& operator&(std::string& v) {
        uint32_t len;
        *this& len;
        v.clear();
        char buffer[4096];
        uint32_t toRead = len;
        while (toRead != 0) {
            uint32_t l = std::min(toRead, (uint32_t)sizeof(buffer));
            m_stream.read(buffer, l);
            if (!m_stream) throw std::runtime_error("malformed data");
            v += std::string(buffer, l);
            toRead -= l;
        }
        return *this;
    }

    const archive& operator&(const std::string& v) const {
        uint32_t len = v.length();
        *this& len;
        m_stream.write(v.c_str(), len);
        return *this;
    }

private:
    template <class T>
    T Swap(const T& v) const {
        return EndianSwapper::SwapByte<T, sizeof(T)>::Swap(v);
    }

private:
    STREAM_TYPE& m_stream;
};

}  // namespace ME::meta

#endif
