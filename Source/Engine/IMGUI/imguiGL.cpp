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

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdio>

#include "imgui.hpp"
#include "imguiGL.hpp"

#include "stb_image.h"

// Some math headers don't have PI defined.
static const float PI = 3.14159265f;

void imguifree(void *ptr, void *userptr);
void *imguimalloc(size_t size, void *userptr);

#ifndef IMGUI_BYPASS_FONTSTASH_IMPLEMENTATION
#define FONTSTASH_IMPLEMENTATION  // Expands implementation
#define GLFONTSTASH_IMPLEMENTATION// Expands implementation
#endif

#if 1
#include "fontstash.h"
#include <cstdio> // malloc, free, fopen, fclose, ftell, fseek, fread
#include <string>// memset

//#include <GLFW/glfw3.h>             // Or any other GL header of your choice.
#include "fontstashGL.h"
// Create GL stash for 512x512 texture, our coordinate system has zero at top-left.
struct FONScontext *fs;
// Add font to stash.
struct font_style
{
    int face;
    float size;
} fonts[8];
#else
#include <cmath>
#define STBTT_malloc(x, y) imguimalloc(x, y)
#define STBTT_free(x, y) imguifree(x, y)
#define STBTT_ifloor(x) ((int) std::floorf(x))
#define STBTT_iceil(x) ((int) std::ceilf(x))
#include "stb_truetype.h"
static stbtt_bakedchar g_cdata[96];// ASCII 32..126 is 95 glyphs
#endif

void imguifree(void *ptr, void * /*userptr*/) {
    delete[] ptr;
}

void *imguimalloc(size_t size, void * /*userptr*/) {
    return new char[size];
}


enum {
    TEMP_COORD_COUNT = 100
};
enum {
    CIRCLE_VERTS = 8 * 4
};

static float g_tempCoords[TEMP_COORD_COUNT * 2];
static float g_tempNormals[TEMP_COORD_COUNT * 2];
$GL3(
        static float g_tempVertices[TEMP_COORD_COUNT * 12 + (TEMP_COORD_COUNT - 2) * 6];
        static float g_tempTextureCoords[TEMP_COORD_COUNT * 12 + (TEMP_COORD_COUNT - 2) * 6];
        static float g_tempColors[TEMP_COORD_COUNT * 24 + (TEMP_COORD_COUNT - 2) * 12];)

static float g_circleVerts[CIRCLE_VERTS * 2];

static GLuint g_ftex = 0;
$GL3(
        static GLuint g_whitetex = 0;
        static GLuint g_vao = 0;
        static GLuint g_vbos[3] = {0, 0, 0};
        static GLuint g_program = 0;
        static GLuint g_programViewportLocation = 0;
        static GLuint g_programTextureLocation = 0;)

inline unsigned int RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return (r) | (g << 8) | (b << 16) | (a << 24);
}

static void drawPolygon(const float *coords, unsigned numCoords, float r, unsigned int col, bool grad = true) {
    if (numCoords > TEMP_COORD_COUNT) numCoords = TEMP_COORD_COUNT;

    for (unsigned i = 0, j = numCoords - 1; i < numCoords; j = i++) {
        const float *v0 = &coords[j * 2];
        const float *v1 = &coords[i * 2];
        float dx = v1[0] - v0[0];
        float dy = v1[1] - v0[1];
        float d = sqrtf(dx * dx + dy * dy);
        if (d > 0) {
            d = 1.0f / d;
            dx *= d;
            dy *= d;
        }
        g_tempNormals[j * 2 + 0] = dy;
        g_tempNormals[j * 2 + 1] = -dx;
    }

    $GL3(
            float colf[4] = {(float) (col & 0xff) / 255.f, (float) ((col >> 8) & 0xff) / 255.f, (float) ((col >> 16) & 0xff) / 255.f, (float) ((col >> 24) & 0xff) / 255.f};
            float colTransf[4] = {(float) (col & 0xff) / 255.f, (float) ((col >> 8) & 0xff) / 255.f, (float) ((col >> 16) & 0xff) / 255.f, 0};)
    for (unsigned i = 0, j = numCoords - 1; i < numCoords; j = i++) {
        float dlx0 = g_tempNormals[j * 2 + 0];
        float dly0 = g_tempNormals[j * 2 + 1];
        float dlx1 = g_tempNormals[i * 2 + 0];
        float dly1 = g_tempNormals[i * 2 + 1];
        float dmx = (dlx0 + dlx1) * 0.5f;
        float dmy = (dly0 + dly1) * 0.5f;
        float dmr2 = dmx * dmx + dmy * dmy;
        if (dmr2 > 0.000001f) {
            float scale = 1.0f / dmr2;
            if (scale > 10.0f) scale = 10.0f;
            dmx *= scale;
            dmy *= scale;
        }
        g_tempCoords[i * 2 + 0] = coords[i * 2 + 0] + dmx * r;
        g_tempCoords[i * 2 + 1] = coords[i * 2 + 1] + dmy * r;
    }

    $GL3(
            int vSize = (grad ? numCoords * 12 : 0) + (numCoords - 2) * 6;
            int uvSize = (grad ? numCoords * 2 * 6 : 0) + (numCoords - 2) * 2 * 3;
            int cSize = (grad ? numCoords * 4 * 6 : 0) + (numCoords - 2) * 4 * 3;
            float *v = g_tempVertices;
            float *uv = g_tempTextureCoords;
            memset(uv, 0, uvSize * sizeof(float));
            float *c = g_tempColors;
            memset(c, 1, cSize * sizeof(float));

            float *ptrV = v;
            float *ptrC = c;)
    $GL2(
            unsigned int colTrans = RGBA(col & 0xff, (col >> 8) & 0xff, (col >> 16) & 0xff, 0);

            glBegin(grad ? GL_TRIANGLES : GL_TRIANGLE_STRIP);

            glColor4ubv((GLubyte *) &col);)
    if (grad)
        for (unsigned i = 0, j = numCoords - 1; i < numCoords; j = i++) {
            $GL3(
                            *ptrV = coords[i * 2];
                            *(ptrV + 1) = coords[i * 2 + 1];
                            ptrV += 2;
                            *ptrV = coords[j * 2];
                            *(ptrV + 1) = coords[j * 2 + 1];
                            ptrV += 2;
                            *ptrV = g_tempCoords[j * 2];
                            *(ptrV + 1) = g_tempCoords[j * 2 + 1];
                            ptrV += 2;
                            *ptrV = g_tempCoords[j * 2];
                            *(ptrV + 1) = g_tempCoords[j * 2 + 1];
                            ptrV += 2;
                            *ptrV = g_tempCoords[i * 2];
                            *(ptrV + 1) = g_tempCoords[i * 2 + 1];
                            ptrV += 2;
                            *ptrV = coords[i * 2];
                            *(ptrV + 1) = coords[i * 2 + 1];
                            ptrV += 2;

                            *ptrC = colf[0];
                            *(ptrC + 1) = colf[1];
                            *(ptrC + 2) = colf[2];
                            *(ptrC + 3) = colf[3];
                            ptrC += 4;
                            *ptrC = colf[0];
                            *(ptrC + 1) = colf[1];
                            *(ptrC + 2) = colf[2];
                            *(ptrC + 3) = colf[3];
                            ptrC += 4;
                            *ptrC = colTransf[0];
                            *(ptrC + 1) = colTransf[1];
                            *(ptrC + 2) = colTransf[2];
                            *(ptrC + 3) = colTransf[3];
                            ptrC += 4;
                            *ptrC = colTransf[0];
                            *(ptrC + 1) = colTransf[1];
                            *(ptrC + 2) = colTransf[2];
                            *(ptrC + 3) = colTransf[3];
                            ptrC += 4;
                            *ptrC = colTransf[0];
                            *(ptrC + 1) = colTransf[1];
                            *(ptrC + 2) = colTransf[2];
                            *(ptrC + 3) = colTransf[3];
                            ptrC += 4;
                            *ptrC = colf[0];
                            *(ptrC + 1) = colf[1];
                            *(ptrC + 2) = colf[2];
                            *(ptrC + 3) = colf[3];
                            ptrC += 4;)
            $GL2(
                    glVertex2fv(&coords[i * 2]);
                    glVertex2fv(&coords[j * 2]);
                    glColor4ubv((GLubyte *) &colTrans);
                    glVertex2fv(&g_tempCoords[j * 2]);

                    glVertex2fv(&g_tempCoords[j * 2]);
                    glVertex2fv(&g_tempCoords[i * 2]);
                    glColor4ubv((GLubyte *) &col);
                    glVertex2fv(&coords[i * 2]);)
        }
    $GL2(
            glColor4ubv((GLubyte *) &col);)
    for (unsigned i = 2; i < numCoords; ++i) {
        $GL3(
                        *ptrV = coords[0];
                        *(ptrV + 1) = coords[1];
                        ptrV += 2;
                        *ptrV = coords[(i - 1) * 2];
                        *(ptrV + 1) = coords[(i - 1) * 2 + 1];
                        ptrV += 2;
                        *ptrV = coords[i * 2];
                        *(ptrV + 1) = coords[i * 2 + 1];
                        ptrV += 2;

                        *ptrC = colf[0];
                        *(ptrC + 1) = colf[1];
                        *(ptrC + 2) = colf[2];
                        *(ptrC + 3) = colf[3];
                        ptrC += 4;
                        *ptrC = colf[0];
                        *(ptrC + 1) = colf[1];
                        *(ptrC + 2) = colf[2];
                        *(ptrC + 3) = colf[3];
                        ptrC += 4;
                        *ptrC = colf[0];
                        *(ptrC + 1) = colf[1];
                        *(ptrC + 2) = colf[2];
                        *(ptrC + 3) = colf[3];
                        ptrC += 4;)
        $GL2(
                if (grad)
                        glVertex2fv(&coords[0]);
                else glVertex2fv(&coords[(i - 2) * 2]);

                glVertex2fv(&coords[(i - 1) * 2]);
                glVertex2fv(&coords[i * 2]);)
    }
    $GL3(
            glBindTexture(GL_TEXTURE_2D, g_whitetex);
            glBindVertexArray(g_vao);
            glBindBuffer(GL_ARRAY_BUFFER, g_vbos[0]);
            glBufferData(GL_ARRAY_BUFFER, vSize * sizeof(float), v, GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, g_vbos[1]);
            glBufferData(GL_ARRAY_BUFFER, uvSize * sizeof(float), uv, GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, g_vbos[2]);
            glBufferData(GL_ARRAY_BUFFER, cSize * sizeof(float), c, GL_STATIC_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, (numCoords * 2 + numCoords - 2) * 3);)
    $GL2(
            glEnd();)
}

static void drawRect(float x, float y, float w, float h, float fth, unsigned int col) {
    float verts[4 * 2] =
            {
                    x + 0.5f,
                    y + 0.5f,
                    x + w - 0.5f,
                    y + 0.5f,
                    x + w - 0.5f,
                    y + h - 0.5f,
                    x + 0.5f,
                    y + h - 0.5f,
            };
    drawPolygon(verts, 4, fth, col);
}

static void drawArc(float x, float y, float radius, float from, float to, float fth, unsigned int col) {
    GLfloat twicePi = 2.0f * M_PI;

    const unsigned n = /* ( to - from ) * */ unsigned(1.5 * CIRCLE_VERTS);
    const float *cverts = g_circleVerts;

    float outer = radius;
    float inner = radius - fth;
    float i = from;

    std::vector<float> verts;

    i = from;

    float inc = (to - from) / n;
    for (float i = from; i < to; i += inc) {
        verts.push_back(x + (outer * sin(i * twicePi)));
        verts.push_back(y + (outer * cos(i * twicePi)));
        verts.push_back(x + (inner * sin(i * twicePi)));
        verts.push_back(y + (inner * cos(i * twicePi)));
    }

    verts.push_back(x + (outer * sin(to * twicePi)));
    verts.push_back(y + (outer * cos(to * twicePi)));
    verts.push_back(x + (inner * sin(to * twicePi)));
    verts.push_back(y + (inner * cos(to * twicePi)));

    drawPolygon(verts.data(), verts.size() / 2, fth, col, false);
}


/*
static void drawEllipse(float x, float y, float w, float h, float fth, unsigned int col)
{
	float verts[CIRCLE_VERTS*2];
	const float* cverts = g_circleVerts;
	float* v = verts;

	for (int i = 0; i < CIRCLE_VERTS; ++i)
	{
		*v++ = x + cverts[i*2]*w;
		*v++ = y + cverts[i*2+1]*h;
	}

	drawPolygon(verts, CIRCLE_VERTS, fth, col);
}
*/

static void drawRoundedRect(float x, float y, float w, float h, float r, float fth, unsigned int col) {
    const unsigned n = CIRCLE_VERTS / 4;
    float verts[(n + 1) * 4 * 2];
    const float *cverts = g_circleVerts;
    float *v = verts;

    for (unsigned i = 0; i <= n; ++i) {
        *v++ = x + w - r + cverts[i * 2] * r;
        *v++ = y + h - r + cverts[i * 2 + 1] * r;
    }

    for (unsigned i = n; i <= n * 2; ++i) {
        *v++ = x + r + cverts[i * 2] * r;
        *v++ = y + h - r + cverts[i * 2 + 1] * r;
    }

    for (unsigned i = n * 2; i <= n * 3; ++i) {
        *v++ = x + r + cverts[i * 2] * r;
        *v++ = y + r + cverts[i * 2 + 1] * r;
    }

    for (unsigned i = n * 3; i < n * 4; ++i) {
        *v++ = x + w - r + cverts[i * 2] * r;
        *v++ = y + r + cverts[i * 2 + 1] * r;
    }
    *v++ = x + w - r + cverts[0] * r;
    *v++ = y + r + cverts[1] * r;

    drawPolygon(verts, (n + 1) * 4, fth, col);
}


static void drawLine(float x0, float y0, float x1, float y1, float r, float fth, unsigned int col) {
    float dx = x1 - x0;
    float dy = y1 - y0;
    float d = sqrtf(dx * dx + dy * dy);
    if (d > 0.0001f) {
        d = 1.0f / d;
        dx *= d;
        dy *= d;
    }
    float nx = dy;
    float ny = -dx;
    float verts[4 * 2];
    r -= fth;
    r *= 0.5f;
    if (r < 0.01f) r = 0.01f;
    dx *= r;
    dy *= r;
    nx *= r;
    ny *= r;

    verts[0] = x0 - dx - nx;
    verts[1] = y0 - dy - ny;

    verts[2] = x0 - dx + nx;
    verts[3] = y0 - dy + ny;

    verts[4] = x1 + dx + nx;
    verts[5] = y1 + dy + ny;

    verts[6] = x1 + dx - nx;
    verts[7] = y1 + dy - ny;

    drawPolygon(verts, 4, fth, col);
}

extern int (*imguiRenderTextLength)(const char *text);

static bool setText(int font) {
    bool ok = true;
    if (font <= 0 || font > 7) font = 1, ok = false;
    fonsSetFont(fs, fonts[font].face);
    fonsSetSize(fs, fonts[font].size);
    fonsSetAlign(fs, IMGUI_ALIGN_CENTER | IMGUI_ALIGN_BASELINE);
    return ok;
}

int imguiRenderGLFontGetWidth(const char *ftext) {
    float bounds[4];
    if (setText(ftext[0])) {
        fonsTextBounds(fs, &ftext[1], 0, bounds);
    } else {
        fonsTextBounds(fs, &ftext[0], 0, bounds);
    }
    return int(bounds[2] - bounds[0]);
}

bool imguiRenderGLInit() {
    imguiRenderTextLength = imguiRenderGLFontGetWidth;

    for (int i = 0; i < CIRCLE_VERTS; ++i) {
        float a = (float) i / (float) CIRCLE_VERTS * PI * 2;
        g_circleVerts[i * 2 + 0] = cosf(a);
        g_circleVerts[i * 2 + 1] = sinf(a);
    }

    // Create GL stash for 512x512 texture, our coordinate system has zero at top-left.
    fs = glfonsCreate(512, 512, FONS_ZERO_BOTTOMLEFT);

    $GL3(
            unsigned char white_alpha = 255;
            glGenTextures(1, &g_whitetex);
            glBindTexture(GL_TEXTURE_2D, g_whitetex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &white_alpha);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glGenVertexArrays(1, &g_vao);
            glGenBuffers(3, g_vbos);

            glBindVertexArray(g_vao);
            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);

            glBindBuffer(GL_ARRAY_BUFFER, g_vbos[0]);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 2, (void *) 0);
            glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, g_vbos[1]);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 2, (void *) 0);
            glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, g_vbos[2]);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 4, (void *) 0);
            glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW);
            g_program = glCreateProgram();

            const char *vs =
                    "#version 130\n"
                    "uniform vec2 Viewport;\n"
                    "in vec2 VertexPosition;\n"
                    "in vec2 VertexTexCoord;\n"
                    "in vec4 VertexColor;\n"
                    "out vec2 texCoord;\n"
                    "out vec4 vertexColor;\n"
                    "void main(void)\n"
                    "{\n"
                    "    vertexColor = VertexColor;\n"
                    "    texCoord = VertexTexCoord;\n"
                    "    gl_Position = vec4(VertexPosition * 2.0 / Viewport - 1.0, 0.f, 1.0);\n"
                    "}\n";
            GLuint vso = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vso, 1, (const char **) &vs, NULL);
            glCompileShader(vso);
            glAttachShader(g_program, vso);

            const char *fs =
                    "#version 130\n"
                    "in vec2 texCoord;\n"
                    "in vec4 vertexColor;\n"
                    "uniform sampler2D Texture;\n"
                    "out vec4  Color;\n"
                    "void main(void)\n"
                    "{\n"
                    "    float alpha = texture(Texture, texCoord).r;\n"
                    "    Color = vec4(vertexColor.rgb, vertexColor.a * alpha);\n"
                    "}\n";
            GLuint fso = glCreateShader(GL_FRAGMENT_SHADER);

            glShaderSource(fso, 1, (const char **) &fs, NULL);
            glCompileShader(fso);
            glAttachShader(g_program, fso);

            glBindAttribLocation(g_program, 0, "VertexPosition");
            glBindAttribLocation(g_program, 1, "VertexTexCoord");
            glBindAttribLocation(g_program, 2, "VertexColor");
            glBindFragDataLocation(g_program, 0, "Color");
            glLinkProgram(g_program);
            glDeleteShader(vso);
            glDeleteShader(fso);

            glUseProgram(g_program);
            g_programViewportLocation = glGetUniformLocation(g_program, "Viewport");
            g_programTextureLocation = glGetUniformLocation(g_program, "Texture");

            glUseProgram(0);)

    return true;
}

bool imguiRenderGLFontInit(int font, float pt, const char *fontpath) {
    FILE *fp = fopen(fontpath, "rb");
    if (!fp) return false;
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    unsigned char *ttfBuffer = (unsigned char *) malloc(size);
    if (!ttfBuffer) {
        fclose(fp);
        return false;
    }

    fread(ttfBuffer, 1, size, fp);
    fclose(fp);
    fp = 0;

    fonts[font].face = fonsAddFontMem(fs, "sans", (unsigned char *) (ttfBuffer), size, 1);
    fonts[font].size = pt;

    //free(ttfBuffer);
    return true;
}

bool imguiRenderGLFontInit(int font, float pt, const void *buf, unsigned size) {
    if (!buf)
        return false;
    if (!size)
        return false;

    // Add font to stash.
    fonts[font].face = fonsAddFontMem(fs, "sans", (unsigned char *) (buf), size, 0);
    fonts[font].size = pt;

    return true;
}

void imguiRenderGLDestroy() {
    if (g_ftex) {
        glDeleteTextures(1, &g_ftex);
        g_ftex = 0;
    }
    $GL3(
            if (g_vao) {
                glDeleteVertexArrays(1, &g_vao);
                glDeleteBuffers(3, g_vbos);
                g_vao = 0;
            }

            if (g_program) {
                glDeleteProgram(g_program);
                g_program = 0;
            })
}

#if 0
static void getBakedQuad(stbtt_bakedchar *chardata, int pw, int ph, int char_index,
						 float *xpos, float *ypos, stbtt_aligned_quad *q)
{
	stbtt_bakedchar *b = chardata + char_index;
	int round_x = STBTT_ifloor(*xpos + b->xoff);
	int round_y = STBTT_ifloor(*ypos - b->yoff);

	q->x0 = (float)round_x;
	q->y0 = (float)round_y;
	q->x1 = (float)round_x + b->x1 - b->x0;
	q->y1 = (float)round_y - b->y1 + b->y0;

	q->s0 = b->x0 / (float)pw;
	q->t0 = b->y0 / (float)pw;
	q->s1 = b->x1 / (float)ph;
	q->t1 = b->y1 / (float)ph;

	*xpos += b->xadvance;
}
#endif

static void drawText(float x, float y, const char *ftext, int align, unsigned int col) {
    /*$GL3(
	glUseProgram(0);
	)*/
    if (setText(ftext[0])) {
        fonsSetColor(fs, col);
        fonsSetAlign(fs, align);
        // void fonsSetSpacing(struct FONScontext* s, float spacing);
        // void fonsSetBlur(struct FONScontext* s, float blur);
        fonsDrawText(fs, x, y, &ftext[1], NULL);
    } else {
        fonsSetColor(fs, col);
        fonsSetAlign(fs, align);
        fonsDrawText(fs, x, y, &ftext[0], NULL);
    }
    /*$GL3(
	glUseProgram(g_program);
	)
	*/
}

void imguiRenderGLDraw(int width, int height) {
    const imguiGfxCmd *q = imguiGetRenderQueue();
    int nq = imguiGetRenderQueueSize();

    const float s = 1.0f / 8.0f;
    $GL3(
            glViewport(0, 0, width, height);
            glUseProgram(g_program);
            glActiveTexture(GL_TEXTURE0);
            glUniform2f(g_programViewportLocation, (float) width, (float) height);
            glUniform1i(g_programTextureLocation, 0);)

    glDisable(GL_SCISSOR_TEST);
    for (int i = 0; i < nq; ++i) {
        const imguiGfxCmd &cmd = q[i];
        if (cmd.type == IMGUI_GFXCMD_RECT) {
            if (cmd.rect.r == 0) {
                drawRect((float) cmd.rect.x * s + 0.5f, (float) cmd.rect.y * s + 0.5f,
                         (float) cmd.rect.w * s - 1, (float) cmd.rect.h * s - 1,
                         1.0f, cmd.col);
            } else {
                drawRoundedRect((float) cmd.rect.x * s + 0.5f, (float) cmd.rect.y * s + 0.5f,
                                (float) cmd.rect.w * s - 1, (float) cmd.rect.h * s - 1,
                                (float) cmd.rect.r * s, 1.0f, cmd.col);
            }
        } else if (cmd.type == IMGUI_GFXCMD_LINE) {
            drawLine(cmd.line.x0 * s, cmd.line.y0 * s, cmd.line.x1 * s, cmd.line.y1 * s, cmd.line.r * s, 1.0f, cmd.col);
        } else if (cmd.type == IMGUI_GFXCMD_TRIANGLE) {
            if (cmd.flags == 1) {
                const float verts[3 * 2] =
                        {
                                (float) cmd.rect.x * s + 0.5f,
                                (float) cmd.rect.y * s + 0.5f,
                                (float) cmd.rect.x * s + 0.5f + (float) cmd.rect.w * s - 1,
                                (float) cmd.rect.y * s + 0.5f + (float) cmd.rect.h * s / 2 - 0.5f,
                                (float) cmd.rect.x * s + 0.5f,
                                (float) cmd.rect.y * s + 0.5f + (float) cmd.rect.h * s - 1,
                        };
                drawPolygon(verts, 3, 1.0f, cmd.col);
            }
            if (cmd.flags == 2) {
                const float verts[3 * 2] =
                        {
                                (float) cmd.rect.x * s + 0.5f,
                                (float) cmd.rect.y * s + 0.5f + (float) cmd.rect.h * s - 1,
                                (float) cmd.rect.x * s + 0.5f + (float) cmd.rect.w * s / 2 - 0.5f,
                                (float) cmd.rect.y * s + 0.5f,
                                (float) cmd.rect.x * s + 0.5f + (float) cmd.rect.w * s - 1,
                                (float) cmd.rect.y * s + 0.5f + (float) cmd.rect.h * s - 1,
                        };
                drawPolygon(verts, 3, 1.0f, cmd.col);
            }
        } else if (cmd.type == IMGUI_GFXCMD_TEXT) {
            drawText(cmd.text.x, cmd.text.y, cmd.text.text, cmd.text.align, cmd.col);
        } else if (cmd.type == IMGUI_GFXCMD_SCISSOR) {
            if (cmd.flags) {
                glEnable(GL_SCISSOR_TEST);
                glScissor(cmd.rect.x, cmd.rect.y, cmd.rect.w, cmd.rect.h);
            } else {
                glDisable(GL_SCISSOR_TEST);
            }
        } else if (cmd.type == IMGUI_GFXCMD_ARC) {
            drawArc(cmd.arc.x, cmd.arc.y, cmd.arc.r, cmd.arc.t0, cmd.arc.t1, cmd.arc.w, cmd.col);
        } else if (cmd.type == IMGUI_GFXCMD_TEXTURE) {
            /*
				drawRect((float)cmd.rect.x*s+0.5f, (float)cmd.rect.y*s+0.5f,
						 (float)cmd.rect.w*s-1, (float)cmd.rect.h*s-1,
						 1.0f, cmd.col);
						 */
        }
    }
    glDisable(GL_SCISSOR_TEST);
}

unsigned imguiRenderGLMakeTexture(const void *data, unsigned size) {
    int w, h, n;
    unsigned char *rgba = stbi_load_from_memory((unsigned char *) data, size, &w, &h, &n, 4);
    if (!rgba)
        return 0;

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    // submit decoded data to texture

    GLint UnpackAlignment;

    // Here we bind the texture and set up the filtering.
    //glBindTexture(GL_TEXTURE_2D, id);

    // Set unpack alignment to one byte
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &UnpackAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLenum type = GL_RGBA;//GL_ALPHA, GL_LUMINANCE, GL_RGBA

#if 0//if buildmipmaps

                GLint ret = gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, imageWidth, imageHeight,
//                                                type, GL_UNSIGNED_BYTE, image.data());
                                                type, GL_UNSIGNED_BYTE, &image[0]);

#else// else...

    glTexImage2D(GL_TEXTURE_2D, 0 /*LOD*/, GL_RGBA, w, h,
                 0 /*border*/, GL_RGBA, GL_UNSIGNED_BYTE, data);

#endif

    // Restore old unpack alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, UnpackAlignment);

    stbi_image_free(rgba);
    return id;
}

unsigned imguiRenderGLMakeTexture(const char *imagepath) {
    FILE *fp = fopen(imagepath, "rb");
    if (!fp) {
        return 0;
    }
    fseek(fp, 0L, SEEK_END);
    unsigned size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    unsigned char *data = new unsigned char[size];
    fread(data, size, 1, fp);
    fclose(fp);
    unsigned id = imguiRenderGLMakeTexture(data, size);
    delete[] data;
    return id;
}
