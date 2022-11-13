// Copyright(c) 2022, KaoruXun All rights reserved.

#pragma once

#include "Libs/raylib/raylib.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void Mtd_ImGuiSetup(bool dark);
    void Mtd_ImGuiBegin();
    void Mtd_ImGuiEnd();
    void Mtd_ImGuiShutdown();

    void Mtd_ImGuiBeginInitImGui();
    void Mtd_ImGuiEndInitImGui();
    void Mtd_ImGuiReloadFonts();

    void Mtd_ImGuiImage(const Texture *image);
    bool Mtd_ImGuiImageButton(const Texture *image);
    void Mtd_ImGuiImageSize(const Texture *image, int width, int height);
    void Mtd_ImGuiImageRect(const Texture *image, int destWidth, int destHeight, Rectangle sourceRect);

#ifdef __cplusplus
}
#endif
