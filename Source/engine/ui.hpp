// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include <map>

#include "core/core.h"
#include "engine/imgui_core.hpp"
#include "engine/imgui_impl.hpp"
#include "engine/renderer/renderer_opengl.h"
#include "game/game_resources.hpp"
#include "renderer/renderer_gpu.h"

typedef enum elementType { coloredRectangle, texturedRectangle, textElement, lineElement } ElementType;
typedef struct UIElement {
    ElementType type;

    int minRectX;
    int minRectY;

    int maxRectX;
    int maxRectY;

    METAENGINE_Color color;
    Texture* texture;
    std::string text;

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