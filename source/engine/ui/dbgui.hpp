// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_DBGUI_HPP
#define ME_DBGUI_HPP

#include <inttypes.h>

#include <algorithm>
#include <array>
#include <regex>
#include <string>
#include <vector>

#include "engine/audio/audio.h"
#include "engine/core/base_debug.hpp"
#include "engine/core/base_memory.h"
#include "engine/core/basic_types.h"
#include "engine/core/global.hpp"
#include "engine/core/io/packer.hpp"
#include "engine/core/macros.hpp"
#include "engine/core/profiler.hpp"
#include "engine/core/sdl_wrapper.h"
#include "engine/cvar.hpp"
#include "engine/game_datastruct.hpp"
#include "engine/scripting/scripting.hpp"
#include "engine/ui/imgui_impl.hpp"
#include "libs/imgui/font_awesome.h"
#include "libs/imgui/text_editor.h"

namespace ME {

class Material;
class WorldMeta;

#define LANG(_c) global.I18N.Get(_c).c_str()
#define ICON_LANG(_i, _c) std::string(std::string(_i) + " " + global.I18N.Get(_c)).c_str()

#define ME_DESIRED_FRAME_RATE 30.0f
#define ME_MINIMUM_FRAME_RATE 20.0f
#define ME_FLASHL_TIME_IN_MS 333.0f

static const int s_maxLevelColors = 11;
static const ImU32 s_levelColors[s_maxLevelColors] = {IM_COL32(90, 150, 110, 255), IM_COL32(80, 180, 115, 255),  IM_COL32(129, 195, 110, 255), IM_COL32(170, 190, 100, 255),
                                                      IM_COL32(210, 200, 80, 255), IM_COL32(230, 210, 115, 255), IM_COL32(240, 180, 90, 255),  IM_COL32(240, 140, 65, 255),
                                                      IM_COL32(250, 110, 40, 255), IM_COL32(250, 75, 25, 255),   IM_COL32(250, 50, 0, 255)};

static u64 s_timeSinceStatClicked = ME_profiler_get_clock();
static const char* s_statClickedName = 0;
static u32 s_statClickedLevel = 0;

struct pan_and_zoon {
    float m_offset;
    float m_startPan;
    float m_zoom;

    pan_and_zoon() {
        m_offset = 0.0f;
        m_startPan = 0.0f;
        m_zoom = 1.0f;
    }

    inline float w2s(float wld, float minX, float maxX) { return minX + wld * (maxX - minX) * m_zoom - m_offset; }
    inline float w2sdelta(float wld, float minX, float maxX) { return wld * (maxX - minX) * m_zoom; }
    inline float s2w(float scr, float minX, float maxX) { return (scr + m_offset - minX) / ((maxX - minX) * m_zoom); }
};

ME_INLINE void flash_color(ImU32& _drawColor, uint64_t _elapsedTime) {
    ImVec4 white4 = ImColor(IM_COL32_WHITE);

    float msSince = profiler_clock2ms(_elapsedTime, profiler_get_clock_frequency());
    msSince = std::min(msSince, ME_FLASHL_TIME_IN_MS);
    msSince = 1.0f - (msSince / ME_FLASHL_TIME_IN_MS);

    ImVec4 col4 = ImColor(_drawColor);
    _drawColor = ImColor(col4.x + (white4.x - col4.x) * msSince, col4.y + (white4.y - col4.y) * msSince, col4.z + (white4.z - col4.z) * msSince, 255.0f);
}

ME_INLINE void flash_color_named(ImU32& _drawColor, profiler_scope& _cs, uint64_t _elapsedTime) {
    if (s_statClickedName && (strcmp(_cs.m_name, s_statClickedName) == 0) && (_cs.m_level == s_statClickedLevel)) flash_color(_drawColor, _elapsedTime);
}

static_inline ImVec4 tri_color(float _cmp, float _min1, float _min2) {
    ImVec4 col = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    if (_cmp > _min1) col = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    if (_cmp > _min2) col = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    return col;
}

struct frame_info {
    float m_time;
    u32 m_offset;
    u32 m_size;
};

ME_INLINE struct sort_scopes {
    ME_INLINE bool operator()(const profiler_scope& a, const profiler_scope& b) const {
        if (a.m_threadID < b.m_threadID) return true;
        if (b.m_threadID < a.m_threadID) return false;

        if (a.m_level < b.m_level) return true;
        if (b.m_level < a.m_level) return false;

        if (a.m_start < b.m_start) return true;
        if (b.m_start < a.m_start) return false;

        return false;
    }
} customLess;

ME_INLINE struct sort_frame_info_chrono {
    ME_INLINE bool operator()(const frame_info& a, const frame_info& b) const {
        if (a.m_offset < b.m_offset) return true;
        return false;
    }
} customChrono;

ME_INLINE struct sort_frame_info_desc {
    ME_INLINE bool operator()(const frame_info& a, const frame_info& b) const {
        if (a.m_time > b.m_time) return true;
        return false;
    }
} customDesc;

ME_INLINE struct sort_frame_info_asc {
    ME_INLINE bool operator()(const frame_info& a, const frame_info& b) const {
        if (a.m_time < b.m_time) return true;
        return false;
    }
} customAsc;

ME_INLINE struct sort_excusive {
    ME_INLINE bool operator()(const profiler_scope& a, const profiler_scope& b) const { return (a.m_stats->m_exclusiveTimeTotal > b.m_stats->m_exclusiveTimeTotal); }
} customLessExc;

ME_INLINE struct sort_inclusive {
    ME_INLINE bool operator()(const profiler_scope& a, const profiler_scope& b) const { return (a.m_stats->m_inclusiveTimeTotal > b.m_stats->m_inclusiveTimeTotal); }
} customLessInc;

int profiler_draw_frame(profiler_frame* _data, void* _buffer = 0, size_t _bufferSize = 0, bool _inGame = true, bool _multi = false);
void profiler_draw_stats(profiler_frame* _data, bool _multi = false);

class OpenGL3TextureManager {
public:
    ~OpenGL3TextureManager() {
        for (int i = 0; i < mTextures.size(); ++i) {
            GLuint tid = mTextures[i];
            glDeleteTextures(1, &tid);
        }
        mTextures.clear();
    }

    ME_INLINE ImTextureID createTexture(void* pixels, int width, int height) {
        // Upload texture to graphics system
        GLuint texture_id = 0;
        GLint last_texture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_UNPACK_ROW_LENGTH
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glBindTexture(GL_TEXTURE_2D, last_texture);
        mTextures.reserve(mTextures.size() + 1);
        mTextures.push_back(texture_id);
        return (ImTextureID)(intptr_t)texture_id;
    }

    ME_INLINE void deleteTexture(ImTextureID id) {
        GLuint tex = (GLuint)(intptr_t)id;
        glDeleteTextures(1, &tex);
    }

private:
    typedef ImVector<GLuint> Textures;

    Textures mTextures;
};

struct dbgui_base {
public:
    virtual void init() = 0;
    virtual void end() = 0;
    virtual void draw() = 0;
};

enum console_result { OK, ERR, EXIT };

class console : public dbgui_base {

public:
    void init() override;
    void end() override;
    void draw() override;

public:
    void display_full(bool* bInteractingWithTextbox) noexcept;
    void display(bool* bInteractingWithTextbox) noexcept;

    void draw_internal_display() noexcept;

    static void add_to_message_log(const std::string& msg, log_type type) noexcept;
    // static void add_command(const command_type &cmd) noexcept;

    void set_log_colour(ImVec4 colour, log_type type) noexcept;

    void print_command_info(cvar::BaseCommand*);
    std::string execute(std::string Command, std::queue<std::string> args, console_result&);
    bool eval(std::string& cmd);

private:
    friend class logger;

    cvar::ConVar convar;

    // ImVec4 success = {0.0f, 1.0f, 0.0f, 1.0f};
    ImVec4 warning = {1.0f, 1.0f, 0.0f, 1.0f};
    ImVec4 error = {1.0f, 0.0f, 0.0f, 1.0f};
    ImVec4 note = ME_rgba2imvec(0, 183, 255, 255);
    ImVec4 message = {1.0f, 1.0f, 1.0f, 1.0f};
};

class pack_editor : public dbgui_base {
private:
    // pack editor
    ME_pack_reader pack_reader;
    bool pack_reader_is_loaded = false;
    u8 majorVersion;
    u8 minorVersion;
    u8 patchVersion;
    bool isLittleEndian;
    u64 itemCount;
    ME_pack_result result;
    std::string file;

    // file
    bool filebrowser = false;

public:
    void init() override;
    void end() override;
    void draw() override;
};

enum ImGuiWindowTags {
    UI_None = 0,
    UI_MainMenu = 1 << 0,
    UI_GCManager = 1 << 1,
};

enum EditorTags { Editor_Code = 0, Editor_Markdown = 1 };

class code_editor : public dbgui_base {
    struct EditorView {
        EditorTags tags;

        std::string file;
        std::string content;
        bool is_edited = false;

        bool operator==(EditorView v) { return (v.file == this->file) && (v.tags == this->tags); }
    };

    std::vector<EditorView> view_contents;
    TextEditor text_editor;
    EditorView* view_editing = nullptr;

    bool filebrowser = false;

public:
    void init() override;
    void end() override;
    void draw() override;
};

using dbgui_ref = ref<dbgui_base>;

class dbgui {
private:
    ImGuiContext* m_imgui = nullptr;
    ImGuiID dockspace_id = 0;

    std::vector<dbgui_base*> dbgui_list = {};

private:
    static void (*RendererShutdownFunction)();
    static void (*PlatformShutdownFunction)();
    static void (*RendererNewFrameFunction)();
    static void (*PlatformNewFrameFunction)();
    static void (*RenderFunction)(ImDrawData*);

public:
    dbgui();

    void Init();
    void End();
    void NewFrame();
    void Draw();
    void Update();

    ImVec2 NextWindows(ImGuiWindowTags tag, ImVec2 pos) const noexcept;

    ImGuiID GetMainDockID() const noexcept { return dockspace_id; }

    ImGuiContext* getImGuiCtx() const noexcept {
        ME_ASSERT(m_imgui, "Miss ImGuiContext");
        return m_imgui;
    }
};

}  // namespace ME

#endif
