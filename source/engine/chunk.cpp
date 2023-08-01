// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "chunk.hpp"

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "engine/core/core.hpp"
#include "engine/core/global.hpp"
#include "engine/core/platform.h"
#include "libs/lz4/lz4.h"

namespace ME {

void Chunk::ChunkInit(int x, int y, const std::string &worldName) {
    this->x = x;
    this->y = y;
    pack_filename = std::string(worldName + "/chunks/c_" + std::to_string(x) + "_" + std::to_string(y) + ".pack");
}

void Chunk::ChunkDelete() {
    if (this->tiles) delete[] this->tiles;
    if (this->layer2) delete[] this->layer2;
    if (this->background) delete[] this->background;

    // 清空群系索引
    this->biomes_id.clear();
}

void Chunk::ChunkLoadMeta() {
    // datapack_node *dp;
    // char *s;
    // char *s1;

    // datapack_bin leveldata;
    // datapack_bin leveldata2;

    // int x, y, phase;

    // int src_size;
    // int compressed_size;
    // int src_size2;
    // int compressed_size2;

    // dp = datapack_map(CHUNK_DATABASE_FORMAT, &s, &s1, &phase, &x, &y, &src_size, &compressed_size, &src_size2, &compressed_size2, &leveldata, &leveldata2);
    // datapack_load(dp, DATAPACK_FILE, this->pack_filename.c_str());
    // datapack_unpack(dp, 0);

    // if (dp) {
    //     this->generationPhase = phase;
    //     this->hasMeta = true;
    //     datapack_free(dp);
    // } else {
    // }

    // if (s) free(s);
    // if (s1) free(s1);
    // if (leveldata.addr) free(leveldata.addr);
    // if (leveldata2.addr) free(leveldata2.addr);

    std::string line;
    std::ifstream myfile(this->pack_filename);
    if (myfile.is_open()) {
        getline(myfile, line, '\n');

        int phase = stoi(line);
        this->generationPhase = phase;
        this->hasMeta = true;
    }
}

void Chunk::ChunkRead() {
    MaterialInstance *tiles = new MaterialInstance[CHUNK_W * CHUNK_H];
    if (tiles == NULL) throw std::runtime_error("Failed to allocate memory for Chunk tiles array.");
    MaterialInstance *layer2 = new MaterialInstance[CHUNK_W * CHUNK_H];
    if (layer2 == NULL) throw std::runtime_error("Failed to allocate memory for Chunk layer2 array.");
    u32 *background = new u32[CHUNK_W * CHUNK_H];

    std::string line;
    std::ifstream myfile(this->pack_filename, std::ios::binary);

    if (myfile.is_open()) {
        int state = 0;

        myfile.read((char *)&this->generationPhase, sizeof(i8));

        this->hasMeta = true;
        state = 1;

        // METADOT_BUG(std::format("Datapack test result: {0} {1} {2} {3}", leveldata.sz, compressed_size, leveldata2.sz, compressed_size2).c_str());

        // 验证区块位置
        // if (x != this->x || y != this->y)
        //     throw std::runtime_error("Wrong region block read sequence (" + std::to_string(x) + "," + std::to_string(y) + " should be " + std::to_string(this->x) + "," + std::to_string(this->y) +
        //                              ")");

        // if (leveldata.sz != compressed_size || leveldata2.sz != compressed_size2) throw std::runtime_error("Unexpected block compression data");

        // this->hasMeta = true;
        // state = 1;

        // unsigned int content;
        // for (int i = 0; i < CHUNK_W * CHUNK_H; i++) {
        //  myfile.read((char*)&content, sizeof(unsigned int));
        //  int id = content;
        //  myfile.read((char*)&content, sizeof(unsigned int));
        //  u32 color = content;
        //  tiles[i] = MaterialInstance(GAME()->materials_container[id], color);
        // }
        // for (int i = 0; i < CHUNK_W * CHUNK_H; i++) {
        //  myfile.read((char*)&content, sizeof(unsigned int));
        //  int id = content;
        //  myfile.read((char*)&content, sizeof(unsigned int));
        //  u32 color = content;
        //  layer2[i] = MaterialInstance(GAME()->materials_container[id], color);
        // }
        // for (int i = 0; i < CHUNK_W * CHUNK_H; i++) {
        //  myfile.read((char*)&content, sizeof(unsigned int));
        //  background[i] = content;
        // }

        int src_size;
        myfile.read((char *)&src_size, sizeof(int));

        // 两层MaterialInstanceData包括tiles[]和layer2[]
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

        const int decompressed_size = LZ4_decompress_safe((char *)compressed_data, (char *)readBuf, compressed_size, src_size);

        free(compressed_data);

        // 基本上，如果触发这两个检查中的任何一个，块都是不可读的，要么是因为写错了，要么是因为损坏。
        // TODO：让区块在损坏时重新生成(可能还会保存损坏区块的副本？)
        if (decompressed_size < 0) {
            METADOT_ERROR(std::format("Error decompressing chunk tile data @ {0},{1} (err {2}).", this->x, this->y, decompressed_size).c_str());
        } else if (decompressed_size != src_size) {
            METADOT_ERROR(std::format("Decompressed chunk tile data is corrupt! @ {0},{1} (was {2}, expected {3}).", this->x, this->y, decompressed_size, src_size).c_str());
        }

        // copy everything but the material pointer
        // memcpy(tiles, readBuf, CHUNK_W * CHUNK_H * sizeof(MaterialInstance));
        // memcpy(layer2, &readBuf[CHUNK_W * CHUNK_H], CHUNK_W * CHUNK_H * sizeof(MaterialInstance));

        // copy the material pointer
        for (int i = 0; i < CHUNK_W * CHUNK_H; i++) {
            // twice as fast to set fields instead of making new ones
            tiles[i].color = readBuf[i].color;
            tiles[i].temperature = readBuf[i].temperature;
            tiles[i].mat = GAME()->materials_array[readBuf[i].index];
            // tiles[i].id = MaterialInstance::_curID++;
            // tiles[i].color = readBuf[i].color;
            // tiles[i].temperature = readBuf[i].temperature;
            // tiles[i] = MaterialInstance(Materials::MATERIALS_ARRAY[buf[i].index], buf[i].color, buf[i].temperature);

            layer2[i].color = readBuf[i + CHUNK_W * CHUNK_H].color;
            layer2[i].temperature = readBuf[i + CHUNK_W * CHUNK_H].temperature;
            layer2[i].mat = GAME()->materials_array[readBuf[CHUNK_W * CHUNK_H + i].index];
            // layer2[i].id = MaterialInstance::_curID++;
            // layer2[i].color = readBuf[CHUNK_W * CHUNK_H + i].color;
            // layer2[i].temperature = readBuf[CHUNK_W * CHUNK_H + i].temperature;
            // layer2[i] = MaterialInstance(Materials::MATERIALS_ARRAY[buf[CHUNK_W * CHUNK_H + i].index], buf[CHUNK_W * CHUNK_H + i].color, buf[CHUNK_W * CHUNK_H + i].temperature);
        }

        char *compressed_data2 = (char *)malloc(compressed_size2);

        myfile.read((char *)compressed_data2, compressed_size2);

        const int decompressed_size2 = LZ4_decompress_safe((char *)compressed_data2, (char *)background, compressed_size2, src_size2);

        free(compressed_data2);

        if (decompressed_size2 < 0) {
            METADOT_ERROR(std::format("Error decompressing chunk background data @ {0},{1} (err {2}).", this->x, this->y, decompressed_size2).c_str());
        } else if (decompressed_size2 != src_size2) {
            METADOT_ERROR(std::format("Decompressed chunk background data is corrupt! @ {0},{1} (was {2}, expected {3}).", this->x, this->y, decompressed_size2, src_size2).c_str());
        }

        free(readBuf);

        myfile.close();
    } else {
        METADOT_ERROR(std::format("Read chunk {0},{1} faild", this->x, this->y).c_str());
    }

    // if (s) free(s);
    // if (s1) free(s1);
    // if (leveldata.addr) free(leveldata.addr);
    // if (leveldata2.addr) free(leveldata2.addr);

    this->tiles = tiles;
    this->layer2 = layer2;
    this->background = background;
    this->hasTileCache = true;
}

void Chunk::ChunkWrite(MaterialInstance *tiles, MaterialInstance *layer2, u32 *background) {
    this->tiles = tiles;
    this->layer2 = layer2;
    this->background = background;
    if (this->tiles == NULL || this->layer2 == NULL || this->background == NULL) return;
    this->hasTileCache = true;

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
        buf[i] = {(u16)tiles[i].mat->id, tiles[i].color, tiles[i].temperature};
        buf[CHUNK_W * CHUNK_H + i] = {(u16)layer2[i].mat->id, layer2[i].color, layer2[i].temperature};
    }

    const char *const src = (char *)buf;
    const int src_size = (int)(CHUNK_W * CHUNK_H * 2 * sizeof(MaterialInstanceData));
    const int max_dst_size = LZ4_compressBound(src_size);

    char *compressed_data = (char *)malloc((size_t)max_dst_size);

    const int compressed_data_size = LZ4_compress_fast(src, compressed_data, src_size, max_dst_size, 10);

    if (compressed_data_size <= 0) {
        METADOT_ERROR(std::format("Failed to compress chunk tile data @ {0},{1} (err {2})", this->x, this->y, compressed_data_size).c_str());
    }

    // if(compressed_data_size > 0){
    //     METADOT_BUG("Compression data1 ratio: %f", (f32)compressed_data_size / src_size * 100);
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
        METADOT_ERROR(std::format("Failed to compress chunk tile data @ {0},{1} (err {2})", this->x, this->y, compressed_data_size2).c_str());
    }

    // if(compressed_data_size2 > 0){
    //     METADOT_BUG("Compression data2 ratio: %f", (f32)compressed_data_size2 / src_size2 * 100);
    // }

    char *n_compressed_data2 = (char *)realloc(compressed_data2, (size_t)compressed_data_size2);
    if (n_compressed_data2 == NULL) throw std::runtime_error("Failed to realloc memory for Chunk::write compressed_data2.");
    compressed_data2 = n_compressed_data2;

    // datapack_node *dp;

    // datapack_bin leveldata;
    // datapack_bin leveldata2;

    // const char *s = "Is man one of God's blunders? Or is God one of man's blunders?";

    // const char *filename = this->pack_filename.c_str();
    // dp = datapack_map(CHUNK_DATABASE_FORMAT, &s, &filename, &this->generationPhase, &this->x, &this->y, &src_size, &compressed_data_size, &src_size2, &compressed_data_size2, &leveldata,
    // &leveldata2);

    // leveldata.sz = compressed_data_size;
    // leveldata.addr = compressed_data;
    // leveldata2.sz = compressed_data_size2;
    // leveldata2.addr = compressed_data2;

    // datapack_pack(dp, 0);
    // datapack_dump(dp, DATAPACK_FILE, this->pack_filename.c_str());
    // datapack_free(dp);

    // free(compressed_data);
    // free(compressed_data2);

    // delete[] buf;

    std::ofstream myfile;
    myfile.open(this->pack_filename, std::ios::binary);
    myfile.write((char *)&this->generationPhase, sizeof(int8_t));

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

bool Chunk::ChunkHasFile() {
    struct stat buffer;
    return (stat(this->pack_filename.c_str(), &buffer) == 0);
}

u64 Chunk::get_chunk_size() {

    // size_t biomes_vectorSize = sizeof(biomes_id);
    size_t biomes_elementSize = sizeof(int);
    size_t biomes_elementCount = biomes_id.size();
    size_t biomes_totalSize = /*biomes_vectorSize +*/ biomes_elementSize * biomes_elementCount;

    // size_t polys_vectorSize = sizeof(polys);
    size_t polys_elementSize = sizeof(b2PolygonShape);
    size_t polys_elementCount = polys.size();
    size_t polys_totalSize = /*polys_vectorSize +*/ polys_elementSize * polys_elementCount;

    size_t total = sizeof(MaterialInstance) * CHUNK_W * CHUNK_H * 2;

    total += sizeof(u32) * CHUNK_W * CHUNK_H;
    total += biomes_totalSize + polys_totalSize;

    total += sizeof(Chunk);

    return total;
}

}  // namespace ME