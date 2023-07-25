// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_GAMEUI_HPP
#define ME_GAMEUI_HPP

#include <vector>

#include "engine/core/global.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/scripting/lua_wrapper.hpp"
#include "engine/ui/imgui_impl.hpp"
#include "game_datastruct.hpp"
#include "world.hpp"

namespace ME {

class game;

namespace GameUI {

class GameUI {
public:
    // GameUI visiable list
    bool visible_mainmenu = true;
    bool visible_debugdraw = true;

    // MainMenu
    bool MainMenuUI__setup = false;
    bool MainMenuUI__connectButtonEnabled = false;
    ImVec2 MainMenuUI__pos = ImVec2(0, 0);
    std::vector<std::tuple<std::string, WorldMeta>> MainMenuUI__worlds = {};
    long long MainMenuUI__lastRefresh = 0;
    char MainMenuUI__worldNameBuf[32] = "";
    bool MainMenuUI__createWorldButtonEnabled = false;
    std::string MainMenuUI__worldFolderLabel = "";
    int MainMenuUI__selIndex = 0;

    // ME_debugdraw
    int DebugDrawUI__selIndex = -1;
    std::vector<R_Image *> DebugDrawUI__images = {};
    std::vector<R_Image *> DebugDrawUI__tools_images = {};
    Material *DebugDrawUI__selectedMaterial = &GAME()->materials_list.GENERIC_AIR;

    // OptionsUI
    int OptionsUI__item_current_idx = 0;
    bool OptionsUI__vsync = false;
    bool OptionsUI__minimizeOnFocus = false;

    std::map<std::string, FMOD::Studio::Bus *> OptionsUI__busMap = {};
};

void DebugDrawUI__Setup();
void DebugDrawUI__Draw(game *game);
void DebugDrawUI__End();

void MainMenuUI__RefreshWorlds(game *game);
void MainMenuUI__Setup();
void MainMenuUI__Draw(game *game);
void MainMenuUI__DrawMainMenu(game *game);
void MainMenuUI__DrawInGame(game *game);
void MainMenuUI__DrawWorldLists(game *game);
void MainMenuUI__reset(game *game);
void MainMenuUI__DrawCreateWorldUI(game *game);
void MainMenuUI__inputChanged(std::string text, game *game);

void OptionsUI__Draw(game *game);
void OptionsUI__DrawGeneral(game *game);
void OptionsUI__DrawVideo(game *game);
void OptionsUI__DrawAudio(game *game);
void OptionsUI__DrawInput(game *game);

}  // namespace GameUI

extern GameUI::GameUI gameUI;

}  // namespace ME

#endif