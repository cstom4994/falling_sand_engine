// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_CONTROLS_HPP_
#define _METADOT_CONTROLS_HPP_

#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "core/cpp/vector.hpp"
#include "core/core.hpp"
#include "sdl_wrapper.h"

enum EnumControlMode { MOMENTARY, RISING, FALLING, TOGGLE, TYPE };

class KeyControl {
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

    bool get();
};

class ControlSystem {
public:
    static bool lmouse;
    static bool mmouse;
    static bool rmouse;

    static KeyControl *STATS_DISPLAY;
    static KeyControl *STATS_DISPLAY_DETAILED;
    static KeyControl *DEBUG_UI;
    static KeyControl *DEBUG_REFRESH;
    static KeyControl *DEBUG_UPDATE_WORLD_MESH;
    static KeyControl *DEBUG_TICK;
    static KeyControl *DEBUG_EXPLODE;
    static KeyControl *DEBUG_CARVE;
    static KeyControl *DEBUG_RIGID;
    static KeyControl *DEBUG_DRAW;
    static KeyControl *DEBUG_BRUSHSIZE_INC;
    static KeyControl *DEBUG_BRUSHSIZE_DEC;
    static KeyControl *DEBUG_TOGGLE_PLAYER;
    static KeyControl *PLAYER_UP;
    static KeyControl *PLAYER_DOWN;
    static KeyControl *PLAYER_LEFT;
    static KeyControl *PLAYER_RIGHT;
    static KeyControl *ZOOM_IN;
    static KeyControl *ZOOM_OUT;
    static KeyControl *PAUSE;

    static MetaEngine::vector<MetaEngine::Ref<KeyControl>> keyControls;
    static std::map<std::string, int> SDLKeymap;

    static bool initted;

    // Update key state
    static void KeyEvent(C_KeyboardEvent event);
    // Register the key binding
    static void InitKey();
    // Add key binding
    static KeyControl *Add(MetaEngine::Ref<KeyControl> c);

    static void InitKeymap();
    static std::string SDLKeyToString(int sdlkey);
    static int StringToSDLKey(const std::string &s);
};

#endif
