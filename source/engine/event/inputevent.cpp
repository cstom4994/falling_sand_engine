// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "inputevent.hpp"

#include <map>
#include <memory>
#include <string>

#include "engine/core/global.hpp"
#include "engine/core/sdl_wrapper.h"
#include "engine/game.hpp"
#include "engine/utils/stl.hpp"

namespace ME {

std::vector<ref<KeyControl>> input::keyControls = {};
std::map<std::string, int> input::SDLKeymap = {};
bool input::initted = false;

KeyControl *input::STATS_DISPLAY = nullptr;
KeyControl *input::STATS_DISPLAY_DETAILED = nullptr;
KeyControl *input::DEBUG_UI = nullptr;
KeyControl *input::CONSOLE_UI = nullptr;
KeyControl *input::DEBUG_REFRESH = nullptr;
KeyControl *input::DEBUG_UPDATE_WORLD_MESH = nullptr;
KeyControl *input::DEBUG_TICK = nullptr;
KeyControl *input::DEBUG_EXPLODE = nullptr;
KeyControl *input::DEBUG_CARVE = nullptr;
KeyControl *input::DEBUG_RIGID = nullptr;
KeyControl *input::DEBUG_DRAW = nullptr;
KeyControl *input::DEBUG_BRUSHSIZE_INC = nullptr;
KeyControl *input::DEBUG_BRUSHSIZE_DEC = nullptr;
KeyControl *input::DEBUG_TOGGLE_PLAYER = nullptr;
KeyControl *input::PLAYER_UP = nullptr;
KeyControl *input::PLAYER_LEFT = nullptr;
KeyControl *input::PLAYER_DOWN = nullptr;
KeyControl *input::PLAYER_RIGHT = nullptr;
KeyControl *input::ZOOM_IN = nullptr;
KeyControl *input::ZOOM_OUT = nullptr;
KeyControl *input::PAUSE = nullptr;

bool input::lmouse_down = false;
bool input::mmouse_down = false;
bool input::rmouse_down = false;

bool input::lmouse_up = true;
bool input::mmouse_up = true;
bool input::rmouse_up = true;

int input::mouse_x = 0;
int input::mouse_y = 0;

void input::KeyEvent(C_KeyboardEvent event) {

    if (the<gui>().push_input(event)) return;

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

void input::InitKey() {
    STATS_DISPLAY = Add(create_ref<KeyControl>(SDLK_o, RISING));
    STATS_DISPLAY_DETAILED = Add(create_ref<KeyControl>(SDLK_LSHIFT, MOMENTARY));
    DEBUG_UI = Add(create_ref<KeyControl>(SDLK_TAB, RISING));
    CONSOLE_UI = Add(create_ref<KeyControl>(SDLK_BACKQUOTE, RISING));
    DEBUG_REFRESH = Add(create_ref<KeyControl>(SDLK_KP_0, RISING));
    DEBUG_UPDATE_WORLD_MESH = Add(create_ref<KeyControl>(SDLK_KP_1, RISING));
    DEBUG_TICK = Add(create_ref<KeyControl>(SDLK_KP_2, RISING));
    DEBUG_EXPLODE = Add(create_ref<KeyControl>(SDLK_e, RISING));
    DEBUG_CARVE = Add(create_ref<KeyControl>(SDLK_c, RISING));
    DEBUG_RIGID = Add(create_ref<KeyControl>(SDLK_r, RISING));
    DEBUG_DRAW = Add(create_ref<KeyControl>(SDLK_x, MOMENTARY));
    DEBUG_BRUSHSIZE_INC = Add(create_ref<KeyControl>(']', TYPE));
    DEBUG_BRUSHSIZE_DEC = Add(create_ref<KeyControl>('[', TYPE));
    DEBUG_TOGGLE_PLAYER = Add(create_ref<KeyControl>(SDLK_p, RISING));
    PLAYER_UP = Add(create_ref<KeyControl>(SDLK_w, MOMENTARY));
    PLAYER_LEFT = Add(create_ref<KeyControl>(SDLK_a, MOMENTARY));
    PLAYER_DOWN = Add(create_ref<KeyControl>(SDLK_s, MOMENTARY));
    PLAYER_RIGHT = Add(create_ref<KeyControl>(SDLK_d, MOMENTARY));
    ZOOM_IN = Add(create_ref<KeyControl>(SDLK_PAGEUP, MOMENTARY));
    ZOOM_OUT = Add(create_ref<KeyControl>(SDLK_PAGEDOWN, MOMENTARY));
    PAUSE = Add(create_ref<KeyControl>(SDLK_ESCAPE, RISING));
}

KeyControl *input::Add(ref<KeyControl> c) {

    if (!initted) {
        keyControls = {};
        initted = true;
    }

    keyControls.push_back(c);
    return c.get();
}

void input::InitKeymap() {
    if (SDLKeymap.size() == 0) {
        SDLKeymap.insert(std::pair<std::string, int>("backspace", 8));
        SDLKeymap.insert(std::pair<std::string, int>("tab", 9));
        SDLKeymap.insert(std::pair<std::string, int>("clear", 12));
        SDLKeymap.insert(std::pair<std::string, int>("return", 13));
        SDLKeymap.insert(std::pair<std::string, int>("pause", 19));
        SDLKeymap.insert(std::pair<std::string, int>("escape", 27));
        SDLKeymap.insert(std::pair<std::string, int>("space", 32));
        SDLKeymap.insert(std::pair<std::string, int>("!", 33));
        SDLKeymap.insert(std::pair<std::string, int>("\"", 34));
        SDLKeymap.insert(std::pair<std::string, int>("#", 35));
        SDLKeymap.insert(std::pair<std::string, int>("$", 36));
        SDLKeymap.insert(std::pair<std::string, int>("&", 38));
        SDLKeymap.insert(std::pair<std::string, int>("'", 39));
        SDLKeymap.insert(std::pair<std::string, int>("(", 40));
        SDLKeymap.insert(std::pair<std::string, int>(")", 41));
        SDLKeymap.insert(std::pair<std::string, int>("*", 42));
        SDLKeymap.insert(std::pair<std::string, int>("+", 43));
        SDLKeymap.insert(std::pair<std::string, int>(",", 44));
        SDLKeymap.insert(std::pair<std::string, int>("-", 45));
        SDLKeymap.insert(std::pair<std::string, int>(".", 46));
        SDLKeymap.insert(std::pair<std::string, int>("/", 47));
        SDLKeymap.insert(std::pair<std::string, int>("0", 48));
        SDLKeymap.insert(std::pair<std::string, int>("1", 49));
        SDLKeymap.insert(std::pair<std::string, int>("2", 50));
        SDLKeymap.insert(std::pair<std::string, int>("3", 51));
        SDLKeymap.insert(std::pair<std::string, int>("4", 52));
        SDLKeymap.insert(std::pair<std::string, int>("5", 53));
        SDLKeymap.insert(std::pair<std::string, int>("6", 54));
        SDLKeymap.insert(std::pair<std::string, int>("7", 55));
        SDLKeymap.insert(std::pair<std::string, int>("8", 56));
        SDLKeymap.insert(std::pair<std::string, int>("9", 57));
        SDLKeymap.insert(std::pair<std::string, int>(":", 58));
        SDLKeymap.insert(std::pair<std::string, int>(";", 59));
        SDLKeymap.insert(std::pair<std::string, int>("<", 60));
        SDLKeymap.insert(std::pair<std::string, int>("=", 61));
        SDLKeymap.insert(std::pair<std::string, int>(">", 62));
        SDLKeymap.insert(std::pair<std::string, int>("?", 63));
        SDLKeymap.insert(std::pair<std::string, int>("@", 64));
        SDLKeymap.insert(std::pair<std::string, int>("[", 91));
        SDLKeymap.insert(std::pair<std::string, int>("\\", 92));
        SDLKeymap.insert(std::pair<std::string, int>("]", 93));
        SDLKeymap.insert(std::pair<std::string, int>("^", 94));
        SDLKeymap.insert(std::pair<std::string, int>("_", 95));
        SDLKeymap.insert(std::pair<std::string, int>("backquote", 96));
        SDLKeymap.insert(std::pair<std::string, int>("a", 97));
        SDLKeymap.insert(std::pair<std::string, int>("b", 98));
        SDLKeymap.insert(std::pair<std::string, int>("c", 99));
        SDLKeymap.insert(std::pair<std::string, int>("d", 100));
        SDLKeymap.insert(std::pair<std::string, int>("e", 101));
        SDLKeymap.insert(std::pair<std::string, int>("f", 102));
        SDLKeymap.insert(std::pair<std::string, int>("g", 103));
        SDLKeymap.insert(std::pair<std::string, int>("h", 104));
        SDLKeymap.insert(std::pair<std::string, int>("i", 105));
        SDLKeymap.insert(std::pair<std::string, int>("j", 106));
        SDLKeymap.insert(std::pair<std::string, int>("k", 107));
        SDLKeymap.insert(std::pair<std::string, int>("l", 108));
        SDLKeymap.insert(std::pair<std::string, int>("m", 109));
        SDLKeymap.insert(std::pair<std::string, int>("n", 110));
        SDLKeymap.insert(std::pair<std::string, int>("o", 111));
        SDLKeymap.insert(std::pair<std::string, int>("p", 112));
        SDLKeymap.insert(std::pair<std::string, int>("q", 113));
        SDLKeymap.insert(std::pair<std::string, int>("r", 114));
        SDLKeymap.insert(std::pair<std::string, int>("s", 115));
        SDLKeymap.insert(std::pair<std::string, int>("t", 116));
        SDLKeymap.insert(std::pair<std::string, int>("u", 117));
        SDLKeymap.insert(std::pair<std::string, int>("v", 118));
        SDLKeymap.insert(std::pair<std::string, int>("w", 119));
        SDLKeymap.insert(std::pair<std::string, int>("x", 120));
        SDLKeymap.insert(std::pair<std::string, int>("y", 121));
        SDLKeymap.insert(std::pair<std::string, int>("z", 122));
        SDLKeymap.insert(std::pair<std::string, int>("delete", 127));
        SDLKeymap.insert(std::pair<std::string, int>("world_0", 160));
        SDLKeymap.insert(std::pair<std::string, int>("world_1", 161));
        SDLKeymap.insert(std::pair<std::string, int>("world_2", 162));
        SDLKeymap.insert(std::pair<std::string, int>("world_3", 163));
        SDLKeymap.insert(std::pair<std::string, int>("world_4", 164));
        SDLKeymap.insert(std::pair<std::string, int>("world_5", 165));
        SDLKeymap.insert(std::pair<std::string, int>("world_6", 166));
        SDLKeymap.insert(std::pair<std::string, int>("world_7", 167));
        SDLKeymap.insert(std::pair<std::string, int>("world_8", 168));
        SDLKeymap.insert(std::pair<std::string, int>("world_9", 169));
        SDLKeymap.insert(std::pair<std::string, int>("world_10", 170));
        SDLKeymap.insert(std::pair<std::string, int>("world_11", 171));
        SDLKeymap.insert(std::pair<std::string, int>("world_12", 172));
        SDLKeymap.insert(std::pair<std::string, int>("world_13", 173));
        SDLKeymap.insert(std::pair<std::string, int>("world_14", 174));
        SDLKeymap.insert(std::pair<std::string, int>("world_15", 175));
        SDLKeymap.insert(std::pair<std::string, int>("world_16", 176));
        SDLKeymap.insert(std::pair<std::string, int>("world_17", 177));
        SDLKeymap.insert(std::pair<std::string, int>("world_18", 178));
        SDLKeymap.insert(std::pair<std::string, int>("world_19", 179));
        SDLKeymap.insert(std::pair<std::string, int>("world_20", 180));
        SDLKeymap.insert(std::pair<std::string, int>("world_21", 181));
        SDLKeymap.insert(std::pair<std::string, int>("world_22", 182));
        SDLKeymap.insert(std::pair<std::string, int>("world_23", 183));
        SDLKeymap.insert(std::pair<std::string, int>("world_24", 184));
        SDLKeymap.insert(std::pair<std::string, int>("world_25", 185));
        SDLKeymap.insert(std::pair<std::string, int>("world_26", 186));
        SDLKeymap.insert(std::pair<std::string, int>("world_27", 187));
        SDLKeymap.insert(std::pair<std::string, int>("world_28", 188));
        SDLKeymap.insert(std::pair<std::string, int>("world_29", 189));
        SDLKeymap.insert(std::pair<std::string, int>("world_30", 190));
        SDLKeymap.insert(std::pair<std::string, int>("world_31", 191));
        SDLKeymap.insert(std::pair<std::string, int>("world_32", 192));
        SDLKeymap.insert(std::pair<std::string, int>("world_33", 193));
        SDLKeymap.insert(std::pair<std::string, int>("world_34", 194));
        SDLKeymap.insert(std::pair<std::string, int>("world_35", 195));
        SDLKeymap.insert(std::pair<std::string, int>("world_36", 196));
        SDLKeymap.insert(std::pair<std::string, int>("world_37", 197));
        SDLKeymap.insert(std::pair<std::string, int>("world_38", 198));
        SDLKeymap.insert(std::pair<std::string, int>("world_39", 199));
        SDLKeymap.insert(std::pair<std::string, int>("world_40", 200));
        SDLKeymap.insert(std::pair<std::string, int>("world_41", 201));
        SDLKeymap.insert(std::pair<std::string, int>("world_42", 202));
        SDLKeymap.insert(std::pair<std::string, int>("world_43", 203));
        SDLKeymap.insert(std::pair<std::string, int>("world_44", 204));
        SDLKeymap.insert(std::pair<std::string, int>("world_45", 205));
        SDLKeymap.insert(std::pair<std::string, int>("world_46", 206));
        SDLKeymap.insert(std::pair<std::string, int>("world_47", 207));
        SDLKeymap.insert(std::pair<std::string, int>("world_48", 208));
        SDLKeymap.insert(std::pair<std::string, int>("world_49", 209));
        SDLKeymap.insert(std::pair<std::string, int>("world_50", 210));
        SDLKeymap.insert(std::pair<std::string, int>("world_51", 211));
        SDLKeymap.insert(std::pair<std::string, int>("world_52", 212));
        SDLKeymap.insert(std::pair<std::string, int>("world_53", 213));
        SDLKeymap.insert(std::pair<std::string, int>("world_54", 214));
        SDLKeymap.insert(std::pair<std::string, int>("world_55", 215));
        SDLKeymap.insert(std::pair<std::string, int>("world_56", 216));
        SDLKeymap.insert(std::pair<std::string, int>("world_57", 217));
        SDLKeymap.insert(std::pair<std::string, int>("world_58", 218));
        SDLKeymap.insert(std::pair<std::string, int>("world_59", 219));
        SDLKeymap.insert(std::pair<std::string, int>("world_60", 220));
        SDLKeymap.insert(std::pair<std::string, int>("world_61", 221));
        SDLKeymap.insert(std::pair<std::string, int>("world_62", 222));
        SDLKeymap.insert(std::pair<std::string, int>("world_63", 223));
        SDLKeymap.insert(std::pair<std::string, int>("world_64", 224));
        SDLKeymap.insert(std::pair<std::string, int>("world_65", 225));
        SDLKeymap.insert(std::pair<std::string, int>("world_66", 226));
        SDLKeymap.insert(std::pair<std::string, int>("world_67", 227));
        SDLKeymap.insert(std::pair<std::string, int>("world_68", 228));
        SDLKeymap.insert(std::pair<std::string, int>("world_69", 229));
        SDLKeymap.insert(std::pair<std::string, int>("world_70", 230));
        SDLKeymap.insert(std::pair<std::string, int>("world_71", 231));
        SDLKeymap.insert(std::pair<std::string, int>("world_72", 232));
        SDLKeymap.insert(std::pair<std::string, int>("world_73", 233));
        SDLKeymap.insert(std::pair<std::string, int>("world_74", 234));
        SDLKeymap.insert(std::pair<std::string, int>("world_75", 235));
        SDLKeymap.insert(std::pair<std::string, int>("world_76", 236));
        SDLKeymap.insert(std::pair<std::string, int>("world_77", 237));
        SDLKeymap.insert(std::pair<std::string, int>("world_78", 238));
        SDLKeymap.insert(std::pair<std::string, int>("world_79", 239));
        SDLKeymap.insert(std::pair<std::string, int>("world_80", 240));
        SDLKeymap.insert(std::pair<std::string, int>("world_81", 241));
        SDLKeymap.insert(std::pair<std::string, int>("world_82", 242));
        SDLKeymap.insert(std::pair<std::string, int>("world_83", 243));
        SDLKeymap.insert(std::pair<std::string, int>("world_84", 244));
        SDLKeymap.insert(std::pair<std::string, int>("world_85", 245));
        SDLKeymap.insert(std::pair<std::string, int>("world_86", 246));
        SDLKeymap.insert(std::pair<std::string, int>("world_87", 247));
        SDLKeymap.insert(std::pair<std::string, int>("world_88", 248));
        SDLKeymap.insert(std::pair<std::string, int>("world_89", 249));
        SDLKeymap.insert(std::pair<std::string, int>("world_90", 250));
        SDLKeymap.insert(std::pair<std::string, int>("world_91", 251));
        SDLKeymap.insert(std::pair<std::string, int>("world_92", 252));
        SDLKeymap.insert(std::pair<std::string, int>("world_93", 253));
        SDLKeymap.insert(std::pair<std::string, int>("world_94", 254));
        SDLKeymap.insert(std::pair<std::string, int>("world_95", 255));
        SDLKeymap.insert(std::pair<std::string, int>("[0]", 256));
        SDLKeymap.insert(std::pair<std::string, int>("[1]", 257));
        SDLKeymap.insert(std::pair<std::string, int>("[2]", 258));
        SDLKeymap.insert(std::pair<std::string, int>("[3]", 259));
        SDLKeymap.insert(std::pair<std::string, int>("[4]", 260));
        SDLKeymap.insert(std::pair<std::string, int>("[5]", 261));
        SDLKeymap.insert(std::pair<std::string, int>("[6]", 262));
        SDLKeymap.insert(std::pair<std::string, int>("[7]", 263));
        SDLKeymap.insert(std::pair<std::string, int>("[8]", 264));
        SDLKeymap.insert(std::pair<std::string, int>("[9]", 265));
        SDLKeymap.insert(std::pair<std::string, int>("[.]", 266));
        SDLKeymap.insert(std::pair<std::string, int>("[/]", 267));
        SDLKeymap.insert(std::pair<std::string, int>("[*]", 268));
        SDLKeymap.insert(std::pair<std::string, int>("[-]", 269));
        SDLKeymap.insert(std::pair<std::string, int>("[+]", 270));
        SDLKeymap.insert(std::pair<std::string, int>("enter", 271));
        SDLKeymap.insert(std::pair<std::string, int>("equals", 272));
        SDLKeymap.insert(std::pair<std::string, int>("up", 273));
        SDLKeymap.insert(std::pair<std::string, int>("down", 274));
        SDLKeymap.insert(std::pair<std::string, int>("right", 275));
        SDLKeymap.insert(std::pair<std::string, int>("left", 276));
        SDLKeymap.insert(std::pair<std::string, int>("insert", 277));
        SDLKeymap.insert(std::pair<std::string, int>("home", 278));
        SDLKeymap.insert(std::pair<std::string, int>("end", 279));
        SDLKeymap.insert(std::pair<std::string, int>("page_up", 280));
        SDLKeymap.insert(std::pair<std::string, int>("page_down", 281));
        SDLKeymap.insert(std::pair<std::string, int>("f1", 282));
        SDLKeymap.insert(std::pair<std::string, int>("f2", 283));
        SDLKeymap.insert(std::pair<std::string, int>("f3", 284));
        SDLKeymap.insert(std::pair<std::string, int>("f4", 285));
        SDLKeymap.insert(std::pair<std::string, int>("f5", 286));
        SDLKeymap.insert(std::pair<std::string, int>("f6", 287));
        SDLKeymap.insert(std::pair<std::string, int>("f7", 288));
        SDLKeymap.insert(std::pair<std::string, int>("f8", 289));
        SDLKeymap.insert(std::pair<std::string, int>("f9", 290));
        SDLKeymap.insert(std::pair<std::string, int>("f10", 291));
        SDLKeymap.insert(std::pair<std::string, int>("f11", 292));
        SDLKeymap.insert(std::pair<std::string, int>("f12", 293));
        SDLKeymap.insert(std::pair<std::string, int>("f13", 294));
        SDLKeymap.insert(std::pair<std::string, int>("f14", 295));
        SDLKeymap.insert(std::pair<std::string, int>("f15", 296));
        SDLKeymap.insert(std::pair<std::string, int>("numlock", 300));
        SDLKeymap.insert(std::pair<std::string, int>("caps_lock", 301));
        SDLKeymap.insert(std::pair<std::string, int>("scroll_lock", 302));
        SDLKeymap.insert(std::pair<std::string, int>("right_shift", 303));
        SDLKeymap.insert(std::pair<std::string, int>("left_shift", 304));
        SDLKeymap.insert(std::pair<std::string, int>("right_ctrl", 305));
        SDLKeymap.insert(std::pair<std::string, int>("left_ctrl", 306));
        SDLKeymap.insert(std::pair<std::string, int>("right_alt", 307));
        SDLKeymap.insert(std::pair<std::string, int>("left_alt", 308));
        SDLKeymap.insert(std::pair<std::string, int>("right_meta", 309));
        SDLKeymap.insert(std::pair<std::string, int>("left_meta", 310));
        SDLKeymap.insert(std::pair<std::string, int>("left_super", 311));
        SDLKeymap.insert(std::pair<std::string, int>("right_super", 312));
        SDLKeymap.insert(std::pair<std::string, int>("alt_gr", 313));
        SDLKeymap.insert(std::pair<std::string, int>("compose", 314));
        SDLKeymap.insert(std::pair<std::string, int>("help", 315));
        SDLKeymap.insert(std::pair<std::string, int>("print_screen", 316));
        SDLKeymap.insert(std::pair<std::string, int>("sys_req", 317));
        SDLKeymap.insert(std::pair<std::string, int>("break", 318));
        SDLKeymap.insert(std::pair<std::string, int>("menu", 319));
        SDLKeymap.insert(std::pair<std::string, int>("power", 320));
        SDLKeymap.insert(std::pair<std::string, int>("euro", 321));
        SDLKeymap.insert(std::pair<std::string, int>("undo", 322));
    }
}

std::string input::SDLKeyToString(int sdlkey) {
    InitKeymap();

    std::map<std::string, int>::iterator i;
    for (i = SDLKeymap.begin(); i != SDLKeymap.end(); ++i) {
        if ((int)(i->second) == sdlkey) return i->first;
    }

    return "unknown";
}

int input::StringToSDLKey(const std::string &s) {
    InitKeymap();

    std::map<std::string, int>::iterator i = SDLKeymap.find(cpp::Lowercase(s));
    if (i == SDLKeymap.end()) return 0;

    return i->second;
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

}  // namespace ME