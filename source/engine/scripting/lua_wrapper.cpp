
#include "lua_wrapper.hpp"

#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "engine/core/core.hpp"
#include "engine/core/io/filesystem.h"

#pragma region lua_safe_alloc

void *lua_simple_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    (void)ud;
    (void)osize; /* not used */
    if (nsize == 0) {
        free(ptr);
        return NULL;
    } else
        return realloc(ptr, nsize);
}

struct lua_allocator *new_allocator(void) {
    struct lua_allocator *alloc = (struct lua_allocator *)malloc(sizeof(struct lua_allocator));
    alloc->blocks = (struct lua_mem_block *)malloc(sizeof(struct lua_mem_block) * 4);
    alloc->nb_blocks = 0;
    alloc->size_blocks = 4;
    alloc->total_allocated = 0;
#ifdef _DEBUG_ALLOC
    fprintf(stderr, "Created allocator %p\n", alloc);
#endif
    return alloc;
}

void delete_allocator(struct lua_allocator *alloc) {
    size_t blk;
#ifdef _DEBUG_ALLOC
    fprintf(stderr, "Deleting allocator %p\n", alloc);
#endif
    for (blk = 0; blk < alloc->nb_blocks; ++blk) free(alloc->blocks[blk].ptr);
    free(alloc->blocks);
    free(alloc);
}

void *lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    struct lua_allocator *alloc = (struct lua_allocator *)ud;
    size_t blk;
#ifdef _DEBUG_ALLOC
    {
        size_t i;
        for (i = 0; i < alloc->nb_blocks; ++i) {
            fprintf(stderr, "%p %u ", alloc->blocks[i].ptr, alloc->blocks[i].size);
            if (i % 4 == 0) fprintf(stderr, "\n");
        }
        if (i % 4 != 1)
            fprintf(stderr, "\n\n");
        else
            fprintf(stderr, "\n");
    }
    fprintf(stderr,
            "Lua request on allocator %p: "
            "ptr=%p, osize=%u, nsize=%u\n",
            alloc, ptr, osize, nsize);
#endif
    if (ptr == NULL) {
        if (nsize == 0) {
#ifdef _DEBUG_ALLOC
            fprintf(stderr, "Returning NULL...\n");
#endif
            return NULL;
        }
        blk = alloc->nb_blocks;
        alloc->nb_blocks++;
        if (alloc->nb_blocks > alloc->size_blocks) {
            alloc->size_blocks *= 2;
            alloc->blocks = (struct lua_mem_block *)realloc(alloc->blocks, sizeof(struct lua_mem_block) * alloc->size_blocks);
#ifdef _DEBUG_ALLOC
            fprintf(stderr, "Growing block table to %u blocks\n", alloc->size_blocks);
#endif
        }
        alloc->blocks[blk].ptr = NULL;
        alloc->blocks[blk].size = 0;
#ifdef _DEBUG_ALLOC
        fprintf(stderr, "Allocated new block %u\n", blk);
#endif
    } else {
        /* Bisect to the block */
        size_t low = 0, high = alloc->nb_blocks;
        while (low < high) {
            size_t mid = (low + high) / 2;
            if (alloc->blocks[mid].ptr < ptr)
                low = mid + 1;
            else
                high = mid;
        }
        blk = low;
#ifdef _DEBUG_ALLOC
        fprintf(stderr, "Found block %u\n", blk);
#endif
    }
    assert(alloc->blocks[blk].ptr == ptr && (!ptr || alloc->blocks[blk].size == osize));
    if (nsize == 0) {
        free(ptr);
        alloc->total_allocated -= osize;
        memmove(&alloc->blocks[blk], &alloc->blocks[blk + 1], sizeof(struct lua_mem_block) * (alloc->nb_blocks - blk - 1));
        alloc->nb_blocks--;
#ifdef _DEBUG_ALLOC
        fprintf(stderr, "Removed block, now have %u blocks, %u bytes\n\n", alloc->nb_blocks, alloc->total_allocated);
#endif
        return NULL;
    } else {
        ptr = realloc(ptr, nsize);
#ifdef _DEBUG_ALLOC
        fprintf(stderr, "ptr=%p ", ptr);
#endif
        alloc->blocks[blk].ptr = ptr;
        alloc->total_allocated += nsize - alloc->blocks[blk].size;
        alloc->blocks[blk].size = nsize;
        while (blk > 0 && alloc->blocks[blk].ptr < alloc->blocks[blk - 1].ptr) {
            struct lua_mem_block tmp = alloc->blocks[blk];
            alloc->blocks[blk] = alloc->blocks[blk - 1];
            alloc->blocks[blk - 1] = tmp;
            blk--;
#ifdef _DEBUG_ALLOC
            fprintf(stderr, "< ");
#endif
        }
        while (blk + 1 < alloc->nb_blocks && alloc->blocks[blk].ptr > alloc->blocks[blk + 1].ptr) {
            struct lua_mem_block tmp = alloc->blocks[blk];
            alloc->blocks[blk] = alloc->blocks[blk + 1];
            alloc->blocks[blk + 1] = tmp;
            blk++;
#ifdef _DEBUG_ALLOC
            fprintf(stderr, "> ");
#endif
        }
#ifdef _DEBUG_ALLOC
        fprintf(stderr, "\n");
        fprintf(stderr, "Moved block to %u, now have %u bytes\n\n", blk, alloc->total_allocated);
#endif
        return ptr;
    }
}

#pragma endregion lua_safe_alloc

#pragma region LuaA

void luaA_open(lua_State *L) {

    lua_pushinteger(L, 0);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_index");
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_ids");
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_names");
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_sizes");

    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "stack_push");
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "stack_to");

    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs");
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs_offset");
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums");
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums_sizes");
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums_values");
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "functions");

    lua_newuserdata(L, LUAA_RETURN_STACK_SIZE);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "call_ret_stk");
    lua_newuserdata(L, LUAA_ARGUMENT_STACK_SIZE);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "call_arg_stk");
    lua_pushinteger(L, 0);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "call_ret_ptr");
    lua_pushinteger(L, 0);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "call_arg_ptr");

    // compiler does weird macro expansion with "bool" so no magic macro for you
    luaA_conversion_type(L, luaA_type_add(L, "bool", sizeof(bool)), luaA_push_bool, luaA_to_bool);
    luaA_conversion_type(L, luaA_type_add(L, "_Bool", sizeof(bool)), luaA_push_bool, luaA_to_bool);
    luaA_conversion(L, char, luaA_push_char, luaA_to_char);
    luaA_conversion(L, signed char, luaA_push_signed_char, luaA_to_signed_char);
    luaA_conversion(L, unsigned char, luaA_push_unsigned_char, luaA_to_unsigned_char);
    luaA_conversion(L, short, luaA_push_short, luaA_to_short);
    luaA_conversion(L, unsigned short, luaA_push_unsigned_short, luaA_to_unsigned_short);
    luaA_conversion(L, int, luaA_push_int, luaA_to_int);
    luaA_conversion(L, unsigned int, luaA_push_unsigned_int, luaA_to_unsigned_int);
    luaA_conversion(L, long, luaA_push_long, luaA_to_long);
    luaA_conversion(L, unsigned long, luaA_push_unsigned_long, luaA_to_unsigned_long);
    luaA_conversion(L, long long, luaA_push_long_long, luaA_to_long_long);
    luaA_conversion(L, unsigned long long, luaA_push_unsigned_long_long, luaA_to_unsigned_long_long);
    luaA_conversion(L, float, luaA_push_float, luaA_to_float);
    luaA_conversion(L, double, luaA_push_double, luaA_to_double);
    luaA_conversion(L, long double, luaA_push_long_double, luaA_to_long_double);

    luaA_conversion_push_type(L, luaA_type_add(L, "const bool", sizeof(bool)), luaA_push_bool);
    luaA_conversion_push_type(L, luaA_type_add(L, "const _Bool", sizeof(bool)), luaA_push_bool);
    luaA_conversion_push(L, const char, luaA_push_char);
    luaA_conversion_push(L, const signed char, luaA_push_signed_char);
    luaA_conversion_push(L, const unsigned char, luaA_push_unsigned_char);
    luaA_conversion_push(L, const short, luaA_push_short);
    luaA_conversion_push(L, const unsigned short, luaA_push_unsigned_short);
    luaA_conversion_push(L, const int, luaA_push_int);
    luaA_conversion_push(L, const unsigned int, luaA_push_unsigned_int);
    luaA_conversion_push(L, const long, luaA_push_long);
    luaA_conversion_push(L, const unsigned long, luaA_push_unsigned_long);
    luaA_conversion_push(L, const long long, luaA_push_long_long);
    luaA_conversion_push(L, const unsigned long long, luaA_push_unsigned_long_long);
    luaA_conversion_push(L, const float, luaA_push_float);
    luaA_conversion_push(L, const double, luaA_push_double);
    luaA_conversion_push(L, const long double, luaA_push_long_double);

    luaA_conversion(L, char *, luaA_push_char_ptr, luaA_to_char_ptr);
    luaA_conversion(L, const char *, luaA_push_const_char_ptr, luaA_to_const_char_ptr);
    luaA_conversion(L, void *, luaA_push_void_ptr, luaA_to_void_ptr);

    luaA_conversion_push_type(L, luaA_type_add(L, "void", 1), luaA_push_void);  // sizeof(void) is 1 on gcc
}

void luaA_close(lua_State *L) {

    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_index");
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_ids");
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_names");
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_sizes");

    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "stack_push");
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "stack_to");

    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs");
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs_offset");
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums");
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums_sizes");
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums_values");
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "functions");

    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "call_ret_stk");
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "call_arg_stk");
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "call_ret_ptr");
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "call_arg_ptr");
}

/*
** Types
*/

luaA_Type luaA_type_add(lua_State *L, const char *type, size_t size) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_ids");
    lua_getfield(L, -1, type);

    if (lua_isnumber(L, -1)) {

        luaA_Type id = lua_tointeger(L, -1);
        lua_pop(L, 2);
        return id;

    } else {

        lua_pop(L, 2);

        lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_index");

        luaA_Type id = lua_tointeger(L, -1);
        lua_pop(L, 1);
        id++;

        lua_pushinteger(L, id);
        lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_index");

        lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_ids");
        lua_pushinteger(L, id);
        lua_setfield(L, -2, type);
        lua_pop(L, 1);

        lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_names");
        lua_pushinteger(L, id);
        lua_pushstring(L, type);
        lua_settable(L, -3);
        lua_pop(L, 1);

        lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_sizes");
        lua_pushinteger(L, id);
        lua_pushinteger(L, size);
        lua_settable(L, -3);
        lua_pop(L, 1);

        return id;
    }
}

luaA_Type luaA_type_find(lua_State *L, const char *type) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_ids");
    lua_getfield(L, -1, type);

    luaA_Type id = lua_isnil(L, -1) ? LUAA_INVALID_TYPE : lua_tointeger(L, -1);
    lua_pop(L, 2);

    return id;
}

const char *luaA_typename(lua_State *L, luaA_Type id) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_names");
    lua_pushinteger(L, id);
    lua_gettable(L, -2);

    const char *type = lua_isnil(L, -1) ? "LUAA_INVALID_TYPE" : lua_tostring(L, -1);
    lua_pop(L, 2);

    return type;
}

size_t luaA_typesize(lua_State *L, luaA_Type id) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "type_sizes");
    lua_pushinteger(L, id);
    lua_gettable(L, -2);

    size_t size = lua_isnil(L, -1) ? -1 : lua_tointeger(L, -1);
    lua_pop(L, 2);

    return size;
}

/*
** Stack
*/

int luaA_push_type(lua_State *L, luaA_Type type_id, const void *c_in) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "stack_push");
    lua_pushinteger(L, type_id);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {
        luaA_Pushfunc func = (luaA_Pushfunc)lua_touserdata(L, -1);
        lua_pop(L, 2);
        return func(L, type_id, c_in);
    }

    lua_pop(L, 2);

    if (luaA_struct_registered_type(L, type_id)) {
        return luaA_struct_push_type(L, type_id, c_in);
    }

    if (luaA_enum_registered_type(L, type_id)) {
        return luaA_enum_push_type(L, type_id, c_in);
    }

    lua_pushfstring(L, "luaA_push: conversion to Lua object from type '%s' not registered!", luaA_typename(L, type_id));
    lua_error(L);
    return 0;
}

void luaA_to_type(lua_State *L, luaA_Type type_id, void *c_out, int index) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "stack_to");
    lua_pushinteger(L, type_id);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {
        luaA_Tofunc func = (luaA_Tofunc)lua_touserdata(L, -1);
        lua_pop(L, 2);
        func(L, type_id, c_out, index);
        return;
    }

    lua_pop(L, 2);

    if (luaA_struct_registered_type(L, type_id)) {
        luaA_struct_to_type(L, type_id, c_out, index);
        return;
    }

    if (luaA_enum_registered_type(L, type_id)) {
        luaA_enum_to_type(L, type_id, c_out, index);
        return;
    }

    lua_pushfstring(L, "luaA_to: conversion from Lua object to type '%s' not registered!", luaA_typename(L, type_id));
    lua_error(L);
}

void luaA_conversion_type(lua_State *L, luaA_Type type_id, luaA_Pushfunc push_func, luaA_Tofunc to_func) {
    luaA_conversion_push_type(L, type_id, push_func);
    luaA_conversion_to_type(L, type_id, to_func);
}

void luaA_conversion_push_type(lua_State *L, luaA_Type type_id, luaA_Pushfunc func) {
    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "stack_push");
    lua_pushinteger(L, type_id);
    lua_pushlightuserdata(L, func);
    lua_settable(L, -3);
    lua_pop(L, 1);
}

void luaA_conversion_to_type(lua_State *L, luaA_Type type_id, luaA_Tofunc func) {
    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "stack_to");
    lua_pushinteger(L, type_id);
    lua_pushlightuserdata(L, func);
    lua_settable(L, -3);
    lua_pop(L, 1);
}

int luaA_push_bool(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushboolean(L, *(bool *)c_in);
    return 1;
}

void luaA_to_bool(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(bool *)c_out = lua_toboolean(L, index); }

int luaA_push_char(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushinteger(L, *(char *)c_in);
    return 1;
}

void luaA_to_char(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(char *)c_out = lua_tointeger(L, index); }

int luaA_push_signed_char(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushinteger(L, *(signed char *)c_in);
    return 1;
}

void luaA_to_signed_char(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(signed char *)c_out = lua_tointeger(L, index); }

int luaA_push_unsigned_char(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushinteger(L, *(unsigned char *)c_in);
    return 1;
}

void luaA_to_unsigned_char(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(unsigned char *)c_out = lua_tointeger(L, index); }

int luaA_push_short(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushinteger(L, *(short *)c_in);
    return 1;
}

void luaA_to_short(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(short *)c_out = lua_tointeger(L, index); }

int luaA_push_unsigned_short(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushinteger(L, *(unsigned short *)c_in);
    return 1;
}

void luaA_to_unsigned_short(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(unsigned short *)c_out = lua_tointeger(L, index); }

int luaA_push_int(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushinteger(L, *(int *)c_in);
    return 1;
}

void luaA_to_int(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(int *)c_out = lua_tointeger(L, index); }

int luaA_push_unsigned_int(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushinteger(L, *(unsigned int *)c_in);
    return 1;
}

void luaA_to_unsigned_int(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(unsigned int *)c_out = lua_tointeger(L, index); }

int luaA_push_long(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushinteger(L, *(long *)c_in);
    return 1;
}

void luaA_to_long(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(long *)c_out = lua_tointeger(L, index); }

int luaA_push_unsigned_long(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushinteger(L, *(unsigned long *)c_in);
    return 1;
}

void luaA_to_unsigned_long(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(unsigned long *)c_out = lua_tointeger(L, index); }

int luaA_push_long_long(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushinteger(L, *(long long *)c_in);
    return 1;
}

void luaA_to_long_long(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(long long *)c_out = lua_tointeger(L, index); }

int luaA_push_unsigned_long_long(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushinteger(L, *(unsigned long long *)c_in);
    return 1;
}

void luaA_to_unsigned_long_long(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(unsigned long long *)c_out = lua_tointeger(L, index); }

int luaA_push_float(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushnumber(L, *(float *)c_in);
    return 1;
}

void luaA_to_float(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(float *)c_out = lua_tonumber(L, index); }

int luaA_push_double(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushnumber(L, *(double *)c_in);
    return 1;
}

void luaA_to_double(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(double *)c_out = lua_tonumber(L, index); }

int luaA_push_long_double(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushnumber(L, *(long double *)c_in);
    return 1;
}

void luaA_to_long_double(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(long double *)c_out = lua_tonumber(L, index); }

int luaA_push_char_ptr(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushstring(L, *(char **)c_in);
    return 1;
}

void luaA_to_char_ptr(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(char **)c_out = (char *)lua_tostring(L, index); }

int luaA_push_const_char_ptr(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushstring(L, *(const char **)c_in);
    return 1;
}

void luaA_to_const_char_ptr(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(const char **)c_out = lua_tostring(L, index); }

int luaA_push_void_ptr(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushlightuserdata(L, *(void **)c_in);
    return 1;
}

void luaA_to_void_ptr(lua_State *L, luaA_Type type_id, void *c_out, int index) { *(void **)c_out = (void *)lua_touserdata(L, index); }

int luaA_push_void(lua_State *L, luaA_Type type_id, const void *c_in) {
    lua_pushnil(L);
    return 1;
}

bool luaA_conversion_registered_type(lua_State *L, luaA_Type type_id) { return (luaA_conversion_push_registered_type(L, type_id) && luaA_conversion_to_registered_type(L, type_id)); }

bool luaA_conversion_push_registered_type(lua_State *L, luaA_Type type_id) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "stack_push");
    lua_pushinteger(L, type_id);
    lua_gettable(L, -2);

    bool reg = !lua_isnil(L, -1);
    lua_pop(L, 2);

    return reg;
}

bool luaA_conversion_to_registered_type(lua_State *L, luaA_Type type_id) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "stack_to");
    lua_pushinteger(L, type_id);
    lua_gettable(L, -2);

    bool reg = !lua_isnil(L, -1);
    lua_pop(L, 2);

    return reg;
}

/*
** Structs
*/

int luaA_struct_push_member_offset_type(lua_State *L, luaA_Type type, size_t offset, const void *c_in) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs_offset");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {

        lua_pushinteger(L, offset);
        lua_gettable(L, -2);

        if (!lua_isnil(L, -1)) {
            lua_getfield(L, -1, "type");
            luaA_Type stype = lua_tointeger(L, -1);
            lua_pop(L, 4);
            return luaA_push_type(L, stype, (char *)c_in + offset);
        }

        lua_pop(L, 3);
        lua_pushfstring(L, "luaA_struct_push_member: Member offset '%d' not registered for struct '%s'!", offset, luaA_typename(L, type));
        lua_error(L);
    }

    lua_pop(L, 2);
    lua_pushfstring(L, "luaA_struct_push_member: Struct '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
    return 0;
}

int luaA_struct_push_member_name_type(lua_State *L, luaA_Type type, const char *member, const void *c_in) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {

        lua_getfield(L, -1, member);

        if (!lua_isnil(L, -1)) {
            lua_getfield(L, -1, "type");
            luaA_Type stype = lua_tointeger(L, -1);
            lua_pop(L, 1);
            lua_getfield(L, -1, "offset");
            size_t offset = lua_tointeger(L, -1);
            lua_pop(L, 4);
            return luaA_push_type(L, stype, (char *)c_in + offset);
        }

        lua_pop(L, 3);
        lua_pushfstring(L, "luaA_struct_push_member: Member name '%s' not registered for struct '%s'!", member, luaA_typename(L, type));
        lua_error(L);
    }

    lua_pop(L, 2);
    lua_pushfstring(L, "luaA_struct_push_member: Struct '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
    return 0;
}

void luaA_struct_to_member_offset_type(lua_State *L, luaA_Type type, size_t offset, void *c_out, int index) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs_offset");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {

        lua_pushinteger(L, offset);
        lua_gettable(L, -2);

        if (!lua_isnil(L, -1)) {
            lua_getfield(L, -1, "type");
            luaA_Type stype = lua_tointeger(L, -1);
            lua_pop(L, 4);
            luaA_to_type(L, stype, (char *)c_out + offset, index);
            return;
        }

        lua_pop(L, 3);
        lua_pushfstring(L, "luaA_struct_to_member: Member offset '%d' not registered for struct '%s'!", offset, luaA_typename(L, type));
        lua_error(L);
    }

    lua_pop(L, 2);
    lua_pushfstring(L, "luaA_struct_to_member: Struct '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
}

void luaA_struct_to_member_name_type(lua_State *L, luaA_Type type, const char *member, void *c_out, int index) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {

        lua_pushstring(L, member);
        lua_gettable(L, -2);

        if (!lua_isnil(L, -1)) {
            lua_getfield(L, -1, "type");
            luaA_Type stype = lua_tointeger(L, -1);
            lua_pop(L, 1);
            lua_getfield(L, -1, "offset");
            size_t offset = lua_tointeger(L, -1);
            lua_pop(L, 4);
            luaA_to_type(L, stype, (char *)c_out + offset, index);
            return;
        }

        lua_pop(L, 3);
        lua_pushfstring(L, "luaA_struct_to_member: Member name '%s' not registered for struct '%s'!", member, luaA_typename(L, type));
        lua_error(L);
    }

    lua_pop(L, 2);
    lua_pushfstring(L, "luaA_struct_to_member: Struct '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
}

bool luaA_struct_has_member_offset_type(lua_State *L, luaA_Type type, size_t offset) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs_offset");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {

        lua_pushinteger(L, offset);
        lua_gettable(L, -2);

        if (!lua_isnil(L, -1)) {
            lua_pop(L, 3);
            return true;
        }

        lua_pop(L, 3);
        return false;
    }

    lua_pop(L, 2);
    lua_pushfstring(L, "luaA_struct_has_member: Struct '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
    return false;
}

bool luaA_struct_has_member_name_type(lua_State *L, luaA_Type type, const char *member) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {

        lua_pushstring(L, member);
        lua_gettable(L, -2);

        if (!lua_isnil(L, -1)) {
            lua_pop(L, 3);
            return true;
        }

        lua_pop(L, 3);
        return false;
    }

    lua_pop(L, 2);
    lua_pushfstring(L, "luaA_struct_has_member: Struct '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
    return false;
}

luaA_Type luaA_struct_typeof_member_offset_type(lua_State *L, luaA_Type type, size_t offset) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs_offset");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {

        lua_pushinteger(L, offset);
        lua_gettable(L, -2);

        if (!lua_isnil(L, -1)) {
            lua_getfield(L, -1, "type");
            luaA_Type stype = lua_tointeger(L, -1);
            lua_pop(L, 4);
            return stype;
        }

        lua_pop(L, 3);
        lua_pushfstring(L, "luaA_struct_typeof_member: Member offset '%d' not registered for struct '%s'!", offset, luaA_typename(L, type));
        lua_error(L);
    }

    lua_pop(L, 2);
    lua_pushfstring(L, "luaA_struct_typeof_member: Struct '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
    return 0;
}

luaA_Type luaA_struct_typeof_member_name_type(lua_State *L, luaA_Type type, const char *member) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {

        lua_pushstring(L, member);
        lua_gettable(L, -2);

        if (!lua_isnil(L, -1)) {
            lua_getfield(L, -1, "type");
            luaA_Type type = lua_tointeger(L, -1);
            lua_pop(L, 4);
            return type;
        }

        lua_pop(L, 3);
        lua_pushfstring(L, "luaA_struct_typeof_member: Member name '%s' not registered for struct '%s'!", member, luaA_typename(L, type));
        lua_error(L);
    }

    lua_pop(L, 2);
    lua_pushfstring(L, "luaA_struct_typeof_member: Struct '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
    return 0;
}

void luaA_struct_type(lua_State *L, luaA_Type type) {
    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs");
    lua_pushinteger(L, type);
    lua_newtable(L);
    lua_settable(L, -3);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs_offset");
    lua_pushinteger(L, type);
    lua_newtable(L);
    lua_settable(L, -3);
    lua_pop(L, 1);
}

void luaA_struct_member_type(lua_State *L, luaA_Type type, const char *member, luaA_Type mtype, size_t offset) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {

        lua_newtable(L);

        lua_pushinteger(L, mtype);
        lua_setfield(L, -2, "type");
        lua_pushinteger(L, offset);
        lua_setfield(L, -2, "offset");
        lua_pushstring(L, member);
        lua_setfield(L, -2, "name");

        lua_setfield(L, -2, member);

        lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs_offset");
        lua_pushinteger(L, type);
        lua_gettable(L, -2);

        lua_pushinteger(L, offset);
        lua_getfield(L, -4, member);
        lua_settable(L, -3);
        lua_pop(L, 4);
        return;
    }

    lua_pop(L, 2);
    lua_pushfstring(L, "luaA_struct_member: Struct '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
}

bool luaA_struct_registered_type(lua_State *L, luaA_Type type) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    bool reg = !lua_isnil(L, -1);
    lua_pop(L, 2);

    return reg;
}

int luaA_struct_push_type(lua_State *L, luaA_Type type, const void *c_in) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {
        lua_remove(L, -2);
        lua_newtable(L);

        lua_pushnil(L);
        while (lua_next(L, -3)) {

            if (lua_type(L, -2) == LUA_TSTRING) {
                lua_getfield(L, -1, "name");
                const char *name = lua_tostring(L, -1);
                lua_pop(L, 1);
                int num = luaA_struct_push_member_name_type(L, type, name, c_in);
                if (num > 1) {
                    lua_pop(L, 5);
                    lua_pushfstring(L,
                                    "luaA_struct_push: Conversion pushed %d values to stack,"
                                    " don't know how to include in struct!",
                                    num);
                    lua_error(L);
                }
                lua_remove(L, -2);
                lua_pushvalue(L, -2);
                lua_insert(L, -2);
                lua_settable(L, -4);
            } else {
                lua_pop(L, 1);
            }
        }

        lua_remove(L, -2);
        return 1;
    }

    lua_pop(L, 2);
    lua_pushfstring(L, "lua_struct_push: Struct '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
    return 0;
}

void luaA_struct_to_type(lua_State *L, luaA_Type type, void *c_out, int index) {

    lua_pushnil(L);
    while (lua_next(L, index - 1)) {

        if (lua_type(L, -2) == LUA_TSTRING) {
            luaA_struct_to_member_name_type(L, type, lua_tostring(L, -2), c_out, -1);
        }

        lua_pop(L, 1);
    }
}

const char *luaA_struct_next_member_name_type(lua_State *L, luaA_Type type, const char *member) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "structs");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {

        if (!member) {
            lua_pushnil(L);
        } else {
            lua_pushstring(L, member);
        }
        if (!lua_next(L, -2)) {
            lua_pop(L, 2);
            return LUAA_INVALID_MEMBER_NAME;
        }
        const char *result = lua_tostring(L, -2);
        lua_pop(L, 4);
        return result;
    }

    lua_pop(L, 2);
    lua_pushfstring(L, "luaA_struct_next_member: Struct '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
    return NULL;
}

/*
** Enums
*/

int luaA_enum_push_type(lua_State *L, luaA_Type type, const void *value) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums_values");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {

        lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums_sizes");
        lua_pushinteger(L, type);
        lua_gettable(L, -2);
        size_t size = lua_tointeger(L, -1);
        lua_pop(L, 2);

        lua_Integer lvalue = 0;
        memcpy(&lvalue, value, size);

        lua_pushinteger(L, lvalue);
        lua_gettable(L, -2);

        if (!lua_isnil(L, -1)) {
            lua_getfield(L, -1, "name");
            lua_remove(L, -2);
            lua_remove(L, -2);
            lua_remove(L, -2);
            return 1;
        }

        lua_pop(L, 3);
        lua_pushfstring(L, "luaA_enum_push: Enum '%s' value %d not registered!", luaA_typename(L, type), lvalue);
        lua_error(L);
        return 0;
    }

    lua_pop(L, 2);
    lua_pushfstring(L, "luaA_enum_push: Enum '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
    return 0;
}

void luaA_enum_to_type(lua_State *L, luaA_Type type, void *c_out, int index) {

    const char *name = lua_tostring(L, index);

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {

        lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums_sizes");
        lua_pushinteger(L, type);
        lua_gettable(L, -2);
        size_t size = lua_tointeger(L, -1);
        lua_pop(L, 2);

        lua_pushstring(L, name);
        lua_gettable(L, -2);

        if (!lua_isnil(L, -1)) {
            lua_getfield(L, -1, "value");
            lua_Integer value = lua_tointeger(L, -1);
            lua_pop(L, 4);
            memcpy(c_out, &value, size);
            return;
        }

        lua_pop(L, 3);
        lua_pushfstring(L, "luaA_enum_to: Enum '%s' field '%s' not registered!", luaA_typename(L, type), name);
        lua_error(L);
        return;
    }

    lua_pop(L, 3);
    lua_pushfstring(L, "luaA_enum_to: Enum '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
    return;
}

bool luaA_enum_has_value_type(lua_State *L, luaA_Type type, const void *value) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums_values");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {

        lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums_sizes");
        lua_pushinteger(L, type);
        lua_gettable(L, -2);
        size_t size = lua_tointeger(L, -1);
        lua_pop(L, 2);

        lua_Integer lvalue = 0;
        memcpy(&lvalue, value, size);

        lua_pushinteger(L, lvalue);
        lua_gettable(L, -2);

        if (lua_isnil(L, -1)) {
            lua_pop(L, 3);
            return false;
        } else {
            lua_pop(L, 3);
            return true;
        }
    }

    lua_pop(L, 2);
    lua_pushfstring(L, "luaA_enum_has_value: Enum '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
    return false;
}

bool luaA_enum_has_name_type(lua_State *L, luaA_Type type, const char *name) {
    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {

        lua_getfield(L, -1, name);

        if (lua_isnil(L, -1)) {
            lua_pop(L, 3);
            return false;
        } else {
            lua_pop(L, 3);
            return true;
        }
    }

    lua_pop(L, 2);
    lua_pushfstring(L, "luaA_enum_has_name: Enum '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
    return false;
}

void luaA_enum_type(lua_State *L, luaA_Type type, size_t size) {
    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums");
    lua_pushinteger(L, type);
    lua_newtable(L);
    lua_settable(L, -3);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums_values");
    lua_pushinteger(L, type);
    lua_newtable(L);
    lua_settable(L, -3);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums_sizes");
    lua_pushinteger(L, type);
    lua_pushinteger(L, size);
    lua_settable(L, -3);
    lua_pop(L, 1);
}

void luaA_enum_value_type(lua_State *L, luaA_Type type, const void *value, const char *name) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {

        lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums_sizes");
        lua_pushinteger(L, type);
        lua_gettable(L, -2);
        size_t size = lua_tointeger(L, -1);
        lua_pop(L, 2);

        lua_newtable(L);

        lua_Integer lvalue = 0;
        memcpy(&lvalue, value, size);

        lua_pushinteger(L, lvalue);
        lua_setfield(L, -2, "value");

        lua_pushstring(L, name);
        lua_setfield(L, -2, "name");

        lua_setfield(L, -2, name);

        lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums_values");
        lua_pushinteger(L, type);
        lua_gettable(L, -2);
        lua_pushinteger(L, lvalue);
        lua_getfield(L, -4, name);
        lua_settable(L, -3);

        lua_pop(L, 4);
        return;
    }

    lua_pop(L, 2);
    lua_pushfstring(L, "luaA_enum_value: Enum '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
}

bool luaA_enum_registered_type(lua_State *L, luaA_Type type) {
    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);
    bool reg = !lua_isnil(L, -1);
    lua_pop(L, 2);
    return reg;
}

const char *luaA_enum_next_value_name_type(lua_State *L, luaA_Type type, const char *member) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "enums");
    lua_pushinteger(L, type);
    lua_gettable(L, -2);

    if (!lua_isnil(L, -1)) {

        if (!member) {
            lua_pushnil(L);
        } else {
            lua_pushstring(L, member);
        }
        if (!lua_next(L, -2)) {
            lua_pop(L, 2);
            return LUAA_INVALID_MEMBER_NAME;
        }
        const char *result = lua_tostring(L, -2);
        lua_pop(L, 4);
        return result;
    }

    lua_pop(L, 2);
    lua_pushfstring(L, "luaA_enum_next_enum_name_type: Enum '%s' not registered!", luaA_typename(L, type));
    lua_error(L);
    return NULL;
}

/*
** Functions
*/

static int luaA_call_entry(lua_State *L) {

    /* Get return size */

    lua_getfield(L, -1, "ret_type");
    luaA_Type ret_type = lua_tointeger(L, -1);
    lua_pop(L, 1);

    size_t ret_size = luaA_typesize(L, ret_type);

    /* Get total arguments sizes */

    lua_getfield(L, -1, "arg_types");

    size_t arg_size = 0;
    size_t arg_num = lua_rawlen(L, -1);

    if (lua_gettop(L) < arg_num + 2) {
        lua_pop(L, 1);
        lua_pushfstring(L, "luaA_call: Too few arguments to function!");
        lua_error(L);
        return 0;
    }

    for (int i = 0; i < arg_num; i++) {
        lua_pushinteger(L, i + 1);
        lua_gettable(L, -2);
        luaA_Type arg_type = lua_tointeger(L, -1);
        lua_pop(L, 1);
        arg_size += luaA_typesize(L, arg_type);
    }

    lua_pop(L, 1);

    /* Test to see if using heap */

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "call_ret_stk");
    void *ret_stack = lua_touserdata(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "call_arg_stk");
    void *arg_stack = lua_touserdata(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "call_ret_ptr");
    lua_Integer ret_ptr = lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "call_arg_ptr");
    lua_Integer arg_ptr = lua_tointeger(L, -1);
    lua_pop(L, 1);

    void *ret_data = (char *)ret_stack + ret_ptr;
    void *arg_data = (char *)arg_stack + arg_ptr;

    /* If fixed allocation exhausted use heap instead */

    bool ret_heap = false;
    bool arg_heap = false;

    if (ret_ptr + ret_size > LUAA_RETURN_STACK_SIZE) {
        ret_heap = true;
        ret_data = malloc(ret_size);
        if (ret_data == NULL) {
            lua_pushfstring(L, "luaA_call: Out of memory!");
            lua_error(L);
            return 0;
        }
    }

    if (arg_ptr + arg_size > LUAA_ARGUMENT_STACK_SIZE) {
        arg_heap = true;
        arg_data = malloc(arg_size);
        if (arg_data == NULL) {
            if (ret_heap) {
                free(ret_data);
            }
            lua_pushfstring(L, "luaA_call: Out of memory!");
            lua_error(L);
            return 0;
        }
    }

    /* If not using heap update stack pointers */

    if (!ret_heap) {
        lua_pushinteger(L, ret_ptr + ret_size);
        lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "call_ret_ptr");
    }

    if (!arg_heap) {
        lua_pushinteger(L, arg_ptr + arg_size);
        lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "call_arg_ptr");
    }

    /* Pop args and place in memory */

    lua_getfield(L, -1, "arg_types");

    void *arg_pos = arg_data;
    for (int i = 0; i < arg_num; i++) {
        lua_pushinteger(L, i + 1);
        lua_gettable(L, -2);
        luaA_Type arg_type = lua_tointeger(L, -1);
        lua_pop(L, 1);
        luaA_to_type(L, arg_type, arg_pos, -arg_num + i - 2);
        arg_pos = (char *)arg_pos + luaA_typesize(L, arg_type);
    }

    lua_pop(L, 1);

    /* Pop arguments from stack */

    for (int i = 0; i < arg_num; i++) {
        lua_remove(L, -2);
    }

    /* Get Function Pointer and Call */

    lua_getfield(L, -1, "auto_func");
    luaA_Func auto_func = (luaA_Func)lua_touserdata(L, -1);
    lua_pop(L, 2);

    auto_func(ret_data, arg_data);

    int count = luaA_push_type(L, ret_type, ret_data);

    /* Either free heap data or reduce stack pointers */

    if (!ret_heap) {
        lua_pushinteger(L, ret_ptr);
        lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "call_ret_ptr");
    } else {
        free(ret_data);
    }

    if (!arg_heap) {
        lua_pushinteger(L, arg_ptr);
        lua_setfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "argument_ptr");
    } else {
        free(arg_data);
    }

    return count;
}

int luaA_call(lua_State *L, void *func_ptr) {
    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "functions");
    lua_pushlightuserdata(L, func_ptr);
    lua_gettable(L, -2);
    lua_remove(L, -2);

    if (!lua_isnil(L, -1)) {
        return luaA_call_entry(L);
    }

    lua_pop(L, 1);
    lua_pushfstring(L, "luaA_call: Function with address '%p' is not registered!", func_ptr);
    lua_error(L);
    return 0;
}

int luaA_call_name(lua_State *L, const char *func_name) {
    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "functions");
    lua_pushstring(L, func_name);
    lua_gettable(L, -2);
    lua_remove(L, -2);

    if (!lua_isnil(L, -1)) {
        return luaA_call_entry(L);
    }

    lua_pop(L, 1);
    lua_pushfstring(L, "luaA_call_name: Function '%s' is not registered!", func_name);
    lua_error(L);
    return 0;
}

void luaA_function_register_type(lua_State *L, void *src_func, luaA_Func auto_func, const char *name, luaA_Type ret_t, int num_args, ...) {

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "functions");
    lua_pushstring(L, name);

    lua_newtable(L);

    lua_pushlightuserdata(L, src_func);
    lua_setfield(L, -2, "src_func");
    lua_pushlightuserdata(L, auto_func);
    lua_setfield(L, -2, "auto_func");

    lua_pushinteger(L, ret_t);
    lua_setfield(L, -2, "ret_type");

    lua_pushstring(L, "arg_types");
    lua_newtable(L);

    va_list va;
    va_start(va, num_args);
    for (int i = 0; i < num_args; i++) {
        lua_pushinteger(L, i + 1);
        lua_pushinteger(L, va_arg(va, luaA_Type));
        lua_settable(L, -3);
    }
    va_end(va);

    lua_settable(L, -3);
    lua_settable(L, -3);
    lua_pop(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "functions");
    lua_pushlightuserdata(L, src_func);

    lua_getfield(L, LUA_REGISTRYINDEX, LUAA_REGISTRYPREFIX "functions");
    lua_getfield(L, -1, name);
    lua_remove(L, -2);

    lua_settable(L, -3);
    lua_pop(L, 1);
}

#pragma endregion LuaA

int luaopen_debugger(lua_State *lua) {
    if (luaL_dofile(lua, METADOT_RESLOC("data/scripts/libs/debugger.lua"))) lua_error(lua);
    return 1;
}

static const char *MODULE_NAME = "DEBUGGER_LUA_MODULE";
static const char *MSGH = "DEBUGGER_LUA_MSGH";

void ME_debug_setup(lua_State *lua, const char *name, const char *globalName, lua_CFunction readFunc, lua_CFunction writeFunc) {
    // Check that the module name was not already defined.
    lua_getfield(lua, LUA_REGISTRYINDEX, MODULE_NAME);
    assert(lua_isnil(lua, -1) || strcmp(name, luaL_checkstring(lua, -1)));
    lua_pop(lua, 1);

    // Push the module name into the registry.
    lua_pushstring(lua, name);
    lua_setfield(lua, LUA_REGISTRYINDEX, MODULE_NAME);

    // Preload the module
    luaL_requiref(lua, name, luaopen_debugger, false);

    // Insert the msgh function into the registry.
    lua_getfield(lua, -1, "msgh");
    lua_setfield(lua, LUA_REGISTRYINDEX, MSGH);

    if (readFunc) {
        lua_pushcfunction(lua, readFunc);
        lua_setfield(lua, -2, "read");
    }

    if (writeFunc) {
        lua_pushcfunction(lua, writeFunc);
        lua_setfield(lua, -2, "write");
    }

    if (globalName) {
        lua_setglobal(lua, globalName);
    } else {
        lua_pop(lua, 1);
    }
}

int ME_debug_pcall(lua_State *lua, int nargs, int nresults, int msgh) {
    // Call regular lua_pcall() if a message handler is provided.
    if (msgh) return lua_pcall(lua, nargs, nresults, msgh);

    // Grab the msgh function out of the registry.
    lua_getfield(lua, LUA_REGISTRYINDEX, MSGH);
    if (lua_isnil(lua, -1)) {
        luaL_error(lua, "Tried to call dbg_call() before calling dbg_setup().");
    }

    // Move the error handler just below the function.
    msgh = lua_gettop(lua) - (1 + nargs);
    lua_insert(lua, msgh);

    // Call the function.
    int err = lua_pcall(lua, nargs, nresults, msgh);

    // Remove the debug handler.
    lua_remove(lua, msgh);

    return err;
}

int ME_preload(lua_State *L, lua_CFunction f, const char *name) {
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    lua_pushcfunction(L, f);
    lua_setfield(L, -2, name);
    lua_pop(L, 2);
    return 0;
}

int ME_preload_auto(lua_State *L, lua_CFunction f, const char *name) {
    ME_preload(L, f, name);
    lua_getglobal(L, "require");
    lua_pushstring(L, name);
    lua_call(L, 1, 0);
    return 0;
}

void ME_load(lua_State *L, const luaL_Reg *l, const char *name) {
    lua_getglobal(L, name);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
    }
    luaL_setfuncs(L, l, 0);
    lua_setglobal(L, name);
}

void ME_loadover(lua_State *L, const luaL_Reg *l, const char *name) {
    lua_newtable(L);
    luaL_setfuncs(L, l, 0);
    lua_setglobal(L, name);
}

#pragma region LuaMemSafe

const char *const REG_POLICY = "ME_safelua_policy";
const char *const REG_CANCEL = "ME_safelua_cancel";
const char *const REG_CANCELUDATA = "ME_safelua_canceludata";
const char *const REG_CANCELJMP = "ME_safelua_canceljmp";

#define setudregistry(state, value, name)                \
    do {                                                 \
        lua_State *state_ = (state);                     \
        lua_pushlightuserdata(state_, (value));          \
        lua_setfield(state_, LUA_REGISTRYINDEX, (name)); \
    } while (0)

static void *getudregistry(lua_State *state, const char *name) {
    void *ret;
    lua_getfield(state, LUA_REGISTRYINDEX, name);
    ret = lua_touserdata(state, -1);
    lua_pop(state, 1);
    return ret;
}

struct Handler {
    ME_safelua_Handler callback;
    void *udata;
};

struct Policy {
    struct Allocator *allocator;
    struct Handler *handlers;
    size_t nb_handlers, size_handlers;
};

lua_State *ME_safelua_open(void) {
    struct Policy *policy = (Policy *)malloc(sizeof(struct Policy));
    policy->allocator = (Allocator *)new_allocator();
    policy->handlers = NULL;
    policy->nb_handlers = policy->size_handlers = 0;

    /* Create state with allocator */
    lua_State *state = lua_newstate(lua_alloc, policy->allocator);

    /* Open standard libraries */
    luaL_requiref(state, "", luaopen_base, 0);
    luaL_requiref(state, "coroutine", luaopen_coroutine, 1);
    luaL_requiref(state, "package", luaopen_package, 1);
    luaL_requiref(state, "string", luaopen_string, 1);
    luaL_requiref(state, "table", luaopen_table, 1);
    luaL_requiref(state, "math", luaopen_math, 1);
    luaL_requiref(state, "utf8", luaopen_utf8, 1);
    luaL_requiref(state, "io", luaopen_io, 1);
    luaL_requiref(state, "os", luaopen_os, 1);
    // without luaopen_debug

    setudregistry(state, policy, REG_POLICY);

    return state;
}

static void free_resources(struct Policy *policy, int why) {
    size_t i;
    for (i = 0; i < policy->nb_handlers; ++i) policy->handlers[i].callback(why, policy->handlers[i].udata);
    free(policy->handlers);
}

void ME_safelua_close(lua_State *state) {
    struct Policy *policy = (Policy *)getudregistry(state, REG_POLICY);
    lua_close(state);
    free_resources(policy, SAFELUA_FINISHED);
    delete_allocator((struct lua_allocator *)policy->allocator);
    free(policy);
}

static void cancel_hook(lua_State *state, lua_Debug *ar) { ME_safelua_checkcancel(state); }

int ME_safelua_pcallk(lua_State *state, int nargs, int nresults, int errfunc, int ctx, lua_KFunction k, ME_safelua_CancelCheck cancel, void *canceludata) {
    struct Policy *policy = (Policy *)getudregistry(state, REG_POLICY);
    jmp_buf env;
    int ret;

    /* If catch point is already set, do a normal pcallk */
    if (getudregistry(state, REG_CANCELJMP) != NULL)
        return lua_pcallk(state, nargs, nresults, errfunc, ctx, k);
    else {
        /* Set hook from which we will be able to exit */
        lua_sethook(state, cancel_hook, LUA_MASKCOUNT, 10);

        if (setjmp(env) == 0) {
            setudregistry(state, &env, REG_CANCELJMP);
            setudregistry(state, cancel, REG_CANCEL);
            setudregistry(state, canceludata, REG_CANCELUDATA);
            ret = lua_pcallk(state, nargs, nresults, errfunc, ctx, k);
            lua_pushnil(state);
            lua_setfield(state, LUA_REGISTRYINDEX, REG_CANCEL);
            lua_pushnil(state);
            lua_setfield(state, LUA_REGISTRYINDEX, REG_CANCELJMP);
            lua_sethook(state, cancel_hook, 0, 0);
        } else {
            free_resources(policy, SAFELUA_CANCELED);
            delete_allocator((struct lua_allocator *)policy->allocator);
            free(policy);
            ret = SAFELUA_CANCELED;
        }

        return ret;
    }
}

void ME_safelua_checkcancel(lua_State *state) {
    if (ME_safelua_shouldcancel(state)) ME_safelua_cancel(state);
}

void ME_safelua_cancel(lua_State *state) {
    jmp_buf *env = (jmp_buf *)getudregistry(state, REG_CANCELJMP);
    if (env) longjmp(*env, 1);
}

int ME_safelua_shouldcancel(lua_State *state) {
    ME_safelua_CancelCheck cancel = (ME_safelua_CancelCheck)getudregistry(state, REG_CANCEL);
    void *canceludata = getudregistry(state, REG_CANCELUDATA);

    return cancel && cancel(state, canceludata);
}

void ME_safelua_add_handler(lua_State *state, ME_safelua_Handler handler, void *handlerudata) {
    struct Policy *policy = (Policy *)getudregistry(state, REG_POLICY);
    if (++policy->nb_handlers > policy->size_handlers) {
        if (policy->size_handlers == 0)
            policy->size_handlers = 4;
        else
            policy->size_handlers *= 2;
        policy->handlers = (Handler *)realloc(policy->handlers, sizeof(struct Handler) * policy->size_handlers);
    }
    {
        struct Handler *h = &policy->handlers[policy->nb_handlers - 1];
        h->callback = handler;
        h->udata = handlerudata;
    }
}

int ME_safelua_remove_handler(lua_State *state, ME_safelua_Handler handler, void *handlerudata) {
    struct Policy *policy = (Policy *)getudregistry(state, REG_POLICY);
    size_t found = 0;
    size_t pos = 0;
    for (; pos < policy->nb_handlers; ++pos) {
        if (policy->handlers[pos].callback == handler && policy->handlers[pos].udata == handlerudata)
            found++;
        else
            policy->handlers[pos - found] = policy->handlers[pos];
    }
    policy->nb_handlers -= found;
    return found;
}

#pragma endregion LuaMemSafe