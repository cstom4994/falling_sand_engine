// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_CONTROLS_HPP_
#define _METADOT_CONTROLS_HPP_

#include <cstdio>
#include <vector>

#include "core/cpp/vector.hpp"
#include "engine/sdl_wrapper.h"

enum EnumControlMode { MOMENTARY, RISING, FALLING, TOGGLE, TYPE };

class Control {
public:
    virtual bool get() = 0;
};

class KeyControl : public Control {
public:
    C_Keycode key;
    EnumControlMode mode;

    bool raw;
    bool lastRaw;
    bool lastState;

    KeyControl(C_Keycode key, EnumControlMode mode) {
        this->key = key;
        this->mode = mode;

        raw = false;
        lastRaw = false;
        lastState = false;
    }

    virtual bool get();
};

class Controls {
public:
    static bool lmouse;
    static bool mmouse;
    static bool rmouse;

    static Control *STATS_DISPLAY;
    static Control *STATS_DISPLAY_DETAILED;
    static Control *DEBUG_UI;
    static Control *DEBUG_REFRESH;
    static Control *DEBUG_UPDATE_WORLD_MESH;
    static Control *DEBUG_TICK;
    static Control *DEBUG_EXPLODE;
    static Control *DEBUG_CARVE;
    static Control *DEBUG_RIGID;
    static Control *DEBUG_DRAW;
    static Control *DEBUG_BRUSHSIZE_INC;
    static Control *DEBUG_BRUSHSIZE_DEC;
    static Control *DEBUG_TOGGLE_PLAYER;
    static Control *PLAYER_UP;
    static Control *PLAYER_DOWN;
    static Control *PLAYER_LEFT;
    static Control *PLAYER_RIGHT;
    static Control *ZOOM_IN;
    static Control *ZOOM_OUT;
    static Control *PAUSE;

    static MetaEngine::vector<std::shared_ptr<KeyControl>> keyControls;

    static bool initted;

    // Update key state
    static void KeyEvent(C_KeyboardEvent event);
    // Register the key binding
    static void InitKey();
    // Add key binding
    static KeyControl *Add(std::shared_ptr<KeyControl> c);
};

#endif
