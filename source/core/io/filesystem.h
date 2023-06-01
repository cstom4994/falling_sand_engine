// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_FILESYSTEM_HPP
#define ME_FILESYSTEM_HPP

#include <cstring>
#include <filesystem>
#include <string_view>

#include "core/core.hpp"
#include "core/macros.hpp"
#include "core/platform.h"

// #define FUTIL_ASSERT_EXIST(stringPath)                                                             \
//     ME_ASSERT(FUtil_exists(METADOT_RESLOC(stringPath)),                                       \
//                    ("%s", std::format("FILE: {0} does not exist", stringPath)))

#define FUTIL_ASSERT_EXIST(stringPath)

bool InitFilesystem();

#define METADOT_RESLOC(x) x

std::vector<char> fs_read_entire_file_to_memory(const char* path, size_t& size);
std::string normalizePath(const std::string& path, char delimiter = '/');

#endif