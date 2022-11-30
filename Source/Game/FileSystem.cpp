// Copyright(c) 2022, KaoruXun All rights reserved.

#include "FileSystem.hpp"
#include "Game/Utils.hpp"

#include "Engine/Platforms/Platform.hpp"


std::string Resource::s_ProjectRootPath;
std::string Resource::s_DataPath;

void Resource::init() {
    auto currentDir = std::filesystem::path(FUtil::getExecutableFolderPath());
    for (int i = 0; i < 3; ++i) {
        currentDir = currentDir.parent_path();
        if (std::filesystem::exists(currentDir.string() + "/data")) {
            s_ProjectRootPath = currentDir.string() + "/";
            s_DataPath = s_ProjectRootPath + "data";
            METADOT_INFO("Runtime folder detected: {0}", s_ProjectRootPath.c_str());
            return;
        }
    }
    METADOT_ERROR("Runtime folder detect failed");
}

const std::string &Resource::getDataPath() {
    return s_DataPath;
}

std::string Resource::getResourceLoc(std::string_view resPath) {
    if (s_ProjectRootPath.empty()) {
        std::cout << "Try to load resource when ResourceLoc is unloaded (" << resPath << ")" << std::endl;
    }
    if (SUtil::startsWith(resPath, "data") || SUtil::startsWith(resPath, "/data"))
        return s_ProjectRootPath + (s_ProjectRootPath.empty() ? "" : "/") + std::string(resPath);
    if (SUtil::startsWith(resPath, "saves") || SUtil::startsWith(resPath, "/saves"))
        return std::string(resPath);
    return std::string(resPath);
}

std::string Resource::getLocalPath(std::string_view resPath) {
    auto res = std::string(resPath);
    FUtil::cleanPathString(res);
    size_t offset = 0;
    size_t out = std::string::npos;
    while (true) {
        auto index = res.find("/data/", offset);
        if (index == std::string::npos)
            break;
        offset = index + 1;
        out = index;
    }
    if (out == std::string::npos)
        return "";
    return res.substr(out + 1);
}


std::string GameDir::getPath(std::string filePathRel) {
    return this->gameDir + filePathRel;
}

std::string GameDir::getWorldPath(std::string worldName) {
    return this->getPath("worlds/" + worldName);
}

void FUtil::cleanPathString(std::string &s) {
    SUtil::replaceWith(s, '\\', '/');
    SUtil::replaceWith(s, "//", "/", 2);
}

std::string FUtil::readFileString(std::string_view path) {
    auto p = METADOT_RESLOC(path);
    if (!exists(p))
        return "";
    const std::ifstream input_stream(p, std::ios_base::in);

    if (input_stream.fail()) {
        METADOT_ERROR("Failed to open file {}", p);
        return "";
    }

    std::stringstream buffer;
    buffer << input_stream.rdbuf();
    return buffer.str();
}

uint64_t FUtil::lastWriteTime(std::string_view path) {
    return std::filesystem::last_write_time(path).time_since_epoch().count();
}


std::string FUtil::getAbsolutePath(const char *fileName) {
    std::string out = getExecutableFolderPath() + fileName;
    return std::filesystem::exists(out) ? out : "";
}

const std::string &FUtil::getExecutableFolderPath() {
    static std::string out;
    if (out.empty()) {
        out = Platforms::GetExecutablePath().substr(0, Platforms::GetExecutablePath().find_last_of('/') + 1);
    }
    return out;
}

std::vector<std::string> FUtil::fileList(std::string_view folder_path, FileSearchFlags flags) {
    std::vector<std::string> out;
    std::filesystem::file_time_type bestTime;
    bool nope = flags & (FileSearchFlags_Oldest | FileSearchFlags_Newest);
    if (!FUtil::exists(folder_path))
        return out;

    if (!(flags & FileSearchFlags_Recursive)) {
        for (const auto &entry: std::filesystem::directory_iterator(folder_path)) {
#if defined(_MSC_VER)
            std::string s = SUtil::ws2s(std::wstring(entry.path().c_str()));
#else
            std::string s = SUtil::ws2s(std::wstring(entry.path().wstring()));
#endif
            if (entry.is_directory() && flags & FileSearchFlags_OnlyFiles) {
            } else if (entry.is_regular_file() && flags & FileSearchFlags_OnlyDirectories) {
            } else {
                if (nope) {
                    nope = false;
                    bestTime = last_write_time(entry.path());
                    out.push_back(s);
                }
                auto currentTime = last_write_time(entry.path());

                if (
                        ((flags & FileSearchFlags_Newest) && currentTime > bestTime) || ((flags & FileSearchFlags_Oldest) && currentTime < bestTime)) {
                    bestTime = currentTime;
                    out.clear();
                    out.push_back(s);
                } else if (!(flags & (FileSearchFlags_Oldest | FileSearchFlags_Newest)))
                    out.push_back(s);
            }
        }
    } else {
        for (const auto &entry: std::filesystem::recursive_directory_iterator(folder_path)) {
#if defined(_MSC_VER)
            std::string s = SUtil::ws2s(std::wstring(entry.path().c_str()));
#else
            std::string s = SUtil::ws2s(std::wstring(entry.path().wstring()));
#endif
            if (entry.is_directory() && flags & FileSearchFlags_OnlyFiles) {
            } else if (entry.is_regular_file() && flags & FileSearchFlags_OnlyDirectories) {
            } else {
                if (nope) {
                    nope = false;
                    bestTime = last_write_time(entry.path());
                    out.push_back(s);
                }
                auto currentTime = last_write_time(entry.path());

                if (
                        ((flags & FileSearchFlags_Newest) && currentTime > bestTime) || ((flags & FileSearchFlags_Oldest) && currentTime < bestTime)) {
                    bestTime = currentTime;
                    out.clear();
                    out.push_back(s);
                } else if (!(flags & (FileSearchFlags_Oldest | FileSearchFlags_Newest)))
                    out.push_back(s);
            }
        }
    }
    return out;
}