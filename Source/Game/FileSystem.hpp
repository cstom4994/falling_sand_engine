// Copyright(c) 2022, KaoruXun All rights reserved.


#pragma once

#include "Game/Core.hpp"
#include "Game/DebugImpl.hpp"
#include "Game/InEngine.h"
#include "Game/Macros.hpp"
#include "Game/Utils.hpp"

namespace MetaEngine {

#define FUTIL_ASSERT_EXIST(stringPath) METADOT_ASSERT(FUtil::exists(stringPath), MetaEngine::Utils::Format("FILE: {0} does not exist", stringPath))

    class ResourceMan {
    private:
        static std::string s_ProjectRootPath;
        static std::string s_ExeRootPath;
        static std::string s_DataPath;
        static std::string s_ScriptPath;

    public:
        static std::string getResourceLoc(std::string_view resPath);
        static std::string getLocalPath(std::string_view resPath);
        static void init();
        static const std::string &getDataPath();
    };

#define METADOT_RESLOC(x) MetaEngine::ResourceMan::getResourceLoc(x)

#define METADOT_RESLOC_STR(x) METADOT_RESLOC(x).c_str()

    class GameDir {
        std::string gameDir;

    public:
        GameDir(std::string gameDir) {
            this->gameDir = gameDir;
        }

        GameDir() {
            gameDir = "";
        }

        std::string getPath(std::string filePathRel);
        std::string getWorldPath(std::string worldName);
    };


    namespace FUtil {
        inline bool exists(std::string_view path) { return std::filesystem::exists(METADOT_RESLOC(path)); }

        //inline void assertFileExists(std::string_view path) {
        //	ASSERT(FUtil::exists(ND_RESLOC(path)), "FILE: {0} does not exist", path);
        //}
        // folder path of current executable
        const std::string &getExecutableFolderPath();

        typedef int FileSearchFlags;

        enum FileSearchFlags_ {
            FileSearchFlags_None = 0,
            FileSearchFlags_Recursive = 1 << 0,
            FileSearchFlags_OnlyDirectories = 1 << 1,
            FileSearchFlags_OnlyFiles = 1 << 2,
            // return newest folder/file with in folder
            FileSearchFlags_Newest = 1 << 3,
            // return oldest folder/file with in folder
            FileSearchFlags_Oldest = 1 << 4,
        };

        std::vector<std::string> fileList(std::string_view folder_path, FileSearchFlags flags = FileSearchFlags_None);

        // absolute path of fileName in the same folder as executable
        // or "" if filename doesn't exist
        std::string getAbsolutePath(const char *fileName);

        void cleanPathString(std::string &s);

        inline std::string cleanPathStringConst(std::string_view s) {
            auto out = std::string(s);
            cleanPathString(out);
            return out;
        }


        inline void removeSuffix(std::string &s) {
            if (s.find_last_of('.') != std::string::npos)
                s = s.substr(0, s.find_last_of('.'));
        }

        inline std::string removeSuffixConst(std::string_view s) {
            auto out = std::string(s);
            removeSuffix(out);
            return out;
        }

        inline bool copyFile(std::string_view src, std::string_view dest) {
            std::filesystem::copy(src, dest);
            return true;
        }

        std::string readFileString(std::string_view path);

        auto ReadFile(const std::string &filename);

        uint64_t lastWriteTime(std::string_view path);
    }// namespace FUtil
}// namespace MetaEngine


#ifndef INCLUDE_METAENGINE_PHYSFS_H_
#define INCLUDE_METAENGINE_PHYSFS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef METAENGINE_PHYSFS_DEF
#ifdef METAENGINE_PHYSFS_STATIC
#define METAENGINE_PHYSFS_DEF static
#else
#define METAENGINE_PHYSFS_DEF extern
#endif
#endif

    METAENGINE_PHYSFS_DEF bool InitPhysFS();                                                                                                  // Initialize the PhysFS file system
    METAENGINE_PHYSFS_DEF bool ClosePhysFS();                                                                                                 // Close the PhysFS file system
    METAENGINE_PHYSFS_DEF bool IsPhysFSReady();                                                                                               // Check if PhysFS has been initialized successfully
    METAENGINE_PHYSFS_DEF bool MountPhysFS(const char *newDir, const char *mountPoint);                                                       // Mount the given directory or archive as a mount point
    METAENGINE_PHYSFS_DEF bool MountPhysFSFromMemory(const unsigned char *fileData, int dataSize, const char *newDir, const char *mountPoint);// Mount the given file data as a mount point
    METAENGINE_PHYSFS_DEF bool UnmountPhysFS(const char *oldDir);                                                                             // Unmounts the given directory
    METAENGINE_PHYSFS_DEF bool FileExistsInPhysFS(const char *fileName);                                                                      // Check if the given file exists in PhysFS
    METAENGINE_PHYSFS_DEF bool DirectoryExistsInPhysFS(const char *dirPath);                                                                  // Check if the given directory exists in PhysFS
    METAENGINE_PHYSFS_DEF unsigned char *LoadFileDataFromPhysFS(const char *fileName, unsigned int *bytesRead);                               // Load a data buffer from PhysFS (memory should be freed)
    METAENGINE_PHYSFS_DEF char *LoadFileTextFromPhysFS(const char *fileName);                                                                 // Load text from a file (memory should be freed)
    METAENGINE_PHYSFS_DEF bool SetPhysFSWriteDirectory(const char *newDir);                                                                   // Set the base directory where PhysFS should write files to (defaults to the current working directory)
    METAENGINE_PHYSFS_DEF bool SaveFileDataToPhysFS(const char *fileName, void *data, unsigned int bytesToWrite);                             // Save the given file data in PhysFS
    METAENGINE_PHYSFS_DEF bool SaveFileTextToPhysFS(const char *fileName, char *text);                                                        // Save the given file text in PhysFS
    METAENGINE_PHYSFS_DEF char **GetDirectoryFilesFromPhysFS(const char *dirPath, int *count);                                                // Get filenames in a directory path (memory should be freed)
    METAENGINE_PHYSFS_DEF void ClearDirectoryFilesFromPhysFS(char **filesList);                                                               // Clear directory files paths buffers (free memory)
    METAENGINE_PHYSFS_DEF long GetFileModTimeFromPhysFS(const char *fileName);                                                                // Get file modification time (last write time) from PhysFS

    METAENGINE_PHYSFS_DEF void SetPhysFSCallbacks();                                                      // Set the raylib file loader/saver callbacks to use PhysFS
    METAENGINE_PHYSFS_DEF const char *GetPerfDirectory(const char *organization, const char *application);// Get the user's current config directory for the application.

#ifdef __cplusplus
}
#endif

#endif// INCLUDE_METAENGINE_PHYSFS_H_
