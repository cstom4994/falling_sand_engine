// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include <array>
#include <mutex>
#include <string>
#include <vector>
#include <cstring>

#include "Game/ImGuiTerminal.hpp"

struct custom_command_struct
{
    bool should_close = false;
};

class terminal_commands : public ImTerm::basic_terminal_helper<terminal_commands, custom_command_struct> {
public:
    terminal_commands();

    static std::vector<std::string> no_completion(argument_type &) { return {}; }

    static void clear(argument_type &);
    static void configure_term(argument_type &);
    static std::vector<std::string> configure_term_autocomplete(argument_type &);
    static void echo(argument_type &);
    static void exit(argument_type &);
    static void help(argument_type &);
    static void quit(argument_type &);
};

#endif  // CONSOLE_HPP
