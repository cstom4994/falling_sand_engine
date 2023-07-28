// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_UI_HPP
#define ME_UI_HPP

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "engine/core/core.hpp"
#include "engine/game_basic.hpp"
#include "engine/meta/reflection.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/renderer/renderer_opengl.h"
#include "engine/textures.hpp"
#include "engine/ui/dbgui.hpp"
#include "engine/ui/font.hpp"
#include "engine/ui/imgui_impl.hpp"
#include "game/player.hpp"

namespace ME {

class UIBackground {
public:
    virtual void draw(R_Target* t, MErect* bounds) = 0;
};

class SolidBackground : public UIBackground {
public:
    u32 color;
    void draw(R_Target* t, MErect* bounds);

    SolidBackground(u32 col) { this->color = col; }
};

class UINode {
public:
    bool visible = true;

    MErect* bounds;
    bool drawBorder = false;

    UINode* parent = NULL;
    std::vector<UINode*> children = {};

    virtual void draw(R_Target* t, int transformX, int transformY);

    virtual bool checkEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY);

    virtual bool onEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY) { return false; };

    UINode(MErect* bounds) { this->bounds = bounds; }
};

class UI : public UINode {
public:
    UIBackground* background = nullptr;
    void draw(R_Target* t, int transformX, int transformY);

    bool onEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY);

    UI(MErect* bounds) : UINode(bounds){};
};

class UILabel : public UINode {
public:
    std::string text;
    font_index font;
    u32 textColor;
    int align;

    void draw(R_Target* t, int transformX, int transformY);

    UILabel(MErect* bounds, std::string text, font_index font, u32 textColor, int align) : UINode(bounds) {
        this->text = text;
        this->font = font;
        this->textColor = textColor;
        this->align = align;

        // surface = TTF_RenderText_Solid(font, text.c_str(), {(textColor >> 16) & 0xff, (textColor >> 8) & 0xff, (textColor >> 0) & 0xff});
    }
};

class UIButton : public UILabel {
public:
    bool hovered = false;
    bool disabled = false;
    std::function<void()> selectCallback = []() {};
    std::function<void()> hoverCallback = []() {};

    bool checkEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY);
    bool onEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY);
    void draw(R_Target* t, int transformX, int transformY);

    C_Surface* surface = NULL;
    R_Image* texture = NULL;

    C_Surface* surfaceDisabled = NULL;
    R_Image* textureDisabled = NULL;

    void updateTexture() {
        // surfaceDisabled = TTF_RenderText_Solid(font, text.c_str(), {(textColor >> 16) & 0xff, (textColor >> 8) & 0xff, (textColor >> 0) & 0xff, 0x80});
        textureDisabled = NULL;
    }

    UIButton(MErect* bounds, std::string text, font_index font, u32 textColor, int align)
        : UILabel(bounds, text, font, textColor, align){
                  // surfaceDisabled = TTF_RenderText_Solid(font, text.c_str(), {(textColor >> 16) & 0xff, (textColor >> 8) & 0xff, (textColor >> 0) & 0xff, 0x80});
          };
};

class UICheckbox : public UINode {
public:
    bool checked = false;
    std::function<void(bool)> callback = [](bool checked) {};

    void draw(R_Target* t, int transformX, int transformY);

    bool onEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY);

    UICheckbox(MErect* bounds, bool checked) : UINode(bounds) { this->checked = checked; }
};

class UITextArea : public UINode {
public:
    font_index font;
    std::string text;
    std::string placeholder;
    int maxLength = -1;
    std::function<void(std::string)> callback = [](std::string checked) {};
    std::function<bool(char)> isValidChar = [](char ch) { return true; };
    bool focused = false;
    bool dirty = true;
    int cursorIndex = 0;
    long long lastCursorTimer = 0;

    void draw(R_Target* t, int transformX, int transformY);

    bool checkEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY);
    bool onEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY);

    UITextArea(MErect* bounds, std::string defaultText, std::string placeholder, font_index font) : UINode(bounds) {
        this->text = defaultText;
        this->placeholder = placeholder;
        this->font = font;
    }
};

class ChiselNode : public UINode {
public:
    RigidBody* rb = nullptr;
    C_Surface* surface = nullptr;
    R_Image* texture = nullptr;

    void draw(R_Target* t, int transformX, int transformY);

    bool onEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY);

    bool checkEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY);

    ChiselNode(MErect* bounds) : UINode(bounds){};
    ~ChiselNode() noexcept;
};

class MaterialNode : public UINode {
public:
    Material* mat;
    C_Surface* surface;
    R_Image* texture = NULL;
    std::function<void(Material*)> selectCallback = [](Material* mat) {};
    std::function<void(Material*)> hoverCallback = [](Material* mat) {};

    void draw(R_Target* t, int transformX, int transformY);

    bool onEvent(C_Event ev, R_Target* t, world* world, int transformX, int transformY);

    MaterialNode(MErect* bounds, Material* mat) : UINode(bounds) {
        this->mat = mat;
        surface = SDL_CreateRGBSurfaceWithFormat(0, bounds->w, bounds->h, 32, SDL_PIXELFORMAT_ARGB8888);
        for (int x = 0; x < bounds->w; x++) {
            for (int y = 0; y < bounds->h; y++) {
                MaterialInstance m = TilesCreate(mat->id, x, y);
                ME_get_pixel(surface, x, y) = m.color + (m.mat->alpha << 24);
            }
        }
    }
};

class gui : public module<gui> {

public:
    ref<dbgui> imgui;

    std::vector<UI*> uis;
    UI* debugUI = nullptr;
    UI* chiselUI = nullptr;

public:
    gui();
    ~gui() noexcept;

    void init() override;
    void end() override;
    void registerLua(lua_wrapper::State* p_lua) override;

    void render_postupdate();
    void render_update();
    void render();
    void render_imgui();

    bool push_input(C_KeyboardEvent event);
    bool push_event(C_Event event);

    void DrawPoint(MEvec3 pos, float size, Texture* texture, u8 r, u8 g, u8 b);
    void DrawLine(MEvec3 min, MEvec3 max, float thickness, u8 r, u8 g, u8 b);

    void chisel_ui(RigidBody* cur);
};

}  // namespace ME

#endif