
///////////////////////////////////////////////////////////////////////////////////////////////////
/// Implementation - Introspection (In development)
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cinttypes>
#include <vector>

#include "core/core.hpp"
#include "engine/internal/builtin_box2d.h"
#include "engine/renderer/renderer_gpu.h"
#include "imgui.h"
#include "libs/glad/glad.h"

namespace MetaEngine {
const char *GLEnumToString(GLenum e);

class Drawing {
public:
    static b2Vec2 rotate_point(float cx, float cy, float angle, b2Vec2 p);
    static void drawPolygon(R_Target *renderer, METAENGINE_Color col, b2Vec2 *verts, int x, int y, float scale, int count, float angle, float cx, float cy);
    static U32 darkenColor(U32 col, float brightness);
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

    METAENGINE_Color convertColor(const b2Color &color);

    void DrawPolygon(const b2Vec2 *vertices, I32 vertexCount, const b2Color &color);

    void DrawSolidPolygon(const b2Vec2 *vertices, I32 vertexCount, const b2Color &color);

    void DrawCircle(const b2Vec2 &center, float radius, const b2Color &color);

    void DrawSolidCircle(const b2Vec2 &center, float radius, const b2Vec2 &axis, const b2Color &color);

    void DrawSegment(const b2Vec2 &p1, const b2Vec2 &p2, const b2Color &color);

    void DrawTransform(const b2Transform &xf);

    void DrawPoint(const b2Vec2 &p, float size, const b2Color &color);

    void DrawString(int x, int y, const char *string, ...);

    void DrawString(const b2Vec2 &p, const char *string, ...);

    void DrawAABB(b2AABB *aabb, const b2Color &color);
};
#endif
