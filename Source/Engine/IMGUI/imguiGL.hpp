//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
// Copyright (c) 2011-2014 Mario 'rlyeh' Rodriguez
// Copyright (c) 2013 Adrien Herubel
// Copyright (c) 2022, KaoruXun
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//


#ifndef IMGUIGL_HPP
#define IMGUIGL_HPP

#ifndef $yes
#define $yes(...) __VA_ARGS__
#endif

#ifndef $no
#define $no(...)
#endif

#ifndef $GL2
#define $GL2 $yes
#endif

#ifndef $GL3
#define $GL3 $no
#endif

#include "Engine/lib/final_dynamic_opengl.h"

bool imguiRenderGLInit();
void imguiRenderGLDestroy();
void imguiRenderGLDraw(int width, int height);

bool imguiRenderGLFontInit(int font, float pt, const void *data, unsigned size);
bool imguiRenderGLFontInit(int font, float pt, const char *fontpath);

unsigned imguiRenderGLMakeTexture(const void *data, unsigned size);
unsigned imguiRenderGLMakeTexture(const char *imagepath);
void imguiRenderGLDestroyTexture(unsigned id);

#endif// IMGUIGL_HPP
