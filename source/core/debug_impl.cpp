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

DebugInfo metadot_metadata() {

    DebugInfo dinfo;

#ifdef _WIN32
    dinfo.platform = "win32";
#elif defined __linux__
    dinfo.platform = "linux";
#elif defined __APPLE__
    dinfo.platform = "apple";
#elif defined __unix__
    dinfo.platform = "unix";
#else
    dinfo.platform = "unknown";
#endif

#if defined(__clang__)
    dinfo.compiler = "clang";
    dinfo.compiler_version = std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__) + "." + std::to_string(__clang_patchlevel__);
#elif defined(__GNUC__) || defined(__GNUG__)
    dinfo.compiler = "gcc";
    dinfo.compiler_version = std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." + std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
    dinfo.compiler = "msvc";
    dinfo.compiler_version = _MSC_VER;
#else
    dinfo.compiler = "unknown";
    dinfo.compiler_version = "unknown";
#endif

#ifdef __cplusplus
    dinfo.cpp = std::to_string(__cplusplus);
#else
    dinfo.cpp = "unknown";
#endif
    return dinfo;
}
