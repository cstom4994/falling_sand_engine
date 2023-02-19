
#pragma once

#include <stdbool.h>
#include <stdio.h>

#if __linux__ || __APPLE__
#define openFile(filePath, mode) fopen(filePath, mode)
#define seekFile(file, offset, whence) fseeko(file, offset, whence)
#define tellFile(file) ftello(file)
#elif _WIN32
inline static FILE* openFile(const char* filePath, const char* mode) {
    FILE* file;

    errno_t error = fopen_s(&file, filePath, mode);

    if (error != 0) return NULL;

    return file;
}

#define seekFile(file, offset, whence) _fseeki64(file, offset, whence)
#define tellFile(file) _ftelli64(file)
#else
#error Unsupported operating system
#endif

#define closeFile(file) fclose(file)

bool createDirectory(const char* path);
bool isDirectoryExists(const char* path);

#if __APPLE__
const char* getDataDirectory(bool isShared);
const char* getAppDataDirectory(const char* appName, bool isShared);
const char* getResourcesDirectory();
#endif
