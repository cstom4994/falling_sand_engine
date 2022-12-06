// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_IMGUILAYER_HPP_
#define _METADOT_IMGUILAYER_HPP_

#include "Core/DebugImpl.hpp"
#include "Engine/Audio.h"
#include "Engine/ImGuiImplement.hpp"
#include "Engine/SDLWrapper.hpp"
#include "Libs/ImGui/TextEditor.h"

class Material;
class WorldMeta;

enum ImGuiWindowTags {

    UI_None = 0,
    UI_MainMenu = 1 << 0,
    UI_GCManager = 1 << 1,
};

class ImGuiCore {
private:
    struct ImGuiWin
    {
        std::string name;
        bool *opened;
    };

    std::vector<ImGuiWin> m_wins;

    C_Window *window;
    void *gl_context;

    ImGuiContext *m_imgui = nullptr;

    TextEditor editor;

public:
    ImGuiCore();
    ~ImGuiCore() = default;
    void Init(C_Window *window, void *gl_context);
    void onDetach();
    void begin();
    void end();
    void Render();
    void registerWindow(std::string_view windowName, bool *opened);
    ImVec2 GetNextWindowsPos(ImGuiWindowTags tag, ImVec2 pos);

    ImGuiContext *getImGuiCtx() {
        METADOT_ASSERT(m_imgui, "Miss ImGuiContext");
        return m_imgui;
    }
};

#endif