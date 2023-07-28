// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "engine/ui/ui.hpp"

#include <memory>

#include "engine/core/core.hpp"
#include "engine/core/global.hpp"
#include "engine/core/mathlib.hpp"
#include "engine/core/platform.h"
#include "engine/core/profiler.hpp"
#include "engine/engine.hpp"
#include "engine/event/inputevent.hpp"
#include "engine/game.hpp"
#include "engine/renderer/gpu.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/renderer/renderer_opengl.h"
#include "engine/textures.hpp"

namespace ME {

f32 box_distence(MErect box, MEvec2 A) {
    if (A.x >= box.x && A.x <= box.x + box.w && A.y >= box.y && A.y <= box.y + box.h) return -1.0f;
    return 0;
}

void gui::render_postupdate() { imgui->NewFrame(); }

void gui::render() {
    // GUI边界检测 防止UI窗口移动到视图外
    for (auto& v : uis) {
        v->bounds->x = std::fmax(0, std::fmin(v->bounds->x, the<engine>().eng()->windowWidth - v->bounds->w));
        v->bounds->y = std::fmax(0, std::fmin(v->bounds->y, the<engine>().eng()->windowHeight - v->bounds->h));
    }

    auto* target = the<engine>().eng()->target;

    debugUI->draw(target, 0, 0);

    if (chiselUI != NULL) {
        if (!chiselUI->visible) {
            uis.erase(std::remove(uis.begin(), uis.end(), chiselUI), uis.end());
            delete chiselUI;
            chiselUI = NULL;
        } else {
            chiselUI->draw(target, 0, 0);
        }
    }
}

void gui::render_imgui() {
    ME_profiler_scope_auto("DrawImGui");
    imgui->Draw();
}

void gui::render_update() {

    imgui->Update();
    the<scripting>().fast_call_func("OnGameGUIUpdate")();

    if (global.game->state == LOADING) return;

    bool ImGuiOnControl = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

bool gui::push_input(C_KeyboardEvent event) {
    if (event.type != SDL_KEYDOWN) return false;
    // if (uidata->oninput != nullptr) {
    //     if (event.keysym.sym == SDLK_BACKSPACE) {
    //         // TODO fix Chinese input
    //         // uidata->oninput->text = uidata->oninput->text.substr(0, uidata->oninput->text.length() - (SUtil::is_chinese_c(uidata->oninput->text.back()) ? 2 : 1));
    //         uidata->oninput->text = uidata->oninput->text.substr(0, uidata->oninput->text.length() - 1);
    //         return true;
    //     } else if (event.keysym.sym == SDLK_ESCAPE) {
    //         uidata->oninput = nullptr;
    //         return true;
    //     }
    //     uidata->oninput->text += input::SDLKeyToString(event.keysym.sym);
    //     return true;
    // }
    return false;
}

bool gui::push_event(C_Event event) {
    for (auto& v : uis) {
        if (v->visible && v->checkEvent(event, the<engine>().eng()->target, global.game->Iso.world.get(), 0, 0)) {
            return true;
            break;
        }
    }
    return false;
}

void gui::DrawPoint(MEvec3 pos, float size, Texture* texture, u8 r, u8 g, u8 b) {
    MEvec3 min = {pos.x - size, pos.y - size, 0};
    MEvec3 max = {pos.x + size, pos.y + size, 0};

    if (texture) {
        // DrawRectangleTextured(min, max, texture, r, g, b);
    } else {
        // DrawRectangle(min, max, r, g, b);
    }
}

void gui::DrawLine(MEvec3 min, MEvec3 max, float thickness, u8 r, u8 g, u8 b) { R_Line(the<engine>().eng()->target, min.x, min.y, max.x, max.y, {r, g, b, 255}); }

void gui::chisel_ui(RigidBody* cur) {

    chiselUI = new UI(new MErect{0, 0, cur->get_surface()->w * 4.f + 10 + 10, cur->get_surface()->h * 4.f + 10 + 40});
    chiselUI->bounds->x = the<engine>().eng()->windowWidth / 2 - chiselUI->bounds->w / 2;
    chiselUI->bounds->y = the<engine>().eng()->windowHeight / 2 - chiselUI->bounds->h / 2;

    UILabel* chiselUITitleLabel = new UILabel(new MErect{chiselUI->bounds->w / 2, 10, 1, 30}, "Chisel", global.game->basic_font, 0xffffff, ALIGN_CENTER);
    chiselUI->children.push_back(chiselUITitleLabel);

    chiselUI->background = new SolidBackground(0xC0505050);
    chiselUI->drawBorder = true;

    ChiselNode* chisel = new ChiselNode(new MErect{10, 40, chiselUI->bounds->w - 20, chiselUI->bounds->h - 10 - 40});
    chisel->drawBorder = true;
    chisel->rb = cur;
    SDL_Surface* surf =
            SDL_CreateRGBSurfaceWithFormat(cur->get_surface()->flags, cur->get_surface()->w, cur->get_surface()->h, cur->get_surface()->format->BitsPerPixel, cur->get_surface()->format->format);
    SDL_BlitSurface(cur->get_surface(), NULL, surf, NULL);
    chisel->surface = surf;
    chisel->texture = R_CopyImageFromSurface(surf);
    R_SetImageFilter(chisel->texture, R_FILTER_NEAREST);
    chiselUI->children.push_back(chisel);

    uis.push_back(chiselUI);
}

gui::gui() {}

gui::~gui() noexcept {}

void gui::init() {
    METADOT_INFO("Loading ImGUI");
    imgui = create_ref<dbgui>();
    imgui->Init();

    METADOT_INFO("Setting up GUI...");

    // set up main menu ui

    // set up debug ui
    debugUI = new UI(new MErect{25, 25, 200, 350});
    debugUI->background = new SolidBackground(0x80000000);
    debugUI->drawBorder = true;
    debugUI->visible = true;

    UILabel* titleLabel = new UILabel(new MErect{1, 20, 1, 30}, "喵喵", global.game->basic_font, 0xffffff, ALIGN_CENTER);
    debugUI->children.push_back(titleLabel);

    UIButton* button_play = new UIButton(new MErect{50, 50, 100, 30}, "New", global.game->basic_font, 0xffffff, ALIGN_CENTER);
    button_play->drawBorder = true;

    UILabel* button_play_title = new UILabel(new MErect{0, 0, 1, 30}, "button_play_title", global.game->basic_font, 0xffffff, ALIGN_CENTER);
    button_play->children.push_back(button_play_title);
    button_play->selectCallback = [] { METADOT_INFO("button_play->hoverCallback"); };

    debugUI->children.push_back(button_play);

    uis.push_back(debugUI);
}

void gui::end() {
    imgui->End();
    imgui.reset();
}

void gui::registerLua(lua_wrapper::State* p_lua) {}

void UI::draw(R_Target* t, int transformX, int transformY) {
    if (!visible) return;

    background->draw(t, bounds);

    UINode::draw(t, transformX, transformY);
}

bool UI::onEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY) {
    if (!visible) return false;
    if (ev.type == SDL_MOUSEMOTION && ev.motion.state & SDL_BUTTON_LMASK) {
        bounds->x += ev.motion.xrel;
        bounds->y += ev.motion.yrel;

        return true;
    } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
        return true;
    }
    return false;
}

void SolidBackground::draw(R_Target* t, MErect* bounds) {
    R_RectangleFilled(t, bounds->x, bounds->y, bounds->x + bounds->w, bounds->y + bounds->h, {(color >> 16) & 0xff, (color >> 8) & 0xff, (color >> 0) & 0xff, (color >> 24) & 0xff});
}

void UINode::draw(R_Target* t, int transformX, int transformY) {
    if (!visible) return;

    for (auto& c : children) {
        c->parent = this;
        c->draw(t, bounds->x + transformX, bounds->y + transformY);
    }

    if (/*Settings::draw_uinode_bounds ||*/ drawBorder) {
        R_Rectangle(t, bounds->x + transformX, bounds->y + transformY, bounds->x + transformX + bounds->w, bounds->y + transformY + bounds->h, {0xcc, 0xcc, 0xcc, 0xff});
    }
}

bool UINode::checkEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY) {

    if (!visible) return false;

    bool ok = false;

    switch (ev.type) {
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            ok = ev.button.x >= bounds->x + transformX && ev.button.x <= bounds->x + transformX + bounds->w && ev.button.y >= bounds->y + transformY &&
                 ev.button.y <= bounds->y + transformY + bounds->h;
            break;
        case SDL_MOUSEMOTION:
            ok = (ev.motion.x >= bounds->x + transformX && ev.button.x <= bounds->x + transformX + bounds->w && ev.button.y >= bounds->y + transformY &&
                  ev.button.y <= bounds->y + transformY + bounds->h) ||
                 (ev.motion.x - ev.motion.xrel >= bounds->x + transformX && ev.button.x - ev.motion.xrel <= bounds->x + transformX + bounds->w &&
                  ev.button.y - ev.motion.yrel >= bounds->y + transformY && ev.button.y - ev.motion.yrel <= bounds->y + transformY + bounds->h);
            break;
        case SDL_KEYDOWN:
            ok = true;
            break;
        case SDL_TEXTINPUT:
            ok = true;
            break;
        case SDL_TEXTEDITING:
            ok = true;
            break;
    }

    if (ok) {
        bool stopPropagation = false;
        for (auto& c : children) {
            stopPropagation = stopPropagation || c->checkEvent(ev, t, world, bounds->x + transformX, bounds->y + transformY);
        }

        if (!stopPropagation) {
            return onEvent(ev, t, world, bounds->x + transformX, bounds->y + transformY);
        } else {
            return true;
        }
    }

    return false;
}

void UILabel::draw(R_Target* t, int transformX, int transformY) {

    if (!visible) return;

    // drawText(r, text.c_str(), font, bounds->x + transformX, bounds->y + transformY, (textColor >> 16) & 0xff, (textColor >> 8) & 0xff, (textColor >> 0) & 0xff, 0, 0, 0, align);

    // if (texture == NULL && NULL != surface) {
    //     texture = R_CopyImageFromSurface(surface);
    //     R_SetImageFilter(texture, R_FILTER_NEAREST);
    // }

    // if (NULL != texture) R_Blit(texture, NULL, t, bounds->x + transformX + 1 - align * surface->w / 2 + surface->w * 0.5, bounds->y + transformY + 1 + surface->h * 0.5);

    the<fontcache>().push(text, font, (f32)bounds->x + transformX, (f32)bounds->y + transformY);

    UINode::draw(t, transformX, transformY);
}

bool UIButton::checkEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY) {

    if (!visible) return false;
    if (disabled) return false;

    bool ok = false;

    if (ev.type == SDL_MOUSEMOTION) {
        return onEvent(ev, t, world, transformX, transformY);
    }

    return UINode::checkEvent(ev, t, world, transformX, transformY);
}

bool UIButton::onEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY) {

    if (ev.type == SDL_MOUSEBUTTONUP) {
        selectCallback();
        return true;
    } else if (ev.type == SDL_MOUSEMOTION) {

        hovered = (ev.motion.x >= bounds->x + transformX && ev.button.x <= bounds->x + transformX + bounds->w && ev.button.y >= bounds->y + transformY &&
                   ev.button.y <= bounds->y + transformY + bounds->h) ||
                  (ev.motion.x - ev.motion.xrel >= bounds->x + transformX && ev.button.x - ev.motion.xrel <= bounds->x + transformX + bounds->w &&
                   ev.button.y - ev.motion.yrel >= bounds->y + transformY && ev.button.y - ev.motion.yrel <= bounds->y + transformY + bounds->h);
        // hovered = true;
        // printf("hovering = %s\n", hovering ? "true" : "false");

        // hoverCallback();
        return hovered;
    }
    return false;
}

void UIButton::draw(R_Target* t, int transformX, int transformY) {

    if (!visible) return;

    if (/*Settings::draw_uinode_bounds ||*/ drawBorder) {

        MErect tb{(float)(bounds->x + transformX), (float)(bounds->y + transformY), (float)(bounds->w), (float)(bounds->h)};
        if (disabled) {
            R_RectangleFilled2(t, tb, {0x48, 0x48, 0x48, 0x80});
            R_Rectangle2(t, tb, {0x77, 0x77, 0x77, 0xff});
        } else {
            R_RectangleFilled2(t, tb, hovered ? MEcolor{0xAA, 0xAA, 0xAA, 0x80} : MEcolor{0x66, 0x66, 0x66, 0x80});
            R_Rectangle2(t, tb, hovered ? MEcolor{0xDD, 0xDD, 0xDD, 0xFF} : MEcolor{0x88, 0x88, 0x88, 0xFF});
        }
    }

    if (textureDisabled == NULL && NULL != surfaceDisabled) {
        textureDisabled = R_CopyImageFromSurface(surfaceDisabled);
        R_SetImageFilter(textureDisabled, R_FILTER_NEAREST);
    }

    if (texture == NULL && NULL != surface) {
        texture = R_CopyImageFromSurface(surface);
        R_SetImageFilter(texture, R_FILTER_NEAREST);
    }

    if (surface != NULL) {
        R_Image* tex = disabled ? textureDisabled : texture;

        R_Blit(tex, NULL, t, bounds->x + transformX + 1 - align * surface->w / 2 + bounds->w / 2 + surface->w * 0.5, bounds->y + transformY + 1 + surface->h * 0.5);
    }

    if (!text.empty()) {
    }

    for (auto& c : children) {
        c->parent = this;
        c->draw(t, bounds->x + transformX, bounds->y + transformY);
    }
}

void UICheckbox::draw(R_Target* t, int transformX, int transformY) {

    if (!visible) return;

    MErect tb = {(float)(bounds->x + transformX), (float)(bounds->y + transformY), (float)(bounds->w), (float)(bounds->h)};
    MEcolor col;
    if (checked) {
        col = {0x00, 0xff, 0x00, 0xff};
    } else {
        col = {0xff, 0x00, 0x00, 0xff};
    }
    R_RectangleFilled2(t, tb, col);

    UINode::draw(t, transformX, transformY);
}

bool UICheckbox::onEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY) {

    if (ev.type == SDL_MOUSEBUTTONDOWN) {
        checked = !checked;
        callback(checked);
        return true;
    } else if (ev.type == SDL_MOUSEMOTION) {
        return true;
    }
    return false;
}

void UITextArea::draw(R_Target* t, int transformX, int transformY) {

    if (!visible) return;

    MErect tb = {(float)(bounds->x + transformX), (float)(bounds->y + transformY), (float)(bounds->w), (float)(bounds->h)};
    R_RectangleFilled2(t, tb, focused ? MEcolor{0x80, 0x80, 0x80, 0xA0} : MEcolor{0x60, 0x60, 0x60, 0xA0});
    R_Rectangle2(t, tb, {0xff, 0xff, 0xff, 0xff});

    int cursorX = 0;
    if (cursorIndex > text.size()) cursorIndex = text.size();
    if (cursorIndex < 0) cursorIndex = 0;
    // if (text.size() > 0) {
    //     if (maxLength != -1) {
    //         if (text.size() > maxLength) {
    //             text = text.substr(0, maxLength);
    //             if (cursorIndex > text.size()) cursorIndex = text.size();
    //             if (cursorIndex < 0) cursorIndex = 0;
    //         }
    //     }

    //    if (dirty) {
    //        textParams = Drawing::drawTextParams(t, text.c_str(), font, (int)tb.x + 1, (int)tb.y + 2, 0xff, 0xff, 0xff, ALIGN_LEFT);
    //        dirty = false;
    //    }

    //    Drawing::drawText(t, textParams, (int)tb.x + 1, (int)tb.y + 1, ALIGN_LEFT);

    //    std::string precursor = text.substr(0, cursorIndex);
    //    int w, h;
    //    if (TTF_SizeText(font, precursor.c_str(), &w, &h)) {
    //        logError("TTF_SizeText failed: {}", TTF_GetError());
    //    } else {
    //        cursorX += w;
    //    }
    //} else if (!focused) {

    //    if (dirty) {
    //        textParams = Drawing::drawTextParams(t, placeholder.c_str(), font, (int)tb.x + 1, (int)tb.y + 2, 0xbb, 0xbb, 0xbb, false, ALIGN_LEFT);
    //        dirty = false;
    //    }

    //    Drawing::drawText(t, textParams, (int)tb.x + 1, (int)tb.y + 2, false, ALIGN_LEFT);
    //}

    int font_height = 14;

    if (focused && (ME_gettime() - lastCursorTimer) % 1000 < 500) {
        R_Line(t, tb.x + 3 + cursorX - 1, tb.y + 2 + 2, tb.x + 2 + cursorX - 1, tb.y + 2 + font_height, {0xff, 0xff, 0xff, 0xff});
        R_Line(t, tb.x + 3 + cursorX, tb.y + 2 + 2, tb.x + 2 + cursorX, tb.y + 2 + font_height, {0xaa, 0xaa, 0xaa, 0x80});
    }

    UINode::draw(t, transformX, transformY);
}

bool UITextArea::checkEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY) {

    if (!visible) return false;

    bool ok = false;

    if (ev.type == SDL_MOUSEBUTTONDOWN) {
        return onEvent(ev, t, world, transformX, transformY);
    }

    return UINode::checkEvent(ev, t, world, transformX, transformY);
}

bool UITextArea::onEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY) {

    if (ev.type == SDL_MOUSEBUTTONDOWN) {

        bool hovered = (ev.motion.x >= bounds->x + transformX && ev.button.x <= bounds->x + transformX + bounds->w && ev.button.y >= bounds->y + transformY &&
                        ev.button.y <= bounds->y + transformY + bounds->h) ||
                       (ev.motion.x - ev.motion.xrel >= bounds->x + transformX && ev.button.x - ev.motion.xrel <= bounds->x + transformX + bounds->w &&
                        ev.button.y - ev.motion.yrel >= bounds->y + transformY && ev.button.y - ev.motion.yrel <= bounds->y + transformY + bounds->h);

        bool wasFocused = focused;
        focused = hovered;

        if (focused) {
            if (text.size() > 0) {
                bool found = false;
                int accW = 0;
                for (int i = 0; i < text.size(); i++) {
                    // int w, h;
                    //  if (TTF_SizeText(font, std::string(1, text.at(i)).c_str(), &w, &h)) {
                    //      METADOT_ERROR("Font_SizeText failed");
                    //  } else {
                    //      accW += w;
                    //      if (ev.button.x + 3 < accW + (bounds->x + transformX) + 2) {
                    //          cursorIndex = i;
                    //          found = true;
                    //          break;
                    //      }
                    //  }
                }
                if (!found) {
                    cursorIndex = text.size();
                }
            } else {
                cursorIndex = 0;
            }
            lastCursorTimer = ME_gettime();
            callback(text);
        }

        if (focused && !wasFocused) {
            SDL_StartTextInput();
        } else if (!focused && wasFocused) {
            SDL_StopTextInput();
        }

        return focused;
    } else if (ev.type == SDL_MOUSEMOTION) {
        return true;
    } else if (ev.type == SDL_KEYDOWN && focused) {
        SDL_Keycode ch = ev.key.keysym.sym;
        if (ch == SDLK_BACKSPACE && text.size() > 0 && cursorIndex > 0) {
            text = text.erase(cursorIndex - 1, 1);
            if (cursorIndex > 0) cursorIndex--;
            lastCursorTimer = ME_gettime();
            dirty = true;
            callback(text);
        } else if (ch == SDLK_DELETE && text.size() > 0 && cursorIndex < text.size()) {
            text = text.erase(cursorIndex, 1);
            lastCursorTimer = ME_gettime();
            dirty = true;
            callback(text);
        } else if (ch == SDLK_HOME) {
            cursorIndex = 0;
            lastCursorTimer = ME_gettime();
        } else if (ch == SDLK_END) {
            cursorIndex = text.size();
            lastCursorTimer = ME_gettime();
        } else if (ch == SDLK_LEFT) {
            if ((ev.key.keysym.mod & SDL_Keymod::KMOD_LCTRL) || (ev.key.keysym.mod & SDL_Keymod::KMOD_RCTRL)) {
                bool foundAlpha = false;
                for (int i = cursorIndex; i > 0; i--) {
                    char ch = text.at(i - 1);
                    if (!isalnum(ch)) {
                        if (foundAlpha) {
                            cursorIndex++;
                            break;
                        }
                    } else {
                        foundAlpha = true;
                    }
                    cursorIndex = i - 1;
                }
                if (cursorIndex > 0) cursorIndex--;
            } else {
                if (cursorIndex > 0) cursorIndex--;
            }
            lastCursorTimer = ME_gettime();
        } else if (ch == SDLK_RIGHT) {
            if ((ev.key.keysym.mod & SDL_Keymod::KMOD_LCTRL) || (ev.key.keysym.mod & SDL_Keymod::KMOD_RCTRL)) {
                bool foundNonAlpha = false;
                for (int i = cursorIndex; i < text.size(); i++) {
                    char ch = text.at(i);
                    if (!isalnum(ch)) {
                        foundNonAlpha = true;
                    } else {
                        if (foundNonAlpha) {

                            break;
                        }
                    }
                    cursorIndex = i;
                }
                if (cursorIndex < text.size()) cursorIndex++;
            } else {
                if (cursorIndex < text.size()) cursorIndex++;
            }
            lastCursorTimer = ME_gettime();
        }
        return true;
    } else if (ev.type == SDL_TEXTINPUT && focused) {
        text = text.insert(cursorIndex, ev.text.text, strlen(ev.text.text));
        dirty = true;
        // text += ev.text.text;
        if (cursorIndex < text.size()) cursorIndex++;
        callback(text);
        return true;
    } else if (ev.type == SDL_TEXTEDITING && focused) {
        auto composition = ev.edit.text;
        auto cursor = ev.edit.start;
        auto selection_len = ev.edit.length;
        METADOT_BUG("UI_TEXTEDITING");
        return true;
    }
    return false;
}

void ChiselNode::draw(R_Target* t, int transformX, int transformY) {

    int scale = 4;

    MErect dst = {(float)(bounds->x + transformX), (float)(bounds->y + transformY), (float)(bounds->w), (float)(bounds->h)};
    R_BlitRect(texture, NULL, t, &dst);

    for (int x = 1; x < surface->w; x++) {
        R_Line(t, dst.x + x * scale, dst.y, dst.x + x * scale, dst.y + dst.h, {0, 0, 0, 0x80});
    }

    for (int y = 1; y < surface->h; y++) {
        R_Line(t, dst.x, dst.y + y * scale, dst.x + dst.w, dst.y + y * scale, {0, 0, 0, 0x80});
    }

    UINode::draw(t, transformX, transformY);
}

bool ChiselNode::onEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY) {

    if ((ev.type == SDL_MOUSEMOTION && ev.motion.state & SDL_BUTTON_LMASK) || (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button & SDL_BUTTON_LMASK)) {
        int lx = ev.motion.x - transformX;
        int ly = ev.motion.y - transformY;

        int px = lx / 4;
        int py = ly / 4;

        ME_get_pixel(surface, px, py) = 0x00000000;

        R_FreeImage(texture);
        texture = R_CopyImageFromSurface(surface);
        R_SetImageFilter(texture, R_FILTER_NEAREST);
        return true;
    } else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {

        // SDL_FreeSurface(rb->surface);
        // R_FreeImage(rb->texture);

        // rb->surface = surface;
        // rb->texture = texture;

        auto tex = create_ref<Texture>(surface, false);
        rb->setTexture(tex);

        world->updateRigidBodyHitbox(rb);
        parent->visible = false;
        return true;
    }
    return false;
}

bool ChiselNode::checkEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY) {

    if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {
        UINode::checkEvent(ev, t, world, transformX, transformY);
        return true;
    }
    return UINode::checkEvent(ev, t, world, transformX, transformY);
}

ChiselNode::~ChiselNode() noexcept {
    if (this->texture) R_FreeImage(texture);
    if (this->surface) SDL_FreeSurface(surface);
}

void MaterialNode::draw(R_Target* t, int transformX, int transformY) {

    if (texture == NULL) {
        texture = R_CopyImageFromSurface(surface);
        R_SetImageFilter(texture, R_FILTER_NEAREST);
    }

    R_BlitScale(texture, NULL, t, bounds->x + transformX + bounds->w / 2, bounds->y + transformY + bounds->h / 2, bounds->w / texture->w, bounds->h / texture->h);

    UINode::draw(t, transformX, transformY);
}

bool MaterialNode::onEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY) {

    if (ev.type == SDL_MOUSEBUTTONDOWN) {
        selectCallback(mat);
        return true;
    } else if (ev.type == SDL_MOUSEMOTION) {
        hoverCallback(mat);
        return true;
    }
    return false;
}

}  // namespace ME