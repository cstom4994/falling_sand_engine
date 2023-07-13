// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_FILESYSTEM_HPP
#define ME_FILESYSTEM_HPP

#include <cstring>
#include <filesystem>
#include <fstream>
#include <string_view>

#include "engine/core/core.hpp"
#include "engine/core/macros.hpp"
#include "engine/core/platform.h"

// #define FUTIL_ASSERT_EXIST(stringPath)                                                             \
//     ME_ASSERT(FUtil_exists(METADOT_RESLOC(stringPath)),                                       \
//                    ("%s", std::format("FILE: {0} does not exist", stringPath)))

#define FUTIL_ASSERT_EXIST(stringPath)

bool InitFilesystem();
std::string ME_fs_get_path(std::string path);

#define METADOT_RESLOC(x) ME_fs_get_path(x).c_str()

std::vector<char> ME_fs_read_file_to_vec(const char* path, size_t& size);
std::string normalizePath(const std::string& path, char delimiter = '/');

ME_INLINE bool ME_fs_exists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

ME_INLINE const char* ME_fs_get_filename(const char* path) {
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

char* ME_fs_readfilestring(const char* path);
void ME_fs_freestring(void* ptr);
std::string ME_fs_normalize_path(const std::string& messyPath);
bool ME_fs_directory_exists(const std::filesystem::path& path, std::filesystem::file_status status = std::filesystem::file_status{});
void ME_fs_create_directory(const std::string& directory_name);
std::string ME_fs_readfile(const std::string& filename);

#endif