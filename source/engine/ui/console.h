
#ifndef ME_CONSOLE_H
#define ME_CONSOLE_H

#include "engine/core/utils/utility.hpp"
#include "engine/ui/imgui_helper.hpp"

namespace ME {

class MEconsole {
public:
    void display_full(bool *bInteractingWithTextbox) noexcept;
    void display(bool *bInteractingWithTextbox) noexcept;

    static void add_to_message_log(const std::string &msg, log_type type) noexcept;
    // static void add_command(const command_type &cmd) noexcept;

    void set_log_colour(ImVec4 colour, log_type type) noexcept;

private:
    friend class logger_internal;

    static void show_help_message(const std::string &) noexcept;

    ImVec4 success = {0.0f, 1.0f, 0.0f, 1.0f};
    ImVec4 warning = {1.0f, 1.0f, 0.0f, 1.0f};
    ImVec4 error = {1.0f, 0.0f, 0.0f, 1.0f};
    ImVec4 note = ME_rgba2imvec(0, 183, 255, 255);
    ImVec4 message = {1.0f, 1.0f, 1.0f, 1.0f};
};
}  // namespace ME

#endif