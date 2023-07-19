// Copyright(c) 2023, KaoruXun All rights reserved.

#include "utility.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>

#include "engine/core/base_memory.h"
#include "engine/core/core.hpp"
#include "engine/core/platform.h"
#include "engine/scripting/scripting.hpp"
// #include "runtime/ui/ui.hpp"

#define TIME_COUNT(x) std::chrono::time_point_cast<std::chrono::microseconds>(x).time_since_epoch().count()

namespace ME {

void Timer::start() noexcept { startPos = std::chrono::high_resolution_clock::now(); }

void Timer::stop() noexcept {
    auto endTime = std::chrono::high_resolution_clock::now();
    duration = static_cast<double>(TIME_COUNT(endTime) - TIME_COUNT(startPos)) * 0.001;
}

Timer::~Timer() noexcept { stop(); }

double Timer::get() const noexcept { return duration; }

void Logger::set_crash_on_error(bool bError) noexcept { loggerInternal.using_errors = bError; }

void Logger::set_current_log_file(const char *file) noexcept {
    loggerInternal.shutdown_file_stream();
    loggerInternal.fileout = std::ofstream(file);
}

void Logger::set_log_operation(log_operations op) noexcept { loggerInternal.operation_type = op; }

void LoggerInternal::writeline(std::string &msg) {
    OutputDebugStringA(msg.c_str());
    std::cout << msg;
}

std::string LoggerInternal::get_current_time() noexcept {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::string realTime = std::ctime(&now);
    realTime.erase(24);

    return realTime;
}

void LoggerInternal::shutdown_file_stream() noexcept { fileout.close(); }

LoggerInternal::LoggerInternal() noexcept {}

LoggerInternal::~LoggerInternal() noexcept { shutdown_file_stream(); }

std::vector<std::string> split(std::string strToSplit, char delimeter) {
    std::stringstream ss(strToSplit);
    std::string item;
    std::vector<std::string> splittedStrings;
    while (getline(ss, item, delimeter)) {
        splittedStrings.push_back(item);
    }
    return splittedStrings;
}

std::vector<std::string> string_split(std::string s, const char delimiter) {
    size_t start = 0;
    size_t end = s.find_first_of(delimiter);

    std::vector<std::string> output;

    while (end <= std::string::npos) {
        output.emplace_back(s.substr(start, end - start));

        if (end == std::string::npos) break;

        start = end + 1;
        end = s.find_first_of(delimiter, start);
    }

    return output;
}

std::vector<std::string> split2(std::string const &original, char separator) {
    std::vector<std::string> results;
    std::string::const_iterator start = original.begin();
    std::string::const_iterator end = original.end();
    std::string::const_iterator next = std::find(start, end, separator);
    while (next != end) {
        results.push_back(std::string(start, next));
        start = next + 1;
        next = std::find(start, end, separator);
    }
    results.push_back(std::string(start, next));
    return results;
}

long long ME_gettime() {
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return ms;
}

double ME_gettime_d() {
    double t;
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    t = (ft.dwHighDateTime * 4294967296.0 / 1e7) + ft.dwLowDateTime / 1e7;
    t -= 11644473600.0;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    t = tv.tv_sec + tv.tv_usec / 1e6;
#endif
    return t;
}

time_t ME_gettime_mkgmtime(struct tm *unixdate) {
    assert(unixdate != nullptr);
    time_t fakeUnixtime = mktime(unixdate);
    struct tm *fakeDate = gmtime(&fakeUnixtime);

    int32_t nOffSet = fakeDate->tm_hour - unixdate->tm_hour;
    if (nOffSet > 12) {
        nOffSet = 24 - nOffSet;
    }
    return fakeUnixtime - nOffSet * 3600;
}

const int *ME_str_utf8to_codepointsarray(const char *c, int *length) {
    // todo use something better than std::vector
    std::vector<int> out;
    char byte1 = 0;
    while ((byte1 = *c++) != 0) {
        if (!(byte1 & 0b10000000)) {
            out.push_back(byte1);
        }
        if ((byte1 & 0b11100000) == 0b11000000) {
            // starts with 110
            char byte2 = *c++;
            int b1 = (int)byte1 & 0b00011111;
            int b2 = (int)byte2 & 0b00111111;

            out.push_back(b2 | (b1 << 6));
        } else if ((byte1 & 0b11110000) == 0b11100000) {
            // starts with 1110
            char byte2 = *c++;
            char byte3 = *c++;

            int b1 = (int)byte1 & 0b00001111;
            int b2 = (int)byte2 & 0b00111111;
            int b3 = (int)byte3 & 0b00111111;

            out.push_back(b3 | (b2 << 6) | (b1 << 6));
        } else if ((byte1 & 0b11111000) == 0b11110000) {
            // starts with 1110
            char byte2 = *c++;
            char byte3 = *c++;
            char byte4 = *c++;

            int b1 = (int)byte1 & 0b00000111;
            int b2 = (int)byte2 & 0b00111111;
            int b3 = (int)byte3 & 0b00111111;
            int b4 = (int)byte4 & 0b00111111;

            out.push_back(b4 | (b3 << 6) | (b2 << 12) | (b1 << 18));
        }
    }
    if (length) *length = out.size();
    if (out.empty()) return nullptr;

    out.push_back(0);
    auto o = (int *)ME_MALLOC(out.size() * sizeof(int));
    memcpy(o, out.data(), out.size() * sizeof(int));
    return o;
}

std::u32string ME_str_utf8to_codepoints(const char *c) {
    // todo use something better than std::vector
    std::u32string out;
    uint8_t byte1;
    while ((byte1 = *c++) != 0) {
        if (!(byte1 & 0b10000000)) {
            out.push_back(byte1);
        }
        if ((byte1 & 0b11100000) == 0b11000000) {
            // starts with 110
            uint8_t byte2 = *c++;
            int b1 = (int)byte1 & 0b00011111;
            int b2 = (int)byte2 & 0b00111111;

            out.push_back(b2 | (b1 << 6));
        } else if ((byte1 & 0b11110000) == 0b11100000) {
            // starts with 1110
            uint8_t byte2 = *c++;
            uint8_t byte3 = *c++;

            int b1 = (int)byte1 & 0b00001111;
            int b2 = (int)byte2 & 0b00111111;
            int b3 = (int)byte3 & 0b00111111;

            out.push_back(b3 | (b2 << 6) | (b1 << 12));
        } else if ((byte1 & 0b11111000) == 0b11110000) {
            // starts with 11110
            uint8_t byte2 = *c++;
            uint8_t byte3 = *c++;
            uint8_t byte4 = *c++;

            int b1 = (int)byte1 & 0b00000111;
            int b2 = (int)byte2 & 0b00111111;
            int b3 = (int)byte3 & 0b00111111;
            int b4 = (int)byte4 & 0b00111111;

            out.push_back(b4 | (b3 << 6) | (b2 << 12) | (b1 << 18));
        }
    }
    return out;
}

std::string ME_str_u32stringto_string(std::u32string_view s) {
    std::string out;
    for (auto c : s) out += (char)c;
    return out;
}

// --------------- Lua stack manipulation functions ---------------

// Creates an table with the xyz entries and populate with the vector values
void Vector3ToTable(lua_State *L, MEvec3 vector) {

    lua_newtable(L);
    lua_pushliteral(L, "x");      // x index
    lua_pushnumber(L, vector.x);  // x value
    lua_rawset(L, -3);            // Store x in table

    lua_pushliteral(L, "y");      // y index
    lua_pushnumber(L, vector.y);  // y value
    lua_rawset(L, -3);            // Store y in table

    lua_pushliteral(L, "z");      // z index
    lua_pushnumber(L, vector.z);  // z value
    lua_rawset(L, -3);            // Store z in table
}

}  // namespace ME