// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_GAMEUI_HPP
#define ME_GAMEUI_HPP

#include <vector>

#include "game_datastruct.hpp"
#include "engine/ui/imgui_impl.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/scripting/lua_wrapper.hpp"
#include "world.hpp"

class Game;

struct I18N {
    void Init();
    void Load(std::string lang);
    std::string Get(std::string text);
};

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
    Material *DebugDrawUI__selectedMaterial = &MaterialsList::GENERIC_AIR;
    u8 DebugDrawUI__brushSize = 5;

    // OptionsUI
    int OptionsUI__item_current_idx = 0;
    bool OptionsUI__vsync = false;
    bool OptionsUI__minimizeOnFocus = false;
};

void DrawDebugUI(Game *game);

void DebugDrawUI__Setup();
void DebugDrawUI__Draw(Game *game);

void MainMenuUI__RefreshWorlds(Game *game);
void MainMenuUI__Setup();
void MainMenuUI__Draw(Game *game);
void MainMenuUI__DrawMainMenu(Game *game);
void MainMenuUI__DrawInGame(Game *game);
void MainMenuUI__DrawWorldLists(Game *game);
void MainMenuUI__reset(Game *game);
void MainMenuUI__DrawCreateWorldUI(Game *game);
void MainMenuUI__inputChanged(std::string text, Game *game);

void OptionsUI__Draw(Game *game);
void OptionsUI__DrawGeneral(Game *game);
void OptionsUI__DrawVideo(Game *game);
void OptionsUI__DrawAudio(Game *game);
void OptionsUI__DrawInput(Game *game);

}  // namespace GameUI

extern GameUI::GameUI gameUI;

#endif