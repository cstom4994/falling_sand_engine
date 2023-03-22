// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "filesystem.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>

#include "core/alloc.hpp"
#include "core/core.h"
#include "core/cpp/utils.hpp"
#include "core/platform.h"
#include "core/stl/stl.h"
#include "datapackage.h"
#include "engine/engine.h"
#include "core/platform.h"

IMPLENGINE();

bool InitFilesystem() {

    auto currentDir = std::filesystem::path(std::filesystem::current_path());

#if 1

    for (int i = 0; i < 3; ++i) {
        if (std::filesystem::exists(currentDir / "Data")) {
            Core.gamepath = metadot_path_normalize(currentDir.string().c_str());
            // s_DataPath = currentDir / "Data";
            METADOT_INFO("Game data path detected: %s (Base: %s)", Core.gamepath.c_str(), std::filesystem::current_path().string().c_str());

            // if (metadot_is_error(err)) {
            //     METADOT_ASSERT_E(0);
            // } else if (true) {
            //     // Put the base directory (the path to the exe) onto the file system search path.
            //     // metadot_fs_mount(Core.gamepath.c_str(), "", true);
            // }

            return METADOT_OK;
        }
        currentDir = currentDir.parent_path();
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
            source = (char*)malloc(sizeof(char) * (bufsize + 1));

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
    free(source); /* Don't forget to call free() later! */
    return R_null;
}

void metadot_fs_freestring(void* ptr) {
    if (NULL != ptr) free(ptr);
}

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

std::vector<char> fs_read_entire_file_to_memory(const char* path, size_t &size) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    std::streamsize size_ = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size_);
    if (file.read(buffer.data(), size_)) {
        size = size_;
        return buffer;
    } else {
        METADOT_ERROR("Unable to load file %s", path);
        return std::vector<char>();
    }
}

std::string normalizePath(const std::string& messyPath) {
    std::filesystem::path path(messyPath);
    std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(path);
    std::string npath = canonicalPath.make_preferred().string();
    return npath;
}