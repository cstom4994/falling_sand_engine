// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "filesystem.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include "core/alloc.h"
#include "core/core.h"
#include "core/cpp/utils.hpp"
#include "core/platform.h"
#include "datapackage.h"
#include "engine.h"
#include "engine_platform.h"
#include "libs/physfs/physfs.h"

IMPLENGINE();

bool InitFilesystem() {

    METAENGINE_Result err = metadot_fs_init(NULL);

    auto currentDir = std::filesystem::path(metadot_fs_getExecutableFolderPath());

#ifdef METADOT_DEBUG

    for (int i = 0; i < 3; ++i) {
        currentDir = currentDir.parent_path();
        if (std::filesystem::exists(currentDir / "Data")) {
            Core.gamepath = currentDir;
            // s_DataPath = currentDir / "Data";
            METADOT_INFO("Game data path detected: %s (Base: %s)", Core.gamepath.c_str(), metadot_fs_getExecutableFolderPath());

            if (metadot_is_error(err)) {
                METADOT_ASSERT_E(0);
            } else if (true) {
                // Put the base directory (the path to the exe) onto the file system search path.
                metadot_fs_mount(Core.gamepath.c_str(), "", true);
            }

            return METADOT_OK;
        }
    }

    METADOT_ERROR("Game data path detect failed");
    return METADOT_FAILED;

#else

    if (!std::filesystem::exists(currentDir / "Data")) {
        METADOT_ERROR("Game data path detect failed");
        return METADOT_FAILED;
    } else {
        Core.gamepath = currentDir;
        METADOT_INFO("Game data path detected: %s", Core.gamepath.c_str());
        return METADOT_OK;
    }

#endif
}

const char* metadot_fs_getExecutableFolderPath() {
    const char* out = PHYSFS_getBaseDir();
    return out;
}

// Char pointer get form futil_readfilestring must be gc_free manually
char* metadot_fs_readfilestring(const char* path) {
    char* source = NULL;
    FILE* fp = fopen(METADOT_RESLOC(path), "r");
    if (fp != NULL) {
        /* Go to the end of the file. */
        if (fseek(fp, 0L, SEEK_END) == 0) {
            /* Get the size of the file. */
            long bufsize = ftell(fp);
            if (bufsize == -1) { /* Error */
            }

            /* Allocate our buffer to that size. */
            source = (char*)gc_malloc(&gc, sizeof(char) * (bufsize + 1));

            /* Go back to the start of the file. */
            if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */
            }

            /* Read the entire file into memory. */
            size_t newLen = fread(source, sizeof(char), bufsize, fp);
            if (ferror(fp) != 0) {
                fputs("Error reading file", stderr);
                METADOT_ERROR("Error reading file %s", METADOT_RESLOC(path));
            } else {
                source[newLen++] = '\0'; /* Just to be safe. */
            }
        }
        fclose(fp);
        return source;
    }
    gc_free(&gc, source); /* Don't forget to call free() later! */
    return R_null;
}

void metadot_fs_freestring(void* ptr) {
    if (NULL != ptr) gc_free(&gc, ptr);
}

#include "core/alloc.h"
#include "core/stl.h"
#include "libs/physfs/physfs.h"

#define METAENGINE_FILE_SYSTEM_BUFFERED_IO_SIZE (2 * METAENGINE_MB)

char* metadot_path_get_filename(const char* path) {
    int at = slast_index_of(path, '/');
    if (at == -1) return NULL;
    return smake(path + at + 1);
}

char* metadot_path_get_filename_no_ext(const char* path) {
    int at = slast_index_of(path, '.');
    if (at == -1) return NULL;
    char* s = (char*)metadot_path_get_filename(path);
    at = slast_index_of(s, '.');
    if (at == -1 || at == 0) {
        sfree(s);
        return NULL;
    }
    serase(s, at, slen(s) - at);
    return s;
}

char* metadot_path_get_ext(const char* path) {
    int at = slast_index_of(path, '.');
    if (at == -1 || path[at + 1] == 0 || path[at + 1] == '/') return NULL;
    return smake(path + at);
}

char* metadot_path_pop(const char* path) {
    char* in = (char*)path;
    if (sisdyna(in)) {
        if (slast(in) == '/') spop(in);
        int at = slast_index_of(path, '/');
        if (at == -1 || at == 0) return sset(in, "/");
        serase(in, at, slen(in) - at);
        return in;
    } else {
        char* s = sdup(path);
        if (slast(s) == '/') spop(s);
        int at = slast_index_of(path, '/');
        if (at == -1 || at == 0) return sset(s, "/");
        serase(s, at, slen(s) - at);
        return s;
    }
}

char* metadot_path_pop_n(const char* path, int n) {
    char* s = (char*)path;
    while (n--) {
        s = sppop(s);
    }
    return s;
}

char* metadot_path_compact(const char* path, int n) {
    int len = (int)METAENGINE_STRLEN(path);
    if (n <= 6) return NULL;
    if (len < n) return sdup(path);
    int at = slast_index_of(path, '/');
    if (at == -1 || at == 0) {
        char* s = sdup(path);
        serase(s, n, slen(s) - n);
        serase(s, n - 3, 3);
        return sappend(s, "...");
    }
    int remaining = len - at - 1;
    if (remaining >= n - 3) {
        char* s = smake("...");
        sappend_range(s, path, path + at - 6);
        return sappend(s, "...");
    } else {
        char* s = sdup(path);
        int len_s = slen(s);
        int to_erase = len_s - (remaining - 3);
        serase(s, remaining - 3, to_erase);
        sappend(s, "...");
        return sappend(s, path + at);
    }
}

char* metadot_path_directory_of(const char* path) {
    if (!*path || *path == '.' && METAENGINE_STRLEN(path) < 3) return NULL;
    if (sequ(path, "../")) return NULL;
    if (sequ(path, "/")) return NULL;
    int at = slast_index_of(path, '/');
    if (at == -1) return NULL;
    if (at == 0) return smake("/");
    char* s = smake(path);
    serase(s, at, slen(s) - at);
    at = slast_index_of(s, '/');
    if (at == -1) {
        int l = slen(s);
        if (slen(s) == 2) {
            return s;
        } else {
            s[0] = '/';
            return s;
        }
    }
    return serase(s, 0, at);
}

char* metadot_path_top_directory(const char* path) {
    int at = sfirst_index_of(path, '/');
    if (at == -1) return NULL;
    int next = sfirst_index_of(path + at + 1, '/');
    if (next == -1) return smake("/");
    char* s = sdup(path);
    return serase(s, next + 1, slen(s) - (next + 1));
}

char* metadot_path_normalize(const char* path) {
    char* result = NULL;
    int len = (int)METAENGINE_STRLEN(path);
    if (*path != '\\' && *path != '/') {
        bool windows_drive = len >= 2 && path[1] == ':';
        if (!windows_drive) {
            spush(result, '/');
        }
    }
    bool prev_was_dot = false;
    bool prev_was_dotdot = false;
    for (int i = 0; i < len; ++i) {
        char c = path[i];
        int l = slen(result);
        if (c == '\\' || c == '/') {
            if (!prev_was_dot) {
                spush(result, '/');
            } else if (prev_was_dotdot) {
                sppop(result);
                spush(result, '/');
            }
            prev_was_dot = false;
            prev_was_dotdot = false;
        } else if (c == '.') {
            if (prev_was_dot) {
                prev_was_dotdot = true;
            }
            prev_was_dot = true;
        } else {
            spush(result, c);
            prev_was_dot = false;
            prev_was_dotdot = false;
        }
    }
    sreplace(result, "//", "/");
    if (slen(result) > 1 && slast(result) == '/') spop(result);
    return result;
}

//--------------------------------------------------------------------------------------------------

METAENGINE_Result metadot_fs_set_write_directory(const char* platform_dependent_directory) {
    if (!PHYSFS_setWriteDir(platform_dependent_directory)) {
        return metadot_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    } else {
        return metadot_result_success();
    }
}

METAENGINE_Result metadot_fs_mount(const char* archive, const char* mount_point, bool append_to_path) {
    if (!PHYSFS_mount(archive, mount_point, (int)append_to_path)) {
        return metadot_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    } else {
        return metadot_result_success();
    }
}

METAENGINE_Result metadot_fs_dismount(const char* archive) {
    if (!PHYSFS_unmount(archive)) {
        return metadot_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    } else {
        return metadot_result_success();
    }
}

static METADOT_INLINE METAENGINE_FileType s_file_type(PHYSFS_FileType type) {
    switch (type) {
        case PHYSFS_FILETYPE_REGULAR:
            return METAENGINE_FILE_TYPE_REGULAR;
        case PHYSFS_FILETYPE_DIRECTORY:
            return METAENGINE_FILE_TYPE_DIRECTORY;
        case PHYSFS_FILETYPE_SYMLINK:
            return METAENGINE_FILE_TYPE_SYMLINK;
        default:
            return METAENGINE_FILE_TYPE_OTHER;
    }
}

METAENGINE_Result metadot_fs_stat(const char* virtual_path, METAENGINE_Stat* stat) {
    PHYSFS_Stat physfs_stat;
    if (!PHYSFS_stat(virtual_path, &physfs_stat)) {
        return metadot_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    } else {
        stat->type = s_file_type(physfs_stat.filetype);
        stat->is_read_only = physfs_stat.readonly;
        stat->size = physfs_stat.filesize;
        stat->last_modified_time = physfs_stat.modtime;
        stat->created_time = physfs_stat.createtime;
        stat->last_accessed_time = physfs_stat.accesstime;
        return metadot_result_success();
    }
}

METAENGINE_File* metadot_fs_create_file(const char* virtual_path) {
    PHYSFS_file* file = PHYSFS_openWrite(virtual_path);
    if (!file) {
        return NULL;
    } else if (!PHYSFS_setBuffer(file, METAENGINE_FILE_SYSTEM_BUFFERED_IO_SIZE)) {
        PHYSFS_close(file);
        return NULL;
    }
    return (METAENGINE_File*)file;
}

METAENGINE_File* metadot_fs_open_file_for_write(const char* virtual_path) {
    PHYSFS_file* file = PHYSFS_openWrite(virtual_path);
    if (!file) {
        return NULL;
    } else if (!PHYSFS_setBuffer(file, METAENGINE_FILE_SYSTEM_BUFFERED_IO_SIZE)) {
        PHYSFS_close(file);
        return NULL;
    }
    return (METAENGINE_File*)file;
}

METAENGINE_File* metadot_fs_open_file_for_append(const char* virtual_path) {
    PHYSFS_file* file = PHYSFS_openAppend(virtual_path);
    if (!file) {
        return NULL;
    } else if (!PHYSFS_setBuffer(file, METAENGINE_FILE_SYSTEM_BUFFERED_IO_SIZE)) {
        PHYSFS_close(file);
        return NULL;
    }
    return (METAENGINE_File*)file;
}

METAENGINE_File* metadot_fs_open_file_for_read(const char* virtual_path) {
    PHYSFS_file* file = PHYSFS_openRead(virtual_path);
    if (!file) {
        return NULL;
    } else if (!PHYSFS_setBuffer(file, METAENGINE_FILE_SYSTEM_BUFFERED_IO_SIZE)) {
        PHYSFS_close(file);
        return NULL;
    }
    return (METAENGINE_File*)file;
}

METAENGINE_Result metadot_fs_close(METAENGINE_File* file) {
    if (!PHYSFS_close((PHYSFS_file*)file)) {
        return metadot_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    } else {
        return metadot_result_success();
    }
}

METAENGINE_Result metadot_fs_delete(const char* virtual_path) {
    if (!PHYSFS_delete(virtual_path)) {
        return metadot_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    } else {
        return metadot_result_success();
    }
}

METAENGINE_Result metadot_fs_create_directory(const char* virtual_path) {
    if (!PHYSFS_mkdir(virtual_path)) {
        return metadot_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    } else {
        return metadot_result_success();
    }
}

const char** metadot_fs_enumerate_directory(const char* virtual_path) {
    const char** file_list = (const char**)PHYSFS_enumerateFiles(virtual_path);
    if (!file_list) {
        return NULL;
    }
    return file_list;
}

void metadot_fs_free_enumerated_directory(const char** directory_list) { PHYSFS_freeList(directory_list); }

const char* metadot_fs_get_backend_specific_error_message() { return PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()); }

const char* metadot_fs_get_user_directory(const char* company_name, const char* game_name) { return PHYSFS_getPrefDir(company_name, game_name); }

const char* metadot_fs_get_actual_path(const char* virtual_path) { return PHYSFS_getRealDir(virtual_path); }

bool metadot_fs_file_exists(const char* virtual_path) { return PHYSFS_exists(virtual_path) ? true : false; }

size_t metadot_fs_read(METAENGINE_File* file, void* buffer, size_t bytes) { return (size_t)PHYSFS_readBytes((PHYSFS_file*)file, buffer, (PHYSFS_uint64)bytes); }

size_t metadot_fs_write(METAENGINE_File* file, const void* buffer, size_t bytes) { return (size_t)PHYSFS_writeBytes((PHYSFS_file*)file, buffer, (PHYSFS_uint64)bytes); }

METAENGINE_Result metadot_fs_eof(METAENGINE_File* file) {
    if (!PHYSFS_eof((PHYSFS_file*)file)) {
        return metadot_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    } else {
        return metadot_result_success();
    }
}

size_t metadot_fs_tell(METAENGINE_File* file) { return (size_t)PHYSFS_tell((PHYSFS_file*)file); }

METAENGINE_Result metadot_fs_seek(METAENGINE_File* file, size_t position) {
    if (!PHYSFS_seek((PHYSFS_file*)file, (PHYSFS_uint64)position)) {
        return metadot_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    } else {
        return metadot_result_success();
    }
}

size_t metadot_fs_size(METAENGINE_File* file) { return (size_t)PHYSFS_fileLength((PHYSFS_file*)file); }

void* metadot_fs_read_entire_file_to_memory(const char* virtual_path, size_t* size) {
    METAENGINE_File* file = metadot_fs_open_file_for_read(virtual_path);
    if (!file) {
        METADOT_BUG("Can't load file %s", virtual_path);
        return NULL;
    }
    size_t sz = metadot_fs_size(file);
    void* data = METAENGINE_FW_ALLOC(sz);
    size_t sz_read = metadot_fs_read(file, data, sz);
    METADOT_ASSERT_E(sz == sz_read);
    if (size) *size = sz_read;
    metadot_fs_close(file);
    return data;
}

char* metadot_fs_read_entire_file_to_memory_and_nul_terminate(const char* virtual_path, size_t* size) {
    METAENGINE_File* file = metadot_fs_open_file_for_read(virtual_path);
    void* data = NULL;
    if (!file) return NULL;
    size_t sz = metadot_fs_size(file) + 1;
    data = METAENGINE_FW_ALLOC(sz);
    size_t sz_read = metadot_fs_read(file, data, sz);
    METADOT_ASSERT_E(sz == sz_read + 1);
    ((char*)data)[sz - 1] = 0;
    if (size) *size = sz;
    metadot_fs_close(file);
    return (char*)data;
}

METAENGINE_Result metadot_fs_write_entire_buffer_to_file(const char* virtual_path, const void* data, size_t size) {
    METAENGINE_File* file = metadot_fs_open_file_for_write(virtual_path);
    if (!file) return metadot_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    uint64_t sz = metadot_fs_write(file, data, (PHYSFS_uint64)size);
    if (sz != size) {
        return metadot_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    }
    metadot_fs_close(file);
    return metadot_result_success();
}

METAENGINE_Result metadot_fs_init(const char* argv0) {
    if (!PHYSFS_init(argv0)) {
        return metadot_result_error(PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
    } else {
        return metadot_result_success();
    }
}

void metadot_fs_destroy() { PHYSFS_deinit(); }
