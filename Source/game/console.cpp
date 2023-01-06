// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "console.hpp"

#include <cstring>
#include <string>

#include "core/debug_impl.hpp"
#include "core/global.hpp"
#include "cvar.hpp"
#include "engine/memory.hpp"
#include "engine/reflectionflat.hpp"
#include "game/game.hpp"

#define LANG(_c) global.I18N.Get(_c).c_str()

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

CVar::System &ImGuiConsole::System() { return m_ConsoleSystem; }

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
            CVar::Arg<String>("filter_str"));

    // m_ConsoleSystem.RegisterCommand(
    //         "run", "Run given script",
    //         [this](const String &filter) { m_ConsoleSystem.RunScript(filter.m_String); },
    //         CVar::Arg<String>("script_name"));
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

            if (item.m_Type == CVar::COMMAND) {
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

            if (item.m_Type == CVar::COMMAND && m_TimeStamps) {

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
            CVar::AutoComplete *console_autocomplete;

            if (startSubtrPos == std::string::npos) {
                startSubtrPos = 0;
                console_autocomplete = &console->m_ConsoleSystem.CmdAutocomplete();
            } else {
                startSubtrPos += 1;
                console_autocomplete = &console->m_ConsoleSystem.VarAutocomplete();
            }

            if (!trim_str.empty()) {

                if (!console->m_CmdSuggestions.empty()) {
                    console->m_ConsoleSystem.Log(CVar::COMMAND) << "Suggestions: " << CVar::endl;

                    for (const auto &suggestion : console->m_CmdSuggestions) console->m_ConsoleSystem.Log(CVar::LOG) << suggestion << CVar::endl;

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

void Console::Init() {
    METADOT_NEW(C, console, ImGuiConsole, global.I18N.Get("ui_console"));

    // Our state
    ImVec4 clear_color = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);

    // Register variables
    console->System().RegisterVariable("background_color", clear_color, imvec4_setter);

    console->System().RegisterVariable("plPosX", GameData_.plPosX, CVar::Arg<F32>(""));
    console->System().RegisterVariable("plPosY", GameData_.plPosY, CVar::Arg<F32>(""));

    console->System().RegisterVariable("scale", global.game->scale, CVar::Arg<int>(""));

    visit_struct::for_each(global.game->GameIsolate_.globaldef, [&](const char *name, auto &value) { console->System().RegisterVariable(name, value, CVar::Arg<int>("")); });

    // Register custom commands
    console->System().RegisterCommand("random_background_color", "Assigns a random color to the background application", [&clear_color]() {
        clear_color.x = (rand() % 256) / 256.f;
        clear_color.y = (rand() % 256) / 256.f;
        clear_color.z = (rand() % 256) / 256.f;
        clear_color.w = (rand() % 256) / 256.f;
    });
    console->System().RegisterCommand("reset_background_color", "Reset background color to its original value", [&clear_color, val = clear_color]() { clear_color = val; });

    console->System().RegisterCommand("print_methods", "a", [this]() { PrintAllMethods(); });
    console->System().RegisterCommand(
            "lua", "dostring",
            [&](const char *s) {
                auto l = global.scripts->LuaRuntime;
                l->GetWrapper()->dostring(s);
            },
            CVar::Arg<String>(""));

    console->System().Log(CVar::ItemType::INFO) << "Welcome to the console!" << CVar::endl;
    console->System().Log(CVar::ItemType::INFO) << "The following variables have been exposed to the console:" << CVar::endl << CVar::endl;
    console->System().Log(CVar::ItemType::INFO) << "\tbackground_color - set: [int int int int]" << CVar::endl;
    console->System().Log(CVar::ItemType::INFO) << CVar::endl << "Try running the following command:" << CVar::endl;
    console->System().Log(CVar::ItemType::INFO) << "\tset background_color [255 0 0 255]" << CVar::endl << CVar::endl;
}

void Console::End() { METADOT_DELETE(C, console, ImGuiConsole); }

void Console::DrawUI() {
    METADOT_ASSERT_E(console);
    console->Draw();
}

void Console::PrintAllMethods() {
    METADOT_ASSERT_E(console);
    for (auto &cmds : console->System().Commands()) {
        console->System().Log(CVar::ItemType::LOG) << "\t" << cmds.first << CVar::endl;
    }
}
