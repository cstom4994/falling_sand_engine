// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_FILESYSTEM_HPP_
#define _METADOT_FILESYSTEM_HPP_

#include <cstring>
#include <string_view>

#include "core/core.hpp"
#include "core/macros.h"
#include "core/platform.h"
#include "core/stl.h"

// #define FUTIL_ASSERT_EXIST(stringPath)                                                             \
//     METADOT_ASSERT(FUtil_exists(METADOT_RESLOC(stringPath)),                                       \
//                    ("%s", MetaEngine::Format("FILE: {0} does not exist", stringPath)))

#define FUTIL_ASSERT_EXIST(stringPath)

bool InitFilesystem();

R_bool InitPhysFS();  // Initialize the PhysFS file system
R_bool InitPhysFSEx(const char *newDir,
                    const char *mountPoint);  // Initialize the PhysFS file system with a mount point.
R_bool ClosePhysFS();                         // Close the PhysFS file system
R_bool IsPhysFSReady();                       // Check if PhysFS has been initialized successfully
R_bool MountPhysFS(const char *newDir,
                   const char *mountPoint);  // Mount the given directory or archive as a mount point
R_bool MountPhysFSFromMemory(const unsigned char *fileData, int dataSize, const char *newDir,
                             const char *mountPoint);  // Mount the given file data as a mount point
R_bool UnmountPhysFS(const char *oldDir);              // Unmounts the given directory
R_bool FileExistsInPhysFS(const char *fileName);       // Check if the given file exists in PhysFS
R_bool DirectoryExistsInPhysFS(const char *dirPath);   // Check if the given directory exists in PhysFS
unsigned char *LoadFileDataFromPhysFS(const char *fileName,
                                      unsigned int *bytesRead);  // Load a data buffer from PhysFS (memory should be freed)
char *LoadFileTextFromPhysFS(const char *fileName);              // Load text from a file (memory should be freed)
R_bool SetPhysFSWriteDirectory(const char *newDir);              // Set the base directory where PhysFS should write files to (defaults to the current working directory)
R_bool SaveFileDataToPhysFS(const char *fileName, void *data,
                            unsigned int bytesToWrite);  // Save the given file data in PhysFS
R_bool SaveFileTextToPhysFS(const char *fileName,
                            char *text);  // Save the given file text in PhysFS

long GetFileModTimeFromPhysFS(const char *fileName);  // Get file modification time (last write time) from PhysFS

void SetPhysFSCallbacks();  // Set the raylib file loader/saver callbacks to use PhysFS
const char *GetPerfDirectory(const char *organization,
                             const char *application);  // Get the user's current config directory for the application.


#define METADOT_RESLOC(x) x

inline R_bool FUtil_exists(const char* path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0 || S_ISDIR(buffer.st_mode));
}

// folder path of current executable
const char* metadot_fs_getExecutableFolderPath();

inline const char* FUtil_GetFileName(const char* path) {
    int len = strlen(path);
    int flag = 0;

    for (int i = len - 1; i > 0; i--) {
        if (path[i] == '\\' || path[i] == '//' || path[i] == '/') {
            flag = 1;
            path = path + i + 1;
            break;
        }
    }
    return path;
}

char* metadot_fs_readfilestring(const char* path);
void metadot_fs_freestring(void* ptr);

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

METAENGINE_Result METADOT_CDECL metadot_fs_init(const char* argv0);
void METADOT_CDECL metadot_fs_destroy();

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

/**
 * Returns the directory of a given file or directory. Returns a new string.
 *
 * Example:
 *
 *     const char* filename = spfname("/data/collections/rare/big_gem.txt");
 *     printf("%s\n", filename);
 *
 * Would print:
 *
 *     /rare
 */
#define spdir_of(s) metadot_path_directory_of(s)

/**
 * Returns the directory of a given file or directory. Returns a new string.
 *
 * Example:
 *
 *     const char* filename = spfname("/data/collections/rare/big_gem.txt");
 *     printf("%s\n", filename);
 *
 * Would print:
 *
 *     /data
 */
#define sptop_of(s) metadot_path_top_directory(s)

/**
 * Normalizes a path. This means a few specific things:
 *
 * - All '\\' are replaced with '/'.
 * - Any duplicate '////' are replaced with a single '/'.
 * - Trailing '/' are removed.
 * - Dot folders are resolved, e.g.
 *
 *    spnorm("/a/b/./c") -> "/a/b/c"
 *    spnorm("/a/b/../c") -> "/a/c"
 *
 * - The first character is always '/', unless it's a windows drive, e.g.
 *
 *    spnorm("C:\\Users\\Randy\\Documents") -> "C:/Users/Randy/Documents"
 */
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

//--------------------------------------------------------------------------------------------------
// Virtual file system.

/**
 * MetaEngine Framework (CF) uses a virtual file system. This has a bunch of benefits over directly
 * accessing files. CF's file system is a wrap layer over PhysFS (https://icculus.org/physfs/).
 *
 * - More safe + secure.
 * - More portable.
 * - More versatile.
 *
 * Your game gets one directory to write files, and no other. This prevents the game from
 * accessing any other areas of the disk, keeping things simple and secure. You can set the
 * write directory with `metadot_fs_set_write_directory`. During development you can set this
 * directory to wherever you like, but when shipping your game it's highly recommended to
 * set the write directory as `metadot_fs_get_user_directory`. This directory is guaranteed to be
 * a write-enabled and safe place to store game-specific files for your player.
 *
 * File paths such as "./" and "../ are not allowed as they open up a variety of security
 * holes. Additionally, Windows-slashes "\\" or colons ":" are not allowed in file paths
 * either. The top level folder can start with or without a slash. For example, these are
 * both valid paths.
 *
 *     "/content/images/tree.png"
 *     "content/images/tree.png"
 *
 * Drive letters are entirely hidden. Instead the file system uses a virtual path. You must
 * add any folders you want to access onto the path. Whenever CF searches for a file, it
 * searches the path in alphabetical order and returns the first match found. To add a
 * directory to the path use `metadot_fs_mount`. For example, let's say our game's content files
 * are laid out something like this:
 *
 *     /content
 *     --/images
 *     --/sounds
 *     --/scripts
 *
 * A good method is to mount the content folder as a blank string, like this:
 *
 *     metadot_fs_mount("/content", "", true);
 *
 * From here on whenever we want to find something in the content folder, it will be treated
 * as the top-level directory to search from. We can open one of the images or sounds like so:
 *
 *     metadot_fs_open_file_for_read("/images/flower.png");
 *     metadot_fs_open_file_for_read("/sounds/laugh.wav");
 *
 * This grants a lot of flexibility. We can move entire directories around on disk and then
 * rename the mount point without changing the rest of the code.
 *
 * Mounting can reference archive files. Here's a list of supported archive formats.
 *
 *    .ZIP (pkZip/WinZip/Info-ZIP compatible)
 *    .7Z  (7zip archives)
 *    .ISO (ISO9660 files, CD-ROM images)
 *    .GRP (Build Engine groupfile archives)
 *    .PAK (Quake I/II archive format)
 *    .HOG (Descent I/II HOG file archives)
 *    .MVL (Descent II movielib archives)
 *    .WAD (DOOM engine archives)
 *    .VDF (Gothic I/II engine archives)
 *    .SLB (Independence War archives)
 *
 * Whenever an archive is mounted the file system treats it like a normal directory. No extra
 * work is needed. This lets us do really cool things, like deploy patches by downloading
 * new archive files and appending them to an earlier place in the search path. This also works
 * to add mod support to your game, and provides a simple way of storing multiple versions of
 * a single file without overwriting each other on the actual disk.
 *
 * By default CF mounts the base directory when you call `make_app`. This can be disabled by
 * passing the `APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT` flag to `make_app`.
 */

typedef struct METAENGINE_File METAENGINE_File;

#define METAENGINE_FILE_TYPE_DEFS           \
    METAENGINE_ENUM(FILE_TYPE_REGULAR, 0)   \
    METAENGINE_ENUM(FILE_TYPE_DIRECTORY, 1) \
    METAENGINE_ENUM(FILE_TYPE_SYMLINK, 2)   \
    METAENGINE_ENUM(FILE_TYPE_OTHER, 3)

typedef enum METAENGINE_FileType {
#define METAENGINE_ENUM(K, V) METAENGINE_##K = V,
    METAENGINE_FILE_TYPE_DEFS
#undef METAENGINE_ENUM
} METAENGINE_FileType;

METADOT_INLINE const char* metadot_file_type_to_string(METAENGINE_FileType type) {
    switch (type) {
#define METAENGINE_ENUM(K, V) \
    case METAENGINE_##K:      \
        return METAENGINE_STRINGIZE(METAENGINE_##K);
        METAENGINE_FILE_TYPE_DEFS
#undef METAENGINE_ENUM
        default:
            return NULL;
    }
}

typedef struct METAENGINE_Stat {
    METAENGINE_FileType type;
    int is_read_only;
    size_t size;
    uint64_t last_modified_time;
    uint64_t created_time;
    uint64_t last_accessed_time;
} METAENGINE_Stat;

/**
 * Sets a path safe to store game-specific files, such as save data or profiles. The path is in
 * platform-dependent notation. It's highly recommend to use `metadot_fs_get_user_directory` and pass
 * it into this function when shipping your game. This function will fail if any files are from
 * the write directory are currently open.
 */
METAENGINE_Result METADOT_CDECL metadot_fs_set_write_directory(const char* platform_dependent_directory);

/**
 * Returns a path safe to store game-specific files, such as save data or profiles. The path is
 * in platform-dependent notation. The location of this folder varies depending on the OS. You
 * should probably pass this into `metadot_fs_set_write_directory` as well as `metadot_fs_mount`.
 *
 *     Windows example:
 *     "C:\\Users\\OS_user_name\\AppData\\Roaming\\my_company\\my_game"
 *
 *     Linux example:
 *     "/home/OS_user_name/.local/share/my_game"
 *
 *     MacOS X example:
 *     "/Users/OS_user_name/Library/Application Support/my_game"
 *
 * You should assume this directory is the only safe place to write files.
 */
const char* METADOT_CDECL metadot_fs_get_user_directory(const char* company_name, const char* game_name);

/**
 * Adds a new archive/directory onto the search path.
 *
 * archive        - Platform-dependent notation. The archive or directory to mount.
 * mount_point    - The new virtual path for `archive`.
 * append_to_path - If true `mount_point` is appended onto the end of the path. If false it
 *                  will be prepended.
 *
 * Each individual archive can only be mounted once. Duplicate mount attempts will be ignored.
 *
 * You can mount multiple archives onto a single mount point. This is a great way to support
 * modding or download patches, as duplicate entries will be searched for on the path as normal,
 * without the need to overwrite each other on the actual disk.
 *
 * You can mount an actual directory or an archive file. If it's an archive the vitrual file
 * system will treat it like a normal directory for you. There are a variety of archive file
 * formats supported (see top of file).
 *
 * By default CF mounts the base directory when you call `make_app`. This can be disabled by
 * passing the `APP_OPTIONS_FILE_SYSTEM_DONT_DEFAULT_MOUNT` flag to `make_app`.
 */
METAENGINE_Result METADOT_CDECL metadot_fs_mount(const char* archive, const char* mount_point, bool append_to_path /*= true*/);

/**
 * Removes an archive from the path, specified in platform-dependent notation. This function
 * does not remove a `mount_point` from the virtual file system, but only the actual archive
 * that was previously mounted.
 */
METAENGINE_Result METADOT_CDECL metadot_fs_dismount(const char* archive);

/**
 * Fetches file information at the given virtual path, such as file size or creation time.
 */
METAENGINE_Result METADOT_CDECL metadot_fs_stat(const char* virtual_path, METAENGINE_Stat* stat);

/**
 * Opens a file for writing relative to the write directory.
 * The write directory is specified by you when calling `metadot_fs_set_write_directory`.
 */
METAENGINE_File* METADOT_CDECL metadot_fs_create_file(const char* virtual_path);

/**
 * Opens a file for writing relative to the write directory.
 * The write directory is specified by you when calling `metadot_fs_set_write_directory`.
 */
METAENGINE_File* METADOT_CDECL metadot_fs_open_file_for_write(const char* virtual_path);

/**
 * Opens a file for appending relative to the write directory.
 * The write directory is specified by you when calling `metadot_fs_set_write_directory`.
 */
METAENGINE_File* METADOT_CDECL metadot_fs_open_file_for_append(const char* virtual_path);

/**
 * Opens a file for reading. If you just want some basic information about the file (such as
 * it's size or when it was created), you can use `metadot_fs_stat` instead.
 */
METAENGINE_File* METADOT_CDECL metadot_fs_open_file_for_read(const char* virtual_path);

/**
 * Close a file.
 */
METAENGINE_Result METADOT_CDECL metadot_fs_close(METAENGINE_File* file);

/**
 * Deletes a file or directory. The directory must be empty, otherwise this function
 * will fail.
 */
METAENGINE_Result METADOT_CDECL metadot_fs_delete(const char* virtual_path);

/**
 * Creates a directory at the path. All missing directories are also created.
 */
METAENGINE_Result METADOT_CDECL metadot_fs_create_directory(const char* virtual_path);

/**
 * Returns a list of files and directories in the given directory. The list is sorted.
 * Results are collected by visiting the search path for all real directories mounted
 * on `virtual_path`. No duplicate file names will be reported. The list itself is
 * sorted alphabetically, though you can further sort it however you like. Free the list
 * up with `metadot_fs_free_enumerated_directory` when done. The final element of the list
 * is NULL.
 *
 * Example to loop over a list:
 *
 *     const char** list = metadot_fs_enumerate_directory("/data");
 *     for (const char** i = list; *i; ++i) {
 *         printf("Found %s\n", *i);
 *     }
 */
const char** METADOT_CDECL metadot_fs_enumerate_directory(const char* virtual_path);

/**
 * Frees a file list from `metadot_fs_create_directory`.
 */
void METADOT_CDECL metadot_fs_free_enumerated_directory(const char** directory_list);

/**
 * Frees a file list from `metadot_fs_create_directory`.
 */
bool METADOT_CDECL metadot_fs_file_exists(const char* virtual_path);

/**
 * Reads bytes from a file opened in read mode. The file must be opened in read mode
 * with `metadot_fs_open_file_for_read`. Returns the number of bytes read. Returns -1 on failure.
 */
size_t METADOT_CDECL metadot_fs_read(METAENGINE_File* file, void* buffer, size_t bytes);

/**
 * Writes bytes from a file opened in write mode. The file must be opened in write mode
 * with `metadot_fs_open_file_for_write`. Returns the number of bytes written. Returns -1 on failure.
 */
size_t METADOT_CDECL metadot_fs_write(METAENGINE_File* file, const void* buffer, size_t bytes);

/**
 * Check to see if the eof has been found after reading a file opened in read mode.
 */
METAENGINE_Result METADOT_CDECL metadot_fs_eof(METAENGINE_File* file);

/**
 * Returns the current position within the file. This is an offset from the beginning of
 * the file. Returns -1 on failure.
 */
size_t METADOT_CDECL metadot_fs_tell(METAENGINE_File* file);

/**
 * Sets the current position within a file. This is an offset from the beginning of the file.
 * The next read or write will happen at this position.
 */
METAENGINE_Result METADOT_CDECL metadot_fs_seek(METAENGINE_File* file, size_t position);

/**
 * Returns the size of a file in bytes. You might want to use `metadot_fs_stat` instead.
 */
size_t METADOT_CDECL metadot_fs_size(METAENGINE_File* file);

/**
 * Reads an entire file into a buffer of memory, and returns the buffer to you. Call `METAENGINE_FW_FREE`
 * on it when done.
 */
void* METADOT_CDECL metadot_fs_read_entire_file_to_memory(const char* virtual_path, size_t* size);

/**
 * Reads an entire file into a buffer of memory, and returns the buffer to you as a nul-term-
 * inated C string. Call `METAENGINE_FW_FREE` on it when done.
 */
char* METADOT_CDECL metadot_fs_read_entire_file_to_memory_and_nul_terminate(const char* virtual_path, size_t* size);

/**
 * Writes an entire buffer of data to a file as binary data.
 */
METAENGINE_Result METADOT_CDECL metadot_fs_write_entire_buffer_to_file(const char* virtual_path, const void* data, size_t size);

/**
 * Feel free to call this whenever an error occurs in any of the file system functions
 * to try and get a detailed description on what might have happened. Often times this
 * string is already returned to you inside a `METAENGINE_Result`.
 */
const char* METADOT_CDECL metadot_fs_get_backend_specific_error_message();

/**
 * Converts a virtual path to an actual path. This can be useful for editors, asset
 * hotloading, or other similar development features. When shipping your game it's highly
 * recommended to not call this function at all, and only use it for development
 * purposes.
 *
 * If the virtual path points to a completely fake directory this will return the first
 * archive found there.
 *
 * This function can point to a directory, an archive, a file, or NULL if nothing
 * suitable was found at all.
 */
const char* METADOT_CDECL metadot_fs_get_actual_path(const char* virtual_path);

#ifdef __cplusplus
}
#endif  // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

namespace MetaEngine {

using Stat = METAENGINE_Stat;
using File = METAENGINE_File;

using FileType = METAENGINE_FileType;
#define METAENGINE_ENUM(K, V) METADOT_INLINE constexpr FileType K = METAENGINE_##K;
METAENGINE_FILE_TYPE_DEFS
#undef METAENGINE_ENUM

METADOT_INLINE const char* to_string(FileType type) {
    switch (type) {
#define METAENGINE_ENUM(K, V) \
    case METAENGINE_##K:      \
        return #K;
        METAENGINE_FILE_TYPE_DEFS
#undef METAENGINE_ENUM
        default:
            return NULL;
    }
}

// METADOT_INLINE const char* fs_get_base_dir() { return metadot_fs_get_base_directory(); }
METADOT_INLINE Result fs_set_write_dir(const char* platform_dependent_directory) { return metadot_fs_set_write_directory(platform_dependent_directory); }
METADOT_INLINE Result fs_mount(const char* archive, const char* mount_point, bool append_to_path = true) { return metadot_fs_mount(archive, mount_point, append_to_path); }
METADOT_INLINE Result fs_dismount(const char* archive) { return metadot_fs_dismount(archive); }
METADOT_INLINE Result fs_stat(const char* virtual_path, Stat* stat) { return metadot_fs_stat(virtual_path, stat); }
METADOT_INLINE File* fs_create_file(const char* virtual_path) { return metadot_fs_create_file(virtual_path); }
METADOT_INLINE File* fs_open_file_for_write(const char* virtual_path) { return metadot_fs_open_file_for_write(virtual_path); }
METADOT_INLINE File* fs_open_file_for_append(const char* virtual_path) { return metadot_fs_open_file_for_append(virtual_path); }
METADOT_INLINE File* fs_open_file_for_read(const char* virtual_path) { return metadot_fs_open_file_for_read(virtual_path); }
METADOT_INLINE Result fs_close(File* file) { return metadot_fs_close(file); }
METADOT_INLINE Result fs_delete(const char* virtual_path) { return metadot_fs_delete(virtual_path); }
METADOT_INLINE Result fs_create_dir(const char* virtual_path) { return metadot_fs_create_directory(virtual_path); }
METADOT_INLINE const char** fs_enumerate_directory(const char* virtual_path) { return metadot_fs_enumerate_directory(virtual_path); }
METADOT_INLINE void fs_free_enumerated_directory(const char** directory_list) { metadot_fs_free_enumerated_directory(directory_list); }
METADOT_INLINE bool fs_file_exists(const char* virtual_path) { return metadot_fs_file_exists(virtual_path); }
METADOT_INLINE size_t fs_read(File* file, void* buffer, size_t bytes) { return metadot_fs_read(file, buffer, bytes); }
METADOT_INLINE size_t fs_write(File* file, const void* buffer, size_t bytes) { return metadot_fs_write(file, buffer, bytes); }
METADOT_INLINE Result fs_eof(File* file) { return metadot_fs_eof(file); }
METADOT_INLINE size_t fs_tell(File* file) { return metadot_fs_tell(file); }
METADOT_INLINE Result fs_seek(File* file, size_t position) { return metadot_fs_seek(file, position); }
METADOT_INLINE size_t fs_size(File* file) { return metadot_fs_size(file); }
METADOT_INLINE void* fs_read_entire_file_to_memory(const char* virtual_path, size_t* size = NULL) { return metadot_fs_read_entire_file_to_memory(virtual_path, size); }
METADOT_INLINE char* fs_read_entire_file_to_memory_and_nul_terminate(const char* virtual_path, size_t* size = NULL) {
    return metadot_fs_read_entire_file_to_memory_and_nul_terminate(virtual_path, size);
}
METADOT_INLINE Result fs_write_entire_buffer_to_file(const char* virtual_path, const void* data, size_t size) { return metadot_fs_write_entire_buffer_to_file(virtual_path, data, size); }
METADOT_INLINE const char* fs_get_backend_specific_error_message() { return metadot_fs_get_backend_specific_error_message(); }
METADOT_INLINE const char* fs_get_user_directory(const char* org, const char* app) { return metadot_fs_get_user_directory(org, app); }
METADOT_INLINE const char* fs_get_actual_path(const char* virtual_path) { return metadot_fs_get_actual_path(virtual_path); }

}  // namespace MetaEngine

#endif
