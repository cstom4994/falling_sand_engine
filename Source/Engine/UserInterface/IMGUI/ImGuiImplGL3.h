
#ifndef _METADOT_IMGUIIMPLGL_HPP_
#define _METADOT_IMGUIIMPLGL_HPP_

#include "imgui.h"// IMGUI_IMPL_API

// Backend API
bool ImGui_ImplOpenGL3_Init(const char *glsl_version = nullptr);
void ImGui_ImplOpenGL3_Shutdown();
void ImGui_ImplOpenGL3_NewFrame();
void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData *draw_data);

// (Optional) Called by Init/NewFrame/Shutdown
bool ImGui_ImplOpenGL3_CreateFontsTexture();
void ImGui_ImplOpenGL3_DestroyFontsTexture();
bool ImGui_ImplOpenGL3_CreateDeviceObjects();
void ImGui_ImplOpenGL3_DestroyDeviceObjects();

#endif
