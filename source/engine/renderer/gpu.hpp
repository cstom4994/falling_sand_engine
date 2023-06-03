
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

#define ME_GL_STATE_BACKUP()                                                     \
    GLenum last_active_texture;                                                  \
    glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint *)&last_active_texture);             \
    glActiveTexture(GL_TEXTURE0);                                                \
    GLuint last_program;                                                         \
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *)&last_program);                   \
    GLuint last_texture;                                                         \
    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint *)&last_texture);                \
    GLuint last_array_buffer;                                                    \
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint *)&last_array_buffer);         \
    GLint last_viewport[4];                                                      \
    glGetIntegerv(GL_VIEWPORT, last_viewport);                                   \
    GLint last_scissor_box[4];                                                   \
    glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);                             \
    GLenum last_blend_src_rgb;                                                   \
    glGetIntegerv(GL_BLEND_SRC_RGB, (GLint *)&last_blend_src_rgb);               \
    GLenum last_blend_dst_rgb;                                                   \
    glGetIntegerv(GL_BLEND_DST_RGB, (GLint *)&last_blend_dst_rgb);               \
    GLenum last_blend_src_alpha;                                                 \
    glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint *)&last_blend_src_alpha);           \
    GLenum last_blend_dst_alpha;                                                 \
    glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint *)&last_blend_dst_alpha);           \
    GLenum last_blend_equation_rgb;                                              \
    glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint *)&last_blend_equation_rgb);     \
    GLenum last_blend_equation_alpha;                                            \
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint *)&last_blend_equation_alpha); \
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);                         \
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);                 \
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);               \
    GLboolean last_enable_stencil_test = glIsEnabled(GL_STENCIL_TEST);           \
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);           \
    GLboolean last_enable_mutisample = glIsEnabled(GL_MULTISAMPLE);              \
    GLboolean last_enable_framebuffer_srgb = glIsEnabled(GL_FRAMEBUFFER_SRGB)

#define ME_GL_STATE_RESTORE()                                                                                \
    glUseProgram(last_program);                                                                              \
    glBindTexture(GL_TEXTURE_2D, last_texture);                                                              \
    glActiveTexture(last_active_texture);                                                                    \
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);                                                        \
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);                             \
    glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha); \
    if (last_enable_blend)                                                                                   \
        glEnable(GL_BLEND);                                                                                  \
    else                                                                                                     \
        glDisable(GL_BLEND);                                                                                 \
    if (last_enable_cull_face)                                                                               \
        glEnable(GL_CULL_FACE);                                                                              \
    else                                                                                                     \
        glDisable(GL_CULL_FACE);                                                                             \
    if (last_enable_depth_test)                                                                              \
        glEnable(GL_DEPTH_TEST);                                                                             \
    else                                                                                                     \
        glDisable(GL_DEPTH_TEST);                                                                            \
    if (last_enable_stencil_test)                                                                            \
        glEnable(GL_STENCIL_TEST);                                                                           \
    else                                                                                                     \
        glDisable(GL_STENCIL_TEST);                                                                          \
    if (last_enable_scissor_test)                                                                            \
        glEnable(GL_SCISSOR_TEST);                                                                           \
    else                                                                                                     \
        glDisable(GL_SCISSOR_TEST);                                                                          \
    if (last_enable_mutisample)                                                                              \
        glEnable(GL_MULTISAMPLE);                                                                            \
    else                                                                                                     \
        glDisable(GL_MULTISAMPLE);                                                                           \
    if (last_enable_framebuffer_srgb)                                                                        \
        glEnable(GL_FRAMEBUFFER_SRGB);                                                                       \
    else                                                                                                     \
        glDisable(GL_FRAMEBUFFER_SRGB);                                                                      \
    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);    \
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3])

namespace MetaEngine {
class Drawing {
public:
    static MEvec2 rotate_point(float cx, float cy, float angle, MEvec2 p);
    static void drawPolygon(R_Target *renderer, ME_Color col, MEvec2 *verts, int x, int y, float scale, int count, float angle, float cx, float cy);
    static u32 darkenColor(u32 col, float brightness);
    static void drawText(std::string text, ME_Color col, int x, int y);
    static void drawTextWithPlate(R_Target *target, std::string text, ME_Color col, int x, int y, ME_Color backcolor = {77, 77, 77, 140});
};
}  // namespace MetaEngine

namespace ME {

class DebugOutputGL final {
public:
    DebugOutputGL() {}
    ~DebugOutputGL() {}

    static void GLerrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *msg, const void *data) {
        if (severity == GL_DEBUG_SEVERITY_NOTIFICATION || type == 0x8250) {
            return;
        }

        METADOT_BUG("Catch from GL debug output \n   Source: ", getStringForSource(source), "\n   Type: ", getStringForType(type), "\n   Severity: ", getStringForSeverity(severity),
                    "\n   DebugCall: ", msg);
    }

private:
    static std::string getStringForSource(GLenum source) {
        switch (source) {
            case GL_DEBUG_SOURCE_API:
                return "API";
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                return "Window system";
            case GL_DEBUG_SOURCE_SHADER_COMPILER:
                return "Shader compiler";
            case GL_DEBUG_SOURCE_THIRD_PARTY:
                return "Third party";
            case GL_DEBUG_SOURCE_APPLICATION:
                return "Application";
            case GL_DEBUG_SOURCE_OTHER:
                return "Other";
            default:
                assert(false);
                return "";
        }
    }

    static std::string getStringForType(GLenum type) {
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:
                return "Error";
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                return "Deprecated behavior";
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                return "Undefined behavior";
            case GL_DEBUG_TYPE_PORTABILITY:
                return "Portability issue";
            case GL_DEBUG_TYPE_PERFORMANCE:
                return "Performance issue";
            case GL_DEBUG_TYPE_MARKER:
                return "Stream annotation";
            case GL_DEBUG_TYPE_OTHER:
                return "Other";
            default:
                assert(false);
                return "";
        }
    }

    static std::string getStringForSeverity(GLenum severity) {
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
                return "High";
            case GL_DEBUG_SEVERITY_MEDIUM:
                return "Medium";
            case GL_DEBUG_SEVERITY_LOW:
                return "Low";
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                return "Notification";
            default:
                assert(false);
                return ("");
        }
    }
};

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

void render_uniform_variable(GLuint program, GLenum type, const char *name, GLint location);

float get_scrollable_height();

void inspect_shader(const char *label, GLuint program);
void inspect_vertex_array(const char *label, GLuint vao);
}  // namespace ME

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