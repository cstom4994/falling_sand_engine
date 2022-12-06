// Copyright(c) 2022, KaoruXun All rights reserved.

// This script is responsible for managing biomes generation
// currently includes only basic information groups.

var biome_modifiers = {
    "DEFAULT": { "id": 0 },
    "TEST_1": { "id": 1 },
    "TEST_1_2": { "id": 2 },
    "TEST_2": { "id": 3 },
    "TEST_2_2": { "id": 4 },
    "TEST_3": { "id": 5 },
    "TEST_3_2": { "id": 6 },
    "TEST_4": { "id": 7 },
    "TEST_4_2": { "id": 8 },

    "PLAINS": { "id": 9 },
    "MOUNTAINS": { "id": 10 },
    "FOREST": { "id": 11 }
}

for (var b in biome_modifiers) {
    let biome_data = biome_modifiers[b];
    let id = biome_data["id"];
    test.create_biome(b, id);
}
