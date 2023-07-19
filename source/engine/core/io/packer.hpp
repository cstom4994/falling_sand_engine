
#ifndef ME_PACKER_HPP
#define ME_PACKER_HPP

#include <cstdarg>
#include <cstddef>
#include <cstdlib>

#include "engine/core/basic_types.h"
#include "engine/core/macros.hpp"

#define PACK_HEADER_SIZE 8

namespace ME {

typedef enum ME_packresult_t {
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
} ME_packresult_t;

typedef u8 ME_pack_result;

typedef struct pack_iteminfo {
    u32 zipSize;
    u32 dataSize;
    u64 fileOffset;
    u8 pathSize;
} pack_iteminfo;

ME_PRIVATE(const char *const)
pack_result_strings[PACK_RESULT_COUNT] = {
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

inline static const char *pack_result_to_string(ME_pack_result result) {
    if (result >= PACK_RESULT_COUNT) return "Unknown PACK result";
    return pack_result_strings[result];
}

typedef struct ME_packreader_t ME_packreader_t;
typedef ME_packreader_t *ME_pack_reader;

ME_pack_result ME_create_file_pack_reader(const char *filePath, u32 dataBufferCapacity, bool isResourcesDirectory, ME_pack_reader *pack_reader);
void ME_destroy_pack_reader(ME_pack_reader pack_reader);
u64 ME_get_pack_item_count(ME_pack_reader pack_reader);
bool ME_get_pack_item_index(ME_pack_reader pack_reader, const char *path, u64 *index);
u32 ME_get_pack_item_data_size(ME_pack_reader pack_reader, u64 index);
const char *ME_get_pack_item_path(ME_pack_reader pack_reader, u64 index);
ME_pack_result ME_read_pack_item_data(ME_pack_reader pack_reader, u64 index, const u8 **data, u32 *size);
ME_pack_result ME_read_pack_path_item_data(ME_pack_reader pack_reader, const char *path, const u8 **data, u32 *size);
void ME_free_pack_reader_buffers(ME_pack_reader pack_reader);
ME_pack_result ME_unpack_files(const char *filePath, bool printProgress);
ME_pack_result ME_pack_files(const char *packPath, u64 fileCount, const char **filePaths, bool printProgress);

void ME_get_pack_library_version(u8 *majorVersion, u8 *minorVersion, u8 *patchVersion);
ME_pack_result ME_get_pack_info(const char *filePath, u8 *majorVersion, u8 *minorVersion, u8 *patchVersion, bool *isLittleEndian, u64 *itemCount);

}  // namespace ME

#endif