// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_IMGUILAYER_HPP_
#define _METADOT_IMGUILAYER_HPP_

#include "Core/DebugImpl.hpp"
#include "Engine/Audio.hpp"
#include "Engine/ImGuiImplement.hpp"
#include "Engine/SDLWrapper.h"
#include "Libs/ImGui/TextEditor.h"
#include <regex>
#include <string>
#include <vector>

class Material;
class WorldMeta;

enum ImGuiWindowTags {

    UI_None = 0,
    UI_MainMenu = 1 << 0,
    UI_GCManager = 1 << 1,
};

enum EditorTags {
    Editor_Code = 0,
    Editor_Markdown = 1
};

class ImGuiCore {
private:
    struct ImGuiWin
    {
        std::string name;
        bool *opened;
    };

    struct EditorView
    {
        EditorTags tags;

        std::string file;
        std::string content;
        bool is_edited = false;

        bool operator==(EditorView v) { return (v.file == this->file) && (v.tags == this->tags); }
    };

    std::vector<ImGuiWin> m_wins;

    C_Window *window;
    void *gl_context;

    ImGuiContext *m_imgui = nullptr;

    std::vector<EditorView> view_contents;
    TextEditor editor;
    EditorView *view_editing = nullptr;
    ImGuiWidget::FileBrowser fileDialog;

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
