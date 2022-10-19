// Copyright(c) 2022, KaoruXun All rights reserved.

#include "FileSystem.hpp"
#include "Utils.hpp"

#define PHYSFS_DECL METAENGINE_PHYSFS_DEF
#include "Engine/lib/physfs/physfs.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace MetaEngine {


    std::string GameDir::getPath(std::string filePathRel) {
        return this->gameDir + filePathRel;
    }

    std::string GameDir::getWorldPath(std::string worldName) {
        return this->getPath("worlds/" + worldName);
    }

    void FUtil::cleanPathString(std::string &s) {
        SUtil::replaceWith(s, '\\', '/');
        SUtil::replaceWith(s, "//", "/", 2);
    }

    std::string FUtil::readFileString(std::string_view path) {
        auto p = METADOT_RESLOC(path);
        if (!exists(p))
            return "";
        const std::ifstream input_stream(p, std::ios_base::binary);

        if (input_stream.fail()) {
            METADOT_ERROR("Failed to open file {}", p);
            return "";
        }

        std::stringstream buffer;
        buffer << input_stream.rdbuf();
        return buffer.str();
    }

    uint64_t FUtil::lastWriteTime(std::string_view path) {
        return std::filesystem::last_write_time(path).time_since_epoch().count();
    }


    std::string FUtil::getAbsolutePath(const char *fileName) {
        std::string out = getExecutableFolderPath() + fileName;
        return std::filesystem::exists(out) ? out : "";
    }

    const std::string &FUtil::getExecutableFolderPath() {
        static std::string out;
        if (out.empty()) {
            out = getExecutablePath().substr(0, getExecutablePath().find_last_of('/') + 1);
        }
        return out;
    }

    const std::string &FUtil::getExecutablePath() {
        static std::string out;
#if defined(_WIN32)
        if (out.empty()) {
            WCHAR path[260];
            GetModuleFileNameW(NULL, path, 260);
            out = SUtil::ws2s(std::wstring(path));
            cleanPathString(out);
        }
#endif
        return out;
    }

    std::vector<std::string> FUtil::fileList(std::string_view folder_path, FileSearchFlags flags) {
        std::vector<std::string> out;
        std::filesystem::file_time_type bestTime;
        bool nope = flags & (FileSearchFlags_Oldest | FileSearchFlags_Newest);
        if (!FUtil::exists(folder_path))
            return out;

        if (!(flags & FileSearchFlags_Recursive)) {
            for (const auto &entry: std::filesystem::directory_iterator(folder_path)) {
#if defined(_MSC_VER)
                std::string s = SUtil::ws2s(std::wstring(entry.path().c_str()));
#else
                std::string s = SUtil::ws2s(std::wstring(entry.path().wstring()));
#endif
                if (entry.is_directory() && flags & FileSearchFlags_OnlyFiles) {
                } else if (entry.is_regular_file() && flags & FileSearchFlags_OnlyDirectories) {
                } else {
                    if (nope) {
                        nope = false;
                        bestTime = last_write_time(entry.path());
                        out.push_back(s);
                    }
                    auto currentTime = last_write_time(entry.path());

                    if (
                            ((flags & FileSearchFlags_Newest) && currentTime > bestTime) || ((flags & FileSearchFlags_Oldest) && currentTime < bestTime)) {
                        bestTime = currentTime;
                        out.clear();
                        out.push_back(s);
                    } else if (!(flags & (FileSearchFlags_Oldest | FileSearchFlags_Newest)))
                        out.push_back(s);
                }
            }
        } else {
            for (const auto &entry: std::filesystem::recursive_directory_iterator(folder_path)) {
#if defined(_MSC_VER)
                std::string s = SUtil::ws2s(std::wstring(entry.path().c_str()));
#else
                std::string s = SUtil::ws2s(std::wstring(entry.path().wstring()));
#endif
                if (entry.is_directory() && flags & FileSearchFlags_OnlyFiles) {
                } else if (entry.is_regular_file() && flags & FileSearchFlags_OnlyDirectories) {
                } else {
                    if (nope) {
                        nope = false;
                        bestTime = last_write_time(entry.path());
                        out.push_back(s);
                    }
                    auto currentTime = last_write_time(entry.path());

                    if (
                            ((flags & FileSearchFlags_Newest) && currentTime > bestTime) || ((flags & FileSearchFlags_Oldest) && currentTime < bestTime)) {
                        bestTime = currentTime;
                        out.clear();
                        out.push_back(s);
                    } else if (!(flags & (FileSearchFlags_Oldest | FileSearchFlags_Newest)))
                        out.push_back(s);
                }
            }
        }
        return out;
    }
}// namespace MetaEngine

// MetaDot Physfs Binding

extern "C"
{

    void TracePhysFSError(const char *detail) {
        PHYSFS_ErrorCode errorCode = PHYSFS_getLastErrorCode();
        if (errorCode == PHYSFS_ERR_OK) {
            METADOT_WARN("PHYSFS: {0}", detail);
        } else {
            const char *errorMessage = PHYSFS_getErrorByCode(errorCode);
            METADOT_WARN("PHYSFS: {0} ({1})", errorMessage, detail);
        }
    }

    unsigned char *LoadFileDataFromPhysFS(const char *fileName, unsigned int *bytesRead) {
        if (!FileExistsInPhysFS(fileName)) {
            METADOT_WARN("PHYSFS: Tried to load unexisting file {0}", fileName);
            *bytesRead = 0;
            return 0;
        }

        // Open up the file.
        PHYSFS_File *handle = PHYSFS_openRead(fileName);
        if (handle == 0) {
            TracePhysFSError(fileName);
            *bytesRead = 0;
            return 0;
        }

        // Check to see how large the file is.
        int size = PHYSFS_fileLength(handle);
        if (size == -1) {
            *bytesRead = 0;
            PHYSFS_close(handle);
            METADOT_WARN("PHYSFS: Cannot determine size of file {0}", fileName);
            return 0;
        }

        // Close safely when it's empty.
        if (size == 0) {
            PHYSFS_close(handle);
            *bytesRead = 0;
            return 0;
        }

        // Read the file, return if it's empty.
        void *buffer = METAENGINE_MALLOC(size);
        int read = PHYSFS_readBytes(handle, buffer, size);
        if (read < 0) {
            *bytesRead = 0;
            METAENGINE_FREE(buffer);
            PHYSFS_close(handle);
            TracePhysFSError(fileName);
            return 0;
        }

        // Close the file handle, and return the bytes read and the buffer.
        PHYSFS_close(handle);
        *bytesRead = read;
        return (unsigned char *) buffer;
    }

    bool InitPhysFS() {
        // Initialize PhysFS.
        if (PHYSFS_init(0) == 0) {
            TracePhysFSError("InitPhysFS() failed");
            return false;
        }

        // Set the default write directory, and report success.
        SetPhysFSWriteDirectory(METADOT_RESLOC_STR("data"));
        METADOT_TRACE("PHYSFS: Initialized PhysFS");
        return true;
    }

    bool IsPhysFSReady() {
        return PHYSFS_isInit() != 0;
    }

    bool MountPhysFS(const char *newDir, const char *mountPoint) {
        if (PHYSFS_mount(newDir, mountPoint, 1) == 0) {
            TracePhysFSError(mountPoint);
            return false;
        }

        METADOT_TRACE("PHYSFS: Mounted {0} at {1}", newDir, mountPoint);
        return true;
    }

    bool MountPhysFSFromMemory(const unsigned char *fileData, int dataSize, const char *newDir, const char *mountPoint) {
        if (dataSize <= 0) {
            METADOT_WARN("PHYSFS: Cannot mount a data size of 0");
            return false;
        }

        if (PHYSFS_mountMemory(fileData, dataSize, 0, newDir, mountPoint, 1) == 0) {
            TracePhysFSError(MetaEngine::Utils::Format("Failed to mount {0} at {1}", newDir, mountPoint).c_str());
            return false;
        }

        METADOT_TRACE("PHYSFS: Mounted memory {0} at {1}", newDir, mountPoint);
        return true;
    }

    bool UnmountPhysFS(const char *oldDir) {
        if (PHYSFS_unmount(oldDir) == 0) {
            METADOT_WARN("PHYSFS: Failed to unmount directory {0}", oldDir);
            return false;
        }

        METADOT_TRACE("PHYSFS: Unmounted {0}", oldDir);
        return true;
    }

    bool FileExistsInPhysFS(const char *fileName) {
        PHYSFS_Stat stat;
        if (PHYSFS_stat(fileName, &stat) == 0) {
            return false;
        }
        return stat.filetype == PHYSFS_FILETYPE_REGULAR;
    }

    bool DirectoryExistsInPhysFS(const char *dirPath) {
        PHYSFS_Stat stat;
        if (PHYSFS_stat(dirPath, &stat) == 0) {
            return false;
        }
        return stat.filetype == PHYSFS_FILETYPE_DIRECTORY;
    }

    char *LoadFileTextFromPhysFS(const char *fileName) {
        unsigned int bytesRead;
        return (char *) LoadFileDataFromPhysFS(fileName, &bytesRead);
    }

    bool SetPhysFSWriteDirectory(const char *newDir) {
        if (PHYSFS_setWriteDir(newDir) == 0) {
            TracePhysFSError(newDir);
            return false;
        }

        return true;
    }

    bool SaveFileDataToPhysFS(const char *fileName, void *data, unsigned int bytesToWrite) {
        // Protect against empty writes.
        if (bytesToWrite == 0) {
            return true;
        }

        // Open the file.
        PHYSFS_File *handle = PHYSFS_openWrite(fileName);
        if (handle == 0) {
            TracePhysFSError(fileName);
            return false;
        }

        // Write the data to the file handle.
        if (PHYSFS_writeBytes(handle, data, bytesToWrite) < 0) {
            PHYSFS_close(handle);
            TracePhysFSError(fileName);
            return false;
        }

        PHYSFS_close(handle);
        return true;
    }

    bool SaveFileTextToPhysFS(const char *fileName, char *text) {
        return SaveFileDataToPhysFS(fileName, text, std::strlen(text));
    }

    char **GetDirectoryFilesFromPhysFS(const char *dirPath, int *count) {
        // Make sure the directory exists.
        if (!DirectoryExistsInPhysFS(dirPath)) {
            METADOT_WARN("PHYSFS: Can't get files from non-existant directory ({0})", dirPath);
            return 0;
        }

        // Load the list of files from PhysFS.
        char **list = PHYSFS_enumerateFiles(dirPath);

        // Find out how many files there were.
        int number = 0;
        for (char **i = list; *i != 0; i++) {
            number++;
        }

        // Output the count and the list.
        *count = number;
        return list;
    }

    void ClearDirectoryFilesFromPhysFS(char **filesList) {
        PHYSFS_freeList(filesList);
    }

    long GetFileModTimeFromPhysFS(const char *fileName) {
        PHYSFS_Stat stat;
        if (PHYSFS_stat(fileName, &stat) == 0) {
            METADOT_WARN("PHYSFS: Cannot get mod time of file ({0})", fileName);
            return -1;
        }

        return stat.modtime;
    }

    bool ClosePhysFS() {
        if (PHYSFS_deinit() == 0) {
            TracePhysFSError("ClosePhysFS() unsuccessful");
            return false;
        }
        METADOT_TRACE("PHYSFS: Closed successfully");
        return true;
    }

    const char *GetPerfDirectory(const char *organization, const char *application) {
        const char *output = PHYSFS_getPrefDir(organization, application);
        if (output == 0) {
            TracePhysFSError("Failed to get perf directory");
            return 0;
        }
        METADOT_TRACE("PHYSFS: Perf Directory: {0}", output);
        return output;
    }
}
