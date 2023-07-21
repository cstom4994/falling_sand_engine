// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_UI_HPP
#define ME_UI_HPP

#include <functional>
#include <map>

#include "engine/core/core.hpp"
#include "engine/game_basic.hpp"
#include "engine/meta/reflection.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/renderer/renderer_opengl.h"
#include "engine/textures.hpp"
#include "engine/ui/imgui_impl.hpp"
#include "engine/ui/dbgui.hpp"
#include "game/player.hpp"

namespace ME {

typedef enum elementType { coloredRectangle, texturedRectangle, textElement, lineElement, buttonElement, progressBarElement, windowElement, listBoxElement, inputBoxElement } ElementType;

typedef union UIElementClass {
    struct UIElementState_Button {
        MEcolor hot_color;
        void (*func)(void);
    } button;

    struct UIElementState_Window {
    } window;

    struct UIElementState_ProgressBar {
        u8 bar_type;
        f32 bar_current;
        f32 bar_limit;
        MEcolor bar_color;
        MEcolor bar_text_color;
    } progressbar;

    struct UIElementState_ListBox {
        u8 list_type;
        char** list;
        MEcolor list_bg_color;
        MEcolor list_hover_color;
        MEcolor list_text_color;
    } listbox;

    struct UIElementState_InputBox {
        MEcolor bg_color;
        MEcolor text_color;
    } inputbox;

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

struct UIElement {
    ElementType type;

    bool visible = true;

    ref<UIElement> parent;

    UIMovable movable;
    UIResizable resizable;

    int x, y, w, h;

    u8 state;
    MEcolor color;
    TextureRef texture;
    std::string text;

    UIElementClass cclass;

    int textW, textH;

    float lineThickness;
};

// static_assert(sizeof(UIElement) == 176);

typedef struct UIData {
    ref<ImGuiLayer> imgui;

    // std::map<std::string, UIElement> elementLists = {};
    std::map<std::string, ref<UIElement>> elementLists = {};

    UIElement* oninput = nullptr;
    UIElement* onmoving = nullptr;
} UIData;

class UISystem : public IGameSystem {
public:
    REGISTER_SYSTEM(UISystem)

    UIData* uidata = nullptr;

    void create() override;
    void destory() override;
    void reload() override;
    void registerLua(lua_wrapper::State& s_lua) override;

    void UIRendererInit();
    void UIRendererPostUpdate();
    void UIRendererUpdate();
    void UIRendererDraw();
    void UIRendererDrawImGui();
    void UIRendererFree();
    bool UIRendererInput(C_KeyboardEvent event);

    bool UIIsMouseOnControls();

    void DrawPoint(MEvec3 pos, float size, Texture* texture, u8 r, u8 g, u8 b);
    void DrawLine(MEvec3 min, MEvec3 max, float thickness, u8 r, u8 g, u8 b);
};

}  // namespace ME

#endif