

#ifndef IMTERM_MISC_HPP
#define IMTERM_MISC_HPP

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                                                     ///
///  Copyright C 2019, Lucas Lazare                                                                                                     ///
///  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation         ///
///  files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy,         ///
///  modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software     ///
///  is furnished to do so, subject to the following conditions:                                                                        ///
///                                                                                                                                     ///
///  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.     ///
///                                                                                                                                     ///
///  The Software is provided “as is”, without warranty of any kind, express or implied, including but not limited to the               ///
///  warranties of merchantability, fitness for a particular purpose and noninfringement. In no event shall the authors or              ///
///  copyright holders be liable for any claim, damages or other liability, whether in an action of contract, tort or otherwise,        ///
///  arising from, out of or in connection with the software or the use or other dealings in the Software.                              ///
///                                                                                                                                     ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>

namespace misc {

    // std::identity is c++20
    struct identity
    {
        template<typename T>
        constexpr T &&operator()(T &&t) const {
            return std::forward<T>(t);
        }
    };

    namespace details {
        struct structured_void
        {
        };
        inline structured_void no_value{};

        template<typename T>
        struct non_void
        {
            using type = T;
        };
        template<>
        struct non_void<void>
        {
            using type = structured_void;
        };
    }// namespace details
    // type T if T is non void, details::structured void otherwise
    template<typename T>
    using non_void_t = typename details::non_void<T>::type;


    // Returns the length of the given byte string, at most buffer_size
    constexpr unsigned int strnlen(const char *beg, unsigned int buffer_size) {
        unsigned int len = 0;
        while (len < buffer_size && beg[len] != '\0') {
            ++len;
        }
        return len;
    }

    // std::is_sorted is not constexpr pre C++20
    template<typename ForwardIt, typename EndIt, typename Comparator = std::less<std::remove_reference_t<decltype(*std::declval<ForwardIt>())>>>
    constexpr bool is_sorted(ForwardIt it, EndIt last, Comparator &&comp = {}) {
        if (it == last) {
            return true;
        }

        for (ForwardIt next = std::next(it); next != last; ++next, ++it) {
            if (comp(*next, *it)) {
                return false;
            }
        }
        return true;
    }

    // returns the length of the longest element of the list, minimum 0
    template<typename ForwardIt, typename EndIt, typename SizeExtractor>
    constexpr auto max_size(ForwardIt it, EndIt last, SizeExtractor &&size_of) -> decltype(size_of(*it)) {
        using size_type = decltype(size_of(*it));
        size_type max_size = 0u;
        while (it != last) {
            size_type current_size = size_of(*it++);
            if (current_size > max_size) {
                max_size = current_size;
            }
        }
        return max_size;
    }

    // copies until it reaches the end of any of the two collections
    template<typename ForwardIt, typename OutputIt>
    constexpr void copy(ForwardIt src_beg, ForwardIt src_end, OutputIt dest_beg, OutputIt dest_end) {
        while (src_beg != src_end && dest_beg != dest_end) {
            *dest_beg++ = *src_beg++;
        }
    }

    // same as misc::copy, but in reversed order
    template<typename BidirIt1, typename BidirIt2>
    constexpr void copy_backward(BidirIt1 src_beg, BidirIt1 src_end, BidirIt2 dest_beg, BidirIt2 dest_end) {
        auto copy_length = std::distance(src_beg, src_end);
        auto avail_length = std::distance(dest_beg, dest_end);
        if (avail_length < copy_length) {
            std::advance(src_end, avail_length - copy_length);
        } else {
            std::advance(dest_end, copy_length - avail_length);
        }
        while (src_beg != src_end && dest_beg != dest_end) {
            *--dest_end = *--src_end;
        }
    }

    // Returns new end of dest collection
    // ignores the n first values of the destination
    template<typename ForwardIt, typename RandomAccessIt>
    constexpr RandomAccessIt erase_insert(ForwardIt src_beg, ForwardIt src_end, RandomAccessIt dest_beg, RandomAccessIt dest_end, RandomAccessIt dest_max, unsigned int n) {
        n = std::min(static_cast<unsigned>(std::distance(dest_beg, dest_end)), n);
        auto copy_length = static_cast<unsigned>(std::distance(src_beg, src_end));
        auto avail_length = static_cast<unsigned>(std::distance(dest_end, dest_max)) + n;

        if (copy_length <= avail_length) {
            misc::copy_backward(dest_beg + n, dest_end, dest_beg + copy_length, dest_end + copy_length);
            misc::copy(src_beg, src_end, dest_beg, dest_beg + copy_length);
            return dest_end + copy_length - n;

        } else {
            std::advance(src_beg, copy_length - avail_length);
            copy_length = avail_length;
            misc::copy_backward(dest_beg + n, dest_end, dest_beg + copy_length, dest_end + copy_length);
            misc::copy(src_beg, src_end, dest_beg, dest_beg + copy_length);
            return dest_end + copy_length - n;
        }
    }

    // returns the last element in the range [begin, end) that is equal to val, returns end if there is no such element
    template<typename ForwardIterator, typename ValueType>
    constexpr ForwardIterator find_last(ForwardIterator begin, ForwardIterator end, const ValueType &val) {
        auto rend = std::reverse_iterator(begin);
        auto rbegin = std::reverse_iterator(end);
        auto search = std::find(rbegin, rend, val);
        if (search == rend) {
            return end;
        }
        return std::prev(search.base());
    }

    // Returns an iterator to the first character that has no a space after him
    template<typename ForwardIterator, typename SpaceDetector>
    constexpr ForwardIterator find_terminating_word(ForwardIterator begin, ForwardIterator end, SpaceDetector &&is_space_pred) {
        auto rend = std::reverse_iterator(begin);
        auto rbegin = std::reverse_iterator(end);

        int sp_size = 0;
        auto is_space = [&sp_size, &is_space_pred, &end](char c) {
            sp_size = is_space_pred(std::string_view{&c, static_cast<unsigned>(&*std::prev(end) - &c)});
            return sp_size > 0;
        };

        auto search = std::find_if(rbegin, rend, is_space);
        if (search == rend) {
            return begin;
        }
        ForwardIterator it = std::prev(search.base());
        it += sp_size;
        return it;
    }

    constexpr bool success(std::errc ec) {
        return ec == std::errc{};
    }

    // Search any element starting by "prefix" in the sorted collection formed by [c_beg, c_end)
    // str_ext must map dectype(*c_beg) to std::string_view
    // transform is whatever transformation you want to do to the matching elements.
    template<typename ForwardIt, typename StrExtractor = identity, typename Transform = identity>
    auto prefix_search(std::string_view prefix, ForwardIt c_beg, ForwardIt c_end,
                       StrExtractor &&str_ext = {}, Transform &&transform = {}) -> std::vector<std::decay_t<decltype(transform(*c_beg))>> {

        auto lower = std::lower_bound(c_beg, c_end, prefix);
        auto higher = std::upper_bound(lower, c_end, prefix, [&str_ext](std::string_view pre, const auto &element) {
            std::string_view val = str_ext(element);
            return pre.substr(0, std::min(val.size(), pre.size())) < val.substr(0, std::min(val.size(), pre.size()));
        });

        std::vector<std::decay_t<decltype(transform(*c_beg))>> ans;
        std::transform(lower, higher, std::back_inserter(ans), std::forward<Transform>(transform));
        return ans;
    }

    // returns an iterator to the first element starting by "prefix"
    // is_space is a predicate returning a value greater than 0 if a given string_view starts by a space and 0 otherwise
    // str_ext is an unary functor maping decltype(*beg) to std::string_view
    template<typename ForwardIt, typename IsSpace, typename StrExtractor = identity>
    ForwardIt find_first_prefixed(std::string_view prefix, ForwardIt beg, ForwardIt end, IsSpace &&is_space, StrExtractor &&str_ext = {}) {
        // std::string_view::start_with is C++20
        auto start_with = [&](std::string_view str, std::string_view pr) {
            std::string_view::size_type idx = 0;
            int space_count{};
            while (idx < str.size() && (space_count = is_space(str.substr(idx))) > 0) {
                idx += space_count;
            }
            return (str.size() - idx) >= pr.size() ? str.substr(idx, pr.size()) == pr : false;
        };
        while (beg != end) {
            if (start_with(str_ext(*beg), prefix)) {
                return beg;
            }
            ++beg;
        }
        return beg;
    }


    namespace details {
        template<typename AlwaysVoid, template<typename...> typename Op, typename... Args>
        struct detector : std::false_type
        {
            using type = std::is_same<void, void>;
        };

        template<template<typename...> typename Op, typename... Args>
        struct detector<std::void_t<Op<Args...>>, Op, Args...> : std::true_type
        {
            using type = Op<Args...>;
        };
    }// namespace details

    template<template<typename...> typename Op, typename... Args>
    using is_detected = typename details::detector<void, Op, Args...>;

    template<template<typename...> typename Op, typename... Args>
    using is_detected_t = typename is_detected<Op, Args...>::type;

    // compile time function detection, return type agnostic
    template<template<typename...> typename Op, typename... Args>
    constexpr bool is_detected_v = is_detected<Op, Args...>::value;

    // compile time function detection
    template<template<typename...> typename Op, typename ReturnType, typename... Args>
    constexpr bool is_detected_with_return_type_v = std::is_same_v<is_detected_t<Op, Args...>, ReturnType>;

    // dummy mutex
    struct no_mutex
    {
        constexpr no_mutex() noexcept = default;
        constexpr void lock() {}
        constexpr void unlock() {}
        constexpr bool try_lock() { return true; }
    };
}// namespace misc

#endif//IMTERM_MISC_HPP


#ifndef IMTERM_UTILS_HPP
#define IMTERM_UTILS_HPP

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                                                     ///
///  Copyright C 2019, Lucas Lazare                                                                                                     ///
///  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation         ///
///  files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy,         ///
///  modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software     ///
///  is furnished to do so, subject to the following conditions:                                                                        ///
///                                                                                                                                     ///
///  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.     ///
///                                                                                                                                     ///
///  The Software is provided “as is”, without warranty of any kind, express or implied, including but not limited to the               ///
///  warranties of merchantability, fitness for a particular purpose and noninfringement. In no event shall the authors or              ///
///  copyright holders be liable for any claim, damages or other liability, whether in an action of contract, tort or otherwise,        ///
///  arising from, out of or in connection with the software or the use or other dealings in the Software.                              ///
///                                                                                                                                     ///
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <imgui.h>
#include <optional>
#include <string>
#include <string_view>

namespace ImTerm {
    // argument passed to commands
    template<typename Terminal>
    struct argument_t
    {
        using value_type = misc::non_void_t<typename Terminal::value_type>;

        value_type &val;// misc::details::structured_void if Terminal::value_type is void, reference to Terminal::value_type otherwise
        Terminal &term; // reference to the ImTerm::terminal that called the command

        std::vector<std::string> command_line;// list of arguments the user specified in the command line. command_line[0] is the command name
    };

    // structure used to represent a command
    template<typename Terminal>
    struct command_t
    {
        using command_function = void (*)(argument_t<Terminal> &);
        using further_completion_function = std::vector<std::string> (*)(argument_t<Terminal> &argument_line);

        std::string_view name{};       // name of the command
        std::string_view description{};// short description
        command_function call{};       // function doing whatever you want

        further_completion_function complete{};// function called when users starts typing in arguments for your command
        // return a vector of strings containing possible completions.

        friend constexpr bool operator<(const command_t &lhs, const command_t &rhs) {
            return lhs.name < rhs.name;
        }

        friend constexpr bool operator<(const command_t &lhs, const std::string_view &rhs) {
            return lhs.name < rhs;
        }

        friend constexpr bool operator<(const std::string_view &lhs, const command_t &rhs) {
            return lhs < rhs.name;
        }
    };

    struct message
    {
        enum class type {
            user_input,            // terminal wants to log user input
            error,                 // terminal wants to log an error in user input
            cmd_history_completion,// terminal wants to log that it replaced "!:*" family input in the appropriate string
        };
        struct severity
        {
            enum severity_t {// done this way to be used as array index without a cast
                trace,
                debug,
                info,
                warn,
                err,
                critical
            };
        };

        severity::severity_t severity;// severity of the message
        std::string value;            // text to be displayed

        // the actual color used depends on the message's severity
        size_t color_beg;// color begins at value.data() + color_beg
        size_t color_end;// color ends at value.data() + color_end - 1
        // if color_beg == color_end, nothing is colorized.

        bool is_term_message;// if set to true, current msg is considered to be originated from the terminal,
        // never filtered out by severity filter, and applying for different rules regarding colors.
        // severity is also ignored for such messages
    };

    enum class config_panels {
        autoscroll,
        autowrap,
        clearbutton,
        filter,
        long_filter,// like filter, but takes up space
        loglevel,
        blank,// invisible panel that takes up place, aligning items to the right. More than one can be used, splitting up the consumed space
        // ie: {clearbutton (C), blank, filter (F), blank, loglevel (L)} will result in the layout [C           F           L]
        // Shares space with long_filter
        none
    };

    enum class position {
        up,
        down,
        nowhere// disabled
    };

    // Various settable colors for the terminal
    // if an optional is empty, current ImGui color will be used
    struct theme
    {
        struct constexpr_color
        {
            float r, g, b, a;

            ImVec4 imv4() const {
                return {r, g, b, a};
            }
        };

        std::string_view name;// if you want to give a name to the theme

        std::optional<constexpr_color> text;                       // global text color
        std::optional<constexpr_color> window_bg;                  // ImGuiCol_WindowBg & ImGuiCol_ChildBg
        std::optional<constexpr_color> border;                     // ImGuiCol_Border
        std::optional<constexpr_color> border_shadow;              // ImGuiCol_BorderShadow
        std::optional<constexpr_color> button;                     // ImGuiCol_Button
        std::optional<constexpr_color> button_hovered;             // ImGuiCol_ButtonHovered
        std::optional<constexpr_color> button_active;              // ImGuiCol_ButtonActive
        std::optional<constexpr_color> frame_bg;                   // ImGuiCol_FrameBg
        std::optional<constexpr_color> frame_bg_hovered;           // ImGuiCol_FrameBgHovered
        std::optional<constexpr_color> frame_bg_active;            // ImGuiCol_FrameBgActive
        std::optional<constexpr_color> text_selected_bg;           // ImGuiCol_TextSelectedBg, for text input field
        std::optional<constexpr_color> check_mark;                 // ImGuiCol_CheckMark
        std::optional<constexpr_color> title_bg;                   // ImGuiCol_TitleBg
        std::optional<constexpr_color> title_bg_active;            // ImGuiCol_TitleBgActive
        std::optional<constexpr_color> title_bg_collapsed;         // ImGuiCol_TitleBgCollapsed
        std::optional<constexpr_color> message_panel;              // logging panel
        std::optional<constexpr_color> auto_complete_selected;     // left-most text in the autocompletion OSD
        std::optional<constexpr_color> auto_complete_non_selected; // every text but the left most in the autocompletion OSD
        std::optional<constexpr_color> auto_complete_separator;    // color for the separator in the autocompletion OSD
        std::optional<constexpr_color> cmd_backlog;                // color for message type user_input
        std::optional<constexpr_color> cmd_history_completed;      // color for message type cmd_history_completion
        std::optional<constexpr_color> log_level_drop_down_list_bg;// ImGuiCol_PopupBg
        std::optional<constexpr_color> log_level_active;           // ImGuiCol_HeaderActive
        std::optional<constexpr_color> log_level_hovered;          // ImGuiCol_HeaderHovered
        std::optional<constexpr_color> log_level_selected;         // ImGuiCol_Header
        std::optional<constexpr_color> scrollbar_bg;               // ImGuiCol_ScrollbarBg
        std::optional<constexpr_color> scrollbar_grab;             // ImGuiCol_ScrollbarGrab
        std::optional<constexpr_color> scrollbar_grab_active;      // ImGuiCol_ScrollbarGrabActive
        std::optional<constexpr_color> scrollbar_grab_hovered;     // ImGuiCol_ScrollbarGrabHovered
        std::optional<constexpr_color> filter_hint;                // ImGuiCol_TextDisabled
        std::optional<constexpr_color> filter_text;                // user input in log filter
        std::optional<constexpr_color> matching_text;              // text matching the log filter

        std::array<std::optional<constexpr_color>, message::severity::critical + 1> log_level_colors{};// colors by severity
    };


    namespace themes {

        constexpr theme light = {
                "Light Rainbow",
                theme::constexpr_color{0.100f, 0.100f, 0.100f, 1.000f},//text
                theme::constexpr_color{0.243f, 0.443f, 0.624f, 1.000f},//window_bg
                theme::constexpr_color{0.600f, 0.600f, 0.600f, 1.000f},//border
                theme::constexpr_color{0.000f, 0.000f, 0.000f, 0.000f},//border_shadow
                theme::constexpr_color{0.902f, 0.843f, 0.843f, 0.875f},//button
                theme::constexpr_color{0.824f, 0.765f, 0.765f, 0.875f},//button_hovered
                theme::constexpr_color{0.627f, 0.569f, 0.569f, 0.875f},//button_active
                theme::constexpr_color{0.902f, 0.843f, 0.843f, 0.875f},//frame_bg
                theme::constexpr_color{0.824f, 0.765f, 0.765f, 0.875f},//frame_bg_hovered
                theme::constexpr_color{0.627f, 0.569f, 0.569f, 0.875f},//frame_bg_active
                theme::constexpr_color{0.260f, 0.590f, 0.980f, 0.350f},//text_selected_bg
                theme::constexpr_color{0.843f, 0.000f, 0.373f, 1.000f},//check_mark
                theme::constexpr_color{0.243f, 0.443f, 0.624f, 0.850f},//title_bg
                theme::constexpr_color{0.165f, 0.365f, 0.506f, 1.000f},//title_bg_active
                theme::constexpr_color{0.243f, 0.443f, 0.624f, 0.850f},//title_collapsed
                theme::constexpr_color{0.902f, 0.843f, 0.843f, 0.875f},//message_panel
                theme::constexpr_color{0.196f, 1.000f, 0.196f, 1.000f},//auto_complete_selected
                theme::constexpr_color{0.000f, 0.000f, 0.000f, 1.000f},//auto_complete_non_selected
                theme::constexpr_color{0.000f, 0.000f, 0.000f, 0.392f},//auto_complete_separator
                theme::constexpr_color{0.519f, 0.118f, 0.715f, 1.000f},//cmd_backlog
                theme::constexpr_color{1.000f, 0.430f, 0.059f, 1.000f},//cmd_history_completed
                theme::constexpr_color{0.901f, 0.843f, 0.843f, 0.784f},//log_level_drop_down_list_bg
                theme::constexpr_color{0.443f, 0.705f, 1.000f, 1.000f},//log_level_active
                theme::constexpr_color{0.443f, 0.705f, 0.784f, 0.705f},//log_level_hovered
                theme::constexpr_color{0.443f, 0.623f, 0.949f, 1.000f},//log_level_selected
                theme::constexpr_color{0.000f, 0.000f, 0.000f, 0.000f},//scrollbar_bg
                theme::constexpr_color{0.470f, 0.470f, 0.588f, 1.000f},//scrollbar_grab
                theme::constexpr_color{0.392f, 0.392f, 0.509f, 1.000f},//scrollbar_grab_active
                theme::constexpr_color{0.509f, 0.509f, 0.666f, 1.000f},//scrollbar_grab_hovered
                theme::constexpr_color{0.470f, 0.470f, 0.470f, 1.000f},//filter_hint
                theme::constexpr_color{0.100f, 0.100f, 0.100f, 1.000f},//filter_text
                theme::constexpr_color{0.549f, 0.196f, 0.039f, 1.000f},//matching_text
                {
                        theme::constexpr_color{0.078f, 0.117f, 0.764f, 1.f},// log_level::trace
                        theme::constexpr_color{0.100f, 0.100f, 0.100f, 1.f},// log_level::debug
                        theme::constexpr_color{0.301f, 0.529f, 0.000f, 1.f},// log_level::info
                        theme::constexpr_color{0.784f, 0.431f, 0.058f, 1.f},// log_level::warning
                        theme::constexpr_color{0.901f, 0.117f, 0.117f, 1.f},// log_level::error
                        theme::constexpr_color{0.901f, 0.117f, 0.117f, 1.f},// log_level::critical
                }};

        constexpr theme cherry{
                "Dark Cherry",
                theme::constexpr_color{0.649f, 0.661f, 0.669f, 1.000f},//text
                theme::constexpr_color{0.130f, 0.140f, 0.170f, 1.000f},//window_bg
                theme::constexpr_color{0.310f, 0.310f, 1.000f, 0.000f},//border
                theme::constexpr_color{0.000f, 0.000f, 0.000f, 0.000f},//border_shadow
                theme::constexpr_color{0.470f, 0.770f, 0.830f, 0.140f},//button
                theme::constexpr_color{0.455f, 0.198f, 0.301f, 0.860f},//button_hovered
                theme::constexpr_color{0.455f, 0.198f, 0.301f, 1.000f},//button_active
                theme::constexpr_color{0.200f, 0.220f, 0.270f, 1.000f},//frame_bg
                theme::constexpr_color{0.455f, 0.198f, 0.301f, 0.780f},//frame_bg_hovered
                theme::constexpr_color{0.455f, 0.198f, 0.301f, 1.000f},//frame_bg_active
                theme::constexpr_color{0.455f, 0.198f, 0.301f, 0.430f},//text_selected_bg
                theme::constexpr_color{0.710f, 0.202f, 0.207f, 1.000f},//check_mark
                theme::constexpr_color{0.232f, 0.201f, 0.271f, 1.000f},//title_bg
                theme::constexpr_color{0.502f, 0.075f, 0.256f, 1.000f},//title_bg_active
                theme::constexpr_color{0.200f, 0.220f, 0.270f, 0.750f},//title_bg_collapsed
                theme::constexpr_color{0.100f, 0.100f, 0.100f, 0.500f},//message_panel
                theme::constexpr_color{1.000f, 1.000f, 1.000f, 1.000f},//auto_complete_selected
                theme::constexpr_color{0.500f, 0.450f, 0.450f, 1.000f},//auto_complete_non_selected
                theme::constexpr_color{0.600f, 0.600f, 0.600f, 1.000f},//auto_complete_separator
                theme::constexpr_color{0.860f, 0.930f, 0.890f, 1.000f},//cmd_backlog
                theme::constexpr_color{0.153f, 0.596f, 0.498f, 1.000f},//cmd_history_completed
                theme::constexpr_color{0.100f, 0.100f, 0.100f, 0.860f},//log_level_drop_down_list_bg
                theme::constexpr_color{0.730f, 0.130f, 0.370f, 1.000f},//log_level_active
                theme::constexpr_color{0.450f, 0.190f, 0.300f, 0.430f},//log_level_hovered
                theme::constexpr_color{0.730f, 0.130f, 0.370f, 0.580f},//log_level_selected
                theme::constexpr_color{0.000f, 0.000f, 0.000f, 0.000f},//scrollbar_bg
                theme::constexpr_color{0.690f, 0.690f, 0.690f, 0.800f},//scrollbar_grab
                theme::constexpr_color{0.490f, 0.490f, 0.490f, 0.800f},//scrollbar_grab_active
                theme::constexpr_color{0.490f, 0.490f, 0.490f, 1.000f},//scrollbar_grab_hovered
                theme::constexpr_color{0.649f, 0.661f, 0.669f, 1.000f},//filter_hint
                theme::constexpr_color{1.000f, 1.000f, 1.000f, 1.000f},//filter_text
                theme::constexpr_color{0.490f, 0.240f, 1.000f, 1.000f},//matching_text
                {
                        theme::constexpr_color{0.549f, 0.561f, 0.569f, 1.f},// log_level::trace
                        theme::constexpr_color{0.153f, 0.596f, 0.498f, 1.f},// log_level::debug
                        theme::constexpr_color{0.459f, 0.686f, 0.129f, 1.f},// log_level::info
                        theme::constexpr_color{0.839f, 0.749f, 0.333f, 1.f},// log_level::warning
                        theme::constexpr_color{1.000f, 0.420f, 0.408f, 1.f},// log_level::error
                        theme::constexpr_color{1.000f, 0.420f, 0.408f, 1.f},// log_level::critical
                },
        };

        constexpr std::array list{
                cherry,
                light};
    }// namespace themes
}// namespace ImTerm


#endif//IMTERM_UTILS_HPP


#ifndef IMTERM_TERMINAL_HPP
#define IMTERM_TERMINAL_HPP

#include <array>
#include <atomic>
#include <imgui.h>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#ifdef IMTERM_USE_FMT
#include "fmt/format.h"
#endif

namespace ImTerm {

    // checking that you can use a given class as a TerminalHelper
    // giving human-friendlier error messages than just letting the compiler explode
    namespace details {
        template<typename TerminalHelper, typename CommandTypeCref>
        struct assert_wellformed
        {

            template<typename T>
            using find_commands_by_prefix_method_v1 =
                    decltype(std::declval<T &>().find_commands_by_prefix(std::declval<std::string_view>()));

            template<typename T>
            using find_commands_by_prefix_method_v2 =
                    decltype(std::declval<T &>().find_commands_by_prefix(std::declval<const char *>(),
                                                                         std::declval<const char *>()));

            template<typename T>
            using list_commands_method = decltype(std::declval<T &>().list_commands());

            template<typename T>
            using format_method = decltype(std::declval<T &>().format(std::declval<std::string>(), std::declval<message::type>()));

            static_assert(
                    misc::is_detected_with_return_type_v<find_commands_by_prefix_method_v1, std::vector<CommandTypeCref>, TerminalHelper>,
                    "TerminalHelper should implement the method 'std::vector<command_type_cref> find_command_by_prefix(std::string_view)'. "
                    "See term::terminal_helper_example for reference");
            static_assert(
                    misc::is_detected_with_return_type_v<find_commands_by_prefix_method_v2, std::vector<CommandTypeCref>, TerminalHelper>,
                    "TerminalHelper should implement the method 'std::vector<command_type_cref> find_command_by_prefix(const char*, const char*)'. "
                    "See term::terminal_helper_example for reference");
            static_assert(
                    misc::is_detected_with_return_type_v<list_commands_method, std::vector<CommandTypeCref>, TerminalHelper>,
                    "TerminalHelper should implement the method 'std::vector<command_type_cref> list_commands()'. "
                    "See term::terminal_helper_example for reference");
            static_assert(
                    misc::is_detected_with_return_type_v<format_method, std::optional<message>, TerminalHelper>,
                    "TerminalHelper should implement the method 'std::optional<term::message> format(std::string, term::message::type)'. "
                    "See term::terminal_helper_example for reference");
        };
    }// namespace details

    template<typename TerminalHelper>
    class terminal {
        using buffer_type = std::array<char, 1024>;
        using small_buffer_type = std::array<char, 128>;

    public:
        using value_type = misc::non_void_t<typename TerminalHelper::value_type>;
        using command_type = command_t<terminal<TerminalHelper>>;
        using command_type_cref = std::reference_wrapper<const command_type>;
        using argument_type = argument_t<terminal>;

        using terminal_helper_is_valid = details::assert_wellformed<TerminalHelper, command_type_cref>;

        inline static const std::vector<config_panels> DEFAULT_ORDER = {config_panels::clearbutton,
                                                                        config_panels::autoscroll, config_panels::autowrap, config_panels::long_filter, config_panels::loglevel};

        // You shall call this constructor you used a non void value_type
        template<typename T = value_type, typename = std::enable_if_t<!std::is_same_v<T, misc::details::structured_void>>>
        explicit terminal(value_type &arg_value, const char *window_name_ = "terminal", int base_width_ = 900,
                          int base_height_ = 200, std::shared_ptr<TerminalHelper> th = std::make_shared<TerminalHelper>())
            : terminal(arg_value, window_name_, base_width_, base_height_, std::move(th), terminal_helper_is_valid{}) {}


        // You shall call this constructor you used a void value_type
        template<typename T = value_type, typename = std::enable_if_t<std::is_same_v<T, misc::details::structured_void>>>
        explicit terminal(const char *window_name_ = "terminal", int base_width_ = 900,
                          int base_height_ = 200, std::shared_ptr<TerminalHelper> th = std::make_shared<TerminalHelper>())
            : terminal(misc::details::no_value, window_name_, base_width_, base_height_, std::move(th), terminal_helper_is_valid{}) {}

        // Returns the underlying terminal helper
        std::shared_ptr<TerminalHelper> get_terminal_helper() {
            return m_t_helper;
        }

        // shows the terminal. Call at each frame (in a more ImGui style, this would be something like ImGui::terminal(....);
        // returns true if the terminal thinks it should still be displayed next frame, false if it thinks it should be hidden
        // return value is true except if a command required a close, or if the "escape" key was pressed.
        bool show(const std::vector<config_panels> &panels_order = DEFAULT_ORDER) noexcept;

        // returns the command line history
        const std::vector<std::string> &get_history() const noexcept {
            return m_command_history;
        }

        // if invoked, the next call to "show" will return false
        void set_should_close() noexcept {
            m_close_request = true;
        }

        // clears all theme's optionals
        void reset_colors() noexcept;

        // returns current theme
        struct theme &theme()
        {
            return m_colors;
        }

        // set whether the autocompletion OSD should be above/under the text input, or if it should be disabled
        void set_autocomplete_pos(position p) {
            m_autocomplete_pos = p;
        }

        // returns current autocompletion position
        position get_autocomplete_pos() const {
            return m_autocomplete_pos;
        }

        // logs a text to the message panel
        // added as terminal message with info severity
        void add_text(std::string str, unsigned int color_beg, unsigned int color_end);

        // logs a text to the message panel, color spans from color_beg to the end of the message
        // added as terminal message with info severity
        void add_text(std::string str, unsigned int color_beg) {
            add_text(str, color_beg, static_cast<unsigned>(str.size()));
        }

        // logs a colorless text to the message panel
        // added as a terminal message with info severity,
        void add_text(std::string str) {
            add_text(str, 0, 0);
        }

        // logs a text to the message panel
        // added as terminal message with warn severity
        void add_text_err(std::string str, unsigned int color_beg, unsigned int color_end);

        // logs a text to the message panel, color spans from color_beg to the end of the message
        // added as terminal message with warn severity
        void add_text_err(std::string str, unsigned int color_beg) {
            add_text_err(str, color_beg, static_cast<unsigned>(str.size()));
        }

        // logs a colorless text to the message panel
        // added as terminal message with warn severity
        void add_text_err(std::string str) {
            add_text_err(str, 0, 0);
        }

        // logs a message to the message panel
        void add_message(const message &msg) {
            add_message(message{msg});
        }
        void add_message(message &&msg);

        // clears the message panel
        void clear();

        message::severity::severity_t log_level() noexcept {
            return m_level + m_lowest_log_level_val;
        }

        void log_level(message::severity::severity_t new_level) noexcept {
            if (m_lowest_log_level_val > new_level) {
                set_min_log_level(new_level);
            }
            m_level = new_level - m_lowest_log_level_val;
        }

        // returns the text used to label the button that clears the message panel
        // set it to an empty optional if you don't want the button to be displayed
        std::optional<std::string> &clear_text() noexcept {
            return m_clear_text;
        }

        // returns the text used to label the checkbox enabling or disabling message panel auto scrolling
        // set it to an empty optional if you don't want the checkbox to be displayed
        std::optional<std::string> &autoscroll_text() noexcept {
            return m_autoscroll_text;
        }

        // returns the text used to label the checkbox enabling or disabling message panel text auto wrap
        // set it to an empty optional if you don't want the checkbox to be displayed
        std::optional<std::string> &autowrap_text() noexcept {
            return m_autowrap_text;
        }

        // returns the text used to label the drop down list used to select the minimum severity to be displayed
        // set it to an empty optional if you don't want the drop down list to be displayed
        std::optional<std::string> &log_level_text() noexcept {
            return m_log_level_text;
        }

        // returns the text used to label the text input used to filter out logs
        // set it to an empty optional if you don't want the filter to be displayed
        std::optional<std::string> &filter_hint() noexcept {
            return m_filter_hint;
        }

        // allows you to set the text in the log_level drop down list
        // the std::string_view/s are copied, so you don't need to manage their life-time
        // set log_level_text() to an empty optional if you want to disable the drop down list
        void set_level_list_text(std::string_view trace_str, std::string_view debug_str, std::string_view info_str,
                                 std::string_view warn_str, std::string_view err_str, std::string_view critical_str, std::string_view none_str);

        // sets the maximum verbosity a user can set in the terminal with the log_level drop down list
        // for instance, if you pass 'info', the user will be able to select 'info','warning','error', 'critical', and 'none',
        // but will never be able to view 'trace' and 'debug' messages
        void set_min_log_level(message::severity::severity_t level);

        // Adds custom flags to the terminal window
        void set_flags(ImGuiWindowFlags flags) noexcept {
            m_flags = flags;
        }

        // Sets the maximum number of saved messages
        void set_max_log_len(std::vector<message>::size_type max_size);

        // Sets the size of the terminal
        void set_size(unsigned int x, unsigned int y) noexcept {
            set_width(x);
            set_height(y);
        }

        void set_size(ImVec2 sz) noexcept {
            assert(sz.x >= 0);
            assert(sz.y >= 0);
            set_size(static_cast<unsigned>(sz.x), static_cast<unsigned>(sz.y));
        }

        void set_width(unsigned int x) noexcept {
            m_base_width = x;
            m_update_width = true;
        }

        void set_height(unsigned int y) noexcept {
            m_base_height = y;
            m_update_height = true;
        }

        // Get last window size
        ImVec2 get_size() const noexcept {
            return m_current_size;
        }

        // Allows / disallow resize on a given axis
        void allow_x_resize() noexcept {
            m_allow_x_resize = true;
        }

        void allow_y_resize() noexcept {
            m_allow_y_resize = true;
        }
        void disallow_x_resize() noexcept {
            m_allow_x_resize = false;
        }

        void disallow_y_resize() noexcept {
            m_allow_y_resize = false;
        }
        void set_x_resize_allowance(bool allowed) noexcept {
            m_allow_x_resize = allowed;
        }

        void set_y_resize_allowance(bool allowed) noexcept {
            m_allow_y_resize = allowed;
        }

        // executes a statement, simulating user input
        // returns false if given string is too long to be interpreted
        // if true is returned, any text inputed by the user is overridden
        bool execute(std::string_view str) noexcept {
            if (str.size() <= m_command_buffer.size()) {
                std::copy(str.begin(), str.end(), m_command_buffer.begin());
                m_buffer_usage = str.size();
                call_command();
                m_buffer_usage = 0;
                m_command_buffer[0] = '\0';
                return true;
            }
            return false;
        }

    private:
        explicit terminal(value_type &arg_value, const char *window_name_, int base_width_, int base_height_, std::shared_ptr<TerminalHelper> th, terminal_helper_is_valid &&);

        void try_log(std::string_view str, message::type type);

        void compute_text_size() noexcept;

        void display_settings_bar(const std::vector<config_panels> &panels_order) noexcept;

        void display_messages() noexcept;

        void display_command_line() noexcept;

        // displaying command_line itself
        void show_input_text() noexcept;

        void handle_unfocus() noexcept;

        void show_autocomplete() noexcept;

        void call_command() noexcept;

        void push_message(message &&);

        std::optional<std::string> resolve_history_reference(std::string_view str, bool &modified) const noexcept;

        std::pair<bool, std::string> resolve_history_references(std::string_view str, bool &modified) const;


        static int command_line_callback_st(ImGuiInputTextCallbackData *data) noexcept;

        int command_line_callback(ImGuiInputTextCallbackData *data) noexcept;

        static int try_push_style(ImGuiCol col, const std::optional<ImVec4> &color) {
            if (color) {
                ImGui::PushStyleColor(col, *color);
                return 1;
            }
            return 0;
        }

        static int try_push_style(ImGuiCol col, const std::optional<theme::constexpr_color> &color) {
            if (color) {
                ImGui::PushStyleColor(col, color->imv4());
                return 1;
            }
            return 0;
        }


        int is_space(std::string_view str) const;

        bool is_digit(char c) const;

        unsigned long get_length(std::string_view str) const;

        // Returns a vector containing each element that were space separated
        // Returns an empty optional if a '"' char was not matched with a closing '"',
        //                except if ignore_non_match was set to true
        std::optional<std::vector<std::string>> split_by_space(std::string_view in, bool ignore_non_match = false) const;

        inline void try_lock() {
            while (m_flag.test_and_set(std::memory_order_seq_cst)) {}
        }

        inline void try_unlock() {
            m_flag.clear(std::memory_order_seq_cst);
        }

        ////////////

        value_type &m_argument_value;
        mutable std::shared_ptr<TerminalHelper> m_t_helper;

        bool m_should_show_next_frame{true};
        bool m_close_request{false};

        const char *const m_window_name;
        ImGuiWindowFlags m_flags{ImGuiWindowFlags_None};

        int m_base_width;
        int m_base_height;
        bool m_update_width{true};
        bool m_update_height{true};

        bool m_allow_x_resize{true};
        bool m_allow_y_resize{true};

        ImVec2 m_current_size{0.f, 0.f};

        struct theme m_colors
        {
        };

        // configuration
        bool m_autoscroll{true};// TODO: accessors
        bool m_autowrap{true};  // TODO: accessors
        std::vector<std::string>::size_type m_last_size{0u};
        int m_level{message::severity::trace};// TODO: accessors
#ifdef IMTERM_ENABLE_REGEX
        bool m_regex_search{true};// TODO: accessors, button
#endif

        std::optional<std::string> m_autoscroll_text;
        std::optional<std::string> m_clear_text;
        std::optional<std::string> m_log_level_text;
        std::optional<std::string> m_autowrap_text;
        std::optional<std::string> m_filter_hint;
        std::string m_level_list_text{};
        const char *m_longest_log_level{nullptr};// points to the longest log level, in m_level_list_text
        const char *m_lowest_log_level{nullptr}; // points to the lowest log level possible, in m_level_list_text
        message::severity::severity_t m_lowest_log_level_val{message::severity::trace};

        small_buffer_type m_log_text_filter_buffer{};
        small_buffer_type::size_type m_log_text_filter_buffer_usage{0u};


        // message panel variables
        unsigned long m_last_flush_at_history{0u};// for the [-n] indicator on command line
        bool m_flush_bit{false};
        std::vector<message> m_logs{};
        std::vector<message>::size_type m_max_log_len{5'000};// TODO: command
        std::vector<message>::size_type m_current_log_oldest_idx{0};


        // command line variables
        buffer_type m_command_buffer{};
        buffer_type::size_type m_buffer_usage{0u};// max accessible: command_buffer[buffer_usage - 1]
                                                  // (buffer_usage might be 0 for empty string)
        buffer_type::size_type m_previous_buffer_usage{0u};
        bool m_should_take_focus{false};

        ImGuiID m_previously_active_id{0u};
        ImGuiID m_input_text_id{0u};

        // autocompletion
        std::vector<command_type_cref> m_current_autocomplete{};
        std::vector<std::string> m_current_autocomplete_strings{};
        std::string_view m_autocomlete_separator{" | "};
        position m_autocomplete_pos{position::down};
        bool m_command_entered{false};

        // command line: completion using history
        std::string m_command_line_backup{};
        std::string_view m_command_line_backup_prefix{};
        std::vector<std::string> m_command_history{};
        std::optional<std::vector<std::string>::iterator> m_current_history_selection{};

        bool m_ignore_next_textinput{false};
        bool m_has_focus{false};

        std::atomic_flag m_flag;
    };
}// namespace ImTerm


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <imgui.h>
#include <imgui_internal.h>
#include <iterator>
#include <map>
#include <optional>
#ifdef IMTERM_ENABLE_REGEX
#include <regex>
#endif

namespace ImTerm {
    namespace details {
        template<typename T>
        using is_space_method = decltype(std::declval<T &>().is_space(std::declval<std::string_view>()));

        template<typename TerminalHelper>
        std::enable_if_t<misc::is_detected_v<is_space_method, TerminalHelper>, int> constexpr is_space(std::shared_ptr<TerminalHelper> &t_h, std::string_view str) {
            static_assert(std::is_same_v<decltype(t_h->is_space(str)), int>, "TerminalHelper::is_space(std::string_view) should return an int");
            return t_h->is_space(str);
        }

        template<typename TerminalHelper>
        std::enable_if_t<!misc::is_detected_v<is_space_method, TerminalHelper>, int> constexpr is_space(std::shared_ptr<TerminalHelper> &, std::string_view str) {
            return str[0] == ' ' ? 1 : 0;
        }

        template<typename T>
        using get_length_method = decltype(std::declval<T &>().get_length(std::declval<std::string_view>()));

        template<typename TerminalHelper>
        std::enable_if_t<misc::is_detected_v<get_length_method, TerminalHelper>, unsigned long> constexpr get_length(std::shared_ptr<TerminalHelper> &t_h, std::string_view str) {
            static_assert(std::is_same_v<decltype(t_h->get_length(str)), int>, "TerminalHelper::get_length(std::string_view) should return an int");
            return t_h->get_length(str);
        }

        template<typename TerminalHelper>
        std::enable_if_t<!misc::is_detected_v<get_length_method, TerminalHelper>, unsigned long> constexpr get_length(std::shared_ptr<TerminalHelper> &, std::string_view str) {
            return str.size();
        }

        template<typename T>
        using set_terminal_method = decltype(std::declval<T &>().set_terminal(std::declval<terminal<T> &>()));

        template<typename TerminalHelper>
        std::enable_if_t<misc::is_detected_v<set_terminal_method, TerminalHelper>>
        assign_terminal(TerminalHelper &helper, terminal<TerminalHelper> &terminal) {
            helper.set_terminal(terminal);
        }

        template<typename TerminalHelper>
        std::enable_if_t<!misc::is_detected_v<set_terminal_method, TerminalHelper>>
        assign_terminal(TerminalHelper &helper, terminal<TerminalHelper> &terminal) {}

        // simple as in "non regex"
        inline std::map<std::string::const_iterator, std::pair<unsigned long, std::optional<theme::constexpr_color>>>
        simple_colors_split(std::string_view filter, const message &msg, const std::optional<theme::constexpr_color> &matching_text_color) {

            std::map<std::string::const_iterator, std::pair<unsigned long, std::optional<theme::constexpr_color>>> colors;
            if (filter.empty()) {
                colors.emplace(msg.value.cbegin() + msg.color_end, std::pair{msg.value.size() - msg.color_end, std::optional<theme::constexpr_color>{}});
                colors.emplace(msg.value.cbegin() + msg.color_beg, std::pair{msg.color_end - msg.color_beg, std::optional<theme::constexpr_color>{}});
                colors.emplace(msg.value.cbegin(), std::pair{msg.color_beg, std::optional<theme::constexpr_color>{}});
                return colors;
            }

            auto it = std::search(msg.value.cbegin(), msg.value.cend(), filter.begin(), filter.end());
            if (it == msg.value.end()) {
                return colors;
            }

            auto distance = static_cast<unsigned long>(std::distance(msg.value.cbegin(), it));
            if (distance > msg.color_beg) {
                colors.emplace(msg.value.cbegin(), std::pair{msg.color_beg, std::optional<theme::constexpr_color>{}});
                if (distance > msg.color_end) {
                    colors.emplace(msg.value.cbegin() + msg.color_beg, std::pair{msg.color_end - msg.color_beg, std::optional<theme::constexpr_color>{}});
                    colors.emplace(msg.value.cbegin() + msg.color_end, std::pair{distance - msg.color_end, std::optional<theme::constexpr_color>{}});
                } else {
                    colors.emplace(msg.value.cbegin() + msg.color_beg, std::pair{distance - msg.color_beg, std::optional<theme::constexpr_color>{}});
                }
            } else {
                colors.emplace(msg.value.cbegin(), std::pair{distance, std::optional<theme::constexpr_color>{}});
                colors.emplace(msg.value.cbegin() + msg.color_beg, std::pair{0, std::optional<theme::constexpr_color>{}});
            }
            colors.emplace(msg.value.cbegin() + msg.color_end, std::pair{0, std::optional<theme::constexpr_color>{}});

            std::string::const_iterator last_valid;
            do {
                colors[it] = std::pair{filter.size(), matching_text_color};
                last_valid = it + filter.size();
                it = std::search(last_valid, msg.value.cend(), filter.begin(), filter.end());

                distance = static_cast<unsigned long>(std::distance(last_valid, it));
                if (last_valid < msg.value.cbegin() + msg.color_beg && last_valid + distance > msg.value.cbegin() + msg.color_beg) {

                    auto mid_point = static_cast<unsigned long>(msg.color_beg + msg.value.cbegin() - last_valid);
                    colors[last_valid] = std::pair{mid_point, std::optional<theme::constexpr_color>{}};

                    if (last_valid + distance < msg.value.cbegin() + msg.color_end) {
                        colors[last_valid + mid_point] = std::pair{distance - mid_point, std::optional<theme::constexpr_color>{}};
                    } else {
                        auto len = msg.color_end - msg.color_beg;
                        colors[last_valid + mid_point] = std::pair{len, std::optional<theme::constexpr_color>{}};
                        colors[last_valid + mid_point + len] = std::pair{distance - mid_point - len, std::optional<theme::constexpr_color>{}};
                    }

                } else if (last_valid < msg.value.cbegin() + msg.color_end && last_valid + distance > msg.value.cbegin() + msg.color_end) {
                    auto mid_point = static_cast<unsigned long>(msg.color_end + msg.value.cbegin() - last_valid);
                    colors[last_valid] = std::pair{mid_point, std::optional<theme::constexpr_color>{}};
                    colors[last_valid + mid_point] = std::pair{distance - mid_point, std::optional<theme::constexpr_color>{}};

                } else {
                    colors[last_valid] = std::pair{distance, std::optional<theme::constexpr_color>{}};
                }

            } while (it != msg.value.cend());
            return colors;
        }

#ifdef IMTERM_ENABLE_REGEX
        inline std::map<std::string::const_iterator, std::pair<unsigned long, std::optional<theme::constexpr_color>>>
        regex_colors_split(std::string_view filter, const message &msg, const std::optional<theme::constexpr_color> &matching_text_color) {
            auto make_pair = [](auto len, std::optional<theme::constexpr_color> color = {}) {
                return std::pair{static_cast<unsigned long>(len), color};
            };

            std::map<std::string::const_iterator, std::pair<unsigned long, std::optional<theme::constexpr_color>>> colors;
            if (filter.empty()) {
                colors.emplace(msg.value.cbegin() + msg.color_end, make_pair(msg.value.size() - msg.color_end));
                colors.emplace(msg.value.cbegin() + msg.color_beg, make_pair(msg.color_end - msg.color_beg));
                colors.emplace(msg.value.cbegin(), make_pair(msg.color_beg));
                return colors;
            }

            std::smatch matches;
            std::regex_search(msg.value, matches, std::regex(filter.begin(), filter.end()));

            if (matches.empty()) {
                return colors;
            }

            const auto &match = matches[0];
            auto match_len = std::distance(match.first, match.second);
            colors.emplace(match.first, make_pair(match_len, matching_text_color));

            auto match_begin = static_cast<size_t>(std::distance(msg.value.cbegin(), match.first));
            auto match_end = match_begin + match_len;
            if (match_begin > msg.color_beg) {
                if (match_begin > msg.color_end) {
                    colors.emplace(match.second, make_pair(std::distance(match.second, msg.value.cend())));
                    colors.emplace(msg.value.cbegin() + msg.color_end, make_pair(match_begin - msg.color_end));
                    colors.emplace(msg.value.cbegin() + msg.color_beg, make_pair(msg.color_end - msg.color_beg));
                } else {
                    if (match_end > msg.color_end) {
                        colors.emplace(match.second, make_pair(std::distance(match.second, msg.value.cend())));
                        colors.emplace(msg.value.cbegin() + msg.color_end, make_pair(0u));
                        colors.emplace(msg.value.cbegin() + msg.color_beg, make_pair(match_begin - msg.color_beg));
                    } else {
                        colors.emplace(msg.value.cbegin() + msg.color_end, make_pair(std::distance(msg.value.begin() + msg.color_end, msg.value.end())));
                        colors.emplace(match.second, make_pair(std::distance(match.second, msg.value.cbegin() + msg.color_end)));
                        colors.emplace(msg.value.cbegin() + msg.color_beg, make_pair(match_begin - msg.color_beg));
                    }
                }
                colors.emplace(msg.value.cbegin(), make_pair(msg.color_beg));
            } else {
                if (match_end > msg.color_beg) {
                    if (match_end > msg.color_end) {
                        colors.emplace(msg.value.cbegin() + match_end, make_pair(msg.value.size() - match_end));
                        colors.emplace(msg.value.cbegin(), make_pair(match_begin));
                        colors.emplace(msg.value.cbegin() + msg.color_end, make_pair(0));
                    } else {
                        colors.emplace(msg.value.cbegin() + msg.color_end, make_pair(msg.value.size() - msg.color_end));
                        colors.emplace(msg.value.cbegin() + match_end, make_pair(msg.color_end - match_end));
                        colors.emplace(msg.value.cbegin() + msg.color_beg, make_pair(0));
                        colors.emplace(msg.value.cbegin(), make_pair(match_begin));
                    }
                } else {
                    colors.emplace(msg.value.cbegin() + msg.color_end, make_pair(msg.value.size() - msg.color_end));
                    colors.emplace(msg.value.cbegin() + msg.color_beg, make_pair(msg.color_end - msg.color_beg));
                    colors.emplace(match.second, make_pair(msg.color_beg - match_end));
                    colors.emplace(msg.value.cbegin(), make_pair(match_begin));
                }
            }
            return colors;
        }
#endif
    }// namespace details

    template<typename TerminalHelper>
    terminal<TerminalHelper>::terminal(value_type &arg_value, const char *window_name_, int base_width_, int base_height_, std::shared_ptr<TerminalHelper> th, terminal_helper_is_valid && /*unused*/)
        : m_argument_value{arg_value}, m_t_helper{std::move(th)}, m_window_name(window_name_), m_base_width(base_width_), m_base_height(base_height_), m_autoscroll_text{"自动滚动"}, m_clear_text{"清空"}, m_log_level_text{"日志等级"}, m_autowrap_text{"自动换行"}, m_filter_hint{"过滤..."} {
        m_flag.clear();
        assert(m_t_helper != nullptr);
        details::assign_terminal(*m_t_helper, *this);

        std::fill(m_command_buffer.begin(), m_command_buffer.end(), '\0');
        std::fill(m_log_text_filter_buffer.begin(), m_log_text_filter_buffer.end(), '\0');
        set_level_list_text("trace", "debug", "info", "warning", "error", "critical", "none");
    }

    template<typename TerminalHelper>
    bool terminal<TerminalHelper>::show(const std::vector<config_panels> &panels_order) noexcept {
        if (m_flush_bit) {
            m_last_flush_at_history = m_command_history.size();
            m_flush_bit = false;
        }

        m_should_show_next_frame = !m_close_request;
        m_close_request = false;

        if (m_update_height) {
            if (m_update_width) {
                ImGui::SetNextWindowSizeConstraints({static_cast<float>(m_base_width), static_cast<float>(m_base_height)},
                                                    {static_cast<float>(m_base_width), static_cast<float>(m_base_height)});
                m_update_width = false;
            } else {
                ImGui::SetNextWindowSizeConstraints({-1.f, static_cast<float>(m_base_height)}, {-1.f, static_cast<float>(m_base_height)});
            }
            m_update_height = false;
        } else if (m_update_width) {
            ImGui::SetNextWindowSizeConstraints({static_cast<float>(m_base_width), -1.f}, {static_cast<float>(m_base_width), -1.f});
            m_update_width = false;
        } else {
            if (!m_allow_x_resize) {
                if (!m_allow_y_resize) {
                    ImGui::SetNextWindowSizeConstraints(m_current_size, m_current_size);
                } else {
                    ImGui::SetNextWindowSizeConstraints({m_current_size.x, 0.f}, {m_current_size.x, std::numeric_limits<float>::infinity()});
                }
            } else if (!m_allow_y_resize) {
                ImGui::SetNextWindowSizeConstraints({0.f, m_current_size.y}, {std::numeric_limits<float>::infinity(), m_current_size.y});
            }
        }

        int pop_count = 0;
        pop_count += try_push_style(ImGuiCol_Text, m_colors.text);
        pop_count += try_push_style(ImGuiCol_WindowBg, m_colors.window_bg);
        pop_count += try_push_style(ImGuiCol_ChildBg, m_colors.window_bg);
        pop_count += try_push_style(ImGuiCol_Border, m_colors.border);
        pop_count += try_push_style(ImGuiCol_BorderShadow, m_colors.border_shadow);
        pop_count += try_push_style(ImGuiCol_Button, m_colors.button);
        pop_count += try_push_style(ImGuiCol_ButtonHovered, m_colors.button_hovered);
        pop_count += try_push_style(ImGuiCol_ButtonActive, m_colors.button_active);
        pop_count += try_push_style(ImGuiCol_FrameBg, m_colors.frame_bg);
        pop_count += try_push_style(ImGuiCol_FrameBgHovered, m_colors.frame_bg_hovered);
        pop_count += try_push_style(ImGuiCol_FrameBgActive, m_colors.frame_bg_active);
        pop_count += try_push_style(ImGuiCol_TextSelectedBg, m_colors.text_selected_bg);
        pop_count += try_push_style(ImGuiCol_CheckMark, m_colors.check_mark);
        pop_count += try_push_style(ImGuiCol_TitleBg, m_colors.title_bg);
        pop_count += try_push_style(ImGuiCol_TitleBgActive, m_colors.title_bg_active);
        pop_count += try_push_style(ImGuiCol_TitleBgCollapsed, m_colors.title_bg_collapsed);
        pop_count += try_push_style(ImGuiCol_Header, m_colors.log_level_selected);
        pop_count += try_push_style(ImGuiCol_HeaderActive, m_colors.log_level_active);
        pop_count += try_push_style(ImGuiCol_HeaderHovered, m_colors.log_level_hovered);
        pop_count += try_push_style(ImGuiCol_PopupBg, m_colors.log_level_drop_down_list_bg);
        pop_count += try_push_style(ImGuiCol_ScrollbarBg, m_colors.scrollbar_bg);
        pop_count += try_push_style(ImGuiCol_ScrollbarGrab, m_colors.scrollbar_grab);
        pop_count += try_push_style(ImGuiCol_ScrollbarGrabActive, m_colors.scrollbar_grab_active);
        pop_count += try_push_style(ImGuiCol_ScrollbarGrabHovered, m_colors.scrollbar_grab_hovered);

        if (m_has_focus) {
            ImGui::PushStyleColor(ImGuiCol_TitleBg, ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive));
            ++pop_count;
            m_has_focus = false;
        }

        if (!ImGui::Begin(m_window_name, nullptr, ImGuiWindowFlags_NoScrollbar | m_flags)) {
            ImGui::End();
            ImGui::PopStyleColor(pop_count);
            return true;
        }
        m_current_size = ImGui::GetWindowSize();

        display_settings_bar(panels_order);
        try_lock();
        display_messages();
        try_unlock();
        display_command_line();

        ImGui::End();
        ImGui::PopStyleColor(pop_count);

        return m_should_show_next_frame;
    }

    template<typename TerminalHelper>
    void terminal<TerminalHelper>::reset_colors() noexcept {
        for (std::optional<theme::constexpr_color> &color: m_colors.log_level_colors) {
            color.reset();
        }
        m_colors.text.reset();
        m_colors.window_bg.reset();
        m_colors.border.reset();
        m_colors.border_shadow.reset();
        m_colors.button.reset();
        m_colors.button_hovered.reset();
        m_colors.button_active.reset();
        m_colors.frame_bg.reset();
        m_colors.frame_bg_hovered.reset();
        m_colors.frame_bg_active.reset();
        m_colors.text_selected_bg.reset();
        m_colors.check_mark.reset();
        m_colors.title_bg.reset();
        m_colors.title_bg_active.reset();
        m_colors.title_bg_collapsed.reset();
        m_colors.message_panel.reset();
        m_colors.auto_complete_selected.reset();
        m_colors.auto_complete_non_selected.reset();
        m_colors.auto_complete_separator.reset();
        m_colors.auto_complete_selected.reset();
        m_colors.auto_complete_non_selected.reset();
        m_colors.auto_complete_separator.reset();
        m_colors.cmd_backlog.reset();
        m_colors.cmd_history_completed.reset();
        m_colors.log_level_drop_down_list_bg.reset();
        m_colors.log_level_active.reset();
        m_colors.log_level_hovered.reset();
        m_colors.log_level_selected.reset();
        m_colors.scrollbar_bg.reset();
        m_colors.scrollbar_grab.reset();
        m_colors.scrollbar_grab_active.reset();
        m_colors.scrollbar_grab_hovered.reset();
    }

    template<typename TerminalHelper>
    void terminal<TerminalHelper>::add_text(std::string str, unsigned int color_beg, unsigned int color_end) {
        message msg;
        msg.is_term_message = true;
        msg.severity = message::severity::info;
        msg.color_beg = color_beg;
        msg.color_end = color_end;
        msg.value = std::move(str);
        push_message(std::move(msg));
    }

    template<typename TerminalHelper>
    void terminal<TerminalHelper>::add_text_err(std::string str, unsigned int color_beg, unsigned int color_end) {
        message msg;
        msg.is_term_message = true;
        msg.severity = message::severity::warn;
        msg.color_beg = color_beg;
        msg.color_end = color_end;
        msg.value = std::move(str);
        push_message(std::move(msg));
    }

    template<typename TerminalHelper>
    void terminal<TerminalHelper>::add_message(message &&msg) {
        if (msg.is_term_message && msg.severity != message::severity::warn) {
            msg.severity = message::severity::info;
        }
        push_message(std::move(msg));
    }

    template<typename TerminalHelper>
    void terminal<TerminalHelper>::clear() {
        m_flush_bit = true;
        try_lock();
        m_logs.clear();
        m_current_log_oldest_idx = 0u;
        try_unlock();
    }

    template<typename TerminalHelper>
    void terminal<TerminalHelper>::set_level_list_text(std::string_view trace_str, std::string_view debug_str, std::string_view info_str, std::string_view warn_str, std::string_view err_str, std::string_view critical_str, std::string_view none_str) {

        m_level_list_text.clear();
        m_level_list_text.reserve(
                trace_str.size() + 1 + debug_str.size() + 1 + info_str.size() + 1 + warn_str.size() + 1 + err_str.size() + 1 + critical_str.size() + 1 + 1);

        const std::string_view *const levels[] = {&trace_str, &debug_str, &info_str, &warn_str, &err_str, &critical_str, &none_str};

        for (const std::string_view *const lvl: levels) {
            std::copy(lvl->begin(), lvl->end(), std::back_inserter(m_level_list_text));
            m_level_list_text.push_back('\0');
        }
        m_level_list_text.push_back('\0');

        set_min_log_level(m_lowest_log_level_val);
    }


    template<typename TerminalHelper>
    void terminal<TerminalHelper>::set_min_log_level(message::severity::severity_t level) {
        m_level = m_level + m_lowest_log_level_val - level;

        m_lowest_log_level_val = level;
        m_lowest_log_level = m_level_list_text.data();
        for (int i = level; i > 0; --i) {
            m_lowest_log_level += std::strlen(m_lowest_log_level) + 1;
        }

        m_longest_log_level = "";
        std::size_t longest_len = 0u;
        const char *current_str = m_lowest_log_level;

        for (int i = level; i < message::severity::critical + 2; ++i) {
            auto length = std::strlen(current_str);
            auto regular_len = get_length({current_str, length});
            if (regular_len > longest_len) {
                longest_len = regular_len;
                m_longest_log_level = current_str;
            }
            current_str += length + 1;
        }
    }

    template<typename TerminalHelper>
    void terminal<TerminalHelper>::set_max_log_len(std::vector<message>::size_type max_size) {
        try_lock();
        std::vector<message> new_msg_vect;
        new_msg_vect.reserve(max_size);
        for (auto i = 0u; i < std::min(max_size, m_logs.size() - m_current_log_oldest_idx); ++i) {
            new_msg_vect.emplace_back(std::move(m_logs[i + m_current_log_oldest_idx]));
        }
        for (auto i = 0u; i < std::min(max_size - new_msg_vect.size(), m_logs.size() - new_msg_vect.size()); ++i) {
            new_msg_vect.emplace_back(std::move(m_logs[i]));
        }
        m_logs = std::move(new_msg_vect);
        m_current_log_oldest_idx = 0u;
        m_max_log_len = max_size;
        try_unlock();
    }

    template<typename TerminalHelper>
    void terminal<TerminalHelper>::try_log(std::string_view str, message::type type) {
        message::severity::severity_t severity;
        switch (type) {
            case message::type::user_input:
                severity = message::severity::trace;
                break;
            case message::type::error:
                severity = message::severity::err;
                break;
            case message::type::cmd_history_completion:
                severity = message::severity::debug;
                break;
        }
        std::optional<message> msg = m_t_helper->format({str.data(), str.size()}, type);
        if (msg) {
            msg->is_term_message = true;
            msg->severity = severity;
            push_message(std::move(*msg));
        }
    }

    template<typename TerminalHelper>
    void terminal<TerminalHelper>::display_settings_bar(const std::vector<config_panels> &panels_order) noexcept {
        if (panels_order.empty()) {
            return;
        }
        if (!m_autoscroll_text && !m_autowrap_text && !m_clear_text && !m_filter_hint && !m_log_level_text) {
            return;
        }

        const float autoscroll_size = !m_autoscroll_text ? 0.f : ImGui::CalcTextSize(m_autoscroll_text->data()).x + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x;

        const float autowrap_size = !m_autowrap_text ? 0.f : ImGui::CalcTextSize(m_autowrap_text->data()).x + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x;

        const float clearbutton_size = !m_clear_text ? 0.f : ImGui::CalcTextSize(m_clear_text->data()).x + ImGui::GetStyle().FramePadding.x * 2.f;

        const float filter_size = !m_filter_hint ? 0.f : ImGui::CalcTextSize(m_filter_hint->data()).x + ImGui::GetStyle().FramePadding.x * 2.f;

        const float loglevel_selector_size = !m_log_level_text ? 0.f : ImGui::CalcTextSize(m_longest_log_level).x + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x * 2.f;

        const float loglevel_global_size = !m_log_level_text ? 0.f : ImGui::CalcTextSize(m_log_level_text->data()).x + ImGui::GetStyle().ItemSpacing.x + loglevel_selector_size;

        unsigned space_consumer_count = 0u;
        float required_space = ImGui::GetStyle().ItemSpacing.x * (panels_order.size() - 1);
        for (config_panels panel: panels_order) {
            switch (panel) {
                case config_panels::autoscroll:
                    required_space += autoscroll_size;
                    break;
                case config_panels::autowrap:
                    required_space += autowrap_size;
                    break;
                case config_panels::clearbutton:
                    required_space += clearbutton_size;
                    break;
                case config_panels::filter:
                    required_space += filter_size;
                    break;
                case config_panels::loglevel:
                    required_space += loglevel_global_size;
                    break;
                case config_panels::long_filter:
                    [[fallthrough]];
                case config_panels::blank:
                    ++space_consumer_count;
                    break;
                default:
                    break;
            }
        }

        float consumer_width = std::max((ImGui::GetContentRegionAvail().x - required_space) / static_cast<float>(space_consumer_count), 0.1f);

        bool same_line_req{false};
        auto same_line = [&same_line_req]() {
            if (same_line_req) {
                ImGui::SameLine();
            }
            same_line_req = true;
        };

        auto show_filter = [this](float size) {
            if (m_filter_hint) {

                int pop_count = try_push_style(ImGuiCol_TextDisabled, m_colors.filter_hint);
                pop_count += try_push_style(ImGuiCol_Text, m_colors.filter_text);

                ImGui::PushItemWidth(size);
                if (ImGui::InputTextWithHint("##terminal:settings:text_filter", m_filter_hint->data(), m_log_text_filter_buffer.data(), m_log_text_filter_buffer.size())) {
                    m_log_text_filter_buffer_usage = misc::strnlen(m_log_text_filter_buffer.data(), m_log_text_filter_buffer.size());
                }
                ImGui::PopItemWidth();

                ImGui::PopStyleColor(pop_count);
            } else {
                ImGui::Dummy(ImVec2(size, 1.f));
            }
        };

        for (unsigned int i = 0; i < panels_order.size(); ++i) {
            ImGui::PushID(i);
            switch (panels_order[i]) {
                case config_panels::autoscroll:
                    if (m_autoscroll_text) {
                        ImGui::Checkbox(m_autoscroll_text->data(), &m_autoscroll);
                    } else {
                        ImGui::Dummy(ImVec2(autoscroll_size, 1.f));
                    }
                    break;
                case config_panels::autowrap:
                    if (m_autowrap_text) {
                        ImGui::Checkbox(m_autowrap_text->data(), &m_autowrap);
                    } else {
                        ImGui::Dummy(ImVec2(autowrap_size, 1.f));
                    }
                    break;
                case config_panels::blank:
                    ImGui::Dummy(ImVec2(consumer_width, 1.f));
                    break;
                case config_panels::clearbutton:
                    if (m_clear_text) {
                        if (ImGui::Button(m_clear_text->data())) {
                            clear();
                        }
                    } else {
                        ImGui::Dummy(ImVec2(clearbutton_size, 1.f));
                    }
                    break;
                case config_panels::filter:
                    show_filter(filter_size);
                    break;
                case config_panels::long_filter:
                    show_filter(consumer_width);
                    break;
                case config_panels::loglevel:
                    if (m_log_level_text) {
                        ImGui::TextUnformatted(m_log_level_text->data(), m_log_level_text->data() + m_log_level_text->size());

                        ImGui::SameLine();
                        ImGui::PushItemWidth(loglevel_selector_size);
                        ImGui::Combo("##terminal:log_level_selector:combo", &m_level, m_lowest_log_level);
                        ImGui::PopItemWidth();
                    } else {
                        ImGui::Dummy(ImVec2(loglevel_global_size, 1.f));
                    }
                    break;
                default:
                    break;
            }
            ImGui::PopID();
            ImGui::SameLine();
        }
        ImGui::NewLine();
    }

    template<typename TerminalHelper>
    void terminal<TerminalHelper>::display_messages() noexcept {

        ImVec2 avail_space = ImGui::GetContentRegionAvail();
        float commandline_height = ImGui::CalcTextSize("a").y + ImGui::GetStyle().FramePadding.y * 4.f;
        if (avail_space.y > commandline_height) {

            int style_push_count = try_push_style(ImGuiCol_ChildBg, m_colors.message_panel);
            if (ImGui::BeginChild("terminal:logs_window", ImVec2(avail_space.x, avail_space.y - commandline_height), false,
                                  ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoTitleBar)) {

                unsigned traced_count = 0;
                void (*text_formatted)(const char *, ...);
                if (m_autowrap) {
                    text_formatted = ImGui::TextWrapped;
                } else {
                    text_formatted = ImGui::Text;
                }

                auto print_single_message = [this, &traced_count, &text_formatted](const message &msg) {
                    if (msg.severity < (m_level + m_lowest_log_level_val) && !msg.is_term_message) {
                        return;
                    }

                    if (msg.value.empty()) {
                        ImGui::NewLine();
                        return;
                    }

                    std::map<std::string::const_iterator, std::pair<unsigned long, std::optional<theme::constexpr_color>>> colors;
#ifdef IMTERM_ENABLE_REGEX
                    if (m_regex_search) {
                        try {
                            std::string_view filter{m_log_text_filter_buffer.data(), m_log_text_filter_buffer_usage};
                            colors = details::regex_colors_split(filter, msg, m_colors.matching_text);
                        } catch (const std::regex_error &) {
                            return;// malformed regex is treated as no match
                        }
                    } else {
                        std::string_view filter{m_log_text_filter_buffer.data(), m_log_text_filter_buffer_usage};
                        colors = details::simple_colors_split(filter, msg, m_colors.matching_text);
                    }
#else
                    std::string_view filter{m_log_text_filter_buffer.data(), m_log_text_filter_buffer_usage};
                    colors = details::simple_colors_split(filter, msg, m_colors.matching_text);
#endif
                    if (colors.empty()) {
                        return;
                    }

                    unsigned int msg_col_pop = 0u;
                    for (const auto &color: colors) {
                        if (color.first == msg.value.begin() + msg.color_beg) {
                            if (msg.is_term_message) {
                                if (msg.severity == message::severity::trace) {
                                    msg_col_pop += try_push_style(ImGuiCol_Text, m_colors.cmd_backlog);
                                    text_formatted("[%d] ", static_cast<int>(traced_count + m_last_flush_at_history - m_command_history.size()));
                                    ++traced_count;
                                    ImGui::SameLine(0.f, 0.f);
                                } else if (msg.severity == message::severity::debug) {
                                    msg_col_pop += try_push_style(ImGuiCol_Text, m_colors.cmd_history_completed);
                                } else {
                                    msg_col_pop += try_push_style(ImGuiCol_Text, m_colors.log_level_colors[msg.severity]);
                                }
                            } else {
                                msg_col_pop += try_push_style(ImGuiCol_Text, m_colors.log_level_colors[msg.severity]);
                            }
                        }
                        if (color.first == msg.value.begin() + msg.color_end) {
                            ImGui::PopStyleColor(msg_col_pop);
                            msg_col_pop = 0u;
                        }
                        if (color.second.first != 0) {
                            const int pop = try_push_style(ImGuiCol_Text, color.second.second);
                            text_formatted("%.*s", color.second.first, &*color.first);
                            ImGui::PopStyleColor(pop);
                            ImGui::SameLine(0.f, 0.f);
                        }
                    }
                    ImGui::PopStyleColor(msg_col_pop);
                    ImGui::NewLine();
                };
                if (m_current_log_oldest_idx < m_logs.size()) {
                    for (size_t i = m_current_log_oldest_idx; i < m_logs.size(); ++i) {
                        print_single_message(m_logs[i]);
                    }
                    for (size_t i = 0u; i < m_current_log_oldest_idx; ++i) {
                        print_single_message(m_logs[i]);
                    }
                }
            }
            if (m_autoscroll) {
                if (m_last_size != m_logs.size()) {
                    ImGui::SetScrollHereY(1.f);
                    m_last_size = m_logs.size();
                }
            } else {
                m_last_size = 0u;
            }
            ImGui::PopStyleColor(style_push_count);
            ImGui::EndChild();
        }
    }

    template<typename TerminalHelper>
    void terminal<TerminalHelper>::display_command_line() noexcept {
        if (!m_command_entered && ImGui::GetActiveID() == m_input_text_id && m_input_text_id != 0 && m_current_autocomplete.empty()) {
            if (m_autocomplete_pos != position::nowhere && m_buffer_usage == 0u && m_current_autocomplete_strings.empty()) {
                m_current_autocomplete = m_t_helper->list_commands();
            }
        }

        ImGui::Separator();
        show_input_text();
        handle_unfocus();
        show_autocomplete();
    }

    template<typename TerminalHelper>
    void terminal<TerminalHelper>::show_input_text() noexcept {
        ImGui::PushItemWidth(-1.f);
        if (m_should_take_focus) {
            ImGui::SetKeyboardFocusHere();
            m_should_take_focus = false;
        }
        m_previous_buffer_usage = m_buffer_usage;

        if (ImGui::InputText("##terminal:input_text", m_command_buffer.data(), m_command_buffer.size(),
                             ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory,
                             terminal::command_line_callback_st, this) &&
            !m_ignore_next_textinput) {
            m_current_history_selection = {};
            if (m_buffer_usage > 0u && m_command_buffer[m_buffer_usage - 1] == '\0') {
                --m_buffer_usage;
            } else if (m_buffer_usage + 1 < m_command_buffer.size() && m_command_buffer[m_buffer_usage + 1] == '\0' && m_command_buffer[m_buffer_usage] != '\0') {
                ++m_buffer_usage;
            } else {
                m_buffer_usage = misc::strnlen(m_command_buffer.data(), m_command_buffer.size());
            }

            if (m_autocomplete_pos != position::nowhere) {

                int sp_count = 0;
                auto is_space_lbd = [this, &sp_count](char c) {
                    if (sp_count > 0) {
                        --sp_count;
                        return true;
                    } else {
                        sp_count = is_space({&c, static_cast<unsigned>(m_command_buffer.data() + m_buffer_usage - &c)});
                        if (sp_count > 0) {
                            --sp_count;
                            return true;
                        }
                        return false;
                    }
                };
                char *beg = std::find_if_not(m_command_buffer.data(), m_command_buffer.data() + m_buffer_usage,
                                             is_space_lbd);
                sp_count = 0;
                const char *ed = std::find_if(beg, m_command_buffer.data() + m_buffer_usage, is_space_lbd);

                if (ed == m_command_buffer.data() + m_buffer_usage) {
                    m_current_autocomplete = m_t_helper->find_commands_by_prefix(beg, ed);
                    m_current_autocomplete_strings.clear();
                    m_command_entered = true;
                } else {
                    m_command_entered = false;
                    m_current_autocomplete.clear();
                    std::vector<command_type_cref> cmds = m_t_helper->find_commands_by_prefix(beg, ed);

                    if (!cmds.empty()) {
                        std::string_view sv{m_command_buffer.data(), m_buffer_usage};
                        std::optional<std::vector<std::string>> splitted = split_by_space(sv, true);
                        assert(splitted);
                        argument_type arg{m_argument_value, *this, *splitted};
                        m_current_autocomplete_strings = cmds[0].get().complete(arg);
                    }
                }
            } else {
                m_command_entered = false;
            }
        }
        m_ignore_next_textinput = false;
        ImGui::PopItemWidth();

        if (m_input_text_id == 0u) {
            m_input_text_id = ImGui::GetItemID();
        }
    }

    template<typename TerminalHelper>
    void terminal<TerminalHelper>::handle_unfocus() noexcept {
        auto clear_frame = [this]() {
            m_command_buffer[0] = '\0';
            m_buffer_usage = 0u;
            m_command_line_backup_prefix.remove_prefix(m_command_line_backup_prefix.size());
            m_command_line_backup.clear();
            m_current_history_selection = {};
            m_current_autocomplete.clear();
        };

        if (m_previously_active_id == m_input_text_id && ImGui::GetActiveID() != m_input_text_id) {
            if (ImGui::IsKeyPressedMap(ImGuiKey_Enter)) {
                call_command();
                m_should_take_focus = true;
                clear_frame();

            } else if (ImGui::IsKeyPressedMap(ImGuiKey_Escape)) {
                if (m_buffer_usage == 0u && m_previous_buffer_usage == 0u) {
                    m_should_show_next_frame = false;// should hide on next frames
                } else {
                    m_should_take_focus = true;
                }
                clear_frame();
            }
        }
        m_previously_active_id = ImGui::GetActiveID();
    }

    template<typename TerminalHelper>
    void terminal<TerminalHelper>::show_autocomplete() noexcept {
        constexpr ImGuiWindowFlags overlay_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        if (m_autocomplete_pos == position::nowhere) {
            return;
        }

        if ((m_input_text_id == ImGui::GetActiveID() || m_should_take_focus) && (!m_current_autocomplete.empty() || !m_current_autocomplete_strings.empty())) {
            m_has_focus = true;

            ImGui::SetNextWindowBgAlpha(0.9f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::SetNextWindowFocus();

            ImVec2 auto_complete_pos = ImGui::GetItemRectMin();

            if (m_autocomplete_pos == position::up) {
                auto_complete_pos.y -= (ImGui::CalcTextSize("a").y + ImGui::GetStyle().FramePadding.y) * 2.f;
            } else {
                auto_complete_pos.y = ImGui::GetItemRectMax().y;
            }

            ImVec2 auto_complete_max_size = ImGui::GetItemRectSize();
            auto_complete_max_size.y = -1.f;
            ImGui::SetNextWindowPos(auto_complete_pos);
            ImGui::SetNextWindowSizeConstraints({0.f, 0.f}, auto_complete_max_size);
            if (ImGui::Begin("##terminal:auto_complete", nullptr, overlay_flags)) {
                ImGui::SetActiveID(m_input_text_id, ImGui::GetCurrentWindow());

                auto print_separator = [this]() {
                    ImGui::SameLine(0.f, 0.f);
                    int pop = try_push_style(ImGuiCol_Text, m_colors.auto_complete_separator);
                    ImGui::TextUnformatted(m_autocomlete_separator.data(),
                                           m_autocomlete_separator.data() + m_autocomlete_separator.size());
                    ImGui::PopStyleColor(pop);
                    ImGui::SameLine(0.f, 0.f);
                };

                int max_displayable_sv = 0;
                float separator_length = ImGui::CalcTextSize(m_autocomlete_separator.data(),
                                                             m_autocomlete_separator.data() + m_autocomlete_separator.size())
                                                 .x;
                float total_text_length = ImGui::CalcTextSize("...").x;

                std::vector<std::string_view> autocomplete_text;
                if (m_current_autocomplete_strings.empty()) {
                    autocomplete_text.reserve(m_current_autocomplete.size());
                    for (const command_type &cmd: m_current_autocomplete) {
                        autocomplete_text.emplace_back(cmd.name);
                    }
                } else {
                    autocomplete_text.reserve(m_current_autocomplete_strings.size());
                    for (const std::string &str: m_current_autocomplete_strings) {
                        autocomplete_text.emplace_back(str);
                    }
                }

                for (const std::string_view &sv: autocomplete_text) {
                    float t_len = ImGui::CalcTextSize(sv.data(), sv.data() + sv.size()).x + separator_length;
                    if (t_len + total_text_length < auto_complete_max_size.x) {
                        total_text_length += t_len;
                        ++max_displayable_sv;
                    } else {
                        break;
                    }
                }

                std::string_view last;
                int pop_count = 0;

                if (max_displayable_sv != 0) {
                    const std::string_view &first = autocomplete_text[0];
                    pop_count += try_push_style(ImGuiCol_Text, m_colors.auto_complete_selected);
                    ImGui::TextUnformatted(first.data(), first.data() + first.size());
                    pop_count += try_push_style(ImGuiCol_Text, m_colors.auto_complete_non_selected);
                    for (int i = 1; i < max_displayable_sv; ++i) {
                        const std::string_view vs = autocomplete_text[i];
                        print_separator();
                        ImGui::TextUnformatted(vs.data(), vs.data() + vs.size());
                    }
                    ImGui::PopStyleColor(pop_count);
                    if (max_displayable_sv < static_cast<long>(autocomplete_text.size())) {
                        last = autocomplete_text[max_displayable_sv];
                    }
                }

                pop_count = 0;
                if (max_displayable_sv < static_cast<long>(autocomplete_text.size())) {

                    if (max_displayable_sv == 0) {
                        last = autocomplete_text.front();
                        pop_count += try_push_style(ImGuiCol_Text, m_colors.auto_complete_selected);
                        total_text_length -= separator_length;
                    } else {
                        pop_count += try_push_style(ImGuiCol_Text, m_colors.auto_complete_non_selected);
                        print_separator();
                    }

                    std::vector<char> buf;
                    buf.resize(last.size() + 4);
                    std::copy(last.begin(), last.end(), buf.begin());
                    std::fill(buf.begin() + last.size(), buf.end(), '.');
                    auto size = static_cast<unsigned>(last.size() + 3);
                    while (size >= 4 && total_text_length + ImGui::CalcTextSize(buf.data(), buf.data() + size).x >= auto_complete_max_size.x) {
                        buf[size - 4] = '.';
                        --size;
                    }
                    while (size != 0 && total_text_length + ImGui::CalcTextSize(buf.data(), buf.data() + size).x >= auto_complete_max_size.x) {
                        --size;
                    }
                    ImGui::TextUnformatted(buf.data(), buf.data() + size);
                    ImGui::PopStyleColor(pop_count);
                }
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }
    }

    template<typename TerminalHelper>
    void terminal<TerminalHelper>::call_command() noexcept {
        if (m_buffer_usage == 0) {
            return;
        }

        m_current_autocomplete_strings.clear();
        m_current_autocomplete.clear();

        bool modified{};
        std::pair<bool, std::string> resolved = resolve_history_references({m_command_buffer.data(), m_buffer_usage}, modified);

        if (!resolved.first) {
            try_log(R"(No such event: )" + resolved.second, message::type::error);
            return;
        }

        std::optional<std::vector<std::string>> splitted = split_by_space(resolved.second);
        if (!splitted) {
            try_log({m_command_buffer.data(), m_buffer_usage}, message::type::user_input);
            try_log("Unmatched \"", message::type::error);
            return;
        }

        try_log({m_command_buffer.data(), m_buffer_usage}, message::type::user_input);
        if (splitted->empty()) {
            return;
        }
        if (modified) {
            try_log("> " + resolved.second, message::type::cmd_history_completion);
        }

        std::vector<command_type_cref> matching_command_list = m_t_helper->find_commands_by_prefix(splitted->front());
        if (matching_command_list.empty()) {
            splitted->front() += ": command not found";
            try_log(splitted->front(), message::type::error);
            m_command_history.emplace_back(std::move(resolved.second));
            return;
        }

        argument_type arg{m_argument_value, *this, *splitted};

        matching_command_list[0].get().call(arg);
        m_command_history.emplace_back(std::move(resolved.second));// resolved.second has ownership over *splitted
    }

    template<typename TerminalHelper>
    std::pair<bool, std::string> terminal<TerminalHelper>::resolve_history_references(std::string_view str, bool &modified) const {
        enum class state {
            nothing,// matched nothing
            part_1, // matched one char: '!'
            part_2, // matched !-
            part_3, // matched !-[n]
            part_4, // matched !-[n]:
            finalize// matched !-[n]:[n]
        };

        modified = false;
        if (str.empty()) {
            return {true, {}};
        }

        if (str.size() == 1) {
            return {(str[0] != '!'), {str.data(), str.size()}};
        }

        std::string ans;
        ans.reserve(str.size());

        auto substr_beg = str.data();
        auto it = substr_beg;

        state current_state = state::nothing;

        auto resolve = [&](std::string_view history_request, bool add_escaping = true) -> bool {
            bool local_modified{};
            std::optional<std::string> solved = resolve_history_reference(history_request, local_modified);
            if (!solved) {
                return false;
            }

            auto is_space_lbd = [&solved, this](char c) {
                return is_space({&c, static_cast<unsigned>(&solved.value()[solved->size() - 1] + 1 - &c)}) > 0;
            };

            modified |= local_modified;
            if (add_escaping) {
                if (solved->empty()) {
                    ans += R"("")";
                } else if (std::find_if(solved->begin(), solved->end(), is_space_lbd) != solved->end()) {
                    ans += '"';
                    ans += *solved;
                    ans += '"';
                } else {
                    ans += *solved;
                }
            } else {
                ans += *solved;
            }
            substr_beg = std::next(it);
            current_state = state::nothing;
            return true;
        };

        const char *const end = str.data() + str.size();
        while (it != end) {

            if (*it == '\\') {
                do {
                    ++it;
                    if (it != end) {
                        ++it;
                    }
                } while (it != end && *it == '\\');

                if (current_state != state::nothing) {
                    return {false, {substr_beg, it}};
                }

                if (it == end) {
                    break;
                }
            }


            switch (current_state) {
                case state::nothing:
                    if (*it == '!') {
                        current_state = state::part_1;
                        ans += std::string_view{substr_beg, static_cast<unsigned>(it - substr_beg)};
                        substr_beg = it;
                    }
                    break;

                case state::part_1:
                    if (*it == '-') {
                        current_state = state::part_2;
                    } else if (*it == ':') {
                        current_state = state::part_4;
                    } else if (*it == '!') {
                        if (!resolve("!!", false)) {
                            return {false, "!!"};
                        }
                    } else {
                        current_state = state::nothing;
                    }
                    break;
                case state::part_2:
                    if (is_digit(*it)) {
                        current_state = state::part_3;
                    } else {
                        return {false, {substr_beg, it}};
                    }
                    break;
                case state::part_3:
                    if (*it == ':') {
                        current_state = state::part_4;
                    } else if (!is_digit(*it)) {
                        if (!resolve({substr_beg, static_cast<unsigned>(it - substr_beg)}, false)) {
                            return {false, {substr_beg, it}};
                        }
                    }
                    break;
                case state::part_4:
                    if (is_digit(*it)) {
                        current_state = state::finalize;
                    } else if (*it == '*') {
                        if (!resolve({substr_beg, static_cast<unsigned>(it + 1 - substr_beg)}, false)) {
                            return {false, {substr_beg, it}};
                        }
                    } else {
                        return {false, {substr_beg, it}};
                    }
                    break;
                case state::finalize:
                    if (!is_digit(*it)) {
                        if (!resolve({substr_beg, static_cast<unsigned>(it - substr_beg)})) {
                            return {false, {substr_beg, it}};
                        }
                        substr_beg = it;
                        continue;// we should loop without incrementing the pointer ; current character was not parsed
                    }
                    break;
            }

            ++it;
        }

        bool escape = true;
        if (substr_beg != it) {
            switch (current_state) {
                case state::nothing:
                    [[fallthrough]];
                case state::part_1:
                    ans += std::string_view{substr_beg, static_cast<unsigned>(it - substr_beg)};
                    break;
                case state::part_2:
                    [[fallthrough]];
                case state::part_4:
                    return {false, {substr_beg, it}};
                case state::part_3:
                    escape = false;
                    [[fallthrough]];
                case state::finalize:
                    if (!resolve({substr_beg, static_cast<unsigned>(it - substr_beg)}, escape)) {
                        return {false, {substr_beg, it}};
                    }
                    break;
            }
        }

        return {true, std::move(ans)};
    }

    template<typename TerminalHelper>
    std::optional<std::string> terminal<TerminalHelper>::resolve_history_reference(std::string_view str, bool &modified) const noexcept {
        modified = false;

        if (str.empty() || str[0] != '!') {
            return std::string{str.begin(), str.end()};
        }

        if (str.size() < 2) {
            return {};
        }

        if (str[1] == '!') {
            if (m_command_history.empty() || str.size() != 2) {
                return {};
            } else {
                modified = true;
                return {m_command_history.back()};
            }
        }

        // ![stuff]
        unsigned int backward_jump = 1;
        unsigned int char_idx = 1;
        if (str[1] == '-') {
            if (str.size() <= 2 || !is_digit(str[2])) {
                return {};
            }

            unsigned int val{0};
            std::from_chars_result res = std::from_chars(str.data() + 2, str.data() + str.size(), val, 10);
            if (val == 0) {
                return {};// val == 0  <=> (garbage input || user inputted 0)
            }

            backward_jump = val;
            char_idx = static_cast<unsigned int>(res.ptr - str.data());
        }

        if (m_command_history.size() < backward_jump) {
            return {};
        }

        if (char_idx >= str.size()) {
            modified = true;
            return m_command_history[m_command_history.size() - backward_jump];
        }

        if (str[char_idx] != ':') {
            return {};
        }


        ++char_idx;
        if (str.size() <= char_idx) {
            return {};
        }

        if (str[char_idx] == '*') {
            modified = true;
            const std::string &cmd = m_command_history[m_command_history.size() - backward_jump];

            int sp_count = 0;
            auto is_space_lbd = [&sp_count, &cmd, this](char c) {
                if (sp_count > 0) {
                    --sp_count;
                    return true;
                }
                sp_count = is_space({&c, static_cast<unsigned>(&*cmd.end() - &c)});
                if (sp_count > 0) {
                    --sp_count;
                    return true;
                }
                return false;
            };

            auto first_non_space = std::find_if_not(cmd.begin(), cmd.end(), is_space_lbd);
            sp_count = 0;
            auto first_space = std::find_if(first_non_space, cmd.end(), is_space_lbd);
            sp_count = 0;
            first_non_space = std::find_if_not(first_space, cmd.end(), is_space_lbd);

            if (first_non_space == cmd.end()) {
                return std::string{""};
            }
            return std::string{first_non_space, cmd.end()};
        }

        if (!is_digit(str[char_idx])) {
            return {};
        }

        unsigned int val1{};
        std::from_chars_result res1 = std::from_chars(str.data() + char_idx, str.data() + str.size(), val1, 10);
        if (!misc::success(res1.ec) || res1.ptr != str.data() + str.size()) {// either unsuccessful or we didn't reach the end of the string
            return {};
        }

        const std::string &cmd = m_command_history[m_command_history.size() - backward_jump];// 1 <= backward_jump <= command_history.size()
        std::optional<std::vector<std::string>> args = split_by_space(cmd);

        if (!args || args->size() <= val1) {
            return {};
        }

        modified = true;
        return (*args)[val1];
    }

    template<typename TerminalHelper>
    int terminal<TerminalHelper>::command_line_callback_st(ImGuiInputTextCallbackData *data) noexcept {
        return reinterpret_cast<terminal *>(data->UserData)->command_line_callback(data);
    }

    template<typename TerminalHelper>
    int terminal<TerminalHelper>::command_line_callback(ImGuiInputTextCallbackData *data) noexcept {

        auto paste_buffer = [data](auto begin, auto end, auto buffer_shift) {
            misc::copy(begin, end, data->Buf + buffer_shift, data->Buf + data->BufSize - 1);
            data->BufTextLen = std::min(static_cast<int>(std::distance(begin, end) + buffer_shift), data->BufSize - 1);
            data->Buf[data->BufTextLen] = '\0';
            data->BufDirty = true;
            data->SelectionStart = data->SelectionEnd;
            data->CursorPos = data->BufTextLen;
        };

        auto auto_complete_buffer = [data, this](std::string &&str, auto reference_size) {
            auto buff_end = misc::erase_insert(str.begin(), str.end(), data->Buf + data->CursorPos - reference_size, data->Buf + m_buffer_usage, data->Buf + data->BufSize, reference_size);

            data->BufTextLen = static_cast<unsigned>(std::distance(data->Buf, buff_end));
            data->Buf[data->BufTextLen] = '\0';
            data->BufDirty = true;
            data->SelectionStart = data->SelectionEnd;
            data->CursorPos = std::min(static_cast<int>(data->CursorPos + str.size() - reference_size), data->BufTextLen);
            m_buffer_usage = static_cast<unsigned>(data->BufTextLen);
        };

        if (data->EventKey == ImGuiKey_Tab) {
            std::vector<std::string_view> autocomplete_text;
            if (m_current_autocomplete_strings.empty()) {
                autocomplete_text.reserve(m_current_autocomplete.size());
                for (const command_type &cmd: m_current_autocomplete) {
                    autocomplete_text.emplace_back(cmd.name);
                }
            } else {
                autocomplete_text.reserve(m_current_autocomplete_strings.size());
                for (const std::string &str: m_current_autocomplete_strings) {
                    autocomplete_text.emplace_back(str);
                }
            }


            if (autocomplete_text.empty()) {
                if (m_buffer_usage == 0 || data->CursorPos < 2) {
                    return 0;
                }

                auto excl = misc::find_last(m_command_buffer.data(), m_command_buffer.data() + data->CursorPos, '!');
                if (excl == m_command_buffer.data() + data->CursorPos) {
                    return 0;
                }
                if (excl == m_command_buffer.data() + data->CursorPos - 1 && m_command_buffer[data->CursorPos - 2] == '!') {
                    --excl;
                }
                bool modified{};
                std::string_view reference{excl, static_cast<unsigned>(m_command_buffer.data() + data->CursorPos - excl)};
                std::optional<std::string> val = resolve_history_reference(reference, modified);
                if (!modified) {
                    return 0;
                }

                if (reference.substr(reference.size() - 2) != ":*" && reference.find(':') != std::string_view::npos) {
                    auto is_space_lbd = [&val, this](char c) {
                        return is_space({&c, static_cast<unsigned>(&val.value()[val->size()] + 1 - &c)}) > 0;
                    };

                    if (std::find_if(val->begin(), val->end(), is_space_lbd) != val->end()) {
                        val = '"' + std::move(*val) + '"';
                    }
                }
                auto_complete_buffer(std::move(*val), static_cast<unsigned>(reference.size()));

                return 0;
            }

            std::string_view complete_sv = autocomplete_text[0];

            auto quote_count = std::count(m_command_buffer.data(), m_command_buffer.data() + m_buffer_usage, '"');
            const char *command_beg = nullptr;
            if (quote_count % 2) {
                command_beg = misc::find_last(m_command_buffer.data(), m_command_buffer.data() + m_buffer_usage, '"');
            } else {
                command_beg = misc::find_terminating_word(m_command_buffer.data(), m_command_buffer.data() + m_buffer_usage, [this](std::string_view sv) { return is_space(sv); });
                ;
            }


            bool space_found = std::find_if(complete_sv.begin(), complete_sv.end(), [this, &complete_sv](char c) { return is_space({&c, static_cast<unsigned>(&complete_sv[complete_sv.size() - 1] + 1 - &c)}) > 0; }) != complete_sv.end();

            if (space_found) {
                std::string complete;
                complete.reserve(complete_sv.size() + 2);
                complete = '"';
                complete += complete_sv;
                complete += '"';
                paste_buffer(complete.data(), complete.data() + complete.size(), command_beg - m_command_buffer.data());
            } else {
                paste_buffer(complete_sv.data(), complete_sv.data() + complete_sv.size(), command_beg - m_command_buffer.data());
            }

            m_buffer_usage = static_cast<unsigned>(data->BufTextLen);
            m_current_autocomplete.clear();
            m_current_autocomplete_strings.clear();

        } else if (data->EventKey == ImGuiKey_UpArrow) {
            if (m_command_history.empty()) {
                return 0;
            }
            m_ignore_next_textinput = true;

            if (!m_current_history_selection) {

                m_current_history_selection = m_command_history.end();
                m_command_line_backup = std::string(m_command_buffer.data(), m_command_buffer.data() + m_buffer_usage);
                m_command_line_backup_prefix = m_command_line_backup;

                auto is_space_lbd = [this](unsigned int idx) {
                    const char *ptr = &m_command_line_backup_prefix[idx];
                    return is_space({ptr, static_cast<unsigned>(m_command_line_backup_prefix.size() - idx)});
                };
                unsigned int idx = 0;
                int space_count = 0;
                while (idx < m_command_line_backup_prefix.size() && (space_count = is_space_lbd(idx)) > 0) {
                    idx += space_count;
                }
                if (idx > m_command_line_backup_prefix.size()) {
                    idx = static_cast<unsigned>(m_command_line_backup_prefix.size());
                }

                m_command_line_backup_prefix.remove_prefix(idx);
                m_current_autocomplete.clear();
                m_current_autocomplete_strings.clear();
            }

            auto it = misc::find_first_prefixed(
                    m_command_line_backup_prefix, std::reverse_iterator(*m_current_history_selection), m_command_history.rend(), [this](std::string_view str) { return is_space(str); });

            if (it != m_command_history.rend()) {
                m_current_history_selection = std::prev(it.base());
                paste_buffer((*m_current_history_selection)->begin() + m_command_line_backup_prefix.size(), (*m_current_history_selection)->end(), m_command_line_backup.size());
                m_buffer_usage = static_cast<unsigned>(data->BufTextLen);
            } else {
                if (m_current_history_selection == m_command_history.end()) {
                    // no auto completion occured
                    m_ignore_next_textinput = false;
                    m_current_history_selection = {};
                    m_command_line_backup_prefix = {};
                    m_command_line_backup.clear();
                }
            }

        } else if (data->EventKey == ImGuiKey_DownArrow) {

            if (!m_current_history_selection) {
                return 0;
            }
            m_ignore_next_textinput = true;

            m_current_history_selection = misc::find_first_prefixed(
                    m_command_line_backup_prefix, std::next(*m_current_history_selection), m_command_history.end(), [this](std::string_view str) { return is_space(str); });

            if (m_current_history_selection != m_command_history.end()) {
                paste_buffer((*m_current_history_selection)->begin() + m_command_line_backup_prefix.size(), (*m_current_history_selection)->end(), m_command_line_backup.size());
                m_buffer_usage = static_cast<unsigned>(data->BufTextLen);

            } else {
                data->BufTextLen = static_cast<int>(m_command_line_backup.size());
                data->Buf[data->BufTextLen] = '\0';
                data->BufDirty = true;
                data->SelectionStart = data->SelectionEnd;
                data->CursorPos = data->BufTextLen;
                m_buffer_usage = static_cast<unsigned>(data->BufTextLen);

                m_current_history_selection = {};
                m_command_line_backup_prefix = {};
                m_command_line_backup.clear();
            }

        } else {
            // todo: log in some meaningfull way ?
        }

        return 0;
    }

    template<typename TerminalHelper>
    int terminal<TerminalHelper>::is_space(std::string_view str) const {
        return details::is_space(m_t_helper, str);
    }

    template<typename TerminalHelper>
    bool terminal<TerminalHelper>::is_digit(char c) const {
        return c >= '0' && c <= '9';
    }

    template<typename TerminalHelper>
    unsigned long terminal<TerminalHelper>::get_length(std::string_view str) const {
        return details::get_length(m_t_helper, str);
    }

    template<typename TerminalHelper>
    std::optional<std::vector<std::string>> terminal<TerminalHelper>::split_by_space(std::string_view in, bool ignore_non_match) const {
        std::vector<std::string> out;

        const char *it = &in[0];
        const char *const in_end = &in[in.size() - 1] + 1;

        auto skip_spaces = [&]() {
            int space_count;
            do {
                space_count = is_space({it, static_cast<unsigned>(in_end - it)});
                it += space_count;
            } while (it != in_end && space_count > 0);
        };

        if (it != in_end) {
            skip_spaces();
        }

        if (it == in_end) {
            return out;
        }

        bool matched_quote{};
        bool matched_space{};
        std::string current_string{};
        do {
            if (*it == '"') {
                bool escaped;
                do {
                    escaped = (*it == '\\');
                    ++it;

                    if (it != in_end && (*it != '"' || escaped)) {
                        if (*it == '\\') {
                            if (escaped) {
                                current_string += *it;
                            }
                        } else {
                            current_string += *it;
                        }
                    } else {
                        break;
                    }

                } while (true);

                if (it == in_end) {
                    if (!ignore_non_match) {
                        return {};
                    }
                } else {
                    ++it;
                }
                matched_quote = true;
                matched_space = false;

            } else if (is_space({it, static_cast<unsigned>(in_end - it)}) > 0) {
                out.emplace_back(std::move(current_string));
                current_string = {};
                skip_spaces();
                matched_space = true;
                matched_quote = false;

            } else if (*it == '\\') {
                matched_quote = false;
                matched_space = false;
                ++it;
                if (it != in_end) {
                    current_string += *it;
                    ++it;
                }
            } else {
                matched_space = false;
                matched_quote = false;
                current_string += *it;
                ++it;
            }

        } while (it != in_end);

        if (!current_string.empty()) {
            out.emplace_back(std::move(current_string));
        } else if (matched_quote || matched_space) {
            out.emplace_back();
        }

        return out;
    }

    template<typename TerminalHelper>
    void terminal<TerminalHelper>::push_message(message &&msg) {
        try_lock();
        if (m_logs.size() == m_max_log_len) {
            m_logs[m_current_log_oldest_idx] = std::move(msg);
            m_current_log_oldest_idx = (m_current_log_oldest_idx + 1) % m_logs.size();
        } else {
            m_logs.emplace_back(std::move(msg));
        }
        try_unlock();
    }
}// namespace ImTerm


#undef IMTERM_FMT_INCLUDED

#endif//IMTERM_TERMINAL_HPP


#ifndef TERMINAL_HELPER_HPP
#define TERMINAL_HELPER_HPP


#include <array>
#include <set>

#if __has_include("spdlog/spdlog.h")
#include "spdlog/common.h"
#include "spdlog/details/log_msg.h"
#include "spdlog/formatter.h"
#include "spdlog/pattern_formatter.h"
#include "spdlog/sinks/base_sink.h"

#define IMTERM_SPDLOG_INCLUDED
// helpers
namespace ImTerm::details {
    constexpr message::severity::severity_t to_imterm_severity(spdlog::level::level_enum);
    constexpr spdlog::level::level_enum to_spdlog_severity(message::severity::severity_t);
    constexpr spdlog::level::level_enum to_spdlog_severity(message::type);
    inline ImTerm::message to_imterm_msg(const spdlog::details::log_msg &);
}// namespace ImTerm::details
#endif

namespace ImTerm {


    // terminal_helper_example is meant to be an example
    // if you want to inherit from one, pick term::basic_terminal_helper (see below)
    template<typename T>
    class terminal_helper_example {
    public:
        using value_type = T;// < Mandatory, this type will be passed to commands via argument_type
        using term_t = ImTerm::terminal<terminal_helper_example>;
        using command_type = ImTerm::command_t<ImTerm::terminal<terminal_helper_example>>;
        using argument_type = ImTerm::argument_t<ImTerm::terminal<terminal_helper_example>>;
        using command_type_cref = std::reference_wrapper<const command_type>;

        // mandatory : return every command starting by prefix
        std::vector<command_type_cref> find_commands_by_prefix(std::string_view prefix) {

            auto compare_by_name = [](const command_type &cmd) { return cmd.name; };
            auto map_to_cref = [](const command_type &cmd) { return std::cref(cmd); };

            return misc::prefix_search(prefix, cmd_list.begin(), cmd_list.end(), std::move(compare_by_name), std::move(map_to_cref));
        }

        // mandatory : return every command starting by the text formed by [beg, end)
        std::vector<command_type_cref> find_commands_by_prefix(const char *beg, const char *end) {
            return find_commands_by_prefix({beg, static_cast<unsigned>(end - beg)});
        }

        // mandatory: returns the full command list
        std::vector<command_type_cref> list_commands() {
            return find_commands_by_prefix(std::string_view{});
        }

        // mandatory: formats the given string.
        // msg type is either user_input, error, or cmd_history_completion
        // return an empty optional if you do not want the string to be logged
        // message's members 'is_term_message' and 'severity' are ignored
        std::optional<ImTerm::message> format(std::string str, [[maybe_unused]] ImTerm::message::type msg_type) {
            ImTerm::message msg;
            msg.color_beg = msg.color_end = 0u;
            msg.value = std::move(str);
            return {std::move(msg)};// other fields are ignored
        }

        // optional : to be friendlier with non ascii encoding
        // return value : 0 if str does not begin by a space
        //                otherwise, the number of characters participating in the representation of the beginning space
        // should not return a value > str.size()
        // the passed string_view is never empty (at least one char)
        //		int is_space(std::string_view str) const {
        //			return str[0] == ' ';
        //		}

        // optional : to be friendlier with non ascii encoding
        // return value : number of glyphs represented in str (== str.size() for ascii)
        //		unsigned long get_length(std::string_view str) {
        //	      return str.size();
        //		}

        // optional : called right after terminal's construction. You may store the terminal for future usage
        //		void set_terminal(term_t& term) {
        //		}


        // command samples (implemented as static methods, but they can be outside of a class, if you will to)
        static std::vector<std::string> no_completion(argument_type &) { return {}; }

        static void clear(argument_type &arg) {
            arg.term.clear();
        }

        static void echo(argument_type &arg) {
            std::string str{};
            str = arg.command_line[1];
            for (auto it = std::next(arg.command_line.begin(), 2); it != arg.command_line.end(); ++it) {
                str.reserve(str.size() + it->size() + 1);
                str += ' ';
                str += *it;
            }
            message msg;
            msg.value = std::move(str);
            msg.color_beg = msg.color_end = 0;// color is disabled when color_beg == color_end
            // other parameters are ignored
            arg.term.add_message(std::move(msg));
        }

        static constexpr std::array<command_type, 2> cmd_list = {
                command_type{"clear ", "clears the screen", clear, no_completion},
                command_type{"echo ", "prints text to the screen", echo, no_completion},
        };
    };

    // Basic terminal helper
    // You may inherit to save some hassle
    // Template parameter TerminalHelper is in most cases the derived class (and should be if you don't know what to put)
    // Template parameter Value is the type passed to commands together with the other arguments
    // You may add commands with the 'add_command_' method.
    // Refer to terminal_helper_example (see above) for a commented example
    template<typename TerminalHelper, typename Value>
    class basic_terminal_helper {
    public:
        using value_type = Value;
        using term_t = ImTerm::terminal<TerminalHelper>;
        using command_type = ImTerm::command_t<ImTerm::terminal<TerminalHelper>>;
        using argument_type = ImTerm::argument_t<ImTerm::terminal<TerminalHelper>>;
        using command_type_cref = std::reference_wrapper<const command_type>;

        basic_terminal_helper() = default;
        basic_terminal_helper(const basic_terminal_helper &) = default;
        basic_terminal_helper(basic_terminal_helper &&) noexcept = default;

        std::vector<command_type_cref> find_commands_by_prefix(std::string_view prefix) {
            auto compare_name = [](const command_type &cmd) { return cmd.name; };
            auto map_to_cref = [](const command_type &cmd) { return std::cref(cmd); };

            return misc::prefix_search(prefix, cmd_list_.begin(), cmd_list_.end(), std::move(compare_name),
                                       std::move(map_to_cref));
        }

        std::vector<command_type_cref> find_commands_by_prefix(const char *beg, const char *end) {
            return find_commands_by_prefix({beg, static_cast<unsigned>(end - beg)});
        }

        std::vector<command_type_cref> list_commands() {
            std::vector<command_type_cref> ans;
            ans.reserve(cmd_list_.size());
            for (const command_type &cmd: cmd_list_) {
                ans.emplace_back(cmd);
            }
            return ans;
        }

        std::optional<ImTerm::message> format(std::string str, ImTerm::message::type) {
            ImTerm::message msg;
            msg.value = std::move(str);
            msg.color_beg = msg.color_end = 0u;
            return {std::move(msg)};
        }

    protected:
        void add_command_(const command_type &cmd) {
            cmd_list_.emplace(cmd);
        }

        std::set<command_type> cmd_list_{};
    };

#ifdef IMTERM_SPDLOG_INCLUDED

    // Basic spdlog terminal helper inheriting spdlog::sinks::sink (logging messages to the terminal)
    // You may inherit to save some hassle
    // Template parameter TerminalHelper is in most cases the derived class (and should be if you don't know what to put)
    // Template parameter Value is the type passed to commands together with the other arguments
    // Template parameter Mutex is passed to spdlog::sinks::base_sink
    // You may add commands with the 'add_command_' method.
    // Refer to terminal_helper_example (see above) for a commented example
    // should not be used after move
    template<typename TerminalHelper, typename Value, typename Mutex>
    class basic_spdlog_terminal_helper : public basic_terminal_helper<TerminalHelper, Value>, public spdlog::sinks::base_sink<Mutex> {
        using SinkBase = spdlog::sinks::base_sink<Mutex>;
        using TermHBase = basic_terminal_helper<TerminalHelper, Value>;

    public:
        using typename TermHBase::term_t;

        explicit basic_spdlog_terminal_helper(std::string terminal_to_terminal_logger_name = "ImTerm Terminal")
            : logger_name_{std::move(terminal_to_terminal_logger_name)} {
            set_terminal_pattern_("%T.%e - [%^command line%$]: %v", message::type::error);
            set_terminal_pattern_("%T.%e - %^%v%$", message::type::user_input);
            set_terminal_pattern_("%T.%e - %^%v%$", message::type::cmd_history_completion);
        }

        basic_spdlog_terminal_helper(basic_spdlog_terminal_helper &&other) noexcept
            : SinkBase{}, TermHBase(std::move(other)), terminal_{std::exchange(other.terminal_, nullptr)}, terminal_formatter_{std::move(other.terminal_formatter_)}, logger_name_{std::move(other.logger_name_)} {
            SinkBase::set_level(other.level());
            SinkBase::set_formatter(std::move(other.formatter_));
        }

        virtual ~basic_spdlog_terminal_helper() noexcept = default;

        std::optional<ImTerm::message> format(std::string str, [[maybe_unused]] ImTerm::message::type type) {
            spdlog::details::log_msg msg(logger_name_, {}, str);
            spdlog::memory_buf_t buff{};
            terminal_formatter_[static_cast<int>(type)]->format(msg, buff);

            ImTerm::message term_msg = details::to_imterm_msg(msg);
            term_msg.value = fmt::to_string(buff);
            return term_msg;
        }

        // this method is called automatically right after ImTerm::terminal's construction
        // used to sink logs to the message panel
        void set_terminal(term_t &term) {
            terminal_ = &term;
        }

        // set logging pattern per message type, for feed-back messages from the terminal
        void set_terminal_pattern(const std::string &pattern, ImTerm::message::type type) {
            std::lock_guard<Mutex> lock(SinkBase::mutex_);
            set_terminal_pattern_(std::make_unique<spdlog::pattern_formatter>(pattern), type);
        }

        void set_terminal_formatter(std::unique_ptr<spdlog::formatter> &&terminal_formatter, ImTerm::message::type type) {
            std::lock_guard<Mutex> lock(SinkBase::mutex_);
            set_terminal_formatter_(std::move(terminal_formatter), type);
        }

    protected:
        void set_terminal_pattern_(const std::string &pattern, ImTerm::message::type type) {
            set_terminal_formatter_(std::make_unique<spdlog::pattern_formatter>(pattern), type);
        }

        void set_terminal_formatter_(std::unique_ptr<spdlog::formatter> &&terminal_formatter, ImTerm::message::type type) {
            terminal_formatter_[static_cast<int>(type)] = std::move(terminal_formatter);
        }

        void sink_it_(const spdlog::details::log_msg &msg) override {
            if (msg.level == spdlog::level::off) {
                return;
            }
            assert(terminal_ != nullptr);
            spdlog::memory_buf_t buff{};
            SinkBase::formatter_->format(msg, buff);
            terminal_->add_message({details::to_imterm_severity(msg.level), fmt::to_string(buff), msg.color_range_start, msg.color_range_end, false});
        }

        void flush_() override {}

        term_t *terminal_{};
        std::array<std::unique_ptr<spdlog::formatter>, 3> terminal_formatter_{};// user_input, error, cmd_history_completion (c.f. ImTerm::message::type)
        std::string logger_name_;
    };


    namespace details {

        constexpr message::severity::severity_t to_imterm_severity(spdlog::level::level_enum lvl) {
            assert(lvl != spdlog::level::off);
            if constexpr (
                    ImTerm::message::severity::trace == static_cast<int>(spdlog::level::trace) &&
                    ImTerm::message::severity::debug == static_cast<int>(spdlog::level::debug) &&
                    ImTerm::message::severity::info == static_cast<int>(spdlog::level::info) &&
                    ImTerm::message::severity::warn == static_cast<int>(spdlog::level::warn) &&
                    ImTerm::message::severity::err == static_cast<int>(spdlog::level::err) &&
                    ImTerm::message::severity::critical == static_cast<int>(spdlog::level::critical)) {
                return static_cast<message::severity::severity_t>(lvl);
            } else {
                switch (lvl) {
                    case spdlog::level::trace:
                        return ImTerm::message::severity::trace;
                    case spdlog::level::debug:
                        return ImTerm::message::severity::debug;
                    case spdlog::level::info:
                        return ImTerm::message::severity::info;
                    case spdlog::level::warn:
                        return ImTerm::message::severity::warn;
                    case spdlog::level::err:
                        return ImTerm::message::severity::err;
                    case spdlog::level::critical:
                        return ImTerm::message::severity::critical;
                    default:
                        assert(false);
                        return {};
                }
            }
        }

        constexpr spdlog::level::level_enum to_spdlog_severity(message::severity::severity_t lvl) {
            if constexpr (
                    ImTerm::message::severity::trace == static_cast<int>(spdlog::level::trace) &&
                    ImTerm::message::severity::debug == static_cast<int>(spdlog::level::debug) &&
                    ImTerm::message::severity::info == static_cast<int>(spdlog::level::info) &&
                    ImTerm::message::severity::warn == static_cast<int>(spdlog::level::warn) &&
                    ImTerm::message::severity::err == static_cast<int>(spdlog::level::err) &&
                    ImTerm::message::severity::critical == static_cast<int>(spdlog::level::critical)) {
                return static_cast<spdlog::level::level_enum>(lvl);
            } else {
                switch (lvl) {
                    case ImTerm::message::severity::trace:
                        return spdlog::level::trace;
                    case ImTerm::message::severity::debug:
                        return spdlog::level::debug;
                    case ImTerm::message::severity::info:
                        return spdlog::level::info;
                    case ImTerm::message::severity::warn:
                        return spdlog::level::warn;
                    case ImTerm::message::severity::err:
                        return spdlog::level::err;
                    case ImTerm::message::severity::critical:
                        return spdlog::level::critical;
                }
            }
        }

        inline ImTerm::message to_imterm_msg(const spdlog::details::log_msg &msg) {
            ImTerm::message term_msg{};
            term_msg.severity = to_imterm_severity(msg.level);
            term_msg.color_beg = msg.color_range_start;
            term_msg.color_end = msg.color_range_end;
            return term_msg;
        }
    }// namespace details
#endif

}// namespace ImTerm


#undef IMTERM_SPDLOG_INCLUDED

#endif//TERMINAL_HELPER_HPP
