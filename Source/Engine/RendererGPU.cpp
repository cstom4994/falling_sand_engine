// Copyright(c) 2022, KaoruXun All rights reserved.

#include "RendererGPU.h"
#include "Core/Core.hpp"
#include "Core/DebugImpl.hpp"
#include "Core/Global.hpp"
#include "Core/Macros.hpp"
#include "Engine/ImGuiImplement.hpp"
#include "Engine/Memory.hpp"
#include "Engine/SDLWrapper.hpp"
#include "Game/ImGuiCore.hpp"
#include "Game/InEngine.h"
#include "RendererGPU.h"

#include "external/stb_image.h"
#include "external/stb_image_write.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#define _METAENGINE_Render_GLT_TEXT2D_POSITION_LOCATION 0
#define _METAENGINE_Render_GLT_TEXT2D_TEXCOORD_LOCATION 1

#define _METAENGINE_Render_GLT_TEXT2D_POSITION_SIZE 2
#define _METAENGINE_Render_GLT_TEXT2D_TEXCOORD_SIZE 2
#define _METAENGINE_Render_GLT_TEXT2D_VERTEX_SIZE                                                  \
    (_METAENGINE_Render_GLT_TEXT2D_POSITION_SIZE + _METAENGINE_Render_GLT_TEXT2D_TEXCOORD_SIZE)

#define _METAENGINE_Render_GLT_TEXT2D_POSITION_OFFSET 0
#define _METAENGINE_Render_GLT_TEXT2D_TEXCOORD_OFFSET _METAENGINE_Render_GLT_TEXT2D_POSITION_SIZE

#define _METAENGINE_Render_GLT_MAT4_INDEX(row, column) ((row) + (column) *4)

static const char *_METAENGINE_Render_GLT_FontGlyphCharacters =
        " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-+/():;%&`*#=[]\"";
#define _METAENGINE_Render_GLT_FontGlyphCount 83

#define _METAENGINE_Render_GLT_FontGlyphMinChar ' '
#define _METAENGINE_Render_GLT_FontGlyphMaxChar 'z'

static const int _METAENGINE_Render_GLT_FontGlyphHeight = 17;

typedef struct _METAENGINE_Render_GLTglyph
{
    char c;

    GLint x, y;
    GLint w, h;

    GLfloat u1, v1;
    GLfloat u2, v2;

    GLboolean drawable;
} _METAENGINE_Render_GLTglyph;

typedef struct _METAENGINE_Render_GLTglyphdata
{
    uint32_t x, y;
    uint32_t w, h;

    uint32_t marginLeft, marginTop;
    uint32_t marginRight, marginBottom;

    uint16_t dataWidth, dataHeight;
} _METAENGINE_Render_GLTglyphdata;

static _METAENGINE_Render_GLTglyph
        _METAENGINE_Render_GLT_FontGlyphs[_METAENGINE_Render_GLT_FontGlyphCount];

#define _METAENGINE_Render_GLT_FontGlyphLength                                                     \
    (_METAENGINE_Render_GLT_FontGlyphMaxChar - _METAENGINE_Render_GLT_FontGlyphMinChar + 1)
static _METAENGINE_Render_GLTglyph
        _METAENGINE_Render_GLT_FontGlyphs2[_METAENGINE_Render_GLT_FontGlyphLength];

static GLuint _METAENGINE_Render_GLT_Text2DShader = METAENGINE_Render_GLT_NULL_HANDLE;
static GLuint _METAENGINE_Render_GLT_Text2DFontTexture = METAENGINE_Render_GLT_NULL_HANDLE;

static GLint _METAENGINE_Render_GLT_Text2DShaderMVPUniformLocation = -1;
static GLint _METAENGINE_Render_GLT_Text2DShaderColorUniformLocation = -1;

static GLfloat _METAENGINE_Render_GLT_Text2DProjectionMatrix[16];

static GLint _METAENGINE_Render_GLT_SwayShader = METAENGINE_Render_GLT_NULL_HANDLE;

struct METAENGINE_Render_GLTtext
{
    char *_text;
    GLsizei _textLength;

    GLboolean _dirty;

    GLsizei vertexCount;
    GLfloat *_vertices;

    GLuint _vao;
    GLuint _vbo;
};

void _METAENGINE_Render_GLT_GetViewportSize(GLint *width, GLint *height);

void _METAENGINE_Render_GLT_Mat4Mult(const GLfloat lhs[16], const GLfloat rhs[16],
                                     GLfloat result[16]);

void _METAENGINE_Render_GLT_UpdateBuffers(METAENGINE_Render_GLTtext *text);

GLboolean _METAENGINE_Render_GLT_CreateText2DShader(void);
GLboolean _METAENGINE_Render_GLT_CreateText2DFontTexture(void);

METAENGINE_Render_GLTtext *METAENGINE_Render_GLT_CreateText(void) {
    METAENGINE_Render_GLTtext *text =
            (METAENGINE_Render_GLTtext *) calloc(1, sizeof(METAENGINE_Render_GLTtext));

    METADOT_ASSERT_E(text);

    if (!text) return METAENGINE_Render_GLT_NULL;

    glGenVertexArrays(1, &text->_vao);
    glGenBuffers(1, &text->_vbo);

    METADOT_ASSERT_E(text->_vao);
    METADOT_ASSERT_E(text->_vbo);

    if (!text->_vao || !text->_vbo) {
        METAENGINE_Render_GLT_DeleteText(text);
        return METAENGINE_Render_GLT_NULL;
    }

    glBindVertexArray(text->_vao);

    glBindBuffer(GL_ARRAY_BUFFER, text->_vbo);

    glEnableVertexAttribArray(_METAENGINE_Render_GLT_TEXT2D_POSITION_LOCATION);
    glVertexAttribPointer(
            _METAENGINE_Render_GLT_TEXT2D_POSITION_LOCATION,
            _METAENGINE_Render_GLT_TEXT2D_POSITION_SIZE, GL_FLOAT, GL_FALSE,
            (_METAENGINE_Render_GLT_TEXT2D_VERTEX_SIZE * sizeof(GLfloat)),
            (const void *) (_METAENGINE_Render_GLT_TEXT2D_POSITION_OFFSET * sizeof(GLfloat)));

    glEnableVertexAttribArray(_METAENGINE_Render_GLT_TEXT2D_TEXCOORD_LOCATION);
    glVertexAttribPointer(
            _METAENGINE_Render_GLT_TEXT2D_TEXCOORD_LOCATION,
            _METAENGINE_Render_GLT_TEXT2D_TEXCOORD_SIZE, GL_FLOAT, GL_FALSE,
            (_METAENGINE_Render_GLT_TEXT2D_VERTEX_SIZE * sizeof(GLfloat)),
            (const void *) (_METAENGINE_Render_GLT_TEXT2D_TEXCOORD_OFFSET * sizeof(GLfloat)));

    glBindVertexArray(0);

    return text;
}

void METAENGINE_Render_GLT_DeleteText(METAENGINE_Render_GLTtext *text) {
    if (!text) return;

    if (text->_vao) {
        glDeleteVertexArrays(1, &text->_vao);
        text->_vao = METAENGINE_Render_GLT_NULL_HANDLE;
    }

    if (text->_vbo) {
        glDeleteBuffers(1, &text->_vbo);
        text->_vbo = METAENGINE_Render_GLT_NULL_HANDLE;
    }

    if (text->_text) free(text->_text);

    if (text->_vertices) free(text->_vertices);

    free(text);
}

bool METAENGINE_Render_GLT_SetText(METAENGINE_Render_GLTtext *text, const char *string) {
    if (!text) return 0;

    int strLength = 0;

    if (string) strLength = strlen(string);

    if (strLength) {
        if (text->_text) {
            if (strcmp(string, text->_text) == 0) return GL_TRUE;

            free(text->_text);
            text->_text = METAENGINE_Render_GLT_NULL;
        }

        text->_text = (char *) malloc((strLength + 1) * sizeof(char));

        if (text->_text) {
            memcpy(text->_text, string, (strLength + 1) * sizeof(char));

            text->_textLength = strLength;
            text->_dirty = GL_TRUE;

            return 1;
        }
    } else {
        if (text->_text) {
            free(text->_text);
            text->_text = METAENGINE_Render_GLT_NULL;
        } else {
            return 1;
        }

        text->_textLength = 0;
        text->_dirty = GL_TRUE;

        return 1;
    }

    return 0;
}

const char *METAENGINE_Render_GLT_GetText(METAENGINE_Render_GLTtext *text) {
    if (text && text->_text) return text->_text;

    return "\0";
}

void METAENGINE_Render_GLT_Viewport(GLsizei width, GLsizei height) {
    METADOT_ASSERT_E(width > 0);
    METADOT_ASSERT_E(height > 0);

    const GLfloat left = 0.0f;
    const GLfloat right = (GLfloat) width;
    const GLfloat bottom = (GLfloat) height;
    const GLfloat top = 0.0f;
    const GLfloat zNear = -1.0f;
    const GLfloat zFar = 1.0f;

    const GLfloat projection[16] = {
            (2.0f / (right - left)),
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            (2.0f / (top - bottom)),
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            (-2.0f / (zFar - zNear)),
            0.0f,

            -((right + left) / (right - left)),
            -((top + bottom) / (top - bottom)),
            -((zFar + zNear) / (zFar - zNear)),
            1.0f,
    };

    memcpy(_METAENGINE_Render_GLT_Text2DProjectionMatrix, projection, 16 * sizeof(GLfloat));
}

void METAENGINE_Render_GLT_BeginDraw() {
    glGetIntegerv(GL_CURRENT_PROGRAM, &_METAENGINE_Render_GLT_SwayShader);

    glUseProgram(_METAENGINE_Render_GLT_Text2DShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _METAENGINE_Render_GLT_Text2DFontTexture);
}

void METAENGINE_Render_GLT_EndDraw() {
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(_METAENGINE_Render_GLT_SwayShader);
}

#define _METAENGINE_Render_GLT_DrawText()                                                          \
    glUniformMatrix4fv(_METAENGINE_Render_GLT_Text2DShaderMVPUniformLocation, 1, GL_FALSE, mvp);   \
                                                                                                   \
    glBindVertexArray(text->_vao);                                                                 \
    glDrawArrays(GL_TRIANGLES, 0, text->vertexCount);

void METAENGINE_Render_GLT_DrawText(METAENGINE_Render_GLTtext *text, const GLfloat mvp[16]) {
    if (!text) return;

    if (text->_dirty) _METAENGINE_Render_GLT_UpdateBuffers(text);

    if (!text->vertexCount) return;

    _METAENGINE_Render_GLT_DrawText();
}

void METAENGINE_Render_GLT_DrawText2D(METAENGINE_Render_GLTtext *text, GLfloat x, GLfloat y,
                                      GLfloat scale) {
    if (!text) return;

    if (text->_dirty) _METAENGINE_Render_GLT_UpdateBuffers(text);

    if (!text->vertexCount) return;

#ifndef METAENGINE_Render_GLT_MANUAL_VIEWPORT
    GLint viewportWidth, viewportHeight;
    _METAENGINE_Render_GLT_GetViewportSize(&viewportWidth, &viewportHeight);
    METAENGINE_Render_GLT_Viewport(viewportWidth, viewportHeight);
#endif

    const GLfloat model[16] = {
            scale, 0.0f, 0.0f,  0.0f, 0.0f, scale, 0.0f, 0.0f,
            0.0f,  0.0f, scale, 0.0f, x,    y,     0.0f, 1.0f,
    };

    GLfloat mvp[16];
    _METAENGINE_Render_GLT_Mat4Mult(_METAENGINE_Render_GLT_Text2DProjectionMatrix, model, mvp);

    _METAENGINE_Render_GLT_DrawText();
}

void METAENGINE_Render_GLT_DrawText2DAligned(METAENGINE_Render_GLTtext *text, GLfloat x, GLfloat y,
                                             GLfloat scale, int horizontalAlignment,
                                             int verticalAlignment) {
    if (!text) return;

    if (text->_dirty) _METAENGINE_Render_GLT_UpdateBuffers(text);

    if (!text->vertexCount) return;

    if (horizontalAlignment == METAENGINE_Render_GLT_CENTER)
        x -= METAENGINE_Render_GLT_GetTextWidth(text, scale) * 0.5f;
    else if (horizontalAlignment == METAENGINE_Render_GLT_RIGHT)
        x -= METAENGINE_Render_GLT_GetTextWidth(text, scale);

    if (verticalAlignment == METAENGINE_Render_GLT_CENTER)
        y -= METAENGINE_Render_GLT_GetTextHeight(text, scale) * 0.5f;
    else if (verticalAlignment == METAENGINE_Render_GLT_RIGHT)
        y -= METAENGINE_Render_GLT_GetTextHeight(text, scale);

    METAENGINE_Render_GLT_DrawText2D(text, x, y, scale);
}

void METAENGINE_Render_GLT_DrawText3D(METAENGINE_Render_GLTtext *text, GLfloat x, GLfloat y,
                                      GLfloat z, GLfloat scale, GLfloat view[16],
                                      GLfloat projection[16]) {
    if (!text) return;

    if (text->_dirty) _METAENGINE_Render_GLT_UpdateBuffers(text);

    if (!text->vertexCount) return;

    const GLfloat model[16] = {
            scale, 0.0f,
            0.0f,  0.0f,
            0.0f,  -scale,
            0.0f,  0.0f,
            0.0f,  0.0f,
            scale, 0.0f,
            x,     y + (GLfloat) _METAENGINE_Render_GLT_FontGlyphHeight * scale,
            z,     1.0f,
    };

    GLfloat mvp[16];
    GLfloat vp[16];

    _METAENGINE_Render_GLT_Mat4Mult(projection, view, vp);
    _METAENGINE_Render_GLT_Mat4Mult(vp, model, mvp);

    _METAENGINE_Render_GLT_DrawText();
}

void METAENGINE_Render_GLT_Color(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    glUniform4f(_METAENGINE_Render_GLT_Text2DShaderColorUniformLocation, r, g, b, a);
}

void METAENGINE_Render_GLT_GetColor(GLfloat *r, GLfloat *g, GLfloat *b, GLfloat *a) {
    GLfloat color[4];
    glGetUniformfv(_METAENGINE_Render_GLT_Text2DShader,
                   _METAENGINE_Render_GLT_Text2DShaderColorUniformLocation, color);

    if (r) (*r) = color[0];
    if (g) (*g) = color[1];
    if (b) (*b) = color[2];
    if (a) (*a) = color[3];
}

GLfloat METAENGINE_Render_GLT_GetLineHeight(GLfloat scale) {
    return (GLfloat) _METAENGINE_Render_GLT_FontGlyphHeight * scale;
}

GLfloat METAENGINE_Render_GLT_GetTextWidth(const METAENGINE_Render_GLTtext *text, GLfloat scale) {
    if (!text || !text->_text) return 0.0f;

    GLfloat maxWidth = 0.0f;
    GLfloat width = 0.0f;

    _METAENGINE_Render_GLTglyph glyph;

    char c;
    int i;
    for (i = 0; i < text->_textLength; i++) {
        c = text->_text[i];

        if ((c == '\n') || (c == '\r')) {
            if (width > maxWidth) maxWidth = width;

            width = 0.0f;

            continue;
        }

        if (!METAENGINE_Render_GLT_IsCharacterSupported(c)) {
#ifdef METAENGINE_Render_GLT_UNKNOWN_CHARACTER
            c = METAENGINE_Render_GLT_UNKNOWN_CHARACTER;
            if (!METAENGINE_Render_GLT_IsCharacterSupported(c)) continue;
#else
            continue;
#endif
        }

        glyph = _METAENGINE_Render_GLT_FontGlyphs2[c - _METAENGINE_Render_GLT_FontGlyphMinChar];

        width += (GLfloat) glyph.w;
    }

    if (width > maxWidth) maxWidth = width;

    return maxWidth * scale;
}

GLfloat METAENGINE_Render_GLT_GetTextHeight(const METAENGINE_Render_GLTtext *text, GLfloat scale) {
    if (!text || !text->_text) return 0.0f;

    return (GLfloat) (METAENGINE_Render_GLT_CountNewLines(text->_text) + 1) *
           METAENGINE_Render_GLT_GetLineHeight(scale);
}

GLboolean METAENGINE_Render_GLT_IsCharacterSupported(const char c) {
    if (c == '\t') return GL_TRUE;
    if (c == '\n') return GL_TRUE;
    if (c == '\r') return GL_TRUE;

    int i;
    for (i = 0; i < _METAENGINE_Render_GLT_FontGlyphCount; i++) {
        if (_METAENGINE_Render_GLT_FontGlyphCharacters[i] == c) return GL_TRUE;
    }

    return GL_FALSE;
}

GLint METAENGINE_Render_GLT_CountSupportedCharacters(const char *str) {
    if (!str) return 0;

    GLint count = 0;

    while ((*str) != '\0') {
        if (METAENGINE_Render_GLT_IsCharacterSupported(*str)) count++;

        str++;
    }

    return count;
}

GLboolean METAENGINE_Render_GLT_IsCharacterDrawable(const char c) {
    if (c < _METAENGINE_Render_GLT_FontGlyphMinChar) return GL_FALSE;
    if (c > _METAENGINE_Render_GLT_FontGlyphMaxChar) return GL_FALSE;

    if (_METAENGINE_Render_GLT_FontGlyphs2[c - _METAENGINE_Render_GLT_FontGlyphMinChar].drawable)
        return GL_TRUE;

    return GL_FALSE;
}

GLint METAENGINE_Render_GLT_CountDrawableCharacters(const char *str) {
    if (!str) return 0;

    GLint count = 0;

    while ((*str) != '\0') {
        if (METAENGINE_Render_GLT_IsCharacterDrawable(*str)) count++;

        str++;
    }

    return count;
}

GLint METAENGINE_Render_GLT_CountNewLines(const char *str) {
    GLint count = 0;

    while ((str = strchr(str, '\n')) != NULL) {
        count++;
        str++;
    }

    return count;
}

void _METAENGINE_Render_GLT_GetViewportSize(GLint *width, GLint *height) {
    GLint dimensions[4];
    glGetIntegerv(GL_VIEWPORT, dimensions);

    if (width) (*width) = dimensions[2];
    if (height) (*height) = dimensions[3];
}

void _METAENGINE_Render_GLT_Mat4Mult(const GLfloat lhs[16], const GLfloat rhs[16],
                                     GLfloat result[16]) {
    int c, r, i;

    for (c = 0; c < 4; c++) {
        for (r = 0; r < 4; r++) {
            result[_METAENGINE_Render_GLT_MAT4_INDEX(r, c)] = 0.0f;

            for (i = 0; i < 4; i++)
                result[_METAENGINE_Render_GLT_MAT4_INDEX(r, c)] +=
                        lhs[_METAENGINE_Render_GLT_MAT4_INDEX(r, i)] *
                        rhs[_METAENGINE_Render_GLT_MAT4_INDEX(i, c)];
        }
    }
}

void _METAENGINE_Render_GLT_UpdateBuffers(METAENGINE_Render_GLTtext *text) {
    if (!text || !text->_dirty) return;

    if (text->_vertices) {
        text->vertexCount = 0;

        free(text->_vertices);
        text->_vertices = METAENGINE_Render_GLT_NULL;
    }

    if (!text->_text || !text->_textLength) {
        text->_dirty = GL_FALSE;
        return;
    }

    const GLsizei countDrawable = METAENGINE_Render_GLT_CountDrawableCharacters(text->_text);

    if (!countDrawable) {
        text->_dirty = GL_FALSE;
        return;
    }

    const GLsizei vertexCount =
            countDrawable * 2 * 3;// 3 vertices in a triangle, 2 triangles in a quad

    const GLsizei vertexSize = _METAENGINE_Render_GLT_TEXT2D_VERTEX_SIZE;
    GLfloat *vertices = (GLfloat *) malloc(vertexCount * vertexSize * sizeof(GLfloat));

    if (!vertices) return;

    GLsizei vertexElementIndex = 0;

    GLfloat glyphX = 0.0f;
    GLfloat glyphY = 0.0f;

    GLfloat glyphWidth;
    const GLfloat glyphHeight = (GLfloat) _METAENGINE_Render_GLT_FontGlyphHeight;

    const GLfloat glyphAdvanceX = 0.0f;
    const GLfloat glyphAdvanceY = 0.0f;

    _METAENGINE_Render_GLTglyph glyph;

    char c;
    int i;
    for (i = 0; i < text->_textLength; i++) {
        c = text->_text[i];

        if (c == '\n') {
            glyphX = 0.0f;
            glyphY += glyphHeight + glyphAdvanceY;

            continue;
        } else if (c == '\r') {
            glyphX = 0.0f;

            continue;
        }

        if (!METAENGINE_Render_GLT_IsCharacterSupported(c)) {
#ifdef METAENGINE_Render_GLT_UNKNOWN_CHARACTER
            c = METAENGINE_Render_GLT_UNKNOWN_CHARACTER;
            if (!METAENGINE_Render_GLT_IsCharacterSupported(c)) continue;
#else
            continue;
#endif
        }

        glyph = _METAENGINE_Render_GLT_FontGlyphs2[c - _METAENGINE_Render_GLT_FontGlyphMinChar];

        glyphWidth = (GLfloat) glyph.w;

        if (glyph.drawable) {
            vertices[vertexElementIndex++] = glyphX;
            vertices[vertexElementIndex++] = glyphY;
            vertices[vertexElementIndex++] = glyph.u1;
            vertices[vertexElementIndex++] = glyph.v1;

            vertices[vertexElementIndex++] = glyphX + glyphWidth;
            vertices[vertexElementIndex++] = glyphY + glyphHeight;
            vertices[vertexElementIndex++] = glyph.u2;
            vertices[vertexElementIndex++] = glyph.v2;

            vertices[vertexElementIndex++] = glyphX + glyphWidth;
            vertices[vertexElementIndex++] = glyphY;
            vertices[vertexElementIndex++] = glyph.u2;
            vertices[vertexElementIndex++] = glyph.v1;

            vertices[vertexElementIndex++] = glyphX;
            vertices[vertexElementIndex++] = glyphY;
            vertices[vertexElementIndex++] = glyph.u1;
            vertices[vertexElementIndex++] = glyph.v1;

            vertices[vertexElementIndex++] = glyphX;
            vertices[vertexElementIndex++] = glyphY + glyphHeight;
            vertices[vertexElementIndex++] = glyph.u1;
            vertices[vertexElementIndex++] = glyph.v2;

            vertices[vertexElementIndex++] = glyphX + glyphWidth;
            vertices[vertexElementIndex++] = glyphY + glyphHeight;
            vertices[vertexElementIndex++] = glyph.u2;
            vertices[vertexElementIndex++] = glyph.v2;
        }

        glyphX += glyphWidth + glyphAdvanceX;
    }

    text->vertexCount = vertexCount;
    text->_vertices = vertices;

    glBindBuffer(GL_ARRAY_BUFFER, text->_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertexCount * _METAENGINE_Render_GLT_TEXT2D_VERTEX_SIZE * sizeof(GLfloat),
                 vertices, GL_DYNAMIC_DRAW);

    text->_dirty = GL_FALSE;
}

GLboolean METAENGINE_Render_GLT_Init(void) {
    if (METAENGINE_Render_GLT_Initialized) return GL_TRUE;

    if (!_METAENGINE_Render_GLT_CreateText2DShader()) return GL_FALSE;

    if (!_METAENGINE_Render_GLT_CreateText2DFontTexture()) return GL_FALSE;

    METAENGINE_Render_GLT_Initialized = GL_TRUE;
    return GL_TRUE;
}

void METAENGINE_Render_GLT_Terminate(void) {
    if (_METAENGINE_Render_GLT_Text2DShader != METAENGINE_Render_GLT_NULL_HANDLE) {
        glDeleteProgram(_METAENGINE_Render_GLT_Text2DShader);
        _METAENGINE_Render_GLT_Text2DShader = METAENGINE_Render_GLT_NULL_HANDLE;
    }

    if (_METAENGINE_Render_GLT_Text2DFontTexture != METAENGINE_Render_GLT_NULL_HANDLE) {
        glDeleteTextures(1, &_METAENGINE_Render_GLT_Text2DFontTexture);
        _METAENGINE_Render_GLT_Text2DFontTexture = METAENGINE_Render_GLT_NULL_HANDLE;
    }

    METAENGINE_Render_GLT_Initialized = GL_FALSE;
}

static const GLchar *_METAENGINE_Render_GLT_Text2DVertexShaderSource =
        "#version 330 core\n"
        "\n"
        "in vec2 position;\n"
        "in vec2 texCoord;\n"
        "\n"
        "uniform mat4 mvp;\n"
        "\n"
        "out vec2 fTexCoord;\n"
        "\n"
        "void main()\n"
        "{\n"
        "	fTexCoord = texCoord;\n"
        "	\n"
        "	gl_Position = mvp * vec4(position, 0.0, 1.0);\n"
        "}\n";

static const GLchar *_METAENGINE_Render_GLT_Text2DFragmentShaderSource =
        "#version 330 core\n"
        "\n"
        "out vec4 fragColor;\n"
        "\n"
        "uniform sampler2D diffuse;\n"
        "\n"
        "uniform vec4 color = vec4(1.0, 1.0, 1.0, 1.0);\n"
        "\n"
        "in vec2 fTexCoord;\n"
        "\n"
        "void main()\n"
        "{\n"
        "	fragColor = texture(diffuse, fTexCoord) * color;\n"
        "}\n";

GLboolean _METAENGINE_Render_GLT_CreateText2DShader(void) {
    GLuint vertexShader, fragmentShader;
    GLint compileStatus, linkStatus;

#ifdef METAENGINE_Render_GLT_DEBUG_PRINT
    GLint infoLogLength;
    GLsizei infoLogSize;
    GLchar *infoLog;
#endif

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &_METAENGINE_Render_GLT_Text2DVertexShaderSource, NULL);
    glCompileShader(vertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compileStatus);

    if (compileStatus != GL_TRUE) {
#ifdef METAENGINE_Render_GLT_DEBUG_PRINT
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &infoLogLength);

        // If no information log exists 0 is returned or 1 for a
        // string only containing the null termination char.
        if (infoLogLength > 1) {
            infoLogSize = infoLogLength * sizeof(GLchar);
            infoLog = (GLchar *) malloc(infoLogSize);

            glGetShaderInfoLog(vertexShader, infoLogSize, NULL, infoLog);

            printf("Vertex Shader #%u <Info Log>:\n%s\n", vertexShader, infoLog);

            free(infoLog);
        }
#endif

        glDeleteShader(vertexShader);
        METAENGINE_Render_GLT_Terminate();

#ifdef METAENGINE_Render_GLT_DEBUG
        METADOT_ASSERT_E(compileStatus == GL_TRUE);
        return GL_FALSE;
#else
        return GL_FALSE;
#endif
    }

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &_METAENGINE_Render_GLT_Text2DFragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compileStatus);

    if (compileStatus != GL_TRUE) {
#ifdef METAENGINE_Render_GLT_DEBUG_PRINT
        glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &infoLogLength);

        // If no information log exists 0 is returned or 1 for a
        // string only containing the null termination char.
        if (infoLogLength > 1) {
            infoLogSize = infoLogLength * sizeof(GLchar);
            infoLog = (GLchar *) malloc(infoLogSize);

            glGetShaderInfoLog(fragmentShader, infoLogSize, NULL, infoLog);

            printf("Fragment Shader #%u <Info Log>:\n%s\n", fragmentShader, infoLog);

            free(infoLog);
        }
#endif

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        METAENGINE_Render_GLT_Terminate();

#ifdef METAENGINE_Render_GLT_DEBUG
        METADOT_ASSERT_E(compileStatus == GL_TRUE);
        return GL_FALSE;
#else
        return GL_FALSE;
#endif
    }

    _METAENGINE_Render_GLT_Text2DShader = glCreateProgram();

    glAttachShader(_METAENGINE_Render_GLT_Text2DShader, vertexShader);
    glAttachShader(_METAENGINE_Render_GLT_Text2DShader, fragmentShader);

    glBindAttribLocation(_METAENGINE_Render_GLT_Text2DShader,
                         _METAENGINE_Render_GLT_TEXT2D_POSITION_LOCATION, "position");
    glBindAttribLocation(_METAENGINE_Render_GLT_Text2DShader,
                         _METAENGINE_Render_GLT_TEXT2D_TEXCOORD_LOCATION, "texCoord");

    glBindFragDataLocation(_METAENGINE_Render_GLT_Text2DShader, 0, "fragColor");

    glLinkProgram(_METAENGINE_Render_GLT_Text2DShader);

    glDetachShader(_METAENGINE_Render_GLT_Text2DShader, vertexShader);
    glDeleteShader(vertexShader);

    glDetachShader(_METAENGINE_Render_GLT_Text2DShader, fragmentShader);
    glDeleteShader(fragmentShader);

    glGetProgramiv(_METAENGINE_Render_GLT_Text2DShader, GL_LINK_STATUS, &linkStatus);

    if (linkStatus != GL_TRUE) {
#ifdef METAENGINE_Render_GLT_DEBUG_PRINT
        glGetProgramiv(_METAENGINE_Render_GLT_Text2DShader, GL_INFO_LOG_LENGTH, &infoLogLength);

        // If no information log exists 0 is returned or 1 for a
        // string only containing the null termination char.
        if (infoLogLength > 1) {
            infoLogSize = infoLogLength * sizeof(GLchar);
            infoLog = (GLchar *) malloc(infoLogSize);

            glGetProgramInfoLog(_METAENGINE_Render_GLT_Text2DShader, infoLogSize, NULL, infoLog);

            printf("Program #%u <Info Log>:\n%s\n", _METAENGINE_Render_GLT_Text2DShader, infoLog);

            free(infoLog);
        }
#endif

        METAENGINE_Render_GLT_Terminate();

#ifdef METAENGINE_Render_GLT_DEBUG
        METADOT_ASSERT_E(linkStatus == GL_TRUE);
        return GL_FALSE;
#else
        return GL_FALSE;
#endif
    }

    glUseProgram(_METAENGINE_Render_GLT_Text2DShader);

    _METAENGINE_Render_GLT_Text2DShaderMVPUniformLocation =
            glGetUniformLocation(_METAENGINE_Render_GLT_Text2DShader, "mvp");
    _METAENGINE_Render_GLT_Text2DShaderColorUniformLocation =
            glGetUniformLocation(_METAENGINE_Render_GLT_Text2DShader, "color");

    glUniform1i(glGetUniformLocation(_METAENGINE_Render_GLT_Text2DShader, "diffuse"), 0);

    glUseProgram(0);

    return GL_TRUE;
}

static const uint64_t _METAENGINE_Render_GLT_FontGlyphRects[_METAENGINE_Render_GLT_FontGlyphCount] =
        {
                0x1100040000, 0x304090004, 0x30209000D, 0x304090016, 0x30209001F, 0x304090028,
                0x302090031,  0x409003A,   0x302090043, 0x30109004C, 0x1080055,   0x30209005D,
                0x302090066,  0x3040A006F, 0x304090079, 0x304090082, 0x409008B,   0x4090094,
                0x30409009D,  0x3040900A6, 0x3020900AF, 0x3040900B8, 0x3040900C1, 0x3040A00CA,
                0x3040900D4,  0x40A00DD,   0x3040900E7, 0x3020900F0, 0x3020900F9, 0x302090102,
                0x30209010B,  0x302090114, 0x30209011D, 0x302090126, 0x30209012F, 0x302070138,
                0x30209013F,  0x302090148, 0x302090151, 0x3020A015A, 0x3020A0164, 0x30209016E,
                0x302090177,  0x102090180, 0x302090189, 0x302090192, 0x30209019B, 0x3020901A4,
                0x3020901AD,  0x3020A01B6, 0x3020901C0, 0x3020901C9, 0x3020901D2, 0x3020901DB,
                0x3020901E4,  0x3020901ED, 0x3020901F6, 0x3020A01FF, 0x302090209, 0x302090212,
                0x30209021B,  0x302090224, 0x30209022D, 0x309060236, 0x10906023C, 0x302070242,
                0x302090249,  0x706090252, 0x50409025B, 0x202090264, 0x10207026D, 0x102070274,
                0x30406027B,  0x104060281, 0x2010B0287, 0x3020A0292, 0xB0007029C, 0x5040A02AA,
                0x3020A02B4,  0x6050902BE, 0x20702C7,   0x20702CE,   0xB010902D5,
};

#define _METAENGINE_Render_GLT_FONT_GLYPH_DATA_TYPE uint64_t
#define _METAENGINE_Render_GLT_FONT_PIXEL_SIZE_BITS 2

#define _METAENGINE_Render_GLT_FontGlyphDataCount 387

static const _METAENGINE_Render_GLT_FONT_GLYPH_DATA_TYPE
        _METAENGINE_Render_GLT_FontGlyphData[_METAENGINE_Render_GLT_FontGlyphDataCount] = {
                0x551695416A901554, 0x569695A5A56AA55A, 0x555554155545AA9,  0x916AA41569005A40,
                0xA5A569695A5A5696, 0x51555556AA569695, 0x696916A941554155, 0x69155A55569555A5,
                0x15541555456A9569, 0xA9569545A4005500, 0x569695A5A569695A, 0x5545AA9569695A5A,
                0x916A941554555415, 0x55A56AA95A5A5696, 0x40555416A9555695, 0x55A45AA505550155,
                0xA55AAA455A555691, 0x169005A45569155,  0xA945554015400554, 0x569695A5A569695A,
                0x9545AA9569695A5A, 0x4555555AA95A5556, 0x55A4016900154555, 0xA569695A5A45AA90,
                0x69695A5A569695A5, 0x9001541555455555, 0x5AA4155505A4016,  0xA40169005A501695,
                0x155555AAA4569505, 0x5A405A4015405555, 0x5A505A545AA45554, 0x5A405A405A405A40,
                0x555556A95A555A40, 0x569005A400551554, 0x9569A569695A5A45, 0xA5A56969169A556A,
                0xA405555555155555, 0x169005A50169505A, 0x9505A40169005A40, 0x5555155555AAA456,
                0x95A66916AA905555, 0x695A6695A6695A66, 0x154555555A5695A6, 0x5696916AA4155555,
                0x9695A5A569695A5A, 0x45555155555A5A56, 0xA5A5696916A94155, 0x9569695A5A569695,
                0x155515541555456A, 0x695A5A5696916AA4, 0x56AA569695A5A569, 0x5540169155A55569,
                0x56AA515550015400, 0x9695A5A569695A5A, 0x5A5516AA55A5A56,  0x500155005A401695,
                0xA56A695A5A455555, 0x169015A55569556,  0x54005500155005A4, 0x555A555695AA9455,
                0xAA569555A5505AA5, 0x15415551555556,   0x5A55AAA455A50169, 0x40169005A4556915,
                0x550155505AA5055A, 0xA569695A5A455555, 0x69695A5A569695A5, 0x555554155545AA95,
                0x5A5A569695A5A455, 0xA515AA55A5A56969, 0x1545505500555055, 0x95A6695A6695A569,
                0xA4569A55A6695A66, 0x5551555015554569, 0x456A9569695A5A45, 0xA5A5696916A95569,
                0x4155545555155555, 0xA45A5A45A5A45A5A, 0xA945A5A45A5A45A5, 0x56A915A555695056,
                0x4555501554055551, 0x6945695169555AAA, 0x55AAA45569156955, 0x5A50055055551555,
                0xA569695A5A45AA50, 0x69695A5A56AA95A5, 0x555555155555A5A5, 0x5A5A5696916AA415,
                0x5A5696956AA56969, 0x1555556AA569695A, 0x96916A9415541555, 0x9555A555695A5A56,
                0x6A9569695A5A4556, 0xA405551554155545, 0x69695A5A45A6905A, 0x695A5A569695A5A5,
                0x5550555555AA55A,  0x5A555695AAA45555, 0x4556916AA4156955, 0x5555AAA45569155A,
                0x95AAA45555555515, 0x6AA41569555A5556, 0xA40169155A455691, 0x1554005500155005,
                0x695A5A5696916A94, 0x5A5A56A69555A555, 0x54155545AA956969, 0x569695A5A4555555,
                0x9695AAA569695A5A, 0x55A5A569695A5A56, 0x55AA455555551555, 0x5A416905A456915A,
                0x515555AA45A51690, 0x169005A400550055, 0x9555A40169005A40, 0x456A9569695A5A56,
                0xA5A4555515541555, 0xA55A69569A569695, 0x6969169A45A6915A, 0x555555155555A5A5,
                0x5A40169005A400,   0x5A40169005A40169, 0x155555AAA4556900, 0x695A569154555555,
                0x6695A6695A9A95A5, 0xA5695A5695A6695A, 0x55154555555A5695, 0x95A5695A56915455,
                0x695AA695A6A95A5A, 0x5695A5695A5695A9, 0x155455154555555A, 0x695A5A5696916A94,
                0x5A5A569695A5A569, 0x541555456A956969, 0x5696916AA4155515, 0x56956AA569695A5A,
                0x5005A40169155A55, 0x6A94155400550015, 0xA569695A5A569691, 0x69695A5A569695A5,
                0x5A5415A5456A95,   0x16AA415555500155, 0xAA569695A5A56969, 0x569695A5A55A6956,
                0x5545555155555A5A, 0x5555A5696916A941, 0xA5545A5005A5155A, 0x41555456A9569695,
                0x56955AAA45555155, 0x9005A40169055A51, 0x5A40169005A4016,  0x5A45555055001550,
                0x569695A5A569695A, 0x9695A5A569695A5A, 0x515541555456A956, 0xA5A569695A5A4555,
                0xA569695A5A569695, 0x555055A515AA55A5, 0x95A5691545505500, 0x695A6695A5695A56,
                0x9A4569A55A6695A6, 0x555015554169A456, 0x9569695A5A455551, 0x5A6515A515694566,
                0x555A5A569695A5A4, 0x5A5A455555555155, 0xA9569695A5A56969, 0x169015A41569456,
                0x55505500155005A4, 0x5A55169555AAA45,  0x55A455A555A515A5, 0x5155555AAA455690,
                0x696916A941554555, 0xA95A5A56A695A9A5, 0x56A9569695A6A569, 0x9401540155415554,
                0x5A5516AA45A9516,  0xA40169005A401695, 0x4154005540169005, 0xA5A5696916A94155,
                0x9556945695169555, 0x55555AAA45569156, 0x6916A94155455551, 0x56A5169555A5A569,
                0xA9569695A5A56955, 0x15415541555456,   0x4169A4055A4005A4, 0xA916969169A5169A,
                0x50056954569555AA, 0x5AAA455551540015, 0xAA41569555A55569, 0x55A555A551695516,
                0x55005550555555AA, 0x915694569416A401, 0xA5A569695A5A45AA, 0x41555456A9569695,
                0x69555AAA45555155, 0x9415A415A5056951, 0x169015A40569056,  0xA941554015400554,
                0x569A95A5A5696916, 0x9695A5A56A6956A9, 0x415541555456A956, 0xA5A5696916A94155,
                0x516AA55A5A569695, 0x155415A915694569, 0x555A95A915505540, 0x5A55A95A91555545,
                0x1694154154555569, 0xA456956A95AA56A9, 0x55416905A415515,  0x696916A941554154,
                0x9055A515A555A5A5, 0x5A4016900554056,  0xAA45555055001550, 0x5505555155555A,
                0x6955AAA4569505A4, 0x5500155055A515,   0x690169405A400550, 0x90569415A415A505,
                0x569015A415A5056,  0x6941540015400554, 0xA456915A55A55691, 0x16905A505A416905,
                0x6901555405541694, 0x16905A505A416941, 0x6955A45A516905A4, 0xA455415415545695,
                0x6A45555515556A56, 0x56A45555515556A5, 0xA56A45555515556A, 0x5505515555A56956,
                0x690569A4016A5001, 0x4056954169A9459A, 0x416A690156941569, 0x15A9505A695169A6,
                0x4015505540055540, 0x94169A4169A405A9, 0x5A56A9A4555A415A, 0x555169A955A5A55A,
                0x6945A90555555415, 0x1555154055416941, 0x56AAA456A545A690, 0x40555515A69156A5,
                0x6945A69015550555, 0xA6915A6956AAA45A, 0x5A6956AAA45A6955, 0x455540555515A691,
                0xAA9555556AA91555, 0xA915555554555556, 0x416905A556955A56, 0x5A416905A416905A,
                0x555515555AA45690, 0x6905A516955AA455, 0x416905A416905A41, 0x55556A95A556905A,
                0xA5A5696915555554, 0x5555155555,
};

GLboolean _METAENGINE_Render_GLT_CreateText2DFontTexture(void) {
    if (METAENGINE_Render_GLT_Initialized) return GL_TRUE;

    memset(_METAENGINE_Render_GLT_FontGlyphs, 0,
           _METAENGINE_Render_GLT_FontGlyphCount * sizeof(_METAENGINE_Render_GLTglyph));
    memset(_METAENGINE_Render_GLT_FontGlyphs2, 0,
           _METAENGINE_Render_GLT_FontGlyphLength * sizeof(_METAENGINE_Render_GLTglyph));

    GLsizei texWidth = 0;
    GLsizei texHeight = 0;

    GLsizei drawableGlyphCount = 0;

    _METAENGINE_Render_GLTglyphdata *glyphsData = (_METAENGINE_Render_GLTglyphdata *) calloc(
            _METAENGINE_Render_GLT_FontGlyphCount, sizeof(_METAENGINE_Render_GLTglyphdata));

    uint64_t glyphPacked;
    uint32_t glyphMarginPacked;

    uint16_t glyphX, glyphY, glyphWidth, glyphHeight;
    uint16_t glyphMarginLeft, glyphMarginTop, glyphMarginRight, glyphMarginBottom;

    uint16_t glyphDataWidth, glyphDataHeight;

    glyphMarginLeft = 0;
    glyphMarginRight = 0;

    _METAENGINE_Render_GLTglyph *glyph;
    _METAENGINE_Render_GLTglyphdata *glyphData;

    char c;
    int i;
    int x, y;

    for (i = 0; i < _METAENGINE_Render_GLT_FontGlyphCount; i++) {
        c = _METAENGINE_Render_GLT_FontGlyphCharacters[i];

        glyphPacked = _METAENGINE_Render_GLT_FontGlyphRects[i];

        glyphX = (glyphPacked >> (uint64_t) (8 * 0)) & 0xFFFF;
        glyphWidth = (glyphPacked >> (uint64_t) (8 * 2)) & 0xFF;

        glyphY = 0;
        glyphHeight = _METAENGINE_Render_GLT_FontGlyphHeight;

        glyphMarginPacked = (glyphPacked >> (uint64_t) (8 * 3)) & 0xFFFF;

        glyphMarginTop = (glyphMarginPacked >> (uint16_t) (0)) & 0xFF;
        glyphMarginBottom = (glyphMarginPacked >> (uint16_t) (8)) & 0xFF;

        glyphDataWidth = glyphWidth;
        glyphDataHeight = glyphHeight - (glyphMarginTop + glyphMarginBottom);

        glyph = &_METAENGINE_Render_GLT_FontGlyphs[i];

        glyph->c = c;

        glyph->x = glyphX;
        glyph->y = glyphY;
        glyph->w = glyphWidth;
        glyph->h = glyphHeight;

        glyphData = &glyphsData[i];

        glyphData->x = glyphX;
        glyphData->y = glyphY;
        glyphData->w = glyphWidth;
        glyphData->h = glyphHeight;

        glyphData->marginLeft = glyphMarginLeft;
        glyphData->marginTop = glyphMarginTop;
        glyphData->marginRight = glyphMarginRight;
        glyphData->marginBottom = glyphMarginBottom;

        glyphData->dataWidth = glyphDataWidth;
        glyphData->dataHeight = glyphDataHeight;

        glyph->drawable = GL_FALSE;

        if ((glyphDataWidth > 0) && (glyphDataHeight > 0)) glyph->drawable = GL_TRUE;

        if (glyph->drawable) {
            drawableGlyphCount++;

            texWidth += (GLsizei) glyphWidth;

            if (texHeight < glyphHeight) texHeight = glyphHeight;
        }
    }

    const GLsizei textureGlyphPadding = 1;// amount of pixels added around the whole bitmap texture
    const GLsizei textureGlyphSpacing =
            1;// amount of pixels added between each glyph on the final bitmap texture

    texWidth += textureGlyphSpacing * (drawableGlyphCount - 1);

    texWidth += textureGlyphPadding * 2;
    texHeight += textureGlyphPadding * 2;

    const GLsizei texAreaSize = texWidth * texHeight;

    const GLsizei texPixelComponents = 4;// R, G, B, A
    GLubyte *texData = (GLubyte *) malloc(texAreaSize * texPixelComponents * sizeof(GLubyte));

    GLsizei texPixelIndex;

    for (texPixelIndex = 0; texPixelIndex < (texAreaSize * texPixelComponents); texPixelIndex++)
        texData[texPixelIndex] = 0;

#define _METAENGINE_Render_GLT_TEX_PIXEL_INDEX(x, y)                                               \
    ((y) *texWidth * texPixelComponents + (x) *texPixelComponents)

#define _METAENGINE_Render_GLT_TEX_SET_PIXEL(x, y, r, g, b, a)                                     \
    {                                                                                              \
        texPixelIndex = _METAENGINE_Render_GLT_TEX_PIXEL_INDEX(x, y);                              \
        texData[texPixelIndex + 0] = r;                                                            \
        texData[texPixelIndex + 1] = g;                                                            \
        texData[texPixelIndex + 2] = b;                                                            \
        texData[texPixelIndex + 3] = a;                                                            \
    }

    const int glyphDataTypeSizeBits =
            sizeof(_METAENGINE_Render_GLT_FONT_GLYPH_DATA_TYPE) * 8;// 8 bits in a byte

    int data0Index = 0;
    int data1Index = 0;

    int bit0Index = 0;
    int bit1Index = 1;

    char c0 = '0';
    char c1 = '0';

    GLuint r, g, b, a;

    GLfloat u1, v1;
    GLfloat u2, v2;

    GLsizei texX = 0;
    GLsizei texY = 0;

    texX += textureGlyphPadding;

    for (i = 0; i < _METAENGINE_Render_GLT_FontGlyphCount; i++) {
        glyph = &_METAENGINE_Render_GLT_FontGlyphs[i];
        glyphData = &glyphsData[i];

        if (!glyph->drawable) continue;

        c = glyph->c;

        glyphX = glyph->x;
        glyphY = glyph->y;
        glyphWidth = glyph->w;
        glyphHeight = glyph->h;

        glyphMarginLeft = glyphData->marginLeft;
        glyphMarginTop = glyphData->marginTop;
        glyphMarginRight = glyphData->marginRight;
        glyphMarginBottom = glyphData->marginBottom;

        glyphDataWidth = glyphData->dataWidth;
        glyphDataHeight = glyphData->dataHeight;

        texY = textureGlyphPadding;

        u1 = (GLfloat) texX / (GLfloat) texWidth;
        v1 = (GLfloat) texY / (GLfloat) texHeight;

        u2 = (GLfloat) glyphWidth / (GLfloat) texWidth;
        v2 = (GLfloat) glyphHeight / (GLfloat) texHeight;

        glyph->u1 = u1;
        glyph->v1 = v1;

        glyph->u2 = u1 + u2;
        glyph->v2 = v1 + v2;

        texX += glyphMarginLeft;
        texY += glyphMarginTop;

        for (y = 0; y < glyphDataHeight; y++) {
            for (x = 0; x < glyphDataWidth; x++) {
                c0 = (_METAENGINE_Render_GLT_FontGlyphData[data0Index] >> bit0Index) & 1;
                c1 = (_METAENGINE_Render_GLT_FontGlyphData[data1Index] >> bit1Index) & 1;

                if ((c0 == 0) && (c1 == 0)) {
                    r = 0;
                    g = 0;
                    b = 0;
                    a = 0;
                } else if ((c0 == 0) && (c1 == 1)) {
                    r = 255;
                    g = 255;
                    b = 255;
                    a = 255;
                } else if ((c0 == 1) && (c1 == 0)) {
                    r = 0;
                    g = 0;
                    b = 0;
                    a = 255;
                }

                _METAENGINE_Render_GLT_TEX_SET_PIXEL(texX + x, texY + y, r, g, b, a);

                bit0Index += _METAENGINE_Render_GLT_FONT_PIXEL_SIZE_BITS;
                bit1Index += _METAENGINE_Render_GLT_FONT_PIXEL_SIZE_BITS;

                if (bit0Index >= glyphDataTypeSizeBits) {
                    bit0Index = bit0Index % glyphDataTypeSizeBits;
                    data0Index++;
                }

                if (bit1Index >= glyphDataTypeSizeBits) {
                    bit1Index = bit1Index % glyphDataTypeSizeBits;
                    data1Index++;
                }
            }
        }

        texX += glyphDataWidth;
        texY += glyphDataHeight;

        texX += glyphMarginRight;
        texX += textureGlyphSpacing;
    }

    for (i = 0; i < _METAENGINE_Render_GLT_FontGlyphCount; i++) {
        glyph = &_METAENGINE_Render_GLT_FontGlyphs[i];

        _METAENGINE_Render_GLT_FontGlyphs2[glyph->c - _METAENGINE_Render_GLT_FontGlyphMinChar] =
                *glyph;
    }

#undef _METAENGINE_Render_GLT_TEX_PIXEL_INDEX
#undef _METAENGINE_Render_GLT_TEX_SET_PIXEL

    glGenTextures(1, &_METAENGINE_Render_GLT_Text2DFontTexture);
    glBindTexture(GL_TEXTURE_2D, _METAENGINE_Render_GLT_Text2DFontTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 texData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    free(texData);

    free(glyphsData);

    return GL_TRUE;
}

void Drawing::drawText(std::string name, std::string text, uint8_t x, uint8_t y, ImVec4 col) {
    auto func = [&] { ImGui::TextColored(col, "%s", text.c_str()); };
    drawTextEx(name, x, y, func);
}

void Drawing::drawTextEx(std::string name, uint8_t x, uint8_t y, std::function<void()> func) {
    ImGui::SetNextWindowPos(
            global.ImGuiCore->GetNextWindowsPos(ImGuiWindowTags::UI_MainMenu, ImVec2(x, y)));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking |
                             ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin(name.c_str(), NULL, flags);
    func();
    ImGui::End();
}

b2Vec2 Drawing::rotate_point(float cx, float cy, float angle, b2Vec2 p) {
    float s = sin(angle);
    float c = cos(angle);

    // translate point back to origin:
    p.x -= cx;
    p.y -= cy;

    // rotate point
    float xnew = p.x * c - p.y * s;
    float ynew = p.x * s + p.y * c;

    // translate point back:
    return b2Vec2(xnew + cx, ynew + cy);
}

void Drawing::drawPolygon(METAENGINE_Render_Target *target, METAENGINE_Color col, b2Vec2 *verts,
                          int x, int y, float scale, int count, float angle, float cx, float cy) {
    if (count < 2) return;
    b2Vec2 last = rotate_point(cx, cy, angle, verts[count - 1]);
    for (int i = 0; i < count; i++) {
        b2Vec2 rot = rotate_point(cx, cy, angle, verts[i]);
        METAENGINE_Render_Line(target, x + last.x * scale, y + last.y * scale, x + rot.x * scale,
                               y + rot.y * scale, col);
        last = rot;
    }
}

uint32 Drawing::darkenColor(uint32 color, float brightness) {
    int a = (color >> 24) & 0xFF;
    int r = (int) (((color >> 16) & 0xFF) * brightness);
    int g = (int) (((color >> 8) & 0xFF) * brightness);
    int b = (int) ((color & 0xFF) * brightness);

    return (a << 24) | (r << 16) | (g << 8) | b;
}

#ifndef PI
#define PI 3.1415926f
#endif

#define RAD_PER_DEG 0.017453293f
#define DEG_PER_RAD 57.2957795f

void gpu_init_renderer_register(void);
void gpu_free_renderer_register(void);

typedef struct METAENGINE_Render_RendererRegistration
{
    METAENGINE_Render_RendererID id;
    METAENGINE_Render_Renderer *(*createFn)(METAENGINE_Render_RendererID request);
    void (*freeFn)(METAENGINE_Render_Renderer *);
} METAENGINE_Render_RendererRegistration;

static bool gpu_renderer_register_is_initialized = false;

static METAENGINE_Render_Renderer *gpu_renderer_map;
static METAENGINE_Render_RendererRegistration gpu_renderer_register;
static METAENGINE_Render_RendererID gpu_renderer_order;

METAENGINE_Render_RendererID METAENGINE_Render_GetRendererID() {
    gpu_init_renderer_register();
    return gpu_renderer_register.id;
}

METAENGINE_Render_Renderer *METAENGINE_Render_CreateRenderer_OpenGL_3(
        METAENGINE_Render_RendererID request);
void METAENGINE_Render_FreeRenderer_OpenGL_3(METAENGINE_Render_Renderer *renderer);

void METAENGINE_Render_RegisterRenderer(
        METAENGINE_Render_RendererID id,
        METAENGINE_Render_Renderer *(*create_renderer)(METAENGINE_Render_RendererID request),
        void (*free_renderer)(METAENGINE_Render_Renderer *renderer)) {

    if (create_renderer == NULL) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_USER_ERROR,
                                        "NULL renderer create callback");
        return;
    }
    if (free_renderer == NULL) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_USER_ERROR,
                                        "NULL renderer free callback");
        return;
    }

    gpu_renderer_register.id = id;
    gpu_renderer_register.createFn = create_renderer;
    gpu_renderer_register.freeFn = free_renderer;
}

void gpu_register_built_in_renderers(void) {
#ifdef __MACOSX__
    // Depending on OS X version, it might only support core GL 3.3 or 3.2
    METAENGINE_Render_RegisterRenderer(
            METAENGINE_Render_MakeRendererID("OpenGL 3", METAENGINE_Render_GL_VERSION_MAJOR,
                                             METAENGINE_Render_GL_VERSION_MINOR),
            &METAENGINE_Render_CreateRenderer_OpenGL_3, &METAENGINE_Render_FreeRenderer_OpenGL_3);
#else
    METAENGINE_Render_RegisterRenderer(
            METAENGINE_Render_MakeRendererID("OpenGL 3", METAENGINE_Render_RENDERER_OPENGL_3, 3, 0),
            &METAENGINE_Render_CreateRenderer_OpenGL_3, &METAENGINE_Render_FreeRenderer_OpenGL_3);
#endif
}

void gpu_init_renderer_register(void) {

    if (gpu_renderer_register_is_initialized) return;

    gpu_renderer_register.id.name = "Unknown";
    gpu_renderer_register.createFn = NULL;
    gpu_renderer_register.freeFn = NULL;

    gpu_renderer_map = NULL;

    gpu_renderer_order = METAENGINE_Render_MakeRendererID(
            "OpenGL 3", METAENGINE_Render_GL_VERSION_MAJOR, METAENGINE_Render_GL_VERSION_MINOR);

    gpu_renderer_register_is_initialized = 1;

    gpu_register_built_in_renderers();
}

void gpu_free_renderer_register(void) {
    gpu_renderer_register.id.name = "Unknown";
    gpu_renderer_register.createFn = NULL;
    gpu_renderer_register.freeFn = NULL;
    gpu_renderer_map = NULL;
    gpu_renderer_register_is_initialized = 0;
}

void METAENGINE_Render_GetRendererOrder(int *order_size, METAENGINE_Render_RendererID *order) {
    if (order != NULL) memcpy(order, &gpu_renderer_order, sizeof(METAENGINE_Render_RendererID));
}

// Get a renderer from the map.
METAENGINE_Render_Renderer *METAENGINE_Render_GetRenderer(METAENGINE_Render_RendererID id) {
    gpu_init_renderer_register();
    return gpu_renderer_map;
}

// Free renderer memory according to how the registry instructs
void gpu_free_renderer_memory(METAENGINE_Render_Renderer *renderer) {
    if (renderer == NULL) return;
    gpu_renderer_register.freeFn(renderer);
}

// Remove a renderer from the active map and free it.
void METAENGINE_Render_FreeRenderer(METAENGINE_Render_Renderer *renderer) {
    METAENGINE_Render_Renderer *current_renderer;

    if (renderer == NULL) return;

    current_renderer = METAENGINE_Render_GetCurrentRenderer();
    if (current_renderer == renderer)
        METAENGINE_Render_SetCurrentRenderer(METAENGINE_Render_MakeRendererID("Unknown", 0, 0));

    if (renderer == gpu_renderer_map) {
        gpu_free_renderer_memory(renderer);
        gpu_renderer_map = NULL;
        return;
    }
}

#define GET_ALPHA(sdl_color) ((sdl_color).a)

#define CHECK_RENDERER (gpu_current_renderer != NULL)
#define MAKE_CURRENT_IF_NONE(target)                                                               \
    do {                                                                                           \
        if (gpu_current_renderer->current_context_target == NULL && target != NULL &&              \
            target->context != NULL)                                                               \
            METAENGINE_Render_MakeCurrent(target, target->context->windowID);                      \
    } while (0)
#define CHECK_CONTEXT (gpu_current_renderer->current_context_target != NULL)
#define RETURN_ERROR(code, details)                                                                \
    do {                                                                                           \
        METAENGINE_Render_PushErrorCode(__func__, code, "%s", details);                            \
        return;                                                                                    \
    } while (0)

int gpu_strcasecmp(const char *s1, const char *s2);

void gpu_init_renderer_register(void);
void gpu_free_renderer_register(void);

/*! A mapping of windowID to a METAENGINE_Render_Target to facilitate METAENGINE_Render_GetWindowTarget(). */
typedef struct METAENGINE_Render_WindowMapping
{
    Uint32 windowID;
    METAENGINE_Render_Target *target;
} METAENGINE_Render_WindowMapping;

static METAENGINE_Render_Renderer *gpu_current_renderer = NULL;

#define METAENGINE_Render_DEFAULT_MAX_NUM_ERRORS 20
#define METAENGINE_Render_ERROR_FUNCTION_STRING_MAX 128
#define METAENGINE_Render_ERROR_DETAILS_STRING_MAX 512
static METAENGINE_Render_ErrorObject *gpu_error_code_queue = NULL;
static unsigned int gpu_num_error_codes = 0;
static unsigned int gpu_error_code_queue_size = METAENGINE_Render_DEFAULT_MAX_NUM_ERRORS;
static METAENGINE_Render_ErrorObject gpu_error_code_result;

#define METAENGINE_Render_INITIAL_WINDOW_MAPPINGS_SIZE 10
static METAENGINE_Render_WindowMapping *gpu_window_mappings = NULL;
static int gpu_window_mappings_size = 0;
static int gpu_num_window_mappings = 0;

static Uint32 gpu_init_windowID = 0;

static METAENGINE_Render_InitFlagEnum gpu_preinit_flags = METAENGINE_Render_DEFAULT_INIT_FLAGS;
static METAENGINE_Render_InitFlagEnum gpu_required_features = 0;

static bool gpu_initialized_SDL_core = false;
static bool gpu_initialized_SDL = false;

void METAENGINE_Render_SetCurrentRenderer(METAENGINE_Render_RendererID id) {
    gpu_current_renderer = METAENGINE_Render_GetRenderer(id);

    if (gpu_current_renderer != NULL)
        gpu_current_renderer->impl->SetAsCurrent(gpu_current_renderer);
}

void METAENGINE_Render_ResetRendererState(void) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->ResetRendererState(gpu_current_renderer);
}

void METAENGINE_Render_SetCoordinateMode(bool use_math_coords) {
    if (gpu_current_renderer == NULL) return;

    gpu_current_renderer->coordinate_mode = use_math_coords;
}

bool METAENGINE_Render_GetCoordinateMode(void) {
    if (gpu_current_renderer == NULL) return false;

    return gpu_current_renderer->coordinate_mode;
}

METAENGINE_Render_Renderer *METAENGINE_Render_GetCurrentRenderer(void) {
    return gpu_current_renderer;
}

Uint32 METAENGINE_Render_GetCurrentShaderProgram(void) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return 0;

    return gpu_current_renderer->current_context_target->context->current_shader_program;
}

void METAENGINE_Render_SetInitWindow(Uint32 windowID) { gpu_init_windowID = windowID; }

Uint32 METAENGINE_Render_GetInitWindow(void) { return gpu_init_windowID; }

void METAENGINE_Render_SetPreInitFlags(METAENGINE_Render_InitFlagEnum METAENGINE_Render_flags) {
    gpu_preinit_flags = METAENGINE_Render_flags;
}

METAENGINE_Render_InitFlagEnum METAENGINE_Render_GetPreInitFlags(void) { return gpu_preinit_flags; }

void METAENGINE_Render_SetRequiredFeatures(METAENGINE_Render_FeatureEnum features) {
    gpu_required_features = features;
}

METAENGINE_Render_FeatureEnum METAENGINE_Render_GetRequiredFeatures(void) {
    return gpu_required_features;
}

static void gpu_init_error_queue(void) {
    if (gpu_error_code_queue == NULL) {
        unsigned int i;
        gpu_error_code_queue = (METAENGINE_Render_ErrorObject *) SDL_malloc(
                sizeof(METAENGINE_Render_ErrorObject) * gpu_error_code_queue_size);

        for (i = 0; i < gpu_error_code_queue_size; i++) {
            gpu_error_code_queue[i].function =
                    (char *) SDL_malloc(METAENGINE_Render_ERROR_FUNCTION_STRING_MAX + 1);
            gpu_error_code_queue[i].error = METAENGINE_Render_ERROR_NONE;
            gpu_error_code_queue[i].details =
                    (char *) SDL_malloc(METAENGINE_Render_ERROR_DETAILS_STRING_MAX + 1);
        }
        gpu_num_error_codes = 0;

        gpu_error_code_result.function =
                (char *) SDL_malloc(METAENGINE_Render_ERROR_FUNCTION_STRING_MAX + 1);
        gpu_error_code_result.error = METAENGINE_Render_ERROR_NONE;
        gpu_error_code_result.details =
                (char *) SDL_malloc(METAENGINE_Render_ERROR_DETAILS_STRING_MAX + 1);
    }
}

static void gpu_init_window_mappings(void) {
    if (gpu_window_mappings == NULL) {
        gpu_window_mappings_size = METAENGINE_Render_INITIAL_WINDOW_MAPPINGS_SIZE;
        gpu_window_mappings = (METAENGINE_Render_WindowMapping *) SDL_malloc(
                gpu_window_mappings_size * sizeof(METAENGINE_Render_WindowMapping));
        gpu_num_window_mappings = 0;
    }
}

void METAENGINE_Render_AddWindowMapping(METAENGINE_Render_Target *target) {
    Uint32 windowID;
    int i;

    if (gpu_window_mappings == NULL) gpu_init_window_mappings();

    if (target == NULL || target->context == NULL) return;

    windowID = target->context->windowID;
    if (windowID == 0)// Invalid window ID
        return;

    // Check for duplicates
    for (i = 0; i < gpu_num_window_mappings; i++) {
        if (gpu_window_mappings[i].windowID == windowID) {
            if (gpu_window_mappings[i].target != target)
                METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_DATA_ERROR,
                                                "WindowID %u already has a mapping.", windowID);
            return;
        }
        // Don't check the target because it's okay for a single target to be used with multiple windows
    }

    // Check if list is big enough to hold another
    if (gpu_num_window_mappings >= gpu_window_mappings_size) {
        METAENGINE_Render_WindowMapping *new_array;
        gpu_window_mappings_size *= 2;
        new_array = (METAENGINE_Render_WindowMapping *) SDL_malloc(
                gpu_window_mappings_size * sizeof(METAENGINE_Render_WindowMapping));
        memcpy(new_array, gpu_window_mappings,
               gpu_num_window_mappings * sizeof(METAENGINE_Render_WindowMapping));
        SDL_free(gpu_window_mappings);
        gpu_window_mappings = new_array;
    }

    // Add to end of list
    {
        METAENGINE_Render_WindowMapping m;
        m.windowID = windowID;
        m.target = target;
        gpu_window_mappings[gpu_num_window_mappings] = m;
    }
    gpu_num_window_mappings++;
}

void METAENGINE_Render_RemoveWindowMapping(Uint32 windowID) {
    int i;

    if (gpu_window_mappings == NULL) gpu_init_window_mappings();

    if (windowID == 0)// Invalid window ID
        return;

    // Find the occurrence
    for (i = 0; i < gpu_num_window_mappings; i++) {
        if (gpu_window_mappings[i].windowID == windowID) {
            int num_to_move;

            // Unset the target's window
            gpu_window_mappings[i].target->context->windowID = 0;

            // Move the remaining entries to replace the removed one
            gpu_num_window_mappings--;
            num_to_move = gpu_num_window_mappings - i;
            if (num_to_move > 0)
                memmove(&gpu_window_mappings[i], &gpu_window_mappings[i + 1],
                        num_to_move * sizeof(METAENGINE_Render_WindowMapping));
            return;
        }
    }
}

void METAENGINE_Render_RemoveWindowMappingByTarget(METAENGINE_Render_Target *target) {
    Uint32 windowID;
    int i;

    if (gpu_window_mappings == NULL) gpu_init_window_mappings();

    if (target == NULL || target->context == NULL) return;

    windowID = target->context->windowID;
    if (windowID == 0)// Invalid window ID
        return;

    // Unset the target's window
    target->context->windowID = 0;

    // Find the occurrences
    for (i = 0; i < gpu_num_window_mappings; ++i) {
        if (gpu_window_mappings[i].target == target) {
            // Move the remaining entries to replace the removed one
            int num_to_move;
            gpu_num_window_mappings--;
            num_to_move = gpu_num_window_mappings - i;
            if (num_to_move > 0)
                memmove(&gpu_window_mappings[i], &gpu_window_mappings[i + 1],
                        num_to_move * sizeof(METAENGINE_Render_WindowMapping));
            return;
        }
    }
}

METAENGINE_Render_Target *METAENGINE_Render_GetWindowTarget(Uint32 windowID) {
    int i;

    if (gpu_window_mappings == NULL) gpu_init_window_mappings();

    if (windowID == 0)// Invalid window ID
        return NULL;

    // Find the occurrence
    for (i = 0; i < gpu_num_window_mappings; ++i) {
        if (gpu_window_mappings[i].windowID == windowID) return gpu_window_mappings[i].target;
    }

    return NULL;
}

METAENGINE_Render_Target *METAENGINE_Render_Init(Uint16 w, Uint16 h,
                                                 METAENGINE_Render_WindowFlagEnum SDL_flags) {
    METAENGINE_Render_RendererID renderer_order;

    gpu_init_error_queue();
    gpu_init_renderer_register();

    int renderer_order_size = 1;
    METAENGINE_Render_GetRendererOrder(&renderer_order_size, &renderer_order);

    METAENGINE_Render_Target *screen =
            METAENGINE_Render_InitRendererByID(renderer_order, w, h, SDL_flags);
    if (screen != NULL) return screen;

    METAENGINE_Render_PushErrorCode("METAENGINE_Render_Init", METAENGINE_Render_ERROR_BACKEND_ERROR,
                                    "No renderer out of %d was able to initialize properly",
                                    renderer_order_size);
    return NULL;
}

METAENGINE_Render_Target *METAENGINE_Render_InitRenderer(
        Uint16 w, Uint16 h, METAENGINE_Render_WindowFlagEnum SDL_flags) {
    // Search registry for this renderer and use that id
    return METAENGINE_Render_InitRendererByID(METAENGINE_Render_GetRendererID(), w, h, SDL_flags);
}

METAENGINE_Render_Target *METAENGINE_Render_InitRendererByID(
        METAENGINE_Render_RendererID renderer_request, Uint16 w, Uint16 h,
        METAENGINE_Render_WindowFlagEnum SDL_flags) {
    METAENGINE_Render_Renderer *renderer;
    METAENGINE_Render_Target *screen;

    gpu_init_error_queue();
    gpu_init_renderer_register();

    if (gpu_renderer_map == NULL) {
        // Create

        if (gpu_renderer_register.createFn != NULL) {
            // Use the registered name
            renderer_request.name = gpu_renderer_register.id.name;
            renderer = gpu_renderer_register.createFn(renderer_request);
        }

        if (renderer == nullptr) {
            METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_BACKEND_ERROR,
                                            "Failed to create new renderer.");
            return nullptr;
        }

        gpu_renderer_map = renderer;
    }

    // renderer = gpu_create_and_add_renderer(renderer_request);
    if (renderer == NULL) return NULL;

    METAENGINE_Render_SetCurrentRenderer(renderer->id);

    screen = renderer->impl->Init(renderer, renderer_request, w, h, SDL_flags);
    if (screen == NULL) {
        METAENGINE_Render_PushErrorCode(
                "METAENGINE_Render_InitRendererByID", METAENGINE_Render_ERROR_BACKEND_ERROR,
                "Renderer %s failed to initialize properly", renderer->id.name);
        // Init failed, destroy the renderer...
        // Erase the window mappings
        gpu_num_window_mappings = 0;
        METAENGINE_Render_CloseCurrentRenderer();
    } else
        METAENGINE_Render_SetInitWindow(0);
    return screen;
}

bool METAENGINE_Render_IsFeatureEnabled(METAENGINE_Render_FeatureEnum feature) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return false;

    return ((gpu_current_renderer->enabled_features & feature) == feature);
}

METAENGINE_Render_Target *METAENGINE_Render_CreateTargetFromWindow(Uint32 windowID) {
    if (gpu_current_renderer == NULL) return NULL;

    return gpu_current_renderer->impl->CreateTargetFromWindow(gpu_current_renderer, windowID, NULL);
}

METAENGINE_Render_Target *METAENGINE_Render_CreateAliasTarget(METAENGINE_Render_Target *target) {
    if (!CHECK_RENDERER) return NULL;
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) return NULL;

    return gpu_current_renderer->impl->CreateAliasTarget(gpu_current_renderer, target);
}

void METAENGINE_Render_MakeCurrent(METAENGINE_Render_Target *target, Uint32 windowID) {
    if (gpu_current_renderer == NULL) return;

    gpu_current_renderer->impl->MakeCurrent(gpu_current_renderer, target, windowID);
}

bool METAENGINE_Render_SetFullscreen(bool enable_fullscreen, bool use_desktop_resolution) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return false;

    return gpu_current_renderer->impl->SetFullscreen(gpu_current_renderer, enable_fullscreen,
                                                     use_desktop_resolution);
}

bool METAENGINE_Render_GetFullscreen(void) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetContextTarget();
    if (target == NULL) return false;
    return (SDL_GetWindowFlags(SDL_GetWindowFromID(target->context->windowID)) &
            (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0;
}

METAENGINE_Render_Target *METAENGINE_Render_GetActiveTarget(void) {
    METAENGINE_Render_Target *context_target = METAENGINE_Render_GetContextTarget();
    if (context_target == NULL) return NULL;

    return context_target->context->active_target;
}

bool METAENGINE_Render_SetActiveTarget(METAENGINE_Render_Target *target) {
    if (gpu_current_renderer == NULL) return false;

    return gpu_current_renderer->impl->SetActiveTarget(gpu_current_renderer, target);
}

bool METAENGINE_Render_AddDepthBuffer(METAENGINE_Render_Target *target) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL ||
        target == NULL)
        return false;

    return gpu_current_renderer->impl->AddDepthBuffer(gpu_current_renderer, target);
}

void METAENGINE_Render_SetDepthTest(METAENGINE_Render_Target *target, bool enable) {
    if (target != NULL) target->use_depth_test = enable;
}

void METAENGINE_Render_SetDepthWrite(METAENGINE_Render_Target *target, bool enable) {
    if (target != NULL) target->use_depth_write = enable;
}

void METAENGINE_Render_SetDepthFunction(METAENGINE_Render_Target *target,
                                        METAENGINE_Render_ComparisonEnum compare_operation) {
    if (target != NULL) target->depth_function = compare_operation;
}

bool METAENGINE_Render_SetWindowResolution(Uint16 w, Uint16 h) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL ||
        w == 0 || h == 0)
        return false;

    return gpu_current_renderer->impl->SetWindowResolution(gpu_current_renderer, w, h);
}

void METAENGINE_Render_GetVirtualResolution(METAENGINE_Render_Target *target, Uint16 *w,
                                            Uint16 *h) {
    // No checking here for NULL w or h...  Should we?
    if (target == NULL) {
        *w = 0;
        *h = 0;
    } else {
        *w = target->w;
        *h = target->h;
    }
}

void METAENGINE_Render_SetVirtualResolution(METAENGINE_Render_Target *target, Uint16 w, Uint16 h) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");
    if (w == 0 || h == 0) return;

    gpu_current_renderer->impl->SetVirtualResolution(gpu_current_renderer, target, w, h);
}

void METAENGINE_Render_UnsetVirtualResolution(METAENGINE_Render_Target *target) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    gpu_current_renderer->impl->UnsetVirtualResolution(gpu_current_renderer, target);
}

void METAENGINE_Render_SetImageVirtualResolution(METAENGINE_Render_Image *image, Uint16 w,
                                                 Uint16 h) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL ||
        w == 0 || h == 0)
        return;

    if (image == NULL) return;

    METAENGINE_Render_FlushBlitBuffer();// TODO: Perhaps move SetImageVirtualResolution into the renderer so we can check to see if this image is bound first.
    image->w = w;
    image->h = h;
    image->using_virtual_resolution = 1;
}

void METAENGINE_Render_UnsetImageVirtualResolution(METAENGINE_Render_Image *image) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    if (image == NULL) return;

    METAENGINE_Render_FlushBlitBuffer();// TODO: Perhaps move SetImageVirtualResolution into the renderer so we can check to see if this image is bound first.
    image->w = image->base_w;
    image->h = image->base_h;
    image->using_virtual_resolution = 0;
}

void gpu_free_error_queue(void) {
    unsigned int i;
    // Free the error queue
    for (i = 0; i < gpu_error_code_queue_size; i++) {
        SDL_free(gpu_error_code_queue[i].function);
        gpu_error_code_queue[i].function = NULL;
        SDL_free(gpu_error_code_queue[i].details);
        gpu_error_code_queue[i].details = NULL;
    }
    SDL_free(gpu_error_code_queue);
    gpu_error_code_queue = NULL;
    gpu_num_error_codes = 0;

    SDL_free(gpu_error_code_result.function);
    gpu_error_code_result.function = NULL;
    SDL_free(gpu_error_code_result.details);
    gpu_error_code_result.details = NULL;
}

// Deletes all existing errors
void METAENGINE_Render_SetErrorQueueMax(unsigned int max) {
    gpu_free_error_queue();

    // Reallocate with new size
    gpu_error_code_queue_size = max;
    gpu_init_error_queue();
}

void METAENGINE_Render_CloseCurrentRenderer(void) {
    if (gpu_current_renderer == NULL) return;

    gpu_current_renderer->impl->Quit(gpu_current_renderer);
    METAENGINE_Render_FreeRenderer(gpu_current_renderer);
}

void METAENGINE_Render_Quit(void) {
    if (gpu_num_error_codes > 0)
        METADOT_ERROR("METAENGINE_Render_Quit: {} uncleared error{}.", gpu_num_error_codes,
                      (gpu_num_error_codes > 1 ? "s" : ""));

    gpu_free_error_queue();

    if (gpu_current_renderer == NULL) return;

    gpu_current_renderer->impl->Quit(gpu_current_renderer);
    METAENGINE_Render_FreeRenderer(gpu_current_renderer);
    // FIXME: Free all renderers
    gpu_current_renderer = NULL;

    gpu_init_windowID = 0;

    // Free window mappings
    SDL_free(gpu_window_mappings);
    gpu_window_mappings = NULL;
    gpu_window_mappings_size = 0;
    gpu_num_window_mappings = 0;

    gpu_free_renderer_register();

    if (gpu_initialized_SDL) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        gpu_initialized_SDL = 0;

        if (gpu_initialized_SDL_core) {
            SDL_Quit();
            gpu_initialized_SDL_core = 0;
        }
    }
}

void METAENGINE_Render_PushErrorCode(const char *function, METAENGINE_Render_ErrorEnum error,
                                     const char *details, ...) {
    gpu_init_error_queue();

#if defined(METADOT_DEBUG)
    // Print the message
    if (details != NULL) {
        char buf[METAENGINE_Render_ERROR_DETAILS_STRING_MAX];
        va_list lst;
        va_start(lst, details);
        vsnprintf(buf, METAENGINE_Render_ERROR_DETAILS_STRING_MAX, details, lst);
        va_end(lst);

        METADOT_ERROR("{0}: {1} - {2}", (function == NULL ? "NULL" : function),
                      METAENGINE_Render_GetErrorString(error), buf);
    } else
        METADOT_ERROR("{0}: {1}", (function == NULL ? "NULL" : function),
                      METAENGINE_Render_GetErrorString(error));
#endif

    if (gpu_num_error_codes < gpu_error_code_queue_size) {
        if (function == NULL) gpu_error_code_queue[gpu_num_error_codes].function[0] = '\0';
        else {
            strncpy(gpu_error_code_queue[gpu_num_error_codes].function, function,
                    METAENGINE_Render_ERROR_FUNCTION_STRING_MAX);
            gpu_error_code_queue[gpu_num_error_codes]
                    .function[METAENGINE_Render_ERROR_FUNCTION_STRING_MAX] = '\0';
        }
        gpu_error_code_queue[gpu_num_error_codes].error = error;
        if (details == NULL) gpu_error_code_queue[gpu_num_error_codes].details[0] = '\0';
        else {
            va_list lst;
            va_start(lst, details);
            vsnprintf(gpu_error_code_queue[gpu_num_error_codes].details,
                      METAENGINE_Render_ERROR_DETAILS_STRING_MAX, details, lst);
            va_end(lst);
        }
        gpu_num_error_codes++;
    }
}

METAENGINE_Render_ErrorObject METAENGINE_Render_PopErrorCode(void) {
    unsigned int i;
    METAENGINE_Render_ErrorObject result = {NULL, NULL, METAENGINE_Render_ERROR_NONE};

    gpu_init_error_queue();

    if (gpu_num_error_codes <= 0) return result;

    // Pop the oldest
    strcpy(gpu_error_code_result.function, gpu_error_code_queue[0].function);
    gpu_error_code_result.error = gpu_error_code_queue[0].error;
    strcpy(gpu_error_code_result.details, gpu_error_code_queue[0].details);

    // We'll be returning that one
    result = gpu_error_code_result;

    // Move the rest down
    gpu_num_error_codes--;
    for (i = 0; i < gpu_num_error_codes; i++) {
        strcpy(gpu_error_code_queue[i].function, gpu_error_code_queue[i + 1].function);
        gpu_error_code_queue[i].error = gpu_error_code_queue[i + 1].error;
        strcpy(gpu_error_code_queue[i].details, gpu_error_code_queue[i + 1].details);
    }
    return result;
}

const char *METAENGINE_Render_GetErrorString(METAENGINE_Render_ErrorEnum error) {
    switch (error) {
        case METAENGINE_Render_ERROR_NONE:
            return "NO ERROR";
        case METAENGINE_Render_ERROR_BACKEND_ERROR:
            return "BACKEND ERROR";
        case METAENGINE_Render_ERROR_DATA_ERROR:
            return "DATA ERROR";
        case METAENGINE_Render_ERROR_USER_ERROR:
            return "USER ERROR";
        case METAENGINE_Render_ERROR_UNSUPPORTED_FUNCTION:
            return "UNSUPPORTED FUNCTION";
        case METAENGINE_Render_ERROR_NULL_ARGUMENT:
            return "NULL ARGUMENT";
        case METAENGINE_Render_ERROR_FILE_NOT_FOUND:
            return "FILE NOT FOUND";
    }
    return "UNKNOWN ERROR";
}

void METAENGINE_Render_GetVirtualCoords(METAENGINE_Render_Target *target, float *x, float *y,
                                        float displayX, float displayY) {
    if (target == NULL || gpu_current_renderer == NULL) return;

    // Scale from raw window/image coords to the virtual scale
    if (target->context != NULL) {
        if (x != NULL) *x = (displayX * target->w) / target->context->window_w;
        if (y != NULL) *y = (displayY * target->h) / target->context->window_h;
    } else if (target->image != NULL) {
        if (x != NULL) *x = (displayX * target->w) / target->image->w;
        if (y != NULL) *y = (displayY * target->h) / target->image->h;
    } else {
        // What is the backing for this target?!
        if (x != NULL) *x = displayX;
        if (y != NULL) *y = displayY;
    }

    // Invert coordinates to math coords
    if (gpu_current_renderer->coordinate_mode) *y = target->h - *y;
}

METAENGINE_Render_Rect METAENGINE_Render_MakeRect(float x, float y, float w, float h) {
    METAENGINE_Render_Rect r;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;

    return r;
}

METAENGINE_Color METAENGINE_Render_MakeColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    METAENGINE_Color c;
    c.r = r;
    c.g = g;
    c.b = b;
    GET_ALPHA(c) = a;

    return c;
}

METAENGINE_Render_RendererID METAENGINE_Render_MakeRendererID(const char *name, int major_version,
                                                              int minor_version) {
    METAENGINE_Render_RendererID r;
    r.name = name;
    r.major_version = major_version;
    r.minor_version = minor_version;

    return r;
}

void METAENGINE_Render_SetViewport(METAENGINE_Render_Target *target,
                                   METAENGINE_Render_Rect viewport) {
    if (target != NULL) target->viewport = viewport;
}

void METAENGINE_Render_UnsetViewport(METAENGINE_Render_Target *target) {
    if (target != NULL) target->viewport = METAENGINE_Render_MakeRect(0, 0, target->w, target->h);
}

METAENGINE_Render_Camera METAENGINE_Render_GetDefaultCamera(void) {
    METAENGINE_Render_Camera cam = {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, -100.0f, 100.0f, true};
    return cam;
}

METAENGINE_Render_Camera METAENGINE_Render_GetCamera(METAENGINE_Render_Target *target) {
    if (target == NULL) return METAENGINE_Render_GetDefaultCamera();
    return target->camera;
}

METAENGINE_Render_Camera METAENGINE_Render_SetCamera(METAENGINE_Render_Target *target,
                                                     METAENGINE_Render_Camera *cam) {
    if (gpu_current_renderer == NULL) return METAENGINE_Render_GetDefaultCamera();
    MAKE_CURRENT_IF_NONE(target);
    if (gpu_current_renderer->current_context_target == NULL)
        return METAENGINE_Render_GetDefaultCamera();
    // TODO: Remove from renderer and flush here
    return gpu_current_renderer->impl->SetCamera(gpu_current_renderer, target, cam);
}

void METAENGINE_Render_EnableCamera(METAENGINE_Render_Target *target, bool use_camera) {
    if (target == NULL) return;
    // TODO: Flush here
    target->use_camera = use_camera;
}

bool METAENGINE_Render_IsCameraEnabled(METAENGINE_Render_Target *target) {
    if (target == NULL) return false;
    return target->use_camera;
}

METAENGINE_Render_Image *METAENGINE_Render_CreateImage(Uint16 w, Uint16 h,
                                                       METAENGINE_Render_FormatEnum format) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return gpu_current_renderer->impl->CreateImage(gpu_current_renderer, w, h, format);
}

METAENGINE_Render_Image *METAENGINE_Render_CreateImageUsingTexture(
        METAENGINE_Render_TextureHandle handle, bool take_ownership) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return gpu_current_renderer->impl->CreateImageUsingTexture(gpu_current_renderer, handle,
                                                               take_ownership);
}

METAENGINE_Render_Image *METAENGINE_Render_CreateAliasImage(METAENGINE_Render_Image *image) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return gpu_current_renderer->impl->CreateAliasImage(gpu_current_renderer, image);
}

METAENGINE_Render_Image *METAENGINE_Render_CopyImage(METAENGINE_Render_Image *image) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return gpu_current_renderer->impl->CopyImage(gpu_current_renderer, image);
}

void METAENGINE_Render_UpdateImage(METAENGINE_Render_Image *image,
                                   const METAENGINE_Render_Rect *image_rect, void *surface,
                                   const METAENGINE_Render_Rect *surface_rect) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->UpdateImage(gpu_current_renderer, image, image_rect, surface,
                                            surface_rect);
}

void METAENGINE_Render_UpdateImageBytes(METAENGINE_Render_Image *image,
                                        const METAENGINE_Render_Rect *image_rect,
                                        const unsigned char *bytes, int bytes_per_row) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->UpdateImageBytes(gpu_current_renderer, image, image_rect, bytes,
                                                 bytes_per_row);
}

bool METAENGINE_Render_ReplaceImage(METAENGINE_Render_Image *image, void *surface,
                                    const METAENGINE_Render_Rect *surface_rect) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return false;

    return gpu_current_renderer->impl->ReplaceImage(gpu_current_renderer, image, surface,
                                                    surface_rect);
}

static SDL_Surface *gpu_copy_raw_surface_data(unsigned char *data, int width, int height,
                                              int channels) {
    int i;
    Uint32 Rmask, Gmask, Bmask, Amask = 0;
    SDL_Surface *result;

    if (data == NULL) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_DATA_ERROR,
                                        "Got NULL data");
        return NULL;
    }

    switch (channels) {
        case 1:
            Rmask = Gmask = Bmask = 0;// Use default RGB masks for 8-bit
            break;
        case 2:
            Rmask = Gmask = Bmask = 0;// Use default RGB masks for 16-bit
            break;
        case 3:
            // These are reversed from what SDL_image uses...  That is bad. :(  Needs testing.
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
            Rmask = 0xff0000;
            Gmask = 0x00ff00;
            Bmask = 0x0000ff;
#else
            Rmask = 0x0000ff;
            Gmask = 0x00ff00;
            Bmask = 0xff0000;
#endif
            break;
        case 4:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
            rmask = 0xff000000;
            gmask = 0x00ff0000;
            bmask = 0x0000ff00;
            amask = 0x000000ff;
#else
            Rmask = 0x000000ff;
            Gmask = 0x0000ff00;
            Bmask = 0x00ff0000;
            Amask = 0xff000000;
#endif
            break;
        default:
            Rmask = Gmask = Bmask = 0;
            METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_DATA_ERROR,
                                            "Invalid number of channels: %d", channels);
            return NULL;
            break;
    }

    result = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, channels * 8, Rmask, Gmask, Bmask,
                                  Amask);
    //result = SDL_CreateRGBSurfaceFrom(data, width, height, channels * 8, width * channels, Rmask, Gmask, Bmask, Amask);
    if (result == NULL) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_DATA_ERROR,
                                        "Failed to create new %dx%d surface", width, height);
        return NULL;
    }

    // Copy row-by-row in case the pitch doesn't match
    for (i = 0; i < height; ++i) {
        memcpy((Uint8 *) result->pixels + i * result->pitch, data + channels * width * i,
               channels * width);
    }

    if (result != NULL && result->format->palette != NULL) {
        // SDL_CreateRGBSurface has no idea what palette to use, so it uses a blank one.
        // We'll at least create a grayscale one, but it's not ideal...
        // Better would be to get the palette from stbi, but stbi doesn't do that!
        METAENGINE_Color colors[256];

        for (i = 0; i < 256; i++) { colors[i].r = colors[i].g = colors[i].b = (Uint8) i; }

        /* Set palette */
        SDL_SetPaletteColors(result->format->palette, (SDL_Color *) colors, 0, 256);
    }

    return result;
}

// From http://stackoverflow.com/questions/5309471/getting-file-extension-in-c
static const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

METAENGINE_Render_Image *METAENGINE_Render_CopyImageFromSurface(void *surface) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return gpu_current_renderer->impl->CopyImageFromSurface(gpu_current_renderer, surface, NULL);
}

METAENGINE_Render_Image *METAENGINE_Render_CopyImageFromSurfaceRect(
        SDL_Surface *surface, METAENGINE_Render_Rect *surface_rect) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return gpu_current_renderer->impl->CopyImageFromSurface(gpu_current_renderer, surface,
                                                            surface_rect);
}

METAENGINE_Render_Image *METAENGINE_Render_CopyImageFromTarget(METAENGINE_Render_Target *target) {
    if (gpu_current_renderer == NULL) return NULL;
    MAKE_CURRENT_IF_NONE(target);
    if (gpu_current_renderer->current_context_target == NULL) return NULL;

    return gpu_current_renderer->impl->CopyImageFromTarget(gpu_current_renderer, target);
}

void *METAENGINE_Render_CopySurfaceFromTarget(METAENGINE_Render_Target *target) {
    if (gpu_current_renderer == NULL) return NULL;
    MAKE_CURRENT_IF_NONE(target);
    if (gpu_current_renderer->current_context_target == NULL) return NULL;

    return gpu_current_renderer->impl->CopySurfaceFromTarget(gpu_current_renderer, target);
}

void *METAENGINE_Render_CopySurfaceFromImage(METAENGINE_Render_Image *image) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return gpu_current_renderer->impl->CopySurfaceFromImage(gpu_current_renderer, image);
}

void METAENGINE_Render_FreeImage(METAENGINE_Render_Image *image) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->FreeImage(gpu_current_renderer, image);
}

METAENGINE_Render_Target *METAENGINE_Render_GetContextTarget(void) {
    if (gpu_current_renderer == NULL) return NULL;

    return gpu_current_renderer->current_context_target;
}

METAENGINE_Render_Target *METAENGINE_Render_LoadTarget(METAENGINE_Render_Image *image) {
    METAENGINE_Render_Target *result = METAENGINE_Render_GetTarget(image);

    if (result != NULL) result->refcount++;

    return result;
}

METAENGINE_Render_Target *METAENGINE_Render_GetTarget(METAENGINE_Render_Image *image) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return gpu_current_renderer->impl->GetTarget(gpu_current_renderer, image);
}

void METAENGINE_Render_FreeTarget(METAENGINE_Render_Target *target) {
    if (gpu_current_renderer == NULL) return;

    gpu_current_renderer->impl->FreeTarget(gpu_current_renderer, target);
}

void METAENGINE_Render_Blit(METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect,
                            METAENGINE_Render_Target *target, float x, float y) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    if (image == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "image");
    if (target == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "target");

    gpu_current_renderer->impl->Blit(gpu_current_renderer, image, src_rect, target, x, y);
}

void METAENGINE_Render_BlitRotate(METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect,
                                  METAENGINE_Render_Target *target, float x, float y,
                                  float degrees) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    if (image == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "image");
    if (target == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "target");

    gpu_current_renderer->impl->BlitRotate(gpu_current_renderer, image, src_rect, target, x, y,
                                           degrees);
}

void METAENGINE_Render_BlitScale(METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect,
                                 METAENGINE_Render_Target *target, float x, float y, float scaleX,
                                 float scaleY) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    if (image == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "image");
    if (target == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "target");

    gpu_current_renderer->impl->BlitScale(gpu_current_renderer, image, src_rect, target, x, y,
                                          scaleX, scaleY);
}

void METAENGINE_Render_BlitTransform(METAENGINE_Render_Image *image,
                                     METAENGINE_Render_Rect *src_rect,
                                     METAENGINE_Render_Target *target, float x, float y,
                                     float degrees, float scaleX, float scaleY) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    if (image == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "image");
    if (target == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "target");

    gpu_current_renderer->impl->BlitTransform(gpu_current_renderer, image, src_rect, target, x, y,
                                              degrees, scaleX, scaleY);
}

void METAENGINE_Render_BlitTransformX(METAENGINE_Render_Image *image,
                                      METAENGINE_Render_Rect *src_rect,
                                      METAENGINE_Render_Target *target, float x, float y,
                                      float pivot_x, float pivot_y, float degrees, float scaleX,
                                      float scaleY) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    if (image == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "image");
    if (target == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "target");

    gpu_current_renderer->impl->BlitTransformX(gpu_current_renderer, image, src_rect, target, x, y,
                                               pivot_x, pivot_y, degrees, scaleX, scaleY);
}

void METAENGINE_Render_BlitRect(METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect,
                                METAENGINE_Render_Target *target,
                                METAENGINE_Render_Rect *dest_rect) {
    float w = 0.0f;
    float h = 0.0f;

    if (image == NULL) return;

    if (src_rect == NULL) {
        w = image->w;
        h = image->h;
    } else {
        w = src_rect->w;
        h = src_rect->h;
    }

    METAENGINE_Render_BlitRectX(image, src_rect, target, dest_rect, 0.0f, w * 0.5f, h * 0.5f,
                                METAENGINE_Render_FLIP_NONE);
}

void METAENGINE_Render_BlitRectX(METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect,
                                 METAENGINE_Render_Target *target,
                                 METAENGINE_Render_Rect *dest_rect, float degrees, float pivot_x,
                                 float pivot_y, METAENGINE_Render_FlipEnum flip_direction) {
    float w, h;
    float dx, dy;
    float dw, dh;
    float scale_x, scale_y;

    if (image == NULL || target == NULL) return;

    if (src_rect == NULL) {
        w = image->w;
        h = image->h;
    } else {
        w = src_rect->w;
        h = src_rect->h;
    }

    if (dest_rect == NULL) {
        dx = 0.0f;
        dy = 0.0f;
        dw = target->w;
        dh = target->h;
    } else {
        dx = dest_rect->x;
        dy = dest_rect->y;
        dw = dest_rect->w;
        dh = dest_rect->h;
    }

    scale_x = dw / w;
    scale_y = dh / h;

    if (flip_direction & METAENGINE_Render_FLIP_HORIZONTAL) {
        scale_x = -scale_x;
        dx += dw;
        pivot_x = w - pivot_x;
    }
    if (flip_direction & METAENGINE_Render_FLIP_VERTICAL) {
        scale_y = -scale_y;
        dy += dh;
        pivot_y = h - pivot_y;
    }

    METAENGINE_Render_BlitTransformX(image, src_rect, target, dx + pivot_x * scale_x,
                                     dy + pivot_y * scale_y, pivot_x, pivot_y, degrees, scale_x,
                                     scale_y);
}

void METAENGINE_Render_TriangleBatch(METAENGINE_Render_Image *image,
                                     METAENGINE_Render_Target *target, unsigned short num_vertices,
                                     float *values, unsigned int num_indices,
                                     unsigned short *indices,
                                     METAENGINE_Render_BatchFlagEnum flags) {
    METAENGINE_Render_PrimitiveBatchV(image, target, METAENGINE_Render_TRIANGLES, num_vertices,
                                      (void *) values, num_indices, indices, flags);
}

void METAENGINE_Render_TriangleBatchX(METAENGINE_Render_Image *image,
                                      METAENGINE_Render_Target *target, unsigned short num_vertices,
                                      void *values, unsigned int num_indices,
                                      unsigned short *indices,
                                      METAENGINE_Render_BatchFlagEnum flags) {
    METAENGINE_Render_PrimitiveBatchV(image, target, METAENGINE_Render_TRIANGLES, num_vertices,
                                      values, num_indices, indices, flags);
}

void METAENGINE_Render_PrimitiveBatch(METAENGINE_Render_Image *image,
                                      METAENGINE_Render_Target *target,
                                      METAENGINE_Render_PrimitiveEnum primitive_type,
                                      unsigned short num_vertices, float *values,
                                      unsigned int num_indices, unsigned short *indices,
                                      METAENGINE_Render_BatchFlagEnum flags) {
    METAENGINE_Render_PrimitiveBatchV(image, target, primitive_type, num_vertices, (void *) values,
                                      num_indices, indices, flags);
}

void METAENGINE_Render_PrimitiveBatchV(METAENGINE_Render_Image *image,
                                       METAENGINE_Render_Target *target,
                                       METAENGINE_Render_PrimitiveEnum primitive_type,
                                       unsigned short num_vertices, void *values,
                                       unsigned int num_indices, unsigned short *indices,
                                       METAENGINE_Render_BatchFlagEnum flags) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    if (target == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "target");

    if (num_vertices == 0) return;

    gpu_current_renderer->impl->PrimitiveBatchV(gpu_current_renderer, image, target, primitive_type,
                                                num_vertices, values, num_indices, indices, flags);
}

void METAENGINE_Render_GenerateMipmaps(METAENGINE_Render_Image *image) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->GenerateMipmaps(gpu_current_renderer, image);
}

METAENGINE_Render_Rect METAENGINE_Render_SetClipRect(METAENGINE_Render_Target *target,
                                                     METAENGINE_Render_Rect rect) {
    if (target == NULL || gpu_current_renderer == NULL ||
        gpu_current_renderer->current_context_target == NULL) {
        METAENGINE_Render_Rect r = {0, 0, 0, 0};
        return r;
    }

    return gpu_current_renderer->impl->SetClip(gpu_current_renderer, target, (Sint16) rect.x,
                                               (Sint16) rect.y, (Uint16) rect.w, (Uint16) rect.h);
}

METAENGINE_Render_Rect METAENGINE_Render_SetClip(METAENGINE_Render_Target *target, Sint16 x,
                                                 Sint16 y, Uint16 w, Uint16 h) {
    if (target == NULL || gpu_current_renderer == NULL ||
        gpu_current_renderer->current_context_target == NULL) {
        METAENGINE_Render_Rect r = {0, 0, 0, 0};
        return r;
    }

    return gpu_current_renderer->impl->SetClip(gpu_current_renderer, target, x, y, w, h);
}

void METAENGINE_Render_UnsetClip(METAENGINE_Render_Target *target) {
    if (target == NULL || gpu_current_renderer == NULL ||
        gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->UnsetClip(gpu_current_renderer, target);
}

/* Adapted from SDL_IntersectRect() */
bool METAENGINE_Render_IntersectRect(METAENGINE_Render_Rect A, METAENGINE_Render_Rect B,
                                     METAENGINE_Render_Rect *result) {
    bool has_horiz_intersection = false;
    float Amin, Amax, Bmin, Bmax;
    METAENGINE_Render_Rect intersection;

    // Special case for empty rects
    if (A.w <= 0.0f || A.h <= 0.0f || B.w <= 0.0f || B.h <= 0.0f) return false;

    // Horizontal intersection
    Amin = A.x;
    Amax = Amin + A.w;
    Bmin = B.x;
    Bmax = Bmin + B.w;
    if (Bmin > Amin) Amin = Bmin;
    if (Bmax < Amax) Amax = Bmax;

    intersection.x = Amin;
    intersection.w = Amax - Amin;

    has_horiz_intersection = (Amax > Amin);

    // Vertical intersection
    Amin = A.y;
    Amax = Amin + A.h;
    Bmin = B.y;
    Bmax = Bmin + B.h;
    if (Bmin > Amin) Amin = Bmin;
    if (Bmax < Amax) Amax = Bmax;

    intersection.y = Amin;
    intersection.h = Amax - Amin;

    if (has_horiz_intersection && Amax > Amin) {
        if (result != NULL) *result = intersection;
        return true;
    } else
        return false;
}

bool METAENGINE_Render_IntersectClipRect(METAENGINE_Render_Target *target, METAENGINE_Render_Rect B,
                                         METAENGINE_Render_Rect *result) {
    if (target == NULL) return false;

    if (!target->use_clip_rect) {
        METAENGINE_Render_Rect A = {0, 0, static_cast<float>(target->w),
                                    static_cast<float>(target->h)};
        return METAENGINE_Render_IntersectRect(A, B, result);
    }

    return METAENGINE_Render_IntersectRect(target->clip_rect, B, result);
}

void METAENGINE_Render_SetColor(METAENGINE_Render_Image *image, METAENGINE_Color color) {
    if (image == NULL) return;

    image->color = color;
}

void METAENGINE_Render_SetRGB(METAENGINE_Render_Image *image, Uint8 r, Uint8 g, Uint8 b) {
    METAENGINE_Color c;
    c.r = r;
    c.g = g;
    c.b = b;
    GET_ALPHA(c) = 255;

    if (image == NULL) return;

    image->color = c;
}

void METAENGINE_Render_SetRGBA(METAENGINE_Render_Image *image, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    METAENGINE_Color c;
    c.r = r;
    c.g = g;
    c.b = b;
    GET_ALPHA(c) = a;

    if (image == NULL) return;

    image->color = c;
}

void METAENGINE_Render_UnsetColor(METAENGINE_Render_Image *image) {
    METAENGINE_Color c = {255, 255, 255, 255};
    if (image == NULL) return;

    image->color = c;
}

void METAENGINE_Render_SetTargetColor(METAENGINE_Render_Target *target, METAENGINE_Color color) {
    if (target == NULL) return;

    target->use_color = 1;
    target->color = color;
}

void METAENGINE_Render_SetTargetRGB(METAENGINE_Render_Target *target, Uint8 r, Uint8 g, Uint8 b) {
    METAENGINE_Color c;
    c.r = r;
    c.g = g;
    c.b = b;
    GET_ALPHA(c) = 255;

    if (target == NULL) return;

    target->use_color = !(r == 255 && g == 255 && b == 255);
    target->color = c;
}

void METAENGINE_Render_SetTargetRGBA(METAENGINE_Render_Target *target, Uint8 r, Uint8 g, Uint8 b,
                                     Uint8 a) {
    METAENGINE_Color c;
    c.r = r;
    c.g = g;
    c.b = b;
    GET_ALPHA(c) = a;

    if (target == NULL) return;

    target->use_color = !(r == 255 && g == 255 && b == 255 && a == 255);
    target->color = c;
}

void METAENGINE_Render_UnsetTargetColor(METAENGINE_Render_Target *target) {
    METAENGINE_Color c = {255, 255, 255, 255};
    if (target == NULL) return;

    target->use_color = false;
    target->color = c;
}

bool METAENGINE_Render_GetBlending(METAENGINE_Render_Image *image) {
    if (image == NULL) return false;

    return image->use_blending;
}

void METAENGINE_Render_SetBlending(METAENGINE_Render_Image *image, bool enable) {
    if (image == NULL) return;

    image->use_blending = enable;
}

void METAENGINE_Render_SetShapeBlending(bool enable) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->current_context_target->context->shapes_use_blending = enable;
}

METAENGINE_Render_BlendMode METAENGINE_Render_GetBlendModeFromPreset(
        METAENGINE_Render_BlendPresetEnum preset) {
    switch (preset) {
        case METAENGINE_Render_BLEND_NORMAL: {
            METAENGINE_Render_BlendMode b = {
                    METAENGINE_Render_FUNC_SRC_ALPHA, METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                    METAENGINE_Render_FUNC_SRC_ALPHA, METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                    METAENGINE_Render_EQ_ADD,         METAENGINE_Render_EQ_ADD};
            return b;
        } break;
        case METAENGINE_Render_BLEND_PREMULTIPLIED_ALPHA: {
            METAENGINE_Render_BlendMode b = {
                    METAENGINE_Render_FUNC_ONE, METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                    METAENGINE_Render_FUNC_ONE, METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                    METAENGINE_Render_EQ_ADD,   METAENGINE_Render_EQ_ADD};
            return b;
        } break;
        case METAENGINE_Render_BLEND_MULTIPLY: {
            METAENGINE_Render_BlendMode b = {
                    METAENGINE_Render_FUNC_DST_COLOR, METAENGINE_Render_FUNC_ZERO,
                    METAENGINE_Render_FUNC_SRC_ALPHA, METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                    METAENGINE_Render_EQ_ADD,         METAENGINE_Render_EQ_ADD};
            return b;
        } break;
        case METAENGINE_Render_BLEND_ADD: {
            METAENGINE_Render_BlendMode b = {
                    METAENGINE_Render_FUNC_SRC_ALPHA, METAENGINE_Render_FUNC_ONE,
                    METAENGINE_Render_FUNC_SRC_ALPHA, METAENGINE_Render_FUNC_ONE,
                    METAENGINE_Render_EQ_ADD,         METAENGINE_Render_EQ_ADD};
            return b;
        } break;
        case METAENGINE_Render_BLEND_SUBTRACT:
            // FIXME: Use src alpha for source components?
            {
                METAENGINE_Render_BlendMode b = {
                        METAENGINE_Render_FUNC_ONE,    METAENGINE_Render_FUNC_ONE,
                        METAENGINE_Render_FUNC_ONE,    METAENGINE_Render_FUNC_ONE,
                        METAENGINE_Render_EQ_SUBTRACT, METAENGINE_Render_EQ_SUBTRACT};
                return b;
            }
            break;
        case METAENGINE_Render_BLEND_MOD_ALPHA:
            // Don't disturb the colors, but multiply the dest alpha by the src alpha
            {
                METAENGINE_Render_BlendMode b = {
                        METAENGINE_Render_FUNC_ZERO, METAENGINE_Render_FUNC_ONE,
                        METAENGINE_Render_FUNC_ZERO, METAENGINE_Render_FUNC_SRC_ALPHA,
                        METAENGINE_Render_EQ_ADD,    METAENGINE_Render_EQ_ADD};
                return b;
            }
            break;
        case METAENGINE_Render_BLEND_SET_ALPHA:
            // Don't disturb the colors, but set the alpha to the src alpha
            {
                METAENGINE_Render_BlendMode b = {
                        METAENGINE_Render_FUNC_ZERO, METAENGINE_Render_FUNC_ONE,
                        METAENGINE_Render_FUNC_ONE,  METAENGINE_Render_FUNC_ZERO,
                        METAENGINE_Render_EQ_ADD,    METAENGINE_Render_EQ_ADD};
                return b;
            }
            break;
        case METAENGINE_Render_BLEND_SET: {
            METAENGINE_Render_BlendMode b = {
                    METAENGINE_Render_FUNC_ONE, METAENGINE_Render_FUNC_ZERO,
                    METAENGINE_Render_FUNC_ONE, METAENGINE_Render_FUNC_ZERO,
                    METAENGINE_Render_EQ_ADD,   METAENGINE_Render_EQ_ADD};
            return b;
        } break;
        case METAENGINE_Render_BLEND_NORMAL_KEEP_ALPHA: {
            METAENGINE_Render_BlendMode b = {
                    METAENGINE_Render_FUNC_SRC_ALPHA, METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                    METAENGINE_Render_FUNC_ZERO,      METAENGINE_Render_FUNC_ONE,
                    METAENGINE_Render_EQ_ADD,         METAENGINE_Render_EQ_ADD};
            return b;
        } break;
        case METAENGINE_Render_BLEND_NORMAL_ADD_ALPHA: {
            METAENGINE_Render_BlendMode b = {
                    METAENGINE_Render_FUNC_SRC_ALPHA, METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                    METAENGINE_Render_FUNC_ONE,       METAENGINE_Render_FUNC_ONE,
                    METAENGINE_Render_EQ_ADD,         METAENGINE_Render_EQ_ADD};
            return b;
        } break;
        case METAENGINE_Render_BLEND_NORMAL_FACTOR_ALPHA: {
            METAENGINE_Render_BlendMode b = {METAENGINE_Render_FUNC_SRC_ALPHA,
                                             METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                                             METAENGINE_Render_FUNC_ONE_MINUS_DST_ALPHA,
                                             METAENGINE_Render_FUNC_ONE,
                                             METAENGINE_Render_EQ_ADD,
                                             METAENGINE_Render_EQ_ADD};
            return b;
        } break;
        default:
            METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_USER_ERROR,
                                            "Blend preset not supported: %d", preset);
            {
                METAENGINE_Render_BlendMode b = {METAENGINE_Render_FUNC_SRC_ALPHA,
                                                 METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                                                 METAENGINE_Render_FUNC_SRC_ALPHA,
                                                 METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                                                 METAENGINE_Render_EQ_ADD,
                                                 METAENGINE_Render_EQ_ADD};
                return b;
            }
            break;
    }
}

void METAENGINE_Render_SetBlendFunction(METAENGINE_Render_Image *image,
                                        METAENGINE_Render_BlendFuncEnum source_color,
                                        METAENGINE_Render_BlendFuncEnum dest_color,
                                        METAENGINE_Render_BlendFuncEnum source_alpha,
                                        METAENGINE_Render_BlendFuncEnum dest_alpha) {
    if (image == NULL) return;

    image->blend_mode.source_color = source_color;
    image->blend_mode.dest_color = dest_color;
    image->blend_mode.source_alpha = source_alpha;
    image->blend_mode.dest_alpha = dest_alpha;
}

void METAENGINE_Render_SetBlendEquation(METAENGINE_Render_Image *image,
                                        METAENGINE_Render_BlendEqEnum color_equation,
                                        METAENGINE_Render_BlendEqEnum alpha_equation) {
    if (image == NULL) return;

    image->blend_mode.color_equation = color_equation;
    image->blend_mode.alpha_equation = alpha_equation;
}

void METAENGINE_Render_SetBlendMode(METAENGINE_Render_Image *image,
                                    METAENGINE_Render_BlendPresetEnum preset) {
    METAENGINE_Render_BlendMode b;
    if (image == NULL) return;

    b = METAENGINE_Render_GetBlendModeFromPreset(preset);
    METAENGINE_Render_SetBlendFunction(image, b.source_color, b.dest_color, b.source_alpha,
                                       b.dest_alpha);
    METAENGINE_Render_SetBlendEquation(image, b.color_equation, b.alpha_equation);
}

void METAENGINE_Render_SetShapeBlendFunction(METAENGINE_Render_BlendFuncEnum source_color,
                                             METAENGINE_Render_BlendFuncEnum dest_color,
                                             METAENGINE_Render_BlendFuncEnum source_alpha,
                                             METAENGINE_Render_BlendFuncEnum dest_alpha) {
    METAENGINE_Render_Context *context;
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    context = gpu_current_renderer->current_context_target->context;

    context->shapes_blend_mode.source_color = source_color;
    context->shapes_blend_mode.dest_color = dest_color;
    context->shapes_blend_mode.source_alpha = source_alpha;
    context->shapes_blend_mode.dest_alpha = dest_alpha;
}

void METAENGINE_Render_SetShapeBlendEquation(METAENGINE_Render_BlendEqEnum color_equation,
                                             METAENGINE_Render_BlendEqEnum alpha_equation) {
    METAENGINE_Render_Context *context;
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    context = gpu_current_renderer->current_context_target->context;

    context->shapes_blend_mode.color_equation = color_equation;
    context->shapes_blend_mode.alpha_equation = alpha_equation;
}

void METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BlendPresetEnum preset) {
    METAENGINE_Render_BlendMode b;
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    b = METAENGINE_Render_GetBlendModeFromPreset(preset);
    METAENGINE_Render_SetShapeBlendFunction(b.source_color, b.dest_color, b.source_alpha,
                                            b.dest_alpha);
    METAENGINE_Render_SetShapeBlendEquation(b.color_equation, b.alpha_equation);
}

void METAENGINE_Render_SetImageFilter(METAENGINE_Render_Image *image,
                                      METAENGINE_Render_FilterEnum filter) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;
    if (image == NULL) return;

    gpu_current_renderer->impl->SetImageFilter(gpu_current_renderer, image, filter);
}

void METAENGINE_Render_SetDefaultAnchor(float anchor_x, float anchor_y) {
    if (gpu_current_renderer == NULL) return;

    gpu_current_renderer->default_image_anchor_x = anchor_x;
    gpu_current_renderer->default_image_anchor_y = anchor_y;
}

void METAENGINE_Render_GetDefaultAnchor(float *anchor_x, float *anchor_y) {
    if (gpu_current_renderer == NULL) return;

    if (anchor_x != NULL) *anchor_x = gpu_current_renderer->default_image_anchor_x;

    if (anchor_y != NULL) *anchor_y = gpu_current_renderer->default_image_anchor_y;
}

void METAENGINE_Render_SetAnchor(METAENGINE_Render_Image *image, float anchor_x, float anchor_y) {
    if (image == NULL) return;

    image->anchor_x = anchor_x;
    image->anchor_y = anchor_y;
}

void METAENGINE_Render_GetAnchor(METAENGINE_Render_Image *image, float *anchor_x, float *anchor_y) {
    if (image == NULL) return;

    if (anchor_x != NULL) *anchor_x = image->anchor_x;

    if (anchor_y != NULL) *anchor_y = image->anchor_y;
}

METAENGINE_Render_SnapEnum METAENGINE_Render_GetSnapMode(METAENGINE_Render_Image *image) {
    if (image == NULL) return (METAENGINE_Render_SnapEnum) 0;

    return image->snap_mode;
}

void METAENGINE_Render_SetSnapMode(METAENGINE_Render_Image *image,
                                   METAENGINE_Render_SnapEnum mode) {
    if (image == NULL) return;

    image->snap_mode = mode;
}

void METAENGINE_Render_SetWrapMode(METAENGINE_Render_Image *image,
                                   METAENGINE_Render_WrapEnum wrap_mode_x,
                                   METAENGINE_Render_WrapEnum wrap_mode_y) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;
    if (image == NULL) return;

    gpu_current_renderer->impl->SetWrapMode(gpu_current_renderer, image, wrap_mode_x, wrap_mode_y);
}

METAENGINE_Render_TextureHandle METAENGINE_Render_GetTextureHandle(METAENGINE_Render_Image *image) {
    if (image == NULL || image->renderer == NULL) return 0;
    return image->renderer->impl->GetTextureHandle(image->renderer, image);
}

METAENGINE_Color METAENGINE_Render_GetPixel(METAENGINE_Render_Target *target, Sint16 x, Sint16 y) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL) {
        METAENGINE_Color c = {0, 0, 0, 0};
        return c;
    }

    return gpu_current_renderer->impl->GetPixel(gpu_current_renderer, target, x, y);
}

void METAENGINE_Render_Clear(METAENGINE_Render_Target *target) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    gpu_current_renderer->impl->ClearRGBA(gpu_current_renderer, target, 0, 0, 0, 0);
}

void METAENGINE_Render_ClearColor(METAENGINE_Render_Target *target, METAENGINE_Color color) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    gpu_current_renderer->impl->ClearRGBA(gpu_current_renderer, target, color.r, color.g, color.b,
                                          GET_ALPHA(color));
}

void METAENGINE_Render_ClearRGB(METAENGINE_Render_Target *target, Uint8 r, Uint8 g, Uint8 b) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    gpu_current_renderer->impl->ClearRGBA(gpu_current_renderer, target, r, g, b, 255);
}

void METAENGINE_Render_ClearRGBA(METAENGINE_Render_Target *target, Uint8 r, Uint8 g, Uint8 b,
                                 Uint8 a) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    gpu_current_renderer->impl->ClearRGBA(gpu_current_renderer, target, r, g, b, a);
}

void METAENGINE_Render_FlushBlitBuffer(void) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->FlushBlitBuffer(gpu_current_renderer);
}

void METAENGINE_Render_Flip(METAENGINE_Render_Target *target) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");

    if (target != NULL && target->context == NULL) {
        gpu_current_renderer->impl->FlushBlitBuffer(gpu_current_renderer);
        return;
    }

    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    gpu_current_renderer->impl->Flip(gpu_current_renderer, target);
}

// Shader API

Uint32 METAENGINE_Render_CompileShader(METAENGINE_Render_ShaderEnum shader_type,
                                       const char *shader_source) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return 0;

    return gpu_current_renderer->impl->CompileShader(gpu_current_renderer, shader_type,
                                                     shader_source);
}

bool METAENGINE_Render_LinkShaderProgram(Uint32 program_object) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return false;

    return gpu_current_renderer->impl->LinkShaderProgram(gpu_current_renderer, program_object);
}

Uint32 METAENGINE_Render_CreateShaderProgram(void) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return 0;

    return gpu_current_renderer->impl->CreateShaderProgram(gpu_current_renderer);
}

Uint32 METAENGINE_Render_LinkShaders(Uint32 shader_object1, Uint32 shader_object2) {
    Uint32 shaders[2];
    shaders[0] = shader_object1;
    shaders[1] = shader_object2;
    return METAENGINE_Render_LinkManyShaders(shaders, 2);
}

Uint32 METAENGINE_Render_LinkManyShaders(Uint32 *shader_objects, int count) {
    Uint32 p;
    int i;

    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return 0;

    if ((gpu_current_renderer->enabled_features & METAENGINE_Render_FEATURE_BASIC_SHADERS) !=
        METAENGINE_Render_FEATURE_BASIC_SHADERS)
        return 0;

    p = gpu_current_renderer->impl->CreateShaderProgram(gpu_current_renderer);

    for (i = 0; i < count; i++)
        gpu_current_renderer->impl->AttachShader(gpu_current_renderer, p, shader_objects[i]);

    if (gpu_current_renderer->impl->LinkShaderProgram(gpu_current_renderer, p)) return p;

    gpu_current_renderer->impl->FreeShaderProgram(gpu_current_renderer, p);
    return 0;
}

void METAENGINE_Render_FreeShader(Uint32 shader_object) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->FreeShader(gpu_current_renderer, shader_object);
}

void METAENGINE_Render_FreeShaderProgram(Uint32 program_object) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->FreeShaderProgram(gpu_current_renderer, program_object);
}

void METAENGINE_Render_AttachShader(Uint32 program_object, Uint32 shader_object) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->AttachShader(gpu_current_renderer, program_object, shader_object);
}

void METAENGINE_Render_DetachShader(Uint32 program_object, Uint32 shader_object) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->DetachShader(gpu_current_renderer, program_object, shader_object);
}

bool METAENGINE_Render_IsDefaultShaderProgram(Uint32 program_object) {
    METAENGINE_Render_Context *context;

    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return false;

    context = gpu_current_renderer->current_context_target->context;
    return (program_object == context->default_textured_shader_program ||
            program_object == context->default_untextured_shader_program);
}

void METAENGINE_Render_ActivateShaderProgram(Uint32 program_object,
                                             METAENGINE_Render_ShaderBlock *block) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->ActivateShaderProgram(gpu_current_renderer, program_object, block);
}

void METAENGINE_Render_DeactivateShaderProgram(void) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->DeactivateShaderProgram(gpu_current_renderer);
}

const char *METAENGINE_Render_GetShaderMessage(void) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return gpu_current_renderer->impl->GetShaderMessage(gpu_current_renderer);
}

int METAENGINE_Render_GetAttributeLocation(Uint32 program_object, const char *attrib_name) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return 0;

    return gpu_current_renderer->impl->GetAttributeLocation(gpu_current_renderer, program_object,
                                                            attrib_name);
}

METAENGINE_Render_AttributeFormat METAENGINE_Render_MakeAttributeFormat(
        int num_elems_per_vertex, METAENGINE_Render_TypeEnum type, bool normalize, int stride_bytes,
        int offset_bytes) {
    METAENGINE_Render_AttributeFormat f;
    f.is_per_sprite = false;
    f.num_elems_per_value = num_elems_per_vertex;
    f.type = type;
    f.normalize = normalize;
    f.stride_bytes = stride_bytes;
    f.offset_bytes = offset_bytes;
    return f;
}

METAENGINE_Render_Attribute METAENGINE_Render_MakeAttribute(
        int location, void *values, METAENGINE_Render_AttributeFormat format) {
    METAENGINE_Render_Attribute a;
    a.location = location;
    a.values = values;
    a.format = format;
    return a;
}

int METAENGINE_Render_GetUniformLocation(Uint32 program_object, const char *uniform_name) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return 0;

    return gpu_current_renderer->impl->GetUniformLocation(gpu_current_renderer, program_object,
                                                          uniform_name);
}

METAENGINE_Render_ShaderBlock METAENGINE_Render_LoadShaderBlock(Uint32 program_object,
                                                                const char *position_name,
                                                                const char *texcoord_name,
                                                                const char *color_name,
                                                                const char *modelViewMatrix_name) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL) {
        METAENGINE_Render_ShaderBlock b;
        b.position_loc = -1;
        b.texcoord_loc = -1;
        b.color_loc = -1;
        b.modelViewProjection_loc = -1;
        return b;
    }

    return gpu_current_renderer->impl->LoadShaderBlock(gpu_current_renderer, program_object,
                                                       position_name, texcoord_name, color_name,
                                                       modelViewMatrix_name);
}

void METAENGINE_Render_SetShaderBlock(METAENGINE_Render_ShaderBlock block) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->current_context_target->context->current_shader_block = block;
}

METAENGINE_Render_ShaderBlock METAENGINE_Render_GetShaderBlock(void) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL) {
        METAENGINE_Render_ShaderBlock b;
        b.position_loc = -1;
        b.texcoord_loc = -1;
        b.color_loc = -1;
        b.modelViewProjection_loc = -1;
        return b;
    }

    return gpu_current_renderer->current_context_target->context->current_shader_block;
}

void METAENGINE_Render_SetShaderImage(METAENGINE_Render_Image *image, int location,
                                      int image_unit) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->SetShaderImage(gpu_current_renderer, image, location, image_unit);
}

void METAENGINE_Render_GetUniformiv(Uint32 program_object, int location, int *values) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->GetUniformiv(gpu_current_renderer, program_object, location,
                                             values);
}

void METAENGINE_Render_SetUniformi(int location, int value) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->SetUniformi(gpu_current_renderer, location, value);
}

void METAENGINE_Render_SetUniformiv(int location, int num_elements_per_value, int num_values,
                                    int *values) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->SetUniformiv(gpu_current_renderer, location, num_elements_per_value,
                                             num_values, values);
}

void METAENGINE_Render_GetUniformuiv(Uint32 program_object, int location, unsigned int *values) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->GetUniformuiv(gpu_current_renderer, program_object, location,
                                              values);
}

void METAENGINE_Render_SetUniformui(int location, unsigned int value) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->SetUniformui(gpu_current_renderer, location, value);
}

void METAENGINE_Render_SetUniformuiv(int location, int num_elements_per_value, int num_values,
                                     unsigned int *values) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->SetUniformuiv(gpu_current_renderer, location,
                                              num_elements_per_value, num_values, values);
}

void METAENGINE_Render_GetUniformfv(Uint32 program_object, int location, float *values) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->GetUniformfv(gpu_current_renderer, program_object, location,
                                             values);
}

void METAENGINE_Render_SetUniformf(int location, float value) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->SetUniformf(gpu_current_renderer, location, value);
}

void METAENGINE_Render_SetUniformfv(int location, int num_elements_per_value, int num_values,
                                    float *values) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->SetUniformfv(gpu_current_renderer, location, num_elements_per_value,
                                             num_values, values);
}

// Same as METAENGINE_Render_GetUniformfv()
void METAENGINE_Render_GetUniformMatrixfv(Uint32 program_object, int location, float *values) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->GetUniformfv(gpu_current_renderer, program_object, location,
                                             values);
}

void METAENGINE_Render_SetUniformMatrixfv(int location, int num_matrices, int num_rows,
                                          int num_columns, bool transpose, float *values) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->SetUniformMatrixfv(gpu_current_renderer, location, num_matrices,
                                                   num_rows, num_columns, transpose, values);
}

void METAENGINE_Render_SetAttributef(int location, float value) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->SetAttributef(gpu_current_renderer, location, value);
}

void METAENGINE_Render_SetAttributei(int location, int value) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->SetAttributei(gpu_current_renderer, location, value);
}

void METAENGINE_Render_SetAttributeui(int location, unsigned int value) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->SetAttributeui(gpu_current_renderer, location, value);
}

void METAENGINE_Render_SetAttributefv(int location, int num_elements, float *value) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->SetAttributefv(gpu_current_renderer, location, num_elements, value);
}

void METAENGINE_Render_SetAttributeiv(int location, int num_elements, int *value) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->SetAttributeiv(gpu_current_renderer, location, num_elements, value);
}

void METAENGINE_Render_SetAttributeuiv(int location, int num_elements, unsigned int *value) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->SetAttributeuiv(gpu_current_renderer, location, num_elements,
                                                value);
}

void METAENGINE_Render_SetAttributeSource(int num_values, METAENGINE_Render_Attribute source) {
    if (gpu_current_renderer == NULL || gpu_current_renderer->current_context_target == NULL)
        return;

    gpu_current_renderer->impl->SetAttributeSource(gpu_current_renderer, num_values, source);
}

// gpu_strcasecmp()
// A portable strcasecmp() from UC Berkeley
/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific written prior permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

/*
 * This array is designed for mapping upper and lower case letter
 * together for a case independent comparison.  The mappings are
 * based upon ascii character sequences.
 */
static const char caseless_charmap[] = {
        '\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007', '\010', '\011', '\012',
        '\013', '\014', '\015', '\016', '\017', '\020', '\021', '\022', '\023', '\024', '\025',
        '\026', '\027', '\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037', '\040',
        '\041', '\042', '\043', '\044', '\045', '\046', '\047', '\050', '\051', '\052', '\053',
        '\054', '\055', '\056', '\057', '\060', '\061', '\062', '\063', '\064', '\065', '\066',
        '\067', '\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077', '\100', '\141',
        '\142', '\143', '\144', '\145', '\146', '\147', '\150', '\151', '\152', '\153', '\154',
        '\155', '\156', '\157', '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
        '\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137', '\140', '\141', '\142',
        '\143', '\144', '\145', '\146', '\147', '\150', '\151', '\152', '\153', '\154', '\155',
        '\156', '\157', '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167', '\170',
        '\171', '\172', '\173', '\174', '\175', '\176', '\177', '\200', '\201', '\202', '\203',
        '\204', '\205', '\206', '\207', '\210', '\211', '\212', '\213', '\214', '\215', '\216',
        '\217', '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227', '\230', '\231',
        '\232', '\233', '\234', '\235', '\236', '\237', '\240', '\241', '\242', '\243', '\244',
        '\245', '\246', '\247', '\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
        '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267', '\270', '\271', '\272',
        '\273', '\274', '\275', '\276', '\277', '\300', '\341', '\342', '\343', '\344', '\345',
        '\346', '\347', '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357', '\360',
        '\361', '\362', '\363', '\364', '\365', '\366', '\367', '\370', '\371', '\372', '\333',
        '\334', '\335', '\336', '\337', '\340', '\341', '\342', '\343', '\344', '\345', '\346',
        '\347', '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357', '\360', '\361',
        '\362', '\363', '\364', '\365', '\366', '\367', '\370', '\371', '\372', '\373', '\374',
        '\375', '\376', '\377',
};

int gpu_strcasecmp(const char *s1, const char *s2) {
    unsigned char u1, u2;

    do {
        u1 = (unsigned char) *s1++;
        u2 = (unsigned char) *s2++;
        if (caseless_charmap[u1] != caseless_charmap[u2])
            return caseless_charmap[u1] - caseless_charmap[u2];
    } while (u1 != '\0');

    return 0;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

METAENGINE_Render_MatrixStack *METAENGINE_Render_CreateMatrixStack(void) {
    METAENGINE_Render_MatrixStack *stack =
            (METAENGINE_Render_MatrixStack *) SDL_malloc(sizeof(METAENGINE_Render_MatrixStack));
    stack->matrix = NULL;
    stack->size = 0;
    stack->storage_size = 0;
    METAENGINE_Render_InitMatrixStack(stack);
    return stack;
}

void METAENGINE_Render_FreeMatrixStack(METAENGINE_Render_MatrixStack *stack) {
    METAENGINE_Render_ClearMatrixStack(stack);
    SDL_free(stack);
}

void METAENGINE_Render_InitMatrixStack(METAENGINE_Render_MatrixStack *stack) {
    if (stack == NULL) return;

    if (stack->storage_size != 0) METAENGINE_Render_ClearMatrixStack(stack);

    stack->storage_size = 1;
    stack->size = 1;

    stack->matrix = (float **) SDL_malloc(sizeof(float *) * stack->storage_size);
    stack->matrix[0] = (float *) SDL_malloc(sizeof(float) * 16);
    METAENGINE_Render_MatrixIdentity(stack->matrix[0]);
}

void METAENGINE_Render_CopyMatrixStack(const METAENGINE_Render_MatrixStack *source,
                                       METAENGINE_Render_MatrixStack *dest) {
    unsigned int i;
    unsigned int matrix_size = sizeof(float) * 16;
    if (source == NULL || dest == NULL) return;

    METAENGINE_Render_ClearMatrixStack(dest);
    dest->matrix = (float **) SDL_malloc(sizeof(float *) * source->storage_size);
    for (i = 0; i < source->storage_size; ++i) {
        dest->matrix[i] = (float *) SDL_malloc(matrix_size);
        memcpy(dest->matrix[i], source->matrix[i], matrix_size);
    }
    dest->storage_size = source->storage_size;
}

void METAENGINE_Render_ClearMatrixStack(METAENGINE_Render_MatrixStack *stack) {
    unsigned int i;
    for (i = 0; i < stack->storage_size; ++i) { SDL_free(stack->matrix[i]); }
    SDL_free(stack->matrix);

    stack->matrix = NULL;
    stack->storage_size = 0;
}

void METAENGINE_Render_ResetProjection(METAENGINE_Render_Target *target) {
    if (target == NULL) return;

    bool invert = (target->image != NULL);

    // Set up default projection
    float *projection_matrix = METAENGINE_Render_GetTopMatrix(&target->projection_matrix);
    METAENGINE_Render_MatrixIdentity(projection_matrix);

    if (!invert ^ METAENGINE_Render_GetCoordinateMode())
        METAENGINE_Render_MatrixOrtho(projection_matrix, 0, target->w, target->h, 0,
                                      target->camera.z_near, target->camera.z_far);
    else
        METAENGINE_Render_MatrixOrtho(
                projection_matrix, 0, target->w, 0, target->h, target->camera.z_near,
                target->camera
                        .z_far);// Special inverted orthographic projection because tex coords are inverted already for render-to-texture
}

// Column-major
#define INDEX(row, col) ((col) *4 + (row))

float METAENGINE_Render_VectorLength(const float *vec3) {
    return sqrtf(vec3[0] * vec3[0] + vec3[1] * vec3[1] + vec3[2] * vec3[2]);
}

void METAENGINE_Render_VectorNormalize(float *vec3) {
    float mag = METAENGINE_Render_VectorLength(vec3);
    vec3[0] /= mag;
    vec3[1] /= mag;
    vec3[2] /= mag;
}

float METAENGINE_Render_VectorDot(const float *A, const float *B) {
    return A[0] * B[0] + A[1] * B[1] + A[2] * B[2];
}

void METAENGINE_Render_VectorCross(float *result, const float *A, const float *B) {
    result[0] = A[1] * B[2] - A[2] * B[1];
    result[1] = A[2] * B[0] - A[0] * B[2];
    result[2] = A[0] * B[1] - A[1] * B[0];
}

void METAENGINE_Render_VectorCopy(float *result, const float *A) {
    result[0] = A[0];
    result[1] = A[1];
    result[2] = A[2];
}

void METAENGINE_Render_VectorApplyMatrix(float *vec3, const float *matrix_4x4) {
    float x = matrix_4x4[0] * vec3[0] + matrix_4x4[4] * vec3[1] + matrix_4x4[8] * vec3[2] +
              matrix_4x4[12];
    float y = matrix_4x4[1] * vec3[0] + matrix_4x4[5] * vec3[1] + matrix_4x4[9] * vec3[2] +
              matrix_4x4[13];
    float z = matrix_4x4[2] * vec3[0] + matrix_4x4[6] * vec3[1] + matrix_4x4[10] * vec3[2] +
              matrix_4x4[14];
    float w = matrix_4x4[3] * vec3[0] + matrix_4x4[7] * vec3[1] + matrix_4x4[11] * vec3[2] +
              matrix_4x4[15];
    vec3[0] = x / w;
    vec3[1] = y / w;
    vec3[2] = z / w;
}

void METAENGINE_Render_Vector4ApplyMatrix(float *vec4, const float *matrix_4x4) {
    float x = matrix_4x4[0] * vec4[0] + matrix_4x4[4] * vec4[1] + matrix_4x4[8] * vec4[2] +
              matrix_4x4[12] * vec4[3];
    float y = matrix_4x4[1] * vec4[0] + matrix_4x4[5] * vec4[1] + matrix_4x4[9] * vec4[2] +
              matrix_4x4[13] * vec4[3];
    float z = matrix_4x4[2] * vec4[0] + matrix_4x4[6] * vec4[1] + matrix_4x4[10] * vec4[2] +
              matrix_4x4[14] * vec4[3];
    float w = matrix_4x4[3] * vec4[0] + matrix_4x4[7] * vec4[1] + matrix_4x4[11] * vec4[2] +
              matrix_4x4[15] * vec4[3];

    vec4[0] = x;
    vec4[1] = y;
    vec4[2] = z;
    vec4[3] = w;
    if (w != 0.0f) {
        vec4[0] = x / w;
        vec4[1] = y / w;
        vec4[2] = z / w;
        vec4[3] = 1;
    }
}

// Matrix math implementations based on Wayne Cochran's (wcochran) matrix.c

#define FILL_MATRIX_4x4(A, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15)   \
    A[0] = a0;                                                                                     \
    A[1] = a1;                                                                                     \
    A[2] = a2;                                                                                     \
    A[3] = a3;                                                                                     \
    A[4] = a4;                                                                                     \
    A[5] = a5;                                                                                     \
    A[6] = a6;                                                                                     \
    A[7] = a7;                                                                                     \
    A[8] = a8;                                                                                     \
    A[9] = a9;                                                                                     \
    A[10] = a10;                                                                                   \
    A[11] = a11;                                                                                   \
    A[12] = a12;                                                                                   \
    A[13] = a13;                                                                                   \
    A[14] = a14;                                                                                   \
    A[15] = a15;

void METAENGINE_Render_MatrixCopy(float *result, const float *A) {
    memcpy(result, A, 16 * sizeof(float));
}

void METAENGINE_Render_MatrixIdentity(float *result) {
    memset(result, 0, 16 * sizeof(float));
    result[0] = result[5] = result[10] = result[15] = 1;
}

void METAENGINE_Render_MatrixOrtho(float *result, float left, float right, float bottom, float top,
                                   float z_near, float z_far) {
    if (result == NULL) return;

    {
#ifdef ROW_MAJOR
        float A[16];
        FILL_MATRIX_4x4(A, 2 / (right - left), 0, 0, -(right + left) / (right - left), 0,
                        2 / (top - bottom), 0, -(top + bottom) / (top - bottom), 0, 0,
                        -2 / (z_far - z_near), -(z_far + z_near) / (z_far - z_near), 0, 0, 0, 1);
#else
        float A[16];
        FILL_MATRIX_4x4(A, 2 / (right - left), 0, 0, 0, 0, 2 / (top - bottom), 0, 0, 0, 0,
                        -2 / (z_far - z_near), 0, -(right + left) / (right - left),
                        -(top + bottom) / (top - bottom), -(z_far + z_near) / (z_far - z_near), 1);
#endif

        METAENGINE_Render_MultiplyAndAssign(result, A);
    }
}

void METAENGINE_Render_MatrixFrustum(float *result, float left, float right, float bottom,
                                     float top, float z_near, float z_far) {
    if (result == NULL) return;

    {
        float A[16];
        FILL_MATRIX_4x4(A, 2 * z_near / (right - left), 0, 0, 0, 0, 2 * z_near / (top - bottom), 0,
                        0, (right + left) / (right - left), (top + bottom) / (top - bottom),
                        -(z_far + z_near) / (z_far - z_near), -1, 0, 0,
                        -(2 * z_far * z_near) / (z_far - z_near), 0);

        METAENGINE_Render_MultiplyAndAssign(result, A);
    }
}

void METAENGINE_Render_MatrixPerspective(float *result, float fovy, float aspect, float z_near,
                                         float z_far) {
    float fW, fH;

    // Make it left-handed?
    fovy = -fovy;
    aspect = -aspect;

    fH = tanf((fovy / 360) * PI) * z_near;
    fW = fH * aspect;
    METAENGINE_Render_MatrixFrustum(result, -fW, fW, -fH, fH, z_near, z_far);
}

void METAENGINE_Render_MatrixLookAt(float *matrix, float eye_x, float eye_y, float eye_z,
                                    float target_x, float target_y, float target_z, float up_x,
                                    float up_y, float up_z) {
    float forward[3] = {target_x - eye_x, target_y - eye_y, target_z - eye_z};
    float up[3] = {up_x, up_y, up_z};
    float side[3];
    float view[16];

    METAENGINE_Render_VectorNormalize(forward);
    METAENGINE_Render_VectorNormalize(up);

    // Calculate sideways vector
    METAENGINE_Render_VectorCross(side, forward, up);

    // Calculate new up vector
    METAENGINE_Render_VectorCross(up, side, forward);

    // Set up view matrix
    view[0] = side[0];
    view[4] = side[1];
    view[8] = side[2];
    view[12] = 0.0f;

    view[1] = up[0];
    view[5] = up[1];
    view[9] = up[2];
    view[13] = 0.0f;

    view[2] = -forward[0];
    view[6] = -forward[1];
    view[10] = -forward[2];
    view[14] = 0.0f;

    view[3] = view[7] = view[11] = 0.0f;
    view[15] = 1.0f;

    METAENGINE_Render_MultiplyAndAssign(matrix, view);
    METAENGINE_Render_MatrixTranslate(matrix, -eye_x, -eye_y, -eye_z);
}

void METAENGINE_Render_MatrixTranslate(float *result, float x, float y, float z) {
    if (result == NULL) return;

    {
#ifdef ROW_MAJOR
        float A[16];
        FILL_MATRIX_4x4(A, 1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z, 0, 0, 0, 1);
#else
        float A[16];
        FILL_MATRIX_4x4(A, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1);
#endif

        METAENGINE_Render_MultiplyAndAssign(result, A);
    }
}

void METAENGINE_Render_MatrixScale(float *result, float sx, float sy, float sz) {
    if (result == NULL) return;

    {
        float A[16];
        FILL_MATRIX_4x4(A, sx, 0, 0, 0, 0, sy, 0, 0, 0, 0, sz, 0, 0, 0, 0, 1);

        METAENGINE_Render_MultiplyAndAssign(result, A);
    }
}

void METAENGINE_Render_MatrixRotate(float *result, float degrees, float x, float y, float z) {
    float p, radians, c, s, c_, zc_, yc_, xzc_, xyc_, yzc_, xs, ys, zs;

    if (result == NULL) return;

    p = 1 / sqrtf(x * x + y * y + z * z);
    x *= p;
    y *= p;
    z *= p;
    radians = degrees * RAD_PER_DEG;
    c = cosf(radians);
    s = sinf(radians);
    c_ = 1 - c;
    zc_ = z * c_;
    yc_ = y * c_;
    xzc_ = x * zc_;
    xyc_ = x * y * c_;
    yzc_ = y * zc_;
    xs = x * s;
    ys = y * s;
    zs = z * s;

    {
#ifdef ROW_MAJOR
        float A[16];
        FILL_MATRIX_4x4(A, x * x * c_ + c, xyc_ - zs, xzc_ + ys, 0, xyc_ + zs, y * yc_ + c,
                        yzc_ - xs, 0, xzc_ - ys, yzc_ + xs, z * zc_ + c, 0, 0, 0, 0, 1);
#else
        float A[16];
        FILL_MATRIX_4x4(A, x * x * c_ + c, xyc_ + zs, xzc_ - ys, 0, xyc_ - zs, y * yc_ + c,
                        yzc_ + xs, 0, xzc_ + ys, yzc_ - xs, z * zc_ + c, 0, 0, 0, 0, 1);
#endif

        METAENGINE_Render_MultiplyAndAssign(result, A);
    }
}

// Matrix multiply: result = A * B
void METAENGINE_Render_MatrixMultiply(float *result, const float *A, const float *B) {
    float(*matR)[4] = (float(*)[4]) result;
    float(*matA)[4] = (float(*)[4]) A;
    float(*matB)[4] = (float(*)[4]) B;
    matR[0][0] = matB[0][0] * matA[0][0] + matB[0][1] * matA[1][0] + matB[0][2] * matA[2][0] +
                 matB[0][3] * matA[3][0];
    matR[0][1] = matB[0][0] * matA[0][1] + matB[0][1] * matA[1][1] + matB[0][2] * matA[2][1] +
                 matB[0][3] * matA[3][1];
    matR[0][2] = matB[0][0] * matA[0][2] + matB[0][1] * matA[1][2] + matB[0][2] * matA[2][2] +
                 matB[0][3] * matA[3][2];
    matR[0][3] = matB[0][0] * matA[0][3] + matB[0][1] * matA[1][3] + matB[0][2] * matA[2][3] +
                 matB[0][3] * matA[3][3];
    matR[1][0] = matB[1][0] * matA[0][0] + matB[1][1] * matA[1][0] + matB[1][2] * matA[2][0] +
                 matB[1][3] * matA[3][0];
    matR[1][1] = matB[1][0] * matA[0][1] + matB[1][1] * matA[1][1] + matB[1][2] * matA[2][1] +
                 matB[1][3] * matA[3][1];
    matR[1][2] = matB[1][0] * matA[0][2] + matB[1][1] * matA[1][2] + matB[1][2] * matA[2][2] +
                 matB[1][3] * matA[3][2];
    matR[1][3] = matB[1][0] * matA[0][3] + matB[1][1] * matA[1][3] + matB[1][2] * matA[2][3] +
                 matB[1][3] * matA[3][3];
    matR[2][0] = matB[2][0] * matA[0][0] + matB[2][1] * matA[1][0] + matB[2][2] * matA[2][0] +
                 matB[2][3] * matA[3][0];
    matR[2][1] = matB[2][0] * matA[0][1] + matB[2][1] * matA[1][1] + matB[2][2] * matA[2][1] +
                 matB[2][3] * matA[3][1];
    matR[2][2] = matB[2][0] * matA[0][2] + matB[2][1] * matA[1][2] + matB[2][2] * matA[2][2] +
                 matB[2][3] * matA[3][2];
    matR[2][3] = matB[2][0] * matA[0][3] + matB[2][1] * matA[1][3] + matB[2][2] * matA[2][3] +
                 matB[2][3] * matA[3][3];
    matR[3][0] = matB[3][0] * matA[0][0] + matB[3][1] * matA[1][0] + matB[3][2] * matA[2][0] +
                 matB[3][3] * matA[3][0];
    matR[3][1] = matB[3][0] * matA[0][1] + matB[3][1] * matA[1][1] + matB[3][2] * matA[2][1] +
                 matB[3][3] * matA[3][1];
    matR[3][2] = matB[3][0] * matA[0][2] + matB[3][1] * matA[1][2] + matB[3][2] * matA[2][2] +
                 matB[3][3] * matA[3][2];
    matR[3][3] = matB[3][0] * matA[0][3] + matB[3][1] * matA[1][3] + matB[3][2] * matA[2][3] +
                 matB[3][3] * matA[3][3];
}

void METAENGINE_Render_MultiplyAndAssign(float *result, const float *B) {
    float temp[16];
    METAENGINE_Render_MatrixMultiply(temp, result, B);
    METAENGINE_Render_MatrixCopy(result, temp);
}

// Can be used up to two times per line evaluation...
const char *METAENGINE_Render_GetMatrixString(const float *A) {
    static char buffer[512];
    static char buffer2[512];
    static char flip = 0;

    char *b = (flip ? buffer : buffer2);
    flip = !flip;

    snprintf(b, 512,
             "%.1f %.1f %.1f %.1f\n"
             "%.1f %.1f %.1f %.1f\n"
             "%.1f %.1f %.1f %.1f\n"
             "%.1f %.1f %.1f %.1f",
             A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[8], A[9], A[10], A[11], A[12], A[13],
             A[14], A[15]);
    return b;
}

void METAENGINE_Render_MatrixMode(METAENGINE_Render_Target *target, int matrix_mode) {
    METAENGINE_Render_Target *context_target;
    if (target == NULL) return;

    METAENGINE_Render_FlushBlitBuffer();
    target->matrix_mode = matrix_mode;

    context_target = METAENGINE_Render_GetContextTarget();
    if (context_target != NULL && context_target == target->context_target)
        context_target->context->active_target = target;
}

float *METAENGINE_Render_GetModel(void) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL) return NULL;
    return METAENGINE_Render_GetTopMatrix(&target->model_matrix);
}

float *METAENGINE_Render_GetView(void) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL) return NULL;
    return METAENGINE_Render_GetTopMatrix(&target->view_matrix);
}

float *METAENGINE_Render_GetProjection(void) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL) return NULL;
    return METAENGINE_Render_GetTopMatrix(&target->projection_matrix);
}

float *METAENGINE_Render_GetCurrentMatrix(void) {
    METAENGINE_Render_MatrixStack *stack;
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL) return NULL;
    if (target->matrix_mode == METAENGINE_Render_MODEL) stack = &target->model_matrix;
    else if (target->matrix_mode == METAENGINE_Render_VIEW)
        stack = &target->view_matrix;
    else// if(target->matrix_mode == METAENGINE_Render_PROJECTION)
        stack = &target->projection_matrix;

    return METAENGINE_Render_GetTopMatrix(stack);
}

void METAENGINE_Render_PushMatrix(void) {
    METAENGINE_Render_MatrixStack *stack;
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL) return;

    if (target->matrix_mode == METAENGINE_Render_MODEL) stack = &target->model_matrix;
    else if (target->matrix_mode == METAENGINE_Render_VIEW)
        stack = &target->view_matrix;
    else// if(target->matrix_mode == METAENGINE_Render_PROJECTION)
        stack = &target->projection_matrix;

    if (stack->size + 1 >= stack->storage_size) {
        // Grow matrix stack (1, 6, 16, 36, ...)

        // Alloc new one
        unsigned int new_storage_size = stack->storage_size * 2 + 4;
        float **new_stack = (float **) SDL_malloc(sizeof(float *) * new_storage_size);
        unsigned int i;
        for (i = 0; i < new_storage_size; ++i) {
            new_stack[i] = (float *) SDL_malloc(sizeof(float) * 16);
        }
        // Copy old one
        for (i = 0; i < stack->size; ++i) {
            METAENGINE_Render_MatrixCopy(new_stack[i], stack->matrix[i]);
        }
        // Free old one
        for (i = 0; i < stack->storage_size; ++i) { SDL_free(stack->matrix[i]); }
        SDL_free(stack->matrix);

        // Switch to new one
        stack->storage_size = new_storage_size;
        stack->matrix = new_stack;
    }
    METAENGINE_Render_MatrixCopy(stack->matrix[stack->size], stack->matrix[stack->size - 1]);
    stack->size++;
}

void METAENGINE_Render_PopMatrix(void) {
    METAENGINE_Render_MatrixStack *stack;

    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL) return;

    // FIXME: Flushing here is not always necessary if this isn't the last target
    METAENGINE_Render_FlushBlitBuffer();

    if (target->matrix_mode == METAENGINE_Render_MODEL) stack = &target->model_matrix;
    else if (target->matrix_mode == METAENGINE_Render_VIEW)
        stack = &target->view_matrix;
    else//if(target->matrix_mode == METAENGINE_Render_PROJECTION)
        stack = &target->projection_matrix;

    if (stack->size == 0) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_USER_ERROR,
                                        "Matrix stack is empty.");
    } else if (stack->size == 1) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_USER_ERROR,
                                        "Matrix stack would become empty!");
    } else
        stack->size--;
}

void METAENGINE_Render_SetProjection(const float *A) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL || A == NULL) return;

    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixCopy(METAENGINE_Render_GetProjection(), A);
}

void METAENGINE_Render_SetModel(const float *A) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL || A == NULL) return;

    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixCopy(METAENGINE_Render_GetModel(), A);
}

void METAENGINE_Render_SetView(const float *A) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL || A == NULL) return;

    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixCopy(METAENGINE_Render_GetView(), A);
}

void METAENGINE_Render_SetProjectionFromStack(METAENGINE_Render_MatrixStack *stack) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL || stack == NULL) return;

    METAENGINE_Render_SetProjection(METAENGINE_Render_GetTopMatrix(stack));
}

void METAENGINE_Render_SetModelFromStack(METAENGINE_Render_MatrixStack *stack) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL || stack == NULL) return;

    METAENGINE_Render_SetModel(METAENGINE_Render_GetTopMatrix(stack));
}

void METAENGINE_Render_SetViewFromStack(METAENGINE_Render_MatrixStack *stack) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL || stack == NULL) return;

    METAENGINE_Render_SetView(METAENGINE_Render_GetTopMatrix(stack));
}

float *METAENGINE_Render_GetTopMatrix(METAENGINE_Render_MatrixStack *stack) {
    if (stack == NULL || stack->size == 0) return NULL;
    return stack->matrix[stack->size - 1];
}

void METAENGINE_Render_LoadIdentity(void) {
    float *result = METAENGINE_Render_GetCurrentMatrix();
    if (result == NULL) return;

    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixIdentity(result);
}

void METAENGINE_Render_LoadMatrix(const float *A) {
    float *result = METAENGINE_Render_GetCurrentMatrix();
    if (result == NULL) return;
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixCopy(result, A);
}

void METAENGINE_Render_Ortho(float left, float right, float bottom, float top, float z_near,
                             float z_far) {
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixOrtho(METAENGINE_Render_GetCurrentMatrix(), left, right, bottom, top,
                                  z_near, z_far);
}

void METAENGINE_Render_Frustum(float left, float right, float bottom, float top, float z_near,
                               float z_far) {
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixFrustum(METAENGINE_Render_GetCurrentMatrix(), left, right, bottom, top,
                                    z_near, z_far);
}

void METAENGINE_Render_Perspective(float fovy, float aspect, float z_near, float z_far) {
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixPerspective(METAENGINE_Render_GetCurrentMatrix(), fovy, aspect, z_near,
                                        z_far);
}

void METAENGINE_Render_LookAt(float eye_x, float eye_y, float eye_z, float target_x, float target_y,
                              float target_z, float up_x, float up_y, float up_z) {
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixLookAt(METAENGINE_Render_GetCurrentMatrix(), eye_x, eye_y, eye_z,
                                   target_x, target_y, target_z, up_x, up_y, up_z);
}

void METAENGINE_Render_Translate(float x, float y, float z) {
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixTranslate(METAENGINE_Render_GetCurrentMatrix(), x, y, z);
}

void METAENGINE_Render_Scale(float sx, float sy, float sz) {
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixScale(METAENGINE_Render_GetCurrentMatrix(), sx, sy, sz);
}

void METAENGINE_Render_Rotate(float degrees, float x, float y, float z) {
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixRotate(METAENGINE_Render_GetCurrentMatrix(), degrees, x, y, z);
}

void METAENGINE_Render_MultMatrix(const float *A) {
    float *result = METAENGINE_Render_GetCurrentMatrix();
    if (result == NULL) return;
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MultiplyAndAssign(result, A);
}

void METAENGINE_Render_GetModelViewProjection(float *result) {
    // MVP = P * V * M
    METAENGINE_Render_MatrixMultiply(result, METAENGINE_Render_GetProjection(),
                                     METAENGINE_Render_GetView());
    METAENGINE_Render_MultiplyAndAssign(result, METAENGINE_Render_GetModel());
}

#define CHECK_RENDERER()                                                                           \
    METAENGINE_Render_Renderer *renderer = METAENGINE_Render_GetCurrentRenderer();                 \
    if (renderer == NULL) return;

#define CHECK_RENDERER_1(ret)                                                                      \
    METAENGINE_Render_Renderer *renderer = METAENGINE_Render_GetCurrentRenderer();                 \
    if (renderer == NULL) return ret;

float METAENGINE_Render_SetLineThickness(float thickness) {
    CHECK_RENDERER_1(1.0f);
    return renderer->impl->SetLineThickness(renderer, thickness);
}

float METAENGINE_Render_GetLineThickness(void) {
    CHECK_RENDERER_1(1.0f);
    return renderer->impl->GetLineThickness(renderer);
}

void METAENGINE_Render_Pixel(METAENGINE_Render_Target *target, float x, float y,
                             METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->Pixel(renderer, target, x, y, color);
}

void METAENGINE_Render_Line(METAENGINE_Render_Target *target, float x1, float y1, float x2,
                            float y2, METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->Line(renderer, target, x1, y1, x2, y2, color);
}

void METAENGINE_Render_Arc(METAENGINE_Render_Target *target, float x, float y, float radius,
                           float start_angle, float end_angle, METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->Arc(renderer, target, x, y, radius, start_angle, end_angle, color);
}

void METAENGINE_Render_ArcFilled(METAENGINE_Render_Target *target, float x, float y, float radius,
                                 float start_angle, float end_angle, METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->ArcFilled(renderer, target, x, y, radius, start_angle, end_angle, color);
}

void METAENGINE_Render_Circle(METAENGINE_Render_Target *target, float x, float y, float radius,
                              METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->Circle(renderer, target, x, y, radius, color);
}

void METAENGINE_Render_CircleFilled(METAENGINE_Render_Target *target, float x, float y,
                                    float radius, METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->CircleFilled(renderer, target, x, y, radius, color);
}

void METAENGINE_Render_Ellipse(METAENGINE_Render_Target *target, float x, float y, float rx,
                               float ry, float degrees, METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->Ellipse(renderer, target, x, y, rx, ry, degrees, color);
}

void METAENGINE_Render_EllipseFilled(METAENGINE_Render_Target *target, float x, float y, float rx,
                                     float ry, float degrees, METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->EllipseFilled(renderer, target, x, y, rx, ry, degrees, color);
}

void METAENGINE_Render_Sector(METAENGINE_Render_Target *target, float x, float y,
                              float inner_radius, float outer_radius, float start_angle,
                              float end_angle, METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->Sector(renderer, target, x, y, inner_radius, outer_radius, start_angle,
                           end_angle, color);
}

void METAENGINE_Render_SectorFilled(METAENGINE_Render_Target *target, float x, float y,
                                    float inner_radius, float outer_radius, float start_angle,
                                    float end_angle, METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->SectorFilled(renderer, target, x, y, inner_radius, outer_radius, start_angle,
                                 end_angle, color);
}

void METAENGINE_Render_Tri(METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2,
                           float x3, float y3, METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->Tri(renderer, target, x1, y1, x2, y2, x3, y3, color);
}

void METAENGINE_Render_TriFilled(METAENGINE_Render_Target *target, float x1, float y1, float x2,
                                 float y2, float x3, float y3, METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->TriFilled(renderer, target, x1, y1, x2, y2, x3, y3, color);
}

void METAENGINE_Render_Rectangle(METAENGINE_Render_Target *target, float x1, float y1, float x2,
                                 float y2, METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->Rectangle(renderer, target, x1, y1, x2, y2, color);
}

void METAENGINE_Render_Rectangle2(METAENGINE_Render_Target *target, METAENGINE_Render_Rect rect,
                                  METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->Rectangle(renderer, target, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h,
                              color);
}

void METAENGINE_Render_RectangleFilled(METAENGINE_Render_Target *target, float x1, float y1,
                                       float x2, float y2, METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->RectangleFilled(renderer, target, x1, y1, x2, y2, color);
}

void METAENGINE_Render_RectangleFilled2(METAENGINE_Render_Target *target,
                                        METAENGINE_Render_Rect rect, METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->RectangleFilled(renderer, target, rect.x, rect.y, rect.x + rect.w,
                                    rect.y + rect.h, color);
}

void METAENGINE_Render_RectangleRound(METAENGINE_Render_Target *target, float x1, float y1,
                                      float x2, float y2, float radius, METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->RectangleRound(renderer, target, x1, y1, x2, y2, radius, color);
}

void METAENGINE_Render_RectangleRound2(METAENGINE_Render_Target *target,
                                       METAENGINE_Render_Rect rect, float radius,
                                       METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->RectangleRound(renderer, target, rect.x, rect.y, rect.x + rect.w,
                                   rect.y + rect.h, radius, color);
}

void METAENGINE_Render_RectangleRoundFilled(METAENGINE_Render_Target *target, float x1, float y1,
                                            float x2, float y2, float radius,
                                            METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->RectangleRoundFilled(renderer, target, x1, y1, x2, y2, radius, color);
}

void METAENGINE_Render_RectangleRoundFilled2(METAENGINE_Render_Target *target,
                                             METAENGINE_Render_Rect rect, float radius,
                                             METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->RectangleRoundFilled(renderer, target, rect.x, rect.y, rect.x + rect.w,
                                         rect.y + rect.h, radius, color);
}

void METAENGINE_Render_Polygon(METAENGINE_Render_Target *target, unsigned int num_vertices,
                               float *vertices, METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->Polygon(renderer, target, num_vertices, vertices, color);
}

void METAENGINE_Render_Polyline(METAENGINE_Render_Target *target, unsigned int num_vertices,
                                float *vertices, METAENGINE_Color color, bool close_loop) {
    CHECK_RENDERER();
    renderer->impl->Polyline(renderer, target, num_vertices, vertices, color, close_loop);
}

void METAENGINE_Render_PolygonFilled(METAENGINE_Render_Target *target, unsigned int num_vertices,
                                     float *vertices, METAENGINE_Color color) {
    CHECK_RENDERER();
    renderer->impl->PolygonFilled(renderer, target, num_vertices, vertices, color);
}

#define METAENGINE_Render_TO_STRING_GENERATOR(x)                                                   \
    case x:                                                                                        \
        return #x;                                                                                 \
        break;

const char *METAENGINE::GLEnumToString(GLenum e) {
    switch (e) {
        // shader:
        METAENGINE_Render_TO_STRING_GENERATOR(GL_VERTEX_SHADER);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_GEOMETRY_SHADER);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_FRAGMENT_SHADER);

        // buffer usage:
        METAENGINE_Render_TO_STRING_GENERATOR(GL_STREAM_DRAW);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_STREAM_READ);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_STREAM_COPY);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_STATIC_DRAW);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_STATIC_READ);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_STATIC_COPY);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_DYNAMIC_DRAW);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_DYNAMIC_READ);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_DYNAMIC_COPY);

        // errors:
        METAENGINE_Render_TO_STRING_GENERATOR(GL_NO_ERROR);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INVALID_ENUM);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INVALID_VALUE);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INVALID_OPERATION);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INVALID_FRAMEBUFFER_OPERATION);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_OUT_OF_MEMORY);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_STACK_UNDERFLOW);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_STACK_OVERFLOW);

        // types:
        METAENGINE_Render_TO_STRING_GENERATOR(GL_BYTE);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_BYTE);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SHORT);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_SHORT);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_FLOAT);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_FLOAT_VEC2);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_FLOAT_VEC3);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_FLOAT_VEC4);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_DOUBLE);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_DOUBLE_VEC2);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_DOUBLE_VEC3);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_DOUBLE_VEC4);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INT);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_VEC2);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_VEC3);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_VEC4);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_VEC2);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_VEC3);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_VEC4);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_BOOL);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_BOOL_VEC2);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_BOOL_VEC3);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_BOOL_VEC4);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_FLOAT_MAT2);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_FLOAT_MAT3);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_FLOAT_MAT4);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_FLOAT_MAT2x3);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_FLOAT_MAT2x4);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_FLOAT_MAT3x2);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_FLOAT_MAT3x4);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_FLOAT_MAT4x2);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_FLOAT_MAT4x3);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_DOUBLE_MAT2);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_DOUBLE_MAT3);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_DOUBLE_MAT4);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_DOUBLE_MAT2x3);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_DOUBLE_MAT2x4);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_DOUBLE_MAT3x2);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_DOUBLE_MAT3x4);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_DOUBLE_MAT4x2);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_DOUBLE_MAT4x3);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SAMPLER_1D);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SAMPLER_2D);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SAMPLER_3D);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SAMPLER_CUBE);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SAMPLER_1D_SHADOW);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SAMPLER_2D_SHADOW);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SAMPLER_1D_ARRAY);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SAMPLER_2D_ARRAY);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SAMPLER_1D_ARRAY_SHADOW);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SAMPLER_2D_ARRAY_SHADOW);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SAMPLER_2D_MULTISAMPLE);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SAMPLER_2D_MULTISAMPLE_ARRAY);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SAMPLER_CUBE_SHADOW);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SAMPLER_BUFFER);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SAMPLER_2D_RECT);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_SAMPLER_2D_RECT_SHADOW);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_SAMPLER_1D);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_SAMPLER_2D);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_SAMPLER_3D);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_SAMPLER_CUBE);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_SAMPLER_1D_ARRAY);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_SAMPLER_2D_ARRAY);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_SAMPLER_2D_MULTISAMPLE);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_SAMPLER_BUFFER);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_SAMPLER_2D_RECT);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_1D);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_2D);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_3D);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_CUBE);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_1D_ARRAY);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_2D_ARRAY);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_BUFFER);
        METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_2D_RECT);

        // METAENGINE_Render_TO_STRING_GENERATOR(GL_IMAGE_1D);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_IMAGE_2D);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_IMAGE_3D);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_IMAGE_2D_RECT);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_IMAGE_CUBE);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_IMAGE_BUFFER);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_IMAGE_1D_ARRAY);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_IMAGE_2D_ARRAY);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_IMAGE_2D_MULTISAMPLE);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_IMAGE_2D_MULTISAMPLE_ARRAY);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_IMAGE_1D);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_IMAGE_2D);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_IMAGE_3D);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_IMAGE_2D_RECT);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_IMAGE_CUBE);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_IMAGE_BUFFER);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_IMAGE_1D_ARRAY);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_IMAGE_2D_ARRAY);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_IMAGE_2D_MULTISAMPLE);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_1D);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_2D);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_3D);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_2D_RECT);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_CUBE);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_BUFFER);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_1D_ARRAY);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_2D_ARRAY);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY);
        // METAENGINE_Render_TO_STRING_GENERATOR(GL_UNSIGNED_INT_ATOMIC_COUNTER);
    }

    static char buffer[32];
    std::sprintf(buffer, "Unknown GLenum: (0x%04x)", e);
    return buffer;
}
#undef METAENGINE_Render_TO_STRING_GENERATOR

void METAENGINE::Detail::RenderUniformVariable(GLuint program, GLenum type, const char *name,
                                               GLint location) {
    static bool is_color = false;
    switch (type) {
        case GL_FLOAT:
            METAENGINE_Render_INTROSPECTION_GENERATE_VARIABLE_RENDER(
                    GLfloat, 1, GL_FLOAT, glGetUniformfv, glProgramUniform1fv, ImGui::DragFloat);
            break;

        case GL_FLOAT_VEC2:
            METAENGINE_Render_INTROSPECTION_GENERATE_VARIABLE_RENDER(
                    GLfloat, 2, GL_FLOAT_VEC2, glGetUniformfv, glProgramUniform2fv,
                    ImGui::DragFloat2);
            break;

        case GL_FLOAT_VEC3: {
            ImGui::Checkbox("##is_color", &is_color);
            ImGui::SameLine();
            ImGui::Text("GL_FLOAT_VEC3 %s", name);
            ImGui::SameLine();
            float value[3];
            glGetUniformfv(program, location, &value[0]);
            if ((!is_color && ImGui::DragFloat3("", &value[0])) ||
                (is_color && ImGui::ColorEdit3("Color", &value[0], ImGuiColorEditFlags_NoLabel)))
                glProgramUniform3fv(program, location, 1, &value[0]);
        } break;

        case GL_FLOAT_VEC4: {
            ImGui::Checkbox("##is_color", &is_color);
            ImGui::SameLine();
            ImGui::Text("GL_FLOAT_VEC4 %s", name);
            ImGui::SameLine();
            float value[4];
            glGetUniformfv(program, location, &value[0]);
            if ((!is_color && ImGui::DragFloat4("", &value[0])) ||
                (is_color &&
                 ImGui::ColorEdit4("Color", &value[0],
                                   ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar |
                                           ImGuiColorEditFlags_AlphaPreviewHalf)))
                glProgramUniform4fv(program, location, 1, &value[0]);
        } break;

        case GL_INT:
            METAENGINE_Render_INTROSPECTION_GENERATE_VARIABLE_RENDER(
                    GLint, 1, GL_INT, glGetUniformiv, glProgramUniform1iv, ImGui::DragInt);
            break;

        case GL_INT_VEC2:
            METAENGINE_Render_INTROSPECTION_GENERATE_VARIABLE_RENDER(
                    GLint, 2, GL_INT, glGetUniformiv, glProgramUniform2iv, ImGui::DragInt2);
            break;

        case GL_INT_VEC3:
            METAENGINE_Render_INTROSPECTION_GENERATE_VARIABLE_RENDER(
                    GLint, 3, GL_INT, glGetUniformiv, glProgramUniform3iv, ImGui::DragInt3);
            break;

        case GL_INT_VEC4:
            METAENGINE_Render_INTROSPECTION_GENERATE_VARIABLE_RENDER(
                    GLint, 4, GL_INT, glGetUniformiv, glProgramUniform4iv, ImGui::DragInt4);
            break;

        case GL_SAMPLER_2D:
            METAENGINE_Render_INTROSPECTION_GENERATE_VARIABLE_RENDER(
                    GLint, 1, GL_SAMPLER_2D, glGetUniformiv, glProgramUniform1iv, ImGui::DragInt);
            break;

        case GL_FLOAT_MAT2:
            METAENGINE_Render_INTROSPECTION_GENERATE_MATRIX_RENDER(
                    GLfloat, 2, 2, GL_FLOAT_MAT2, glGetUniformfv, glProgramUniformMatrix2fv,
                    ImGui::DragFloat2);
            break;

        case GL_FLOAT_MAT3:
            METAENGINE_Render_INTROSPECTION_GENERATE_MATRIX_RENDER(
                    GLfloat, 3, 3, GL_FLOAT_MAT3, glGetUniformfv, glProgramUniformMatrix3fv,
                    ImGui::DragFloat3);
            break;

        case GL_FLOAT_MAT4:
            METAENGINE_Render_INTROSPECTION_GENERATE_MATRIX_RENDER(
                    GLfloat, 4, 4, GL_FLOAT_MAT4, glGetUniformfv, glProgramUniformMatrix4fv,
                    ImGui::DragFloat4);
            break;

        case GL_FLOAT_MAT2x3:
            METAENGINE_Render_INTROSPECTION_GENERATE_MATRIX_RENDER(
                    GLfloat, 3, 2, GL_FLOAT_MAT2x3, glGetUniformfv, glProgramUniformMatrix2x3fv,
                    ImGui::DragFloat3);
            break;

        case GL_FLOAT_MAT2x4:
            METAENGINE_Render_INTROSPECTION_GENERATE_MATRIX_RENDER(
                    GLfloat, 4, 2, GL_FLOAT_MAT2x4, glGetUniformfv, glProgramUniformMatrix2x4fv,
                    ImGui::DragFloat4);
            break;

        case GL_FLOAT_MAT3x2:
            METAENGINE_Render_INTROSPECTION_GENERATE_MATRIX_RENDER(
                    GLfloat, 2, 3, GL_FLOAT_MAT3x2, glGetUniformfv, glProgramUniformMatrix3x2fv,
                    ImGui::DragFloat2);
            break;

        case GL_FLOAT_MAT3x4:
            METAENGINE_Render_INTROSPECTION_GENERATE_MATRIX_RENDER(
                    GLfloat, 4, 3, GL_FLOAT_MAT3x4, glGetUniformfv, glProgramUniformMatrix3x2fv,
                    ImGui::DragFloat4);
            break;

        default:
            ImGui::Text("%s has type %s, which isn't supported yet!", name, GLEnumToString(type));
            break;
    }
}

#undef METAENGINE_Render_INTROSPECTION_GENERATE_VARIABLE_RENDER
#undef METAENGINE_Render_INTROSPECTION_GENERATE_MATRIX_RENDER

float METAENGINE::Detail::GetScrollableHeight() { return ImGui::GetTextLineHeight() * 16; }

void METAENGINE::IntrospectShader(const char *label, GLuint program) {
    METADOT_ASSERT(label != nullptr, "The label supplied with program: {} is nullptr", program);
    METADOT_ASSERT(glIsProgram(program), "The program: {} is not a valid shader program", program);

    ImGui::PushID(label);
    if (ImGui::CollapsingHeader(label)) {
        // Uniforms
        ImGui::Indent();
        if (ImGui::CollapsingHeader("Uniforms", ImGuiTreeNodeFlags_DefaultOpen)) {
            GLint uniform_count;
            glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniform_count);

            // Read the length of the longest active uniform.
            GLint max_name_length;
            glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_name_length);

            static std::vector<char> name;
            name.resize(max_name_length);

            for (int i = 0; i < uniform_count; i++) {
                GLint ignored;
                GLenum type;
                glGetActiveUniform(program, i, max_name_length, nullptr, &ignored, &type,
                                   name.data());

                const auto location = glGetUniformLocation(program, name.data());
                ImGui::Indent();
                ImGui::PushID(i);
                ImGui::PushItemWidth(-1.0f);
                Detail::RenderUniformVariable(program, type, name.data(), location);
                ImGui::PopItemWidth();
                ImGui::PopID();
                ImGui::Unindent();
            }
        }
        ImGui::Unindent();

        // Shaders
        ImGui::Indent();
        if (ImGui::CollapsingHeader("Shaders")) {
            GLint shader_count;
            glGetProgramiv(program, GL_ATTACHED_SHADERS, &shader_count);

            static std::vector<GLuint> attached_shaders;
            attached_shaders.resize(shader_count);
            glGetAttachedShaders(program, shader_count, nullptr, attached_shaders.data());

            for (const auto &shader: attached_shaders) {
                GLint source_length = 0;
                glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &source_length);
                static std::vector<char> source;
                source.resize(source_length);
                glGetShaderSource(shader, source_length, nullptr, source.data());

                GLint type = 0;
                glGetShaderiv(shader, GL_SHADER_TYPE, &type);

                ImGui::Indent();
                auto string_type = GLEnumToString(type);
                ImGui::PushID(string_type);
                if (ImGui::CollapsingHeader(string_type)) {
                    auto y_size = std::min(ImGui::CalcTextSize(source.data()).y,
                                           Detail::GetScrollableHeight());
                    ImGui::InputTextMultiline("", source.data(), source.size(),
                                              ImVec2(-1.0f, y_size), ImGuiInputTextFlags_ReadOnly);
                }
                ImGui::PopID();
                ImGui::Unindent();
            }
        }
        ImGui::Unindent();
    }
    ImGui::PopID();
}

void METAENGINE::IntrospectVertexArray(const char *label, GLuint vao) {
    METADOT_ASSERT(label != nullptr, "The label supplied with VAO: %u is nullptr", vao);
    METADOT_ASSERT(glIsVertexArray(vao), "The VAO: %u is not a valid vertex array object", vao);

    ImGui::PushID(label);
    if (ImGui::CollapsingHeader(label)) {
        ImGui::Indent();

        // Get current bound vertex buffer object so we can reset it back once we are finished.
        GLint current_vbo = 0;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &current_vbo);

        // Get current bound vertex array object so we can reset it back once we are finished.
        GLint current_vao = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao);
        glBindVertexArray(vao);

        // Get the maximum number of vertex attributes,
        // minimum is 4, I have 16, means that whatever number of attributes is here, it should be reasonable to iterate over.
        GLint max_vertex_attribs = 0;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs);

        GLint ebo = 0;
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &ebo);

        // EBO Visualization
        char buffer[128];
        std::sprintf(buffer, "Element Array Buffer: %d", ebo);
        ImGui::PushID(buffer);
        if (ImGui::CollapsingHeader(buffer)) {
            ImGui::Indent();
            // Assuming unsigned int atm, as I have not found a way to get out the type of the element array buffer.
            int size = 0;
            glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
            size /= sizeof(GLuint);
            ImGui::Text("Size: %d", size);

            if (ImGui::TreeNode("Buffer Contents")) {
                // TODO: Find a better way to put this out on screen, because this solution will probably not scale good when we get a lot of indices.
                //       Possible solution: Make it into columns, like the VBO's, and present the indices as triangles.
                auto ptr = (GLuint *) glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY);
                for (int i = 0; i < size; i++) {
                    ImGui::Text("%u", ptr[i]);
                    ImGui::SameLine();
                    if ((i + 1) % 3 == 0) ImGui::NewLine();
                }

                glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

                ImGui::TreePop();
            }

            ImGui::Unindent();
        }
        ImGui::PopID();

        // VBO Visualization
        for (intptr_t i = 0; i < max_vertex_attribs; i++) {
            GLint enabled = 0;
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);

            if (!enabled) continue;

            std::sprintf(buffer, "Attribute: %" PRIdPTR "", i);
            ImGui::PushID(buffer);
            if (ImGui::CollapsingHeader(buffer)) {
                ImGui::Indent();
                // Display meta data
                GLint buffer = 0;
                glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &buffer);
                ImGui::Text("Buffer: %d", buffer);

                GLint type = 0;
                glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, &type);
                ImGui::Text("Type: %s", GLEnumToString(type));

                GLint dimensions = 0;
                glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &dimensions);
                ImGui::Text("Dimensions: %d", dimensions);

                // Need to bind buffer to get access to parameteriv, and for mapping later
                glBindBuffer(GL_ARRAY_BUFFER, buffer);

                GLint size = 0;
                glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
                ImGui::Text("Size in bytes: %d", size);

                GLint stride = 0;
                glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
                ImGui::Text("Stride in bytes: %d", stride);

                GLvoid *offset = nullptr;
                glGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &offset);
                ImGui::Text("Offset in bytes: %" PRIdPTR "", (intptr_t) offset);

                GLint usage = 0;
                glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_USAGE, &usage);
                ImGui::Text("Usage: %s", GLEnumToString(usage));

                // Create table with indexes and actual contents
                if (ImGui::TreeNode("Buffer Contents")) {
                    ImGui::BeginChild(ImGui::GetID("vbo contents"),
                                      ImVec2(-1.0f, Detail::GetScrollableHeight()), true,
                                      ImGuiWindowFlags_AlwaysVerticalScrollbar);
                    ImGui::Columns(dimensions + 1);
                    const char *descriptors[] = {"index", "x", "y", "z", "w"};
                    for (int j = 0; j < dimensions + 1; j++) {
                        ImGui::Text("%s", descriptors[j]);
                        ImGui::NextColumn();
                    }
                    ImGui::Separator();

                    auto ptr =
                            (char *) glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY) + (intptr_t) offset;
                    for (int j = 0, c = 0; j < size; j += stride, c++) {
                        ImGui::Text("%d", c);
                        ImGui::NextColumn();
                        for (int k = 0; k < dimensions; k++) {
                            switch (type) {
                                case GL_BYTE:
                                    ImGui::Text("% d", *(GLbyte *) &ptr[j + k * sizeof(GLbyte)]);
                                    break;
                                case GL_UNSIGNED_BYTE:
                                    ImGui::Text("%u", *(GLubyte *) &ptr[j + k * sizeof(GLubyte)]);
                                    break;
                                case GL_SHORT:
                                    ImGui::Text("% d", *(GLshort *) &ptr[j + k * sizeof(GLshort)]);
                                    break;
                                case GL_UNSIGNED_SHORT:
                                    ImGui::Text("%u", *(GLushort *) &ptr[j + k * sizeof(GLushort)]);
                                    break;
                                case GL_INT:
                                    ImGui::Text("% d", *(GLint *) &ptr[j + k * sizeof(GLint)]);
                                    break;
                                case GL_UNSIGNED_INT:
                                    ImGui::Text("%u", *(GLuint *) &ptr[j + k * sizeof(GLuint)]);
                                    break;
                                case GL_FLOAT:
                                    ImGui::Text("% f", *(GLfloat *) &ptr[j + k * sizeof(GLfloat)]);
                                    break;
                                case GL_DOUBLE:
                                    ImGui::Text("% f",
                                                *(GLdouble *) &ptr[j + k * sizeof(GLdouble)]);
                                    break;
                            }
                            ImGui::NextColumn();
                        }
                    }
                    glUnmapBuffer(GL_ARRAY_BUFFER);
                    ImGui::EndChild();
                    ImGui::TreePop();
                }
                ImGui::Unindent();
            }
            ImGui::PopID();
        }

        // Cleanup
        glBindVertexArray(current_vao);
        glBindBuffer(GL_ARRAY_BUFFER, current_vbo);

        ImGui::Unindent();
    }
    ImGui::PopID();
}

DebugDraw::DebugDraw(METAENGINE_Render_Target *target) {
    this->target = target;
    m_drawFlags = 0;
}

DebugDraw::~DebugDraw() {}

void DebugDraw::Create() {}

void DebugDraw::Destroy() {}

b2Vec2 DebugDraw::transform(const b2Vec2 &pt) {
    float x = ((pt.x) * scale + xOfs);
    float y = ((pt.y) * scale + yOfs);
    return b2Vec2(x, y);
}

METAENGINE_Color DebugDraw::convertColor(const b2Color &color) {
    return {(UInt8) (color.r * 255), (UInt8) (color.g * 255), (UInt8) (color.b * 255),
            (UInt8) (color.a * 255)};
}

void DebugDraw::DrawPolygon(const b2Vec2 *vertices, int32 vertexCount, const b2Color &color) {
    b2Vec2 *verts = new b2Vec2[vertexCount];

    for (int i = 0; i < vertexCount; i++) { verts[i] = transform(vertices[i]); }

    // the "(float*)verts" assumes a b2Vec2 is equal to two floats (which it is)
    METAENGINE_Render_Polygon(target, vertexCount, (float *) verts, convertColor(color));

    delete[] verts;
}

void DebugDraw::DrawSolidPolygon(const b2Vec2 *vertices, int32 vertexCount, const b2Color &color) {
    b2Vec2 *verts = new b2Vec2[vertexCount];

    for (int i = 0; i < vertexCount; i++) { verts[i] = transform(vertices[i]); }

    // the "(float*)verts" assumes a b2Vec2 is equal to two floats (which it is)
    METAENGINE_Color c2 = convertColor(color);
    c2.a *= 0.25;
    METAENGINE_Render_PolygonFilled(target, vertexCount, (float *) verts, c2);
    METAENGINE_Render_Polygon(target, vertexCount, (float *) verts, convertColor(color));

    delete[] verts;
}

void DebugDraw::DrawCircle(const b2Vec2 &center, float radius, const b2Color &color) {
    b2Vec2 tr = transform(center);
    METAENGINE_Render_Circle(target, tr.x, tr.y, radius * scale, convertColor(color));
}

void DebugDraw::DrawSolidCircle(const b2Vec2 &center, float radius, const b2Vec2 &axis,
                                const b2Color &color) {
    b2Vec2 tr = transform(center);
    METAENGINE_Render_CircleFilled(target, tr.x, tr.y, radius * scale, convertColor(color));
}

void DebugDraw::DrawSegment(const b2Vec2 &p1, const b2Vec2 &p2, const b2Color &color) {
    b2Vec2 tr1 = transform(p1);
    b2Vec2 tr2 = transform(p2);
    METAENGINE_Render_Line(target, tr1.x, tr1.y, tr2.x, tr2.y, convertColor(color));
}

void DebugDraw::DrawTransform(const b2Transform &xf) {
    const float k_axisScale = 8.0f;
    b2Vec2 p1 = xf.p, p2;
    b2Vec2 tr1 = transform(p1), tr2;

    p2 = p1 + k_axisScale * xf.q.GetXAxis();
    tr2 = transform(p2);
    METAENGINE_Render_Line(target, tr1.x, tr1.y, tr2.x, tr2.y, {0xff, 0x00, 0x00, 0xcc});

    p2 = p1 + k_axisScale * xf.q.GetYAxis();
    tr2 = transform(p2);
    METAENGINE_Render_Line(target, tr1.x, tr1.y, tr2.x, tr2.y, {0x00, 0xff, 0x00, 0xcc});
}

void DebugDraw::DrawPoint(const b2Vec2 &p, float size, const b2Color &color) {
    b2Vec2 tr = transform(p);
    METAENGINE_Render_CircleFilled(target, tr.x, tr.y, 2, convertColor(color));
}

void DebugDraw::DrawString(int x, int y, const char *string, ...) {}

void DebugDraw::DrawString(const b2Vec2 &p, const char *string, ...) {}

void DebugDraw::DrawAABB(b2AABB *aabb, const b2Color &color) {
    b2Vec2 tr1 = transform(aabb->lowerBound);
    b2Vec2 tr2 = transform(aabb->upperBound);
    METAENGINE_Render_Line(target, tr1.x, tr1.y, tr2.x, tr1.y, convertColor(color));
    METAENGINE_Render_Line(target, tr2.x, tr1.y, tr2.x, tr2.y, convertColor(color));
    METAENGINE_Render_Line(target, tr2.x, tr2.y, tr1.x, tr2.y, convertColor(color));
    METAENGINE_Render_Line(target, tr1.x, tr2.y, tr1.x, tr1.y, convertColor(color));
}