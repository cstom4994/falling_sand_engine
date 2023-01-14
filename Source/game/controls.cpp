// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "controls.hpp"

#include <memory>

#include "engine/sdl_wrapper.h"

MetaEngine::vector<std::shared_ptr<KeyControl>> Controls::keyControls = {};
bool Controls::initted = false;

Control *Controls::STATS_DISPLAY = nullptr;
Control *Controls::STATS_DISPLAY_DETAILED = nullptr;
Control *Controls::DEBUG_UI = nullptr;
Control *Controls::DEBUG_REFRESH = nullptr;
Control *Controls::DEBUG_UPDATE_WORLD_MESH = nullptr;
Control *Controls::DEBUG_TICK = nullptr;
Control *Controls::DEBUG_EXPLODE = nullptr;
Control *Controls::DEBUG_CARVE = nullptr;
Control *Controls::DEBUG_RIGID = nullptr;
Control *Controls::DEBUG_DRAW = nullptr;
Control *Controls::DEBUG_BRUSHSIZE_INC = nullptr;
Control *Controls::DEBUG_BRUSHSIZE_DEC = nullptr;
Control *Controls::DEBUG_TOGGLE_PLAYER = nullptr;
Control *Controls::PLAYER_UP = nullptr;
Control *Controls::PLAYER_LEFT = nullptr;
Control *Controls::PLAYER_DOWN = nullptr;
Control *Controls::PLAYER_RIGHT = nullptr;
Control *Controls::ZOOM_IN = nullptr;
Control *Controls::ZOOM_OUT = nullptr;
Control *Controls::PAUSE = nullptr;

bool Controls::lmouse = false;
bool Controls::mmouse = false;
bool Controls::rmouse = false;

void Controls::KeyEvent(C_KeyboardEvent event) {
    for (auto &v : keyControls) {
        if (v->key == event.keysym.sym) {

            bool newState = false;
            switch (event.type) {
                case SDL_KEYDOWN:
                    newState = true;
                    break;
                case SDL_KEYUP:
                    newState = false;
                    break;
            }

            if (event.repeat == 0 || v->mode == TYPE) {
                v->raw = newState;
            }
        }
    }
}

void Controls::InitKey() {
    STATS_DISPLAY = Add(std::make_shared<KeyControl>(SDLK_o, RISING));
    STATS_DISPLAY_DETAILED = Add(std::make_shared<KeyControl>(SDLK_LSHIFT, MOMENTARY));
    DEBUG_UI = Add(std::make_shared<KeyControl>(SDLK_TAB, RISING));
    DEBUG_REFRESH = Add(std::make_shared<KeyControl>(SDLK_KP_0, RISING));
    DEBUG_UPDATE_WORLD_MESH = Add(std::make_shared<KeyControl>(SDLK_KP_1, RISING));
    DEBUG_TICK = Add(std::make_shared<KeyControl>(SDLK_KP_2, RISING));
    DEBUG_EXPLODE = Add(std::make_shared<KeyControl>(SDLK_e, RISING));
    DEBUG_CARVE = Add(std::make_shared<KeyControl>(SDLK_c, RISING));
    DEBUG_RIGID = Add(std::make_shared<KeyControl>(SDLK_r, RISING));
    DEBUG_DRAW = Add(std::make_shared<KeyControl>(SDLK_x, MOMENTARY));
    DEBUG_BRUSHSIZE_INC = Add(std::make_shared<KeyControl>(']', TYPE));
    DEBUG_BRUSHSIZE_DEC = Add(std::make_shared<KeyControl>('[', TYPE));
    DEBUG_TOGGLE_PLAYER = Add(std::make_shared<KeyControl>(SDLK_p, RISING));
    PLAYER_UP = Add(std::make_shared<KeyControl>(SDLK_w, MOMENTARY));
    PLAYER_LEFT = Add(std::make_shared<KeyControl>(SDLK_a, MOMENTARY));
    PLAYER_DOWN = Add(std::make_shared<KeyControl>(SDLK_s, MOMENTARY));
    PLAYER_RIGHT = Add(std::make_shared<KeyControl>(SDLK_d, MOMENTARY));
    ZOOM_IN = Add(std::make_shared<KeyControl>(SDLK_PAGEUP, MOMENTARY));
    ZOOM_OUT = Add(std::make_shared<KeyControl>(SDLK_PAGEDOWN, MOMENTARY));
    PAUSE = Add(std::make_shared<KeyControl>(SDLK_ESCAPE, RISING));
}

KeyControl *Controls::Add(std::shared_ptr<KeyControl> c) {

    if (!initted) {
        keyControls = {};
        initted = true;
    }

    keyControls.push_back(c);
    return c.get();
}

bool KeyControl::get() {

    bool ret = false;
    switch (mode) {
        case MOMENTARY:
            ret = raw;
            break;
        case RISING:
            ret = raw && !lastRaw;
            break;
        case FALLING:
            ret = !raw && lastRaw;
            break;
        case TOGGLE:
            if (raw && !lastRaw) lastState ^= true;
            ret = lastState;
            break;
        case TYPE:
            ret = raw;
            raw = false;
            break;
    }

    lastRaw = raw;
    return ret;
}