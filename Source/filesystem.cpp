// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "filesystem.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include "core/alloc.h"
#include "core/core.h"
#include "core/cpp/utils.hpp"
#include "datapackage.h"
#include "engine.h"
#include "engine_platform.h"
#include "libs/physfs/physfs.h"
#include "platform_detail.h"

IMPLENGINE();

bool InitFilesystem() {

    InitPhysFS();

    auto currentDir = std::filesystem::path(FUtil_getExecutableFolderPath());

#ifdef METADOT_DEBUG

    for (int i = 0; i < 3; ++i) {
        currentDir = currentDir.parent_path();
        if (std::filesystem::exists(currentDir / "Data")) {
            Core.gamepath = currentDir;
            // s_DataPath = currentDir / "Data";
            METADOT_INFO("Game data path detected: %s", Core.gamepath.c_str());
            return METADOT_OK;
        }
    }

    METADOT_ERROR("Game data path detect failed");
    return METADOT_FAILED;

#else

    if (!std::filesystem::exists(currentDir / "Data")) {
        METADOT_ERROR("Game data path detect failed");
        return METADOT_FAILED;
    } else {
        Core.gamepath = currentDir;
        METADOT_INFO("Game data path detected: %s", Core.gamepath.c_str());
        return METADOT_OK;
    }

#endif
}

const char *FUtil_getExecutableFolderPath() {
    const char *out = PHYSFS_getBaseDir();
    return out;
}

// Char pointer get form futil_readfilestring must be gc_free manually
char *futil_readfilestring(const char *path) {
    char *source = NULL;
    FILE *fp = fopen(METADOT_RESLOC(path), "r");
    if (fp != NULL) {
        /* Go to the end of the file. */
        if (fseek(fp, 0L, SEEK_END) == 0) {
            /* Get the size of the file. */
            long bufsize = ftell(fp);
            if (bufsize == -1) { /* Error */
            }

            /* Allocate our buffer to that size. */
            source = (char *)gc_malloc(&gc, sizeof(char) * (bufsize + 1));

            /* Go back to the start of the file. */
            if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */
            }

            /* Read the entire file into memory. */
            size_t newLen = fread(source, sizeof(char), bufsize, fp);
            if (ferror(fp) != 0) {
                fputs("Error reading file", stderr);
                METADOT_ERROR("Error reading file %s", METADOT_RESLOC(path));
            } else {
                source[newLen++] = '\0'; /* Just to be safe. */
            }
        }
        fclose(fp);
        return source;
    }
    gc_free(&gc, source); /* Don't forget to call free() later! */
    return R_null;
}

void futil_freestring(void *ptr) {
    if (NULL != ptr) gc_free(&gc, ptr);
}
