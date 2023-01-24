// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "engine.h"

#include "core/alloc.h"
#include "core/core.h"
#include "core/global.hpp"
#include "engine_core.h"
#include "engine_ecs.h"
#include "engine_platform.h"
#include "filesystem.h"
#include "game.hpp"
#include "game_resources.hpp"

/////////////////////////////////External data//////////////////////////////////

IMPLENGINE();

extern unsigned char initializedECS;

////////////////////////////////////////////////////////////////////////////////

unsigned char initializedEngine = 0;

//-------- Engine Functions called from main -------------

// Engine initialization function
// Return 1 if suceeded, 0 if failed
int InitEngine(void (*InitCppReflection)()) {
    if (initializedEngine) {
        METADOT_WARN("InitEngine: Engine already initialized");
    }
    if (!initializedECS) {
        METADOT_WARN(
                "InitEngine: ECS not initialized! Initialize and configure ECS before "
                "initializing the engine!");
        return 0;
    }

    METADOT_INFO("Initializing Engine...");

    InitTime();
    InitFilesystem();
    InitScreen(960, 540, 1, 60);

    if (!InitCore() || !InitWindow()) {
        EndEngine(1);
        return 0;
    }

    InitCppReflection();

    // Call initialization function of all systems
    ListCellPointer current = GetFirstCell(ECS.SystemList);
    while (current) {
        System *curSystem = ((System *)GetElement(*current));
        curSystem->systemInit();
        current = GetNextCell(current);
    }

    // Disable text input
    //  SDL_StopTextInput();

    // Open up resource bundle memory space
    global.game->GameIsolate_.texturepack = (TexturePack *)gc_malloc(&gc, sizeof(TexturePack));

    initializedEngine = 1;
    METADOT_INFO("Engine sucessfully initialized!");
    return 1;
}

void EngineUpdate() {
    UpdateTime();

    // Run systems updates

    // Remove missing child connections from parents
    int e;
    for (e = 0; e <= ECS.maxUsedIndex; e++) {
        if (IsValidEntity(e) && EntityIsParent(e)) {
            int i = 0;
            ListCellPointer child = GetFirstCell(*GetChildsList(e));
            while (child) {
                if (!IsValidEntity(GetElementAsType(child, EntityID))) {
                    child = GetNextCell(child);
                    RemoveListIndex(GetChildsList(e), i);
                } else {
                    i++;
                    child = GetNextCell(child);
                }
            }
        }
    }

    // Iterate through the systems list
    ListCellPointer currentSystem = GetFirstCell(ECS.SystemList);
    ListForEach(currentSystem, ECS.SystemList) {
        System sys = GetElementAsType(currentSystem, System);
        if (!sys.enabled) continue;
        sys.systemUpdate();
    }
}
void EngineUpdateEnd() {
    // SDL_GL_SwapWindow(Core.window);
    WaitUntilNextFrame();

    ProcessTickTime();
}

void EndEngine(int errorOcurred) {

    gc_free(&gc, global.game->GameIsolate_.texturepack);

    FreeECS();

    if (SDL_WasInit(SDL_INIT_EVERYTHING) != 0) SDL_Quit();

    if (errorOcurred) {
        METADOT_WARN("Engine finished with errors!");
        system("pause");
    } else {
        METADOT_INFO("Engine finished sucessfully");
    }
}