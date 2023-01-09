// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "engine/utils.hpp"

#include <chrono>
#include <cstdint>
#include <ctime>

#include "core/global.hpp"
#include "engine/code_reflection.hpp"
#include "engine/memory.hpp"

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

long long Time::millis() {
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return ms;
}

time_t Time::mkgmtime(struct tm *unixdate) {
    assert(unixdate != nullptr);
    time_t fakeUnixtime = mktime(unixdate);
    struct tm *fakeDate = gmtime(&fakeUnixtime);

    int32_t nOffSet = fakeDate->tm_hour - unixdate->tm_hour;
    if (nOffSet > 12) {
        nOffSet = 24 - nOffSet;
    }
    return fakeUnixtime - nOffSet * 3600;
}

///////////////////////////////////////////////////////////////////////////////

Timer::Timer() : myOffSet(GetTime()), myPause(false), myPauseTime(0), myLastUpdate(0) { METADOT_ASSERT_E(sizeof(I64) == 8); }

//.............................................................................

Timer::~Timer() {}

///////////////////////////////////////////////////////////////////////////////

Timer &Timer::operator=(const Timer &other) {
    // BUGBUG	Does not work because the myBaseTimer might be different from
    //			the one where these where copied
    myOffSet = other.myOffSet;
    myPause = other.myPause;
    myPauseTime = other.myPauseTime;
    myLastUpdate = other.myLastUpdate;

    return *this;
}

//=============================================================================

Timer Timer::operator-(const Timer &other) { return (Timer(*this) -= (other)); }

//=============================================================================

Timer Timer::operator-(Timer::Ticks time) { return (Timer(*this) -= (time)); }

//=============================================================================

Timer &Timer::operator-=(const Timer &other) { return operator-=(other.GetTime()); }

//=============================================================================

Timer &Timer::operator-=(Timer::Ticks time) {
    myOffSet += time;
    myPauseTime -= time;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////

void Timer::SetTime(Timer::Ticks time) {
    myOffSet = (I64)GetTime() - (I64)time;
    myPauseTime = time;
}

//=============================================================================

Timer::Ticks Timer::GetTime() const {
    if (myPause) {
        if (myPauseTime < 0) return 0;

        return (Timer::Ticks)myPauseTime;
    }

    if ((unsigned)myOffSet > GetTime()) return 0;

    return (GetTime() - (Timer::Ticks)myOffSet);
}

//.............................................................................

float Timer::GetSeconds() const { return ((float)GetTime() / 1000.0f); }

//.............................................................................

Timer::Ticks Timer::GetDerivate() const { return (GetTime() - myLastUpdate); }

//.............................................................................

float Timer::GetDerivateSeconds() const { return ((float)GetDerivate() / 1000.0f); }

//.............................................................................

void Timer::Updated() { myLastUpdate = GetTime(); }

//.............................................................................

void Timer::Pause() {
    if (myPause == false) {
        myPauseTime = GetTime();
        myPause = true;
    }
}

//.............................................................................

void Timer::Resume() {
    if (myPause == true) {
        myPause = false;
        SetTime((Timer::Ticks)myPauseTime);
        // myOffSet += ( GetTime() - myPauseTime );
    }
}

//.............................................................................

void Timer::Reset() {
    // myPause = false;
    myPauseTime = 0;
    myOffSet = GetTime();
    myLastUpdate = 0;
}

namespace SUtil {
const int *utf8toCodePointsArray(const char *c, int *length) {
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
    auto o = (int *)METAENGINE_MALLOC(out.size() * sizeof(int));
    memcpy(o, out.data(), out.size() * sizeof(int));
    return o;
}

std::u32string utf8toCodePoints(const char *c) {
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

std::string u32StringToString(std::u32string_view s) {
    std::string out;
    for (auto c : s) out += (char)c;
    return out;
}
}  // namespace SUtil
