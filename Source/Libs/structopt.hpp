
//  (C) Copyright 2015 - 2018 Christopher Beck

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

// Hack by KaoruXun for MetaDot
// Fork from https://github.com/p-ranav/structopt

#include "VisitStruct.hpp"
#include "magic_enum.hpp"

#pragma once
#include <array>
using std::array;

namespace structopt {

    template<typename>
    struct array_size;
    template<typename T, size_t N>
    struct array_size<array<T, N>>
    {
        static size_t const size = N;
    };

}// namespace structopt
#pragma once

namespace structopt {

    template<typename Test, template<typename...> class Ref>
    struct is_specialization : std::false_type
    {
    };

    template<template<typename...> class Ref, typename... Args>
    struct is_specialization<Ref<Args...>, Ref> : std::true_type
    {
    };

}// namespace structopt
#pragma once
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace structopt {

    // specialize a type for all of the STL containers.
    namespace is_stl_container_impl {
        template<typename T>
        struct is_stl_container : std::false_type
        {
        };
        template<typename T, std::size_t N>
        struct is_stl_container<std::array<T, N>> : std::true_type
        {
        };
        template<typename... Args>
        struct is_stl_container<std::vector<Args...>> : std::true_type
        {
        };
        template<typename... Args>
        struct is_stl_container<std::deque<Args...>> : std::true_type
        {
        };
        template<typename... Args>
        struct is_stl_container<std::list<Args...>> : std::true_type
        {
        };
        template<typename... Args>
        struct is_stl_container<std::forward_list<Args...>> : std::true_type
        {
        };
        template<typename... Args>
        struct is_stl_container<std::set<Args...>> : std::true_type
        {
        };
        template<typename... Args>
        struct is_stl_container<std::multiset<Args...>> : std::true_type
        {
        };
        template<typename... Args>
        struct is_stl_container<std::map<Args...>> : std::true_type
        {
        };
        template<typename... Args>
        struct is_stl_container<std::multimap<Args...>> : std::true_type
        {
        };
        template<typename... Args>
        struct is_stl_container<std::unordered_set<Args...>> : std::true_type
        {
        };
        template<typename... Args>
        struct is_stl_container<std::unordered_multiset<Args...>> : std::true_type
        {
        };
        template<typename... Args>
        struct is_stl_container<std::unordered_map<Args...>> : std::true_type
        {
        };
        template<typename... Args>
        struct is_stl_container<std::unordered_multimap<Args...>> : std::true_type
        {
        };
        template<typename... Args>
        struct is_stl_container<std::stack<Args...>> : std::true_type
        {
        };
        template<typename... Args>
        struct is_stl_container<std::queue<Args...>> : std::true_type
        {
        };
        template<typename... Args>
        struct is_stl_container<std::priority_queue<Args...>> : std::true_type
        {
        };
    }// namespace is_stl_container_impl

    template<class T>
    struct is_array : std::is_array<T>
    {
    };
    template<class T, std::size_t N>
    struct is_array<std::array<T, N>> : std::true_type
    {
    };
    // optional:
    template<class T>
    struct is_array<T const> : is_array<T>
    {
    };
    template<class T>
    struct is_array<T volatile> : is_array<T>
    {
    };
    template<class T>
    struct is_array<T volatile const> : is_array<T>
    {
    };

    // type trait to utilize the implementation type traits as well as decay the type
    template<typename T>
    struct is_stl_container
    {
        static constexpr bool const value =
                is_stl_container_impl::is_stl_container<std::decay_t<T>>::value;
    };

}// namespace structopt

#pragma once
#include <string>

namespace structopt {

    namespace details {

        static inline bool string_replace(std::string &str, const std::string &from,
                                          const std::string &to) {
            size_t start_pos = str.find(from);
            if (start_pos == std::string::npos)
                return false;
            str.replace(start_pos, from.length(), to);
            return true;
        }

        inline std::string string_to_kebab(std::string str) {
            // Generate kebab case and present as option
            details::string_replace(str, "_", "-");
            return str;
        }

    }// namespace details

}// namespace structopt
#pragma once
#include <string>

namespace structopt {

    namespace details {

        static inline bool is_binary_notation(std::string const &input) {
            return input.compare(0, 2, "0b") == 0 && input.size() > 2 &&
                   input.find_first_not_of("01", 2) == std::string::npos;
        }

        static inline bool is_hex_notation(std::string const &input) {
            return input.compare(0, 2, "0x") == 0 && input.size() > 2 &&
                   input.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
        }

        static inline bool is_octal_notation(std::string const &input) {
            return input.compare(0, 1, "0") == 0 && input.size() > 1 &&
                   input.find_first_not_of("01234567", 1) == std::string::npos;
        }

        static inline bool is_valid_number(const std::string &input) {
            if (is_binary_notation(input) || is_hex_notation(input) || is_octal_notation(input)) {
                return true;
            }

            if (input.empty()) {
                return false;
            }

            std::size_t i = 0, j = input.length() - 1;

            // Handling whitespaces
            while (i < input.length() && input[i] == ' ')
                i++;
            while (input[j] == ' ')
                j--;

            if (i > j)
                return false;

            // if string is of length 1 and the only
            // character is not a digit
            if (i == j && !(input[i] >= '0' && input[i] <= '9'))
                return false;

            // If the 1st char is not '+', '-', '.' or digit
            if (input[i] != '.' && input[i] != '+' && input[i] != '-' &&
                !(input[i] >= '0' && input[i] <= '9'))
                return false;

            // To check if a '.' or 'e' is found in given
            // string. We use this flag to make sure that
            // either of them appear only once.
            bool dot_or_exp = false;

            for (; i <= j; i++) {
                // If any of the char does not belong to
                // {digit, +, -, ., e}
                if (input[i] != 'e' && input[i] != '.' && input[i] != '+' && input[i] != '-' &&
                    !(input[i] >= '0' && input[i] <= '9'))
                    return false;

                if (input[i] == '.') {
                    // checks if the char 'e' has already
                    // occurred before '.' If yes, return false;.
                    if (dot_or_exp == true)
                        return false;

                    // If '.' is the last character.
                    if (i + 1 > input.length())
                        return false;

                    // if '.' is not followed by a digit.
                    if (!(input[i + 1] >= '0' && input[i + 1] <= '9'))
                        return false;
                }

                else if (input[i] == 'e') {
                    // set dot_or_exp = 1 when e is encountered.
                    dot_or_exp = true;

                    // if there is no digit before 'e'.
                    if (!(input[i - 1] >= '0' && input[i - 1] <= '9'))
                        return false;

                    // If 'e' is the last Character
                    if (i + 1 > input.length())
                        return false;

                    // if e is not followed either by
                    // '+', '-' or a digit
                    if (input[i + 1] != '+' && input[i + 1] != '-' &&
                        (input[i + 1] >= '0' && input[i] <= '9'))
                        return false;
                }
            }

            /* If the string skips all above cases, then
  it is numeric*/
            return true;
        }

    }// namespace details

}// namespace structopt

#pragma once
#include <algorithm>
#include <iostream>
#include <optional>
#include <queue>
#include <string>
// #include <structopt/is_specialization.hpp>
// #include <structopt/string.hpp>
// #include <structopt/third_party/visit_struct/visit_struct.hpp>
#include <type_traits>
#include <vector>

namespace structopt {

    class app;

    namespace details {

        struct visitor
        {
            std::string name;
            std::string version;
            std::optional<std::string> help;
            std::vector<std::string> field_names;
            std::deque<std::string> positional_field_names;// mutated by parser
            std::deque<std::string> positional_field_names_for_help;
            std::deque<std::string> vector_like_positional_field_names;
            std::deque<std::string> flag_field_names;
            std::deque<std::string> optional_field_names;
            std::deque<std::string> nested_struct_field_names;

            visitor() = default;

            explicit visitor(const std::string &name, const std::string &version)
                : name(name), version(version) {}

            explicit visitor(const std::string &name, const std::string &version, const std::string &help)
                : name(name), version(version), help(help) {}

            // Visitor function for std::optional - could be an option or a flag
            template<typename T>
            inline typename std::enable_if<structopt::is_specialization<T, std::optional>::value,
                                           void>::type
            operator()(const char *name, T &) {
                field_names.push_back(name);
                if constexpr (std::is_same<typename T::value_type, bool>::value) {
                    flag_field_names.push_back(name);
                } else {
                    optional_field_names.push_back(name);
                }
            }

            // Visitor function for any positional field (not std::optional)
            template<typename T>
            inline typename std::enable_if<!structopt::is_specialization<T, std::optional>::value &&
                                                   !visit_struct::traits::is_visitable<T>::value,
                                           void>::type
            operator()(const char *name, T &) {
                field_names.push_back(name);
                positional_field_names.push_back(name);
                positional_field_names_for_help.push_back(name);
                if constexpr (structopt::is_specialization<T, std::deque>::value ||
                              structopt::is_specialization<T, std::list>::value ||
                              structopt::is_specialization<T, std::vector>::value ||
                              structopt::is_specialization<T, std::set>::value ||
                              structopt::is_specialization<T, std::multiset>::value ||
                              structopt::is_specialization<T, std::unordered_set>::value ||
                              structopt::is_specialization<T, std::unordered_multiset>::value ||
                              structopt::is_specialization<T, std::queue>::value ||
                              structopt::is_specialization<T, std::stack>::value ||
                              structopt::is_specialization<T, std::priority_queue>::value) {
                    // keep track of vector-like fields as these (even though positional)
                    // can be happy without any arguments
                    vector_like_positional_field_names.push_back(name);
                }
            }

            // Visitor function for nested structs
            template<typename T>
            inline typename std::enable_if<visit_struct::traits::is_visitable<T>::value, void>::type
            operator()(const char *name, T &) {
                field_names.push_back(name);
                nested_struct_field_names.push_back(name);
            }

            bool is_field_name(const std::string &field_name) {
                return std::find(field_names.begin(), field_names.end(), field_name) !=
                       field_names.end();
            }

            void print_help(std::ostream &os) const {
                if (help.has_value() && help.value().size() > 0) {
                    os << help.value();
                } else {
                    os << "\nUSAGE: " << name << " ";

                    if (flag_field_names.empty() == false) {
                        os << "[FLAGS] ";
                    }

                    if (optional_field_names.empty() == false) {
                        os << "[OPTIONS] ";
                    }

                    if (nested_struct_field_names.empty() == false) {
                        os << "[SUBCOMMANDS] ";
                    }

                    for (auto &field: positional_field_names_for_help) {
                        os << field << " ";
                    }

                    bool has_h = false;
                    bool has_v = false;
                    if (flag_field_names.empty() == false) {
                        os << "\n\nFLAGS:\n";
                        for (auto &flag: flag_field_names) {

                            // Generate kebab case and present as flag
                            auto kebab_case = details::string_to_kebab(flag);
                            std::string long_form = "";
                            if (kebab_case != flag) {
                                long_form = kebab_case;
                            } else {
                                long_form = flag;
                            }

                            os << "    -" << flag[0] << ", --" << flag << "\n";

                            switch (flag[0]) {
                                case 'h':
                                    has_h = true;
                                    break;
                                case 'v':
                                    has_v = true;
                                    break;
                            }
                        }
                    } else {
                        os << "\n";
                    }

                    if (optional_field_names.empty() == false) {
                        os << "\nOPTIONS:\n";
                        for (auto &option: optional_field_names) {

                            // Generate kebab case and present as option
                            auto kebab_case = details::string_to_kebab(option);
                            std::string long_form = "";
                            if (kebab_case != option) {
                                long_form = kebab_case;
                            } else {
                                long_form = option;
                            }

                            if ((has_v && option == "version") || (has_h && option == "help")) {
                                os << "    --" << long_form << " <" << option << ">\n";
                            } else {
                                os << "    -" << option[0] << ", --" << long_form << " <" << option << ">"
                                   << "\n";
                            }

                            switch (option[0]) {
                                case 'h':
                                    has_h = true;
                                    break;
                                case 'v':
                                    has_v = true;
                                    break;
                            }
                        }
                    }

                    if (nested_struct_field_names.empty() == false) {
                        os << "\nSUBCOMMANDS:\n";
                        for (auto &sc: nested_struct_field_names) {
                            os << "    " << sc << "\n";
                        }
                    }

                    if (positional_field_names_for_help.empty() == false) {
                        os << "\nARGS:\n";
                        for (auto &arg: positional_field_names_for_help) {
                            os << "    " << arg << "\n";
                        }
                    }
                }
            }
        };

    }// namespace details

}// namespace structopt

#pragma once
#include <exception>
#include <sstream>
#include <string>
// #include <structopt/visitor.hpp>

namespace structopt {

    class exception : public std::exception {
        std::string what_{""};
        std::string help_{""};
        details::visitor visitor_;

    public:
        exception(const std::string &what, const details::visitor &visitor)
            : what_(what), help_(""), visitor_(visitor) {
            std::stringstream os;
            visitor_.print_help(os);
            help_ = os.str();
        }

        const char *what() const throw() { return what_.c_str(); }

        const char *help() const throw() { return help_.c_str(); }
    };

}// namespace structopt
#pragma once
#include <optional>
// #include <structopt/visitor.hpp>

namespace structopt {

    namespace details {
        struct parser;
    }

    class sub_command {
        std::optional<bool> invoked_;
        details::visitor visitor_;

        friend struct structopt::details::parser;

    public:
        bool has_value() const { return invoked_.has_value(); }
    };

}// namespace structopt
#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
// #include <structopt/array_size.hpp>
// #include <structopt/exception.hpp>
// #include <structopt/is_number.hpp>
// #include <structopt/is_specialization.hpp>
// #include <structopt/sub_command.hpp>
// #include <structopt/third_party/magic_enum/magic_enum.hpp>
// #include <structopt/third_party/visit_struct/visit_struct.hpp>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace structopt {

    namespace details {

        struct parser
        {
            structopt::details::visitor visitor;
            std::vector<std::string> arguments;
            std::size_t current_index{1};
            std::size_t next_index{1};
            bool double_dash_encountered{false};// "--" option-argument delimiter
            bool sub_command_invoked{false};
            std::string already_invoked_subcommand_name{""};

            bool is_optional(const std::string &name) {
                if (double_dash_encountered) {
                    return false;
                } else if (name == "--") {
                    double_dash_encountered = true;
                    return false;
                } else if (is_valid_number(name)) {
                    return false;
                }

                bool result = false;
                if (name.size() >= 2) {
                    // e.g., -b, -v
                    if (name[0] == '-') {
                        result = true;
                        // e.g., --verbose
                        if (name[1] == '-') {
                            result = true;
                        }
                    }
                }
                return result;
            }

            bool is_kebab_case(const std::string &next, const std::string &field_name) {
                bool result = false;
                auto maybe_kebab_case = next;
                if (maybe_kebab_case.size() > 1 && maybe_kebab_case[0] == '-') {
                    // remove first dash
                    maybe_kebab_case.erase(0, 1);
                    if (maybe_kebab_case[0] == '-') {
                        // there is a second leading dash
                        // remove it
                        maybe_kebab_case.erase(0, 1);
                    }
                    std::replace(maybe_kebab_case.begin(), maybe_kebab_case.end(), '-', '_');
                    if (maybe_kebab_case == field_name) {
                        result = true;
                    }
                }
                return result;
            }

            bool is_optional_field(const std::string &next, const std::string &field_name) {
                bool result = false;
                if (next == "-" + field_name || next == "--" + field_name ||
                    next == "-" + std::string(1, field_name[0]) || is_kebab_case(next, field_name)) {
                    // okay `next` matches _a_ field name (which is an optional field)
                    result = true;
                }
                return result;
            }

            bool is_optional_field(const std::string &next) {
                if (!is_optional(next)) {
                    return false;
                }

                bool result = false;
                for (auto &field_name: visitor.field_names) {
                    result = is_optional_field(next, field_name);
                    if (result) {
                        break;
                    }
                }
                return result;
            }

            // checks if the next argument is a delimited optional field
            // e.g., -std=c++17, where std matches a field name
            // and it is delimited by one of the two allowed delimiters: `=` and `:`
            //
            // if true, the return value includes the delimiter that was used
            std::pair<bool, char> is_delimited_optional_argument(const std::string &next) {
                bool success = false;
                char delimiter = '\0';

                auto equal_pos = next.find('=');
                auto colon_pos = next.find(':');

                if (equal_pos == std::string::npos && colon_pos == std::string::npos) {
                    // not delimited
                    return {success, delimiter};
                } else {
                    // assume `=` comes first
                    char c = '=';

                    if (colon_pos < equal_pos) {
                        // confirmed: `:` comes first
                        c = ':';
                    }

                    // split `next` into key and value
                    // check if key is an optional field
                    std::string key;
                    bool delimiter_found = false;
                    for (size_t i = 0; i < next.size(); i++) {
                        if (next[i] == c && !delimiter_found) {
                            delimiter = c;
                            delimiter_found = true;
                        } else {
                            if (!delimiter_found) {
                                key += next[i];
                            }
                        }
                    }

                    // check if `key` is a valid optional field
                    if (delimiter_found && is_optional_field(key)) {
                        success = true;
                    }
                }
                return {success, delimiter};
            }

            std::pair<std::string, std::string> split_delimited_argument(char delimiter,
                                                                         const std::string &next) {
                std::string key, value;
                bool delimiter_found = false;
                for (size_t i = 0; i < next.size(); i++) {
                    if (next[i] == delimiter && !delimiter_found) {
                        delimiter_found = true;
                    } else {
                        if (!delimiter_found) {
                            key += next[i];
                        } else {
                            value += next[i];
                        }
                    }
                }
                return {key, value};
            }

            // Strip the initial dashes on the left of an optional argument
            // e.g., --verbose => verbose
            // e.g., -log-level => log-level
            std::string lstrip_dashes(const std::string &next) {
                std::string result;
                bool prefix_dashes_ended = false;
                for (auto &c: next) {
                    if (prefix_dashes_ended == false && c != '-') {
                        prefix_dashes_ended = true;
                    }
                    if (prefix_dashes_ended) {
                        result += c;
                    }
                }
                return result;
            }

            // Get the optional field name if any from
            // e.g., `-v` => `verbose`
            // e.g., `-log-level` => `log_level`
            std::optional<std::string> get_full_optional_field_name(const std::string &next) {
                std::optional<std::string> result;

                if (next.size() == 2 && next[0] == '-') {
                    // short form of optional argument
                    for (auto &oarg: visitor.optional_field_names) {
                        if (oarg[0] == next[1]) {
                            // second character of next matches first character of some optional field_name
                            result = oarg;
                            break;
                        }
                    }
                } else {
                    // long form of optional argument

                    // strip dashes on the left
                    std::string potential_field_name = lstrip_dashes(next);

                    // replace `-` in the middle with `_`
                    std::replace(potential_field_name.begin(), potential_field_name.end(), '-', '_');

                    // check if `potential_field_name` is in the optional field names list
                    for (auto &oarg: visitor.optional_field_names) {
                        if (oarg == potential_field_name) {
                            result = oarg;
                            break;
                        }
                    }
                }

                return result;
            }

            template<typename T>
            std::pair<T, bool> parse_argument(const char *name) {
                if (next_index >= arguments.size()) {
                    return {T(), false};
                }
                T result;
                bool success = true;
                if constexpr (visit_struct::traits::is_visitable<T>::value) {
                    result = parse_nested_struct<T>(name);
                } else if constexpr (std::is_enum<T>::value) {
                    result = parse_enum_argument<T>(name);
                    next_index += 1;
                } else if constexpr (structopt::is_specialization<T, std::pair>::value) {
                    result = parse_pair_argument<typename T::first_type, typename T::second_type>(name);
                } else if constexpr (structopt::is_specialization<T, std::tuple>::value) {
                    result = parse_tuple_argument<T>(name);
                } else if constexpr (!is_stl_container<T>::value) {
                    result = parse_single_argument<T>(name);
                    next_index += 1;
                } else if constexpr (structopt::is_array<T>::value) {
                    constexpr std::size_t N = structopt::array_size<T>::size;
                    result = parse_array_argument<typename T::value_type, N>(name);
                } else if constexpr (structopt::is_specialization<T, std::deque>::value ||
                                     structopt::is_specialization<T, std::list>::value ||
                                     structopt::is_specialization<T, std::vector>::value) {
                    result = parse_vector_like_argument<T>(name);
                } else if constexpr (structopt::is_specialization<T, std::set>::value ||
                                     structopt::is_specialization<T, std::multiset>::value ||
                                     structopt::is_specialization<T, std::unordered_set>::value ||
                                     structopt::is_specialization<T,
                                                                  std::unordered_multiset>::value) {
                    result = parse_set_argument<T>(name);
                } else if constexpr (structopt::is_specialization<T, std::queue>::value ||
                                     structopt::is_specialization<T, std::stack>::value ||
                                     structopt::is_specialization<T, std::priority_queue>::value) {
                    result = parse_container_adapter_argument<T>(name);
                } else {
                    success = false;
                }
                return {result, success};
            }

            template<typename T>
            std::optional<T> parse_optional_argument(const char *name) {
                next_index += 1;
                std::optional<T> result;
                if (next_index < arguments.size()) {
                    auto [value, success] = parse_argument<T>(name);
                    if (success) {
                        result = value;
                    } else {
                        throw structopt::exception(
                                "Error: failed to correctly parse optional argument `" + std::string{name} +
                                        "`.",
                                visitor);
                    }
                } else {
                    throw structopt::exception("Error: expected value for optional argument `" +
                                                       std::string{name} + "`.",
                                               visitor);
                }
                return result;
            }

            // Any field that can be constructed using std::stringstream
            // Not container type
            // Not a visitable type, i.e., a nested struct
            template<typename T>
            inline typename std::enable_if<!visit_struct::traits::is_visitable<T>::value, T>::type
            parse_single_argument(const char *) {
                std::string argument = arguments[next_index];
                std::istringstream ss(argument);
                T result;

                if constexpr (std::is_integral<T>::value) {
                    if (is_hex_notation(argument)) {
                        ss >> std::hex >> result;
                    } else if (is_octal_notation(argument)) {
                        ss >> std::oct >> result;
                    } else if (is_binary_notation(argument)) {
                        argument.erase(0, 2);// remove "0b"
                        result = static_cast<T>(std::stoi(argument, nullptr, 2));
                    } else {
                        ss >> std::dec >> result;
                    }
                } else {
                    ss >> result;
                }

                return result;
            }

            // Nested visitable struct
            template<typename T>
            inline typename std::enable_if<visit_struct::traits::is_visitable<T>::value, T>::type
            parse_nested_struct(const char *name) {

                T argument_struct;

                if constexpr (std::is_base_of<structopt::sub_command, T>::value) {
                    argument_struct.invoked_ = true;
                }

                // Save struct field names
                argument_struct.visitor_.name = name;// sub-command name; not the program
                argument_struct.visitor_.version = visitor.version;
                visit_struct::for_each(argument_struct, argument_struct.visitor_);

                // add `help` and `version` optional arguments
                argument_struct.visitor_.optional_field_names.push_back("help");
                argument_struct.visitor_.optional_field_names.push_back("version");

                if (!sub_command_invoked) {
                    sub_command_invoked = true;
                    already_invoked_subcommand_name = name;
                } else {
                    // a sub-command has already been invoked
                    throw structopt::exception(
                            "Error: failed to invoke sub-command `" + std::string{name} +
                                    "` because a different sub-command, `" + already_invoked_subcommand_name +
                                    "`, has already been invoked.",
                            argument_struct.visitor_);
                }

                structopt::details::parser parser;
                parser.next_index = 0;
                parser.current_index = 0;
                parser.double_dash_encountered = double_dash_encountered;
                parser.visitor = argument_struct.visitor_;

                std::copy(arguments.begin() + next_index, arguments.end(),
                          std::back_inserter(parser.arguments));

                for (std::size_t i = 0; i < parser.arguments.size(); i++) {
                    parser.current_index = i;
                    visit_struct::for_each(argument_struct, parser);
                }

                // directly call the parser to check for `help` and `version` flags
                std::optional<bool> help = false, version = false;
                for (std::size_t i = 0; i < parser.arguments.size(); i++) {
                    parser.operator()("help", help);
                    parser.operator()("version", version);

                    if (help == true) {
                        // if help is requested, print help and exit
                        argument_struct.visitor_.print_help(std::cout);
                        exit(EXIT_SUCCESS);
                    } else if (version == true) {
                        // if version is requested, print version and exit
                        std::cout << argument_struct.visitor_.version << "\n";
                        exit(EXIT_SUCCESS);
                    }
                }

                // if all positional arguments were provided
                // this list would be empty
                if (!parser.visitor.positional_field_names.empty()) {
                    for (auto &field_name: parser.visitor.positional_field_names) {
                        if (std::find(parser.visitor.vector_like_positional_field_names.begin(),
                                      parser.visitor.vector_like_positional_field_names.end(),
                                      field_name) ==
                            parser.visitor.vector_like_positional_field_names.end()) {
                            // this positional argument is not a vector-like argument
                            // it expects value(s)
                            throw structopt::exception("Error: expected value for positional argument `" +
                                                               field_name + "`.",
                                                       argument_struct.visitor_);
                        }
                    }
                }

                // update current and next
                current_index += parser.next_index;
                next_index += parser.next_index;

                return argument_struct;
            }

            // Pair argument
            template<typename T1, typename T2>
            std::pair<T1, T2> parse_pair_argument(const char *name) {
                std::pair<T1, T2> result;
                {
                    // Pair first
                    auto [value, success] = parse_argument<T1>(name);
                    if (success) {
                        result.first = value;
                    } else {
                        if (next_index == arguments.size()) {
                            // end of arguments list
                            // first argument not provided
                            throw structopt::exception("Error: failed to correctly parse the pair `" +
                                                               std::string{name} +
                                                               "`. Expected 2 arguments, 0 provided.",
                                                       visitor);
                        } else {
                            throw structopt::exception(
                                    "Error: failed to correctly parse first element of pair `" +
                                            std::string{name} + "`",
                                    visitor);
                        }
                    }
                }
                {
                    // Pair second
                    auto [value, success] = parse_argument<T2>(name);
                    if (success) {
                        result.second = value;
                    } else {
                        if (next_index == arguments.size()) {
                            // end of arguments list
                            // second argument not provided
                            throw structopt::exception("Error: failed to correctly parse the pair `" +
                                                               std::string{name} +
                                                               "`. Expected 2 arguments, only 1 provided.",
                                                       visitor);
                        } else {
                            throw structopt::exception(
                                    "Error: failed to correctly parse second element of pair `" +
                                            std::string{name} + "`",
                                    visitor);
                        }
                    }
                }
                return result;
            }

            // Array argument
            template<typename T, std::size_t N>
            std::array<T, N> parse_array_argument(const char *name) {
                std::array<T, N> result{};

                const auto arguments_left = arguments.size() - next_index;
                if (arguments_left == 0 || arguments_left < N) {
                    throw structopt::exception("Error: expected " + std::to_string(N) +
                                                       " values for std::array argument `" + name +
                                                       "` - instead got only " +
                                                       std::to_string(arguments_left) + " arguments.",
                                               visitor);
                }

                for (std::size_t i = 0; i < N; i++) {
                    auto [value, success] = parse_argument<T>(name);
                    if (success) {
                        result[i] = value;
                    }
                }
                return result;
            }

            template<class Tuple, class F, std::size_t... I>
            constexpr F for_each_impl(Tuple &&t, F &&f, std::index_sequence<I...>) {
                return (void) std::initializer_list<int>{
                               (std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))), 0)...},
                       f;
            }

            template<class Tuple, class F>
            constexpr F for_each(Tuple &&t, F &&f) {
                return for_each_impl(std::forward<Tuple>(t), std::forward<F>(f),
                                     std::make_index_sequence<
                                             std::tuple_size<std::remove_reference_t<Tuple>>::value>{});
            }

            // Parse single tuple element
            template<typename T>
            void parse_tuple_element(const char *name, std::size_t index, std::size_t size,
                                     T &&result) {
                auto [value, success] = parse_argument<typename std::remove_reference<T>::type>(name);
                if (success) {
                    result = value;
                } else {
                    if (next_index == arguments.size()) {
                        // end of arguments list
                        // failed to parse tuple <>. expected `size` arguments, `index` provided
                        throw structopt::exception("Error: failed to correctly parse tuple `" +
                                                           std::string{name} + "`. Expected " +
                                                           std::to_string(size) + " arguments, " +
                                                           std::to_string(index) + " provided.",
                                                   visitor);
                    } else {
                        throw structopt::exception("Error: failed to correctly parse tuple `" +
                                                           std::string{name} +
                                                           "` {size = " + std::to_string(size) +
                                                           "} at index " + std::to_string(index) + ".",
                                                   visitor);
                    }
                }
            }

            // Tuple argument
            template<typename Tuple>
            Tuple parse_tuple_argument(const char *name) {
                Tuple result;
                std::size_t i = 0;
                constexpr auto tuple_size = std::tuple_size<Tuple>::value;
                for_each(result, [&](auto &&arg) {
                    parse_tuple_element(name, i, tuple_size, arg);
                    i += 1;
                });
                return result;
            }

            // Vector, deque, list
            template<typename T>
            T parse_vector_like_argument(const char *name) {
                T result;

                // Parse from current till end
                while (next_index < arguments.size()) {
                    const auto next = arguments[next_index];
                    if (is_optional_field(next) || std::string{next} == "--" ||
                        is_delimited_optional_argument(next).first) {
                        if (std::string{next} == "--") {
                            double_dash_encountered = true;
                            next_index += 1;
                        }
                        // this marks the end of the container (break here)
                        break;
                    }
                    auto [value, success] = parse_argument<typename T::value_type>(name);
                    if (success) {
                        result.push_back(value);
                    }
                }
                return result;
            }

            // stack, queue, priority_queue
            template<typename T>
            T parse_container_adapter_argument(const char *name) {
                T result;
                // Parse from current till end
                while (next_index < arguments.size()) {
                    const auto next = arguments[next_index];
                    if (is_optional_field(next) || std::string{next} == "--" ||
                        is_delimited_optional_argument(next).first) {
                        if (std::string{next} == "--") {
                            double_dash_encountered = true;
                            next_index += 1;
                        }
                        // this marks the end of the container (break here)
                        break;
                    }
                    auto [value, success] = parse_argument<typename T::value_type>(name);
                    if (success) {
                        result.push(value);
                    }
                }
                return result;
            }

            // Set, multiset, unordered_set, unordered_multiset
            template<typename T>
            T parse_set_argument(const char *name) {
                T result;
                // Parse from current till end
                while (next_index < arguments.size()) {
                    const auto next = arguments[next_index];
                    if (is_optional_field(next) || std::string{next} == "--" ||
                        is_delimited_optional_argument(next).first) {
                        if (std::string{next} == "--") {
                            double_dash_encountered = true;
                            next_index += 1;
                        }
                        // this marks the end of the container (break here)
                        break;
                    }
                    auto [value, success] = parse_argument<typename T::value_type>(name);
                    if (success) {
                        result.insert(value);
                    }
                }
                return result;
            }

            // Enum class
            template<typename T>
            T parse_enum_argument(const char *name) {
                T result;
                auto maybe_enum_value = magic_enum::enum_cast<T>(arguments[next_index]);
                if (maybe_enum_value.has_value()) {
                    result = maybe_enum_value.value();
                } else {
                    constexpr auto allowed_names = magic_enum::enum_names<T>();

                    std::string allowed_names_string = "";
                    if (allowed_names.size()) {
                        for (size_t i = 0; i < allowed_names.size() - 1; i++) {
                            allowed_names_string += std::string{allowed_names[i]} + ", ";
                        }
                        allowed_names_string += allowed_names[allowed_names.size() - 1];
                    }

                    throw structopt::exception(
                            "Error: unexpected input `" + std::string{arguments[next_index]} +
                                    "` provided for enum argument `" + std::string{name} +
                                    "`. Allowed values are {" + allowed_names_string + "}",
                            visitor);
                    // TODO: Throw error invalid enum option
                }
                return result;
            }

            // Visitor function for nested struct
            template<typename T>
            inline typename std::enable_if<visit_struct::traits::is_visitable<T>::value, void>::type
            operator()(const char *name, T &value) {
                if (next_index > current_index) {
                    current_index = next_index;
                }

                if (current_index < arguments.size()) {
                    const auto next = arguments[current_index];
                    const auto field_name = std::string{name};

                    // Check if `next` is the start of a subcommand
                    if (visitor.is_field_name(next) && next == field_name) {
                        next_index += 1;
                        value = parse_nested_struct<T>(name);
                    }
                }
            }

            // Visitor function for any positional field (not std::optional)
            template<typename T>
            inline typename std::enable_if<!structopt::is_specialization<T, std::optional>::value &&
                                                   !visit_struct::traits::is_visitable<T>::value,
                                           void>::type
            operator()(const char *name, T &result) {
                if (next_index > current_index) {
                    current_index = next_index;
                }

                if (current_index < arguments.size()) {
                    const auto next = arguments[current_index];

                    if (is_optional(next)) {
                        return;
                    }

                    if (visitor.positional_field_names.empty()) {
                        // We're not looking to save any more positional fields
                        // all of them already have a value
                        throw structopt::exception("Error: unexpected argument '" + next + "'", visitor);
                        return;
                    }

                    const auto field_name = visitor.positional_field_names.front();

                    // // This will be parsed as a subcommand (nested struct)
                    // if (visitor.is_field_name(next) && next == field_name) {
                    //   return;
                    // }

                    if (field_name != std::string{name}) {
                        // current field is not the one we want to parse
                        return;
                    }

                    // Remove from the positional field list as it is about to be parsed
                    visitor.positional_field_names.pop_front();

                    auto [value, success] = parse_argument<T>(field_name.c_str());
                    if (success) {
                        result = value;
                    } else {
                        // positional field does not yet have a value
                        visitor.positional_field_names.push_front(field_name);
                    }
                }
            }

            // Visitor function for std::optional field
            template<typename T>
            inline typename std::enable_if<structopt::is_specialization<T, std::optional>::value,
                                           void>::type
            operator()(const char *name, T &value) {
                if (next_index > current_index) {
                    current_index = next_index;
                }

                if (current_index < arguments.size()) {
                    const auto next = arguments[current_index];
                    const auto field_name = std::string{name};

                    if (next == "--" && double_dash_encountered == false) {
                        double_dash_encountered = true;
                        next_index += 1;
                        return;
                    }

                    // if `next` looks like an optional argument
                    // i.e., starts with `-` or `--`
                    // see if you can find an optional field in the struct with a matching name

                    // check if the current argument looks like it could be this optional field
                    if (double_dash_encountered == false && is_optional_field(next, field_name)) {

                        // this is an optional argument matching the current struct field
                        if constexpr (std::is_same<typename T::value_type, bool>::value) {
                            // It is a boolean optional argument
                            // Does it have a default value?
                            // If yes, this is a FLAG argument, e.g,, "--verbose" will set it to true if the
                            // default value is false No need to write "--verbose true"
                            if (value.has_value()) {
                                // The field already has a default value!
                                value = !value.value();// simply toggle it
                                next_index += 1;
                            } else {
                                // boolean optional argument doesn't have a default value
                                // expect one
                                value = parse_optional_argument<typename T::value_type>(name);
                            }
                        } else {
                            // Not std::optional<bool>
                            // Parse the argument type <T>
                            value = parse_optional_argument<typename T::value_type>(name);
                        }
                    } else {
                        if (double_dash_encountered == false) {

                            // maybe this is an optional argument that is delimited with '=' or ':'
                            // e.g., --foo=bar or --foo:BAR
                            if (next.size() > 1 && next[0] == '-') {
                                const auto [success, delimiter] = is_delimited_optional_argument(next);
                                if (success) {
                                    const auto [lhs, rhs] = split_delimited_argument(delimiter, next);
                                    // update next_index and return
                                    // the parser will take care of the rest

                                    // if `lhs` is an optional argument (i.e., maps to an optional field in the original struct), then insert into arguments list
                                    auto potential_field_name = get_full_optional_field_name(lhs);
                                    if (potential_field_name.has_value()) {
                                        for (auto &arg: {rhs, lhs}) {
                                            const auto begin = arguments.begin();
                                            arguments.insert(begin + next_index + 1, arg);
                                        }
                                    }
                                    // get past the current argument, e.g., `--foo=bar`
                                    next_index += 1;
                                    return;
                                }
                            }

                            // A direct match of optional argument with field_name has not happened
                            // This _could_ be a combined argument
                            // e.g., -abc => -a, -b, and -c where each of these is a flag argument

                            std::vector<std::string> potential_combined_argument;

                            if (is_optional_field(next) == false && next[0] == '-' &&
                                (next.size() > 1 && next[1] != '-')) {
                                for (std::size_t i = 1; i < next.size(); i++) {
                                    potential_combined_argument.push_back("-" + std::string(1, next[i]));
                                }
                            }

                            if (!potential_combined_argument.empty()) {
                                bool is_combined_argument = true;
                                for (auto &arg: potential_combined_argument) {
                                    if (!is_optional_field(arg)) {
                                        is_combined_argument = false;
                                        // TODO: report error unrecognized option in combined argument
                                    }
                                }

                                if (is_combined_argument) {

                                    // check and make sure the current field_name is
                                    // in `potential_combined_argument`
                                    //
                                    // Let's say the argument `next` is `-abc`
                                    // the current field name is `b`
                                    // 1. Split `-abc` into `-a`, `-b`, and `-c`
                                    // 2. Check if `-b` is in the above list
                                    //    1. If yes, consider this as a combined argument
                                    //       push the list of arguments (split up) into `arguments`
                                    //    2. If no, nothing to do here
                                    bool field_name_matched = false;
                                    for (auto &arg: potential_combined_argument) {
                                        if (arg == "-" + std::string(1, field_name[0])) {
                                            field_name_matched = true;
                                        }
                                    }

                                    if (field_name_matched) {
                                        // confirmed: this is a combined argument

                                        // insert the individual options that make up the combined argument
                                        // right after the combined argument
                                        // e.g., ""./main -abc" becomes "./main -abc -a -b -c"
                                        // Once this is done, increment `next_index` so that the parser loop will
                                        // service
                                        // `-a`, `-b` and `-c` like any other optional arguments (flags and
                                        // otherwise)
                                        for (std::vector<std::string>::reverse_iterator it =
                                                     potential_combined_argument.rbegin();
                                             it != potential_combined_argument.rend(); ++it) {
                                            auto &arg = *it;
                                            if (next_index < arguments.size()) {
                                                auto begin = arguments.begin();
                                                arguments.insert(begin + next_index + 1, arg);
                                            } else {
                                                arguments.push_back(arg);
                                            }
                                        }

                                        // get past the current combined argument
                                        next_index += 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        };

        // Specialization for std::string
        template<>
        inline std::string parser::parse_single_argument<std::string>(const char *) {
            return arguments[next_index];
        }

        // Specialization for bool
        // yes, YES, on, 1, true, TRUE, etc. = true
        // no, NO, off, 0, false, FALSE, etc. = false
        // Converts argument to lower case before check
        template<>
        inline bool parser::parse_single_argument<bool>(const char *name) {
            if (next_index > current_index) {
                current_index = next_index;
            }

            if (current_index < arguments.size()) {
                const std::vector<std::string> true_strings{"on", "yes", "1", "true"};
                const std::vector<std::string> false_strings{"off", "no", "0", "false"};
                std::string current_argument = arguments[current_index];

                // Convert argument to lower case
                std::transform(current_argument.begin(), current_argument.end(),
                               current_argument.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                // Detect if argument is true or false
                if (std::find(true_strings.begin(), true_strings.end(), current_argument) !=
                    true_strings.end()) {
                    return true;
                } else if (std::find(false_strings.begin(), false_strings.end(), current_argument) !=
                           false_strings.end()) {
                    return false;
                } else {
                    throw structopt::exception("Error: failed to parse boolean argument `" +
                                                       std::string{name} + "`." + " `" + current_argument +
                                                       "`" + " is invalid.",
                                               visitor);
                    return false;
                }
            } else {
                return false;
            }
        }

    }// namespace details

}// namespace structopt

#pragma once
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
// #include <structopt/is_stl_container.hpp>
// #include <structopt/parser.hpp>
// #include <structopt/third_party/visit_struct/visit_struct.hpp>
#include <type_traits>
#include <vector>

#define STRUCTOPT VISITABLE_STRUCT

namespace structopt {

    class app {
        details::visitor visitor;

    public:
        explicit app(const std::string &name, const std::string &version = "", const std::string &help = "")
            : visitor(name, version, help) {}

        template<typename T>
        T parse(const std::vector<std::string> &arguments) {
            T argument_struct = T();

            // Visit the struct and save flag, optional and positional field names
            visit_struct::for_each(argument_struct, visitor);

            // add `help` and `version` optional arguments
            visitor.optional_field_names.push_back("help");
            visitor.optional_field_names.push_back("version");

            // Construct the argument parser
            structopt::details::parser parser;
            parser.visitor = visitor;
            parser.arguments = arguments;

            for (std::size_t i = 1; i < parser.arguments.size(); i++) {
                parser.current_index = i;
                visit_struct::for_each(argument_struct, parser);
            }

            // directly call the parser to check for `help` and `version` flags
            std::optional<bool> help = false, version = false;
            for (std::size_t i = 1; i < parser.arguments.size(); i++) {
                parser.operator()("help", help);
                parser.operator()("version", version);

                if (help == true) {
                    // if help is requested, print help and exit
                    visitor.print_help(std::cout);
                    exit(EXIT_SUCCESS);
                } else if (version == true) {
                    // if version is requested, print version and exit
                    std::cout << visitor.version << "\n";
                    exit(EXIT_SUCCESS);
                }
            }

            // if all positional arguments were provided
            // this list would be empty
            if (!parser.visitor.positional_field_names.empty()) {
                for (auto &field_name: parser.visitor.positional_field_names) {
                    if (std::find(parser.visitor.vector_like_positional_field_names.begin(),
                                  parser.visitor.vector_like_positional_field_names.end(),
                                  field_name) ==
                        parser.visitor.vector_like_positional_field_names.end()) {
                        // this positional argument is not a vector-like argument
                        // it expects value(s)
                        throw structopt::exception("Error: expected value for positional argument `" +
                                                           field_name + "`.",
                                                   parser.visitor);
                    }
                }
            }

            if (parser.current_index < parser.arguments.size()) {
                throw structopt::exception("Error: unrecognized argument '" + parser.arguments[parser.current_index] + "'", parser.visitor);
            }

            return argument_struct;
        }

        template<typename T>
        T parse(int argc, char *argv[]) {
            std::vector<std::string> arguments;
            std::copy(argv, argv + argc, std::back_inserter(arguments));
            return parse<T>(arguments);
        }

        std::string help() const {
            std::stringstream os;
            visitor.print_help(os);
            return os.str();
        }
    };

}// namespace structopt
