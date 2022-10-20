// Copyright(c) 2022, KaoruXun All rights reserved.


#include "Core.hpp"

#include "Engine/InEngine.h"

namespace MetaEngine {

}

// STL
#include <cmath>
#include <limits>
#include <unordered_map>

#include "Engine/FileSystem.hpp"
#include "Engine/Utils.hpp"

namespace MetaEngine {
    std::string ResourceMan::s_resPath;
    std::string ResourceMan::s_resPathFolder;

    void ResourceMan::init() {
        auto currentDir = std::filesystem::path(FUtil::getExecutableFolderPath());
        for (int i = 0; i < 3; ++i) {
            currentDir = currentDir.parent_path();
            if (std::filesystem::exists(currentDir.string() + "/data")) {
                s_resPath = currentDir.string() + "/";
                s_resPathFolder = s_resPath + "data";
                METADOT_TRACE("Res folder found: {}data", s_resPath.c_str());
                return;
            }
        }
        METADOT_ERROR("Res folder not found");
    }

    const std::string &ResourceMan::getResPath() {
        return s_resPathFolder;
    }

    std::string ResourceMan::getResourceLoc(std::string_view resPath) {
        if (s_resPath.empty())
            std::cout << "try to load resource when ResourceMan is unloaded (" << resPath << ")" << std::endl;
        if (SUtil::startsWith(resPath, "data") || SUtil::startsWith(resPath, "/data"))
            return s_resPath + std::string(resPath);
        return std::string(resPath);
    }

    std::string ResourceMan::getLocalPath(std::string_view resPath) {
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
}// namespace MetaEngine