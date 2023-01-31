

#include "stl.h"

#include "core/alloc.hpp"
#include "sdl_wrapper.h"

#define CUTE_SYNC_IMPLEMENTATION
#ifdef METADOT_PLATFORM_WINDOWS
#define CUTE_SYNC_WINDOWS
#else
#define CUTE_SYNC_SDL
#endif
#define METAENGINE_THREAD_ALLOC METAENGINE_FW_ALLOC
#define METAENGINE_THREAD_FREE METAENGINE_FW_FREE
#include "libs/cute/cute_sync.h"

using namespace MetaEngine;

#pragma region array

void* metadot_agrow(const void* a, int new_size, size_t element_size) {
    METAENGINE_ACANARY(a);
    METADOT_ASSERT_E(acap(a) <= (SIZE_MAX - 1) / 2);
    int new_capacity = std::max(2 * acap(a), std::max(new_size, 16));
    METADOT_ASSERT_E(new_size <= new_capacity);
    METADOT_ASSERT_E(new_capacity <= (SIZE_MAX - sizeof(METAENGINE_Ahdr)) / element_size);
    size_t total_size = sizeof(METAENGINE_Ahdr) + new_capacity * element_size;
    METAENGINE_Ahdr* hdr;
    if (a) {
        if (!METAENGINE_AHDR(a)->is_static) {
            hdr = (METAENGINE_Ahdr*)METAENGINE_FW_REALLOC(METAENGINE_AHDR(a), total_size);
        } else {
            hdr = (METAENGINE_Ahdr*)METAENGINE_FW_ALLOC(total_size);
            METAENGINE_MEMCPY(hdr + 1, a, alen(a) * element_size);
            hdr->size = asize(a);
            hdr->cookie = METAENGINE_ACOOKIE;
        }
    } else {
        hdr = (METAENGINE_Ahdr*)METAENGINE_FW_ALLOC(total_size);
        hdr->size = 0;
        hdr->cookie = METAENGINE_ACOOKIE;
    }
    hdr->capacity = new_capacity;
    hdr->is_static = false;
    hdr->data = (char*)(hdr + 1);  // For debugging convenience.
    return (void*)(hdr + 1);
}

void* metadot_astatic(const void* a, int buffer_size, size_t element_size) {
    METAENGINE_Ahdr* hdr = (METAENGINE_Ahdr*)a;
    hdr->size = 0;
    hdr->cookie = METAENGINE_ACOOKIE;
    if (sizeof(METAENGINE_Ahdr) <= element_size) {
        hdr->capacity = buffer_size / (int)element_size - 1;
    } else {
        int elements_taken = sizeof(METAENGINE_Ahdr) / (int)element_size + (sizeof(METAENGINE_Ahdr) % (int)element_size > 0);
        hdr->capacity = buffer_size / (int)element_size - elements_taken;
    }
    hdr->data = (char*)(hdr + 1);  // For debugging convenience.
    hdr->is_static = true;
    return (void*)(hdr + 1);
}

void* metadot_aset(const void* a, const void* b, size_t element_size) {
    METAENGINE_ACANARY(a);
    METAENGINE_ACANARY(b);
    if (acap(a) < asize(b)) {
        int len = asize(b);
        a = metadot_agrow(a, asize(b), element_size);
    }
    METAENGINE_MEMCPY((void*)a, b, asize(b) * element_size);
    alen(a) = asize(b);
    return (void*)a;
}

#pragma endregion array

#pragma region string

char* metadot_sset(char* a, const char* b) {
    METAENGINE_ACANARY(a);
    int bsize = (int)(b ? METAENGINE_STRLEN(b) : 0) + 1;
    if (!bsize) {
        sclear(a);
        return a;
    } else if (acap(a) < bsize) {
        a = (char*)metadot_agrow(a, bsize, 1);
    }
    METAENGINE_MEMCPY((void*)a, b, bsize);
    string_size(a) = bsize;
    return a;
}

char* metadot_sfmt(char* s, const char* fmt, ...) {
    METAENGINE_ACANARY(s);
    va_list args;
    va_start(args, fmt);
    int n = 1 + vsnprintf(s, scap(s), fmt, args);
    va_end(args);
    if (n > scap(s)) {
        sfit(s, n);
        va_start(args, fmt);
        n = 1 + vsnprintf(s, scap(s), fmt, args);
        va_end(args);
    }
    string_size(s) = n;
    return s;
}

char* metadot_sfmt_append(char* s, const char* fmt, ...) {
    METAENGINE_ACANARY(s);
    va_list args;
    va_start(args, fmt);
    int capacity = scap(s) - scount(s);
    int n = 1 + vsnprintf(s + slen(s), capacity, fmt, args);
    va_end(args);
    if (n > capacity) {
        afit(s, n + slen(s));
        va_start(args, fmt);
        int new_capacity = scap(s) - scount(s);
        n = 1 + vsnprintf(s + slen(s), new_capacity, fmt, args);
        METADOT_ASSERT_E(n <= new_capacity);
        va_end(args);
    }
    alen(s) += n - 1;
    return s;
}

char* metadot_svfmt(char* s, const char* fmt, va_list args) {
    METAENGINE_ACANARY(s);
    va_list copy_args;
    va_copy(copy_args, args);
    int n = 1 + vsnprintf(s, scap(s), fmt, args);
    if (n > scap(s)) {
        sfit(s, n);
        n = 1 + vsnprintf(s, scap(s), fmt, copy_args);
        va_end(copy_args);
    }
    string_size(s) = n;
    return s;
}

char* metadot_svfmt_append(char* s, const char* fmt, va_list args) {
    METAENGINE_ACANARY(s);
    va_list copy_args;
    va_copy(copy_args, args);
    int capacity = scap(s) - scount(s);
    int n = 1 + vsnprintf(s + slen(s), capacity, fmt, copy_args);
    va_end(copy_args);
    if (n > capacity) {
        afit(s, n + slen(s));
        int new_capacity = scap(s) - scount(s);
        n = 1 + vsnprintf(s + slen(s), new_capacity, fmt, args);
        METADOT_ASSERT_E(n <= new_capacity);
    }
    alen(s) += n - 1;
    return s;
}

bool metadot_sprefix(char* s, const char* prefix) {
    METAENGINE_ACANARY(s);
    int prefix_len = (int)(prefix ? METAENGINE_STRLEN(prefix) : 0);
    bool a = slen(s) >= prefix_len;
    bool b = !METAENGINE_MEMCMP(s, prefix, prefix_len);
    return a && b;
}

bool metadot_ssuffix(char* s, const char* suffix) {
    METAENGINE_ACANARY(s);
    int suffix_len = (int)(suffix ? METAENGINE_STRLEN(suffix) : 0);
    bool a = slen(s) >= suffix_len;
    bool b = !METAENGINE_MEMCMP(s + slen(s) - suffix_len, suffix, suffix_len);
    return a && b;
}

void metadot_stoupper(char* s) {
    METAENGINE_ACANARY(s);
    if (!s) return;
    for (int i = 0; i < slen(s); ++i) {
        s[i] = toupper(s[i]);
    }
}

void metadot_stolower(char* s) {
    METAENGINE_ACANARY(s);
    if (!s) return;
    for (int i = 0; i < slen(s); ++i) {
        s[i] = tolower(s[i]);
    }
}

char* metadot_sappend(char* a, const char* b) {
    METAENGINE_ACANARY(a);
    int blen = (int)METAENGINE_STRLEN(b);
    if (blen <= 0) return a;
    sfit(a, slen(a) + blen + 1);
    METAENGINE_MEMCPY(a + slen(a), b, blen);
    string_size(a) += blen;
    a[slen(a)] = 0;
    return a;
}

char* metadot_sappend_range(char* a, const char* b, const char* b_end) {
    METAENGINE_ACANARY(a);
    int blen = (int)(b_end - b);
    if (blen <= 0) return a;
    sfit(a, slen(a) + blen + 1);
    METAENGINE_MEMCPY(a + slen(a), b, blen);
    string_size(a) += blen;
    a[slen(a)] = 0;
    return a;
}

char* metadot_strim(char* s) {
    METAENGINE_ACANARY(s);
    char* start = s;
    char* end = s + slen(s) - 1;
    while (isspace(*start)) start++;
    while (isspace(*end)) end--;
    size_t len = end - start + 1;
    METAENGINE_MEMMOVE(s, start, len);
    s[len] = 0;
    string_size(s) = (int)(len + 1);
    return s;
}

char* metadot_sltrim(char* s) {
    METAENGINE_ACANARY(s);
    char* start = s;
    while (isspace(*start)) start++;
    size_t len = slen(s) - (start - s);
    METAENGINE_MEMMOVE(s, start, len);
    s[len] = 0;
    string_size(s) = (int)(len + 1);
    return s;
}

char* metadot_srtrim(char* s) {
    METAENGINE_ACANARY(s);
    while (isspace(*(s + slen(s) - 1))) string_size(s)--;
    s[slen(s)] = 0;
    return s;
}

char* metadot_slpad(char* s, char pad, int count) {
    METAENGINE_ACANARY(s);
    int cap = scap(s) - scount(s);
    if (cap < count) {
        sfit(s, scount(s) + cap);
    }
    METAENGINE_MEMMOVE(s + count, s, scount(s));
    METAENGINE_MEMSET(s, pad, count);
    string_size(s) += count;
    return s;
}

char* metadot_srpad(char* s, char pad, int count) {
    METAENGINE_ACANARY(s);
    int cap = scap(s) - scount(s);
    if (cap < count) {
        sfit(s, scount(s) + cap);
    }
    METAENGINE_MEMSET(s + slen(s), pad, count);
    string_size(s) += count;
    s[slen(s)] = 0;
    return s;
}

char* metadot_ssplit_once(char* s, char split_c) {
    METAENGINE_ACANARY(s);
    char* start = s;
    char* end = s + slen(s) - 1;
    while (start < end) {
        if (*start == split_c) {
            break;
        }
        ++start;
    }
    int len = (int)(start - s);
    if (len + 1 == slen(s)) return NULL;
    char* split = NULL;
    sfit(split, len + 1);
    string_size(split) = len + 1;
    METAENGINE_MEMCPY(split, s, len);
    split[len] = 0;
    int new_len = slen(s) - len - 1;
    METAENGINE_MEMMOVE(s, s + len + 1, new_len);
    string_size(s) = new_len + 1;
    s[new_len] = 0;
    return split;
}

char** metadot_ssplit(const char* s, char split_c) {
    METAENGINE_ACANARY(s);
    char* copy = NULL;
    char** result = NULL;
    char* split = NULL;
    sset(copy, s);
    while ((split = ssplit_once(copy, split_c))) {
        apush(result, split);
    }
    apush(result, copy);
    return result;
}

int metadot_sfirst_index_of(const char* s, char c) {
    if (!s) return -1;
    const char* p = METAENGINE_STRCHR(s, c);
    if (!p) return -1;
    return ((int)(uintptr_t)(p - s));
}

int metadot_slast_index_of(const char* s, char c) {
    if (!s) return -1;
    const char* p = METAENGINE_STRRCHR(s, c);
    if (!p) return -1;
    return ((int)(uintptr_t)(p - s));
}

static uint64_t s_stoint(const char* s) {
    char* end;
    uint64_t result = METAENGINE_STRTOLL(s, &end, 10);
    METADOT_ASSERT_E(end == s + METAENGINE_STRLEN(s));
    return result;
}

int metadot_stoint(const char* s) { return (int)s_stoint(s); }

uint64_t metadot_stouint(const char* s) { return s_stoint(s); }

static double s_stod(const char* s) {
    char* end;
    double result = METAENGINE_STRTOD(s, &end);
    METADOT_ASSERT_E(end == s + METAENGINE_STRLEN(s));
    return result;
}

float metadot_stofloat(const char* s) { return (float)s_stod(s); }

double metadot_stodouble(const char* s) { return s_stod(s); }

uint64_t metadot_stohex(const char* s) {
    if (!METAENGINE_STRNCMP(s, "#", 1)) s += 1;
    if (!METAENGINE_STRNCMP(s, "0x", 2)) s += 2;
    char* end;
    uint64_t result = METAENGINE_STRTOLL(s, &end, 16);
    METADOT_ASSERT_E(end == s + METAENGINE_STRLEN(s));
    return result;
}

char* metadot_sreplace(char* s, const char* replace_me, const char* with_me) {
    METAENGINE_ACANARY(s);
    size_t replace_len = METAENGINE_STRLEN(replace_me);
    size_t with_len = METAENGINE_STRLEN(with_me);
    char* start = s;
    char* find;
    while ((find = sfind(s, replace_me))) {
        if (replace_len > with_len) {
            int remaining = scount(s) - (int)(s - start) - (int)with_len;
            METAENGINE_MEMCPY(find, with_me, with_len);
            METAENGINE_MEMMOVE(find + with_len, find + replace_len, remaining);
            string_size(s) -= (int)(replace_len - with_len);
        } else {
            int remaining = scount(s) - (int)(s - start) - (int)replace_len;
            int diff = (int)(with_len - replace_len);
            sfit(s, scount(s) + diff);
            METAENGINE_MEMMOVE(find + with_len, find + replace_len, remaining);
            METAENGINE_MEMCPY(find, with_me, with_len);
            string_size(s) += diff;
        }
    }
    return s;
}

char* metadot_serase(char* s, int index, int count) {
    METAENGINE_ACANARY(s);
    if (index < 0) {
        count += index;
        index = 0;
        if (count <= 0) return s;
    }
    if (index >= slen(s)) return s;
    if (index + count >= slen(s)) {
        string_size(s) = index + 1;
        s[index] = 0;
        return s;
    } else {
        int remaining = scount(s) - (count + index) + 1;
        METAENGINE_MEMMOVE(s + index, s + count + index, remaining);
        string_size(s) -= count;
    }
    return s;
}

char* metadot_spop(char* s) {
    METAENGINE_ACANARY(s);
    if (s && slen(s)) s[--string_size(s) - 1] = 0;
    return s;
}

char* metadot_spopn(char* s, int n) {
    METAENGINE_ACANARY(s);
    if (!s || n < 0) return s;
    while (string_size(s) > 1 && n--) {
        string_size(s)--;
    }
    s[slen(s)] = 0;
    return s;
}

using intern_t = metadot_intern_t;

struct intern_table_t {
    htbl intern_t** interns;
    Arena arena;
    ReadWriteLock lock;

    METADOT_INLINE void read_lock() { metadot_read_lock(&lock); }
    METADOT_INLINE void read_unlock() { metadot_read_unlock(&lock); }
    METADOT_INLINE void write_lock() { metadot_write_lock(&lock); }
    METADOT_INLINE void write_unlock() { metadot_write_unlock(&lock); }
};

static intern_table_t* g_intern_table;

static intern_table_t* s_inst() {
    // Locklessly get/instantiate a global instance.
    intern_table_t* inst = (intern_table_t*)metadot_atomic_ptr_get((void**)&g_intern_table);
    if (!inst) {
        // Create a new instance of the table.
        inst = (intern_table_t*)METAENGINE_FW_ALLOC(sizeof(intern_table_t));
        METAENGINE_MEMSET(inst, 0, sizeof(*inst));
        inst->interns = NULL;
        inst->arena.alignment = 8;
        inst->arena.block_size = METAENGINE_MB;
        inst->lock = metadot_make_rw_lock();

        // Try and set the global pointer. If this fails it means another thread
        // has raced us and completed first, so then just destroy ours and use theirs.
        Result result = metadot_atomic_ptr_cas((void**)&g_intern_table, NULL, inst);
        if (is_error(result)) {
            metadot_destroy_rw_lock(&inst->lock);
            METAENGINE_FW_FREE(inst);
            inst = (intern_table_t*)metadot_atomic_ptr_get((void**)&g_intern_table);
            METADOT_ASSERT_E(inst);
        }
    }
    return inst;
}

const char* metadot_sintern(const char* s) { return metadot_sintern_range(s, s + METAENGINE_STRLEN(s)); }

const char* metadot_sintern_range(const char* start, const char* end) {
    intern_table_t* table = s_inst();
    intern_t* intern = NULL;
    int len = (int)(end - start);
    uint64_t hash = fnv1a(start, len);

    // Fast-path, fetch already intern'd strings and return them as-is.
    // Uses a mere read-lock, which is just a couple atomic read operations
    // and supports *many* simultaneous lockless readers.
    if (table->interns) {
        table->read_lock();
        intern = hget(table->interns, hash);
        table->read_unlock();
        if (intern) {
            // This loop is highly unlikely to ever actually run more than one iteration.
            intern_t* i = intern;
            while (i) {
                if (len == i->len && !METAENGINE_STRNCMP(i->string, start, len)) {
                    return i->string;
                }
                i = i->next;
            }
        }
    }

    // String is not yet interned, create a new allocation for it.
    // We write-lock the data structure, this will wait for all readers to flush.
    METADOT_ASSERT_E(!intern || intern->cookie == METAENGINE_INTERN_COOKIE);
    intern_t* list = intern;
    table->write_lock();
    intern = (intern_t*)arena_alloc(&table->arena, sizeof(intern_t) + len + 1);
    hset(table->interns, hash, intern);
    intern->cookie = METAENGINE_INTERN_COOKIE;
    intern->len = len;
    intern->string = (char*)(intern + 1);
    METAENGINE_MEMCPY((char*)intern->string, start, len);
    ((char*)intern->string)[len] = 0;
    intern->next = list;
    table->write_unlock();

    // Return a copy of the string as a stable pointer.
    return intern->string;
}

void metadot_sinuke_intern_table() {
    intern_table_t* table = s_inst();
    table->write_lock();
    metadot_atomic_ptr_set((void**)&g_intern_table, NULL);
    table->write_unlock();
    arena_reset(&table->arena);
    hfree(table->interns);
    metadot_destroy_rw_lock(&table->lock);
    METAENGINE_FW_FREE(table);
}

// All invalid characters are encoded as the "replacement character" 0xFFFD for both
// UTF8 and UTF16 functions.

char* metadot_string_append_UTF8_impl(char* s, int codepoint) {
    METAENGINE_ACANARY(s);
    if (codepoint > 0x10FFFF) codepoint = 0xFFFD;
#define METAENGINE_EMIT(X, Y, Z) spush(s, X | ((codepoint >> Y) & Z))
    if (codepoint < 0x80) {
        METAENGINE_EMIT(0x00, 0, 0x7F);
    } else if (codepoint < 0x800) {
        METAENGINE_EMIT(0xC0, 6, 0x1F);
        METAENGINE_EMIT(0x80, 0, 0x3F);
    } else if (codepoint < 0x10000) {
        METAENGINE_EMIT(0xE0, 12, 0xF);
        METAENGINE_EMIT(0x80, 6, 0x3F);
        METAENGINE_EMIT(0x80, 0, 0x3F);
    } else {
        METAENGINE_EMIT(0xF0, 18, 0x7);
        METAENGINE_EMIT(0x80, 12, 0x3F);
        METAENGINE_EMIT(0x80, 6, 0x3F);
        METAENGINE_EMIT(0x80, 0, 0x3F);
    }
    return s;
}

const char* metadot_decode_UTF8(const char* s, int* codepoint) {
    unsigned char c = *s++;
    int extra = 0;
    int min = 0;
    *codepoint = 0;
    if (c >= 0xF0) {
        *codepoint = c & 0x07;
        extra = 3;
        min = 0x10000;
    } else if (c >= 0xE0) {
        *codepoint = c & 0x0F;
        extra = 2;
        min = 0x800;
    } else if (c >= 0xC0) {
        *codepoint = c & 0x1F;
        extra = 1;
        min = 0x80;
    } else if (c >= 0x80) {
        *codepoint = 0xFFFD;
    } else
        *codepoint = c;
    while (extra--) {
        c = *s++;
        if ((c & 0xC0) != 0x80) {
            *codepoint = 0xFFFD;
            break;
        }
        *codepoint = ((*codepoint) << 6) | (c & 0x3F);
    }
    if (*codepoint < min) *codepoint = 0xFFFD;
    return s;
}

const uint16_t* metadot_decode_UTF16(const uint16_t* s, int* codepoint) {
    int W1 = *s++;
    if (W1 < 0xD800 || W1 > 0xDFFF) {
        *codepoint = W1;
    } else if (W1 > 0xD800 && W1 < 0xDBFF) {
        int W2 = *s++;
        if (W2 > 0xDC00 && W2 < 0xDFFF)
            *codepoint = 0x10000 + (((W1 & 0x03FF) << 10) | W2 & 0x03FF);
        else
            *codepoint = 0xFFFD;
    } else
        *codepoint = 0xFFFD;
    return s;
}

#pragma endregion string

#pragma region result

static int s_message_box_flags(METAENGINE_MessageBoxType type) {
    switch (type) {
        case METAENGINE_MESSAGE_BOX_TYPE_ERROR:
            return SDL_MESSAGEBOX_ERROR;
        case METAENGINE_MESSAGE_BOX_TYPE_WARNING:
            return SDL_MESSAGEBOX_WARNING;
        case METAENGINE_MESSAGE_BOX_TYPE_INFORMATION:
            return SDL_MESSAGEBOX_INFORMATION;
    }
    return SDL_MESSAGEBOX_ERROR;
}

void metadot_message_box(METAENGINE_MessageBoxType type, const char* title, const char* text) {
    // TODO SDL windows
    SDL_ShowSimpleMessageBox(s_message_box_flags(type), title, text, NULL);
}

#pragma endregion result

#pragma region hashtable

// Original implementation by Mattias Gustavsson
// https://github.com/mattiasgustavsson/libs/blob/main/hashtable.h

// Prime table sizes are used to help security of the table at the expense of the % operator,
// as opposed to the speed << operator, for lookups.
// By using prime table sizes we ensure all bits of each hash are utilized. This helps mitigate
// DoS attacks on the table lookup function.
static int s_primes[] = {31,     67,      127,     257,     509,     1021,     2053,     4099,     8191,      16381,     32771,     65537,      131071,    262147,
                         524287, 1048573, 2097143, 4194301, 8388617, 16777213, 33554467, 67108859, 134217757, 268435459, 536870909, 1073741827, 2147483647};

static METADOT_INLINE int s_next_prime(int a) {
    int i = 0;
    while (s_primes[i] <= a) ++i;
    return s_primes[i];
}

static METADOT_INLINE void* s_get_item(const METAENGINE_Hhdr* table, int index) {
    uint8_t* items = (uint8_t*)table->items_data;
    return items + index * table->item_size;
}

void* metadot_hashtable_make_impl(int key_size, int item_size, int capacity) {
    METADOT_ASSERT_E(capacity);

    METAENGINE_Hhdr* table = (METAENGINE_Hhdr*)METAENGINE_FW_CALLOC(sizeof(METAENGINE_Hhdr) + (capacity + 1) * item_size);
    table->cookie = METAENGINE_HCOOKIE;
    table->key_size = key_size;
    table->item_size = item_size;

    // Space is made for a zero'd out "hidden item" to represent failed lookups.
    // This is critical to support return-by-value polymorphism in the C macro API for `hget` and `hfind`.
    // We also "pass" in values to `hadd` through this space.
    table->hidden_item = (void*)((uintptr_t)(table + 1));
    table->items_data = (void*)((uintptr_t)(table + 1) + item_size);
    table->slot_capacity = s_next_prime(capacity);
    table->slots = (METAENGINE_Hslot*)METAENGINE_FW_CALLOC(table->slot_capacity * sizeof(METAENGINE_Hslot));
    for (int i = 0; i < table->slot_capacity; ++i) {
        table->slots[i].item_index = -1;
    }
    table->item_capacity = capacity;
    table->items_key = METAENGINE_FW_ALLOC(capacity * key_size);
    table->items_slot_index = (int*)METAENGINE_FW_ALLOC(capacity * sizeof(*table->items_slot_index));
    table->temp_key = METAENGINE_FW_ALLOC(key_size);
    table->temp_item = METAENGINE_FW_ALLOC(item_size);

    return s_get_item(table, 0);
}

void metadot_hashtable_free_impl(METAENGINE_Hhdr* table) {
    if (!table) return;
    METAENGINE_FW_FREE(table->slots);
    METAENGINE_FW_FREE(table->items_key);
    METAENGINE_FW_FREE(table->items_slot_index);
    METAENGINE_FW_FREE(table->temp_key);
    METAENGINE_FW_FREE(table->temp_item);
    METAENGINE_FW_FREE(table);
}

static METADOT_INLINE int s_keys_equal(const METAENGINE_Hhdr* table, const void* a, const void* b) { return !METAENGINE_MEMCMP(a, b, table->key_size); }

static METADOT_INLINE void* s_get_key(const METAENGINE_Hhdr* table, int index) {
    uint8_t* keys = (uint8_t*)table->items_key;
    return keys + index * table->key_size;
}

static int s_find_slot(const METAENGINE_Hhdr* table, uint32_t hash, const void* key) {
    uint32_t slot_capacity = (uint32_t)table->slot_capacity;
    int base_slot = (int)(hash % slot_capacity);
    int base_count = table->slots[base_slot].base_count;
    int slot = base_slot;

    while (base_count > 0) {
        if (table->slots[slot].item_index >= 0) {
            uint32_t slot_hash = table->slots[slot].key_hash;
            int slot_base = (int)(slot_hash % slot_capacity);
            if (slot_base == base_slot) {
                METADOT_ASSERT_E(base_count > 0);
                --base_count;
                const void* slot_key = s_get_key(table, table->slots[slot].item_index);
                if (slot_hash == hash && s_keys_equal(table, slot_key, key)) {
                    return slot;
                }
            }
        }
        slot = (int)((slot + 1) % table->slot_capacity);
    }

    return -1;
}

static void s_expand_slots(METAENGINE_Hhdr* table) {
    int old_capacity = table->slot_capacity;
    METAENGINE_Hslot* old_slots = table->slots;

    table->slot_capacity = s_next_prime(table->slot_capacity);
    uint32_t slot_capacity = (uint32_t)table->slot_capacity;

    table->slots = (METAENGINE_Hslot*)METAENGINE_FW_CALLOC(table->slot_capacity * sizeof(METAENGINE_Hslot));
    METADOT_ASSERT_E(table->slots);
    for (int i = 0; i < table->slot_capacity; ++i) {
        table->slots[i].item_index = -1;
    }

    for (int i = 0; i < old_capacity; ++i) {
        if (old_slots[i].item_index >= 0) {
            uint32_t hash = old_slots[i].key_hash;
            int base_slot = (int)(hash % slot_capacity);
            int slot = base_slot;
            while (table->slots[slot].item_index >= 0) {
                slot = (slot + 1) % slot_capacity;
            }
            table->slots[slot].key_hash = hash;
            int item_index = old_slots[i].item_index;
            table->slots[slot].item_index = item_index;
            table->items_slot_index[item_index] = slot;
            ++table->slots[base_slot].base_count;
        }
    }

    METAENGINE_FW_FREE(old_slots);
}

static METAENGINE_Hhdr* s_expand_items(METAENGINE_Hhdr* table) {
    int capacity = table->item_capacity * 2;
    table = (METAENGINE_Hhdr*)METAENGINE_FW_REALLOC(table, sizeof(METAENGINE_Hhdr) + (capacity + 1) * table->item_size);
    table->item_capacity = capacity;
    table->hidden_item = (void*)((uintptr_t)(table + 1));
    table->items_data = (void*)((uintptr_t)(table + 1) + table->item_size);
    table->items_key = METAENGINE_FW_REALLOC(table->items_key, capacity * table->key_size);
    table->items_slot_index = (int*)METAENGINE_FW_REALLOC(table->items_slot_index, capacity * sizeof(*table->items_slot_index));
    return table;
}

void* metadot_hashtable_insert_impl2(METAENGINE_Hhdr* table, const void* key, const void* item) {
    uint32_t hash = (uint32_t)fnv1a(key, table->key_size);
    METADOT_ASSERT_E(s_find_slot(table, hash, key) < 0);

    if (table->count >= (table->slot_capacity - table->slot_capacity / 3)) {
        s_expand_slots(table);
    }

    uint32_t slot_capacity = (uint32_t)table->slot_capacity;
    int base_slot = (int)(hash % slot_capacity);
    int base_count = table->slots[base_slot].base_count;
    int slot = base_slot;
    int first_free = slot;
    while (base_count) {
        if (table->slots[slot].item_index < 0 && table->slots[first_free].item_index >= 0) first_free = slot;
        uint32_t slot_hash = table->slots[slot].key_hash;
        int slot_base = (int)(slot_hash % slot_capacity);
        if (slot_base == base_slot) {
            --base_count;
        }
        slot = (slot + 1) % table->slot_capacity;
    }

    slot = first_free;
    while (table->slots[slot].item_index >= 0) {
        slot = (slot + 1) % slot_capacity;
    }

    if (table->count >= table->item_capacity) {
        table = s_expand_items(table);

        // Update the "hidden item" pointer, as it was invalidated by the item array expansion
        // since the hidden item is at index -1.
        item = (void*)((uintptr_t)(table + 1));
    }

    METADOT_ASSERT_E(table->count < table->item_capacity);
    METADOT_ASSERT_E(table->slots[slot].item_index < 0 && (hash % slot_capacity) == (uint32_t)base_slot);
    table->slots[slot].key_hash = hash;
    table->slots[slot].item_index = table->count;
    ++table->slots[base_slot].base_count;

    void* item_dst = s_get_item(table, table->count);
    void* key_dst = s_get_key(table, table->count);
    if (item) METAENGINE_MEMCPY(item_dst, item, table->item_size);
    METAENGINE_MEMCPY(key_dst, key, table->key_size);
    table->items_slot_index[table->count] = slot;
    table->return_index = table->count++;

    return s_get_item(table, 0);
}

void* metadot_hashtable_insert_impl3(METAENGINE_Hhdr* table, const void* key) { return metadot_hashtable_insert_impl2(table, key, table->hidden_item); }

void* metadot_hashtable_insert_impl(METAENGINE_Hhdr* table, uint64_t key) { return metadot_hashtable_insert_impl2(table, &key, table->hidden_item); }

void metadot_hashtable_remove_impl2(METAENGINE_Hhdr* table, const void* key) {
    uint32_t hash = (uint32_t)fnv1a(key, table->key_size);
    int slot = s_find_slot(table, hash, key);
    METADOT_ASSERT_E(slot >= 0);

    int base_slot = (int)(hash % (uint32_t)table->slot_capacity);
    int index = table->slots[slot].item_index;
    int last_index = table->count - 1;
    --table->slots[base_slot].base_count;
    table->slots[slot].item_index = -1;

    if (index != last_index) {
        void* dst_key = s_get_key(table, index);
        void* src_key = s_get_key(table, last_index);
        METAENGINE_MEMCPY(dst_key, src_key, (size_t)table->key_size);
        void* dst_item = s_get_item(table, index);
        void* src_item = s_get_item(table, last_index);
        METAENGINE_MEMCPY(dst_item, src_item, (size_t)table->item_size);
        table->items_slot_index[index] = table->items_slot_index[last_index];
        table->slots[table->items_slot_index[last_index]].item_index = index;
    }
    --table->count;
}

void metadot_hashtable_remove_impl(METAENGINE_Hhdr* table, uint64_t key) { metadot_hashtable_remove_impl2(table, &key); }

void metadot_hashtable_clear_impl(METAENGINE_Hhdr* table) {
    table->count = 0;
    for (int i = 0; i < table->slot_capacity; ++i) {
        table->slots[i].base_count = 0;
        table->slots[i].item_index = -1;
    }
}

int metadot_hashtable_find_impl2(const METAENGINE_Hhdr* table, const void* key) {
    int slot = s_find_slot(table, (uint32_t)fnv1a(key, table->key_size), key);
    if (slot < 0) {
        // We will be "returning" a zero'd out item through `hget` with this
        // hidden item.
        METAENGINE_MEMSET(table->hidden_item, 0, table->item_size);
        return -1;
    }
    int index = table->slots[slot].item_index;
    return index;
}

int metadot_hashtable_find_impl(const METAENGINE_Hhdr* table, uint64_t key) { return metadot_hashtable_find_impl2(table, &key); }

bool metadot_hashtable_has_impl(METAENGINE_Hhdr* table, uint64_t key) { return !!metadot_hashtable_find_impl(table, key); }

int metadot_hashtable_count_impl(const METAENGINE_Hhdr* table) { return table->count; }

void* metadot_hashtable_items_impl(const METAENGINE_Hhdr* table) { return table->items_data; }

void* metadot_hashtable_keys_impl(const METAENGINE_Hhdr* table) { return table->items_key; }

void metadot_hashtable_swap_impl(METAENGINE_Hhdr* table, int index_a, int index_b) {
    if (index_a < 0 || index_a >= table->count || index_b < 0 || index_b >= table->count) return;

    int slot_a = table->items_slot_index[index_a];
    int slot_b = table->items_slot_index[index_b];

    table->items_slot_index[index_a] = slot_b;
    table->items_slot_index[index_b] = slot_a;

    void* key_a = s_get_key(table, index_a);
    void* key_b = s_get_key(table, index_b);
    METAENGINE_MEMCPY(table->temp_key, key_a, table->key_size);
    METAENGINE_MEMCPY(key_a, key_b, table->key_size);
    METAENGINE_MEMCPY(key_b, table->temp_key, table->key_size);

    void* item_a = s_get_item(table, index_a);
    void* item_b = s_get_item(table, index_b);
    METAENGINE_MEMCPY(table->temp_item, item_a, table->item_size);
    METAENGINE_MEMCPY(item_a, item_b, table->item_size);
    METAENGINE_MEMCPY(item_b, table->temp_item, table->item_size);

    table->slots[slot_a].item_index = index_b;
    table->slots[slot_b].item_index = index_a;
}

#pragma endregion hashtable

#pragma region handle_table

union METAENGINE_HandleEntry {
    struct {
        uint64_t user_index : 32;
        uint64_t user_type : 15;
        uint64_t active : 1;
        uint64_t generation : 16;
    } data;
    uint64_t val = 0;
};

struct METAENGINE_HandleTable {
    METAENGINE_HandleTable() : m_handles() {}

    uint32_t m_freelist = ~0;
    MetaEngine::Array<METAENGINE_HandleEntry> m_handles;
};

static void s_add_elements_to_freelist(METAENGINE_HandleTable* table, int first_index, int last_index) {
    METAENGINE_HandleEntry* m_handles = table->m_handles.data();
    for (int i = first_index; i < last_index; ++i) {
        METAENGINE_HandleEntry handle;
        handle.data.user_index = i + 1;
        handle.data.generation = 0;
        m_handles[i] = handle;
    }

    METAENGINE_HandleEntry last_handle;
    last_handle.data.user_index = UINT32_MAX;
    last_handle.data.generation = 0;
    m_handles[last_index] = last_handle;

    table->m_freelist = first_index;
}

METAENGINE_HandleTable* metadot_make_handle_allocator(int initial_capacity) {
    METAENGINE_HandleTable* table = (METAENGINE_HandleTable*)METAENGINE_FW_ALLOC(sizeof(METAENGINE_HandleTable));
    METAENGINE_FW_PLACEMENT_NEW(table) METAENGINE_HandleTable();

    if (initial_capacity) {
        table->m_handles.ensure_capacity(initial_capacity);
        int last_index = table->m_handles.capacity() - 1;
        s_add_elements_to_freelist(table, 0, last_index);
    }

    return table;
}

void metadot_destroy_handle_allocator(METAENGINE_HandleTable* table) {
    if (!table) return;
    table->~METAENGINE_HandleTable();
    METAENGINE_FW_FREE(table);
}

METAENGINE_Handle metadot_handle_allocator_alloc(METAENGINE_HandleTable* table, uint32_t index, uint16_t type) {
    int freelist_index = table->m_freelist;
    if (freelist_index == UINT32_MAX) {
        int first_index = table->m_handles.capacity();
        if (!first_index) first_index = 1;
        table->m_handles.ensure_capacity(first_index * 2);
        int last_index = table->m_handles.capacity() - 1;
        s_add_elements_to_freelist(table, first_index, last_index);
        freelist_index = table->m_freelist;
    }

    // Pop m_freelist.
    METAENGINE_HandleEntry* m_handles = table->m_handles.data();
    table->m_freelist = m_handles[freelist_index].data.user_index;

    // Setup handle indices.
    m_handles[freelist_index].data.user_index = index;
    m_handles[freelist_index].data.user_type = type;
    m_handles[freelist_index].data.active = true;
    METAENGINE_Handle handle = (((uint64_t)freelist_index) << 32) | (((uint64_t)type) << 16) | m_handles[freelist_index].data.generation;
    return handle;
}

static METADOT_INLINE uint32_t s_table_index(METAENGINE_Handle handle) { return (uint32_t)((handle & 0xFFFFFFFF00000000ULL) >> 32); }

uint32_t metadot_handle_allocator_get_index(METAENGINE_HandleTable* table, METAENGINE_Handle handle) {
    METAENGINE_HandleEntry* m_handles = table->m_handles.data();
    uint32_t table_index = s_table_index(handle);
    uint64_t generation = handle & 0xFFFF;
    METADOT_ASSERT_E(m_handles[table_index].data.generation == generation);
    return m_handles[table_index].data.user_index;
}

uint16_t metadot_handle_allocator_get_type(METAENGINE_HandleTable* table, METAENGINE_Handle handle) {
    METAENGINE_HandleEntry* m_handles = table->m_handles.data();
    uint32_t table_index = s_table_index(handle);
    uint64_t generation = handle & 0xFFFF;
    METADOT_ASSERT_E(m_handles[table_index].data.generation == generation);
    return m_handles[table_index].data.user_type;
}

bool metadot_handle_allocator_is_active(METAENGINE_HandleTable* table, METAENGINE_Handle handle) {
    METAENGINE_HandleEntry* m_handles = table->m_handles.data();
    uint32_t table_index = s_table_index(handle);
    uint64_t generation = handle & 0xFFFF;
    METADOT_ASSERT_E(m_handles[table_index].data.generation == generation);
    return m_handles[table_index].data.active;
}

void metadot_handle_allocator_activate(METAENGINE_HandleTable* table, METAENGINE_Handle handle) {
    METAENGINE_HandleEntry* m_handles = table->m_handles.data();
    uint32_t table_index = s_table_index(handle);
    uint64_t generation = handle & 0xFFFF;
    METADOT_ASSERT_E(m_handles[table_index].data.generation == generation);
    m_handles[table_index].data.active = true;
}

void metadot_handle_allocator_deactivate(METAENGINE_HandleTable* table, METAENGINE_Handle handle) {
    METAENGINE_HandleEntry* m_handles = table->m_handles.data();
    uint32_t table_index = s_table_index(handle);
    uint64_t generation = handle & 0xFFFF;
    METADOT_ASSERT_E(m_handles[table_index].data.generation == generation);
    m_handles[table_index].data.active = false;
}

void metadot_handle_allocator_update_index(METAENGINE_HandleTable* table, METAENGINE_Handle handle, uint32_t index) {
    METAENGINE_HandleEntry* m_handles = table->m_handles.data();
    uint32_t table_index = s_table_index(handle);
    uint64_t generation = handle & 0xFFFF;
    METADOT_ASSERT_E(m_handles[table_index].data.generation == generation);
    m_handles[table_index].data.user_index = index;
}

void metadot_handle_allocator_free(METAENGINE_HandleTable* table, METAENGINE_Handle handle) {
    // Push handle onto m_freelist.
    METAENGINE_HandleEntry* m_handles = table->m_handles.data();
    uint32_t table_index = s_table_index(handle);
    m_handles[table_index].data.user_index = table->m_freelist;
    m_handles[table_index].data.generation++;
    table->m_freelist = table_index;
}

int metadot_handle_allocator_is_handle_valid(METAENGINE_HandleTable* table, METAENGINE_Handle handle) {
    METAENGINE_HandleEntry* m_handles = table->m_handles.data();
    uint32_t table_index = s_table_index(handle);
    uint64_t generation = handle & 0xFFFF;
    uint64_t type = (handle & 0x00000000FFFF0000ULL) >> 16;
    bool match_generation = m_handles[table_index].data.generation == generation;
    bool match_type = m_handles[table_index].data.user_type == type;
    return match_generation && match_type;
}

#pragma endregion handle_table

#pragma region concurrency

METAENGINE_Mutex metadot_make_mutex() { return cute_mutex_create(); }

void metadot_destroy_mutex(METAENGINE_Mutex* mutex) { cute_mutex_destroy(mutex); }

METAENGINE_Result metadot_mutex_lock(METAENGINE_Mutex* mutex) { return metadot_result_make(cute_lock(mutex), NULL); }

METAENGINE_Result metadot_mutex_unlock(METAENGINE_Mutex* mutex) { return metadot_result_make(cute_unlock(mutex), NULL); }

bool METAENGINE_Mutexrylock(METAENGINE_Mutex* mutex) { return !!cute_trylock(mutex); }

METAENGINE_ConditionVariable metadot_make_cv() { return cute_cv_create(); }

void metadot_destroy_cv(METAENGINE_ConditionVariable* cv) { cute_cv_destroy(cv); }

METAENGINE_Result metadot_cv_wake_all(METAENGINE_ConditionVariable* cv) { return metadot_result_make(cute_cv_wake_all(cv), NULL); }

METAENGINE_Result metadot_cv_wake_one(METAENGINE_ConditionVariable* cv) { return metadot_result_make(cute_cv_wake_one(cv), NULL); }

METAENGINE_Result metadot_cv_wait(METAENGINE_ConditionVariable* cv, METAENGINE_Mutex* mutex) { return metadot_result_make(cute_cv_wait(cv, mutex), NULL); }

METAENGINE_Semaphore metadot_make_sem(int initial_count) { return cute_semaphore_create(initial_count); }

void metadot_destroy_sem(METAENGINE_Semaphore* semaphore) { cute_semaphore_destroy(semaphore); }

METAENGINE_Result metadot_sem_post(METAENGINE_Semaphore* semaphore) { return metadot_result_make(cute_semaphore_post(semaphore), NULL); }

METAENGINE_Result metadot_sem_try(METAENGINE_Semaphore* semaphore) { return metadot_result_make(cute_semaphore_try(semaphore), NULL); }

METAENGINE_Result metadot_sem_wait(METAENGINE_Semaphore* semaphore) { return metadot_result_make(cute_semaphore_wait(semaphore), NULL); }

METAENGINE_Result metadot_sem_value(METAENGINE_Semaphore* semaphore) { return metadot_result_make(cute_semaphore_value(semaphore), NULL); }

METAENGINE_Thread* metadot_thread_create(METAENGINE_ThreadFn func, const char* name, void* udata) { return cute_thread_create(func, name, udata); }

void metadot_thread_detach(METAENGINE_Thread* thread) { cute_thread_detach(thread); }

METAENGINE_ThreadId metadot_thread_get_id(METAENGINE_Thread* thread) { return cute_thread_get_id(thread); }

METAENGINE_ThreadId metadot_thread_id() { return cute_thread_id(); }

METAENGINE_Result metadot_thread_wait(METAENGINE_Thread* thread) { return metadot_result_make(cute_thread_wait(thread), NULL); }

int metadot_core_count() { return cute_core_count(); }

int metadot_cacheline_size() { return cute_cacheline_size(); }

METAENGINE_AtomicInt metadot_atomic_zero() {
    METAENGINE_AtomicInt result;
    result.i = 0;
    return result;
}

int metadot_atomic_add(METAENGINE_AtomicInt* atomic, int addend) { return cute_atomic_add(atomic, addend); }

int metadot_atomic_set(METAENGINE_AtomicInt* atomic, int value) { return cute_atomic_set(atomic, value); }

int metadot_atomic_get(METAENGINE_AtomicInt* atomic) { return cute_atomic_get(atomic); }

METAENGINE_Result metadot_atomic_cas(METAENGINE_AtomicInt* atomic, int expected, int value) { return metadot_result_make(cute_atomic_cas(atomic, expected, value), NULL); }

void* metadot_atomic_ptr_set(void** atomic, void* value) { return cute_atomic_ptr_set(atomic, value); }

void* metadot_atomic_ptr_get(void** atomic) { return cute_atomic_ptr_get(atomic); }

METAENGINE_Result metadot_atomic_ptr_cas(void** atomic, void* expected, void* value) { return metadot_result_make(cute_atomic_ptr_cas(atomic, expected, value), NULL); }

METAENGINE_ReadWriteLock metadot_make_rw_lock() { return cute_rw_lock_create(); }

void metadot_destroy_rw_lock(METAENGINE_ReadWriteLock* rw) {
    if (rw) cute_rw_lock_destroy(rw);
}

void metadot_read_lock(METAENGINE_ReadWriteLock* rw) { cute_read_lock(rw); }

void metadot_read_unlock(METAENGINE_ReadWriteLock* rw) { cute_read_unlock(rw); }

void metadot_write_lock(METAENGINE_ReadWriteLock* rw) { cute_write_lock(rw); }

void metadot_write_unlock(METAENGINE_ReadWriteLock* rw) { cute_write_unlock(rw); }

METAENGINE_Threadpool* metadot_make_threadpool(int thread_count) { return cute_threadpool_create(thread_count, NULL); }

void metadot_threadpool_add_task(METAENGINE_Threadpool* pool, METAENGINE_TaskFn* task, void* param) { cute_threadpool_add_task(pool, task, param); }

void metadot_threadpool_kick_and_wait(METAENGINE_Threadpool* pool) { cute_threadpool_kick_and_wait(pool); }

void metadot_threadpool_kick(METAENGINE_Threadpool* pool) { cute_threadpool_kick(pool); }

void metadot_destroy_threadpool(METAENGINE_Threadpool* pool) { cute_threadpool_destroy(pool); }

#pragma endregion concurrency