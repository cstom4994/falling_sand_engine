// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_BIOME_HPP_
#define _METADOT_BIOME_HPP_

#include <utility>
#define INC_Biome

class Biome {
public:
    int id;
    explicit Biome(int id) : id(std::move(id)){};
};

class Biomes {
public:
    static Biome DEFAULT;
    static Biome TEST_1;
    static Biome TEST_1_2;
    static Biome TEST_2;
    static Biome TEST_2_2;
    static Biome TEST_3;
    static Biome TEST_3_2;
    static Biome TEST_4;
    static Biome TEST_4_2;

    static Biome PLAINS;
    static Biome MOUNTAINS;
    static Biome FOREST;
};

#endif