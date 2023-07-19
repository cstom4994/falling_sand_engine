// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "pack_editor.h"

#include "engine/core/io/filesystem.h"
#include "engine/ui/imgui_helper.hpp"

namespace ME {

void PackEditor::Init() {
    // file_dialog.SetTitle("选择包");
    // file_dialog.SetTypeFilters({".pack"});
}

void PackEditor::End() {
    if (pack_reader_is_loaded) {
        ME_destroy_pack_reader(pack_reader);
    }
}

void PackEditor::Draw() {
    if (ImGui::Begin("包编辑器")) {

        if (ImGui::Button("打开包") && !pack_reader_is_loaded) filebrowser = true;
        ImGui::SameLine();
        if (ImGui::Button("关闭包")) {
            ME_destroy_pack_reader(pack_reader);
            pack_reader_is_loaded = false;
        }
        ImGui::SameLine();
        ImGui::Text("MetaDot Pack [%d.%d.%d]\n", PACK_VERSION_MAJOR, PACK_VERSION_MINOR, PACK_VERSION_PATCH);

        // file_dialog.Display();

        if (filebrowser) {
            auto file = ImGuiHelper::file_browser(ME_fs_get_path("data/scripts"));

            if (!file.empty()) {

                if (pack_reader_is_loaded) {

                } else {
                    result = ME_get_pack_info(file.c_str(), &majorVersion, &minorVersion, &patchVersion, &isLittleEndian, &itemCount);
                    if (result != SUCCESS_PACK_RESULT) return;

                    result = ME_create_file_pack_reader(file.c_str(), 0, false, &pack_reader);
                    if (result != SUCCESS_PACK_RESULT) return;

                    if (result == SUCCESS_PACK_RESULT) itemCount = ME_get_pack_item_count(pack_reader);

                    pack_reader_is_loaded = true;
                }

                filebrowser = false;
            }
        }

        if (pack_reader_is_loaded && result == SUCCESS_PACK_RESULT) {

            static int item_current_idx = -1;

            ImGui::Text(
                    "包信息:\n"
                    " 打包版本: %d.%d.%d\n"
                    " 小端模式: %s\n"
                    " 文件数量: %llu\n\n",
                    majorVersion, minorVersion, patchVersion, isLittleEndian ? "true" : "false", (long long unsigned int)itemCount);

            ImVec2 outer_size = ImVec2(0.0f, ImGui::GetContentRegionMax().y - 250.0f);
            if (ImGui::BeginTable("ui_pack_file_table", 4,
                                  ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg |
                                          ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti,
                                  outer_size)) {
                ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
                ImGui::TableSetupColumn("索引", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("文件名", ImGuiTableColumnFlags_WidthFixed, 550.0f);
                ImGui::TableSetupColumn("大小", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();

                for (u64 i = 0; i < itemCount; ++i) {
                    ImGui::PushID(i);
                    ImGui::TableNextRow(ImGuiTableRowFlags_None);
                    if (ImGui::TableSetColumnIndex(0)) {
                        // if (ImGui::Selectable(std::format("{0}", ).c_str(), item_current_idx, ImGuiSelectableFlags_SpanAllColumns)) {
                        //     item_current_idx = i;
                        // }
                        ImGui::Text("%llu", (long long unsigned int)i);
                    }
                    if (ImGui::TableSetColumnIndex(1)) {
                        ImGui::Text("%s", ME_get_pack_item_path(pack_reader, i));
                    }
                    if (ImGui::TableSetColumnIndex(2)) {
                        ImGui::Text("%u", ME_get_pack_item_data_size(pack_reader, i));
                    }
                    if (ImGui::TableSetColumnIndex(3)) {
                        if (ImGui::SmallButton("查看")) {
                            item_current_idx = i;
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("替换")) {
                        }
                        ImGui::SameLine();
                        if (ImGui::SmallButton("删除")) {
                        }
                    }
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }

            if (item_current_idx >= 0) {
                ImGui::Text("文件索引: %u\n包内路径: %s\n文件大小: %u\n", item_current_idx, ME_get_pack_item_path(pack_reader, item_current_idx),
                            ME_get_pack_item_data_size(pack_reader, item_current_idx));
            }

        } else if (result != SUCCESS_PACK_RESULT) {
            ImGui::Text("错误: %s.\n", pack_result_to_string(result));
        }
    }
    ImGui::End();
}

}  // namespace ME