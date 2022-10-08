// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.


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
            std::cout << "try to load resource when ResourceMan is unloaded\n        ::" << resPath << std::endl;
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


    std::stringstream Log::s_stream;
    pprint::PrettyPrinter Log::printer;

    void Log::init() {
        static bool noInit = true;
        if (!noInit)
            return;
        noInit = false;

        //s_CoreLoggerSinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        //s_CoreLoggerSinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>("log.txt", 1048576 * 1, 1, true));
        //s_CoreLoggerSinks[1]->set_level(spdlog::level::trace);

        // pattern formatting https://github.com/gabime/spdlog/wiki/3.-Custom-formatting

        //s_CoreLoggerSinks[0]->set_pattern("[%-20s] %^[%T] %l: %v%$");
        //s_CoreLoggerSinks[1]->set_pattern("[%-20s] %^[%T] %l: %v%$");

        //s_CoreLogger = std::make_shared<spdlog::logger>("MetaDot", begin(s_CoreLoggerSinks), end(s_CoreLoggerSinks));
        //s_CoreLogger->set_level(spdlog::level::trace);
        //s_CoreLogger->flush_on(spdlog::level::trace);

        //register it if you need to access it globally
        //spdlog::register_logger(s_CoreLogger);

        METADOT_INFO("Logging Start");
    }

    void Log::flush() {
        //s_CoreLogger->flush();
    }
}// namespace MetaEngine


namespace MetaEngine {

    static std::unordered_map<std::string, Type *> types;

    Type::Type(const char *name, Type *parent)
        : name(name), parent(parent), id(0), inited(false) {
    }

    void Type::init() {
        static UInt32 nextId = 1;

        // Make sure we don't init twice, that would be bad
        if (inited)
            return;

        // Note: we add it here, not in the constructor, because some Types can get initialized before the map!
        types[name] = this;
        id = nextId++;
        bits[id] = true;
        inited = true;

        if (!parent)
            return;
        if (!parent->inited)
            parent->init();
        bits |= parent->bits;
    }

    UInt32 Type::getId() {
        if (!inited)
            init();
        return id;
    }

    const char *Type::getName() const {
        return name;
    }

    Type *Type::byName(const char *name) {
        auto pos = types.find(name);
        if (pos == types.end())
            return nullptr;
        return pos->second;
    }


}// namespace MetaEngine
