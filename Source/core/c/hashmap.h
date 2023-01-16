
#ifndef _METADOT_C_HASHMAP_H_
#define _METADOT_C_HASHMAP_H_

#if defined(_MSC_VER)
// Workaround a bug in the MSVC runtime where it uses __cplusplus when not
// defined.
#pragma warning(push, 0)
#pragma warning(disable : 4668)
#endif
#include <stdlib.h>
#include <string.h>

#if (defined(_MSC_VER) && defined(__AVX__)) || (!defined(_MSC_VER) && defined(__SSE4_2__))
#define HASHMAP_SSE42
#endif

#if defined(HASHMAP_SSE42)
#include <nmmintrin.h>
#endif

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(_MSC_VER)
#pragma warning(push)
/* Stop MSVC complaining about unreferenced functions */
#pragma warning(disable : 4505)
/* Stop MSVC complaining about not inlining functions */
#pragma warning(disable : 4710)
/* Stop MSVC complaining about inlining functions! */
#pragma warning(disable : 4711)
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

#if defined(_MSC_VER)
#define HASHMAP_USED
#elif defined(__GNUC__)
#define HASHMAP_USED __attribute__((used))
#else
#define HASHMAP_USED
#endif

/* We need to keep keys and values. */
struct hashmap_element_s {
    const char *key;
    unsigned key_len;
    int in_use;
    void *data;
};

/* A hashmap has some maximum size and current size, as well as the data to
 * hold. */
struct hashmap_s {
    unsigned table_size;
    unsigned size;
    struct hashmap_element_s *data;
};

#define HASHMAP_MAX_CHAIN_LENGTH (8)

#if defined(__cplusplus)
extern "C" {
#endif

/// @brief Create a hashmap.
/// @param initial_size The initial size of the hashmap. Must be a power of two.
/// @param out_hashmap The storage for the created hashmap.
/// @return On success 0 is returned.
///
/// Note that the initial size of the hashmap must be a power of two, and
/// creation of the hashmap will fail if this is not the case.
static int hashmap_create(const unsigned initial_size, struct hashmap_s *const out_hashmap) HASHMAP_USED;

/// @brief Put an element into the hashmap.
/// @param hashmap The hashmap to insert into.
/// @param key The string key to use.
/// @param len The length of the string key.
/// @param value The value to insert.
/// @return On success 0 is returned.
///
/// The key string slice is not copied when creating the hashmap entry, and thus
/// must remain a valid pointer until the hashmap entry is removed or the
/// hashmap is destroyed.
static int hashmap_put(struct hashmap_s *const hashmap, const char *const key, const unsigned len, void *const value) HASHMAP_USED;

/// @brief Get an element from the hashmap.
/// @param hashmap The hashmap to get from.
/// @param key The string key to use.
/// @param len The length of the string key.
/// @return The previously set element, or NULL if none exists.
static void *hashmap_get(const struct hashmap_s *const hashmap, const char *const key, const unsigned len) HASHMAP_USED;

/// @brief Remove an element from the hashmap.
/// @param hashmap The hashmap to remove from.
/// @param key The string key to use.
/// @param len The length of the string key.
/// @return On success 0 is returned.
static int hashmap_remove(struct hashmap_s *const hashmap, const char *const key, const unsigned len) HASHMAP_USED;

/// @brief Remove an element from the hashmap.
/// @param hashmap The hashmap to remove from.
/// @param key The string key to use.
/// @param len The length of the string key.
/// @return On success the original stored key pointer is returned, on failure
/// NULL is returned.
static const char *hashmap_remove_and_return_key(struct hashmap_s *const hashmap, const char *const key, const unsigned len) HASHMAP_USED;

/// @brief Iterate over all the elements in a hashmap.
/// @param hashmap The hashmap to iterate over.
/// @param f The function pointer to call on each element.
/// @param context The context to pass as the first argument to f.
/// @return If the entire hashmap was iterated then 0 is returned. Otherwise if
/// the callback function f returned non-zero then non-zero is returned.
static int hashmap_iterate(const struct hashmap_s *const hashmap, int (*f)(void *const context, void *const value), void *const context) HASHMAP_USED;

/// @brief Iterate over all the elements in a hashmap.
/// @param hashmap The hashmap to iterate over.
/// @param f The function pointer to call on each element.
/// @param context The context to pass as the first argument to f.
/// @return If the entire hashmap was iterated then 0 is returned.
/// Otherwise if the callback function f returned positive then the positive
/// value is returned.  If the callback function returns -1, the current item
/// is removed and iteration continues.
static int hashmap_iterate_pairs(struct hashmap_s *const hashmap, int (*f)(void *const, struct hashmap_element_s *const), void *const context) HASHMAP_USED;

/// @brief Get the size of the hashmap.
/// @param hashmap The hashmap to get the size of.
/// @return The size of the hashmap.
static unsigned hashmap_num_entries(const struct hashmap_s *const hashmap) HASHMAP_USED;

/// @brief Destroy the hashmap.
/// @param hashmap The hashmap to destroy.
static void hashmap_destroy(struct hashmap_s *const hashmap) HASHMAP_USED;

static unsigned hashmap_crc32_helper(const char *const s, const unsigned len) HASHMAP_USED;
static unsigned hashmap_hash_helper_int_helper(const struct hashmap_s *const m, const char *const keystring, const unsigned len) HASHMAP_USED;
static int hashmap_match_helper(const struct hashmap_element_s *const element, const char *const key, const unsigned len) HASHMAP_USED;
static int hashmap_hash_helper(const struct hashmap_s *const m, const char *const key, const unsigned len, unsigned *const out_index) HASHMAP_USED;
static int hashmap_rehash_iterator(void *const new_hash, struct hashmap_element_s *const e) HASHMAP_USED;
static int hashmap_rehash_helper(struct hashmap_s *const m) HASHMAP_USED;

#if defined(__cplusplus)
}
#endif

#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
