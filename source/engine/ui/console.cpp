
#include "console.h"

#include "engine/scripting/scripting.hpp"

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
        switch (a.second) {
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

        ImGui::TextColored(colour, "%s", a.first.c_str());
    }

    static std::string command;
    if ((ImGui::InputTextWithHint("##Input", "Enter any command here", &command) || ImGui::IsItemActive()) && bInteractingWithTextbox != nullptr) *bInteractingWithTextbox = true;
    ImGui::SameLine();
    if (ImGui::Button("Send##consoleCommand")) {
        // for (auto &a : loggerInternal.commands) {
        //     if (command.rfind(a.cmd, 0) == 0) {
        //         a.func(command);
        //         break;
        //     }
        // }
        //ME_scripting::get_singleton_ptr()->run_command(command);
        command.clear();
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
}

void ME::MEconsole::add_to_message_log(const std::string &msg, log_type type) noexcept { loggerInternal.message_log.emplace_back(msg, type); }

void ME::MEconsole::show_help_message(const std::string &) noexcept {
    // for (const auto &a : loggerInternal.commands) {
    //     add_to_message_log(std::string(a.cmd) + " - " + a.cmdHint, ME_LOG_TYPE_MESSAGE);
    // }
}

void ME::MEconsole::display_full(bool *bInteractingWithTextbox) noexcept {
    ImGui::Begin("Developer Console");
    display(bInteractingWithTextbox);
    ImGui::End();
}