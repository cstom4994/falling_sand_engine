// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_IMGUILAYER_HPP_
#define _METADOT_IMGUILAYER_HPP_

#include <regex>
#include <string>
#include <vector>

#include "audio/audio.h"
#include "core/debug.hpp"
#include "libs/imgui/text_editor.h"
#include "sdl_wrapper.h"
#include "ui/imgui/imgui_css.h"
#include "ui/imgui/imgui_impl.hpp"

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
    ImGuiCSS::Context *ctx;
    ImGuiCSS::Document *document;
    ImGuiID dockspace_id;

private:
    static void (*RendererShutdownFunction)();
    static void (*PlatformShutdownFunction)();

    static void (*RendererNewFrameFunction)();
    static void (*PlatformNewFrameFunction)();

    static void (*RenderFunction)(ImDrawData *);

public:
    ImGuiCore();
    void Init();
    void End();
    void NewFrame();
    void Draw();
    void Update();
    const ImVec2 GetNextWindowsPos(ImGuiWindowTags tag, ImVec2 pos);
    const ImGuiID GetMainDockID() { return dockspace_id; }
    const

            ImGuiContext *
            getImGuiCtx() {
        METADOT_ASSERT(m_imgui, "Miss ImGuiContext");
        return m_imgui;
    }

    ImGuiCSS::Context *getImGuiCSSCtx() {
        METADOT_ASSERT(ctx, "Miss ImGuiContext");
        return ctx;
    }
};

#endif
