// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "core/core.h"
#include "engine/imgui_core.hpp"
#include "engine/imgui_impl.hpp"
#include "engine/renderer/renderer_opengl.h"

void UIRendererInit();
void UIRendererPostUpdate();
void UIRendererUpdate();
void UIRendererDraw();
void UIRendererFree();

void DrawPoint(Vector3 pos, float size, GLuint texture, U8 r, U8 g, U8 b);
void DrawLine(Vector3 min, Vector3 max, float thickness, U8 r, U8 g, U8 b);