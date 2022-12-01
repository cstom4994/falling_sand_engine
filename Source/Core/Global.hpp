// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_GLOBAL_HPP_
#define _METADOT_GLOBAL_HPP_

#include "Engine/AudioEngine.h"
#include "Engine/ImGuiBase.hpp"
#include "Engine/Platform.hpp"
#include "Engine/Refl.hpp"
#include "Game/FileSystem.hpp"
#include "Game/Shaders.hpp"

#include <map>

class Game;
class Client;
class Server;
class Scripts;
class ImGuiLayer;

#define RegisterFunctions(name, func)                                                              \
    MetaEngine::any_function func_log_info{&func};                                                 \
    global->HostData->Functions.insert(std::make_pair(#name, name))

struct Global
{
    Game *game = nullptr;
    Scripts *scripts = nullptr;

    ImGuiLayer *ImGuiLayer = nullptr;

    ShaderWorker shaderworker;
    CAudioEngine audioEngine;
    GameDir GameDir;
    Platform platform;

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

    void tick();
};

extern Global global;

#endif