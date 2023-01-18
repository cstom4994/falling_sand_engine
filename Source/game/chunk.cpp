// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "chunk.hpp"

#include <sstream>
#include <string>
#include <vector>

#include "core/core.h"
#include "core/core.hpp"
#include "core/cpp/utils.hpp"
#include "core/global.hpp"
#include "engine/engine_platform.h"
#include "lz4/lz4.h"

// Chunk::~Chunk() {
//     if (tiles) delete[] tiles;
//     if (layer2) delete[] layer2;
//     if (background) delete[] background;
//     if (!biomes.empty()) biomes.resize(0);
// }

void Chunk_Init(Chunk *_struct, int x, int y, char *worldName) {
    METADOT_ASSERT_E(_struct);
    _struct->x = x;
    _struct->y = y;
    _struct->fname = std::string(std::string(worldName) + "/chunks/c_" + std::to_string(x) + "_" + std::to_string(y) + ".region");
}

void Chunk_Delete(Chunk *_struct) {
    METADOT_ASSERT_E(_struct);
    if (_struct->tiles) delete[] _struct->tiles;
    if (_struct->layer2) delete[] _struct->layer2;
    if (_struct->background) delete[] _struct->background;
    if (!_struct->biomes.empty()) _struct->biomes.resize(0);
}

void Chunk_loadMeta(Chunk *_struct) {
    std::string line;
    std::ifstream myfile(_struct->fname);
    if (myfile.is_open()) {
        getline(myfile, line, '\n');

        int phase = stoi(line);
        _struct->generationPhase = phase;
        _struct->hasMeta = true;
    }
}

// MaterialInstanceData* Chunk::readBuf = (MaterialInstanceData*)malloc(CHUNK_W * CHUNK_H * 2 * sizeof(MaterialInstanceData));

void Chunk_read(Chunk *_struct) {
    // use malloc here instead of new so it doesn't call the constructor
    MaterialInstance *tiles = (MaterialInstance *)malloc(CHUNK_W * CHUNK_H * sizeof(MaterialInstance));
    if (tiles == NULL) throw std::runtime_error("Failed to allocate memory for Chunk tiles array.");
    MaterialInstance *layer2 = (MaterialInstance *)malloc(CHUNK_W * CHUNK_H * sizeof(MaterialInstance));
    if (layer2 == NULL) throw std::runtime_error("Failed to allocate memory for Chunk layer2 array.");
    // MaterialInstance* tiles = new MaterialInstance[CHUNK_W * CHUNK_H];
    // MaterialInstance* layer2 = new MaterialInstance[CHUNK_W * CHUNK_H];
    U32 *background = new U32[CHUNK_W * CHUNK_H];

    std::string line;
    std::ifstream myfile(_struct->fname, std::ios::binary);

    if (myfile.is_open()) {
        int state = 0;

        myfile.read((char *)&_struct->generationPhase, sizeof(int8_t));

        _struct->hasMeta = true;
        state = 1;

        // unsigned int content;
        // for (int i = 0; i < CHUNK_W * CHUNK_H; i++) {
        // 	myfile.read((char*)&content, sizeof(unsigned int));
        // 	int id = content;
        // 	myfile.read((char*)&content, sizeof(unsigned int));
        // 	U32 color = content;
        // 	tiles[i] = MaterialInstance(global.GameData_.materials_container[id], color);
        // }
        // for (int i = 0; i < CHUNK_W * CHUNK_H; i++) {
        // 	myfile.read((char*)&content, sizeof(unsigned int));
        // 	int id = content;
        // 	myfile.read((char*)&content, sizeof(unsigned int));
        // 	U32 color = content;
        // 	layer2[i] = MaterialInstance(global.GameData_.materials_container[id], color);
        // }
        // for (int i = 0; i < CHUNK_W * CHUNK_H; i++) {
        // 	myfile.read((char*)&content, sizeof(unsigned int));
        // 	background[i] = content;
        // }

        int src_size;
        myfile.read((char *)&src_size, sizeof(int));

        if (src_size != CHUNK_W * CHUNK_H * 2 * sizeof(MaterialInstanceData))
            throw std::runtime_error("Chunk src_size was different from expected: " + std::to_string(src_size) + " vs " + std::to_string(CHUNK_W * CHUNK_H * 2 * sizeof(MaterialInstanceData)));

        int compressed_size;
        myfile.read((char *)&compressed_size, sizeof(int));

        int src_size2;
        myfile.read((char *)&src_size2, sizeof(int));
        int desSize = CHUNK_W * CHUNK_H * sizeof(unsigned int);

        if (src_size2 != desSize) throw std::runtime_error("Chunk src_size2 was different from expected: " + std::to_string(src_size2) + " vs " + std::to_string(desSize));

        int compressed_size2;
        myfile.read((char *)&compressed_size2, sizeof(int));

        MaterialInstanceData *readBuf = (MaterialInstanceData *)malloc(src_size);

        if (readBuf == NULL) throw std::runtime_error("Failed to allocate memory for Chunk readBuf.");

        char *compressed_data = (char *)malloc(compressed_size);

        myfile.read((char *)compressed_data, compressed_size);

        const int decompressed_size = LZ4_decompress_safe(compressed_data, (char *)readBuf, compressed_size, src_size);

        free(compressed_data);

        // basically, if either of these checks trigger, the chunk is unreadable, either due to miswriting it or corruption
        // TODO: have the chunk regenerate on corruption (maybe save copies of corrupt chunks as well?)
        if (decompressed_size < 0) {
            METADOT_ERROR("Error decompressing chunk tile data @ %d,%d (err %d).", _struct->x, _struct->y, decompressed_size);
        } else if (decompressed_size != src_size) {
            METADOT_ERROR("Decompressed chunk tile data is corrupt! @ %d,%d (was %d, expected %d).", _struct->x, _struct->y, decompressed_size, src_size);
        }

        // copy everything but the material pointer
        // memcpy(tiles, readBuf, CHUNK_W * CHUNK_H * sizeof(MaterialInstance));
        // memcpy(layer2, &readBuf[CHUNK_W * CHUNK_H], CHUNK_W * CHUNK_H * sizeof(MaterialInstance));

        // copy the material pointer
        for (int i = 0; i < CHUNK_W * CHUNK_H; i++) {
            // twice as fast to set fields instead of making new ones
            tiles[i].color = readBuf[i].color;
            tiles[i].temperature = readBuf[i].temperature;
            tiles[i].mat = global.GameData_.materials_array[readBuf[i].index];
            tiles[i].id = MaterialInstance::_curID++;
            // tiles[i].color = readBuf[i].color;
            // tiles[i].temperature = readBuf[i].temperature;
            // tiles[i] = MaterialInstance(Materials::MATERIALS_ARRAY[buf[i].index], buf[i].color, buf[i].temperature);

            layer2[i].color = readBuf[i + CHUNK_W * CHUNK_H].color;
            layer2[i].temperature = readBuf[i + CHUNK_W * CHUNK_H].temperature;
            layer2[i].mat = global.GameData_.materials_array[readBuf[CHUNK_W * CHUNK_H + i].index];
            layer2[i].id = MaterialInstance::_curID++;
            // layer2[i].color = readBuf[CHUNK_W * CHUNK_H + i].color;
            // layer2[i].temperature = readBuf[CHUNK_W * CHUNK_H + i].temperature;
            // layer2[i] = MaterialInstance(Materials::MATERIALS_ARRAY[buf[CHUNK_W * CHUNK_H + i].index], buf[CHUNK_W * CHUNK_H + i].color, buf[CHUNK_W * CHUNK_H + i].temperature);
        }

        // delete readBuf;

        char *compressed_data2 = (char *)malloc(compressed_size2);

        myfile.read((char *)compressed_data2, compressed_size2);

        const int decompressed_size2 = LZ4_decompress_safe(compressed_data2, (char *)background, compressed_size2, src_size2);

        free(compressed_data2);

        if (decompressed_size2 < 0) {
            METADOT_ERROR("Error decompressing chunk background data @ %d,%d (err %d).", _struct->x, _struct->y, decompressed_size2);
        } else if (decompressed_size2 != src_size2) {
            METADOT_ERROR("Decompressed chunk background data is corrupt! @ %d,%d (was %d, expected %d).", _struct->x, _struct->y, decompressed_size2, src_size2);
        }

        free(readBuf);

        myfile.close();
    }

    _struct->tiles = tiles;
    _struct->layer2 = layer2;
    _struct->background = background;
    _struct->hasTileCache = true;
}

void Chunk_write(Chunk *_struct, MaterialInstance *tiles, MaterialInstance *layer2, U32 *background) {
    _struct->tiles = tiles;
    _struct->layer2 = layer2;
    _struct->background = background;
    if (_struct->tiles == NULL || _struct->layer2 == NULL || _struct->background == NULL) return;
    _struct->hasTileCache = true;

    // TODO: make these loops faster
    /*for (int i = 0; i < CHUNK_W * CHUNK_H; i++) {
        myfile.write((char*)&tiles[i].mat.id, sizeof(unsigned int));
        myfile.write((char*)&tiles[i].color, sizeof(unsigned int));
    }
    for (int i = 0; i < CHUNK_W * CHUNK_H; i++) {
        myfile.write((char*)&layer2[i].mat.id, sizeof(unsigned int));
        myfile.write((char*)&layer2[i].color, sizeof(unsigned int));
    }*/
    /*for (int i = 0; i < CHUNK_W * CHUNK_H; i++) {
        myfile.write((char*)&background[i], sizeof(unsigned int));
    }*/

    MaterialInstanceData *buf = new MaterialInstanceData[CHUNK_W * CHUNK_H * 2];
    for (int i = 0; i < CHUNK_W * CHUNK_H; i++) {
        buf[i] = {(U16)tiles[i].mat->id, tiles[i].color, tiles[i].temperature};
        buf[CHUNK_W * CHUNK_H + i] = {(U16)layer2[i].mat->id, layer2[i].color, layer2[i].temperature};
    }

    const char *const src = (char *)buf;
    const int src_size = (int)(CHUNK_W * CHUNK_H * 2 * sizeof(MaterialInstanceData));
    const int max_dst_size = LZ4_compressBound(src_size);

    char *compressed_data = (char *)malloc((size_t)max_dst_size);

    const int compressed_data_size = LZ4_compress_fast(src, compressed_data, src_size, max_dst_size, 10);

    if (compressed_data_size <= 0) {
        METADOT_ERROR("Failed to compress chunk tile data @ %d,%d (err %d)", _struct->x, _struct->y, compressed_data_size);
    }

    /*if(compressed_data_size > 0){
        METADOT_BUG("Compression ratio: {}", (F32)compressed_data_size / src_size * 100);
    }*/

    char *n_compressed_data = (char *)realloc(compressed_data, compressed_data_size);
    if (n_compressed_data == NULL) throw std::runtime_error("Failed to realloc memory for Chunk::write compressed_data.");
    compressed_data = n_compressed_data;

    // bg compress

    const char *const src2 = (char *)background;
    const int src_size2 = (int)(CHUNK_W * CHUNK_H * sizeof(unsigned int));
    const int max_dst_size2 = LZ4_compressBound(src_size2);

    char *compressed_data2 = (char *)malloc((size_t)max_dst_size2);

    const int compressed_data_size2 = LZ4_compress_fast(src2, compressed_data2, src_size2, max_dst_size2, 10);

    if (compressed_data_size2 <= 0) {
        METADOT_ERROR("Failed to compress chunk tile data @ %d,%d (err %d)", _struct->x, _struct->y, compressed_data_size2);
    }

    /*if(compressed_data_size2 > 0){
        METADOT_BUG("Compression ratio: {}", (F32)compressed_data_size2 / src_size2 * 100);
    }*/

    char *n_compressed_data2 = (char *)realloc(compressed_data2, (size_t)compressed_data_size2);
    if (n_compressed_data2 == NULL) throw std::runtime_error("Failed to realloc memory for Chunk::write compressed_data2.");
    compressed_data2 = n_compressed_data2;

    std::ofstream myfile;
    myfile.open(_struct->fname, std::ios::binary);
    myfile.write((char *)&_struct->generationPhase, sizeof(int8_t));

    myfile.write((char *)&src_size, sizeof(int));
    myfile.write((char *)&compressed_data_size, sizeof(int));
    myfile.write((char *)&src_size2, sizeof(int));
    myfile.write((char *)&compressed_data_size2, sizeof(int));

    myfile.write((char *)compressed_data, compressed_data_size);
    myfile.write((char *)compressed_data2, compressed_data_size2);

    free(compressed_data);
    free(compressed_data2);

    delete[] buf;

    myfile.close();
}

bool Chunk_hasFile(Chunk *_struct) {
    struct stat buffer;
    return (stat(_struct->fname.c_str(), &buffer) == 0);
}
