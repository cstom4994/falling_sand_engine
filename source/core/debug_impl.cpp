// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "debug_impl.hpp"

#include "core/core.hpp"

static const char *date = __DATE__;
static const char *mon[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char mond[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int metadot_buildnum(void) {
    int m = 0, d = 0, y = 0;
    static int b = 0;

    if (b != 0) return b;

    for (m = 0; m < 11; m++) {
        if (!strncmp(&date[0], mon[m], 3)) break;
        d += mond[m];
    }

    d += atoi(&date[4]) - 1;
    y = atoi(&date[7]) - 2000;
    b = d + (int)((y - 1) * 365.25f);

    if (((y % 4) == 0) && m > 1) {
        b += 1;
    }
    b -= 7950;

    return b;
}

const std::string metadot_metadata() {
    std::string result;

    result += "Copyright(c) 2022-2023, KaoruXun All rights reserved.\n";
    result += "MetaDot\n";

#ifdef _WIN32
    result += "platform win32\n";
#elif defined __linux__
    result += "platform linux\n";
#elif defined __APPLE__
    result += "platform apple\n";
#elif defined __unix__
    result += "platform unix\n";
#else
    result += "platform unknown\n";
#endif

#if defined(__clang__)
    result += "compiler.family = clang\n";
    result += "compiler.version = ";
    result += std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__) + "." + std::to_string(__clang_patchlevel__);
    result += "\n";
#elif defined(__GNUC__) || defined(__GNUG__)
    result += "compiler.family = gcc\n";
    result += "compiler.version = ";
    result += std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." + std::to_string(__GNUC_PATCHLEVEL__);
    result += "\n";
#elif defined(_MSC_VER)
    result += "compiler.family = msvc\n";
    result += "compiler.version = ";
    result += _MSC_VER;
    result += "\n";
#else
    result += "compiler.family = unknown\n";
    result += "compiler.version = unknown";
    result += "\n";
#endif

#ifdef __cplusplus
    result += "compiler.c++ = ";
    result += std::to_string(__cplusplus);
    result += "\n";
#else
    result += "compiler.c++ = unknown\n";
#endif
    return result;
}
