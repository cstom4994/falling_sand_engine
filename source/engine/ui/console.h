
#ifndef ME_CONSOLE_H
#define ME_CONSOLE_H

#include "engine/utils/utility.hpp"
#include "engine/cvar.hpp"
#include "engine/ui/imgui_helper.hpp"

namespace ME {

enum MEconsole_result { OK, ERR, EXIT };

class MEconsole {

public:
    void Init();
    void End();

public:
    void display_full(bool *bInteractingWithTextbox) noexcept;
    void display(bool *bInteractingWithTextbox) noexcept;

    void draw_internal_display() noexcept;

    static void add_to_message_log(const std::string &msg, log_type type) noexcept;
    // static void add_command(const command_type &cmd) noexcept;

    void set_log_colour(ImVec4 colour, log_type type) noexcept;

    void PrintCommandInfo(cvar::BaseCommand *);
    std::string execute(std::string Command, std::queue<std::string> args, MEconsole_result &);
    bool eval(std::string &cmd);

private:
    friend class LoggerInternal;

    cvar::ConVar convar;

    ImVec4 success = {0.0f, 1.0f, 0.0f, 1.0f};
    ImVec4 warning = {1.0f, 1.0f, 0.0f, 1.0f};
    ImVec4 error = {1.0f, 0.0f, 0.0f, 1.0f};
    ImVec4 note = ME_rgba2imvec(0, 183, 255, 255);
    ImVec4 message = {1.0f, 1.0f, 1.0f, 1.0f};
};
}  // namespace ME

#endif