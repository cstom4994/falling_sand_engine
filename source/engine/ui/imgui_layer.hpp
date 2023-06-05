// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_IMGUILAYER_HPP
#define ME_IMGUILAYER_HPP

#include <inttypes.h>

#include <algorithm>
#include <array>
#include <regex>
#include <string>
#include <vector>

#include "engine/audio/audio.h"
#include "engine/core/debug.hpp"
#include "engine/core/macros.hpp"
#include "engine/core/memory.h"
#include "engine/core/profiler.hpp"
#include "engine/core/sdl_wrapper.h"
#include "engine/game_datastruct.hpp"
#include "engine/ui/console.h"
#include "engine/ui/file_browser.h"
#include "engine/ui/imgui_impl.hpp"
#include "engine/ui/pack_editor.h"
#include "libs/imgui/text_editor.h"

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

void profiler_draw_frame_navigation(frame_info* _infos, u32 _numInfos);
int profiler_draw_frame(profiler_frame* _data, void* _buffer = 0, size_t _bufferSize = 0, bool _inGame = true, bool _multi = false);
void profiler_draw_stats(profiler_frame* _data, bool _multi = false);

namespace ME {

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
}  // namespace ME

class Material;
class WorldMeta;

enum ImGuiWindowTags {

    UI_None = 0,
    UI_MainMenu = 1 << 0,
    UI_GCManager = 1 << 1,
};

enum EditorTags { Editor_Code = 0, Editor_Markdown = 1 };

class ImGuiLayer {
private:
    struct EditorView {
        EditorTags tags;

        std::string file;
        std::string content;
        bool is_edited = false;

        bool operator==(EditorView v) { return (v.file == this->file) && (v.tags == this->tags); }
    };

    ImGuiContext* m_imgui = nullptr;

    std::vector<EditorView> view_contents;
    TextEditor editor;
    EditorView* view_editing = nullptr;
    ImGui::FileBrowser fileDialog;
    ImGuiID dockspace_id;

    // console
    ME::MEconsole console;

    // pack editor
    PackEditor m_pack_editor;

private:
    static void (*RendererShutdownFunction)();
    static void (*PlatformShutdownFunction)();
    static void (*RendererNewFrameFunction)();
    static void (*PlatformNewFrameFunction)();
    static void (*RenderFunction)(ImDrawData*);

public:
    ImGuiLayer();
    void Init();
    void End();
    void NewFrame();
    void Draw();
    void Update();
    const ImVec2 NextWindows(ImGuiWindowTags tag, ImVec2 pos);
    const ImGuiID GetMainDockID() { return dockspace_id; }
    const ImGuiContext* getImGuiCtx() {
        ME_ASSERT(m_imgui, "Miss ImGuiContext");
        return m_imgui;
    }
};

#endif
