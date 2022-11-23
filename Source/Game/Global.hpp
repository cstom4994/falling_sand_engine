// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_GLOBAL_HPP_
#define _METADOT_GLOBAL_HPP_

#include "AudioEngine/AudioEngine.h"
#include "Engine/Meta/Refl.hpp"
#include "Engine/Platforms/Platform.hpp"
#include "Engine/UserInterface/IMGUI/ImGuiBase.hpp"
#include "Game/FileSystem.hpp"
#include "Game/Legacy/Shaders.hpp"

#include <map>

class Game;
class Client;
class Server;
class Scripts;

namespace MetaEngine {
    class ImGuiLayer;
}

#define RegisterFunctions(name, func)              \
    MetaEngine::any_function func_log_info{&func}; \
    global->HostData->Functions.insert(std::make_pair(#name, name))

struct Global
{
    Game *game = nullptr;
    Scripts *scripts = nullptr;

    MetaEngine::ImGuiLayer *ImGuiLayer = nullptr;

    ShaderWorker shaderworker;
    CAudioEngine audioEngine;
    MetaEngine::GameDir GameDir;
    Platform platform;

    Client *client = nullptr;
    Server *server = nullptr;

    struct
    {
        ImGuiContext *imgui_context = nullptr;
        void *wndh = nullptr;

        std::map<std::string, MetaEngine::any_function> Functions;

        // CppSource Functions register
        void (*draw)(void);
    } HostData;

    Global() {}

    void tick();
};

extern Global global;

#endif