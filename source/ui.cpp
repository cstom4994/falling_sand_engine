// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "ui.hpp"

#include <memory>

#include "core/core.h"
#include "core/global.hpp"
#include "engine/engine.h"
#include "engine/engine_input.hpp"
#include "engine/engine_platform.h"
#include "game.hpp"
#include "game_resources.hpp"
#include "mathlib.h"
#include "mathlib.hpp"
#include "renderer/gpu.hpp"
#include "renderer/renderer_gpu.h"
#include "renderer/renderer_opengl.h"
#include "renderer/renderer_utils.h"
#include "ui_layout.h"

IMPLENGINE();

// Color definitions
metadot_vec3 bgPanelColor = {0.02, 0.02, 0.05};
metadot_vec3 bgLightColor = {0.2, 0.2, 0.35};
metadot_vec3 bgMediumColor = {0.1, 0.1, 0.15};

metadot_vec3 fieldColor = {0.2, 0.2, 0.2};
metadot_vec3 fieldEditingColor = {0.3, 0.3, 0.3};
metadot_vec3 buttonOverColor = {0.3, 0.3, 0.4};

metadot_vec3 scrollbarInactiveColor = {0.3, 0.3, 0.3};
metadot_vec3 scrollbarOverColor = {0.5, 0.5, 0.5};

metadot_vec3 menuTabColor = {0.05, 0.05, 0.10};
metadot_vec3 menuActiveTabColor = {0.15, 0.15, 0.2};

metadot_vec3 brightWhite = {250.0f / 255.0f, 250.0f / 255.0f, 250.0f / 255.0f};
metadot_vec3 lightWhite = {200.0f / 255.0f, 200.0f / 255.0f, 200.0f / 255.0f};

void UIRendererInit() {
    // UIData
    METADOT_INFO("Loading UIData");
    global.uidata = new UIData;

    METADOT_INFO("Loading ImGUI");
    METADOT_NEW(C, global.uidata->imguiCore, ImGuiCore);
    global.uidata->imguiCore->Init();

    layout_init_context(&global.uidata->layoutContext);

    // Test element drawing
    UIElement *testElement1 = new UIElement{.type = ElementType::windowElement,
                                            .x = 50,
                                            .y = 50,
                                            .w = 200,
                                            .h = 200,
                                            .state = 0,
                                            .color = bgPanelColor,
                                            .texture = LoadTexture("data/assets/ui/demo_background.png"),
                                            .cclass = {.window = (UI_Window){}}};

    UIElement *testElement2 = new UIElement{.type = ElementType::textElement, .parent = testElement1, .x = 5, .y = 5, .w = 40, .h = 20, .color = {1, 255, 1, 255}, .text = "哈哈哈哈哈嗝"};
    UIElement *testElement3 = new UIElement{.type = ElementType::buttonElement,
                                            .parent = testElement1,
                                            .x = 20,
                                            .y = 40,
                                            .w = 20,
                                            .h = 20,
                                            .state = 0,
                                            .color = {255, 0, 20, 255},
                                            .text = "按钮捏",
                                            .cclass = {.button = (UI_Button){.hot_color = {1, 255, 1, 255}, .func = []() { METADOT_INFO("button pressed"); }}}};

    UIElement *testElement4 = new UIElement{
            .type = ElementType::progressBarElement,
            .parent = testElement1,
            .x = 20,
            .y = 20,
            .w = 45,
            .h = 25,
            .state = 0,
            .color = {54, 54, 54, 255},
            .text = "进度条",
            .cclass = {.progressbar = (UI_ProgressBar){.bar_type = 1, .bar_current = 50.0f, .bar_limit = 1000.0f, .bar_color = {54, 54, 54, 255}, .bar_text_color = {255, 255, 255, 255}}}};

    global.uidata->elementLists.insert(std::make_pair("testElement1", testElement1));
    global.uidata->elementLists.insert(std::make_pair("testElement2", testElement2));
    global.uidata->elementLists.insert(std::make_pair("testElement3", testElement3));
    global.uidata->elementLists.insert(std::make_pair("testElement4", testElement4));
}

void UIRendererPostUpdate() {
    global.uidata->imguiCore->NewFrame();

    auto ctx = &global.uidata->layoutContext;

    // Update UI layout context

    for (auto &&e : global.uidata->elementLists) {
        if (e.second->type == ElementType::windowElement) {
            e.second->cclass.window.layout_id = layout_item(ctx);

            layout_set_size_xy(ctx, e.second->cclass.window.layout_id, e.second->w, e.second->h);
            // layout_set_behave(ctx, child, LAYOUT_FILL);
            // layout_insert(ctx, root, child);
        }
    }

    layout_run_context(ctx);
}

void UIRendererDraw() {

    if (global.game->state == LOADING) return;

    auto ctx = &global.uidata->layoutContext;

    // Drawing element
    for (auto &&e : global.uidata->elementLists) {

        if (!e.second->visible) return;

        int p_x, p_y;
        if (e.second->parent != nullptr) {
            if (!e.second->parent->visible) return;
            p_x = e.second->parent->x;
            p_y = e.second->parent->y;
            // METADOT_BUG("%s .parent %s", e.first.c_str(), e.second->parent->text.c_str());
        } else {
            p_x = 0;
            p_y = 0;
        }

        // Image cache
        R_Image *Img = nullptr;
        if (e.second->texture) {
            Img = R_CopyImageFromSurface(e.second->texture->surface);
        }

        if (e.second->type == ElementType::lineElement) {
            R_Line(Render.target, p_x + e.second->x, p_y + e.second->y, p_x + e.second->x + e.second->w, p_y + e.second->y + e.second->h, e.second->color);
        }
        if (e.second->type == ElementType::coloredRectangle || e.second->type == windowElement) {
            if (!Img) R_RectangleFilled(Render.target, p_x + e.second->x, p_y + e.second->y, p_x + e.second->x + e.second->w, p_y + e.second->y + e.second->h, e.second->color);
        }
        if (e.second->type == ElementType::progressBarElement) {
            // METADOT_BUG("parent xy %d %d", p_x, p_y);
            int drect = e.second->w;
            float p = e.second->cclass.progressbar.bar_current / e.second->cclass.progressbar.bar_limit;
            int c = p_x + e.second->x + p * drect;
            R_RectangleRound(Render.target, p_x + e.second->x, p_y + e.second->y, p_x + e.second->x + e.second->w, p_y + e.second->y + e.second->h, 2.0f, e.second->color);
            R_RectangleFilled(Render.target, p_x + e.second->x, p_y + e.second->y, c, p_y + e.second->y + e.second->h, e.second->color);

            if (e.second->cclass.progressbar.bar_type == 1) MetaEngine::Drawing::drawText(e.second->text, e.second->cclass.progressbar.bar_text_color, p_x + e.second->x, p_y + e.second->y);
        }
        if (e.second->type == ElementType::texturedRectangle || e.second->type == ElementType::buttonElement) {
            if (Img) {
                R_SetImageFilter(Img, R_FILTER_NEAREST);
                R_SetBlendMode(Img, R_BLEND_NORMAL);
                metadot_rect dest{.x = (float)(e.second->x + p_x), .y = (float)(e.second->y + p_y), .w = (float)e.second->w, .h = (float)e.second->h};
                R_BlitRect(Img, NULL, Render.target, &dest);
            }
        }
        if (e.second->type == ElementType::windowElement) {
            layout_vec2 win_s = layout_get_size(ctx, e.second->cclass.window.layout_id);
            if (Img) {
                R_SetImageFilter(Img, R_FILTER_NEAREST);
                R_SetBlendMode(Img, R_BLEND_NORMAL);
                metadot_rect dest{.x = (float)(e.second->x), .y = (float)(e.second->y), .w = (float)(win_s[0]), .h = (float)(win_s[1])};
                R_BlitRect(Img, NULL, Render.target, &dest);
            }
        }
        if (e.second->type == ElementType::buttonElement) {
            if (e.second->state == 1) {
                MetaEngine::Drawing::drawText(e.second->text, e.second->cclass.button.hot_color, p_x + e.second->x, p_y + e.second->y);
            } else {
                MetaEngine::Drawing::drawText(e.second->text, e.second->color, p_x + e.second->x, p_y + e.second->y);
            }
        }
        if (e.second->type == ElementType::textElement) {
            MetaEngine::Drawing::drawText(e.second->text, e.second->color, p_x + e.second->x, p_y + e.second->y);
        }

        if (Img) R_FreeImage(Img);

        if (global.game->GameIsolate_.globaldef.draw_ui_debug)
            R_Rectangle(Render.target, p_x + e.second->x, p_y + e.second->y, p_x + e.second->x + e.second->w, p_y + e.second->y + e.second->h, {255, 20, 147, 255});
    }
}

void UIRendererDrawImGui() { global.uidata->imguiCore->Draw(); }

F32 BoxDistence(metadot_rect box, R_vec2 A) {
    if (A.x >= box.x && A.x <= box.x + box.w && A.y >= box.y && A.y <= box.y + box.h) return -1.0f;
    return 0;
}

void UIRendererUpdate() {

    global.uidata->imguiCore->Update();
    auto &l = Scripts::GetSingletonPtr()->LuaCoreCpp->s_lua;
    LuaWrapper::LuaFunction OnGameGUIUpdate = l["OnGameGUIUpdate"];
    OnGameGUIUpdate();

    if (global.game->state == LOADING) return;

    bool ImGuiOnControl = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;

    // Mouse pos
    int x, y;
    metadot_get_mousepos(&x, &y);

    for (auto &&e : global.uidata->elementLists) {

        int p_x, p_y;
        if (e.second->parent != nullptr) {
            p_x = e.second->parent->x;
            p_y = e.second->parent->y;
            // METADOT_BUG("%s .parent %s", e.first.c_str(), e.second->parent->text.c_str());
        } else {
            p_x = 0;
            p_y = 0;
        }

        metadot_rect rect{.x = (float)e.second->x + p_x, .y = (float)e.second->y + p_y, .w = (float)e.second->w, .h = (float)e.second->h};
        if (e.second->type == ElementType::windowElement) {
            // Move window
            if (BoxDistence(rect, {(float)x, (float)y}) < 0.0f) {
                if (ControlSystem::lmouse && !ImGuiOnControl) {  // && y - e.second->y < 15.0f
                    if (!e.second->movable.moving) {
                        e.second->movable.mx = x;
                        e.second->movable.my = y;
                        e.second->movable.moving = true;
                    }
                    // METADOT_INFO("window move %d %d", x, y);
                    e.second->x = e.second->movable.ox + (x - e.second->movable.mx);
                    e.second->y = e.second->movable.oy + (y - e.second->movable.my);
                    // e.second->minRectX = (x - e.second->minRectX) + global.uidata->mouse_dx;
                    // e.second->minRectY = (y - e.second->minRectY) + global.uidata->mouse_dy;
                } else {
                    e.second->movable.moving = false;
                    e.second->movable.ox = e.second->x;
                    e.second->movable.oy = e.second->y;
                }
            }
        }
        if (e.second->type == ElementType::buttonElement) {
            // Pressed button
            if (BoxDistence(rect, {(float)x, (float)y}) < 0.0f) {
                e.second->state = 1;
                if (ControlSystem::lmouse && !ImGuiOnControl && NULL != e.second->cclass.button.func) {
                    e.second->state = 2;
                    e.second->cclass.button.func();
                }
            } else {
                e.second->state = 0;
            }
        }
        if (e.second->type == ElementType::progressBarElement) {
            // Test haha
            e.second->cclass.progressbar.bar_current = (e.second->cclass.progressbar.bar_current < e.second->cclass.progressbar.bar_limit) ? e.second->cclass.progressbar.bar_current + 1 : 0;
        }
    }
}

void UIRendererFree() {
    global.uidata->imguiCore->End();
    METADOT_DELETE(C, global.uidata->imguiCore, ImGuiCore);

    for (auto &&e : global.uidata->elementLists) {
        if (static_cast<bool>(e.second->texture)) Eng_DestroyTexture(e.second->texture);
        if (static_cast<bool>(e.second)) delete e.second;
    }

    delete global.uidata;
}

bool UIIsMouseOnControls() {
    // Mouse pos
    int x, y;
    metadot_get_mousepos(&x, &y);

    for (auto &&e : global.uidata->elementLists) {
        metadot_rect rect{.x = (float)e.second->x, .y = (float)e.second->y, .w = (float)e.second->w, .h = (float)e.second->h};
        if (BoxDistence(rect, {(float)x, (float)y}) < 0.0f) return true;
    }
    return false;
}

void DrawPoint(metadot_vec3 pos, float size, Texture *texture, U8 r, U8 g, U8 b) {
    metadot_vec3 min = {pos.X - size, pos.Y - size, 0};
    metadot_vec3 max = {pos.X + size, pos.Y + size, 0};

    if (texture) {
        // DrawRectangleTextured(min, max, texture, r, g, b);
    } else {
        // DrawRectangle(min, max, r, g, b);
    }
}

void DrawLine(metadot_vec3 min, metadot_vec3 max, float thickness, U8 r, U8 g, U8 b) { R_Line(Render.target, min.X, min.Y, max.X, max.Y, {r, g, b, 255}); }

void DrawTextWithPlate() {}