// Copyright(c) 2022, KaoruXun All rights reserved.

#include "filesystem.h"
#include "core/core.h"

#include "core/gc.h"
#include "engine/engine_cpp.h"
#include "engine/engine_platform.h"
#include "libs/physfs/physfs.h"
#include "platform_detail.h"

#include <stdio.h>
#include <string.h>
#include <sys/malloc.h>

char *ResourceWorker_ProjectRootPath;
char *ResourceWorker_DataPath;

void ResourceWorker_init() {

    char currentDir[MAX_PATH];
    strcpy(currentDir, FUtil_getExecutableFolderPath());
    char dir[MAX_PATH];
    strcpy(dir, currentDir);
    strcat(dir, "data/");
    if (!FUtil_exists(dir)) {
        strcpy(dir, currentDir);
        strcat(dir, "../");
        ResourceWorker_ProjectRootPath = dir;
        strcat(dir, "data/");
        ResourceWorker_DataPath = dir;
        if (!FUtil_exists(dir)) { METADOT_ERROR("Check runtime folder failed %s", dir); }
        METADOT_BUG("Runtime folder detected: %s", ResourceWorker_ProjectRootPath);
        return;
    }
    ResourceWorker_ProjectRootPath = currentDir;
    ResourceWorker_DataPath = dir;
    METADOT_BUG("Runtime folder detected: %s", ResourceWorker_ProjectRootPath);
    return;
}

char *ResourceWorker_getDataPath() { return ResourceWorker_DataPath; }

const char *FUtil_getExecutableFolderPath() {
    const char *out = PHYSFS_getBaseDir();
    return out;
}

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
            source = (char *) gc_malloc(&gc, sizeof(char) * (bufsize + 1));

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
