// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_UI_HPP_
#define _METADOT_UI_HPP_

#include <functional>
#include <map>

#include "core/core.h"
#include "game_basic.hpp"
#include "game_resources.hpp"
#include "libs/parallel_hashmap/btree.h"
#include "meta/meta.hpp"
#include "renderer/renderer_gpu.h"
#include "renderer/renderer_opengl.h"
#include "ui/imgui/imgui_impl.hpp"
#include "ui/imgui/imgui_layer.hpp"

typedef enum elementType { coloredRectangle, texturedRectangle, textElement, lineElement, buttonElement, progressBarElement, windowElement, listBoxElement } ElementType;

typedef union UIElementClass {
    struct UIElementState_Button {
        METAENGINE_Color hot_color;
        void (*func)(void);
    } button;

    struct UIElementState_Window {
    } window;

    struct UIElementState_ProgressBar {
        U8 bar_type;
        F32 bar_current;
        F32 bar_limit;
        METAENGINE_Color bar_color;
        METAENGINE_Color bar_text_color;
    } progressbar;

    struct UIElementState_ListBox {
        U8 list_type;
        char** list;
        METAENGINE_Color list_bg_color;
        METAENGINE_Color list_hover_color;
        METAENGINE_Color list_text_color;
    } listbox;

} UIElementClass;

typedef struct UIMovable {
    bool moving = false;
    int ox = 0, oy = 0;
    int mx = 0, my = 0;
} UIMovable;

typedef struct UIResizable {
    bool resizable = false;
    bool resizing = false;
    int mx = 0, my = 0;
    int ow = 0, oh = 0;
    int mw = 0, mh = 0;
} UIResizable;

typedef struct UIElement {
    ElementType type;

    bool visible = true;

    MetaEngine::Ref<UIElement> parent;

    UIMovable movable;
    UIResizable resizable;

    int x, y, w, h;

    U8 state;
    METAENGINE_Color color;
    Texture* texture;
    std::string text;

    UIElementClass cclass;

    int textW, textH;

    float lineThickness;

} UIElement;

// static_assert(sizeof(UIElement) == 176);

typedef struct UIData {
    ImGuiLayer* imgui = nullptr;

    // std::map<std::string, UIElement> elementLists = {};
    phmap::btree_map<std::string, MetaEngine::Ref<UIElement>> elementLists = {};
} UIData;

class UISystem : public IGameSystem {
public:
    REGISTER_SYSTEM(UISystem)

    UIData* uidata = nullptr;

    void Create() override;
    void Destory() override;
    void Reload() override;
    void RegisterLua(LuaWrapper::State& s_lua) override;

    void UIRendererInit();
    void UIRendererPostUpdate();
    void UIRendererUpdate();
    void UIRendererDraw();
    void UIRendererDrawImGui();
    void UIRendererFree();

    bool UIIsMouseOnControls();

    void DrawPoint(vec3 pos, float size, Texture* texture, U8 r, U8 g, U8 b);
    void DrawLine(vec3 min, vec3 max, float thickness, U8 r, U8 g, U8 b);
};

#endif