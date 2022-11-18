// Copyright(c) 2022, KaoruXun All rights reserved.


#include "Game/InEngine.h"

#include "Controls.hpp"

std::vector<KeyControl *> Controls::keyControls = {};
//KeyControl** Controls::keyControls = new KeyControl*[NUM_CONTROLS];
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

void Controls::keyEvent(SDL_KeyboardEvent event) {
    //if (keyControls[event.keysym.sym]) {}

    //METADOT_BUG("SDL_KEYEV {0:c} {0:d} {0:d}", event.keysym.sym, event.repeat, event.keysym.sym);

    for (auto &v: keyControls) {
        if (v->key == event.keysym.sym) {
            //METADOT_BUG("match KEY {0:d}", event.keysym.sym);

            bool newState = false;
            switch (event.type) {
                case SDL_KEYDOWN:
                    //METADOT_BUG("SDL_KEYDOWN {0:c} {0:d} {0:d}", event.keysym.sym, event.repeat, event.keysym.sym);
                    newState = true;
                    break;
                case SDL_KEYUP:
                    //METADOT_BUG("SDL_KEYUP {0:c}", event.keysym.sym);
                    newState = false;
                    break;
            }

            if (event.repeat == 0 || v->mode == TYPE) {
                //METADOT_BUG("write raw");
                v->raw = newState;
            }
        }
    }
}

void Controls::initKey() {
    STATS_DISPLAY = add(new KeyControl(SDLK_F3, RISING));
    STATS_DISPLAY_DETAILED = add(new KeyControl(SDLK_LSHIFT, MOMENTARY));

    DEBUG_UI = add(new KeyControl(SDLK_F4, RISING));
    DEBUG_REFRESH = add(new KeyControl(SDLK_KP_0, RISING));
    DEBUG_UPDATE_WORLD_MESH = add(new KeyControl(SDLK_KP_1, RISING));
    DEBUG_TICK = add(new KeyControl(SDLK_KP_2, RISING));
    DEBUG_EXPLODE = add(new KeyControl(SDLK_e, RISING));
    DEBUG_CARVE = add(new KeyControl(SDLK_c, RISING));
    DEBUG_RIGID = add(new KeyControl(SDLK_r, RISING));

    DEBUG_DRAW = new MultiControl(ControlCombine::AND, {add(new KeyControl(SDLK_x, MOMENTARY)), add(new KeyControl(SDLK_LCTRL, MOMENTARY))});
    DEBUG_BRUSHSIZE_INC = add(new KeyControl(']', TYPE));
    DEBUG_BRUSHSIZE_DEC = add(new KeyControl('[', TYPE));

    DEBUG_TOGGLE_PLAYER = add(new KeyControl(SDLK_p, RISING));

    PLAYER_UP = new MultiControl(ControlCombine::OR, {add(new KeyControl(SDLK_w, MOMENTARY)), add(new KeyControl(SDLK_SPACE, MOMENTARY))});
    PLAYER_LEFT = add(new KeyControl(SDLK_a, MOMENTARY));
    PLAYER_DOWN = add(new KeyControl(SDLK_s, MOMENTARY));
    PLAYER_RIGHT = add(new KeyControl(SDLK_d, MOMENTARY));

    ZOOM_IN = add(new KeyControl(SDLK_PAGEUP, MOMENTARY));
    ZOOM_OUT = add(new KeyControl(SDLK_PAGEDOWN, MOMENTARY));

    PAUSE = add(new KeyControl(SDLK_ESCAPE, RISING));
}

KeyControl *Controls::add(KeyControl *c) {

    if (!initted) {
        /*for (int i = 0; i < NUM_CONTROLS; i++) {
            keyControls[i] = NULL;
        }*/
        keyControls = {};
        initted = true;
    }

    //return keyControls[c->key] = c;
    keyControls.push_back(c);
    return c;
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

bool MultiControl::get() {
    if (this->combine == ControlCombine::OR) {
        for (auto &v: this->controls) {
            if (v->get()) return true;
        }
        return false;
    } else if (this->combine == ControlCombine::AND) {
        for (auto &v: this->controls) {
            if (!v->get()) return false;
        }
        return true;
    }

    return false;
}
