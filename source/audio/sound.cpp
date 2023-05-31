
#include "sound.h"

#include "core/macros.h"

#undef STB_VORBIS_HEADER_ONLY
#include "libs/external/stb_vorbis.c"

#ifndef METAENGINE_SOUND_MINIMUM_BUFFERED_SAMPLES
#define METAENGINE_SOUND_MINIMUM_BUFFERED_SAMPLES 1024
#endif

#if !defined(METAENGINE_SOUND_ASSERT)
#include <assert.h>
#define METAENGINE_SOUND_ASSERT assert
#endif

#if !defined(METAENGINE_SOUND_ALLOC)
#include <stdlib.h>
#define METAENGINE_SOUND_ALLOC(size, ctx) malloc(size)
#endif

#if !defined(METAENGINE_SOUND_FREE)
#include <stdlib.h>
#define METAENGINE_SOUND_FREE(mem, ctx) free(mem)
#endif

#ifndef METAENGINE_SOUND_MEMCPY
#include <string.h>
#define METAENGINE_SOUND_MEMCPY memcpy
#endif

#ifndef METAENGINE_SOUND_MEMSET
#include <string.h>
#define METAENGINE_SOUND_MEMSET memset
#endif

#ifndef METAENGINE_SOUND_MEMCMP
#include <string.h>
#define METAENGINE_SOUND_MEMCMP memcmp
#endif

#ifndef METAENGINE_SOUND_SEEK_SET
#include <stdio.h>
#define METAENGINE_SOUND_SEEK_SET SEEK_SET
#endif

#ifndef METAENGINE_SOUND_SEEK_END
#include <stdio.h>
#define METAENGINE_SOUND_SEEK_END SEEK_END
#endif

#ifndef METAENGINE_SOUND_FILE
#include <stdio.h>
#define METAENGINE_SOUND_FILE FILE
#endif

#ifndef METAENGINE_SOUND_FOPEN
#include <stdio.h>
#define METAENGINE_SOUND_FOPEN fopen
#endif

#ifndef METAENGINE_SOUND_FSEEK
#include <stdio.h>
#define METAENGINE_SOUND_FSEEK fseek
#endif

#ifndef METAENGINE_SOUND_FREAD
#include <stdio.h>
#define METAENGINE_SOUND_FREAD fread
#endif

#ifndef METAENGINE_SOUND_FTELL
#include <stdio.h>
#define METAENGINE_SOUND_FTELL ftell
#endif

#ifndef METAENGINE_SOUND_FCLOSE
#include <stdio.h>
#define METAENGINE_SOUND_FCLOSE fclose
#endif

// Platform detection.
#define METAENGINE_SOUND_WINDOWS 1
#define METAENGINE_SOUND_APPLE 2
#define METAENGINE_SOUND_SDL 3

#if defined(_WIN32)

#if !defined _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#if !defined _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#define METAENGINE_SOUND_PLATFORM METAENGINE_SOUND_WINDOWS

#elif defined(__APPLE__)

#define METAENGINE_SOUND_PLATFORM METAENGINE_SOUND_APPLE

#else

// Just use SDL on other esoteric platforms.
#define METAENGINE_SOUND_PLATFORM METAENGINE_SOUND_SDL

#endif

// Use METAENGINE_SOUND_FORCE_SDL to override the above macros and use the SDL port.
#ifdef METAENGINE_SOUND_FORCE_SDL

#undef METAENGINE_SOUND_PLATFORM
#define METAENGINE_SOUND_PLATFORM METAENGINE_SOUND_SDL

#endif

// Platform specific file inclusions.
#if METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_WINDOWS

#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#ifndef _WAVEFORMATEX_
#include <mmreg.h>
#include <mmsystem.h>
#endif

#include <dsound.h>
#undef PlaySound

#ifdef _MSC_VER
#pragma comment(lib, "dsound.lib")
#endif

#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_APPLE

#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudio.h>
#include <mach/mach_time.h>
#include <pthread.h>

#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_SDL

#ifndef SDL_h_
#include <SDL.h>
#endif
#ifndef _WIN32
#include <alloca.h>
#endif

#else

#error Unsupported platform - please choose one of METAENGINE_SOUND_WINDOWS, METAENGINE_SOUND_APPLE, METAENGINE_SOUND_SDL.

#endif

#ifdef METAENGINE_SOUND_SCALAR_MODE

#include <limits.h>

#define METAENGINE_SOUND_SATURATE16(X) (int16_t)((X) > SHRT_MAX ? SHRT_MAX : ((X) < SHRT_MIN ? SHRT_MIN : (X)))

typedef struct cs__m128 {
    float a, b, c, d;
} cs__m128;

typedef struct cs__m128i {
    int32_t a, b, c, d;
} cs__m128i;

cs__m128 cs_mm_set_ps(float e3, float e2, float e1, float e0) {
    cs__m128 a;
    a.a = e0;
    a.b = e1;
    a.c = e2;
    a.d = e3;
    return a;
}

cs__m128 cs_mm_set1_ps(float e) {
    cs__m128 a;
    a.a = e;
    a.b = e;
    a.c = e;
    a.d = e;
    return a;
}

cs__m128 cs_mm_load_ps(float const* mem_addr) {
    cs__m128 a;
    a.a = mem_addr[0];
    a.b = mem_addr[1];
    a.c = mem_addr[2];
    a.d = mem_addr[3];
    return a;
}

cs__m128 cs_mm_add_ps(cs__m128 a, cs__m128 b) {
    cs__m128 c;
    c.a = a.a + b.a;
    c.b = a.b + b.b;
    c.c = a.c + b.c;
    c.d = a.d + b.d;
    return c;
}

cs__m128 cs_mm_mul_ps(cs__m128 a, cs__m128 b) {
    cs__m128 c;
    c.a = a.a * b.a;
    c.b = a.b * b.b;
    c.c = a.c * b.c;
    c.d = a.d * b.d;
    return c;
}

cs__m128i cs_mm_cvtps_epi32(cs__m128 a) {
    cs__m128i b;
    b.a = a.a;
    b.b = a.b;
    b.c = a.c;
    b.d = a.d;
    return b;
}

cs__m128i cs_mm_unpacklo_epi32(cs__m128i a, cs__m128i b) {
    cs__m128i c;
    c.a = a.a;
    c.b = b.a;
    c.c = a.b;
    c.d = b.b;
    return c;
}

cs__m128i cs_mm_unpackhi_epi32(cs__m128i a, cs__m128i b) {
    cs__m128i c;
    c.a = a.c;
    c.b = b.c;
    c.c = a.d;
    c.d = b.d;
    return c;
}

cs__m128i cs_mm_packs_epi32(cs__m128i a, cs__m128i b) {
    union {
        int16_t c[8];
        cs__m128i m;
    } dst;
    dst.c[0] = METAENGINE_SOUND_SATURATE16(a.a);
    dst.c[1] = METAENGINE_SOUND_SATURATE16(a.b);
    dst.c[2] = METAENGINE_SOUND_SATURATE16(a.c);
    dst.c[3] = METAENGINE_SOUND_SATURATE16(a.d);
    dst.c[4] = METAENGINE_SOUND_SATURATE16(b.a);
    dst.c[5] = METAENGINE_SOUND_SATURATE16(b.b);
    dst.c[6] = METAENGINE_SOUND_SATURATE16(b.c);
    dst.c[7] = METAENGINE_SOUND_SATURATE16(b.d);
    return dst.m;
}

#else  // METAENGINE_SOUND_SCALAR_MODE

#ifdef __METADOT_ARCH_ARM
#include "libs/sse2neon.h"
#else
#include <emmintrin.h>
#include <xmmintrin.h>
#endif

#define cs__m128 __m128
#define cs__m128i __m128i

#define cs_mm_set_ps _mm_set_ps
#define cs_mm_set1_ps _mm_set1_ps
#define cs_mm_load_ps _mm_load_ps
#define cs_mm_add_ps _mm_add_ps
#define cs_mm_mul_ps _mm_mul_ps
#define cs_mm_cvtps_epi32 _mm_cvtps_epi32
#define cs_mm_unpacklo_epi32 _mm_unpacklo_epi32
#define cs_mm_unpackhi_epi32 _mm_unpackhi_epi32
#define cs_mm_packs_epi32 _mm_packs_epi32

#endif  // METAENGINE_SOUND_SCALAR_MODE

#define METAENGINE_SOUND_ALIGN(X, Y) ((((size_t)X) + ((Y)-1)) & ~((Y)-1))
#define METAENGINE_SOUND_TRUNC(X, Y) ((size_t)(X) & ~((Y)-1))

// -------------------------------------------------------------------------------------------------
// hashtable.h implementation by Mattias Gustavsson
// See: http://www.mattiasgustavsson.com/ and https://github.com/mattiasgustavsson/libs/blob/master/hashtable.h
// begin hashtable.h

#ifndef HASHTABLE_MEMSET
#define HASHTABLE_MEMSET(ptr, val, n) METAENGINE_SOUND_MEMSET(ptr, val, n)
#endif

#ifndef HASHTABLE_MEMCPY
#define HASHTABLE_MEMCPY(dst, src, n) METAENGINE_SOUND_MEMCPY(dst, src, n)
#endif

#ifndef HASHTABLE_MALLOC
#define HASHTABLE_MALLOC(ctx, size) METAENGINE_SOUND_ALLOC(size, ctx)
#endif

#ifndef HASHTABLE_FREE
#define HASHTABLE_FREE(ctx, ptr) METAENGINE_SOUND_FREE(ptr, ctx)
#endif

#define HASHTABLE_IMPLEMENTATION

#ifdef HASHTABLE_IMPLEMENTATION
#ifndef HASHTABLE_IMPLEMENTATION_ONCE
#define HASHTABLE_IMPLEMENTATION_ONCE

// hashtable.h implementation by Mattias Gustavsson
// See: http://www.mattiasgustavsson.com/ and https://github.com/mattiasgustavsson/libs/blob/master/hashtable.h
// begin hashtable.h (continuing from first time)

#ifndef HASHTABLE_SIZE_T
#include <stddef.h>
#define HASHTABLE_SIZE_T size_t
#endif

#ifndef HASHTABLE_ASSERT
#include <assert.h>
#define HASHTABLE_ASSERT(x) assert(x)
#endif

#ifndef HASHTABLE_MEMSET
#include <string.h>
#define HASHTABLE_MEMSET(ptr, val, cnt) (memset(ptr, val, cnt))
#endif

#ifndef HASHTABLE_MEMCPY
#include <string.h>
#define HASHTABLE_MEMCPY(dst, src, cnt) (memcpy(dst, src, cnt))
#endif

#ifndef HASHTABLE_MALLOC
#include <stdlib.h>
#define HASHTABLE_MALLOC(ctx, size) (malloc(size))
#define HASHTABLE_FREE(ctx, ptr) (free(ptr))
#endif

static HASHTABLE_U32 hashtable_internal_pow2ceil(HASHTABLE_U32 v) {
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    ++v;
    v += (v == 0);
    return v;
}

void hashtable_init(hashtable_t* table, int item_size, int initial_capacity, void* memctx) {
    initial_capacity = (int)hashtable_internal_pow2ceil(initial_capacity >= 0 ? (HASHTABLE_U32)initial_capacity : 32U);
    table->memctx = memctx;
    table->count = 0;
    table->item_size = item_size;
    table->slot_capacity = (int)hashtable_internal_pow2ceil((HASHTABLE_U32)(initial_capacity + initial_capacity / 2));
    int slots_size = (int)(table->slot_capacity * sizeof(*table->slots));
    table->slots = (struct hashtable_internal_slot_t*)HASHTABLE_MALLOC(table->memctx, (HASHTABLE_SIZE_T)slots_size);
    HASHTABLE_ASSERT(table->slots);
    HASHTABLE_MEMSET(table->slots, 0, (HASHTABLE_SIZE_T)slots_size);
    table->item_capacity = (int)hashtable_internal_pow2ceil((HASHTABLE_U32)initial_capacity);
    table->items_key = (HASHTABLE_U64*)HASHTABLE_MALLOC(table->memctx, table->item_capacity * (sizeof(*table->items_key) + sizeof(*table->items_slot) + table->item_size) + table->item_size);
    HASHTABLE_ASSERT(table->items_key);
    table->items_slot = (int*)(table->items_key + table->item_capacity);
    table->items_data = (void*)(table->items_slot + table->item_capacity);
    table->swap_temp = (void*)(((uintptr_t)table->items_data) + table->item_size * table->item_capacity);
}

void hashtable_term(hashtable_t* table) {
    HASHTABLE_FREE(table->memctx, table->items_key);
    HASHTABLE_FREE(table->memctx, table->slots);
}

// from https://gist.github.com/badboy/6267743
static HASHTABLE_U32 hashtable_internal_calculate_hash(HASHTABLE_U64 key) {
    key = (~key) + (key << 18);
    key = key ^ (key >> 31);
    key = key * 21;
    key = key ^ (key >> 11);
    key = key + (key << 6);
    key = key ^ (key >> 22);
    HASHTABLE_ASSERT(key);
    return (HASHTABLE_U32)key;
}

static int hashtable_internal_find_slot(hashtable_t const* table, HASHTABLE_U64 key) {
    int const slot_mask = table->slot_capacity - 1;
    HASHTABLE_U32 const hash = hashtable_internal_calculate_hash(key);

    int const base_slot = (int)(hash & (HASHTABLE_U32)slot_mask);
    int base_count = table->slots[base_slot].base_count;
    int slot = base_slot;

    while (base_count > 0) {
        HASHTABLE_U32 slot_hash = table->slots[slot].key_hash;
        if (slot_hash) {
            int slot_base = (int)(slot_hash & (HASHTABLE_U32)slot_mask);
            if (slot_base == base_slot) {
                HASHTABLE_ASSERT(base_count > 0);
                --base_count;
                if (slot_hash == hash && table->items_key[table->slots[slot].item_index] == key) return slot;
            }
        }
        slot = (slot + 1) & slot_mask;
    }

    return -1;
}

static void hashtable_internal_expand_slots(hashtable_t* table) {
    int const old_capacity = table->slot_capacity;
    struct hashtable_internal_slot_t* old_slots = table->slots;

    table->slot_capacity *= 2;
    int const slot_mask = table->slot_capacity - 1;

    int const size = (int)(table->slot_capacity * sizeof(*table->slots));
    table->slots = (struct hashtable_internal_slot_t*)HASHTABLE_MALLOC(table->memctx, (HASHTABLE_SIZE_T)size);
    HASHTABLE_ASSERT(table->slots);
    HASHTABLE_MEMSET(table->slots, 0, (HASHTABLE_SIZE_T)size);

    for (int i = 0; i < old_capacity; ++i) {
        HASHTABLE_U32 const hash = old_slots[i].key_hash;
        if (hash) {
            int const base_slot = (int)(hash & (HASHTABLE_U32)slot_mask);
            int slot = base_slot;
            while (table->slots[slot].key_hash) slot = (slot + 1) & slot_mask;
            table->slots[slot].key_hash = hash;
            int item_index = old_slots[i].item_index;
            table->slots[slot].item_index = item_index;
            table->items_slot[item_index] = slot;
            ++table->slots[base_slot].base_count;
        }
    }

    HASHTABLE_FREE(table->memctx, old_slots);
}

static void hashtable_internal_expand_items(hashtable_t* table) {
    table->item_capacity *= 2;
    HASHTABLE_U64* const new_items_key =
            (HASHTABLE_U64*)HASHTABLE_MALLOC(table->memctx, table->item_capacity * (sizeof(*table->items_key) + sizeof(*table->items_slot) + table->item_size) + table->item_size);
    HASHTABLE_ASSERT(new_items_key);

    int* const new_items_slot = (int*)(new_items_key + table->item_capacity);
    void* const new_items_data = (void*)(new_items_slot + table->item_capacity);
    void* const new_swap_temp = (void*)(((uintptr_t)new_items_data) + table->item_size * table->item_capacity);

    HASHTABLE_MEMCPY(new_items_key, table->items_key, table->count * sizeof(*table->items_key));
    HASHTABLE_MEMCPY(new_items_slot, table->items_slot, table->count * sizeof(*table->items_key));
    HASHTABLE_MEMCPY(new_items_data, table->items_data, (HASHTABLE_SIZE_T)table->count * table->item_size);

    HASHTABLE_FREE(table->memctx, table->items_key);

    table->items_key = new_items_key;
    table->items_slot = new_items_slot;
    table->items_data = new_items_data;
    table->swap_temp = new_swap_temp;
}

void* hashtable_insert(hashtable_t* table, HASHTABLE_U64 key, void const* item) {
    HASHTABLE_ASSERT(hashtable_internal_find_slot(table, key) < 0);

    if (table->count >= (table->slot_capacity - table->slot_capacity / 3)) hashtable_internal_expand_slots(table);

    int const slot_mask = table->slot_capacity - 1;
    HASHTABLE_U32 const hash = hashtable_internal_calculate_hash(key);

    int const base_slot = (int)(hash & (HASHTABLE_U32)slot_mask);
    int base_count = table->slots[base_slot].base_count;
    int slot = base_slot;
    int first_free = slot;
    while (base_count) {
        HASHTABLE_U32 const slot_hash = table->slots[slot].key_hash;
        if (slot_hash == 0 && table->slots[first_free].key_hash != 0) first_free = slot;
        int slot_base = (int)(slot_hash & (HASHTABLE_U32)slot_mask);
        if (slot_base == base_slot) --base_count;
        slot = (slot + 1) & slot_mask;
    }

    slot = first_free;
    while (table->slots[slot].key_hash) slot = (slot + 1) & slot_mask;

    if (table->count >= table->item_capacity) hashtable_internal_expand_items(table);

    HASHTABLE_ASSERT(!table->slots[slot].key_hash && (hash & (HASHTABLE_U32)slot_mask) == (HASHTABLE_U32)base_slot);
    HASHTABLE_ASSERT(hash);
    table->slots[slot].key_hash = hash;
    table->slots[slot].item_index = table->count;
    ++table->slots[base_slot].base_count;

    void* dest_item = (void*)(((uintptr_t)table->items_data) + table->count * table->item_size);
    memcpy(dest_item, item, (HASHTABLE_SIZE_T)table->item_size);
    table->items_key[table->count] = key;
    table->items_slot[table->count] = slot;
    ++table->count;
    return dest_item;
}

void hashtable_remove(hashtable_t* table, HASHTABLE_U64 key) {
    int const slot = hashtable_internal_find_slot(table, key);
    HASHTABLE_ASSERT(slot >= 0);

    int const slot_mask = table->slot_capacity - 1;
    HASHTABLE_U32 const hash = table->slots[slot].key_hash;
    int const base_slot = (int)(hash & (HASHTABLE_U32)slot_mask);
    HASHTABLE_ASSERT(hash);
    --table->slots[base_slot].base_count;
    table->slots[slot].key_hash = 0;

    int index = table->slots[slot].item_index;
    int last_index = table->count - 1;
    if (index != last_index) {
        table->items_key[index] = table->items_key[last_index];
        table->items_slot[index] = table->items_slot[last_index];
        void* dst_item = (void*)(((uintptr_t)table->items_data) + index * table->item_size);
        void* src_item = (void*)(((uintptr_t)table->items_data) + last_index * table->item_size);
        HASHTABLE_MEMCPY(dst_item, src_item, (HASHTABLE_SIZE_T)table->item_size);
        table->slots[table->items_slot[last_index]].item_index = index;
    }
    --table->count;
}

void hashtable_clear(hashtable_t* table) {
    table->count = 0;
    HASHTABLE_MEMSET(table->slots, 0, table->slot_capacity * sizeof(*table->slots));
}

void* hashtable_find(hashtable_t const* table, HASHTABLE_U64 key) {
    int const slot = hashtable_internal_find_slot(table, key);
    if (slot < 0) return 0;

    int const index = table->slots[slot].item_index;
    void* const item = (void*)(((uintptr_t)table->items_data) + index * table->item_size);
    return item;
}

int hashtable_count(hashtable_t const* table) { return table->count; }

void* hashtable_items(hashtable_t const* table) { return table->items_data; }

HASHTABLE_U64 const* hashtable_keys(hashtable_t const* table) { return table->items_key; }

void hashtable_swap(hashtable_t* table, int index_a, int index_b) {
    if (index_a < 0 || index_a >= table->count || index_b < 0 || index_b >= table->count) return;

    int slot_a = table->items_slot[index_a];
    int slot_b = table->items_slot[index_b];

    table->items_slot[index_a] = slot_b;
    table->items_slot[index_b] = slot_a;

    HASHTABLE_U64 temp_key = table->items_key[index_a];
    table->items_key[index_a] = table->items_key[index_b];
    table->items_key[index_b] = temp_key;

    void* item_a = (void*)(((uintptr_t)table->items_data) + index_a * table->item_size);
    void* item_b = (void*)(((uintptr_t)table->items_data) + index_b * table->item_size);
    HASHTABLE_MEMCPY(table->swap_temp, item_a, table->item_size);
    HASHTABLE_MEMCPY(item_a, item_b, table->item_size);
    HASHTABLE_MEMCPY(item_b, table->swap_temp, table->item_size);

    table->slots[slot_a].item_index = index_b;
    table->slots[slot_b].item_index = index_a;
}

#endif  /* HASHTABLE_IMPLEMENTATION */
#endif  // HASHTABLE_IMPLEMENTATION_ONCE

/*
contributors:
    Randy Gaul (hashtable_clear, hashtable_swap )
revision history:
    1.1     added hashtable_clear, hashtable_swap
    1.0     first released version
*/

/*
------------------------------------------------------------------------------
This software is available under 2 licenses - you may choose the one you like.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2015 Mattias Gustavsson
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/

// end of hashtable.h

// -------------------------------------------------------------------------------------------------
// Doubly list.

typedef struct cs_list_node_t {
    struct cs_list_node_t* next /* = this */;
    struct cs_list_node_t* prev /* = this */;
} cs_list_node_t;

typedef struct cs_list_t {
    cs_list_node_t nodes;
} cs_list_t;

#define METAENGINE_SOUND_OFFSET_OF(T, member) ((size_t)((uintptr_t)(&(((T*)0)->member))))
#define METAENGINE_SOUND_LIST_NODE(T, member, ptr) ((cs_list_node_t*)((uintptr_t)ptr + METAENGINE_SOUND_OFFSET_OF(T, member)))
#define METAENGINE_SOUND_LIST_HOST(T, member, ptr) ((T*)((uintptr_t)ptr - METAENGINE_SOUND_OFFSET_OF(T, member)))

void cs_list_init_node(cs_list_node_t* node) {
    node->next = node;
    node->prev = node;
}

void cs_list_init(cs_list_t* list) { cs_list_init_node(&list->nodes); }

void cs_list_push_front(cs_list_t* list, cs_list_node_t* node) {
    node->next = list->nodes.next;
    node->prev = &list->nodes;
    list->nodes.next->prev = node;
    list->nodes.next = node;
}

void cs_list_push_back(cs_list_t* list, cs_list_node_t* node) {
    node->prev = list->nodes.prev;
    node->next = &list->nodes;
    list->nodes.prev->next = node;
    list->nodes.prev = node;
}

void cs_list_remove(cs_list_node_t* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    cs_list_init_node(node);
}

cs_list_node_t* cs_list_pop_front(cs_list_t* list) {
    cs_list_node_t* node = list->nodes.next;
    cs_list_remove(node);
    return node;
}

cs_list_node_t* cs_list_pop_back(cs_list_t* list) {
    cs_list_node_t* node = list->nodes.prev;
    cs_list_remove(node);
    return node;
}

int cs_list_empty(cs_list_t* list) { return list->nodes.next == list->nodes.prev && list->nodes.next == &list->nodes; }

cs_list_node_t* cs_list_begin(cs_list_t* list) { return list->nodes.next; }

cs_list_node_t* cs_list_end(cs_list_t* list) { return &list->nodes; }

cs_list_node_t* cs_list_front(cs_list_t* list) { return list->nodes.next; }

cs_list_node_t* cs_list_back(cs_list_t* list) { return list->nodes.prev; }

// -------------------------------------------------------------------------------------------------

const char* cs_error_as_string(cs_error_t error) {
    switch (error) {
        case METAENGINE_SOUND_ERROR_NONE:
            return "METAENGINE_SOUND_ERROR_NONE";
        case METAENGINE_SOUND_ERROR_IMPLEMENTATION_ERROR_PLEASE_REPORT_THIS_ON_GITHUB:
            return "METAENGINE_SOUND_ERROR_IMPLEMENTATION_ERROR_PLEASE_REPORT_THIS_ON_GITHUB";
        case METAENGINE_SOUND_ERROR_FILE_NOT_FOUND:
            return "METAENGINE_SOUND_ERROR_FILE_NOT_FOUND";
        case METAENGINE_SOUND_ERROR_INVALID_SOUND:
            return "METAENGINE_SOUND_ERROR_INVALID_SOUND";
        case METAENGINE_SOUND_ERROR_HWND_IS_NULL:
            return "METAENGINE_SOUND_ERROR_HWND_IS_NULL";
        case METAENGINE_SOUND_ERROR_DIRECTSOUND_CREATE_FAILED:
            return "METAENGINE_SOUND_ERROR_DIRECTSOUND_CREATE_FAILED";
        case METAENGINE_SOUND_ERROR_CREATESOUNDBUFFER_FAILED:
            return "METAENGINE_SOUND_ERROR_CREATESOUNDBUFFER_FAILED";
        case METAENGINE_SOUND_ERROR_SETFORMAT_FAILED:
            return "METAENGINE_SOUND_ERROR_SETFORMAT_FAILED";
        case METAENGINE_SOUND_ERROR_AUDIOCOMPONENTFINDNEXT_FAILED:
            return "METAENGINE_SOUND_ERROR_AUDIOCOMPONENTFINDNEXT_FAILED";
        case METAENGINE_SOUND_ERROR_AUDIOCOMPONENTINSTANCENEW_FAILED:
            return "METAENGINE_SOUND_ERROR_AUDIOCOMPONENTINSTANCENEW_FAILED";
        case METAENGINE_SOUND_ERROR_FAILED_TO_SET_STREAM_FORMAT:
            return "METAENGINE_SOUND_ERROR_FAILED_TO_SET_STREAM_FORMAT";
        case METAENGINE_SOUND_ERROR_FAILED_TO_SET_RENDER_CALLBACK:
            return "METAENGINE_SOUND_ERROR_FAILED_TO_SET_RENDER_CALLBACK";
        case METAENGINE_SOUND_ERROR_AUDIOUNITINITIALIZE_FAILED:
            return "METAENGINE_SOUND_ERROR_AUDIOUNITINITIALIZE_FAILED";
        case METAENGINE_SOUND_ERROR_AUDIOUNITSTART_FAILED:
            return "METAENGINE_SOUND_ERROR_AUDIOUNITSTART_FAILED";
        case METAENGINE_SOUND_ERROR_CANT_OPEN_AUDIO_DEVICE:
            return "METAENGINE_SOUND_ERROR_CANT_OPEN_AUDIO_DEVICE";
        case METAENGINE_SOUND_ERROR_CANT_INIT_SDL_AUDIO:
            return "METAENGINE_SOUND_ERROR_CANT_INIT_SDL_AUDIO";
        case METAENGINE_SOUND_ERROR_THE_FILE_IS_NOT_A_WAV_FILE:
            return "METAENGINE_SOUND_ERROR_THE_FILE_IS_NOT_A_WAV_FILE";
        case METAENGINE_SOUND_ERROR_WAV_FILE_FORMAT_CHUNK_NOT_FOUND:
            return "METAENGINE_SOUND_ERROR_WAV_FILE_FORMAT_CHUNK_NOT_FOUND";
        case METAENGINE_SOUND_ERROR_WAV_DATA_CHUNK_NOT_FOUND:
            return "METAENGINE_SOUND_ERROR_WAV_DATA_CHUNK_NOT_FOUND";
        case METAENGINE_SOUND_ERROR_ONLY_PCM_WAV_FILES_ARE_SUPPORTED:
            return "METAENGINE_SOUND_ERROR_ONLY_PCM_WAV_FILES_ARE_SUPPORTED";
        case METAENGINE_SOUND_ERROR_WAV_ONLY_MONO_OR_STEREO_IS_SUPPORTED:
            return "METAENGINE_SOUND_ERROR_WAV_ONLY_MONO_OR_STEREO_IS_SUPPORTED";
        case METAENGINE_SOUND_ERROR_WAV_ONLY_16_BITS_PER_SAMPLE_SUPPORTED:
            return "METAENGINE_SOUND_ERROR_WAV_ONLY_16_BITS_PER_SAMPLE_SUPPORTED";
        case METAENGINE_SOUND_ERROR_CANNOT_SWITCH_MUSIC_WHILE_PAUSED:
            return "METAENGINE_SOUND_ERROR_CANNOT_SWITCH_MUSIC_WHILE_PAUSED";
        case METAENGINE_SOUND_ERROR_CANNOT_CROSSFADE_WHILE_MUSIC_IS_PAUSED:
            return "METAENGINE_SOUND_ERROR_CANNOT_CROSSFADE_WHILE_MUSIC_IS_PAUSED";
        case METAENGINE_SOUND_ERROR_CANNOT_FADEOUT_WHILE_MUSIC_IS_PAUSED:
            return "METAENGINE_SOUND_ERROR_CANNOT_FADEOUT_WHILE_MUSIC_IS_PAUSED";
        case METAENGINE_SOUND_ERROR_TRIED_TO_SET_SAMPLE_INDEX_BEYOND_THE_AUDIO_SOURCES_SAMPLE_COUNT:
            return "METAENGINE_SOUND_ERROR_TRIED_TO_SET_SAMPLE_INDEX_BEYOND_THE_AUDIO_SOURCES_SAMPLE_COUNT";
        case METAENGINE_SOUND_ERROR_STB_VORBIS_DECODE_FAILED:
            return "METAENGINE_SOUND_ERROR_STB_VORBIS_DECODE_FAILED";
        case METAENGINE_SOUND_ERROR_OGG_UNSUPPORTED_CHANNEL_COUNT:
            return "METAENGINE_SOUND_ERROR_OGG_UNSUPPORTED_CHANNEL_COUN";
        default:
            return "UNKNOWN";
    }
}

// MetaEngine sound context functions.

void cs_mix();

typedef struct cs_audio_source_t {
    int sample_rate;
    int sample_count;
    int channel_count;

    // Number of instances currently referencing this audio. Must be zero
    // in order to safely delete the audio. References are automatically
    // updated whenever playing instances are inserted into the context.
    int playing_count;

    // The actual raw audio samples in memory.
    void* channels[2];
} cs_audio_source_t;

typedef struct cs_sound_inst_t {
    uint64_t id;
    bool is_music;
    bool active;
    bool paused;
    bool looped;
    float volume;
    float pan0;
    float pan1;
    uint64_t sample_index;
    cs_audio_source_t* audio;
    cs_list_node_t node;
} cs_sound_inst_t;

typedef enum cs_music_state_t {
    METAENGINE_SOUND_MUSIC_STATE_NONE,
    METAENGINE_SOUND_MUSIC_STATE_PLAYING,
    METAENGINE_SOUND_MUSIC_STATE_FADE_OUT,
    METAENGINE_SOUND_MUSIC_STATE_FADE_IN,
    METAENGINE_SOUND_MUSIC_STATE_SWITCH_TO_0,
    METAENGINE_SOUND_MUSIC_STATE_SWITCH_TO_1,
    METAENGINE_SOUND_MUSIC_STATE_CROSSFADE,
    METAENGINE_SOUND_MUSIC_STATE_PAUSED
} cs_music_state_t;

#define METAENGINE_SOUND_PAGE_INSTANCE_COUNT 1024

typedef struct cs_inst_page_t {
    struct cs_inst_page_t* next;
    cs_sound_inst_t instances[METAENGINE_SOUND_PAGE_INSTANCE_COUNT];
} cs_inst_page_t;

typedef struct cs_context_t {
    float global_pan /* = 0.5f */;
    float global_volume /* = 1.0f */;
    bool global_pause /* = false */;
    float music_volume /* = 1.0f */;
    float sound_volume /* = 1.0f */;

    bool music_paused /* = false */;
    bool music_looped /* = true */;
    float t /* = 0 */;
    float fade /* = 0 */;
    float fade_switch_1 /* = 0 */;
    cs_music_state_t music_state /* = MUSIC_STATE_NONE */;
    cs_music_state_t music_state_to_resume_from_paused /* = MUSIC_STATE_NONE */;
    cs_sound_inst_t* music_playing /* = NULL */;
    cs_sound_inst_t* music_next /* = NULL */;

    int audio_sources_to_free_capacity /* = 0 */;
    int audio_sources_to_free_size /* = 0 */;
    cs_audio_source_t** audio_sources_to_free /* = NULL */;
    uint64_t instance_id_gen /* = 1 */;
    hashtable_t instance_map;  // <uint64_t, cs_audio_source_t*>
    cs_inst_page_t* pages /* = NULL */;
    cs_list_t playing_sounds;
    cs_list_t free_sounds;
    void* mem_ctx /* = NULL */;

    unsigned latency_samples;
    int Hz;
    int bps;
    int wide_count;
    cs__m128* floatA;
    cs__m128* floatB;
    cs__m128i* samples;
    bool separate_thread;
    bool running;
    int sleep_milliseconds;

#if METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_WINDOWS

    DWORD last_cursor;
    unsigned running_index;
    int buffer_size;
    LPDIRECTSOUND dsound;
    LPDIRECTSOUNDBUFFER primary;
    LPDIRECTSOUNDBUFFER secondary;

    // data for cs_mix thread, enable these with cs_spawn_mix_thread
    CRITICAL_SECTION critical_section;

#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_APPLE

    unsigned index0;  // read
    unsigned index1;  // write
    unsigned samples_in_circular_buffer;
    int sample_count;

    // platform specific stuff
    AudioComponentInstance inst;

    // data for cs_mix thread, enable these with cs_spawn_mix_thread
    pthread_t thread;
    pthread_mutex_t mutex;

#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_SDL

    unsigned index0;  // read
    unsigned index1;  // write
    unsigned samples_in_circular_buffer;
    int sample_count;
    SDL_AudioDeviceID dev;

    // data for cs_mix thread, enable these with cs_spawn_mix_thread
    SDL_Thread* thread;
    SDL_mutex* mutex;

#endif
} cs_context_t;

cs_context_t* s_ctx = NULL;

void cs_sleep(int milliseconds) {
#if METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_WINDOWS
    Sleep(milliseconds);
#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_APPLE
    struct timespec ts = {0, milliseconds * 1000000};
    nanosleep(&ts, NULL);
#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_SDL
    SDL_Delay(milliseconds);
#endif
}

static void* cs_malloc16(size_t size, void* mem_ctx) {
    (void)mem_ctx;
    void* p = METAENGINE_SOUND_ALLOC(size + 16, mem_ctx);
    if (!p) return 0;
    unsigned char offset = (size_t)p & 15;
    p = (void*)METAENGINE_SOUND_ALIGN(p + 1, 16);
    *((char*)p - 1) = 16 - offset;
    METAENGINE_SOUND_ASSERT(!((size_t)p & 15));
    return p;
}

static void cs_free16(void* p, void* mem_ctx) {
    (void)mem_ctx;
    if (!p) return;
    METAENGINE_SOUND_FREE((char*)p - (((size_t) * ((char*)p - 1)) & 0xFF), NULL);
}

#if METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_SDL || METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_APPLE

static int cs_samples_written() { return s_ctx->samples_in_circular_buffer; }

static int cs_samples_unwritten() { return s_ctx->sample_count - s_ctx->samples_in_circular_buffer; }

static int cs_samples_to_mix() {
    int lat = s_ctx->latency_samples;
    int written = cs_samples_written();
    int dif = lat - written;
    METAENGINE_SOUND_ASSERT(dif >= 0);
    if (dif) {
        int unwritten = cs_samples_unwritten();
        return dif < unwritten ? dif : unwritten;
    }
    return 0;
}

#define METAENGINE_SOUND_SAMPLES_TO_BYTES(interleaved_sample_count) ((interleaved_sample_count)*s_ctx->bps)
#define METAENGINE_SOUND_BYTES_TO_SAMPLES(byte_count) ((byte_count) / s_ctx->bps)

static void cs_push_bytes(void* data, int size) {
    int index1 = s_ctx->index1;
    int samples_to_write = METAENGINE_SOUND_BYTES_TO_SAMPLES(size);
    int sample_count = s_ctx->sample_count;

    int unwritten = cs_samples_unwritten();
    if (unwritten < samples_to_write) samples_to_write = unwritten;
    int samples_to_end = sample_count - index1;

    if (samples_to_write > samples_to_end) {
        METAENGINE_SOUND_MEMCPY((char*)s_ctx->samples + METAENGINE_SOUND_SAMPLES_TO_BYTES(index1), data, METAENGINE_SOUND_SAMPLES_TO_BYTES(samples_to_end));
        METAENGINE_SOUND_MEMCPY(s_ctx->samples, (char*)data + METAENGINE_SOUND_SAMPLES_TO_BYTES(samples_to_end), size - METAENGINE_SOUND_SAMPLES_TO_BYTES(samples_to_end));
        s_ctx->index1 = (samples_to_write - samples_to_end) % sample_count;
    } else {
        METAENGINE_SOUND_MEMCPY((char*)s_ctx->samples + METAENGINE_SOUND_SAMPLES_TO_BYTES(index1), data, size);
        s_ctx->index1 = (s_ctx->index1 + samples_to_write) % sample_count;
    }

    s_ctx->samples_in_circular_buffer += samples_to_write;
}

static int cs_pull_bytes(void* dst, int size) {
    int index0 = s_ctx->index0;
    int allowed_size = METAENGINE_SOUND_SAMPLES_TO_BYTES(cs_samples_written());
    int sample_count = s_ctx->sample_count;
    int zeros = 0;

    if (allowed_size < size) {
        zeros = size - allowed_size;
        size = allowed_size;
    }

    int samples_to_read = METAENGINE_SOUND_BYTES_TO_SAMPLES(size);
    int samples_to_end = sample_count - index0;

    if (samples_to_read > samples_to_end) {
        METAENGINE_SOUND_MEMCPY(dst, ((char*)s_ctx->samples) + METAENGINE_SOUND_SAMPLES_TO_BYTES(index0), METAENGINE_SOUND_SAMPLES_TO_BYTES(samples_to_end));
        METAENGINE_SOUND_MEMCPY(((char*)dst) + METAENGINE_SOUND_SAMPLES_TO_BYTES(samples_to_end), s_ctx->samples, size - METAENGINE_SOUND_SAMPLES_TO_BYTES(samples_to_end));
        s_ctx->index0 = (samples_to_read - samples_to_end) % sample_count;
    } else {
        METAENGINE_SOUND_MEMCPY(dst, ((char*)s_ctx->samples) + METAENGINE_SOUND_SAMPLES_TO_BYTES(index0), size);
        s_ctx->index0 = (s_ctx->index0 + samples_to_read) % sample_count;
    }

    s_ctx->samples_in_circular_buffer -= samples_to_read;

    return zeros;
}

#endif

#if METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_WINDOWS

static DWORD WINAPI cs_ctx_thread(LPVOID lpParameter) {
    (void)lpParameter;
    while (s_ctx->running) {
        cs_mix();
        if (s_ctx->sleep_milliseconds)
            cs_sleep(s_ctx->sleep_milliseconds);
        else
            YieldProcessor();
    }

    s_ctx->separate_thread = false;
    return 0;
}

#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_APPLE

static void* cs_ctx_thread(void* udata) {
    while (s_ctx->running) {
        cs_mix();
        if (s_ctx->sleep_milliseconds)
            cs_sleep(s_ctx->sleep_milliseconds);
        else
            pthread_yield_np();
    }

    s_ctx->separate_thread = 0;
    pthread_exit(0);
    return 0;
}

static OSStatus cs_memcpy_to_coreaudio(void* udata, AudioUnitRenderActionFlags* ioActionFlags, const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList* ioData) {
    int bps = s_ctx->bps;
    int samples_requested_to_consume = inNumberFrames;
    AudioBuffer* buffer = ioData->mBuffers;

    METAENGINE_SOUND_ASSERT(ioData->mNumberBuffers == 1);
    METAENGINE_SOUND_ASSERT(buffer->mNumberChannels == 2);
    int byte_size = buffer->mDataByteSize;
    METAENGINE_SOUND_ASSERT(byte_size == samples_requested_to_consume * bps);

    int zero_bytes = cs_pull_bytes(s_ctx, buffer->mData, byte_size);
    METAENGINE_SOUND_MEMSET(((char*)buffer->mData) + (byte_size - zero_bytes), 0, zero_bytes);

    return noErr;
}

#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_SDL

int cs_ctx_thread(void* udata) {
    while (s_ctx->running) {
        cs_mix();
        if (s_ctx->sleep_milliseconds)
            cs_sleep(s_ctx->sleep_milliseconds);
        else
            cs_sleep(1);
    }

    s_ctx->separate_thread = false;
    return 0;
}

static void cs_sdl_audio_callback(void* udata, Uint8* stream, int len) {
    int zero_bytes = cs_pull_bytes(stream, len);
    METAENGINE_SOUND_MEMSET(stream + (len - zero_bytes), 0, zero_bytes);
}

#endif

static void s_add_page() {
    cs_inst_page_t* page = (cs_inst_page_t*)METAENGINE_SOUND_ALLOC(sizeof(cs_inst_page_t), user_allocator_context);
    for (int i = 0; i < METAENGINE_SOUND_PAGE_INSTANCE_COUNT; ++i) {
        cs_list_init_node(&page->instances[i].node);
        cs_list_push_back(&s_ctx->free_sounds, &page->instances[i].node);
    }
    page->next = s_ctx->pages;
    s_ctx->pages = page;
}

cs_error_t cs_init(void* os_handle, unsigned play_frequency_in_Hz, int buffered_samples, void* user_allocator_context /* = NULL */) {
    buffered_samples = buffered_samples < METAENGINE_SOUND_MINIMUM_BUFFERED_SAMPLES ? METAENGINE_SOUND_MINIMUM_BUFFERED_SAMPLES : buffered_samples;
    int sample_count = buffered_samples;
    int wide_count = (int)METAENGINE_SOUND_ALIGN(sample_count, 4);
    int bps = sizeof(uint16_t) * 2;

#if METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_WINDOWS

    int buffer_size = buffered_samples * bps;
    LPDIRECTSOUND dsound = NULL;
    LPDIRECTSOUNDBUFFER primary_buffer = NULL;
    LPDIRECTSOUNDBUFFER secondary_buffer = NULL;

    if (!os_handle) return METAENGINE_SOUND_ERROR_HWND_IS_NULL;
    {
        WAVEFORMATEX format = {0, 0, 0, 0, 0, 0, 0};
        DSBUFFERDESC bufdesc = {0, 0, 0, 0, 0, {0, 0, 0, 0}};
        HRESULT res = DirectSoundCreate(0, &dsound, 0);
        if (res != DS_OK) return METAENGINE_SOUND_ERROR_DIRECTSOUND_CREATE_FAILED;
        IDirectSound_SetCooperativeLevel(dsound, (HWND)os_handle, DSSCL_PRIORITY);
        bufdesc.dwSize = sizeof(bufdesc);
        bufdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;

        res = IDirectSound_CreateSoundBuffer(dsound, &bufdesc, &primary_buffer, 0);
        if (res != DS_OK) METAENGINE_SOUND_ERROR_CREATESOUNDBUFFER_FAILED;

        format.wFormatTag = WAVE_FORMAT_PCM;
        format.nChannels = 2;
        format.nSamplesPerSec = play_frequency_in_Hz;
        format.wBitsPerSample = 16;
        format.nBlockAlign = (format.nChannels * format.wBitsPerSample) / 8;
        format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
        format.cbSize = 0;
        res = IDirectSoundBuffer_SetFormat(primary_buffer, &format);
        if (res != DS_OK) METAENGINE_SOUND_ERROR_SETFORMAT_FAILED;

        bufdesc.dwSize = sizeof(bufdesc);
        bufdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
        bufdesc.dwBufferBytes = buffer_size;
        bufdesc.lpwfxFormat = &format;
        res = IDirectSound_CreateSoundBuffer(dsound, &bufdesc, &secondary_buffer, 0);
        if (res != DS_OK) METAENGINE_SOUND_ERROR_SETFORMAT_FAILED;

        // Silence the initial audio buffer.
        void* region1;
        DWORD size1;
        void* region2;
        DWORD size2;
        res = IDirectSoundBuffer_Lock(secondary_buffer, 0, bufdesc.dwBufferBytes, &region1, &size1, &region2, &size2, DSBLOCK_ENTIREBUFFER);
        if (res == DS_OK) {
            METAENGINE_SOUND_MEMSET(region1, 0, size1);
            IDirectSoundBuffer_Unlock(secondary_buffer, region1, size1, region2, size2);
        }
    }

#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_APPLE

    AudioComponentDescription comp_desc = {0};
    comp_desc.componentType = kAudioUnitType_Output;
    comp_desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    comp_desc.componentFlags = 0;
    comp_desc.componentFlagsMask = 0;
    comp_desc.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent comp = AudioComponentFindNext(NULL, &comp_desc);
    if (!comp) return METAENGINE_SOUND_ERROR_AUDIOCOMPONENTFINDNEXT_FAILED;

    AudioStreamBasicDescription stream_desc = {0};
    stream_desc.mSampleRate = (double)play_frequency_in_Hz;
    stream_desc.mFormatID = kAudioFormatLinearPCM;
    stream_desc.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked;
    stream_desc.mFramesPerPacket = 1;
    stream_desc.mChannelsPerFrame = 2;
    stream_desc.mBitsPerChannel = sizeof(uint16_t) * 8;
    stream_desc.mBytesPerPacket = bps;
    stream_desc.mBytesPerFrame = bps;
    stream_desc.mReserved = 0;

    AudioComponentInstance inst;
    OSStatus ret;
    AURenderCallbackStruct input;

    ret = AudioComponentInstanceNew(comp, &inst);
    if (ret != noErr) return METAENGINE_SOUND_ERROR_AUDIOCOMPONENTINSTANCENEW_FAILED;

    ret = AudioUnitSetProperty(inst, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &stream_desc, sizeof(stream_desc));
    if (ret != noErr) return METAENGINE_SOUND_ERROR_FAILED_TO_SET_STREAM_FORMAT;

    ret = AudioUnitInitialize(inst);
    if (ret != noErr) return METAENGINE_SOUND_ERROR_AUDIOUNITINITIALIZE_FAILED;

    ret = AudioOutputUnitStart(inst);
    if (ret != noErr) return METAENGINE_SOUND_ERROR_AUDIOUNITSTART_FAILED;

#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_SDL

    SDL_AudioSpec wanted, have;
    int ret = SDL_InitSubSystem(SDL_INIT_AUDIO);
    if (ret < 0) return METAENGINE_SOUND_ERROR_CANT_INIT_SDL_AUDIO;

#endif

    s_ctx = (cs_context_t*)METAENGINE_SOUND_ALLOC(sizeof(cs_context_t), user_allocator_context);
    s_ctx->global_pan = 0.5f;
    s_ctx->global_volume = 1.0f;
    s_ctx->global_pause = false;
    s_ctx->music_volume = 1.0f;
    s_ctx->sound_volume = 1.0f;
    s_ctx->music_looped = true;
    s_ctx->music_paused = false;
    s_ctx->t = 0;
    s_ctx->fade = 0;
    s_ctx->fade_switch_1 = 0;
    s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_NONE;
    s_ctx->music_state_to_resume_from_paused = METAENGINE_SOUND_MUSIC_STATE_NONE;
    s_ctx->music_playing = NULL;
    s_ctx->music_next = NULL;
    s_ctx->audio_sources_to_free_capacity = 32;
    s_ctx->audio_sources_to_free_size = 0;
    s_ctx->audio_sources_to_free = (cs_audio_source_t**)METAENGINE_SOUND_ALLOC(sizeof(cs_audio_source_t*) * s_ctx->audio_sources_to_free_capacity, s_ctx->mem_ctx);
    s_ctx->instance_id_gen = 1;
    hashtable_init(&s_ctx->instance_map, sizeof(cs_audio_source_t*), 1024, user_allocator_context);
    s_ctx->pages = NULL;
    cs_list_init(&s_ctx->playing_sounds);
    cs_list_init(&s_ctx->free_sounds);
    s_add_page();
    s_ctx->mem_ctx = user_allocator_context;
    s_ctx->latency_samples = buffered_samples;
    s_ctx->Hz = play_frequency_in_Hz;
    s_ctx->bps = bps;
    s_ctx->wide_count = wide_count;
    s_ctx->floatA = (cs__m128*)cs_malloc16(sizeof(cs__m128) * wide_count, s_ctx->mem_ctx);
    s_ctx->floatB = (cs__m128*)cs_malloc16(sizeof(cs__m128) * wide_count, s_ctx->mem_ctx);
    s_ctx->samples = (cs__m128i*)cs_malloc16(sizeof(cs__m128i) * wide_count, s_ctx->mem_ctx);
    s_ctx->running = true;
    s_ctx->separate_thread = false;
    s_ctx->sleep_milliseconds = 0;

#if METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_WINDOWS

    s_ctx->last_cursor = 0;
    s_ctx->running_index = 0;
    s_ctx->buffer_size = buffer_size;
    s_ctx->dsound = dsound;
    s_ctx->primary = primary_buffer;
    s_ctx->secondary = secondary_buffer;
    InitializeCriticalSectionAndSpinCount(&s_ctx->critical_section, 0x00000400);

#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_APPLE

    s_ctx->index0 = 0;
    s_ctx->index1 = 0;
    s_ctx->samples_in_circular_buffer = 0;
    s_ctx->sample_count = wide_count * 4;
    s_ctx->inst = inst;
    pthread_mutex_init(&s_ctx->mutex, NULL);

    input.inputProc = cs_memcpy_to_coreaudio;
    input.inputProcRefCon = s_ctx;
    ret = AudioUnitSetProperty(inst, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &input, sizeof(input));
    if (ret != noErr) return METAENGINE_SOUND_ERROR_FAILED_TO_SET_RENDER_CALLBACK;  // This leaks memory, oh well.

#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_SDL

    SDL_memset(&wanted, 0, sizeof(wanted));
    SDL_memset(&have, 0, sizeof(have));
    wanted.freq = play_frequency_in_Hz;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = 2; /* 1 = mono, 2 = stereo */
    wanted.samples = buffered_samples;
    wanted.callback = cs_sdl_audio_callback;
    wanted.userdata = s_ctx;
    s_ctx->index0 = 0;
    s_ctx->index1 = 0;
    s_ctx->samples_in_circular_buffer = 0;
    s_ctx->sample_count = wide_count * 4;
    s_ctx->dev = SDL_OpenAudioDevice(NULL, 0, &wanted, &have, 0);
    if (s_ctx->dev < 0) return METAENGINE_SOUND_ERROR_CANT_OPEN_AUDIO_DEVICE;  // This leaks memory, oh well.
    SDL_PauseAudioDevice(s_ctx->dev, 0);
    s_ctx->mutex = SDL_CreateMutex();

#endif

    return METAENGINE_SOUND_ERROR_NONE;
}

void cs_lock();
void cs_unlock();

void cs_shutdown() {
    if (!s_ctx) return;
    if (s_ctx->separate_thread) {
        cs_lock();
        s_ctx->running = false;
        cs_unlock();
        while (s_ctx->separate_thread) cs_sleep(1);
    }
#if METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_WINDOWS

    DeleteCriticalSection(&s_ctx->critical_section);
    IDirectSoundBuffer_Release(s_ctx->secondary);
    IDirectSoundBuffer_Release(s_ctx->primary);
    IDirectSoundBuffer_Release(s_ctx->dsound);

#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_APPLE
#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_SDL

    SDL_DestroyMutex(s_ctx->mutex);
    SDL_CloseAudioDevice(s_ctx->dev);

#endif

    cs_inst_page_t* page = s_ctx->pages;
    while (page) {
        cs_inst_page_t* next = page->next;
        METAENGINE_SOUND_FREE(page, s_ctx->mem_ctx);
        page = next;
    }

    for (int i = 0; i < s_ctx->audio_sources_to_free_size; ++i) {
        cs_audio_source_t* audio = s_ctx->audio_sources_to_free[i];
        cs_free16(audio->channels[0], s_ctx->mem_ctx);
        METAENGINE_SOUND_FREE(audio, s_ctx->mem_ctx);
    }
    METAENGINE_SOUND_FREE(s_ctx->audio_sources_to_free, s_ctx->mem_ctx);

    cs_free16(s_ctx->floatA, s_ctx->mem_ctx);
    cs_free16(s_ctx->floatB, s_ctx->mem_ctx);
    cs_free16(s_ctx->samples, s_ctx->mem_ctx);
    hashtable_term(&s_ctx->instance_map);
    void* mem_ctx = s_ctx->mem_ctx;
    (void)mem_ctx;
    METAENGINE_SOUND_FREE(s_ctx, mem_ctx);
    s_ctx = NULL;
}

static float s_smoothstep(float x) { return x * x * (3.0f - 2.0f * x); }

void cs_update(float dt) {
    if (!s_ctx->separate_thread) cs_mix();

    switch (s_ctx->music_state) {
        case METAENGINE_SOUND_MUSIC_STATE_FADE_OUT: {
            s_ctx->t += dt;
            if (s_ctx->t >= s_ctx->fade) {
                s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_NONE;
                s_ctx->music_playing->active = false;
                s_ctx->music_playing = NULL;
            } else {
                s_ctx->music_playing->volume = s_smoothstep(((s_ctx->fade - s_ctx->t) / s_ctx->fade));
                ;
            }
        } break;

        case METAENGINE_SOUND_MUSIC_STATE_FADE_IN: {
            s_ctx->t += dt;
            if (s_ctx->t >= s_ctx->fade) {
                s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_PLAYING;
                s_ctx->t = s_ctx->fade;
            }
            s_ctx->music_playing->volume = s_smoothstep(1.0f - ((s_ctx->fade - s_ctx->t) / s_ctx->fade));
        } break;

        case METAENGINE_SOUND_MUSIC_STATE_SWITCH_TO_0: {
            s_ctx->t += dt;
            if (s_ctx->t >= s_ctx->fade) {
                s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_SWITCH_TO_1;
                s_ctx->music_playing->active = false;
                s_ctx->music_playing->volume = 0;
                s_ctx->t = 0;
                s_ctx->fade = s_ctx->fade_switch_1;
                s_ctx->fade_switch_1 = 0;
                s_ctx->music_next->paused = false;
            } else {
                s_ctx->music_playing->volume = s_smoothstep(((s_ctx->fade - s_ctx->t) / s_ctx->fade));
                ;
            }
        } break;

        case METAENGINE_SOUND_MUSIC_STATE_SWITCH_TO_1: {
            s_ctx->t += dt;
            if (s_ctx->t >= s_ctx->fade) {
                s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_PLAYING;
                s_ctx->t = s_ctx->fade;
                s_ctx->music_next->volume = 1.0f;
                s_ctx->music_playing = s_ctx->music_next;
                s_ctx->music_next = NULL;
            } else {
                float t = s_smoothstep(1.0f - ((s_ctx->fade - s_ctx->t) / s_ctx->fade));
                float volume = t;
                s_ctx->music_next->volume = volume;
            }
        } break;

        case METAENGINE_SOUND_MUSIC_STATE_CROSSFADE: {
            s_ctx->t += dt;
            if (s_ctx->t >= s_ctx->fade) {
                s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_PLAYING;
                s_ctx->music_playing->active = false;
                s_ctx->music_next->volume = 1.0f;
                s_ctx->music_playing = s_ctx->music_next;
                s_ctx->music_next = NULL;
            } else {
                float t0 = s_smoothstep(((s_ctx->fade - s_ctx->t) / s_ctx->fade));
                float t1 = s_smoothstep(1.0f - ((s_ctx->fade - s_ctx->t) / s_ctx->fade));
                float v0 = t0;
                float v1 = t1;
                s_ctx->music_playing->volume = v0;
                s_ctx->music_next->volume = v1;
            }
        } break;

        default:
            break;
    }
}

void cs_set_global_volume(float volume_0_to_1) {
    if (volume_0_to_1 < 0) volume_0_to_1 = 0;
    s_ctx->global_volume = volume_0_to_1;
}

void cs_set_global_pan(float pan_0_to_1) {
    if (pan_0_to_1 < 0) pan_0_to_1 = 0;
    if (pan_0_to_1 > 1) pan_0_to_1 = 1;
    s_ctx->global_pan = pan_0_to_1;
}

void cs_set_global_pause(bool true_for_paused) { s_ctx->global_pause = true_for_paused; }

void cs_lock() {
#if METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_WINDOWS
    EnterCriticalSection(&s_ctx->critical_section);
#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_APPLE
    pthread_mutex_lock(&s_ctx->mutex);
#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_SDL
    SDL_LockMutex(s_ctx->mutex);
#endif
}

void cs_unlock() {
#if METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_WINDOWS
    LeaveCriticalSection(&s_ctx->critical_section);
#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_APPLE
    pthread_mutex_unlock(&s_ctx->mutex);
#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_SDL
    SDL_UnlockMutex(s_ctx->mutex);
#endif
}

#if METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_WINDOWS

static void cs_dsound_get_bytes_to_fill(int* byte_to_lock, int* bytes_to_write) {
    DWORD play_cursor;
    DWORD write_cursor;
    DWORD lock;
    DWORD target_cursor;
    DWORD write;
    DWORD status;

    HRESULT hr = IDirectSoundBuffer_GetCurrentPosition(s_ctx->secondary, &play_cursor, &write_cursor);
    if (hr != DS_OK) {
        if (hr == DSERR_BUFFERLOST) {
            hr = IDirectSoundBuffer_Restore(s_ctx->secondary);
        }
        *byte_to_lock = write_cursor;
        *bytes_to_write = s_ctx->latency_samples * s_ctx->bps;
        if (!SUCCEEDED(hr)) {
            return;
        }
    }

    s_ctx->last_cursor = write_cursor;

    IDirectSoundBuffer_GetStatus(s_ctx->secondary, &status);
    if (!(status & DSBSTATUS_PLAYING)) {
        hr = IDirectSoundBuffer_Play(s_ctx->secondary, 0, 0, DSBPLAY_LOOPING);
        if (!SUCCEEDED(hr)) {
            return;
        }
    }

    lock = (s_ctx->running_index * s_ctx->bps) % s_ctx->buffer_size;
    target_cursor = (write_cursor + s_ctx->latency_samples * s_ctx->bps);
    if (target_cursor > (DWORD)s_ctx->buffer_size) target_cursor %= s_ctx->buffer_size;
    target_cursor = (DWORD)METAENGINE_SOUND_TRUNC(target_cursor, 16);

    if (lock > target_cursor) {
        write = (s_ctx->buffer_size - lock) + target_cursor;
    } else {
        write = target_cursor - lock;
    }

    *byte_to_lock = lock;
    *bytes_to_write = write;
}

static void cs_dsound_memcpy_to_driver(int16_t* samples, int byte_to_lock, int bytes_to_write) {
    // copy mixer buffers to direct sound
    void* region1;
    DWORD size1;
    void* region2;
    DWORD size2;
    HRESULT hr = IDirectSoundBuffer_Lock(s_ctx->secondary, byte_to_lock, bytes_to_write, &region1, &size1, &region2, &size2, 0);
    if (hr == DSERR_BUFFERLOST) {
        IDirectSoundBuffer_Restore(s_ctx->secondary);
        hr = IDirectSoundBuffer_Lock(s_ctx->secondary, byte_to_lock, bytes_to_write, &region1, &size1, &region2, &size2, 0);
    }
    if (!SUCCEEDED(hr)) {
        return;
    }

    unsigned running_index = s_ctx->running_index;
    INT16* sample1 = (INT16*)region1;
    DWORD sample1_count = size1 / s_ctx->bps;
    memcpy(sample1, samples, sample1_count * sizeof(INT16) * 2);
    samples += sample1_count * 2;
    running_index += sample1_count;

    INT16* sample2 = (INT16*)region2;
    DWORD sample2_count = size2 / s_ctx->bps;
    memcpy(sample2, samples, sample2_count * sizeof(INT16) * 2);
    samples += sample2_count * 2;
    running_index += sample2_count;

    IDirectSoundBuffer_Unlock(s_ctx->secondary, region1, size1, region2, size2);
    s_ctx->running_index = running_index;
}

void cs_dsound_dont_run_too_fast() {
    DWORD status;
    DWORD cursor;
    DWORD junk;
    HRESULT hr;

    hr = IDirectSoundBuffer_GetCurrentPosition(s_ctx->secondary, &junk, &cursor);
    if (hr != DS_OK) {
        if (hr == DSERR_BUFFERLOST) {
            IDirectSoundBuffer_Restore(s_ctx->secondary);
        }
        return;
    }

    // Prevent mixing thread from sending samples too quickly.
    while (cursor == s_ctx->last_cursor) {
        cs_sleep(1);

        IDirectSoundBuffer_GetStatus(s_ctx->secondary, &status);
        if ((status & DSBSTATUS_BUFFERLOST)) {
            IDirectSoundBuffer_Restore(s_ctx->secondary);
            IDirectSoundBuffer_GetStatus(s_ctx->secondary, &status);
            if ((status & DSBSTATUS_BUFFERLOST)) {
                break;
            }
        }

        hr = IDirectSoundBuffer_GetCurrentPosition(s_ctx->secondary, &junk, &cursor);
        if (hr != DS_OK) {
            // Eek! Not much to do here I guess.
            return;
        }
    }
}

#endif  // METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_WINDOWS

void cs_mix() {
    cs__m128i* samples;
    cs__m128* floatA;
    cs__m128* floatB;
    cs__m128 zero;
    int wide_count;
    int samples_to_write;

    cs_lock();

#if METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_WINDOWS

    int byte_to_lock;
    int bytes_to_write;
    cs_dsound_get_bytes_to_fill(&byte_to_lock, &bytes_to_write);

    if (bytes_to_write < (int)s_ctx->latency_samples) goto unlock;
    samples_to_write = bytes_to_write / s_ctx->bps;

#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_APPLE || METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_SDL

    int bytes_to_write;
    samples_to_write = cs_samples_to_mix();
    if (!samples_to_write) goto unlock;
    bytes_to_write = samples_to_write * s_ctx->bps;

#endif

    // Clear mixer buffers.
    wide_count = (int)METAENGINE_SOUND_ALIGN(samples_to_write, 4) / 4;

    floatA = s_ctx->floatA;
    floatB = s_ctx->floatB;
    zero = cs_mm_set1_ps(0.0f);

    for (int i = 0; i < wide_count; ++i) {
        floatA[i] = zero;
        floatB[i] = zero;
    }

    // Mix all playing sounds into the mixer buffers.
    if (!s_ctx->global_pause && !cs_list_empty(&s_ctx->playing_sounds)) {
        cs_list_node_t* playing_node = cs_list_begin(&s_ctx->playing_sounds);
        cs_list_node_t* end_node = cs_list_end(&s_ctx->playing_sounds);
        do {
            cs_list_node_t* next_node = playing_node->next;
            cs_sound_inst_t* playing = METAENGINE_SOUND_LIST_HOST(cs_sound_inst_t, node, playing_node);
            cs_audio_source_t* audio = playing->audio;

            if (!playing->active || !s_ctx->running) goto remove;
            if (!audio) goto remove;
            if (playing->paused) goto get_next_playing_sound;

            {
                cs__m128* cA = (cs__m128*)audio->channels[0];
                cs__m128* cB = (cs__m128*)audio->channels[1];

                // Attempted to play a sound with no audio.
                // Make sure the audio file was loaded properly.
                METAENGINE_SOUND_ASSERT(cA);

                int mix_count = samples_to_write;
                int offset = (int)playing->sample_index;
                int remaining = audio->sample_count - offset;
                if (remaining < mix_count) mix_count = remaining;
                METAENGINE_SOUND_ASSERT(remaining > 0);

                float gpan0 = 1.0f - s_ctx->global_pan;
                float gpan1 = s_ctx->global_pan;
                float vA0 = playing->volume * playing->pan0 * gpan0 * s_ctx->global_volume;
                float vB0 = playing->volume * playing->pan1 * gpan1 * s_ctx->global_volume;
                if (!playing->is_music) {
                    vA0 *= s_ctx->sound_volume;
                    vB0 *= s_ctx->sound_volume;
                } else {
                    vA0 *= s_ctx->music_volume;
                    vB0 *= s_ctx->music_volume;
                }
                cs__m128 vA = cs_mm_set1_ps(vA0);
                cs__m128 vB = cs_mm_set1_ps(vB0);

                // Skip sound if it's delay is longer than mix_count and
                // handle various delay cases.
                int delay_offset = 0;
                if (offset < 0) {
                    int samples_till_positive = -offset;
                    int mix_leftover = mix_count - samples_till_positive;

                    if (mix_leftover <= 0) {
                        playing->sample_index += mix_count;
                        goto get_next_playing_sound;
                    } else {
                        offset = 0;
                        delay_offset = samples_till_positive;
                        mix_count = mix_leftover;
                    }
                }
                METAENGINE_SOUND_ASSERT(!(delay_offset & 3));

                // SIMD offets.
                int mix_wide = (int)METAENGINE_SOUND_ALIGN(mix_count, 4) / 4;
                int offset_wide = (int)METAENGINE_SOUND_TRUNC(offset, 4) / 4;
                int delay_wide = (int)METAENGINE_SOUND_ALIGN(delay_offset, 4) / 4;
                int sample_count = (mix_wide - 2 * delay_wide) * 4;
                (void)sample_count;

                // apply volume, load samples into float buffers
                switch (audio->channel_count) {
                    case 1:
                        for (int i = delay_wide; i < mix_wide - delay_wide; ++i) {
                            cs__m128 A = cA[i + offset_wide];
                            cs__m128 B = cs_mm_mul_ps(A, vB);
                            A = cs_mm_mul_ps(A, vA);
                            floatA[i] = cs_mm_add_ps(floatA[i], A);
                            floatB[i] = cs_mm_add_ps(floatB[i], B);
                        }
                        break;

                    case 2: {
                        for (int i = delay_wide; i < mix_wide - delay_wide; ++i) {
                            cs__m128 A = cA[i + offset_wide];
                            cs__m128 B = cB[i + offset_wide];

                            A = cs_mm_mul_ps(A, vA);
                            B = cs_mm_mul_ps(B, vB);
                            floatA[i] = cs_mm_add_ps(floatA[i], A);
                            floatB[i] = cs_mm_add_ps(floatB[i], B);
                        }
                    } break;
                }

                // playing list logic
                playing->sample_index += mix_count;
                METAENGINE_SOUND_ASSERT(playing->sample_index <= audio->sample_count);
                if (playing->sample_index == audio->sample_count) {
                    if (playing->looped) {
                        playing->sample_index = 0;
                        goto get_next_playing_sound;
                    }

                    goto remove;
                }
            }

        get_next_playing_sound:
            playing_node = next_node;
            continue;

        remove:
            playing->sample_index = 0;
            playing->active = false;

            if (playing->audio) {
                playing->audio->playing_count -= 1;
                METAENGINE_SOUND_ASSERT(playing->audio->playing_count >= 0);
            }

            cs_list_remove(playing_node);
            cs_list_push_front(&s_ctx->free_sounds, playing_node);
            hashtable_remove(&s_ctx->instance_map, playing->id);
            playing_node = next_node;
            continue;
        } while (playing_node != end_node);
    }

    // load all floats into 16 bit packed interleaved samples
#if METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_WINDOWS

    samples = s_ctx->samples;
    for (int i = 0; i < wide_count; ++i) {
        cs__m128i a = cs_mm_cvtps_epi32(floatA[i]);
        cs__m128i b = cs_mm_cvtps_epi32(floatB[i]);
        cs__m128i a0b0a1b1 = cs_mm_unpacklo_epi32(a, b);
        cs__m128i a2b2a3b3 = cs_mm_unpackhi_epi32(a, b);
        samples[i] = cs_mm_packs_epi32(a0b0a1b1, a2b2a3b3);
    }
    cs_dsound_memcpy_to_driver((int16_t*)samples, byte_to_lock, bytes_to_write);
    cs_dsound_dont_run_too_fast();

#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_APPLE || METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_SDL

    // Since the ctx->samples array is already in use as a ring buffer
    // reusing floatA to store output is a good way to temporarly store
    // the final samples. Then a single ring buffer push can be used
    // afterwards. Pretty hacky, but whatever :)
    samples = (cs__m128i*)floatA;
    for (int i = 0; i < wide_count; ++i) {
        cs__m128i a = cs_mm_cvtps_epi32(floatA[i]);
        cs__m128i b = cs_mm_cvtps_epi32(floatB[i]);
        cs__m128i a0b0a1b1 = cs_mm_unpacklo_epi32(a, b);
        cs__m128i a2b2a3b3 = cs_mm_unpackhi_epi32(a, b);
        samples[i] = cs_mm_packs_epi32(a0b0a1b1, a2b2a3b3);
    }

    cs_push_bytes(samples, bytes_to_write);

#endif

    // Free up any queue'd free's for audio sources at zero refcount.
    for (int i = 0; i < s_ctx->audio_sources_to_free_size;) {
        cs_audio_source_t* audio = s_ctx->audio_sources_to_free[i];
        if (audio->playing_count == 0) {
            cs_free16(audio->channels[0], s_ctx->mem_ctx);
            METAENGINE_SOUND_FREE(audio, s_ctx->mem_ctx);
            s_ctx->audio_sources_to_free[i] = s_ctx->audio_sources_to_free[--s_ctx->audio_sources_to_free_size];
        } else {
            ++i;
        }
    }

unlock:
    cs_unlock();
}

void cs_spawn_mix_thread() {
    if (s_ctx->separate_thread) return;
    s_ctx->separate_thread = true;
#if METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_WINDOWS
    CreateThread(0, 0, cs_ctx_thread, s_ctx, 0, 0);
#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_APPLE
    pthread_create(&s_ctx->thread, 0, cs_ctx_thread, s_ctx);
#elif METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_SDL
    s_ctx->thread = SDL_CreateThread(&cs_ctx_thread, "CuteSoundThread", s_ctx);
#endif
}

void cs_mix_thread_sleep_delay(int milliseconds) { s_ctx->sleep_milliseconds = milliseconds; }

void* cs_get_context_ptr() { return (void*)s_ctx; }

void cs_set_context_ptr(void* ctx) { s_ctx = (cs_context_t*)ctx; }

// -------------------------------------------------------------------------------------------------
// Loaded sounds.

static void* cs_read_file_to_memory(const char* path, int* size, void* mem_ctx) {
    (void)mem_ctx;
    void* data = 0;
    METAENGINE_SOUND_FILE* fp = METAENGINE_SOUND_FOPEN(path, "rb");
    int sizeNum = 0;

    if (fp) {
        METAENGINE_SOUND_FSEEK(fp, 0, METAENGINE_SOUND_SEEK_END);
        sizeNum = (int)METAENGINE_SOUND_FTELL(fp);
        METAENGINE_SOUND_FSEEK(fp, 0, METAENGINE_SOUND_SEEK_SET);
        data = METAENGINE_SOUND_ALLOC(sizeNum, mem_ctx);
        (void)(METAENGINE_SOUND_FREAD(data, sizeNum, 1, fp) + 1);
        METAENGINE_SOUND_FCLOSE(fp);
    }

    if (size) *size = sizeNum;
    return data;
}

static int cs_four_cc(const char* CC, void* memory) {
    if (!METAENGINE_SOUND_MEMCMP(CC, memory, 4)) return 1;
    return 0;
}

static char* cs_next(char* data) {
    uint32_t size = *(uint32_t*)(data + 4);
    size = (size + 1) & ~1;
    return data + 8 + size;
}

static void cs_last_element(cs__m128* a, int i, int j, int16_t* samples, int offset) {
    switch (offset) {
        case 1:
            a[i] = cs_mm_set_ps(samples[j], 0.0f, 0.0f, 0.0f);
            break;

        case 2:
            a[i] = cs_mm_set_ps(samples[j], samples[j + 1], 0.0f, 0.0f);
            break;

        case 3:
            a[i] = cs_mm_set_ps(samples[j], samples[j + 1], samples[j + 2], 0.0f);
            break;

        case 0:
            a[i] = cs_mm_set_ps(samples[j], samples[j + 1], samples[j + 2], samples[j + 3]);
            break;
    }
}

cs_audio_source_t* cs_load_wav(const char* path, cs_error_t* err /* = NULL */) {
    int size;
    void* wav = cs_read_file_to_memory(path, &size, s_ctx->mem_ctx);
    if (!wav) return NULL;
    cs_audio_source_t* audio = cs_read_mem_wav(wav, size, err);
    METAENGINE_SOUND_FREE(wav, s_ctx->mem_ctx);
    return audio;
}

cs_audio_source_t* cs_read_mem_wav(const void* memory, size_t size, cs_error_t* err) {
    if (err) *err = METAENGINE_SOUND_ERROR_NONE;
    if (!memory) {
        if (err) *err = METAENGINE_SOUND_ERROR_FILE_NOT_FOUND;
        return NULL;
    }

#pragma pack(push, 1)
    typedef struct {
        uint16_t wFormatTag;
        uint16_t nChannels;
        uint32_t nSamplesPerSec;
        uint32_t nAvgBytesPerSec;
        uint16_t nBlockAlign;
        uint16_t wBitsPerSample;
        uint16_t cbSize;
        uint16_t wValidBitsPerSample;
        uint32_t dwChannelMask;
        uint8_t SubFormat[18];
    } Fmt;
#pragma pack(pop)

    cs_audio_source_t* audio = NULL;
    char* data = (char*)memory;
    char* end = data + size;
    if (!cs_four_cc("RIFF", data)) {
        if (err) *err = METAENGINE_SOUND_ERROR_THE_FILE_IS_NOT_A_WAV_FILE;
        return NULL;
    }
    if (!cs_four_cc("WAVE", data + 8)) {
        if (err) *err = METAENGINE_SOUND_ERROR_THE_FILE_IS_NOT_A_WAV_FILE;
        return NULL;
    }

    data += 12;

    while (1) {
        if (!(end > data)) {
            if (err) *err = METAENGINE_SOUND_ERROR_WAV_FILE_FORMAT_CHUNK_NOT_FOUND;
            return NULL;
        }
        if (cs_four_cc("fmt ", data)) break;
        data = cs_next(data);
    }

    Fmt fmt;
    fmt = *(Fmt*)(data + 8);
    if (fmt.wFormatTag != 1) {
        if (err) *err = METAENGINE_SOUND_ERROR_WAV_FILE_FORMAT_CHUNK_NOT_FOUND;
        return NULL;
    }
    if (!(fmt.nChannels == 1 || fmt.nChannels == 2)) {
        if (err) *err = METAENGINE_SOUND_ERROR_WAV_ONLY_MONO_OR_STEREO_IS_SUPPORTED;
        return NULL;
    }
    if (!(fmt.wBitsPerSample == 16)) {
        if (err) *err = METAENGINE_SOUND_ERROR_WAV_ONLY_16_BITS_PER_SAMPLE_SUPPORTED;
        return NULL;
    }
    if (!(fmt.nBlockAlign == fmt.nChannels * 2)) {
        if (err) *err = METAENGINE_SOUND_ERROR_IMPLEMENTATION_ERROR_PLEASE_REPORT_THIS_ON_GITHUB;
        return NULL;
    }

    while (1) {
        if (!(end > data)) {
            if (err) *err = METAENGINE_SOUND_ERROR_WAV_DATA_CHUNK_NOT_FOUND;
            return NULL;
        }
        if (cs_four_cc("data", data)) break;
        data = cs_next(data);
    }

    audio = (cs_audio_source_t*)METAENGINE_SOUND_ALLOC(sizeof(cs_audio_source_t), s_ctx->mem_ctx);
    METAENGINE_SOUND_MEMSET(audio, 0, sizeof(*audio));
    audio->sample_rate = (int)fmt.nSamplesPerSec;

    {
        int sample_size = *((uint32_t*)(data + 4));
        int sample_count = sample_size / (fmt.nChannels * sizeof(uint16_t));
        audio->sample_count = sample_count;
        audio->channel_count = fmt.nChannels;

        int wide_count = (int)METAENGINE_SOUND_ALIGN(sample_count, 4);
        wide_count /= 4;
        int wide_offset = sample_count & 3;
        int16_t* samples = (int16_t*)(data + 8);
        float* sample = (float*)alloca(sizeof(float) * 4 + 16);
        sample = (float*)METAENGINE_SOUND_ALIGN(sample, 16);

        switch (audio->channel_count) {
            case 1: {
                audio->channels[0] = cs_malloc16(wide_count * sizeof(cs__m128), NULL);
                audio->channels[1] = 0;
                cs__m128* a = (cs__m128*)audio->channels[0];

                for (int i = 0, j = 0; i < wide_count - 1; ++i, j += 4) {
                    sample[0] = (float)samples[j];
                    sample[1] = (float)samples[j + 1];
                    sample[2] = (float)samples[j + 2];
                    sample[3] = (float)samples[j + 3];
                    a[i] = cs_mm_load_ps(sample);
                }

                cs_last_element(a, wide_count - 1, (wide_count - 1) * 4, samples, wide_offset);
            } break;

            case 2: {
                cs__m128* a = (cs__m128*)cs_malloc16(wide_count * sizeof(cs__m128) * 2, NULL);
                cs__m128* b = a + wide_count;

                for (int i = 0, j = 0; i < wide_count - 1; ++i, j += 8) {
                    sample[0] = (float)samples[j];
                    sample[1] = (float)samples[j + 2];
                    sample[2] = (float)samples[j + 4];
                    sample[3] = (float)samples[j + 6];
                    a[i] = cs_mm_load_ps(sample);

                    sample[0] = (float)samples[j + 1];
                    sample[1] = (float)samples[j + 3];
                    sample[2] = (float)samples[j + 5];
                    sample[3] = (float)samples[j + 7];
                    b[i] = cs_mm_load_ps(sample);
                }

                cs_last_element(a, wide_count - 1, (wide_count - 1) * 4, samples, wide_offset);
                cs_last_element(b, wide_count - 1, (wide_count - 1) * 4 + 4, samples, wide_offset);
                audio->channels[0] = a;
                audio->channels[1] = b;
            } break;

            default:
                if (err) *err = METAENGINE_SOUND_ERROR_WAV_ONLY_MONO_OR_STEREO_IS_SUPPORTED;
                METAENGINE_SOUND_ASSERT(false);
        }
    }

    if (err) *err = METAENGINE_SOUND_ERROR_NONE;
    return audio;
}

void cs_free_audio_source(cs_audio_source_t* audio) {
    cs_lock();
    if (audio->playing_count == 0) {
        cs_free16(audio->channels[0], s_ctx->mem_ctx);
        METAENGINE_SOUND_FREE(audio, s_ctx->mem_ctx);
    } else {
        if (s_ctx->audio_sources_to_free_size == s_ctx->audio_sources_to_free_capacity) {
            int new_capacity = s_ctx->audio_sources_to_free_capacity * 2;
            cs_audio_source_t** new_sources = (cs_audio_source_t**)METAENGINE_SOUND_ALLOC(new_capacity, s_ctx->mem_ctx);
            METAENGINE_SOUND_MEMCPY(new_sources, s_ctx->audio_sources_to_free, sizeof(cs_audio_source_t*) * s_ctx->audio_sources_to_free_size);
            METAENGINE_SOUND_FREE(s_ctx->audio_sources_to_free, s_ctx->mem_ctx);
            s_ctx->audio_sources_to_free = new_sources;
            s_ctx->audio_sources_to_free_capacity = new_capacity;
        }
        s_ctx->audio_sources_to_free[s_ctx->audio_sources_to_free_size++] = audio;
    }
    cs_unlock();
}

#if METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_SDL && defined(SDL_rwops_h_) && defined(METAENGINE_SOUND_SDL_RWOPS)

// Load an SDL_RWops object's data into memory.
// Ripped straight from: https://wiki.libsdl.org/SDL_RWread
static void* cs_read_rw_to_memory(SDL_RWops* rw, int* size, void* mem_ctx) {
    Sint64 res_size = SDL_RWsize(rw);
    char* data = (char*)METAENGINE_SOUND_ALLOC((size_t)(res_size + 1), mem_ctx);

    Sint64 nb_read_total = 0, nb_read = 1;
    char* buf = data;
    while (nb_read_total < res_size && nb_read != 0) {
        nb_read = SDL_RWread(rw, buf, 1, (size_t)(res_size - nb_read_total));
        nb_read_total += nb_read;
        buf += nb_read;
    }

    SDL_RWclose(rw);

    if (nb_read_total != res_size) {
        METAENGINE_SOUND_FREE(data, NULL);
        return NULL;
    }

    if (size) *size = (int)res_size;
    return data;
}

cs_audio_source_t* cs_load_wav_rw(SDL_RWops* context, cs_error_t* err) {
    int size;
    char* wav = (char*)cs_read_rw_to_memory(context, &size, s_ctx->mem_ctx);
    if (!memory) return NULL;
    cs_audio_source_t* audio = cs_read_mem_wav(wav, length, err);
    METAENGINE_SOUND_FREE(wav, s_ctx->mem_ctx);
    return audio;
}

#endif

// If stb_vorbis was included *before* cute_sound go ahead and create
// some functions for dealing with OGG files.
#ifdef STB_VORBIS_INCLUDE_STB_VORBIS_H

cs_audio_source_t* cs_read_mem_ogg(const void* memory, size_t length, cs_error_t* err) {
    int16_t* samples = 0;
    cs_audio_source_t* audio = NULL;
    int channel_count;
    int sample_rate;
    int sample_count = stb_vorbis_decode_memory((const unsigned char*)memory, (int)length, &channel_count, &sample_rate, &samples);
    if (sample_count <= 0) {
        if (err) *err = METAENGINE_SOUND_ERROR_STB_VORBIS_DECODE_FAILED;
        return NULL;
    }
    audio = (cs_audio_source_t*)METAENGINE_SOUND_ALLOC(sizeof(cs_audio_source_t), s_ctx->mem_ctx);
    METAENGINE_SOUND_MEMSET(audio, 0, sizeof(*audio));

    {
        int wide_count = (int)METAENGINE_SOUND_ALIGN(sample_count, 4) / 4;
        int wide_offset = sample_count & 3;
        float* sample = (float*)alloca(sizeof(float) * 4 + 16);
        sample = (float*)METAENGINE_SOUND_ALIGN(sample, 16);
        cs__m128* a = NULL;
        cs__m128* b = NULL;

        switch (channel_count) {
            case 1: {
                a = (cs__m128*)cs_malloc16(wide_count * sizeof(cs__m128), NULL);
                b = 0;

                for (int i = 0, j = 0; i < wide_count - 1; ++i, j += 4) {
                    sample[0] = (float)samples[j];
                    sample[1] = (float)samples[j + 1];
                    sample[2] = (float)samples[j + 2];
                    sample[3] = (float)samples[j + 3];
                    a[i] = cs_mm_load_ps(sample);
                }

                cs_last_element(a, wide_count - 1, (wide_count - 1) * 4, samples, wide_offset);
            } break;

            case 2:
                a = (cs__m128*)cs_malloc16(wide_count * sizeof(cs__m128) * 2, NULL);
                b = a + wide_count;

                for (int i = 0, j = 0; i < wide_count - 1; ++i, j += 8) {
                    sample[0] = (float)samples[j];
                    sample[1] = (float)samples[j + 2];
                    sample[2] = (float)samples[j + 4];
                    sample[3] = (float)samples[j + 6];
                    a[i] = cs_mm_load_ps(sample);

                    sample[0] = (float)samples[j + 1];
                    sample[1] = (float)samples[j + 3];
                    sample[2] = (float)samples[j + 5];
                    sample[3] = (float)samples[j + 7];
                    b[i] = cs_mm_load_ps(sample);
                }

                cs_last_element(a, wide_count - 1, (wide_count - 1) * 4, samples, wide_offset);
                cs_last_element(b, wide_count - 1, (wide_count - 1) * 4 + 4, samples, wide_offset);
                break;

            default:
                if (err) *err = METAENGINE_SOUND_ERROR_OGG_UNSUPPORTED_CHANNEL_COUNT;
                METAENGINE_SOUND_ASSERT(false);
        }

        audio->sample_rate = sample_rate;
        audio->sample_count = sample_count;
        audio->channel_count = channel_count;
        audio->channels[0] = a;
        audio->channels[1] = b;
        audio->playing_count = 0;
        free(samples);
    }

    if (err) *err = METAENGINE_SOUND_ERROR_NONE;
    return audio;
}

cs_audio_source_t* cs_load_ogg(const char* path, cs_error_t* err) {
    int length;
    void* memory = cs_read_file_to_memory(path, &length, NULL);
    if (!memory) return NULL;
    cs_audio_source_t* audio = cs_read_mem_ogg(memory, length, err);
    METAENGINE_SOUND_FREE(memory, NULL);
    return audio;
}

#if METAENGINE_SOUND_PLATFORM == METAENGINE_SOUND_SDL && defined(SDL_rwops_h_) && defined(METAENGINE_SOUND_SDL_RWOPS)

cs_audio_source_t* cs_load_ogg_rw(SDL_RWops* rw, cs_error_t* err) {
    int length;
    void* memory = cs_read_rw_to_memory(rw, &length, s_ctx->mem_ctx);
    if (!memory) return NULL;
    cs_audio_source_t* audio = cs_read_ogg_wav(memory, length, err);
    METAENGINE_SOUND_FREE(memory, s_ctx->mem_ctx);
    return audio;
}

#endif
#endif  // STB_VORBIS_INCLUDE_STB_VORBIS_H

// -------------------------------------------------------------------------------------------------
// Music sounds.

static void s_insert(cs_sound_inst_t* inst) {
    cs_lock();
    cs_list_push_back(&s_ctx->playing_sounds, &inst->node);
    inst->audio->playing_count += 1;
    inst->active = true;
    inst->id = s_ctx->instance_id_gen++;
    hashtable_insert(&s_ctx->instance_map, inst->id, inst);
    // s_on_make_playing(inst);
    cs_unlock();
}

static cs_sound_inst_t* s_inst_music(cs_audio_source_t* src, float volume) {
    if (cs_list_empty(&s_ctx->free_sounds)) {
        s_add_page();
    }
    METAENGINE_SOUND_ASSERT(!cs_list_empty(&s_ctx->free_sounds));
    cs_sound_inst_t* inst = METAENGINE_SOUND_LIST_HOST(cs_sound_inst_t, node, cs_list_pop_back(&s_ctx->free_sounds));
    inst->is_music = true;
    inst->looped = s_ctx->music_looped;
    if (!s_ctx->music_paused) inst->paused = false;
    inst->volume = volume;
    inst->pan0 = 0.5f;
    inst->pan1 = 0.5f;
    inst->audio = src;
    inst->sample_index = 0;
    cs_list_init_node(&inst->node);
    s_insert(inst);
    return inst;
}

static cs_sound_inst_t* s_inst(cs_audio_source_t* src, cs_sound_params_t params) {
    if (cs_list_empty(&s_ctx->free_sounds)) {
        s_add_page();
    }
    METAENGINE_SOUND_ASSERT(!cs_list_empty(&s_ctx->free_sounds));
    cs_sound_inst_t* inst = METAENGINE_SOUND_LIST_HOST(cs_sound_inst_t, node, cs_list_pop_back(&s_ctx->free_sounds));
    float pan = params.pan;
    if (pan > 1.0f)
        pan = 1.0f;
    else if (pan < 0.0f)
        pan = 0.0f;
    float panl = 1.0f - pan;
    float panr = pan;
    inst->is_music = false;
    inst->paused = params.paused;
    inst->looped = params.looped;
    inst->volume = params.volume;
    inst->pan0 = panl;
    inst->pan1 = panr;
    inst->audio = src;
    inst->sample_index = 0;
    cs_list_init_node(&inst->node);
    s_insert(inst);
    return inst;
}

void cs_music_play(cs_audio_source_t* audio_source, float fade_in_time) {
    if (s_ctx->music_state != METAENGINE_SOUND_MUSIC_STATE_PLAYING) {
        cs_music_stop(0);
    }

    if (fade_in_time < 0) fade_in_time = 0;
    if (fade_in_time) {
        s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_FADE_IN;
    } else {
        s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_PLAYING;
    }
    s_ctx->fade = fade_in_time;
    s_ctx->t = 0;

    METAENGINE_SOUND_ASSERT(s_ctx->music_playing == NULL);
    METAENGINE_SOUND_ASSERT(s_ctx->music_next == NULL);
    cs_sound_inst_t* inst = s_inst_music(audio_source, fade_in_time == 0 ? 1.0f : 0);
    s_ctx->music_playing = inst;
}

void cs_music_stop(float fade_out_time) {
    if (fade_out_time < 0) fade_out_time = 0;

    if (fade_out_time == 0) {
        // Immediately turn off all music if no fade out time.
        if (s_ctx->music_playing) {
            s_ctx->music_playing->active = false;
            s_ctx->music_playing->paused = false;
        }
        if (s_ctx->music_next) {
            s_ctx->music_next->active = false;
            s_ctx->music_next->paused = false;
        }
        s_ctx->music_playing = NULL;
        s_ctx->music_next = NULL;
        s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_NONE;
    } else {
        switch (s_ctx->music_state) {
            case METAENGINE_SOUND_MUSIC_STATE_NONE:
                break;

            case METAENGINE_SOUND_MUSIC_STATE_PLAYING:
                s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_FADE_OUT;
                s_ctx->fade = fade_out_time;
                s_ctx->t = 0;
                break;

            case METAENGINE_SOUND_MUSIC_STATE_FADE_OUT:
                break;

            case METAENGINE_SOUND_MUSIC_STATE_FADE_IN: {
                s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_FADE_OUT;
                s_ctx->t = s_smoothstep(((s_ctx->fade - s_ctx->t) / s_ctx->fade));
                s_ctx->fade = fade_out_time;
            } break;

            case METAENGINE_SOUND_MUSIC_STATE_SWITCH_TO_0: {
                s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_FADE_OUT;
                s_ctx->t = s_smoothstep(((s_ctx->fade - s_ctx->t) / s_ctx->fade));
                s_ctx->fade = fade_out_time;
                s_ctx->music_next = NULL;
            } break;

            case METAENGINE_SOUND_MUSIC_STATE_SWITCH_TO_1:
                // Fall-through.

            case METAENGINE_SOUND_MUSIC_STATE_CROSSFADE: {
                s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_FADE_OUT;
                s_ctx->t = s_smoothstep(((s_ctx->fade - s_ctx->t) / s_ctx->fade));
                s_ctx->fade = fade_out_time;
                s_ctx->music_playing = s_ctx->music_next;
                s_ctx->music_next = NULL;
            } break;

            case METAENGINE_SOUND_MUSIC_STATE_PAUSED:
                cs_music_stop(0);
        }
    }
}

void cs_music_set_volume(float volume_0_to_1) {
    if (volume_0_to_1 < 0) volume_0_to_1 = 0;
    s_ctx->music_volume = volume_0_to_1;
    if (s_ctx->music_playing) s_ctx->music_playing->volume = volume_0_to_1;
    if (s_ctx->music_next) s_ctx->music_next->volume = volume_0_to_1;
}

void cs_music_set_loop(bool true_to_loop) {
    s_ctx->music_looped = true_to_loop;
    if (s_ctx->music_playing) s_ctx->music_playing->looped = true_to_loop;
    if (s_ctx->music_next) s_ctx->music_next->looped = true_to_loop;
}

void cs_music_pause() {
    if (s_ctx->music_state == METAENGINE_SOUND_MUSIC_STATE_PAUSED) return;
    if (s_ctx->music_playing) s_ctx->music_playing->paused = true;
    if (s_ctx->music_next) s_ctx->music_next->paused = true;
    s_ctx->music_paused = true;
    s_ctx->music_state_to_resume_from_paused = s_ctx->music_state;
    s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_PAUSED;
}

void cs_music_resume() {
    if (s_ctx->music_state != METAENGINE_SOUND_MUSIC_STATE_PAUSED) return;
    if (s_ctx->music_playing) s_ctx->music_playing->paused = false;
    if (s_ctx->music_next) s_ctx->music_next->paused = false;
    s_ctx->music_state = s_ctx->music_state_to_resume_from_paused;
}

void cs_music_switch_to(cs_audio_source_t* audio_source, float fade_out_time, float fade_in_time) {
    if (fade_in_time < 0) fade_in_time = 0;
    if (fade_out_time < 0) fade_out_time = 0;

    switch (s_ctx->music_state) {
        case METAENGINE_SOUND_MUSIC_STATE_NONE:
            cs_music_play(audio_source, fade_in_time);
            break;

        case METAENGINE_SOUND_MUSIC_STATE_PLAYING: {
            METAENGINE_SOUND_ASSERT(s_ctx->music_next == NULL);
            cs_sound_inst_t* inst = s_inst_music(audio_source, fade_in_time == 0 ? 1.0f : 0);
            s_ctx->music_next = inst;

            s_ctx->fade = fade_out_time;
            s_ctx->fade_switch_1 = fade_in_time;
            s_ctx->t = 0;
            s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_SWITCH_TO_0;
        } break;

        case METAENGINE_SOUND_MUSIC_STATE_FADE_OUT: {
            METAENGINE_SOUND_ASSERT(s_ctx->music_next == NULL);
            cs_sound_inst_t* inst = s_inst_music(audio_source, fade_in_time == 0 ? 1.0f : 0);
            s_ctx->music_next = inst;

            s_ctx->fade_switch_1 = fade_in_time;
            s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_SWITCH_TO_0;
        } break;

        case METAENGINE_SOUND_MUSIC_STATE_FADE_IN: {
            METAENGINE_SOUND_ASSERT(s_ctx->music_next == NULL);
            cs_sound_inst_t* inst = s_inst_music(audio_source, fade_in_time == 0 ? 1.0f : 0);
            s_ctx->music_next = inst;

            s_ctx->fade_switch_1 = fade_in_time;
            s_ctx->t = s_smoothstep(((s_ctx->fade - s_ctx->t) / s_ctx->fade));
            s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_SWITCH_TO_0;
        } break;

        case METAENGINE_SOUND_MUSIC_STATE_SWITCH_TO_0: {
            METAENGINE_SOUND_ASSERT(s_ctx->music_next != NULL);
            cs_sound_inst_t* inst = s_inst_music(audio_source, fade_in_time == 0 ? 1.0f : 0);
            s_ctx->music_next->active = false;
            s_ctx->music_next = inst;
            s_ctx->fade_switch_1 = fade_in_time;
        } break;

        case METAENGINE_SOUND_MUSIC_STATE_CROSSFADE:  // Fall-through.
        case METAENGINE_SOUND_MUSIC_STATE_SWITCH_TO_1: {
            METAENGINE_SOUND_ASSERT(s_ctx->music_next != NULL);
            cs_sound_inst_t* inst = s_inst_music(audio_source, fade_in_time == 0 ? 1.0f : 0);
            s_ctx->music_playing = s_ctx->music_next;
            s_ctx->music_next = inst;

            s_ctx->t = s_smoothstep(((s_ctx->fade - s_ctx->t) / s_ctx->fade));
            s_ctx->fade_switch_1 = fade_in_time;
            s_ctx->fade = fade_out_time;
            s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_SWITCH_TO_0;
        } break;

        case METAENGINE_SOUND_MUSIC_STATE_PAUSED:
            cs_music_stop(0);
            cs_music_switch_to(audio_source, fade_out_time, fade_in_time);
            break;
    }
}

void cs_music_crossfade(cs_audio_source_t* audio_source, float cross_fade_time) {
    if (cross_fade_time < 0) cross_fade_time = 0;

    switch (s_ctx->music_state) {
        case METAENGINE_SOUND_MUSIC_STATE_NONE:
            cs_music_play(audio_source, cross_fade_time);

        case METAENGINE_SOUND_MUSIC_STATE_PLAYING: {
            METAENGINE_SOUND_ASSERT(s_ctx->music_next == NULL);
            s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_CROSSFADE;

            cs_sound_inst_t* inst = s_inst_music(audio_source, cross_fade_time == 0 ? 1.0f : 0);
            inst->paused = false;
            s_ctx->music_next = inst;

            s_ctx->fade = cross_fade_time;
            s_ctx->t = 0;
        } break;

        case METAENGINE_SOUND_MUSIC_STATE_FADE_OUT:
            METAENGINE_SOUND_ASSERT(s_ctx->music_next == NULL);
            // Fall-through.

        case METAENGINE_SOUND_MUSIC_STATE_FADE_IN: {
            s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_CROSSFADE;

            cs_sound_inst_t* inst = s_inst_music(audio_source, cross_fade_time == 0 ? 1.0f : 0);
            inst->paused = false;
            s_ctx->music_next = inst;

            s_ctx->fade = cross_fade_time;
        } break;

        case METAENGINE_SOUND_MUSIC_STATE_SWITCH_TO_0: {
            s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_CROSSFADE;
            s_ctx->music_next->active = false;

            cs_sound_inst_t* inst = s_inst_music(audio_source, cross_fade_time == 0 ? 1.0f : 0);
            inst->paused = false;
            s_ctx->music_next = inst;

            s_ctx->fade = cross_fade_time;
        } break;

        case METAENGINE_SOUND_MUSIC_STATE_SWITCH_TO_1:  // Fall-through.
        case METAENGINE_SOUND_MUSIC_STATE_CROSSFADE: {
            s_ctx->music_state = METAENGINE_SOUND_MUSIC_STATE_CROSSFADE;
            s_ctx->music_playing->active = false;
            s_ctx->music_playing = s_ctx->music_next;

            cs_sound_inst_t* inst = s_inst_music(audio_source, cross_fade_time == 0 ? 1.0f : 0);
            inst->paused = false;
            s_ctx->music_next = inst;

            s_ctx->fade = cross_fade_time;
        } break;

        case METAENGINE_SOUND_MUSIC_STATE_PAUSED:
            cs_music_stop(0);
            cs_music_crossfade(audio_source, cross_fade_time);
    }
}

uint64_t cs_music_get_sample_index() {
    if (s_ctx->music_playing)
        return 0;
    else
        return s_ctx->music_playing->sample_index;
}

cs_error_t cs_music_set_sample_index(uint64_t sample_index) {
    if (s_ctx->music_playing) return METAENGINE_SOUND_ERROR_INVALID_SOUND;
    if (sample_index > s_ctx->music_playing->audio->sample_count) return METAENGINE_SOUND_ERROR_TRIED_TO_SET_SAMPLE_INDEX_BEYOND_THE_AUDIO_SOURCES_SAMPLE_COUNT;
    s_ctx->music_playing->sample_index = sample_index;
    return METAENGINE_SOUND_ERROR_NONE;
}

// -------------------------------------------------------------------------------------------------
// Playing sounds.

cs_sound_params_t cs_sound_params_default() {
    cs_sound_params_t params;
    params.paused = false;
    params.looped = false;
    params.volume = 1.0f;
    params.pan = 0.5f;
    params.delay = 0.0f;
    return params;
}

static cs_sound_inst_t* s_get_inst(cs_playing_sound_t sound) { return (cs_sound_inst_t*)hashtable_find(&s_ctx->instance_map, sound.id); }

cs_playing_sound_t cs_play_sound(cs_audio_source_t* audio, cs_sound_params_t params) {
    cs_sound_inst_t* inst = s_inst(audio, params);
    cs_playing_sound_t sound = {inst->id};
    return sound;
}

bool cs_sound_is_active(cs_playing_sound_t sound) {
    cs_sound_inst_t* inst = s_get_inst(sound);
    if (!inst) return false;
    return inst->active;
}

bool cs_sound_get_is_paused(cs_playing_sound_t sound) {
    cs_sound_inst_t* inst = s_get_inst(sound);
    if (!inst) return false;
    return inst->paused;
}

bool cs_sound_get_is_looped(cs_playing_sound_t sound) {
    cs_sound_inst_t* inst = s_get_inst(sound);
    if (!inst) return false;
    return inst->looped;
}

float cs_sound_get_volume(cs_playing_sound_t sound) {
    cs_sound_inst_t* inst = s_get_inst(sound);
    if (!inst) return 0;
    return inst->volume;
}

uint64_t cs_sound_get_sample_index(cs_playing_sound_t sound) {
    cs_sound_inst_t* inst = s_get_inst(sound);
    if (!inst) return 0;
    return inst->sample_index;
}

void cs_sound_set_is_paused(cs_playing_sound_t sound, bool true_for_paused) {
    cs_sound_inst_t* inst = s_get_inst(sound);
    if (!inst) return;
    inst->paused = true_for_paused;
}

void cs_sound_set_is_looped(cs_playing_sound_t sound, bool true_for_looped) {
    cs_sound_inst_t* inst = s_get_inst(sound);
    if (!inst) return;
    inst->looped = true_for_looped;
}

void cs_sound_set_volume(cs_playing_sound_t sound, float volume_0_to_1) {
    if (volume_0_to_1 < 0) volume_0_to_1 = 0;
    cs_sound_inst_t* inst = s_get_inst(sound);
    if (!inst) return;
    inst->volume = volume_0_to_1;
}

cs_error_t cs_sound_set_sample_index(cs_playing_sound_t sound, uint64_t sample_index) {
    cs_sound_inst_t* inst = s_get_inst(sound);
    if (!inst) return METAENGINE_SOUND_ERROR_INVALID_SOUND;
    if (sample_index > inst->audio->sample_count) return METAENGINE_SOUND_ERROR_TRIED_TO_SET_SAMPLE_INDEX_BEYOND_THE_AUDIO_SOURCES_SAMPLE_COUNT;
    inst->sample_index = sample_index;
    return METAENGINE_SOUND_ERROR_NONE;
}

void cs_set_playing_sounds_volume(float volume_0_to_1) {
    if (volume_0_to_1 < 0) volume_0_to_1 = 0;
    s_ctx->sound_volume = volume_0_to_1;
}

void cs_stop_all_playing_sounds() {
    cs_lock();

    // Set all playing sounds (that aren't music) active to false.
    if (cs_list_empty(&s_ctx->playing_sounds)) return;
    cs_list_node_t* playing_sound = cs_list_begin(&s_ctx->playing_sounds);
    cs_list_node_t* end = cs_list_end(&s_ctx->playing_sounds);

    do {
        cs_sound_inst_t* inst = METAENGINE_SOUND_LIST_HOST(cs_sound_inst_t, node, playing_sound);
        cs_list_node_t* next = playing_sound->next;
        if (inst != s_ctx->music_playing && inst != s_ctx->music_next) {
            inst->active = false;  // Let cs_mix handle cleaning this up.
        }
        playing_sound = next;
    } while (playing_sound != end);

    cs_unlock();
}