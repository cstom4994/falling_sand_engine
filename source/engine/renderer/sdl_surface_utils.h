
#ifndef ME_SDL_SURFACE_UTILS_H
#define ME_SDL_SURFACE_UTILS_H

#include "engine/core/sdl_wrapper.h"

namespace ME::SurfaceBase {

SDL_Texture *R_Create_texture_from_surface(SDL_Renderer *sdlRenderer, SDL_Surface *surf, int format = SDL_PIXELFORMAT_RGBA8888, bool destroySurface = true);
SDL_Surface *R_Create_filled_surface_rgba(int w, int h, Uint8 color_key_r, Uint8 color_key_g, Uint8 color_key_b, Uint8 alpha = 255);

Uint32 get_pixel32(SDL_Surface *surface, int x, int y);

Uint8 merge_channel(Uint8 a, Uint8 b, float amount);

void put_pixel32(SDL_Surface *surface, int x, int y, Uint32 pixel);

void R_Surface_horizontal_line_color_rgba(SDL_Surface *surface, int y, int x1, int x2, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
void R_Surface_vertical_line_color_rgba(SDL_Surface *surface, int x, int y1, int y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
bool R_Surface_circle_color_rgba(SDL_Surface *surface, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);

SDL_Surface *R_Surface_grayscale(SDL_Surface *surface);
SDL_Surface *R_Surface_invert(SDL_Surface *surface);
SDL_Surface *R_Surface_merge_color_rgba(SDL_Surface *surface, Uint8 color_key_r, Uint8 color_key_g, Uint8 color_key_b, float amount);
SDL_Surface *R_Surface_recolor(SDL_Surface *surface, Uint8 color_key_r, Uint8 color_key_g, Uint8 color_key_b, float amount);
SDL_Surface *R_Surface_remove_color_rgba(SDL_Surface *surface, Uint8 color_key_r, Uint8 color_key_g, Uint8 color_key_b);
SDL_Surface *R_Surface_flip(SDL_Surface *surface, int flags);

};  // namespace ME::SurfaceBase

#define SMOOTHING_OFF 0
#define SMOOTHING_ON 1

SDL_Surface *rotozoomSurface(SDL_Surface *src, double angle, double zoom, int smooth);
SDL_Surface *rotozoomSurfaceXY(SDL_Surface *src, double angle, double zoomx, double zoomy, int smooth);

void rotozoomSurfaceSize(int width, int height, double angle, double zoom, int *dstwidth, int *dstheight);
void rotozoomSurfaceSizeXY(int width, int height, double angle, double zoomx, double zoomy, int *dstwidth, int *dstheight);

SDL_Surface *zoomSurface(SDL_Surface *src, double zoomx, double zoomy, int smooth);
void zoomSurfaceSize(int width, int height, double zoomx, double zoomy, int *dstwidth, int *dstheight);
SDL_Surface *shrinkSurface(SDL_Surface *src, int factorx, int factory);
SDL_Surface *rotateSurface90Degrees(SDL_Surface *src, int numClockwiseTurns);

#endif