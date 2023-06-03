// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "imgui_layer.hpp"

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <map>
#include <string>
#include <type_traits>

#include "engine/audio/audio.h"
#include "engine/chunk.hpp"
#include "engine/core/const.h"
#include "engine/core/core.hpp"
#include "engine/core/cpp/utils.hpp"
#include "engine/core/dbgtools.h"
#include "engine/core/debug.hpp"
#include "engine/core/global.hpp"
#include "engine/core/io/filesystem.h"
#include "engine/core/macros.hpp"
#include "engine/core/memory.h"
#include "engine/core/utils/utility.hpp"
#include "engine/engine.h"
#include "engine/game.hpp"
#include "engine/game_datastruct.hpp"
#include "engine/game_ui.hpp"
#include "engine/meta/reflection.hpp"
#include "engine/meta/static_relfection.hpp"
#include "engine/npc.hpp"
#include "engine/reflectionflat.hpp"
#include "engine/renderer/gpu.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/scripting/lua_wrapper.hpp"
#include "engine/scripting/scripting.hpp"
#include "engine/ui/imgui_impl.hpp"
#include "engine/ui/ui.hpp"
#include "libs/glad/glad.h"
#include "libs/imgui/font_awesome.h"
#include "libs/imgui/imgui.h"

IMPLENGINE();

#define LANG(_c) global.I18N.Get(_c).c_str()
#define ICON_LANG(_i, _c) std::string(std::string(_i) + " " + global.I18N.Get(_c)).c_str()

void profiler_draw_frame_bavigation(frame_info *_infos, uint32_t _numInfos) {
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1510.0f, 140.0f), ImGuiCond_FirstUseEver);

    ImGui::Begin("Frame navigator", 0, ImGuiWindowFlags_NoScrollbar);

    static int sortKind = 0;
    ImGui::Text("Sort frames by:  ");
    ImGui::SameLine();
    ImGui::RadioButton("Chronological", &sortKind, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Descending", &sortKind, 1);
    ImGui::SameLine();
    ImGui::RadioButton("Ascending", &sortKind, 2);
    ImGui::Separator();

    switch (sortKind) {
        case 0:
            std::sort(&_infos[0], &_infos[_numInfos], customChrono);
            break;

        case 1:
            std::sort(&_infos[0], &_infos[_numInfos], customDesc);
            break;

        case 2:
            std::sort(&_infos[0], &_infos[_numInfos], customAsc);
            break;
    };

    float maxTime = 0;
    for (uint32_t i = 0; i < _numInfos; ++i) {
        if (maxTime < _infos[i].m_time) maxTime = _infos[i].m_time;
    }

    const ImVec2 s = ImGui::GetWindowSize();
    const ImVec2 p = ImGui::GetWindowPos();

    ImGui::BeginChild("", ImVec2(s.x, 70), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);

    ImGui::PlotHistogram("", (const float *)_infos, _numInfos, 0, "", 0.f, maxTime, ImVec2(_numInfos * 10, 50), sizeof(frame_info));

    // if (ImGui::IsMouseClicked(0) && (idx != -1)) {
    //     profilerFrameLoad(g_fileName, _infos[idx].m_offset, _infos[idx].m_size);
    // }

    ImGui::EndChild();

    ImGui::End();
}

int profiler_draw_frame(profiler_frame *_data, void *_buffer, size_t _bufferSize, bool _inGame, bool _multi) {
    int ret = 0;

    // if (fabs(_data->m_startTime - _data->m_endtime) == 0.0f) return ret;

    std::sort(&_data->m_scopes[0], &_data->m_scopes[_data->m_numScopes], customLess);

    ImGui::SetNextWindowPos(ImVec2(10.0f, _multi ? 160.0f : 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(900.0f, 480.0f), ImGuiCond_FirstUseEver);

    static bool pause = false;
    bool noMove = (pause && _inGame) || !_inGame;
    noMove = noMove && ImGui::GetIO().KeyCtrl;

    ME_PRIVATE(ImVec2) winpos = ImGui::GetWindowPos();

    if (noMove) ImGui::SetNextWindowPos(winpos);

    ImGui::Begin(LANG("ui_profiler"), 0, noMove ? ImGuiWindowFlags_NoMove : 0);

    ImGui::BeginTabBar("ui_profiler_tabbar");

    if (ImGui::BeginTabItem(CC("帧监测"))) {

        if (!noMove) winpos = ImGui::GetWindowPos();

        float deltaTime = profiler_clock2ms(_data->m_endtime - _data->m_startTime, _data->m_CPUFrequency);
        float frameRate = 1000.0f / deltaTime;

        ImVec4 col = tri_color(frameRate, ME_MINIMUM_FRAME_RATE, ME_DESIRED_FRAME_RATE);

        ImGui::Text("FPS: ");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::Text("%.1f    ", 1000.0f / deltaTime);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("%s", CC("帧耗时"));
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::Text("%.3f ms   ", deltaTime);
        ImGui::PopStyleColor();
        ImGui::SameLine();

        if (_inGame) {
            ImGui::Text("平均帧率: ");
            ImGui::PushStyleColor(ImGuiCol_Text, tri_color(ImGui::GetIO().Framerate, ME_MINIMUM_FRAME_RATE, ME_DESIRED_FRAME_RATE));
            ImGui::SameLine();
            ImGui::Text("%.1f   ", ImGui::GetIO().Framerate);
            ImGui::PopStyleColor();
        } else {
            float prevFrameTime = profiler_clock2ms(_data->m_prevFrameTime, _data->m_CPUFrequency);
            ImGui::SameLine();
            ImGui::Text("上一帧: ");
            ImGui::PushStyleColor(ImGuiCol_Text, tri_color(1000.0f / prevFrameTime, ME_MINIMUM_FRAME_RATE, ME_DESIRED_FRAME_RATE));
            ImGui::SameLine();
            ImGui::Text("%.3f ms  %.2f fps   ", prevFrameTime, 1000.0f / prevFrameTime);
            ImGui::PopStyleColor();
            ImGui::SameLine();

            // ImGui::Text("Platform: ");
            // ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 1.0f, 1.0f));
            // ImGui::SameLine();
            // ImGui::Text("%s   ", ProfilerGetPlatformName(_data->m_platformID));
            // ImGui::PopStyleColor();
        }

        if (_inGame) {
            ImGui::SameLine();
            ImGui::Checkbox("暂停捕获   ", &pause);
        }

        bool resetZoom = false;
        static float threshold = 0.0f;
        static int thresholdLevel = 0;

        if (_inGame) {
            ImGui::PushItemWidth(210);
            ImGui::SliderFloat("阈值   ", &threshold, 0.0f, 15.0f);

            ImGui::SameLine();
            ImGui::PushItemWidth(120);
            ImGui::SliderInt("阈值级别", &thresholdLevel, 0, 23);

            ImGui::SameLine();
            if (ImGui::Button("保存帧")) ret = ME_profiler_save(_data, _buffer, _bufferSize);
        } else {
            ImGui::Text("捕获阈值: ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0, 1.0f, 1.0f, 1.0f), "%.2f ", _data->m_timeThreshold);
            ImGui::SameLine();
            ImGui::Text("ms   ");
            ImGui::SameLine();
            ImGui::Text("门限等级: ");
            ImGui::SameLine();
            if (_data->m_levelThreshold == 0)
                ImGui::TextColored(ImVec4(0, 1.0f, 1.0f, 1.0f), "整帧   ");
            else
                ImGui::TextColored(ImVec4(0, 1.0f, 1.0f, 1.0f), "%d   ", _data->m_levelThreshold);
        }

        ImGui::SameLine();
        resetZoom = ImGui::Button("重置缩放和平移");

        const ImVec2 p = ImGui::GetCursorScreenPos();
        const ImVec2 s = ImGui::GetWindowSize();

        float frameStartX = p.x + 3.0f;
        float frameEndX = frameStartX + s.x - 23;
        float frameStartY = p.y;

        static pan_and_zoon paz;

        ImVec2 mpos = ImGui::GetMousePos();

        if (ImGui::IsMouseDragging(0) && ImGui::GetIO().KeyCtrl) {
            paz.m_offset -= (mpos.x - paz.m_startPan);
            paz.m_startPan = mpos.x;
        } else
            paz.m_startPan = mpos.x;

        float mXpre = paz.s2w(mpos.x, frameStartX, frameEndX);

        if (ImGui::GetIO().KeyCtrl) paz.m_zoom += ImGui::GetIO().MouseWheel / 30.0f;
        if (ImGui::GetIO().KeysDown[65] && ImGui::IsWindowHovered())  // 'a'
            paz.m_zoom *= 1.1f;
        if (ImGui::GetIO().KeysDown[90] && ImGui::IsWindowHovered())  // 'z'
            paz.m_zoom /= 1.1f;

        paz.m_zoom = std::max(paz.m_zoom, 1.0f);

        float mXpost = paz.s2w(mpos.x, frameStartX, frameEndX);
        float mXdelta = mXpost - mXpre;

        paz.m_offset -= paz.w2sdelta(mXdelta, frameStartX, frameEndX);

        // snap to edge
        float leX = paz.w2s(0.0f, frameStartX, frameEndX);
        float reX = paz.w2s(1.0f, frameStartX, frameEndX);

        if (leX > frameStartX) paz.m_offset += leX - frameStartX;
        if (reX < frameEndX) paz.m_offset -= frameEndX - reX;

        if (resetZoom || (_inGame ? !pause : false)) {
            paz.m_offset = 0.0f;
            paz.m_zoom = 1.0f;
        }

        ME_profiler_set_paused(pause);
        ME_profiler_set_threshold(threshold, thresholdLevel);

        static const int ME_MAX_FRAME_TIMES = 128;
        static float s_frameTimes[ME_MAX_FRAME_TIMES];
        static int s_currentFrame = 0;

        float maxFrameTime = 0.0f;
        if (_inGame) {
            frameStartY += 62.0f;

            if (s_currentFrame == 0) memset(s_frameTimes, 0, sizeof(s_frameTimes));

            if (!ME_profiler_is_paused()) {
                s_frameTimes[s_currentFrame % ME_MAX_FRAME_TIMES] = deltaTime;
                ++s_currentFrame;
            }

            float frameTimes[ME_MAX_FRAME_TIMES];
            for (int i = 0; i < ME_MAX_FRAME_TIMES; ++i) {
                frameTimes[i] = s_frameTimes[(s_currentFrame + i) % ME_MAX_FRAME_TIMES];
                maxFrameTime = std::max(maxFrameTime, frameTimes[i]);
            }

            ImGui::Separator();
            ImGui::PlotHistogram("", frameTimes, ME_MAX_FRAME_TIMES, 0, "", 0.f, maxFrameTime, ImVec2(s.x - 9.0f, 45));
        } else {
            frameStartY += 12.0f;
            ImGui::Separator();
        }

        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        maxFrameTime = std::max(maxFrameTime, 0.001f);
        float pct30fps = 33.33f / maxFrameTime;
        float pct60fps = 16.66f / maxFrameTime;

        float minHistY = p.y + 6.0f;
        float maxHistY = p.y + 45.0f;

        float limit30Y = maxHistY - (maxHistY - minHistY) * pct30fps;
        float limit60Y = maxHistY - (maxHistY - minHistY) * pct60fps;

        if (pct30fps <= 1.0f) draw_list->AddLine(ImVec2(frameStartX - 3.0f, limit30Y), ImVec2(frameEndX + 3.0f, limit30Y), IM_COL32(255, 255, 96, 255));

        if (pct60fps <= 1.0f) draw_list->AddLine(ImVec2(frameStartX - 3.0f, limit60Y), ImVec2(frameEndX + 3.0f, limit60Y), IM_COL32(96, 255, 96, 255));

        if (_data->m_numScopes == 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.23f, 0.23f, 1.0f), "没有范围数据!");
            ImGui::End();
            return ret;
        }

        uint64_t threadID = _data->m_scopes[0].m_threadID;
        bool writeThreadName = true;

        uint64_t totalTime = _data->m_endtime - _data->m_startTime;

        float barHeight = 21.0f;
        float bottom = 0.0f;

        uint64_t currTime = ME_profiler_get_clock();

        for (uint32_t i = 0; i < _data->m_numScopes; ++i) {
            profiler_scope &cs = _data->m_scopes[i];
            if (!cs.m_name) continue;

            if (cs.m_threadID != threadID) {
                threadID = cs.m_threadID;
                frameStartY = bottom + barHeight;
                writeThreadName = true;
            }

            if (writeThreadName) {
                ImVec2 tlt = ImVec2(frameStartX, frameStartY);
                ImVec2 brt = ImVec2(frameEndX, frameStartY + barHeight);

                draw_list->PushClipRect(tlt, brt, true);
                draw_list->AddRectFilled(tlt, brt, IM_COL32(45, 45, 60, 255));
                const char *threadName = "Unnamed thread";
                for (uint32_t j = 0; j < _data->m_numThreads; ++j)
                    if (_data->m_threads[j].m_threadID == threadID) {
                        threadName = _data->m_threads[j].m_name;
                        break;
                    }
                tlt.x += 3;
                char buffer[512];
                snprintf(buffer, 512, "%s  -  0x%" PRIx64, threadName, threadID);
                draw_list->AddText(tlt, IM_COL32(255, 255, 255, 255), buffer);
                draw_list->PopClipRect();

                frameStartY += barHeight;
                writeThreadName = false;
            }

            // handle wrap around
            int64_t sX = int64_t(cs.m_start - _data->m_startTime);
            if (sX < 0) sX = -sX;
            int64_t eX = int64_t(cs.m_end - _data->m_startTime);
            if (eX < 0) eX = -eX;

            float startXpct = float(sX) / float(totalTime);
            float endXpct = float(eX) / float(totalTime);

            float startX = paz.w2s(startXpct, frameStartX, frameEndX);
            float endX = paz.w2s(endXpct, frameStartX, frameEndX);

            ImVec2 tl = ImVec2(startX, frameStartY + cs.m_level * (barHeight + 1.0f));
            ImVec2 br = ImVec2(endX, frameStartY + cs.m_level * (barHeight + 1.0f) + barHeight);

            bottom = std::max(bottom, br.y);

            int level = cs.m_level;
            if (cs.m_level >= s_maxLevelColors) level = s_maxLevelColors - 1;

            ImU32 drawColor = s_levelColors[level];
            flash_color_named(drawColor, cs, currTime - s_timeSinceStatClicked);

            if (ImGui::IsMouseClicked(0) && ImGui::IsMouseHoveringRect(tl, br) && ImGui::IsWindowHovered()) {
                s_timeSinceStatClicked = currTime;
                s_statClickedName = cs.m_name;
                s_statClickedLevel = cs.m_level;
            }

            if ((thresholdLevel == (int)cs.m_level + 1) && (threshold <= profiler_clock2ms(cs.m_end - cs.m_start, _data->m_CPUFrequency))) flash_color(drawColor, currTime - _data->m_endtime);

            draw_list->PushClipRect(tl, br, true);
            draw_list->AddRectFilled(tl, br, drawColor);
            tl.x += 3;
            draw_list->AddText(tl, IM_COL32(0, 0, 0, 255), cs.m_name);
            draw_list->PopClipRect();

            if (ImGui::IsMouseHoveringRect(tl, br) && ImGui::IsWindowHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextColored(ImVec4(255, 255, 0, 255), "%s", cs.m_name);
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0, 255, 255, 255), "Time: ");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(230, 230, 230, 255), "%.3f ms", profiler_clock2ms(cs.m_end - cs.m_start, _data->m_CPUFrequency));
                ImGui::TextColored(ImVec4(0, 255, 255, 255), "File: ");
                ImGui::SameLine();
                ImGui::Text("%s", cs.m_file);
                ImGui::TextColored(ImVec4(0, 255, 255, 255), "%s", "Line: ");
                ImGui::SameLine();
                ImGui::Text("%d", cs.m_line);
                ImGui::EndTooltip();
            }
        }

        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("内存")) {
        ImGui::Text("MemCurrentUsage: %.2f mb", ME_mem_current_usage_mb());
        ImGui::Text("MemTotalAllocated: %.2lf mb", (f64)(g_allocation_metrics.total_allocated / 1048576.0));
        ImGui::Text("MemTotalFree: %.2lf mb", (f64)(g_allocation_metrics.total_free / 1048576.0));
        ImGui::Text("GC MemAllocInUsed: %.2lf mb", (f64)(ME_memory_bytes_inuse() / 1048576.0));

#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049

        GLint total_mem_kb = 0;
        glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX, &total_mem_kb);

        GLint cur_avail_mem_kb = 0;
        glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX, &cur_avail_mem_kb);

        ImGui::Text("GPU MemTotalAvailable: %.2lf mb", (f64)(cur_avail_mem_kb / 1024.0f));
        ImGui::Text("GPU MemCurrentUsage: %.2lf mb", (f64)((total_mem_kb - cur_avail_mem_kb) / 1024.0f));

        // #if defined(ME_DEBUG)
        //         ImGui::Dummy(ImVec2(0.0f, 10.0f));
        //         ImGui::Text("GC:\n");
        //         for (auto [name, size] : allocation_metrics::MemoryDebugMap) {
        //             ImGui::Text("%s", ME::Format("   {0} {1}", name, size).c_str());
        //         }
        //         // ImGui::Auto(allocation_metrics::MemoryDebugMap, "map");
        // #endif

        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();

    ImGui::End();
    return ret;
}

void profiler_draw_stats(profiler_frame *_data, bool _multi) {
    ImGui::SetNextWindowPos(ImVec2(920.0f, _multi ? 160.0f : 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600.0f, 900.0f), ImGuiCond_FirstUseEver);

    ImGui::Begin("Frame stats");

    if (_data->m_numScopes == 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.23f, 0.23f, 1.0f), "No scope data!");
        ImGui::End();
        return;
    }

    float deltaTime = profiler_clock2ms(_data->m_endtime - _data->m_startTime, _data->m_CPUFrequency);

    static int exclusive = 0;
    ImGui::Text("Sort by:  ");
    ImGui::SameLine();
    ImGui::RadioButton("Exclusive time", &exclusive, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Inclusive time", &exclusive, 1);
    ImGui::Separator();

    if (exclusive == 0)
        std::sort(&_data->m_scopesStats[0], &_data->m_scopesStats[_data->m_numScopesStats], customLessExc);
    else
        std::sort(&_data->m_scopesStats[0], &_data->m_scopesStats[_data->m_numScopesStats], customLessInc);

    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImVec2 s = ImGui::GetWindowSize();

    float frameStartX = p.x + 3.0f;
    float frameEndX = frameStartX + s.x - 23;
    float frameStartY = p.y + 21.0f;

    uint64_t totalTime = 0;
    if (exclusive == 0)
        totalTime = _data->m_scopesStats[0].m_stats->m_exclusiveTimeTotal;
    else
        totalTime = _data->m_scopesStats[0].m_stats->m_inclusiveTimeTotal;

    float barHeight = 21.0f;

    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    for (uint32_t i = 0; i < _data->m_numScopesStats; i++) {
        profiler_scope &cs = _data->m_scopesStats[i];

        float endXpct = float(cs.m_stats->m_exclusiveTimeTotal) / float(totalTime);
        if (exclusive == 1) endXpct = float(cs.m_stats->m_inclusiveTimeTotal) / float(totalTime);

        float startX = frameStartX;
        float endX = frameStartX + endXpct * (frameEndX - frameStartX);

        ImVec2 tl = ImVec2(startX, frameStartY);
        ImVec2 br = ImVec2(endX, frameStartY + barHeight);

        int colIdx = s_maxLevelColors - 1 - i;
        if (colIdx < 0) colIdx = 0;
        ImU32 drawColor = s_levelColors[colIdx];

        ImVec2 brE = ImVec2(frameEndX, frameStartY + barHeight);
        bool hoverRow = ImGui::IsMouseHoveringRect(tl, brE);

        if (ImGui::IsMouseClicked(0) && hoverRow && ImGui::IsWindowHovered()) {
            s_timeSinceStatClicked = ME_profiler_get_clock();
            s_statClickedName = cs.m_name;
            s_statClickedLevel = cs.m_level;
        }

        flash_color_named(drawColor, cs, ME_profiler_get_clock() - s_timeSinceStatClicked);

        char buffer[1024];
        snprintf(buffer, 1024, "[%d] %s", cs.m_stats->m_occurences, cs.m_name);
        draw_list->PushClipRect(tl, br, true);
        draw_list->AddRectFilled(tl, br, drawColor);
        draw_list->AddText(tl, IM_COL32(0, 0, 0, 255), buffer);
        draw_list->AddText(ImVec2(startX + 1, frameStartY + 1), IM_COL32(255, 255, 255, 255), buffer);
        draw_list->PopClipRect();

        if (hoverRow && ImGui::IsWindowHovered()) {
            ImVec2 htl = ImVec2(endX, frameStartY);
            draw_list->PushClipRect(htl, brE, true);
            draw_list->AddRectFilled(htl, brE, IM_COL32(64, 64, 64, 255));
            draw_list->AddText(tl, IM_COL32(0, 0, 0, 255), buffer);
            draw_list->AddText(ImVec2(startX + 1, frameStartY + 1), IM_COL32(192, 192, 192, 255), buffer);
            draw_list->PopClipRect();

            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(255, 255, 0, 255), "%s", cs.m_name);
            ImGui::Separator();

            float ttime;
            if (exclusive == 0) {
                ImGui::TextColored(ImVec4(0, 255, 255, 255), "Exclusive time total: ");
                ImGui::SameLine();
                ttime = profiler_clock2ms(cs.m_stats->m_exclusiveTimeTotal, _data->m_CPUFrequency);
                ImGui::TextColored(ImVec4(230, 230, 230, 255), "%.4f ms", ttime);
            } else {
                ImGui::TextColored(ImVec4(0, 255, 255, 255), "Inclusive time total: ");
                ImGui::SameLine();
                ttime = profiler_clock2ms(cs.m_stats->m_inclusiveTimeTotal, _data->m_CPUFrequency);
                ImGui::TextColored(ImVec4(230, 230, 230, 255), "%.4f ms", ttime);
            }

            ImGui::TextColored(ImVec4(0, 255, 255, 255), "Of frame: ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(230, 230, 230, 255), "%2.2f %%", 100.0f * ttime / deltaTime);

            ImGui::TextColored(ImVec4(0, 255, 255, 255), "File: ");
            ImGui::SameLine();
            ImGui::Text("%s", cs.m_file);

            ImGui::TextColored(ImVec4(0, 255, 255, 255), "Line: ");
            ImGui::SameLine();
            ImGui::Text("%d", cs.m_line);

            ImGui::TextColored(ImVec4(0, 255, 255, 255), "Count: ");
            ImGui::SameLine();
            ImGui::Text("%d", cs.m_stats->m_occurences);

            ImGui::EndTooltip();
        }

        frameStartY += 1.0f + barHeight;
    }

    ImGui::End();
}

inline void ImGuiInitStyle(const float pixel_ratio, const float dpi_scaling) {
    auto &style = ImGui::GetStyle();
    auto &colors = style.Colors;

    ImGui::StyleColorsDark();

    colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};

    // Headers
    colors[ImGuiCol_Header] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_HeaderHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_HeaderActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Buttons
    colors[ImGuiCol_Button] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_ButtonHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_ButtonActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_FrameBgHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_FrameBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
    colors[ImGuiCol_TabActive] = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
    colors[ImGuiCol_TabUnfocused] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Rounding
    // style.WindowPadding = ImVec2(4.0f, 4.0f);
    // style.FramePadding = ImVec2(6.0f, 4.0f);
    // style.WindowRounding = 10.0f;
    // style.ChildRounding = 4.0f;
    // style.FrameRounding = 4.0f;
    // style.GrabRounding = 4.0f;
}

#if defined(ME_IMM32)

#if defined(_WIN32)
#include <CommCtrl.h>
#include <Windows.h>
#include <vcruntime_string.h>
#endif

static int common_control_initialize() {
    HMODULE comctl32 = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"Comctl32.dll", &comctl32)) {
        return EXIT_FAILURE;
    }

    assert(comctl32 != nullptr);
    if (comctl32) {
        {
            typename std::add_pointer<decltype(InitCommonControlsEx)>::type lpfnInitCommonControlsEx =
                    reinterpret_cast<typename std::add_pointer<decltype(InitCommonControlsEx)>::type>(GetProcAddress(comctl32, "InitCommonControlsEx"));

            if (lpfnInitCommonControlsEx) {
                const INITCOMMONCONTROLSEX initcommoncontrolsex = {sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES};
                if (!lpfnInitCommonControlsEx(&initcommoncontrolsex)) {
                    assert(!" InitCommonControlsEx(&initcommoncontrolsex) ");
                    return EXIT_FAILURE;
                }
                // OutputDebugStringW(L"initCommonControlsEx Enable\n");
                return 0;
            }
        }
        {
            InitCommonControls();
            // OutputDebugStringW(L"initCommonControls Enable\n");
            return 0;
        }
    }
    return 1;
}

#endif

const ImVec2 ImGuiLayer::NextWindows(ImGuiWindowTags tag, ImVec2 pos) {
    if (tag & UI_MainMenu) ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImVec2 windowspos = ImGui::GetPlatformIO().Platform_GetWindowPos(ImGui::GetMainViewport());
        pos += windowspos;
    }
    return pos;
}

void (*ImGuiLayer::RendererShutdownFunction)();
void (*ImGuiLayer::PlatformShutdownFunction)();
void (*ImGuiLayer::RendererNewFrameFunction)();
void (*ImGuiLayer::PlatformNewFrameFunction)();
void (*ImGuiLayer::RenderFunction)(ImDrawData *);

ImGuiLayer::ImGuiLayer() {
    RendererShutdownFunction = ImGui_ImplOpenGL3_Shutdown;
    PlatformShutdownFunction = ImGui_ImplSDL2_Shutdown;
    RendererNewFrameFunction = ImGui_ImplOpenGL3_NewFrame;
    PlatformNewFrameFunction = ImGui_ImplSDL2_NewFrame;
    RenderFunction = ImGui_ImplOpenGL3_RenderDrawData;
}

#if defined(ME_IMM32)
ME_imgui_imm imguiIMMCommunication{};
#endif

ME_PRIVATE(void *) ImGuiMalloc(size_t sz, void *user_data) { return ME_MALLOC(sz); }
ME_PRIVATE(void) ImGuiFree(void *ptr, void *user_data) { ME_FREE(ptr); }

void ImGuiLayer::Init() {

    IMGUI_CHECKVERSION();

    ImGui::SetAllocatorFunctions(ImGuiMalloc, ImGuiFree);

    m_imgui = ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    io.IniFilename = "imgui.ini";

    ImFontConfig config;
    config.OversampleH = 1;
    config.OversampleV = 1;
    config.PixelSnapH = 1;

    f32 scale = 1.0f;

    io.Fonts->AddFontFromFileTTF(METADOT_RESLOC("data/assets/fonts/fusion-pixel.ttf"), 12.0f, &config, io.Fonts->GetGlyphRangesChineseFull());

    {
        // Font Awesome
        static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        config.OversampleH = 2;
        config.OversampleV = 2;
        icons_config.PixelSnapH = true;
        io.Fonts->AddFontFromFileTTF(METADOT_RESLOC("data/assets/fonts/fa-solid-900.ttf"), 13.0f, &icons_config, icons_ranges);
    }
    {
        // Font Awesome
        static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        config.OversampleH = 2;
        config.OversampleV = 2;
        icons_config.PixelSnapH = true;
        io.Fonts->AddFontFromFileTTF(METADOT_RESLOC("data/assets/fonts/fa-regular-400.ttf"), 13.0f, &icons_config, icons_ranges);
    }

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplSDL2_Init(Core.window, Core.glContext);

    ImGui_ImplOpenGL3_Init();

    style.ScaleAllSizes(scale);

    ImGuiInitStyle(0.5f, 0.5f);

#if defined(ME_IMM32)
    common_control_initialize();
    ME_ASSERT_E(imguiIMMCommunication.subclassify(Core.window));
#endif

    m_pack_editor.init();

    editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());

#if 0

        // set your own known preprocessor symbols...
        static const char* ppnames[] = { "NULL", "PM_REMOVE",
            "ZeroMemory", "DXGI_SWAP_EFFECT_DISCARD", "D3D_FEATURE_LEVEL", "D3D_DRIVER_TYPE_HARDWARE", "WINAPI","D3D11_SDK_VERSION", "assert" };
        // ... and their corresponding values
        static const char* ppvalues[] = {
            "#define NULL ((void*)0)",
            "#define PM_REMOVE (0x0001)",
            "Microsoft's own memory zapper function\n(which is a macro actually)\nvoid ZeroMemory(\n\t[in] PVOID  Destination,\n\t[in] SIZE_T Length\n); ",
            "enum DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_DISCARD = 0",
            "enum D3D_FEATURE_LEVEL",
            "enum D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE  = ( D3D_DRIVER_TYPE_UNKNOWN + 1 )",
            "#define WINAPI __stdcall",
            "#define D3D11_SDK_VERSION (7)",
            " #define assert(expression) (void)(                                                  \n"
            "    (!!(expression)) ||                                                              \n"
            "    (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \n"
            " )"
        };

        for (int i = 0; i < sizeof(ppnames) / sizeof(ppnames[0]); ++i)
        {
            TextEditor::Identifier id;
            id.mDeclaration = ppvalues[i];
            lang.mPreprocIdentifiers.insert(std::make_pair(std::string(ppnames[i]), id));
        }

        // set your own identifiers
        static const char* identifiers[] = {
            "HWND", "HRESULT", "LPRESULT","D3D11_RENDER_TARGET_VIEW_DESC", "DXGI_SWAP_CHAIN_DESC","MSG","LRESULT","WPARAM", "LPARAM","UINT","LPVOID",
            "ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer", "ID3D10Blob", "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
            "ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView", "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
            "IDXGISwapChain", "ID3D11RenderTargetView", "ID3D11Texture2D", "TextEditor" };
        static const char* idecls[] =
        {
            "typedef HWND_* HWND", "typedef long HRESULT", "typedef long* LPRESULT", "struct D3D11_RENDER_TARGET_VIEW_DESC", "struct DXGI_SWAP_CHAIN_DESC",
            "typedef tagMSG MSG\n * Message structure","typedef LONG_PTR LRESULT","WPARAM", "LPARAM","UINT","LPVOID",
            "ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer", "ID3D10Blob", "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
            "ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView", "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
            "IDXGISwapChain", "ID3D11RenderTargetView", "ID3D11Texture2D", "class TextEditor" };
        for (int i = 0; i < sizeof(identifiers) / sizeof(identifiers[0]); ++i)
        {
            TextEditor::Identifier id;
            id.mDeclaration = std::string(idecls[i]);
            lang.mIdentifiers.insert(std::make_pair(std::string(identifiers[i]), id));
        }
        editor.SetLanguageDefinition(lang);
        //editor.SetPalette(TextEditor::GetLightPalette());

        // error markers
        TextEditor::ErrorMarkers markers;
        markers.insert(std::make_pair<int, std::string>(6, "Example error here:\nInclude file not found: \"TextEditor.h\""));
        markers.insert(std::make_pair<int, std::string>(41, "Another example error"));
        editor.SetErrorMarkers(markers);

#endif

    fileDialog.SetPwd(METADOT_RESLOC("data/scripts"));
    fileDialog.SetTitle("选择文件");
    fileDialog.SetTypeFilters({".lua"});

    // auto create_console = [&]() {
    //     METADOT_NEW(C, console_imgui, ImGuiConsole, global.I18N.Get("ui_console"));

    //    // Our state
    //    ImVec4 clear_color = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);

    //    // Register variables
    //    console_imgui->System().RegisterVariable("background_color", clear_color, imvec4_setter);

    //    console_imgui->System().RegisterVariable("plPosX", global.GameData_.plPosX, Command::Arg<f32>(""));
    //    console_imgui->System().RegisterVariable("plPosY", global.GameData_.plPosY, Command::Arg<f32>(""));

    //    console_imgui->System().RegisterVariable("scale", Screen.gameScale, Command::Arg<i32>(""));

    //    ME::Struct::for_each(global.game->GameIsolate_.globaldef, [&](const char *name, auto &value) { console_imgui->System().RegisterVariable(name, value, Command::Arg<int>("")); });

    //    // Register custom commands
    //    console_imgui->System().RegisterCommand("random_background_color", "Assigns a random color to the background application", [&clear_color]() {
    //        clear_color.x = (rand() % 256) / 256.f;
    //        clear_color.y = (rand() % 256) / 256.f;
    //        clear_color.z = (rand() % 256) / 256.f;
    //        clear_color.w = (rand() % 256) / 256.f;
    //    });
    //    console_imgui->System().RegisterCommand("reset_background_color", "Reset background color to its original value", [&clear_color, val = clear_color]() { clear_color = val; });

    //    console_imgui->System().RegisterCommand("print_methods", "a", [this]() {
    //        for (auto &cmds : console_imgui->System().Commands()) {
    //            console_imgui->System().Log(Command::ItemType::LOG) << "\t" << cmds.first << Command::endl;
    //        }
    //    });
    //    console_imgui->System().RegisterCommand(
    //            "lua", "dostring",
    //            [&](const char *s) {
    //                auto &l = Scripting::get_singleton_ptr()->Lua;
    //                l->s_lua.dostring(s);
    //            },
    //            Command::Arg<String>(""));

    //    console_imgui->System().RegisterVariable("tps", Time.maxTps, Command::Arg<u32>(""));

    //    console_imgui->System().Log(Command::ItemType::INFO) << "游戏控制终端" << Command::endl;
    //};

    // create_console();
}

void ImGuiLayer::End() {
    m_pack_editor.end();

    RendererShutdownFunction();
    PlatformShutdownFunction();
    ImGui::DestroyContext();
}

void ImGuiLayer::NewFrame() {
    RendererNewFrameFunction();
    PlatformNewFrameFunction();
    ImGui::NewFrame();
}

void ImGuiLayer::Draw() {
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    ImGui::Render();
    SDL_GL_MakeCurrent(Core.window, Core.glContext);

    RenderFunction(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        C_Window *backup_current_window = SDL_GL_GetCurrentWindow();
        C_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }
}

auto CollapsingHeader = [](const char *name) -> bool {
    ImGuiStyle &style = ImGui::GetStyle();
    ImGui::PushStyleColor(ImGuiCol_Header, style.Colors[ImGuiCol_Button]);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, style.Colors[ImGuiCol_ButtonHovered]);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, style.Colors[ImGuiCol_ButtonActive]);
    bool b = ImGui::CollapsingHeader(name);
    ImGui::PopStyleColor(3);
    return b;
};

void ImGuiLayer::Update() {

    ImGuiIO &io = ImGui::GetIO();

#if defined(ME_IMM32)
    imguiIMMCommunication();
#endif

    // ImGui::Begin("Progress Indicators");
    // const ImU32 col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
    // const ImU32 bg = ImGui::GetColorU32(ImGuiCol_Button);
    // ImGui::Spinner("##spinner", 15, 6, col);
    // ImGui::BufferingBar("##buffer_bar", 0.7f, ImVec2(400, 6), bg, col);
    // ImGui::End();

    MarkdownData md1;
    md1.data = R"markdown(

# Table

Name &nbsp; &nbsp; &nbsp; &nbsp; | Multiline &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;<br>header  | [Link&nbsp;](#link1)
:------|:-------------------|:--
Value-One | Long <br>explanation <br>with \<br\>\'s|1
~~Value-Two~~ | __text auto wrapped__\: long explanation here |25 37 43 56 78 90
**etc** | [~~Some **link**~~](https://github.com/mekhontsev/ImGuiMarkdown)|3

# List

1. First ordered list item
2. 测试中文
   * Unordered sub-list 1.
   * Unordered sub-list 2.
1. Actual numbers don't matter, just that it's a number
   1. **Ordered** sub-list 1
   2. **Ordered** sub-list 2
4. And another item with minuses.
   - __sub-list with underline__
   - sub-list with escapes: \[looks like\]\(link\)
5. ~~Item with pluses and strikethrough~~.
   + sub-list 1
   + sub-list 2
   + [Just a link](https://github.com/mekhontsev/ImGuiMarkdown).
      * Item with [link1](#link1)
      * Item with bold [**link2**](#link1)
6. test12345

)markdown";

    if (global.game->GameIsolate_.globaldef.draw_imgui_debug) {
        ImGui::ShowDemoWindow();
    }

    bool interactingWithTextbox;
    if (global.game->GameIsolate_.globaldef.draw_console) console.display_full(&interactingWithTextbox);

    if (global.game->GameIsolate_.globaldef.draw_pack_editor) m_pack_editor.draw();

    auto cpos = editor.GetCursorPosition();
    if (global.game->GameIsolate_.globaldef.ui_tweak) {

        ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        static ImVec2 DockSpaceSize = {400.0f - 16.0f, viewport->Size.y - 16.0f};
        ImGui::SetNextWindowSize(DockSpaceSize, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos({viewport->Size.x - DockSpaceSize.x - 8.0f, 8.0f}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowViewport(viewport->ID);

        window_flags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings;
        dockspace_flags |= ImGuiDockNodeFlags_PassthruCentralNode;

        ImGui::Begin("Engine", NULL, window_flags);
        // DockSpaceSize = {ImGui::GetWindowSize().x, viewport->Size.y - 16.0f};
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        } else {
            ME_ASSERT_E(0);
        }
        ImGui::End();

        ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
        if (ImGui::Begin(LANG("ui_tweaks"), NULL, ImGuiWindowFlags_MenuBar)) {
            ImGui::BeginTabBar("ui_tweaks_tabbar");

            if (ImGui::BeginTabItem(ICON_LANG(ICON_FA_TERMINAL, "ui_console"))) {
                // console_imgui->Draw();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(ICON_LANG(ICON_FA_INFO, "ui_info"))) {
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

                R_Renderer *renderer = R_GetCurrentRenderer();
                R_RendererID id = renderer->id;

                ImGui::Text("Using renderer: %s", glGetString(GL_RENDERER));
                ImGui::Text("OpenGL version supported: %s", glGetString(GL_VERSION));
                ImGui::Text("Engine renderer: %s (%d.%d)\n", id.name, id.major_version, id.minor_version);
                ImGui::Text("Shader versions supported: %d to %d\n", renderer->min_shader_version, renderer->max_shader_version);
                ImGui::Text("Platform: %s\n", metadot_metadata().platform.c_str());
                ImGui::Text("Compiler: %s %s (with cpp %s)\n", metadot_metadata().compiler.c_str(), metadot_metadata().compiler_version.c_str(), metadot_metadata().cpp.c_str());

                ImGui::Separator();

                //                MarkdownData TickInfoPanel;
                //                TickInfoPanel.data = R"(
                // Info &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; | Data &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; | Comments &nbsp;
                //:-----------|:------------|:--------
                // TickCount | {0} | Nothing
                // DeltaTime | {1} | Nothing
                // TPS | {2} | Nothing
                // Mspt | {3} | Nothing
                // Delta | {4} | Nothing
                // STDTime | {5} | Nothing
                // CSTDTime | {6} | Nothing)";
                //
                //                 time_t rawtime;
                //                 rawtime = time(NULL);
                //                 struct tm *timeinfo = localtime(&rawtime);
                //
                //                 TickInfoPanel.data = std::format(TickInfoPanel.data, Time.tickCount, Time.deltaTime, Time.tps, Time.mspt, Time.now - Time.lastTickTime, ME_gettime(), rawtime);
                //
                //                 ImGui::Auto(TickInfoPanel);

                // ImGui::Text("\nnow: %d-%02d-%02d %02d:%02d:%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
                //  ImGui::Text("_curID %d", MaterialInstance::_curID);

                ImGui::Dummy(ImVec2(0.0f, 20.0f));

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(ICON_LANG(ICON_FA_SPIDER, "ui_test"))) {
                ImGui::BeginTabBar(CC("测试#haha"));
                static bool play;
                if (ImGui::BeginTabItem(CC("测试"))) {
                    if (ImGui::Button("调用回溯")) print_callstack();
                    ImGui::SameLine();
                    if (ImGui::Button("NPC")) {

                        MEvec4 pl_transform{-global.game->GameIsolate_.world->loadZone.x + global.game->GameIsolate_.world->tickZone.x + global.game->GameIsolate_.world->tickZone.w / 2.0f,
                                            -global.game->GameIsolate_.world->loadZone.y + global.game->GameIsolate_.world->tickZone.y + global.game->GameIsolate_.world->tickZone.h / 2.0f, 10, 20};

                        b2PolygonShape sh;
                        sh.SetAsBox(pl_transform.z / 2.0f + 1, pl_transform.w / 2.0f);
                        RigidBody *rb = global.game->GameIsolate_.world->makeRigidBody(b2BodyType::b2_kinematicBody, pl_transform.x + pl_transform.z / 2.0f - 0.5,
                                                                                       pl_transform.y + pl_transform.w / 2.0f - 0.5, 0, sh, 1, 1, NULL);
                        rb->body->SetGravityScale(0);
                        rb->body->SetLinearDamping(0);
                        rb->body->SetAngularDamping(0);

                        auto npc = global.game->GameIsolate_.world->Reg().create_entity();
                        MetaEngine::ECS::entity_filler(npc)
                                .component<Controlable>()
                                .component<WorldEntity>(true, pl_transform.x, pl_transform.y, 0.0f, 0.0f, (int)pl_transform.z, (int)pl_transform.w, rb, std::string("NPC"))
                                .component<Bot>(1);

                        auto npc_bot = global.game->GameIsolate_.world->Reg().find_component<Bot>(npc);
                        // npc_bot->setItemInHand(global.game->GameIsolate_.world->Reg().find_component<WorldEntity>(global.game->GameIsolate_.world->player), i3,
                        // global.game->GameIsolate_.world.get());
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Audio")) {
                        play ^= true;
                        static ME_Audio *test_audio = metadot_audio_load_wav(METADOT_RESLOC("data/assets/audio/02_c03_normal_135.wav"));
                        if (play) {
                            ME_ASSERT_E(test_audio);
                            // metadot_music_play(test_audio, 0.f);
                            // metadot_audio_destroy(test_audio);
                            int err;
                            metadot_play_sound(test_audio, metadot_sound_params_defaults(), &err);
                        } else {
                            MetaEngine::audio_set_pause(false);
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("StaticRefl")) {

                        if (global.game->GameIsolate_.world == nullptr || !global.game->GameIsolate_.world->isPlayerInWorld()) {
                        } else {
                            using namespace ME::meta::static_refl;

                            TypeInfo<Player>::DFS_ForEach([](auto t, std::size_t depth) {
                                for (std::size_t i = 0; i < depth; i++) std::cout << "  ";
                                std::cout << t.name << std::endl;
                            });

                            std::cout << "[non DFS]" << std::endl;
                            TypeInfo<Player>::fields.ForEach([&](auto field) {
                                std::cout << field.name << std::endl;
                                if (field.name == "holdtype" && TypeInfo<EnumPlayerHoldType>::fields.NameOfValue(field.value) == "hammer") {
                                    std::cout << "   Passed" << std::endl;
                                    // if constexpr (std::is_same<decltype(field.value), EnumPlayerHoldType>().value) {
                                    //     std::cout << "   " << field.value << std::endl;
                                    // }
                                }
                            });

                            std::cout << "[DFS]" << std::endl;
                            TypeInfo<Player>::DFS_ForEach([](auto t, std::size_t) { t.fields.ForEach([](auto field) { std::cout << field.name << std::endl; }); });

                            constexpr std::size_t fieldNum = TypeInfo<Player>::DFS_Acc(0, [](auto acc, auto, auto) { return acc + 1; });
                            std::cout << "field number : " << fieldNum << std::endl;

                            std::cout << "[var]" << std::endl;

                            // std::cout << "[left]" << std::endl;
                            // TypeInfo<Player>::ForEachVarOf(std::move(*global.game->GameIsolate_.world->player), [](auto field, auto &&var) {
                            //     static_assert(std::is_rvalue_reference_v<decltype(var)>);
                            //     std::cout << field.name << " : " << var << std::endl;
                            // });
                            // std::cout << "[right]" << std::endl;
                            // TypeInfo<Player>::ForEachVarOf(*global.game->GameIsolate_.world->player, [](auto field, auto &&var) {
                            //     static_assert(std::is_lvalue_reference_v<decltype(var)>);
                            //     std::cout << field.name << " : " << var << std::endl;
                            // });

                            auto p = global.game->GameIsolate_.world->Reg().find_component<Player>(global.game->GameIsolate_.world->player);

                            // Just for test
                            Item *i3 = new Item();
                            i3->setFlag(ItemFlags::ItemFlags_Hammer);
                            i3->surface = LoadTexture("data/assets/objects/testHammer.png")->surface;
                            i3->texture = R_CopyImageFromSurface(i3->surface);
                            R_SetImageFilter(i3->texture, R_FILTER_NEAREST);
                            i3->pivotX = 2;

                            TypeInfo<Player>::fields.ForEach([&](auto field) {
                                if constexpr (field.is_func) {
                                    if (field.name != "setItemInHand") return;
                                    if constexpr (field.ValueTypeIsSameWith(static_cast<void (Player::*)(WorldEntity * we, Item * item, World * world) /* const */>(&Player::setItemInHand)))
                                        (p->*(field.value))(global.game->GameIsolate_.world->Reg().find_component<WorldEntity>(global.game->GameIsolate_.world->player), i3,
                                                            global.game->GameIsolate_.world.get());
                                    // else if constexpr (field.ValueTypeIsSameWith(static_cast<void (Player::*)(Item *item, World *world) /* const */>(&Player::setItemInHand)))
                                    //     std::cout << (p.*(field.value))(1.f) << std::endl;
                                    else
                                        assert(false);
                                }
                            });

                            // virtual

                            std::cout << "[Virtual Bases]" << std::endl;
                            constexpr auto vbs = TypeInfo<Player>::VirtualBases();
                            vbs.ForEach([](auto info) { std::cout << info.name << std::endl; });

                            std::cout << "[Tree]" << std::endl;
                            TypeInfo<Player>::DFS_ForEach([](auto t, std::size_t depth) {
                                for (std::size_t i = 0; i < depth; i++) std::cout << "  ";
                                std::cout << t.name << std::endl;
                            });

                            std::cout << "[field]" << std::endl;
                            TypeInfo<Player>::DFS_ForEach([](auto t, std::size_t) { t.fields.ForEach([](auto field) { std::cout << field.name << std::endl; }); });

                            // std::cout << "[var]" << std::endl;

                            // std::cout << "[var : left]" << std::endl;
                            // TypeInfo<Player>::ForEachVarOf(std::move(p), [](auto field, auto &&var) {
                            //     static_assert(std::is_rvalue_reference_v<decltype(var)>);
                            //     std::cout << var << std::endl;
                            // });
                            // std::cout << "[var : right]" << std::endl;
                            // TypeInfo<Player>::ForEachVarOf(p, [](auto field, auto &&var) {
                            //     static_assert(std::is_lvalue_reference_v<decltype(var)>);
                            //     std::cout << field.name << " : " << var << std::endl;
                            // });
                        }
                    }
                    ImGui::Checkbox("Profiler", &global.game->GameIsolate_.globaldef.draw_profiler);
                    ImGui::Checkbox("控制台", &global.game->GameIsolate_.globaldef.draw_console);
                    ImGui::Checkbox("包编辑器", &global.game->GameIsolate_.globaldef.draw_pack_editor);
                    ImGui::Checkbox("UI", &global.game->GameIsolate_.ui->uidata->elementLists["testElement1"]->visible);
                    if (ImGui::Button("Meo")) {
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem(CC("自动序列测试"))) {
                    // ShowAutoTestWindow();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(ICON_LANG(ICON_FA_DESKTOP, "ui_debug"))) {
                if (CollapsingHeader(ICON_LANG(ICON_FA_VECTOR_SQUARE, "ui_telemetry"))) {
                    GameUI::DrawDebugUI(global.game);
                }
#define INSPECTSHADER(_c) ME::inspect_shader(#_c, global.game->GameIsolate_.shaderworker->_c->shader)
                if (CollapsingHeader(CC("GLSL"))) {
                    ImGui::Indent();
                    INSPECTSHADER(newLightingShader);
                    INSPECTSHADER(fireShader);
                    INSPECTSHADER(fire2Shader);
                    INSPECTSHADER(waterShader);
                    INSPECTSHADER(waterFlowPassShader);
                    INSPECTSHADER(untexturedShader);
                    INSPECTSHADER(blurShader);
                    ImGui::Unindent();
                }
#undef INSPECTSHADER
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        ImGui::End();

        ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
        if (ImGui::Begin(LANG("ui_inspecting"), NULL)) {
            ImGui::BeginTabBar("ui_inspect");
            if (ImGui::BeginTabItem(ICON_LANG(ICON_FA_HASHTAG, "ui_scene"))) {
                if (CollapsingHeader(LANG("ui_chunk"))) {
                    static bool check_rigidbody = false;
                    ImGui::Checkbox(CC("只查看刚体有效"), &check_rigidbody);

                    static MEvec2 check_chunk = {1, 1};
                    static Chunk *check_chunk_ptr = nullptr;

                    if (ImGui::BeginCombo("ChunkList", CC("选择检视区块..."))) {
                        for (auto &p1 : global.game->GameIsolate_.world->chunkCache)
                            for (auto &p2 : p1.second) {
                                if (ImGui::Selectable(p2.second->pack_filename.c_str())) {
                                    check_chunk.x = p2.second->x;
                                    check_chunk.y = p2.second->y;
                                    check_chunk_ptr = p2.second;
                                }
                            }
                        ImGui::EndCombo();
                    }

                    ImGui::Indent();
                    if (check_chunk_ptr != nullptr)
                        ME::meta::static_refl::TypeInfo<Chunk>::ForEachVarOf(*check_chunk_ptr, [&](const auto &field, auto &&var) {
                            if (field.name == "pack_filename") return;

                            if (check_rigidbody)
                                if (field.name == "rb" && &var) {
                                    return;
                                }
                            // constexpr auto tstr_range = TSTR("Meta::Msg");

                            // if constexpr (decltype(field.attrs)::Contains(tstr_range)) {
                            //     auto r = attr_init(tstr_range, field.attrs.Find(tstr_range).value);
                            //     // cout << "[" << tstr_range.View() << "] " << r.minV << ", " << r.maxV << endl;
                            // }
                            ImGui::Auto(var, std::string(field.name));
                        });
                    ImGui::Unindent();
                }
                if (CollapsingHeader(LANG("ui_entities"))) {

                    ImGui::Indent();
                    ImGui::Auto(global.game->GameIsolate_.world->rigidBodies, "刚体");
                    ImGui::Auto(global.game->GameIsolate_.world->worldRigidBodies, "世界刚体");

                    ImGui::Text("ECS: %lu %lu", global.game->GameIsolate_.world->Reg().memory_usage().entities, global.game->GameIsolate_.world->Reg().memory_usage().components);

                    global.game->GameIsolate_.world->Reg().for_joined_components<WorldEntity, Player>([&](MetaEngine::ECS::entity, WorldEntity &we, Player &p) {
                        ImGui::Auto(we, "实体");
                        ImGui::Auto(p, "玩家");
                    });

                    ImGui::Unindent();
                    // static RigidBody *check_rigidbody_ptr = nullptr;

                    // if (ImGui::BeginCombo("RigidbodyList", CC("选择检视刚体..."))) {
                    //     for (auto p1 : global.game->GameIsolate_.world->rigidBodies)
                    //         if (ImGui::Selectable(p1->name.c_str())) check_rigidbody_ptr = p1;
                    //     ImGui::EndCombo();
                    // }

                    // if (check_rigidbody_ptr != nullptr) {
                    //     ImGui::Text("刚体名: %s", check_rigidbody_ptr->name.c_str());
                    //     ME::meta::static_refl::TypeInfo<RigidBody>::ForEachVarOf(*check_rigidbody_ptr, [&](const auto &field, auto &&var) { ImGui::Auto(var, std::string(field.name)); });
                    // }
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(ICON_LANG(ICON_FA_PROJECT_DIAGRAM, "ui_system"))) {
                if (ImGui::BeginTable("ui_system_table", 4,
                                      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_NoSavedSettings)) {
                    ImGui::TableSetupColumn("Priority", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, 0);
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 0.0f, 1);
                    ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, 2);
                    ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_None, 0.0f, 3);

                    ImGui::TableHeadersRow();

                    // ME::Struct::for_each(global.game->GameIsolate_.globaldef, [&](const char *name, auto &value) { console_imgui->System().RegisterVariable(name, value,
                    // Command::Arg<int>(""));
                    // });
                    int i = 0;

                    for (auto &s : global.game->GameIsolate_.systemList) {

                        ImGui::PushID(i++);
                        ImGui::TableNextRow(ImGuiTableRowFlags_None);

                        if (ImGui::TableSetColumnIndex(0)) ImGui::Text("%d", s->priority);
                        if (ImGui::TableSetColumnIndex(1)) ImGui::TextUnformatted(s->getName().c_str());
                        if (ImGui::TableSetColumnIndex(2)) {
                            if (ImGui::SmallButton("Reload")) {
                                METADOT_BUG("Reloading %s", s->getName().c_str());
                                s->Reload();
                            }
                            ImGui::SameLine();
                            if (ImGui::SmallButton("Edit")) {
                            }
                        }
                        if (ImGui::TableSetColumnIndex(3)) ImGui::Text("描述");

                        ImGui::PopID();
                    }

                    ImGui::EndTable();
                }

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();

        ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
        if (ImGui::Begin(LANG("ui_scripts_editor"), NULL, ImGuiWindowFlags_MenuBar)) {

            // if (ImGui::BeginTabItem(ICON_LANG(ICON_FA_EDIT, "ui_scripts_editor"))) {
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu(LANG("ui_file"))) {
                    if (ImGui::MenuItem(LANG("ui_open"))) {
                        fileDialog.Open();
                    }
                    if (ImGui::MenuItem(LANG("ui_save"))) {
                        if (view_editing && view_contents.size()) {
                            auto textToSave = editor.GetText();
                            std::ofstream o(view_editing->file);
                            o << textToSave;
                        }
                    }
                    if (ImGui::MenuItem(LANG("ui_close"))) {
                        for (auto &code : view_contents) {
                            if (code.file == view_editing->file) {
                                view_contents.erase(std::remove(std::begin(view_contents), std::end(view_contents), code), std::end(view_contents));
                            }
                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu(LANG("ui_edit"))) {
                    bool ro = editor.IsReadOnly();
                    if (ImGui::MenuItem(LANG("ui_readonly_mode"), nullptr, &ro)) editor.SetReadOnly(ro);
                    ImGui::Separator();

                    if (ImGui::MenuItem(LANG("ui_undo"), "ALT-Backspace", nullptr, !ro && editor.CanUndo())) editor.Undo();
                    if (ImGui::MenuItem(LANG("ui_redo"), "Ctrl-Y", nullptr, !ro && editor.CanRedo())) editor.Redo();

                    ImGui::Separator();

                    if (ImGui::MenuItem(LANG("ui_copy"), "Ctrl-C", nullptr, editor.HasSelection())) editor.Copy();
                    if (ImGui::MenuItem(LANG("ui_cut"), "Ctrl-X", nullptr, !ro && editor.HasSelection())) editor.Cut();
                    if (ImGui::MenuItem(LANG("ui_delete"), "Del", nullptr, !ro && editor.HasSelection())) editor.Delete();
                    if (ImGui::MenuItem(LANG("ui_paste"), "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr)) editor.Paste();

                    ImGui::Separator();

                    if (ImGui::MenuItem(LANG("ui_selectall"), nullptr, nullptr)) editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu(LANG("ui_view"))) {
                    if (ImGui::MenuItem("Dark palette")) editor.SetPalette(TextEditor::GetDarkPalette());
                    if (ImGui::MenuItem("Light palette")) editor.SetPalette(TextEditor::GetLightPalette());
                    if (ImGui::MenuItem("Retro blue palette")) editor.SetPalette(TextEditor::GetRetroBluePalette());
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            fileDialog.Display();

            if (fileDialog.HasSelected()) {
                bool shouldopen = true;
                auto fileopen = fileDialog.GetSelected().string();
                for (auto code : view_contents)
                    if (code.file == fileopen) shouldopen = false;
                if (shouldopen) {
                    std::ifstream i(fileopen);
                    if (i.good()) {
                        std::string str((std::istreambuf_iterator<char>(i)), std::istreambuf_iterator<char>());
                        view_contents.push_back(EditorView{.tags = EditorTags::Editor_Code, .file = fileopen, .content = str});
                    }
                }
                fileDialog.ClearSelected();
            }

            ImGui::BeginTabBar("ViewContents");

            for (auto &view : view_contents) {
                if (ImGui::BeginTabItem(ME_fs_get_filename(view.file.c_str()))) {
                    view_editing = &view;

                    if (!view_editing->is_edited) {
                        if (view.tags == EditorTags::Editor_Code) editor.SetText(view_editing->content);
                        view_editing->is_edited = true;
                    }

                    ImGui::EndTabItem();
                } else {
                    if (view.is_edited) {
                        view.is_edited = false;
                    }
                }
            }

            if (view_editing && view_contents.size()) {
                switch (view_editing->tags) {
                    case Editor_Code:
                        ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(), editor.IsOverwrite() ? "Ovr" : "Ins",
                                    editor.CanUndo() ? "*" : " ", editor.GetLanguageDefinition().mName.c_str(), ME_fs_get_filename(view_editing->file.c_str()));

                        editor.Render("TextEditor");
                        break;
                    case Editor_Markdown:
                        break;
                    default:
                        break;
                }
            }
            ImGui::EndTabBar();
        }
        ImGui::End();

        ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
        if (ImGui::Begin(LANG("ui_cvars"))) {

            if (ImGui::BeginTable("ui_cvars_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, 0);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 0.0f, 1);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, 2);
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_None, 0.0f, 3);

                ImGui::TableHeadersRow();

                int n;
                auto ShowCVar = [&](const char *name, auto &value) {
                    ImGui::TableNextRow(ImGuiTableRowFlags_None);

                    if (ImGui::TableSetColumnIndex(0)) ImGui::Text("%d", n++);
                    if (ImGui::TableSetColumnIndex(1)) ImGui::TextUnformatted(name);
                    if (ImGui::TableSetColumnIndex(2)) {
                        if constexpr (std::is_same_v<decltype(value), bool>) {
                            ImGui::TextUnformatted(BOOL_STRING(value));
                        } else {
                            ImGui::TextUnformatted(std::to_string(value).c_str());
                        }
                    }
                    if (ImGui::TableSetColumnIndex(3)) {
                        if (ImGui::SmallButton("Reset")) {
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Edit")) {
                        }
                    }
                };

                ME::Struct::for_each(global.game->GameIsolate_.globaldef, ShowCVar);

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }
}