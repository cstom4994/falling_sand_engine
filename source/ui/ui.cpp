// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "ui/ui.hpp"

#include <memory>

#include "core/core.h"
#include "core/global.hpp"
#include "core/math/mathlib.hpp"
#include "core/platform.h"
#include "core/profiler/profiler.h"
#include "engine/engine.h"
#include "event/inputevent.hpp"
#include "game.hpp"
#include "game_resources.hpp"
#include "renderer/gpu.hpp"
#include "renderer/renderer_gpu.h"
#include "renderer/renderer_opengl.h"

IMPLENGINE();

// Color definitions
METAENGINE_Color bgPanelColor = {0.02, 0.02, 0.05};
METAENGINE_Color bgLightColor = {0.2, 0.2, 0.35};
METAENGINE_Color bgMediumColor = {0.1, 0.1, 0.15};

METAENGINE_Color fieldColor = {0.2, 0.2, 0.2};
METAENGINE_Color fieldEditingColor = {0.3, 0.3, 0.3};
METAENGINE_Color buttonOverColor = {0.3, 0.3, 0.4};

METAENGINE_Color scrollbarInactiveColor = {0.3, 0.3, 0.3};
METAENGINE_Color scrollbarOverColor = {0.5, 0.5, 0.5};

METAENGINE_Color menuTabColor = {0.05, 0.05, 0.10};
METAENGINE_Color menuActiveTabColor = {0.15, 0.15, 0.2};

METAENGINE_Color brightWhite = {250.0f / 255.0f, 250.0f / 255.0f, 250.0f / 255.0f};
METAENGINE_Color lightWhite = {200.0f / 255.0f, 200.0f / 255.0f, 200.0f / 255.0f};

void UISystem::UIRendererInit() {
    // UIData
    METADOT_INFO("Loading UIData");
    uidata = new UIData;

    METADOT_INFO("Loading ImGUI");
    uidata->imgui = MetaEngine::CreateRef<ImGuiLayer>();
    uidata->imgui->Init();

    // Test element drawing
    auto testElement1 = MetaEngine::CreateRef<UIElement>(UIElement{.type = ElementType::windowElement,
                                                                   .resizable = {true},
                                                                   .x = 50,
                                                                   .y = 50,
                                                                   .w = 200,
                                                                   .h = 200,
                                                                   .state = 0,
                                                                   .color = bgPanelColor,
                                                                   .texture = LoadTexture("data/assets/ui/demo_background.png"),
                                                                   .cclass = {.window = {}}});

    auto testElement2 =
            MetaEngine::CreateRef<UIElement>(UIElement{.type = ElementType::textElement, .parent = testElement1, .x = 5, .y = 5, .w = 40, .h = 20, .color = {1, 255, 1, 255}, .text = "哈哈哈哈哈嗝"});
    auto testElement3 = MetaEngine::CreateRef<UIElement>(UIElement{.type = ElementType::buttonElement,
                                                                   .parent = testElement1,
                                                                   .x = 20,
                                                                   .y = 40,
                                                                   .w = 20,
                                                                   .h = 20,
                                                                   .state = 0,
                                                                   .color = {255, 0, 20, 255},
                                                                   .text = "按钮捏",
                                                                   .cclass = {.button = {.hot_color = {1, 255, 1, 255}, .func = []() { METADOT_INFO("button pressed"); }}}});

    auto testElement4 = MetaEngine::CreateRef<UIElement>(
            UIElement{.type = ElementType::progressBarElement,
                      .parent = testElement1,
                      .x = 20,
                      .y = 20,
                      .w = 45,
                      .h = 25,
                      .state = 0,
                      .color = {54, 54, 54, 255},
                      .text = "进度条",
                      .cclass = {.progressbar = {.bar_type = 1, .bar_current = 50.0f, .bar_limit = 1000.0f, .bar_color = {54, 54, 54, 255}, .bar_text_color = {255, 255, 255, 255}}}});

    auto testElement5 = MetaEngine::CreateRef<UIElement>(
            UIElement{.type = ElementType::texturedRectangle, .parent = testElement1, .x = 40, .y = 20, .w = 100, .h = 50, .state = 0, .color = {}, .texture = LoadTexture("data/assets/ui/logo.png")});

    auto testElement6 = MetaEngine::CreateRef<UIElement>(UIElement{.type = ElementType::inputBoxElement,
                                                                   .parent = testElement1,
                                                                   .x = 40,
                                                                   .y = 80,
                                                                   .w = 60,
                                                                   .h = 20,
                                                                   .state = 0,
                                                                   .color = {54, 54, 54, 255},
                                                                   .text = "编辑框1",
                                                                   .cclass = {.inputbox = {.bg_color = {54, 54, 54, 255}, .text_color = {255, 0, 20, 255}}}});

    auto testElement7 = MetaEngine::CreateRef<UIElement>(UIElement{.type = ElementType::inputBoxElement,
                                                                   .parent = testElement1,
                                                                   .x = 40,
                                                                   .y = 105,
                                                                   .w = 60,
                                                                   .h = 20,
                                                                   .state = 0,
                                                                   .color = {54, 54, 54, 255},
                                                                   .text = "编辑框2",
                                                                   .cclass = {.inputbox = {.bg_color = {54, 54, 54, 255}, .text_color = {255, 0, 20, 255}}}});

    uidata->elementLists.insert(std::make_pair("testElement1", testElement1));
    uidata->elementLists.insert(std::make_pair("testElement2", testElement2));
    uidata->elementLists.insert(std::make_pair("testElement3", testElement3));
    uidata->elementLists.insert(std::make_pair("testElement4", testElement4));
    uidata->elementLists.insert(std::make_pair("testElement5", testElement5));
    uidata->elementLists.insert(std::make_pair("testElement6", testElement6));
    uidata->elementLists.insert(std::make_pair("testElement7", testElement7));
}

void UISystem::UIRendererPostUpdate() {
    uidata->imgui->NewFrame();
    // Update UI layout context

    for (auto &&e : uidata->elementLists) {
        if (e.second->type == ElementType::windowElement) {
            // layout_set_behave(ctx, child, LAYOUT_FILL);
            // layout_insert(ctx, root, child);
        }
    }
}

void UISystem::UIRendererDraw() {

    // METADOT_SCOPE_BEGIN(UIRendererDraw);

    if (global.game->state == LOADING) {
        // METADOT_SCOPE_END(UIRendererDraw);
        return;
    }

    // Drawing element
    for (auto &&e : uidata->elementLists) {

        if (!e.second->visible) continue;

        int p_x, p_y;
        if (e.second->parent != nullptr) {
            if (!e.second->parent->visible) continue;
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
            // layout_vec2 win_s = layout_get_size(ctx, e.second->cclass.window.layout_id);
            if (Img) {
                R_SetImageFilter(Img, R_FILTER_NEAREST);
                R_SetBlendMode(Img, R_BLEND_NORMAL);
                metadot_rect dest{.x = (float)(e.second->x), .y = (float)(e.second->y), .w = (float)(e.second->w), .h = (float)(e.second->h)};
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
        if (e.second->type == ElementType::textElement || e.second->type == ElementType::inputBoxElement) {
            MetaEngine::Drawing::drawText(e.second->text, e.second->color, p_x + e.second->x, p_y + e.second->y);
        }
        if (e.second->type == ElementType::inputBoxElement) {
            R_Rectangle(Render.target, p_x + e.second->x, p_y + e.second->y, p_x + e.second->x + e.second->w, p_y + e.second->y + e.second->h, e.second->color);

            if (e.second.get() == uidata->oninput) {
                int slen = ImGui::CalcTextSize(e.second->text.c_str()).x + 2;
                R_RectangleFilled(Render.target, p_x + e.second->x + slen, p_y + e.second->y + 2, p_x + e.second->x + slen + 2, p_y + e.second->y + e.second->h - 4,
                                  e.second->cclass.inputbox.bg_color);
            }
        }

        if (Img) R_FreeImage(Img);

        if (global.game->GameIsolate_.globaldef.draw_ui_debug) {
            R_Rectangle(Render.target, p_x + e.second->x, p_y + e.second->y, p_x + e.second->x + e.second->w, p_y + e.second->y + e.second->h, {255, 20, 147, 255});
            if (e.second->resizable.resizable) {
                R_Rectangle(Render.target, p_x + e.second->x + e.second->w - 20.0f, p_y + e.second->y + e.second->h - 20.0f, p_x + e.second->x + e.second->w, p_y + e.second->y + e.second->h,
                            {40, 20, 147, 255});
            }
        }
    }

    // METADOT_SCOPE_END(UIRendererDraw);
}

void UISystem::UIRendererDrawImGui() { uidata->imgui->Draw(); }

F32 BoxDistence(metadot_rect box, vec2 A) {
    if (A.x >= box.x && A.x <= box.x + box.w && A.y >= box.y && A.y <= box.y + box.h) return -1.0f;
    return 0;
}

void UISystem::UIRendererUpdate() {

    uidata->imgui->Update();
    auto &l = Scripting::GetSingletonPtr()->Lua->s_lua;
    LuaWrapper::LuaFunction OnGameGUIUpdate = l["OnGameGUIUpdate"];
    OnGameGUIUpdate();

    if (global.game->state == LOADING) return;

    bool ImGuiOnControl = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;

    // Mouse pos
    int x = ControlSystem::mouse_x, y = ControlSystem::mouse_y;

    auto clear_state = [&]() {
        uidata->oninput = nullptr;
        uidata->onmoving = nullptr;
    };

    for (auto &&e : uidata->elementLists) {

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
            // Resize window
            if (e.second->resizable.resizing || (e.second->resizable.resizable && !e.second->movable.moving && BoxDistence(rect, {(float)x, (float)y}) < 0.0f &&
                                                 abs(y - e.second->y - e.second->h) < 20.0f && abs(x - e.second->x - e.second->w) < 20.0f)) {
                if (ControlSystem::lmouse_down && !ImGuiOnControl) {
                    if (!e.second->resizable.resizing) {
                        e.second->resizable.mx = x;
                        e.second->resizable.my = y;
                        e.second->resizable.resizing = true;
                    }
                    e.second->w = e.second->resizable.ow + (x - e.second->resizable.mx);
                    e.second->h = e.second->resizable.oh + (y - e.second->resizable.my);
                    clear_state();
                } else {
                    e.second->resizable.resizing = false;
                    e.second->resizable.ow = e.second->w;
                    e.second->resizable.oh = e.second->h;
                }
                continue;
            }
            // Move window
            if (e.second->movable.moving || (!e.second->resizable.resizing && BoxDistence(rect, {(float)x, (float)y}) < 0.0f)) {
                if (ControlSystem::lmouse_down && !ImGuiOnControl) {  // && y - e.second->y < 15.0f
                    if (!e.second->movable.moving) {
                        e.second->movable.mx = x;
                        e.second->movable.my = y;
                        e.second->movable.moving = true;
                    }
                    // METADOT_INFO("window move %d %d", x, y);
                    e.second->x = e.second->movable.ox + (x - e.second->movable.mx);
                    e.second->y = e.second->movable.oy + (y - e.second->movable.my);
                    // e.second->minRectX = (x - e.second->minRectX) + global.game->GameIsolate_.ui->uidata->mouse_dx;
                    // e.second->minRectY = (y - e.second->minRectY) + global.game->GameIsolate_.ui->uidata->mouse_dy;
                    clear_state();
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
                if (ControlSystem::lmouse_down && !ImGuiOnControl && NULL != e.second->cclass.button.func) {
                    e.second->state = 2;
                    e.second->cclass.button.func();
                    clear_state();
                }
            } else {
                e.second->state = 0;
            }
        }
        if (e.second->type == ElementType::progressBarElement) {
            // Test haha
            e.second->cclass.progressbar.bar_current = (e.second->cclass.progressbar.bar_current < e.second->cclass.progressbar.bar_limit) ? e.second->cclass.progressbar.bar_current + 1 : 0;
        }
        if (e.second->type == ElementType::inputBoxElement) {
            if (BoxDistence(rect, {(float)x, (float)y}) < 0.0f) {
                e.second->state = 1;
                if (ControlSystem::lmouse_down && !ImGuiOnControl) {
                    e.second->state = 2;
                    uidata->oninput = e.second.get();
                }
            } else {
                e.second->state = 0;
            }
        }
    }
}

void UISystem::UIRendererFree() {
    uidata->imgui->End();
    uidata->imgui.reset();

    for (auto &&e : uidata->elementLists) {
        if (static_cast<bool>(e.second->texture)) DestroyTexture(e.second->texture);
        // if (static_cast<bool>(e.second)) delete e.second;
        if (e.second.get()) e.second.reset();
    }

    delete uidata;
}

bool UISystem::UIRendererInput(C_KeyboardEvent event) {
    if (event.type != SDL_KEYDOWN) return false;
    if (uidata->oninput != nullptr) {
        if (event.keysym.sym == SDLK_BACKSPACE) {
            // TODO fix Chinese input
            // uidata->oninput->text = uidata->oninput->text.substr(0, uidata->oninput->text.length() - (SUtil::is_chinese_c(uidata->oninput->text.back()) ? 2 : 1));
            uidata->oninput->text = uidata->oninput->text.substr(0, uidata->oninput->text.length() - 1);
            return true;
        } else if (event.keysym.sym == SDLK_ESCAPE) {
            uidata->oninput = nullptr;
            return true;
        }
        uidata->oninput->text += ControlSystem::SDLKeyToString(event.keysym.sym);
        return true;
    }
    return false;
}

bool UISystem::UIIsMouseOnControls() {
    // Mouse pos
    int x = ControlSystem::mouse_x, y = ControlSystem::mouse_y;

    for (auto &&e : uidata->elementLists) {
        metadot_rect rect{.x = (float)e.second->x, .y = (float)e.second->y, .w = (float)e.second->w, .h = (float)e.second->h};
        if (BoxDistence(rect, {(float)x, (float)y}) < 0.0f) return true;
    }
    return false;
}

void UISystem::DrawPoint(vec3 pos, float size, Texture *texture, U8 r, U8 g, U8 b) {
    vec3 min = {pos.x - size, pos.y - size, 0};
    vec3 max = {pos.x + size, pos.y + size, 0};

    if (texture) {
        // DrawRectangleTextured(min, max, texture, r, g, b);
    } else {
        // DrawRectangle(min, max, r, g, b);
    }
}

void UISystem::DrawLine(vec3 min, vec3 max, float thickness, U8 r, U8 g, U8 b) { R_Line(Render.target, min.x, min.y, max.x, max.y, {r, g, b, 255}); }

void UISystem::Create() { UIRendererInit(); }

void UISystem::Destory() { UIRendererFree(); }

void UISystem::Reload() {}

void UISystem::RegisterLua(LuaWrapper::State &s_lua) {}
