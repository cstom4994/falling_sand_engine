// Copyright(c) 2023, KaoruXun All rights reserved.

#ifndef ME_UTILS_HPP
#define ME_UTILS_HPP

#include <algorithm>
#include <atomic>
#include <bitset>
#include <cassert>
#include <chrono>
#include <cmath>
#include <codecvt>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/core/core.hpp"
#include "engine/core/mathlib.hpp"
#include "engine/meta/static_relfection.hpp"

struct lua_State;

namespace ME {

template <typename T>
std::string to_str(T x) {
    if constexpr (std::is_same<T, std::string>::value)
        return x;
    else if constexpr (std::is_same<T, const char *>::value)
        return std::string(x);
    else
        return std::to_string(x);
}

template <typename T>
std::string str(T x) {
    return to_str(x);
}
template <typename T1, typename... T>
std::string str(T1 x, T... args) {
    return to_str(x) + str(args...);
}

enum [[maybe_unused]] log_colour { ME_LOG_COLOUR_GREEN = 0, ME_LOG_COLOUR_YELLOW = 1, ME_LOG_COLOUR_RED = 2, ME_LOG_COLOUR_WHITE = 3, ME_LOG_COLOUR_BLUE = 4, ME_LOG_COLOUR_NULL = 5 };

enum log_type { ME_LOG_TYPE_WARNING = 1, ME_LOG_TYPE_ERROR = 2, ME_LOG_TYPE_NOTE = 4, ME_LOG_TYPE_SUCCESS = 0, ME_LOG_TYPE_MESSAGE = 3 };

enum [[maybe_unused]] log_operations {
    // This is the default operation
    ME_LOG_OPERATION_TERMINAL,
    ME_LOG_OPERATION_FILE,
    ME_LOG_OPERATION_FILE_AND_TERMINAL,
};

// The offset by which the names of the given message types are listed in the logColours array below
constexpr uint8_t logTypeOffset = 6;

// A constexpr list of strings, elements 0-5 are escape codes for colours, 6-11 are the messages used for the given
// escape colours
constexpr const char *logColours[] = {"\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[37m", "\x1b[34m", "\x1b[0m", "Success", "Warning", "Error", "Message", "Debug", "Null"};

struct log_msg {
    std::string msg;
    log_type type;
    i64 time;
};

class LoggerInternal {
public:
    LoggerInternal() noexcept;
    ~LoggerInternal() noexcept;

private:
    friend class Logger;
    friend class MEconsole;

    static void writeline(std::string &msg);

    template <bool bFile, typename... args>
    void log(const char *message, log_type type, args &&...argv) noexcept {
        std::string output;
        bool bError = false;
        if (type == ME_LOG_TYPE_ERROR && using_errors) bError = true;

        output = std::string(logColours[type + logTypeOffset]) + ": " + message;
        std::stringstream ss;
        (ss << ... << argv);
        output += ss.str();
        output += "\n";

        // if constexpr (bFile)
        //     fileout << output << std::endl;
        // else
        //     std::cout << logColours[type] << output << logColours[logTypeOffset - 1] << std::endl;

        writeline(output);

        message_log.emplace_back(log_msg{output, type, ME_gettime()});

        if (bError) {
#ifdef NO_INSTANT_CRASH
            std::cin.get();
#endif
            std::terminate();
        }
    }

    std::ofstream fileout;
    bool using_errors = false;
    std::vector<log_msg> message_log;

    log_operations operation_type = ME_LOG_OPERATION_TERMINAL;

    static std::string get_current_time() noexcept;
    void shutdown_file_stream() noexcept;
};

inline LoggerInternal loggerInternal;

class Logger {
public:
    static void set_crash_on_error(bool bError) noexcept;

    static void set_current_log_file(const char *file) noexcept;

    static void set_log_operation(log_operations op) noexcept;

    template <typename... args>
    static void log(log_type type, const char *message, args &&...argv) noexcept {
        if (loggerInternal.operation_type == ME_LOG_OPERATION_FILE_AND_TERMINAL) {
            loggerInternal.log<false>(message, type, argv...);
            loggerInternal.log<true>(message, type, argv...);
        } else if (loggerInternal.operation_type == ME_LOG_OPERATION_TERMINAL)
            loggerInternal.log<false>(message, type, argv...);
        else
            loggerInternal.log<true>(message, type, argv...);
    }
};

ME_INLINE auto ME_time_to_string(std::time_t now = std::time(nullptr)) -> std::string {
    const auto tp = std::localtime(&now);
    char buffer[32];
    return std::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", tp) ? buffer : "1970-01-01_00:00:00";
}

class Timer {
public:
    void start() noexcept;
    void stop() noexcept;
    [[nodiscard]] double get() const noexcept;
    ~Timer() noexcept;

private:
    double duration = 0;

    std::chrono::time_point<std::chrono::high_resolution_clock> startPos;
};

template <typename T>
ME_INLINE void println(T res) {
    std::cout << res << std::endl;
}

template <typename... Args>
ME_INLINE void println(std::string_view fmt, Args &&...args) {
    auto res = std::format(fmt, std::forward<Args>(args)...);
    println(res);
}

template <typename... Args>
void println(const Args &...args) {
    (std::cout << ... << args) << std::endl;
}

}  // namespace ME

template <typename T>
class MoveOnly {
public:
    static_assert(std::is_default_constructible_v<T>);
    static_assert(std::is_trivially_copy_constructible_v<T>);
    static_assert(std::is_trivially_copy_assignable_v<T>);
    static_assert(std::is_trivially_move_constructible_v<T>);
    static_assert(std::is_trivially_move_assignable_v<T>);
    static_assert(std::is_swappable_v<T>);

    MoveOnly() = default;

    ~MoveOnly() = default;

    explicit MoveOnly(const T &val) noexcept { m_value = val; }

    MoveOnly(const MoveOnly &) = delete;

    MoveOnly &operator=(const MoveOnly &) = delete;

    MoveOnly(MoveOnly &&other) noexcept { m_value = std::exchange(other.m_value, T{}); }

    MoveOnly &operator=(MoveOnly &&other) noexcept {
        std::swap(m_value, other.m_value);
        return *this;
    }

    operator const T &() const {  // NOLINT(google-explicit-constructor)
        return m_value;
    }

    MoveOnly &operator=(const T &val) {
        m_value = val;
        return *this;
    }

    // MoveOnly<T> has the same memory layout as T
    // So it's perfectly safe to overload operator&
    T *operator&()  // NOLINT(google-runtime-operator)
    {
        return &m_value;
    }

    const T *operator&() const {  // NOLINT(google-runtime-operator)
        return &m_value;
    }

    T *operator->() { return &m_value; }

    const T *operator->() const { return &m_value; }

    bool operator==(const T &val) const { return m_value == val; }

    bool operator!=(const T &val) const { return m_value != val; }

private:
    T m_value{};
};

static_assert(sizeof(int) == sizeof(MoveOnly<int>));

#define MOVE_ONLY(T)                  \
    T(const T &) = delete;            \
    T &operator=(const T &) = delete; \
    T(T &&) noexcept = default;       \
    T &operator=(T &&) noexcept = default;

#define NO_MOVE_OR_COPY(T)            \
    T(const T &) = delete;            \
    T &operator=(const T &) = delete; \
    T(T &&) = delete;                 \
    T &operator=(T &&) = delete;

//--------------------------------------------------------------------------------------------------------------------------------//
// LOGGING FUNCTIONS

#if defined(ME_DEBUG)
#define METADOT_BUG(...) ME::Logger::log(ME::ME_LOG_TYPE_NOTE, std::format("[Native] {0}:{1} ", __func__, __LINE__).c_str(), __VA_ARGS__)
#else
#define METADOT_BUG(...)
#endif
#define METADOT_TRACE(...) ME::Logger::log(ME::ME_LOG_TYPE_NOTE, __VA_ARGS__)
#define METADOT_INFO(...) ME::Logger::log(ME::ME_LOG_TYPE_MESSAGE, __VA_ARGS__)
#define METADOT_WARN(...) ME::Logger::log(ME::ME_LOG_TYPE_WARNING, __VA_ARGS__)
#define METADOT_ERROR(...) ME::Logger::log(ME::ME_LOG_TYPE_ERROR, __VA_ARGS__)

#define METADOT_LOG_SCOPE_FUNCTION(...)
#define METADOT_LOG_SCOPE_F(...)

template <typename... Args>
bool DebugCheck(const bool succeeded, const char *failMessage, Args... args) {
    if (succeeded) return true;

    ME_BUG(failMessage, args...);
    return false;
}

template <typename... Args>
void DebugCheckCritical(const bool succeeded, const char *failMessage, Args... args) {
    if (DebugCheck(succeeded, failMessage, args...)) return;

    exit(EXIT_FAILURE);
}

namespace ME {

template <class T>
class singleton {
public:
    virtual ~singleton() {}

    static T *get_singleton_ptr() {
        if (this_instance.get() == NULL) {
            ME::ref<T> t(new T);
            this_instance = t;
        }
        return this_instance.get();
    }

    static T &get_singleton() { return (*get_singleton_ptr()); }

    static void clean() {
        ME::ref<T> t;
        this_instance = t;
    }

protected:
    singleton() {}

    static ME::ref<T> this_instance;
};

template <typename T>
ME::ref<T> singleton<T>::this_instance;

//=============================================================================

template <typename T>
class static_singleton {
public:
    virtual ~static_singleton() {}

    static T *get_singleton_ptr() {
        static T *this_instance = 0;
        if (this_instance == 0) this_instance = new T;

        return this_instance;
    }

    static T &get_singleton() { return (*get_singleton_ptr()); }

protected:
    static_singleton() {}
};

template <typename T>
class singleton_ptr {
public:
    singleton_ptr() {}
    ~singleton_ptr() {}

    static T *get_singleton_ptr() {
        if (this_instance.get() == NULL) {
            ME::ref<T> t(::new T);
            this_instance = t;
        }

        return this_instance.get();
    }

    static T &get_singleton() { return (*get_singleton_ptr()); }

    static void Delete() {
        ME::ref<T> t;
        this_instance = t;
    }

    T *operator->() const { return get_singleton_ptr(); }

    T &operator*() const { return get_singleton(); }

private:
    static ME::ref<T> this_instance;
};

template <typename T>
ME::ref<T> singleton_ptr<T>::this_instance;

template <typename T>
inline T *get_singleton_ptr() {
    return singleton_ptr<T>::get_singleton_ptr();
}

struct pointer_sorter {
    template <class T>
    bool operator()(const T *a, const T *b) const {
        if (a == 0) {
            return b != 0;
        } else if (b == 0) {
            return false;
        } else {
            return (*a) < (*b);
        }
    }

    template <class T>
    bool operator()(const T *a, const T *b) {
        if (a == 0) {
            return b != 0;
        } else if (b == 0) {
            return false;
        } else {
            return (*a) < (*b);
        }
    }
};

//-----------------------------------------------------------------------------

// thread pool

template <typename R>
bool future_is_ready(std::future<R> const &f) {
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

template <typename T>
class thread_pool_queue {
public:
    bool push(T const &value) {
        std::unique_lock<std::mutex> lock(this->mutex);
        this->q.push(value);
        return true;
    }
    // deletes the retrieved element, do not use for non integral types
    bool pop(T &v) {
        std::unique_lock<std::mutex> lock(this->mutex);
        if (this->q.empty()) return false;
        v = this->q.front();
        this->q.pop();
        return true;
    }
    bool empty() {
        std::unique_lock<std::mutex> lock(this->mutex);
        return this->q.empty();
    }

private:
    std::queue<T> q;
    std::mutex mutex;
};

class thread_pool {

public:
    thread_pool() { this->init(); }
    thread_pool(int nThreads) {
        this->init();
        this->resize(nThreads);
    }

    // the destructor waits for all the functions in the queue to be finished
    ~thread_pool() { this->stop(true); }

    // get the number of running threads in the pool
    int size() { return static_cast<int>(this->threads.size()); }

    // number of idle threads
    int n_idle() { return this->nWaiting; }
    std::thread &get_thread(int i) { return *this->threads[i]; }

    // change the number of threads in the pool
    // should be called from one thread, otherwise be careful to not interleave, also with this->stop()
    // nThreads must be >= 0
    void resize(int nThreads) {
        if (!this->isStop && !this->isDone) {
            int oldNThreads = static_cast<int>(this->threads.size());
            if (oldNThreads <= nThreads) {  // if the number of threads is increased
                this->threads.resize(nThreads);
                this->flags.resize(nThreads);

                for (int i = oldNThreads; i < nThreads; ++i) {
                    this->flags[i] = std::make_shared<std::atomic<bool>>(false);
                    this->set_thread(i);
                }
            } else {  // the number of threads is decreased
                for (int i = oldNThreads - 1; i >= nThreads; --i) {
                    *this->flags[i] = true;  // this thread will finish
                    this->threads[i]->detach();
                }
                {
                    // stop the detached threads that were waiting
                    std::unique_lock<std::mutex> lock(this->mutex);
                    this->cv.notify_all();
                }
                this->threads.resize(nThreads);  // safe to delete because the threads are detached
                this->flags.resize(nThreads);    // safe to delete because the threads have copies of shared_ptr of the flags, not originals
            }
        }
    }

    // empty the queue
    void clear_queue() {
        std::function<void(int id)> *_f;
        while (this->q.pop(_f)) delete _f;  // empty the queue
    }

    // pops a functional wrapper to the original function
    std::function<void(int)> pop() {
        std::function<void(int id)> *_f = nullptr;
        this->q.pop(_f);
        std::unique_ptr<std::function<void(int id)>> func(_f);  // at return, delete the function even if an exception occurred
        std::function<void(int)> f;
        if (_f) f = *_f;
        return f;
    }

    // wait for all computing threads to finish and stop all threads
    // may be called asynchronously to not pause the calling thread while waiting
    // if isWait == true, all the functions in the queue are run, otherwise the queue is cleared without running the functions
    void stop(bool isWait = false) {
        if (!isWait) {
            if (this->isStop) return;
            this->isStop = true;
            for (int i = 0, n = this->size(); i < n; ++i) {
                *this->flags[i] = true;  // command the threads to stop
            }
            this->clear_queue();  // empty the queue
        } else {
            if (this->isDone || this->isStop) return;
            this->isDone = true;  // give the waiting threads a command to finish
        }
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            this->cv.notify_all();  // stop all waiting threads
        }
        for (int i = 0; i < static_cast<int>(this->threads.size()); ++i) {  // wait for the computing threads to finish
            if (this->threads[i]->joinable()) this->threads[i]->join();
        }
        // if there were no threads in the pool but some functors in the queue, the functors are not deleted by the threads
        // therefore delete them here
        this->clear_queue();
        this->threads.clear();
        this->flags.clear();
    }

    template <typename F, typename... Rest>
    auto push(F &&f, Rest &&...rest) -> std::future<decltype(f(0, rest...))> {
        auto pck = std::make_shared<std::packaged_task<decltype(f(0, rest...))(int)>>(std::bind(std::forward<F>(f), std::placeholders::_1, std::forward<Rest>(rest)...));
        auto _f = new std::function<void(int id)>([pck](int id) { (*pck)(id); });
        this->q.push(_f);
        std::unique_lock<std::mutex> lock(this->mutex);
        this->cv.notify_one();
        return pck->get_future();
    }

    // run the user's function that excepts argument int - id of the running thread. returned value is templatized
    // operator returns std::future, where the user can get the result and rethrow the catched exceptins
    template <typename F>
    auto push(F &&f) -> std::future<decltype(f(0))> {
        auto pck = std::make_shared<std::packaged_task<decltype(f(0))(int)>>(std::forward<F>(f));
        auto _f = new std::function<void(int id)>([pck](int id) { (*pck)(id); });
        this->q.push(_f);
        std::unique_lock<std::mutex> lock(this->mutex);
        this->cv.notify_one();
        return pck->get_future();
    }

private:
    // deleted
    thread_pool(const thread_pool &);             // = delete;
    thread_pool(thread_pool &&);                  // = delete;
    thread_pool &operator=(const thread_pool &);  // = delete;
    thread_pool &operator=(thread_pool &&);       // = delete;

    void set_thread(int i) {
        std::shared_ptr<std::atomic<bool>> flag(this->flags[i]);  // a copy of the shared ptr to the flag
        auto f = [this, i, flag /* a copy of the shared ptr to the flag */]() {
            std::atomic<bool> &_flag = *flag;
            std::function<void(int id)> *_f;
            bool isPop = this->q.pop(_f);
            while (true) {
                while (isPop) {                                             // if there is anything in the queue
                    std::unique_ptr<std::function<void(int id)>> func(_f);  // at return, delete the function even if an exception occurred
                    (*_f)(i);
                    if (_flag)
                        return;  // the thread is wanted to stop, return even if the queue is not empty yet
                    else
                        isPop = this->q.pop(_f);
                }
                // the queue is empty here, wait for the next command
                std::unique_lock<std::mutex> lock(this->mutex);
                ++this->nWaiting;
                this->cv.wait(lock, [this, &_f, &isPop, &_flag]() {
                    isPop = this->q.pop(_f);
                    return isPop || this->isDone || _flag;
                });
                --this->nWaiting;
                if (!isPop) return;  // if the queue is empty and this->isDone == true or *flag then return
            }
        };
        this->threads[i].reset(new std::thread(f));  // compiler may not support std::make_unique()
    }

    void init() {
        this->nWaiting = 0;
        this->isStop = false;
        this->isDone = false;
    }

    std::vector<std::unique_ptr<std::thread>> threads;
    std::vector<std::shared_ptr<std::atomic<bool>>> flags;
    thread_pool_queue<std::function<void(int id)> *> q;
    std::atomic<bool> isDone;
    std::atomic<bool> isStop;
    std::atomic<int> nWaiting;

    std::mutex mutex;
    std::condition_variable cv;
};

}  // namespace ME

std::vector<std::string> split(std::string strToSplit, char delimeter);
std::vector<std::string> string_split(std::string s, const char delimiter);
std::vector<std::string> split2(std::string const &original, char separator);

struct MEstring {

    MEstring() = default;

    MEstring(const char *str [[maybe_unused]]) : m_String(str ? str : "") {}

    MEstring(std::string str) : m_String(std::move(str)) {}

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

template <typename T>
ME_INLINE std::string to_string(const T &val) {
    std::stringstream ss;
    ss << val;
    return ss.str();
}

long long ME_gettime();
double ME_gettime_d();
time_t ME_gettime_mkgmtime(struct tm *unixdate);

inline std::string ws2s(const std::wstring &wstr) {
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

inline bool ME_str_is_chinese_c(const char str) { return str & 0x80; }

inline bool ME_str_is_chinese_str(const std::string &str) {
    for (int i = 0; i < str.length(); i++)
        if (ME_str_is_chinese_c(str[i])) return true;
    return false;
}

inline bool ME_str_equals(const char *a, const char *c) { return strcmp(a, c) == 0; }

inline bool ME_str_starts_with(std::string_view s, std::string_view prefix) { return prefix.size() <= s.size() && (strncmp(prefix.data(), s.data(), prefix.size()) == 0); }

inline bool ME_str_starts_with(std::string_view s, char prefix) { return !s.empty() && s[0] == prefix; }

inline bool ME_str_starts_with(const char *s, const char *prefix) { return strncmp(s, prefix, strlen(prefix)) == 0; }

inline bool ME_str_ends_with(std::string_view s, std::string_view suffix) { return suffix.size() <= s.size() && strncmp(suffix.data(), s.data() + s.size() - suffix.size(), suffix.size()) == 0; }

inline bool ME_str_ends_with(std::string_view s, char suffix) { return !s.empty() && s[s.size() - 1] == suffix; }

inline bool ME_str_ends_with(const char *s, const char *suffix) {
    auto sizeS = strlen(s);
    auto sizeSuf = strlen(suffix);

    return sizeSuf <= sizeS && strncmp(suffix, s + sizeS - sizeSuf, sizeSuf) == 0;
}

inline void ME_str_to_lower(char *s) {
    int l = strlen(s);
    int ind = 0;
    // spec of "simd"
    for (int i = 0; i < l / 4; i++) {
        s[ind] = std::tolower(s[ind]);
        s[ind + 1] = std::tolower(s[ind + 1]);
        s[ind + 2] = std::tolower(s[ind + 2]);
        s[ind + 3] = std::tolower(s[ind + 3]);
        ind += 4;
    }
    // do the rest linearly
    for (int i = 0; i < (l & 3); ++i) {
        s[ind++] = std::tolower(s[ind]);
    }
}

inline void ME_str_to_lower(std::string &ss) {
    int l = ss.size();
    auto s = ss.data();
    int ind = 0;
    // spec of "simd"
    for (int i = 0; i < l / 4; i++) {
        s[ind] = std::tolower(s[ind]);
        s[ind + 1] = std::tolower(s[ind + 1]);
        s[ind + 2] = std::tolower(s[ind + 2]);
        s[ind + 3] = std::tolower(s[ind + 3]);
        ind += 4;
    }
    // do the rest linearly
    for (int i = 0; i < (l & 3); ++i) s[ind++] = std::tolower(s[ind]);
}

inline void ME_str_to_upper(char *s) {
    int l = strlen(s);
    int ind = 0;
    // spec of "simd"
    for (int i = 0; i < l / 4; i++) {
        s[ind] = std::toupper(s[ind]);
        s[ind + 1] = std::toupper(s[ind + 1]);
        s[ind + 2] = std::toupper(s[ind + 2]);
        s[ind + 3] = std::toupper(s[ind + 3]);
        ind += 4;
    }
    // do the rest linearly
    for (int i = 0; i < (l & 3); ++i) {
        s[ind++] = std::toupper(s[ind]);
    }
}

inline void ME_str_to_upper(std::string &ss) {
    int l = ss.size();
    auto s = ss.data();
    int ind = 0;
    // spec of "simd"
    for (int i = 0; i < l / 4; i++) {
        s[ind] = std::toupper(s[ind]);
        s[ind + 1] = std::toupper(s[ind + 1]);
        s[ind + 2] = std::toupper(s[ind + 2]);
        s[ind + 3] = std::toupper(s[ind + 3]);
        ind += 4;
    }
    // do the rest linearly
    for (int i = 0; i < (l & 3); ++i) s[ind++] = std::toupper(s[ind]);
}

inline bool ME_str_replace_with(char *src, char what, char with) {
    for (int i = 0; true; ++i) {
        auto &id = src[i];
        if (id == '\0') return true;
        bool isWhat = id == what;
        id = isWhat * with + src[i] * (!isWhat);
    }
}

inline bool ME_str_replace_with(std::string &src, char what, char with) {
    for (int i = 0; i < src.size(); ++i) {
        auto &id = src.data()[i];
        bool isWhat = id == what;
        id = isWhat * with + src[i] * (!isWhat);
    }
    return true;
}

inline bool ME_str_replace_with(std::string &src, const char *what, const char *with) {
    std::string out;
    size_t whatlen = strlen(what);
    out.reserve(src.size());
    size_t ind = 0;
    size_t lastInd = 0;
    while (true) {
        ind = src.find(what, ind);
        if (ind == std::string::npos) {
            out += src.substr(lastInd);
            break;
        }
        out += src.substr(lastInd, ind - lastInd) + with;
        ind += whatlen;
        lastInd = ind;
    }
    src = out;
    return true;
}

inline bool ME_str_replace_with(std::string &src, const char *what, const char *with, int times) {
    for (int i = 0; i < times; ++i) ME_str_replace_with(src, what, with);
    return true;
}

// populates words with words that were crated by splitting line with one char of dividers
// NOTE!: when using views
//  if you delete the line (or src of the line)
//      -> string_views will no longer be valid!!!!!
/*template <typename StringOrView>
void splitString(const Viewo& line, std::vector<StringOrView>& words, const char* divider = " \n\t")
{
    auto beginIdx = line.find_first_not_of(divider, 0);
    while (beginIdx != Stringo::npos)
    {
        auto endIdx = line.find_first_of(divider, beginIdx);
        if (endIdx != Stringo::npos)
        {
            words.emplace_back(line.begin() + beginIdx, endIdx - beginIdx);
            beginIdx = line.find_first_not_of(divider, endIdx);
        }
        else {
            words.emplace_back(line.begin() + beginIdx, line.size() - beginIdx);
            break;
        }
    }
}*/
inline void ME_str_split(const std::string &line, std::vector<std::string> &words, const char *divider = " \n\t") {
    auto beginIdx = line.find_first_not_of(divider, 0);
    while (beginIdx != std::string::npos) {
        auto endIdx = line.find_first_of(divider, beginIdx);
        if (endIdx != std::string::npos) {
            words.emplace_back(line.data() + beginIdx, endIdx - beginIdx);
            beginIdx = line.find_first_not_of(divider, endIdx);
        } else {
            words.emplace_back(line.data() + beginIdx, line.size() - beginIdx);
            break;
        }
    }
}

// iterates over a string_view divided by "divider" (any chars in const char*)
// Example:
//      if(!ignoreBlanks)
//          "\n\nBoo\n" -> "","","Boo",""
//      else
//          "\n\nBoo\n" -> "Boo"
//
//
//  enable throwException to throw Stringo("No Word Left")
//  if(operator bool()==false)
//      -> on calling operator*() or operator->()
//

template <bool ignoreBlanks = false, typename DividerType = const char *, bool throwException = false>
class split_iterator {
    static_assert(std::is_same<DividerType, const char *>::value || std::is_same<DividerType, char>::value);

private:
    std::string_view m_source;
    std::string_view m_pointer;
    // this fixes 1 edge case when m_source end with m_divider
    DividerType m_divider;
    bool m_ending = false;

    void step() {
        if (!operator bool()) return;

        if constexpr (ignoreBlanks) {
            bool first = true;
            while (m_pointer.empty() || first) {
                first = false;

                if (!operator bool()) return;
                // check if this is ending
                // the next one will be false
                if (m_source.size() == m_pointer.size()) {
                    m_source = std::string_view(nullptr, 0);
                } else {
                    auto viewSize = m_pointer.size();
                    m_source = std::string_view(m_source.data() + viewSize + 1, m_source.size() - viewSize - 1);
                    // shift source by viewSize

                    auto nextDivi = m_source.find_first_of(m_divider, 0);
                    if (nextDivi != std::string::npos)
                        m_pointer = std::string_view(m_source.data(), nextDivi);
                    else
                        m_pointer = std::string_view(m_source.data(), m_source.size());
                }
            }
        } else {
            // check if this is ending
            // the next one will be false
            if (m_source.size() == m_pointer.size()) {
                m_source = std::string_view(nullptr, 0);
                m_ending = false;
            } else {
                auto viewSize = m_pointer.size();
                m_source = std::string_view(m_source.data() + viewSize + 1, m_source.size() - viewSize - 1);
                // shift source by viewSize
                if (m_source.empty()) m_ending = true;
                auto nextDivi = m_source.find_first_of(m_divider, 0);
                if (nextDivi != std::string::npos)
                    m_pointer = std::string_view(m_source.data(), nextDivi);
                else
                    m_pointer = std::string_view(m_source.data(), m_source.size());
            }
        }
    }

public:
    split_iterator(std::string_view src, DividerType divider) : m_source(src), m_divider(divider) {
        auto div = m_source.find_first_of(m_divider, 0);
        m_pointer = std::string_view(m_source.data(), div == std::string::npos ? m_source.size() : div);
        if constexpr (ignoreBlanks) {
            if (m_pointer.empty()) step();
        }
    }

    split_iterator(const split_iterator &s) = default;

    operator bool() const { return !m_source.empty() || m_ending; }

    split_iterator &operator+=(size_t delta) {
        while (this->operator bool() && delta) {
            delta--;
            step();
        }
        return (*this);
    }

    split_iterator &operator++() {
        if (this->operator bool()) step();
        return (*this);
    }

    split_iterator operator++(int) {
        auto temp(*this);
        step();
        return temp;
    }

    const std::string_view &operator*() {
        if (throwException && !operator bool())  // Attempt to access* it or it->when no word is left in the iterator
            throw std::string("No Word Left");
        return m_pointer;
    }

    const std::string_view *operator->() {
        if (throwException && !operator bool())  // Attempt to access *it or it-> when no word is left in the iterator
            throw std::string("No Word Left");
        return &m_pointer;
    }
};

// removes comments: (replaces with ' ')
//  1. one liner starting with "//"
//  2. block comment bounded by "/*" and "*/"
inline void ME_str_remove_comments(std::string &src) {
    size_t offset = 0;
    bool opened = false;  // multiliner opened
    size_t openedStart = 0;
    while (true) {
        auto slash = src.find_first_of('/', offset);
        if (slash != std::string::npos) {
            std::string s = src.substr(slash);
            if (!opened) {
                if (src.size() == slash - 1) return;

                char next = src[slash + 1];
                if (next == '/')  // one liner
                {
                    auto end = src.find_first_of('\n', slash + 1);
                    if (end == std::string::npos) {
                        memset(src.data() + slash, ' ', src.size() - 1 - slash);
                        return;
                    }
                    memset(src.data() + slash, ' ', end - slash);
                    offset = end;
                } else if (next == '*') {
                    opened = true;
                    offset = slash + 1;
                    openedStart = slash;
                } else
                    offset = slash + 1;
            } else {
                if (src[slash - 1] == '*') {
                    opened = false;
                    memset(src.data() + openedStart, ' ', slash - openedStart);
                    offset = slash + 1;
                }
                offset = slash + 1;
            }
        } else if (opened) {
            memset(src.data() + openedStart, ' ', src.size() - 1 - openedStart);
            return;
        } else
            return;
    }
}

// converts utf8 encoded string to zero terminated int array of codePoints
// transfers ownership of returned array (don't forget free())
// length will be set to size returned array (excluding zero terminator)
const int *ME_str_utf8to_codepointsarray(const char *c, int *length = nullptr);

std::u32string ME_str_utf8to_codepoints(const char *c);

inline std::u32string ME_str_utf8to_codepoints(const std::string &c) { return ME_str_utf8to_codepoints(c.c_str()); }

// converts ascii u32 string to string
// use only if you know that there are only ascii characters
std::string ME_str_u32stringto_string(std::u32string_view s);

// returns first occurrence of digit or nullptr
inline const char *ME_str_skip_tonextdigit(const char *c) {
    c--;
    while (*(++c)) {
        if (*c >= '0' && *c <= '9') return c;
    }
    return nullptr;
}

// keeps parsing numbers until size is reached or until there are no numbers
// actualSize is set to number of numbers actually parsed
template <int size, typename numberType>
void parseNumbers(const char *c, numberType ray[size], int *actualSize = nullptr) {
    size_t chars = 0;

    for (int i = 0; i < size; ++i) {
        if ((c = ME_str_skip_tonextdigit(c + chars)) != nullptr) {
            if (std::is_same<numberType, int>::value)
                ray[i] = std::stoi(c, &chars);
            else if (std::is_same<numberType, float>::value)
                ray[i] = std::stof(c, &chars);
            else if (std::is_same<numberType, double>::value)
                ray[i] = std::stod(c, &chars);
            else
                static_assert("invalid type");
        } else {
            if (actualSize) *actualSize = i;
            return;
        }
    }
    if (actualSize) *actualSize = size;
}

// keeps parsing numbers until size is reached or until there are no numbers
// actualSize is set to number of numbers actually parsed
template <int size, typename numberType>
void parseNumbers(const std::string &s, numberType ray[size], int *actualSize = nullptr) {
    parseNumbers<size>(s.c_str(), ray, actualSize);
}

#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#ifndef FILENAME_MAX
#define FILENAME_MAX 256
#endif

// #define max(a, b)                                                                                  \
//     ({                                                                                             \
//         __typeof__(a) _a = (a);                                                                    \
//         __typeof__(b) _b = (b);                                                                    \
//         _a > _b ? _a : _b;                                                                         \
//     })

// #define min(a, b)                                                                                  \
//     ({                                                                                             \
//         __typeof__(a) _a = (a);                                                                    \
//         __typeof__(b) _b = (b);                                                                    \
//         _a < _b ? _a : _b;                                                                         \
//     })

/* --- PRINTF_BYTE_TO_BINARY macro's --- */
#define PRINTF_BINARY_PATTERN_INT8 "%c%c%c%c%c%c%c%c"
#define PRINTF_BYTE_TO_BINARY_INT8(i)                                                                                                                                       \
    (((i)&0x80ll) ? '1' : '0'), (((i)&0x40ll) ? '1' : '0'), (((i)&0x20ll) ? '1' : '0'), (((i)&0x10ll) ? '1' : '0'), (((i)&0x08ll) ? '1' : '0'), (((i)&0x04ll) ? '1' : '0'), \
            (((i)&0x02ll) ? '1' : '0'), (((i)&0x01ll) ? '1' : '0')

#define PRINTF_BINARY_PATTERN_INT16 PRINTF_BINARY_PATTERN_INT8 PRINTF_BINARY_PATTERN_INT8
#define PRINTF_BYTE_TO_BINARY_INT16(i) PRINTF_BYTE_TO_BINARY_INT8((i) >> 8), PRINTF_BYTE_TO_BINARY_INT8(i)
#define PRINTF_BINARY_PATTERN_INT32 PRINTF_BINARY_PATTERN_INT16 PRINTF_BINARY_PATTERN_INT16
#define PRINTF_BYTE_TO_BINARY_INT32(i) PRINTF_BYTE_TO_BINARY_INT16((i) >> 16), PRINTF_BYTE_TO_BINARY_INT16(i)
#define PRINTF_BINARY_PATTERN_INT64 PRINTF_BINARY_PATTERN_INT32 PRINTF_BINARY_PATTERN_INT32
#define PRINTF_BYTE_TO_BINARY_INT64(i) PRINTF_BYTE_TO_BINARY_INT32((i) >> 32), PRINTF_BYTE_TO_BINARY_INT32(i)
/* --- end macros --- */

// vsnprintf replacement from Valentin Milea:
// http://stackoverflow.com/questions/2915672/snprintf-and-visual-studio-2010
#if defined(_MSC_VER) && _MSC_VER < 1900

#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

ME_INLINE int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap) {
    int count = -1;

    if (size != 0) count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1) count = _vscprintf(format, ap);

    return count;
}

ME_INLINE int c99_snprintf(char *outBuf, size_t size, const char *format, ...) {
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
}

#endif

// Trie structures and interface

// Add all supported types here, as 'Trie_type'
typedef enum TrieType { Trie_None, Trie_Pointer, Trie_String, Trie_MEvec3, Trie_double, Trie_float, Trie_char, Trie_int } TrieType;

// Structure used to retrieve all the data from the trie
// Add all supported types inside the union, as 'type* typeValue'
typedef struct TrieElement {
    char *key;
    TrieType type;
    unsigned size;
    union {
        void *pointerValue;
        char *stringValue;
        MEvec3 *vector3Value;
        f64 *doubleValue;
        f32 *floatValue;
        char *charValue;
        int *intValue;
    };
} TrieElement;

#define TRIE_ALPHABET_SIZE 256
typedef struct TrieCell {
    TrieType elementType;
    unsigned maxKeySize;
    union {
        unsigned numberOfElements;
        unsigned elementSize;
    };
    // In a leaf and branch, the '\0' points to the data stored, while in a trunk it points to NULL
    // In both branch and trunk, all the other characters points to other Tries
    void *branch[TRIE_ALPHABET_SIZE];
} Trie;

Trie InitTrie();
void FreeTrie(Trie *trie);

void InsertTrie(Trie *trie, const char *key, const void *value, int size, TrieType valueType);
void InsertTrieString(Trie *trie, const char *key, const char *value);

int TrieContainsKey(Trie trie, const char *key);
void *GetTrieElement(Trie trie, const char *key);
void *GetTrieElementWithProperties(Trie trie, const char *key, int *sizeOut, TrieType *typeOut);
void *GetTrieElementAsPointer(Trie trie, const char *key, void *defaultValue);
char *GetTrieElementAsString(Trie trie, const char *key, char *defaultValue);

// Macro to generate headers for the insertion and retrieval functions
// Remember to call the function template macro on utils.c and to modify the TrieType enum when adding more types
#define TRIE_TYPE_FUNCTION_HEADER_MACRO(type)                        \
    void InsertTrie_##type(Trie *trie, const char *key, type value); \
    type GetTrieElementAs_##type(Trie trie, const char *key, type defaultValue);

TRIE_TYPE_FUNCTION_HEADER_MACRO(MEvec3)
TRIE_TYPE_FUNCTION_HEADER_MACRO(f64)
TRIE_TYPE_FUNCTION_HEADER_MACRO(f32)
TRIE_TYPE_FUNCTION_HEADER_MACRO(char)
TRIE_TYPE_FUNCTION_HEADER_MACRO(int)

// Functions to obtain all data inside a trie
// The data returned should be used before any new replace modifications
// are made in the trie, as the data pointed can be freed when replaced
TrieElement *GetTrieElementsArray(Trie trie, int *outElementsCount);
void FreeTrieElementsArray(TrieElement *elementsArray, int elementsCount);

void Vector3ToTable(lua_State *L, MEvec3 vector);

//--------------------------------------------------------------------------------------------------------------------------------//
// DEBUG LOGGING:

// represents different message types
typedef enum MEmessageType {
    ME_MESSAGE_CPU_MEMORY,  // the message is about CPU memory usage
    ME_MESSAGE_GPU_MEMORY,  // the message is about GPU memory usage
    ME_MESSAGE_SHADER,      // the message is about shader compilation
    ME_MESSAGE_FILE_IO      // the message is about file I/O (just used for if opening a file fails)
} MEmessageType;

template <>
struct ME::meta::static_refl::TypeInfo<MEmessageType> : TypeInfoBase<MEmessageType> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("ME_MESSAGE_CPU_MEMORY"), Type::ME_MESSAGE_CPU_MEMORY},
            Field{TSTR("ME_MESSAGE_GPU_MEMORY"), Type::ME_MESSAGE_GPU_MEMORY},
            Field{TSTR("ME_MESSAGE_SHADER"), Type::ME_MESSAGE_SHADER},
            Field{TSTR("ME_MESSAGE_FILE_IO"), Type::ME_MESSAGE_FILE_IO},
    };
};

// represents different message severities
typedef enum MEmessageSeverity {
    ME_MESSAGE_NOTE,   // the message is purely informative, no error has occured
    ME_MESSAGE_ERROR,  // an error has occured, but the engine is still able to run without crashing
    ME_MESSAGE_FATAL   // a fatal error has occured, and the engine will likely not be able to continue running without crashing
} MEmessageSeverity;

// the message callback function, the last parameter is the actual message in string format
extern void (*g_engine_message_callback)(MEmessageType, MEmessageSeverity, const char *);  // TODO: figure out a better way to use this than a global variable

#endif
