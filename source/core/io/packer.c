
#include "packer.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"
#include "libs/lz4/lz4.h"

typedef struct pack_item {
    pack_iteminfo info;
    char* path;
} pack_item;

struct packreader_t {
    FILE* file;
    uint64_t itemCount;
    pack_item* items;
    uint8_t* dataBuffer;
    uint8_t* zipBuffer;
    uint32_t dataSize;
    uint32_t zipSize;
    pack_item searchItem;
};

inline static void destroyPackItems(uint64_t itemCount, pack_item* items) {
    assert(itemCount == 0 || (itemCount > 0 && items));

    for (uint64_t i = 0; i < itemCount; i++) free(items[i].path);
    free(items);
}
inline static pack_result createPackItems(FILE* packFile, uint64_t itemCount, pack_item** _items) {
    assert(packFile);
    assert(itemCount > 0);
    assert(_items);

    pack_item* items = malloc(itemCount * sizeof(pack_item));

    if (!items) return FAILED_TO_ALLOCATE_PACK_RESULT;

    for (uint64_t i = 0; i < itemCount; i++) {
        pack_iteminfo info;

        size_t result = fread(&info, sizeof(pack_iteminfo), 1, packFile);

        if (result != 1) {
            destroyPackItems(i, items);
            return FAILED_TO_READ_FILE_PACK_RESULT;
        }

        if (info.dataSize == 0 || info.pathSize == 0) {
            destroyPackItems(i, items);
            return BAD_DATA_SIZE_PACK_RESULT;
        }

        char* path = malloc((info.pathSize + 1) * sizeof(char));

        if (!path) {
            destroyPackItems(i, items);
            return FAILED_TO_ALLOCATE_PACK_RESULT;
        }

        result = fread(path, sizeof(char), info.pathSize, packFile);

        path[info.pathSize] = 0;

        if (result != info.pathSize) {
            destroyPackItems(i, items);
            return FAILED_TO_READ_FILE_PACK_RESULT;
        }

        int64_t fileOffset = info.zipSize > 0 ? info.zipSize : info.dataSize;

        int seekResult = seekFile(packFile, fileOffset, SEEK_CUR);

        if (seekResult != 0) {
            destroyPackItems(i, items);
            return FAILED_TO_SEEK_FILE_PACK_RESULT;
        }

        pack_item* item = &items[i];
        item->info = info;
        item->path = path;
    }

    *_items = items;
    return SUCCESS_PACK_RESULT;
}
pack_result createFilePackReader(const char* filePath, uint32_t dataBufferCapacity, bool isResourcesDirectory, pack_reader* packReader) {
    assert(filePath);
    assert(packReader);

    pack_reader pack = calloc(1, sizeof(packreader_t));

    if (!pack) return FAILED_TO_ALLOCATE_PACK_RESULT;

    pack->zipBuffer = NULL;
    pack->zipSize = 0;

    char* path;

#if __APPLE__
    if (isResourcesDirectory) {
        const char* resourcesDirectory = getResourcesDirectory();

        if (!resourcesDirectory) {
            destroyPackReader(pack);
            return FAILED_TO_GET_DIRECTORY_PACK_RESULT;
        }

        size_t filePathLength = strlen(filePath);
        size_t resourcesPathLength = strlen(resourcesDirectory);
        size_t pathLength = filePathLength + resourcesPathLength + 2;

        path = malloc(pathLength * sizeof(char));

        if (!path) {
            destroyPackReader(pack);
            return FAILED_TO_ALLOCATE_PACK_RESULT;
        }

        memcpy(path, resourcesDirectory, resourcesPathLength * sizeof(char));
        path[resourcesPathLength] = '/';
        memcpy(path + resourcesPathLength + 1, filePath, filePathLength * sizeof(char));
        path[resourcesPathLength + filePathLength + 1] = '\0';
    } else {
        path = (char*)filePath;
    }
#else
    path = (char*)filePath;
#endif

    FILE* file = openFile(path, "rb");

#if __APPLE__
    if (isResourcesDirectory) free(path);
#endif

    if (!file) {
        destroyPackReader(pack);
        return FAILED_TO_OPEN_FILE_PACK_RESULT;
    }

    pack->file = file;

    char header[PACK_HEADER_SIZE];

    size_t result = fread(header, sizeof(char), PACK_HEADER_SIZE, file);

    if (result != PACK_HEADER_SIZE) {
        destroyPackReader(pack);
        return FAILED_TO_READ_FILE_PACK_RESULT;
    }

    if (header[0] != 'P' || header[1] != 'A' || header[2] != 'C' || header[3] != 'K') {
        destroyPackReader(pack);
        return BAD_FILE_TYPE_PACK_RESULT;
    }

    if (header[4] != PACK_VERSION_MAJOR || header[5] != PACK_VERSION_MINOR) {
        destroyPackReader(pack);
        return BAD_FILE_VERSION_PACK_RESULT;
    }

    // Skipping PATCH version check

    if (header[7] != !PACK_LITTLE_ENDIAN) {
        destroyPackReader(pack);
        return BAD_FILE_ENDIANNESS_PACK_RESULT;
    }

    uint64_t itemCount;

    result = fread(&itemCount, sizeof(uint64_t), 1, file);

    if (result != 1) {
        destroyPackReader(pack);
        return FAILED_TO_READ_FILE_PACK_RESULT;
    }

    if (itemCount == 0) {
        destroyPackReader(pack);
        return BAD_DATA_SIZE_PACK_RESULT;
    }

    pack_item* items;

    pack_result packResult = createPackItems(file, itemCount, &items);

    if (packResult != SUCCESS_PACK_RESULT) {
        destroyPackReader(pack);
        ;
        return packResult;
    }

    pack->itemCount = itemCount;
    pack->items = items;

    uint8_t* dataBuffer;

    if (dataBufferCapacity > 0) {
        dataBuffer = malloc(dataBufferCapacity * sizeof(uint8_t));

        if (!dataBuffer) {
            destroyPackReader(pack);
            return FAILED_TO_ALLOCATE_PACK_RESULT;
        }
    } else {
        dataBuffer = NULL;
    }

    pack->dataBuffer = dataBuffer;
    pack->dataSize = dataBufferCapacity;

    *packReader = pack;
    return SUCCESS_PACK_RESULT;
}
void destroyPackReader(pack_reader packReader) {
    if (!packReader) return;

    free(packReader->dataBuffer);
    free(packReader->zipBuffer);
    destroyPackItems(packReader->itemCount, packReader->items);
    if (packReader->file) closeFile(packReader->file);
    free(packReader);
}

uint64_t getPackItemCount(pack_reader packReader) {
    assert(packReader);
    return packReader->itemCount;
}

static int comparePackItems(const void* _a, const void* _b) {
    // NOTE: a and b should not be NULL!
    // Skipping here assertions for debug build speed.

    const pack_item* a = _a;
    const pack_item* b = _b;

    int difference = (int)a->info.pathSize - (int)b->info.pathSize;

    if (difference != 0) return difference;

    return memcmp(a->path, b->path, a->info.pathSize * sizeof(char));
}
bool getPackItemIndex(pack_reader packReader, const char* path, uint64_t* index) {
    assert(packReader);
    assert(path);
    assert(index);
    assert(strlen(path) <= UINT8_MAX);

    pack_item* searchItem = &packReader->searchItem;

    searchItem->info.pathSize = (uint8_t)strlen(path);
    searchItem->path = (char*)path;

    pack_item* item = bsearch(searchItem, packReader->items, packReader->itemCount, sizeof(pack_item), comparePackItems);

    if (!item) return false;

    *index = item - packReader->items;
    return true;
}

uint32_t getPackItemDataSize(pack_reader packReader, uint64_t index) {
    assert(packReader);
    assert(index < packReader->itemCount);
    return packReader->items[index].info.dataSize;
}

const char* getPackItemPath(pack_reader packReader, uint64_t index) {
    assert(packReader);
    assert(index < packReader->itemCount);
    return packReader->items[index].path;
}

pack_result readPackItemData(pack_reader packReader, uint64_t index, const uint8_t** data, uint32_t* size) {
    assert(packReader);
    assert(index < packReader->itemCount);
    assert(data);
    assert(size);

    pack_iteminfo info = packReader->items[index].info;
    uint8_t* dataBuffer = packReader->dataBuffer;

    if (dataBuffer) {
        if (info.dataSize > packReader->dataSize) {
            dataBuffer = realloc(dataBuffer, info.dataSize * sizeof(uint8_t));

            if (!dataBuffer) return FAILED_TO_ALLOCATE_PACK_RESULT;

            packReader->dataBuffer = dataBuffer;
            packReader->dataSize = info.dataSize;
        }
    } else {
        dataBuffer = malloc(info.dataSize * sizeof(uint8_t));

        if (!dataBuffer) return FAILED_TO_ALLOCATE_PACK_RESULT;

        packReader->dataBuffer = dataBuffer;
        packReader->dataSize = info.dataSize;
    }

    uint8_t* zipBuffer = packReader->zipBuffer;

    if (zipBuffer) {
        if (info.zipSize > packReader->zipSize) {
            zipBuffer = realloc(zipBuffer, info.zipSize * sizeof(uint8_t));

            if (!zipBuffer) return FAILED_TO_ALLOCATE_PACK_RESULT;

            packReader->zipBuffer = zipBuffer;
            packReader->zipSize = info.zipSize;
        }
    } else {
        if (info.zipSize > 0) {
            zipBuffer = malloc(info.zipSize * sizeof(uint8_t));

            if (!zipBuffer) return FAILED_TO_ALLOCATE_PACK_RESULT;

            packReader->zipBuffer = zipBuffer;
            packReader->zipSize = info.zipSize;
        }
    }

    FILE* file = packReader->file;

    int64_t fileOffset = (int64_t)(info.fileOffset + sizeof(pack_iteminfo) + info.pathSize);

    int seekResult = seekFile(file, fileOffset, SEEK_SET);

    if (seekResult != 0) return FAILED_TO_SEEK_FILE_PACK_RESULT;

    if (info.zipSize > 0) {
        size_t result = fread(zipBuffer, sizeof(uint8_t), info.zipSize, file);

        if (result != info.zipSize) return FAILED_TO_READ_FILE_PACK_RESULT;

        const int decompressed_size = LZ4_decompress_safe(zipBuffer, dataBuffer, info.zipSize, info.dataSize);

        if (decompressed_size < 0 || result != info.dataSize) {
            return FAILED_TO_DECOMPRESS_PACK_RESULT;
        }
    } else {
        size_t result = fread(dataBuffer, sizeof(uint8_t), info.dataSize, file);

        if (result != info.dataSize) return FAILED_TO_READ_FILE_PACK_RESULT;
    }

    *data = dataBuffer;
    *size = info.dataSize;
    return SUCCESS_PACK_RESULT;
}

pack_result readPackPathItemData(pack_reader packReader, const char* path, const uint8_t** data, uint32_t* size) {
    assert(packReader);
    assert(path);
    assert(data);
    assert(size);
    assert(strlen(path) <= UINT8_MAX);

    uint64_t index;

    if (!getPackItemIndex(packReader, path, &index)) {
        return FAILED_TO_GET_ITEM_PACK_RESULT;
    }

    return readPackItemData(packReader, index, data, size);
}

void freePackReaderBuffers(pack_reader packReader) {
    assert(packReader);
    free(packReader->dataBuffer);
    free(packReader->zipBuffer);
    packReader->dataBuffer = NULL;
    packReader->zipBuffer = NULL;
}

inline static void removePackItemFiles(uint64_t itemCount, pack_item* packItems) {
    assert(itemCount == 0 || (itemCount > 0 && packItems));

    for (uint64_t i = 0; i < itemCount; i++) remove(packItems[i].path);
}
pack_result unpackFiles(const char* filePath, bool printProgress) {
    assert(filePath);

    pack_reader packReader;

    pack_result packResult = createFilePackReader(filePath, 0, false, &packReader);

    if (packResult != SUCCESS_PACK_RESULT) return packResult;

    uint64_t totalRawSize = 0, totalZipSize = 0;

    uint64_t itemCount = packReader->itemCount;
    pack_item* items = packReader->items;

    for (uint64_t i = 0; i < itemCount; i++) {
        pack_item* item = &items[i];

        if (printProgress) {
            printf("Unpacking \"%s\" file. ", item->path);
            fflush(stdout);
        }

        const uint8_t* dataBuffer;
        uint32_t dataSize;

        packResult = readPackItemData(packReader, i, &dataBuffer, &dataSize);

        if (packResult != SUCCESS_PACK_RESULT) {
            removePackItemFiles(i, items);
            destroyPackReader(packReader);
            return packResult;
        }

        uint8_t pathSize = item->info.pathSize;

        char itemPath[UINT8_MAX + 1];

        memcpy(itemPath, item->path, pathSize * sizeof(char));
        itemPath[pathSize] = 0;

        for (uint8_t j = 0; j < pathSize; j++) {
            if (itemPath[j] == '/' || itemPath[j] == '\\') {
                itemPath[j] = '-';
            }
        }

        FILE* itemFile = openFile(itemPath, "wb");

        if (!itemFile) {
            removePackItemFiles(i, items);
            destroyPackReader(packReader);
            return FAILED_TO_OPEN_FILE_PACK_RESULT;
        }

        size_t result = fwrite(dataBuffer, sizeof(uint8_t), dataSize, itemFile);

        closeFile(itemFile);

        if (result != dataSize) {
            removePackItemFiles(i, items);
            destroyPackReader(packReader);
            return FAILED_TO_OPEN_FILE_PACK_RESULT;
        }

        if (printProgress) {
            uint32_t rawFileSize = item->info.dataSize;
            uint32_t zipFileSize = item->info.zipSize > 0 ? item->info.zipSize : item->info.dataSize;

            totalRawSize += rawFileSize;
            totalZipSize += zipFileSize;

            int progress = (int)(((float)(i + 1) / (float)itemCount) * 100.0f);

            printf("(%u/%u bytes) [%d%%]\n", rawFileSize, zipFileSize, progress);
            fflush(stdout);
        }
    }

    destroyPackReader(packReader);

    if (printProgress) {
        printf("Unpacked %llu files. (%llu/%llu bytes)\n", (long long unsigned int)itemCount, (long long unsigned int)totalRawSize, (long long unsigned int)totalZipSize);
    }

    return SUCCESS_PACK_RESULT;
}

inline static pack_result writePackItems(FILE* packFile, uint64_t itemCount, char** itemPaths, bool printProgress) {
    assert(packFile);
    assert(itemCount > 0);
    assert(itemPaths);

    uint32_t bufferSize = 1;

    uint8_t* itemData = malloc(sizeof(uint8_t));

    if (!itemData) return FAILED_TO_ALLOCATE_PACK_RESULT;

    uint8_t* zipData = malloc(sizeof(uint8_t));

    if (!zipData) {
        free(itemData);
        return FAILED_TO_ALLOCATE_PACK_RESULT;
    }

    uint64_t totalZipSize = 0, totalRawSize = 0;

    for (uint64_t i = 0; i < itemCount; i++) {
        char* itemPath = itemPaths[i];

        if (printProgress) {
            printf("Packing \"%s\" file. ", itemPath);
            fflush(stdout);
        }

        size_t pathSize = strlen(itemPath);

        if (pathSize > UINT8_MAX) {
            free(zipData);
            free(itemData);
            return BAD_DATA_SIZE_PACK_RESULT;
        }

        FILE* itemFile = openFile(itemPath, "rb");

        if (!itemFile) {
            free(zipData);
            free(itemData);
            return FAILED_TO_OPEN_FILE_PACK_RESULT;
        }

        int seekResult = seekFile(itemFile, 0, SEEK_END);

        if (seekResult != 0) {
            closeFile(itemFile);
            free(zipData);
            free(itemData);
            return FAILED_TO_SEEK_FILE_PACK_RESULT;
        }

        uint64_t itemSize = (uint64_t)tellFile(itemFile);

        if (itemSize == 0 || itemSize > UINT32_MAX) {
            closeFile(itemFile);
            free(zipData);
            free(itemData);
            return BAD_DATA_SIZE_PACK_RESULT;
        }

        seekResult = seekFile(itemFile, 0, SEEK_SET);

        if (seekResult != 0) {
            closeFile(itemFile);
            free(zipData);
            free(itemData);
            return FAILED_TO_SEEK_FILE_PACK_RESULT;
        }

        if (itemSize > bufferSize) {
            uint8_t* newBuffer = realloc(itemData, itemSize * sizeof(uint8_t));

            if (!newBuffer) {
                closeFile(itemFile);
                free(zipData);
                free(itemData);
                return FAILED_TO_ALLOCATE_PACK_RESULT;
            }

            itemData = newBuffer;

            newBuffer = realloc(zipData, itemSize * sizeof(uint8_t));

            if (!newBuffer) {
                closeFile(itemFile);
                free(zipData);
                free(itemData);
                return FAILED_TO_ALLOCATE_PACK_RESULT;
            }

            zipData = newBuffer;
        }

        size_t result = fread(itemData, sizeof(uint8_t), itemSize, itemFile);

        closeFile(itemFile);

        if (result != itemSize) {
            free(zipData);
            free(itemData);
            return FAILED_TO_READ_FILE_PACK_RESULT;
        }

        size_t zipSize;

        if (itemSize > 1) {

            const int max_dst_size = LZ4_compressBound(itemSize);

            zipSize = LZ4_compress_fast(itemData, zipData, itemSize, max_dst_size, 10);

            if (zipSize <= 0 || zipSize >= itemSize) {
                zipSize = 0;
            }
        } else {
            zipSize = 0;
        }

        int64_t fileOffset = tellFile(packFile);

        pack_iteminfo info = {
                (uint32_t)zipSize,
                (uint32_t)itemSize,
                (uint64_t)fileOffset,
                (uint8_t)pathSize,
        };

        result = fwrite(&info, sizeof(pack_iteminfo), 1, packFile);

        if (result != 1) {
            free(zipData);
            free(itemData);
            return FAILED_TO_WRITE_FILE_PACK_RESULT;
        }

        result = fwrite(itemPath, sizeof(char), info.pathSize, packFile);

        if (result != info.pathSize) {
            free(zipData);
            free(itemData);
            return FAILED_TO_WRITE_FILE_PACK_RESULT;
        }

        if (zipSize > 0) {
            result = fwrite(zipData, sizeof(uint8_t), zipSize, packFile);

            if (result != zipSize) {
                free(zipData);
                free(itemData);
                return FAILED_TO_WRITE_FILE_PACK_RESULT;
            }
        } else {
            result = fwrite(itemData, sizeof(uint8_t), itemSize, packFile);

            if (result != itemSize) {
                free(zipData);
                free(itemData);
                return FAILED_TO_WRITE_FILE_PACK_RESULT;
            }
        }

        if (printProgress) {
            uint32_t zipFileSize = zipSize > 0 ? (uint32_t)zipSize : (uint32_t)itemSize;
            uint32_t rawFileSize = (uint32_t)itemSize;

            totalZipSize += zipFileSize;
            totalRawSize += rawFileSize;

            int progress = (int)(((float)(i + 1) / (float)itemCount) * 100.0f);

            printf("(%u/%u bytes) [%d%%]\n", zipFileSize, rawFileSize, progress);
            fflush(stdout);
        }
    }

    free(zipData);
    free(itemData);

    if (printProgress) {
        int compression = (int)((1.0 - (double)(totalZipSize) / (double)totalRawSize) * 100.0);
        printf("Packed %llu files. (%llu/%llu bytes, %d%% saved)\n", (long long unsigned int)itemCount, (long long unsigned int)totalZipSize, (long long unsigned int)totalRawSize, compression);
    }

    return SUCCESS_PACK_RESULT;
}
static int comparePackItemPaths(const void* _a, const void* _b) {
    // NOTE: a and b should not be NULL!
    // Skipping here assertions for debug build speed.

    char* a = *(char**)_a;
    char* b = *(char**)_b;
    uint8_t al = (uint8_t)strlen(a);
    uint8_t bl = (uint8_t)strlen(b);

    int difference = al - bl;

    if (difference != 0) return difference;

    return memcmp(a, b, al * sizeof(uint8_t));
}
pack_result packFiles(const char* filePath, uint64_t fileCount, const char** filePaths, bool printProgress) {
    assert(filePath);
    assert(fileCount > 0);
    assert(filePaths);

    char** itemPaths = malloc(fileCount * sizeof(char*));

    if (!itemPaths) return FAILED_TO_ALLOCATE_PACK_RESULT;

    uint64_t itemCount = 0;

    for (uint64_t i = 0; i < fileCount; i++) {
        bool alreadyAdded = false;

        for (uint64_t j = 0; j < itemCount; j++) {
            if (i != j && strcmp(filePaths[i], itemPaths[j]) == 0) alreadyAdded = true;
        }

        if (!alreadyAdded) itemPaths[itemCount++] = (char*)filePaths[i];
    }

    qsort(itemPaths, itemCount, sizeof(char*), comparePackItemPaths);

    FILE* packFile = openFile(filePath, "wb");

    if (!packFile) {
        free(itemPaths);
        return FAILED_TO_CREATE_FILE_PACK_RESULT;
    }

    char header[PACK_HEADER_SIZE] = {
            'P', 'A', 'C', 'K', PACK_VERSION_MAJOR, PACK_VERSION_MINOR, PACK_VERSION_PATCH, !PACK_LITTLE_ENDIAN,
    };

    size_t writeResult = fwrite(header, sizeof(char), PACK_HEADER_SIZE, packFile);

    if (writeResult != PACK_HEADER_SIZE) {
        free(itemPaths);
        closeFile(packFile);
        remove(filePath);
        return FAILED_TO_WRITE_FILE_PACK_RESULT;
    }

    writeResult = fwrite(&itemCount, sizeof(uint64_t), 1, packFile);

    if (writeResult != 1) {
        free(itemPaths);
        closeFile(packFile);
        remove(filePath);
        return FAILED_TO_WRITE_FILE_PACK_RESULT;
    }

    pack_result packResult = writePackItems(packFile, itemCount, itemPaths, printProgress);

    free(itemPaths);
    closeFile(packFile);

    if (packResult != SUCCESS_PACK_RESULT) {
        remove(filePath);
        return packResult;
    }

    return SUCCESS_PACK_RESULT;
}

void getPackLibraryVersion(uint8_t* majorVersion, uint8_t* minorVersion, uint8_t* patchVersion) {
    assert(majorVersion);
    assert(minorVersion);
    assert(patchVersion);

    *majorVersion = PACK_VERSION_MAJOR;
    *minorVersion = PACK_VERSION_MINOR;
    *patchVersion = PACK_VERSION_PATCH;
}

pack_result getPackInfo(const char* filePath, uint8_t* majorVersion, uint8_t* minorVersion, uint8_t* patchVersion, bool* isLittleEndian, uint64_t* _itemCount) {
    assert(filePath);
    assert(majorVersion);
    assert(minorVersion);
    assert(patchVersion);
    assert(isLittleEndian);
    assert(_itemCount);

    FILE* file = openFile(filePath, "rb");

    if (!file) return FAILED_TO_OPEN_FILE_PACK_RESULT;

    char header[PACK_HEADER_SIZE];

    size_t result = fread(header, sizeof(char), PACK_HEADER_SIZE, file);

    if (result != PACK_HEADER_SIZE) {
        closeFile(file);
        return FAILED_TO_READ_FILE_PACK_RESULT;
    }

    if (header[0] != 'P' || header[1] != 'A' || header[2] != 'C' || header[3] != 'K') {
        closeFile(file);
        return BAD_FILE_TYPE_PACK_RESULT;
    }

    uint64_t itemCount;

    result = fread(&itemCount, sizeof(uint64_t), 1, file);

    closeFile(file);

    if (result != 1) return FAILED_TO_READ_FILE_PACK_RESULT;

    *majorVersion = header[4];
    *minorVersion = header[5];
    *patchVersion = header[6];
    *isLittleEndian = !header[7];
    *_itemCount = itemCount;
    return SUCCESS_PACK_RESULT;
}
