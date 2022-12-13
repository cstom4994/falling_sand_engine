
#include "DataPackage.h"

#include "Libs/physfs/physfs.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void TracePhysFSError(const char *detail) {}

unsigned char *LoadFileDataFromPhysFS(const char *fileName, unsigned int *bytesRead) {
    if (!FileExistsInPhysFS(fileName)) {

        *bytesRead = 0;
        return 0;
    }

    void *handle = PHYSFS_openRead(fileName);
    if (handle == 0) {
        TracePhysFSError(fileName);
        *bytesRead = 0;
        return 0;
    }

    int size = PHYSFS_fileLength(handle);
    if (size == -1) {
        *bytesRead = 0;
        PHYSFS_close(handle);

        return 0;
    }

    if (size == 0) {
        PHYSFS_close(handle);
        *bytesRead = 0;
        return 0;
    }

    void *buffer = malloc(size);
    int read = PHYSFS_readBytes(handle, buffer, size);
    if (read < 0) {
        *bytesRead = 0;
        free(buffer);
        PHYSFS_close(handle);
        TracePhysFSError(fileName);
        return 0;
    }

    PHYSFS_close(handle);
    *bytesRead = read;
    return buffer;
}

R_bool InitPhysFS() {

    if (PHYSFS_init(0) == 0) {
        TracePhysFSError("InitPhysFS() failed");
        return false;
    }

    //SetPhysFSWriteDirectory(GetWorkingDirectory());

    return true;
}

R_bool InitPhysFSEx(const char *newDir, const char *mountPoint) {
    if (InitPhysFS()) { return MountPhysFS(newDir, mountPoint); }
    return false;
}

R_bool IsPhysFSReady() { return PHYSFS_isInit() != 0; }

R_bool MountPhysFS(const char *newDir, const char *mountPoint) {
    if (PHYSFS_mount(newDir, mountPoint, 1) == 0) {
        TracePhysFSError(mountPoint);
        return false;
    }

    return true;
}

R_bool MountPhysFSFromMemory(const unsigned char *fileData, int dataSize, const char *newDir,
                             const char *mountPoint) {
    if (dataSize <= 0) { return false; }

    if (PHYSFS_mountMemory(fileData, dataSize, 0, newDir, mountPoint, 1) == 0) {
        //TracePhysFSError(sprintf("Failed to mount '%s' at '%s'", newDir, mountPoint));
        return false;
    }

    return true;
}

R_bool UnmountPhysFS(const char *oldDir) {
    if (PHYSFS_unmount(oldDir) == 0) { return false; }

    return true;
}

R_bool FileExistsInPhysFS(const char *fileName) {
    PHYSFS_Stat stat;
    if (PHYSFS_stat(fileName, &stat) == 0) { return false; }
    return stat.filetype == PHYSFS_FILETYPE_REGULAR;
}

R_bool DirectoryExistsInPhysFS(const char *dirPath) {
    PHYSFS_Stat stat;
    if (PHYSFS_stat(dirPath, &stat) == 0) { return false; }
    return stat.filetype == PHYSFS_FILETYPE_DIRECTORY;
}

R_bool SetPhysFSWriteDirectory(const char *newDir) {
    if (PHYSFS_setWriteDir(newDir) == 0) {
        TracePhysFSError(newDir);
        return false;
    }

    return true;
}

R_bool SaveFileDataToPhysFS(const char *fileName, void *data, unsigned int bytesToWrite) {

    if (bytesToWrite == 0) { return true; }

    void *handle = PHYSFS_openWrite(fileName);
    if (handle == 0) {
        TracePhysFSError(fileName);
        return false;
    }

    if (PHYSFS_writeBytes(handle, data, bytesToWrite) < 0) {
        PHYSFS_close(handle);
        TracePhysFSError(fileName);
        return false;
    }

    PHYSFS_close(handle);
    return true;
}

R_bool SaveFileTextToPhysFS(const char *fileName, char *text) {
    return SaveFileDataToPhysFS(fileName, text, strlen(text));
}

long GetFileModTimeFromPhysFS(const char *fileName) {
    PHYSFS_Stat stat;
    if (PHYSFS_stat(fileName, &stat) == 0) { return -1; }

    return stat.modtime;
}

R_bool ClosePhysFS() {
    if (PHYSFS_deinit() == 0) {
        TracePhysFSError("ClosePhysFS() unsuccessful");
        return false;
    }

    return true;
}

void SetPhysFSCallbacks() {
    // SetLoadFileDataCallback(LoadFileDataFromPhysFS);
    // SetSaveFileDataCallback(SaveFileDataToPhysFS);
    // SetLoadFileTextCallback(LoadFileTextFromPhysFS);
    // SetSaveFileTextCallback(SaveFileTextToPhysFS);
}

const char *GetPerfDirectory(const char *organization, const char *application) {
    const char *output = PHYSFS_getPrefDir(organization, application);
    if (output == 0) {
        TracePhysFSError("Failed to get perf directory");
        return 0;
    }

    return output;
}