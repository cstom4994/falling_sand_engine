// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_PACK_EDITOR_H
#define ME_PACK_EDITOR_H

#include "engine/core/basic_types.h"
#include "engine/core/io/packer.hpp"
#include "engine/ui/imgui_helper.hpp"

class PackEditor {
private:
    // pack editor
    ME_pack_reader pack_reader;
    bool pack_reader_is_loaded = false;
    u8 majorVersion;
    u8 minorVersion;
    u8 patchVersion;
    bool isLittleEndian;
    u64 itemCount;
    ME_pack_result result;

    // file
    bool filebrowser = false;

public:
    void Init();
    void End();
    void Draw();
};

#endif