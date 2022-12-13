
#include "Core/Core.h"

#if defined(__cplusplus)
extern "C"
{
#endif

    R_bool InitPhysFS();// Initialize the PhysFS file system
    R_bool InitPhysFSEx(
            const char *newDir,
            const char *mountPoint);// Initialize the PhysFS file system with a mount point.
    R_bool ClosePhysFS();           // Close the PhysFS file system
    R_bool IsPhysFSReady();         // Check if PhysFS has been initialized successfully
    R_bool MountPhysFS(
            const char *newDir,
            const char *mountPoint);// Mount the given directory or archive as a mount point
    R_bool MountPhysFSFromMemory(
            const unsigned char *fileData, int dataSize, const char *newDir,
            const char *mountPoint);                // Mount the given file data as a mount point
    R_bool UnmountPhysFS(const char *oldDir);       // Unmounts the given directory
    R_bool FileExistsInPhysFS(const char *fileName);// Check if the given file exists in PhysFS
    R_bool DirectoryExistsInPhysFS(
            const char *dirPath);// Check if the given directory exists in PhysFS
    unsigned char *LoadFileDataFromPhysFS(
            const char *fileName,
            unsigned int *bytesRead);// Load a data buffer from PhysFS (memory should be freed)
    char *LoadFileTextFromPhysFS(
            const char *fileName);// Load text from a file (memory should be freed)
    R_bool SetPhysFSWriteDirectory(
            const char *
                    newDir);// Set the base directory where PhysFS should write files to (defaults to the current working directory)
    R_bool SaveFileDataToPhysFS(const char *fileName, void *data,
                                unsigned int bytesToWrite);// Save the given file data in PhysFS
    R_bool SaveFileTextToPhysFS(const char *fileName,
                                char *text);// Save the given file text in PhysFS

    long GetFileModTimeFromPhysFS(
            const char *fileName);// Get file modification time (last write time) from PhysFS

    void SetPhysFSCallbacks();// Set the raylib file loader/saver callbacks to use PhysFS
    const char *GetPerfDirectory(
            const char *organization,
            const char *application);// Get the user's current config directory for the application.


#if defined(__cplusplus)
}
#endif