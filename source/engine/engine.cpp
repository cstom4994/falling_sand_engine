// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "engine/engine.h"

#include <cstdlib>

#include "core/alloc.hpp"
#include "core/core.h"
#include "core/global.hpp"
#include "core/io/filesystem.h"
#include "engine/engine_core.h"
#include "core/platform.h"
#include "game.hpp"
#include "game_resources.hpp"

/////////////////////////////////External data//////////////////////////////////

IMPLENGINE();

////////////////////////////////////////////////////////////////////////////////

unsigned char initializedEngine = 0;

//-------- Engine Functions called from main -------------

// Engine initialization function
// Return 1 if suceeded, 0 if failed
int InitEngine(void (*InitCppReflection)()) {

    if (initializedEngine) {
        METADOT_WARN("InitEngine: Engine already initialized");
    }

    METADOT_INFO("Initializing Engine...");

    bool init = InitTime() || InitFilesystem() || InitScreen(960, 540, 1, 60) || InitCore() || metadot_initwindow();

    if (init) {
        EndEngine(1);
        return METADOT_FAILED;
    }

    InitCppReflection();

    // Open up resource bundle memory space
    global.game->GameIsolate_.texturepack = new TexturePack;

    initializedEngine = 1;
    METADOT_INFO("Engine sucessfully initialized!");
    return METADOT_OK;
}

void EngineUpdate() {
    UpdateTime();

}
void EngineUpdateEnd() {
    // SDL_GL_SwapWindow(Core.window);
    WaitUntilNextFrame();

    ProcessTickTime();
}

void EndEngine(int errorOcurred) {

    delete global.game->GameIsolate_.texturepack;

    if (SDL_WasInit(SDL_INIT_EVERYTHING) != 0) SDL_Quit();

    if (errorOcurred) {
        METADOT_WARN("Engine finished with errors!");
        abort();
    } else {
        METADOT_INFO("Engine finished sucessfully");
    }
}
void DrawSplash() {
    R_Clear(Render.target);
    R_Flip(Render.target);
    Texture *splashSurf = LoadTexture("data/assets/ui/splash.png");
    R_Image *splashImg = R_CopyImageFromSurface(splashSurf->surface);
    R_SetImageFilter(splashImg, R_FILTER_NEAREST);
    R_BlitRect(splashImg, NULL, Render.target, NULL);
    R_FreeImage(splashImg);
    DestroyTexture(splashSurf);
    R_Flip(Render.target);
}
