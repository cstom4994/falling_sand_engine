// Copyright(c) 2022-2023, KaoruXun

// This source file may include
// https://github.com/ocornut/imgui (MIT) by Omar Cornut

#ifndef ME_IMGUI_IMPL_HPP
#define ME_IMGUI_IMPL_HPP

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <future>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "imgui_utils.hpp"

#if defined(_WIN32)
#define ME_IMM32
#else
#include <sys/stat.h>
#endif

#include "engine/core/core.hpp"

// Backend API
bool ImGui_ImplOpenGL3_Init();
void ImGui_ImplOpenGL3_Shutdown();
void ImGui_ImplOpenGL3_NewFrame();
void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData *draw_data);

// (Optional) Called by Init/NewFrame/Shutdown
bool ImGui_ImplOpenGL3_CreateFontsTexture();
void ImGui_ImplOpenGL3_DestroyFontsTexture();
bool ImGui_ImplOpenGL3_CreateDeviceObjects();
void ImGui_ImplOpenGL3_DestroyDeviceObjects();

struct SDL_Window;
struct SDL_Renderer;
typedef union SDL_Event SDL_Event;

bool ImGui_ImplSDL2_Init(SDL_Window *window, void *sdl_gl_context);
void ImGui_ImplSDL2_Shutdown();
void ImGui_ImplSDL2_NewFrame();
bool ImGui_ImplSDL2_ProcessEvent(const SDL_Event *event);

#endif
