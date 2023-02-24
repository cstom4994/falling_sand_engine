// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "chunk.hpp"

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/core.h"
#include "core/core.hpp"
#include "core/cpp/utils.hpp"
#include "core/global.hpp"
#include "core/io/datapackage.h"
#include "core/platform.h"
#include "libs/lz4/lz4.h"

void ChunkInit(Chunk *_struct, int x, int y, char *worldName) {
    METADOT_ASSERT_E(_struct);
    _struct->x = x;
    _struct->y = y;
    _struct->pack_filename = std::string(std::string(worldName) + "/chunks/c_" + std::to_string(x) + "_" + std::to_string(y) + ".pack");
}

void ChunkDelete(Chunk *_struct) {
    METADOT_ASSERT_E(_struct);
    if (_struct->tiles) delete[] _struct->tiles;
    if (_struct->layer2) delete[] _struct->layer2;
    if (_struct->background) delete[] _struct->background;
    if (!_struct->biomes.empty()) _struct->biomes.resize(0);
}

void ChunkLoadMeta(Chunk *_struct) {
    datapack_node *dp;
    char *s;
    char *s1;

    datapack_bin leveldata;
    datapack_bin leveldata2;

    int x, y, phase;

    int src_size;
    int compressed_size;
    int src_size2;
    int compressed_size2;

    dp = datapack_map(CHUNK_DATABASE_FORMAT, &s, &s1, &phase, &x, &y, &src_size, &compressed_size, &src_size2, &compressed_size2, &leveldata, &leveldata2);
    datapack_load(dp, DATAPACK_FILE, _struct->pack_filename.c_str());
    datapack_unpack(dp, 0);

    if (dp) {
        _struct->generationPhase = phase;
        _struct->hasMeta.set_value(true);
        datapack_free(dp);
    } else {
    }

    if (s) free(s);
    if (s1) free(s1);
    if (leveldata.addr) free(leveldata.addr);
    if (leveldata2.addr) free(leveldata2.addr);
}

void ChunkRead(Chunk *_struct) {
    MaterialInstance *tiles = new MaterialInstance[CHUNK_W * CHUNK_H];
    if (tiles == NULL) throw std::runtime_error("Failed to allocate memory for Chunk tiles array.");
    MaterialInstance *layer2 = new MaterialInstance[CHUNK_W * CHUNK_H];
    if (layer2 == NULL) throw std::runtime_error("Failed to allocate memory for Chunk layer2 array.");
    // MaterialInstance *tiles = new MaterialInstance[CHUNK_W * CHUNK_H];
    // MaterialInstance *layer2 = new MaterialInstance[CHUNK_W * CHUNK_H];
    U32 *background = new U32[CHUNK_W * CHUNK_H];

    datapack_node *dp;
    char *s;
    char *s1;

    // Read x y from pack
    int x, y;

    // Used to verify compressed data
    int src_size;
    int compressed_size;
    int src_size2;
    int compressed_size2;

    datapack_bin leveldata;
    datapack_bin leveldata2;

    dp = datapack_map(CHUNK_DATABASE_FORMAT, &s, &s1, &_struct->generationPhase, &x, &y, &src_size, &compressed_size, &src_size2, &compressed_size2, &leveldata, &leveldata2);
    datapack_load(dp, DATAPACK_FILE, _struct->pack_filename.c_str());
    datapack_unpack(dp, 0);

    if (dp) {
        int state = 0;

        // METADOT_BUG("Datapack test result: %d, %d, %d, %d", leveldata.sz, compressed_size, leveldata2.sz, compressed_size2);

        if (x != _struct->x || y != _struct->y)
            throw std::runtime_error("Wrong region block read sequence (" + std::to_string(x) + "," + std::to_string(y) + " should be " + std::to_string(_struct->x) + "," +
                                     std::to_string(_struct->y) + ")");

        if (leveldata.sz != compressed_size || leveldata2.sz != compressed_size2) throw std::runtime_error("Unexpected block compression data");

        _struct->hasMeta.set_value(true);
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

        if (src_size != CHUNK_W * CHUNK_H * 2 * sizeof(MaterialInstanceData))
            throw std::runtime_error("Chunk src_size was different from expected: " + std::to_string(src_size) + " vs " + std::to_string(CHUNK_W * CHUNK_H * 2 * sizeof(MaterialInstanceData)));

        int desSize = CHUNK_W * CHUNK_H * sizeof(unsigned int);

        if (src_size2 != desSize) throw std::runtime_error("Chunk src_size2 was different from expected: " + std::to_string(src_size2) + " vs " + std::to_string(desSize));

        MaterialInstanceData *readBuf = (MaterialInstanceData *)malloc(src_size);
        if (readBuf == NULL) throw std::runtime_error("Failed to allocate memory for Chunk readBuf.");

        const int decompressed_size = LZ4_decompress_safe((char *)leveldata.addr, (char *)readBuf, compressed_size, src_size);

        // 基本上，如果触发这两个检查中的任何一个，块都是不可读的，要么是因为写错了，要么是因为损坏。
        // TODO：让区块在损坏时重新生成(可能还会保存损坏区块的副本？)
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

        const int decompressed_size2 = LZ4_decompress_safe((char *)leveldata2.addr, (char *)background, compressed_size2, src_size2);

        if (decompressed_size2 < 0) {
            METADOT_ERROR("Error decompressing chunk background data @ %d,%d (err %d).", _struct->x, _struct->y, decompressed_size2);
        } else if (decompressed_size2 != src_size2) {
            METADOT_ERROR("Decompressed chunk background data is corrupt! @ %d,%d (was %d, expected %d).", _struct->x, _struct->y, decompressed_size2, src_size2);
        }

        free(readBuf);

        datapack_free(dp);
    } else {
        METADOT_ERROR("Read chunk %d %d faild", _struct->x, _struct->y);
    }

    if (s) free(s);
    if (s1) free(s1);
    if (leveldata.addr) free(leveldata.addr);
    if (leveldata2.addr) free(leveldata2.addr);

    _struct->tiles = tiles;
    _struct->layer2 = layer2;
    _struct->background = background;
    _struct->hasTileCache = true;
}

void ChunkWrite(Chunk *_struct, MaterialInstance *tiles, MaterialInstance *layer2, U32 *background) {
    _struct->tiles = tiles;
    _struct->layer2 = layer2;
    _struct->background = background;
    if (_struct->tiles == NULL || _struct->layer2 == NULL || _struct->background == NULL) return;
    _struct->hasTileCache = true;

    // TODO: make these loops faster
    // for (int i = 0; i < CHUNK_W * CHUNK_H; i++) {
    //     myfile.write((char *)&tiles[i].mat.id, sizeof(unsigned int));
    //     myfile.write((char *)&tiles[i].color, sizeof(unsigned int));
    // }
    // for (int i = 0; i < CHUNK_W * CHUNK_H; i++) {
    //     myfile.write((char *)&layer2[i].mat.id, sizeof(unsigned int));
    //     myfile.write((char *)&layer2[i].color, sizeof(unsigned int));
    // }
    // for (int i = 0; i < CHUNK_W * CHUNK_H; i++) {
    //     myfile.write((char *)&background[i], sizeof(unsigned int));
    // }

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

    // if(compressed_data_size > 0){
    //     METADOT_BUG("Compression data1 ratio: %f", (F32)compressed_data_size / src_size * 100);
    // }

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

    // if(compressed_data_size2 > 0){
    //     METADOT_BUG("Compression data2 ratio: %f", (F32)compressed_data_size2 / src_size2 * 100);
    // }

    char *n_compressed_data2 = (char *)realloc(compressed_data2, (size_t)compressed_data_size2);
    if (n_compressed_data2 == NULL) throw std::runtime_error("Failed to realloc memory for Chunk::write compressed_data2.");
    compressed_data2 = n_compressed_data2;

    datapack_node *dp;

    datapack_bin leveldata;
    datapack_bin leveldata2;

    const char *s = "Is man one of God's blunders? Or is God one of man's blunders?";

    const char *filename = _struct->pack_filename.c_str();
    dp = datapack_map(CHUNK_DATABASE_FORMAT, &s, &filename, &_struct->generationPhase, &_struct->x, &_struct->y, &src_size, &compressed_data_size, &src_size2, &compressed_data_size2, &leveldata,
                      &leveldata2);

    leveldata.sz = compressed_data_size;
    leveldata.addr = compressed_data;
    leveldata2.sz = compressed_data_size2;
    leveldata2.addr = compressed_data2;

    datapack_pack(dp, 0);
    datapack_dump(dp, DATAPACK_FILE, _struct->pack_filename.c_str());
    datapack_free(dp);

    free(compressed_data);
    free(compressed_data2);

    delete[] buf;
}

bool ChunkHasFile(Chunk *_struct) {
    struct stat buffer;
    return (stat(_struct->pack_filename.c_str(), &buffer) == 0);
}
