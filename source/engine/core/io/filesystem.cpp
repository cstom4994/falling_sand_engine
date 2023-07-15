// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "filesystem.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>

#include "datapackage.h"
#include "engine/core/base_memory.h"
#include "engine/core/core.hpp"
#include "engine/core/platform.h"
#include "engine/engine.h"
#include "engine/utils/utils.hpp"

bool InitFilesystem() {

    auto currentDir = std::filesystem::path(std::filesystem::current_path());

    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    ENGINE()->exepath = std::filesystem::path(buffer).parent_path().string();

    for (int i = 0; i < 3; ++i) {
        if (std::filesystem::exists(currentDir / "Data")) {
            ENGINE()->gamepath = normalizePath(currentDir.string()).c_str();
            METADOT_INFO(std::format("Game data path detected: {0} (Base: {1})", ENGINE()->gamepath, std::filesystem::current_path().string().c_str()).c_str());
            return METADOT_OK;
        }
        currentDir = currentDir.parent_path();
    }

    METADOT_ERROR("Game data path detect failed");
    return METADOT_FAILED;
}

std::string ME_fs_get_path(std::string path) {
    if (ENGINE()->gamepath.empty()) {
        ME_ASSERT(ENGINE()->gamepath.empty(), "gamepath not detected");
        return {path};
    } else {
        std::string get_path{ENGINE()->gamepath};
        get_path.append(path);
        return get_path;
    }
}

#define ME_FILE_SYSTEM_BUFFERED_IO_SIZE (2 * ME_MB)

std::vector<char> ME_fs_read_file_to_vec(const char *path, size_t &size) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    std::streamsize size_ = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size_);
    if (file.read(buffer.data(), size_)) {
        size = size_;
        return buffer;
    } else {
        METADOT_ERROR("Unable to load file ", path);
        return std::vector<char>();
    }
}

std::string normalizePath2(const std::string &messyPath) {
    std::filesystem::path path(messyPath);
    std::filesystem::path canonicalPath = std::filesystem::canonical(path);
    std::string npath = canonicalPath.make_preferred().string();
    return npath;
}

std::string normalizePath(const std::string &path, char delimiter) {
    static constexpr char delims[] = "/\\";

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
            if (*it != '.') throw std::logic_error("bad path");
            if (norm.empty() || norm.back() != delimiter) throw std::logic_error("bad path");

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

bool ME_fs_directory_exists(const std::filesystem::path &path, std::filesystem::file_status status) {
    if (std::filesystem::status_known(status) ? std::filesystem::exists(status) : std::filesystem::exists(path)) {
        return true;
    }

    return false;
}

void ME_fs_create_directory(const std::string &directory_name) { std::filesystem::create_directories(directory_name); }

char *ME_fs_readfilestring(const char *path) {
    char *source = NULL;
    FILE *fp = fopen(path, "r");
    if (fp != NULL) {
        if (fseek(fp, 0L, SEEK_END) == 0) {
            long bufsize = ftell(fp);
            if (bufsize == -1) {
            }

            source = (char *)ME_MALLOC(sizeof(char) * (bufsize + 1));

            if (fseek(fp, 0L, SEEK_SET) != 0) {
            }

            size_t newLen = fread(source, sizeof(char), bufsize, fp);
            if (ferror(fp) != 0) {
                METADOT_ERROR("Error reading file @ ", METADOT_RESLOC(path));
            } else {
                source[newLen++] = '\0';
            }
        }
        fclose(fp);
        return source;
    }
    ME_fs_freestring(source);
    return NULL;
}

void ME_fs_freestring(void *ptr) {
    if (NULL != ptr) ME_FREE(ptr);
}

std::string ME_fs_normalize_path(const std::string &messyPath) {
    std::filesystem::path path(messyPath);
    std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(path);
    std::string npath = canonicalPath.make_preferred().string();
    return npath;
}

std::string ME_fs_readfile(const std::string &filename) {
    std::ifstream ifs(filename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

    std::ifstream::pos_type fileSize = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    std::vector<char> bytes(fileSize);
    ifs.read(bytes.data(), fileSize);

    return std::string(bytes.data(), fileSize);
}