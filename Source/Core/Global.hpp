// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_GLOBAL_HPP_
#define _METADOT_GLOBAL_HPP_

#include "Core/Core.hpp"
#include "Engine/Audio.h"
#include "Engine/CodeReflection.hpp"
#include "Engine/ImGuiImplement.hpp"
#include "Engine/Platform.hpp"
#include "Engine/Shaders.hpp"
#include "Game/FileSystem.hpp"
#include "Game/GameResources.hpp"

#include <map>

class Game;
class Client;
class Server;
class Scripts;
class ImGuiCore;

#define RegisterFunctions(name, func)                                                              \
    MetaEngine::any_function func_log_info{&func};                                                 \
    global->HostData->Functions.insert(std::make_pair(#name, name))

struct Global
{
    Game *game = nullptr;
    Scripts *scripts = nullptr;

    ImGuiCore *ImGuiCore = nullptr;

    ShaderWorker shaderworker;
    Audio audioEngine;
    GameDir GameDir;
    Platform platform;
    I18N I18N;

    Client *client = nullptr;
    Server *server = nullptr;

    struct
    {
        ImGuiContext *imgui_context = nullptr;
        void *wndh = nullptr;

        std::map<std::string, Meta::any_function> Functions;

        // CppSource Functions register
        void (*draw)(void);
    } HostData;

    Global() {}
};

extern Global global;

#endif