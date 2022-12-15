// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_FILESYSTEM_HPP_
#define _METADOT_FILESYSTEM_HPP_

#include "core/core.h"
#include "core/macros.h"
#include "platform_detail.h"

#include <string.h>

// #define FUTIL_ASSERT_EXIST(stringPath)                                                             \
//     METADOT_ASSERT(FUtil_exists(METADOT_RESLOC(stringPath)),                                       \
//                    ("%s", MetaEngine::Format("FILE: {0} does not exist", stringPath)))

#define FUTIL_ASSERT_EXIST(stringPath)

#if defined(__cplusplus)
extern "C"
{
#endif

    extern char *ResourceWorker_ProjectRootPath;
    extern char *ResourceWorker_DataPath;

    void ResourceWorker_init();
    char *ResourceWorker_getDataPath();

#define METADOT_RESLOC(x) x

    inline R_bool FUtil_exists(const char *path) {
        struct stat buffer;
        return (stat(path, &buffer) == 0 || S_ISDIR(buffer.st_mode));
    }

    // folder path of current executable
    const char *FUtil_getExecutableFolderPath();

    inline const char *FUtil_GetFileName(const char *path) {
        int len = strlen(path);
        int flag = 0;

        for (int i = len - 1; i > 0; i--) {
            if (path[i] == '\\' || path[i] == '//' || path[i] == '/') {
                flag = 1;
                path = path + i + 1;
                break;
            }
        }
        return path;
    }

    char *futil_readfilestring(const char *path);

#if defined(__cplusplus)
}
#endif

#endif
