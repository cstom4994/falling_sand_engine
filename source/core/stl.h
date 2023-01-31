

#ifndef METAENGINE_ARRAY_H
#define METAENGINE_ARRAY_H

#include <inttypes.h>
#include <stdarg.h>

#include "core/alloc.hpp"
#include "core/core.h"
#include "libs/cute/cute_sync.h"

#pragma region c_func

#ifndef METAENGINE_MEMCPY
#include <string.h>
#define METAENGINE_MEMCPY memcpy
#endif

#ifndef METAENGINE_MEMMOVE
#include <string.h>
#define METAENGINE_MEMMOVE memmove
#endif

#ifndef METAENGINE_MEMSET
#include <string.h>
#define METAENGINE_MEMSET memset
#endif

#ifndef METAENGINE_STRCPY
#include <string.h>
#define METAENGINE_STRCPY strcpy
#endif

#ifndef METAENGINE_STRNCPY
#include <string.h>
#define METAENGINE_STRNCPY strncpy
#endif

#ifndef METAENGINE_STRLEN
#include <string.h>
#define METAENGINE_STRLEN strlen
#endif

#ifndef METAENGINE_MEMCHR
#include <string.h>
#define METAENGINE_MEMCHR memchr
#endif

#ifndef METAENGINE_STRSTR
#include <string.h>
#define METAENGINE_STRSTR strstr
#endif

#ifndef METAENGINE_MEMCMP
#include <string.h>
#define METAENGINE_MEMCMP memcmp
#endif

#ifndef METAENGINE_STRCMP
#include <string.h>
#define METAENGINE_STRCMP strcmp
#endif

#ifndef METAENGINE_STRICMP
#include <string.h>
#ifdef _WIN32
#define METAENGINE_STRICMP stricmp
#else
#define METAENGINE_STRICMP strcasecmp
#endif
#endif

#ifndef METAENGINE_STRDUP
#include <string.h>
#define METAENGINE_STRDUP strdup
#endif

#ifndef METAENGINE_STRNCMP
#include <string.h>
#define METAENGINE_STRNCMP strncmp
#endif

#ifndef METAENGINE_STRNICMP
#include <string.h>
#ifdef _WIN32
#define METAENGINE_STRNICMP strnicmp
#else
#define METAENGINE_STRNICMP strncasecmp
#endif
#endif

#ifndef METAENGINE_SNPRINTF
#include <stdio.h>
#define METAENGINE_SNPRINTF snprintf
#endif

#ifndef METAENGINE_STRTOLL
#include <stdlib.h>
#define METAENGINE_STRTOLL strtoll
#endif

#ifndef METAENGINE_STRTOD
#include <stdlib.h>
#define METAENGINE_STRTOD strtod
#endif

#ifndef METAENGINE_TOLOWER
#include <ctype.h>
#define METAENGINE_TOLOWER tolower
#endif

#ifndef METAENGINE_TOUPPER
#include <ctype.h>
#define METAENGINE_TOUPPER toupper
#endif

#ifndef METAENGINE_STRCHR
#include <string.h>
#define METAENGINE_STRCHR strchr
#endif

#ifndef METAENGINE_STRRCHR
#include <string.h>
#define METAENGINE_STRRCHR strrchr
#endif

#pragma endregion c_func

#pragma region array

//--------------------------------------------------------------------------------------------------
// C API

#ifndef METAENGINE_NO_SHORTHAND_API
/**
 * This is *optional* and _completely_ empty macro. It's only purpose is to provide a bit of visual
 * indication a type is a dynamic array. One downside of the C-macro API is the opaque nature of the pointer
 * type. Since the macros use polymorphism on typed pointers, there's no actual array struct type.
 *
 * It can get really annoying to sometimes forget if a pointer is an array, a hashtable, or just a
 * pointer. This macro can be used to markup the type to make it much more clear for function parameters
 * or struct member definitions. It's saying "Hey, I'm a dynamic array!" to mitigate this downside.
 */
#define dyna

/**
 * Gets the number of elements in the array. Must not be NULL.
 * It's a proper l-value so you can assign or increment it.
 *
 * Example:
 *
 *     int* a = NULL;
 *     apush(a, 5);
 *     METADOT_ASSERT_E(alen(a) == 1);
 *     alen(a)--;
 *     METADOT_ASSERT_E(alen(a) == 0);
 *     afree(a);
 */
#define alen(a) metadot_array_len(a)

/**
 * Gets the number of elements in the array. Can be NULL.
 *
 * Example:
 *
 *     int* a = NULL;
 *     apush(a, 5);
 *     METADOT_ASSERT_E(asize(a) == 1);
 *     afree(a);
 */
#define asize(a) metadot_array_size(a)

/**
 * Gets the number of elements in the array. Can be NULL.
 *
 * Example:
 *
 *     int* a = NULL;
 *     apush(a, 5);
 *     METADOT_ASSERT_E(acount(a) == 1);
 *     afree(a);
 */
#define acount(a) metadot_array_count(a)

/**
 * Gets the capacity of the array. The capacity automatically grows if the size
 * of the array grows over the capacity. You can use `afit` to ensure a minimum
 * capacity as an optimization.
 */
#define acap(a) metadot_array_capacity(a)

/**
 * Ensures the capacity of the array is at least n elements.
 * Does not change the size/count of the array.
 */
#define afit(a, n) metadot_array_fit(a, n)

/**
 * Pushes an element onto the back of the array.
 *
 * a    - The array. If NULL a new array will get allocated and returned.
 * ...  - The value to push onto the back of the array.
 *
 * Example:
 *
 *     int* a = NULL;
 *     apush(a, 5);
 *     apush(a, 13);
 *     METADOT_ASSERT_E(a[0] == 5);
 *     METADOT_ASSERT_E(a[1] == 13);
 *     METADOT_ASSERT_E(asize(a) == 2);
 *     afree(a);
 */
#define apush(a, ...) metadot_array_push(a, (__VA_ARGS__))

/**
 * Pops and returns an element off the back of the array. Cannot be NULL.
 */
#define apop(a) metadot_array_pop(a)

/**
 * Returns a pointer one element beyond the end of the array.
 */
#define aend(a) metadot_array_end(a)

/**
 * Returns the last element in the array.
 */
#define alast(a) metadot_array_last(a)

/**
 * Sets the array's count to zero. Does not free any resources.
 */
#define aclear(a) metadot_array_clear(a)

/**
 * Copies the array b into array a. Will automatically fit a if needed.
 */
#define aset(a, b) metadot_array_set(a, b)

/**
 * Returns the hash of all the bytes in the array.
 */
#define ahash(a) metadot_array_hash(a)

/**
 * Creates an array with an initial static storage backing. Will grow onto the heap
 * if the size becomes too large.
 *
 * a           - A typed pointer, can be NULL. Will be assigned + returnde back to you.
 * buffer      - Pointer to a static memory buffer.
 * buffer_size - The size of `buffer` in bytes.
 */
#define astatic(a, buffer, buffer_size) metadot_array_static(a, buffer, buffer_size)

/**
 * Frees up all resources used by the array. Sets a to NULL.
 */
#define afree(a) metadot_array_free(a)
#endif  // METAENGINE_NO_SHORTHAND_API

//--------------------------------------------------------------------------------------------------
// Longform C API.

#define metadot_array_len(a) (METAENGINE_ACANARY(a), METAENGINE_AHDR(a)->size)
#define metadot_array_size(a) (a ? metadot_array_len(a) : 0)
#define metadot_array_count(a) metadot_array_size(a)
#define metadot_array_capacity(a) ((a) ? METAENGINE_AHDR(a)->capacity : 0)
#define metadot_array_fit(a, n) ((n) <= metadot_array_capacity(a) ? 0 : (*(void**)&(a) = metadot_agrow((a), (n), sizeof(*a))))
#define metadot_array_push(a, ...) (METAENGINE_ACANARY(a), metadot_array_fit((a), 1 + ((a) ? metadot_array_len(a) : 0)), (a)[metadot_array_len(a)++] = (__VA_ARGS__))
#define metadot_array_pop(a) (a[--metadot_array_len(a)])
#define metadot_array_end(a) (a + metadot_array_size(a))
#define metadot_array_last(a) (a[metadot_array_len(a) - 1])
#define metadot_array_clear(a) (METAENGINE_ACANARY(a), (a) ? metadot_array_len(a) = 0 : 0)
#define metadot_array_set(a, b) (*(void**)&(a) = metadot_aset((void*)(a), (void*)(b), sizeof(*a)))
#define metadot_array_hash(a) metadot_fnv1a(a, metadot_array_size(a))
#define metadot_array_static(a, buffer, buffer_size) (*(void**)&(a) = metadot_astatic(buffer, buffer_size, sizeof(*a)))
#define metadot_array_free(a)                                                            \
    do {                                                                                 \
        METAENGINE_ACANARY(a);                                                           \
        if (a && !METAENGINE_AHDR(a)->is_static) METAENGINE_FW_FREE(METAENGINE_AHDR(a)); \
        a = NULL;                                                                        \
    } while (0)

//--------------------------------------------------------------------------------------------------
// Hidden API - Not intended for direct use.

#define METAENGINE_AHDR(a) ((METAENGINE_Ahdr*)a - 1)
#define METAENGINE_ACOOKIE 0xE6F7E359
#define METAENGINE_ACANARY(a) ((a) ? METADOT_ASSERT_E(METAENGINE_AHDR(a)->cookie == METAENGINE_ACOOKIE) : (void)0)  // Detects buffer underruns.

// *Hidden* array header.
typedef struct METAENGINE_Ahdr {
    int size;
    int capacity;
    bool is_static;
    char* data;
    uint32_t cookie;
} METAENGINE_Ahdr;

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

void* METADOT_CDECL metadot_agrow(const void* a, int new_size, size_t element_size);
void* METADOT_CDECL metadot_astatic(const void* a, int capacity, size_t element_size);
void* METADOT_CDECL metadot_aset(const void* a, const void* b, size_t element_size);

#ifdef __cplusplus
}
#endif  // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

/**
 * Implements a basic growable array data structure. Constructors and destructors are called, but this
 * class does *not* act as a drop-in replacement for std::vector, as there are no iterators. Your elements
 * CAN NOT store a pointer or reference to themselves or other elements.
 *
 * The main purpose of this class is to reduce the lines of code included compared to std::vector,
 * and also more importantly to have fast debug performance.
 */

namespace MetaEngine {

template <typename T>
struct Array {
    Array() {}
    Array(METAENGINE_InitializerList<T> list);
    Array(const Array<T>& other);
    Array(Array<T>&& other);
    Array(int capacity);
    ~Array();

    T& add();
    T& add(const T& item);
    T& add(T&& item);
    T pop();
    void unordered_remove(int index);
    void clear();
    void ensure_capacity(int num_elements);
    void ensure_count(int count);
    void set_count(int count);
    void reverse();

    int capacity() const;
    int count() const;
    int size() const;
    bool empty() const;

    T* begin();
    const T* begin() const;
    T* end();
    const T* end() const;

    T& operator[](int index);
    const T& operator[](int index) const;

    T* operator+(int index);
    const T* operator+(int index) const;

    Array<T>& operator=(const Array<T>& rhs);
    Array<T>& operator=(Array<T>&& rhs);
    Array<T>& steal_from(Array<T>* steal_from_me);
    Array<T>& steal_from(Array<T>& steal_from_me);

    T& last();
    const T& last() const;

    T* data();
    const T* data() const;

private:
    T* m_ptr = NULL;
};

// -------------------------------------------------------------------------------------------------

template <typename T>
Array<T>::Array(METAENGINE_InitializerList<T> list) {
    afit(m_ptr, (int)list.size());
    for (const T* i = list.begin(); i < list.end(); ++i) {
        add(*i);
    }
}

template <typename T>
Array<T>::Array(const Array<T>& other) {
    afit(m_ptr, (int)other.count());
    for (int i = 0; i < other.count(); ++i) {
        add(other[i]);
    }
}

template <typename T>
Array<T>::Array(Array<T>&& other) {
    steal_from(&other);
}

template <typename T>
Array<T>::Array(int capacity) {
    afit(m_ptr, capacity);
}

template <typename T>
Array<T>::~Array() {
    int len = asize(m_ptr);
    for (int i = 0; i < len; ++i) {
        T* slot = m_ptr + i;
        slot->~T();
    }
    afree(m_ptr);
}

template <typename T>
Array<T>& Array<T>::steal_from(Array<T>* steal_from_me) {
    afree(m_ptr);
    m_ptr = steal_from_me->m_ptr;
    steal_from_me->m_ptr = NULL;
    return *this;
}

template <typename T>
Array<T>& Array<T>::steal_from(Array<T>& steal_from_me) {
    afree(m_ptr);
    m_ptr = steal_from_me.m_ptr;
    steal_from_me.m_ptr = NULL;
    return *this;
}

template <typename T>
T& Array<T>::add() {
    afit(m_ptr, asize(m_ptr) + 1);
    T* slot = m_ptr + alen(m_ptr)++;
    METAENGINE_FW_PLACEMENT_NEW(slot) T;
    return *slot;
}

template <typename T>
T& Array<T>::add(const T& item) {
    afit(m_ptr, asize(m_ptr) + 1);
    T* slot = m_ptr + alen(m_ptr)++;
    METAENGINE_FW_PLACEMENT_NEW(slot) T(item);
    return *slot;
}

template <typename T>
T& Array<T>::add(T&& item) {
    afit(m_ptr, asize(m_ptr) + 1);
    T* slot = m_ptr + alen(m_ptr)++;
    METAENGINE_FW_PLACEMENT_NEW(slot) T(metadot_move(item));
    return *slot;
}

template <typename T>
T Array<T>::pop() {
    METADOT_ASSERT_E(!empty());
    T* slot = m_ptr + alen(m_ptr) - 1;
    T val = move(apop(m_ptr));
    slot->~T();
    return val;
}

template <typename T>
void Array<T>::unordered_remove(int index) {
    METADOT_ASSERT_E(index >= 0 && index < asize(m_ptr));
    T* slot = m_ptr + index;
    slot->~T();
    m_ptr[index] = m_ptr[--alen(m_ptr)];
}

template <typename T>
void Array<T>::clear() {
    aclear(m_ptr);
}

template <typename T>
void Array<T>::ensure_capacity(int num_elements) {
    afit(m_ptr, num_elements);
}

template <typename T>
void Array<T>::set_count(int count) {
    METADOT_ASSERT_E(count < acap(m_ptr) || !count);
    afit(m_ptr, count);
    if (asize(m_ptr) > count) {
        for (int i = count; i < asize(m_ptr); ++i) {
            T* slot = m_ptr + i;
            slot->~T();
        }
    } else if (asize(m_ptr) < count) {
        for (int i = asize(m_ptr); i < count; ++i) {
            T* slot = m_ptr + i;
            METAENGINE_FW_PLACEMENT_NEW(slot) T;
        }
    }
    if (m_ptr) alen(m_ptr) = count;
}

template <typename T>
void Array<T>::ensure_count(int count) {
    if (!count) return;
    int old_count = asize(m_ptr);
    afit(m_ptr, count);
    if (alen(m_ptr) < count) {
        alen(m_ptr) = count;
        for (int i = old_count; i < count; ++i) {
            T* slot = m_ptr + i;
            METAENGINE_FW_PLACEMENT_NEW(slot) T;
        }
    }
}

template <typename T>
void Array<T>::reverse() {
    T* a = m_ptr;
    T* b = m_ptr + (asize(m_ptr) - 1);

    while (a < b) {
        T t = *a;
        *a = *b;
        *b = t;
        ++a;
        --b;
    }
}

template <typename T>
int Array<T>::capacity() const {
    return acap(m_ptr);
}

template <typename T>
int Array<T>::count() const {
    return asize(m_ptr);
}

template <typename T>
int Array<T>::size() const {
    return asize(m_ptr);
}

template <typename T>
bool Array<T>::empty() const {
    int size = asize(m_ptr);
    return size <= 0;
}

template <typename T>
T* Array<T>::begin() {
    return m_ptr;
}

template <typename T>
const T* Array<T>::begin() const {
    return m_ptr;
}

template <typename T>
T* Array<T>::end() {
    return m_ptr + count();
}

template <typename T>
const T* Array<T>::end() const {
    return m_ptr + count();
}

template <typename T>
T& Array<T>::operator[](int index) {
    METADOT_ASSERT_E(index >= 0 && index < count());
    return m_ptr[index];
}

template <typename T>
const T& Array<T>::operator[](int index) const {
    METADOT_ASSERT_E(index >= 0 && index < count());
    return m_ptr[index];
}

template <typename T>
T* Array<T>::data() {
    return m_ptr;
}

template <typename T>
const T* Array<T>::data() const {
    return m_ptr;
}

template <typename T>
T* Array<T>::operator+(int index) {
    METADOT_ASSERT_E(index >= 0 && index < count());
    return m_ptr + index;
}

template <typename T>
const T* Array<T>::operator+(int index) const {
    METADOT_ASSERT_E(index >= 0 && index < count());
    return m_ptr + index;
}

template <typename T>
Array<T>& Array<T>::operator=(const Array<T>& rhs) {
    set_count(0);
    afit(m_ptr, (int)rhs.count());
    for (int i = 0; i < rhs.count(); ++i) {
        add(rhs[i]);
    }
    return *this;
}

template <typename T>
Array<T>& Array<T>::operator=(Array<T>&& rhs) {
    afree(m_ptr);
    m_ptr = rhs->m_ptr;
    rhs->m_ptr = NULL;
}

template <typename T>
T& Array<T>::last() {
    return *(aend(m_ptr) - 1);
}

template <typename T>
const T& Array<T>::last() const {
    return *(aend(m_ptr) - 1);
}

}  // namespace MetaEngine

#pragma endregion array

#pragma region string

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// General purpose C-string API. 100% compatible with normal C-strings.
// Automatically grows on the heap as needed.
// Free it up with `sfree` when done.
//
// Example:
//
//     char* s = NULL;
//     sset(s, "Hello world!");
//     printf("%s", s);
//     sfree(s);

#ifndef METAENGINE_NO_SHORTHAND_API
/**
 * Gets the number of characters in the string, not counting the nul-terminator. Can be NULL.
 */
#define slen(s) metadot_string_len(s)

/**
 * Returns whether or not the string is empty. Both "" and NULL count as empty.
 */
#define sempty(s) metadot_string_empty(s)

/**
 * Pushes character `ch` onto the end of the string (does not overwite the nul-byte).
 * If the string is empty a nul-byte is pushed afterwards.
 * Can be NULL, will create a new string and assign `s` if so.
 */
#define spush(s, ch) metadot_string_push(s, ch)

/**
 * Frees up all resources used by the string and sets it to NULL.
 */
#define sfree(s) metadot_string_free(s)

/**
 * Gets the number of characters in the string. Must not be NULL.
 * Returns a proper l-value, so you can assign or increment it.
 *
 * Example:
 *
 *     char* s = NULL;
 *     spush(s, 'a');
 *     METADOT_ASSERT_E(string_size(s) == 1);
 *     string_size(s)--;
 *     METADOT_ASSERT_E(string_size(a) == 0);
 *     sfree(string_size);
 */
#define string_size(s) metadot_string_size(s)

/**
 * Gets the number of characters in the string. Cannot be NULL.
 */
#define scount(s) metadot_string_count(s)

/**
 * Gets the capacity of the string. Can be NULL.
 * This is not the number of characters, but the size of the internal buffer.
 * The capacity automatically grows as necessary, but you can use `sfit` to ensure
 * a minimum capacity manually, as an optimization.
 */
#define scap(s) metadot_string_cap(s)

/**
 * Returns the last character in the string. Not the nul-byte. Undefined for empty strings.
 */
#define slast(s) metadot_string_last(s)

/**
 * Sets the string size to zero. Does not free up any resources.
 */
#define sclear(s) metadot_string_clear(s)

/**
 * Ensures the capacity of the string is at least n elements.
 * Does not change the size/count of the string, or the len.
 * Can be NULL.
 */
#define sfit(s, n) metadot_string_fit(s, n)

/**
 * Printf's into the string using the format string `fmt`.
 * The string will be overwritten from the beginning.
 * Will automatically adjust capacity as needed.
 */
#define sfmt(s, fmt, ...) metadot_string_fmt(s, fmt, (__VA_ARGS__))

/**
 * Printf's into the *end* of the string, using the format string `fmt`.
 * All printed data is appended to the end of the string.
 * Will automatically adjust it's capacity as needed.
 */
#define sfmt_append(s, fmt, ...) metadot_string_fmt_append(s, fmt, (__VA_ARGS__))

/**
 * Printf's into the string using the format string `fmt`.
 * The string will be overwritten from the beginning.
 * Will automatically adjust it's capacity as needed.
 * args must be a `va_list`.
 * You probably are looking for `sfmt` instead.
 */
#define svfmt(s, fmt, args) metadot_string_vfmt(s, fmt, args)

/**
 * Printf's into the *end* of the string, using the format string `fmt`.
 * All printed data is appended to the end of the string.
 * Will automatically adjust it's capacity as needed.
 * args must be a `va_list`.
 * You probably are looking for `sfmt` instead.
 */
#define svfmt_append(s, fmt, args) metadot_string_vfmt_append(s, fmt, args)

/**
 * Copies the string b into string a.
 * Automatically resizes as necessary.
 */
#define sset(a, b) metadot_string_set(a, b)

/**
 * Returns a completely new string copy.
 * You must free the copy with `sfree` when done.
 */
#define sdup(s) metadot_string_dup(s)

/**
 * Returns a completely new string copy.
 * You must free the copy with `sfree` when done.
 */
#define smake(s) metadot_string_make(s)

/**
 * Returns 0 if the two strings are equivalent.
 * Otherwise returns 1 if a[i] > b[i], or -1 if a[i] < b[i].
 */
#define scmp(a, b) metadot_string_cmp(a, b)

/**
 * Returns 0 if the two strings are equivalent, ignoring case.
 * Otherwise returns 1 if a[i] > b[i], or -1 if a[i] < b[i].
 * Ignores case.
 */
#define sicmp(a, b) metadot_string_icmp(a, b)

/**
 * Returns false if the two strings are equivalent, true otherwise.
 */
#define sequ(a, b) metadot_string_equ(a, b)

/**
 * Returns false if the two strings are equivalent, true otherwise.
 * Ignores case.
 */
#define siequ(a, b) metadot_string_iequ(a, b)

/**
 * Returns true if `prefix` is the prefix of `s`, false otherwise.
 */
#define sprefix(s, prefix) metadot_string_prefix(s, prefix)

/**
 * Returns true if `suffix` is the suffix of `s`, false otherwise.
 */
#define ssuffix(s, suffix) metadot_string_suffix(s, suffix)

/**
 * Returns true if s contains the substring `contains_me`.
 */
#define scontains(s, contains_me) metadot_string_contains(s, contains_me)

/**
 * Sets all characters in the string to upper case.
 */
#define stoupper(s) metadot_string_toupper(s)

/**
 * Sets all characters in the string to lower case.
 */
#define stolower(s) metadot_string_tolower(s)

/**
 * Returns a hash of the string as uint64_t.
 */
#define shash(s) metadot_string_hash(s)

/**
 * Appends the string b onto the end of a.
 * You can technically do this with `sfmt`, but this function is optimized much faster.
 */
#define sappend(a, b) metadot_string_append(a, b)

/**
 * Appends the string b onto the end of a.
 * You can technically do this with `sfmt`, but this function is optimized much faster.
 */
#define scat(a, b) metadot_string_append(a, b)

/**
 * Appends a range of characters from string b onto the end of a.
 * You can technically do this with `sfmt`, but this function is optimized much faster.
 */
#define sappend_range(a, b, b_end) metadot_string_append_range(a, b, b_end)

/**
 * Appends a range of characters from string b onto the end of a.
 * You can technically do this with `sfmt`, but this function is optimized much faster.
 */
#define scat_range(a, b, b_end) metadot_string_append_range(a, b, b_end)

/**
 * Removes all whitespace from the beginning and end of the string.
 */
#define strim(s) metadot_string_trim(s)

/**
 * Removes all whitespace from the beginning of the string.
 */
#define sltrim(s) metadot_string_ltrim(s)

/**
 * Removes all whitespace from the end of the string.
 */
#define srtrim(s) metadot_string_rtrim(s)

/**
 * Places n characters `ch` onto the front of the string.
 */
#define slpad(s, ch, n) metadot_string_lpad(s, ch, n)

/**
 * Appends n characters `ch` onto the end of the string.
 */
#define srpad(s, ch, n) metadot_string_rpad(s, ch, n)

/**
 * Splits a string about the character `ch` one time, scanning from left-to-right.
 * `s` will contain the string to the right of `ch`.
 * Returns the string to the left of `ch`.
 * If `ch` isn't found, simply returns NULL and does not modify `s`.
 * You must call `sfree` on the returned string.
 *
 * This function is intended to be used in a loop, successively chopping off pieces of `s`.
 * A much easier, but slightly slower, version of this function is `ssplit`, which returns
 * an array of strings.
 */
#define ssplit_once(s, ch) metadot_string_split_once(s, ch)

/**
 * Splits a string about the character `ch`, scanning from left-to-right.
 * `s` is not modified.
 * Returns an array of all delimited strings.
 * You must call `sfree` on the returned strings and `afree` on the returned array.
 *
 *     char* s = NULL;
 *     sset(s, "split.here.in.a.loop");
 *     const char* splits_expected[] = {
 *         "split",
 *         "here",
 *         "in",
 *         "a",
 *         "loop",
 *     };
 *     char** array_of_splits = ssplit(s, '.');
 *     for (int i = 0; i < alen(array_of_splits); ++i) {
 *         const char* split = array_of_splits[i];
 *         METAENGINE_TEST_ASSERT(sequ(split, splits_expected[i]));
 *         sfree(split);
 *     }
 *     afree(array_of_splits);
 */
#define ssplit(s, ch) metadot_string_split(s, ch)

/**
 * Scanning from left-to-right, returns the first index of `ch` found.
 * Returns -1 if none are found.
 */
#define sfirst_index_of(s, ch) metadot_string_first_index_of(s, ch)

/**
 * Scanning from right-to-left, returns the first index of `ch` found.
 * Returns -1 if none are found.
 */
#define slast_index_of(s, ch) metadot_string_last_index_of(s, ch)

/**
 * Scanning from left-to-right, returns a pointer to the substring `find`.
 * Returns NULL if not found.
 */
#define sfind(s, find) metadot_string_find(s, find)

/**
 * Converts an int64_t to a string and assigns `s` to it.
 */
#define sint(s, i) metadot_string_int(s, i)

/**
 * Converts a uint64_t to a string and assigns `s` to it.
 */
#define suint(s, uint) metadot_string_uint(s, uint)

/**
 * Converts a float to a string and assigns `s` to it.
 */
#define sfloat(s, f) metadot_string_float(s, f)

/**
 * Converts a double to a string and assigns `s` to it.
 */
#define sdouble(s, f) metadot_string_double(s, f)

/**
 * Converts a uint64_t to a hex-string and assigns `s` to it.
 */
#define shex(s, uint) metadot_string_hex(s, uint)

/**
 * Converts a string to a bool and returns it.
 */
#define sbool(s, b) metadot_string_bool(s, b)

/**
 * Converts a string to an int64_t and returns it.
 */
#define stoint(s) metadot_string_toint(s)

/**
 * Converts a string to an uint64_t and returns it.
 */
#define stouint(s) metadot_string_touint(s)

/**
 * Converts a string to a float and returns it.
 */
#define stofloat(s) metadot_string_tofloat(s)

/**
 * Converts a string to a double and returns it.
 */
#define stodouble(s) metadot_string_todouble(s)

/**
 * Converts a hex-string to a uint64_t and returns it.
 * Supports srings that start with "0x", "#", or no prefix.
 */
#define stohex(s) metadot_string_tohex(s)

/**
 * Converts a string to a bool and returns it.
 */
#define stobool(s) metadot_string_tobool(s)

/**
 * Replaces all substrings `replace_me` with the substring `with_me`.
 */
#define sreplace(s, replace_me, with_me) metadot_string_replace(s, replace_me, with_me)

/**
 * Deletes a number of characters from the string.
 */
#define serase(s, index, count) metadot_string_erase(s, index, count)

/**
 * Removes a character from the end of the string.
 */
#define spop(s) (s = metadot_string_pop(s))

/**
 * Removes n characters from the back of a string.
 */
#define spopn(s, n) (s = metadot_string_pop_n(s, n))

/**
 * Creates a string with an initial static storage backing. Will grow onto the heap
 * if the size becomes too large.
 *
 * s           - Your string, can be NULL. Should be char*.
 * buffer      - Pointer to a static memory buffer.
 * buffer_size - The size of `buffer` in bytes.
 */
#define sstatic(s, buffer, buffer_size) metadot_string_static(s, buffer, buffer_size)

/**
 * Returns true if `s` is a dynamically alloced string from this C string API.
 * This can be evaluated at compile time for string literals.
 */
#define sisdyna(s) metadot_string_is_dynamic(s)

//--------------------------------------------------------------------------------------------------
// UTF8 and UTF16.

/**
 * Encodes a UTF32 codepoint (as `uint32_t`) as UTF8. The UTF8 bytes are appended onto the string.
 *
 * Each UTF32 codepoint represents a single character. Each character can be encoded from 1 to 4
 * bytes. Therefor, this function will push 1 to 4 bytes onto the string.
 *
 * If an invalid codepoint is found the "replacement character" 0xFFFD will be appended instead, which
 * looks like question mark inside of a dark diamond.
 */
#define sappend_UTF8(s, codepoint) metadot_string_append_UTF8(s, codepoint)

/**
 * Decodes a single UTF8 character from the string as a UTF32 codepoint.
 *
 * The return value is not a new string, but just s + bytes, where bytes is anywhere from 1 to 4.
 * You can use this function in a loop to decode one codepoint at a time, where each codepoint
 * represents a single UTF8 character.
 *
 *     int cp;
 *     const char* tmp = my_string;
 *     while (*tmp) {
 *         tmp = metadot_decode_UTF8(tmp, &cp);
 *         DoSomethingWithCodepoint(cp);
 *     }
 */
const char* METADOT_CDECL metadot_decode_UTF8(const char* s, int* codepoint);

/**
 * Decodes a single UTF16 character from the string as a UTF32 codepoint.
 *
 * The return value is not a new string, but just s + count, where count is anywhere from 1 to 2.
 * You can use this function in a loop to decode one codepoint at a time, where each codepoint
 * represents a single UTF8 character.
 *
 *     int cp;
 *     const uint16_t* tmp = my_string;
 *     while (tmp) {
 *         tmp = metadot_decode_UTF16(tmp, &cp);
 *         DoSomethingWithCodepoint(cp);
 *     }
 *
 * You can convert a UTF16 string to UTF8 by calling `sappend_UTF8` on another string
 * instance inside the above example loop. Here's an example function to return a new string
 * instance in UTF8 form given a UTF16 string.
 *
 * char* utf8(uint16_t* text)
 * {
 *     int cp;
 *     char* s = NULL;
 *     while (*text) {
 *         text = metadot_decode_UTF16(text, &cp);
 *         s = sappend_UTF8(s, cp);
 *     }
 *     return s;
 * }
 */
const uint16_t* METADOT_CDECL metadot_decode_UTF16(const uint16_t* s, int* codepoint);

//--------------------------------------------------------------------------------------------------
// String Intering C API (global string table).
// ^      ^

/**
 * Global string table.
 * Only one copy of each unique string is stored inside.
 * Use this function to get a stable pointer to a string.
 * Primarily used as a memory optimization to reduce duplicate strings.
 * You *can not* modify this string in any way. It is 100% immutable.
 * You can hash returned pointers directly into hash tables (instead of hashing the entire string).
 * You can simply compare pointers for equality, as opposed to comparing the string contents.
 * You may optionally call `sinuke` to free all resources used by the global string table.
 */
#define sintern(s) metadot_sintern(s)

/**
 * Global string table.
 * Only one copy of each unique string is stored inside.
 * Use this function to get a stable pointer to a string.
 * Primarily used as a memory optimization to reduce duplicate strings.
 * You *can not* modify this string in any way. It is 100% immutable.
 * You can hash returned pointers directly into hash tables (instead of hashing the entire string).
 * You can simply compare pointers for equality, as opposed to comparing the string contents.
 * You may optionally call `sinuke` to free all resources used by the global string table.
 */
#define sintern_range(start, end) metadot_sintern_range(start, end)

/**
 * Returns true if this string is a valid intern'd string (it was returned to you by `sintern`).
 * Returns false for all other strings.
 * This is *not* a secure method -- do not use it on any unvalidated strings. It's designed to be
 * very simple and fast, nothing more.
 */
#define sivalid(s) (((metadot_intern_t*)s - 1)->cookie == METAENGINE_INTERN_COOKIE)

/**
 * Returns the length of an intern'd string.
 */
#define silen(s) (((metadot_intern_t*)s - 1)->len)

/**
 * Frees up all resources used by the global string table built by `sintern`.
 * All strings previously returned by `sintern` are now invalid.
 */
#define sinuke() metadot_sinuke()
#endif  // METAENGINE_NO_SHORTHAND_API

//--------------------------------------------------------------------------------------------------
// Longform C API.

#define metadot_string_len(s) ((int)((size_t)metadot_array_count(s) - 1))
#define metadot_string_empty(s) (s ? metadot_string_len(s) < 1 : 1)
#define metadot_string_push(s, ch)         \
    do {                                   \
        if (!s)                            \
            metadot_array_push(s, ch);     \
        else                               \
            s[metadot_string_len(s)] = ch; \
        metadot_array_push(s, 0);          \
    } while (0)
#define metadot_string_free(s) metadot_array_free(s)
#define metadot_string_size(s) metadot_array_len(s)
#define metadot_string_count(s) metadot_array_count(s)
#define metadot_string_cap(s) metadot_array_capacity(s)
#define metadot_string_last(s) (s[metadot_string_len(s) - 1])
#define metadot_string_clear(s) (metadot_array_clear(s), metadot_array_push(s, 0))
#define metadot_string_fit(s, n) metadot_array_fit(s, n)
#define metadot_string_fmt(s, fmt, ...) (s = metadot_sfmt(s, fmt, __VA_ARGS__))
#define metadot_string_fmt_append(s, fmt, ...) (s = metadot_sfmt_append(s, fmt, __VA_ARGS__))
#define metadot_string_vfmt(s, fmt, args) (s = metadot_svfmt(s, fmt, args))
#define metadot_string_vfmt_append(s, fmt, args) (s = metadot_svfmt_append(s, fmt, args))
#define metadot_string_set(a, b) (a = metadot_sset(a, b))
#define metadot_string_dup(s) metadot_sset(NULL, s)
#define metadot_string_make(s) metadot_sset(NULL, s)
#define metadot_string_cmp(a, b) METAENGINE_STRCMP(a, b)
#define metadot_string_icmp(a, b) METAENGINE_STRICMP(a, b)
#define metadot_string_equ(a, b) ((!(a) && !(b)) || !METAENGINE_STRCMP(a, b))
#define metadot_string_iequ(a, b) ((!(a) && !(b)) || !METAENGINE_STRICMP(a, b))
#define metadot_string_prefix(s, prefix) metadot_sprefix(s, prefix)
#define metadot_string_suffix(s, suffix) metadot_ssuffix(s, suffix)
#define metadot_string_contains(s, contains_me) (metadot_string_len(s) >= METAENGINE_STRLEN(contains_me) && !!METAENGINE_STRSTR(s, contains_me))
#define metadot_string_toupper(s) metadot_stoupper(s)
#define metadot_string_tolower(s) metadot_stolower(s)
#define metadot_string_hash(s) ahash(s)
#define metadot_string_append(a, b) (a = metadot_sappend(a, b))
#define metadot_string_append_range(a, b, b_end) (a = metadot_sappend_range(a, b, b_end))
#define metadot_string_trim(s) (s = metadot_strim(s))
#define metadot_string_ltrim(s) (s = metadot_sltrim(s))
#define metadot_string_rtrim(s) (s = metadot_srtrim(s))
#define metadot_string_lpad(s, ch, n) (s = metadot_slpad(s, ch, n))
#define metadot_string_rpad(s, ch, n) (s = metadot_srpad(s, ch, n))
#define metadot_string_split_once(s, ch) metadot_ssplit_once(s, ch)
#define metadot_string_split(s, ch) metadot_ssplit(s, ch)
#define metadot_string_first_index_of(s, ch) metadot_sfirst_index_of(s, ch)
#define metadot_string_last_index_of(s, ch) metadot_slast_index_of(s, ch)
#define metadot_string_find(s, find) METAENGINE_STRSTR(s, find)
#define metadot_string_int(s, i) metadot_string_fmt(s, "%d", i)
#define metadot_string_uint(s, uint) metadot_string_fmt(s, "%" PRIu64, uint)
#define metadot_string_float(s, f) metadot_string_fmt(s, "%f", f)
#define metadot_string_double(s, f) metadot_string_fmt(s, "%f", d)
#define metadot_string_hex(s, uint) metadot_string_fmt(s, "0x%x", uint)
#define metadot_string_bool(s, b) metadot_string_fmt(s, "%s", b ? "true" : "false")
#define metadot_string_toint(s) metadot_stoint(s)
#define metadot_string_touint(s) metadot_stouint(s)
#define metadot_string_tofloat(s) metadot_stofloat(s)
#define metadot_string_todouble(s) metadot_stodouble(s)
#define metadot_string_tohex(s) metadot_stohex(s)
#define metadot_string_tobool(s) (!METAENGINE_STRCMP(s, "true"))
#define metadot_string_replace(s, replace_me, with_me) (s = metadot_sreplace(s, replace_me, with_me))
#define metadot_string_erase(s, index, count) (s = metadot_serase(s, index, count))
#define metadot_string_pop(s) (s = metadot_spop(s))
#define metadot_string_pop_n(s, n) (s = metadot_spopn(s, n))
#define metadot_string_static(s, buffer, buffer_size) (metadot_array_static(s, buffer, buffer_size), metadot_array_push(s, 0))
#define metadot_string_is_dynamic(s) (s && !((#s)[0] == '"') && METAENGINE_AHDR(s)->cookie == METAENGINE_ACOOKIE)
#define metadot_sinuke() metadot_sinuke_intern_table()
#define metadot_string_append_UTF8(s, codepoint) (s = metadot_string_append_UTF8_impl(s, codepoint))

//--------------------------------------------------------------------------------------------------
// Hidden API - Not intended for direct use.

#define METAENGINE_INTERN_COOKIE 0x75AFC82E  // Used for sanity/type checking.

typedef struct metadot_intern_t {
    uint32_t cookie;  // Type check.
    int len;
    struct metadot_intern_t* next;
    const char* string;  // For debugging convenience but allocated after this struct.
} metadot_intern_t;

char* METADOT_CDECL metadot_sset(char* a, const char* b);
char* METADOT_CDECL metadot_sfmt(char* s, const char* fmt, ...);
char* METADOT_CDECL metadot_sfmt_append(char* s, const char* fmt, ...);
char* METADOT_CDECL metadot_svfmt(char* s, const char* fmt, va_list args);
char* METADOT_CDECL metadot_svfmt_append(char* s, const char* fmt, va_list args);
bool METADOT_CDECL metadot_sprefix(char* s, const char* prefix);
bool METADOT_CDECL metadot_ssuffix(char* s, const char* suffix);
void METADOT_CDECL metadot_stoupper(char* s);
void METADOT_CDECL metadot_stolower(char* s);
char* METADOT_CDECL metadot_sappend(char* a, const char* b);
char* METADOT_CDECL metadot_sappend_range(char* a, const char* b, const char* b_end);
char* METADOT_CDECL metadot_strim(char* s);
char* METADOT_CDECL metadot_sltrim(char* s);
char* METADOT_CDECL metadot_srtrim(char* s);
char* METADOT_CDECL metadot_slpad(char* s, char pad, int count);
char* METADOT_CDECL metadot_srpad(char* s, char pad, int count);
char* METADOT_CDECL metadot_ssplit_once(char* s, char split_c);
char** METADOT_CDECL metadot_ssplit(const char* s, char split_c);
int METADOT_CDECL metadot_sfirst_index_of(const char* s, char c);
int METADOT_CDECL metadot_slast_index_of(const char* s, char c);
int METADOT_CDECL metadot_stoint(const char* s);
uint64_t METADOT_CDECL metadot_stouint(const char* s);
float METADOT_CDECL metadot_stofloat(const char* s);
double METADOT_CDECL metadot_stodouble(const char* s);
uint64_t METADOT_CDECL metadot_stohex(const char* s);
char* METADOT_CDECL metadot_sreplace(char* s, const char* replace_me, const char* with_me);
char* METADOT_CDECL metadot_serase(char* s, int index, int count);
char* METADOT_CDECL metadot_spop(char* s);
char* METADOT_CDECL metadot_spopn(char* s, int n);
char* METADOT_CDECL metadot_string_append_UTF8_impl(char* s, int codepoint);

const char* METADOT_CDECL metadot_sintern(const char* s);
const char* METADOT_CDECL metadot_sintern_range(const char* start, const char* end);
void METADOT_CDECL metadot_sinuke_intern_table();

#ifdef __cplusplus
}
#endif  // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

namespace MetaEngine {

METADOT_INLINE uint64_t constexpr fnv1a(const void* data, int size) {
    const char* s = (const char*)data;
    uint64_t h = 14695981039346656037ULL;
    char c = 0;
    while (size--) {
        h = h ^ (uint64_t)(*s++);
        h = h * 1099511628211ULL;
    }
    return h;
}

/**
 * General purpose string class.
 *
 * Each string is stored in its own buffer internally.
 * The buffer starts out statically allocated, and grows onto the heap as necessary.
 */
struct String {
    METADOT_INLINE String() { s_static(); }
    METADOT_INLINE String(const char* s) {
        s_static();
        sset(m_str, s);
    }
    METADOT_INLINE String(const char* start, const char* end) {
        s_static();
        int length = (int)(end - start);
        sfit(m_str, length);
        METAENGINE_STRNCPY(m_str, start, length);
        METAENGINE_AHDR(m_str)->size = length + 1;
    }
    METADOT_INLINE String(const String& s) {
        s_static();
        sset(m_str, s);
    }
    METADOT_INLINE String(String&& s) {
        if (METAENGINE_AHDR(s.m_str)->is_static) {
            s_static();
            sset(m_str, s);
        } else {
            m_str = s.m_str;
        }
        s.m_str = NULL;
    }
    METADOT_INLINE ~String() { sfree(m_str); }

    METADOT_INLINE static String steal_from(char* cute_c_api_string) {
        METAENGINE_ACANARY(cute_c_api_string);
        String r;
        r.m_str = cute_c_api_string;
        return r;
    }
    METADOT_INLINE static String from(int i) {
        String r;
        sint(r.m_str, i);
        return r;
    }
    METADOT_INLINE static String from(uint64_t uint) {
        String r;
        suint(r.m_str, uint);
        return r;
    }
    METADOT_INLINE static String from(float f) {
        String r;
        sfloat(r.m_str, f);
        return r;
    }
    METADOT_INLINE static String from(double f) {
        String r;
        sfloat(r.m_str, f);
        return r;
    }
    METADOT_INLINE static String from_hex(uint64_t uint) {
        String r;
        shex(r.m_str, uint);
        return r;
    }
    METADOT_INLINE static String from(bool b) {
        String r;
        sbool(r.m_str, b);
        return r;
    }

    METADOT_INLINE int to_int() const { return stoint(m_str); }
    METADOT_INLINE uint64_t to_uint() const { return stouint(m_str); }
    METADOT_INLINE float to_float() const { return stofloat(m_str); }
    METADOT_INLINE double to_double() const { return stodouble(m_str); }
    METADOT_INLINE uint64_t to_hex() const { return stohex(m_str); }
    METADOT_INLINE bool to_bool() const { return stobool(m_str); }

    METADOT_INLINE const char* c_str() const { return m_str; }
    METADOT_INLINE char* c_str() { return m_str; }
    METADOT_INLINE const char* begin() const { return m_str; }
    METADOT_INLINE char* begin() { return m_str; }
    METADOT_INLINE const char* end() const { return m_str + string_size(m_str); }
    METADOT_INLINE char* end() { return m_str + string_size(m_str); }
    METADOT_INLINE char last() const { return slast(m_str); }
    METADOT_INLINE operator const char*() const { return m_str; }
    METADOT_INLINE operator char*() const { return m_str; }

    METADOT_INLINE char& operator[](int index) {
        s_chki(index);
        return m_str[index];
    }
    METADOT_INLINE const char& operator[](int index) const {
        s_chki(index);
        return m_str[index];
    }

    METADOT_INLINE int len() const { return slen(m_str); }
    METADOT_INLINE int capacity() const { return scap(m_str); }
    METADOT_INLINE int size() const { return string_size(m_str); }
    METADOT_INLINE int count() const { return string_size(m_str); }
    METADOT_INLINE void ensure_capacity(int capacity) { sfit(m_str, capacity); }
    METADOT_INLINE void fit(int capacity) { sfit(m_str, capacity); }
    METADOT_INLINE void set_len(int len) {
        sfit(m_str, len + 1);
        string_size(m_str) = len + 1;
    }
    METADOT_INLINE bool empty() const { return sempty(m_str); }

    METADOT_INLINE String& add(char ch) {
        spush(m_str, ch);
        return *this;
    }
    METADOT_INLINE String& append(const char* s) {
        sappend(m_str, s);
        return *this;
    }
    METADOT_INLINE String& append(const char* start, const char* end) {
        sappend_range(m_str, start, end);
        return *this;
    }
    METADOT_INLINE String& append(int codepoint) {
        sappend_UTF8(m_str, codepoint);
        return *this;
    }
    METADOT_INLINE String& fmt(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        svfmt(m_str, fmt, args);
        va_end(args);
        return *this;
    }
    METADOT_INLINE String& fmt_append(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        svfmt_append(m_str, fmt, args);
        va_end(args);
        return *this;
    }
    METADOT_INLINE String& trim() {
        strim(m_str);
        return *this;
    }
    METADOT_INLINE String& ltrim() {
        sltrim(m_str);
        return *this;
    }
    METADOT_INLINE String& rtrim() {
        srtrim(m_str);
        return *this;
    }
    METADOT_INLINE String& lpad(char pad, int count) {
        slpad(m_str, pad, count);
        return *this;
    }
    METADOT_INLINE String& rpad(char pad, int count) {
        srpad(m_str, pad, count);
        return *this;
    }
    METADOT_INLINE String& set(const char* s) {
        sset(m_str, s);
        return *this;
    }
    METADOT_INLINE String& operator=(const char* s) {
        sset(m_str, s);
        return *this;
    }
    METADOT_INLINE String& operator=(const String& s) {
        sset(m_str, s);
        return *this;
    }
    METADOT_INLINE String& operator=(String&& s) {
        sset(m_str, s);
        return *this;
    }
    METADOT_INLINE Array<String> split(char split_c) {
        Array<String> r;
        char** s = ssplit(m_str, split_c);
        for (int i = 0; i < alen(s); ++i) r.add(metadot_move(steal_from(s[i])));
        return r;
    }
    METADOT_INLINE char pop() { return apop(m_str); }
    METADOT_INLINE int first_index_of(char ch) const { return sfirst_index_of(m_str, ch); }
    METADOT_INLINE int last_index_of(char ch) const { return slast_index_of(m_str, ch); }
    METADOT_INLINE int first_index_of(char ch, int offset) const { return sfirst_index_of(m_str + offset, ch); }
    METADOT_INLINE int last_index_of(char ch, int offset) const { return slast_index_of(m_str + offset, ch); }
    METADOT_INLINE String find(const char* find_me) const { return String(sfind(m_str, find_me)); }
    METADOT_INLINE String& replace(const char* replace_me, const char* with_me) {
        sreplace(m_str, replace_me, with_me);
        return *this;
    }
    METADOT_INLINE String& erase(int index, int count) {
        serase(m_str, index, count);
        return *this;
    }
    METADOT_INLINE String dup() const { return steal_from(sdup(m_str)); }
    METADOT_INLINE void clear() { sclear(m_str); }

    METADOT_INLINE bool starts_with(const char* s) const { return sprefix(m_str, s); }
    METADOT_INLINE bool ends_with(const char* s) const { return ssuffix(m_str, s); }
    METADOT_INLINE bool prefix(const char* s) const { return sprefix(m_str, s); }
    METADOT_INLINE bool suffix(const char* s) const { return ssuffix(m_str, s); }
    METADOT_INLINE bool operator==(const char* s) { return !METAENGINE_STRCMP(m_str, s); }
    METADOT_INLINE bool operator!=(const char* s) { return METAENGINE_STRCMP(m_str, s); }
    METADOT_INLINE bool compare(const char* s, bool no_case = false) { return no_case ? sequ(m_str, s) : siequ(m_str, s); }
    METADOT_INLINE bool cmp(const char* s, bool no_case = false) { compare(s, no_case); }
    METADOT_INLINE bool contains(const char* contains_me) { return scontains(m_str, contains_me); }
    METADOT_INLINE void to_upper() { stoupper(m_str); }
    METADOT_INLINE void to_lower() { stolower(m_str); }
    METADOT_INLINE uint64_t hash() const { return shash(m_str); }

private:
    char* m_str = u.m_buffer;
    union {
        char m_buffer[64];
        void* align;
    } u;

    METADOT_INLINE void s_static() {
        sstatic(m_str, u.m_buffer, sizeof(u.m_buffer));
        METADOT_ASSERT_E(slen(m_str) == 0);
    }
    METADOT_INLINE void s_chki(int i) const { METADOT_ASSERT_E(i >= 0 && i < string_size(m_str)); }
};

METADOT_INLINE String operator+(const String& a, const String& b) {
    String result = a;
    result.append(b);
    return result;
}

/**
 * UTF8 decoder. Load it up with a string and read `.codepoint`. Call `next` to fetch the
 * next codepoint.
 *
 * Example:
 *
 *     UTF8 utf8 = UTF8(my_string_in_utf8_format);
 *     while (utf8.next()) {
 *         DoSomethingWithCodepoint(utf8.codepoint);
 *     }
 */
struct UTF8 {
    METADOT_INLINE UTF8() {}
    METADOT_INLINE UTF8(const char* text) { this->text = text; }

    METADOT_INLINE bool next() {
        if (*text) {
            text = metadot_decode_UTF8(text, &codepoint);
            return true;
        } else
            return false;
    }

    int codepoint;
    const char* text = NULL;
};

/**
 * UTF16 decoder. Load it up with a string and read `.codepoint`. Call `next` to fetch the
 * next codepoint.
 *
 * Example:
 *
 *     UTF16 utf16 = UTF16(my_string_in_utf16_format);
 *     while (utf16.next()) {
 *         DoSomethingWithCodepoint(utf16.codepoint);
 *     }
 */
struct UTF16 {
    METADOT_INLINE UTF16() {}
    METADOT_INLINE UTF16(const uint16_t* text) { this->text = text; }

    METADOT_INLINE bool next() {
        if (*text) {
            text = metadot_decode_UTF16(text, &codepoint);
            return true;
        } else
            return false;
    }

    int codepoint;
    const uint16_t* text = NULL;
};

}  // namespace MetaEngine

#pragma endregion string

#pragma region result

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#define METAENGINE_RESULT_DEFS         \
    METAENGINE_ENUM(RESULT_SUCCESS, 0) \
    METAENGINE_ENUM(RESULT_ERROR, -1)

enum {
#define METAENGINE_ENUM(K, V) METAENGINE_##K = V,
    METAENGINE_RESULT_DEFS
#undef METAENGINE_ENUM
};

typedef struct METAENGINE_Result {
    int code;
    const char* details;
} METAENGINE_Result;

METADOT_INLINE bool metadot_is_error(METAENGINE_Result result) { return result.code == METAENGINE_RESULT_ERROR; }

METADOT_INLINE METAENGINE_Result metadot_result_make(int code, const char* details) {
    METAENGINE_Result result;
    result.code = code;
    result.details = details;
    return result;
}
METADOT_INLINE METAENGINE_Result metadot_result_error(const char* details) {
    METAENGINE_Result result;
    result.code = METAENGINE_RESULT_ERROR;
    result.details = details;
    return result;
}
METADOT_INLINE METAENGINE_Result metadot_result_success() {
    METAENGINE_Result result;
    result.code = METAENGINE_RESULT_SUCCESS;
    result.details = NULL;
    return result;
}

#define METAENGINE_MESSAGE_BOX_TYPE_DEFS         \
    METAENGINE_ENUM(MESSAGE_BOX_TYPE_ERROR, 0)   \
    METAENGINE_ENUM(MESSAGE_BOX_TYPE_WARNING, 1) \
    METAENGINE_ENUM(MESSAGE_BOX_TYPE_INFORMATION, 2)

typedef enum METAENGINE_MessageBoxType {
#define METAENGINE_ENUM(K, V) METAENGINE_##K = V,
    METAENGINE_MESSAGE_BOX_TYPE_DEFS
#undef METAENGINE_ENUM
} METAENGINE_MessageBoxType;

METADOT_INLINE const char* metadot_message_box_type_to_string(METAENGINE_MessageBoxType type) {
    switch (type) {
#define METAENGINE_ENUM(K, V) \
    case METAENGINE_##K:      \
        return METAENGINE_STRINGIZE(METAENGINE_##K);
        METAENGINE_MESSAGE_BOX_TYPE_DEFS
#undef METAENGINE_ENUM
        default:
            return NULL;
    }
}

void METADOT_CDECL metadot_message_box(METAENGINE_MessageBoxType type, const char* title, const char* text);

#ifdef __cplusplus
}
#endif  // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

namespace MetaEngine {

using Result = METAENGINE_Result;

enum : int {
#define METAENGINE_ENUM(K, V) K = V,
    METAENGINE_RESULT_DEFS
#undef METAENGINE_ENUM
};

using MessageBoxType = METAENGINE_MessageBoxType;
#define METAENGINE_ENUM(K, V) METADOT_INLINE constexpr MessageBoxType K = METAENGINE_##K;
METAENGINE_MESSAGE_BOX_TYPE_DEFS
#undef METAENGINE_ENUM

METADOT_INLINE const char* to_string(MessageBoxType type) {
    switch (type) {
#define METAENGINE_ENUM(K, V) \
    case METAENGINE_##K:      \
        return #K;
        METAENGINE_MESSAGE_BOX_TYPE_DEFS
#undef METAENGINE_ENUM
        default:
            return NULL;
    }
}

METADOT_INLINE bool is_error(Result error) { return metadot_is_error(error); }

METADOT_INLINE Result result_make(int code, const char* details) { return metadot_result_make(code, details); }
METADOT_INLINE Result result_failure(const char* details) { return metadot_result_error(details); }
METADOT_INLINE Result result_success() { return metadot_result_success(); }
METADOT_INLINE void message_box(MessageBoxType type, const char* title, const char* text) { return metadot_message_box(type, title, text); }

}  // namespace MetaEngine

#pragma endregion result

#pragma region hashtable

//--------------------------------------------------------------------------------------------------
// C API

// A hashtable for storing and looking up {key, item} pairs.
// Implemented in C with macros to support fancy polymorphism.
// The API works on typed pointers, just create one as NULL and start going.
// Free it with `hfree` when done.
//
// Example:
//
//     htbl v2* pts = NULL;
//     hset(pts, 0, V2(3, 5)); // Contructs a new table on-the-spot.
//                             // The table is *hidden* behind `pts`.
//     hset(pts, 10, v2(-1, -1);
//     hset(pts, -2, v2(0, 0));
//
//     // Use `hget` to fetch values.
//     v2 a = hget(pts, 0);
//     v2 b = hget(pts, 10);
//     v2 c = hget(pts, -2);
//
//     // Loop over {key, item} pairs like so:
//     const uint64_t* keys = hkeys(pts);
//     for (int i = 0; i < hcount(pts); ++i) {
//         uint64_t key = keys[i];
//         v2 v = pts[i];
//         // ...
//     }
//
//     hfree(pts);

#ifndef METAENGINE_NO_SHORTHAND_API
/**
 * This is *optional* and _completely_ empty macro. It's only purpose is to provide a bit of visual
 * indication a type is a table. One downside of the C-macro API is the opaque nature of the table
 * type. Since the macros use polymorphism on typed pointers, there's no actual `metadot_hashtable_t` type.
 *
 * It can get really annoying to sometimes forget if a pointer is an array, a hashtable, or just a
 * pointer. This macro can be used to markup the type to make it much more clear for function parameters
 * or struct member definitions. It's saying "Hey, I'm a hashtable!" to mitigate this downside.
 */
#define htbl

/**
 * Add's a {key, item} pair. Creates a new table if `h` is NULL. Call `hfree` when done.
 * Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 *
 * h   - The hashtable. Will create a new table if h is NULL.
 *       h needs to be a pointer to the type of items in the table.
 * k   - The key for lookups. Must be unique. Will be typecasted to uint64_t.
 * ... - An item to place into the table, by value.
 *
 * Example:
 *
 *     htbl int* table = NULL;
 *     hset(table, 0, 5);
 *     hset(table, 1, 12);
 *     METADOT_ASSERT_E(hget(table, 0) == 5);
 *     METADOT_ASSERT_E(hget(table, 1) == 12);
 *     hfree(table);
 */
#define hset(h, k, ...) metadot_hashtable_set(h, k, (__VA_ARGS__))

/**
 * Add's a {key, item} pair. Creates a new table if `h` is NULL. Call `hfree` when done.
 * Keys are always typecasted to `uint64_t` e.g. you can use pointers as keys.
 *
 * h   - The hashtable. Will create a new table if h is NULL.
 *       h needs to be a pointer for the type of your items in the table.
 * k   - The key for lookups. Must be unique. Will be typecasted to uint64_t.
 * ... - An item to place into the table, by value.
 *
 * Example:
 *
 *     htbl int* table = NULL;
 *     hadd(table, 0, 5);
 *     hadd(table, 1, 12);
 *     METADOT_ASSERT_E(hget(table, 0) == 5);
 *     METADOT_ASSERT_E(hget(table, 1) == 12);
 *     hfree(table);
 */
#define hadd(h, k, ...) metadot_hashtable_add(h, k, (__VA_ARGS__))

/**
 * Fetches the item that `k` maps to.
 *
 * h   - The hashtable. Will assert h is NULL.
 *       h needs to be a pointer to the type of items in the table.
 * k   - The key for lookups. Must be unique. Will be typecasted to uint64_t.
 *
 * Items are returned by value, not pointer. If the item doesn't exist a zero'd out item
 * is instead returned. If you want to get a pointer (so you can see if it's `NULL` in
 * case the item didn't exist, then use `hget_ptr`). You can also call `hhas` for a bool.
 *
 * Example:
 *
 *     htbl v2* table = NULL;
 *     hadd(table, 10, V2(-1, 1));
 *     v2 v = hget(table, 10);
 *     METADOT_ASSERT_E(v.x == -1);
 *     METADOT_ASSERT_E(v.y == 1);
 *     hfree(table);
 */
#define hget(h, k) metadot_hashtable_get(h, k)

/**
 * Fetches the item that `k` maps to.
 *
 * h   - The hashtable. Will assert if h is NULL.
 *       h needs to be a pointer to the type of items in the table.
 * k   - The key for lookups. Must be unique. Will be typecasted to uint64_t.
 *
 * Items are returned by value, not pointer. If the item doesn't exist a zero'd out item
 * is instead returned. If you want to get a pointer (so you can see if it's `NULL` in
 * case the item didn't exist, then use `hget_ptr`). You can also call `hhas` for a bool.
 *
 * Example:
 *
 *     htbl v2* table = NULL;
 *     hadd(table, 10, V2(-1, 1));
 *     v2 v = hfind(table, 10);
 *     METADOT_ASSERT_E(v.x == -1);
 *     METADOT_ASSERT_E(v.y == 1);
 *     hfree(table);
 */
#define hfind(h, k) metadot_hashtable_find(h, k)

/**
 * Fetches a pointer to the item that `k` maps to. Returns NULL if not found.
 *
 * h   - The hashtable. Can be NULL.
 *       h needs to be a pointer to the type of items in the table.
 * k   - The key for lookups. Must be unique. Will be typecasted to uint64_t.
 *
 * Example:
 *
 *     htbl v2* table = NULL;
 *     hadd(table, 10, V2(-1, 1));
 *     v2* v = hget_ptr(table, 10);
 *     METADOT_ASSERT_E(v);
 *     METADOT_ASSERT_E(v->x == -1);
 *     METADOT_ASSERT_E(v->y == 1);
 *     hfree(table);
 */
#define hget_ptr(h, k) metadot_hashtable_get_ptr(h, k)

/**
 * Fetches a pointer to the item that `k` maps to. Returns NULL if not found.
 *
 * h   - The hashtable. Can be NULL.
 *       h needs to be a pointer to the type of items in the table.
 * k   - The key for lookups. Must be unique. Will be typecasted to uint64_t.
 *
 * Example:
 *
 *     htbl v2* table = NULL;
 *     hadd(table, 10, V2(-1, 1));
 *     v2* v = hfind_ptr(table, 10);
 *     METADOT_ASSERT_E(v);
 *     METADOT_ASSERT_E(v->x == -1);
 *     METADOT_ASSERT_E(v->y == 1);
 *     hfree(table);
 */
#define hfind_ptr(h, k) metadot_hashtable_find_ptr(h, k)

/**
 * Check to see if an item exists in the table.
 *
 * h   - The hashtable. Can be NULL.
 *       h needs to be a pointer to the type of items in the table.
 * k   - The key for lookups. Must be unique. Will be typecasted to uint64_t.
 *
 * Example:
 *
 *     htbl v2* table = NULL;
 *     hadd(table, 10, V2(-1, 1));
 *     METADOT_ASSERT_E(hhas(table, 10));
 *     hfree(table);
 */
#define hhas(h, k) metadot_hashtable_has(h, k)

/**
 * Removes an item from the table. Asserts if the item does not exist.
 *
 * h   - The hashtable. Can be NULL.
 *       h needs to be a pointer to the type of items in the table.
 * k   - The key for lookups. Must be unique. Will be typecasted to uint64_t.
 *
 * Example:
 *
 *     htbl v2* table = NULL;
 *     hadd(table, 10, V2(-1, 1));
 *     hdel(table, 10);
 *     hfree(table);
 */
#define hdel(h, k) metadot_hashtable_del(h, k)

/**
 * Clears the hashtable. The count of items will now be zero.
 * Does not free any memory. Call `hfree` when you are done.
 *
 * h   - The hashtable. Can be NULL.
 *       h needs to be a pointer to the type of items in the table.
 */
#define hclear(h) metadot_hashtable_clear(h)

/**
 * Get a const pointer to the array of keys. The keys are type uint64_t.
 *
 * h   - The hashtable. Can be NULL.
 *       h needs to be a pointer to the type of items in the table.
 *
 * Example:
 *
 *     htbl v2* table = my_table();
 *     const uint64_t* keys = hkeys(table);
 *     for (int i = 0; i < hcount(table); ++i) {
 *         uint64_t key = keys[i];
 *         v2 item = table[i];
 *         // ...
 *     }
 */
#define hkeys(h) metadot_hashtable_keys(h)

/**
 * Get a pointer to the array of items.
 * This macro doesn't do much as `h` is already a valid pointer to the items.
 *
 * h   - The hashtable. Can be NULL.
 *       h needs to be a pointer to the type of items in the table.
 *
 * Example:
 *
 *     htbl v2* table = my_table();
 *     const uint64_t* keys = hkeys(table);
 *     for (int i = 0; i < hcount(table); ++i) {
 *         uint64_t key = keys[i];
 *         v2 item = table[i]; // Could also do `hitems(table)` here.
 *         // ...
 *     }
 */
#define hitems(h) metadot_hashtable_items(h)

/**
 * Swaps internal ordering of two {key, item} pairs without ruining the hashing.
 * Use this for e.g. implementing a priority queue on top of the hash table.
 *
 * h       - The hashtable. Can be NULL.
 *           h needs to be a pointer to the type of items in the table.
 * index_a - Index to the first item to swap.
 * index_b - Index to the second item to swap.
 *
 * Example:
 *
 *     htbl v2* table = my_table();
 *     const uint64_t* keys = hkeys(table);
 *     for (int i = 0; i < hcount(table); ++i) {
 *         for (int j = 0; j < hcount(table); ++j) {
 *             if (my_need_swap(table, i, j)) {
 *                 hswap(h, i, j);
 *             }
 *         }
 *     }
 */
#define hswap(h, index_a, index_b) metadot_hashtable_swap(h, index_a, index_b)

/**
 * The number of {key, item} pairs in the table.
 * h can be NULL.
 */
#define hsize(h) metadot_hashtable_size(h)

/**
 * The number of {key, item} pairs in the table.
 * h can be NULL.
 */
#define hcount(h) metadot_hashtable_count(h)

/**
 * Frees up all resources used and sets h to NULL.
 * h can be NULL.
 */
#define hfree(h) metadot_hashtable_free(h)
#endif  // METAENGINE_NO_SHORTHAND_API

//--------------------------------------------------------------------------------------------------
// Longform C API.

#define metadot_htbl
#define metadot_hashtable_set(h, k, ...)                                                                                                         \
    ((h) ? (h) : (*(void**)&(h) = metadot_hashtable_make_impl(sizeof(uint64_t), sizeof(*(h)), 1)), METAENGINE_HCANARY(h), h[-1] = (__VA_ARGS__), \
     *(void**)&(h) = metadot_hashtable_insert_impl(METAENGINE_HHDR(h), (uint64_t)k), h + METAENGINE_HHDR(h)->return_index)
#define metadot_hashtable_add(h, k, ...) metadot_hashtable_set(h, k, (__VA_ARGS__))
#define metadot_hashtable_get(h, k) ((h)[metadot_hashtable_find_impl(METAENGINE_HHDR(h), (uint64_t)k)])
#define metadot_hashtable_find(h, k) metadot_hashtable_get(h, k)
#define metadot_hashtable_get_ptr(h, k) (metadot_hashtable_find_impl(METAENGINE_HHDR(h), (uint64_t)k), METAENGINE_HHDR(h)->return_index < 0 ? NULL : (h) + METAENGINE_HHDR(h)->return_index)
#define metadot_hashtable_find_ptr(h, k) metadot_hashtable_get_ptr(h, k)
#define metadot_hashtable_has(h, k) (h ? metadot_hashtable_remove_impl(METAENGINE_HHDR(h), (uint64_t)k) : NULL)
#define metadot_hashtable_del(h, k) (h ? metadot_hashtable_remove_impl(METAENGINE_HHDR(h), (uint64_t)k) : (void)0)
#define metadot_hashtable_clear(h) (METAENGINE_HCANARY(h), metadot_hashtable_clear_impl(METAENGINE_HHDR(h)))
#define metadot_hashtable_keys(h) (METAENGINE_HCANARY(h), h ? (const uint64_t*)metadot_hashtable_keys_impl(METAENGINE_HHDR(h))) : (const uint64_t*)NULL)
#define metadot_hashtable_items(h) (METAENGINE_HCANARY(h), h)
#define metadot_hashtable_swap(h, index_a, index_b) (METAENGINE_HCANARY(h), metadot_hashtable_swap_impl(METAENGINE_HHDR(h), index_a, index_b))
#define metadot_hashtable_size(h) (h ? metadot_hashtable_count_impl(METAENGINE_HHDR(h)) : 0)
#define metadot_hashtable_count(h) metadot_hashtable_size(h)
#define metadot_hashtable_free(h)                               \
    do {                                                        \
        METAENGINE_HCANARY(h);                                  \
        if (h) metadot_hashtable_free_impl(METAENGINE_HHDR(h)); \
        h = NULL;                                               \
    } while (0)

//--------------------------------------------------------------------------------------------------
// Hidden API - Not intended for direct use.

#define METAENGINE_HHDR(h) (((METAENGINE_Hhdr*)(h - 1) - 1))                                                      // Converts pointer from the user-array to table header.
#define METAENGINE_HCOOKIE 0xE6F7E359                                                                             // Magic number used for sanity/type checks.
#define METAENGINE_HCANARY(h) (h ? METADOT_ASSERT_E(METAENGINE_HHDR(h)->cookie == METAENGINE_HCOOKIE) : (void)0)  // Sanity/type check.

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef struct METAENGINE_Hslot {
    uint32_t key_hash;
    int item_index;
    int base_count;
} METAENGINE_Hslot;

typedef struct METAENGINE_Hhdr {
    int key_size;
    int item_size;
    int item_capacity;
    int count;
    int slot_capacity;
    METAENGINE_Hslot* slots;
    void* items_key;
    int* items_slot_index;
    int return_index;
    void* hidden_item;
    void* items_data;
    void* temp_key;
    void* temp_item;
    uint32_t cookie;
} METAENGINE_Hhdr;

void* METADOT_CDECL metadot_hashtable_make_impl(int key_size, int item_size, int capacity);
void METADOT_CDECL metadot_hashtable_free_impl(METAENGINE_Hhdr* table);
void* METADOT_CDECL metadot_hashtable_insert_impl(METAENGINE_Hhdr* table, uint64_t key);
void* METADOT_CDECL metadot_hashtable_insert_impl2(METAENGINE_Hhdr* table, const void* key, const void* item);
void* METADOT_CDECL metadot_hashtable_insert_impl3(METAENGINE_Hhdr* table, const void* key);
void METADOT_CDECL metadot_hashtable_remove_impl(METAENGINE_Hhdr* table, uint64_t key);
void METADOT_CDECL metadot_hashtable_remove_impl2(METAENGINE_Hhdr* table, const void* key);
bool METADOT_CDECL metadot_hashtable_has_impl(METAENGINE_Hhdr* table, uint64_t key);
int METADOT_CDECL metadot_hashtable_find_impl(const METAENGINE_Hhdr* table, uint64_t key);
int METADOT_CDECL metadot_hashtable_find_impl2(const METAENGINE_Hhdr* table, const void* key);
int METADOT_CDECL metadot_hashtable_count_impl(const METAENGINE_Hhdr* table);
void* METADOT_CDECL metadot_hashtable_items_impl(const METAENGINE_Hhdr* table);
void* METADOT_CDECL metadot_hashtable_keys_impl(const METAENGINE_Hhdr* table);
void METADOT_CDECL metadot_hashtable_clear_impl(METAENGINE_Hhdr* table);
void METADOT_CDECL metadot_hashtable_swap_impl(METAENGINE_Hhdr* table, int index_a, int index_b);

#ifdef __cplusplus
}
#endif  // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

namespace MetaEngine {

// General purpose {key, item} pair mapping via internal hash table.
// Keys are treated as mere byte buffers (Plain Old Data).
// Items have contructors/destructors called, but are *not* allowed to store references/pointers to themselves.
template <typename K, typename T>
struct Dictionary {
    Dictionary();
    Dictionary(const Dictionary<K, T>& other);
    Dictionary(Dictionary<K, T>&& other);
    Dictionary(int capacity);
    ~Dictionary();

    T& get(const K& key);
    T& find(const K& key) { return get(key); }
    const T& get(const K& key) const;
    const T& find(const K& key) const { return get(key); }
    T* try_get(const K& key);
    T* try_find(const K& key) { return try_get(key); }
    const T* try_get(const K& key) const;
    const T* try_find(const K& key) const { return try_get(key); }
    bool has(const K& key) const { return try_get(key) ? true : false; }

    T* insert(const K& key);
    T* insert(const K& key, const T& val);
    T* insert(const K& key, T&& val);
    void remove(const K& key);

    void clear();

    int count() const;
    T* items();
    const T* items() const;
    T* vals() { return items(); }
    const T* vals() const { return items(); }
    K* keys();
    const K* keys() const;

    void swap(int index_a, int index_b);

    Dictionary<K, T>& operator=(const Dictionary<K, T>& rhs);
    Dictionary<K, T>& operator=(Dictionary<K, T>&& rhs);

private:
    METAENGINE_Hhdr* m_table = NULL;
};

// -------------------------------------------------------------------------------------------------

template <typename K, typename T>
Dictionary<K, T>::Dictionary() {
    m_table = METAENGINE_HHDR((T*)metadot_hashtable_make_impl(sizeof(K), sizeof(T), 32));
}

template <typename K, typename T>
Dictionary<K, T>::Dictionary(const Dictionary<K, T>& other) {
    int n = other.count();
    const T* items = other.items();
    const K* keys = other.keys();
    if (n) {
        m_table = METAENGINE_HHDR((T*)metadot_hashtable_make_impl(sizeof(K), sizeof(T), n));
        for (int i = 0; i < n; ++i) {
            insert(keys[i], items[i]);
        }
    }
}

template <typename K, typename T>
Dictionary<K, T>::Dictionary(Dictionary<K, T>&& other) {
    m_table = other.m_table;
    other.m_table = NULL;
}

template <typename K, typename T>
Dictionary<K, T>::Dictionary(int capacity) {
    m_table = METAENGINE_HHDR((T*)metadot_hashtable_make_impl(sizeof(K), sizeof(T), capacity));
}

template <typename K, typename T>
Dictionary<K, T>::~Dictionary() {
    T* elements = items();
    for (int i = 0; i < count(); ++i) {
        (elements + i)->~T();
    }
    metadot_hashtable_free_impl(m_table);
    m_table = NULL;
}

template <typename K, typename T>
T& Dictionary<K, T>::get(const K& key) {
    int index = metadot_hashtable_find_impl2(m_table, &key);
    return items()[index];
}

template <typename K, typename T>
const T& Dictionary<K, T>::get(const K& key) const {
    int index = metadot_hashtable_find_impl2(m_table, &key);
    return items()[index];
}

template <typename K, typename T>
T* Dictionary<K, T>::try_get(const K& key) {
    if (!m_table) return NULL;
    int index = metadot_hashtable_find_impl2(m_table, &key);
    if (index >= 0)
        return items() + index;
    else
        return NULL;
}

template <typename K, typename T>
const T* Dictionary<K, T>::try_get(const K& key) const {
    if (!m_table) return NULL;
    int index = metadot_hashtable_find_impl2(m_table, &key);
    if (index >= 0)
        return items() + index;
    else
        return NULL;
}

template <typename K, typename T>
T* Dictionary<K, T>::insert(const K& key) {
    m_table = METAENGINE_HHDR((T*)metadot_hashtable_insert_impl3(m_table, &key));
    int index = m_table->return_index;
    if (index < 0) return NULL;
    T* result = items() + index;
    METAENGINE_FW_PLACEMENT_NEW(result) T();
    return result;
}

template <typename K, typename T>
T* Dictionary<K, T>::insert(const K& key, const T& val) {
    m_table = METAENGINE_HHDR((T*)metadot_hashtable_insert_impl2(m_table, &key, &val));
    int index = m_table->return_index;
    if (index < 0) return NULL;
    T* result = items() + index;
    METAENGINE_FW_PLACEMENT_NEW(result) T(val);
    return result;
}

template <typename K, typename T>
T* Dictionary<K, T>::insert(const K& key, T&& val) {
    m_table = METAENGINE_HHDR((T*)metadot_hashtable_insert_impl2(m_table, &key, &val));
    int index = m_table->return_index;
    if (index < 0) return NULL;
    T* result = items() + index;
    METAENGINE_FW_PLACEMENT_NEW(result) T(move(val));
    return result;
}

template <typename K, typename T>
void Dictionary<K, T>::remove(const K& key) {
    T* slot = try_find(key);
    if (slot) {
        slot->~T();
        metadot_hashtable_remove_impl2(m_table, &key);
    }
}

template <typename K, typename T>
void Dictionary<K, T>::clear() {
    T* elements = items();
    for (int i = 0; i < count(); ++i) {
        (elements + i)->~T();
    }
    if (m_table) metadot_hashtable_clear_impl(m_table);
}

template <typename K, typename T>
int Dictionary<K, T>::count() const {
    return m_table ? metadot_hashtable_count_impl(m_table) : 0;
}

template <typename K, typename T>
T* Dictionary<K, T>::items() {
    return m_table ? (T*)(m_table + 1) + 1 : NULL;
}

template <typename K, typename T>
const T* Dictionary<K, T>::items() const {
    return m_table ? (const T*)(m_table + 1) + 1 : NULL;
}

template <typename K, typename T>
K* Dictionary<K, T>::keys() {
    return m_table ? (K*)metadot_hashtable_keys_impl(m_table) : NULL;
}

template <typename K, typename T>
const K* Dictionary<K, T>::keys() const {
    return m_table ? (const K*)metadot_hashtable_keys_impl(m_table) : NULL;
}

template <typename K, typename T>
void Dictionary<K, T>::swap(int index_a, int index_b) {
    metadot_hashtable_swap_impl(m_table, index_a, index_b);
}

template <typename K, typename T>
Dictionary<K, T>& Dictionary<K, T>::operator=(const Dictionary<K, T>& rhs) {
    clear();
    int n = rhs.count();
    const T* items = rhs.items();
    const K* keys = rhs.keys();
    if (n) {
        m_table = METAENGINE_HHDR((T*)metadot_hashtable_make_impl(sizeof(K), sizeof(T), n));
        for (int i = 0; i < n; ++i) {
            insert(keys[i], items[i]);
        }
    }
    return *this;
}

template <typename K, typename T>
Dictionary<K, T>& Dictionary<K, T>::operator=(Dictionary<K, T>&& rhs) {
    this->~Dictionary<K, T>();
    m_table = rhs.m_table;
    rhs.m_table = NULL;
    return *this;
}

}  // namespace MetaEngine

#pragma endregion hashtable

#pragma region handle_table

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef struct METAENGINE_HandleTable METAENGINE_HandleTable;

typedef uint64_t METAENGINE_Handle;
#define METAENGINE_INVALID_HANDLE (~0ULL)

METAENGINE_HandleTable* METADOT_CDECL metadot_make_handle_allocator(int initial_capacity);
void METADOT_CDECL metadot_destroy_handle_allocator(METAENGINE_HandleTable* table);

METAENGINE_Handle METADOT_CDECL metadot_handle_allocator_alloc(METAENGINE_HandleTable* table, uint32_t index, uint16_t type /*= 0*/);
uint32_t METADOT_CDECL metadot_handle_allocator_get_index(METAENGINE_HandleTable* table, METAENGINE_Handle handle);
uint16_t METADOT_CDECL metadot_handle_allocator_get_type(METAENGINE_HandleTable* table, METAENGINE_Handle handle);
bool METADOT_CDECL metadot_handle_allocator_is_active(METAENGINE_HandleTable* table, METAENGINE_Handle handle);
void METADOT_CDECL metadot_handle_allocator_activate(METAENGINE_HandleTable* table, METAENGINE_Handle handle);
void METADOT_CDECL metadot_handle_allocator_deactivate(METAENGINE_HandleTable* table, METAENGINE_Handle handle);
void METADOT_CDECL metadot_handle_allocator_update_index(METAENGINE_HandleTable* table, METAENGINE_Handle handle, uint32_t index);
void METADOT_CDECL metadot_handle_allocator_free(METAENGINE_HandleTable* table, METAENGINE_Handle handle);
int METADOT_CDECL metadot_handle_allocator_is_handle_valid(METAENGINE_HandleTable* table, METAENGINE_Handle handle);

#ifdef __cplusplus
}
#endif  // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API
namespace MetaEngine {

using Handle = uint64_t;

struct HandleTable {
    METADOT_INLINE HandleTable(int initial_capacity = 0) : m_alloc(metadot_make_handle_allocator(initial_capacity)) {}

    METADOT_INLINE ~HandleTable() {
        metadot_destroy_handle_allocator(m_alloc);
        m_alloc = NULL;
    }

    METADOT_INLINE METAENGINE_Handle alloc_handle(uint32_t index, uint16_t type = 0) { return metadot_handle_allocator_alloc(m_alloc, index, type); }

    METADOT_INLINE METAENGINE_Handle alloc_handle() { return metadot_handle_allocator_alloc(m_alloc, ~0, 0); }

    METADOT_INLINE uint32_t get_index(METAENGINE_Handle handle) { return metadot_handle_allocator_get_index(m_alloc, handle); }

    METADOT_INLINE uint16_t get_type(METAENGINE_Handle handle) { return metadot_handle_allocator_get_type(m_alloc, handle); }

    METADOT_INLINE void update_index(METAENGINE_Handle handle, uint32_t index) { metadot_handle_allocator_update_index(m_alloc, handle, index); }

    METADOT_INLINE void free_handle(METAENGINE_Handle handle) { metadot_handle_allocator_free(m_alloc, handle); }

    METADOT_INLINE bool is_valid(METAENGINE_Handle handle) { return !!metadot_handle_allocator_is_handle_valid(m_alloc, handle); }

    METADOT_INLINE bool is_active(METAENGINE_Handle handle) { return metadot_handle_allocator_is_active(m_alloc, handle); }

    METADOT_INLINE void activate(METAENGINE_Handle handle) { metadot_handle_allocator_activate(m_alloc, handle); }

    METADOT_INLINE void deactivate(METAENGINE_Handle handle) { metadot_handle_allocator_deactivate(m_alloc, handle); }

    METAENGINE_HandleTable* m_alloc;
};

}  // namespace MetaEngine

#pragma endregion handle_table

#pragma region concurrency

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef cute_mutex_t METAENGINE_Mutex;
typedef cute_cv_t METAENGINE_ConditionVariable;
typedef cute_atomic_int_t METAENGINE_AtomicInt;
typedef cute_semaphore_t METAENGINE_Semaphore;
typedef cute_thread_t METAENGINE_Thread;
typedef cute_thread_id_t METAENGINE_ThreadId;
typedef cute_thread_fn METAENGINE_ThreadFn;
typedef cute_rw_lock_t METAENGINE_ReadWriteLock;
typedef cute_threadpool_t METAENGINE_Threadpool;

METAENGINE_Mutex METADOT_CDECL metadot_make_mutex();
void METADOT_CDECL metadot_destroy_mutex(METAENGINE_Mutex* mutex);
METAENGINE_Result METADOT_CDECL metadot_mutex_lock(METAENGINE_Mutex* mutex);
METAENGINE_Result METADOT_CDECL metadot_mutex_unlock(METAENGINE_Mutex* mutex);
bool METADOT_CDECL METAENGINE_Mutexrylock(METAENGINE_Mutex* mutex);

METAENGINE_ConditionVariable METADOT_CDECL metadot_make_cv();
void METADOT_CDECL metadot_destroy_cv(METAENGINE_ConditionVariable* cv);
METAENGINE_Result METADOT_CDECL metadot_cv_wake_all(METAENGINE_ConditionVariable* cv);
METAENGINE_Result METADOT_CDECL metadot_cv_wake_one(METAENGINE_ConditionVariable* cv);
METAENGINE_Result METADOT_CDECL metadot_cv_wait(METAENGINE_ConditionVariable* cv, METAENGINE_Mutex* mutex);

METAENGINE_Semaphore METADOT_CDECL metadot_make_sem(int initial_count);
void METADOT_CDECL metadot_destroy_sem(METAENGINE_Semaphore* semaphore);
METAENGINE_Result METADOT_CDECL metadot_sem_post(METAENGINE_Semaphore* semaphore);
METAENGINE_Result METADOT_CDECL metadot_sem_try(METAENGINE_Semaphore* semaphore);
METAENGINE_Result METADOT_CDECL metadot_sem_wait(METAENGINE_Semaphore* semaphore);
METAENGINE_Result METADOT_CDECL metadot_sem_value(METAENGINE_Semaphore* semaphore);

METAENGINE_Thread* METADOT_CDECL metadot_thread_create(METAENGINE_ThreadFn func, const char* name, void* udata);
void METADOT_CDECL metadot_thread_detach(METAENGINE_Thread* thread);
METAENGINE_ThreadId METADOT_CDECL metadot_thread_get_id(METAENGINE_Thread* thread);
METAENGINE_ThreadId METADOT_CDECL metadot_thread_id();
METAENGINE_Result METADOT_CDECL metadot_thread_wait(METAENGINE_Thread* thread);

int METADOT_CDECL metadot_core_count();
int METADOT_CDECL metadot_cacheline_size();

METAENGINE_AtomicInt METADOT_CDECL metadot_atomic_zero();
int METADOT_CDECL metadot_atomic_add(METAENGINE_AtomicInt* atomic, int addend);
int METADOT_CDECL metadot_atomic_set(METAENGINE_AtomicInt* atomic, int value);
int METADOT_CDECL metadot_atomic_get(METAENGINE_AtomicInt* atomic);
METAENGINE_Result METADOT_CDECL metadot_atomic_cas(METAENGINE_AtomicInt* atomic, int expected, int value);
void* METADOT_CDECL metadot_atomic_ptr_set(void** atomic, void* value);
void* METADOT_CDECL metadot_atomic_ptr_get(void** atomic);
METAENGINE_Result METADOT_CDECL metadot_atomic_ptr_cas(void** atomic, void* expected, void* value);

METAENGINE_ReadWriteLock METADOT_CDECL metadot_make_rw_lock();
void METADOT_CDECL metadot_destroy_rw_lock(METAENGINE_ReadWriteLock* rw);
void METADOT_CDECL metadot_read_lock(METAENGINE_ReadWriteLock* rw);
void METADOT_CDECL metadot_read_unlock(METAENGINE_ReadWriteLock* rw);
void METADOT_CDECL metadot_write_lock(METAENGINE_ReadWriteLock* rw);
void METADOT_CDECL metadot_write_unlock(METAENGINE_ReadWriteLock* rw);

typedef void(METADOT_CDECL METAENGINE_TaskFn)(void* param);

METAENGINE_Threadpool* METADOT_CDECL metadot_make_threadpool(int thread_count);
void METADOT_CDECL metadot_destroy_threadpool(METAENGINE_Threadpool* pool);
void METADOT_CDECL metadot_threadpool_add_task(METAENGINE_Threadpool* pool, METAENGINE_TaskFn* task, void* param);
void METADOT_CDECL metadot_threadpool_kick_and_wait(METAENGINE_Threadpool* pool);
void METADOT_CDECL metadot_threadpool_kick(METAENGINE_Threadpool* pool);

#ifdef __cplusplus
}
#endif  // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

namespace MetaEngine {

using Mutex = METAENGINE_Mutex;
using ConditionVariable = METAENGINE_ConditionVariable;
using AtomicInt = METAENGINE_AtomicInt;
using Semaphore = METAENGINE_Semaphore;
using Thread = METAENGINE_Thread;
using ThreadId = METAENGINE_ThreadId;
using ThreadFn = METAENGINE_ThreadFn;
using ReadWriteLock = METAENGINE_ReadWriteLock;
using Threadpool = METAENGINE_Threadpool;
using TaskFn = METAENGINE_TaskFn;

METADOT_INLINE Mutex make_mutex() { return metadot_make_mutex(); }
METADOT_INLINE void destroy_mutex(Mutex* mutex) { metadot_destroy_mutex(mutex); }
METADOT_INLINE Result mutex_lock(Mutex* mutex) { return metadot_mutex_lock(mutex); }
METADOT_INLINE Result mutex_unlock(Mutex* mutex) { return metadot_mutex_unlock(mutex); }
METADOT_INLINE bool Mutexrylock(Mutex* mutex) { return METAENGINE_Mutexrylock(mutex); }

METADOT_INLINE ConditionVariable make_cv() { return metadot_make_cv(); }
METADOT_INLINE void destroy_cv(ConditionVariable* cv) { metadot_destroy_cv(cv); }
METADOT_INLINE Result cv_wake_all(ConditionVariable* cv) { return metadot_cv_wake_all(cv); }
METADOT_INLINE Result cv_wake_one(ConditionVariable* cv) { return metadot_cv_wake_one(cv); }
METADOT_INLINE Result cv_wait(ConditionVariable* cv, Mutex* mutex) { return metadot_cv_wait(cv, mutex); }

METADOT_INLINE Semaphore make_sem(int initial_count) { return metadot_make_sem(initial_count); }
METADOT_INLINE void destroy_sem(Semaphore* semaphore) { metadot_destroy_sem(semaphore); }
METADOT_INLINE Result sem_post(Semaphore* semaphore) { return metadot_sem_post(semaphore); }
METADOT_INLINE Result sem_try(Semaphore* semaphore) { return metadot_sem_try(semaphore); }
METADOT_INLINE Result sem_wait(Semaphore* semaphore) { return metadot_sem_wait(semaphore); }
METADOT_INLINE Result sem_value(Semaphore* semaphore) { return metadot_sem_value(semaphore); }

METADOT_INLINE Thread* thread_create(ThreadFn func, const char* name, void* udata) { return metadot_thread_create(func, name, udata); }
METADOT_INLINE void thread_detach(Thread* thread) { metadot_thread_detach(thread); }
METADOT_INLINE ThreadId thread_get_id(Thread* thread) { return metadot_thread_get_id(thread); }
METADOT_INLINE ThreadId thread_id() { return metadot_thread_id(); }
METADOT_INLINE Result thread_wait(Thread* thread) { return metadot_thread_wait(thread); }

METADOT_INLINE int core_count() { return metadot_core_count(); }
METADOT_INLINE int cacheline_size() { return metadot_cacheline_size(); }

METADOT_INLINE AtomicInt atomic_zero() { return metadot_atomic_zero(); }
METADOT_INLINE int atomic_add(AtomicInt* atomic, int addend) { return metadot_atomic_add(atomic, addend); }
METADOT_INLINE int atomic_set(AtomicInt* atomic, int value) { return metadot_atomic_set(atomic, value); }
METADOT_INLINE int atomic_get(AtomicInt* atomic) { return metadot_atomic_get(atomic); }
METADOT_INLINE Result atomic_cas(AtomicInt* atomic, int expected, int value) { return metadot_atomic_cas(atomic, expected, value); }
METADOT_INLINE void* atomic_ptr_set(void** atomic, void* value) { return metadot_atomic_ptr_set(atomic, value); }
METADOT_INLINE void* atomic_ptr_get(void** atomic) { return metadot_atomic_ptr_get(atomic); }
METADOT_INLINE Result atomic_ptr_cas(void** atomic, void* expected, void* value) { return metadot_atomic_ptr_cas(atomic, expected, value); }

METADOT_INLINE ReadWriteLock make_rw_lock() { return metadot_make_rw_lock(); }
METADOT_INLINE void destroy_rw_lock(ReadWriteLock* rw) { metadot_destroy_rw_lock(rw); }
METADOT_INLINE void read_lock(ReadWriteLock* rw) { metadot_read_lock(rw); }
METADOT_INLINE void read_unlock(ReadWriteLock* rw) { metadot_read_unlock(rw); }
METADOT_INLINE void write_lock(ReadWriteLock* rw) { metadot_write_lock(rw); }
METADOT_INLINE void write_unlock(ReadWriteLock* rw) { metadot_write_unlock(rw); }

METADOT_INLINE Threadpool* make_threadpool(int thread_count) { return metadot_make_threadpool(thread_count); }
METADOT_INLINE void destroy_threadpool(Threadpool* pool) { return metadot_destroy_threadpool(pool); }
METADOT_INLINE void threadpool_add_task(Threadpool* pool, TaskFn* task, void* param) { return metadot_threadpool_add_task(pool, task, param); }
METADOT_INLINE void threadpool_kick_and_wait(Threadpool* pool) { return metadot_threadpool_kick_and_wait(pool); }
METADOT_INLINE void threadpool_kick(Threadpool* pool) { return metadot_threadpool_kick(pool); }

}  // namespace MetaEngine

#pragma endregion concurrency

#endif  // METAENGINE_ARRAY_H
