
#include "io.h"

#include <assert.h>

#if __linux__ || __APPLE__
#include <sys/stat.h>

bool createDirectory(const char* path) {
    assert(path);
    return mkdir(path, 0777) == 0;
}
bool isDirectoryExists(const char* path) {
    assert(path);
    struct stat sb;
    return stat(path, &sb) == 0 && S_ISDIR(sb.st_mode);
}
#elif _WIN32
#include <windows.h>

bool createDirectory(const char* path) {
    assert(path);
    return CreateDirectoryA(path, NULL) == TRUE;
}
bool isDirectoryExists(const char* path) {
    assert(path);
    DWORD attribs = GetFileAttributes(path);

    return (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY));
}
#else
#error Unknown operating system
#endif
