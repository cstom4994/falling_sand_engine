// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_CONTROLS_HPP
#define ME_CONTROLS_HPP

#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "engine/core/core.hpp"
#include "engine/core/sdl_wrapper.h"

namespace ME {

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
    static bool lmouse_down;
    static bool mmouse_down;
    static bool rmouse_down;

    static bool lmouse_up;
    static bool mmouse_up;
    static bool rmouse_up;

    static int mouse_x;
    static int mouse_y;

    static KeyControl *STATS_DISPLAY;
    static KeyControl *STATS_DISPLAY_DETAILED;
    static KeyControl *DEBUG_UI;
    static KeyControl *CONSOLE_UI;
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

    static std::vector<ref<KeyControl>> keyControls;
    static std::map<std::string, int> SDLKeymap;

    static bool initted;

    // Update key state
    static void KeyEvent(C_KeyboardEvent event);
    // Register the key binding
    static void InitKey();
    // Add key binding
    static KeyControl *Add(ref<KeyControl> c);

    static void InitKeymap();
    static std::string SDLKeyToString(int sdlkey);
    static int StringToSDLKey(const std::string &s);
};

}  // namespace ME

#endif
