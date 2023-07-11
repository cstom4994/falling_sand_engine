

#include "sdl_surface_utils.h"

#include <cmath>

namespace ME::SurfaceBase {
Uint32 get_pixel32(SDL_Surface *surface, int x, int y) {
    if (surface != NULL) {
        if (x >= 0 && x < surface->w && y >= 0 && y < surface->h) {
            Uint32 *ptr = (Uint32 *)surface->pixels;
            int lineoffset = y * (surface->pitch / 4);
            return ptr[lineoffset + x];
        }
    }
    return 0;
}

Uint8 merge_channel(Uint8 a, Uint8 b, float amount) {
    float result = (b * amount) + (a * (1.0f - amount));
    return (Uint8)result;
}

void put_pixel32(SDL_Surface *surface, int x, int y, Uint32 pixel) {
    if (surface == NULL) {
        return;
    }
    if (x < surface->w && y < surface->h) {

        Uint32 *pixels = (Uint32 *)surface->pixels;

        pixels[(y * surface->w) + x] = pixel;
    }
}

SDL_Texture *R_Create_texture_from_surface(SDL_Renderer *sdlRenderer, SDL_Surface *surf, int format, bool destroySurface) {
    if (surf == NULL) {
        return NULL;
    }

    if (sdlRenderer == NULL) {
        if (destroySurface) {
            SDL_FreeSurface(surf);
            surf = NULL;
        }
        return NULL;
    }
    SDL_Surface *cast_img = SDL_ConvertSurfaceFormat(surf, format, 0);
    SDL_Rect rect = {0, 0, cast_img->w, cast_img->h};
    SDL_Texture *newTexture = SDL_CreateTexture(sdlRenderer, format, SDL_TEXTUREACCESS_STATIC, cast_img->w, cast_img->h);
    SDL_UpdateTexture(newTexture, &rect, cast_img->pixels, cast_img->w * 4);
    SDL_SetTextureBlendMode(newTexture, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(cast_img);
    if (destroySurface) {
        SDL_FreeSurface(surf);
        surf = NULL;
    }
    return newTexture;
}

SDL_Surface *R_Create_filled_surface_rgba(int w, int h, Uint8 color_key_r, Uint8 color_key_g, Uint8 color_key_b, Uint8 alpha) {
    if (w <= 0 || h <= 0) {
        return NULL;
    }
    SDL_Surface *newSurface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA8888);
    if (newSurface == NULL) {
        return NULL;
    }
    int x = 0, y = 0;
    bool surface_is_locked = SDL_MUSTLOCK(newSurface);
    if (surface_is_locked) {
        SDL_LockSurface(newSurface);
    }

    Uint8 rr = 0, bb = 0, gg = 0, aa = 0;
    Uint32 pixel;
    for (x = 0; x < newSurface->w; x++) {

        for (y = 0; y < newSurface->h; y++) {

            pixel = SDL_MapRGBA(newSurface->format, color_key_r, color_key_g, color_key_b, alpha);
            put_pixel32(newSurface, x, y, pixel);
        }
    }

    if (surface_is_locked) {
        SDL_UnlockSurface(newSurface);
    }
    return newSurface;
}

void R_Surface_merge_color_rgba(SDL_Surface *surface, int y, int x1, int x2, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (surface == NULL) {
        return;
    }
    if (surface->w <= x1 && surface->h <= y) {
        return;
    }
    int temp = std::min(x1, x2);
    x2 = std::max(x1, x2);
    x1 = temp;
    Uint32 pixel;
    for (int i = x1; i < x2; i++) {
        pixel = SDL_MapRGBA(surface->format, r, g, b, a);
        put_pixel32(surface, i, y, pixel);
    }
}

void R_Surface_horizontal_line_color_rgba(SDL_Surface *surface, int y, int x1, int x2, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (surface == NULL) {
        return;
    }
    if (surface->w <= x1 && surface->h <= y) {
        return;
    }
    int temp = std::min(x1, x2);
    x2 = std::max(x1, x2);
    x1 = temp;
    Uint32 pixel;
    for (int i = x1; i < x2; i++) {
        pixel = SDL_MapRGBA(surface->format, r, g, b, a);
        put_pixel32(surface, i, y, pixel);
    }
}

void R_Surface_vertical_line_color_rgba(SDL_Surface *surface, int x, int y1, int y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (surface == NULL) {
        return;
    }
    if (surface->h <= y1 && surface->w <= x) {
        return;
    }
    int temp = std::min(y1, y2);
    y2 = std::max(y1, y2);
    y1 = temp;
    Uint32 pixel;
    for (int i = y1; i < y2; i++) {
        pixel = SDL_MapRGBA(surface->format, r, g, b, a);
        put_pixel32(surface, x, i, pixel);
    }
}

bool R_Surface_circle_color_rgba(SDL_Surface *surface, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (surface == NULL) {
        return false;
    }

    Sint16 cx = 0;
    Sint16 cy = rad;
    Sint16 ocx = (Sint16)0xffff;
    Sint16 ocy = (Sint16)0xffff;
    Sint16 df = 1 - rad;
    Sint16 d_e = 3;
    Sint16 d_se = -2 * rad + 5;
    Sint16 xpcx, xmcx, xpcy, xmcy;
    Sint16 ypcy, ymcy, ypcx, ymcx;

    float alphaRatio = 1;
    if (rad <= 0) {
        return false;
    }
    do {
        xpcx = x + cx;
        xmcx = x - cx;
        xpcy = x + cy;
        xmcy = x - cy;
        if (ocy != cy) {
            if (cy > 0) {
                ypcy = y + cy;
                ymcy = y - cy;
                R_Surface_horizontal_line_color_rgba(surface, ypcy, xmcx, xpcx, r, g, b, a);
                R_Surface_horizontal_line_color_rgba(surface, ymcy, xmcx, xpcx, r, g, b, a);
            }

            ocy = cy;
        }
        if (ocx != cx) {
            if (cx != cy) {
                if (cx > 0) {
                    ypcx = y + cx;
                    ymcx = y - cx;
                    R_Surface_horizontal_line_color_rgba(surface, ymcx, xmcy, xpcy, r, g, b, a);
                    R_Surface_horizontal_line_color_rgba(surface, ypcx, xmcy, xpcy, r, g, b, a);
                } else {
                    R_Surface_horizontal_line_color_rgba(surface, y, xmcy, xpcy, r, g, b, a);
                }
            }
            ocx = cx;
        }

        if (df < 0) {
            df += d_e;
            d_e += 2;
            d_se += 2;
        } else {
            df += d_se;
            d_e += 2;
            d_se += 4;
            cy--;
        }
        cx++;
    } while (cx <= cy);
    return true;
}

SDL_Surface *R_Surface_grayscale(SDL_Surface *surface) {
    if (surface == NULL) {
        return NULL;
    }

    SDL_Surface *coloredSurface = NULL;

    coloredSurface = SDL_CreateRGBSurface(0, surface->w, surface->h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask);
    if (coloredSurface != NULL) {
        bool surface_is_locked = SDL_MUSTLOCK(surface);
        if (surface_is_locked) {
            SDL_LockSurface(surface);
        }
        Uint8 rr = 0, bb = 0, gg = 0, aa = 0;
        for (int x = 0; x < coloredSurface->w; x++) {
            for (int y = 0; y < coloredSurface->h; y++) {
                Uint32 pixel = get_pixel32(surface, x, y);
                SDL_GetRGBA(pixel, surface->format, &rr, &gg, &bb, &aa);
                Uint8 grayedColor = (rr + gg + bb) / 3;
                pixel = SDL_MapRGBA(surface->format, grayedColor, grayedColor, grayedColor, aa);
                put_pixel32(coloredSurface, x, y, pixel);
            }
        }
        if (surface_is_locked) {
            SDL_UnlockSurface(surface);
        }
        return coloredSurface;
    }
    return NULL;
}

SDL_Surface *R_Surface_invert(SDL_Surface *surface) {
    if (surface != NULL) {
        SDL_Surface *tempSurface = NULL;
        SDL_Surface *coloredSurface = NULL;

        coloredSurface = SDL_CreateRGBSurface(0, surface->w, surface->h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask);
        if (coloredSurface != NULL) {
            bool surface_is_locked = SDL_MUSTLOCK(surface);
            if (surface_is_locked) {
                SDL_LockSurface(surface);
            }
            Uint8 rr = 0, bb = 0, gg = 0, aa = 0;
            for (int x = 0; x < coloredSurface->w; x++) {
                for (int y = 0; y < coloredSurface->h; y++) {
                    Uint32 pixel = get_pixel32(surface, x, y);
                    SDL_GetRGBA(pixel, surface->format, &rr, &gg, &bb, &aa);
                    pixel = SDL_MapRGBA(surface->format, 255 - rr, 255 - gg, 255 - bb, aa);
                    put_pixel32(coloredSurface, x, y, pixel);
                }
            }
            if (surface_is_locked) {
                SDL_UnlockSurface(surface);
            }
            return coloredSurface;
        } else if (tempSurface != NULL) {
            SDL_FreeSurface(tempSurface);
            tempSurface = NULL;
        }
    }
    return NULL;
}

SDL_Surface *R_Surface_recolor(SDL_Surface *surface, Uint8 color_key_r, Uint8 color_key_g, Uint8 color_key_b, float amount) {
    if (surface != NULL) {

        SDL_Surface *tempSurface = NULL;
        SDL_Surface *coloredSurface = NULL;

        coloredSurface = SDL_CreateRGBSurface(0, surface->w, surface->h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask);
        if (coloredSurface != NULL) {

            bool surface_is_locked = SDL_MUSTLOCK(surface);
            if (surface_is_locked) {
                SDL_LockSurface(surface);
            }

            Uint8 rr = 0, bb = 0, gg = 0, aa = 0;

            for (int x = 0; x < coloredSurface->w; x++) {

                for (int y = 0; y < coloredSurface->h; y++) {

                    Uint32 pixel = get_pixel32(surface, x, y);
                    SDL_GetRGBA(pixel, surface->format, &rr, &gg, &bb, &aa);

                    rr = merge_channel(rr, color_key_r, amount);
                    gg = merge_channel(gg, color_key_g, amount);
                    bb = merge_channel(bb, color_key_b, amount);
                    pixel = SDL_MapRGBA(surface->format, rr, gg, bb, aa);
                    put_pixel32(coloredSurface, x, y, pixel);
                }
            }

            if (surface_is_locked) {
                SDL_UnlockSurface(surface);
            }

            return coloredSurface;
        }
    }
    return NULL;
}

SDL_Surface *R_Surface_remove_color_rgba(SDL_Surface *surface, Uint8 color_key_r, Uint8 color_key_g, Uint8 color_key_b) {
    if (surface != NULL) {

        SDL_Surface *coloredSurface = NULL;

        coloredSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA8888, 0);
        if (coloredSurface != NULL) {

            bool surface_is_locked = SDL_MUSTLOCK(coloredSurface);
            if (surface_is_locked) {
                SDL_LockSurface(coloredSurface);
            }

            Uint8 rr = 0, bb = 0, gg = 0, aa = 0;
            int y = 0;
            Uint32 pixel;

            for (int x = 0; x < coloredSurface->w; x++) {

                for (y = 0; y < coloredSurface->h; y++) {

                    pixel = get_pixel32(coloredSurface, x, y);
                    SDL_GetRGBA(pixel, coloredSurface->format, &rr, &gg, &bb, &aa);

                    if (rr == color_key_r && gg == color_key_g && bb == color_key_b) {
                        pixel = SDL_MapRGBA(coloredSurface->format, 255, 0, 255, 0);
                    }
                    put_pixel32(coloredSurface, x, y, pixel);
                }
            }

            if (surface_is_locked) {
                SDL_UnlockSurface(coloredSurface);
            }
            return coloredSurface;
        }
    }
    return NULL;
}

SDL_Surface *R_Surface_flip(SDL_Surface *surface, int flags) {
    if (surface != NULL) {

        SDL_Surface *flipped = NULL;

        flipped = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA8888, 0);
        if (flipped != NULL) {

            bool surface_is_locked = SDL_MUSTLOCK(flipped);
            if (surface_is_locked) {
                SDL_LockSurface(flipped);
            }

            for (int x = 0, rx = flipped->w - 1; x < flipped->w; x++, rx--) {

                for (int y = 0, ry = flipped->h - 1; y < flipped->h; y++, ry--) {

                    Uint32 pixel = get_pixel32(surface, x, y);

                    if ((flags & SDL_FLIP_VERTICAL) && (flags & SDL_FLIP_HORIZONTAL)) {
                        put_pixel32(flipped, rx, ry, pixel);
                    } else if (flags & SDL_FLIP_HORIZONTAL) {
                        put_pixel32(flipped, rx, y, pixel);
                    } else if (flags & SDL_FLIP_VERTICAL) {
                        put_pixel32(flipped, x, ry, pixel);
                    }
                }
            }

            if (surface_is_locked) {
                SDL_UnlockSurface(flipped);
            }

            return flipped;
        }
    }
    return NULL;
}
}  // namespace ME::SurfaceBase

typedef struct tColorRGBA {
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
} tColorRGBA;

typedef struct tColorY {
    Uint8 y;
} tColorY;

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define GUARD_ROWS (2)

#define VALUE_LIMIT 0.001

Uint32 __colorkey(SDL_Surface *src) {
    Uint32 key = 0;
    SDL_GetColorKey(src, &key);
    return key;
}

/*!
\brief Internal 32 bit integer-factor averaging Shrinker.

Shrinks 32 bit RGBA/ABGR 'src' surface to 'dst' surface.
Averages color and alpha values values of src pixels to calculate dst pixels.
Assumes src and dst surfaces are of 32 bit depth.
Assumes dst surface was allocated with the correct dimensions.

\param src The surface to shrink (input).
\param dst The shrunken surface (output).
\param factorx The horizontal shrinking ratio.
\param factory The vertical shrinking ratio.

\return 0 for success or -1 for error.
*/
int _shrinkSurfaceRGBA(SDL_Surface *src, SDL_Surface *dst, int factorx, int factory) {
    int x, y, dx, dy, dgap, ra, ga, ba, aa;
    int n_average;
    tColorRGBA *sp, *osp, *oosp;
    tColorRGBA *dp;

    /*
     * Averaging integer shrink
     */

    /* Precalculate division factor */
    n_average = factorx * factory;

    /*
     * Scan destination
     */
    sp = (tColorRGBA *)src->pixels;

    dp = (tColorRGBA *)dst->pixels;
    dgap = dst->pitch - dst->w * 4;

    for (y = 0; y < dst->h; y++) {

        osp = sp;
        for (x = 0; x < dst->w; x++) {

            /* Trace out source box and accumulate */
            oosp = sp;
            ra = ga = ba = aa = 0;
            for (dy = 0; dy < factory; dy++) {
                for (dx = 0; dx < factorx; dx++) {
                    ra += sp->r;
                    ga += sp->g;
                    ba += sp->b;
                    aa += sp->a;

                    sp++;
                }
                /* src dx loop */
                sp = (tColorRGBA *)((Uint8 *)sp + (src->pitch - 4 * factorx));  // next y
            }
            /* src dy loop */

            /* next box-x */
            sp = (tColorRGBA *)((Uint8 *)oosp + 4 * factorx);

            /* Store result in destination */
            dp->r = ra / n_average;
            dp->g = ga / n_average;
            dp->b = ba / n_average;
            dp->a = aa / n_average;

            /*
             * Advance destination pointer
             */
            dp++;
        }
        /* dst x loop */

        /* next box-y */
        sp = (tColorRGBA *)((Uint8 *)osp + src->pitch * factory);

        /*
         * Advance destination pointers
         */
        dp = (tColorRGBA *)((Uint8 *)dp + dgap);
    }
    /* dst y loop */

    return (0);
}

/*!
\brief Internal 8 bit integer-factor averaging shrinker.

Shrinks 8bit Y 'src' surface to 'dst' surface.
Averages color (brightness) values values of src pixels to calculate dst pixels.
Assumes src and dst surfaces are of 8 bit depth.
Assumes dst surface was allocated with the correct dimensions.

\param src The surface to shrink (input).
\param dst The shrunken surface (output).
\param factorx The horizontal shrinking ratio.
\param factory The vertical shrinking ratio.

\return 0 for success or -1 for error.
*/
int _shrinkSurfaceY(SDL_Surface *src, SDL_Surface *dst, int factorx, int factory) {
    int x, y, dx, dy, dgap, a;
    int n_average;
    Uint8 *sp, *osp, *oosp;
    Uint8 *dp;

    /*
     * Averaging integer shrink
     */

    /* Precalculate division factor */
    n_average = factorx * factory;

    /*
     * Scan destination
     */
    sp = (Uint8 *)src->pixels;

    dp = (Uint8 *)dst->pixels;
    dgap = dst->pitch - dst->w;

    for (y = 0; y < dst->h; y++) {

        osp = sp;
        for (x = 0; x < dst->w; x++) {

            /* Trace out source box and accumulate */
            oosp = sp;
            a = 0;
            for (dy = 0; dy < factory; dy++) {
                for (dx = 0; dx < factorx; dx++) {
                    a += (*sp);
                    /* next x */
                    sp++;
                }
                /* end src dx loop */
                /* next y */
                sp = (Uint8 *)((Uint8 *)sp + (src->pitch - factorx));
            }
            /* end src dy loop */

            /* next box-x */
            sp = (Uint8 *)((Uint8 *)oosp + factorx);

            /* Store result in destination */
            *dp = a / n_average;

            /*
             * Advance destination pointer
             */
            dp++;
        }
        /* end dst x loop */

        /* next box-y */
        sp = (Uint8 *)((Uint8 *)osp + src->pitch * factory);

        /*
         * Advance destination pointers
         */
        dp = (Uint8 *)((Uint8 *)dp + dgap);
    }
    /* end dst y loop */

    return (0);
}

/*!
\brief Internal 32 bit Zoomer with optional anti-aliasing by bilinear interpolation.

Zooms 32 bit RGBA/ABGR 'src' surface to 'dst' surface.
Assumes src and dst surfaces are of 32 bit depth.
Assumes dst surface was allocated with the correct dimensions.

\param src The surface to zoom (input).
\param dst The zoomed surface (output).
\param flipx Flag indicating if the image should be horizontally flipped.
\param flipy Flag indicating if the image should be vertically flipped.
\param smooth Antialiasing flag; set to SMOOTHING_ON to enable.

\return 0 for success or -1 for error.
*/
int _zoomSurfaceRGBA(SDL_Surface *src, SDL_Surface *dst, int flipx, int flipy, int smooth) {
    int x, y, sx, sy, ssx, ssy, *sax, *say, *csax, *csay, *salast, csx, csy, ex, ey, cx, cy, sstep, sstepx, sstepy;
    tColorRGBA *c00, *c01, *c10, *c11;
    tColorRGBA *sp, *csp, *dp;
    int spixelgap, spixelw, spixelh, dgap, t1, t2;

    /*
     * Allocate memory for row/column increments
     */
    if ((sax = (int *)malloc((dst->w + 1) * sizeof(Uint32))) == NULL) {
        return (-1);
    }
    if ((say = (int *)malloc((dst->h + 1) * sizeof(Uint32))) == NULL) {
        free(sax);
        return (-1);
    }

    /*
     * Precalculate row increments
     */
    spixelw = (src->w - 1);
    spixelh = (src->h - 1);
    if (smooth) {
        sx = (int)(65536.0 * (float)spixelw / (float)(dst->w - 1));
        sy = (int)(65536.0 * (float)spixelh / (float)(dst->h - 1));
    } else {
        sx = (int)(65536.0 * (float)(src->w) / (float)(dst->w));
        sy = (int)(65536.0 * (float)(src->h) / (float)(dst->h));
    }

    /* Maximum scaled source size */
    ssx = (src->w << 16) - 1;
    ssy = (src->h << 16) - 1;

    /* Precalculate horizontal row increments */
    csx = 0;
    csax = sax;
    for (x = 0; x <= dst->w; x++) {
        *csax = csx;
        csax++;
        csx += sx;

        /* Guard from overflows */
        if (csx > ssx) {
            csx = ssx;
        }
    }

    /* Precalculate vertical row increments */
    csy = 0;
    csay = say;
    for (y = 0; y <= dst->h; y++) {
        *csay = csy;
        csay++;
        csy += sy;

        /* Guard from overflows */
        if (csy > ssy) {
            csy = ssy;
        }
    }

    sp = (tColorRGBA *)src->pixels;
    dp = (tColorRGBA *)dst->pixels;
    dgap = dst->pitch - dst->w * 4;
    spixelgap = src->pitch / 4;

    if (flipx) sp += spixelw;
    if (flipy) sp += (spixelgap * spixelh);

    /*
     * Switch between interpolating and non-interpolating code
     */
    if (smooth) {

        /*
         * Interpolating Zoom
         */
        csay = say;
        for (y = 0; y < dst->h; y++) {
            csp = sp;
            csax = sax;
            for (x = 0; x < dst->w; x++) {
                /*
                 * Setup color source pointers
                 */
                ex = (*csax & 0xffff);
                ey = (*csay & 0xffff);
                cx = (*csax >> 16);
                cy = (*csay >> 16);
                sstepx = cx < spixelw;
                sstepy = cy < spixelh;
                c00 = sp;
                c01 = sp;
                c10 = sp;
                if (sstepy) {
                    if (flipy) {
                        c10 -= spixelgap;
                    } else {
                        c10 += spixelgap;
                    }
                }
                c11 = c10;
                if (sstepx) {
                    if (flipx) {
                        c01--;
                        c11--;
                    } else {
                        c01++;
                        c11++;
                    }
                }

                /*
                 * Draw and interpolate colors
                 */
                t1 = ((((c01->r - c00->r) * ex) >> 16) + c00->r) & 0xff;
                t2 = ((((c11->r - c10->r) * ex) >> 16) + c10->r) & 0xff;
                dp->r = (((t2 - t1) * ey) >> 16) + t1;
                t1 = ((((c01->g - c00->g) * ex) >> 16) + c00->g) & 0xff;
                t2 = ((((c11->g - c10->g) * ex) >> 16) + c10->g) & 0xff;
                dp->g = (((t2 - t1) * ey) >> 16) + t1;
                t1 = ((((c01->b - c00->b) * ex) >> 16) + c00->b) & 0xff;
                t2 = ((((c11->b - c10->b) * ex) >> 16) + c10->b) & 0xff;
                dp->b = (((t2 - t1) * ey) >> 16) + t1;
                t1 = ((((c01->a - c00->a) * ex) >> 16) + c00->a) & 0xff;
                t2 = ((((c11->a - c10->a) * ex) >> 16) + c10->a) & 0xff;
                dp->a = (((t2 - t1) * ey) >> 16) + t1;
                /*
                 * Advance source pointer x
                 */
                salast = csax;
                csax++;
                sstep = (*csax >> 16) - (*salast >> 16);
                if (flipx) {
                    sp -= sstep;
                } else {
                    sp += sstep;
                }

                /*
                 * Advance destination pointer x
                 */
                dp++;
            }
            /*
             * Advance source pointer y
             */
            salast = csay;
            csay++;
            sstep = (*csay >> 16) - (*salast >> 16);
            sstep *= spixelgap;
            if (flipy) {
                sp = csp - sstep;
            } else {
                sp = csp + sstep;
            }

            /*
             * Advance destination pointer y
             */
            dp = (tColorRGBA *)((Uint8 *)dp + dgap);
        }
    } else {
        /*
         * Non-Interpolating Zoom
         */
        csay = say;
        for (y = 0; y < dst->h; y++) {
            csp = sp;
            csax = sax;
            for (x = 0; x < dst->w; x++) {
                /*
                 * Draw
                 */
                *dp = *sp;

                /*
                 * Advance source pointer x
                 */
                salast = csax;
                csax++;
                sstep = (*csax >> 16) - (*salast >> 16);
                if (flipx) sstep = -sstep;
                sp += sstep;

                /*
                 * Advance destination pointer x
                 */
                dp++;
            }
            /*
             * Advance source pointer y
             */
            salast = csay;
            csay++;
            sstep = (*csay >> 16) - (*salast >> 16);
            sstep *= spixelgap;
            if (flipy) sstep = -sstep;
            sp = csp + sstep;

            /*
             * Advance destination pointer y
             */
            dp = (tColorRGBA *)((Uint8 *)dp + dgap);
        }
    }

    /*
     * Remove temp arrays
     */
    free(sax);
    free(say);

    return (0);
}

/*!

\brief Internal 8 bit Zoomer without smoothing.

Zooms 8bit palette/Y 'src' surface to 'dst' surface.
Assumes src and dst surfaces are of 8 bit depth.
Assumes dst surface was allocated with the correct dimensions.

\param src The surface to zoom (input).
\param dst The zoomed surface (output).
\param flipx Flag indicating if the image should be horizontally flipped.
\param flipy Flag indicating if the image should be vertically flipped.

\return 0 for success or -1 for error.
*/
int _zoomSurfaceY(SDL_Surface *src, SDL_Surface *dst, int flipx, int flipy) {
    int x, y;
    Uint32 *sax, *say, *csax, *csay;
    int csx, csy;
    Uint8 *sp, *dp, *csp;
    int dgap;

    /*
     * Allocate memory for row increments
     */
    if ((sax = (Uint32 *)malloc((dst->w + 1) * sizeof(Uint32))) == NULL) {
        return (-1);
    }
    if ((say = (Uint32 *)malloc((dst->h + 1) * sizeof(Uint32))) == NULL) {
        free(sax);
        return (-1);
    }

    /*
     * Pointer setup
     */
    sp = csp = (Uint8 *)src->pixels;
    dp = (Uint8 *)dst->pixels;
    dgap = dst->pitch - dst->w;

    if (flipx) csp += (src->w - 1);
    if (flipy) csp = ((Uint8 *)csp + src->pitch * (src->h - 1));

    /*
     * Precalculate row increments
     */
    csx = 0;
    csax = sax;
    for (x = 0; x < dst->w; x++) {
        csx += src->w;
        *csax = 0;
        while (csx >= dst->w) {
            csx -= dst->w;
            (*csax)++;
        }
        (*csax) = (*csax) * (flipx ? -1 : 1);
        csax++;
    }
    csy = 0;
    csay = say;
    for (y = 0; y < dst->h; y++) {
        csy += src->h;
        *csay = 0;
        while (csy >= dst->h) {
            csy -= dst->h;
            (*csay)++;
        }
        (*csay) = (*csay) * (flipy ? -1 : 1);
        csay++;
    }

    /*
     * Draw
     */
    csay = say;
    for (y = 0; y < dst->h; y++) {
        csax = sax;
        sp = csp;
        for (x = 0; x < dst->w; x++) {
            /*
             * Draw
             */
            *dp = *sp;
            /*
             * Advance source pointers
             */
            sp += (*csax);
            csax++;
            /*
             * Advance destination pointer
             */
            dp++;
        }
        /*
         * Advance source pointer (for row)
         */
        csp += ((*csay) * src->pitch);
        csay++;

        /*
         * Advance destination pointers
         */
        dp += dgap;
    }

    /*
     * Remove temp arrays
     */
    free(sax);
    free(say);

    return (0);
}

/*!
\brief Internal 32 bit rotozoomer with optional anti-aliasing.

Rotates and zooms 32 bit RGBA/ABGR 'src' surface to 'dst' surface based on the control
parameters by scanning the destination surface and applying optionally anti-aliasing
by bilinear interpolation.
Assumes src and dst surfaces are of 32 bit depth.
Assumes dst surface was allocated with the correct dimensions.

\param src Source surface.
\param dst Destination surface.
\param cx Horizontal center coordinate.
\param cy Vertical center coordinate.
\param isin Integer version of sine of angle.
\param icos Integer version of cosine of angle.
\param flipx Flag indicating horizontal mirroring should be applied.
\param flipy Flag indicating vertical mirroring should be applied.
\param smooth Flag indicating anti-aliasing should be used.
*/
void ___transformSurfaceRGBA(SDL_Surface *src, SDL_Surface *dst, int cx, int cy, int isin, int icos, int flipx, int flipy, int smooth) {
    int x, y, t1, t2, dx, dy, xd, yd, sdx, sdy, ax, ay, ex, ey, sw, sh;
    tColorRGBA c00, c01, c10, c11, cswap;
    tColorRGBA *pc, *sp;
    int gap;

    /*
     * Variable setup
     */
    xd = ((src->w - dst->w) << 15);
    yd = ((src->h - dst->h) << 15);
    ax = (cx << 16) - (icos * cx);
    ay = (cy << 16) - (isin * cx);
    sw = src->w - 1;
    sh = src->h - 1;
    pc = (tColorRGBA *)dst->pixels;
    gap = dst->pitch - dst->w * 4;

    /*
     * Switch between interpolating and non-interpolating code
     */
    if (smooth) {
        for (y = 0; y < dst->h; y++) {
            dy = cy - y;
            sdx = (ax + (isin * dy)) + xd;
            sdy = (ay - (icos * dy)) + yd;
            for (x = 0; x < dst->w; x++) {
                dx = (sdx >> 16);
                dy = (sdy >> 16);
                if (flipx) dx = sw - dx;
                if (flipy) dy = sh - dy;
                if ((dx > -1) && (dy > -1) && (dx < (src->w - 1)) && (dy < (src->h - 1))) {
                    sp = (tColorRGBA *)src->pixels;
                    ;
                    sp += ((src->pitch / 4) * dy);
                    sp += dx;
                    c00 = *sp;
                    sp += 1;
                    c01 = *sp;
                    sp += (src->pitch / 4);
                    c11 = *sp;
                    sp -= 1;
                    c10 = *sp;
                    if (flipx) {
                        cswap = c00;
                        c00 = c01;
                        c01 = cswap;
                        cswap = c10;
                        c10 = c11;
                        c11 = cswap;
                    }
                    if (flipy) {
                        cswap = c00;
                        c00 = c10;
                        c10 = cswap;
                        cswap = c01;
                        c01 = c11;
                        c11 = cswap;
                    }
                    /*
                     * Interpolate colors
                     */
                    ex = (sdx & 0xffff);
                    ey = (sdy & 0xffff);
                    t1 = ((((c01.r - c00.r) * ex) >> 16) + c00.r) & 0xff;
                    t2 = ((((c11.r - c10.r) * ex) >> 16) + c10.r) & 0xff;
                    pc->r = (((t2 - t1) * ey) >> 16) + t1;
                    t1 = ((((c01.g - c00.g) * ex) >> 16) + c00.g) & 0xff;
                    t2 = ((((c11.g - c10.g) * ex) >> 16) + c10.g) & 0xff;
                    pc->g = (((t2 - t1) * ey) >> 16) + t1;
                    t1 = ((((c01.b - c00.b) * ex) >> 16) + c00.b) & 0xff;
                    t2 = ((((c11.b - c10.b) * ex) >> 16) + c10.b) & 0xff;
                    pc->b = (((t2 - t1) * ey) >> 16) + t1;
                    t1 = ((((c01.a - c00.a) * ex) >> 16) + c00.a) & 0xff;
                    t2 = ((((c11.a - c10.a) * ex) >> 16) + c10.a) & 0xff;
                    pc->a = (((t2 - t1) * ey) >> 16) + t1;
                }
                sdx += icos;
                sdy += isin;
                pc++;
            }
            pc = (tColorRGBA *)((Uint8 *)pc + gap);
        }
    } else {
        for (y = 0; y < dst->h; y++) {
            dy = cy - y;
            sdx = (ax + (isin * dy)) + xd;
            sdy = (ay - (icos * dy)) + yd;
            for (x = 0; x < dst->w; x++) {
                dx = (short)(sdx >> 16);
                dy = (short)(sdy >> 16);
                if (flipx) dx = (src->w - 1) - dx;
                if (flipy) dy = (src->h - 1) - dy;
                if ((dx >= 0) && (dy >= 0) && (dx < src->w) && (dy < src->h)) {
                    sp = (tColorRGBA *)((Uint8 *)src->pixels + src->pitch * dy);
                    sp += dx;
                    *pc = *sp;
                }
                sdx += icos;
                sdy += isin;
                pc++;
            }
            pc = (tColorRGBA *)((Uint8 *)pc + gap);
        }
    }
}

/*!

\brief Rotates and zooms 8 bit palette/Y 'src' surface to 'dst' surface without smoothing.

Rotates and zooms 8 bit RGBA/ABGR 'src' surface to 'dst' surface based on the control
parameters by scanning the destination surface.
Assumes src and dst surfaces are of 8 bit depth.
Assumes dst surface was allocated with the correct dimensions.

\param src Source surface.
\param dst Destination surface.
\param cx Horizontal center coordinate.
\param cy Vertical center coordinate.
\param isin Integer version of sine of angle.
\param icos Integer version of cosine of angle.
\param flipx Flag indicating horizontal mirroring should be applied.
\param flipy Flag indicating vertical mirroring should be applied.
*/
void _transformSurfaceY(SDL_Surface *src, SDL_Surface *dst, int cx, int cy, int isin, int icos, int flipx, int flipy) {
    int x, y, dx, dy, xd, yd, sdx, sdy, ax, ay;
    tColorY *pc, *sp;
    int gap;

    /*
     * Variable setup
     */
    xd = ((src->w - dst->w) << 15);
    yd = ((src->h - dst->h) << 15);
    ax = (cx << 16) - (icos * cx);
    ay = (cy << 16) - (isin * cx);
    pc = (tColorY *)dst->pixels;
    gap = dst->pitch - dst->w;
    /*
     * Clear surface to colorkey
     */
    memset(pc, (int)(__colorkey(src) & 0xff), dst->pitch * dst->h);
    /*
     * Iterate through destination surface
     */
    for (y = 0; y < dst->h; y++) {
        dy = cy - y;
        sdx = (ax + (isin * dy)) + xd;
        sdy = (ay - (icos * dy)) + yd;
        for (x = 0; x < dst->w; x++) {
            dx = (short)(sdx >> 16);
            dy = (short)(sdy >> 16);
            if (flipx) dx = (src->w - 1) - dx;
            if (flipy) dy = (src->h - 1) - dy;
            if ((dx >= 0) && (dy >= 0) && (dx < src->w) && (dy < src->h)) {
                sp = (tColorY *)(src->pixels);
                sp += (src->pitch * dy + dx);
                *pc = *sp;
            }
            sdx += icos;
            sdy += isin;
            pc++;
        }
        pc += gap;
    }
}

/*!
\brief Rotates a 8/16/24/32 bit surface in increments of 90 degrees.

Specialized 90 degree rotator which rotates a 'src' surface in 90 degree
increments clockwise returning a new surface. Faster than rotozoomer since
no scanning or interpolation takes place. Input surface must be 8/16/24/32 bit.
(code contributed by J. Schiller, improved by C. Allport and A. Schiffler)

\param src Source surface to rotate.
\param numClockwiseTurns Number of clockwise 90 degree turns to apply to the source.

\returns The new, rotated surface; or NULL for surfaces with incorrect input format.
*/
SDL_Surface *rotateSurface90Degrees(SDL_Surface *src, int numClockwiseTurns) {
    int row, col, newWidth, newHeight;
    int bpp;
    SDL_Surface *dst;
    Uint8 *srcBuf;
    Uint8 *dstBuf;

    /* Has to be a valid surface pointer and be a 8/16/24/32-bit surface */
    if (!src || !src->format || !(src->format->BitsPerPixel == 8) || (src->format->BitsPerPixel == 16) || (src->format->BitsPerPixel == 24) || (src->format->BitsPerPixel == 32)) {
        return NULL;
    }

    /* normalize numClockwiseTurns */
    while (numClockwiseTurns < 0) {
        numClockwiseTurns += 4;
    }
    numClockwiseTurns = (numClockwiseTurns % 4);

    /* if it's even, our new width will be the same as the source surface */
    newWidth = (numClockwiseTurns % 2) ? (src->h) : (src->w);
    newHeight = (numClockwiseTurns % 2) ? (src->w) : (src->h);
    dst = SDL_CreateRGBSurface(src->flags, newWidth, newHeight, src->format->BitsPerPixel, src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);
    if (!dst) {
        return NULL;
    }

    if (SDL_MUSTLOCK(src)) {
        SDL_LockSurface(src);
    }
    if (SDL_MUSTLOCK(dst)) {
        SDL_LockSurface(dst);
    }

    /* Calculate byte-per-pixel */
    bpp = src->format->BitsPerPixel / 8;

    switch (numClockwiseTurns) {
        case 0: /* Make a copy of the surface */
        {
            /* Unfortunately SDL_BlitSurface cannot be used to make a copy of the surface
            since it does not preserve alpha. */

            if (src->pitch == dst->pitch) {
                /* If the pitch is the same for both surfaces, the memory can be copied all at once. */
                memcpy(dst->pixels, src->pixels, (src->h * src->pitch));
            } else {
                /* If the pitch differs, copy each row separately */
                srcBuf = (Uint8 *)(src->pixels);
                dstBuf = (Uint8 *)(dst->pixels);
                for (row = 0; row < src->h; row++) {
                    memcpy(dstBuf, srcBuf, dst->w * bpp);
                    srcBuf += src->pitch;
                    dstBuf += dst->pitch;
                } /* end for(col) */
            }     /* end for(row) */
        } break;

            /* rotate clockwise */
        case 1: /* rotated 90 degrees clockwise */
        {
            for (row = 0; row < src->h; ++row) {
                srcBuf = (Uint8 *)(src->pixels) + (row * src->pitch);
                dstBuf = (Uint8 *)(dst->pixels) + (dst->w - row - 1) * bpp;
                for (col = 0; col < src->w; ++col) {
                    memcpy(dstBuf, srcBuf, bpp);
                    srcBuf += bpp;
                    dstBuf += dst->pitch;
                }
            }
        } break;

        case 2: /* rotated 180 degrees clockwise */
        {
            for (row = 0; row < src->h; ++row) {
                srcBuf = (Uint8 *)(src->pixels) + (row * src->pitch);
                dstBuf = (Uint8 *)(dst->pixels) + ((dst->h - row - 1) * dst->pitch) + (dst->w - 1) * bpp;
                for (col = 0; col < src->w; ++col) {
                    memcpy(dstBuf, srcBuf, bpp);
                    srcBuf += bpp;
                    dstBuf -= bpp;
                }
            }
        } break;

        case 3: /* rotated 270 degrees clockwise */
        {
            for (row = 0; row < src->h; ++row) {
                srcBuf = (Uint8 *)(src->pixels) + (row * src->pitch);
                dstBuf = (Uint8 *)(dst->pixels) + row + ((dst->h - 1) * dst->pitch);
                for (col = 0; col < src->w; ++col) {
                    *dstBuf = *srcBuf;
                    srcBuf += bpp;
                    dstBuf -= dst->pitch;
                }
            }
        } break;
    }
    /* end switch */

    if (SDL_MUSTLOCK(src)) {
        SDL_UnlockSurface(src);
    }
    if (SDL_MUSTLOCK(dst)) {
        SDL_UnlockSurface(dst);
    }

    return dst;
}

/*!
\brief Internal target surface sizing function for rotozooms with trig result return.

\param width The source surface width.
\param height The source surface height.
\param angle The angle to rotate in degrees.
\param zoomx The horizontal scaling factor.
\param zoomy The vertical scaling factor.
\param dstwidth The calculated width of the destination surface.
\param dstheight The calculated height of the destination surface.
\param canglezoom The sine of the angle adjusted by the zoom factor.
\param sanglezoom The cosine of the angle adjusted by the zoom factor.

*/
void __rotozoomSurfaceSizeTrig(int width, int height, double angle, double zoomx, double zoomy, int *dstwidth, int *dstheight, double *canglezoom, double *sanglezoom) {
    double x, y, cx, cy, sx, sy;
    double radangle;
    int dstwidthhalf, dstheighthalf;

    /*
     * Determine destination width and height by rotating a centered source box
     */
    radangle = angle * (M_PI / 180.0);
    *sanglezoom = sin(radangle);
    *canglezoom = cos(radangle);
    *sanglezoom *= zoomx;
    *canglezoom *= zoomx;
    x = (double)(width / 2);
    y = (double)(height / 2);
    cx = *canglezoom * x;
    cy = *canglezoom * y;
    sx = *sanglezoom * x;
    sy = *sanglezoom * y;

    dstwidthhalf = MAX((int)ceil(MAX(MAX(MAX(fabs(cx + sy), fabs(cx - sy)), fabs(-cx + sy)), fabs(-cx - sy))), 1);
    dstheighthalf = MAX((int)ceil(MAX(MAX(MAX(fabs(sx + cy), fabs(sx - cy)), fabs(-sx + cy)), fabs(-sx - cy))), 1);
    *dstwidth = 2 * dstwidthhalf;
    *dstheight = 2 * dstheighthalf;
}

/*!
\brief Returns the size of the resulting target surface for a rotozoomSurfaceXY() call.

\param width The source surface width.
\param height The source surface height.
\param angle The angle to rotate in degrees.
\param zoomx The horizontal scaling factor.
\param zoomy The vertical scaling factor.
\param dstwidth The calculated width of the rotozoomed destination surface.
\param dstheight The calculated height of the rotozoomed destination surface.
*/
void rotozoomSurfaceSizeXY(int width, int height, double angle, double zoomx, double zoomy, int *dstwidth, int *dstheight) {
    double dummy_sanglezoom, dummy_canglezoom;

    __rotozoomSurfaceSizeTrig(width, height, angle, zoomx, zoomy, dstwidth, dstheight, &dummy_sanglezoom, &dummy_canglezoom);
}

/*!
\brief Returns the size of the resulting target surface for a rotozoomSurface() call.

\param width The source surface width.
\param height The source surface height.
\param angle The angle to rotate in degrees.
\param zoom The scaling factor.
\param dstwidth The calculated width of the rotozoomed destination surface.
\param dstheight The calculated height of the rotozoomed destination surface.
*/
void rotozoomSurfaceSize(int width, int height, double angle, double zoom, int *dstwidth, int *dstheight) {
    double dummy_sanglezoom, dummy_canglezoom;

    __rotozoomSurfaceSizeTrig(width, height, angle, zoom, zoom, dstwidth, dstheight, &dummy_sanglezoom, &dummy_canglezoom);
}

/*!
\brief Rotates and zooms a surface and optional anti-aliasing.

Rotates and zoomes a 32bit or 8bit 'src' surface to newly created 'dst' surface.
'angle' is the rotation in degrees and 'zoom' a scaling factor. If 'smooth' is set
then the destination 32bit surface is anti-aliased. If the surface is not 8bit
or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.

\param src The surface to rotozoom.
\param angle The angle to rotate in degrees.
\param zoom The scaling factor.
\param smooth Antialiasing flag; set to SMOOTHING_ON to enable.

\return The new rotozoomed surface.
*/
SDL_Surface *rotozoomSurface(SDL_Surface *src, double angle, double zoom, int smooth) { return rotozoomSurfaceXY(src, angle, zoom, zoom, smooth); }

/*!
\brief Rotates and zooms a surface with different horizontal and vertival scaling factors and optional anti-aliasing.

Rotates and zooms a 32bit or 8bit 'src' surface to newly created 'dst' surface.
'angle' is the rotation in degrees, 'zoomx and 'zoomy' scaling factors. If 'smooth' is set
then the destination 32bit surface is anti-aliased. If the surface is not 8bit
or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.

\param src The surface to rotozoom.
\param angle The angle to rotate in degrees.
\param zoomx The horizontal scaling factor.
\param zoomy The vertical scaling factor.
\param smooth Antialiasing flag; set to SMOOTHING_ON to enable.

\return The new rotozoomed surface.
*/
SDL_Surface *rotozoomSurfaceXY(SDL_Surface *src, double angle, double zoomx, double zoomy, int smooth) {
    SDL_Surface *rz_src;
    SDL_Surface *rz_dst;
    double zoominv;
    double sanglezoom, canglezoom, sanglezoominv, canglezoominv;
    int dstwidthhalf, dstwidth, dstheighthalf, dstheight;
    int is32bit;
    int i, src_converted;
    int flipx, flipy;

    /*
     * Sanity check
     */
    if (src == NULL) {
        return (NULL);
    }

    /*
     * Determine if source surface is 32bit or 8bit
     */
    is32bit = (src->format->BitsPerPixel == 32);
    if ((is32bit) || (src->format->BitsPerPixel == 8)) {
        /*
         * Use source surface 'as is'
         */
        rz_src = src;
        src_converted = 0;
    } else {
        /*
         * New source surface is 32bit with a defined RGBA ordering
         */
        rz_src = SDL_CreateRGBSurface(SDL_SWSURFACE, src->w, src->h, 32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                      0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#else
                                      0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#endif
        );

        SDL_BlitSurface(src, NULL, rz_src, NULL);

        src_converted = 1;
        is32bit = 1;
    }

    /*
     * Sanity check zoom factor
     */
    flipx = (zoomx < 0.0);
    if (flipx) zoomx = -zoomx;
    flipy = (zoomy < 0.0);
    if (flipy) zoomy = -zoomy;
    if (zoomx < VALUE_LIMIT) zoomx = VALUE_LIMIT;
    if (zoomy < VALUE_LIMIT) zoomy = VALUE_LIMIT;
    zoominv = 65536.0 / (zoomx * zoomx);

    /*
     * Check if we have a rotozoom or just a zoom
     */
    if (fabs(angle) > VALUE_LIMIT) {

        /*
         * Angle!=0: full rotozoom
         */
        /*
         * -----------------------
         */

        /* Determine target size */
        __rotozoomSurfaceSizeTrig(rz_src->w, rz_src->h, angle, zoomx, zoomy, &dstwidth, &dstheight, &canglezoom, &sanglezoom);

        /*
         * Calculate target factors from sin/cos and zoom
         */
        sanglezoominv = sanglezoom;
        canglezoominv = canglezoom;
        sanglezoominv *= zoominv;
        canglezoominv *= zoominv;

        /* Calculate half size */
        dstwidthhalf = dstwidth / 2;
        dstheighthalf = dstheight / 2;

        /*
         * Alloc space to completely contain the rotated surface
         */
        rz_dst = NULL;
        if (is32bit) {
            /*
             * Target surface is 32bit with source RGBA/ABGR ordering
             */
            rz_dst = SDL_CreateRGBSurface(SDL_SWSURFACE, dstwidth, dstheight + GUARD_ROWS, 32, rz_src->format->Rmask, rz_src->format->Gmask, rz_src->format->Bmask, rz_src->format->Amask);
        } else {
            /*
             * Target surface is 8bit
             */
            rz_dst = SDL_CreateRGBSurface(SDL_SWSURFACE, dstwidth, dstheight + GUARD_ROWS, 8, 0, 0, 0, 0);
        }

        /* Check target */
        if (rz_dst == NULL) return NULL;

        /* Adjust for guard rows */
        rz_dst->h = dstheight;

        /*
         * Lock source surface
         */
        if (SDL_MUSTLOCK(rz_src)) {
            SDL_LockSurface(rz_src);
        }

        /*
         * Check which kind of surface we have
         */
        if (is32bit) {
            /*
             * Call the 32bit transformation routine to do the rotation (using alpha)
             */
            ___transformSurfaceRGBA(rz_src, rz_dst, dstwidthhalf, dstheighthalf, (int)(sanglezoominv), (int)(canglezoominv), flipx, flipy, smooth);
        } else {
            /*
             * Copy palette and colorkey info
             */
            for (i = 0; i < rz_src->format->palette->ncolors; i++) {
                rz_dst->format->palette->colors[i] = rz_src->format->palette->colors[i];
            }
            rz_dst->format->palette->ncolors = rz_src->format->palette->ncolors;
            /*
             * Call the 8bit transformation routine to do the rotation
             */
            _transformSurfaceY(rz_src, rz_dst, dstwidthhalf, dstheighthalf, (int)(sanglezoominv), (int)(canglezoominv), flipx, flipy);
        }
        /*
         * Unlock source surface
         */
        if (SDL_MUSTLOCK(rz_src)) {
            SDL_UnlockSurface(rz_src);
        }

    } else {

        /*
         * Angle=0: Just a zoom
         */
        /*
         * --------------------
         */

        /*
         * Calculate target size
         */
        zoomSurfaceSize(rz_src->w, rz_src->h, zoomx, zoomy, &dstwidth, &dstheight);

        /*
         * Alloc space to completely contain the zoomed surface
         */
        rz_dst = NULL;
        if (is32bit) {
            /*
             * Target surface is 32bit with source RGBA/ABGR ordering
             */
            rz_dst = SDL_CreateRGBSurface(SDL_SWSURFACE, dstwidth, dstheight + GUARD_ROWS, 32, rz_src->format->Rmask, rz_src->format->Gmask, rz_src->format->Bmask, rz_src->format->Amask);
        } else {
            /*
             * Target surface is 8bit
             */
            rz_dst = SDL_CreateRGBSurface(SDL_SWSURFACE, dstwidth, dstheight + GUARD_ROWS, 8, 0, 0, 0, 0);
        }

        /* Check target */
        if (rz_dst == NULL) return NULL;

        /* Adjust for guard rows */
        rz_dst->h = dstheight;

        /*
         * Lock source surface
         */
        if (SDL_MUSTLOCK(rz_src)) {
            SDL_LockSurface(rz_src);
        }

        /*
         * Check which kind of surface we have
         */
        if (is32bit) {
            /*
             * Call the 32bit transformation routine to do the zooming (using alpha)
             */
            _zoomSurfaceRGBA(rz_src, rz_dst, flipx, flipy, smooth);

        } else {
            /*
             * Copy palette and colorkey info
             */
            for (i = 0; i < rz_src->format->palette->ncolors; i++) {
                rz_dst->format->palette->colors[i] = rz_src->format->palette->colors[i];
            }
            rz_dst->format->palette->ncolors = rz_src->format->palette->ncolors;

            /*
             * Call the 8bit transformation routine to do the zooming
             */
            _zoomSurfaceY(rz_src, rz_dst, flipx, flipy);
        }

        /*
         * Unlock source surface
         */
        if (SDL_MUSTLOCK(rz_src)) {
            SDL_UnlockSurface(rz_src);
        }
    }

    /*
     * Cleanup temp surface
     */
    if (src_converted) {
        SDL_FreeSurface(rz_src);
    }

    /*
     * Return destination surface
     */
    return (rz_dst);
}

/*!
\brief Calculates the size of the target surface for a zoomSurface() call.

The minimum size of the target surface is 1. The input factors can be positive or negative.

\param width The width of the source surface to zoom.
\param height The height of the source surface to zoom.
\param zoomx The horizontal zoom factor.
\param zoomy The vertical zoom factor.
\param dstwidth Pointer to an integer to store the calculated width of the zoomed target surface.
\param dstheight Pointer to an integer to store the calculated height of the zoomed target surface.
*/
void zoomSurfaceSize(int width, int height, double zoomx, double zoomy, int *dstwidth, int *dstheight) {
    /*
     * Make zoom factors positive
     */
    int flipx, flipy;
    flipx = (zoomx < 0.0);
    if (flipx) zoomx = -zoomx;
    flipy = (zoomy < 0.0);
    if (flipy) zoomy = -zoomy;

    /*
     * Sanity check zoom factors
     */
    if (zoomx < VALUE_LIMIT) {
        zoomx = VALUE_LIMIT;
    }
    if (zoomy < VALUE_LIMIT) {
        zoomy = VALUE_LIMIT;
    }

    /*
     * Calculate target size
     */
    *dstwidth = (int)floor(((double)width * zoomx) + 0.5);
    *dstheight = (int)floor(((double)height * zoomy) + 0.5);
    if (*dstwidth < 1) {
        *dstwidth = 1;
    }
    if (*dstheight < 1) {
        *dstheight = 1;
    }
}

/*!
\brief Zoom a surface by independent horizontal and vertical factors with optional smoothing.

Zooms a 32bit or 8bit 'src' surface to newly created 'dst' surface.
'zoomx' and 'zoomy' are scaling factors for width and height. If 'smooth' is on
then the destination 32bit surface is anti-aliased. If the surface is not 8bit
or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.
If zoom factors are negative, the image is flipped on the axes.

\param src The surface to zoom.
\param zoomx The horizontal zoom factor.
\param zoomy The vertical zoom factor.
\param smooth Antialiasing flag; set to SMOOTHING_ON to enable.

\return The new, zoomed surface.
*/
SDL_Surface *zoomSurface(SDL_Surface *src, double zoomx, double zoomy, int smooth) {
    SDL_Surface *rz_src;
    SDL_Surface *rz_dst;
    int dstwidth, dstheight;
    int is32bit;
    int i, src_converted;
    int flipx, flipy;

    /*
     * Sanity check
     */
    if (src == NULL) return (NULL);

    /*
     * Determine if source surface is 32bit or 8bit
     */
    is32bit = (src->format->BitsPerPixel == 32);
    if ((is32bit) || (src->format->BitsPerPixel == 8)) {
        /*
         * Use source surface 'as is'
         */
        rz_src = src;
        src_converted = 0;
    } else {
        /*
         * New source surface is 32bit with a defined RGBA ordering
         */
        rz_src = SDL_CreateRGBSurface(SDL_SWSURFACE, src->w, src->h, 32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                      0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#else
                                      0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#endif
        );
        if (rz_src == NULL) {
            return NULL;
        }
        SDL_BlitSurface(src, NULL, rz_src, NULL);
        src_converted = 1;
        is32bit = 1;
    }

    flipx = (zoomx < 0.0);
    if (flipx) zoomx = -zoomx;
    flipy = (zoomy < 0.0);
    if (flipy) zoomy = -zoomy;

    /* Get size if target */
    zoomSurfaceSize(rz_src->w, rz_src->h, zoomx, zoomy, &dstwidth, &dstheight);

    /*
     * Alloc space to completely contain the zoomed surface
     */
    rz_dst = NULL;
    if (is32bit) {
        /*
         * Target surface is 32bit with source RGBA/ABGR ordering
         */
        rz_dst = SDL_CreateRGBSurface(SDL_SWSURFACE, dstwidth, dstheight + GUARD_ROWS, 32, rz_src->format->Rmask, rz_src->format->Gmask, rz_src->format->Bmask, rz_src->format->Amask);
    } else {
        /*
         * Target surface is 8bit
         */
        rz_dst = SDL_CreateRGBSurface(SDL_SWSURFACE, dstwidth, dstheight + GUARD_ROWS, 8, 0, 0, 0, 0);
    }

    /* Check target */
    if (rz_dst == NULL) {
        /*
         * Cleanup temp surface
         */
        if (src_converted) {
            SDL_FreeSurface(rz_src);
        }
        return NULL;
    }

    /* Adjust for guard rows */
    rz_dst->h = dstheight;

    /*
     * Lock source surface
     */
    if (SDL_MUSTLOCK(rz_src)) {
        SDL_LockSurface(rz_src);
    }

    /*
     * Check which kind of surface we have
     */
    if (is32bit) {
        /*
         * Call the 32bit transformation routine to do the zooming (using alpha)
         */
        _zoomSurfaceRGBA(rz_src, rz_dst, flipx, flipy, smooth);
    } else {
        /*
         * Copy palette and colorkey info
         */
        for (i = 0; i < rz_src->format->palette->ncolors; i++) {
            rz_dst->format->palette->colors[i] = rz_src->format->palette->colors[i];
        }
        rz_dst->format->palette->ncolors = rz_src->format->palette->ncolors;
        /*
         * Call the 8bit transformation routine to do the zooming
         */
        _zoomSurfaceY(rz_src, rz_dst, flipx, flipy);
    }
    /*
     * Unlock source surface
     */
    if (SDL_MUSTLOCK(rz_src)) {
        SDL_UnlockSurface(rz_src);
    }

    /*
     * Cleanup temp surface
     */
    if (src_converted) {
        SDL_FreeSurface(rz_src);
    }

    /*
     * Return destination surface
     */
    return (rz_dst);
}

/*!
\brief Shrink a surface by an integer ratio using averaging.

Shrinks a 32bit or 8bit 'src' surface to a newly created 'dst' surface.
'factorx' and 'factory' are the shrinking ratios (i.e. 2=1/2 the size,
3=1/3 the size, etc.) The destination surface is antialiased by averaging
the source box RGBA or Y information. If the surface is not 8bit
or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.
The input surface is not modified. The output surface is newly allocated.

\param src The surface to shrink.
\param factorx The horizontal shrinking ratio.
\param factory The vertical shrinking ratio.

\return The new, shrunken surface.
*/
/*@null@*/
SDL_Surface *shrinkSurface(SDL_Surface *src, int factorx, int factory) {
    int result;
    SDL_Surface *rz_src;
    SDL_Surface *rz_dst = NULL;
    int dstwidth, dstheight;
    int is32bit;
    int i, src_converted;
    int haveError = 0;

    /*
     * Sanity check
     */
    if (src == NULL) {
        return (NULL);
    }

    /*
     * Determine if source surface is 32bit or 8bit
     */
    is32bit = (src->format->BitsPerPixel == 32);
    if ((is32bit) || (src->format->BitsPerPixel == 8)) {
        /*
         * Use source surface 'as is'
         */
        rz_src = src;
        src_converted = 0;
    } else {
        /*
         * New source surface is 32bit with a defined RGBA ordering
         */
        rz_src = SDL_CreateRGBSurface(SDL_SWSURFACE, src->w, src->h, 32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                      0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#else
                                      0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#endif
        );
        if (rz_src == NULL) {
            haveError = 1;
            goto exitShrinkSurface;
        }

        SDL_BlitSurface(src, NULL, rz_src, NULL);
        src_converted = 1;
        is32bit = 1;
    }

    /*
     * Lock the surface
     */
    if (SDL_MUSTLOCK(rz_src)) {
        if (SDL_LockSurface(rz_src) < 0) {
            haveError = 1;
            goto exitShrinkSurface;
        }
    }

    /* Get size for target */
    dstwidth = rz_src->w / factorx;
    while (dstwidth * factorx > rz_src->w) {
        dstwidth--;
    }
    dstheight = rz_src->h / factory;
    while (dstheight * factory > rz_src->h) {
        dstheight--;
    }

    /*
     * Alloc space to completely contain the shrunken surface
     * (with added guard rows)
     */
    if (is32bit == 1) {
        /*
         * Target surface is 32bit with source RGBA/ABGR ordering
         */
        rz_dst = SDL_CreateRGBSurface(SDL_SWSURFACE, dstwidth, dstheight + GUARD_ROWS, 32, rz_src->format->Rmask, rz_src->format->Gmask, rz_src->format->Bmask, rz_src->format->Amask);
    } else {
        /*
         * Target surface is 8bit
         */
        rz_dst = SDL_CreateRGBSurface(SDL_SWSURFACE, dstwidth, dstheight + GUARD_ROWS, 8, 0, 0, 0, 0);
    }

    /* Check target */
    if (rz_dst == NULL) {
        haveError = 1;
        goto exitShrinkSurface;
    }

    /* Adjust for guard rows */
    rz_dst->h = dstheight;

    /*
     * Check which kind of surface we have
     */
    if (is32bit == 1) {
        /*
         * Call the 32bit transformation routine to do the shrinking (using alpha)
         */
        result = _shrinkSurfaceRGBA(rz_src, rz_dst, factorx, factory);
        if ((result != 0) || (rz_dst == NULL)) {
            haveError = 1;
            goto exitShrinkSurface;
        }
    } else {
        /*
         * Copy palette and colorkey info
         */
        for (i = 0; i < rz_src->format->palette->ncolors; i++) {
            rz_dst->format->palette->colors[i] = rz_src->format->palette->colors[i];
        }
        rz_dst->format->palette->ncolors = rz_src->format->palette->ncolors;
        /*
         * Call the 8bit transformation routine to do the shrinking
         */
        result = _shrinkSurfaceY(rz_src, rz_dst, factorx, factory);
        if (result != 0) {
            haveError = 1;
            goto exitShrinkSurface;
        }
    }

exitShrinkSurface:
    if (rz_src != NULL) {
        /*
         * Unlock source surface
         */
        if (SDL_MUSTLOCK(rz_src)) {
            SDL_UnlockSurface(rz_src);
        }

        /*
         * Cleanup temp surface
         */
        if (src_converted == 1) {
            SDL_FreeSurface(rz_src);
        }
    }

    /* Check error state; maybe need to cleanup destination */
    if (haveError == 1) {
        if (rz_dst != NULL) {
            SDL_FreeSurface(rz_dst);
        }
        rz_dst = NULL;
    }

    /*
     * Return destination surface
     */
    return (rz_dst);
}
