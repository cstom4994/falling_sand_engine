
#include "lua_wrapper.h"

#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "Libs/external/queue.h"

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

struct LuaAllocator *new_allocator(void) {
    struct LuaAllocator *alloc = malloc(sizeof(struct LuaAllocator));
    alloc->blocks = malloc(sizeof(struct LuaMemBlock) * 4);
    alloc->nb_blocks = 0;
    alloc->size_blocks = 4;
    alloc->total_allocated = 0;
#ifdef _DEBUG_ALLOC
    fprintf(stderr, "Created allocator %p\n", alloc);
#endif
    return alloc;
}

void delete_allocator(struct LuaAllocator *alloc) {
    size_t blk;
#ifdef _DEBUG_ALLOC
    fprintf(stderr, "Deleting allocator %p\n", alloc);
#endif
    for (blk = 0; blk < alloc->nb_blocks; ++blk) free(alloc->blocks[blk].ptr);
    free(alloc->blocks);
    free(alloc);
}

void *lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    struct LuaAllocator *alloc = ud;
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
            alloc->blocks = realloc(alloc->blocks, sizeof(struct LuaMemBlock) * alloc->size_blocks);
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
        memmove(&alloc->blocks[blk], &alloc->blocks[blk + 1], sizeof(struct LuaMemBlock) * (alloc->nb_blocks - blk - 1));
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
            struct LuaMemBlock tmp = alloc->blocks[blk];
            alloc->blocks[blk] = alloc->blocks[blk - 1];
            alloc->blocks[blk - 1] = tmp;
            blk--;
#ifdef _DEBUG_ALLOC
            fprintf(stderr, "< ");
#endif
        }
        while (blk + 1 < alloc->nb_blocks && alloc->blocks[blk].ptr > alloc->blocks[blk + 1].ptr) {
            struct LuaMemBlock tmp = alloc->blocks[blk];
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

    luaA_conversion_push_type(L, luaA_type_add(L, "void", 1), luaA_push_void); // sizeof(void) is 1 on gcc
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
        luaA_Pushfunc func = lua_touserdata(L, -1);
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
        luaA_Tofunc func = lua_touserdata(L, -1);
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
            return luaA_push_type(L, stype, (char*)c_in + offset);
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
        arg_pos = (char*)arg_pos + luaA_typesize(L, arg_type);
    }

    lua_pop(L, 1);

    /* Pop arguments from stack */

    for (int i = 0; i < arg_num; i++) {
        lua_remove(L, -2);
    }

    /* Get Function Pointer and Call */

    lua_getfield(L, -1, "auto_func");
    luaA_Func auto_func = lua_touserdata(L, -1);
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

#pragma region LuaCS

#if 1

#define SPLAY_HEAD(name, type)                        \
    struct name {                                     \
        struct type *sph_root; /* root of the tree */ \
    }

#define SPLAY_INITIALIZER(root) \
    { NULL }

#define SPLAY_INIT(root)         \
    do {                         \
        (root)->sph_root = NULL; \
    } while (/*CONSTCOND*/ 0)

#define SPLAY_ENTRY(type)                           \
    struct {                                        \
        struct type *spe_left;  /* left element */  \
        struct type *spe_right; /* right element */ \
    }

#define SPLAY_LEFT(elm, field) (elm)->field.spe_left
#define SPLAY_RIGHT(elm, field) (elm)->field.spe_right
#define SPLAY_ROOT(head) (head)->sph_root
#define SPLAY_EMPTY(head) (SPLAY_ROOT(head) == NULL)

/* SPLAY_ROTATE_{LEFT,RIGHT} expect that tmp hold SPLAY_{RIGHT,LEFT} */
#define SPLAY_ROTATE_RIGHT(head, tmp, field)                           \
    do {                                                               \
        SPLAY_LEFT((head)->sph_root, field) = SPLAY_RIGHT(tmp, field); \
        SPLAY_RIGHT(tmp, field) = (head)->sph_root;                    \
        (head)->sph_root = tmp;                                        \
    } while (/*CONSTCOND*/ 0)

#define SPLAY_ROTATE_LEFT(head, tmp, field)                            \
    do {                                                               \
        SPLAY_RIGHT((head)->sph_root, field) = SPLAY_LEFT(tmp, field); \
        SPLAY_LEFT(tmp, field) = (head)->sph_root;                     \
        (head)->sph_root = tmp;                                        \
    } while (/*CONSTCOND*/ 0)

#define SPLAY_LINKLEFT(head, tmp, field)                        \
    do {                                                        \
        SPLAY_LEFT(tmp, field) = (head)->sph_root;              \
        tmp = (head)->sph_root;                                 \
        (head)->sph_root = SPLAY_LEFT((head)->sph_root, field); \
    } while (/*CONSTCOND*/ 0)

#define SPLAY_LINKRIGHT(head, tmp, field)                        \
    do {                                                         \
        SPLAY_RIGHT(tmp, field) = (head)->sph_root;              \
        tmp = (head)->sph_root;                                  \
        (head)->sph_root = SPLAY_RIGHT((head)->sph_root, field); \
    } while (/*CONSTCOND*/ 0)

#define SPLAY_ASSEMBLE(head, node, left, right, field)                   \
    do {                                                                 \
        SPLAY_RIGHT(left, field) = SPLAY_LEFT((head)->sph_root, field);  \
        SPLAY_LEFT(right, field) = SPLAY_RIGHT((head)->sph_root, field); \
        SPLAY_LEFT((head)->sph_root, field) = SPLAY_RIGHT(node, field);  \
        SPLAY_RIGHT((head)->sph_root, field) = SPLAY_LEFT(node, field);  \
    } while (/*CONSTCOND*/ 0)

/* Generates prototypes and inline functions */

#define SPLAY_PROTOTYPE(name, type, field, cmp)                                           \
    void name##_SPLAY(struct name *, struct type *);                                      \
    void name##_SPLAY_MINMAX(struct name *, int);                                         \
    struct type *name##_SPLAY_INSERT(struct name *, struct type *);                       \
    struct type *name##_SPLAY_REMOVE(struct name *, struct type *);                       \
                                                                                          \
    /* Finds the node with the same key as elm */                                         \
    static __inline struct type *name##_SPLAY_FIND(struct name *head, struct type *elm) { \
        if (SPLAY_EMPTY(head)) return (NULL);                                             \
        name##_SPLAY(head, elm);                                                          \
        if ((cmp)(elm, (head)->sph_root) == 0) return (head->sph_root);                   \
        return (NULL);                                                                    \
    }                                                                                     \
                                                                                          \
    static __inline struct type *name##_SPLAY_NEXT(struct name *head, struct type *elm) { \
        name##_SPLAY(head, elm);                                                          \
        if (SPLAY_RIGHT(elm, field) != NULL) {                                            \
            elm = SPLAY_RIGHT(elm, field);                                                \
            while (SPLAY_LEFT(elm, field) != NULL) {                                      \
                elm = SPLAY_LEFT(elm, field);                                             \
            }                                                                             \
        } else                                                                            \
            elm = NULL;                                                                   \
        return (elm);                                                                     \
    }                                                                                     \
                                                                                          \
    static __inline struct type *name##_SPLAY_MIN_MAX(struct name *head, int val) {       \
        name##_SPLAY_MINMAX(head, val);                                                   \
        return (SPLAY_ROOT(head));                                                        \
    }

/* Main splay operation.
 * Moves node close to the key of elm to top
 */
#define SPLAY_GENERATE(name, type, field, cmp)                                  \
    struct type *name##_SPLAY_INSERT(struct name *head, struct type *elm) {     \
        if (SPLAY_EMPTY(head)) {                                                \
            SPLAY_LEFT(elm, field) = SPLAY_RIGHT(elm, field) = NULL;            \
        } else {                                                                \
            int __comp;                                                         \
            name##_SPLAY(head, elm);                                            \
            __comp = (cmp)(elm, (head)->sph_root);                              \
            if (__comp < 0) {                                                   \
                SPLAY_LEFT(elm, field) = SPLAY_LEFT((head)->sph_root, field);   \
                SPLAY_RIGHT(elm, field) = (head)->sph_root;                     \
                SPLAY_LEFT((head)->sph_root, field) = NULL;                     \
            } else if (__comp > 0) {                                            \
                SPLAY_RIGHT(elm, field) = SPLAY_RIGHT((head)->sph_root, field); \
                SPLAY_LEFT(elm, field) = (head)->sph_root;                      \
                SPLAY_RIGHT((head)->sph_root, field) = NULL;                    \
            } else                                                              \
                return ((head)->sph_root);                                      \
        }                                                                       \
        (head)->sph_root = (elm);                                               \
        return (NULL);                                                          \
    }                                                                           \
                                                                                \
    struct type *name##_SPLAY_REMOVE(struct name *head, struct type *elm) {     \
        struct type *__tmp;                                                     \
        if (SPLAY_EMPTY(head)) return (NULL);                                   \
        name##_SPLAY(head, elm);                                                \
        if ((cmp)(elm, (head)->sph_root) == 0) {                                \
            if (SPLAY_LEFT((head)->sph_root, field) == NULL) {                  \
                (head)->sph_root = SPLAY_RIGHT((head)->sph_root, field);        \
            } else {                                                            \
                __tmp = SPLAY_RIGHT((head)->sph_root, field);                   \
                (head)->sph_root = SPLAY_LEFT((head)->sph_root, field);         \
                name##_SPLAY(head, elm);                                        \
                SPLAY_RIGHT((head)->sph_root, field) = __tmp;                   \
            }                                                                   \
            return (elm);                                                       \
        }                                                                       \
        return (NULL);                                                          \
    }                                                                           \
                                                                                \
    void name##_SPLAY(struct name *head, struct type *elm) {                    \
        struct type __node, *__left, *__right, *__tmp;                          \
        int __comp;                                                             \
                                                                                \
        SPLAY_LEFT(&__node, field) = SPLAY_RIGHT(&__node, field) = NULL;        \
        __left = __right = &__node;                                             \
                                                                                \
        while ((__comp = (cmp)(elm, (head)->sph_root)) != 0) {                  \
            if (__comp < 0) {                                                   \
                __tmp = SPLAY_LEFT((head)->sph_root, field);                    \
                if (__tmp == NULL) break;                                       \
                if ((cmp)(elm, __tmp) < 0) {                                    \
                    SPLAY_ROTATE_RIGHT(head, __tmp, field);                     \
                    if (SPLAY_LEFT((head)->sph_root, field) == NULL) break;     \
                }                                                               \
                SPLAY_LINKLEFT(head, __right, field);                           \
            } else if (__comp > 0) {                                            \
                __tmp = SPLAY_RIGHT((head)->sph_root, field);                   \
                if (__tmp == NULL) break;                                       \
                if ((cmp)(elm, __tmp) > 0) {                                    \
                    SPLAY_ROTATE_LEFT(head, __tmp, field);                      \
                    if (SPLAY_RIGHT((head)->sph_root, field) == NULL) break;    \
                }                                                               \
                SPLAY_LINKRIGHT(head, __left, field);                           \
            }                                                                   \
        }                                                                       \
        SPLAY_ASSEMBLE(head, &__node, __left, __right, field);                  \
    }                                                                           \
                                                                                \
    /* Splay with either the minimum or the maximum element                     \
     * Used to find minimum or maximum element in tree.                         \
     */                                                                         \
    void name##_SPLAY_MINMAX(struct name *head, int __comp) {                   \
        struct type __node, *__left, *__right, *__tmp;                          \
                                                                                \
        SPLAY_LEFT(&__node, field) = SPLAY_RIGHT(&__node, field) = NULL;        \
        __left = __right = &__node;                                             \
                                                                                \
        while (1) {                                                             \
            if (__comp < 0) {                                                   \
                __tmp = SPLAY_LEFT((head)->sph_root, field);                    \
                if (__tmp == NULL) break;                                       \
                if (__comp < 0) {                                               \
                    SPLAY_ROTATE_RIGHT(head, __tmp, field);                     \
                    if (SPLAY_LEFT((head)->sph_root, field) == NULL) break;     \
                }                                                               \
                SPLAY_LINKLEFT(head, __right, field);                           \
            } else if (__comp > 0) {                                            \
                __tmp = SPLAY_RIGHT((head)->sph_root, field);                   \
                if (__tmp == NULL) break;                                       \
                if (__comp > 0) {                                               \
                    SPLAY_ROTATE_LEFT(head, __tmp, field);                      \
                    if (SPLAY_RIGHT((head)->sph_root, field) == NULL) break;    \
                }                                                               \
                SPLAY_LINKRIGHT(head, __left, field);                           \
            }                                                                   \
        }                                                                       \
        SPLAY_ASSEMBLE(head, &__node, __left, __right, field);                  \
    }

#define SPLAY_NEGINF -1
#define SPLAY_INF 1

#define SPLAY_INSERT(name, x, y) name##_SPLAY_INSERT(x, y)
#define SPLAY_REMOVE(name, x, y) name##_SPLAY_REMOVE(x, y)
#define SPLAY_FIND(name, x, y) name##_SPLAY_FIND(x, y)
#define SPLAY_NEXT(name, x, y) name##_SPLAY_NEXT(x, y)
#define SPLAY_MIN(name, x) (SPLAY_EMPTY(x) ? NULL : name##_SPLAY_MIN_MAX(x, SPLAY_NEGINF))
#define SPLAY_MAX(name, x) (SPLAY_EMPTY(x) ? NULL : name##_SPLAY_MIN_MAX(x, SPLAY_INF))

#define SPLAY_FOREACH(x, name, head) for ((x) = SPLAY_MIN(name, head); (x) != NULL; (x) = SPLAY_NEXT(name, head, x))

/* Macros that define a red-black tree */
#define RB_HEAD(name, type)                           \
    struct name {                                     \
        struct type *rbh_root; /* root of the tree */ \
    }

#define RB_INITIALIZER(root) \
    { NULL }

#define RB_INIT(root)            \
    do {                         \
        (root)->rbh_root = NULL; \
    } while (/*CONSTCOND*/ 0)

#define RB_BLACK 0
#define RB_RED 1
#define RB_ENTRY(type)                                \
    struct {                                          \
        struct type *rbe_left;   /* left element */   \
        struct type *rbe_right;  /* right element */  \
        struct type *rbe_parent; /* parent element */ \
        int rbe_color;           /* node color */     \
    }

#define RB_LEFT(elm, field) (elm)->field.rbe_left
#define RB_RIGHT(elm, field) (elm)->field.rbe_right
#define RB_PARENT(elm, field) (elm)->field.rbe_parent
#define RB_COLOR(elm, field) (elm)->field.rbe_color
#define RB_ROOT(head) (head)->rbh_root
#define RB_EMPTY(head) (RB_ROOT(head) == NULL)

#define RB_SET(elm, parent, field)                         \
    do {                                                   \
        RB_PARENT(elm, field) = parent;                    \
        RB_LEFT(elm, field) = RB_RIGHT(elm, field) = NULL; \
        RB_COLOR(elm, field) = RB_RED;                     \
    } while (/*CONSTCOND*/ 0)

#define RB_SET_BLACKRED(black, red, field) \
    do {                                   \
        RB_COLOR(black, field) = RB_BLACK; \
        RB_COLOR(red, field) = RB_RED;     \
    } while (/*CONSTCOND*/ 0)

#ifndef RB_AUGMENT
#define RB_AUGMENT(x) \
    do {              \
    } while (0)
#endif

#define RB_ROTATE_LEFT(head, elm, tmp, field)                           \
    do {                                                                \
        (tmp) = RB_RIGHT(elm, field);                                   \
        if ((RB_RIGHT(elm, field) = RB_LEFT(tmp, field)) != NULL) {     \
            RB_PARENT(RB_LEFT(tmp, field), field) = (elm);              \
        }                                                               \
        RB_AUGMENT(elm);                                                \
        if ((RB_PARENT(tmp, field) = RB_PARENT(elm, field)) != NULL) {  \
            if ((elm) == RB_LEFT(RB_PARENT(elm, field), field))         \
                RB_LEFT(RB_PARENT(elm, field), field) = (tmp);          \
            else                                                        \
                RB_RIGHT(RB_PARENT(elm, field), field) = (tmp);         \
        } else                                                          \
            (head)->rbh_root = (tmp);                                   \
        RB_LEFT(tmp, field) = (elm);                                    \
        RB_PARENT(elm, field) = (tmp);                                  \
        RB_AUGMENT(tmp);                                                \
        if ((RB_PARENT(tmp, field))) RB_AUGMENT(RB_PARENT(tmp, field)); \
    } while (/*CONSTCOND*/ 0)

#define RB_ROTATE_RIGHT(head, elm, tmp, field)                          \
    do {                                                                \
        (tmp) = RB_LEFT(elm, field);                                    \
        if ((RB_LEFT(elm, field) = RB_RIGHT(tmp, field)) != NULL) {     \
            RB_PARENT(RB_RIGHT(tmp, field), field) = (elm);             \
        }                                                               \
        RB_AUGMENT(elm);                                                \
        if ((RB_PARENT(tmp, field) = RB_PARENT(elm, field)) != NULL) {  \
            if ((elm) == RB_LEFT(RB_PARENT(elm, field), field))         \
                RB_LEFT(RB_PARENT(elm, field), field) = (tmp);          \
            else                                                        \
                RB_RIGHT(RB_PARENT(elm, field), field) = (tmp);         \
        } else                                                          \
            (head)->rbh_root = (tmp);                                   \
        RB_RIGHT(tmp, field) = (elm);                                   \
        RB_PARENT(elm, field) = (tmp);                                  \
        RB_AUGMENT(tmp);                                                \
        if ((RB_PARENT(tmp, field))) RB_AUGMENT(RB_PARENT(tmp, field)); \
    } while (/*CONSTCOND*/ 0)

/* Generates prototypes and inline functions */
#define RB_PROTOTYPE(name, type, field, cmp) RB_PROTOTYPE_INTERNAL(name, type, field, cmp, )
#define RB_PROTOTYPE_STATIC(name, type, field, cmp) RB_PROTOTYPE_INTERNAL(name, type, field, cmp, __unused static)
#define RB_PROTOTYPE_INTERNAL(name, type, field, cmp, attr)                        \
    attr void name##_RB_INSERT_COLOR(struct name *, struct type *);                \
    attr void name##_RB_REMOVE_COLOR(struct name *, struct type *, struct type *); \
    attr struct type *name##_RB_REMOVE(struct name *, struct type *);              \
    attr struct type *name##_RB_INSERT(struct name *, struct type *);              \
    attr struct type *name##_RB_FIND(struct name *, struct type *);                \
    attr struct type *name##_RB_NFIND(struct name *, struct type *);               \
    attr struct type *name##_RB_NEXT(struct type *);                               \
    attr struct type *name##_RB_PREV(struct type *);                               \
    attr struct type *name##_RB_MINMAX(struct name *, int);

/* Main rb operation.
 * Moves node close to the key of elm to top
 */
#define RB_GENERATE(name, type, field, cmp) RB_GENERATE_INTERNAL(name, type, field, cmp, )
#define RB_GENERATE_STATIC(name, type, field, cmp) RB_GENERATE_INTERNAL(name, type, field, cmp, __unused static)
#define RB_GENERATE_INTERNAL(name, type, field, cmp, attr)                                                                                                                                      \
    attr void name##_RB_INSERT_COLOR(struct name *head, struct type *elm) {                                                                                                                     \
        struct type *parent, *gparent, *tmp;                                                                                                                                                    \
        while ((parent = RB_PARENT(elm, field)) != NULL && RB_COLOR(parent, field) == RB_RED) {                                                                                                 \
            gparent = RB_PARENT(parent, field);                                                                                                                                                 \
            if (parent == RB_LEFT(gparent, field)) {                                                                                                                                            \
                tmp = RB_RIGHT(gparent, field);                                                                                                                                                 \
                if (tmp && RB_COLOR(tmp, field) == RB_RED) {                                                                                                                                    \
                    RB_COLOR(tmp, field) = RB_BLACK;                                                                                                                                            \
                    RB_SET_BLACKRED(parent, gparent, field);                                                                                                                                    \
                    elm = gparent;                                                                                                                                                              \
                    continue;                                                                                                                                                                   \
                }                                                                                                                                                                               \
                if (RB_RIGHT(parent, field) == elm) {                                                                                                                                           \
                    RB_ROTATE_LEFT(head, parent, tmp, field);                                                                                                                                   \
                    tmp = parent;                                                                                                                                                               \
                    parent = elm;                                                                                                                                                               \
                    elm = tmp;                                                                                                                                                                  \
                }                                                                                                                                                                               \
                RB_SET_BLACKRED(parent, gparent, field);                                                                                                                                        \
                RB_ROTATE_RIGHT(head, gparent, tmp, field);                                                                                                                                     \
            } else {                                                                                                                                                                            \
                tmp = RB_LEFT(gparent, field);                                                                                                                                                  \
                if (tmp && RB_COLOR(tmp, field) == RB_RED) {                                                                                                                                    \
                    RB_COLOR(tmp, field) = RB_BLACK;                                                                                                                                            \
                    RB_SET_BLACKRED(parent, gparent, field);                                                                                                                                    \
                    elm = gparent;                                                                                                                                                              \
                    continue;                                                                                                                                                                   \
                }                                                                                                                                                                               \
                if (RB_LEFT(parent, field) == elm) {                                                                                                                                            \
                    RB_ROTATE_RIGHT(head, parent, tmp, field);                                                                                                                                  \
                    tmp = parent;                                                                                                                                                               \
                    parent = elm;                                                                                                                                                               \
                    elm = tmp;                                                                                                                                                                  \
                }                                                                                                                                                                               \
                RB_SET_BLACKRED(parent, gparent, field);                                                                                                                                        \
                RB_ROTATE_LEFT(head, gparent, tmp, field);                                                                                                                                      \
            }                                                                                                                                                                                   \
        }                                                                                                                                                                                       \
        RB_COLOR(head->rbh_root, field) = RB_BLACK;                                                                                                                                             \
    }                                                                                                                                                                                           \
                                                                                                                                                                                                \
    attr void name##_RB_REMOVE_COLOR(struct name *head, struct type *parent, struct type *elm) {                                                                                                \
        struct type *tmp;                                                                                                                                                                       \
        while ((elm == NULL || RB_COLOR(elm, field) == RB_BLACK) && elm != RB_ROOT(head)) {                                                                                                     \
            if (RB_LEFT(parent, field) == elm) {                                                                                                                                                \
                tmp = RB_RIGHT(parent, field);                                                                                                                                                  \
                if (RB_COLOR(tmp, field) == RB_RED) {                                                                                                                                           \
                    RB_SET_BLACKRED(tmp, parent, field);                                                                                                                                        \
                    RB_ROTATE_LEFT(head, parent, tmp, field);                                                                                                                                   \
                    tmp = RB_RIGHT(parent, field);                                                                                                                                              \
                }                                                                                                                                                                               \
                if ((RB_LEFT(tmp, field) == NULL || RB_COLOR(RB_LEFT(tmp, field), field) == RB_BLACK) && (RB_RIGHT(tmp, field) == NULL || RB_COLOR(RB_RIGHT(tmp, field), field) == RB_BLACK)) { \
                    RB_COLOR(tmp, field) = RB_RED;                                                                                                                                              \
                    elm = parent;                                                                                                                                                               \
                    parent = RB_PARENT(elm, field);                                                                                                                                             \
                } else {                                                                                                                                                                        \
                    if (RB_RIGHT(tmp, field) == NULL || RB_COLOR(RB_RIGHT(tmp, field), field) == RB_BLACK) {                                                                                    \
                        struct type *oleft;                                                                                                                                                     \
                        if ((oleft = RB_LEFT(tmp, field)) != NULL) RB_COLOR(oleft, field) = RB_BLACK;                                                                                           \
                        RB_COLOR(tmp, field) = RB_RED;                                                                                                                                          \
                        RB_ROTATE_RIGHT(head, tmp, oleft, field);                                                                                                                               \
                        tmp = RB_RIGHT(parent, field);                                                                                                                                          \
                    }                                                                                                                                                                           \
                    RB_COLOR(tmp, field) = RB_COLOR(parent, field);                                                                                                                             \
                    RB_COLOR(parent, field) = RB_BLACK;                                                                                                                                         \
                    if (RB_RIGHT(tmp, field)) RB_COLOR(RB_RIGHT(tmp, field), field) = RB_BLACK;                                                                                                 \
                    RB_ROTATE_LEFT(head, parent, tmp, field);                                                                                                                                   \
                    elm = RB_ROOT(head);                                                                                                                                                        \
                    break;                                                                                                                                                                      \
                }                                                                                                                                                                               \
            } else {                                                                                                                                                                            \
                tmp = RB_LEFT(parent, field);                                                                                                                                                   \
                if (RB_COLOR(tmp, field) == RB_RED) {                                                                                                                                           \
                    RB_SET_BLACKRED(tmp, parent, field);                                                                                                                                        \
                    RB_ROTATE_RIGHT(head, parent, tmp, field);                                                                                                                                  \
                    tmp = RB_LEFT(parent, field);                                                                                                                                               \
                }                                                                                                                                                                               \
                if ((RB_LEFT(tmp, field) == NULL || RB_COLOR(RB_LEFT(tmp, field), field) == RB_BLACK) && (RB_RIGHT(tmp, field) == NULL || RB_COLOR(RB_RIGHT(tmp, field), field) == RB_BLACK)) { \
                    RB_COLOR(tmp, field) = RB_RED;                                                                                                                                              \
                    elm = parent;                                                                                                                                                               \
                    parent = RB_PARENT(elm, field);                                                                                                                                             \
                } else {                                                                                                                                                                        \
                    if (RB_LEFT(tmp, field) == NULL || RB_COLOR(RB_LEFT(tmp, field), field) == RB_BLACK) {                                                                                      \
                        struct type *oright;                                                                                                                                                    \
                        if ((oright = RB_RIGHT(tmp, field)) != NULL) RB_COLOR(oright, field) = RB_BLACK;                                                                                        \
                        RB_COLOR(tmp, field) = RB_RED;                                                                                                                                          \
                        RB_ROTATE_LEFT(head, tmp, oright, field);                                                                                                                               \
                        tmp = RB_LEFT(parent, field);                                                                                                                                           \
                    }                                                                                                                                                                           \
                    RB_COLOR(tmp, field) = RB_COLOR(parent, field);                                                                                                                             \
                    RB_COLOR(parent, field) = RB_BLACK;                                                                                                                                         \
                    if (RB_LEFT(tmp, field)) RB_COLOR(RB_LEFT(tmp, field), field) = RB_BLACK;                                                                                                   \
                    RB_ROTATE_RIGHT(head, parent, tmp, field);                                                                                                                                  \
                    elm = RB_ROOT(head);                                                                                                                                                        \
                    break;                                                                                                                                                                      \
                }                                                                                                                                                                               \
            }                                                                                                                                                                                   \
        }                                                                                                                                                                                       \
        if (elm) RB_COLOR(elm, field) = RB_BLACK;                                                                                                                                               \
    }                                                                                                                                                                                           \
                                                                                                                                                                                                \
    attr struct type *name##_RB_REMOVE(struct name *head, struct type *elm) {                                                                                                                   \
        struct type *child, *parent, *old = elm;                                                                                                                                                \
        int color;                                                                                                                                                                              \
        if (RB_LEFT(elm, field) == NULL)                                                                                                                                                        \
            child = RB_RIGHT(elm, field);                                                                                                                                                       \
        else if (RB_RIGHT(elm, field) == NULL)                                                                                                                                                  \
            child = RB_LEFT(elm, field);                                                                                                                                                        \
        else {                                                                                                                                                                                  \
            struct type *left;                                                                                                                                                                  \
            elm = RB_RIGHT(elm, field);                                                                                                                                                         \
            while ((left = RB_LEFT(elm, field)) != NULL) elm = left;                                                                                                                            \
            child = RB_RIGHT(elm, field);                                                                                                                                                       \
            parent = RB_PARENT(elm, field);                                                                                                                                                     \
            color = RB_COLOR(elm, field);                                                                                                                                                       \
            if (child) RB_PARENT(child, field) = parent;                                                                                                                                        \
            if (parent) {                                                                                                                                                                       \
                if (RB_LEFT(parent, field) == elm)                                                                                                                                              \
                    RB_LEFT(parent, field) = child;                                                                                                                                             \
                else                                                                                                                                                                            \
                    RB_RIGHT(parent, field) = child;                                                                                                                                            \
                RB_AUGMENT(parent);                                                                                                                                                             \
            } else                                                                                                                                                                              \
                RB_ROOT(head) = child;                                                                                                                                                          \
            if (RB_PARENT(elm, field) == old) parent = elm;                                                                                                                                     \
            (elm)->field = (old)->field;                                                                                                                                                        \
            if (RB_PARENT(old, field)) {                                                                                                                                                        \
                if (RB_LEFT(RB_PARENT(old, field), field) == old)                                                                                                                               \
                    RB_LEFT(RB_PARENT(old, field), field) = elm;                                                                                                                                \
                else                                                                                                                                                                            \
                    RB_RIGHT(RB_PARENT(old, field), field) = elm;                                                                                                                               \
                RB_AUGMENT(RB_PARENT(old, field));                                                                                                                                              \
            } else                                                                                                                                                                              \
                RB_ROOT(head) = elm;                                                                                                                                                            \
            RB_PARENT(RB_LEFT(old, field), field) = elm;                                                                                                                                        \
            if (RB_RIGHT(old, field)) RB_PARENT(RB_RIGHT(old, field), field) = elm;                                                                                                             \
            if (parent) {                                                                                                                                                                       \
                left = parent;                                                                                                                                                                  \
                do {                                                                                                                                                                            \
                    RB_AUGMENT(left);                                                                                                                                                           \
                } while ((left = RB_PARENT(left, field)) != NULL);                                                                                                                              \
            }                                                                                                                                                                                   \
            goto color;                                                                                                                                                                         \
        }                                                                                                                                                                                       \
        parent = RB_PARENT(elm, field);                                                                                                                                                         \
        color = RB_COLOR(elm, field);                                                                                                                                                           \
        if (child) RB_PARENT(child, field) = parent;                                                                                                                                            \
        if (parent) {                                                                                                                                                                           \
            if (RB_LEFT(parent, field) == elm)                                                                                                                                                  \
                RB_LEFT(parent, field) = child;                                                                                                                                                 \
            else                                                                                                                                                                                \
                RB_RIGHT(parent, field) = child;                                                                                                                                                \
            RB_AUGMENT(parent);                                                                                                                                                                 \
        } else                                                                                                                                                                                  \
            RB_ROOT(head) = child;                                                                                                                                                              \
    color:                                                                                                                                                                                      \
        if (color == RB_BLACK) name##_RB_REMOVE_COLOR(head, parent, child);                                                                                                                     \
        return (old);                                                                                                                                                                           \
    }                                                                                                                                                                                           \
                                                                                                                                                                                                \
    /* Inserts a node into the RB tree */                                                                                                                                                       \
    attr struct type *name##_RB_INSERT(struct name *head, struct type *elm) {                                                                                                                   \
        struct type *tmp;                                                                                                                                                                       \
        struct type *parent = NULL;                                                                                                                                                             \
        int comp = 0;                                                                                                                                                                           \
        tmp = RB_ROOT(head);                                                                                                                                                                    \
        while (tmp) {                                                                                                                                                                           \
            parent = tmp;                                                                                                                                                                       \
            comp = (cmp)(elm, parent);                                                                                                                                                          \
            if (comp < 0)                                                                                                                                                                       \
                tmp = RB_LEFT(tmp, field);                                                                                                                                                      \
            else if (comp > 0)                                                                                                                                                                  \
                tmp = RB_RIGHT(tmp, field);                                                                                                                                                     \
            else                                                                                                                                                                                \
                return (tmp);                                                                                                                                                                   \
        }                                                                                                                                                                                       \
        RB_SET(elm, parent, field);                                                                                                                                                             \
        if (parent != NULL) {                                                                                                                                                                   \
            if (comp < 0)                                                                                                                                                                       \
                RB_LEFT(parent, field) = elm;                                                                                                                                                   \
            else                                                                                                                                                                                \
                RB_RIGHT(parent, field) = elm;                                                                                                                                                  \
            RB_AUGMENT(parent);                                                                                                                                                                 \
        } else                                                                                                                                                                                  \
            RB_ROOT(head) = elm;                                                                                                                                                                \
        name##_RB_INSERT_COLOR(head, elm);                                                                                                                                                      \
        return (NULL);                                                                                                                                                                          \
    }                                                                                                                                                                                           \
                                                                                                                                                                                                \
    /* Finds the node with the same key as elm */                                                                                                                                               \
    attr struct type *name##_RB_FIND(struct name *head, struct type *elm) {                                                                                                                     \
        struct type *tmp = RB_ROOT(head);                                                                                                                                                       \
        int comp;                                                                                                                                                                               \
        while (tmp) {                                                                                                                                                                           \
            comp = cmp(elm, tmp);                                                                                                                                                               \
            if (comp < 0)                                                                                                                                                                       \
                tmp = RB_LEFT(tmp, field);                                                                                                                                                      \
            else if (comp > 0)                                                                                                                                                                  \
                tmp = RB_RIGHT(tmp, field);                                                                                                                                                     \
            else                                                                                                                                                                                \
                return (tmp);                                                                                                                                                                   \
        }                                                                                                                                                                                       \
        return (NULL);                                                                                                                                                                          \
    }                                                                                                                                                                                           \
                                                                                                                                                                                                \
    /* Finds the first node greater than or equal to the search key */                                                                                                                          \
    attr struct type *name##_RB_NFIND(struct name *head, struct type *elm) {                                                                                                                    \
        struct type *tmp = RB_ROOT(head);                                                                                                                                                       \
        struct type *res = NULL;                                                                                                                                                                \
        int comp;                                                                                                                                                                               \
        while (tmp) {                                                                                                                                                                           \
            comp = cmp(elm, tmp);                                                                                                                                                               \
            if (comp < 0) {                                                                                                                                                                     \
                res = tmp;                                                                                                                                                                      \
                tmp = RB_LEFT(tmp, field);                                                                                                                                                      \
            } else if (comp > 0)                                                                                                                                                                \
                tmp = RB_RIGHT(tmp, field);                                                                                                                                                     \
            else                                                                                                                                                                                \
                return (tmp);                                                                                                                                                                   \
        }                                                                                                                                                                                       \
        return (res);                                                                                                                                                                           \
    }                                                                                                                                                                                           \
                                                                                                                                                                                                \
    /* ARGSUSED */                                                                                                                                                                              \
    attr struct type *name##_RB_NEXT(struct type *elm) {                                                                                                                                        \
        if (RB_RIGHT(elm, field)) {                                                                                                                                                             \
            elm = RB_RIGHT(elm, field);                                                                                                                                                         \
            while (RB_LEFT(elm, field)) elm = RB_LEFT(elm, field);                                                                                                                              \
        } else {                                                                                                                                                                                \
            if (RB_PARENT(elm, field) && (elm == RB_LEFT(RB_PARENT(elm, field), field)))                                                                                                        \
                elm = RB_PARENT(elm, field);                                                                                                                                                    \
            else {                                                                                                                                                                              \
                while (RB_PARENT(elm, field) && (elm == RB_RIGHT(RB_PARENT(elm, field), field))) elm = RB_PARENT(elm, field);                                                                   \
                elm = RB_PARENT(elm, field);                                                                                                                                                    \
            }                                                                                                                                                                                   \
        }                                                                                                                                                                                       \
        return (elm);                                                                                                                                                                           \
    }                                                                                                                                                                                           \
                                                                                                                                                                                                \
    /* ARGSUSED */                                                                                                                                                                              \
    attr struct type *name##_RB_PREV(struct type *elm) {                                                                                                                                        \
        if (RB_LEFT(elm, field)) {                                                                                                                                                              \
            elm = RB_LEFT(elm, field);                                                                                                                                                          \
            while (RB_RIGHT(elm, field)) elm = RB_RIGHT(elm, field);                                                                                                                            \
        } else {                                                                                                                                                                                \
            if (RB_PARENT(elm, field) && (elm == RB_RIGHT(RB_PARENT(elm, field), field)))                                                                                                       \
                elm = RB_PARENT(elm, field);                                                                                                                                                    \
            else {                                                                                                                                                                              \
                while (RB_PARENT(elm, field) && (elm == RB_LEFT(RB_PARENT(elm, field), field))) elm = RB_PARENT(elm, field);                                                                    \
                elm = RB_PARENT(elm, field);                                                                                                                                                    \
            }                                                                                                                                                                                   \
        }                                                                                                                                                                                       \
        return (elm);                                                                                                                                                                           \
    }                                                                                                                                                                                           \
                                                                                                                                                                                                \
    attr struct type *name##_RB_MINMAX(struct name *head, int val) {                                                                                                                            \
        struct type *tmp = RB_ROOT(head);                                                                                                                                                       \
        struct type *parent = NULL;                                                                                                                                                             \
        while (tmp) {                                                                                                                                                                           \
            parent = tmp;                                                                                                                                                                       \
            if (val < 0)                                                                                                                                                                        \
                tmp = RB_LEFT(tmp, field);                                                                                                                                                      \
            else                                                                                                                                                                                \
                tmp = RB_RIGHT(tmp, field);                                                                                                                                                     \
        }                                                                                                                                                                                       \
        return (parent);                                                                                                                                                                        \
    }

#define RB_NEGINF -1
#define RB_INF 1

#define RB_INSERT(name, x, y) name##_RB_INSERT(x, y)
#define RB_REMOVE(name, x, y) name##_RB_REMOVE(x, y)
#define RB_FIND(name, x, y) name##_RB_FIND(x, y)
#define RB_NFIND(name, x, y) name##_RB_NFIND(x, y)
#define RB_NEXT(name, x, y) name##_RB_NEXT(y)
#define RB_PREV(name, x, y) name##_RB_PREV(y)
#define RB_MIN(name, x) name##_RB_MINMAX(x, RB_NEGINF)
#define RB_MAX(name, x) name##_RB_MINMAX(x, RB_INF)

#define RB_FOREACH(x, name, head) for ((x) = RB_MIN(name, head); (x) != NULL; (x) = name##_RB_NEXT(x))

#define RB_FOREACH_FROM(x, name, y) for ((x) = (y); ((x) != NULL) && ((y) = name##_RB_NEXT(x), (x) != NULL); (x) = (y))

#define RB_FOREACH_SAFE(x, name, head, y) for ((x) = RB_MIN(name, head); ((x) != NULL) && ((y) = name##_RB_NEXT(x), (x) != NULL); (x) = (y))

#define RB_FOREACH_REVERSE(x, name, head) for ((x) = RB_MAX(name, head); (x) != NULL; (x) = name##_RB_PREV(x))

#define RB_FOREACH_REVERSE_FROM(x, name, y) for ((x) = (y); ((x) != NULL) && ((y) = name##_RB_PREV(x), (x) != NULL); (x) = (y))

#define RB_FOREACH_REVERSE_SAFE(x, name, head, y) for ((x) = RB_MAX(name, head); ((x) != NULL) && ((y) = name##_RB_PREV(x), (x) != NULL); (x) = (y))

#endif

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LUACS_VERSION "1"

#define METANAMELEN 80
#define METANAME_LUACSTRUCT "luacstruct" LUACS_VERSION
#define METANAME_LUACSENUM "luacenum" LUACS_VERSION
#define METANAME_LUACTYPE "luactype" LUACS_VERSION "."
#define METANAME_LUACARRAY "luacarray" LUACS_VERSION
#define METANAME_LUACARRAYTYPE "luacarraytype" LUACS_VERSION
#define METANAME_LUACSTRUCTOBJ "luacstructobj" LUACS_VERSION
#define METANAME_LUACSENUMVAL "luacenumval" LUACS_VERSION

#define LUACS_REGISTRY_NAME "luacstruct_registry"

#if LUA_VERSION_NUM == 501
#define lua_rawlen(_x, _i) lua_objlen((_x), (_i))
#define lua_absindex(_L, _i) (((_i) > 0 || (_i) <= LUA_REGISTRYINDEX) ? (_i) : lua_gettop(_L) + (_i) + 1)
#endif

#ifdef LUACS_DEBUG
#define LUACS_DBG(...)                \
    do {                              \
        fprintf(stdout, __VA_ARGS__); \
        fputs("\n", stdout);          \
    } while (0 /*CONSTCOND*/)
#define LUACS_ASSERT(_L, _cond)                                    \
    do {                                                           \
        if (!(_cond)) {                                            \
            lua_pushfstring((_L),                                  \
                            "ASSERT(%s) failed "                   \
                            "in %s() at %s:%d",                    \
                            #_cond, __func__, __FILE__, __LINE__); \
            lua_error((_L));                                       \
        }                                                          \
    } while (0 /*CONSTCOND*/)
#else
#define LUACS_DBG(...) ((void)0)
#define LUACS_ASSERT(_L, _cond) ((void)0)
#endif

#ifndef MINIMUM
#define MINIMUM(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#endif
#ifndef MAXIMUM
#define MAXIMUM(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#endif

#ifndef nitems
#define nitems(_x) (sizeof(_x) / sizeof((_x)[0]))
#endif

#ifndef __unused
#define __unused __attribute__((__unused__))
#endif

struct luacstruct {
    const char *typename;
    char metaname[METANAMELEN];
    SPLAY_HEAD(luacstruct_fields, luacstruct_field)
    fields;
    TAILQ_HEAD(, luacstruct_field) sorted;
};

struct luacarraytype {
    const char *typename;
    char metaname[METANAMELEN];
    enum luacstruct_type type;
    size_t size;
    int nmemb;
    int typref;
    unsigned flags;
};

struct luacregeon {
    enum luacstruct_type type;
    int off;
    size_t size;
    int typref;
    unsigned flags;
};

struct luacstruct_field {
    enum luacstruct_type type;
    const char *fieldname;
    struct luacregeon regeon;
    int constval;
    int nmemb;
    unsigned flags;
    int ref;
    SPLAY_ENTRY(luacstruct_field) tree;
    TAILQ_ENTRY(luacstruct_field) queue;
};

struct luacobject {
    enum luacstruct_type type;
    struct luacstruct *cs;
    void* ptr;
    size_t size;
    int nmemb;
    int typref;
    int tblref; /* cache of members */
    unsigned flags;
};

struct luacenum {
    const char *enumname;
    char metaname[METANAMELEN];
    size_t valwidth;
    SPLAY_HEAD(luacenum_labels, luacenum_value)
    labels;
    SPLAY_HEAD(luacenum_values, luacenum_value)
    values;
    int func_get;
    int func_memberof;
};

struct luacenum_value {
    struct luacenum *ce;
    intmax_t value;
    int ref;
    const char *label;
    SPLAY_ENTRY(luacenum_value) treel;
    SPLAY_ENTRY(luacenum_value) treev;
};

static struct luacstruct *luacs_checkstruct(lua_State *, int);
static int luacs_struct__gc(lua_State *);
static struct luacstruct_field *luacs_declare(lua_State *, enum luacstruct_type, const char *, const char *, size_t, int, int, unsigned);
static int luacstruct_field_cmp(struct luacstruct_field *, struct luacstruct_field *);
static struct luacstruct_field *luacsfield_copy(lua_State *, struct luacstruct_field *);
static void luacstruct_field_free(lua_State *, struct luacstruct *, struct luacstruct_field *);
static int luacs_pushctype(lua_State *, enum luacstruct_type, const char *);
static int luacs_newarray0(lua_State *, enum luacstruct_type, int, size_t, int, unsigned, void *);
static int luacs_array__len(lua_State *);
static int luacs_array__index(lua_State *);
static int luacs_array__newindex(lua_State *);
static int luacs_array_copy(lua_State *);
static int luacs_array__next(lua_State *);
static int luacs_array__pairs(lua_State *);
static int luacs_array__gc(lua_State *);
static int luacs_newobject0(lua_State *, void *);
static int luacs_object__luacstructdump(struct lua_State *);
struct luacobj_compat;
static void luacs_object_compat(lua_State *, int, struct luacobj_compat *);
static int luacs_object__eq(lua_State *);
static int luacs_object__tostring(lua_State *);
static int luacs_object__index(lua_State *);
static int luacs_object__get(lua_State *, struct luacobject *, struct luacstruct_field *);
static int luacs_object__newindex(lua_State *);
static int luacs_object_copy(lua_State *);
static int luacs_object__next(lua_State *);
static int luacs_object__pairs(lua_State *);
static int luacs_object__gc(lua_State *);
static int luacs_pushregeon(lua_State *, struct luacobject *, struct luacregeon *);
static void luacs_pullregeon(lua_State *, struct luacobject *, struct luacregeon *, int);
struct luacenum *luacs_checkenum(lua_State *, int);
struct luacenum_value *luacs_enum_get0(struct luacenum *, intmax_t);
static int luacs_enum_get(lua_State *);
static int luacs_enum_memberof(lua_State *);
static int luacs_enum__index(lua_State *);
static int luacs_enum__pairs(lua_State *);
static int luacs_enum__next(lua_State *);
static int luacs_enum__gc(lua_State *);
static int luacs_enumvalue_tointeger(lua_State *);
static int luacs_enumvalue_label(lua_State *);
static int luacs_enumvalue__tostring(lua_State *);
static int luacs_enumvalue__gc(lua_State *);
static int luacs_enumvalue__eq(lua_State *);
static int luacs_enumvalue__lt(lua_State *);
static int luacenum_label_cmp(struct luacenum_value *, struct luacenum_value *);
static int luacenum_value_cmp(struct luacenum_value *, struct luacenum_value *);
static int luacs_ref(lua_State *);
static int luacs_getref(lua_State *, int);
static int luacs_unref(lua_State *, int);

SPLAY_PROTOTYPE(luacstruct_fields, luacstruct_field, tree, luacstruct_field_cmp);
SPLAY_PROTOTYPE(luacenum_labels, luacenum_value, treel, luacenum_label_cmp);
SPLAY_PROTOTYPE(luacenum_values, luacenum_value, treev, luacenum_value_cmp);

/* struct */
int luacs_newstruct0(lua_State *L, const char *tname, const char *supertname) {
    int ret;
    struct luacstruct *cs, *supercs = NULL;
    char metaname[METANAMELEN], buf[128];
    struct luacstruct_field *fieldf, *fieldt;

    if (supertname != NULL) {
        snprintf(metaname, sizeof(metaname), "%s%s", METANAME_LUACTYPE, supertname);
        lua_getfield(L, LUA_REGISTRYINDEX, metaname);
        if (lua_isnil(L, -1)) {
            snprintf(buf, sizeof(buf), "`%s' is not regisiterd", supertname);
            lua_pushstring(L, buf);
            lua_error(L);
            return (0); /* not reached */
        }
        supercs = luacs_checkstruct(L, -1);
    }

    snprintf(metaname, sizeof(metaname), "%s%s", METANAME_LUACTYPE, tname);
    lua_getfield(L, LUA_REGISTRYINDEX, metaname);
    if (!lua_isnil(L, -1)) {
        cs = luacs_checkstruct(L, -1);
        return (1);
    }
    lua_pop(L, 1);

    cs = lua_newuserdata(L, sizeof(struct luacstruct));
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, metaname);
    memcpy(cs->metaname, metaname, MINIMUM(sizeof(metaname), sizeof(cs->metaname)));

    cs->typename = (char*)index(cs->metaname, '.') + 1;
    SPLAY_INIT(&cs->fields);
    TAILQ_INIT(&cs->sorted);

    /* Inherit from super struct if specified */
    if (supercs != NULL) {
        TAILQ_FOREACH(fieldf, &supercs->sorted, queue) {
            if ((fieldt = luacsfield_copy(L, fieldf)) == NULL) {
                strerror_r(errno, buf, sizeof(buf));
                lua_pushstring(L, buf);
                lua_error(L);
                return (0); /* not reached */
            }
            TAILQ_INSERT_TAIL(&cs->sorted, fieldt, queue);
            SPLAY_INSERT(luacstruct_fields, &cs->fields, fieldt);
        }
    }

    if ((ret = luaL_newmetatable(L, METANAME_LUACSTRUCT)) != 0) {
        lua_pushcfunction(L, luacs_struct__gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    return (1);
}

int luacs_delstruct(lua_State *L, const char *tname) {
    char metaname[METANAMELEN];

    snprintf(metaname, sizeof(metaname), "%s%s", METANAME_LUACTYPE, tname);
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, metaname);

    return (0);
}

struct luacstruct *luacs_checkstruct(lua_State *L, int csidx) { return (luaL_checkudata(L, csidx, METANAME_LUACSTRUCT)); }

int luacs_struct__gc(lua_State *L) {
    struct luacstruct *cs;
    struct luacstruct_field *field;

    lua_settop(L, 1);
    cs = luacs_checkstruct(L, 1);
    if (cs) {
        while ((field = SPLAY_MIN(luacstruct_fields, &cs->fields)) != NULL) luacstruct_field_free(L, cs, field);
    }

    return (0);
}

int luacs_declare_field(lua_State *L, enum luacstruct_type _type, const char *tname, const char *name, size_t siz, int off, int nmemb, unsigned flags) {
    luacs_declare(L, _type, tname, name, siz, off, nmemb, flags);
    return (0);
}

struct luacstruct_field *luacs_declare(lua_State *L, enum luacstruct_type _type, const char *tname, const char *name, size_t siz, int off, int nmemb, unsigned flags) {
    struct luacstruct_field *field, *field0;
    struct luacstruct *cs;
    char buf[BUFSIZ];

    cs = luacs_checkstruct(L, -1);
    if ((field = calloc(1, sizeof(struct luacstruct_field))) == NULL) {
        strerror_r(errno, buf, sizeof(buf));
        lua_pushstring(L, buf);
        lua_error(L);
    }
    if ((field->fieldname = strdup(name)) == NULL) {
        free(field);
        strerror_r(errno, buf, sizeof(buf));
        lua_pushstring(L, buf);
        lua_error(L);
    }
    while ((field0 = SPLAY_FIND(luacstruct_fields, &cs->fields, field)) != NULL) luacstruct_field_free(L, cs, field0);
    field->regeon.type = _type;
    field->regeon.off = off;
    field->regeon.size = siz;
    field->regeon.flags = flags;
    field->nmemb = nmemb;
    field->flags = flags;
    switch (_type) {
        case LUACS_TOBJREF:
        case LUACS_TOBJENT:
        case LUACS_TENUM:
        case LUACS_TARRAY:
            luacs_pushctype(L, _type, tname);
            field->regeon.typref = luacs_ref(L);
            break;
        case LUACS_TINT64:
        case LUACS_TUINT64:
            if (sizeof(lua_Integer) < 8) {
                lua_pushliteral(L, "Lua runtime doesn't support 64bit integer");
                lua_error(L);
            }
        default:
            break;
    }
    field->type = (field->nmemb > 0) ? LUACS_TARRAY : _type;

    SPLAY_INSERT(luacstruct_fields, &cs->fields, field);
    TAILQ_FOREACH(field0, &cs->sorted, queue) {
        if (field->regeon.off < field0->regeon.off) break;
    }
    if (field0 == NULL)
        TAILQ_INSERT_TAIL(&cs->sorted, field, queue);
    else
        TAILQ_INSERT_BEFORE(field0, field, queue);

    return (field);
}

int luacs_declare_method(lua_State *L, const char *name, int (*func)(lua_State *)) {
    struct luacstruct_field *field;

    field = luacs_declare(L, LUACS_TMETHOD, NULL, name, 0, 0, 0, LUACS_FREADONLY);
    lua_pushcfunction(L, func);
    field->ref = luacs_ref(L);

    return (0);
}

int luacs_declare_const(lua_State *L, const char *name, int constval) {
    struct luacstruct_field *field;

    field = luacs_declare(L, LUACS_TCONST, NULL, name, 0, 0, 0, LUACS_FREADONLY);
    field->constval = constval;

    return (0);
}

int luacstruct_field_cmp(struct luacstruct_field *a, struct luacstruct_field *b) { return (strcmp(a->fieldname, b->fieldname)); }

struct luacstruct_field *luacsfield_copy(lua_State *L, struct luacstruct_field *from) {
    struct luacstruct_field *to;

    to = calloc(1, sizeof(struct luacstruct_field));
    if (to == NULL) return (NULL);
    if ((to->fieldname = strdup(from->fieldname)) == NULL) {
        free(to);
        return (NULL);
    }
    to->type = from->type;
    to->regeon = from->regeon;
    to->constval = from->constval;
    to->nmemb = from->nmemb;
    to->flags = from->flags;
    /* Update refs */
    if (from->regeon.typref != 0) {
        luacs_getref(L, from->regeon.typref);
        to->regeon.typref = luacs_ref(L);
    }
    if (from->ref != 0) {
        luacs_getref(L, from->ref);
        to->ref = luacs_ref(L);
    }

    return (to);
}

void luacstruct_field_free(lua_State *L, struct luacstruct *cs, struct luacstruct_field *field) {
    if (field) {
        if (field->regeon.typref > 0) luacs_unref(L, field->regeon.typref);
        if (field->ref != 0) luacs_unref(L, field->ref);
        TAILQ_REMOVE(&cs->sorted, field, queue);
        SPLAY_REMOVE(luacstruct_fields, &cs->fields, field);
        free((char *)field->fieldname);
    }
    free(field);
}

int luacs_pushctype(lua_State *L, enum luacstruct_type _type, const char *tname) {
    char metaname[METANAMELEN];

    snprintf(metaname, sizeof(metaname), "%s%s", METANAME_LUACTYPE, tname);
    lua_getfield(L, LUA_REGISTRYINDEX, metaname);
    if (lua_isnil(L, -1)) {
        if (_type == LUACS_TARRAY)
            lua_pushfstring(L, "array `%s' is not registered", tname);
        else
            lua_pushfstring(L, "`%s %s' is not registered", _type == LUACS_TENUM ? "enum" : "struct", tname);
        lua_error(L);
    }
    if (_type == LUACS_TARRAY)
        luaL_checkudata(L, -1, METANAME_LUACARRAYTYPE);
    else if (_type == LUACS_TENUM)
        luacs_checkenum(L, -1);
    else
        luacs_checkstruct(L, -1);

    return (1);
}

/* array */
int luacs_arraytype__gc(lua_State *L) {
    luaL_checkudata(L, -1, METANAME_LUACARRAYTYPE);

    return (0);
}

int luacs_newarraytype(lua_State *L, const char *tname, enum luacstruct_type _type, const char *membtname, size_t size, int nmemb, unsigned flags) {
    int ret;
    struct luacarraytype *cat;
    char metaname[METANAMELEN];

    snprintf(metaname, sizeof(metaname), "%s%s", METANAME_LUACTYPE, tname);
    lua_getfield(L, LUA_REGISTRYINDEX, metaname);
    if (!lua_isnil(L, -1)) {
        cat = luaL_checkudata(L, -1, METANAME_LUACARRAYTYPE);
        return (1);
    }
    lua_pop(L, 1);

    cat = lua_newuserdata(L, sizeof(struct luacarraytype));

    cat->type = _type;
    cat->size = size;
    cat->nmemb = nmemb;
    cat->flags = flags;

    switch (_type) {
        case LUACS_TOBJREF:
        case LUACS_TOBJENT:
        case LUACS_TARRAY:
            if (membtname == NULL) {
                lua_pushfstring(L,
                                "`membtname' argument must be "
                                "specified when creating an array of %s",
                                _type == LUACS_TENUM     ? "LUACS_TENUM"
                                : _type == LUACS_TOBJREF ? "LUACS_TOBJREF"
                                                         : "LUACS_TOBJENT");
                lua_error(L);
            }
            luacs_pushctype(L, _type, membtname);
            cat->typref = luacs_ref(L);
            break;
        default:
            break;
    }

    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, metaname);
    memcpy(cat->metaname, metaname, MINIMUM(sizeof(metaname), sizeof(cat->metaname)));

    cat->typename = index(cat->metaname, '.') + 1;
    if ((ret = luaL_newmetatable(L, METANAME_LUACARRAYTYPE)) != 0) {
        lua_pushcfunction(L, luacs_arraytype__gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    return (1);
}

int luacs_newarray(lua_State *L, enum luacstruct_type _type, const char *membtname, size_t size, int nmemb, unsigned flags, void *ptr) {
    int typref = 0;

    switch (_type) {
        case LUACS_TENUM:
        case LUACS_TOBJREF:
        case LUACS_TOBJENT:
        case LUACS_TARRAY:
            if (membtname == NULL) {
                lua_pushfstring(L,
                                "`membtname' argument must be "
                                "specified when creating an array of %s",
                                _type == LUACS_TENUM     ? "LUACS_TENUM"
                                : _type == LUACS_TOBJREF ? "LUACS_TOBJREF"
                                                         : "LUACS_TOBJENT");
                lua_error(L);
            }
            luacs_pushctype(L, _type, membtname);
            typref = luacs_ref(L);
            break;
        default:
            break;
    }
    return (luacs_newarray0(L, _type, typref, size, nmemb, flags, ptr));
}

int luacs_newarray0(lua_State *L, enum luacstruct_type _type, int typidx, size_t size, int nmemb, unsigned flags, void *ptr) {
    struct luacobject *obj;
    int ret, absidx;

    absidx = lua_absindex(L, typidx);

    if (ptr != NULL) {
        obj = lua_newuserdata(L, sizeof(struct luacobject));
        obj->ptr = ptr;
    } else {
        obj = lua_newuserdata(L, sizeof(struct luacobject) + size * nmemb);
        obj->ptr = (void *)(obj + 1);
    }
    obj->type = _type;
    obj->size = size;
    obj->nmemb = nmemb;
    obj->flags = flags;

    if (typidx != 0) {
        lua_pushvalue(L, absidx);
        obj->typref = luacs_ref(L);
    }

    if (_type == LUACS_TOBJREF || _type == LUACS_TOBJENT || _type == LUACS_TEXTREF || _type == LUACS_TARRAY) {
        lua_newtable(L);
        obj->tblref = luacs_ref(L);
    }
    if ((ret = luaL_newmetatable(L, METANAME_LUACARRAY)) != 0) {
        lua_pushcfunction(L, luacs_array__len);
        lua_setfield(L, -2, "__len");
        lua_pushcfunction(L, luacs_array__index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luacs_array__newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, luacs_array__next);
        lua_pushcclosure(L, luacs_array__pairs, 1);
        lua_setfield(L, -2, "__pairs");
        lua_pushcfunction(L, luacs_array__gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_setmetatable(L, -2);

    return (1);
}

int luacs_array__len(lua_State *L) {
    struct luacobject *obj;

    lua_settop(L, 1);
    obj = luaL_checkudata(L, 1, METANAME_LUACARRAY);
    lua_pushinteger(L, obj->nmemb);

    return (1);
}

int luacs_array__index(lua_State *L) {
    struct luacobject *obj;
    struct luacarraytype *cat;
    int idx;
    struct luacregeon regeon;
    void *ptr;

    lua_settop(L, 2);
    obj = luaL_checkudata(L, 1, METANAME_LUACARRAY);
    idx = luaL_checkinteger(L, 2);
    if (idx < 1 || obj->nmemb < idx) {
        lua_pushnil(L);
        return (1);
    }
    memset(&regeon, 0, sizeof(regeon));

    regeon.type = obj->type;
    regeon.off = (idx - 1) * obj->size;
    regeon.size = obj->size;
    regeon.typref = obj->typref;

    switch (obj->type) {
        default:
            return (luacs_pushregeon(L, obj, &regeon));
            break;
        case LUACS_TOBJREF:
        case LUACS_TOBJENT:
            if (obj->type == LUACS_TOBJENT)
                ptr = (char *)obj->ptr + regeon.off;
            else
                ptr = *(void **)((char *)obj->ptr + regeon.off);
            if (ptr == NULL)
                lua_pushnil(L);
            else {
                luacs_getref(L, obj->tblref);
                lua_rawgeti(L, -1, idx);
                if (lua_isnil(L, -1)) {
                    lua_pop(L, 1);
                    luacs_getref(L, obj->typref);
                    luacs_newobject0(L, ptr);
                    lua_pushvalue(L, -1);
                    lua_rawseti(L, -4, idx);
                    lua_remove(L, -2);
                }
                lua_remove(L, -2);
            }
            break;
        case LUACS_TEXTREF:
            luacs_getref(L, obj->tblref);
            lua_rawgeti(L, -1, idx);
            lua_remove(L, -2);
            break;
        case LUACS_TARRAY:
            ptr = (char *)obj->ptr + regeon.off;
            if (ptr == NULL)
                lua_pushnil(L);
            else {
                luacs_getref(L, obj->tblref);
                lua_rawgeti(L, -1, idx);
                if (lua_isnil(L, -1)) {
                    lua_pop(L, 1);
                    luacs_getref(L, obj->typref);
                    cat = luaL_checkudata(L, -1, METANAME_LUACARRAYTYPE);
                    lua_pop(L, 1);
                    if (cat->typref != 0) luacs_getref(L, cat->typref);
                    luacs_newarray0(L, cat->type, cat->typref, (cat->typref != 0) ? -1 : 0, cat->nmemb, cat->flags, (char *)obj->ptr + regeon.off);
                    if (cat->typref != 0) lua_remove(L, -2);
                    lua_pushvalue(L, -1);
                    lua_rawseti(L, -3, idx);
                }
                lua_remove(L, -2);
            }
            break;
    }

    return (1);
}

int luacs_array__newindex(lua_State *L) {
    struct luacobject *obj, *ano = NULL;
    int idx;
    struct luacregeon regeon;
    struct luacstruct *cs0;

    lua_settop(L, 3);
    obj = luaL_checkudata(L, 1, METANAME_LUACARRAY);
    idx = luaL_checkinteger(L, 2);
    if (idx < 1 || obj->nmemb < idx) { /* out of the range */
        lua_pushfstring(L, "array index %d out of the range 1:%d", idx, obj->nmemb);
        lua_error(L);
    }
    regeon.type = obj->type;
    regeon.off = (idx - 1) * obj->size;
    regeon.size = obj->size;
    regeon.typref = obj->typref;
    regeon.flags = obj->flags;

    if ((obj->flags & LUACS_FREADONLY) != 0) {
    readonly:
        lua_pushliteral(L, "array is readonly");
        lua_error(L);
    }
    switch (regeon.type) {
        default:
            luacs_pullregeon(L, obj, &regeon, 3);
            break;
        case LUACS_TSTRPTR:
            goto readonly;
        case LUACS_TOBJREF:
        case LUACS_TOBJENT:
            /* get c struct of the field */
            luacs_getref(L, regeon.typref);
            cs0 = luacs_checkstruct(L, -1);
            lua_pop(L, 1);
            /* given instance of struct */
            if (regeon.type == LUACS_TOBJENT || !lua_isnil(L, 3)) ano = luaL_checkudata(L, 3, METANAME_LUACSTRUCTOBJ);
            if (ano != NULL && cs0 != ano->cs) {
                lua_pushfstring(L, "must be an instance of `struct %s'", cs0->typename);
                lua_error(L);
            }
            if (regeon.type == LUACS_TOBJENT) {
                lua_pushcfunction(L, luacs_object_copy);

                /* ca't assume the object is cached */
                lua_pushcfunction(L, luacs_array__index);
                lua_pushvalue(L, 1);
                lua_pushinteger(L, 2);
                lua_call(L, 2, 1);

                lua_pushvalue(L, 3);
                lua_call(L, 2, 0);
            } else {
                *(void **)((char *)obj->ptr + regeon.off) = ano ? ano->ptr : NULL;
                /* use the same object */
                luacs_getref(L, obj->tblref);
                lua_pushvalue(L, 3);
                lua_rawseti(L, -2, idx);
                lua_pop(L, 1);
            }
            break;
        case LUACS_TEXTREF:
            luacs_getref(L, obj->tblref);
            lua_pushvalue(L, 3);
            lua_rawseti(L, -2, idx);
            lua_pop(L, 1);
            break;
        case LUACS_TARRAY:
            lua_pushcfunction(L, luacs_array_copy);

            lua_pushcfunction(L, luacs_array__index);
            lua_pushvalue(L, 1);
            lua_pushinteger(L, 2);
            lua_call(L, 2, 1);

            lua_pushvalue(L, 3);
            lua_call(L, 2, 0);
            break;
    }

    return (0);
}

int luacs_array_copy(lua_State *L) {
    struct luacobject *l, *r;
    struct luacregeon regeon;
    int idx;

    lua_settop(L, 2);
    l = luaL_checkudata(L, 1, METANAME_LUACARRAY);
    r = luaL_checkudata(L, 2, METANAME_LUACARRAY);

    if (l->type != r->type) {
        lua_pushliteral(L, "can't copy between arrays of a different type");
        lua_error(L);
    }
    if (l->nmemb != r->nmemb) {
        lua_pushliteral(L, "can't copy between arrays which size are different");
        lua_error(L);
    }
    switch (l->type) {
        default:
            for (idx = 1; idx <= l->nmemb; idx++) {
                regeon.type = l->type;
                regeon.off = (idx - 1) * l->size;
                regeon.size = l->size;
                regeon.typref = l->typref;

                lua_pushcfunction(L, luacs_array__index);
                lua_pushvalue(L, 2);
                lua_pushinteger(L, idx);
                lua_call(L, 2, 1);

                luacs_pullregeon(L, l, &regeon, -1);
                lua_pop(L, 1);
            }
            break;
        case LUACS_TOBJREF:
            luacs_getref(L, l->tblref);
            for (idx = 1; idx <= l->nmemb; idx++) {
                /* use the same pointer */
                *(void **)((char *)l->ptr + (idx - 1) * l->size) = *(void **)((char *)r->ptr + (idx - 1) * r->size);

                lua_pushcfunction(L, luacs_array__index);
                lua_pushvalue(L, 2);
                lua_pushinteger(L, idx);
                lua_call(L, 2, 1);

                lua_rawseti(L, -2, idx);
            }
            lua_pop(L, 1);
            break;
        case LUACS_TOBJENT:
            for (idx = 1; idx <= l->nmemb; idx++) {
                lua_pushcfunction(L, luacs_object_copy);

                lua_pushcfunction(L, luacs_array__index);
                lua_pushvalue(L, 1);
                lua_pushinteger(L, idx);
                lua_call(L, 2, 1);

                lua_pushcfunction(L, luacs_array__index);
                lua_pushvalue(L, 2);
                lua_pushinteger(L, idx);
                lua_call(L, 2, 1);

                lua_call(L, 2, 0);
            }
            break;
        case LUACS_TEXTREF:
            luacs_getref(L, l->tblref);
            for (idx = 1; idx <= l->nmemb; idx++) {
                lua_pushcfunction(L, luacs_array__index);
                lua_pushvalue(L, 2);
                lua_pushinteger(L, idx);
                lua_call(L, 2, 1);

                lua_rawseti(L, -2, idx);
            }
            lua_pop(L, 1);
            break;
        case LUACS_TARRAY:
            for (idx = 1; idx <= l->nmemb; idx++) {
                lua_pushcfunction(L, luacs_array_copy);

                lua_pushcfunction(L, luacs_array__index);
                lua_pushvalue(L, 1);
                lua_pushinteger(L, idx);
                lua_call(L, 2, 1);

                lua_pushcfunction(L, luacs_array__index);
                lua_pushvalue(L, 2);
                lua_pushinteger(L, 2);
                lua_call(L, 2, 1);

                lua_call(L, 2, 0);
            }
            break;
    }

    return (0);
}

int luacs_array__next(lua_State *L) {
    struct luacobject *obj;
    int idx = 0;

    lua_settop(L, 2);
    obj = luaL_checkudata(L, 1, METANAME_LUACARRAY);
    if (!lua_isnil(L, 2)) idx = luaL_checkinteger(L, 2);

    idx++;
    if (idx < 1 || obj->nmemb < idx) {
        lua_pushnil(L);
        return (1);
    }
    lua_pushinteger(L, idx);
    lua_pushcfunction(L, luacs_array__index);
    lua_pushvalue(L, 1);
    lua_pushinteger(L, idx);
    lua_call(L, 2, 1);
    return (2);
}

int luacs_array__pairs(lua_State *L) {
    lua_settop(L, 1);
    luaL_checkudata(L, 1, METANAME_LUACARRAY);

    lua_pushvalue(L, lua_upvalueindex(1));
    lua_pushvalue(L, 1);
    lua_pushnil(L);

    return (3);
}

int luacs_array__gc(lua_State *L) {
    struct luacobject *obj;

    lua_settop(L, 1);
    obj = luaL_checkudata(L, 1, METANAME_LUACARRAY);
    if (obj->typref != 0) luacs_unref(L, obj->typref);

    return (0);
}

/* object */
int luacs_newobject(lua_State *L, const char *tname, void *ptr) {
    int ret;
    char metaname[METANAMELEN];

    snprintf(metaname, sizeof(metaname), "%s%s", METANAME_LUACTYPE, tname);
    lua_getfield(L, LUA_REGISTRYINDEX, metaname);

    ret = luacs_newobject0(L, ptr);
    lua_remove(L, -2);

    return (ret);
}

int luacs_newobject0(lua_State *L, void *ptr) {
    struct luacobject *obj;
    struct luacstruct *cs;
    struct luacstruct_field *field;
    int ret;
    size_t objsiz = 0;

    cs = luacs_checkstruct(L, -1);
    if (ptr != NULL) {
        obj = lua_newuserdata(L, sizeof(struct luacobject));
        obj->ptr = ptr;
    } else {
        TAILQ_FOREACH(field, &cs->sorted, queue) { objsiz = MAXIMUM(objsiz, field->regeon.off + (field->nmemb == 0 ? 1 : field->nmemb) * field->regeon.size); }
        obj = lua_newuserdata(L, sizeof(struct luacobject) + objsiz);
        obj->ptr = (void *)(obj + 1);
    }
    obj->cs = cs;
    lua_pushvalue(L, -2);
    obj->typref = luacs_ref(L);
    if ((ret = luaL_newmetatable(L, METANAME_LUACSTRUCTOBJ)) != 0) {
        lua_pushcfunction(L, luacs_object__index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luacs_object__newindex);
        lua_setfield(L, -2, "__newindex");
        lua_pushcfunction(L, luacs_object__next);
        lua_pushcclosure(L, luacs_object__pairs, 1);
        lua_setfield(L, -2, "__pairs");
        lua_pushcfunction(L, luacs_object__gc);
        lua_setfield(L, -2, "__gc");
        lua_pushcfunction(L, luacs_object__tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, luacs_object__eq);
        lua_setfield(L, -2, "__eq");
        lua_pushcfunction(L, luacs_object__luacstructdump);
        lua_setfield(L, -2, "__luacstructdump");
    }
    lua_setmetatable(L, -2);
    lua_newtable(L);

    obj->tblref = luacs_ref(L);

    return (1);
}

int luacs_object__luacstructdump(struct lua_State *L) {
    struct luacobject *obj;

    lua_settop(L, 1);
    obj = luaL_checkudata(L, 1, METANAME_LUACSTRUCTOBJ);
    lua_pushlightuserdata(L, obj->ptr);
    lua_pushstring(L, obj->cs->typename);

    return (2);
}

struct luacobj_compat {
    void *ptr;
    const char *typ;
};

void *luacs_object_pointer(lua_State *L, int ref, const char *typename) {
    struct luacobj_compat compat;

    memset(&compat, 0, sizeof(compat));
    luacs_object_compat(L, ref, &compat);
    if (typename == NULL || (compat.typ != NULL && strcmp(compat.typ, typename) == 0)) return (compat.ptr);

    return (NULL);
}

void luacs_object_compat(lua_State *L, int ref, struct luacobj_compat *compat) {
    lua_pushvalue(L, ref);
    if (lua_getmetatable(L, -1)) {
        lua_getfield(L, -1, "__luacstructdump");
        if (!lua_isnil(L, -1)) {
            lua_pushvalue(L, -3);
            lua_call(L, 1, 2);
            compat->typ = lua_tostring(L, -1);
            compat->ptr = lua_touserdata(L, -2);
            lua_pop(L, 2);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

int luacs_object__eq(lua_State *L) {
    struct luacobject *obja, *objb;
    void *ptra, *ptrb;

    lua_settop(L, 2);

    ptra = luacs_object_pointer(L, 1, NULL);
    ptrb = luacs_object_pointer(L, 2, NULL);
    if (ptra == NULL || ptrb == NULL) {
        lua_pushboolean(L, false);
        return (1);
    }
    obja = luaL_checkudata(L, 1, METANAME_LUACSTRUCTOBJ);
    objb = luaL_checkudata(L, 2, METANAME_LUACSTRUCTOBJ);
    if (ptra == ptrb) {     /* the same pointer */
        if (obja == objb || /* the same type */
            strcmp(obja->cs->typename, objb->cs->typename) == 0) {
            lua_pushboolean(L, true);
            return (1);
        }
    }
    lua_getfield(L, 1, "__eq");
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushboolean(L, false);
        return (1);
    }
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_call(L, 2, 1);

    return (1);
}

int luacs_object__tostring(lua_State *L) {
    struct luacobject *obj;
    char buf[BUFSIZ];

    lua_settop(L, 1);
    obj = luaL_checkudata(L, 1, METANAME_LUACSTRUCTOBJ);

    lua_getfield(L, 1, "__tostring");
    if (!lua_isnil(L, -1)) {
        lua_pushvalue(L, 1);
        lua_call(L, 1, 1);
    } else {
        snprintf(buf, sizeof(buf), "struct %s: %p", obj->cs->typename, obj->ptr);
        lua_pushstring(L, buf);
    }

    return (1);
}

int luacs_object_typename(lua_State *L) {
    struct luacobj_compat compat;

    memset(&compat, 0, sizeof(compat));
    lua_settop(L, 1);
    luacs_object_compat(L, 1, &compat);
    if (compat.typ != NULL)
        lua_pushstring(L, compat.typ);
    else
        lua_pushnil(L);

    return (1);
}

int luacs_object__index(lua_State *L) {
    struct luacobject *obj;
    struct luacstruct_field fkey, *field;

    lua_settop(L, 2);
    obj = luaL_checkudata(L, 1, METANAME_LUACSTRUCTOBJ);
    fkey.fieldname = luaL_checkstring(L, 2);
    if ((field = SPLAY_FIND(luacstruct_fields, &obj->cs->fields, &fkey)) != NULL)
        return (luacs_object__get(L, obj, field));
    else
        lua_pushnil(L);
    return (1);
}

int luacs_object__get(lua_State *L, struct luacobject *obj, struct luacstruct_field *field) {
    void *ptr;

    switch (field->type) {
        default:
            return (luacs_pushregeon(L, obj, &field->regeon));
        case LUACS_TOBJREF:
        case LUACS_TOBJENT:
            if (field->type == LUACS_TOBJENT)
                ptr = (char *)obj->ptr + field->regeon.off;
            else
                ptr = *(void **)((char *)obj->ptr + field->regeon.off);
            if (ptr == NULL)
                lua_pushnil(L);
            else {
                struct luacobject *cache = NULL;

                luacs_getref(L, obj->tblref);
                lua_getfield(L, -1, field->fieldname);
                if (lua_isnil(L, -1))
                    lua_pop(L, 1);
                else { /* has a cache */
                    cache = luaL_checkudata(L, -1, METANAME_LUACSTRUCTOBJ);
                    /* cached reference may be staled */
                    if (field->type == LUACS_TOBJREF && cache->ptr != *(void **)((char *)obj->ptr + field->regeon.off)) {
                        lua_pop(L, 1);
                        lua_pushnil(L);
                        lua_setfield(L, -2, field->fieldname);
                        cache = NULL;
                    }
                }
                if (cache == NULL) {
                    luacs_getref(L, field->regeon.typref);
                    luacs_newobject0(L, ptr);
                    lua_pushvalue(L, -1);
                    lua_setfield(L, -4, field->fieldname);
                    lua_remove(L, -2);
                }
                lua_remove(L, -2);
            }
            break;
        case LUACS_TEXTREF:
            luacs_getref(L, obj->tblref);
            lua_getfield(L, -1, field->fieldname);
            lua_remove(L, -2);
            break;
        case LUACS_TARRAY:
            /* use the cache if any */
            luacs_getref(L, obj->tblref);
            lua_getfield(L, -1, field->fieldname);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                if (field->regeon.typref != 0) luacs_getref(L, field->regeon.typref);
                luacs_newarray0(L, field->regeon.type, (field->regeon.typref != 0) ? -1 : 0, field->regeon.size, field->nmemb, field->flags, (char *)obj->ptr + field->regeon.off);
                if (field->regeon.typref != 0) lua_remove(L, -2);
                lua_pushvalue(L, -1);
                lua_setfield(L, -3, field->fieldname);
            }
            lua_remove(L, -2);
            break;
        case LUACS_TMETHOD:
            luacs_getref(L, field->ref);
            break;
        case LUACS_TCONST:
            lua_pushinteger(L, field->constval);
            break;
    }

    return (1);
}

int luacs_object__newindex(lua_State *L) {
    struct luacstruct *cs0;
    struct luacobject *obj, *ano = NULL;
    struct luacstruct_field fkey, *field;

    lua_settop(L, 3);
    obj = luaL_checkudata(L, 1, METANAME_LUACSTRUCTOBJ);
    fkey.fieldname = luaL_checkstring(L, 2);
    if ((field = SPLAY_FIND(luacstruct_fields, &obj->cs->fields, &fkey)) != NULL) {
        if ((field->flags & LUACS_FREADONLY) != 0) {
        readonly:
            lua_pushfstring(L, "field `%s' is readonly", field->fieldname);
            lua_error(L);
        }
        switch (field->type) {
            default:
                luacs_pullregeon(L, obj, &field->regeon, 3);
                break;
            case LUACS_TSTRPTR:
                goto readonly;
            case LUACS_TOBJREF:
            case LUACS_TOBJENT:
                /* get c struct of the field */
                luacs_getref(L, field->regeon.typref);
                cs0 = luacs_checkstruct(L, -1);
                lua_pop(L, 1);
                if (field->regeon.type == LUACS_TOBJENT || !lua_isnil(L, 3)) /* given instance of struct */
                    ano = luaL_checkudata(L, 3, METANAME_LUACSTRUCTOBJ);
                /* given instance of struct */
                if (ano != NULL && cs0 != ano->cs) {
                    lua_pushfstring(L,
                                    "`%s' field must be an instance of "
                                    "`struct %s'",
                                    field->fieldname, cs0->typename);
                    lua_error(L);
                }
                if (field->regeon.type == LUACS_TOBJENT) {
                    lua_pushcfunction(L, luacs_object_copy);
                    lua_getfield(L, 1, field->fieldname);
                    lua_pushvalue(L, 3);
                    lua_call(L, 2, 0);
                } else {
                    *(void **)((char *)obj->ptr + field->regeon.off) = ano != NULL ? ano->ptr : NULL;
                    /* use the same object */
                    luacs_getref(L, obj->tblref);
                    lua_pushvalue(L, 3);
                    lua_setfield(L, -2, field->fieldname);
                    lua_pop(L, 1);
                }
                break;
            case LUACS_TEXTREF:
                luacs_getref(L, obj->tblref);
                lua_pushvalue(L, 3);
                lua_setfield(L, -2, field->fieldname);
                lua_pop(L, 1);
                break;
            case LUACS_TARRAY:
                lua_pushcfunction(L, luacs_array_copy);
                lua_getfield(L, 1, field->fieldname);
                lua_pushvalue(L, 3);
                lua_call(L, 2, 0);
                break;
        }
    } else {
        lua_pushfstring(L, "`struct %s' doesn't have field `%s'", obj->cs->typename, fkey.fieldname);
        lua_error(L);
    }

    return (0);
}

int luacs_object_copy(lua_State *L) {
    struct luacobject *l, *r;
    struct luacstruct_field *field;

    lua_settop(L, 2);
    l = luaL_checkudata(L, 1, METANAME_LUACSTRUCTOBJ);
    r = luaL_checkudata(L, 2, METANAME_LUACSTRUCTOBJ);
    if (l->cs != r->cs) {
        lua_pushfstring(L,
                        "copying from `struct %s' instance to `struct %s' "
                        "instance is not supported",
                        l->cs->typename, r->cs->typename);
        lua_error(L);
    }

    TAILQ_FOREACH(field, &l->cs->sorted, queue) {
        if (field->regeon.size > 0)
            memcpy((char *)l->ptr + field->regeon.off, (char *)r->ptr + field->regeon.off, field->regeon.size);
        else if (field->regeon.type == LUACS_TOBJREF || field->regeon.type == LUACS_TOBJENT || field->regeon.type == LUACS_TEXTREF) {
            /* l[fieldname] = r[fieldname] */
            lua_getfield(L, 2, field->fieldname);
            lua_setfield(L, 1, field->fieldname);
        }
    }

    return (0);
}

int luacs_object__next(lua_State *L) {
    struct luacobject *obj;
    struct luacstruct_field *field, fkey;

    lua_settop(L, 2);
    obj = luaL_checkudata(L, 1, METANAME_LUACSTRUCTOBJ);
    if (lua_isnil(L, 2))
        field = TAILQ_FIRST(&obj->cs->sorted);
    else {
        fkey.fieldname = luaL_checkstring(L, 2);
        field = SPLAY_FIND(luacstruct_fields, &obj->cs->fields, &fkey);
        if (field != NULL) field = TAILQ_NEXT(field, queue);
    }
    if (field == NULL) {
        lua_pushnil(L);
        return (1);
    }
    lua_pushstring(L, field->fieldname);
    luacs_object__get(L, obj, field);

    return (2);
}

int luacs_object__pairs(lua_State *L) {
    lua_settop(L, 1);
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_pushvalue(L, 1);
    lua_pushnil(L);

    return (3);
}

int luacs_object__gc(lua_State *L) {
    struct luacobject *obj;
    struct luacstruct_field fkey, *field;

    lua_settop(L, 1);
    obj = luaL_checkudata(L, 1, METANAME_LUACSTRUCTOBJ);
    fkey.fieldname = "__gc";
    if ((field = SPLAY_FIND(luacstruct_fields, &obj->cs->fields, &fkey)) != NULL && field->type == LUACS_TMETHOD) {
        luacs_getref(L, field->ref);
        lua_pushvalue(L, 1);
        lua_pcall(L, 1, 0, 0);
    }
    luacs_unref(L, obj->typref);
    luacs_unref(L, obj->tblref);

    return (0);
}

void *luacs_checkobject(lua_State *L, int idx, const char *typename) {
    struct luacobj_compat compat;

    memset(&compat, 0, sizeof(compat));
    luacs_object_compat(L, idx, &compat);

    if (compat.typ != NULL && strcmp(compat.typ, typename) == 0) return (compat.ptr);

    luaL_error(L, "%s expected, got %s", typename, (compat.typ != NULL) ? compat.typ : luaL_typename(L, idx));
    /* NOTREACHED */
    LUACS_ASSERT(L, 0);
    abort();
}

/* regeon */
int luacs_pushregeon(lua_State *L, struct luacobject *obj, struct luacregeon *regeon) {
    intmax_t ival;
    uintmax_t uval;

    switch (regeon->type) {
        case LUACS_TINT8:
            lua_pushinteger(L, *(int8_t *)((char *)obj->ptr + regeon->off));
            break;
        case LUACS_TINT16:
            ival = *(int16_t *)((char *)obj->ptr + regeon->off);
            if ((regeon->flags & LUACS_FENDIAN) == 0)
                lua_pushinteger(L, ival);
            else if ((regeon->flags & LUACS_FENDIANBIG) != 0)
                lua_pushinteger(L, be16toh(ival));
            else
                lua_pushinteger(L, le16toh(ival));
            break;
        case LUACS_TINT32:
            ival = *(int32_t *)((char *)obj->ptr + regeon->off);
            if ((regeon->flags & LUACS_FENDIAN) == 0)
                lua_pushinteger(L, ival);
            else if ((regeon->flags & LUACS_FENDIANBIG) != 0)
                lua_pushinteger(L, be32toh(ival));
            else
                lua_pushinteger(L, le32toh(ival));
            break;
        case LUACS_TINT64:
            ival = *(int64_t *)((char *)obj->ptr + regeon->off);
            if ((regeon->flags & LUACS_FENDIAN) == 0)
                lua_pushinteger(L, ival);
            else if ((regeon->flags & LUACS_FENDIANBIG) != 0)
                lua_pushinteger(L, be64toh(ival));
            else
                lua_pushinteger(L, le64toh(ival));
            break;
        case LUACS_TUINT8:
            lua_pushinteger(L, *(uint8_t *)((char *)obj->ptr + regeon->off));
            break;
        case LUACS_TUINT16:
            uval = *(uint16_t *)((char *)obj->ptr + regeon->off);
            if ((regeon->flags & LUACS_FENDIAN) == 0)
                lua_pushinteger(L, uval);
            else if ((regeon->flags & LUACS_FENDIANBIG) != 0)
                lua_pushinteger(L, be16toh(uval));
            else
                lua_pushinteger(L, le16toh(uval));
            break;
        case LUACS_TUINT32:
            uval = *(uint32_t *)((char *)obj->ptr + regeon->off);
            if ((regeon->flags & LUACS_FENDIAN) == 0)
                lua_pushinteger(L, uval);
            else if ((regeon->flags & LUACS_FENDIANBIG) != 0)
                lua_pushinteger(L, be32toh(uval));
            else
                lua_pushinteger(L, le32toh(uval));
            break;
        case LUACS_TUINT64:
            uval = *(uint64_t *)((char *)obj->ptr + regeon->off);
            if ((regeon->flags & LUACS_FENDIAN) == 0)
                lua_pushinteger(L, uval);
            else if ((regeon->flags & LUACS_FENDIANBIG) != 0)
                lua_pushinteger(L, be64toh(uval));
            else
                lua_pushinteger(L, le64toh(uval));
            break;
        case LUACS_TBOOL:
            lua_pushboolean(L, *(bool *)((char *)obj->ptr + regeon->off));
            break;
        case LUACS_TSTRING:
            lua_pushstring(L, (const char *)((char *)obj->ptr + regeon->off));
            break;
        case LUACS_TSTRPTR:
            lua_pushstring(L, *(const char **)((char *)obj->ptr + regeon->off));
            break;
        case LUACS_TENUM: {
            intmax_t value;
            struct luacenum_value *val;
            struct luacenum *ce;
            switch (regeon->size) {
                case 1:
                    value = *(int8_t *)((char *)obj->ptr + regeon->off);
                    break;
                case 2:
                    value = *(int16_t *)((char *)obj->ptr + regeon->off);
                    break;
                case 4:
                    value = *(int32_t *)((char *)obj->ptr + regeon->off);
                    break;
                case 8:
                    value = *(int64_t *)((char *)obj->ptr + regeon->off);
                    break;
                default:
                    luaL_error(L, "%s: obj is broken", __func__);
                    abort();
            }
            luacs_getref(L, regeon->typref);
            ce = luacs_checkenum(L, -1);
            lua_pop(L, 1);
            val = luacs_enum_get0(ce, value);
            if (val == NULL)
                lua_pushinteger(L, value);
            else
                luacs_getref(L, val->ref);
            break;
        }
        case LUACS_TBYTEARRAY:
            lua_pushlstring(L, (char *)obj->ptr + regeon->off, regeon->size);
            break;
        default:
            lua_pushnil(L);
            break;
    }

    return (1);
}

void luacs_pullregeon(lua_State *L, struct luacobject *obj, struct luacregeon *regeon, int idx) {
    size_t siz;
    int absidx;
    intmax_t ival;
    uintmax_t uval;

    absidx = lua_absindex(L, idx);

    switch (regeon->type) {
        case LUACS_TINT8:
            *(int8_t *)((char *)obj->ptr + regeon->off) = lua_tointeger(L, absidx);
            break;
        case LUACS_TUINT8:
            *(uint8_t *)((char *)obj->ptr + regeon->off) = lua_tointeger(L, absidx);
            break;
        case LUACS_TINT16:
            ival = lua_tointeger(L, absidx);
            if ((regeon->flags & LUACS_FENDIANBIG) != 0)
                ival = htobe16(ival);
            else if ((regeon->flags & LUACS_FENDIANLITTLE) != 0)
                ival = htole16(ival);
            *(int16_t *)((char *)obj->ptr + regeon->off) = ival;
            break;
        case LUACS_TUINT16:
            uval = lua_tointeger(L, absidx);
            if ((regeon->flags & LUACS_FENDIANBIG) != 0)
                uval = htobe16(uval);
            else if ((regeon->flags & LUACS_FENDIANLITTLE) != 0)
                uval = htole16(uval);
            *(uint16_t *)((char *)obj->ptr + regeon->off) = uval;
            break;
        case LUACS_TINT32:
            ival = lua_tointeger(L, absidx);
            if ((regeon->flags & LUACS_FENDIANBIG) != 0)
                ival = htobe32(ival);
            else if ((regeon->flags & LUACS_FENDIANLITTLE) != 0)
                ival = htole32(ival);
            *(int32_t *)((char *)obj->ptr + regeon->off) = ival;
            break;
        case LUACS_TUINT32:
            uval = lua_tointeger(L, absidx);
            if ((regeon->flags & LUACS_FENDIANBIG) != 0)
                uval = htobe32(uval);
            else if ((regeon->flags & LUACS_FENDIANLITTLE) != 0)
                uval = htole32(uval);
            *(uint32_t *)((char *)obj->ptr + regeon->off) = uval;
            break;
        case LUACS_TINT64:
            ival = lua_tointeger(L, absidx);
            if ((regeon->flags & LUACS_FENDIANBIG) != 0)
                ival = htobe64(ival);
            else if ((regeon->flags & LUACS_FENDIANLITTLE) != 0)
                ival = htole64(ival);
            *(int64_t *)((char *)obj->ptr + regeon->off) = ival;
            break;
        case LUACS_TUINT64:
            uval = lua_tointeger(L, absidx);
            if ((regeon->flags & LUACS_FENDIANBIG) != 0)
                uval = htobe64(uval);
            else if ((regeon->flags & LUACS_FENDIANLITTLE) != 0)
                uval = htole64(uval);
            *(uint64_t *)((char *)obj->ptr + regeon->off) = uval;
            break;
        case LUACS_TBOOL:
            *(bool *)((char *)obj->ptr + regeon->off) = lua_toboolean(L, absidx);
            break;
        case LUACS_TENUM: {
            struct luacenum *ce;
            struct luacenum_value *val;
            void *ptr;
            intmax_t value;

            luacs_getref(L, regeon->typref);
            ce = luacs_checkenum(L, -1);
            if (lua_type(L, absidx) == LUA_TNUMBER) {
                value = lua_tointeger(L, absidx);
                if (luacs_enum_get0(ce, value) == NULL) {
                    lua_pushfstring(L, "must be a valid integer for `enum %s'", ce->enumname);
                    lua_error(L);
                }
            } else if (lua_type(L, absidx) == LUA_TUSERDATA) {
                lua_pushcfunction(L, luacs_enum_memberof);
                lua_pushvalue(L, -2);
                lua_pushvalue(L, absidx);
                lua_call(L, 2, 1);
                if (!lua_toboolean(L, -1)) luaL_error(L, "must be a member of `enum %s", ce->enumname);
                val = lua_touserdata(L, absidx);
                value = val->value;
            } else {
                luaL_error(L, "must be a member of `enum %s", ce->enumname);
                /* NOTREACHED */
                abort();
            }

            ptr = (char *)obj->ptr + regeon->off;
            switch (regeon->size) {
                case 1:
                    *(int8_t *)(ptr) = value;
                    break;
                case 2:
                    *(int16_t *)(ptr) = value;
                    break;
                case 4:
                    *(int32_t *)(ptr) = value;
                    break;
                case 8:
                    *(int64_t *)(ptr) = value;
                    break;
            }
        } break;
        case LUACS_TSTRING:
        case LUACS_TBYTEARRAY:
            luaL_checkstring(L, absidx);
            siz = lua_rawlen(L, absidx);
            luaL_argcheck(L, siz < regeon->size, absidx, "too long");
            memcpy((char *)obj->ptr + regeon->off, lua_tostring(L, absidx), MINIMUM(siz, regeon->size));
            if (regeon->type == LUACS_TSTRING) *(char *)((char *)obj->ptr + regeon->off + regeon->size - 1) = '\0';
            break;
        case LUACS_TOBJREF:
        case LUACS_TOBJENT:
        case LUACS_TEXTREF:
            LUACS_ASSERT(L, 0);
            break;
        default:
            lua_pushnil(L);
            break;
    }
}

/* enum */
int luacs_newenum0(lua_State *L, const char *ename, size_t valwidth) {
    int ret;
    struct luacenum *ce;
    char metaname[METANAMELEN];

    snprintf(metaname, sizeof(metaname), "%s%s", METANAME_LUACTYPE, ename);
    lua_getfield(L, LUA_REGISTRYINDEX, metaname);
    if (!lua_isnil(L, -1)) {
        ce = luacs_checkenum(L, -1);
        return (1);
    }
    lua_pop(L, 1);

    ce = lua_newuserdata(L, sizeof(struct luacenum));
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, metaname);
    memcpy(ce->metaname, metaname, MINIMUM(sizeof(metaname), sizeof(ce->metaname)));

    ce->valwidth = valwidth;
    ce->enumname = index(ce->metaname, '.') + 1;
    SPLAY_INIT(&ce->labels);
    SPLAY_INIT(&ce->values);
    if ((ret = luaL_newmetatable(L, METANAME_LUACSENUM)) != 0) {
        lua_pushcfunction(L, luacs_enum__gc);
        lua_setfield(L, -2, "__gc");
        lua_pushcfunction(L, luacs_enum__index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, luacs_enum__next);
        lua_pushcclosure(L, luacs_enum__pairs, 1);
        lua_setfield(L, -2, "__pairs");
    }
    lua_setmetatable(L, -2);

    lua_pushvalue(L, -1);
    lua_pushcclosure(L, luacs_enum_get, 1);
    ce->func_get = luacs_ref(L);

    lua_pushvalue(L, -1);
    lua_pushcclosure(L, luacs_enum_memberof, 1);
    ce->func_memberof = luacs_ref(L);

    return (1);
}

int luacs_delenum(lua_State *L, const char *ename) {
    char metaname[METANAMELEN];

    lua_pushnil(L);
    snprintf(metaname, sizeof(metaname), "%s%s", METANAME_LUACTYPE, ename);
    lua_setfield(L, LUA_REGISTRYINDEX, metaname);
    return (0);
}

struct luacenum *luacs_checkenum(lua_State *L, int ceidx) { return (luaL_checkudata(L, ceidx, METANAME_LUACSENUM)); }

struct luacenum_value *luacs_enum_get0(struct luacenum *ce, intmax_t value) {
    struct luacenum_value vkey;

    vkey.value = value;
    return (SPLAY_FIND(luacenum_values, &ce->values, &vkey));
}

int luacs_enum_get(lua_State *L) {
    struct luacenum *ce;
    struct luacenum_value *val, vkey;

    lua_settop(L, 1);
    ce = luacs_checkenum(L, lua_upvalueindex(1));
    vkey.value = luaL_checkinteger(L, 1);

    if ((val = SPLAY_FIND(luacenum_values, &ce->values, &vkey)) == NULL)
        lua_pushnil(L);
    else
        luacs_getref(L, val->ref);

    return (1);
}

int luacs_newenumval(lua_State *L, const char *ename, intmax_t ival) {
    struct luacenum *ce;
    struct luacenum_value *cv;
    char metaname[METANAMELEN];

    snprintf(metaname, sizeof(metaname), "%s%s", METANAME_LUACTYPE, ename);
    lua_getfield(L, LUA_REGISTRYINDEX, metaname);
    if (!lua_isnil(L, -1)) {
        ce = luacs_checkenum(L, -1);
        lua_pop(L, 1);
        cv = luacs_enum_get0(ce, ival);
        if (cv != NULL) {
            luacs_getref(L, cv->ref);
            return (1);
        }
        lua_pushnil(L);
    }
    return (1);
}

int luacs_enum_memberof(lua_State *L) {
    struct luacenum *ce;
    struct luacenum_value *val, *val1;

    lua_settop(L, 1);
    ce = luacs_checkenum(L, lua_upvalueindex(1));
    val = luaL_checkudata(L, 1, METANAME_LUACSENUMVAL);
    val1 = luacs_enum_get0(ce, val->value);
    lua_pushboolean(L, val1 != NULL && val1 == val);

    return (1);
}

int luacs_enum__index(lua_State *L) {
    struct luacenum *ce;
    struct luacenum_value *val, vkey;

    lua_settop(L, 2);
    ce = luacs_checkenum(L, 1);
    vkey.label = luaL_checkstring(L, 2);
    if ((val = SPLAY_FIND(luacenum_labels, &ce->labels, &vkey)) == NULL) {
        if (strcmp(vkey.label, "get") == 0)
            luacs_getref(L, ce->func_get);
        else if (strcmp(vkey.label, "memberof") == 0)
            luacs_getref(L, ce->func_memberof);
        else
            lua_pushnil(L);
    } else
        luacs_getref(L, val->ref);

    return (1);
}

int luacs_enum__next(lua_State *L) {
    struct luacenum *ce;
    struct luacenum_value *val, vkey;

    lua_settop(L, 2);
    ce = luacs_checkenum(L, 1);
    if (lua_isnil(L, 2))
        val = SPLAY_MIN(luacenum_values, &ce->values);
    else {
        vkey.label = luaL_checkstring(L, 2);
        /* don't confuse.  key is label, sort by value */
        val = SPLAY_FIND(luacenum_labels, &ce->labels, &vkey);
        if (val != NULL) val = SPLAY_NEXT(luacenum_values, &ce->values, val);
    }
    if (val == NULL) {
        lua_pushnil(L);
        return (1);
    }
    lua_pushstring(L, val->label);
    luacs_getref(L, val->ref);

    return (2);
}

int luacs_enum__pairs(lua_State *L) {
    lua_settop(L, 1);
    luacs_checkenum(L, 1);
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_pushvalue(L, 1);
    lua_pushnil(L);

    return (3);
}

int luacs_enum__gc(lua_State *L) {
    struct luacenum *ce;
    struct luacenum_value *val, *valtmp;

    lua_settop(L, 1);
    ce = luacs_checkenum(L, 1);
    val = SPLAY_MIN(luacenum_labels, &ce->labels);
    while (val != NULL) {
        valtmp = SPLAY_NEXT(luacenum_labels, &ce->labels, val);
        SPLAY_REMOVE(luacenum_labels, &ce->labels, val);
        SPLAY_REMOVE(luacenum_values, &ce->values, val);
        luacs_unref(L, val->ref);
        val = valtmp;
    }
    luacs_unref(L, ce->func_get);

    return (0);
}

int luacs_enum_declare_value(lua_State *L, const char *label, intmax_t value) {
    int ret, llabel;
    struct luacenum_value *val;
    struct luacenum *ce;

    ce = luacs_checkenum(L, -1);
    llabel = strlen(label) + 1;
    val = lua_newuserdata(L, sizeof(struct luacenum_value) + llabel);
    memcpy(val + 1, label, llabel);
    val->label = (char *)(val + 1);
    val->value = value;
    SPLAY_INSERT(luacenum_labels, &ce->labels, val);
    SPLAY_INSERT(luacenum_values, &ce->values, val);

    lua_pushvalue(L, -1);
    val->ce = ce;
    val->ref = luacs_ref(L); /* as for a ref from luacenum */
    if ((ret = luaL_newmetatable(L, METANAME_LUACSENUMVAL)) != 0) {
        lua_pushcfunction(L, luacs_enumvalue__gc);
        lua_setfield(L, -2, "__gc");
        lua_pushcfunction(L, luacs_enumvalue__lt);
        lua_setfield(L, -2, "__lt");
        lua_pushcfunction(L, luacs_enumvalue__eq);
        lua_setfield(L, -2, "__eq");
        lua_pushcfunction(L, luacs_enumvalue__tostring);
        lua_setfield(L, -2, "__tostring");
        lua_pushcfunction(L, luacs_enumvalue_tointeger);
        lua_setfield(L, -2, "tointeger");
        lua_pushcfunction(L, luacs_enumvalue_label);
        lua_setfield(L, -2, "label");
    }
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);
    lua_pop(L, 1);

    return (0);
}

int luacs_enumvalue_tointeger(lua_State *L) {
    struct luacenum_value *val;

    lua_settop(L, 1);
    val = luaL_checkudata(L, 1, METANAME_LUACSENUMVAL);
    lua_pushinteger(L, val->value);

    return (1);
}

int luacs_enumvalue_label(lua_State *L) {
    struct luacenum_value *val;

    lua_settop(L, 1);
    val = luaL_checkudata(L, 1, METANAME_LUACSENUMVAL);
    lua_pushstring(L, val->label);

    return (1);
}

int luacs_enumvalue__tostring(lua_State *L) {
    struct luacenum_value *val;
    char buf[128];

    lua_settop(L, 1);
    val = luaL_checkudata(L, 1, METANAME_LUACSENUMVAL);
    /* Lua supports limitted int width for number2tstr */
    snprintf(buf, sizeof(buf), "%jd", val->value);
    lua_pushfstring(L, "%s(%s)", val->label, buf);

    return (1);
}

int luacs_enumvalue__gc(lua_State *L) {
    lua_settop(L, 1);
    luaL_checkudata(L, 1, METANAME_LUACSENUMVAL);

    return (0);
}

int luacs_enumvalue__eq(lua_State *L) {
    struct luacenum_value *val1, *val2;
    intmax_t ival1, ival2;

    lua_settop(L, 2);
    val1 = luaL_checkudata(L, 1, METANAME_LUACSENUMVAL);
    ival1 = val1->value;

    if (lua_isnumber(L, 2))
        ival2 = lua_tointeger(L, 2);
    else {
        val2 = luaL_checkudata(L, 2, METANAME_LUACSENUMVAL);
        ival2 = val2->value;
    }
    if (ival1 == ival2)
        lua_pushboolean(L, 1);
    else
        lua_pushboolean(L, 0);

    return (1);
}

int luacs_enumvalue__lt(lua_State *L) {
    struct luacenum_value *val1, *val2;
    intmax_t ival1, ival2;

    lua_settop(L, 2);
    val1 = luaL_checkudata(L, 1, METANAME_LUACSENUMVAL);
    ival1 = val1->value;

    if (lua_isnumber(L, 2))
        ival2 = lua_tointeger(L, 2);
    else {
        val2 = luaL_checkudata(L, 2, METANAME_LUACSENUMVAL);
        ival2 = val2->value;
    }
    if (ival1 < ival2)
        lua_pushboolean(L, 1);
    else
        lua_pushboolean(L, 0);

    return (1);
}

int luacenum_label_cmp(struct luacenum_value *a, struct luacenum_value *b) { return (strcmp(a->label, b->label)); }

int luacenum_value_cmp(struct luacenum_value *a, struct luacenum_value *b) {
    intmax_t cmp = a->value - b->value;
    return ((cmp == 0) ? 0 : (cmp < 0) ? -1 : 1);
}

int luacs_checkenumval(lua_State *L, int idx, const char *enumname) {
    struct luacenum_value *val;

    val = luaL_checkudata(L, idx, METANAME_LUACSENUMVAL);
    if (strcmp(val->ce->enumname, enumname) != 0) luaL_error(L, "%s expected, got %s", enumname, val->ce->enumname);

    return (val->value);
}

/* refs */
int luacs_ref(lua_State *L) {
    int ret;

    lua_getfield(L, LUA_REGISTRYINDEX, LUACS_REGISTRY_NAME);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_setfield(L, LUA_REGISTRYINDEX, LUACS_REGISTRY_NAME);
        lua_getfield(L, LUA_REGISTRYINDEX, LUACS_REGISTRY_NAME);
    }
    lua_pushvalue(L, -2);
    ret = luaL_ref(L, -2);
    lua_pop(L, 2);

    return (ret);
}

int luacs_getref(lua_State *L, int ref) {
    lua_getfield(L, LUA_REGISTRYINDEX, LUACS_REGISTRY_NAME);
    if (lua_isnil(L, -1)) return (1);
    lua_rawgeti(L, -1, ref);
    lua_remove(L, -2);
    return (1);
}

int luacs_unref(lua_State *L, int ref) {
    lua_getfield(L, LUA_REGISTRYINDEX, LUACS_REGISTRY_NAME);
    luaL_unref(L, -1, ref);
    lua_pop(L, 1);

    return (0);
}

SPLAY_GENERATE(luacstruct_fields, luacstruct_field, tree, luacstruct_field_cmp);
SPLAY_GENERATE(luacenum_labels, luacenum_value, treel, luacenum_label_cmp);
SPLAY_GENERATE(luacenum_values, luacenum_value, treev, luacenum_value_cmp);

#pragma endregion LuaCS

int luaopen_debugger(lua_State *lua) {
    if (luaL_dofile(lua, ("data/scripts/libs/debugger.lua"))) lua_error(lua);
    return 1;
}

static const char *MODULE_NAME = "DEBUGGER_LUA_MODULE";
static const char *MSGH = "DEBUGGER_LUA_MSGH";

void metadot_debug_setup(lua_State *lua, const char *name, const char *globalName, lua_CFunction readFunc, lua_CFunction writeFunc) {
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

int metadot_debug_pcall(lua_State *lua, int nargs, int nresults, int msgh) {
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

int metadot_preload(lua_State *L, lua_CFunction f, const char *name) {
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    lua_pushcfunction(L, f);
    lua_setfield(L, -2, name);
    lua_pop(L, 2);
    return 0;
}

int metadot_preload_auto(lua_State *L, lua_CFunction f, const char *name) {
    metadot_preload(L, f, name);
    lua_getglobal(L, "require");
    lua_pushstring(L, name);
    lua_call(L, 1, 0);
}

void metadot_load(lua_State *L, const luaL_Reg *l, const char *name) {
    lua_getglobal(L, name);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
    }
    luaL_setfuncs(L, l, 0);
    lua_setglobal(L, name);
}

void metadot_loadover(lua_State *L, const luaL_Reg *l, const char *name) {
    lua_newtable(L);
    luaL_setfuncs(L, l, 0);
    lua_setglobal(L, name);
}

#pragma region LuaMemSafe

const char *const REG_POLICY = "metadot_safelua_policy";
const char *const REG_CANCEL = "metadot_safelua_cancel";
const char *const REG_CANCELUDATA = "metadot_safelua_canceludata";
const char *const REG_CANCELJMP = "metadot_safelua_canceljmp";

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
    metadot_safelua_Handler callback;
    void *udata;
};

struct Policy {
    struct Allocator *allocator;
    struct Handler *handlers;
    size_t nb_handlers, size_handlers;
};

lua_State *metadot_safelua_open(void) {
    struct Policy *policy = malloc(sizeof(struct Policy));
    policy->allocator = new_allocator();
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

void metadot_safelua_close(lua_State *state) {
    struct Policy *policy = getudregistry(state, REG_POLICY);
    lua_close(state);
    free_resources(policy, SAFELUA_FINISHED);
    delete_allocator(policy->allocator);
    free(policy);
}

static void cancel_hook(lua_State *state, lua_Debug *ar) { metadot_safelua_checkcancel(state); }

int metadot_safelua_pcallk(lua_State *state, int nargs, int nresults, int errfunc, int ctx, lua_CFunction k, metadot_safelua_CancelCheck cancel, void *canceludata) {
    struct Policy *policy = getudregistry(state, REG_POLICY);
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
            delete_allocator(policy->allocator);
            free(policy);
            ret = SAFELUA_CANCELED;
        }

        return ret;
    }
}

void metadot_safelua_checkcancel(lua_State *state) {
    if (metadot_safelua_shouldcancel(state)) metadot_safelua_cancel(state);
}

void metadot_safelua_cancel(lua_State *state) {
    jmp_buf *env = getudregistry(state, REG_CANCELJMP);
    if (env) longjmp(*env, 1);
}

int metadot_safelua_shouldcancel(lua_State *state) {
    metadot_safelua_CancelCheck cancel = getudregistry(state, REG_CANCEL);
    void *canceludata = getudregistry(state, REG_CANCELUDATA);

    return cancel && cancel(state, canceludata);
}

void metadot_safelua_add_handler(lua_State *state, metadot_safelua_Handler handler, void *handlerudata) {
    struct Policy *policy = getudregistry(state, REG_POLICY);
    if (++policy->nb_handlers > policy->size_handlers) {
        if (policy->size_handlers == 0)
            policy->size_handlers = 4;
        else
            policy->size_handlers *= 2;
        policy->handlers = realloc(policy->handlers, sizeof(struct Handler) * policy->size_handlers);
    }
    {
        struct Handler *h = &policy->handlers[policy->nb_handlers - 1];
        h->callback = handler;
        h->udata = handlerudata;
    }
}

int metadot_safelua_remove_handler(lua_State *state, metadot_safelua_Handler handler, void *handlerudata) {
    struct Policy *policy = getudregistry(state, REG_POLICY);
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