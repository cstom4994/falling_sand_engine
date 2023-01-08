// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_IMGUILAYER_HPP_
#define _METADOT_IMGUILAYER_HPP_

#include <regex>
#include <string>
#include <vector>

#include "core/debug_impl.hpp"
#include "engine/audio.hpp"
#include "engine/imgui_impl.hpp"
#include "engine/sdl_wrapper.h"
#include "libs/imgui/text_editor.h"

class Material;
class WorldMeta;

enum ImGuiWindowTags {

    UI_None = 0,
    UI_MainMenu = 1 << 0,
    UI_GCManager = 1 << 1,
};

enum EditorTags { Editor_Code = 0, Editor_Markdown = 1 };

class ImGuiCore {
private:
    struct EditorView {
        EditorTags tags;

        std::string file;
        std::string content;
        bool is_edited = false;

        bool operator==(EditorView v) { return (v.file == this->file) && (v.tags == this->tags); }
    };

    ImGuiContext *m_imgui = nullptr;

    std::vector<EditorView> view_contents;
    TextEditor editor;
    EditorView *view_editing = nullptr;
    ImGuiWidget::FileBrowser fileDialog;

public:
    ImGuiCore();
    void Init();
    void onDetach();
    void NewFrame();
    void Draw();
    void Render();
    ImVec2 GetNextWindowsPos(ImGuiWindowTags tag, ImVec2 pos);

    ImGuiContext *getImGuiCtx() {
        METADOT_ASSERT(m_imgui, "Miss ImGuiContext");
        return m_imgui;
    }
};

#endif
