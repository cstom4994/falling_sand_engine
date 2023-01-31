// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include <functional>
#include <map>

#include "meta/meta.hpp"
#include "core/core.h"
#include "imgui/imgui_core.hpp"
#include "imgui/imgui_impl.hpp"
#include "renderer/renderer_opengl.h"
#include "ui_layout.h"
#include "game_resources.hpp"
#include "libs/parallel_hashmap/btree.h"
#include "renderer/renderer_gpu.h"

typedef enum elementType { coloredRectangle, texturedRectangle, textElement, lineElement, buttonElement, progressBarElement, windowElement } ElementType;

typedef struct UIElementState_Button {
    U8 state;
    void (*func)(void);
} UI_Button;
typedef struct UIElementState_Window {
    U8 state;
    layout_id layout_id;
} UI_Window;
typedef struct UIElementState_ProgressBar {
    U8 state;
    U8 bar_type;
    F32 bar_current;
    F32 bar_limit;
    METAENGINE_Color bar_color;
    METAENGINE_Color bar_text_color;
} UI_ProgressBar;

typedef union UIElementClass {
    UI_Button button;
    UI_Window window;
    UI_ProgressBar progressbar;
} UIElementClass;

typedef struct UIElement {
    ElementType type;

    int minRectX;
    int minRectY;

    int maxRectX;
    int maxRectY;

    METAENGINE_Color color;
    Texture* texture;
    std::string text;

    UIElementClass cclass;

    int textW, textH;

    float lineThickness;

} UIElement;

typedef struct UIData {
    ImGuiCore* imguiCore = nullptr;

    // Layout caculate context
    layout_context layoutContext;

    // std::map<std::string, UIElement> elementLists = {};
    phmap::btree_map<std::string, UIElement> elementLists = {};
} UIData;

void UIRendererInit();
void UIRendererPostUpdate();
void UIRendererUpdate();
void UIRendererDraw();
void UIRendererDrawImGui();
void UIRendererFree();

bool UIIsMouseOnControls();

void DrawPoint(metadot_vec3 pos, float size, Texture* texture, U8 r, U8 g, U8 b);
void DrawLine(metadot_vec3 min, metadot_vec3 max, float thickness, U8 r, U8 g, U8 b);