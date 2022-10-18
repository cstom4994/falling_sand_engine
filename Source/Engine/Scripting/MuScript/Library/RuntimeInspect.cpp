// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.

#include "Runtimeinspect.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdlib>


#include <fstream>
#include <iostream>
#include <istream>
#include <iterator>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <utility>
#include <vector>


const char *NARGV_IFS = " \t\n";

typedef struct NARGV
{
    char **argv, *data;
    const char *error_message;
    int argc, data_length, error_index, error_code;
} NARGV;

void nargv_free(NARGV *props) {
    free(props->data);
    free(props->argv);
    free(props);
}

void nargv_ifs(const char *nifs) {
    if (!nifs) {
        NARGV_IFS = " \t\n";
    } else {
        NARGV_IFS = nifs;
    }
}

int nargv_field_seperator(char seperator) {
    const char *list = NARGV_IFS;
    if (seperator) {
        while (*list) {
            if (*list++ == seperator) return 1;
        }
        return 0;
    }
    return 1;// null is always a field seperator
}

NARGV *nargv_parse(const char *input) {

    NARGV *nvp = (NARGV *) calloc(1, sizeof(NARGV));

    if (!input) {
        nvp->error_code = 1;
        nvp->error_message = "cannot parse null pointer";
        return nvp;
    }

    /* Get the input length */
    long input_length = -1;
test_next_input_char:
    if (input[++input_length]) goto test_next_input_char;

    if (!input_length) {
        nvp->error_code = 2;
        nvp->error_message = "cannot parse empty input";
        return nvp;
    }

    int composing_argument = 0;
    long quote = 0;
    long index;
    char look_ahead;

    // FIRST PASS
    // discover how many elements we have, and discover how large a data buffer we need
    for (index = 0; index <= input_length; index++) {

        if (nargv_field_seperator(input[index])) {
            if (composing_argument) {
                // close the argument
                composing_argument = 0;
                nvp->data_length++;
            }
            continue;
        }

        if (!composing_argument) {
            nvp->argc++;
            composing_argument = 1;
        }

        switch (input[index]) {

                /* back slash */
            case '\\':
                // If the sequence is not \' or \" or seperator copy the back slash, and
                // the data
                look_ahead = *(input + index + 1);
                if (look_ahead == '"' || look_ahead == '\'' || nargv_field_seperator(look_ahead)) {
                    index++;
                } else {
                    index++;
                    nvp->data_length++;
                }
                break;

                /* double quote */
            case '"':
                quote = index;
                while (input[++index] != '"') {
                    switch (input[index]) {
                        case '\0':
                            nvp->error_index = quote + 1;
                            nvp->error_code = 3;
                            nvp->error_message = "unterminated double quote";
                            return nvp;
                            break;
                        case '\\':
                            look_ahead = *(input + index + 1);
                            if (look_ahead == '"') {
                                index++;
                            } else {
                                index++;
                                nvp->data_length++;
                            }
                            break;
                    }
                    nvp->data_length++;
                }

                continue;
                break;

                /* single quote */
            case '\'':
                /* copy single quoted data */
                quote = index;// QT used as temp here...
                while (input[++index] != '\'') {
                    if (!input[index]) {
                        // unterminated double quote @ input
                        nvp->error_index = quote + 1;
                        nvp->error_code = 4;
                        nvp->error_message = "unterminated single quote";
                        return nvp;
                    }
                    nvp->data_length++;
                }
                continue;
                break;
        }

        // "record" the data
        nvp->data_length++;
    }

    // +1 for extra NULL pointer required by execv() and friends
    nvp->argv = (char **) calloc(nvp->argc + 1, sizeof(char *));
    nvp->data = (char *) calloc(nvp->data_length, 1);

    // SECOND PASS
    composing_argument = 0;
    quote = 0;

    int data_index = 0;
    int arg_index = 0;

    for (index = 0; index <= input_length; index++) {

        if (nargv_field_seperator(input[index])) {
            if (composing_argument) {
                composing_argument = 0;
                nvp->data[data_index++] = '\0';
            }
            continue;
        }

        if (!composing_argument) {
            nvp->argv[arg_index++] = (nvp->data + data_index);
            composing_argument = 1;
        }

        switch (input[index]) {

                /* back slash */
            case '\\':
                // If the sequence is not \' or \" or field seperator copy the backslash
                look_ahead = *(input + index + 1);
                if (look_ahead == '"' || look_ahead == '\'' || nargv_field_seperator(look_ahead)) {
                    index++;
                } else {
                    nvp->data[data_index++] = input[index++];
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
                            nvp->data[data_index++] = input[index++];
                        }
                    }
                    nvp->data[data_index++] = input[index];
                }
                continue;
                break;

                /* single quote */
            case '\'':
                /* copy single quoted data */
                while (input[++index] != '\'') {
                    nvp->data[data_index++] = input[index];
                }
                continue;
                break;
        }

        // "record" the data
        nvp->data[data_index++] = input[index];
    }

    return nvp;
}


// -- Utility get line

const char *getline() {
    std::vector<char> v;

    for (;;) {
        int c = fgetc(stdin);
        //std::cout << ((char) c) << std::endl;
        if (c == EOF)
            return nullptr;
        if (c == '\n')
            break;
        v.push_back((char) c);
    }

    if (!v.empty() && *(v.end() - 1) != '\0')
        v.push_back('\0');

    char *a = new char(v.size());
    memcpy(a, v.data(), v.size());
    //std::cout << "getline END\n";
    return a;
}


MuScript::Inspect::shell *MuScript::Inspect::shell::singleton = nullptr;

const std::string MuScript::Inspect::shell::exit = "exit";
const std::string MuScript::Inspect::shell::help = "help";
const std::string MuScript::Inspect::shell::about = "about";
const std::string MuScript::Inspect::shell::source = "source";

void MuScript::Inspect::shell::print_about() {
    std::cout << "MuScript::Inspect library version 1.0, Copyright (C) 2022 KaoruXun\n"
              << std::endl;
}

void MuScript::Inspect::shell::print_signature(i_cmd *cmd) {

    switch (cmd->get_form()) {
        case i_cmd::variable:
            //std::cout << style_return << cmd->get_return_type() << " " << style_name << cmd->get_name() << std::endl;
            METADOT_INFO("    {0} {1}", cmd->get_return_type(), cmd->get_name());
            break;

        case i_cmd::function:

            std::string para{};

            bool first = true;
            for (const parameter &p: *cmd) {
                if (!first)
                    para.append(", ");
                first = false;
                //std::cout << style_param << p.get_type();
                para.append(p.get_type());
            }

            //std::cout << ")" << std::endl;
            METADOT_INFO("    {0} {1}({2})", cmd->get_return_type(), cmd->get_name(), para);
            break;
    }
}

void MuScript::Inspect::shell::print_help() {
    for (auto &p: svc) {
        print_signature(p.second);
    }
}

void MuScript::Inspect::shell::source_files(std::queue<std::string> file_names) {

    while (!file_names.empty()) {
        std::string file_name = file_names.front();
        file_names.pop();

        std::ifstream infile(file_name);
        std::string line;
        std::getline(infile, line, '\n');
        while (!line.empty()) {
            if (eval(line.c_str()))
                break;
            std::getline(infile, line, '\n');
        }
    }
}

MuScript::Inspect::shell::shell(MuScript::Inspect::Instance &s) : svc(s) {
    singleton = this;
}

std::string MuScript::Inspect::shell::execute(std::string cmd_name, std::queue<std::string> arguments, exec_result &code) {
    std::string result;

    try {
        result = svc.call(cmd_name, arguments);
        code = exec_ok;
    } catch (MuScript::Inspect::cmd_not_found e) {
        try {
            result = shell_commands(cmd_name, arguments, code);
        } catch (MuScript::Inspect::cmd_not_found e) {
            METADOT_WARN("ERROR: command {0} not found.", cmd_name);
            code = exec_error;
        }

    } catch (MuScript::Inspect::invalid_argument &e) {
        METADOT_WARN("ERROR: argument {0} is invalid.", e.argN);
        code = exec_error;
    } catch (MuScript::Inspect::out_of_range &e) {
        METADOT_WARN("ERROR: argument {0} is out of range.", e.argN);
        code = exec_error;
    } catch (MuScript::Inspect::no_cast_available &e) {
        METADOT_WARN("ERROR: no cast avaliable for argument {0}.", e.argN);
        code = exec_error;
    } catch (MuScript::Inspect::read_only_variable &e) {
        METADOT_WARN("ERROR: variable is read-only.");
        code = exec_error;
    } catch (MuScript::Inspect::wrong_argument_count &e) {
        METADOT_WARN("ERROR: wrong argument count (expected {0}, found {1}).", e.expected, e.provided);
        code = exec_error;
    } catch (MuScript::Inspect::parse_exception &e) {
        METADOT_WARN("ERROR: could not parse argument {0} ({1}) . {2}", e.argN, e.value, e.what());
        code = exec_error;
    } catch (MuScript::Inspect::command_exception &e) {
        METADOT_WARN("ERROR: exception thrown by {0}{1}", cmd_name, e.what());
        code = exec_error;
    }

    return result;
}

std::string MuScript::Inspect::shell::shell_commands(std::string cmd, std::queue<std::string> arguments, exec_result &code) {
    std::string ret;

    if (cmd == exit) {
        code = exec_exit;
    } else if (cmd == help) {
        print_help();
        code = exec_ok;
    } else if (cmd == about) {
        print_about();
        code = exec_ok;
    } else if (cmd == source) {
        source_files(arguments);
    } else {
        throw cmd_not_found(cmd);
    }

    return ret;
}

const MuScript::Inspect::Instance &MuScript::Inspect::shell::get_instance() {
    return svc;
}

bool MuScript::Inspect::shell::eval(const char *line) {
    if (!line) {
        return true;
    }

    std::unique_ptr<NARGV, void (*)(NARGV *)> parsed_args(nargv_parse(line), nargv_free);

    std::string full_command(line);

    // if(interactive && !full_command.empty() && full_command != last_command)
    // 	add_history(full_command.c_str());

    if (!full_command.empty())
        last_command = full_command;

    switch (parsed_args->error_code) {
        case 1:// (provided char* was null)
            // this never happens because we checked line for null
            break;

        case 2:// (string was empty)
            // in this case we simply don't do anything
            return false;

        case 3:// (unterminated double quote)
            METADOT_WARN("Syntax error: {0} at column {1}", parsed_args->error_message, parsed_args->error_index);
            return false;

        case 4:// (unterminated single quote)
            METADOT_WARN("Syntax error: {0} at column {1}", parsed_args->error_message, parsed_args->error_index);
            return false;
    }

    std::string cmd_name(parsed_args->argv[0]);
    if (!cmd_name.empty() && cmd_name[0] == '#')
        return false;

    std::queue<std::string> args;

    for (int i = 1; i < parsed_args->argc; ++i)
        args.push(parsed_args->argv[i]);

    exec_result code;
    std::string result = execute(cmd_name, args, code);

    if (!result.empty())
        METADOT_INFO(result.c_str());

    if (code == exec_exit)
        return true;

    return false;
}

void MuScript::Inspect::shell::start(std::string line) {

    // rl_attempted_completion_function = completion_function;

    bool exit = eval(line.c_str());
}


MuScript::Inspect::Instance::~Instance() {
    for (auto &p: commands)
        delete p.second;
}

void MuScript::Inspect::Instance::remove_command(std::string name) {
    auto it = commands.find(name);
    delete it->second;
    if (it == commands.end())
        commands.erase(it);
}

std::string MuScript::Inspect::Instance::call(std::string name, std::queue<std::string> args) {
    auto it = commands.find(name);
    if (it == commands.end())
        throw cmd_not_found(name);
    return (*it).second->call(args);
}

MuScript::Inspect::Instance::const_iterator MuScript::Inspect::Instance::begin() {
    return commands.begin();
}

MuScript::Inspect::Instance::const_iterator MuScript::Inspect::Instance::end() {
    return commands.end();
}


MuScript::Inspect::parameter::parameter(std::string type, std::string name) : type(type), name(name) {
}

std::string MuScript::Inspect::parameter::get_name() const {
    return name;
}

std::string MuScript::Inspect::parameter::get_type() const {
    return type;
}


MuScript::Inspect::argument_exception::argument_exception() : std::exception() {
}

MuScript::Inspect::argument_exception::argument_exception(std::string s) : std::exception(),
                                                                           argN(0),
                                                                           explanation(s) {
}

MuScript::Inspect::argument_exception::argument_exception(unsigned int argN, std::string value, std::string what) : std::exception(),
                                                                                                                    argN(argN),
                                                                                                                    value(value),
                                                                                                                    explanation(what) {
}

const char *MuScript::Inspect::argument_exception::what() const noexcept {
    return explanation.c_str();
}

MuScript::Inspect::no_cast_available::no_cast_available() : argument_exception() {
}

MuScript::Inspect::no_cast_available::no_cast_available(unsigned int argN, std::string value) : argument_exception(argN, value) {
}

MuScript::Inspect::out_of_range::out_of_range(unsigned int argN, std::string value) : argument_exception(argN, value) {
}

MuScript::Inspect::invalid_argument::invalid_argument(unsigned int argN, std::string value) : argument_exception(argN, value) {
}

MuScript::Inspect::cmd_not_found::cmd_not_found(std::string cmd) : command(cmd) {
}

MuScript::Inspect::wrong_argument_count::wrong_argument_count(std::string command, unsigned int expected, unsigned int provided) : command(command), expected(expected), provided(provided) {
}

MuScript::Inspect::parse_exception::parse_exception(std::string what) : argument_exception(what) {
}

MuScript::Inspect::command_exception::command_exception() {
}

MuScript::Inspect::command_exception::command_exception(std::string s) : whatstr(s) {
}

const char *MuScript::Inspect::command_exception::what() const noexcept {
    return whatstr.c_str();
}


template<>
char MuScript::Inspect::cast<char>(std::string s) {
    if (s.size() > 0)
        return s[0];
    else
        throw std::out_of_range("string to char: expected a character but the argument was empty");
}

template<>
short MuScript::Inspect::cast<short>(std::string s) {
    int v = std::stoi(s);
    test_num_limit<int, short>(v);
    return (short) v;
}

template<>
int MuScript::Inspect::cast<int>(std::string s) {
    return std::stoi(s);
}

template<>
long MuScript::Inspect::cast<long>(std::string s) {
    return std::stol(s);
}

template<>
float MuScript::Inspect::cast<float>(std::string s) {
    return std::stof(s);
}

template<>
double MuScript::Inspect::cast<double>(std::string s) {
    return std::stod(s);
}

template<>
long double MuScript::Inspect::cast<long double>(std::string s) {
    return std::stold(s);
}

template<>
std::string MuScript::Inspect::cast<std::string>(std::string s) {
    return s;
}

template<>
const char *MuScript::Inspect::cast<const char *>(std::string s) {
    return s.c_str();
}


void MuScript::Inspect::i_cmd::add_parameter(const MuScript::Inspect::parameter &p) {
    params.push_back(p);
}

MuScript::Inspect::i_cmd::i_cmd(std::string name) : name(name) {
}

MuScript::Inspect::i_cmd::~i_cmd() {
}

std::string MuScript::Inspect::i_cmd::get_name() const {
    return name;
}

MuScript::Inspect::i_cmd::args_t::size_type MuScript::Inspect::i_cmd::size() const {
    return params.size();
}

MuScript::Inspect::i_cmd::iterator MuScript::Inspect::i_cmd::begin() {
    return params.begin();
}

MuScript::Inspect::i_cmd::iterator MuScript::Inspect::i_cmd::end() {
    return params.end();
}

MuScript::Inspect::i_cmd::const_iterator MuScript::Inspect::i_cmd::cbegin() const {
    return params.cbegin();
}

MuScript::Inspect::i_cmd::const_iterator MuScript::Inspect::i_cmd::cend() const {
    return params.cend();
}
