
#include <SDL.h>

#include <imgui.h>
#include <imgui_internal.h>

#include "Engine/Refl.hpp"

#include <map>


#define RegisterFunctions(name, func) \
    MetaEngine::any_function func_log_info{ &func };\
    this->data.Functions.insert(std::make_pair(#name, name));


struct HostData {
    ImGuiContext* imgui_context = nullptr;
    void* wndh = nullptr;

    // glfw functions that imgui calls on guest side
    SDL_Window* window = nullptr;


    std::map<std::string, MetaEngine::any_function> Functions;

    // CppSource Functions register
    void(*draw)(void);
};