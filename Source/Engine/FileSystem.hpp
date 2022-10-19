// Copyright(c) 2022, KaoruXun All rights reserved.


#pragma once

#include "Engine/InEngine.h"

#include "Engine/DebugImpl.hpp"

namespace MetaEngine {

#define FUTIL_ASSERT_EXIST(stringPath) METADOT_ASSERT(FUtil::exists(stringPath), MetaEngine::Utils::Format("FILE: {0} does not exist", stringPath))


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
        // executable path
        const std::string &getExecutablePath();

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

        uint64_t lastWriteTime(std::string_view path);
    }// namespace FUtil
}// namespace MetaEngine


#include "Engine/Core.hpp"
#include "Engine/Macros.hpp"
#include "Engine/Utils.hpp"

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

#ifdef METAENGINE_PHYSFS_IMPLEMENTATION
#ifndef METAENGINE_PHYSFS_IMPLEMENTATION_ONCE
#define METAENGINE_PHYSFS_IMPLEMENTATION_ONCE

// MiniPhysFS
#define PHYSFS_IMPL
#define PHYSFS_PLATFORM_IMPL
#define PHYSFS_DECL METAENGINE_PHYSFS_DEF
#include "Engine/lib/miniphysfs.h"// NOLINT

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Reports the last PhysFS error to raylib's TraceLog.
     *
     * @param detail Any additional detail to append to the reported error.
     *
     * @see PHYSFS_getLastErrorCode()
     *
     * @internal
     */
    void TracePhysFSError(const char *detail) {
        PHYSFS_ErrorCode errorCode = PHYSFS_getLastErrorCode();
        if (errorCode == PHYSFS_ERR_OK) {
            METADOT_WARN("PHYSFS: {0}", detail);
        } else {
            const char *errorMessage = PHYSFS_getErrorByCode(errorCode);
            METADOT_WARN("PHYSFS: {0} ({1})", errorMessage, detail);
        }
    }

    /**
     * Loads the given file as a byte array from PhysFS (read).
     *
     * @param fileName The file to load.
     * @param bytesRead An unsigned integer to save the bytes that were read.
     *
     * @return The file data as a pointer. Make sure to use UnloadFileData() when finished using the file data.
     *
     * @see UnloadFileData()
     */
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

    /**
     * Initialize the PhysFS virtual file system.
     *
     * @return True on success, false on failure.
     *
     * @see ClosePhysFS()
     */
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

    /**
     * Check if PhysFS has been initialized successfully.
     *
     * @return True if PhysFS is initialized, false otherwise.
     *
     * @see InitPhysFS()
     */
    bool IsPhysFSReady() {
        return PHYSFS_isInit() != 0;
    }

    /**
     * Mounts the given directory, at the given mount point.
     *
     * @param newDir Directory or archive to add to the path, in platform-dependent notation.
     * @param mountPoint Location in the interpolated tree that this archive will be "mounted", in platform-independent notation. NULL or "" is equivalent to "/".
     *
     * @return True on success, false on failure.
     *
     * @see UnmountPhysFS()
     */
    bool MountPhysFS(const char *newDir, const char *mountPoint) {
        if (PHYSFS_mount(newDir, mountPoint, 1) == 0) {
            TracePhysFSError(mountPoint);
            return false;
        }

        METADOT_TRACE("PHYSFS: Mounted {0} at {1}", newDir, mountPoint);
        return true;
    }

    /**
     * Mounts the given file data as a mount point in PhysFS.
     *
     * @param fileData The archive data as a file buffer.
     * @param dataSize The size of the file buffer.
     * @param newDir A filename that can represent the file data. Has to be unique. For example: data.zip
     * @param mountPoint The location in the tree that the archive will be mounted.
     *
     * @return True on success, false on failure.
     *
     * @see MountPhysFS()
     */
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

    /**
     * Unmounts the given directory or archive.
     *
     * @param oldDir The directory that was supplied to MountPhysFS's newDir.
     *
     * @return True on success, false on failure.
     *
     * @see MountPhysFS()
     */
    bool UnmountPhysFS(const char *oldDir) {
        if (PHYSFS_unmount(oldDir) == 0) {
            METADOT_WARN("PHYSFS: Failed to unmount directory {0}", oldDir);
            return false;
        }

        METADOT_TRACE("PHYSFS: Unmounted {0}", oldDir);
        return true;
    }

    /**
     * Determine if a file exists in the search path.
     *
     * @param fileName Filename in platform-independent notation.
     *
     * @return True if the file exists, false otherwise.
     *
     * @see DirectoryExistsInPhysFS()
     */
    bool FileExistsInPhysFS(const char *fileName) {
        PHYSFS_Stat stat;
        if (PHYSFS_stat(fileName, &stat) == 0) {
            return false;
        }
        return stat.filetype == PHYSFS_FILETYPE_REGULAR;
    }

    /**
     * Determine if a directory exists in the search path.
     *
     * @param dirPath Directory in platform-independent notation.
     *
     * @return True if the directory exists, false otherwise.
     *
     * @see FileExistsInPhysFS()
     */
    bool DirectoryExistsInPhysFS(const char *dirPath) {
        PHYSFS_Stat stat;
        if (PHYSFS_stat(dirPath, &stat) == 0) {
            return false;
        }
        return stat.filetype == PHYSFS_FILETYPE_DIRECTORY;
    }


    /**
     * Load text data from file (read). Make sure to call UnloadFileText() when done.
     *
     * @param fileName The file name to load from the PhysFS mount paths.
     *
     * @return A '\0' terminated string.
     *
     * @see UnloadFileText()
     */
    char *LoadFileTextFromPhysFS(const char *fileName) {
        unsigned int bytesRead;
        return (char *) LoadFileDataFromPhysFS(fileName, &bytesRead);
    }

    /**
     * Sets where PhysFS will attempt to write files. Defaults to the current working directory.
     *
     * @param newDir The new directory to be the root for writing files.
     *
     * @return True on success, false on failure.
     */
    bool SetPhysFSWriteDirectory(const char *newDir) {
        if (PHYSFS_setWriteDir(newDir) == 0) {
            TracePhysFSError(newDir);
            return false;
        }

        return true;
    }

    /**
     * Save file data to file (write).
     *
     * @param fileName The name of the file to save.
     * @param data The data to be saved.
     * @param bytesToWrite The amount of bytes that are to be written.
     *
     * @return True on success, false on failure.
     */
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

    /**
     * Save text data to file (write).
     *
     * @param fileName The name of the file to save.
     * @param text A '\0' terminated string.
     *
     * @return True on success, false on failure.
     */
    bool SaveFileTextToPhysFS(const char *fileName, char *text) {
        return SaveFileDataToPhysFS(fileName, text, std::strlen(text));
    }

    /**
     * Gets a list of files in the given directory in PhysFS.
     *
     * Make sure to clear the loaded list by using ClearDirectoryFilesFromPhysFS().
     *
     * @see ClearDirectoryFilesFromPhysFS()
     */
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

    /**
     * Clears the loaded list of directory files from GetDirectoryFilesFromPhysFS().
     *
     * @param filesList The list of files to clear that was provided from GetDirectoryFilesFromPhysFS().
     *
     * @see GetDirectoryFilesFromPhysFS()
     */
    void ClearDirectoryFilesFromPhysFS(char **filesList) {
        PHYSFS_freeList(filesList);
    }

    /**
     * Get file modification time (last write time) from a file in PhysFS.
     *
     * @param fileName The file to retrieve the mod time for.
     *
     * @return The modification time (last write time) of the given file. -1 on failure.
     *
     * @see GetFileModTime()
     */
    long GetFileModTimeFromPhysFS(const char *fileName) {
        PHYSFS_Stat stat;
        if (PHYSFS_stat(fileName, &stat) == 0) {
            METADOT_WARN("PHYSFS: Cannot get mod time of file ({0})", fileName);
            return -1;
        }

        return stat.modtime;
    }

    /**
     * Close the PhysFS virtual file system.
     *
     * @return True on success, false on failure.
     */
    bool ClosePhysFS() {
        if (PHYSFS_deinit() == 0) {
            TracePhysFSError("ClosePhysFS() unsuccessful");
            return false;
        }
        METADOT_TRACE("PHYSFS: Closed successfully");
        return true;
    }

    /**
     * Get the user's configuration directory for the application.
     *
     * @param organization The name of your organization.
     * @param application The name of your application.
     *
     * @return string of user directory in platform-dependent notation.
     *         NULL if there's a problem (creating directory failed, etc)
     */
    const char *GetPerfDirectory(const char *organization, const char *application) {
        const char *output = PHYSFS_getPrefDir(organization, application);
        if (output == 0) {
            TracePhysFSError("Failed to get perf directory");
            return 0;
        }
        METADOT_TRACE("PHYSFS: Perf Directory: {0}", output);
        return output;
    }

#ifdef __cplusplus
}
#endif

#endif// METAENGINE_PHYSFS_IMPLEMENTATION_ONCE
#endif// METAENGINE_PHYSFS_IMPLEMENTATION
