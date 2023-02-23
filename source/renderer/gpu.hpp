
///////////////////////////////////////////////////////////////////////////////////////////////////
/// Implementation - Introspection (In development)
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _METADOT_GPU_HPP_
#define _METADOT_GPU_HPP_

#include <algorithm>
#include <cinttypes>
#include <vector>

#include "core/core.hpp"
#include "physics/box2d.h"
#include "libs/imgui/imgui.h"
#include "metadot_gl.h"
#include "renderer/renderer_gpu.h"

struct ShaderBase;

typedef struct engine_render {
    R_Target *realTarget;
    R_Target *target;
} engine_render;

namespace MetaEngine {
const char *GLEnumToString(GLenum e);

class Drawing {
public:
    static vec2 rotate_point(float cx, float cy, float angle, vec2 p);
    static void drawPolygon(R_Target *renderer, METAENGINE_Color col, vec2 *verts, int x, int y, float scale, int count, float angle, float cx, float cy);
    static U32 darkenColor(U32 col, float brightness);
    static void drawText(std::string text, METAENGINE_Color col, int x, int y);
    static void drawTextWithPlate(R_Target *target, std::string text, METAENGINE_Color col, int x, int y, METAENGINE_Color backcolor = {77, 77, 77, 140});

    static void draw_spinning_triangle(R_Target *screen, ShaderBase *shader);
    static void end_3d(R_Target *screen);
    static void begin_3d(R_Target *screen);
};

namespace Detail {
// Generator macro to avoid duplicating code all the time.
#define R_INTROSPECTION_GENERATE_VARIABLE_RENDER(cputype, count, gltype, glread, glwrite, imguifunc) \
    {                                                                                                \
        ImGui::Text(#gltype " %s:", name);                                                           \
        cputype value[count];                                                                        \
        glread(program, location, &value[0]);                                                        \
        if (imguifunc("", &value[0], 0.25f)) glwrite(program, location, 1, &value[0]);               \
    }

#define R_INTROSPECTION_GENERATE_MATRIX_RENDER(cputype, rows, columns, gltype, glread, glwrite, imguifunc) \
    {                                                                                                      \
        ImGui::Text(#gltype " %s:", name);                                                                 \
        cputype value[rows * columns];                                                                     \
        int size = rows * columns;                                                                         \
        glread(program, location, &value[0]);                                                              \
        int modified = 0;                                                                                  \
        for (int i = 0; i < size; i += rows) {                                                             \
            ImGui::PushID(i);                                                                              \
            modified += imguifunc("", &value[i], 0.25f);                                                   \
            ImGui::PopID();                                                                                \
        }                                                                                                  \
        if (modified) glwrite(program, location, 1, GL_FALSE, value);                                      \
    }

void RenderUniformVariable(GLuint program, GLenum type, const char *name, GLint location);

// #undef R_INTROSPECTION_GENERATE_VARIABLE_RENDER
// #undef R_INTROSPECTION_GENERATE_MATRIX_RENDER

float GetScrollableHeight();
}  // namespace Detail

void IntrospectShader(const char *label, GLuint program);
void IntrospectVertexArray(const char *label, GLuint vao);
}  // namespace MetaEngine

#if 1

class DebugDraw {

private:
    U32 m_drawFlags;

public:
    enum {
        e_shapeBit = 0x0001,        ///< draw shapes
        e_jointBit = 0x0002,        ///< draw joint connections
        e_aabbBit = 0x0004,         ///< draw axis aligned bounding boxes
        e_pairBit = 0x0008,         ///< draw broad-phase pairs
        e_centerOfMassBit = 0x0010  ///< draw center of mass frame
    };

    R_Target *target;
    float xOfs = 0;
    float yOfs = 0;
    float scale = 1;

    DebugDraw(R_Target *target);
    ~DebugDraw();

    void Create();
    void Destroy();

    void SetFlags(U32 flags) { m_drawFlags = flags; }
    U32 GetFlags() const { return m_drawFlags; }
    void AppendFlags(U32 flags) { m_drawFlags |= flags; }
    void ClearFlags(U32 flags) { m_drawFlags &= ~flags; }
    
    b2Vec2 transform(const b2Vec2 &pt);

    void DrawPolygon(const b2Vec2 *vertices, I32 vertexCount, const METAENGINE_Color &color);
    void DrawSolidPolygon(const b2Vec2 *vertices, I32 vertexCount, const METAENGINE_Color &color);
    void DrawCircle(const b2Vec2 &center, float radius, const METAENGINE_Color &color);
    void DrawSolidCircle(const b2Vec2 &center, float radius, const b2Vec2 &axis, const METAENGINE_Color &color);
    void DrawSegment(const b2Vec2 &p1, const b2Vec2 &p2, const METAENGINE_Color &color);
    void DrawTransform(const b2Transform &xf);
    void DrawPoint(const b2Vec2 &p, float size, const METAENGINE_Color &color);
    void DrawString(int x, int y, const char *string, ...);
    void DrawString(const b2Vec2 &p, const char *string, ...);
    void DrawAABB(b2AABB *aabb, const METAENGINE_Color &color);
};
#endif

#endif
