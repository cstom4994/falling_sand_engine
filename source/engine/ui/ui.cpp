// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "engine/ui/ui.hpp"

#include <memory>

#include "engine/core/core.hpp"
#include "engine/core/global.hpp"
#include "engine/core/mathlib.hpp"
#include "engine/core/platform.h"
#include "engine/core/profiler.hpp"
#include "engine/engine.h"
#include "engine/event/inputevent.hpp"
#include "engine/game.hpp"
#include "engine/renderer/gpu.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/renderer/renderer_opengl.h"
#include "engine/textures.hpp"

// Color definitions
MEcolor bgPanelColor = {0.02, 0.02, 0.05};
MEcolor bgLightColor = {0.2, 0.2, 0.35};
MEcolor bgMediumColor = {0.1, 0.1, 0.15};

MEcolor fieldColor = {0.2, 0.2, 0.2};
MEcolor fieldEditingColor = {0.3, 0.3, 0.3};
MEcolor buttonOverColor = {0.3, 0.3, 0.4};

MEcolor scrollbarInactiveColor = {0.3, 0.3, 0.3};
MEcolor scrollbarOverColor = {0.5, 0.5, 0.5};

MEcolor menuTabColor = {0.05, 0.05, 0.10};
MEcolor menuActiveTabColor = {0.15, 0.15, 0.2};

MEcolor brightWhite = {250.0f / 255.0f, 250.0f / 255.0f, 250.0f / 255.0f};
MEcolor lightWhite = {200.0f / 255.0f, 200.0f / 255.0f, 200.0f / 255.0f};

void UISystem::UIRendererInit() {
    // UIData
    METADOT_INFO("Loading UIData");
    uidata = new UIData;

    METADOT_INFO("Loading ImGUI");
    uidata->imgui = ME::create_ref<ImGuiLayer>();
    uidata->imgui->Init();

    // Test element drawing
    auto testElement1 = ME::create_ref<UIElement>(UIElement{.type = ElementType::windowElement,
                                                            .visible = false,
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
            ME::create_ref<UIElement>(UIElement{.type = ElementType::textElement, .parent = testElement1, .x = 5, .y = 5, .w = 40, .h = 20, .color = {1, 255, 1, 255}, .text = "哈哈哈哈哈嗝"});
    auto testElement3 = ME::create_ref<UIElement>(UIElement{.type = ElementType::buttonElement,
                                                            .parent = testElement1,
                                                            .x = 20,
                                                            .y = 40,
                                                            .w = 20,
                                                            .h = 20,
                                                            .state = 0,
                                                            .color = {255, 0, 20, 255},
                                                            .text = "按钮捏",
                                                            .cclass = {.button = {.hot_color = {1, 255, 1, 255}, .func = []() { METADOT_INFO("button pressed"); }}}});

    auto testElement4 = ME::create_ref<UIElement>(
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

    auto testElement5 = ME::create_ref<UIElement>(
            UIElement{.type = ElementType::texturedRectangle, .parent = testElement1, .x = 40, .y = 20, .w = 100, .h = 50, .state = 0, .color = {}, .texture = LoadTexture("data/assets/ui/logo.png")});

    auto testElement6 = ME::create_ref<UIElement>(UIElement{.type = ElementType::inputBoxElement,
                                                            .parent = testElement1,
                                                            .x = 40,
                                                            .y = 80,
                                                            .w = 60,
                                                            .h = 20,
                                                            .state = 0,
                                                            .color = {54, 54, 54, 255},
                                                            .text = "编辑框1",
                                                            .cclass = {.inputbox = {.bg_color = {54, 54, 54, 255}, .text_color = {255, 0, 20, 255}}}});

    auto testElement7 = ME::create_ref<UIElement>(UIElement{.type = ElementType::inputBoxElement,
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

    auto window_menu = ME::create_ref<UIElement>(UIElement{.type = ElementType::windowElement,
                                                           .visible = false,
                                                           .resizable = {true},
                                                           .x = 150,
                                                           .y = 150,
                                                           .w = 200,
                                                           .h = 200,
                                                           .state = 0,
                                                           .color = bgPanelColor,
                                                           .texture = LoadTexture("data/assets/ui/demo_background.png"),
                                                           .cclass = {.window = {}}});

    auto button_play = ME::create_ref<UIElement>(UIElement{.type = ElementType::buttonElement,
                                                           .parent = window_menu,
                                                           .x = 20,
                                                           .y = 40,
                                                           .w = 20,
                                                           .h = 20,
                                                           .state = 0,
                                                           .color = {255, 0, 20, 255},
                                                           .text = "按钮捏",
                                                           .cclass = {.button = {.hot_color = {1, 255, 1, 255}, .func = []() { METADOT_INFO("button pressed"); }}}});

    uidata->elementLists.insert(std::make_pair("window_menu", window_menu));
    uidata->elementLists.insert(std::make_pair("button_play", button_play));
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
            Img = R_CopyImageFromSurface(e.second->texture->surface());
        }

        if (e.second->type == ElementType::lineElement) {
            R_Line(ENGINE()->target, p_x + e.second->x, p_y + e.second->y, p_x + e.second->x + e.second->w, p_y + e.second->y + e.second->h, e.second->color);
        }
        if (e.second->type == ElementType::coloredRectangle || e.second->type == windowElement) {
            if (!Img) R_RectangleFilled(ENGINE()->target, p_x + e.second->x, p_y + e.second->y, p_x + e.second->x + e.second->w, p_y + e.second->y + e.second->h, e.second->color);
        }
        if (e.second->type == ElementType::progressBarElement) {
            // METADOT_BUG("parent xy %d %d", p_x, p_y);
            int drect = e.second->w;
            float p = e.second->cclass.progressbar.bar_current / e.second->cclass.progressbar.bar_limit;
            int c = p_x + e.second->x + p * drect;
            R_RectangleRound(ENGINE()->target, p_x + e.second->x, p_y + e.second->y, p_x + e.second->x + e.second->w, p_y + e.second->y + e.second->h, 2.0f, e.second->color);
            R_RectangleFilled(ENGINE()->target, p_x + e.second->x, p_y + e.second->y, c, p_y + e.second->y + e.second->h, e.second->color);

            if (e.second->cclass.progressbar.bar_type == 1) ME_draw_text(e.second->text, e.second->cclass.progressbar.bar_text_color, p_x + e.second->x, p_y + e.second->y);
        }
        if (e.second->type == ElementType::texturedRectangle || e.second->type == ElementType::buttonElement) {
            if (Img) {
                R_SetImageFilter(Img, R_FILTER_NEAREST);
                R_SetBlendMode(Img, R_BLEND_NORMAL);
                MErect dest{.x = (float)(e.second->x + p_x), .y = (float)(e.second->y + p_y), .w = (float)e.second->w, .h = (float)e.second->h};
                R_BlitRect(Img, NULL, ENGINE()->target, &dest);
            }
        }
        if (e.second->type == ElementType::windowElement) {
            // layout_vec2 win_s = layout_get_size(ctx, e.second->cclass.window.layout_id);
            if (Img) {
                R_SetImageFilter(Img, R_FILTER_NEAREST);
                R_SetBlendMode(Img, R_BLEND_NORMAL);
                MErect dest{.x = (float)(e.second->x), .y = (float)(e.second->y), .w = (float)(e.second->w), .h = (float)(e.second->h)};
                R_BlitRect(Img, NULL, ENGINE()->target, &dest);
            }
        }
        if (e.second->type == ElementType::buttonElement) {
            if (e.second->state == 1) {
                ME_draw_text(e.second->text, e.second->cclass.button.hot_color, p_x + e.second->x, p_y + e.second->y);
            } else {
                ME_draw_text(e.second->text, e.second->color, p_x + e.second->x, p_y + e.second->y);
            }
        }
        if (e.second->type == ElementType::textElement || e.second->type == ElementType::inputBoxElement) {
            ME_draw_text(e.second->text, e.second->color, p_x + e.second->x, p_y + e.second->y);
        }
        if (e.second->type == ElementType::inputBoxElement) {
            R_Rectangle(ENGINE()->target, p_x + e.second->x, p_y + e.second->y, p_x + e.second->x + e.second->w, p_y + e.second->y + e.second->h, e.second->color);

            if (e.second.get() == uidata->oninput) {
                int slen = ImGui::CalcTextSize(e.second->text.c_str()).x + 2;
                R_RectangleFilled(ENGINE()->target, p_x + e.second->x + slen, p_y + e.second->y + 2, p_x + e.second->x + slen + 2, p_y + e.second->y + e.second->h - 4,
                                  e.second->cclass.inputbox.bg_color);
            }
        }

        if (Img) R_FreeImage(Img);

        if (global.game->Iso.globaldef.draw_ui_debug) {
            R_Rectangle(ENGINE()->target, p_x + e.second->x, p_y + e.second->y, p_x + e.second->x + e.second->w, p_y + e.second->y + e.second->h, {255, 20, 147, 255});
            if (e.second->resizable.resizable) {
                R_Rectangle(ENGINE()->target, p_x + e.second->x + e.second->w - 20.0f, p_y + e.second->y + e.second->h - 20.0f, p_x + e.second->x + e.second->w, p_y + e.second->y + e.second->h,
                            {40, 20, 147, 255});
            }
        }
    }

    // METADOT_SCOPE_END(UIRendererDraw);
}

void UISystem::UIRendererDrawImGui() {
    ME_profiler_scope_auto("DrawImGui");
    uidata->imgui->Draw();
}

f32 BoxDistence(MErect box, MEvec2 A) {
    if (A.x >= box.x && A.x <= box.x + box.w && A.y >= box.y && A.y <= box.y + box.h) return -1.0f;
    return 0;
}

void UISystem::UIRendererUpdate() {

    uidata->imgui->Update();
    auto &l = Scripting::get_singleton_ptr()->Lua->s_lua;
    ME::LuaWrapper::LuaFunction OnGameGUIUpdate = l["OnGameGUIUpdate"];
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

        MErect rect{.x = (float)e.second->x + p_x, .y = (float)e.second->y + p_y, .w = (float)e.second->w, .h = (float)e.second->h};
        if (e.second->type == ElementType::windowElement) {
            // Focus window
            if (BoxDistence(rect, {(float)x, (float)y}) < 0.0f) {
                // if (!ImGuiOnControl) {
                //     e.second->state = 1;
                // } else {
                //     e.second->state = 0;
                // }
            }

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
            if (e.second->movable.moving ||
                (uidata->onmoving == ((bool)uidata->onmoving ? e.second.get() : nullptr) && !e.second->resizable.resizing && BoxDistence(rect, {(float)x, (float)y}) < 0.0f)) {
                if (ControlSystem::lmouse_down && !ImGuiOnControl) {  // && y - e.second->y < 15.0f
                    if (!e.second->movable.moving) {
                        e.second->movable.mx = x;
                        e.second->movable.my = y;
                        e.second->movable.moving = true;
                        uidata->onmoving = e.second.get();
                    }
                    // METADOT_INFO("window move %d %d", x, y);
                    e.second->x = e.second->movable.ox + (x - e.second->movable.mx);
                    e.second->y = e.second->movable.oy + (y - e.second->movable.my);
                    // e.second->minRectX = (x - e.second->minRectX) + global.game->GameIsolate_.ui->uidata->mouse_dx;
                    // e.second->minRectY = (y - e.second->minRectY) + global.game->GameIsolate_.ui->uidata->mouse_dy;

                    uidata->oninput = nullptr;
                } else {
                    e.second->movable.moving = false;
                    e.second->movable.ox = e.second->x;
                    e.second->movable.oy = e.second->y;

                    uidata->onmoving = nullptr;
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
        // 目前这个UI系统的贴图不来自贴图包 这里直接释放
        if (static_cast<bool>(e.second->texture)) e.second->texture.reset();
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
        MErect rect{.x = (float)e.second->x, .y = (float)e.second->y, .w = (float)e.second->w, .h = (float)e.second->h};
        if (BoxDistence(rect, {(float)x, (float)y}) < 0.0f) return true;
    }
    return false;
}

void UISystem::DrawPoint(MEvec3 pos, float size, Texture *texture, u8 r, u8 g, u8 b) {
    MEvec3 min = {pos.x - size, pos.y - size, 0};
    MEvec3 max = {pos.x + size, pos.y + size, 0};

    if (texture) {
        // DrawRectangleTextured(min, max, texture, r, g, b);
    } else {
        // DrawRectangle(min, max, r, g, b);
    }
}

void UISystem::DrawLine(MEvec3 min, MEvec3 max, float thickness, u8 r, u8 g, u8 b) { R_Line(ENGINE()->target, min.x, min.y, max.x, max.y, {r, g, b, 255}); }

void UISystem::create() { UIRendererInit(); }

void UISystem::destory() { UIRendererFree(); }

void UISystem::reload() {}

void UISystem::registerLua(ME::LuaWrapper::State &s_lua) {}
