// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "filesystem.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>

#include "engine/core/core.hpp"
#include "engine/core/cpp/utils.hpp"
#include "engine/core/memory.h"
#include "engine/core/platform.h"
#include "datapackage.h"
#include "engine/engine.h"

IMPLENGINE();

bool InitFilesystem() {

    auto currentDir = std::filesystem::path(std::filesystem::current_path());

#if 1

    for (int i = 0; i < 3; ++i) {
        if (std::filesystem::exists(currentDir / "Data")) {
            Core.gamepath = normalizePath(currentDir.string()).c_str();
            METADOT_INFO(std::format("Game data path detected: {0} (Base: {1})", Core.gamepath.c_str(), std::filesystem::current_path().string().c_str()).c_str());

            // if (metadot_is_error(err)) {
            //     ME_ASSERT_E(0);
            // } else if (true) {
            //     // Put the base directory (the path to the exe) onto the file system search path.
            //     // ME_fs_mount(Core.gamepath.c_str(), "", true);
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

#define ME_FILE_SYSTEM_BUFFERED_IO_SIZE (2 * ME_MB)

std::vector<char> fs_read_entire_file_to_memory(const char* path, size_t& size) {
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

std::string normalizePath2(const std::string& messyPath) {
    std::filesystem::path path(messyPath);
    std::filesystem::path canonicalPath = std::filesystem::canonical(path);
    std::string npath = canonicalPath.make_preferred().string();
    return npath;
}

std::string normalizePath(const std::string& path, char delimiter) {
    static constexpr char delims[] = "/\\";
    static const auto error = std::logic_error("bad path");

    std::string norm;
    norm.reserve(path.size() / 2);  // random guess, should be benchmarked

    for (auto it = path.begin(); it != path.end(); it++) {
        if (std::any_of(std::begin(delims), std::end(delims), [it](auto c) { return c == *it; })) {
            if (norm.empty() || norm.back() != delimiter) norm.push_back(delimiter);
        } else if (*it == '.') {
            if (++it == path.end()) break;
            if (std::any_of(std::begin(delims), std::end(delims), [it](auto c) { return c == *it; })) {
                continue;
            }
            if (*it != '.') throw error;
            if (norm.empty() || norm.back() != delimiter) throw error;

            norm.pop_back();
            while (!norm.empty()) {
                norm.pop_back();
                if (norm.back() == delimiter) {
                    norm.pop_back();
                    break;
                }
            }
        } else
            norm.push_back(*it);
    }
    if (!norm.empty() && norm.back() != delimiter) norm.push_back(delimiter);
    return norm;
}