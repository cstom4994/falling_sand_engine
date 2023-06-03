
///////////////////////////////////////////////////////////////////////////////////////////////////
/// Implementation - Introspection (In development)
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef ME_GPU_HPP
#define ME_GPU_HPP

#include <algorithm>
#include <cinttypes>
#include <vector>

#include "engine/core/core.hpp"
#include "engine/core/utils/utility.hpp"
#include "engine/physics/box2d.h"
#include "engine/renderer/renderer_gpu.h"
#include "libs/glad/glad.h"
#include "libs/imgui/imgui.h"

#define R_GET_PIXEL(surface, x, y) *((u32 *)((u8 *)surface->pixels + ((y)*surface->pitch) + ((x) * sizeof(u32))))

struct ShaderBase;

typedef struct engine_render {
    R_Target *realTarget;
    R_Target *target;
} engine_render;

ME_PRIVATE(char const *) gl_error_string(GLenum const err) noexcept {
    switch (err) {
        // opengl 2 errors (8)
        case GL_NO_ERROR:
            return "GL_NO_ERROR";

        case GL_INVALID_ENUM:
            return "GL_INVALID_ENUM";

        case GL_INVALID_VALUE:
            return "GL_INVALID_VALUE";

        case GL_INVALID_OPERATION:
            return "GL_INVALID_OPERATION";

        case GL_STACK_OVERFLOW:
            return "GL_STACK_OVERFLOW";

        case GL_STACK_UNDERFLOW:
            return "GL_STACK_UNDERFLOW";

        case GL_OUT_OF_MEMORY:
            return "GL_OUT_OF_MEMORY";

        // case GL_TABLE_TOO_LARGE:
        //     return "GL_TABLE_TOO_LARGE";

        // opengl 3 errors (1)
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "GL_INVALID_FRAMEBUFFER_OPERATION";

        // gles 2, 3 and gl 4 error are handled by the switch above
        default:
            return std::format("unknown error {0}", err).c_str();
    }
}

ME_PRIVATE(void) ME_check_gl_error(const char *file, const int line) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        METADOT_ERROR(std::format("[Render] {0}({1}) {2}", file, line, gl_error_string(err)).c_str());
    }
}

#define ME_CHECK_GL_ERROR() ME_check_gl_error(__FILE__, __LINE__)

namespace MetaEngine {
const char *GLEnumToString(GLenum e);

class Drawing {
public:
    static MEvec2 rotate_point(float cx, float cy, float angle, MEvec2 p);
    static void drawPolygon(R_Target *renderer, ME_Color col, MEvec2 *verts, int x, int y, float scale, int count, float angle, float cx, float cy);
    static u32 darkenColor(u32 col, float brightness);
    static void drawText(std::string text, ME_Color col, int x, int y);
    static void drawTextWithPlate(R_Target *target, std::string text, ME_Color col, int x, int y, ME_Color backcolor = {77, 77, 77, 140});
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
    u32 m_drawFlags;

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

    void SetFlags(u32 flags) { m_drawFlags = flags; }
    u32 GetFlags() const { return m_drawFlags; }
    void AppendFlags(u32 flags) { m_drawFlags |= flags; }
    void ClearFlags(u32 flags) { m_drawFlags &= ~flags; }

    PVec2 transform(const PVec2 &pt);

    void DrawPolygon(const PVec2 *vertices, i32 vertexCount, const ME_Color &color);
    void DrawSolidPolygon(const PVec2 *vertices, i32 vertexCount, const ME_Color &color);
    void DrawCircle(const PVec2 &center, float radius, const ME_Color &color);
    void DrawSolidCircle(const PVec2 &center, float radius, const PVec2 &axis, const ME_Color &color);
    void DrawSegment(const PVec2 &p1, const PVec2 &p2, const ME_Color &color);
    void DrawTransform(const PTransform &xf);
    void DrawPoint(const PVec2 &p, float size, const ME_Color &color);
    void DrawString(int x, int y, const char *string, ...);
    void DrawString(const PVec2 &p, const char *string, ...);
    void DrawAABB(b2AABB *aabb, const ME_Color &color);
};
#endif

#endif
