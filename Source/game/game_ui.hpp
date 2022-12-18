// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_GAMEUI_HPP_
#define _METADOT_GAMEUI_HPP_

#include "engine/imgui_impl.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "game/World.hpp"
#include "game/materials.hpp"

#include <vector>

class Game;

namespace GameUI {

    void GameUI_Draw(Game *game);

    class DebugUI {
    public:
        static void Draw(Game *game);
    };

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

    class MainMenuUI {
    public:
        static bool visible;

        static int state;

        static bool setup;

        static R_Image *title;

        static bool connectButtonEnabled;

        static ImVec2 pos;

        static std::vector<std::tuple<std::string, WorldMeta>> worlds;

        static long long lastRefresh;

        static void RefreshWorlds(Game *game);

        static void Setup();

        static void Draw(Game *game);

        static void DrawMainMenu(Game *game);

        static void DrawSingleplayer(Game *game);
        static void DrawMultiplayer(Game *game);

        static void DrawCreateWorld(Game *game);

        static void DrawOptions(Game *game);
    };

    class IngameUI {
    public:
        static bool visible;

        static int state;

        static bool setup;

        static void Setup();

        static void Draw(Game *game);

        static void DrawIngame(Game *game);

        static void DrawOptions(Game *game);
    };

    class CreateWorldUI {
    public:
        static bool setup;
        static char worldNameBuf[32];

        static R_Image *materialTestWorld;
        static R_Image *defaultWorld;

        static bool createWorldButtonEnabled;

        static std::string worldFolderLabel;

        static int selIndex;

        static void Setup();
        static void Reset(Game *game);

        static void Draw(Game *game);

        static void inputChanged(std::string text, Game *game);
    };

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

}// namespace GameUI

#endif