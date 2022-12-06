// Copyright(c) 2022, KaoruXun All rights reserved.

#include "EngineFuncWrap.hpp"
#include "Core/Global.hpp"
#include "Engine/LuaWrapper.hpp"
#include "Engine/RendererGPU.h"
#include "Engine/SDLWrapper.hpp"
#include "Engine/Scripting.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>

#define MINIZ_NO_ZLIB_APIS
#include "Libs/miniminiz.h"

#define return_self(L)                                                                             \
    do {                                                                                           \
        lua_settop(L, 1);                                                                          \
        return 1;                                                                                  \
    } while (0)

#if LUA_VERSION_NUM <= 501
#ifndef LUAMOD_API
#define LUAMOD_API LUALIB_API
#endif
#define luaL_setfuncs(L, libs, nups) luaL_register(L, NULL, libs)
#if !defined(LUA_LJDIR) && !defined(luaL_newlib)
#define luaL_newlib(L, libs)                                                                       \
    (lua_createtable(L, 0, sizeof(libs) / sizeof(libs[0])), luaL_register(L, NULL, libs))
#endif
static int lua_relindex(int idx, int onstack) {
    return idx >= 0 || idx <= LUA_REGISTRYINDEX ? idx : idx - onstack;
}
#ifndef LUA_LJDIR
static void luaL_setmetatable(lua_State *L, const char *name) {
    luaL_getmetatable(L, name);
    lua_setmetatable(L, -2);
}
#endif
#ifndef lua_rawsetp
#define lua_rawsetp lua_rawsetp
static void lua_rawsetp(lua_State *L, int idx, const void *p) {
    lua_pushlightuserdata(L, (void *) p);
    lua_insert(L, -2);
    lua_rawset(L, lua_relindex(idx, 1));
}
#endif
#ifndef LUA_LJDIR
static void *luaL_testudata(lua_State *L, int ud, const char *tname) {
    void *p = lua_touserdata(L, ud);
    if (p != NULL) {
        if (lua_getmetatable(L, ud)) {
            luaL_getmetatable(L, tname);
            if (!lua_rawequal(L, -1, -2)) p = NULL;
            lua_pop(L, 2);
            return p;
        }
    }
    return NULL;
}
#endif
#ifndef luaL_tolstring
#define luaL_tolstring luaL_tolstring
static const char *luaL_tolstring(lua_State *L, int idx, size_t *len) {
    if (!luaL_callmeta(L, idx, "__tostring")) { /* no metafield? */
        switch (lua_type(L, idx)) {
            case LUA_TNUMBER:
            case LUA_TSTRING:
                lua_pushvalue(L, idx);
                break;
            case LUA_TBOOLEAN:
                lua_pushstring(L, (lua_toboolean(L, idx) ? "true" : "false"));
                break;
            case LUA_TNIL:
                lua_pushliteral(L, "nil");
                break;
            default:
                lua_pushfstring(L, "%s: %p", luaL_typename(L, idx), lua_topointer(L, idx));
                break;
        }
    }
    return lua_tolstring(L, -1, len);
}
#endif
#endif

#pragma region BindMiniz

static int Ladler32(lua_State *L) {
    size_t len;
    const char *s = luaL_optlstring(L, 1, NULL, &len);
    mz_ulong init;
    if (!lua_isnoneornil(L, 2)) init = (mz_ulong) luaL_checkinteger(L, 2);
    else
        init = mz_adler32(0, NULL, 0);
    if (s == NULL) {
        lua_pushinteger(L, init);
        return 1;
    }
    lua_pushinteger(L, (lua_Integer) mz_adler32(init, (const unsigned char *) s, len));
    return 1;
}

static int Lcrc32(lua_State *L) {
    size_t len;
    const char *s = luaL_optlstring(L, 1, NULL, &len);
    mz_ulong init;
    if (!lua_isnoneornil(L, 2)) init = (mz_ulong) luaL_checkinteger(L, 2);
    else
        init = mz_crc32(0, NULL, 0);
    if (s == NULL) {
        lua_pushinteger(L, init);
        return 1;
    }
    lua_pushinteger(L, (lua_Integer) mz_crc32(init, (const unsigned char *) s, len));
    return 1;
}

#define LMZ_COMPRESSOR "miniz.Compressor"
#define LMZ_DECOMPRESSOR "miniz.Decompressor"

typedef tdefl_compressor lmz_Comp;

typedef struct lmz_Decomp
{
    tinfl_decompressor decomp;
    mz_uint flags;
    mz_uint8 *curr;
    mz_uint8 dict[TINFL_LZ_DICT_SIZE];
} lmz_Decomp;

static void lmz_initcomp(lua_State *L, int start, lmz_Comp *c) {
    static const mz_uint probes[11] = {0, 1, 6, 32, 16, 32, 128, 256, 512, 768, 1500};
    int level = (int) luaL_optinteger(L, start, MZ_DEFAULT_LEVEL);
    mz_uint flags = probes[(level >= 0) ? MZ_MIN(10, level) : MZ_DEFAULT_LEVEL];
    tdefl_status status;
    if (lua_tointeger(L, start + 1) >= 0) flags |= TDEFL_WRITE_ZLIB_HEADER;
    if (level <= 3) flags |= TDEFL_GREEDY_PARSING_FLAG;
    if ((status = tdefl_init(c, NULL, NULL, flags)) != TDEFL_STATUS_OKAY)
        luaL_error(L, "compress failure (%d)", status);
}

static void lmz_initdecomp(lua_State *L, int start, lmz_Decomp *d) {
    int window_bits = (int) luaL_optinteger(L, start, 0);
    d->flags = window_bits >= 0 ? TINFL_FLAG_PARSE_ZLIB_HEADER : 0;
    d->flags |= TINFL_FLAG_HAS_MORE_INPUT;
    d->curr = d->dict;
    tinfl_init(&d->decomp);
}

static int lmz_compress(lua_State *L, int start, lmz_Comp *c, int flush) {
    size_t len, offset = 0, output = 0;
    const char *s = luaL_checklstring(L, start, &len);
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    for (;;) {
        size_t in_size = len - offset;
        size_t out_size = LUAL_BUFFERSIZE;
        tdefl_status status =
                tdefl_compress(c, s + offset, &in_size, (mz_uint8 *) luaL_prepbuffer(&b), &out_size,
                               (tdefl_flush) flush);
        offset += in_size;
        output += out_size;
        luaL_addsize(&b, out_size);
        if (status == TDEFL_STATUS_DONE) {
            luaL_pushresult(&b);
            lua_pushboolean(L, status == TDEFL_STATUS_DONE);
            lua_pushinteger(L, len);
            lua_pushinteger(L, output);
            return 4;
        } else if (status != TDEFL_STATUS_OKAY)
            luaL_error(L, "compress failure (%d)", status);
    }
}

static int lmz_decompress(lua_State *L, int start, lmz_Decomp *d) {
    size_t len, offset = 0, output = 0;
    const char *s = luaL_checklstring(L, start, &len);
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    for (;;) {
        size_t in_size = len - offset;
        size_t out_size = TINFL_LZ_DICT_SIZE - (d->curr - d->dict);
        tinfl_status status =
                tinfl_decompress(&d->decomp, (mz_uint8 *) (s + offset), &in_size, d->dict, d->curr,
                                 &out_size, d->flags & ~TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
        offset += in_size;
        output += out_size;
        if (out_size != 0) luaL_addlstring(&b, (char *) d->curr, out_size);
        if (status == TINFL_STATUS_DONE) {
            luaL_pushresult(&b);
            lua_pushboolean(L, status == TINFL_STATUS_DONE);
            lua_pushinteger(L, len);
            lua_pushinteger(L, output);
            return 4;
        } else if (status < 0)
            luaL_error(L, "decompress failure (%d)", status);
        d->curr = &d->dict[(d->curr + out_size - d->dict) & (TINFL_LZ_DICT_SIZE - 1)];
    }
}

static int Lcomp_tostring(lua_State *L) {
    lmz_Comp *c = (lmz_Comp *) luaL_checkudata(L, 1, LMZ_COMPRESSOR);
    lua_pushfstring(L, LMZ_COMPRESSOR ": %p", c);
    return 1;
}

static int Lcomp_call(lua_State *L) {
    static const char *opts[] = {"sync", "full", "finish", NULL};
    static int flushs[] = {TDEFL_SYNC_FLUSH, TDEFL_FULL_FLUSH, TDEFL_FINISH};
    lmz_Comp *c = (lmz_Comp *) luaL_checkudata(L, 1, LMZ_COMPRESSOR);
    int flush = luaL_checkoption(L, 3, "sync", opts);
    return lmz_compress(L, 2, c, flushs[flush]);
}

static int Lcompress(lua_State *L) {
    lua_settop(L, 3);
    if (lua_type(L, 1) == LUA_TSTRING) {
        lmz_Comp c;
        lmz_initcomp(L, 2, &c);
        return lmz_compress(L, 1, &c, TDEFL_FINISH);
    } else {
        lmz_Comp *c = (lmz_Comp *) lua_newuserdata(L, sizeof(lmz_Comp));
        lmz_initcomp(L, 1, c);
        if (luaL_newmetatable(L, LMZ_COMPRESSOR)) {
            lua_pushcfunction(L, Lcomp_tostring);
            lua_setfield(L, -2, "__tostring");
            lua_pushcfunction(L, Lcomp_call);
            lua_setfield(L, -2, "__call");
        }
        lua_setmetatable(L, -2);
        return 1;
    }
}

static int Ldecomp_tostring(lua_State *L) {
    lmz_Decomp *d = (lmz_Decomp *) luaL_checkudata(L, 1, LMZ_DECOMPRESSOR);
    lua_pushfstring(L, LMZ_DECOMPRESSOR ": %p", d);
    return 1;
}

static int Ldecomp_call(lua_State *L) {
    lmz_Decomp *d = (lmz_Decomp *) luaL_checkudata(L, 1, LMZ_COMPRESSOR);
    return lmz_decompress(L, 2, d);
}

static int Ldecompress(lua_State *L) {
    if (lua_type(L, 1) == LUA_TSTRING) {
        lmz_Decomp d;
        lmz_initdecomp(L, 2, &d);
        return lmz_decompress(L, 1, &d);
    } else {
        lmz_Decomp *d = (lmz_Decomp *) lua_newuserdata(L, sizeof(lmz_Decomp));
        lmz_initdecomp(L, 1, d);
        if (luaL_newmetatable(L, LMZ_DECOMPRESSOR)) {
            lua_pushcfunction(L, Ldecomp_tostring);
            lua_setfield(L, -2, "__tostring");
            lua_pushcfunction(L, Ldecomp_call);
            lua_setfield(L, -2, "__call");
        }
        lua_setmetatable(L, -2);
        return 1;
    }
}

/* zip reader */

#define LMZ_ZIP_READER "miniz.ZipReader"

static int lmz_zip_pusherror(lua_State *L, mz_zip_archive *za, const char *prefix) {
    mz_zip_error err = mz_zip_get_last_error(za);
    const char *emsg = mz_zip_get_error_string(err);
    lua_pushnil(L);
    if (prefix == NULL) lua_pushstring(L, emsg);
    else
        lua_pushfstring(L, "%s: %s", prefix, emsg);
    return 2;
}

static int Lzip_read_string(lua_State *L) {
    size_t len;
    const char *s = luaL_checklstring(L, 1, &len);
    mz_uint32 flags = (mz_uint32) luaL_optinteger(L, 2, 0);
    mz_zip_archive *za = (mz_zip_archive *) lua_newuserdata(L, sizeof(mz_zip_archive));
    mz_zip_zero_struct(za);
    if (!mz_zip_reader_init_mem(za, s, len, flags)) return lmz_zip_pusherror(L, za, NULL);
    luaL_setmetatable(L, LMZ_ZIP_READER);
    lua_pushvalue(L, 1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, za);
    return 1;
}

static int Lzip_read_file(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    mz_uint32 flags = (mz_uint32) luaL_optinteger(L, 2, 0);
    mz_zip_archive *za = (mz_zip_archive *) lua_newuserdata(L, sizeof(mz_zip_archive));
    mz_zip_zero_struct(za);
    if (!mz_zip_reader_init_file(za, filename, flags)) return lmz_zip_pusherror(L, za, filename);
    luaL_setmetatable(L, LMZ_ZIP_READER);
    return 1;
}

static int Lreader_close(lua_State *L) {
    mz_zip_archive *za = (mz_zip_archive *) luaL_checkudata(L, 1, LMZ_ZIP_READER);
    lua_pushboolean(L, mz_zip_reader_end(za));
    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, za);
    return 1;
}

static int Lreader___index(lua_State *L) {
    mz_zip_archive *za = (mz_zip_archive *) luaL_checkudata(L, 1, LMZ_ZIP_READER);
    int type = lua_type(L, 2);
    if (type == LUA_TSTRING) {
        if (lua_getmetatable(L, 1)) {
            lua_pushvalue(L, 2);
            lua_rawget(L, -2);
            return 1;
        }
        return 0;
    } else if (type == LUA_TNUMBER) {
        mz_uint file_index = (mz_uint) luaL_checkinteger(L, 2) - 1;
        char filename[MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE];
        if (!mz_zip_reader_get_filename(za, file_index, filename, MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE))
            return lmz_zip_pusherror(L, za, NULL);
        lua_pushstring(L, filename);
        return 1;
    }
    return 0;
}

static int Lreader___tostring(lua_State *L) {
    mz_zip_archive *za = (mz_zip_archive *) luaL_testudata(L, 1, LMZ_ZIP_READER);
    if (za) lua_pushfstring(L, "miniz.ZipReader: %p", za);
    else
        luaL_tolstring(L, 1, NULL);
    return 1;
}

static int Lreader_get_num_files(lua_State *L) {
    mz_zip_archive *za = (mz_zip_archive *) luaL_checkudata(L, 1, LMZ_ZIP_READER);
    lua_pushinteger(L, mz_zip_reader_get_num_files(za));
    return 1;
}

static int Lreader_get_offset(lua_State *L) {
    mz_zip_archive *za = (mz_zip_archive *) luaL_checkudata(L, 1, LMZ_ZIP_READER);
    lua_pushinteger(L, (lua_Integer) mz_zip_get_archive_file_start_offset(za));
    lua_pushinteger(L, (lua_Integer) mz_zip_get_archive_size(za));
    return 2;
}

static int Lreader_locate_file(lua_State *L) {
    mz_zip_archive *za = (mz_zip_archive *) luaL_checkudata(L, 1, LMZ_ZIP_READER);
    const char *path = luaL_checkstring(L, 2);
    mz_uint32 flags = (mz_uint32) luaL_optinteger(L, 3, 0);
    int index = mz_zip_reader_locate_file(za, path, NULL, flags);
    if (index < 0) return lmz_zip_pusherror(L, za, path);
    lua_pushinteger(L, index + 1);
    return 1;
}

static int Lreader_stat(lua_State *L) {
    mz_zip_archive *za = (mz_zip_archive *) luaL_checkudata(L, 1, LMZ_ZIP_READER);
    mz_uint file_index = (mz_uint) luaL_checkinteger(L, 2) - 1;
    mz_zip_archive_file_stat stat;
    if (!mz_zip_reader_file_stat(za, file_index, &stat)) return lmz_zip_pusherror(L, za, NULL);
    lua_newtable(L);
    lua_pushinteger(L, file_index);
    lua_setfield(L, -2, "index");
    lua_pushinteger(L, stat.m_version_made_by);
    lua_setfield(L, -2, "version_made_by");
    lua_pushinteger(L, stat.m_version_needed);
    lua_setfield(L, -2, "version_needed");
    lua_pushinteger(L, stat.m_bit_flag);
    lua_setfield(L, -2, "bit_flag");
    lua_pushinteger(L, stat.m_method);
    lua_setfield(L, -2, "method");
    lua_pushinteger(L, (lua_Integer) stat.m_time);
    lua_setfield(L, -2, "time");
    lua_pushinteger(L, stat.m_crc32);
    lua_setfield(L, -2, "crc32");
    lua_pushinteger(L, (lua_Integer) stat.m_comp_size);
    lua_setfield(L, -2, "comp_size");
    lua_pushinteger(L, (lua_Integer) stat.m_uncomp_size);
    lua_setfield(L, -2, "uncomp_size");
    lua_pushinteger(L, stat.m_internal_attr);
    lua_setfield(L, -2, "internal_attr");
    lua_pushinteger(L, stat.m_external_attr);
    lua_setfield(L, -2, "external_attr");
    lua_pushstring(L, stat.m_filename);
    lua_setfield(L, -2, "filename");
    lua_pushstring(L, stat.m_comment);
    lua_setfield(L, -2, "comment");
    return 1;
}

static int Lreader_get_filename(lua_State *L) {
    mz_zip_archive *za = (mz_zip_archive *) luaL_checkudata(L, 1, LMZ_ZIP_READER);
    mz_uint file_index = (mz_uint) luaL_checkinteger(L, 2) - 1;
    char filename[MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE];
    if (!mz_zip_reader_get_filename(za, file_index, filename, MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE))
        return lmz_zip_pusherror(L, za, NULL);
    lua_pushstring(L, filename);
    return 1;
}

static int Lreader_is_file_a_directory(lua_State *L) {
    mz_zip_archive *za = (mz_zip_archive *) luaL_checkudata(L, 1, LMZ_ZIP_READER);
    mz_uint file_index = (mz_uint) luaL_checkinteger(L, 2) - 1;
    lua_pushboolean(L, mz_zip_reader_is_file_a_directory(za, file_index));
    return 1;
}

static size_t Lwriter(void *ud, mz_uint64 file_ofs, const void *p, size_t n) {
    (void) file_ofs;
    luaL_addlstring((luaL_Buffer *) ud, (char *) p, n);
    return n;
}

static int Lreader_extract(lua_State *L) {
    mz_zip_archive *za = (mz_zip_archive *) luaL_checkudata(L, 1, LMZ_ZIP_READER);
    mz_uint flags = (mz_uint) luaL_optinteger(L, 3, 0);
    int type = lua_type(L, 2);
    luaL_Buffer b;
    mz_bool result = 0;
    luaL_buffinit(L, &b);
    if (type == LUA_TSTRING)
        result = mz_zip_reader_extract_file_to_callback(za, lua_tostring(L, 2), Lwriter, &b, flags);
    else if (type == LUA_TNUMBER)
        result = mz_zip_reader_extract_to_callback(za, (mz_uint) lua_tointeger(L, 2) - 1, Lwriter,
                                                   &b, flags);
    luaL_pushresult(&b);
    return result ? 1 : 0;
}

static void open_zipreader(lua_State *L) {
    luaL_Reg libs[] = {{"__len", Lreader_get_num_files},
                       {"__gc", Lreader_close},
#define ENTRY(name) {#name, Lreader_##name}
                       ENTRY(__index),
                       ENTRY(__tostring),
                       ENTRY(close),
                       ENTRY(get_num_files),
                       ENTRY(locate_file),
                       ENTRY(stat),
                       ENTRY(get_filename),
                       ENTRY(is_file_a_directory),
                       ENTRY(extract),
                       ENTRY(get_offset),
#undef ENTRY
                       {NULL, NULL}};
    if (luaL_newmetatable(L, LMZ_ZIP_READER)) luaL_setfuncs(L, libs, 0);
}

/* zip writer */

#define LMZ_ZIP_WRITER "miniz.ZipWriter"

static int Lwriter___tostring(lua_State *L) {
    mz_zip_archive *za = (mz_zip_archive *) luaL_testudata(L, 1, LMZ_ZIP_WRITER);
    if (za) lua_pushfstring(L, "miniz.ZipWriter: %p", za);
    else
        luaL_tolstring(L, 1, NULL);
    return 1;
}

static int Lzip_write_string(lua_State *L) {
    size_t size_to_reserve_at_beginning = (size_t) luaL_optinteger(L, 1, 0);
    size_t initial_allocation_size = (size_t) luaL_optinteger(L, 2, LUAL_BUFFERSIZE);
    mz_zip_archive *za = (mz_zip_archive *) lua_newuserdata(L, sizeof(mz_zip_archive));
    mz_zip_zero_struct(za);
    if (!mz_zip_writer_init_heap(za, size_to_reserve_at_beginning, initial_allocation_size))
        return lmz_zip_pusherror(L, za, NULL);
    luaL_setmetatable(L, LMZ_ZIP_WRITER);
    return 1;
}

static int Lzip_write_file(lua_State *L) {
    const char *filename = luaL_checkstring(L, 1);
    size_t size_to_reserve_at_beginning = (size_t) luaL_optinteger(L, 2, 0);
    mz_zip_archive *za = (mz_zip_archive *) lua_newuserdata(L, sizeof(mz_zip_archive));
    mz_zip_zero_struct(za);
    if (!mz_zip_writer_init_file(za, filename, size_to_reserve_at_beginning))
        return lmz_zip_pusherror(L, za, filename);
    luaL_setmetatable(L, LMZ_ZIP_WRITER);
    return 1;
}

static int Lwriter_close(lua_State *L) {
    mz_zip_archive *za = (mz_zip_archive *) luaL_checkudata(L, 1, LMZ_ZIP_WRITER);
    if (mz_zip_get_mode(za) != MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED)
        mz_zip_writer_finalize_archive(za);
    lua_pushboolean(L, mz_zip_writer_end(za));
    return 1;
}

static int Lwriter_add_from_zip_reader(lua_State *L) {
    mz_zip_archive *za = (mz_zip_archive *) luaL_checkudata(L, 1, LMZ_ZIP_WRITER);
    mz_zip_archive *src = (mz_zip_archive *) luaL_checkudata(L, 2, LMZ_ZIP_READER);
    mz_uint file_index = (mz_uint) luaL_checkinteger(L, 3) - 1;
    if (!mz_zip_writer_add_from_zip_reader(za, src, file_index))
        return lmz_zip_pusherror(L, za, NULL);
    return_self(L);
}

static int Lwriter_add_string(lua_State *L) {
    mz_zip_archive *za = (mz_zip_archive *) luaL_checkudata(L, 1, LMZ_ZIP_WRITER);
    const char *path = luaL_checkstring(L, 2);
    size_t len, comment_len;
    const char *s = luaL_checklstring(L, 3, &len);
    const char *comment = luaL_optlstring(L, 5, NULL, &comment_len);
    mz_uint flags = (mz_uint) luaL_optinteger(L, 4, MZ_DEFAULT_LEVEL);
    if (!mz_zip_writer_add_mem_ex(za, path, s, len, comment, (mz_uint16) comment_len, flags, 0, 0))
        return lmz_zip_pusherror(L, za, path);
    return_self(L);
}

static int Lwriter_add_file(lua_State *L) {
    mz_zip_archive *za = (mz_zip_archive *) luaL_checkudata(L, 1, LMZ_ZIP_WRITER);
    const char *path = luaL_checkstring(L, 2);
    const char *filename = luaL_optstring(L, 3, path);
    mz_uint flags = (mz_uint) luaL_optinteger(L, 4, MZ_DEFAULT_LEVEL);
    size_t len;
    const char *comment = luaL_optlstring(L, 5, NULL, &len);
    if (!mz_zip_writer_add_file(za, path, filename, comment, (mz_uint16) len, flags))
        return lmz_zip_pusherror(L, za, filename);
    return_self(L);
}

static int Lwriter_finalize(lua_State *L) {
    mz_zip_archive *za = (mz_zip_archive *) luaL_checkudata(L, 1, LMZ_ZIP_WRITER);
    if (mz_zip_get_type(za) == MZ_ZIP_TYPE_HEAP) {
        size_t len = 0;
        void *s = NULL;
        mz_bool result = mz_zip_writer_finalize_heap_archive(za, &s, &len);
        lua_pushlstring(L, (char *) s, len);
        free(s);
        return result ? 1 : lmz_zip_pusherror(L, za, NULL);
    } else if (!mz_zip_writer_finalize_archive(za))
        return lmz_zip_pusherror(L, za, NULL);
    return_self(L);
}

static void open_zipwriter(lua_State *L) {
    luaL_Reg libs[] = {{"__gc", Lwriter_close},
#define ENTRY(name) {#name, Lwriter_##name}
                       ENTRY(__tostring),
                       ENTRY(close),
                       ENTRY(add_from_zip_reader),
                       ENTRY(add_string),
                       ENTRY(add_file),
                       ENTRY(finalize),
#undef ENTRY
                       {NULL, NULL}};
    if (luaL_newmetatable(L, LMZ_ZIP_WRITER)) {
        luaL_setfuncs(L, libs, 0);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
    }
}

int metadot_bind_miniz(lua_State *L) {
    luaL_Reg libs[] = {{"new_reader", Lzip_read_file},
                       {"new_writer", Lzip_write_string},
#define ENTRY(name) {#name, L##name}
                       ENTRY(adler32),
                       ENTRY(crc32),
                       ENTRY(compress),
                       ENTRY(decompress),
                       ENTRY(zip_read_file),
                       ENTRY(zip_read_string),
                       ENTRY(zip_write_file),
                       ENTRY(zip_write_string),
#undef ENTRY
                       {NULL, NULL}};
    open_zipreader(L);
    open_zipwriter(L);
    luaL_newlib(L, libs);
    lua_setglobal(L, "miniz");
    return 1;
}

#pragma endregion BindMiniz

namespace TestData {
    bool shaderOn = false;

    int pixelScale = DEFAULT_SCALE;
    int setPixelScale = DEFAULT_SCALE;

    METAENGINE_Render_Image *buffer;
    METAENGINE_Render_Target *renderer;
    METAENGINE_Render_Target *bufferTarget;

    UInt8 palette[COLOR_LIMIT][3] = INIT_COLORS;

    int paletteNum = 0;

    int drawOffX = 0;
    int drawOffY = 0;

    int windowWidth;
    int windowHeight;
    int drawX = 0;
    int drawY = 0;

    int lastWindowX = 0;
    int lastWindowY = 0;

    void assessWindow() {
        int winW = 0;
        int winH = 0;
        SDL_GetWindowSize(global.platform.window, &winW, &winH);

        int candidateOne = winH / SCRN_HEIGHT;
        int candidateTwo = winW / SCRN_WIDTH;

        if (winW != 0 && winH != 0) {
            pixelScale = (candidateOne > candidateTwo) ? candidateTwo : candidateOne;
            windowWidth = winW;
            windowHeight = winH;

            drawX = (windowWidth - pixelScale * SCRN_WIDTH) / 2;
            drawY = (windowHeight - pixelScale * SCRN_HEIGHT) / 2;

            METAENGINE_Render_SetWindowResolution(winW, winH);
        }
    }
}// namespace TestData

#pragma region BindImage

#define clamp(v, min, max) (float) ((v) < (min) ? (min) : ((v) > (max) ? (max) : (v)))

#define off(o, t) ((float) (o) -TestData::drawOffX), (float) ((t) -TestData::drawOffY)

struct imageType
{
    int width;
    int height;
    bool free;
    //        int clr;
    int lastRenderNum;
    int remap[COLOR_LIMIT];
    bool remapped;
    char **internalRep;
    SDL_Surface *surface;
    METAENGINE_Render_Image *texture;
};

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
UInt32 rMask = 0xff000000;
UInt32 gMask = 0x00ff0000;
UInt32 bMask = 0x0000ff00;
UInt32 aMask = 0x000000ff;
#else
UInt32 rMask = 0x000000ff;
UInt32 gMask = 0x0000ff00;
UInt32 bMask = 0x00ff0000;
UInt32 aMask = 0xff000000;
#endif

static char getColor(lua_State *L, int arg) {
    int color = luaL_checkinteger(L, arg) - 1;
    return static_cast<char>(
            color == -2
                    ? -1
                    : (color < 0 ? 0 : (color > (COLOR_LIMIT - 1) ? (COLOR_LIMIT - 1) : color)));
}

static UInt32 getRectC(imageType *data, int colorGiven) {
    int color = data->remap[colorGiven];// Palette remapping
    return SDL_MapRGBA(data->surface->format, TestData::palette[color][0],
                       TestData::palette[color][1], TestData::palette[color][2], 255);
}

static imageType *checkImage(lua_State *L) {
    void *ud = luaL_checkudata(L, 1, "MetaDot.Image");
    luaL_argcheck(L, ud != nullptr, 1, "`Image` expected");
    return (imageType *) ud;
}

bool freeCheck(lua_State *L, imageType *data) {
    if (data->free) {
        luaL_error(L, "Attempt to perform Image operation but Image was freed");
        return false;
    }
    return true;
}

static int newImage(lua_State *L) {
    int w = luaL_checkinteger(L, 1);
    int h = luaL_checkinteger(L, 2);
    size_t nbytes = sizeof(imageType);
    auto *a = (imageType *) lua_newuserdata(L, nbytes);

    luaL_getmetatable(L, "MetaDot.Image");
    lua_setmetatable(L, -2);

    a->width = w;
    a->height = h;
    a->free = false;
    //        a->clr = 0;
    a->lastRenderNum = TestData::paletteNum;
    for (int i = 0; i < COLOR_LIMIT; i++) { a->remap[i] = i; }

    a->internalRep = new char *[w];
    for (int i = 0; i < w; i++) {
        a->internalRep[i] = new char[h];
        memset(a->internalRep[i], -1, static_cast<size_t>(h));
    }

    a->surface = SDL_CreateRGBSurface(0, w, h, 32, rMask, gMask, bMask, aMask);

    // Init to black color
    SDL_FillRect(a->surface, nullptr, SDL_MapRGBA(a->surface->format, 0, 0, 0, 0));

    a->texture = METAENGINE_Render_CopyImageFromSurface(a->surface);
    METAENGINE_Render_SetImageFilter(a->texture, METAENGINE_Render_FILTER_NEAREST);
    METAENGINE_Render_SetSnapMode(a->texture, METAENGINE_Render_SNAP_NONE);

    return 1;
}

static int flushImage(lua_State *L) {
    imageType *data = checkImage(L);
    if (!freeCheck(L, data)) return 0;

    METAENGINE_Render_Rect rect = {0, 0, (float) data->width, (float) data->height};
    METAENGINE_Render_UpdateImage(data->texture, &rect, data->surface, &rect);

    return 0;
}

static int renderImage(lua_State *L) {
    imageType *data = checkImage(L);
    if (!freeCheck(L, data)) return 0;

    if (data->lastRenderNum != TestData::paletteNum || data->remapped) {
        UInt32 rectColor = SDL_MapRGBA(data->surface->format, 0, 0, 0, 0);
        SDL_FillRect(data->surface, nullptr, rectColor);

        for (int x = 0; x < data->width; x++) {
            for (int y = 0; y < data->height; y++) {
                SDL_Rect rect = {x, y, 1, 1};
                int c = data->internalRep[x][y];
                if (c >= 0) SDL_FillRect(data->surface, &rect, getRectC(data, c));
            }
        }

        METAENGINE_Render_Rect rect = {0, 0, (float) data->width, (float) data->height};
        METAENGINE_Render_UpdateImage(data->texture, &rect, data->surface, &rect);

        data->lastRenderNum = TestData::paletteNum;
        data->remapped = false;
    }

    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);

    METAENGINE_Render_Rect rect = {off(x, y)};

    int top = lua_gettop(L);
    if (top > 7) {
        METAENGINE_Render_Rect srcRect = {(float) luaL_checkinteger(L, 4),
                                          (float) luaL_checkinteger(L, 5),
                                          clamp(luaL_checkinteger(L, 6), 0, data->width),
                                          clamp(luaL_checkinteger(L, 7), 0, data->height)};

        int scale = luaL_checkinteger(L, 8);

        rect.w = srcRect.w * scale;
        rect.h = srcRect.h * scale;

        METAENGINE_Render_BlitRect(data->texture, &srcRect, TestData::bufferTarget, &rect);
    } else if (top > 6) {
        METAENGINE_Render_Rect srcRect = {
                (float) luaL_checkinteger(L, 4), (float) luaL_checkinteger(L, 5),
                (float) luaL_checkinteger(L, 6), (float) luaL_checkinteger(L, 7)};

        rect.w = srcRect.w;
        rect.h = srcRect.h;

        METAENGINE_Render_BlitRect(data->texture, &srcRect, TestData::bufferTarget, &rect);
    } else if (top > 3) {
        METAENGINE_Render_Rect srcRect = {0, 0, (float) luaL_checkinteger(L, 4),
                                          (float) luaL_checkinteger(L, 5)};

        rect.w = srcRect.w;
        rect.h = srcRect.h;

        METAENGINE_Render_BlitRect(data->texture, &srcRect, TestData::bufferTarget, &rect);
    } else {
        rect.w = data->width;
        rect.h = data->height;

        METAENGINE_Render_BlitRect(data->texture, nullptr, TestData::bufferTarget, &rect);
    }

    return 0;
}

static int freeImage(lua_State *L) {
    imageType *data = checkImage(L);
    if (!freeCheck(L, data)) return 0;

    for (int i = 0; i < data->width; i++) { delete data->internalRep[i]; }
    delete data->internalRep;

    SDL_FreeSurface(data->surface);
    METAENGINE_Render_FreeImage(data->texture);
    data->free = true;

    return 0;
}

static void internalDrawPixel(imageType *data, int x, int y, char c) {
    if (x >= 0 && y >= 0 && x < data->width && y < data->height) { data->internalRep[x][y] = c; }
}

static int imageGetPixel(lua_State *L) {
    imageType *data = checkImage(L);

    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);

    if (x < data->width && x >= 0 && y < data->height && y >= 0) {

        lua_pushinteger(L, data->internalRep[x][y] + 1);
    } else {
        lua_pushinteger(L, 0);
    }

    return 1;
}

static int imageRemap(lua_State *L) {
    imageType *data = checkImage(L);

    int oldColor = getColor(L, 2);
    int newColor = getColor(L, 3);

    data->remap[oldColor] = newColor;
    data->remapped = true;

    return 1;
}

static int imageDrawPixel(lua_State *L) {
    imageType *data = checkImage(L);
    if (!freeCheck(L, data)) return 0;

    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);

    char color = getColor(L, 4);

    if (color >= 0) {
        UInt32 rectColor = getRectC(data, color);
        SDL_Rect rect = {x, y, 1, 1};
        SDL_FillRect(data->surface, &rect, rectColor);
        internalDrawPixel(data, x, y, color);
    }
    return 0;
}

static int imageDrawRectangle(lua_State *L) {
    imageType *data = checkImage(L);
    if (!freeCheck(L, data)) return 0;

    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int w = luaL_checkinteger(L, 4);
    int h = luaL_checkinteger(L, 5);

    char color = getColor(L, 6);

    if (color >= 0) {
        UInt32 rectColor = getRectC(data, color);
        SDL_Rect rect = {x, y, w, h};
        SDL_FillRect(data->surface, &rect, rectColor);
        for (int xp = x; xp < x + w; xp++) {
            for (int yp = y; yp < y + h; yp++) { internalDrawPixel(data, xp, yp, color); }
        }
    }
    return 0;
}

static int imageBlitPixels(lua_State *L) {
    imageType *data = checkImage(L);
    if (!freeCheck(L, data)) return 0;

    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int w = luaL_checkinteger(L, 4);
    int h = luaL_checkinteger(L, 5);

    unsigned long long amt = lua_rawlen(L, -1);
    int len = w * h;
    if (amt < len) {
        luaL_error(L, "blitPixels expected %d pixels, got %d", len, amt);
        return 0;
    }

    for (int i = 1; i <= len; i++) {
        lua_pushnumber(L, i);
        lua_gettable(L, -2);
        if (!lua_isnumber(L, -1)) { luaL_error(L, "Index %d is non-numeric", i); }
        auto color = static_cast<int>(lua_tointeger(L, -1) - 1);
        if (color == -1) { continue; }

        color = color == -2
                        ? -1
                        : (color < 0 ? 0 : (color > (COLOR_LIMIT - 1) ? (COLOR_LIMIT - 1) : color));

        if (color >= 0) {
            int xp = (i - 1) % w;
            int yp = (i - 1) / w;

            UInt32 rectColor = getRectC(data, color);
            SDL_Rect rect = {x + xp, y + yp, 1, 1};
            SDL_FillRect(data->surface, &rect, rectColor);
            internalDrawPixel(data, x + xp, y + yp, static_cast<char>(color));
        }

        lua_pop(L, 1);
    }

    return 0;
}

static int imageClear(lua_State *L) {
    imageType *data = checkImage(L);
    if (!freeCheck(L, data)) return 0;

    UInt32 rectColor = SDL_MapRGBA(data->surface->format, 0, 0, 0, 0);
    SDL_FillRect(data->surface, nullptr, rectColor);
    for (int xp = 0; xp < data->width; xp++) {
        for (int yp = 0; yp < data->height; yp++) { internalDrawPixel(data, xp, yp, 0); }
    }

    return 0;
}

static int imageCopy(lua_State *L) {
    imageType *src = checkImage(L);

    void *ud = luaL_checkudata(L, 2, "MetaDot.Image");
    luaL_argcheck(L, ud != nullptr, 1, "`Image` expected");
    auto *dst = (imageType *) ud;

    int x = luaL_checkinteger(L, 3);
    int y = luaL_checkinteger(L, 4);
    int wi;
    int he;

    SDL_Rect srcRect;

    if (lua_gettop(L) > 4) {
        wi = luaL_checkinteger(L, 5);
        he = luaL_checkinteger(L, 6);
        //srcRect = { luaL_checkinteger(L, 7), luaL_checkinteger(L, 8), wi, he };
        srcRect.x = luaL_checkinteger(L, 7);
        srcRect.y = luaL_checkinteger(L, 8);
        srcRect.w = wi;
        srcRect.h = he;
    } else {
        //srcRect = { 0, 0, src->width, src->height };
        srcRect.x = 0;
        srcRect.y = 0;
        srcRect.w = src->width;
        srcRect.h = src->height;
        wi = src->width;
        he = src->height;
    }

    SDL_Rect rect = {x, y, wi, he};
    SDL_BlitSurface(src->surface, &srcRect, dst->surface, &rect);
    for (int xp = srcRect.x; xp < srcRect.x + srcRect.w; xp++) {
        for (int yp = srcRect.y; yp < srcRect.y + srcRect.h; yp++) {
            if (xp >= src->width || xp < 0 || yp >= src->height || yp < 0) { continue; }
            internalDrawPixel(dst, xp, yp, src->internalRep[xp][yp]);
        }
    }

    return 0;
}

static int imageToString(lua_State *L) {
    imageType *data = checkImage(L);
    if (data->free) {
        lua_pushstring(L, "Image(freed)");
    } else {
        lua_pushfstring(L, "Image(%dx%d)", data->width, data->height);
    }
    return 1;
}

static int imageGetWidth(lua_State *L) {
    imageType *data = checkImage(L);

    lua_pushinteger(L, data->width);
    return 1;
}

static int imageGetHeight(lua_State *L) {
    imageType *data = checkImage(L);

    lua_pushinteger(L, data->height);
    return 1;
}

static const luaL_Reg imageLib[] = {{"newImage", newImage}, {nullptr, nullptr}};

static const luaL_Reg imageLib_m[] = {{"__tostring", imageToString},
                                      {"free", freeImage},
                                      {"flush", flushImage},
                                      {"render", renderImage},
                                      {"clear", imageClear},
                                      {"drawPixel", imageDrawPixel},
                                      {"drawRectangle", imageDrawRectangle},
                                      {"blitPixels", imageBlitPixels},
                                      {"getPixel", imageGetPixel},
                                      {"remap", imageRemap},
                                      {"copy", imageCopy},
                                      {"getWidth", imageGetWidth},
                                      {"getHeight", imageGetHeight},
                                      {nullptr, nullptr}};

int metadot_bind_image(lua_State *L) {

    // imageLib_m
    // imageLib

    if (luaL_newmetatable(L, "MetaDot.Image")) {
        luaL_setfuncs(L, imageLib_m, 0);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");

        // lua_pushstring(L, "__index");
        // lua_pushvalue(L, -2); /* pushes the metatable */
        lua_settable(L, -3); /* metatable.__index = metatable */
        lua_pushstring(L, "__gc");
        lua_pushcfunction(L, freeImage);
        lua_settable(L, -3);
    }

    return 1;
}

#pragma endregion BindImage

#pragma region BindGPU

#define off(o, t) (float) ((o) -TestData::drawOffX), (float) ((t) -TestData::drawOffY)

static int gpu_getColor(lua_State *L, int arg) {
    int color = luaL_checkinteger(L, arg) - 1;
    return color < 0 ? 0 : (color > (COLOR_LIMIT - 1) ? (COLOR_LIMIT - 1) : color);
}

static int gpu_draw_pixel(lua_State *L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);

    int color = gpu_getColor(L, 3);

    METAENGINE_Color colorS = {TestData::palette[color][0], TestData::palette[color][1],
                               TestData::palette[color][2], 255};

    METAENGINE_Render_RectangleFilled(TestData::bufferTarget, off(x, y), off(x + 1, y + 1), colorS);

    return 0;
}

static int gpu_draw_rectangle(lua_State *L) {
    int color = gpu_getColor(L, 5);

    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);

    METAENGINE_Render_Rect rect = {off(x, y), (float) luaL_checkinteger(L, 3),
                                   (float) luaL_checkinteger(L, 4)};

    METAENGINE_Color colorS = {TestData::palette[color][0], TestData::palette[color][1],
                               TestData::palette[color][2], 255};
    METAENGINE_Render_RectangleFilled2(TestData::bufferTarget, rect, colorS);

    return 0;
}

static int gpu_blit_pixels(lua_State *L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int w = luaL_checkinteger(L, 3);
    int h = luaL_checkinteger(L, 4);

    unsigned long long amt = lua_rawlen(L, -1);
    int len = w * h;
    if (amt < len) {
        luaL_error(L, "blitPixels expected %d pixels, got %d", len, amt);
        return 0;
    }

    for (int i = 1; i <= len; i++) {
        lua_pushnumber(L, i);
        lua_gettable(L, -2);
        if (!lua_isnumber(L, -1)) { luaL_error(L, "Index %d is non-numeric", i); }
        int color = (int) lua_tointeger(L, -1) - 1;
        if (color == -1) {
            lua_pop(L, 1);
            continue;
        }

        color = color < 0 ? 0 : (color > (COLOR_LIMIT - 1) ? (COLOR_LIMIT - 1) : color);

        int xp = (i - 1) % w;
        int yp = (i - 1) / w;

        METAENGINE_Render_Rect rect = {off(x + xp, y + yp), 1, 1};

        METAENGINE_Color colorS = {TestData::palette[color][0], TestData::palette[color][1],
                                   TestData::palette[color][2], 255};
        METAENGINE_Render_RectangleFilled2(TestData::bufferTarget, rect, colorS);

        lua_pop(L, 1);
    }

    return 0;
}

static int gpu_set_clipping(lua_State *L) {
    if (lua_gettop(L) == 0) {
        METAENGINE_Render_SetClip(TestData::buffer->target, 0, 0, TestData::buffer->w,
                                  TestData::buffer->h);
        return 0;
    }

    auto x = static_cast<Sint16>(luaL_checkinteger(L, 1));
    auto y = static_cast<Sint16>(luaL_checkinteger(L, 2));
    auto w = static_cast<UInt16>(luaL_checkinteger(L, 3));
    auto h = static_cast<UInt16>(luaL_checkinteger(L, 4));

    METAENGINE_Render_SetClip(TestData::buffer->target, x, y, w, h);

    return 0;
}

static int gpu_set_palette_color(lua_State *L) {
    int slot = gpu_getColor(L, 1);

    auto r = static_cast<UInt8>(luaL_checkinteger(L, 2));
    auto g = static_cast<UInt8>(luaL_checkinteger(L, 3));
    auto b = static_cast<UInt8>(luaL_checkinteger(L, 4));

    TestData::palette[slot][0] = r;
    TestData::palette[slot][1] = g;
    TestData::palette[slot][2] = b;
    TestData::paletteNum++;

    return 0;
}

static int gpu_blit_palette(lua_State *L) {
    auto amt = (char) lua_rawlen(L, -1);
    if (amt < 1) { return 0; }

    amt = static_cast<char>(amt > COLOR_LIMIT ? COLOR_LIMIT : amt);

    for (int i = 1; i <= amt; i++) {
        lua_pushnumber(L, i);
        lua_gettable(L, -2);

        if (lua_type(L, -1) == LUA_TNUMBER) {
            lua_pop(L, 1);
            continue;
        }

        lua_pushnumber(L, 1);
        lua_gettable(L, -2);
        TestData::palette[i - 1][0] = static_cast<UInt8>(luaL_checkinteger(L, -1));

        lua_pushnumber(L, 2);
        lua_gettable(L, -3);
        TestData::palette[i - 1][1] = static_cast<UInt8>(luaL_checkinteger(L, -1));

        lua_pushnumber(L, 3);
        lua_gettable(L, -4);
        TestData::palette[i - 1][2] = static_cast<UInt8>(luaL_checkinteger(L, -1));

        lua_pop(L, 4);
    }

    TestData::paletteNum++;

    return 0;
}

static int gpu_get_palette(lua_State *L) {
    lua_newtable(L);

    for (int i = 0; i < COLOR_LIMIT; i++) {
        lua_pushinteger(L, i + 1);
        lua_newtable(L);
        for (int j = 0; j < 3; j++) {
            lua_pushinteger(L, j + 1);
            lua_pushinteger(L, TestData::palette[i][j]);
            lua_rawset(L, -3);
        }
        lua_rawset(L, -3);
    }

    return 1;
}

static int gpu_get_pixel(lua_State *L) {
    auto x = static_cast<Sint16>(luaL_checkinteger(L, 1));
    auto y = static_cast<Sint16>(luaL_checkinteger(L, 2));
    METAENGINE_Color col = METAENGINE_Render_GetPixel(TestData::buffer->target, x, y);
    for (int i = 0; i < COLOR_LIMIT; i++) {
        UInt8 *pCol = TestData::palette[i];
        if (col.r == pCol[0] && col.g == pCol[1] && col.b == pCol[2]) {
            lua_pushinteger(L, i + 1);
            return 1;
        }
    }

    lua_pushinteger(L, 1);// Should never happen
    return 1;
}

static int gpu_clear(lua_State *L) {
    if (lua_gettop(L) > 0) {
        int color = gpu_getColor(L, 1);
        METAENGINE_Color colorS = {TestData::palette[color][0], TestData::palette[color][1],
                                   TestData::palette[color][2], 255};
        METAENGINE_Render_ClearColor(TestData::bufferTarget, colorS);
    } else {
        METAENGINE_Color colorS = {TestData::palette[0][0], TestData::palette[0][1],
                                   TestData::palette[0][2], 255};
        METAENGINE_Render_ClearColor(TestData::bufferTarget, colorS);
    }

    return 0;
}

int *translateStack;
int tStackUsed = 0;
int tStackSize = 32;

static int gpu_translate(lua_State *L) {
    TestData::drawOffX -= luaL_checkinteger(L, -2);
    TestData::drawOffY -= luaL_checkinteger(L, -1);

    return 0;
}

static int gpu_push(lua_State *L) {
    if (tStackUsed == tStackSize) {
        tStackSize *= 2;
        translateStack = (int *) realloc(translateStack, tStackSize * sizeof(int));
    }

    translateStack[tStackUsed] = TestData::drawOffX;
    translateStack[tStackUsed + 1] = TestData::drawOffY;

    tStackUsed += 2;

    return 0;
}

static int gpu_pop(lua_State *L) {
    if (tStackUsed > 0) {
        tStackUsed -= 2;

        TestData::drawOffX = translateStack[tStackUsed];
        TestData::drawOffY = translateStack[tStackUsed + 1];

        lua_pushboolean(L, true);
        lua_pushinteger(L, tStackUsed / 2);
        return 2;
    } else {
        lua_pushboolean(L, false);
        return 1;
    }
}

static int gpu_set_fullscreen(lua_State *L) {
    auto fsc = static_cast<bool>(lua_toboolean(L, 1));
    if (!METAENGINE_Render_SetFullscreen(fsc, true)) {
        TestData::pixelScale = TestData::setPixelScale;
        METAENGINE_Render_SetWindowResolution(TestData::pixelScale * SCRN_WIDTH,
                                              TestData::pixelScale * SCRN_HEIGHT);

        SDL_SetWindowPosition(global.platform.window, TestData::lastWindowX, TestData::lastWindowY);
    }

    TestData::assessWindow();

    return 0;
}

static int gpu_swap(lua_State *L) {
    METAENGINE_Color colorS = {TestData::palette[0][0], TestData::palette[0][1],
                               TestData::palette[0][2], 255};
    METAENGINE_Render_ClearColor(TestData::renderer, colorS);

    //TestData::shader::updateShader();

    METAENGINE_Render_BlitScale(TestData::buffer, nullptr, TestData::renderer,
                                TestData::windowWidth / 2, TestData::windowHeight / 2,
                                TestData::pixelScale, TestData::pixelScale);

    METAENGINE_Render_Flip(TestData::renderer);

    METAENGINE_Render_DeactivateShaderProgram();

    return 0;
}

static const luaL_Reg gpuLib[] = {{"setPaletteColor", gpu_set_palette_color},
                                  {"blitPalette", gpu_blit_palette},
                                  {"getPalette", gpu_get_palette},
                                  {"drawPixel", gpu_draw_pixel},
                                  {"drawRectangle", gpu_draw_rectangle},
                                  {"blitPixels", gpu_blit_pixels},
                                  {"translate", gpu_translate},
                                  {"push", gpu_push},
                                  {"pop", gpu_pop},
                                  {"setFullscreen", gpu_set_fullscreen},
                                  {"getPixel", gpu_get_pixel},
                                  {"clear", gpu_clear},
                                  {"swap", gpu_swap},
                                  {"clip", gpu_set_clipping},
                                  {nullptr, nullptr}};

int metadot_bind_gpu(lua_State *L) {
    translateStack = new int[32];

    if (luaL_newmetatable(L, "MetaDot.GPU")) {
        luaL_setfuncs(L, gpuLib, 0);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");

        lua_pushnumber(L, SCRN_WIDTH);
        lua_setfield(L, -2, "width");
        lua_pushnumber(L, SCRN_HEIGHT);
        lua_setfield(L, -2, "height");
        return 1;
    }
}

#pragma endregion BindGPU