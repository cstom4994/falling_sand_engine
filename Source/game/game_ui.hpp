// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_GAMEUI_HPP_
#define _METADOT_GAMEUI_HPP_

#include <vector>

#include "engine/imgui/imgui_impl.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/scripting/lua_wrapper.hpp"
#include "game/world.hpp"
#include "game/game_datastruct.hpp"

class Game;

struct I18N {
    void Init();
    void Load(std::string lang);
    std::string Get(std::string text);
};

namespace GameUI {

void DrawDebugUI(Game *game);

class DebugDrawUI {
public:
    static bool visible;
    static int selIndex;
    static std::vector<R_Image *> images;
    static std::vector<R_Image *> tools_images;

    static Material *selectedMaterial;
    static U8 brushSize;

    static void Setup();

    static void Draw(Game *game);
};

extern bool MainMenuUI__visible;

void MainMenuUI__RefreshWorlds(Game *game);
void MainMenuUI__Setup();
void MainMenuUI__Draw(Game *game);
void MainMenuUI__DrawMainMenu(Game *game);
void MainMenuUI__DrawInGame(Game *game);
void MainMenuUI__DrawWorldLists(Game *game);
void MainMenuUI__reset(Game *game);
void MainMenuUI__DrawCreateWorldUI(Game *game);
void MainMenuUI__inputChanged(std::string text, Game *game);

class OptionsUI {
#if defined(METADOT_BUILD_AUDIO)
    static std::map<std::string, FMOD::Studio::Bus *> busMap;
#endif

public:
    static int item_current_idx;
    static bool vsync;
    static bool minimizeOnFocus;

    static void Draw(Game *game);
    static void DrawGeneral(Game *game);
    static void DrawVideo(Game *game);
    static void DrawAudio(Game *game);
    static void DrawInput(Game *game);
};

}  // namespace GameUI

#endif