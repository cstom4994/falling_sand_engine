//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
// Copyright (c) 2011-2014 Mario 'rlyeh' Rodriguez
// Copyright (c) 2013 Florian Deconinck
// Copyright (c) 2013 Adrien Herubel
// Copyright (c) 2022, KaoruXun
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include "imgui.hpp"

#include <cmath>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)// _CRT_SECURE_NO_WARNINGS
#define snprintf _snprintf
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ref: Laird Shaw, http://creativeandcritical.net/str-replace-c/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>


char *replace_str(const char *str, const char *old, const char *recent) {
    char *ret, *r;
    const char *p, *q;
    size_t oldlen = strlen(old);
    size_t count, retlen, newlen = strlen(recent);

    if (oldlen != newlen) {
        for (count = 0, p = str; (q = strstr(p, old)) != NULL; p = q + oldlen)
            count++;
        /* this is undefined if p - str > PTRDIFF_MAX */
        retlen = p - str + strlen(p) + count * (newlen - oldlen);
    } else
        retlen = strlen(str);

    if ((ret = (char *) malloc(retlen + 1)) == NULL)
        return NULL;

    for (r = ret, p = str; (q = strstr(p, old)) != NULL; p = q + oldlen) {
        /* this is undefined if q - p > PTRDIFF_MAX */
        ptrdiff_t l = q - p;
        memcpy(r, p, l);
        r += l;
        memcpy(r, recent, newlen);
        r += newlen;
    }
    strcpy(r, p);

    return ret;
}

static std::string cpToUTF8(int cp) {
    static char str[8];
    int n = 0;
    if (cp < 0x80) n = 1;
    else if (cp < 0x800)
        n = 2;
    else if (cp < 0x10000)
        n = 3;
    else if (cp < 0x200000)
        n = 4;
    else if (cp < 0x4000000)
        n = 5;
    else if (cp <= 0x7fffffff)
        n = 6;
    str[n] = '\0';
    switch (n) {
        case 6:
            str[5] = 0x80 | (cp & 0x3f);
            cp = cp >> 6;
            cp |= 0x4000000;
        case 5:
            str[4] = 0x80 | (cp & 0x3f);
            cp = cp >> 6;
            cp |= 0x200000;
        case 4:
            str[3] = 0x80 | (cp & 0x3f);
            cp = cp >> 6;
            cp |= 0x10000;
        case 3:
            str[2] = 0x80 | (cp & 0x3f);
            cp = cp >> 6;
            cp |= 0x800;
        case 2:
            str[1] = 0x80 | (cp & 0x3f);
            cp = cp >> 6;
            cp |= 0xc0;
        case 1:
            str[0] = cp;
    }
    return str;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum {
    ROTATORY_AREA_WIDTH = 30
};
enum {
    ROTATORY_AREA_HEIGHT = 30
};
enum {
    ROTATORY_RING_WIDTH = 4
};

enum {
    BUTTON_HEIGHT = 20
};
enum {
    SLIDER_MARKER_WIDTH = 10
};
enum {
    SLIDER_MARKER_HEIGHT = 20
};
enum {
    CHECK_SIZE = 8
};
enum {
    DEFAULT_SPACING = 4
};
enum {
    TEXT_HEIGHT = 8
};
enum {
    SCROLL_AREA_PADDING = 6
};
enum {
    INDENT_SIZE = 16
};
enum {
    AREA_HEADER = 28
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static std::vector<unsigned int> colors;

#define alpha(c, a) ((colors[(c)] & 0x00ffffff) | (a << 24))

#define white_alpha(x) alpha(2, (unsigned char) ((x) *255.0 / 256.0))
#define gray_alpha(x) alpha(1, (unsigned char) ((x) *255.0 / 256.0))
#define black_alpha(x) alpha(0, (unsigned char) ((x) *255.0 / 256.0))
#define theme_alpha(x) alpha(colors.size() - 1, (unsigned char) ((x) *255.0 / 256.0))

static void imguiResetColors() {
    colors = {
            imguiRGBA(0, 0, 0),      //black
            imguiRGBA(128, 128, 128),//gray
            imguiRGBA(255, 255, 255),//white
            imguiRGBA(255, 196, 64)  //themed, queue #1
    };
}

void imguiPushColor(unsigned int c) {
    colors.push_back(c);
}
void imguiPopColor() {
    colors.pop_back();
}

void imguiPushColorTheme() {
    colors.push_back(colors[3]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static std::vector<bool> enables;

static void imguiResetEnables() {
    enables = {true};
}

void imguiPushEnable(int enable) {
    enables.push_back(enables.back() && enable > 0);
}
void imguiPopEnable() {
    enables.pop_back();
}

#define enabled (enables.back())

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static unsigned frame = 0, caret = 0;
static void imguiResetCaret() {
    caret = (((++frame %= 60) / (60 / (3 * 2))) % 2);// 60hz/20 = ~3 per sec, then blink %2
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static unsigned g_tween = 0;
static void imguiResetTween() {
    g_tween = 0;
}

void imguiTween(unsigned mode) {
    g_tween = mode % 38;
}

float imguiTween_(int mode, float dt, float t = 1.f, bool loop = false);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int mouse = 0;

static void imguiResetMouse() {
    mouse = IMGUI_MOUSE_ICON_ARROW;
}

int imguiGetMouseCursor() {
    return mouse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int (*imguiRenderTextLength)(const char *text) = 0;

enum {
    IMGUI_KEYBOARD_KEYREPEAT = 160
};// in frames
static const unsigned TEXT_POOL_SIZE = 8000;
static char g_textPool[TEXT_POOL_SIZE];
static unsigned g_textPoolSize = 0;
static const char *allocText(const char *text) {
    unsigned len = strlen(text) + 1;
    if (g_textPoolSize + len >= TEXT_POOL_SIZE)
        return 0;
    char *dst = &g_textPool[g_textPoolSize];
    memcpy(dst, text, len);
    g_textPoolSize += len;
    return dst;
}

static const unsigned GFXCMD_QUEUE_SIZE = 5000;
static imguiGfxCmd g_gfxCmdQueue[GFXCMD_QUEUE_SIZE];
static unsigned g_gfxCmdQueueSize = 0;

static void resetGfxCmdQueue() {
    g_gfxCmdQueueSize = 0;
    g_textPoolSize = 0;
}

static void addGfxCmdScissor(int x, int y, int w, int h) {
    if (g_gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
        return;
    imguiGfxCmd &cmd = g_gfxCmdQueue[g_gfxCmdQueueSize++];
    cmd.type = IMGUI_GFXCMD_SCISSOR;
    cmd.flags = x < 0 ? 0 : 1;// on/off flag.
    cmd.col = imguiRGBA(0, 0, 0, 0);
    cmd.rect.x = (short) x;
    cmd.rect.y = (short) y;
    cmd.rect.w = (short) w;
    cmd.rect.h = (short) h;
}

static imguiGfxRect getLastGfxCmdRect() {
    for (int i = g_gfxCmdQueueSize; i-- > 0;) {
        imguiGfxCmd &cmd = g_gfxCmdQueue[i];
        if (cmd.type == IMGUI_GFXCMD_RECT)
            return cmd.rect;
    }
    return imguiGfxRect();
}

static void addGfxCmdRect(float x, float y, float w, float h, unsigned int color) {
    if (g_gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
        return;
    imguiGfxCmd &cmd = g_gfxCmdQueue[g_gfxCmdQueueSize++];
    cmd.type = IMGUI_GFXCMD_RECT;
    cmd.flags = 0;
    cmd.col = color;
    cmd.rect.x = (short) (x * 8.0f);
    cmd.rect.y = (short) (y * 8.0f);
    cmd.rect.w = (short) (w * 8.0f);
    cmd.rect.h = (short) (h * 8.0f);
    cmd.rect.r = 0;
}

static void addGfxCmdLine(float x0, float y0, float x1, float y1, float r, unsigned int color) {
    if (g_gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
        return;
    imguiGfxCmd &cmd = g_gfxCmdQueue[g_gfxCmdQueueSize++];
    cmd.type = IMGUI_GFXCMD_LINE;
    cmd.flags = 0;
    cmd.col = color;
    cmd.line.x0 = (short) (x0 * 8.0f);
    cmd.line.y0 = (short) (y0 * 8.0f);
    cmd.line.x1 = (short) (x1 * 8.0f);
    cmd.line.y1 = (short) (y1 * 8.0f);
    cmd.line.r = (short) (r * 8.0f);
}

static void addGfxCmdRoundedRect(float x, float y, float w, float h, float r, unsigned int color) {
    if (g_gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
        return;
    imguiGfxCmd &cmd = g_gfxCmdQueue[g_gfxCmdQueueSize++];
    cmd.type = IMGUI_GFXCMD_RECT;
    cmd.flags = 0;
    cmd.col = color;
    cmd.rect.x = (short) (x * 8.0f);
    cmd.rect.y = (short) (y * 8.0f);
    cmd.rect.w = (short) (w * 8.0f);
    cmd.rect.h = (short) (h * 8.0f);
    cmd.rect.r = (short) (r * 8.0f);
}

static void addGfxCmdTriangle(int x, int y, int w, int h, int flags, unsigned int color) {
    if (g_gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
        return;
    imguiGfxCmd &cmd = g_gfxCmdQueue[g_gfxCmdQueueSize++];
    cmd.type = IMGUI_GFXCMD_TRIANGLE;
    cmd.flags = (char) flags;
    cmd.col = color;
    cmd.rect.x = (short) (x * 8.0f);
    cmd.rect.y = (short) (y * 8.0f);
    cmd.rect.w = (short) (w * 8.0f);
    cmd.rect.h = (short) (h * 8.0f);
}

static void addGfxCmdText(int x, int y, int align, const char *text, unsigned int color) {
    if (g_gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
        return;
    imguiGfxCmd &cmd = g_gfxCmdQueue[g_gfxCmdQueueSize++];
    cmd.type = IMGUI_GFXCMD_TEXT;
    cmd.flags = 0;
    cmd.col = color;
    cmd.text.x = (short) x;
    cmd.text.y = (short) y;
    cmd.text.align = align;
    cmd.text.text = allocText(text);
}

static void addGfxCmdArc(float x, float y, float r, float t0, float t1, unsigned int color) {
    if (g_gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
        return;
    imguiGfxCmd &cmd = g_gfxCmdQueue[g_gfxCmdQueueSize++];

    float vinc = 0.05f;
    t0 = floorf(t0 / vinc + 0.5f) * vinc;// Snap to vinc
    t1 = floorf(t1 / vinc + 0.5f) * vinc;// Snap to vinc

    cmd.type = IMGUI_GFXCMD_ARC;
    cmd.flags = 0;// on/off flag.
    cmd.col = color;
    cmd.arc.x = (short) x;
    cmd.arc.y = (short) y;
    cmd.arc.r = r;
    cmd.arc.t0 = t0;
    cmd.arc.t1 = t1;
    cmd.arc.w = ROTATORY_RING_WIDTH;
}

static void addGfxCmdTexture(float x, float y, float w, float h, unsigned id, unsigned int color) {
    if (g_gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
        return;
    imguiGfxCmd &cmd = g_gfxCmdQueue[g_gfxCmdQueueSize++];

    cmd.type = IMGUI_GFXCMD_TEXTURE;
    cmd.flags = 0;// on/off flag.
    cmd.col = color;
    cmd.texture.x = (short) x;
    cmd.texture.y = (short) y;
    cmd.texture.u = x / w;
    cmd.texture.v = y / h;
    cmd.texture.id = id;
}

static std::map<uint32_t, imguiGfxRect> g_rects;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct coord
{
    int widgetX, widgetY, widgetW;
    coord() : widgetX(0), widgetY(0), widgetW(100) {}
};

static struct GuiState : public coord
{
    GuiState() {}

    bool left = false, leftPressed = false, leftReleased = false;
    bool right = false, rightPressed = false, rightReleased = false;

    int mx = -1, my = -1;
    int scrollX = 0, scrollY = 0;

    unsigned codepoint = 0, codepoint_last = 0, codepoint_repeat = 0;

    unsigned int inputable = 0;
    unsigned int active = 0;
    unsigned int hot = 0;
    unsigned int hotToBe = 0;

    bool isHot = false;
    bool isActive = false;
    bool wentActive = false;
    unsigned clicked = 0;
    unsigned over = 0;

    int dragX = 0, dragY = 0;
    float dragOrig = 0;

    bool insideCurrentScrollY = 0;

    unsigned int areaId = 0;
    unsigned int widgetId = 0;

    std::vector<coord> coords;
    void clear() {
        coords.clear();
    }
    void push() {
        coords.push_back(*this);
    }
    void pop() {
        coords.pop_back();
    }
    void set(int pos) {
        if (pos < 0) pos = coords.size() - 1 + pos;
        *((coord *) this) = coords.at(pos);
    }
} g_state;

void imguiStackPush() {
    g_state.push();
}
int imguiStackSet(int pos) {
    int cur = g_state.coords.size() - 1;
    g_state.set(pos);
    return cur;
}
void imguiStackPop() {
    g_state.pop();
}
void imguiSpaceDiv() {
    g_state.widgetW /= 2;
    g_state.push();
}
void imguiSpaceMul() {
    g_state.widgetW *= 2;
    g_state.push();
}
void imguiSpaceShift() {
    g_state.widgetX += g_state.widgetW;
    g_state.push();
}
void imguiSpaceUnshift() {
    g_state.widgetX -= g_state.widgetW;
    g_state.push();
}
void imguiNext() {
}
void imguiBack() {
}

inline bool anyActive() {
    return g_state.active != 0;
}

inline bool isActive(unsigned int id) {
    return g_state.active == id;
}

inline bool isInputable(unsigned int id) {
    return g_state.inputable == id;
}

inline bool isHot(unsigned int id) {
    return g_state.hot == id;
}

inline void cancelHot() {
    g_state.hot = 0;
    mouse = IMGUI_MOUSE_ICON_ARROW;
}

inline bool anyHot() {
    return g_state.hot != 0;
}

inline bool inRect(unsigned id, int x, int y, int w, int h, bool checkScrollY = true) {
    g_rects[id] = {x, y, w, h, 0};

    bool res = (!checkScrollY || g_state.insideCurrentScrollY) && g_state.mx >= x && g_state.mx <= x + w && g_state.my >= y && g_state.my <= y + h;
    if (res) {
        g_state.over = id;
    }
    return res;
}

inline void clearInput() {
    g_state.leftPressed = false;
    g_state.leftReleased = false;
    g_state.scrollY = 0;
}

inline void clearActive() {
    g_state.active = 0;
    // mark all UI for this frame as processed
    clearInput();
}

inline void setActive(unsigned int id) {
    g_state.active = id;
    g_state.inputable = 0;
    g_state.wentActive = true;
}

inline void setInputable(unsigned int id) {
    g_state.inputable = id;
}

inline void setHot(unsigned int id) {
    g_state.hotToBe = id;
}


static bool buttonLogic(unsigned int id, bool over) {
    bool res = false;
    // process down
    if (!anyActive()) {
        if (over)
            setHot(id);
        if (isHot(id) && g_state.leftPressed)
            setActive(id);
    }

    // if button is active, then react on left up
    if (isActive(id)) {
        g_state.isActive = true;
        if (over)
            setHot(id);
        if (g_state.leftReleased) {
            if (isHot(id))
                res = true;
            clearActive();
        }
    }

    if (isHot(id)) {
        g_state.isHot = true;
    }

    if (g_state.isHot) {
        mouse = IMGUI_MOUSE_ICON_HAND;
    }

    if (res) {
        g_state.clicked = id;
    }

    return res;
}

static bool textInputLogic(unsigned int id, bool over) {
    //same as button logic without the react on left up
    bool res = false;
    // process down
    if (!anyActive()) {
        if (over)
            setHot(id);
        if (isHot(id) && g_state.leftPressed)
            setInputable(id);
    }

    if (isHot(id))
        g_state.isHot = true;

    return res;
}

static void updateInput(int mx, int my, unsigned char mbut, int scrollY, unsigned codepoint) {
    bool left = (mbut & IMGUI_MOUSE_BUTTON_LEFT) != 0;
    bool right = (mbut & IMGUI_MOUSE_BUTTON_RIGHT) != 0;

    g_state.mx = mx;
    g_state.my = my;

    g_state.leftPressed = !g_state.left && left;
    g_state.leftReleased = g_state.left && !left;
    g_state.left = left;

    g_state.rightPressed = !g_state.right && right;
    g_state.rightReleased = g_state.right && !right;
    g_state.right = right;

    g_state.scrollY = scrollY;

    if (g_state.codepoint_last != codepoint) {
        g_state.codepoint_repeat = 0;
    } else {
        g_state.codepoint_repeat++;
        if (g_state.codepoint_repeat > IMGUI_KEYBOARD_KEYREPEAT + 1) {
            g_state.codepoint_repeat = unsigned(g_state.codepoint_repeat * 0.80);
        }
    }

    if (!g_state.left) {
        g_state.dragX = g_state.dragY = 0;
    }

    g_state.codepoint_last = g_state.codepoint;
    g_state.codepoint = codepoint;
}

void imguiBeginFrame(int mx, int my, unsigned char mbut, int scrollY, unsigned codepoint) {
    imguiResetCaret();
    imguiResetMouse();
    imguiResetColors();
    imguiResetEnables();
    imguiResetTween();

    updateInput(mx, my, mbut, scrollY, codepoint);

    g_state.hot = g_state.hotToBe;
    g_state.hotToBe = 0;

    g_state.wentActive = false;
    g_state.isActive = false;
    g_state.isHot = false;

    g_state.clicked = 0;
    g_state.over = 0;

    g_state.widgetX = 0;
    g_state.widgetY = 0;
    g_state.widgetW = 0;
    g_state.clear();
    g_state.push();

    g_state.areaId = 1;
    g_state.widgetId = 1;

    resetGfxCmdQueue();
}

void imguiEndFrame() {
    clearInput();
}

const imguiGfxCmd *imguiGetRenderQueue() {
    return g_gfxCmdQueue;
}

int imguiGetRenderQueueSize() {
    return g_gfxCmdQueueSize;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static int g_scrollTop = 0;
static int g_scrollBottom = 0;
static int g_scrollRight = 0;
static int g_scrollAreaTop = 0;
static int *g_scrollVal = 0;
static int g_focusTop = 0;
static int g_focusBottom = 0;
static unsigned int g_scrollId = 0;
static bool g_insideScrollArea = false;

bool imguiBeginScrollArea(const char *name, int x, int y, int w, int h, int *scrollY, bool rounded) {
    g_state.areaId++;
    g_state.widgetId = 0;
    g_scrollId = (g_state.areaId << 16) | g_state.widgetId;

    int AREA_HEADING = name ? AREA_HEADER : AREA_HEADER - BUTTON_HEIGHT;

    g_state.widgetX = x + SCROLL_AREA_PADDING;
    g_state.widgetY = y + h - AREA_HEADING + (scrollY ? *scrollY : 0);// @rlyeh: support for fixed areas
    g_state.widgetW = w - SCROLL_AREA_PADDING * 4;
    g_state.push();
    g_scrollTop = y - AREA_HEADING + h;
    g_scrollBottom = y + SCROLL_AREA_PADDING;
    g_scrollRight = x + w - SCROLL_AREA_PADDING * 3;
    g_scrollVal = scrollY;

    g_scrollAreaTop = g_state.widgetY;

    g_focusTop = y - AREA_HEADING;
    g_focusBottom = y - AREA_HEADING + h;

    g_insideScrollArea = inRect(g_scrollId, x, y, w, h, false);
    g_state.insideCurrentScrollY = g_insideScrollArea;

    if (rounded)
        addGfxCmdRoundedRect((float) x, (float) y, (float) w, (float) h, 6, black_alpha(192));
    else
        addGfxCmdRect((float) x, (float) y, (float) w, (float) h, black_alpha(192));

    if (name)
        addGfxCmdText(x + AREA_HEADER / 2, y + h - AREA_HEADER / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, name, white_alpha(128));

    if (g_scrollVal) {// @rlyeh: support for fixed areas
        addGfxCmdScissor(
                x < 0 ? 0 : x + SCROLL_AREA_PADDING,//@rlyeh: fix scissor clipping problem when scroll area is on left rect client
                y + SCROLL_AREA_PADDING,
                w - SCROLL_AREA_PADDING * 4 + (x < 0 ? x : 0),                                           // @rlyeh: small optimization; @todo: on the right as well
                h > (AREA_HEADING + SCROLL_AREA_PADDING) ? h - (AREA_HEADING + SCROLL_AREA_PADDING) : h);// @rlyeh: fix when collapsing areas ( h <= AREA_HEADER )
    }

    return g_insideScrollArea;
}

void imguiEndScrollArea() {
    if (g_scrollVal) {// @rlyeh: support for fixed areas
        // Disable scissoring.
        addGfxCmdScissor(-1, -1, -1, -1);
    }

    // Draw scroll bar
    int x = g_scrollRight + SCROLL_AREA_PADDING / 2;
    int y = g_scrollBottom;
    int w = SCROLL_AREA_PADDING * 2;
    int h = g_scrollTop - g_scrollBottom;

    int stop = g_scrollAreaTop;
    int sbot = g_state.widgetY;
    int sh = stop - sbot;// The scrollable area height.

    float barHeight = (float) h / (float) sh;

    if (h > AREA_HEADER && barHeight < 1)// @rlyeh: fix when area size is too small
    {
        float barY = (float) (y - sbot) / (float) sh;
        if (barY < 0) barY = 0;
        if (barY > 1) barY = 1;

        // Handle scroll bar logic.
        unsigned int hid = g_scrollId;
        int hx = x;
        int hy = y + (int) (barY * h);
        int hw = w;
        int hh = (int) (barHeight * h);

        const int range = h - (hh - 1);
        bool over = inRect(g_scrollId, hx, hy, hw, hh, true);
        buttonLogic(hid, over);
        float u = (float) (hy - y) / (float) range;
        if (isActive(hid)) {
            if (g_state.wentActive) {
                g_state.dragY = g_state.my;
                g_state.dragOrig = u;
            }
            if (g_state.dragY != g_state.my) {
                u = g_state.dragOrig + (g_state.my - g_state.dragY) / (float) range;
                if (u < 0) u = 0;
                if (u > 1) u = 1;
                *g_scrollVal = (int) ((1 - u) * (sh - h));
            }
        } else if (u <= 0 || u > 1)
            *g_scrollVal = (int) (sh - h);//rlyeh: @fix when resizing windows

        // BG
        addGfxCmdRoundedRect((float) x, (float) y, (float) w, (float) h, (float) w / 2 - 1, black_alpha(196));
        // Bar
        if (isActive(hid))
            addGfxCmdRoundedRect((float) hx, (float) hy, (float) hw, (float) hh, (float) w / 2 - 1, theme_alpha(196));
        else
            addGfxCmdRoundedRect((float) hx, (float) hy, (float) hw, (float) hh, (float) w / 2 - 1, isHot(hid) ? theme_alpha(96) : theme_alpha(64));

        // Handle mouse scrolling.
        if (g_insideScrollArea)// && !anyActive())
        {
            if (g_state.scrollY) {
                *g_scrollVal += 20 * g_state.scrollY;
                if (*g_scrollVal < 0) *g_scrollVal = 0;
                if (*g_scrollVal > (sh - h)) *g_scrollVal = (sh - h);
            }
        }
    } else
        *g_scrollVal = 0;// @rlyeh: fix for mismatching scroll when collapsing/uncollapsing content larger than container height
    g_state.insideCurrentScrollY = false;
}

bool imguiButton(const char *text) {
    g_state.widgetId++;
    unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

    int x = g_state.widgetX;
    int y = g_state.widgetY - BUTTON_HEIGHT;
    int w = g_state.widgetW;
    int h = BUTTON_HEIGHT;

    int offset = w - DEFAULT_SPACING;
    g_state.widgetX += offset;
    g_state.widgetW -= offset;
    g_state.push();
    g_state.widgetX -= offset;
    g_state.widgetW += offset;
    g_state.push();
    g_state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;
    g_state.push();

    bool over = inRect(id, x, y, w, h, true) && enabled;
    bool res = buttonLogic(id, over);

    addGfxCmdRoundedRect((float) x, (float) y, (float) w, (float) h, (float) BUTTON_HEIGHT / 2 - 1, isActive(id) ? gray_alpha(196) : gray_alpha(96));
    if (enabled)
        addGfxCmdText(x + w / 2, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_CENTER | IMGUI_ALIGN_BASELINE, text, isHot(id) ? theme_alpha(256) : theme_alpha(192));
    else
        addGfxCmdText(x + w / 2, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_CENTER | IMGUI_ALIGN_BASELINE, text, gray_alpha(200));

    return res;
}

bool imguiImage(unsigned texture_id) {
    g_state.widgetId++;
    unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

    int x = g_state.widgetX;
    int y = g_state.widgetY - BUTTON_HEIGHT;
    int w = g_state.widgetW;
    int h = BUTTON_HEIGHT;

    int offset = w - DEFAULT_SPACING;
    g_state.widgetX += offset;
    g_state.widgetW -= offset;
    g_state.push();
    g_state.widgetX -= offset;
    g_state.widgetW += offset;
    g_state.push();
    g_state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;
    g_state.push();

    bool over = inRect(id, x, y, w, h, true) && enabled;
    bool res = buttonLogic(id, over);

    addGfxCmdTexture((float) x, (float) y, (float) w, (float) h, id, isActive(id) ? gray_alpha(196) : gray_alpha(96));

    return res;
}

bool imguiItem(const char *text) {
    g_state.widgetId++;
    unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

    int x = g_state.widgetX;
    int y = g_state.widgetY - BUTTON_HEIGHT;
    int w = g_state.widgetW;
    int h = BUTTON_HEIGHT;

    g_state.widgetX += w;
    g_state.widgetW -= w;
    g_state.push();
    g_state.widgetX -= w;
    g_state.widgetW += w;
    g_state.push();
    g_state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;
    g_state.push();

    bool over = inRect(id, x, y, w, h, true) && enabled;
    bool res = buttonLogic(id, over);

    if (isHot(id))
        addGfxCmdRoundedRect((float) x, (float) y, (float) w, (float) h, 2.0f, isActive(id) ? theme_alpha(192) : theme_alpha(96));

    if (enabled)
        addGfxCmdText(x + BUTTON_HEIGHT / 2, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, theme_alpha(200));
    else
        addGfxCmdText(x + BUTTON_HEIGHT / 2, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, gray_alpha(200));

    return res;
}

bool imguiText(const char *text) {
    g_state.widgetId++;
    unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

    int x = g_state.widgetX;
    int y = g_state.widgetY - BUTTON_HEIGHT;
    int w = imguiTextLength(text);
    int h = BUTTON_HEIGHT;

    g_state.widgetX += w;
    g_state.widgetW -= w;
    g_state.push();
    g_state.widgetX -= w;
    g_state.widgetW += w;
    g_state.push();
    g_state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;
    g_state.push();

    bool over = inRect(id, x, y, w, h, true) && enabled;
    bool res = buttonLogic(id, over);

    if (enabled)
        addGfxCmdText(x, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, isHot(id) ? theme_alpha(200) : theme_alpha(192));
    else
        addGfxCmdText(x, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, gray_alpha(200));

    return res;
}

bool imguiIcon(unsigned icon) {
    if (icon >= 0xF000 && icon <= 0xF196) {
        return imguiText((std::string("\3") + imguiTextConv(icon)).c_str());
    } else {
        return imguiText((std::string("\2") + imguiTextConv(icon)).c_str());
    }
}

int imguiToolbar(const std::vector<unsigned> &icons) {
    int res = 0;

    if (imguiIcon(icons.front())) {
        res = 1;
    }

    int pos_ = imguiStackSet(-1);
    imguiStackSet(pos_);

    for (auto begin = icons.begin(), end = icons.end(), it = begin + 1; it != end; ++it) {
        int pos = imguiStackSet(-2);
        if (imguiIcon(*it)) {
            res = 1 + it - begin;
        }
        imguiStackSet(pos);
    }

    imguiStackSet(pos_);

    return res;
}

bool imguiCheck(const char *text, bool checked) {
    g_state.widgetId++;
    unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

    int x = g_state.widgetX;
    int y = g_state.widgetY - BUTTON_HEIGHT;
    int w = g_state.widgetW;
    int h = BUTTON_HEIGHT;
    g_state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;
    g_state.push();

    bool over = inRect(id, x, y, w, h, true) && enabled;
    bool res = buttonLogic(id, over);

    const int cx = x + BUTTON_HEIGHT / 2 - CHECK_SIZE / 2;
    const int cy = y + BUTTON_HEIGHT / 2 - CHECK_SIZE / 2;
    addGfxCmdRoundedRect((float) cx - 3, (float) cy - 3, (float) CHECK_SIZE + 6, (float) CHECK_SIZE + 6, 4, isActive(id) ? gray_alpha(196) : gray_alpha(96));
    if (checked) {
        if (enabled)
            addGfxCmdRoundedRect((float) cx, (float) cy, (float) CHECK_SIZE, (float) CHECK_SIZE, (float) CHECK_SIZE / 2 - 1, isActive(id) ? theme_alpha(256) : theme_alpha(200));
        else
            addGfxCmdRoundedRect((float) cx, (float) cy, (float) CHECK_SIZE, (float) CHECK_SIZE, (float) CHECK_SIZE / 2 - 1, gray_alpha(200));
    }

    if (enabled)
        addGfxCmdText(x + BUTTON_HEIGHT, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, isHot(id) ? theme_alpha(256) : theme_alpha(200));
    else
        addGfxCmdText(x + BUTTON_HEIGHT, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, gray_alpha(200));

    return res;
}

bool imguiCollapse(const char *text, const char *subtext, bool checked) {
    g_state.widgetId++;
    unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

    int x = g_state.widgetX;
    int y = g_state.widgetY - BUTTON_HEIGHT;
    int w = g_state.widgetW;
    int h = BUTTON_HEIGHT;
    g_state.widgetY -= BUTTON_HEIGHT;// + DEFAULT_SPACING;
    g_state.push();

    const int cx = x + BUTTON_HEIGHT / 2 - CHECK_SIZE / 2;
    const int cy = y + BUTTON_HEIGHT / 2 - CHECK_SIZE / 2;

    bool over = inRect(id, x, y, w, h, true) && enabled;
    bool res = buttonLogic(id, over);

    if (checked)
        addGfxCmdTriangle(cx, cy, CHECK_SIZE, CHECK_SIZE, 2, !enabled ? gray_alpha(128) : isActive(id) ? theme_alpha(256)
                                                                                                       : theme_alpha(192));
    else
        addGfxCmdTriangle(cx, cy, CHECK_SIZE, CHECK_SIZE, 1, !enabled ? gray_alpha(128) : isActive(id) ? theme_alpha(256)
                                                                                                       : theme_alpha(192));

    addGfxCmdText(x + BUTTON_HEIGHT, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, !enabled ? gray_alpha(192) : isHot(id) ? theme_alpha(256)
                                                                                                                                                                    : theme_alpha(192));

    if (subtext)
        addGfxCmdText(x + w - BUTTON_HEIGHT / 2, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_RIGHT | IMGUI_ALIGN_BASELINE, subtext, enabled ? theme_alpha(128) : gray_alpha(128));

    return res;
}

void imguiLabel(const char *text) {
    int x = g_state.widgetX;
    int y = g_state.widgetY - BUTTON_HEIGHT;
    g_state.widgetY -= BUTTON_HEIGHT;
    g_state.push();
    addGfxCmdText(x, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, theme_alpha(256));
}

void imguiValue(const char *text) {
    const int x = g_state.widgetX;
    const int y = g_state.widgetY - BUTTON_HEIGHT;
    const int w = g_state.widgetW;
    g_state.widgetY -= BUTTON_HEIGHT;
    g_state.push();
    addGfxCmdText(x + w - BUTTON_HEIGHT / 2, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_RIGHT | IMGUI_ALIGN_BASELINE, text, theme_alpha(192));
}

bool imguiSlider(const char *text, float *val, float vmin, float vmax, float vinc, const char *format) {
    g_state.widgetId++;
    unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

    int x = g_state.widgetX;
    int y = g_state.widgetY - BUTTON_HEIGHT;
    int w = g_state.widgetW;
    int h = SLIDER_MARKER_HEIGHT;
    g_state.widgetY -= SLIDER_MARKER_HEIGHT + DEFAULT_SPACING;
    g_state.push();

    addGfxCmdRoundedRect((float) x, (float) y, (float) w, (float) h, 4.0f, black_alpha(96));

    const int range = w - SLIDER_MARKER_WIDTH;

    float u = (*val - vmin) / (vmax - vmin);
    if (u < 0) u = 0;
    if (u > 1) u = 1;
    int m = (int) (u * range);

    bool over = inRect(id, x + m, y, SLIDER_MARKER_WIDTH, SLIDER_MARKER_HEIGHT, true) && enabled;
    bool res = buttonLogic(id, over);
    bool valChanged = false;

    if (isActive(id)) {
        if (g_state.wentActive) {
            g_state.dragX = g_state.mx;
            g_state.dragOrig = u;
        }
        if (g_state.dragX != g_state.mx) {
            u = g_state.dragOrig + (float) (g_state.mx - g_state.dragX) / (float) range;
            if (u < 0) u = 0;
            if (u > 1) u = 1;
            *val = vmin + u * (vmax - vmin);
            *val = floorf(*val / vinc + 0.5f) * vinc;// Snap to vinc
            m = (int) (u * range);
            valChanged = true;
        }
    }

    unsigned int col = gray_alpha(64);
    if (enabled) {
        if (isActive(id)) col = theme_alpha(256);
        else
            col = isHot(id) ? theme_alpha(128) : theme_alpha(64);
    }
    addGfxCmdRoundedRect((float) (x + m), (float) y, (float) SLIDER_MARKER_WIDTH, (float) SLIDER_MARKER_HEIGHT, 4.0f, col);

    // TODO: fix this, take a look at 'nicenum'.
    int digits = (int) (std::ceilf(std::log10f(vinc)));
    char msg[128];
    const char *replaced = replace_str(format, "%d", "%.*f");
    sprintf(msg, replaced ? replaced : "%.*f", digits >= 0 ? 0 : -digits, *val);
    if (replaced) {
        free((void *) replaced);
    }

    if (enabled) {
        addGfxCmdText(x + SLIDER_MARKER_HEIGHT / 2, y + SLIDER_MARKER_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, isHot(id) | isActive(id) ? theme_alpha(256) : theme_alpha(192));    // @rlyeh: fix blinking colours
        addGfxCmdText(x + w - SLIDER_MARKER_HEIGHT / 2, y + SLIDER_MARKER_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_RIGHT | IMGUI_ALIGN_BASELINE, msg, isHot(id) | isActive(id) ? theme_alpha(256) : theme_alpha(192));// @rlyeh: fix blinking colours
    } else {
        addGfxCmdText(x + SLIDER_MARKER_HEIGHT / 2, y + SLIDER_MARKER_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, gray_alpha(192));
        addGfxCmdText(x + w - SLIDER_MARKER_HEIGHT / 2, y + SLIDER_MARKER_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_RIGHT | IMGUI_ALIGN_BASELINE, msg, gray_alpha(192));
    }

    return res || valChanged;
}

bool imguiLoadingBar(int flow, float radius) {

    g_state.widgetId++;
    unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

    int w = /* ROTATORY_AREA_WIDTH * */ int(radius);
    int h = /* ROTATORY_AREA_HEIGHT */ w;
    int x = g_state.widgetX + 0 + (ROTATORY_AREA_WIDTH - w) / 2;
    int y = g_state.widgetY - h - (ROTATORY_AREA_HEIGHT - h) / 2;
    int cx = x + w / 2;
    int cy = y + h / 2;
    int r = w / 2;

    g_state.widgetX += w;
    g_state.widgetW -= w;
    g_state.push();
    g_state.widgetX -= w;
    g_state.widgetW += w;
    g_state.push();
    g_state.widgetY -= h + DEFAULT_SPACING;
    g_state.push();

    bool over = inRect(id, x, y, w, h, true) && enabled;
    bool clicked = buttonLogic(id, over);

    unsigned int bg = isHot(id) | isActive(id) ? theme_alpha(96) : theme_alpha(64);
    unsigned int fg = theme_alpha(256);
    if (!enabled) {
        bg = gray_alpha(32);
        fg = gray_alpha(192);
    }

    addGfxCmdArc(cx, cy, r, 0, 1, bg);

    float from = (frame % 30) / 30.f;
    if (!flow) from = 1 - from;
    float to = fmodf(from + 0.33f, 1.f);
    if (from < to) {
        addGfxCmdArc(cx, cy, r, from, to, fg);
    } else {
        addGfxCmdArc(cx, cy, r, 0, to, fg);
        addGfxCmdArc(cx, cy, r, from, 1, fg);
    }

    return clicked;
}

bool imguiRotatorySlider(const char *text, float *val, float vmin, float vmax, float vinc, const char *format) {
    g_state.widgetId++;
    unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

    int x = g_state.widgetX;
    int y = g_state.widgetY - BUTTON_HEIGHT - ROTATORY_AREA_HEIGHT / 2;
    int w = ROTATORY_AREA_WIDTH;
    int h = ROTATORY_AREA_HEIGHT;

    g_state.widgetX += w;
    g_state.widgetW -= w;
    g_state.push();
    g_state.widgetX -= w;
    g_state.widgetW += w;
    g_state.push();
    g_state.widgetY -= h + DEFAULT_SPACING;
    g_state.push();

    //        addGfxCmdRoundedRect((float)x, (float)y, (float)w, (float)h, 4.0f, black_alpha(96) );

    bool over = inRect(id, x, y, w, h, true) && enabled;
    bool clicked = buttonLogic(id, over);
    bool valChanged = false;

    if (over && g_state.scrollY) {
        *val += g_state.scrollY * ((vmax - vmin) / 20.f);
        g_state.scrollY = 0;// capture scroll so container's window area does not scroll
    }

    if (clicked) {
        int mx = g_state.mx - (x + w / 2);
        int my = g_state.my - (y + h / 2);
        float distance2_from_r = (mx * mx + my * my);

        // check borders
        int collide = IMGUI_SECTION_OUTSIDE;
        float inner = ROTATORY_AREA_WIDTH / 2 - ROTATORY_RING_WIDTH * 2;
        float outer = ROTATORY_AREA_WIDTH / 2 - ROTATORY_RING_WIDTH;
        /**/ if (distance2_from_r < (inner * inner + inner * inner))
            collide = IMGUI_SECTION_INSIDE;
        else if (distance2_from_r < (outer * outer + outer * outer))
            collide = IMGUI_SECTION_BORDER;
        if (collide >= IMGUI_SECTION_BORDER) {
            // check angle
            float angle = atan2(my, mx) * 180 / 3.1415926535;
            float angle360_clockwise = fmodf(360 + angle, 360);
            float angle360_anticlockwise = fmodf(360 - angle, 360);
            float circle_anticlockwise = fmodf(angle360_anticlockwise + 90, 360);
            //compute new val
            *val = (circle_anticlockwise / 360.f) * (vmax - vmin) + vmin;
        }
    }

    if (*val < vmin) *val = vmin;
    if (*val > vmax) *val = vmax;

    const int range = w - ROTATORY_AREA_WIDTH;

    float u = (*val - vmin) / (vmax - vmin);
    if (u < 0) u = 0;
    if (u > 1) u = 1;
    int m = (int) (ROTATORY_AREA_WIDTH);

    if (0)
        if (isActive(id)) {
            if (g_state.wentActive) {
                g_state.dragX = g_state.mx;
                g_state.dragOrig = u;
            }
            if (g_state.dragX != g_state.mx) {
                u = g_state.dragOrig + (float) (g_state.mx - g_state.dragX) / (float) range;
                if (u < 0) u = 0;
                if (u > 1) u = 1;
                *val = vmin + u * (vmax - vmin);
                *val = floorf(*val / vinc + 0.5f) * vinc;// Snap to vinc
                m = (int) (u * range);
                valChanged = true;
            }
        }

    // TODO: fix this, take a look at 'nicenum'.
    int digits = (int) (std::ceilf(std::log10f(vinc)));
    char msg[128];
    const char *replaced = replace_str(format, "%d", "%.*f");
    sprintf(msg, replaced ? replaced : "%.*f", digits >= 0 ? 0 : -digits, *val);
    if (replaced) {
        free((void *) replaced);
    }

    if (enabled) {
        unsigned int col = isHot(id) | isActive(id) ? theme_alpha(256) : theme_alpha(192);

        imguiDrawArc(float(x) + ROTATORY_AREA_WIDTH / 2, float(y) + ROTATORY_AREA_WIDTH / 2, ROTATORY_AREA_WIDTH / 2, 0.00f, u, col);
        imguiDrawArc(float(x) + ROTATORY_AREA_WIDTH / 2, float(y) + ROTATORY_AREA_WIDTH / 2, ROTATORY_AREA_WIDTH / 2, u, 1.00f, theme_alpha(64));

        //addGfxCmdText(x+SLIDER_MARKER_HEIGHT/2, y+SLIDER_MARKER_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT|IMGUI_ALIGN_BASELINE, text, col ); // @rlyeh: fix blinking colours
        addGfxCmdText(x + w / 2, y + h / 2, IMGUI_ALIGN_CENTER | IMGUI_ALIGN_MIDDLE, msg, col);// @rlyeh: fix blinking colours
    } else {
        unsigned int col = gray_alpha(192);

        imguiDrawArc(float(x) + ROTATORY_AREA_WIDTH / 2, float(y) + ROTATORY_AREA_WIDTH / 2, ROTATORY_AREA_WIDTH / 2, 0.00f, u, col);
        imguiDrawArc(float(x) + ROTATORY_AREA_WIDTH / 2, float(y) + ROTATORY_AREA_WIDTH / 2, ROTATORY_AREA_WIDTH / 2, u, 1.00f, gray_alpha(96));

        //addGfxCmdText(x+SLIDER_MARKER_HEIGHT/2, y+SLIDER_MARKER_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT|IMGUI_ALIGN_BASELINE, text, col );
        addGfxCmdText(x + w / 2, y + h / 2, IMGUI_ALIGN_CENTER | IMGUI_ALIGN_MIDDLE, msg, col);
    }

    return clicked || valChanged;
}

bool imguiXYSlider(const char *text, float *valX, float *valY, float height, float vinc, const char *format) {
    g_state.widgetId++;
    unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

    int w = g_state.widgetW;
    int h = height < BUTTON_HEIGHT ? BUTTON_HEIGHT : height;
    int x = g_state.widgetX;
    int y = g_state.widgetY - height;

    g_state.widgetX += w;
    g_state.widgetW -= w;
    g_state.push();
    g_state.widgetX -= w;
    g_state.widgetW += w;
    g_state.push();
    g_state.widgetY -= h + DEFAULT_SPACING;
    g_state.push();

    bool over = inRect(id, x, y, w, h, true) && enabled;
    bool clicked = buttonLogic(id, over);
    bool valChanged = false;

    if (clicked) {
        *valX = g_state.mx - (x + w / 2);
        *valY = g_state.my - (y + h / 2);
    }

    // TODO: fix this, take a look at 'nicenum'.
    int digits = (int) (std::ceilf(std::log10f(vinc)));
    char msg[128];
    const char *replaced = replace_str(format, "%d", "%.*f");
    sprintf(msg, replaced ? replaced : "(%.*f,%.*f)", digits >= 0 ? 0 : -digits, *valX, digits >= 0 ? 0 : -digits, *valY);
    if (replaced) {
        free((void *) replaced);
    }

    float mx = g_state.mx, my = g_state.my;
    char mmsg[128];
    replaced = replace_str(format, "%d", "%.*f");
    sprintf(mmsg, replaced ? replaced : "(%.*f,%.*f)", digits >= 0 ? 0 : -digits, mx - (x + w / 2), digits >= 0 ? 0 : -digits, my - (y + h / 2));
    if (replaced) {
        free((void *) replaced);
    }

    unsigned int fg = isHot(id) | isActive(id) ? theme_alpha(192) : theme_alpha(128);
    unsigned int bg = isHot(id) | isActive(id) ? theme_alpha(32) : theme_alpha(16);

    if (!enabled) {
        fg = isHot(id) | isActive(id) ? gray_alpha(192) : gray_alpha(128);
        bg = isHot(id) | isActive(id) ? gray_alpha(32) : gray_alpha(16);
    }

    imguiDrawRect(x, y, w, h, bg);

    imguiDrawLine(x + w / 2, y, x + w / 2, y + h, 2, fg);
    imguiDrawLine(x, y + h / 2, x + w, y + h / 2, 2, fg);

    if (over && enabled) {
        imguiDrawLine(mx, y, mx, y + h, 2, bg);
        imguiDrawLine(x, my, x + w, my, 2, bg);
    }

    addGfxCmdText(x + w / 2, y + h / 2, IMGUI_ALIGN_CENTER | IMGUI_ALIGN_BOTTOM, msg, fg);
    addGfxCmdText(x + w / 2, y + h / 2, IMGUI_ALIGN_CENTER | IMGUI_ALIGN_TOP, mmsg, bg);

    float cx = x + w / 2 + (*valX - CHECK_SIZE / 2), cy = y + h / 2 + (*valY - CHECK_SIZE / 2);
    addGfxCmdRoundedRect((float) cx, (float) cy, (float) CHECK_SIZE, (float) CHECK_SIZE, (float) CHECK_SIZE / 2 - 1, fg);

    return clicked || valChanged;
}

bool imguiRange(const char *text, float *val0, float *val1, float vmin, float vmax, float vinc, const char *format) {
    int x = g_state.widgetX;
    int y = g_state.widgetY - BUTTON_HEIGHT;
    int w = g_state.widgetW;
    int h = SLIDER_MARKER_HEIGHT;
    g_state.widgetY -= SLIDER_MARKER_HEIGHT + DEFAULT_SPACING;
    g_state.push();

    // dims

    if (vmin > vmax) {
        float swap = vmin;
        vmin = vmax;
        vmax = swap;
    }
    if (*val0 > *val1) {
        float swap = *val0;
        *val0 = *val1;
        *val1 = swap;
    }
    if (*val0 < vmin) { *val0 = vmin; }
    if (*val1 > vmax) { *val1 = vmax; }

    const int range = w - SLIDER_MARKER_WIDTH;

    float u0 = (*val0 - vmin) / (vmax - vmin);
    if (u0 < 0) u0 = 0;
    if (u0 > 1) u0 = 1;
    int m0 = (int) (u0 * range);

    float u1 = (*val1 - vmin) / (vmax - vmin);
    if (u1 < 0) u1 = 0;
    if (u1 > 1) u1 = 1;
    int m1 = (int) (u1 * range);

    // common

    bool is_highlighted = false;
    bool is_res = false;
    bool has_changed = false;

    // draggable bar
    {
        g_state.widgetId++;
        unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

        bool over = inRect(id, x + m0, y, m1 - m0 + SLIDER_MARKER_WIDTH, h, true) && enabled;
        bool res = buttonLogic(id, over);
        bool valChanged = false;

        if (isActive(id)) {
            if (g_state.wentActive) {
                g_state.dragX = g_state.mx;
                g_state.dragOrig = u0;
            }
            if (g_state.dragX != g_state.mx) {
                float u = u0;
                float *val = val0;
                float diff = *val1 - *val0;

                u = g_state.dragOrig + (float) (g_state.mx - g_state.dragX) / (float) range;
                if (u < 0) u = 0;
                if (u > 1) u = 1;
                *val = vmin + u * (vmax - vmin);
                *val = floorf(*val / vinc + 0.5f) * vinc;// Snap to vinc
                *val1 = *val + diff;
                if (*val1 > vmax) {
                    *val1 = vmax;
                    *val0 = *val1 - diff;
                }
                //m = (int)(u * range);
                has_changed = true;
            }
        }

        unsigned int col = isActive(id) ? theme_alpha(96) : theme_alpha(64);
        if (!enabled) col = gray_alpha(64);

        addGfxCmdRoundedRect((float) x + m0, (float) y, (float) m1 - m0 + SLIDER_MARKER_WIDTH, (float) h, 4.0f, col);
        addGfxCmdRoundedRect((float) x, (float) y, (float) w, (float) h, 4.0f, black_alpha(96));
    }

    // slide #0
    {
        float *val = val0;
        float u = u0, m = m0;

        g_state.widgetId++;
        unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

        bool over = inRect(id, x + m, y, SLIDER_MARKER_WIDTH, SLIDER_MARKER_HEIGHT, true) && enabled;
        bool res = buttonLogic(id, over);
        bool valChanged = false;

        if (isActive(id)) {
            if (g_state.wentActive) {
                g_state.dragX = g_state.mx;
                g_state.dragOrig = u;
            }
            if (g_state.dragX != g_state.mx) {
                u = g_state.dragOrig + (float) (g_state.mx - g_state.dragX) / (float) range;
                if (u < 0) u = 0;
                if (u > 1) u = 1;
                *val = vmin + u * (vmax - vmin);
                *val = floorf(*val / vinc + 0.5f) * vinc;// Snap to vinc
                m = (int) (u * range);
                valChanged = true;
            }
        }

        unsigned int col = gray_alpha(64);
        if (enabled) {
            if (isActive(id)) col = theme_alpha(256);
            else
                col = isHot(id) ? theme_alpha(128) : theme_alpha(64);
        }
        addGfxCmdRoundedRect((float) (x + m), (float) y, (float) SLIDER_MARKER_WIDTH, (float) SLIDER_MARKER_HEIGHT, 4.0f, col);

        is_highlighted |= (isHot(id) | isActive(id));
        is_res |= res;
        has_changed |= valChanged;
    }

    // slide #1
    {
        float *val = val1;
        float u = u1, m = m1;

        g_state.widgetId++;
        unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

        bool over = inRect(id, x + m, y, SLIDER_MARKER_WIDTH, SLIDER_MARKER_HEIGHT, true) && enabled;
        bool res = buttonLogic(id, over);
        bool valChanged = false;

        if (isActive(id)) {
            if (g_state.wentActive) {
                g_state.dragX = g_state.mx;
                g_state.dragOrig = u;
            }
            if (g_state.dragX != g_state.mx) {
                u = g_state.dragOrig + (float) (g_state.mx - g_state.dragX) / (float) range;
                if (u < 0) u = 0;
                if (u > 1) u = 1;
                *val = vmin + u * (vmax - vmin);
                *val = floorf(*val / vinc + 0.5f) * vinc;// Snap to vinc
                m = (int) (u * range);
                valChanged = true;
            }
        }

        unsigned int col = gray_alpha(64);
        if (enabled) {
            if (isActive(id)) col = theme_alpha(256);
            else
                col = isHot(id) ? theme_alpha(128) : theme_alpha(64);
        }
        addGfxCmdRoundedRect((float) (x + m), (float) y, (float) SLIDER_MARKER_WIDTH, (float) SLIDER_MARKER_HEIGHT, 4.0f, col);

        is_highlighted |= (isHot(id) | isActive(id));
        is_res |= res;
        has_changed |= valChanged;
    }

    // text

    // TODO: fix this, take a look at 'nicenum'.
    int digits = (int) (std::ceilf(std::log10f(vinc)));
    char msg[128];
    const char *replaced = replace_str(format, "%d", "%.*f");
    sprintf(msg, replaced ? replaced : "%.*f - %.*f", digits >= 0 ? 0 : -digits, *val0, digits >= 0 ? 0 : -digits, *val1);
    if (replaced) {
        free((void *) replaced);
    }

    if (enabled) {
        addGfxCmdText(x + SLIDER_MARKER_HEIGHT / 2, y + SLIDER_MARKER_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, is_highlighted ? theme_alpha(256) : theme_alpha(192));    // @rlyeh: fix blinking colours
        addGfxCmdText(x + w - SLIDER_MARKER_HEIGHT / 2, y + SLIDER_MARKER_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_RIGHT | IMGUI_ALIGN_BASELINE, msg, is_highlighted ? theme_alpha(256) : theme_alpha(192));// @rlyeh: fix blinking colours
    } else {
        addGfxCmdText(x + SLIDER_MARKER_HEIGHT / 2, y + SLIDER_MARKER_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, gray_alpha(192));
        addGfxCmdText(x + w - SLIDER_MARKER_HEIGHT / 2, y + SLIDER_MARKER_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_RIGHT | IMGUI_ALIGN_BASELINE, msg, gray_alpha(192));
    }

    return is_res || has_changed;
}


bool imguiSliderMarker(const char *text, float *val, float vmin, float vmax, float vinc, const char *format) {
    int x = g_state.widgetX;
    int y = g_state.widgetY - SLIDER_MARKER_HEIGHT;
    int w = SLIDER_MARKER_WIDTH;
    int h = SLIDER_MARKER_HEIGHT;
    g_state.widgetX += w;
    g_state.widgetW -= w;
    g_state.push();
    g_state.widgetX -= w;
    g_state.widgetW += w;
    g_state.push();
    g_state.widgetY -= SLIDER_MARKER_HEIGHT + DEFAULT_SPACING;
    g_state.push();

    if (vmin > vmax) {
        float swap = vmin;
        vmin = vmax;
        vmax = swap;
    }
    if (*val < vmin) { *val = vmin; }
    if (*val > vmax) { *val = vmax; }

    float u = (*val) / (vmax - vmin);
    float m = vmax - vmin;
    float range = m;

    bool is_highlighted = false;
    bool is_res = false;
    bool has_changed = false;

    g_state.widgetId++;
    unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

    bool over = inRect(id, x + m, y, SLIDER_MARKER_WIDTH, SLIDER_MARKER_HEIGHT, true) && enabled;
    bool res = buttonLogic(id, over);
    bool valChanged = false;

    if (over && g_state.scrollY) {
        *val += g_state.scrollY * ((vmax - vmin) / 20.f);
        g_state.scrollY = 0;// capture scroll so container's window area does not scroll
    }

    unsigned int bg = theme_alpha(64);
    unsigned int fg = theme_alpha(128);
    if (isActive(id) || isHot(id)) fg = theme_alpha(256);
    if (!enabled) {
        fg = gray_alpha(64);
        bg = gray_alpha(16);
    }

    addGfxCmdRoundedRect((float) (x), (float) y, (float) SLIDER_MARKER_WIDTH, (float) SLIDER_MARKER_HEIGHT, 4.0f, bg);
    addGfxCmdRect((float) (x), (float) y + SLIDER_MARKER_HEIGHT * u, (float) SLIDER_MARKER_WIDTH, (float) SLIDER_MARKER_HEIGHT * vinc, fg);

    is_highlighted |= (isHot(id) | isActive(id));
    is_res |= res;
    has_changed |= valChanged;

    return is_res || has_changed;
}


bool imguiQuadRange(const char *text, float *val0, float *val1, float vmin, float vmax, float vinc, float *valLO, float *valHI, const char *format) {
    int x = g_state.widgetX;
    int y = g_state.widgetY - BUTTON_HEIGHT;
    int w = g_state.widgetW;
    int h = SLIDER_MARKER_HEIGHT;
    g_state.widgetX += w;
    g_state.widgetW -= w;
    g_state.push();
    g_state.widgetX -= w;
    g_state.widgetW += w;
    g_state.push();
    g_state.widgetY -= SLIDER_MARKER_HEIGHT + DEFAULT_SPACING;
    g_state.push();

    // dims

    if (vmin > vmax) {
        float swap = vmin;
        vmin = vmax;
        vmax = swap;
    }
    if (*val0 > *val1) {
        float swap = *val0;
        *val0 = *val1;
        *val1 = swap;
    }
    if (*val0 < vmin) { *val0 = vmin; }
    if (*val1 > vmax) { *val1 = vmax; }

    const int range = w - SLIDER_MARKER_WIDTH;

    float u0 = (*val0 - vmin) / (vmax - vmin);
    if (u0 < 0) u0 = 0;
    if (u0 > 1) u0 = 1;
    int m0 = (int) (u0 * range);

    float u1 = (*val1 - vmin) / (vmax - vmin);
    if (u1 < 0) u1 = 0;
    if (u1 > 1) u1 = 1;
    int m1 = (int) (u1 * range);

    // common

    bool is_highlighted = false;
    bool is_res = false;
    bool has_changed = false;

    // draggable bar
    {
        g_state.widgetId++;
        unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

        bool over = inRect(id, x + m0, y, m1 - m0 + SLIDER_MARKER_WIDTH, h, true) && enabled;
        bool res = buttonLogic(id, over);
        bool valChanged = false;

        if (isActive(id)) {
            if (g_state.wentActive) {
                g_state.dragX = g_state.mx;
                g_state.dragOrig = u0;
            }
            if (g_state.dragX != g_state.mx) {
                float u = u0;
                float *val = val0;
                float diff = *val1 - *val0;

                u = g_state.dragOrig + (float) (g_state.mx - g_state.dragX) / (float) range;
                if (u < 0) u = 0;
                if (u > 1) u = 1;
                *val = vmin + u * (vmax - vmin);
                *val = floorf(*val / vinc + 0.5f) * vinc;// Snap to vinc
                *val1 = *val + diff;
                if (*val1 > vmax) {
                    *val1 = vmax;
                    *val0 = *val1 - diff;
                }
                //m = (int)(u * range);
                has_changed = true;
            }
        }

        unsigned int col = isActive(id) ? theme_alpha(96) : theme_alpha(64);
        if (!enabled) col = gray_alpha(64);

        addGfxCmdRoundedRect((float) x + m0, (float) y, (float) m1 - m0 + SLIDER_MARKER_WIDTH, (float) h, 4.0f, col);
        addGfxCmdRoundedRect((float) x, (float) y, (float) w, (float) h, 4.0f, black_alpha(96));

        m0 += SLIDER_MARKER_WIDTH;
        m1 -= 0;
        std::vector<float> points(16 * 2);
        for (int i = 0, j = 16; i < j; ++i) {
            float ldt01 = i / float(j - 1);
            float tdt01 = imguiTween_(g_tween, ldt01);
            float tdt10 = 1 - tdt01;
            float &px = points[i * 2 + 0];
            float &py = points[i * 2 + 1];
            px = x + m0 + (m1 - m0) * (ldt01);
            py = y + h * ((*valLO) + (*valHI - *valLO) * (tdt01));
        }
        m1 += 0;
        m0 -= SLIDER_MARKER_WIDTH;
        unsigned lnfg = over ? theme_alpha(256) : theme_alpha(128);
        if (!enabled) lnfg = gray_alpha(96);
        imguiDrawLines(points, 2, lnfg);
    }

    // slide #0
    {
        float *val = val0;
        float u = u0, m = m0;

        unsigned int next_id = (g_state.areaId << 16) | (g_state.widgetId + 1);
        imguiStackPush();
        g_state.widgetX += m;
        g_state.widgetY += SLIDER_MARKER_HEIGHT + DEFAULT_SPACING;
        bool res = imguiSliderMarker("", valLO, 0.f, 1.f, 0.1f, "");
        int pos = imguiStackSet(-3);
        bool valChanged = false;

        if (isActive(next_id)) {
            if (g_state.wentActive) {
                g_state.dragX = g_state.mx;
                g_state.dragOrig = u;
            }
            if (g_state.dragX != g_state.mx) {
                u = g_state.dragOrig + (float) (g_state.mx - g_state.dragX) / (float) range;
                if (u < 0) u = 0;
                if (u > 1) u = 1;
                *val = vmin + u * (vmax - vmin);
                *val = floorf(*val / vinc + 0.5f) * vinc;// Snap to vinc
                m = (int) (u * range);
                valChanged = true;
            }
        }

        is_highlighted |= (isHot(next_id) | isActive(next_id));
        is_res |= res;
        has_changed |= valChanged;
    }

    // slide #1
    {
        float *val = val1;
        float u = u1, m = m1;

        unsigned int next_id = (g_state.areaId << 16) | (g_state.widgetId + 1);
        imguiStackPush();
        g_state.widgetX += m;
        g_state.widgetY += SLIDER_MARKER_HEIGHT + DEFAULT_SPACING;
        bool res = imguiSliderMarker("", valHI, 0.f, 1.f, 0.1f, "");
        int pos = imguiStackSet(-3);
        bool valChanged = false;

        if (isActive(next_id)) {
            if (g_state.wentActive) {
                g_state.dragX = g_state.mx;
                g_state.dragOrig = u;
            }
            if (g_state.dragX != g_state.mx) {
                u = g_state.dragOrig + (float) (g_state.mx - g_state.dragX) / (float) range;
                if (u < 0) u = 0;
                if (u > 1) u = 1;
                *val = vmin + u * (vmax - vmin);
                *val = floorf(*val / vinc + 0.5f) * vinc;// Snap to vinc
                m = (int) (u * range);
                valChanged = true;
            }
        }

        is_highlighted |= (isHot(next_id) | isActive(next_id));
        is_res |= res;
        has_changed |= valChanged;
    }

    // text

    // TODO: fix this, take a look at 'nicenum'.
    int digits = (int) (std::ceilf(std::log10f(vinc)));
    char msg[128];
    const char *replaced = replace_str(format, "%d", "%.*f");
    sprintf(msg, replaced ? replaced : "%.*f - %.*f", digits >= 0 ? 0 : -digits, *val0, digits >= 0 ? 0 : -digits, *val1);
    if (replaced) {
        free((void *) replaced);
    }

    if (enabled) {
        addGfxCmdText(x + SLIDER_MARKER_HEIGHT / 2, y + SLIDER_MARKER_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, is_highlighted ? theme_alpha(256) : theme_alpha(192));    // @rlyeh: fix blinking colours
        addGfxCmdText(x + w - SLIDER_MARKER_HEIGHT / 2, y + SLIDER_MARKER_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_RIGHT | IMGUI_ALIGN_BASELINE, msg, is_highlighted ? theme_alpha(256) : theme_alpha(192));// @rlyeh: fix blinking colours
    } else {
        addGfxCmdText(x + SLIDER_MARKER_HEIGHT / 2, y + SLIDER_MARKER_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, gray_alpha(192));
        addGfxCmdText(x + w - SLIDER_MARKER_HEIGHT / 2, y + SLIDER_MARKER_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_RIGHT | IMGUI_ALIGN_BASELINE, msg, gray_alpha(192));
    }

    return is_res || has_changed;
}

std::string imguiTextConv(const unsigned &utf32) {
    return cpToUTF8(utf32);
}

std::string imguiTextConv(const std::vector<unsigned> &utf32) {
    std::string out;
    for (auto &cp: utf32) {
        out += cpToUTF8(cp);
    }
    return out;
}

std::vector<unsigned> imguiTextConv(const std::string &ascii) {
    std::vector<unsigned> out;
    for (auto &ch: ascii) {
        out.push_back(ch);
    }
    return out;
}

bool imguiTextInput(const char *text, std::vector<unsigned> &utf32, bool is_password) {
    bool res = true;
    //--
    //Handle label
    g_state.widgetId++;
    unsigned int id = (g_state.areaId << 16) | g_state.widgetId;
    int x = g_state.widgetX;
    int y = g_state.widgetY - BUTTON_HEIGHT;
    addGfxCmdText(x, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, enabled ? white_alpha(255) : gray_alpha(128));
    unsigned int textLen = (unsigned int) (imguiTextLength(text));
    //--
    //Handle keyboard input if any
    if (enabled && isInputable(id)) {
        if (g_state.codepoint == 0x0D) {
            //enter
            g_state.inputable = 0;
        } else if (g_state.codepoint == 0x08 && (g_state.codepoint_repeat == 1 || g_state.codepoint_repeat > IMGUI_KEYBOARD_KEYREPEAT)) {
            //backspace wip
            if (utf32.size() && utf32.back() > 8) utf32.pop_back();
        } else if (g_state.codepoint != 0 && (g_state.codepoint_repeat == 1 || g_state.codepoint_repeat > IMGUI_KEYBOARD_KEYREPEAT)) {
            //codepoint
            utf32.push_back(g_state.codepoint);
        }
    }
    //--
    //Handle buffer data
    x += textLen;
    int w = g_state.widgetW - textLen;
    int h = BUTTON_HEIGHT;
    bool over = inRect(id, x, y, w, h);
    res = textInputLogic(id, over);
    std::string utf8 = is_password ? std::string(utf32.size(), '*') : imguiTextConv(utf32);
    if (enabled) {
        if (caret && isInputable(id)) utf8.push_back('|');
        addGfxCmdRoundedRect((float) x, (float) y, (float) w, (float) h, (float) BUTTON_HEIGHT / 2 - 1, isInputable(id) ? theme_alpha(256) : gray_alpha(96));
        addGfxCmdText(x + 7, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, utf8.c_str(), isInputable(id) ? black_alpha(256) : white_alpha(256));
    } else {
        addGfxCmdRoundedRect((float) x, (float) y, (float) w, (float) h, (float) BUTTON_HEIGHT / 2 - 1, gray_alpha(64));
        addGfxCmdText(x + 7, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, utf8.c_str(), white_alpha(128));
    }
    //--
    g_state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;
    g_state.push();
    return res | (g_state.codepoint == 0x0D);
}

void imguiPair(const char *text, const char *value)// @rlyeh: new widget
{
    imguiLabel(text);
    g_state.widgetY += BUTTON_HEIGHT;
    g_state.push();
    imguiValue(value);
}

bool imguiList(const char *text, size_t n_options, const char **options, int &choosing, int &choice, int *scrollY)// @rlyeh: new widget
{
    g_state.widgetId++;
    unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

    int x = g_state.widgetX;
    int y = g_state.widgetY - BUTTON_HEIGHT;
    int w = g_state.widgetW;
    int h = BUTTON_HEIGHT;
    g_state.widgetY -= BUTTON_HEIGHT;// + DEFAULT_SPACING;
    g_state.push();

    const int cx = x + BUTTON_HEIGHT / 2 - CHECK_SIZE / 2;
    const int cy = y + BUTTON_HEIGHT / 2 - CHECK_SIZE / 2;

    bool over = inRect(id, x, y, w, h, true) && enabled;
    bool res = buttonLogic(id, over);

    if (enabled) {
        addGfxCmdRoundedRect((float) x, (float) y, (float) w, (float) h, 2.0f, !isHot(id) ? gray_alpha(64) : (isActive(id) ? theme_alpha(192) : theme_alpha(choosing ? 64 : 96)));

        addGfxCmdTriangle((float) cx, (float) cy, CHECK_SIZE, CHECK_SIZE, choosing ? 2 : 1, isActive(id) ? theme_alpha(256) : theme_alpha(192));

        addGfxCmdText(x + BUTTON_HEIGHT, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, choice < 0 ? text : options[choice], theme_alpha(192));

        //addGfxCmdRoundedRect(x+w-BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, (float)CHECK_SIZE, (float)CHECK_SIZE, (float)CHECK_SIZE/2-1, isActive(id) ? theme_alpha(256) : theme_alpha(192));

        if (res)
            choosing ^= 1;
    } else {
        addGfxCmdRoundedRect((float) x, (float) y, (float) w, (float) h, 2.0f, gray_alpha(64));

        addGfxCmdTriangle((float) cx, (float) cy, CHECK_SIZE, CHECK_SIZE, choosing ? 2 : 1, isActive(id) ? theme_alpha(256) : theme_alpha(192));

        addGfxCmdText(x + BUTTON_HEIGHT, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, choice < 0 ? text : options[choice], gray_alpha(192));

        //addGfxCmdRoundedRect(x+w-BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, (float)CHECK_SIZE, (float)CHECK_SIZE, (float)CHECK_SIZE/2-1, isActive(id) ? theme_alpha(256) : theme_alpha(192));
    }

    bool result = false;

    if (choosing) {
        // choice selector
        imguiIndent();
        if (scrollY)
            imguiBeginScrollArea(0, x, y - 150 - BUTTON_HEIGHT, w, h + 150, scrollY);
        // hotness = are we on focus?
        bool hotness = isHot(id) | isActive(id);
        // choice selector list
        for (size_t n = 0; !result && n < n_options; ++n) {
            // got answer?
            if (imguiItem(options[n]))
                choice = n, choosing = 0, scrollY = 0, result = true;

            unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

            // ensure that widget is still on focus while choosing
            hotness |= isHot(id) | isActive(id);
        }
        // close on blur
        if (!hotness && anyHot()) {}//    choosing = 0;
        if (scrollY)
            imguiEndScrollArea();
        imguiUnindent();
    }

    return result;
}

bool imguiRadio(const char *text, size_t n_options, const char **options, int &clicked)// @rlyeh: new widget
{
    g_state.widgetId++;
    unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

    int x = g_state.widgetX;
    int y = g_state.widgetY - BUTTON_HEIGHT;
    int w = g_state.widgetW;
    int h = BUTTON_HEIGHT;
    g_state.widgetY -= BUTTON_HEIGHT;// + DEFAULT_SPACING;
    g_state.push();

    const int cx = x + BUTTON_HEIGHT / 2 - CHECK_SIZE / 2;
    const int cy = y + BUTTON_HEIGHT / 2 - CHECK_SIZE / 2;

    bool over = inRect(id, x, y, w, h, true) && enabled;
    bool res = buttonLogic(id, over);

    if (enabled)
        addGfxCmdText(cx, cy, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, isHot(id) ? theme_alpha(256) : theme_alpha(192));
    else
        addGfxCmdText(cx, cy, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, gray_alpha(192));

    bool result = false;

    imguiIndent();
    for (size_t i = 0; i < n_options; ++i) {
        bool cl = (clicked == i);
        if (imguiCheck(options[i], cl))
            clicked = i, result = true;
    }
    imguiUnindent();

    return result;
}

void imguiProgressBar(const char *text, float val, bool show_decimals) {
    const float vmin = 0.00f, vmax = 100.00f;

    if (val < 0.f) val = 0.f;
    else if (val > 100.f)
        val = 100.f;

    g_state.widgetId++;
    unsigned int id = (g_state.areaId << 16) | g_state.widgetId;

    int x = g_state.widgetX;
    int y = g_state.widgetY - BUTTON_HEIGHT;
    int w = g_state.widgetW;
    int h = SLIDER_MARKER_HEIGHT;
    g_state.widgetY -= SLIDER_MARKER_HEIGHT + DEFAULT_SPACING;
    g_state.push();

    addGfxCmdRoundedRect((float) x, (float) y, (float) w, (float) h, 4.0f, black_alpha(96));

    const int range = w - SLIDER_MARKER_WIDTH;

    float u = (val - vmin) / (vmax - vmin);
    if (u < 0) u = 0;
    if (u > 1) u = 1;
    int m = (int) (u * range);

    addGfxCmdRoundedRect((float) (x + 0), (float) y, (float) (SLIDER_MARKER_WIDTH + m), (float) SLIDER_MARKER_HEIGHT, 4.0f, theme_alpha(64));

    // TODO: fix this, take a look at 'nicenum'.
    int digits = (int) (std::ceilf(std::log10f(0.01f)));
    char msg[128];
    if (show_decimals)
        sprintf(msg, "%.*f%%", digits >= 0 ? 0 : -digits, val);
    else
        sprintf(msg, "%d%%", int(val));

    addGfxCmdText(x + SLIDER_MARKER_HEIGHT / 2, y + SLIDER_MARKER_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, theme_alpha(192));
    addGfxCmdText(x + w - SLIDER_MARKER_HEIGHT / 2, y + SLIDER_MARKER_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_RIGHT | IMGUI_ALIGN_BASELINE, msg, theme_alpha(192));
}

bool imguiBitmask(const char *text, unsigned *mask, int bits) {
    int x = g_state.widgetX;
    int y = g_state.widgetY - BUTTON_HEIGHT;
    int w = g_state.widgetW;
    int h = BUTTON_HEIGHT;

    //--
    //Handle label
    g_state.widgetId++;
    unsigned int id = (g_state.areaId << 16) | g_state.widgetId;
    //    addGfxCmdText(x, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT|IMGUI_ALIGN_BASELINE, text, white_alpha(255));
    if (enabled)
        addGfxCmdText(x + DEFAULT_SPACING, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, isHot(id) ? theme_alpha(256) : theme_alpha(200));
    else
        addGfxCmdText(x + DEFAULT_SPACING, y + BUTTON_HEIGHT / 2 - TEXT_HEIGHT / 2, IMGUI_ALIGN_LEFT | IMGUI_ALIGN_BASELINE, text, gray_alpha(200));
    unsigned int textLen = (unsigned int) (imguiTextLength(text));
    //--

    bool ress = false;
    unsigned before = *mask;

    const int cxx = x + textLen + (textLen > 0 ? 1 : 0) * DEFAULT_SPACING + CHECK_SIZE / 2;
    const int cy = y + BUTTON_HEIGHT / 2 - CHECK_SIZE / 2;

    int offset = (cxx - x) + bits * (CHECK_SIZE + 8);
    g_state.widgetX += offset;
    g_state.widgetW -= offset;
    g_state.push();
    g_state.widgetX -= offset;
    g_state.widgetW += offset;
    g_state.push();
    g_state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;
    g_state.push();

    for (int i = 0; i < bits; ++i) {//bits; i-- > 0; ) {

        int cx = cxx + (bits - 1 - i) * (CHECK_SIZE + 8);
        bool checked = ((*mask) & (1 << i)) == (1 << i);

        g_state.widgetId++;
        unsigned int id = (g_state.areaId << 16) | g_state.widgetId;
        bool over = inRect(id, (float) cx - 3, (float) cy - 3, (float) CHECK_SIZE + 8, (float) CHECK_SIZE + 8, true) && enabled;
        bool res = buttonLogic(id, over);

        if (res) {
            ress |= res;
            (*mask) ^= (1 << i);
        }

        //addGfxCmdRoundedRect((float)cx-3, (float)cy-3, (float)CHECK_SIZE+8, (float)CHECK_SIZE+8, 4,  isActive(id) ? gray_alpha(196) : gray_alpha(96) );
        if (checked) {
            if (enabled)
                addGfxCmdRoundedRect((float) cx, (float) cy, (float) CHECK_SIZE, (float) CHECK_SIZE, (float) CHECK_SIZE / 2 - 1, isActive(id) ? theme_alpha(256) : theme_alpha(200));
            else
                addGfxCmdRoundedRect((float) cx, (float) cy, (float) CHECK_SIZE, (float) CHECK_SIZE, (float) CHECK_SIZE / 2 - 1, gray_alpha(200));
        } else {
            if (enabled)
                addGfxCmdRoundedRect((float) cx + CHECK_SIZE / 4, (float) cy + CHECK_SIZE / 4, (float) CHECK_SIZE / 2, (float) CHECK_SIZE / 2, (float) CHECK_SIZE / 8 - 1, isActive(id) ? theme_alpha(256) : theme_alpha(200));
            else
                addGfxCmdRoundedRect((float) cx + CHECK_SIZE / 4, (float) cy + CHECK_SIZE / 4, (float) CHECK_SIZE / 2, (float) CHECK_SIZE / 2, (float) CHECK_SIZE / 8 - 1, gray_alpha(200));
        }
    }

    return ress | (before != *mask);
}

void imguiIndent() {
    g_state.widgetX += INDENT_SIZE;
    g_state.widgetW -= INDENT_SIZE;
    g_state.push();
}

void imguiUnindent() {
    g_state.widgetX -= INDENT_SIZE;
    g_state.widgetW += INDENT_SIZE;
    g_state.push();
}

void imguiSeparator() {
    g_state.widgetY -= DEFAULT_SPACING * 3;
    g_state.push();
}

void imguiSeparatorLine() {
    int x = g_state.widgetX;
    int y = g_state.widgetY - DEFAULT_SPACING * 2;
    int w = g_state.widgetW;
    int h = 1;
    g_state.widgetY -= DEFAULT_SPACING * 4;
    g_state.push();

    addGfxCmdRect((float) x, (float) y, (float) w, (float) h, theme_alpha(32));
}

// @todo: fixme, buggy
void imguiTabulator() {
    const int BUTTON_WIDTH = g_state.widgetW > g_state.widgetX ? g_state.widgetW - g_state.widgetX : g_state.widgetX - g_state.widgetW;

    g_state.widgetX += BUTTON_WIDTH;
    g_state.widgetW += BUTTON_WIDTH;

    // should in fact get retrieved from last widget queued size
    g_state.widgetY += BUTTON_HEIGHT;
    g_state.push();
}

// @todo: fixme, buggy
void imguiTabulatorLine() {
    int x = g_state.widgetX + DEFAULT_SPACING * 2;
    int y = g_state.widgetY;
    int w = 1;
    int h = 100;//g_state.widgetH;

    g_state.widgetX += DEFAULT_SPACING * 4;
    g_state.widgetW += DEFAULT_SPACING * 4;
    g_state.push();

    addGfxCmdRect((float) x, (float) y, (float) w, (float) h, theme_alpha(32));
}

void imguiDrawText(int x, int y, imguiTextAlign align, const char *text, unsigned int color) {
    addGfxCmdText(x, y, align, text, color);
}

void imguiDrawLine(float x0, float y0, float x1, float y1, float r, unsigned int color) {
    addGfxCmdLine(x0, y0, x1, y1, r, color);
}

void imguiDrawLines(const std::vector<float> &points2d, float r, unsigned int color) {
    for (int i = 0; i < points2d.size() - 2; i += 2) {
        addGfxCmdLine(points2d[i], points2d[i + 1], points2d[i + 2], points2d[i + 3], r, color);
    }
}

void imguiDrawRect(float x, float y, float w, float h, unsigned int color) {
    addGfxCmdRect(x, y, w, h, color);
}

void imguiDrawArc(float x, float y, float radius, float from, float to, unsigned int color) {
    addGfxCmdArc(x, y, radius, from, to, color);
}

void imguiDrawCircle(float x, float y, float radius, unsigned int color) {
    addGfxCmdArc(x, y, radius, 0, 1, color);
}

void imguiDrawRoundedRect(float x, float y, float w, float h, float r, unsigned int color) {
    addGfxCmdRoundedRect(x, y, w, h, r, color);
}

int imguiTextLength(const char *text) {
    return (*imguiRenderTextLength)(text);
}

static float imguiClamp(float a, float mn, float mx) {
    return a < mn ? mn : (a > mx ? mx : a);
}
static float imguiHue(float h, float m1, float m2) {
    if (h < 0) h += 1;
    if (h > 1) h -= 1;
    /**/ if (h < 1.0f / 6.0f)
        return m1 + (m2 - m1) * h * 6.0f;
    else if (h < 3.0f / 6.0f)
        return m2;
    else if (h < 4.0f / 6.0f)
        return m1 + (m2 - m1) * (2.0f / 3.0f - h) * 6.0f;
    return m1;
}

unsigned int imguiHSLA(float h, float s, float l, unsigned char a) {
    float m1, m2;
    unsigned char r, g, b;
    h = fmodf(h, 1.0f);
    if (h < 0.0f) h += 1.0f;
    s = imguiClamp(s, 0.0f, 1.0f);
    l = imguiClamp(l, 0.0f, 1.0f);
    m2 = l <= 0.5f ? (l * (1 + s)) : (l + s - l * s);
    m1 = 2 * l - m2;
    r = (unsigned char) imguiClamp(imguiHue(h + 1.0f / 3.0f, m1, m2) * 255.0f, 0, 255);
    g = (unsigned char) imguiClamp(imguiHue(h, m1, m2) * 255.0f, 0, 255);
    b = (unsigned char) imguiClamp(imguiHue(h - 1.0f / 3.0f, m1, m2) * 255.0f, 0, 255);
    return imguiRGBA(r, g, b, a);
}

unsigned int imguiRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return (r) | (g << 8) | (b << 16) | (a << 24);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int imguiShowDialog(const char *text, int *answer) {
    int clicked = 0;

    imguiLabel(text);
    imguiSpaceDiv();
    if (imguiButton("yes")) {
        clicked = 1;
        *answer = 1;
    }
    int pos = imguiStackSet(-1);
    imguiSpaceShift();
    if (imguiButton("no")) {
        clicked = 1;
        *answer = 0;
    }
    imguiSpaceUnshift();
    imguiStackSet(pos);
    imguiSpaceMul();

    return clicked;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool imguiWheel(unsigned id);
bool imguiClicked(unsigned id) {
    return g_state.clicked == id;
}
bool imguiClicked2(unsigned id);
bool imguiTrigger(unsigned id);
bool imguiHold(unsigned id);
bool imguiRelease(unsigned id);
bool imguiIdle(unsigned id);
bool imguiDrag(unsigned id);
bool imguiDrop(unsigned id);
bool imguiOver(unsigned id) {
    return g_state.over == id;
}
bool imguiHot(unsigned id) {
    return isHot(id);
}
bool imguiActive(unsigned id) {
    return isActive(id);
}
bool imguiIsIdle() {
    return anyHot() || anyActive() ? false : true;
}

unsigned imguiId() {
    unsigned id = (g_state.areaId << 16) | g_state.widgetId;
    return id;
}

imguiGfxRect imguiRect(unsigned id) {
    auto found = g_rects.find(id);
    if (found == g_rects.end()) {
        return {0, 0, 0, 0, 0};
    }
    return found->second;
}

float imguiTween_(int type, float current, float total, bool looped) {
    const float pi = 3.1415926535897932384626433832795f;

    float t = current, d = total;

    /* small optimization { */

    //if( d <= 0.0f )
    //    return 1.0f;

    if (d <= 0.f || t <= 0.f) {
        return (type == IMGUI_TWEEN_COS2PI_11 || type == IMGUI_TWEEN_SINPI2PI_10 ? 1.f : 0.f);
    }

    if (t >= d) {
        if (looped) {
            // todo: optimize me
            while (t >= d)
                t -= d;
        } else {
            if (type >= IMGUI_TWEEN_SIN2PI_00)
                return 0.f;

            return 1.f;
        }
    }

    /* } */

    switch (type) {
        case IMGUI_TWEEN_LINEAR_01: {
            return t / d;
        }

        case IMGUI_TWEEN_SIN2PI_00: {
            float fDelta = t / d;
            return std::sin(fDelta * 2.0f * pi);
        }

        case IMGUI_TWEEN_SINPI_00: {
            float fDelta = t / d;
            return std::sin(fDelta * pi);
        }

        case IMGUI_TWEEN_SINPI2PI_10: {
            float fDelta = t / d;
            return std::sin((0.5f * pi) * (fDelta + 1));
        }

        case IMGUI_TWEEN_SIN4PI_00: {
            float fDelta = t / d;
            return std::sin(fDelta * 4.0f * pi);
        }

        case IMGUI_TWEEN_SIN3PI4_00: {
            float fDelta = t / d;
            return std::sin(fDelta * 3.0f * pi);
        }

        case IMGUI_TWEEN_SINPI2_01: {
            float fDelta = t / d;
            return std::sin(fDelta * 0.5f * pi);
        }

        case IMGUI_TWEEN_ACELBREAK_01: {
            float fDelta = t / d;
            return (std::sin((fDelta * pi) - (pi * 0.5f)) + 1.0f) * 0.5f;
        }

        case IMGUI_TWEEN_COS2PI_11: {
            float fDelta = t / d;
            return std::cos(fDelta * 2.0f * pi);
        }

        case IMGUI_TWEEN_BACKIN_01: {
            float s = 1.70158f;
            float postFix = t /= d;
            return postFix * t * ((s + 1) * t - s);
        }

        case IMGUI_TWEEN_BACKOUT_01: {
            float s = 1.70158f;
            return 1.f * ((t = t / d - 1) * t * ((s + 1) * t + s) + 1);
        }

        case IMGUI_TWEEN_BACKINOUT_01: {
            float s = 1.70158f;
            if ((t /= d / 2) < 1)
                return 1.f / 2 * (t * t * (((s *= (1.525f)) + 1) * t - s));

            float postFix = t -= 2;
            return 1.f / 2 * ((postFix) *t * (((s *= (1.525f)) + 1) * t + s) + 2);
        }

#define BOUNCE(v)                                      \
    if ((t /= d) < (1 / 2.75f)) {                      \
        v = 1.f * (7.5625f * t * t);                   \
    } else if (t < (2 / 2.75f)) {                      \
        float postFix = t -= (1.5f / 2.75f);           \
                                                       \
        v = 1.f * (7.5625f * (postFix) *t + .75f);     \
    } else if (t < (2.5 / 2.75)) {                     \
        float postFix = t -= (2.25f / 2.75f);          \
                                                       \
        v = 1.f * (7.5625f * (postFix) *t + .9375f);   \
    } else {                                           \
        float postFix = t -= (2.625f / 2.75f);         \
                                                       \
        v = 1.f * (7.5625f * (postFix) *t + .984375f); \
    }

        case IMGUI_TWEEN_BOUNCEIN_01: {
            float v;
            t = d - t;
            BOUNCE(v);
            return 1.f - v;
        }

        case IMGUI_TWEEN_BOUNCEOUT_01: {
            float v;
            BOUNCE(v);
            return v;
        }

        case IMGUI_TWEEN_BOUNCEINOUT_01: {
            float v;
            if (t < d / 2) {
                t = t * 2;
                t = d - t;
                BOUNCE(v);
                return (1.f - v) * .5f;
            } else {
                t = t * 2 - d;
                BOUNCE(v);
                return v * .5f + 1.f * .5f;
            }
        }

#undef BOUNCE

        case IMGUI_TWEEN_CIRCIN_01:
            t /= d;
            return 1.f - std::sqrt(1 - t * t);

        case IMGUI_TWEEN_CIRCOUT_01:
            t /= d;
            t--;
            return std::sqrt(1 - t * t);

        case IMGUI_TWEEN_CIRCINOUT_01:
            t /= d / 2;
            if (t < 1)
                return -1.f / 2 * (std::sqrt(1 - t * t) - 1);

            t -= 2;
            return 1.f / 2 * (std::sqrt(1 - t * t) + 1);


        case IMGUI_TWEEN_ELASTICIN_01: {
            t /= d;

            float p = d * .3f;
            float a = 1.f;
            float s = p / 4;
            float postFix = a * std::pow(2, 10 * (t -= 1));// this is a fix, again, with post-increment operators

            return -(postFix * std::sin((t * d - s) * (2 * pi) / p));
        }

        case IMGUI_TWEEN_ELASTICOUT_01: {
            float p = d * .3f;
            float a = 1.f;
            float s = p / 4;

            return (a * std::pow(2, -10 * t) * std::sin((t * d - s) * (2 * pi) / p) + 1.f);
        }

        case IMGUI_TWEEN_ELASTICINOUT_01: {
            t /= d / 2;

            float p = d * (.3f * 1.5f);
            float a = 1.f;
            float s = p / 4;

            if (t < 1) {
                float postFix = a * std::pow(2, 10 * (t -= 1));// postIncrement is evil
                return -.5f * (postFix * std::sin((t * d - s) * (2 * pi) / p));
            }

            float postFix = a * std::pow(2, -10 * (t -= 1));// postIncrement is evil
            return postFix * std::sin((t * d - s) * (2 * pi) / p) * .5f + 1.f;
        }

        case IMGUI_TWEEN_EXPOIN_01:
            return std::pow(2, 10 * (t / d - 1));

        case IMGUI_TWEEN_EXPOOUT_01:
            return 1.f - (t == d ? 0 : std::pow(2, -10.f * (t / d)));

        case IMGUI_TWEEN_EXPOINOUT_01:
            t /= d / 2;
            if (t < 1)
                return 1.f / 2 * std::pow(2, 10 * (t - 1));

            t--;
            return 1.f / 2 * (-std::pow(2, -10 * t) + 2);

        case IMGUI_TWEEN_QUADIN_01:
            t /= d;
            return t * t;

        case IMGUI_TWEEN_QUADOUT_01:
            t /= d;
            return (2.f - t) * t;

        case IMGUI_TWEEN_QUADINOUT_01:
            t /= d / 2;
            if (t < 1)
                return (1.f / 2 * t * t);

            t--;
            return -1.f / 2 * (t * (t - 2) - 1);

        case IMGUI_TWEEN_CUBICIN_01:
            t /= d;
            return t * t * t;

        case IMGUI_TWEEN_CUBICOUT_01:
            t /= d;
            t--;
            return (1.f + t * t * t);

        case IMGUI_TWEEN_CUBICINOUT_01:
            t /= d / 2;
            if (t < 1)
                return 1.f / 2 * t * t * t;

            t -= 2;
            return 1.f / 2 * (t * t * t + 2);

        case IMGUI_TWEEN_QUARTIN_01:
            t /= d;
            return t * t * t * t;

        case IMGUI_TWEEN_QUARTOUT_01:
            t /= d;
            t--;
            return (1.f - t * t * t * t);

        case IMGUI_TWEEN_QUARTINOUT_01:
            t /= d / 2;
            if (t < 1)
                return 1.f / 2 * t * t * t * t;

            t -= 2;
            return -1.f / 2 * (t * t * t * t - 2);

        case IMGUI_TWEEN_QUINTIN_01:
            t /= d;
            return t * t * t * t * t;

        case IMGUI_TWEEN_QUINTOUT_01:
            t /= d;
            t--;
            return (1.f + t * t * t * t * t);

        case IMGUI_TWEEN_QUINTINOUT_01:
            t /= d / 2;
            if (t < 1)
                return 1.f / 2 * t * t * t * t * t;

            t -= 2;
            return 1.f / 2 * (t * t * t * t * t + 2);

        case IMGUI_TWEEN_SINEIN_01:
            return 1.f - std::cos(t / d * (pi / 2));

        case IMGUI_TWEEN_SINEOUT_01:
            return std::sin(t / d * (pi / 2));

        case IMGUI_TWEEN_SINEINOUT_01:
            return -1.f / 2 * (std::cos(pi * t / d) - 1);


        case IMGUI_TWEEN_SINESQUARE: {
            float A = std::sin(0.5f * (t / d) * pi);
            return A * A;
        }

        case IMGUI_TWEEN_EXPONENTIAL: {
            return 1 / (1 + std::exp(6 - 12 * (t / d)));
        }

        case IMGUI_TWEEN_SCHUBRING1: {
            t /= d;
            return 2 * (t + (0.5f - t) * std::abs(0.5f - t)) - 0.5f;
        }

        case IMGUI_TWEEN_SCHUBRING2: {
            t /= d;
            float p1pass = 2 * (t + (0.5f - t) * std::abs(0.5f - t)) - 0.5f;
            float p2pass = 2 * (p1pass + (0.5f - p1pass) * std::abs(0.5f - p1pass)) - 0.5f;
            return p2pass;
        }

        case IMGUI_TWEEN_SCHUBRING3: {
            t /= d;
            float p1pass = 2 * (t + (0.5f - t) * std::abs(0.5f - t)) - 0.5f;
            float p2pass = 2 * (p1pass + (0.5f - p1pass) * std::abs(0.5f - p1pass)) - 0.5f;
            float pAvg = (p1pass + p2pass) / 2;
            return pAvg;
        }

        default:
            return 0;
    }
}

#ifdef _MSC_VER
#pragma warning(pop)// C4996
#endif
