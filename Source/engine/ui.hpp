// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include <functional>
#include <map>

#include "code_reflection.hpp"
#include "core/core.h"
#include "engine/imgui_core.hpp"
#include "engine/imgui_impl.hpp"
#include "engine/renderer/renderer_opengl.h"
#include "game/game_resources.hpp"
#include "renderer/renderer_gpu.h"

typedef enum elementType { coloredRectangle, texturedRectangle, textElement, lineElement, buttonElement } ElementType;

typedef struct UIElementState_Button {
    U8 state;
    void (*func)(void);
} UI_Button;
typedef struct UIElementState_Window {
    U8 state;
} UI_Window;

typedef union UIElementClass {
    UI_Button button;
    UI_Window window;
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
    ImGuiCore* ImGuiCore = nullptr;
    std::map<std::string, UIElement> elementLists = {};
} UIData;

void UIRendererInit();
void UIRendererPostUpdate();
void UIRendererUpdate();
void UIRendererDraw();
void UIRendererFree();

void DrawPoint(Vector3 pos, float size, Texture* texture, U8 r, U8 g, U8 b);
void DrawLine(Vector3 min, Vector3 max, float thickness, U8 r, U8 g, U8 b);