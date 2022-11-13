// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef METADOT_SIMD_DRAW_H
#define METADOT_SIMD_DRAW_H

#include "Libs/raylib/raylib.h"

RLAPI void  Mtf_SIMD_ImageDraw            (Image *dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint);
RLAPI Image Mtf_SIMD_GenImageColor        (int width, int height, Color color);
RLAPI void  Mtf_SIMD_ImageDrawRectangleRec(Image *dst, Rectangle rec, Color color);
RLAPI void  Mtf_SIMD_ImageDrawRectangle   (Image *dst, int posX, int posY, int width, int height, Color color);
RLAPI void  Mtf_SIMD_ImageClearBackground (Image *dst, Color color);

#endif