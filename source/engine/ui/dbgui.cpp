// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "dbgui.hpp"

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
#include "engine/core/base_debug.hpp"
#include "engine/core/base_memory.h"
#include "engine/core/const.h"
#include "engine/core/core.hpp"
#include "engine/core/dbgtools.h"
#include "engine/core/global.hpp"
#include "engine/core/io/filesystem.h"
#include "engine/core/macros.hpp"
#include "engine/engine.hpp"
#include "engine/game.hpp"
#include "engine/game_datastruct.hpp"
#include "engine/game_ui.hpp"
#include "engine/meta/metadesk/md.h"
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
#include "engine/utils/utility.hpp"
#include "game/items.hpp"
#include "game/player.hpp"
#include "libs/glad/glad.h"

#if defined(_WIN32)
#include <CommCtrl.h>
#include <Psapi.h>
#include <Windows.h>
#include <vcruntime_string.h>
#endif

#define INSPECTSHADER(_c) ::ME::inspect_shader(#_c, global.game->Iso.shaderworker->_c->shader)

namespace ME {

namespace meta_sr = meta::static_refl;

extern int test_wang();

static MD_Arena *arena = 0;

ME_PRIVATE(std::size_t) imgui_mem_usage = 0;

auto CollapsingHeader = [](const char *name) -> bool {
    ImGuiStyle &style = ImGui::GetStyle();
    ImGui::PushStyleColor(ImGuiCol_Header, style.Colors[ImGuiCol_Button]);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, style.Colors[ImGuiCol_ButtonHovered]);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, style.Colors[ImGuiCol_ButtonActive]);
    bool b = ImGui::CollapsingHeader(name);
    ImGui::PopStyleColor(3);
    return b;
};

void DrawDebugUI(game *game) {

    try {

        const meta::r::scope cvar_scope = meta::r::local_scope_("cvar").typedef_<GlobalDEF>("GlobalDEF");

        for (const auto &[type_name, type] : cvar_scope.get_typedefs()) {
            if (!type.is_class()) {
                continue;
            }

            const meta::r::class_type &class_type = type.as_class();
            const meta::r::metadata_map &class_metadata = class_type.get_metadata();

            if (CollapsingHeader(type_name.c_str())) {
                for (const meta::r::member &member : class_type.get_members()) {
                    const meta::r::metadata_map &member_metadata = member.get_metadata();

                    auto f = [&]<typename T>(T arg) {
                        if (member.get_type().get_value_type() == meta::r::resolve_type<T>()) {
                            auto f = member.get(global.game->Iso.globaldef);
                            if (T *s = f.try_as<T>()) {
                                if (member_metadata.find("imgui") != member_metadata.end()) {
                                    auto editor_ui_type = member_metadata.at("imgui").as<std::string>();
                                    if (editor_ui_type == "float_range") {
                                        T t = *s;
                                        ImGui::SliderFloat(member.get_name().c_str(), (float *)&t, member_metadata.at("min").as<float>(), member_metadata.at("max").as<float>(), "", 0);
                                        if (t != *s) member.try_set(global.game->Iso.globaldef, t);
                                    }
                                } else {
                                    T t = *s;
                                    ImGui::Auto(t, member.get_name().c_str());
                                    if (t != *s) member.try_set(global.game->Iso.globaldef, t);
                                }
                                ImGui::Text("    [%s]", member_metadata.at("info").as<std::string>().c_str());
                            } else {
                                ImGui::TextColored({1.0f, 0.0f, 0.0f, 1.0f}, "(error) %s", member.get_name().c_str());
                            }
                        }
                    };

                    using cvar_types_t = std::tuple<CVAR_TYPES()>;

                    cvar_types_t ctt;
                    std::apply([&](auto &&...args) { (f(args), ...); }, ctt);
                }
            }
        }
    } catch (const std::exception ex) {
        METADOT_ERROR(std::format("[Exception] {0}", ex.what()).c_str());
    }

    ImGui::Separator();

    if (ImGui::Checkbox("Draw Background", &global.game->Iso.globaldef.draw_background)) {
        for (int x = 0; x < game->Iso.world->width; x++) {
            for (int y = 0; y < game->Iso.world->height; y++) {
                game->Iso.world->dirty[x + y * game->Iso.world->width] = true;
                game->Iso.world->layer2Dirty[x + y * game->Iso.world->width] = true;
            }
        }
    }

    if (ImGui::Checkbox("Draw Background Grid", &global.game->Iso.globaldef.draw_background_grid)) {
        for (int x = 0; x < game->Iso.world->width; x++) {
            for (int y = 0; y < game->Iso.world->height; y++) {
                game->Iso.world->dirty[x + y * game->Iso.world->width] = true;
                game->Iso.world->layer2Dirty[x + y * game->Iso.world->width] = true;
            }
        }
    }

    if (ImGui::Checkbox("HD Objects", &global.game->Iso.globaldef.hd_objects)) {
        R_FreeTarget(game->TexturePack_.textureObjects->target);
        R_FreeImage(game->TexturePack_.textureObjects);
        R_FreeTarget(game->TexturePack_.textureObjectsBack->target);
        R_FreeImage(game->TexturePack_.textureObjectsBack);
        R_FreeTarget(game->TexturePack_.textureEntities->target);
        R_FreeImage(game->TexturePack_.textureEntities);

        game->TexturePack_.textureObjects =
                R_CreateImage(game->Iso.world->width * (global.game->Iso.globaldef.hd_objects ? global.game->Iso.globaldef.hd_objects_size : 1),
                              game->Iso.world->height * (global.game->Iso.globaldef.hd_objects ? global.game->Iso.globaldef.hd_objects_size : 1), R_FormatEnum::R_FORMAT_RGBA);
        R_SetImageFilter(game->TexturePack_.textureObjects, R_FILTER_NEAREST);

        game->TexturePack_.textureObjectsBack =
                R_CreateImage(game->Iso.world->width * (global.game->Iso.globaldef.hd_objects ? global.game->Iso.globaldef.hd_objects_size : 1),
                              game->Iso.world->height * (global.game->Iso.globaldef.hd_objects ? global.game->Iso.globaldef.hd_objects_size : 1), R_FormatEnum::R_FORMAT_RGBA);
        R_SetImageFilter(game->TexturePack_.textureObjectsBack, R_FILTER_NEAREST);

        R_LoadTarget(game->TexturePack_.textureObjects);
        R_LoadTarget(game->TexturePack_.textureObjectsBack);

        game->TexturePack_.textureEntities =
                R_CreateImage(game->Iso.world->width * (global.game->Iso.globaldef.hd_objects ? global.game->Iso.globaldef.hd_objects_size : 1),
                              game->Iso.world->height * (global.game->Iso.globaldef.hd_objects ? global.game->Iso.globaldef.hd_objects_size : 1), R_FormatEnum::R_FORMAT_RGBA);
        R_SetImageFilter(game->TexturePack_.textureEntities, R_FILTER_NEAREST);

        R_LoadTarget(game->TexturePack_.textureEntities);
    }

    if (CollapsingHeader(CC("GLSL"))) {

        if (ImGui::Button(CC("重新加载GLSL"))) {
            global.game->Iso.shaderworker->reload();
        }

        const char *items[] = {"off", "flow map", "distortion"};
        const char *combo_label = items[global.game->Iso.globaldef.water_overlay];
        ImGui::SetNextItemWidth(80 + 24);
        if (ImGui::BeginCombo("Overlay", combo_label, 0)) {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
                const bool is_selected = (global.game->Iso.globaldef.water_overlay == n);
                if (ImGui::Selectable(items[n], is_selected)) {
                    global.game->Iso.globaldef.water_overlay = n;
                }

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::SeparatorText(CC("Shaders"));

        INSPECTSHADER(newLightingShader);
        INSPECTSHADER(fireShader);
        INSPECTSHADER(fire2Shader);
        INSPECTSHADER(waterShader);
        INSPECTSHADER(waterFlowPassShader);
        INSPECTSHADER(untexturedShader);
        INSPECTSHADER(blurShader);
    }
}

typedef struct ConsoleArgv {
    char **argv, *data;
    const char *error;
    int argc, data_length, error_index, error_code;
} ConsoleArgv;

void ConsoleArgvParseFree(ConsoleArgv *props) {
    free(props->data);
    free(props->argv);
    free(props);
}

ConsoleArgv *ConsoleArgvParse(const char *input) {

    auto field_seperator = [](char seperator) -> int {
        const char *list = " \t\n";
        if (seperator) {
            while (*list) {
                if (*list++ == seperator) return 1;
            }
            return 0;
        }
        return 1;  // null is always a field seperator
    };

    ConsoleArgv *console_argv = (ConsoleArgv *)calloc(1, sizeof(ConsoleArgv));

    if (!input) {
        console_argv->error_code = 1;
        console_argv->error = "cannot parse null pointer";
        return console_argv;
    }

    /* Get the input length */
    long input_length = -1;
test_next_input_char:
    if (input[++input_length]) goto test_next_input_char;

    if (!input_length) {
        console_argv->error_code = 2;
        console_argv->error = "cannot parse empty input";
        return console_argv;
    }

    int composing_argument = 0;
    long quote = 0;
    long index;
    char look_ahead;

    // FIRST PASS
    // discover how many elements we have, and discover how large a data buffer we need
    for (index = 0; index <= input_length; index++) {

        if (field_seperator(input[index])) {
            if (composing_argument) {
                // close the argument
                composing_argument = 0;
                console_argv->data_length++;
            }
            continue;
        }

        if (!composing_argument) {
            console_argv->argc++;
            composing_argument = 1;
        }

        switch (input[index]) {

            /* back slash */
            case '\\':
                // If the sequence is not \' or \" or seperator copy the back slash, and
                // the data
                look_ahead = *(input + index + 1);
                if (look_ahead == '"' || look_ahead == '\'' || field_seperator(look_ahead)) {
                    index++;
                } else {
                    index++;
                    console_argv->data_length++;
                }
                break;

            /* double quote */
            case '"':
                quote = index;
                while (input[++index] != '"') {
                    switch (input[index]) {
                        case '\0':
                            console_argv->error_index = quote + 1;
                            console_argv->error_code = 3;
                            console_argv->error = "unterminated double quote";
                            return console_argv;
                            break;
                        case '\\':
                            look_ahead = *(input + index + 1);
                            if (look_ahead == '"') {
                                index++;
                            } else {
                                index++;
                                console_argv->data_length++;
                            }
                            break;
                    }
                    console_argv->data_length++;
                }

                continue;
                break;

            /* single quote */
            case '\'':
                /* copy single quoted data */
                quote = index;  // QT used as temp here...
                while (input[++index] != '\'') {
                    if (!input[index]) {
                        // unterminated double quote @ input
                        console_argv->error_index = quote + 1;
                        console_argv->error_code = 4;
                        console_argv->error = "unterminated single quote";
                        return console_argv;
                    }
                    console_argv->data_length++;
                }
                continue;
                break;
        }

        // "record" the data
        console_argv->data_length++;
    }

    // +1 for extra NULL pointer required by execv() and friends
    console_argv->argv = (char **)calloc(console_argv->argc + 1, sizeof(char *));
    console_argv->data = (char *)calloc(console_argv->data_length, 1);

    // SECOND PASS
    composing_argument = 0;
    quote = 0;

    int data_index = 0;
    int arg_index = 0;

    for (index = 0; index <= input_length; index++) {

        if (field_seperator(input[index])) {
            if (composing_argument) {
                composing_argument = 0;
                console_argv->data[data_index++] = '\0';
            }
            continue;
        }

        if (!composing_argument) {
            console_argv->argv[arg_index++] = (console_argv->data + data_index);
            composing_argument = 1;
        }

        switch (input[index]) {

            /* back slash */
            case '\\':
                // If the sequence is not \' or \" or field seperator copy the backslash
                look_ahead = *(input + index + 1);
                if (look_ahead == '"' || look_ahead == '\'' || field_seperator(look_ahead)) {
                    index++;
                } else {
                    console_argv->data[data_index++] = input[index++];
                }
                break;

            /* double quote */
            case '"':
                while (input[++index] != '"') {
                    if (input[index] == '\\') {
                        look_ahead = *(input + index + 1);
                        if (look_ahead == '"') {
                            index++;
                        } else {
                            console_argv->data[data_index++] = input[index++];
                        }
                    }
                    console_argv->data[data_index++] = input[index];
                }
                continue;
                break;

            /* single quote */
            case '\'':
                /* copy single quoted data */
                while (input[++index] != '\'') {
                    console_argv->data[data_index++] = input[index];
                }
                continue;
                break;
        }

        // "record" the data
        console_argv->data[data_index++] = input[index];
    }

    return console_argv;
}

void console::set_log_colour(ImVec4 colour, log_type type) noexcept {
    switch (type) {
        case log_type::warning:
            warning = colour;
            return;
        case log_type::error:
            error = colour;
            return;
        case log_type::note:
            note = colour;
            return;
        case log_type::trace:
            message = colour;
            return;
    }
}

void console::display(bool *bInteractingWithTextbox) noexcept {
    for (auto &a : logger::message_log()) {
        ImVec4 colour;
        switch (a.type) {
            case log_type::warning:
                colour = warning;
                break;
            case log_type::error:
                colour = error;
                break;
            case log_type::note:
                colour = note;
                break;
            case log_type::trace:
                colour = message;
                break;
        }

        ImGui::TextColored(colour, "%s", a.msg.c_str());
    }

    static std::string command;
    if ((ImGui::InputTextWithHint("##Input", "Enter any command here", &command) || ImGui::IsItemActive()) && bInteractingWithTextbox != nullptr) *bInteractingWithTextbox = true;
    ImGui::SameLine();
    if (ImGui::Button("Send##consoleCommand") || (*bInteractingWithTextbox && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))) {
        // for (auto &a : loggerInternal.commands) {
        //     if (command.rfind(a.cmd, 0) == 0) {
        //         a.func(command);
        //         break;
        //     }
        // }
        // ME_scripting::get_singleton_ptr()->run_command(command);

        this->eval(command);

        command.clear();
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
}

void console::draw_internal_display() noexcept {

    i64 now = ME_gettime();

    log_msg log_list[10] = {};
    int n = 9;

    std::vector<log_msg>::const_reverse_iterator backwardIterator;
    auto &logger_list = logger::message_log();
    for (backwardIterator = logger_list.crbegin(); backwardIterator != logger_list.crend(); backwardIterator++) {
        if (n < 0) break;

        i64 dtime = now - backwardIterator->time;

        if (dtime > 4000) {
            n--;
            break;  // 直接跳出循环就可以，在此之后的日志只可能比这更老的
        }
        log_list[n] = *backwardIterator;
        n--;
    }

    int x = 10, y = 10;

    if (global.game->Iso.globaldef.ui_tweak) y += 10;

    ImDrawList *draw_list = ImGui::GetBackgroundDrawList();

    for (auto &a : log_list) {

        if (a.msg.empty()) continue;

        i64 dtime = now - a.time;

        ImVec4 colour;
        switch (a.type) {
            case log_type::warning:
                colour = warning;
                break;
            case log_type::error:
                colour = error;
                break;
            case log_type::note:
                colour = note;
                break;
            case log_type::trace:
                colour = message;
                break;
        }

        bool outline = true;

        if (dtime >= 3500) {
            colour.w = std::abs((4000 - dtime) / 500.0f);
            outline = false;
        }

        ME_draw_text(a.msg, ME_imvec2rgba(colour), x, y, true);
        y = y + 12;
    }
}

void console::add_to_message_log(const std::string &msg, log_type type) noexcept {
    // logger::message_log.emplace_back(log_msg{msg, type});
}

void console::init() {
    convar.Command("help", [this]() {
        METADOT_INFO("All commands");
        for (auto &p : this->convar) {
            this->print_command_info(p.second);
        }
    });

    // dostruct::for_each(global.game->Iso.globaldef, [&](const char *name, auto &value) {
    //     // console_imgui->System().RegisterVariable(name, value, Command::Arg<int>(""));
    //     convar.Value(name, value);
    // });

    convar.Value("game_scale", the<engine>().eng()->render_scale);
}

void console::end() {}

void console::draw() {
    bool interactingWithTextbox;
    this->draw_internal_display();
    if (global.game->Iso.globaldef.draw_console) this->display_full(&interactingWithTextbox);
}

void console::print_command_info(cvar::BaseCommand *cmd) {
    switch (cmd->GetCmdType()) {
        case CommandType::CVAR_VAR:
            METADOT_INFO(std::format(" - {0} {1}", cmd->GetReturnType(), cmd->GetName()).c_str());
            break;

        case CommandType::CVAR_FUNC:
            std::string params;
            bool first = true;
            for (const cvar::CommandParameter &p : *cmd) {
                if (!first) params.append(", ");
                first = false;
                params.append(p.GetType());
            }

            METADOT_INFO(std::format(" - {0} {1} ({2})", cmd->GetReturnType(), cmd->GetName(), params).c_str());
            break;
    }
}

std::string console::execute(std::string cmd_name, std::queue<std::string> arguments, console_result &ret) {
    std::string result;

    try {
        result = convar.Call(cmd_name, arguments);
        ret = OK;
    } catch (exception::exception_cvar &e) {
        METADOT_ERROR(std::format("[ConVar] exception thrown {0} : {1}", cmd_name, e.what()).c_str());
        ret = ERR;
    } catch (...) {
    }

    return result;
}

bool console::eval(std::string &cmd) {

    std::unique_ptr<ConsoleArgv, void (*)(ConsoleArgv *)> parsedArgs(ConsoleArgvParse(cmd.c_str()), ConsoleArgvParseFree);

    switch (parsedArgs->error_code) {
        case 1:
            break;

        case 2:
            return false;

        case 3:
        case 4:
            METADOT_ERROR(std::format("[ConVar] Syntax error: {0} at column {1}", parsedArgs->error, parsedArgs->error_index).c_str());
            return false;
    }

    std::string cmd_name(parsedArgs->argv[0]);
    if (!cmd_name.empty() && cmd_name[0] == '#') return false;

    std::queue<std::string> args;
    for (int i = 1; i < parsedArgs->argc; ++i) args.push(parsedArgs->argv[i]);

    console_result code;
    std::string result = this->execute(cmd_name, args, code);

    if (!result.empty()) METADOT_INFO(result.c_str());
    if (code == EXIT) return true;

    return false;
}

void console::display_full(bool *bInteractingWithTextbox) noexcept {
    ImGui::Begin(LANG("ui_console"));
    display(bInteractingWithTextbox);
    ImGui::End();
}

void pack_editor::init() { file = ME_fs_get_path("data/scripts"); }

void pack_editor::end() {
    if (pack_reader_is_loaded) {
        ME_destroy_pack_reader(pack_reader);
    }
}

void pack_editor::draw() {

    if (!global.game->Iso.globaldef.draw_pack_editor) return;

    if (ImGui::Begin("包编辑器")) {

        if (ImGui::Button("打开包") && !pack_reader_is_loaded) filebrowser = true;
        ImGui::SameLine();
        if (ImGui::Button("关闭包")) {
            ME_destroy_pack_reader(pack_reader);
            pack_reader_is_loaded = false;
        }
        ImGui::SameLine();
        ImGui::Text("MetaDot Pack [%d.%d.%d]\n", PACK_VERSION_MAJOR, PACK_VERSION_MINOR, PACK_VERSION_PATCH);

        if (filebrowser) {
            ImGuiHelper::file_browser(file);

            if (!file.empty() && !std::filesystem::is_directory(file)) {

                if (pack_reader_is_loaded) {

                } else {
                    result = ME_get_pack_info(file.c_str(), &majorVersion, &minorVersion, &patchVersion, &isLittleEndian, &itemCount);
                    if (result != SUCCESS_PACK_RESULT) return;

                    result = ME_create_file_pack_reader(file.c_str(), 0, false, &pack_reader);
                    if (result != SUCCESS_PACK_RESULT) return;

                    if (result == SUCCESS_PACK_RESULT) itemCount = ME_get_pack_item_count(pack_reader);

                    pack_reader_is_loaded = true;
                }

                // 复位变量
                filebrowser = false;
                file = ME_fs_get_path("data/scripts");
            }
        }

        if (pack_reader_is_loaded && result == SUCCESS_PACK_RESULT) {

            static int item_current_idx = -1;

            ImGui::Text(
                    "包信息:\n"
                    " 打包版本: %d.%d.%d\n"
                    " 小端模式: %s\n"
                    " 文件数量: %llu\n\n",
                    majorVersion, minorVersion, patchVersion, isLittleEndian ? "true" : "false", (long long unsigned int)itemCount);

            ImVec2 outer_size = ImVec2(0.0f, ImGui::GetContentRegionMax().y - 250.0f);
            if (ImGui::BeginTable("ui_pack_file_table", 4,
                                  ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg |
                                          ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti,
                                  outer_size)) {
                ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
                ImGui::TableSetupColumn("索引", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("文件名", ImGuiTableColumnFlags_WidthFixed, 550.0f);
                ImGui::TableSetupColumn("大小", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();

                for (u64 i = 0; i < itemCount; ++i) {
                    ImGui::PushID(i);
                    ImGui::TableNextRow(ImGuiTableRowFlags_None);
                    if (ImGui::TableSetColumnIndex(0)) {
                        // if (ImGui::Selectable(std::format("{0}", ).c_str(), item_current_idx, ImGuiSelectableFlags_SpanAllColumns)) {
                        //     item_current_idx = i;
                        // }
                        ImGui::Text("%llu", (long long unsigned int)i);
                    }
                    if (ImGui::TableSetColumnIndex(1)) {
                        ImGui::Text("%s", ME_get_pack_item_path(pack_reader, i));
                    }
                    if (ImGui::TableSetColumnIndex(2)) {
                        ImGui::Text("%u", ME_get_pack_item_data_size(pack_reader, i));
                    }
                    if (ImGui::TableSetColumnIndex(3)) {
                        if (ImGui::SmallButton("查看")) {
                            item_current_idx = i;
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("替换")) {
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("删除")) {
                        }
                    }
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }

            if (item_current_idx >= 0) {
                ImGui::Text("文件索引: %u\n包内路径: %s\n文件大小: %u\n", item_current_idx, ME_get_pack_item_path(pack_reader, item_current_idx),
                            ME_get_pack_item_data_size(pack_reader, item_current_idx));
            }

        } else if (result != SUCCESS_PACK_RESULT) {
            ImGui::Text("错误: %s.\n", pack_result_to_string(result));
        }
    }
    ImGui::End();
}

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

        static u32 check_timer;

        ImGui::Text("MemCurrentUsage: %.2f mb", ME_mem_current_usage_mb());
        ImGui::Text("MemTotalAllocated: %.2lf mb", (f64)(g_allocation_metrics.total_allocated / 1048576.0));
        ImGui::Text("MemTotalFree: %.2lf mb", (f64)(g_allocation_metrics.total_free / 1048576.0));
        ImGui::Text("GC MemAllocInUsed: %.2lf mb", (f64)(ME_mem_bytes_inuse() / 1048576.0));

#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049

        GLint total_mem_kb = 0;
        glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX, &total_mem_kb);

        GLint cur_avail_mem_kb = 0;
        glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX, &cur_avail_mem_kb);

        ImGui::Text("GPU MemTotalAvailable: %.2lf mb", (f64)(cur_avail_mem_kb / 1024.0f));
        ImGui::Text("GPU MemCurrentUsage: %.2lf mb", (f64)((total_mem_kb - cur_avail_mem_kb) / 1024.0f));

        auto L = the<scripting>().L;
        lua_gc(L, LUA_GCCOLLECT, 0);
        lua_Integer kb = lua_gc(L, LUA_GCCOUNT, 0);
        lua_Integer bytes = lua_gc(L, LUA_GCCOUNTB, 0);

        ImGui::Text("Lua MemoryUsage: %.2lf mb", ((f64)kb / 1024.0f));
        ImGui::Text("Lua Remaining: %.2lf mb", ((f64)bytes / 1024.0f));

        ImGui::Text("ImGui MemoryUsage: %.2lf mb", ((f64)imgui_mem_usage / 1048576.0));

        ImGui::Text("Test: %ld", global.game->Iso.world->real_layer2.capacity());
        ImGui::Text("Test: %ld", global.game->Iso.world->real_tiles.capacity());
        ImGui::Text("Test: %ld", global.game->Iso.world->background.capacity());

        static u64 virtualMemoryUsed;
        static u64 physicalMemoryUsed;
        static u64 peakVirtualMemoryUsed;
        static u64 peakPhysicalMemoryUsed;

        if (check_timer >= 200) {
            HANDLE hProcess = GetCurrentProcess();
            PROCESS_MEMORY_COUNTERS_EX pmc;

            if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc))) {
                virtualMemoryUsed = pmc.PrivateUsage;
                physicalMemoryUsed = pmc.WorkingSetSize;
                peakVirtualMemoryUsed = pmc.PeakPagefileUsage;
                peakPhysicalMemoryUsed = pmc.PeakWorkingSetSize;

            } else {
                std::cerr << "获取内存信息失败" << std::endl;
            }
        }

        ImGui::Text("Virtual MemoryUsage: %.2lf mb", ((f64)virtualMemoryUsed / 1048576.0));
        ImGui::Text("Real MemoryUsage: %.2lf mb", ((f64)physicalMemoryUsed / 1048576.0));
        ImGui::Text("Virtual MemoryUsage Peak: %.2lf mb", ((f64)peakVirtualMemoryUsed / 1048576.0));
        ImGui::Text("Real MemoryUsage Peak: %.2lf mb", ((f64)peakPhysicalMemoryUsed / 1048576.0));

        check_timer++;

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
    style.WindowPadding = ImVec2(4.0f, 4.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.WindowRounding = 8.0f;
    style.ChildRounding = 2.0f;
    style.FrameRounding = 2.0f;
    style.GrabRounding = 2.0f;
}

#if defined(ME_IMM32)

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

ImVec2 dbgui::NextWindows(dbgui_tag tag, ImVec2 pos) const noexcept {
    if (tag == dbgui_tag::DBGUI_MAINMENU) ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImVec2 windowspos = ImGui::GetPlatformIO().Platform_GetWindowPos(ImGui::GetMainViewport());
        pos += windowspos;
    }
    return pos;
}

void (*dbgui::RendererShutdownFunction)();
void (*dbgui::PlatformShutdownFunction)();
void (*dbgui::RendererNewFrameFunction)();
void (*dbgui::PlatformNewFrameFunction)();
void (*dbgui::RenderFunction)(ImDrawData *);

dbgui::dbgui() {
    RendererShutdownFunction = ImGui_ImplOpenGL3_Shutdown;
    PlatformShutdownFunction = ImGui_ImplSDL2_Shutdown;
    RendererNewFrameFunction = ImGui_ImplOpenGL3_NewFrame;
    PlatformNewFrameFunction = ImGui_ImplSDL2_NewFrame;
    RenderFunction = ImGui_ImplOpenGL3_RenderDrawData;
}

#if defined(ME_IMM32)
ME_imgui_imm imguiIMMCommunication{};
#endif

ME_PRIVATE(void *) ImGuiMalloc(size_t sz, void *user_data) { return ME_mem_alloc_leak_check_alloc((sz), (char *)__FILE__, __LINE__, &imgui_mem_usage); }

ME_PRIVATE(void) ImGuiFree(void *ptr, void *user_data) { ME_mem_alloc_leak_check_free(ptr, &imgui_mem_usage); }

void dbgui::Init() {

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
    // config.OversampleH = 3;
    // config.OversampleV = 3;
    // config.PixelSnapH = 1;

    f32 scale = 1.0f;

    io.Fonts->AddFontFromFileTTF(METADOT_RESLOC("data/assets/fonts/fusion-pixel-12px-monospaced.ttf"), 12.0f * scale, &config, io.Fonts->GetGlyphRangesChineseFull());

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplSDL2_Init(the<engine>().eng()->window, the<engine>().eng()->glContext);

    ImGui_ImplOpenGL3_Init();

    style.ScaleAllSizes(scale);

    ImGuiInitStyle(0.5f, 0.5f);

#if defined(ME_IMM32)
    common_control_initialize();
    ME_ASSERT(imguiIMMCommunication.subclassify(the<engine>().eng()->window));
#endif

    dbgui_list.push_back(static_cast<dbgui_base *>(new console()));
    dbgui_list.push_back(static_cast<dbgui_base *>(new pack_editor()));
    dbgui_list.push_back(static_cast<dbgui_base *>(new code_editor()));

    for (auto &gui : dbgui_list) {
        gui->init();
    }

    // auto create_console = [&]() {
    //     METADOT_NEW(C, console_imgui, ImGuiConsole, global.I18N.Get("ui_console"));

    //    // Our state
    //    ImVec4 clear_color = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);

    //    // Register variables
    //    console_imgui->System().RegisterVariable("background_color", clear_color, imvec4_setter);

    //    console_imgui->System().RegisterVariable("plPosX", GAME()->plPosX, Command::Arg<f32>(""));
    //    console_imgui->System().RegisterVariable("plPosY", GAME()->plPosY, Command::Arg<f32>(""));

    //    console_imgui->System().RegisterVariable("scale", Screen.gameScale, Command::Arg<i32>(""));

    //    meta::dostruct::for_each(global.game->GameIsolate_.globaldef, [&](const char *name, auto &value) { console_imgui->System().RegisterVariable(name, value, Command::Arg<int>("")); });

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

void dbgui::End() {
    for (auto &gui : dbgui_list) {
        gui->end();
        delete gui;
    }

    RendererShutdownFunction();
    PlatformShutdownFunction();
    ImGui::DestroyContext();
}

void dbgui::NewFrame() {
    RendererNewFrameFunction();
    PlatformNewFrameFunction();
    ImGui::NewFrame();
}

void dbgui::Draw() {
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    ImGui::Render();
    SDL_GL_MakeCurrent(the<engine>().eng()->window, the<engine>().eng()->glContext);

    RenderFunction(ImGui::GetDrawData());

    // Update and the<engine>().eng()-> additional Platform Windows
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

void dbgui::Update() {

    ImGuiIO &io = ImGui::GetIO();

#if defined(ME_IMM32)
    imguiIMMCommunication();
#endif

    if (global.game->Iso.globaldef.draw_imgui_debug) {
        ImGui::ShowDemoWindow();
        ShowAutoTestWindow();
    }

    for (auto &gui : dbgui_list) {
        gui->draw();
    }

    if (global.game->Iso.globaldef.ui_tweak) {

        ImGuiViewport *viewport = ImGui::GetMainViewport();

        if (viewport) {
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
        }

        const ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground;

        const ImGuiDockNodeFlags dock_node_flags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

        if (ImGui::Begin("me_dock_space_window", nullptr, window_flags)) {
            ImGui::DockSpace(ImGui::GetID("me_dock_space"), ImVec2(0.f, 0.f), dock_node_flags);

            if (ImGui::BeginMenuBar()) {

                ImGui::TextColored(ImVec4(0.19f, 1.f, 0.196f, 1.f), METADOT_NAME);

                if (ImGui::BeginMenu("File")) {
                    ImGui::EndMenu();
                }

                char fpsText[50];
                snprintf(fpsText, sizeof(fpsText), "%.1f ms/frame (%.1f(%d) FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate, the<engine>().eng()->time.feelsLikeFps);
                // ME_draw_text(fpsText, {255, 255, 255, 255}, the<engine>().eng()->windowWidth - ImGui::CalcTextSize(fpsText).x, 0);

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(fpsText).x - ImGui::GetScrollX());

                ImGui::Text(fpsText);

                ImGui::EndMenuBar();
            }
        }
        ImGui::End();

        ImGui::PopStyleVar(3);

        ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
        if (ImGui::Begin(LANG("ui_tweaks"))) {
            ImGui::BeginTabBar("ui_tweaks_tabbar");

            if (ImGui::BeginTabItem(LANG("ui_test"))) {

                if (ImGui::Button("调用回溯")) print_callstack();
                ImGui::SameLine();

#if 1

                if (ImGui::Button("NPC")) {

                    MEvec4 pl_transform{-global.game->Iso.world->loadZone.x + global.game->Iso.world->tickZone.x + global.game->Iso.world->tickZone.w / 2.0f,
                                        -global.game->Iso.world->loadZone.y + global.game->Iso.world->tickZone.y + global.game->Iso.world->tickZone.h / 2.0f, 10, 20};

                    b2PolygonShape sh;
                    sh.SetAsBox(pl_transform.z / 2.0f + 1, pl_transform.w / 2.0f);
                    RigidBody *rb = global.game->Iso.world->makeRigidBody(b2BodyType::b2_kinematicBody, pl_transform.x + pl_transform.z / 2.0f - 0.5, pl_transform.y + pl_transform.w / 2.0f - 0.5, 0,
                                                                          sh, 1, 1, NULL);
                    rb->body->SetGravityScale(0);
                    rb->body->SetLinearDamping(0);
                    rb->body->SetAngularDamping(0);

                    auto npc = global.game->Iso.world->Reg().create_entity();
                    ecs::entity_filler(npc)
                            .component<Controlable>()
                            .component<WorldEntity>(true, pl_transform.x, pl_transform.y, 0.0f, 0.0f, (int)pl_transform.z, (int)pl_transform.w, rb, std::string("NPC"))
                            .component<Bot>(1);

                    auto npc_bot = global.game->Iso.world->Reg().find_component<Bot>(npc);
                    // npc_bot->setItemInHand(global.game->GameIsolate_.world->Reg().find_component<WorldEntity>(global.game->GameIsolate_.world->player), i3,
                    // global.game->GameIsolate_.world.get());
                }
#endif
                ImGui::SameLine();
                if (ImGui::Button("Audio")) {
                    // static bool play= 1;
                    // static ME_Audio *test_audio = ME_audio_load_wav(METADOT_RESLOC("data/assets/audio/02_c03_normal_135.wav"));
                    // static Sound test_sound;
                    // if (play) {
                    //     ME_ASSERT(test_audio);
                    //     // metadot_music_play(test_audio, 0.f);
                    //     // ME_audio_destroy(test_audio);
                    //     int err;
                    //     test_sound = metadot_play_sound(test_audio, metadot_sound_params_defaults(), &err);
                    //     play ^= 1;
                    // } else {
                    //     metadot_sound_set_is_paused(test_sound, true);
                    //     play ^= 1;
                    // }
                }
                ImGui::SameLine();
                if (ImGui::Button("StaticRefl")) {

                    if (global.game->Iso.world == nullptr || !global.game->Iso.world->isPlayerInWorld()) {
                    } else {

                        meta_sr::TypeInfo<Player>::DFS_ForEach([](auto t, std::size_t depth) {
                            for (std::size_t i = 0; i < depth; i++) std::cout << "  ";
                            std::cout << t.name << std::endl;
                        });

                        std::cout << "[non DFS]" << std::endl;
                        meta_sr::TypeInfo<Player>::fields.ForEach([&](auto field) {
                            std::cout << field.name << std::endl;
                            if (field.name == "holdtype" && meta_sr::TypeInfo<EnumPlayerHoldType>::fields.NameOfValue(field.value) == "hammer") {
                                std::cout << "   Passed" << std::endl;
                                // if constexpr (std::is_same<decltype(field.value), EnumPlayerHoldType>().value) {
                                //     std::cout << "   " << field.value << std::endl;
                                // }
                            }
                        });

                        std::cout << "[DFS]" << std::endl;
                        meta_sr::TypeInfo<Player>::DFS_ForEach([](auto t, std::size_t) { t.fields.ForEach([](auto field) { std::cout << field.name << std::endl; }); });

                        constexpr std::size_t fieldNum = meta_sr::TypeInfo<Player>::DFS_Acc(0, [](auto acc, auto, auto) { return acc + 1; });
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

                        auto p = global.game->Iso.world->Reg().find_component<Player>(global.game->Iso.world->player);

                        // Just for test
                        Item *i3 = new Item();
                        i3->setFlag(ItemFlags::ItemFlags_Hammer);
                        i3->texture = global.game->Iso.texturepack.testHammer;
                        // i3->image = R_CopyImageFromSurface(i3->surface);
                        // R_SetImageFilter(i3->image, R_FILTER_NEAREST);
                        i3->pivotX = 2;

                        meta_sr::TypeInfo<Player>::fields.ForEach([&](auto field) {
                            if constexpr (field.is_func) {
                                if (field.name != "setItemInHand") return;
                                if constexpr (field.ValueTypeIsSameWith(static_cast<void (Player::*)(WorldEntity * we, Item * item, world * world) /* const */>(&Player::setItemInHand)))
                                    (p->*(field.value))(global.game->Iso.world->Reg().find_component<WorldEntity>(global.game->Iso.world->player), i3, global.game->Iso.world.get());
                                // else if constexpr (field.ValueTypeIsSameWith(static_cast<void (Player::*)(Item *item, World *world) /* const */>(&Player::setItemInHand)))
                                //     std::cout << (p.*(field.value))(1.f) << std::endl;
                                else
                                    assert(false);
                            }
                        });

                        // virtual

                        std::cout << "[Virtual Bases]" << std::endl;
                        constexpr auto vbs = meta_sr::TypeInfo<Player>::VirtualBases();
                        vbs.ForEach([](auto info) { std::cout << info.name << std::endl; });

                        std::cout << "[Tree]" << std::endl;
                        meta_sr::TypeInfo<Player>::DFS_ForEach([](auto t, std::size_t depth) {
                            for (std::size_t i = 0; i < depth; i++) std::cout << "  ";
                            std::cout << t.name << std::endl;
                        });

                        std::cout << "[field]" << std::endl;
                        meta_sr::TypeInfo<Player>::DFS_ForEach([](auto t, std::size_t) { t.fields.ForEach([](auto field) { std::cout << field.name << std::endl; }); });

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
                ImGui::Checkbox("Profiler", &global.game->Iso.globaldef.draw_profiler);
                ImGui::Checkbox("控制台", &global.game->Iso.globaldef.draw_console);
                ImGui::Checkbox("包编辑器", &global.game->Iso.globaldef.draw_pack_editor);
                ImGui::Checkbox("脚本编辑器", &global.game->Iso.globaldef.draw_code_editor);
                if (ImGui::Button("Meo")) {
                }
                ImGui::SameLine();
                if (ImGui::Button("Wang")) {
                    test_wang();
                }
                ImGui::SameLine();
                if (ImGui::Button("mdplot")) {
                    extern int test_mdplot();
                    test_mdplot();
                }
                if (ImGui::Button("Memory")) {
                }
                ImGui::SameLine();
                if (ImGui::Button("Metadesk")) {

                    // setup the global arena
                    arena = MD_ArenaAlloc();

                    MD_String8 example_code = MD_S8Lit(
                            R"mdesk(@struct Vec3:
                            {
                              x: F32,
                              y: F32,
                              z: F32,
                            })mdesk");

                    MD_ParseResult parse = MD_ParseWholeString(arena, MD_S8Lit("Generated Test Code"), example_code);

                    // Iterate through each top-level node
                    for (MD_EachNode(node, parse.node->first_child)) {
                        printf("/ %.*s\n", MD_S8VArg(node->string));

                        // Print the name of each of the node's tags
                        for (MD_EachNode(tag, node->first_tag)) {
                            printf("|-- Tag %.*s\n", MD_S8VArg(tag->string));
                        }

                        // Print the name of each of the node's children
                        for (MD_EachNode(child, node->first_child)) {
                            printf("|-- Child %.*s\n", MD_S8VArg(child->string));
                        }
                    }

                    // print the results
                    MD_PrintDebugDumpFromNode(stdout, parse.node, MD_GenerateFlags_All);
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(LANG("ui_debug"))) {
                DrawDebugUI(global.game);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        ImGui::End();

        ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_FirstUseEver);
        if (ImGui::Begin(LANG("ui_inspecting"), NULL)) {
            ImGui::BeginTabBar("ui_inspect");
            if (ImGui::BeginTabItem(LANG("ui_scene"))) {
                if (CollapsingHeader(LANG("ui_chunk"))) {
                    static bool check_rigidbody = false;
                    ImGui::Checkbox(CC("只查看刚体有效"), &check_rigidbody);

                    static MEvec2 check_chunk = {1, 1};
                    static Chunk *check_chunk_ptr = nullptr;

                    if (ImGui::BeginCombo("ChunkList", CC("选择检视区块..."))) {
                        for (auto &p1 : global.game->Iso.world->chunkCache)
                            for (auto &p2 : p1.second) {
                                if (ImGui::Selectable(ME_fs_get_filename(p2.second->pack_filename.c_str()))) {
                                    check_chunk.x = p2.second->x;
                                    check_chunk.y = p2.second->y;
                                    check_chunk_ptr = p2.second;
                                }
                            }
                        ImGui::EndCombo();
                    }

                    ImGui::Indent();
                    if (check_chunk_ptr != nullptr)
                        meta::static_refl::TypeInfo<Chunk>::ForEachVarOf(*check_chunk_ptr, [&](const auto &field, auto &&var) {
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
                    ImGui::Auto(global.game->Iso.world->rigidBodies, "刚体");
                    ImGui::Auto(global.game->Iso.world->worldRigidBodies, "世界刚体");

                    ImGui::Text("ECS: %lu %lu", global.game->Iso.world->Reg().memory_usage().entities, global.game->Iso.world->Reg().memory_usage().components);

                    global.game->Iso.world->Reg().for_joined_components<WorldEntity, Player>([&](ecs::entity, WorldEntity &we, Player &p) {
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
                    //     meta::static_refl::TypeInfo<RigidBody>::ForEachVarOf(*check_rigidbody_ptr, [&](const auto &field, auto &&var) { ImGui::Auto(var, std::string(field.name)); });
                    // }
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(LANG("ui_system"))) {
                if (ImGui::BeginTable("ui_system_table", 4,
                                      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_NoSavedSettings)) {
                    ImGui::TableSetupColumn("Priority", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, 0);
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 0.0f, 1);
                    ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, 2);
                    ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_None, 0.0f, 3);

                    ImGui::TableHeadersRow();

                    // meta::dostruct::for_each(global.game->GameIsolate_.globaldef, [&](const char *name, auto &value) { console_imgui->System().RegisterVariable(name, value,
                    // Command::Arg<int>(""));
                    // });
                    int i = 0;

                    for (auto &s : global.game->Iso.systemList) {

                        ImGui::PushID(i++);
                        ImGui::TableNextRow(ImGuiTableRowFlags_None);

                        if (ImGui::TableSetColumnIndex(0)) ImGui::Text("%d", s->priority);
                        if (ImGui::TableSetColumnIndex(1)) ImGui::TextUnformatted(s->getName().c_str());
                        if (ImGui::TableSetColumnIndex(2)) {
                            if (ImGui::SmallButton("Reload")) {
                                METADOT_BUG("Reloading ", s->getName().c_str());
                                s->reload();
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
        if (ImGui::Begin(LANG("ui_info"))) {

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            R_Renderer *renderer = R_GetCurrentRenderer();
            R_RendererID id = renderer->id;

            ImGui::Text("Using renderer: %s", glGetString(GL_RENDERER));
            ImGui::Text("OpenGL version supported: %s", glGetString(GL_VERSION));
            ImGui::Text("Engine renderer: %s (%d.%d)\n", id.name, id.major_version, id.minor_version);
            ImGui::Text("Shader versions supported: %d to %d\n", renderer->min_shader_version, renderer->max_shader_version);
            ImGui::Text("Platform: %s\n", ME_metadata().platform.c_str());
            ImGui::Text("Compiler: %s %s (with cpp %s)\n", ME_metadata().compiler.c_str(), ME_metadata().compiler_version.c_str(), ME_metadata().cpp.c_str());
        }
        ImGui::End();
    }
}

void code_editor::init() {
    text_editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());

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
            "IDXGISwapChain", "ID3D11ENGINE()->TargetView", "ID3D11Texture2D", "TextEditor" };
        static const char* idecls[] =
        {
            "typedef HWND_* HWND", "typedef long HRESULT", "typedef long* LPRESULT", "struct D3D11_RENDER_TARGET_VIEW_DESC", "struct DXGI_SWAP_CHAIN_DESC",
            "typedef tagMSG MSG\n * Message structure","typedef LONG_PTR LRESULT","WPARAM", "LPARAM","UINT","LPVOID",
            "ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer", "ID3D10Blob", "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
            "ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView", "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
            "IDXGISwapChain", "ID3D11ENGINE()->TargetView", "ID3D11Texture2D", "class TextEditor" };
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
}

void code_editor::end() {}

void code_editor::draw() {

    if (!global.game->Iso.globaldef.draw_code_editor) return;

    auto cpos = text_editor.GetCursorPosition();
    if (ImGui::Begin(LANG("ui_scripts_editor"), NULL, ImGuiWindowFlags_MenuBar)) {

        // if (ImGui::BeginTabItem(ICON_LANG(ICON_FA_EDIT, "ui_scripts_editor"))) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu(LANG("ui_file"))) {
                if (ImGui::MenuItem(LANG("ui_open"))) {
                    // fileDialog.Open();
                    filebrowser = true;
                }
                if (view_editing && view_editing->tags == editor_tag::CODE && ImGui::MenuItem(LANG("ui_save"))) {
                    if (view_editing && view_contents.size()) {
                        auto textToSave = text_editor.GetText();
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

            if (view_editing && view_editing->tags == editor_tag::CODE && ImGui::BeginMenu(LANG("ui_edit"))) {
                bool ro = text_editor.IsReadOnly();
                if (ImGui::MenuItem(LANG("ui_readonly_mode"), nullptr, &ro)) text_editor.SetReadOnly(ro);
                ImGui::Separator();

                if (ImGui::MenuItem(LANG("ui_undo"), "ALT-Backspace", nullptr, !ro && text_editor.CanUndo())) text_editor.Undo();
                if (ImGui::MenuItem(LANG("ui_redo"), "Ctrl-Y", nullptr, !ro && text_editor.CanRedo())) text_editor.Redo();

                ImGui::Separator();

                if (ImGui::MenuItem(LANG("ui_copy"), "Ctrl-C", nullptr, text_editor.HasSelection())) text_editor.Copy();
                if (ImGui::MenuItem(LANG("ui_cut"), "Ctrl-X", nullptr, !ro && text_editor.HasSelection())) text_editor.Cut();
                if (ImGui::MenuItem(LANG("ui_delete"), "Del", nullptr, !ro && text_editor.HasSelection())) text_editor.Delete();
                if (ImGui::MenuItem(LANG("ui_paste"), "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr)) text_editor.Paste();

                ImGui::Separator();

                if (ImGui::MenuItem(LANG("ui_selectall"), nullptr, nullptr)) text_editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(text_editor.GetTotalLines(), 0));

                ImGui::EndMenu();
            }

            if (view_editing && view_editing->tags == editor_tag::CODE && ImGui::BeginMenu(LANG("ui_view"))) {
                if (ImGui::MenuItem("Dark palette")) text_editor.SetPalette(TextEditor::GetDarkPalette());
                if (ImGui::MenuItem("Light palette")) text_editor.SetPalette(TextEditor::GetLightPalette());
                if (ImGui::MenuItem("Retro blue palette")) text_editor.SetPalette(TextEditor::GetRetroBluePalette());
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (filebrowser) {
            if (ImGui::Begin("File Browser", NULL)) {

                static std::string file = ME_fs_get_path("data/scripts");
                ImGuiHelper::file_browser(file);

                if (!file.empty() && !std::filesystem::is_directory(file)) {

                    METADOT_INFO(file.c_str());

                    bool shouldopen = true;
                    for (auto code : view_contents)
                        if (code.file == file) shouldopen = false;
                    if (shouldopen) {
                        auto extension = std::filesystem::path(file).extension().string();
                        std::ifstream i(file);
                        if (i.good()) {
                            if (extension == ".lua") {
                                std::string str((std::istreambuf_iterator<char>(i)), std::istreambuf_iterator<char>());
                                view_contents.push_back(EditorView{.tags = editor_tag::CODE, .file = file, .content = str});
                            } else if (extension == ".md") {
                                std::string str((std::istreambuf_iterator<char>(i)), std::istreambuf_iterator<char>());
                                view_contents.push_back(EditorView{.tags = editor_tag::MARKDOWN, .file = file, .content = str});
                            }
                        }
                    }

                    // 复位变量
                    filebrowser = false;
                    file = ME_fs_get_path("data/scripts");
                }
            }
            ImGui::End();
        }

        ImGui::BeginTabBar("ViewContents");

        for (auto &view : view_contents) {
            if (ImGui::BeginTabItem(ME_fs_get_filename(view.file.c_str()))) {
                view_editing = &view;

                if (!view_editing->is_edited) {
                    if (view.tags == editor_tag::CODE) text_editor.SetText(view_editing->content);
                    view_editing->is_edited = true;
                }

                ImGui::EndTabItem();
            } else {
                if (view.is_edited) view.is_edited = false;
            }
        }

        if (view_editing && view_contents.size()) {
            switch (view_editing->tags) {
                case editor_tag::CODE:
                    ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, text_editor.GetTotalLines(), text_editor.IsOverwrite() ? "Ovr" : "Ins",
                                text_editor.CanUndo() ? "*" : " ", text_editor.GetLanguageDefinition().mName.c_str(), ME_fs_get_filename(view_editing->file.c_str()));

                    text_editor.Render("TextEditor");
                    break;
                case editor_tag::MARKDOWN:
                    ImGuiHelper::markdown(view_editing->content);
                    break;
                default:
                    break;
            }
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

}  // namespace ME