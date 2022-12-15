// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_ENGINERENDER_H_
#define _METADOT_ENGINERENDER_H_

#include "engine/renderer/renderer_gpu.h"

typedef struct engine_render
{
    R_Target *realTarget;
    R_Target *target;
} engine_render;

#endif