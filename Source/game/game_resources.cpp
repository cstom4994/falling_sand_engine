
#include "core/core.h"
#include "engine/filesystem.h"
#include "engine/sdl_wrapper.h"
#include "libs/Ase_Loader.h"

C_Surface *LoadAseprite(const char *path) {

    Ase_Output *ase = Ase_Load(METADOT_RESLOC(path));

    if (NULL == ase) return nullptr;

    SDL_PixelFormatEnum pixel_format;
    if (ase->bpp == 1) {
        pixel_format = SDL_PIXELFORMAT_INDEX8;
    } else if (ase->bpp == 4) {
        pixel_format = SDL_PIXELFORMAT_RGBA32;
    } else {
        METADOT_ERROR("Test %d BPP not supported!", ase->bpp);
    }

    C_Surface *surface =
            SDL_CreateRGBSurfaceWithFormatFrom(ase->pixels, ase->frame_width * ase->num_frames, ase->frame_height, ase->bpp * 8, ase->bpp * ase->frame_width * ase->num_frames, pixel_format);
    if (!surface) METADOT_ERROR("Surface could not be created!, %s", SDL_GetError());
    SDL_SetPaletteColors(surface->format->palette, (SDL_Color *)&ase->palette.entries, 0, ase->palette.num_entries);
    SDL_SetColorKey(surface, SDL_TRUE, ase->palette.color_key);

    return surface;
}