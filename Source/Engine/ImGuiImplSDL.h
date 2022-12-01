
#ifndef _METADOT_IMGUIIMPLSDL_HPP_
#define _METADOT_IMGUIIMPLSDL_HPP_

#include "imgui.h"// IMGUI_IMPL_API

struct SDL_Window;
struct SDL_Renderer;
typedef union SDL_Event SDL_Event;

bool ImGui_ImplSDL2_InitForOpenGL(SDL_Window *window, void *sdl_gl_context);
bool ImGui_ImplSDL2_InitForVulkan(SDL_Window *window);
bool ImGui_ImplSDL2_InitForD3D(SDL_Window *window);
bool ImGui_ImplSDL2_InitForMetal(SDL_Window *window);
bool ImGui_ImplSDL2_InitForSDLRenderer(SDL_Window *window, SDL_Renderer *renderer);
void ImGui_ImplSDL2_Shutdown();
void ImGui_ImplSDL2_NewFrame();
bool ImGui_ImplSDL2_ProcessEvent(const SDL_Event *event);

#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
static inline void ImGui_ImplSDL2_NewFrame(SDL_Window *) {
    ImGui_ImplSDL2_NewFrame();
}// 1.84: removed unnecessary parameter
#endif

#endif