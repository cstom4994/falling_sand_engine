// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_FILESYSTEM_HPP_
#define _METADOT_FILESYSTEM_HPP_

#include <cstring>
#include <string_view>
#include <filesystem>

#include "core/core.hpp"
#include "core/macros.hpp"
#include "core/platform.h"
#include "core/stl/stl.h"

// #define FUTIL_ASSERT_EXIST(stringPath)                                                             \
//     ME_ASSERT(FUtil_exists(METADOT_RESLOC(stringPath)),                                       \
//                    ("%s", MetaEngine::Format("FILE: {0} does not exist", stringPath)))

#define FUTIL_ASSERT_EXIST(stringPath)

bool InitFilesystem();

#define METADOT_RESLOC(x) x

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

//--------------------------------------------------------------------------------------------------
// Path helper functions.
// These are implemented with the C-string API in "cute_string.h"; each function returns a fully
// mutable string you must free up with `sfree` or `metadot_string_free` when done.

#ifndef METAENGINE_NO_SHORTHAND_API
/**
 * Returns the filename portion of a path. Returns a new string.
 *
 * Example:
 *
 *     const char* filename = spfname("/data/collections/rare/big_gem.txt");
 *     printf("%s\n", filename);
 *
 * Would print:
 *
 *     big_gem.txt
 */
#define spfname(s) metadot_path_get_filename(s)

/**
 * Returns the filename portion of a path without the file extension.
 * Returns a new string.
 *
 * Example:
 *
 *     const char* filename = spfname("/data/collections/rare/big_gem.txt");
 *     printf("%s\n", filename);
 *
 * Would print:
 *
 *     big_gem
 */
#define spfname_no_ext(s) metadot_path_get_filename_no_ext(s)

/**
 * Returns the extension of the file for the given path. Returns a new string.
 *
 * Example:
 *
 *     const char* ext = spfname("/data/collections/rare/big_gem.txt");
 *     printf("%s\n", ext);
 *
 * Would print:
 *
 *     .txt
 */
#define spext(s) metadot_path_get_ext(s)

/**
 * Removes the rightmost file or directory from the path. If the string is not a
 * dynamic string from CF's string API, a new string is returned. Otherwise the
 * string is modified in-place.
 */
#define sppop(s) metadot_path_pop(s)

/**
 * Removes the rightmost n files or directories from the path. If the string is
 * not a dynamic string from CF's string API, a new string is returned. Otherwise
 * the string is modified in-place.
 */
#define sppopn(s, n) metadot_path_pop_n(s, n)

/**
 * Squishes the path to be less than or equal to n characters in length. This will
 * insert ellipses "..." into the path as necessary. This function is useful for
 * displaying paths and visualizing them in small boxes or windows. n includes the
 * nul-byte. Returns a new string.
 */
#define spcompact(s, n) metadot_path_compact(s, n)

#define spdir_of(s) metadot_path_directory_of(s)

#define sptop_of(s) metadot_path_top_directory(s)

#define spnorm(s) metadot_path_normalize(s)
#endif  // METAENGINE_NO_SHORTHAND_API

char* METADOT_CDECL metadot_path_get_filename(const char* path);
char* METADOT_CDECL metadot_path_get_filename_no_ext(const char* path);
char* METADOT_CDECL metadot_path_get_ext(const char* path);
char* METADOT_CDECL metadot_path_pop(const char* path);
char* METADOT_CDECL metadot_path_pop_n(const char* path, int n);
char* METADOT_CDECL metadot_path_compact(const char* path, int n);
char* METADOT_CDECL metadot_path_directory_of(const char* path);
char* METADOT_CDECL metadot_path_top_directory(const char* path);
char* METADOT_CDECL metadot_path_normalize(const char* path);

#ifdef __cplusplus
}
#endif  // __cplusplus

std::vector<char> fs_read_entire_file_to_memory(const char* path, size_t& size);
std::string normalizePath(const std::string& messyPath);

#endif