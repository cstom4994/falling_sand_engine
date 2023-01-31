

#ifndef METADOT_PROFILER_IMGUI_H
#define METADOT_PROFILER_IMGUI_H

#include <inttypes.h>

#include <algorithm>

#include "core/alloc.hpp"
#include "core/macros.h"
#include "imgui/imgui_core.hpp"
#include "profiler.h"

#define METADOT_DESIRED_FRAME_RATE 30.0f
#define METADOT_MINIMUM_FRAME_RATE 20.0f
#define METADOT_FLASHL_TIME_IN_MS 333.0f

static const int s_maxLevelColors = 11;
static const ImU32 s_levelColors[s_maxLevelColors] = {IM_COL32(90, 150, 110, 255), IM_COL32(80, 180, 115, 255),  IM_COL32(129, 195, 110, 255), IM_COL32(170, 190, 100, 255),
                                                      IM_COL32(210, 200, 80, 255), IM_COL32(230, 210, 115, 255), IM_COL32(240, 180, 90, 255),  IM_COL32(240, 140, 65, 255),
                                                      IM_COL32(250, 110, 40, 255), IM_COL32(250, 75, 25, 255),   IM_COL32(250, 50, 0, 255)};

static uint64_t s_timeSinceStatClicked = ProfilerGetClock();
static const char* s_statClickedName = 0;
static uint32_t s_statClickedLevel = 0;

struct PanAndZoon {
    float m_offset;
    float m_startPan;
    float m_zoom;

    PanAndZoon() {
        m_offset = 0.0f;
        m_startPan = 0.0f;
        m_zoom = 1.0f;
    }

    inline float w2s(float wld, float minX, float maxX) { return minX + wld * (maxX - minX) * m_zoom - m_offset; }
    inline float w2sdelta(float wld, float minX, float maxX) { return wld * (maxX - minX) * m_zoom; }
    inline float s2w(float scr, float minX, float maxX) { return (scr + m_offset - minX) / ((maxX - minX) * m_zoom); }
};

template <typename T>
static_inline T ProfilerMax(T _v1, T _v2) {
    return _v1 > _v2 ? _v1 : _v2;
}
template <typename T>
static_inline T ProfilerMin(T _v1, T _v2) {
    return _v1 < _v2 ? _v1 : _v2;
}

METADOT_INLINE void flashColor(ImU32& _drawColor, uint64_t _elapsedTime) {
    ImVec4 white4 = ImColor(IM_COL32_WHITE);

    float msSince = ProfilerClock2ms(_elapsedTime, ProfilerGetClockFrequency());
    msSince = ProfilerMin(msSince, METADOT_FLASHL_TIME_IN_MS);
    msSince = 1.0f - (msSince / METADOT_FLASHL_TIME_IN_MS);

    ImVec4 col4 = ImColor(_drawColor);
    _drawColor = ImColor(col4.x + (white4.x - col4.x) * msSince, col4.y + (white4.y - col4.y) * msSince, col4.z + (white4.z - col4.z) * msSince, 255.0f);
}

METADOT_INLINE void flashColorNamed(ImU32& _drawColor, ProfilerScope& _cs, uint64_t _elapsedTime) {
    if (s_statClickedName && (strcmp(_cs.m_name, s_statClickedName) == 0) && (_cs.m_level == s_statClickedLevel)) flashColor(_drawColor, _elapsedTime);
}

static_inline ImVec4 triColor(float _cmp, float _min1, float _min2) {
    ImVec4 col = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    if (_cmp > _min1) col = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    if (_cmp > _min2) col = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    return col;
}

struct FrameInfo {
    float m_time;
    uint32_t m_offset;
    uint32_t m_size;
};

METADOT_INLINE struct SortScopes {
    METADOT_INLINE bool operator()(const ProfilerScope& a, const ProfilerScope& b) const {
        if (a.m_threadID < b.m_threadID) return true;
        if (b.m_threadID < a.m_threadID) return false;

        if (a.m_level < b.m_level) return true;
        if (b.m_level < a.m_level) return false;

        if (a.m_start < b.m_start) return true;
        if (b.m_start < a.m_start) return false;

        return false;
    }
} customLess;

METADOT_INLINE struct SortFrameInfoChrono {
    METADOT_INLINE bool operator()(const FrameInfo& a, const FrameInfo& b) const {
        if (a.m_offset < b.m_offset) return true;
        return false;
    }
} customChrono;

METADOT_INLINE struct SortFrameInfoDesc {
    METADOT_INLINE bool operator()(const FrameInfo& a, const FrameInfo& b) const {
        if (a.m_time > b.m_time) return true;
        return false;
    }
} customDesc;

METADOT_INLINE struct SortFrameInfoAsc {
    METADOT_INLINE bool operator()(const FrameInfo& a, const FrameInfo& b) const {
        if (a.m_time < b.m_time) return true;
        return false;
    }
} customAsc;

METADOT_INLINE struct sortExcusive {
    METADOT_INLINE bool operator()(const ProfilerScope& a, const ProfilerScope& b) const { return (a.m_stats->m_exclusiveTimeTotal > b.m_stats->m_exclusiveTimeTotal); }
} customLessExc;

METADOT_INLINE struct sortInclusive {
    METADOT_INLINE bool operator()(const ProfilerScope& a, const ProfilerScope& b) const { return (a.m_stats->m_inclusiveTimeTotal > b.m_stats->m_inclusiveTimeTotal); }
} customLessInc;

void ProfilerDrawFrameNavigation(FrameInfo* _infos, uint32_t _numInfos);

/* Draws a frame capture inspector dialog using ImGui. */
/* _data       - [in/out] profiler data / single frame capture. User is responsible to release memory using ProfilerRelease */
/* _buffer     - buffer to store data to */
/* _bufferSize - maximum size of buffer, in bytes */
/* Returns: if frame was saved, returns number of bytes written - see ProfilerSave. */
int ProfilerDrawFrame(ProfilerFrame* _data, void* _buffer = 0, size_t _bufferSize = 0, bool _inGame = true, bool _multi = false);

/* Draws a frame capture statistics using ImGui. */
/* NB: frame data **MUST** be processed (done in ProfilerLoad) before using this function. */
/* _data       - [in/out] profiler data / single frame capture. User is responsible to release memory using ProfilerRelease */
void ProfilerDrawStats(ProfilerFrame* _data, bool _multi = false);

#endif  // METADOT_DRAW_H
