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

#include "audio/audio.h"
#include "chunk.hpp"
#include "core/alloc.hpp"
#include "core/const.h"
#include "core/core.h"
#include "core/core.hpp"
#include "core/cpp/command.hpp"
#include "core/cpp/static_relfection.hpp"
#include "core/cpp/utils.hpp"
#include "core/dbgtools.h"
#include "core/debug.hpp"
#include "core/global.hpp"
#include "core/io/filesystem.h"
#include "core/macros.h"
#include "core/stl.h"
#include "engine/engine.h"
#include "game.hpp"
#include "game_datastruct.hpp"
#include "game_ui.hpp"
#include "libs/imgui/font_awesome.h"
#include "libs/imgui/imgui.h"
#include "libs/imgui/implot.h"
#include "meta/meta.hpp"
#include "npc.hpp"
#include "reflectionflat.hpp"
#include "renderer/gpu.hpp"
#include "renderer/metadot_gl.h"
#include "renderer/renderer_gpu.h"
#include "scripting/lua/lua_wrapper.hpp"
#include "scripting/scripting.hpp"
#include "ui/imgui/imgui_impl.hpp"
#include "ui/ui.hpp"

IMPLENGINE();

#define LANG(_c) global.I18N.Get(_c).c_str()
#define ICON_LANG(_i, _c) std::string(std::string(_i) + " " + global.I18N.Get(_c)).c_str()

namespace ImGui {
struct InputTextCallback_UserData {
    std::string *Str;
    ImGuiInputTextCallback ChainCallback;
    void *ChainCallbackUserData;
};

static int InputTextCallback(ImGuiInputTextCallbackData *data) {
    auto *user_data = (InputTextCallback_UserData *)data->UserData;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {

        std::string *str = user_data->Str;
        IM_ASSERT(data->Buf == str->c_str());
        str->resize(data->BufTextLen);
        data->Buf = (char *)str->c_str();
    } else if (user_data->ChainCallback) {

        data->UserData = user_data->ChainCallbackUserData;
        return user_data->ChainCallback(data);
    }
    return 0;
}

bool ConsoleInputText(const char *label, std::string *str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void *user_data) {
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = callback;
    cb_user_data.ChainCallbackUserData = user_data;
    return InputText(label, (char *)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
}
}  // namespace ImGui

ImGuiConsole::ImGuiConsole(std::string c_name, size_t inputBufferSize) : m_ConsoleName(std::move(c_name)) {

    m_Buffer.resize(inputBufferSize);
    m_HistoryIndex = std::numeric_limits<size_t>::min();

    InitIniSettings();

    if (!m_LoadedFromIni) {
        DefaultSettings();
    }

    RegisterConsoleCommands();
}

void ImGuiConsole::Draw() {

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_WindowAlpha);
    // if (!ImGui::Begin(m_ConsoleName.data(), nullptr, ImGuiWindowFlags_MenuBar)) {
    //     ImGui::PopStyleVar();
    //     ImGui::End();
    //     return;
    // }
    ImGui::PopStyleVar();

    MenuBar();

    if (m_FilterBar) {
        FilterBar();
    }

    LogWindow();

    ImGui::Separator();

    InputBar();

    // ImGui::End();
}

Command::System &ImGuiConsole::System() { return m_ConsoleSystem; }

void ImGuiConsole::InitIniSettings() {
    ImGuiContext &g = *ImGui::GetCurrentContext();

    if (g.Initialized && !g.SettingsLoaded && !m_LoadedFromIni) {
        ImGuiSettingsHandler console_ini_handler;
        console_ini_handler.TypeName = "Console";
        console_ini_handler.TypeHash = ImHashStr("Console");
        console_ini_handler.ClearAllFn = SettingsHandler_ClearALl;
        console_ini_handler.ApplyAllFn = SettingsHandler_ApplyAll;
        console_ini_handler.ReadInitFn = SettingsHandler_ReadInit;
        console_ini_handler.ReadOpenFn = SettingsHandler_ReadOpen;
        console_ini_handler.ReadLineFn = SettingsHandler_ReadLine;
        console_ini_handler.WriteAllFn = SettingsHandler_WriteAll;
        console_ini_handler.UserData = this;
        g.SettingsHandlers.push_back(console_ini_handler);
    }
}

void ImGuiConsole::DefaultSettings() {

    m_AutoScroll = true;
    m_ScrollToBottom = false;
    m_ColoredOutput = true;
    m_FilterBar = true;
    m_TimeStamps = true;

    m_WindowAlpha = 1;
    m_ColorPalette[COL_COMMAND] = ImVec4(1.f, 1.f, 1.f, 1.f);
    m_ColorPalette[COL_LOG] = ImVec4(1.f, 1.f, 1.f, 0.5f);
    m_ColorPalette[COL_WARNING] = ImVec4(1.0f, 0.87f, 0.37f, 1.f);
    m_ColorPalette[COL_ERROR] = ImVec4(1.f, 0.365f, 0.365f, 1.f);
    m_ColorPalette[COL_INFO] = ImVec4(0.46f, 0.96f, 0.46f, 1.f);
    m_ColorPalette[COL_TIMESTAMP] = ImVec4(1.f, 1.f, 1.f, 0.5f);
}

void ImGuiConsole::RegisterConsoleCommands() {
    m_ConsoleSystem.RegisterCommand("clear", "Clear console log", [this]() { m_ConsoleSystem.Items().clear(); });

    m_ConsoleSystem.RegisterCommand(
            "filter", "Set screen filter",
            [this](const String &filter) {
                std::memset(m_TextFilter.InputBuf, '\0', 256);

                std::copy(filter.m_String.c_str(), filter.m_String.c_str() + std::min(static_cast<int>(filter.m_String.length()), 255), m_TextFilter.InputBuf);

                m_TextFilter.Build();
            },
            Command::Arg<String>("filter_str"));

    // m_ConsoleSystem.RegisterCommand(
    //         "run", "Run given script",
    //         [this](const String &filter) { m_ConsoleSystem.RunScript(filter.m_String); },
    //         Command::Arg<String>("script_name"));
}

void ImGuiConsole::FilterBar() {
    m_TextFilter.Draw("Filter", ImGui::GetWindowWidth() * 0.25f);
    ImGui::Separator();
}

void ImGuiConsole::LogWindow() {
    const F32 footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if (ImGui::BeginChild("ScrollRegion##", ImVec2(0, -footerHeightToReserve), false, 0)) {

        static const F32 timestamp_width = ImGui::CalcTextSize("00:00:00:0000").x;
        int count = 0;

        ImGui::PushTextWrapPos();

        for (const auto &item : m_ConsoleSystem.Items()) {

            if (!m_TextFilter.PassFilter(item.Get().c_str())) continue;

            if (item.m_Type == Command::COMMAND) {
                if (m_TimeStamps) ImGui::PushTextWrapPos(ImGui::GetColumnWidth() - timestamp_width);
                if (count++ != 0) ImGui::Dummy(ImVec2(-1, ImGui::GetFontSize()));
            }

            if (m_ColoredOutput) {
                ImGui::PushStyleColor(ImGuiCol_Text, m_ColorPalette[item.m_Type]);
                ImGui::TextUnformatted(item.Get().data());
                ImGui::PopStyleColor();
            } else {
                ImGui::TextUnformatted(item.Get().data());
            }

            if (item.m_Type == Command::COMMAND && m_TimeStamps) {

                ImGui::PopTextWrapPos();

                ImGui::SameLine(ImGui::GetColumnWidth(-1) - timestamp_width);

                ImGui::PushStyleColor(ImGuiCol_Text, m_ColorPalette[COL_TIMESTAMP]);
                ImGui::Text("%02d:%02d:%02d:%04d", ((item.m_TimeStamp / 1000 / 3600) % 24), ((item.m_TimeStamp / 1000 / 60) % 60), ((item.m_TimeStamp / 1000) % 60), item.m_TimeStamp % 1000);
                ImGui::PopStyleColor();
            }
        }

        ImGui::PopTextWrapPos();

        if ((m_ScrollToBottom && (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() || m_AutoScroll))) ImGui::SetScrollHereY(1.0f);
        m_ScrollToBottom = false;

        ImGui::EndChild();
    }
}

void ImGuiConsole::InputBar() {

    ImGuiInputTextFlags inputTextFlags = ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_EnterReturnsTrue |
                                         ImGuiInputTextFlags_CallbackAlways;

    bool reclaimFocus = false;

    ImGui::PushItemWidth(-ImGui::GetStyle().ItemSpacing.x * 7);
    if (ImGui::ConsoleInputText("Input", &m_Buffer, inputTextFlags, InputCallback, this)) {

        if (!m_Buffer.empty()) {

            m_ConsoleSystem.RunCommand(m_Buffer);

            m_ScrollToBottom = true;
        }

        reclaimFocus = true;

        m_Buffer.clear();
    }
    ImGui::PopItemWidth();

    if (ImGui::IsItemEdited() && !m_WasPrevFrameTabCompletion) {
        m_CmdSuggestions.clear();
    }
    m_WasPrevFrameTabCompletion = false;

    ImGui::SetItemDefaultFocus();
    if (reclaimFocus) ImGui::SetKeyboardFocusHere(-1);
}

void ImGuiConsole::MenuBar() {
    if (ImGui::BeginMenuBar()) {

        if (ImGui::BeginMenu(LANG("ui_settings"))) {

            ImGui::Checkbox(LANG("ui_color_output"), &m_ColoredOutput);
            ImGui::SameLine();
            HelpMaker("Enable colored command output");

            ImGui::Checkbox(LANG("ui_auto_scroll"), &m_AutoScroll);
            ImGui::SameLine();
            HelpMaker("Automatically scroll to bottom of console log");

            ImGui::Checkbox(LANG("ui_filter_bar"), &m_FilterBar);
            ImGui::SameLine();
            HelpMaker("Enable console filter bar");

            ImGui::Checkbox(LANG("ui_time_stamps"), &m_TimeStamps);
            ImGui::SameLine();
            HelpMaker("Display command execution timestamps");

            if (ImGui::Button(LANG("ui_reset_settings"), ImVec2(ImGui::GetColumnWidth(), 0))) ImGui::OpenPopup("Reset Settings?");

            if (ImGui::BeginPopupModal("Reset Settings?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text(
                        "All settings will be reset to default.\nThis operation cannot be "
                        "undone!\n\n");
                ImGui::Separator();

                if (ImGui::Button(LANG("ui_reset"), ImVec2(120, 0))) {
                    DefaultSettings();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button(LANG("ui_cancel"), ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(LANG("ui_appearance"))) {

            ImGuiColorEditFlags flags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar;

            ImGui::TextUnformatted("Color Palette");
            ImGui::Indent();
            ImGui::ColorEdit4("Command##", (F32 *)&m_ColorPalette[COL_COMMAND], flags);
            ImGui::ColorEdit4("Log##", (F32 *)&m_ColorPalette[COL_LOG], flags);
            ImGui::ColorEdit4("Warning##", (F32 *)&m_ColorPalette[COL_WARNING], flags);
            ImGui::ColorEdit4("Error##", (F32 *)&m_ColorPalette[COL_ERROR], flags);
            ImGui::ColorEdit4("Info##", (F32 *)&m_ColorPalette[COL_INFO], flags);
            ImGui::ColorEdit4("Time Stamp##", (F32 *)&m_ColorPalette[COL_TIMESTAMP], flags);
            ImGui::Unindent();

            ImGui::Separator();

            ImGui::TextUnformatted(LANG("ui_background"));
            ImGui::SliderFloat("Transparency##", &m_WindowAlpha, 0.1f, 1.f);

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void ImGuiConsole::HelpMaker(const char *desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

int ImGuiConsole::InputCallback(ImGuiInputTextCallbackData *data) {

    if (data->BufTextLen == 0 && (data->EventFlag != ImGuiInputTextFlags_CallbackHistory)) return 0;

    std::string input_str = data->Buf;
    std::string trim_str;
    auto console = static_cast<ImGuiConsole *>(data->UserData);

    size_t startPos = console->m_Buffer.find_first_not_of(' ');
    size_t endPos = console->m_Buffer.find_last_not_of(' ');

    if (startPos != std::string::npos && endPos != std::string::npos)
        trim_str = console->m_Buffer.substr(startPos, endPos + 1);
    else
        trim_str = console->m_Buffer;

    switch (data->EventFlag) {
        case ImGuiInputTextFlags_CallbackCompletion: {

            size_t startSubtrPos = trim_str.find_last_of(' ');
            Command::AutoComplete *console_autocomplete;

            if (startSubtrPos == std::string::npos) {
                startSubtrPos = 0;
                console_autocomplete = &console->m_ConsoleSystem.CmdAutocomplete();
            } else {
                startSubtrPos += 1;
                console_autocomplete = &console->m_ConsoleSystem.VarAutocomplete();
            }

            if (!trim_str.empty()) {

                if (!console->m_CmdSuggestions.empty()) {
                    console->m_ConsoleSystem.Log(Command::COMMAND) << "Suggestions: " << Command::endl;

                    for (const auto &suggestion : console->m_CmdSuggestions) console->m_ConsoleSystem.Log(Command::LOG) << suggestion << Command::endl;

                    console->m_CmdSuggestions.clear();
                }

                std::string partial = console_autocomplete->Suggestions(trim_str.substr(startSubtrPos, endPos + 1), console->m_CmdSuggestions);

                if (!console->m_CmdSuggestions.empty() && console->m_CmdSuggestions.size() == 1) {
                    data->DeleteChars(static_cast<int>(startSubtrPos), static_cast<int>(data->BufTextLen - startSubtrPos));
                    data->InsertChars(static_cast<int>(startSubtrPos), console->m_CmdSuggestions[0].data());
                    console->m_CmdSuggestions.clear();
                } else {

                    if (!partial.empty()) {
                        data->DeleteChars(static_cast<int>(startSubtrPos), static_cast<int>(data->BufTextLen - startSubtrPos));
                        data->InsertChars(static_cast<int>(startSubtrPos), partial.data());
                    }
                }
            }

            console->m_WasPrevFrameTabCompletion = true;
        } break;

        case ImGuiInputTextFlags_CallbackHistory: {

            data->DeleteChars(0, data->BufTextLen);

            if (console->m_HistoryIndex == std::numeric_limits<size_t>::min()) console->m_HistoryIndex = console->m_ConsoleSystem.History().GetNewIndex();

            if (data->EventKey == ImGuiKey_UpArrow) {
                if (console->m_HistoryIndex) --(console->m_HistoryIndex);
            } else {
                if (console->m_HistoryIndex < console->m_ConsoleSystem.History().Size()) ++(console->m_HistoryIndex);
            }

            std::string prevCommand = console->m_ConsoleSystem.History()[console->m_HistoryIndex];

            data->InsertChars(data->CursorPos, prevCommand.data());
        } break;

        case ImGuiInputTextFlags_CallbackCharFilter:
        case ImGuiInputTextFlags_CallbackAlways:
        default:
            break;
    }
    return 0;
}

void ImGuiConsole::SettingsHandler_ClearALl(ImGuiContext *ctx, ImGuiSettingsHandler *handler) {}

void ImGuiConsole::SettingsHandler_ReadInit(ImGuiContext *ctx, ImGuiSettingsHandler *handler) {}

void *ImGuiConsole::SettingsHandler_ReadOpen(ImGuiContext *ctx, ImGuiSettingsHandler *handler, const char *name) {
    if (!handler->UserData) return nullptr;

    auto console = static_cast<ImGuiConsole *>(handler->UserData);

    if (strcmp(name, console->m_ConsoleName.c_str()) != 0) return nullptr;
    return (void *)1;
}

void ImGuiConsole::SettingsHandler_ReadLine(ImGuiContext *ctx, ImGuiSettingsHandler *handler, void *entry, const char *line) {
    if (!handler->UserData) return;

    auto console = static_cast<ImGuiConsole *>(handler->UserData);

    console->m_LoadedFromIni = true;

#pragma warning(push)
#pragma warning(disable : 4996)

#define INI_CONSOLE_LOAD_COLOR(type) \
    (std::sscanf(line, #type "=%i,%i,%i,%i", &r, &g, &b, &a) == 4) { console->m_ColorPalette[type] = ImColor(r, g, b, a); }
#define INI_CONSOLE_LOAD_FLOAT(var) \
    (std::sscanf(line, #var "=%f", &f) == 1) { console->var = f; }
#define INI_CONSOLE_LOAD_BOOL(var) \
    (std::sscanf(line, #var "=%i", &b) == 1) { console->var = b == 1; }

    F32 f;
    int r, g, b, a;

    if INI_CONSOLE_LOAD_COLOR (COL_COMMAND)
        else if INI_CONSOLE_LOAD_COLOR (COL_LOG) else if INI_CONSOLE_LOAD_COLOR (COL_WARNING) else if INI_CONSOLE_LOAD_COLOR (COL_ERROR) else if INI_CONSOLE_LOAD_COLOR (
                COL_INFO) else if INI_CONSOLE_LOAD_COLOR (COL_TIMESTAMP) else if INI_CONSOLE_LOAD_FLOAT (m_WindowAlpha)

                else if INI_CONSOLE_LOAD_BOOL (m_AutoScroll) else if INI_CONSOLE_LOAD_BOOL (m_ScrollToBottom) else if INI_CONSOLE_LOAD_BOOL (m_ColoredOutput) else if INI_CONSOLE_LOAD_BOOL (
                        m_FilterBar) else if INI_CONSOLE_LOAD_BOOL (m_TimeStamps)

#pragma warning(pop)
}

void ImGuiConsole::SettingsHandler_ApplyAll(ImGuiContext *ctx, ImGuiSettingsHandler *handler) {
    if (!handler->UserData) return;
}

void ImGuiConsole::SettingsHandler_WriteAll(ImGuiContext *ctx, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buf) {
    if (!handler->UserData) return;

    auto console = static_cast<ImGuiConsole *>(handler->UserData);

#define INI_CONSOLE_SAVE_COLOR(type)                                                                                                                                               \
    buf->appendf(#type "=%i,%i,%i,%i\n", (int)(console->m_ColorPalette[type].x * 255), (int)(console->m_ColorPalette[type].y * 255), (int)(console->m_ColorPalette[type].z * 255), \
                 (int)(console->m_ColorPalette[type].w * 255))

#define INI_CONSOLE_SAVE_FLOAT(var) buf->appendf(#var "=%.3f\n", console->var)
#define INI_CONSOLE_SAVE_BOOL(var) buf->appendf(#var "=%i\n", console->var)

    buf->appendf("[%s][%s]\n", handler->TypeName, console->m_ConsoleName.data());

    INI_CONSOLE_SAVE_BOOL(m_AutoScroll);
    INI_CONSOLE_SAVE_BOOL(m_ScrollToBottom);
    INI_CONSOLE_SAVE_BOOL(m_ColoredOutput);
    INI_CONSOLE_SAVE_BOOL(m_FilterBar);
    INI_CONSOLE_SAVE_BOOL(m_TimeStamps);

    INI_CONSOLE_SAVE_FLOAT(m_WindowAlpha);
    INI_CONSOLE_SAVE_COLOR(COL_COMMAND);
    INI_CONSOLE_SAVE_COLOR(COL_LOG);
    INI_CONSOLE_SAVE_COLOR(COL_WARNING);
    INI_CONSOLE_SAVE_COLOR(COL_ERROR);
    INI_CONSOLE_SAVE_COLOR(COL_INFO);
    INI_CONSOLE_SAVE_COLOR(COL_TIMESTAMP);

    buf->append("\n");
}

void ProfilerDrawFrameNavigation(FrameInfo *_infos, uint32_t _numInfos) {
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

    ImGui::PlotHistogram("", (const float *)_infos, _numInfos, 0, "", 0.f, maxTime, ImVec2(_numInfos * 10, 50), sizeof(FrameInfo));

    // if (ImGui::IsMouseClicked(0) && (idx != -1)) {
    //     profilerFrameLoad(g_fileName, _infos[idx].m_offset, _infos[idx].m_size);
    // }

    ImGui::EndChild();

    ImGui::End();
}

int ProfilerDrawFrame(ProfilerFrame *_data, void *_buffer, size_t _bufferSize, bool _inGame, bool _multi) {
    int ret = 0;

    std::sort(&_data->m_scopes[0], &_data->m_scopes[_data->m_numScopes], customLess);

    ImGui::SetNextWindowPos(ImVec2(10.0f, _multi ? 160.0f : 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(900.0f, 480.0f), ImGuiCond_FirstUseEver);

    static bool pause = false;
    bool noMove = (pause && _inGame) || !_inGame;
    noMove = noMove && ImGui::GetIO().KeyCtrl;

    static ImVec2 winpos = ImGui::GetWindowPos();

    if (noMove) ImGui::SetNextWindowPos(winpos);

    ImGui::Begin(LANG("ui_profiler"), 0, noMove ? ImGuiWindowFlags_NoMove : 0);

    ImGui::BeginTabBar("ui_profiler_tabbar");

    if (ImGui::BeginTabItem(LANG("ui_frameinspector"))) {

        if (!noMove) winpos = ImGui::GetWindowPos();

        float deltaTime = ProfilerClock2ms(_data->m_endtime - _data->m_startTime, _data->m_CPUFrequency);
        float frameRate = 1000.0f / deltaTime;

        ImVec4 col = triColor(frameRate, METADOT_MINIMUM_FRAME_RATE, METADOT_DESIRED_FRAME_RATE);

        ImGui::Text("FPS: ");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::Text("%.1f    ", 1000.0f / deltaTime);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("%s", LANG("ui_frametime"));
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::Text("%.3f ms   ", deltaTime);
        ImGui::PopStyleColor();
        ImGui::SameLine();

        if (_inGame) {
            ImGui::Text("Average FPS: ");
            ImGui::PushStyleColor(ImGuiCol_Text, triColor(ImGui::GetIO().Framerate, METADOT_MINIMUM_FRAME_RATE, METADOT_DESIRED_FRAME_RATE));
            ImGui::SameLine();
            ImGui::Text("%.1f   ", ImGui::GetIO().Framerate);
            ImGui::PopStyleColor();
        } else {
            float prevFrameTime = ProfilerClock2ms(_data->m_prevFrameTime, _data->m_CPUFrequency);
            ImGui::SameLine();
            ImGui::Text("Prev frame: ");
            ImGui::PushStyleColor(ImGuiCol_Text, triColor(1000.0f / prevFrameTime, METADOT_MINIMUM_FRAME_RATE, METADOT_DESIRED_FRAME_RATE));
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
            ImGui::Checkbox("Pause captures   ", &pause);
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
            if (ImGui::Button("Save frame")) ret = ProfilerSave(_data, _buffer, _bufferSize);
        } else {
            ImGui::Text("Capture threshold: ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0, 1.0f, 1.0f, 1.0f), "%.2f ", _data->m_timeThreshold);
            ImGui::SameLine();
            ImGui::Text("ms   ");
            ImGui::SameLine();
            ImGui::Text("Threshold level: ");
            ImGui::SameLine();
            if (_data->m_levelThreshold == 0)
                ImGui::TextColored(ImVec4(0, 1.0f, 1.0f, 1.0f), "whole frame   ");
            else
                ImGui::TextColored(ImVec4(0, 1.0f, 1.0f, 1.0f), "%d   ", _data->m_levelThreshold);
        }

        ImGui::SameLine();
        resetZoom = ImGui::Button("Reset zoom and pan");

        const ImVec2 p = ImGui::GetCursorScreenPos();
        const ImVec2 s = ImGui::GetWindowSize();

        float frameStartX = p.x + 3.0f;
        float frameEndX = frameStartX + s.x - 23;
        float frameStartY = p.y;

        static PanAndZoon paz;

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

        paz.m_zoom = ProfilerMax(paz.m_zoom, 1.0f);

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

        ProfilerSetPaused(pause);
        ProfilerSetThreshold(threshold, thresholdLevel);

        static const int METADOT_MAX_FRAME_TIMES = 128;
        static float s_frameTimes[METADOT_MAX_FRAME_TIMES];
        static int s_currentFrame = 0;

        float maxFrameTime = 0.0f;
        if (_inGame) {
            frameStartY += 62.0f;

            if (s_currentFrame == 0) memset(s_frameTimes, 0, sizeof(s_frameTimes));

            if (!ProfilerIsPaused()) {
                s_frameTimes[s_currentFrame % METADOT_MAX_FRAME_TIMES] = deltaTime;
                ++s_currentFrame;
            }

            float frameTimes[METADOT_MAX_FRAME_TIMES];
            for (int i = 0; i < METADOT_MAX_FRAME_TIMES; ++i) {
                frameTimes[i] = s_frameTimes[(s_currentFrame + i) % METADOT_MAX_FRAME_TIMES];
                maxFrameTime = ProfilerMax(maxFrameTime, frameTimes[i]);
            }

            ImGui::Separator();
            ImGui::PlotHistogram("", frameTimes, METADOT_MAX_FRAME_TIMES, 0, "", 0.f, maxFrameTime, ImVec2(s.x - 9.0f, 45));
        } else {
            frameStartY += 12.0f;
            ImGui::Separator();
        }

        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        maxFrameTime = ProfilerMax(maxFrameTime, 0.001f);
        float pct30fps = 33.33f / maxFrameTime;
        float pct60fps = 16.66f / maxFrameTime;

        float minHistY = p.y + 6.0f;
        float maxHistY = p.y + 45.0f;

        float limit30Y = maxHistY - (maxHistY - minHistY) * pct30fps;
        float limit60Y = maxHistY - (maxHistY - minHistY) * pct60fps;

        if (pct30fps <= 1.0f) draw_list->AddLine(ImVec2(frameStartX - 3.0f, limit30Y), ImVec2(frameEndX + 3.0f, limit30Y), IM_COL32(255, 255, 96, 255));

        if (pct60fps <= 1.0f) draw_list->AddLine(ImVec2(frameStartX - 3.0f, limit60Y), ImVec2(frameEndX + 3.0f, limit60Y), IM_COL32(96, 255, 96, 255));

        if (_data->m_numScopes == 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.23f, 0.23f, 1.0f), "No scope data!");
            ImGui::End();
            return ret;
        }

        uint64_t threadID = _data->m_scopes[0].m_threadID;
        bool writeThreadName = true;

        uint64_t totalTime = _data->m_endtime - _data->m_startTime;

        float barHeight = 21.0f;
        float bottom = 0.0f;

        uint64_t currTime = ProfilerGetClock();

        for (uint32_t i = 0; i < _data->m_numScopes; ++i) {
            ProfilerScope &cs = _data->m_scopes[i];
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

            bottom = ProfilerMax(bottom, br.y);

            int level = cs.m_level;
            if (cs.m_level >= s_maxLevelColors) level = s_maxLevelColors - 1;

            ImU32 drawColor = s_levelColors[level];
            flashColorNamed(drawColor, cs, currTime - s_timeSinceStatClicked);

            if (ImGui::IsMouseClicked(0) && ImGui::IsMouseHoveringRect(tl, br) && ImGui::IsWindowHovered()) {
                s_timeSinceStatClicked = currTime;
                s_statClickedName = cs.m_name;
                s_statClickedLevel = cs.m_level;
            }

            if ((thresholdLevel == (int)cs.m_level + 1) && (threshold <= ProfilerClock2ms(cs.m_end - cs.m_start, _data->m_CPUFrequency))) flashColor(drawColor, currTime - _data->m_endtime);

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
                ImGui::TextColored(ImVec4(230, 230, 230, 255), "%.3f ms", ProfilerClock2ms(cs.m_end - cs.m_start, _data->m_CPUFrequency));
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
        ImGui::Text("MaximumMem: %llu mb", Core.max_mem / 1048576);
        ImGui::Text("MemCurrentUsage: %.2f mb", MemCurrentUsageMB());
        ImGui::Text("MemTotalAllocated: %.2lf mb", (F64)(g_AllocationMetrics.TotalAllocated / 1048576.0f));
        ImGui::Text("MemTotalFree: %.2lf mb", (F64)(g_AllocationMetrics.TotalFree / 1048576.0f));

#if defined(METADOT_DEBUG)
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        ImGui::Text("GC:\n");
        for (auto [name, size] : AllocationMetrics::MemoryDebugMap) {
            ImGui::Text("%s", MetaEngine::Format("   {0} {1}", name, size).c_str());
        }
        // ImGui::Auto(AllocationMetrics::MemoryDebugMap, "map");
#endif

        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();

    ImGui::End();
    return ret;
}

void ProfilerDrawStats(ProfilerFrame *_data, bool _multi) {
    ImGui::SetNextWindowPos(ImVec2(920.0f, _multi ? 160.0f : 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600.0f, 900.0f), ImGuiCond_FirstUseEver);

    ImGui::Begin("Frame stats");

    if (_data->m_numScopes == 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.23f, 0.23f, 1.0f), "No scope data!");
        ImGui::End();
        return;
    }

    float deltaTime = ProfilerClock2ms(_data->m_endtime - _data->m_startTime, _data->m_CPUFrequency);

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
        ProfilerScope &cs = _data->m_scopesStats[i];

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
            s_timeSinceStatClicked = ProfilerGetClock();
            s_statClickedName = cs.m_name;
            s_statClickedLevel = cs.m_level;
        }

        flashColorNamed(drawColor, cs, ProfilerGetClock() - s_timeSinceStatClicked);

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
                ttime = ProfilerClock2ms(cs.m_stats->m_exclusiveTimeTotal, _data->m_CPUFrequency);
                ImGui::TextColored(ImVec4(230, 230, 230, 255), "%.4f ms", ttime);
            } else {
                ImGui::TextColored(ImVec4(0, 255, 255, 255), "Inclusive time total: ");
                ImGui::SameLine();
                ttime = ProfilerClock2ms(cs.m_stats->m_inclusiveTimeTotal, _data->m_CPUFrequency);
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

extern void ShowAutoTestWindow();

#if defined(_METADOT_IMM32)

#if defined(_WIN32)
#include <CommCtrl.h>
#include <Windows.h>
#include <vcruntime_string.h>
#endif /* defined( _WIN32 ) */

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

class OpenGL3TextureManager : public ImGuiTextureManager {
public:
    ~OpenGL3TextureManager() {
        for (int i = 0; i < mTextures.size(); ++i) {
            GLuint tid = mTextures[i];
            glDeleteTextures(1, &tid);
        }
        mTextures.clear();
    }

    ImTextureID createTexture(void *pixels, int width, int height) {
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

    void deleteTexture(ImTextureID id) {
        GLuint tex = (GLuint)(intptr_t)id;
        glDeleteTextures(1, &tex);
    }

private:
    typedef ImVector<GLuint> Textures;

    Textures mTextures;
};

static bool firstRun = false;

#if defined(_METADOT_IMM32)
ImGUIIMMCommunication imguiIMMCommunication{};
#endif

static void *ImGuiMalloc(size_t sz, void *user_data) { return gc_malloc(&gc, sz); }
static void ImGuiFree(void *ptr, void *user_data) { gc_free(&gc, ptr); }

void ImGuiLayer::Init() {

    IMGUI_CHECKVERSION();

    // ImGui::SetAllocatorFunctions(ImGuiMalloc, ImGuiFree);

    m_imgui = ImGui::CreateContext();
    ImPlot::CreateContext();

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

    F32 scale = 1.0f;

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

#if defined(_METADOT_IMM32)
    common_control_initialize();
    VERIFY(imguiIMMCommunication.subclassify(window));
#endif

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

    auto create_console = [&]() {
        METADOT_NEW(C, console_imgui, ImGuiConsole, global.I18N.Get("ui_console"));

        // Our state
        ImVec4 clear_color = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);

        // Register variables
        console_imgui->System().RegisterVariable("background_color", clear_color, imvec4_setter);

        console_imgui->System().RegisterVariable("plPosX", global.GameData_.plPosX, Command::Arg<F32>(""));
        console_imgui->System().RegisterVariable("plPosY", global.GameData_.plPosY, Command::Arg<F32>(""));

        console_imgui->System().RegisterVariable("scale", Screen.gameScale, Command::Arg<I32>(""));

        visit_struct::for_each(global.game->GameIsolate_.globaldef, [&](const char *name, auto &value) { console_imgui->System().RegisterVariable(name, value, Command::Arg<int>("")); });

        // Register custom commands
        console_imgui->System().RegisterCommand("random_background_color", "Assigns a random color to the background application", [&clear_color]() {
            clear_color.x = (rand() % 256) / 256.f;
            clear_color.y = (rand() % 256) / 256.f;
            clear_color.z = (rand() % 256) / 256.f;
            clear_color.w = (rand() % 256) / 256.f;
        });
        console_imgui->System().RegisterCommand("reset_background_color", "Reset background color to its original value", [&clear_color, val = clear_color]() { clear_color = val; });

        console_imgui->System().RegisterCommand("print_methods", "a", [this]() {
            for (auto &cmds : console_imgui->System().Commands()) {
                console_imgui->System().Log(Command::ItemType::LOG) << "\t" << cmds.first << Command::endl;
            }
        });
        console_imgui->System().RegisterCommand(
                "lua", "dostring",
                [&](const char *s) {
                    auto &l = Scripting::GetSingletonPtr()->Lua;
                    l->s_lua.dostring(s);
                },
                Command::Arg<String>(""));

        console_imgui->System().RegisterVariable("tps", Time.maxTps, Command::Arg<U32>(""));

        console_imgui->System().Log(Command::ItemType::INFO) << "Welcome to the console!" << Command::endl;
        console_imgui->System().Log(Command::ItemType::INFO) << "The following variables have been exposed to the console:" << Command::endl << Command::endl;
        console_imgui->System().Log(Command::ItemType::INFO) << "\tbackground_color - set: [int int int int]" << Command::endl;
        console_imgui->System().Log(Command::ItemType::INFO) << Command::endl << "Try running the following command:" << Command::endl;
        console_imgui->System().Log(Command::ItemType::INFO) << "\tset background_color [255 0 0 255]" << Command::endl << Command::endl;
    };

    create_console();

    firstRun = true;
}

void ImGuiLayer::End() {

    METADOT_DELETE(C, console_imgui, ImGuiConsole);

    RendererShutdownFunction();
    PlatformShutdownFunction();
    ImPlot::DestroyContext();
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

#if defined(_METADOT_IMM32)
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
        ImPlot::ShowDemoWindow();
    }

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
            METADOT_ASSERT_E(0);
        }
        ImGui::End();

        ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
        if (ImGui::Begin(LANG("ui_tweaks"), NULL, ImGuiWindowFlags_MenuBar)) {
            ImGui::BeginTabBar("ui_tweaks_tabbar");

            if (ImGui::BeginTabItem(ICON_LANG(ICON_FA_TERMINAL, "ui_console"))) {
                console_imgui->Draw();
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

                MarkdownData TickInfoPanel;
                TickInfoPanel.data = R"(
Info &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; | Data &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; | Comments &nbsp;
:-----------|:------------|:--------
TickCount | {0} | Nothing
DeltaTime | {1} | Nothing
TPS | {2} | Nothing
Mspt | {3} | Nothing
Delta | {4} | Nothing
STDTime | {5} | Nothing
CSTDTime | {6} | Nothing)";

                time_t rawtime;
                rawtime = time(NULL);
                struct tm *timeinfo = localtime(&rawtime);

                TickInfoPanel.data = MetaEngine::Format(TickInfoPanel.data, Time.tickCount, Time.deltaTime, Time.tps, Time.mspt, Time.now - Time.lastTickTime, metadot_gettime(), rawtime);

                ImGui::Auto(TickInfoPanel);

                ImGui::Text("\nnow: %d-%02d-%02d %02d:%02d:%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

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

                        vec4 pl_transform{-global.game->GameIsolate_.world->loadZone.x + global.game->GameIsolate_.world->tickZone.x + global.game->GameIsolate_.world->tickZone.w / 2.0f,
                                          -global.game->GameIsolate_.world->loadZone.y + global.game->GameIsolate_.world->tickZone.y + global.game->GameIsolate_.world->tickZone.h / 2.0f, 10, 20};

                        b2PolygonShape sh;
                        sh.SetAsBox(pl_transform.z / 2.0f + 1, pl_transform.w / 2.0f);
                        RigidBody *rb = global.game->GameIsolate_.world->makeRigidBody(b2BodyType::b2_kinematicBody, pl_transform.pos.x + pl_transform.rect.x / 2.0f - 0.5,
                                                                                       pl_transform.pos.y + pl_transform.rect.y / 2.0f - 0.5, 0, sh, 1, 1, NULL);
                        rb->body->SetGravityScale(0);
                        rb->body->SetLinearDamping(0);
                        rb->body->SetAngularDamping(0);

                        auto npc = global.game->GameIsolate_.world->Reg().create_entity();
                        MetaEngine::ECS::entity_filler(npc)
                                .component<Controlable>()
                                .component<WorldEntity>(true, pl_transform.pos.x, pl_transform.pos.y, 0.0f, 0.0f, (int)pl_transform.rect.x, (int)pl_transform.rect.y, rb, std::string("NPC"))
                                .component<Bot>(1);

                        auto npc_bot = global.game->GameIsolate_.world->Reg().find_component<Bot>(npc);
                        // npc_bot->setItemInHand(global.game->GameIsolate_.world->Reg().find_component<WorldEntity>(global.game->GameIsolate_.world->player), i3, global.game->GameIsolate_.world.get());
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Audio")) {
                        play ^= true;
                        static METAENGINE_Audio *test_audio = metadot_audio_load_wav(METADOT_RESLOC("data/assets/audio/02_c03_normal_135.wav"));
                        if (play) {
                            METADOT_ASSERT_E(test_audio);
                            // metadot_music_play(test_audio, 0.f);
                            // metadot_audio_destroy(test_audio);
                            METAENGINE_Result err;
                            metadot_play_sound(test_audio, metadot_sound_params_defaults(), &err);
                        } else {
                            MetaEngine::audio_set_pause(false);
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("StaticRefl")) {

                        if (global.game->GameIsolate_.world == nullptr || !global.game->GameIsolate_.world->isPlayerInWorld()) {
                        } else {
                            using namespace MetaEngine::StaticRefl;

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
                    ImGui::Checkbox("UI", &global.game->GameIsolate_.ui->uidata->elementLists["testElement1"]->visible);
                    if (ImGui::Button("Meo")) {
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem(CC("自动序列测试"))) {
                    ShowAutoTestWindow();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(ICON_LANG(ICON_FA_DESKTOP, "ui_debug"))) {
                if (CollapsingHeader(ICON_LANG(ICON_FA_VECTOR_SQUARE, "ui_telemetry"))) {
                    GameUI::DrawDebugUI(global.game);
                }
#define INSPECTSHADER(_c) MetaEngine::IntrospectShader(#_c, global.game->GameIsolate_.shaderworker->_c->shader)
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

                    static vec2 check_chunk = {1, 1};
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
                        MetaEngine::StaticRefl::TypeInfo<Chunk>::ForEachVarOf(*check_chunk_ptr, [&](const auto &field, auto &&var) {
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
                    //     MetaEngine::StaticRefl::TypeInfo<RigidBody>::ForEachVarOf(*check_rigidbody_ptr, [&](const auto &field, auto &&var) { ImGui::Auto(var, std::string(field.name)); });
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

                    // visit_struct::for_each(global.game->GameIsolate_.globaldef, [&](const char *name, auto &value) { console_imgui->System().RegisterVariable(name, value, Command::Arg<int>(""));
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
                if (ImGui::BeginTabItem(FUtil_GetFileName(view.file.c_str()))) {
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
                                    editor.CanUndo() ? "*" : " ", editor.GetLanguageDefinition().mName.c_str(), FUtil_GetFileName(view_editing->file.c_str()));

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

                visit_struct::for_each(global.game->GameIsolate_.globaldef, ShowCVar);

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }
}