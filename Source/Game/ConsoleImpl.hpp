// Copyright(c) 2022, KaoruXun All rights reserved.

// Hack form https://github.com/rmxbalanque/csys

#ifndef _METADOT_CONSOLEIMPL_HPP_
#define _METADOT_CONSOLEIMPL_HPP_

#include "Core/Macros.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ConsoleImpl {

    struct String
    {

        String() = default;

        String(const char *str [[maybe_unused]]) : m_String(str ? str : "") {}

        String(std::string str) : m_String(std::move(str)) {}

        operator const char *() { return m_String.c_str(); }

        operator std::string() { return m_String; }

        std::pair<size_t, size_t> NextPoi(size_t &start) const {
            size_t end = m_String.size();
            std::pair<size_t, size_t> range(end + 1, end);
            size_t pos = start;

            for (; pos < end; ++pos)
                if (!std::isspace(m_String[pos])) {
                    range.first = pos;
                    break;
                }

            for (; pos < end; ++pos)
                if (std::isspace(m_String[pos])) {
                    range.second = pos;
                    break;
                }

            start = range.second;
            return range;
        }

        [[nodiscard]] size_t End() const { return m_String.size() + 1; }

        std::string m_String;
    };

    class Exception : public std::exception {
    public:
        explicit Exception(const std::string &message, const std::string &arg)
            : m_Msg(message + ": '" + arg + "'") {}

        explicit Exception(std::string message) : m_Msg(std::move(message)) {}

        ~Exception() override = default;

        [[nodiscard]] const char *what() const noexcept override { return m_Msg.c_str(); }

    protected:
        std::string m_Msg;
    };

    namespace {
        inline const std::string_view s_Reserved("\\[]\"");
        inline constexpr char s_ErrMsgReserved[] = "Reserved chars '\\, [, ], \"' must be escaped "
                                                   "with \\";
    }// namespace

    struct Reserved
    {

        static inline bool IsEscapeChar(char c) { return c == '\\'; }

        static inline bool IsReservedChar(char c) {
            for (auto rc: s_Reserved)
                if (c == rc) return true;
            return false;
        }

        static inline bool IsEscaping(std::string &input, size_t pos) {
            return pos < input.size() - 1 && IsEscapeChar(input[pos]) &&
                   IsReservedChar(input[pos + 1]);
        }

        static inline bool IsEscaped(std::string &input, size_t pos) {
            bool result = false;

            for (size_t i = pos; i > 0; --i)
                if (IsReservedChar(input[i]) && IsEscapeChar(input[i - 1])) result = !result;
                else
                    break;
            return result;
        }

        Reserved() = delete;
        ~Reserved() = delete;
        Reserved(Reserved &&) = delete;
        Reserved(const Reserved &) = delete;
        Reserved &operator=(Reserved &&) = delete;
        Reserved &operator=(const Reserved &) = delete;
    };

    template<typename T>
    struct ArgumentParser
    {

        inline ArgumentParser(String &input, size_t &start);

        T m_Value;
    };

#define ARG_PARSE_BASE_SPEC(TYPE)                                                                  \
    template<>                                                                                     \
    struct ArgumentParser<TYPE>                                                                    \
    {                                                                                              \
        inline ArgumentParser(String &input, size_t &start);                                       \
        TYPE m_Value = 0;                                                                          \
    };                                                                                             \
    inline ArgumentParser<TYPE>::ArgumentParser(String &input, size_t &start)

#define ARG_PARSE_SUBSTR(range) input.m_String.substr(range.first, range.second - range.first)

#define ARG_PARSE_GENERAL_SPEC(TYPE, TYPE_NAME, FUNCTION)                                          \
    ARG_PARSE_BASE_SPEC(TYPE) {                                                                    \
        auto range = input.NextPoi(start);                                                         \
        try {                                                                                      \
            m_Value = (TYPE) FUNCTION(ARG_PARSE_SUBSTR(range), &range.first);                      \
        } catch (const std::out_of_range &) {                                                      \
            throw Exception(std::string("Argument too large for ") + TYPE_NAME,                    \
                            input.m_String.substr(range.first, range.second));                     \
        } catch (const std::invalid_argument &) {                                                  \
            throw Exception(std::string("Missing or invalid ") + TYPE_NAME + " argument",          \
                            input.m_String.substr(range.first, range.second));                     \
        }                                                                                          \
    }

    ARG_PARSE_BASE_SPEC(ConsoleImpl::String) {
        m_Value.m_String.clear();

        static auto GetWord = [](std::string &str, size_t start, size_t end) {
            static std::string invalid_chars;
            invalid_chars.clear();

            std::string result;

            for (size_t i = start; i < end; ++i)

                if (!Reserved::IsReservedChar(str[i])) result.push_back(str[i]);

                else {

                    if (Reserved::IsEscapeChar(str[i]) && Reserved::IsEscaping(str, i))
                        result.push_back(str[++i]);

                    else
                        throw Exception(s_ErrMsgReserved, str.substr(start, end - start));
                }

            return result;
        };

        auto range = input.NextPoi(start);

        if (input.m_String[range.first] != '"')
            m_Value = GetWord(input.m_String, range.first, range.second);

        else {
            ++range.first;
            while (true) {

                range.second = input.m_String.find('"', range.first);
                while (range.second != std::string::npos &&
                       Reserved::IsEscaped(input.m_String, range.second))
                    range.second = input.m_String.find('"', range.second + 1);

                if (range.second == std::string::npos) {
                    range.second = input.m_String.size();
                    throw Exception("Could not find closing '\"'", ARG_PARSE_SUBSTR(range));
                }

                m_Value.m_String += GetWord(input.m_String, range.first, range.second);

                range.first = range.second + 1;

                if (range.first < input.m_String.size() &&
                    !std::isspace(input.m_String[range.first])) {

                    if (input.m_String[range.first] == '"') ++range.first;
                } else

                    break;
            }
        }

        start = range.second + 1;
    }

    ARG_PARSE_BASE_SPEC(bool) {

        static const char *s_err_msg = "Missing or invalid boolean argument";
        static const char *s_false = "false";
        static const char *s_true = "true";

        auto range = input.NextPoi(start);

        input.m_String[range.first] = char(std::tolower(input.m_String[range.first]));

        if (range.second - range.first == 4 && input.m_String[range.first] == 't') {

            for (size_t i = range.first + 1; i < range.second; ++i)
                if ((input.m_String[i] = char(std::tolower(input.m_String[i]))) !=
                    s_true[i - range.first])
                    throw Exception(s_err_msg + std::string(", expected true"),
                                    ARG_PARSE_SUBSTR(range));
            m_Value = true;
        }

        else if (range.second - range.first == 5 && input.m_String[range.first] == 'f') {

            for (size_t i = range.first + 1; i < range.second; ++i)
                if ((input.m_String[i] = char(std::tolower(input.m_String[i]))) !=
                    s_false[i - range.first])
                    throw Exception(s_err_msg + std::string(", expected false"),
                                    ARG_PARSE_SUBSTR(range));
            m_Value = false;
        }

        else
            throw Exception(s_err_msg, ARG_PARSE_SUBSTR(range));
    }

    ARG_PARSE_BASE_SPEC(char) {

        auto range = input.NextPoi(start);
        size_t len = range.second - range.first;

        if (len > 2 || len <= 0)
            throw Exception("Too many or no chars were given", ARG_PARSE_SUBSTR(range));

        else if (len == 2) {

            if (!Reserved::IsEscaping(input.m_String, range.first))
                throw Exception("Too many chars were given", ARG_PARSE_SUBSTR(range));

            m_Value = input.m_String[range.first + 1];
        }

        else if (Reserved::IsReservedChar(input.m_String[range.first]))
            throw Exception(s_ErrMsgReserved, ARG_PARSE_SUBSTR(range));

        else
            m_Value = input.m_String[range.first];
    }

    ARG_PARSE_BASE_SPEC(unsigned char) {

        auto range = input.NextPoi(start);
        size_t len = range.second - range.first;

        if (len > 2 || len <= 0)
            throw Exception("Too many or no chars were given", ARG_PARSE_SUBSTR(range));

        else if (len == 2) {

            if (!Reserved::IsEscaping(input.m_String, range.first))
                throw Exception("Too many chars were given", ARG_PARSE_SUBSTR(range));

            m_Value = static_cast<unsigned char>(input.m_String[range.first + 1]);
        }

        else if (Reserved::IsReservedChar(input.m_String[range.first]))
            throw Exception(s_ErrMsgReserved, ARG_PARSE_SUBSTR(range));

        else
            m_Value = static_cast<unsigned char>(input.m_String[range.first]);
    }

    ARG_PARSE_GENERAL_SPEC(short, "signed short", std::stoi)
    ARG_PARSE_GENERAL_SPEC(unsigned short, "unsigned short", std::stoul)
    ARG_PARSE_GENERAL_SPEC(int, "signed int", std::stoi)
    ARG_PARSE_GENERAL_SPEC(unsigned int, "unsigned int", std::stoul)
    ARG_PARSE_GENERAL_SPEC(long, "long", std::stol)
    ARG_PARSE_GENERAL_SPEC(unsigned long, "unsigned long", std::stoul)
    ARG_PARSE_GENERAL_SPEC(long long, "long long", std::stoll)
    ARG_PARSE_GENERAL_SPEC(unsigned long long, "unsigned long long", std::stoull)
    ARG_PARSE_GENERAL_SPEC(float, "float", std::stof)
    ARG_PARSE_GENERAL_SPEC(double, "double", std::stod)
    ARG_PARSE_GENERAL_SPEC(long double, "long double", std::stold)

    template<typename T>
    struct ArgumentParser<std::vector<T>>
    {

        ArgumentParser(String &input, size_t &start);

        std::vector<T> m_Value;
    };

    template<typename T>
    ArgumentParser<std::vector<T>>::ArgumentParser(String &input, size_t &start) {

        m_Value.clear();

        auto range = input.NextPoi(start);

        if (range.first == input.End()) return;

        if (input.m_String[range.first] != '[')
            throw Exception("Invalid vector argument missing opening [", ARG_PARSE_SUBSTR(range));

        input.m_String[range.first] = ' ';
        while (true) {

            range = input.NextPoi(range.first);

            if (range.first == input.End()) return;

            else if (input.m_String[range.first] == '[')
                m_Value.push_back(ArgumentParser<T>(input, range.first).m_Value);
            else {

                range.second = input.m_String.find(']', range.first);
                while (range.second != std::string::npos &&
                       Reserved::IsEscaped(input.m_String, range.second))
                    range.second = input.m_String.find(']', range.second + 1);

                if (range.second == std::string::npos) {
                    range.second = input.m_String.size();
                    throw Exception("Invalid vector argument missing closing ]",
                                    ARG_PARSE_SUBSTR(range));
                }

                input.m_String[range.second] = ' ';
                start = range.first;

                while (true) {

                    if ((range.first = input.NextPoi(range.first).first) >= range.second) {
                        start = range.first;
                        return;
                    }

                    m_Value.push_back(ArgumentParser<T>(input, start).m_Value);
                    range.first = start;
                }
            }
        }
    }

#define SUPPORT_TYPE(TYPE, TYPE_NAME)                                                              \
    template<>                                                                                     \
    struct is_supported_type<TYPE>                                                                 \
    {                                                                                              \
        static constexpr bool value = true;                                                        \
    };                                                                                             \
    template<>                                                                                     \
    struct ArgData<TYPE>                                                                           \
    {                                                                                              \
        explicit ArgData(String name) : m_Name(std::move(name)), m_Value() {}                      \
        const String m_Name;                                                                       \
        String m_TypeName = TYPE_NAME;                                                             \
        TYPE m_Value;                                                                              \
    };

    using NULL_ARGUMENT = void (*)();

    template<typename>
    struct is_supported_type
    {
        static constexpr bool value = false;
    };

    template<typename T>
    struct ArgData
    {

        explicit ArgData(String name) : m_Name(std::move(name)), m_Value() {}

        const String m_Name = "";
        String m_TypeName = "Unsupported Type";
        T m_Value;
    };

    SUPPORT_TYPE(String, "String")
    SUPPORT_TYPE(bool, "Boolean")
    SUPPORT_TYPE(char, "Char")
    SUPPORT_TYPE(unsigned char, "Unsigned_Char")
    SUPPORT_TYPE(short, "Signed_Short")
    SUPPORT_TYPE(unsigned short, "Unsigned_Short")
    SUPPORT_TYPE(int, "Signed_Int")
    SUPPORT_TYPE(unsigned int, "Unsigned_Int")
    SUPPORT_TYPE(long, "Signed_Long")
    SUPPORT_TYPE(unsigned long, "Unsigned_Long")
    SUPPORT_TYPE(long long, "Signed_Long_Long")
    SUPPORT_TYPE(unsigned long long, "Unsigned_Long_Long")
    SUPPORT_TYPE(float, "Float")
    SUPPORT_TYPE(double, "Double")
    SUPPORT_TYPE(long double, "Long_Double")

    template<typename U>
    struct is_supported_type<std::vector<U>>
    {
        static constexpr bool value = is_supported_type<U>::value;
    };
    template<typename T>
    struct ArgData<std::vector<T>>
    {

        explicit ArgData(String name) : m_Name(std::move(name)) {}

        const String m_Name;
        String m_TypeName = std::string("Vector_Of_") + ArgData<T>("").m_TypeName.m_String;
        std::vector<T> m_Value;
    };

    template<typename T>
    struct Arg
    {

        template<typename U>
        static constexpr bool is_supported_type_v = is_supported_type<U>::value;

    public:
        using ValueType = std::remove_cv_t<std::remove_reference_t<T>>;

        explicit Arg(const String &name) : m_Arg(name) {
            static_assert(is_supported_type_v<ValueType>,
                          "ValueType 'T' is not supported, see 'Supported types' for more help");
        }

        Arg<T> &Parse(String &input, size_t &start) {
            size_t index = start;

            if (input.NextPoi(index).first == input.End())
                throw Exception("Not enough arguments were given", input.m_String);

            m_Arg.m_Value = ArgumentParser<ValueType>(input, start).m_Value;
            return *this;
        }

        std::string Info() {
            return std::string(" [") + m_Arg.m_Name.m_String + ":" + m_Arg.m_TypeName.m_String +
                   "]";
        }

        ArgData<ValueType> m_Arg;
    };

    template<>
    struct Arg<NULL_ARGUMENT>
    {

        Arg<NULL_ARGUMENT> &Parse(String &input, size_t &start) {
            if (input.NextPoi(start).first != input.End())
                throw Exception("Too many arguments were given", input.m_String);
            return *this;
        }
    };
    static const char endl = '\n';

    enum ItemType {
        COMMAND = 0,
        LOG,
        WARNING,
        ERROR,
        INFO,
        NONE
    };

    struct Item
    {

        explicit Item(ItemType type = ItemType::LOG);

        Item(Item &&rhs) = default;

        Item(const Item &rhs) = default;

        Item &operator=(Item &&rhs) = default;

        Item &operator=(const Item &rhs) = default;

        Item &operator<<(std::string_view str);

        [[nodiscard]] std::string Get() const;

        ItemType m_Type;
        std::string m_Data;
        unsigned int m_TimeStamp;
    };

#define LOG_BASIC_TYPE_DECL(type) ItemLog &operator<<(type data)

    class ItemLog {
    public:
        ItemLog &log(ItemType type);

        ItemLog() = default;

        ItemLog(ItemLog &&rhs) = default;

        ItemLog(const ItemLog &rhs) = default;

        ItemLog &operator=(ItemLog &&rhs) = default;

        ItemLog &operator=(const ItemLog &rhs) = default;

        std::vector<Item> &Items();

        void Clear();

        LOG_BASIC_TYPE_DECL(int);
        LOG_BASIC_TYPE_DECL(long);
        LOG_BASIC_TYPE_DECL(float);
        LOG_BASIC_TYPE_DECL(double);
        LOG_BASIC_TYPE_DECL(long long);
        LOG_BASIC_TYPE_DECL(long double);
        LOG_BASIC_TYPE_DECL(unsigned int);
        LOG_BASIC_TYPE_DECL(unsigned long);
        LOG_BASIC_TYPE_DECL(unsigned long long);
        LOG_BASIC_TYPE_DECL(std::string_view);
        LOG_BASIC_TYPE_DECL(char);

    protected:
        std::vector<Item> m_Items;
    };

    METADOT_INLINE static const std::string_view s_Command = "> ";
    METADOT_INLINE static const std::string_view s_Warning = "\t[WARNING]: ";
    METADOT_INLINE static const std::string_view s_Error = "[ERROR]: ";
    METADOT_INLINE static const auto s_TimeBegin = std::chrono::steady_clock::now();

    METADOT_INLINE Item::Item(ItemType type) : m_Type(type) {
        auto timeNow = std::chrono::steady_clock::now();
        m_TimeStamp = static_cast<unsigned int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - s_TimeBegin)
                        .count());
    }

    METADOT_INLINE Item &Item::operator<<(const std::string_view str) {
        m_Data.append(str);
        return *this;
    }

    METADOT_INLINE std::string Item::Get() const {
        switch (m_Type) {
            case COMMAND:
                return s_Command.data() + m_Data;
            case LOG:
                return '\t' + m_Data;
            case WARNING:
                return s_Warning.data() + m_Data;
            case ERROR:
                return s_Error.data() + m_Data;
            case INFO:
                return m_Data;
            case NONE:
            default:
                return "";
        }
    }

#define LOG_BASIC_TYPE_DEF(type)                                                                   \
    METADOT_INLINE ItemLog &ItemLog::operator<<(type data) {                                       \
        m_Items.back() << std::to_string(data);                                                    \
        return *this;                                                                              \
    }

    METADOT_INLINE ItemLog &ItemLog::log(ItemType type) {

        m_Items.emplace_back(type);
        return *this;
    }

    METADOT_INLINE std::vector<Item> &ItemLog::Items() { return m_Items; }

    METADOT_INLINE void ItemLog::Clear() { m_Items.clear(); }

    METADOT_INLINE ItemLog &ItemLog::operator<<(const std::string_view data) {
        m_Items.back() << data;
        return *this;
    }

    METADOT_INLINE ItemLog &ItemLog::operator<<(const char data) {
        m_Items.back().m_Data.append(1, data);
        return *this;
    }

    LOG_BASIC_TYPE_DEF(int)
    LOG_BASIC_TYPE_DEF(long)
    LOG_BASIC_TYPE_DEF(float)
    LOG_BASIC_TYPE_DEF(double)
    LOG_BASIC_TYPE_DEF(long long)
    LOG_BASIC_TYPE_DEF(long double)
    LOG_BASIC_TYPE_DEF(unsigned int)
    LOG_BASIC_TYPE_DEF(unsigned long)
    LOG_BASIC_TYPE_DEF(unsigned long long)

    struct CommandBase
    {

        virtual ~CommandBase() = default;

        virtual Item operator()(String &input) = 0;

        [[nodiscard]] virtual std::string Help() = 0;

        [[nodiscard]] virtual size_t ArgumentCount() const = 0;

        [[nodiscard]] virtual CommandBase *Clone() const = 0;
    };

    template<typename Fn, typename... Args>
    class Command : public CommandBase {
    public:
        Command(String name, String description, Fn function, Args... args)
            : m_Name(std::move(name)), m_Description(std::move(description)), m_Function(function),
              m_Arguments(args..., Arg<NULL_ARGUMENT>()) {}

        Item operator()(String &input) final {
            try {

                constexpr int argumentSize = sizeof...(Args);
                Call(input, std::make_index_sequence<argumentSize + 1>{},
                     std::make_index_sequence<argumentSize>{});
            } catch (Exception &ae) { return Item(ERROR) << (m_Name.m_String + ": " + ae.what()); }
            return Item(NONE);
        }

        [[nodiscard]] std::string Help() final {
            return m_Name.m_String + DisplayArguments(std::make_index_sequence<sizeof...(Args)>{}) +
                   "\n\t\t- " + m_Description.m_String + "\n\n";
        }

        [[nodiscard]] size_t ArgumentCount() const final { return sizeof...(Args); }

        [[nodiscard]] CommandBase *Clone() const final { return new Command<Fn, Args...>(*this); }

    private:
        template<size_t... Is_p, size_t... Is_c>
        void Call(String &input, const std::index_sequence<Is_p...> &,
                  const std::index_sequence<Is_c...> &) {
            size_t start = 0;

            int _[]{0, (void(std::get<Is_p>(m_Arguments).Parse(input, start)), 0)...};
            (void) (_);

            m_Function((std::get<Is_c>(m_Arguments).m_Arg.m_Value)...);
        }

        template<size_t... Is>
        std::string DisplayArguments(const std::index_sequence<Is...> &) {
            return (std::get<Is>(m_Arguments).Info() + ...);
        }

        const String m_Name;
        const String m_Description;
        std::function<void(typename Args::ValueType...)> m_Function;
        std::tuple<Args..., Arg<NULL_ARGUMENT>> m_Arguments;
    };

    template<typename Fn>
    class Command<Fn> : public CommandBase {
    public:
        Command(String name, String description, Fn function)
            : m_Name(std::move(name)), m_Description(std::move(description)), m_Function(function),
              m_Arguments(Arg<NULL_ARGUMENT>()) {}

        Item operator()(String &input) final {

            size_t start = 0;
            try {

                std::get<0>(m_Arguments).Parse(input, start);
            } catch (Exception &ae) { return Item(ERROR) << (m_Name.m_String + ": " + ae.what()); }

            m_Function();
            return Item(NONE);
        }

        [[nodiscard]] std::string Help() final {
            return m_Name.m_String + "\n\t\t- " + m_Description.m_String + "\n\n";
        }

        [[nodiscard]] size_t ArgumentCount() const final { return 0; }

        [[nodiscard]] CommandBase *Clone() const final { return new Command<Fn>(*this); }

    private:
        const String m_Name;
        const String m_Description;
        std::function<void(void)> m_Function;
        std::tuple<Arg<NULL_ARGUMENT>> m_Arguments;
    };

    class AutoComplete {
    public:
        using r_sVector = std::vector<std::string> &;
        using sVector = std::vector<std::string>;

        struct ACNode
        {
            explicit ACNode(const char data, bool isWord = false)
                : m_Data(data), m_IsWord(isWord), m_Less(nullptr), m_Equal(nullptr),
                  m_Greater(nullptr){};

            explicit ACNode(const char &&data, bool isWord = false)
                : m_Data(data), m_IsWord(isWord), m_Less(nullptr), m_Equal(nullptr),
                  m_Greater(nullptr){};

            ~ACNode() {
                delete m_Less;
                delete m_Equal;
                delete m_Greater;
            };

            char m_Data;
            bool m_IsWord;
            ACNode *m_Less;
            ACNode *m_Equal;
            ACNode *m_Greater;
        };

        AutoComplete() = default;

        AutoComplete(const AutoComplete &tree);

        AutoComplete(AutoComplete &&rhs) = default;

        AutoComplete &operator=(const AutoComplete &rhs);

        AutoComplete &operator=(AutoComplete &&rhs) = default;

        template<typename inputType>
        AutoComplete(std::initializer_list<inputType> il) {
            for (const auto &item: il) { Insert(item); }
        }

        template<typename T>
        explicit AutoComplete(const T &items) {
            for (const auto &item: items) { Insert(item); }
        }

        ~AutoComplete();

        [[nodiscard]] size_t Size() const;

        [[nodiscard]] size_t Count() const;

        bool Search(const char *word);

        void Insert(const char *word);

        void Insert(const std::string &word);

        template<typename strType>
        void Insert(const strType &word) {
            ACNode **ptr = &m_Root;
            ++m_Count;

            while (*word != '\0') {

                if (*ptr == nullptr) {
                    *ptr = new ACNode(*word);
                    ++m_Size;
                }

                if (*word < (*ptr)->m_Data) {
                    ptr = &(*ptr)->m_Less;
                } else if (*word == (*ptr)->m_Data) {

                    if (*(word + 1) == '\0') {
                        if ((*ptr)->m_IsWord) --m_Count;

                        (*ptr)->m_IsWord = true;
                    }

                    ptr = &(*ptr)->m_Equal;
                    ++word;
                } else {
                    ptr = &(*ptr)->m_Greater;
                }
            }
        }

        void Remove(const std::string &word);

        template<typename strType>
        void Suggestions(const strType &prefix, r_sVector ac_options) {
            ACNode *ptr = m_Root;
            auto temp = prefix;

            while (ptr) {
                if (*prefix < ptr->m_Data) {
                    ptr = ptr->m_Less;
                } else if (*prefix == ptr->m_Data) {

                    if (*(prefix + 1) == '\0') break;

                    ptr = ptr->m_Equal;
                    ++prefix;
                } else {
                    ptr = ptr->m_Greater;
                }
            }

            if (ptr && ptr->m_IsWord) return;

            if (!ptr) return;

            SuggestionsAux(ptr->m_Equal, ac_options, temp);
        }

        void Suggestions(const char *prefix, r_sVector ac_options);

        std::string Suggestions(const std::string &prefix, r_sVector ac_options);

        void Suggestions(std::string &prefix, r_sVector ac_options, bool partial_complete);

        template<typename strType>
        std::unique_ptr<sVector> Suggestions(const strType &prefix) {
            auto temp = std::make_unique<sVector>();
            Suggestions(prefix, *temp);
            return temp;
        }

        std::unique_ptr<sVector> Suggestions(const char *prefix);

    protected:
        void SuggestionsAux(ACNode *root, r_sVector ac_options, std::string buffer);

        bool RemoveAux(ACNode *root, const char *word);

        void DeepClone(ACNode *src, ACNode *&dest);

        ACNode *m_Root = nullptr;
        size_t m_Size = 0;
        size_t m_Count = 0;
    };

    METADOT_INLINE AutoComplete::~AutoComplete() { delete m_Root; }

    METADOT_INLINE AutoComplete::AutoComplete(const AutoComplete &tree)
        : m_Size(tree.m_Size), m_Count(tree.m_Count) {
        DeepClone(tree.m_Root, m_Root);
    }

    METADOT_INLINE AutoComplete &AutoComplete::operator=(const AutoComplete &rhs) {

        if (&rhs == this) return *this;

        delete m_Root;

        DeepClone(rhs.m_Root, m_Root);
        m_Size = rhs.m_Size;
        m_Count = rhs.m_Count;

        return *this;
    }

    METADOT_INLINE size_t AutoComplete::Size() const { return m_Size; }

    METADOT_INLINE size_t AutoComplete::Count() const { return m_Count; }

    METADOT_INLINE bool AutoComplete::Search(const char *word) {
        ACNode *ptr = m_Root;

        while (ptr) {
            if (*word < ptr->m_Data) {
                ptr = ptr->m_Less;
            } else if (*word == ptr->m_Data) {

                if (*(word + 1) == '\0' && ptr->m_IsWord) return true;

                ptr = ptr->m_Equal;
                ++word;
            } else {
                ptr = ptr->m_Greater;
            }
        }

        return false;
    }

    METADOT_INLINE void AutoComplete::Insert(const char *word) {
        ACNode **ptr = &m_Root;
        ++m_Count;

        while (*word != '\0') {

            if (*ptr == nullptr) {
                *ptr = new ACNode(*word);
                ++m_Size;
            }

            if (*word < (*ptr)->m_Data) {
                ptr = &(*ptr)->m_Less;
            } else if (*word == (*ptr)->m_Data) {

                if (*(word + 1) == '\0') {
                    if ((*ptr)->m_IsWord) --m_Count;

                    (*ptr)->m_IsWord = true;
                }

                ptr = &(*ptr)->m_Equal;
                ++word;
            } else {
                ptr = &(*ptr)->m_Greater;
            }
        }
    }

    METADOT_INLINE void AutoComplete::Insert(const std::string &word) { Insert(word.c_str()); }

    METADOT_INLINE void AutoComplete::Remove(const std::string &word) {
        RemoveAux(m_Root, word.c_str());
    }

    METADOT_INLINE void AutoComplete::Suggestions(const char *prefix,
                                                  std::vector<std::string> &ac_options) {
        ACNode *ptr = m_Root;
        auto temp = prefix;

        while (ptr) {
            if (*prefix < ptr->m_Data) {
                ptr = ptr->m_Less;
            } else if (*prefix == ptr->m_Data) {

                if (*(prefix + 1) == '\0') break;

                ptr = ptr->m_Equal;
                ++prefix;
            } else {
                ptr = ptr->m_Greater;
            }
        }

        if (ptr && ptr->m_IsWord) return;

        if (!ptr) return;

        SuggestionsAux(ptr->m_Equal, ac_options, temp);
    }

    METADOT_INLINE std::string AutoComplete::Suggestions(const std::string &prefix,
                                                         r_sVector &ac_options) {
        std::string temp = prefix;
        Suggestions(temp, ac_options, true);
        return temp;
    }

    METADOT_INLINE void AutoComplete::Suggestions(std::string &prefix, r_sVector ac_options,
                                                  bool partial_complete) {
        ACNode *ptr = m_Root;
        const char *temp = prefix.data();
        size_t prefix_end = prefix.size();

        while (ptr) {
            if (*temp < ptr->m_Data) {
                ptr = ptr->m_Less;
            } else if (*temp == ptr->m_Data) {

                if (*(temp + 1) == '\0') {
                    if (partial_complete) {
                        ACNode *pc_ptr = ptr->m_Equal;

                        while (pc_ptr) {
                            if (pc_ptr->m_Equal && !pc_ptr->m_Less && !pc_ptr->m_Greater)
                                prefix.push_back(pc_ptr->m_Data);
                            else
                                break;

                            pc_ptr = pc_ptr->m_Equal;
                        }
                    }

                    break;
                }

                ptr = ptr->m_Equal;
                ++temp;
            } else {
                ptr = ptr->m_Greater;
            }
        }

        if (ptr && ptr->m_IsWord) return;

        if (!ptr) return;

        SuggestionsAux(ptr->m_Equal, ac_options, prefix.substr(0, prefix_end));
    }

    METADOT_INLINE std::unique_ptr<AutoComplete::sVector> AutoComplete::Suggestions(
            const char *prefix) {
        auto temp = std::make_unique<sVector>();
        Suggestions(prefix, *temp);
        return temp;
    }

    METADOT_INLINE void AutoComplete::SuggestionsAux(AutoComplete::ACNode *root,
                                                     r_sVector ac_options, std::string buffer) {
        if (!root) return;

        if (root->m_Less) SuggestionsAux(root->m_Less, ac_options, buffer);

        if (root->m_IsWord) {
            ac_options.push_back(buffer.append(1, root->m_Data));
            buffer.pop_back();
        }

        if (root->m_Equal) {
            SuggestionsAux(root->m_Equal, ac_options, buffer.append(1, root->m_Data));
            buffer.pop_back();
        }

        if (root->m_Greater) { SuggestionsAux(root->m_Greater, ac_options, buffer); }
    }

    METADOT_INLINE bool AutoComplete::RemoveAux(AutoComplete::ACNode *root, const char *word) {
        if (!root) return false;

        if (*(word + 1) == '\0' && root->m_Data == *word) {

            if (root->m_IsWord) {
                root->m_IsWord = false;
                return (!root->m_Equal && !root->m_Less && !root->m_Greater);
            }

            else
                return false;
        } else {

            if (*word < root->m_Data) RemoveAux(root->m_Less, word);
            else if (*word > root->m_Data)
                RemoveAux(root->m_Greater, word);

            else if (*word == root->m_Data) {

                if (RemoveAux(root->m_Equal, word + 1)) {
                    delete root->m_Equal;
                    root->m_Equal = nullptr;
                    return !root->m_IsWord && (!root->m_Equal && !root->m_Less && !root->m_Greater);
                }
            }
        }

        return false;
    }

    METADOT_INLINE void AutoComplete::DeepClone(AutoComplete::ACNode *src,
                                                AutoComplete::ACNode *&dest) {
        if (!src) return;

        dest = new ACNode(*src);
        DeepClone(src->m_Less, dest->m_Less);
        DeepClone(src->m_Equal, dest->m_Equal);
        DeepClone(src->m_Greater, dest->m_Greater);
    }

    class CommandHistory {
    public:
        explicit CommandHistory(unsigned int maxRecord = 100);

        CommandHistory(CommandHistory &&rhs) = default;

        CommandHistory(const CommandHistory &rhs) = default;

        CommandHistory &operator=(CommandHistory &&rhs) = default;

        CommandHistory &operator=(const CommandHistory &rhs) = default;

        void PushBack(const std::string &line);

        [[nodiscard]] unsigned int GetNewIndex() const;

        const std::string &GetNew();

        [[nodiscard]] unsigned int GetOldIndex() const;

        const std::string &GetOld();

        void Clear();

        const std::string &operator[](size_t index);

        friend std::ostream &operator<<(std::ostream &os, const CommandHistory &history);

        size_t Size();

        size_t Capacity();

    protected:
        unsigned int m_Record;
        unsigned int m_MaxRecord;
        std::vector<std::string> m_History;
    };

    METADOT_INLINE CommandHistory::CommandHistory(unsigned int maxRecord)
        : m_Record(0), m_MaxRecord(maxRecord), m_History(maxRecord) {}

    METADOT_INLINE void CommandHistory::PushBack(const std::string &line) {
        m_History[m_Record++ % m_MaxRecord] = line;
    }

    METADOT_INLINE unsigned int CommandHistory::GetNewIndex() const {
        return (m_Record - 1) % m_MaxRecord;
    }

    METADOT_INLINE const std::string &CommandHistory::GetNew() {
        return m_History[(m_Record - 1) % m_MaxRecord];
    }

    METADOT_INLINE unsigned int CommandHistory::GetOldIndex() const {
        if (m_Record <= m_MaxRecord) return 0;
        else
            return m_Record % m_MaxRecord;
    }

    METADOT_INLINE const std::string &CommandHistory::GetOld() {
        if (m_Record <= m_MaxRecord) return m_History.front();
        else
            return m_History[m_Record % m_MaxRecord];
    }

    METADOT_INLINE void CommandHistory::Clear() { m_Record = 0; }

    METADOT_INLINE const std::string &CommandHistory::operator[](size_t index) {
        return m_History[index];
    }

    METADOT_INLINE std::ostream &operator<<(std::ostream &os, const CommandHistory &history) {
        os << "History: " << '\n';
        for (unsigned int i = 0; i < history.m_Record && history.m_Record <= history.m_MaxRecord;
             ++i)
            std::cout << history.m_History[i] << '\n';
        return os;
    }

    METADOT_INLINE size_t CommandHistory::Size() {
        return m_Record < m_MaxRecord ? m_Record : m_MaxRecord;
    }

    METADOT_INLINE size_t CommandHistory::Capacity() { return m_History.capacity(); }

    class Script {
    public:
        explicit Script(std::string path, bool load_on_init = true);

        explicit Script(const char *path, bool load_on_init = true);

        explicit Script(std::vector<std::string> data);

        Script(Script &&rhs) = default;

        Script(const Script &rhs) = default;

        Script &operator=(Script &&rhs) = default;

        Script &operator=(const Script &rhs) = default;

        void Load();

        void Reload();

        void Unload();

        void SetPath(std::string path);

        const std::vector<std::string> &Data();

    protected:
        std::vector<std::string> m_Data;
        std::string m_Path;
        bool m_FromMemory;
    };

    METADOT_INLINE Script::Script(std::string path, bool load_on_init)
        : m_Path(std::move(path)), m_FromMemory(false) {

        if (load_on_init) Load();
    }

    METADOT_INLINE Script::Script(const char *path, bool load_on_init)
        : m_Path(path), m_FromMemory(false) {

        if (load_on_init) Load();
    }

    METADOT_INLINE Script::Script(std::vector<std::string> data)
        : m_Data(std::move(data)), m_FromMemory(true) {}

    METADOT_INLINE void Script::Load() {
        std::ifstream script_fstream(m_Path);

        if (!script_fstream.good()) throw ConsoleImpl::Exception("Failed to load script", m_Path);

        if (script_fstream.good() && script_fstream.is_open()) {

            std::string line_buf;

            while (std::getline(script_fstream, line_buf)) { m_Data.emplace_back(line_buf); }

            script_fstream.close();
        }
    }

    METADOT_INLINE void Script::Reload() {
        if (m_FromMemory) return;

        Unload();
        Load();
    }

    METADOT_INLINE void Script::Unload() { m_Data.clear(); }

    METADOT_INLINE void Script::SetPath(std::string path) { m_Path = std::move(path); }

    METADOT_INLINE const std::vector<std::string> &Script::Data() { return m_Data; }

    class System {
    public:
        System();

        System(System &&rhs) = default;

        System(const System &rhs);

        System &operator=(System &&rhs) = default;

        System &operator=(const System &rhs);

        void RunCommand(const std::string &line);

        AutoComplete &CmdAutocomplete();

        AutoComplete &VarAutocomplete();

        CommandHistory &History();

        std::vector<Item> &Items();

        ItemLog &Log(ItemType type = ItemType::LOG);

        void RunScript(const std::string &script_name);

        std::unordered_map<std::string, std::unique_ptr<CommandBase>> &Commands();

        std::unordered_map<std::string, std::unique_ptr<Script>> &Scripts();

        template<typename Fn, typename... Args>
        void RegisterCommand(const String &name, const String &description, Fn function,
                             Args... args) {

            static_assert(std::is_invocable_v<Fn, typename Args::ValueType...>,
                          "Arguments specified do not match that of the function");
            static_assert(!std::is_member_function_pointer_v<Fn>,
                          "Non-static member functions are not allowed");

            size_t name_index = 0;
            auto range = name.NextPoi(name_index);

            if (m_Commands.find(name.m_String) != m_Commands.end())
                throw ConsoleImpl::Exception("ERROR: Command already exists");

            else if (range.first == name.End()) {
                Log(ERROR) << "Empty command name given" << ConsoleImpl::endl;
                return;
            }

            std::string command_name =
                    name.m_String.substr(range.first, range.second - range.first);

            if (name.NextPoi(name_index).first != name.End())
                throw ConsoleImpl::Exception(
                        "ERROR: Whitespace separated command names are forbidden");

            if (m_RegisterCommandSuggestion) {
                m_CommandSuggestionTree.Insert(command_name);
                m_VariableSuggestionTree.Insert(command_name);
            }

            m_Commands[name.m_String] =
                    std::make_unique<Command<Fn, Args...>>(name, description, function, args...);

            auto help = [this, command_name]() {
                Log(LOG) << m_Commands[command_name]->Help() << ConsoleImpl::endl;
            };

            m_Commands["help " + command_name] = std::make_unique<Command<decltype(help)>>(
                    "help " + command_name, "Displays help info about command " + command_name,
                    help);
        }

        template<typename T, typename... Types>
        void RegisterVariable(const String &name, T &var, Arg<Types>... args) {
            static_assert(std::is_constructible_v<T, Types...>,
                          "Type of var 'T' can not be constructed with types of 'Types'");
            static_assert(sizeof...(Types) != 0, "Empty variadic list");

            auto var_name = RegisterVariableAux(name, var);

            auto setter = [&var](Types... params) { var = T(params...); };
            m_Commands["set " + var_name] =
                    std::make_unique<Command<decltype(setter), Arg<Types>...>>(
                            "set " + var_name, "Sets the variable " + var_name, setter, args...);
        }

        template<typename T, typename... Types>
        void RegisterVariable(const String &name, T &var, void (*setter)(T &, Types...)) {

            auto var_name = RegisterVariableAux(name, var);

            auto setter_l = [&var, setter](Types... args) { setter(var, args...); };
            m_Commands["set " + var_name] =
                    std::make_unique<Command<decltype(setter_l), Arg<Types>...>>(
                            "set " + var_name, "Sets the variable " + var_name, setter_l,
                            Arg<Types>("")...);
        }

        void RegisterScript(const std::string &name, const std::string &path);

        void UnregisterCommand(const std::string &cmd_name);

        void UnregisterVariable(const std::string &var_name);

        void UnregisterScript(const std::string &script_name);

    protected:
        template<typename T>
        std::string RegisterVariableAux(const String &name, T &var) {

            m_RegisterCommandSuggestion = false;

            size_t name_index = 0;
            auto range = name.NextPoi(name_index);
            if (name.NextPoi(name_index).first != name.End())
                throw ConsoleImpl::Exception(
                        "ERROR: Whitespace separated variable names are forbidden");

            std::string var_name = name.m_String.substr(range.first, range.second - range.first);

            const auto GetFunction = [this, &var]() { m_ItemLog.log(LOG) << var << endl; };

            m_Commands["get " + var_name] = std::make_unique<Command<decltype(GetFunction)>>(
                    "get " + var_name, "Gets the variable " + var_name, GetFunction);

            m_RegisterCommandSuggestion = true;

            m_VariableSuggestionTree.Insert(var_name);

            return var_name;
        }

        void ParseCommandLine(const String &line);

        std::unordered_map<std::string, std::unique_ptr<CommandBase>> m_Commands;
        AutoComplete m_CommandSuggestionTree;
        AutoComplete m_VariableSuggestionTree;
        CommandHistory m_CommandHistory;
        ItemLog m_ItemLog;
        std::unordered_map<std::string, std::unique_ptr<Script>> m_Scripts;
        bool m_RegisterCommandSuggestion = true;
    };

    static const std::string_view s_Set = "set";
    static const std::string_view s_Get = "get";
    static const std::string_view s_Help = "help";
    static const std::string_view s_ErrorNoVar = "No variable provided";
    static const std::string_view s_ErrorSetGetNotFound = "命令不存在/变量未注册";

    METADOT_INLINE System::System() {

        RegisterCommand(s_Help.data(), "指令信息", [this]() {
            Log() << "help [command_name:String] (Optional)\n\t\t- Display command(s) information\n"
                  << ConsoleImpl::endl;
            Log() << "set [variable_name:String] [data]\n\t\t- Assign data to given variable\n"
                  << ConsoleImpl::endl;
            Log() << "get [variable_name:String]\n\t\t- Display data of given variable\n"
                  << ConsoleImpl::endl;

            for (const auto &tuple: Commands()) {

                if (tuple.first.size() >= 5 && (tuple.first[3] == ' ' || tuple.first[4] == ' '))
                    continue;

                if (tuple.first.size() == 4 && (tuple.first == "help")) continue;

                Log() << tuple.second->Help();
            }
        });

        m_CommandSuggestionTree.Insert(s_Set.data());
        m_CommandSuggestionTree.Insert(s_Get.data());
    }

    METADOT_INLINE System::System(const System &rhs)
        : m_CommandSuggestionTree(rhs.m_CommandSuggestionTree),
          m_VariableSuggestionTree(rhs.m_VariableSuggestionTree),
          m_CommandHistory(rhs.m_CommandHistory), m_ItemLog(rhs.m_ItemLog),
          m_RegisterCommandSuggestion(rhs.m_RegisterCommandSuggestion) {

        for (const auto &pair: rhs.m_Commands) {
            m_Commands[pair.first] = std::unique_ptr<CommandBase>(pair.second->Clone());
        }

        for (const auto &pair: rhs.m_Scripts) {
            m_Scripts[pair.first] = std::make_unique<Script>(*pair.second);
        }
    }

    METADOT_INLINE System &System::operator=(const System &rhs) {
        if (this == &rhs) return *this;

        for (const auto &pair: rhs.m_Commands) {
            m_Commands[pair.first] = std::unique_ptr<CommandBase>(pair.second->Clone());
        }

        m_CommandSuggestionTree = rhs.m_CommandSuggestionTree;
        m_VariableSuggestionTree = rhs.m_VariableSuggestionTree;
        m_CommandHistory = rhs.m_CommandHistory;
        m_ItemLog = rhs.m_ItemLog;

        for (const auto &pair: rhs.m_Scripts) {
            m_Scripts[pair.first] = std::make_unique<Script>(*pair.second);
        }

        m_RegisterCommandSuggestion = rhs.m_RegisterCommandSuggestion;

        return *this;
    }

    METADOT_INLINE void System::RunCommand(const std::string &line) {

        if (line.empty()) return;

        Log(ConsoleImpl::ItemType::COMMAND) << line << ConsoleImpl::endl;

        ParseCommandLine(line);
    }

    METADOT_INLINE void System::RunScript(const std::string &script_name) {

        auto script_pair = m_Scripts.find(script_name);

        if (script_pair == m_Scripts.end()) {
            m_ItemLog.log(ERROR) << "Script \"" << script_name << "\" not found"
                                 << ConsoleImpl::endl;
            return;
        }

        m_ItemLog.log(INFO) << "Running \"" << script_name << "\"" << ConsoleImpl::endl;

        if (script_pair->second->Data().empty()) {
            try {
                script_pair->second->Load();
            } catch (ConsoleImpl::Exception &e) { Log(ERROR) << e.what() << ConsoleImpl::endl; }
        }

        for (const auto &cmd: script_pair->second->Data()) { RunCommand(cmd); }
    }

    METADOT_INLINE void System::RegisterScript(const std::string &name, const std::string &path) {

        auto script = m_Scripts.find(name);

        if (script == m_Scripts.end()) {
            m_Scripts[name] = std::make_unique<Script>(path, true);
            m_VariableSuggestionTree.Insert(name);
        } else
            throw ConsoleImpl::Exception("ERROR: Script \'" + name + "\' already registered");
    }

    METADOT_INLINE void System::UnregisterCommand(const std::string &cmd_name) {

        if (cmd_name.empty()) return;

        auto command_it = m_Commands.find(cmd_name);
        auto help_command_it = m_Commands.find("help " + cmd_name);

        if (command_it != m_Commands.end() && help_command_it != m_Commands.end()) {
            m_CommandSuggestionTree.Remove(cmd_name);
            m_VariableSuggestionTree.Remove(cmd_name);

            m_Commands.erase(command_it);
            m_Commands.erase(help_command_it);
        }
    }

    METADOT_INLINE void System::UnregisterVariable(const std::string &var_name) {

        if (var_name.empty()) return;

        auto s_it = m_Commands.find("set " + var_name);
        auto g_it = m_Commands.find("get " + var_name);

        if (s_it != m_Commands.end() && g_it != m_Commands.end()) {
            m_VariableSuggestionTree.Remove(var_name);
            m_Commands.erase(s_it);
            m_Commands.erase(g_it);
        }
    }

    METADOT_INLINE void System::UnregisterScript(const std::string &script_name) {

        if (script_name.empty()) return;

        auto it = m_Scripts.find(script_name);

        if (it != m_Scripts.end()) {
            m_VariableSuggestionTree.Remove(script_name);
            m_Scripts.erase(it);
        }
    }

    METADOT_INLINE AutoComplete &System::CmdAutocomplete() { return m_CommandSuggestionTree; }

    METADOT_INLINE AutoComplete &System::VarAutocomplete() { return m_VariableSuggestionTree; }

    METADOT_INLINE CommandHistory &System::History() { return m_CommandHistory; }

    METADOT_INLINE std::vector<Item> &System::Items() { return m_ItemLog.Items(); }

    METADOT_INLINE ItemLog &System::Log(ItemType type) { return m_ItemLog.log(type); }

    METADOT_INLINE std::unordered_map<std::string, std::unique_ptr<CommandBase>>
            &System::Commands() {
        return m_Commands;
    }

    METADOT_INLINE std::unordered_map<std::string, std::unique_ptr<Script>> &System::Scripts() {
        return m_Scripts;
    }

    METADOT_INLINE void System::ParseCommandLine(const String &line) {

        size_t line_index = 0;
        std::pair<size_t, size_t> range = line.NextPoi(line_index);

        if (range.first == line.End()) return;

        m_CommandHistory.PushBack(line.m_String);

        std::string command_name = line.m_String.substr(range.first, range.second - range.first);

        bool is_cmd_set = command_name == s_Set;
        bool is_cmd_get = command_name == s_Get;
        bool is_cmd_help = !(is_cmd_set || is_cmd_get) ? command_name == s_Help : false;

        if (is_cmd_help) {
            range = line.NextPoi(line_index);
            if (range.first != line.End())
                command_name += " " + line.m_String.substr(range.first, range.second - range.first);
        }

        else if (is_cmd_set || is_cmd_get) {

            if ((range = line.NextPoi(line_index)).first == line.End()) {
                Log(ERROR) << s_ErrorNoVar << endl;
                return;
            } else

                command_name += " " + line.m_String.substr(range.first, range.second - range.first);
        }

        auto command = m_Commands.find(command_name);
        if (command == m_Commands.end()) Log(ERROR) << s_ErrorSetGetNotFound << endl;

        else {

            String arguments =
                    line.m_String.substr(range.second, line.m_String.size() - range.first);

            auto cmd_out = (*command->second)(arguments);

            if (cmd_out.m_Type != NONE) m_ItemLog.Items().emplace_back(cmd_out);
        }
    }
}// namespace ConsoleImpl

#endif
