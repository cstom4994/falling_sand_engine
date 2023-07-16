
#include "console.h"

#include "engine/core/global.hpp"
#include "engine/game.hpp"
#include "engine/scripting/scripting.hpp"

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

void ME::MEconsole::set_log_colour(ImVec4 colour, log_type type) noexcept {
    switch (type) {
        case ME_LOG_TYPE_WARNING:
            warning = colour;
            return;
        case ME_LOG_TYPE_ERROR:
            error = colour;
            return;
        case ME_LOG_TYPE_NOTE:
            note = colour;
            return;
        case ME_LOG_TYPE_SUCCESS:
            success = colour;
            return;
        case ME_LOG_TYPE_MESSAGE:
            message = colour;
            return;
    }
}

void ME::MEconsole::display(bool *bInteractingWithTextbox) noexcept {
    for (auto &a : loggerInternal.message_log) {
        ImVec4 colour;
        switch (a.type) {
            case ME_LOG_TYPE_WARNING:
                colour = warning;
                break;
            case ME_LOG_TYPE_ERROR:
                colour = error;
                break;
            case ME_LOG_TYPE_NOTE:
                colour = note;
                break;
            case ME_LOG_TYPE_SUCCESS:
                colour = success;
                break;
            case ME_LOG_TYPE_MESSAGE:
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

void ME::MEconsole::draw_internal_display() noexcept {

    i64 now = ME_gettime();

    log_msg log_list[10] = {};
    int n = 9;

    std::vector<log_msg>::const_reverse_iterator backwardIterator;
    for (backwardIterator = loggerInternal.message_log.crbegin(); backwardIterator != loggerInternal.message_log.crend(); backwardIterator++) {
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

    ImDrawList *draw_list = ImGui::GetBackgroundDrawList();

    for (auto &a : log_list) {

        if (a.msg.empty()) continue;

        i64 dtime = now - a.time;

        ImVec4 colour;
        switch (a.type) {
            case ME_LOG_TYPE_WARNING:
                colour = warning;
                break;
            case ME_LOG_TYPE_ERROR:
                colour = error;
                break;
            case ME_LOG_TYPE_NOTE:
                colour = note;
                break;
            case ME_LOG_TYPE_SUCCESS:
                colour = success;
                break;
            case ME_LOG_TYPE_MESSAGE:
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

void ME::MEconsole::add_to_message_log(const std::string &msg, log_type type) noexcept { loggerInternal.message_log.emplace_back(log_msg{msg, type}); }

void ME::MEconsole::Init() {
    convar.Command("help", [this]() {
        METADOT_INFO("All commands");
        for (auto &p : this->convar) {
            this->PrintCommandInfo(p.second);
        }
    });

    ME::meta::dostruct::for_each(global.game->GameIsolate_.globaldef, [&](const char *name, auto &value) {
        // console_imgui->System().RegisterVariable(name, value, Command::Arg<int>(""));
        convar.Value(name, value);
    });

    convar.Value("game_scale", ENGINE()->render_scale);
}

void ME::MEconsole::End() {}

void ME::MEconsole::PrintCommandInfo(cvar::BaseCommand *cmd) {
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

std::string ME::MEconsole::execute(std::string cmd_name, std::queue<std::string> arguments, MEconsole_result &ret) {
    std::string result;

    try {
        result = convar.Call(cmd_name, arguments);
        ret = OK;
    } catch (std::exception &e) {
        METADOT_ERROR(std::format("[ConVar] exception thrown {0} : {1}", cmd_name, e.what()).c_str());
        ret = ERR;
    } catch (...) {
    }

    return result;
}

bool ME::MEconsole::eval(std::string &cmd) {

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

    MEconsole_result code;
    std::string result = this->execute(cmd_name, args, code);

    if (!result.empty()) METADOT_INFO(result.c_str());
    if (code == EXIT) return true;

    return false;
}

void ME::MEconsole::display_full(bool *bInteractingWithTextbox) noexcept {
    ImGui::Begin(ICON_LANG(ICON_FA_TERMINAL, "ui_console"));
    display(bInteractingWithTextbox);
    ImGui::End();
}