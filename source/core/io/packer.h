
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PACK_VERSION_MAJOR 0
#define PACK_VERSION_MINOR 0
#define PACK_VERSION_PATCH 1
#define PACK_LITTLE_ENDIAN 1

#define PACK_HEADER_SIZE 8

typedef enum packresult_t {
    SUCCESS_PACK_RESULT = 0,
    FAILED_TO_ALLOCATE_PACK_RESULT = 1,
    FAILED_TO_CREATE_LZ4_PACK_RESULT = 2,
    FAILED_TO_CREATE_FILE_PACK_RESULT = 3,
    FAILED_TO_OPEN_FILE_PACK_RESULT = 4,
    FAILED_TO_WRITE_FILE_PACK_RESULT = 5,
    FAILED_TO_READ_FILE_PACK_RESULT = 6,
    FAILED_TO_SEEK_FILE_PACK_RESULT = 7,
    FAILED_TO_GET_DIRECTORY_PACK_RESULT = 8,
    FAILED_TO_DECOMPRESS_PACK_RESULT = 9,
    FAILED_TO_GET_ITEM_PACK_RESULT = 10,
    BAD_DATA_SIZE_PACK_RESULT = 11,
    BAD_FILE_TYPE_PACK_RESULT = 12,
    BAD_FILE_VERSION_PACK_RESULT = 13,
    BAD_FILE_ENDIANNESS_PACK_RESULT = 14,
    PACK_RESULT_COUNT = 15,
} packresult_t;

typedef uint8_t pack_result;

typedef struct pack_iteminfo {
    uint32_t zipSize;
    uint32_t dataSize;
    uint64_t fileOffset;
    uint8_t pathSize;
} pack_iteminfo;

static const char* const packResultStrings[PACK_RESULT_COUNT] = {
        "Success",
        "Failed to allocate",
        "Failed to create LZ4",
        "Failed to create file",
        "Failed to open file",
        "Failed to write file",
        "Failed to read file",
        "Failed to seek file",
        "Failed to get directory",
        "Failed to decompress",
        "Failed to get item",
        "Bad data size",
        "Bad file type",
        "Bad file version",
        "Bad file endianness",
};

inline static const char* packResultToString(pack_result result) {
    if (result >= PACK_RESULT_COUNT) return "Unknown PACK result";
    return packResultStrings[result];
}

#ifdef __cplusplus
extern "C" {
#endif

typedef struct packreader_t packreader_t;
typedef packreader_t* pack_reader;

pack_result createFilePackReader(const char* filePath, uint32_t dataBufferCapacity, bool isResourcesDirectory, pack_reader* packReader);
void destroyPackReader(pack_reader packReader);
uint64_t getPackItemCount(pack_reader packReader);
bool getPackItemIndex(pack_reader packReader, const char* path, uint64_t* index);
uint32_t getPackItemDataSize(pack_reader packReader, uint64_t index);
const char* getPackItemPath(pack_reader packReader, uint64_t index);
pack_result readPackItemData(pack_reader packReader, uint64_t index, const uint8_t** data, uint32_t* size);
pack_result readPackPathItemData(pack_reader packReader, const char* path, const uint8_t** data, uint32_t* size);
void freePackReaderBuffers(pack_reader packReader);
pack_result unpackFiles(const char* filePath, bool printProgress);
pack_result packFiles(const char* packPath, uint64_t fileCount, const char** filePaths, bool printProgress);

void getPackLibraryVersion(uint8_t* majorVersion, uint8_t* minorVersion, uint8_t* patchVersion);
pack_result getPackInfo(const char* filePath, uint8_t* majorVersion, uint8_t* minorVersion, uint8_t* patchVersion, bool* isLittleEndian, uint64_t* itemCount);

#ifdef __cplusplus
}
#endif