
#ifndef ME_PACK_EDITOR_H
#define ME_PACK_EDITOR_H

#include "engine/core/basic_types.h"
#include "engine/core/io/packer.hpp"
#include "engine/ui/file_browser.h"
#include "engine/ui/imgui_helper.hpp"

class pack_editor {
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
    ImGui::FileBrowser file_dialog;

public:
    void init();
    void end();
    void draw();
};

#endif