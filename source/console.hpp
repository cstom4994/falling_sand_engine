// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_CONSOLE_HPP_
#define _METADOT_CONSOLE_HPP_

#include <array>

#include "cvar.hpp"
#include "game_datastruct.hpp"
#include "imgui/imgui_impl.hpp"

struct ImGuiSettingsHandler;
class ImGuiConsole {
public:
    explicit ImGuiConsole(std::string c_name = "Console", size_t inputBufferSize = 256);

    void Draw();

    CVar::System &System();

protected:
    CVar::System m_ConsoleSystem;
    size_t m_HistoryIndex;

    std::string m_Buffer;
    std::string m_ConsoleName;
    ImGuiTextFilter m_TextFilter;
    bool m_AutoScroll;
    bool m_ColoredOutput;
    bool m_ScrollToBottom;
    bool m_FilterBar;
    bool m_TimeStamps;

    void InitIniSettings();
    void DefaultSettings();
    void RegisterConsoleCommands();

    void MenuBar();
    void FilterBar();
    void InputBar();
    void LogWindow();

    static void HelpMaker(const char *desc);

    F32 m_WindowAlpha;

    enum COLOR_PALETTE {

        COL_COMMAND = 0,
        COL_LOG,
        COL_WARNING,
        COL_ERROR,
        COL_INFO,

        COL_TIMESTAMP,

        COL_COUNT
    };

    std::array<ImVec4, COL_COUNT> m_ColorPalette;

    static int InputCallback(ImGuiInputTextCallbackData *data);
    bool m_WasPrevFrameTabCompletion = false;
    std::vector<std::string> m_CmdSuggestions;

    bool m_LoadedFromIni = false;

    static void SettingsHandler_ClearALl(ImGuiContext *ctx, ImGuiSettingsHandler *handler);

    static void SettingsHandler_ReadInit(ImGuiContext *ctx, ImGuiSettingsHandler *handler);

    static void *SettingsHandler_ReadOpen(ImGuiContext *ctx, ImGuiSettingsHandler *handler, const char *name);

    static void SettingsHandler_ReadLine(ImGuiContext *ctx, ImGuiSettingsHandler *handler, void *entry, const char *line);

    static void SettingsHandler_ApplyAll(ImGuiContext *ctx, ImGuiSettingsHandler *handler);

    static void SettingsHandler_WriteAll(ImGuiContext *ctx, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buf);
};

class ConsoleSystem : public IGameSystem {
public:
    ImGuiConsole *console_imgui;

    void DrawUI();
    void Draw();

    ConsoleSystem(U32 p) : IGameSystem(p){};

    // InternalFuncs
    void PrintAllMethods();

    void Create() override;
    void Destory() override;
    void RegisterLua(LuaWrapper::State &s_lua) override;
};

#endif
