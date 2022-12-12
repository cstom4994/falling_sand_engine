// Copyright(c) 2022, KaoruXun

// This source file may include
// https://github.com/maildrop/DearImGui-with-IMM32 (MIT) by TOGURO Mikito
// https://github.com/mekhontsev/imgui_md (MIT) by mekhontsev
// https://github.com/ocornut/imgui (MIT) by Omar Cornut

#include "ImGuiImplement.hpp"
#include "Core/Core.hpp"
#include "Engine/Renderer/RendererGPU.h"
#include "Engine/SDLWrapper.h"
#include "Game/FileSystem.hpp"
#include "ImGuiImplement.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <string>
#include <utility>

#pragma region ImGuiImplementGL3

#define IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY

// Desktop GL 2.0+ has glPolygonMode() which GL ES and WebGL don't have.
#ifdef GL_POLYGON_MODE
#define IMGUI_IMPL_HAS_POLYGON_MODE
#endif

// Desktop GL 3.2+ has glDrawElementsBaseVertex() which GL ES and WebGL don't have.
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3) && defined(GL_VERSION_3_2)
#define IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
#endif

// Desktop GL 3.3+ has glBindSampler()
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3) && defined(GL_VERSION_3_3)
#define IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
#endif

// Desktop GL 3.1+ has GL_PRIMITIVE_RESTART state
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3) && defined(GL_VERSION_3_1)
#define IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
#endif

// Desktop GL use extension detection
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3)
#define IMGUI_IMPL_OPENGL_MAY_HAVE_EXTENSIONS
#endif

//#define IMGUI_IMPL_OPENGL_DEBUG
#ifdef IMGUI_IMPL_OPENGL_DEBUG
#include <stdio.h>
#define GL_CALL(_CALL)                                                                             \
    do {                                                                                           \
        _CALL;                                                                                     \
        GLenum gl_err = glGetError();                                                              \
        if (gl_err != 0) fprintf(stderr, "GL error 0x%x returned from '%s'.\n", gl_err, #_CALL);   \
    } while (0)// Call with error check
#else
#define GL_CALL(_CALL) _CALL// Call without error check
#endif

// OpenGL Data
struct ImGui_ImplOpenGL3_Data
{
    GLuint GlVersion;// Extracted at runtime using GL_MAJOR_VERSION, GL_MINOR_VERSION queries (e.g. 320 for GL 3.2)
    char GlslVersionString[32];// Specified by user or detected based on compile time GL settings.
    GLuint FontTexture;
    GLuint ShaderHandle;
    GLint AttribLocationTex;// Uniforms location
    GLint AttribLocationProjMtx;
    GLuint AttribLocationVtxPos;// Vertex attributes location
    GLuint AttribLocationVtxUV;
    GLuint AttribLocationVtxColor;
    unsigned int VboHandle, ElementsHandle;
    GLsizeiptr VertexBufferSize;
    GLsizeiptr IndexBufferSize;
    bool HasClipOrigin;
    bool UseBufferSubData;

    ImGui_ImplOpenGL3_Data() { memset((void *) this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplOpenGL3_Data *ImGui_ImplOpenGL3_GetBackendData() {
    return ImGui::GetCurrentContext()
                   ? (ImGui_ImplOpenGL3_Data *) ImGui::GetIO().BackendRendererUserData
                   : nullptr;
}

// Forward Declarations
static void ImGui_ImplOpenGL3_InitPlatformInterface();
static void ImGui_ImplOpenGL3_ShutdownPlatformInterface();

// OpenGL vertex attribute state (for ES 1.0 and ES 2.0 only)
#ifndef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
struct ImGui_ImplOpenGL3_VtxAttribState
{
    GLint Enabled, Size, Type, Normalized, Stride;
    GLvoid *Ptr;

    void GetState(GLint index) {
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &Enabled);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &Size);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &Type);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &Normalized);
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &Stride);
        glGetVertexAttribPointerv(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &Ptr);
    }
    void SetState(GLint index) {
        glVertexAttribPointer(index, Size, Type, (GLboolean) Normalized, Stride, Ptr);
        if (Enabled) glEnableVertexAttribArray(index);
        else
            glDisableVertexAttribArray(index);
    }
};
#endif

// Functions
bool ImGui_ImplOpenGL3_Init() {
    ImGuiIO &io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Initialize our loader
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3) &&                          \
        !defined(IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
    if (!gladLoadGL()) {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return false;
    }
#endif

    // Setup backend capabilities flags
    ImGui_ImplOpenGL3_Data *bd = IM_NEW(ImGui_ImplOpenGL3_Data)();
    io.BackendRendererUserData = (void *) bd;
    io.BackendRendererName = "imgui_impl_opengl3";

    // Query for GL version (e.g. 320 for GL 3.2)
#if !defined(IMGUI_IMPL_OPENGL_ES2)
    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    if (major == 0 && minor == 0) {
        // Query GL_VERSION in desktop GL 2.x, the string will start with "<major>.<minor>"
        const char *gl_version = (const char *) glGetString(GL_VERSION);
        sscanf(gl_version, "%d.%d", &major, &minor);
    }
    bd->GlVersion = (GLuint) (major * 100 + minor * 10);

    bd->UseBufferSubData = false;
    /*
    // Query vendor to enable glBufferSubData kludge
#ifdef _WIN32
    if (const char* vendor = (const char*)glGetString(GL_VENDOR))
        if (strncmp(vendor, "Intel", 5) == 0)
            bd->UseBufferSubData = true;
#endif
    */
#else
    bd->GlVersion = 200;// GLES 2
#endif

#ifdef IMGUI_IMPL_OPENGL_DEBUG
    printf("GL_MAJOR_VERSION = %d\nGL_MINOR_VERSION = %d\nGL_VENDOR = '%s'\nGL_RENDERER = '%s'\n",
           major, minor, (const char *) glGetString(GL_VENDOR),
           (const char *) glGetString(GL_RENDERER));// [DEBUG]
#endif

#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
    if (bd->GlVersion >= 320)
        io.BackendFlags |=
                ImGuiBackendFlags_RendererHasVtxOffset;// We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
#endif
    io.BackendFlags |=
            ImGuiBackendFlags_RendererHasViewports;// We can create multi-viewports on the Renderer side (optional)

    const char *glsl_version = "#version 330";
    IM_ASSERT((int) strlen(glsl_version) + 2 < IM_ARRAYSIZE(bd->GlslVersionString));
    strcpy(bd->GlslVersionString, glsl_version);
    strcat(bd->GlslVersionString, "\n");

    // Make an arbitrary GL call (we don't actually need the result)
    // IF YOU GET A CRASH HERE: it probably means the OpenGL function loader didn't do its job. Let us know!
    GLint current_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);

    // Detect extensions we support
    bd->HasClipOrigin = (bd->GlVersion >= 450);
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_EXTENSIONS
    GLint num_extensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    for (GLint i = 0; i < num_extensions; i++) {
        const char *extension = (const char *) glGetStringi(GL_EXTENSIONS, i);
        if (extension != nullptr && strcmp(extension, "GL_ARB_clip_control") == 0)
            bd->HasClipOrigin = true;
    }
#endif

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        ImGui_ImplOpenGL3_InitPlatformInterface();

    return true;
}

void ImGui_ImplOpenGL3_Shutdown() {
    ImGui_ImplOpenGL3_Data *bd = ImGui_ImplOpenGL3_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO &io = ImGui::GetIO();

    ImGui_ImplOpenGL3_ShutdownPlatformInterface();
    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    IM_DELETE(bd);
}

void ImGui_ImplOpenGL3_NewFrame() {
    ImGui_ImplOpenGL3_Data *bd = ImGui_ImplOpenGL3_GetBackendData();
    IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplOpenGL3_Init()?");

    if (!bd->ShaderHandle) ImGui_ImplOpenGL3_CreateDeviceObjects();
}

static void ImGui_ImplOpenGL3_SetupRenderState(ImDrawData *draw_data, int fb_width, int fb_height,
                                               GLuint vertex_array_object) {
    ImGui_ImplOpenGL3_Data *bd = ImGui_ImplOpenGL3_GetBackendData();

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_SCISSOR_TEST);
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
    if (bd->GlVersion >= 310) glDisable(GL_PRIMITIVE_RESTART);
#endif
#ifdef IMGUI_IMPL_HAS_POLYGON_MODE
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

    // Support for GL 4.5 rarely used glClipControl(GL_UPPER_LEFT)
#if defined(GL_CLIP_ORIGIN)
    bool clip_origin_lower_left = true;
    if (bd->HasClipOrigin) {
        GLenum current_clip_origin = 0;
        glGetIntegerv(GL_CLIP_ORIGIN, (GLint *) &current_clip_origin);
        if (current_clip_origin == GL_UPPER_LEFT) clip_origin_lower_left = false;
    }
#endif

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    GL_CALL(glViewport(0, 0, (GLsizei) fb_width, (GLsizei) fb_height));
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
#if defined(GL_CLIP_ORIGIN)
    if (!clip_origin_lower_left) {
        float tmp = T;
        T = B;
        B = tmp;
    }// Swap top and bottom if origin is upper left
#endif
    const float ortho_projection[4][4] = {
            {2.0f / (R - L), 0.0f, 0.0f, 0.0f},
            {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
            {0.0f, 0.0f, -1.0f, 0.0f},
            {(R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f},
    };
    glUseProgram(bd->ShaderHandle);
    glUniform1i(bd->AttribLocationTex, 0);
    glUniformMatrix4fv(bd->AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);

#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
    if (bd->GlVersion >= 330)
        glBindSampler(
                0,
                0);// We use combined texture/sampler state. Applications using GL 3.3 may set that otherwise.
#endif

    (void) vertex_array_object;
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    glBindVertexArray(vertex_array_object);
#endif

    // Bind vertex/index buffers and setup attributes for ImDrawVert
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, bd->VboHandle));
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bd->ElementsHandle));
    GL_CALL(glEnableVertexAttribArray(bd->AttribLocationVtxPos));
    GL_CALL(glEnableVertexAttribArray(bd->AttribLocationVtxUV));
    GL_CALL(glEnableVertexAttribArray(bd->AttribLocationVtxColor));
    GL_CALL(glVertexAttribPointer(bd->AttribLocationVtxPos, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(ImDrawVert), (GLvoid *) IM_OFFSETOF(ImDrawVert, pos)));
    GL_CALL(glVertexAttribPointer(bd->AttribLocationVtxUV, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(ImDrawVert), (GLvoid *) IM_OFFSETOF(ImDrawVert, uv)));
    GL_CALL(glVertexAttribPointer(bd->AttribLocationVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                                  sizeof(ImDrawVert), (GLvoid *) IM_OFFSETOF(ImDrawVert, col)));
}

// OpenGL3 Render function.
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly.
// This is in order to be able to run within an OpenGL engine that doesn't do so.
void ImGui_ImplOpenGL3_RenderDrawData(ImDrawData *draw_data) {
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int) (draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int) (draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) return;

    ImGui_ImplOpenGL3_Data *bd = ImGui_ImplOpenGL3_GetBackendData();

    // Backup GL state
    GLenum last_active_texture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint *) &last_active_texture);
    glActiveTexture(GL_TEXTURE0);
    GLuint last_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *) &last_program);
    GLuint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint *) &last_texture);
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
    GLuint last_sampler;
    if (bd->GlVersion >= 330) {
        glGetIntegerv(GL_SAMPLER_BINDING, (GLint *) &last_sampler);
    } else {
        last_sampler = 0;
    }
#endif
    GLuint last_array_buffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint *) &last_array_buffer);
#ifndef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    // This is part of VAO on OpenGL 3.0+ and OpenGL ES 3.0+.
    GLint last_element_array_buffer;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
    ImGui_ImplOpenGL3_VtxAttribState last_vtx_attrib_state_pos;
    last_vtx_attrib_state_pos.GetState(bd->AttribLocationVtxPos);
    ImGui_ImplOpenGL3_VtxAttribState last_vtx_attrib_state_uv;
    last_vtx_attrib_state_uv.GetState(bd->AttribLocationVtxUV);
    ImGui_ImplOpenGL3_VtxAttribState last_vtx_attrib_state_color;
    last_vtx_attrib_state_color.GetState(bd->AttribLocationVtxColor);
#endif
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    GLuint last_vertex_array_object;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint *) &last_vertex_array_object);
#endif
#ifdef IMGUI_IMPL_HAS_POLYGON_MODE
    GLint last_polygon_mode[2];
    glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
#endif
    GLint last_viewport[4];
    glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4];
    glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    GLenum last_blend_src_rgb;
    glGetIntegerv(GL_BLEND_SRC_RGB, (GLint *) &last_blend_src_rgb);
    GLenum last_blend_dst_rgb;
    glGetIntegerv(GL_BLEND_DST_RGB, (GLint *) &last_blend_dst_rgb);
    GLenum last_blend_src_alpha;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint *) &last_blend_src_alpha);
    GLenum last_blend_dst_alpha;
    glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint *) &last_blend_dst_alpha);
    GLenum last_blend_equation_rgb;
    glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint *) &last_blend_equation_rgb);
    GLenum last_blend_equation_alpha;
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint *) &last_blend_equation_alpha);
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_stencil_test = glIsEnabled(GL_STENCIL_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
    GLboolean last_enable_primitive_restart =
            (bd->GlVersion >= 310) ? glIsEnabled(GL_PRIMITIVE_RESTART) : GL_FALSE;
#endif

    // Setup desired GL state
    // Recreate the VAO every time (this is to easily allow multiple GL contexts to be rendered to. VAO are not shared among GL contexts)
    // The renderer would actually work without any VAO bound, but then our VertexAttrib calls would overwrite the default one currently bound.
    GLuint vertex_array_object = 0;
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    GL_CALL(glGenVertexArrays(1, &vertex_array_object));
#endif
    ImGui_ImplOpenGL3_SetupRenderState(draw_data, fb_width, fb_height, vertex_array_object);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;// (0,0) unless using multi-viewports
    ImVec2 clip_scale =
            draw_data->FramebufferScale;// (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList *cmd_list = draw_data->CmdLists[n];

        // Upload vertex/index buffers
        // - OpenGL drivers are in a very sorry state nowadays....
        //   During 2021 we attempted to switch from glBufferData() to orphaning+glBufferSubData() following reports
        //   of leaks on Intel GPU when using multi-viewports on Windows.
        // - After this we kept hearing of various display corruptions issues. We started disabling on non-Intel GPU, but issues still got reported on Intel.
        // - We are now back to using exclusively glBufferData(). So bd->UseBufferSubData IS ALWAYS FALSE in this code.
        //   We are keeping the old code path for a while in case people finding new issues may want to test the bd->UseBufferSubData path.
        // - See https://github.com/ocornut/imgui/issues/4468 and please report any corruption issues.
        const GLsizeiptr vtx_buffer_size =
                (GLsizeiptr) cmd_list->VtxBuffer.Size * (int) sizeof(ImDrawVert);
        const GLsizeiptr idx_buffer_size =
                (GLsizeiptr) cmd_list->IdxBuffer.Size * (int) sizeof(ImDrawIdx);
        if (bd->UseBufferSubData) {
            if (bd->VertexBufferSize < vtx_buffer_size) {
                bd->VertexBufferSize = vtx_buffer_size;
                GL_CALL(glBufferData(GL_ARRAY_BUFFER, bd->VertexBufferSize, nullptr,
                                     GL_STREAM_DRAW));
            }
            if (bd->IndexBufferSize < idx_buffer_size) {
                bd->IndexBufferSize = idx_buffer_size;
                GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, bd->IndexBufferSize, nullptr,
                                     GL_STREAM_DRAW));
            }
            GL_CALL(glBufferSubData(GL_ARRAY_BUFFER, 0, vtx_buffer_size,
                                    (const GLvoid *) cmd_list->VtxBuffer.Data));
            GL_CALL(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, idx_buffer_size,
                                    (const GLvoid *) cmd_list->IdxBuffer.Data));
        } else {
            GL_CALL(glBufferData(GL_ARRAY_BUFFER, vtx_buffer_size,
                                 (const GLvoid *) cmd_list->VtxBuffer.Data, GL_STREAM_DRAW));
            GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx_buffer_size,
                                 (const GLvoid *) cmd_list->IdxBuffer.Data, GL_STREAM_DRAW));
        }

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr) {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplOpenGL3_SetupRenderState(draw_data, fb_width, fb_height,
                                                       vertex_array_object);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            } else {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y) continue;

                // Apply scissor/clipping rectangle (Y is inverted in OpenGL)
                GL_CALL(glScissor((int) clip_min.x, (int) ((float) fb_height - clip_max.y),
                                  (int) (clip_max.x - clip_min.x),
                                  (int) (clip_max.y - clip_min.y)));

                // Bind texture, Draw
                GL_CALL(glBindTexture(GL_TEXTURE_2D, (GLuint) (intptr_t) pcmd->GetTexID()));
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
                if (bd->GlVersion >= 320)
                    GL_CALL(glDrawElementsBaseVertex(
                            GL_TRIANGLES, (GLsizei) pcmd->ElemCount,
                            sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                            (void *) (intptr_t) (pcmd->IdxOffset * sizeof(ImDrawIdx)),
                            (GLint) pcmd->VtxOffset));
                else
#endif
                    GL_CALL(glDrawElements(
                            GL_TRIANGLES, (GLsizei) pcmd->ElemCount,
                            sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                            (void *) (intptr_t) (pcmd->IdxOffset * sizeof(ImDrawIdx))));
            }
        }
    }

    // Destroy the temporary VAO
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    GL_CALL(glDeleteVertexArrays(1, &vertex_array_object));
#endif

    // Restore modified GL state
    glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
    if (bd->GlVersion >= 330) glBindSampler(0, last_sampler);
#endif
    glActiveTexture(last_active_texture);
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    glBindVertexArray(last_vertex_array_object);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
#ifndef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    last_vtx_attrib_state_pos.SetState(bd->AttribLocationVtxPos);
    last_vtx_attrib_state_uv.SetState(bd->AttribLocationVtxUV);
    last_vtx_attrib_state_color.SetState(bd->AttribLocationVtxColor);
#endif
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha,
                        last_blend_dst_alpha);
    if (last_enable_blend) glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    if (last_enable_stencil_test) glEnable(GL_STENCIL_TEST);
    else
        glDisable(GL_STENCIL_TEST);
    if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);
#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
    if (bd->GlVersion >= 310) {
        if (last_enable_primitive_restart) glEnable(GL_PRIMITIVE_RESTART);
        else
            glDisable(GL_PRIMITIVE_RESTART);
    }
#endif

#ifdef IMGUI_IMPL_HAS_POLYGON_MODE
    glPolygonMode(GL_FRONT_AND_BACK, (GLenum) last_polygon_mode[0]);
#endif
    glViewport(last_viewport[0], last_viewport[1], (GLsizei) last_viewport[2],
               (GLsizei) last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei) last_scissor_box[2],
              (GLsizei) last_scissor_box[3]);
    (void) bd;// Not all compilation paths use this
}

bool ImGui_ImplOpenGL3_CreateFontsTexture() {
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplOpenGL3_Data *bd = ImGui_ImplOpenGL3_GetBackendData();

    // Build texture atlas
    unsigned char *pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(
            &pixels, &width,
            &height);// Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    // Upload texture to graphics system
    // (Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling)
    GLint last_texture;
    GL_CALL(glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture));
    GL_CALL(glGenTextures(1, &bd->FontTexture));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, bd->FontTexture));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
#ifdef GL_UNPACK_ROW_LENGTH// Not on WebGL/ES
    GL_CALL(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
#endif
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         pixels));

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID) (intptr_t) bd->FontTexture);

    // Restore state
    GL_CALL(glBindTexture(GL_TEXTURE_2D, last_texture));

    return true;
}

void ImGui_ImplOpenGL3_DestroyFontsTexture() {
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplOpenGL3_Data *bd = ImGui_ImplOpenGL3_GetBackendData();
    if (bd->FontTexture) {
        glDeleteTextures(1, &bd->FontTexture);
        io.Fonts->SetTexID(0);
        bd->FontTexture = 0;
    }
}

// If you get an error please report on github. You may try different GL context version or GLSL version. See GL<>GLSL version table at the top of this file.
static bool CheckShader(GLuint handle, const char *desc) {
    ImGui_ImplOpenGL3_Data *bd = ImGui_ImplOpenGL3_GetBackendData();
    GLint status = 0, log_length = 0;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    if ((GLboolean) status == GL_FALSE)
        fprintf(stderr,
                "ERROR: ImGui_ImplOpenGL3_CreateDeviceObjects: failed to compile %s! With GLSL: "
                "%s\n",
                desc, bd->GlslVersionString);
    if (log_length > 1) {
        ImVector<char> buf;
        buf.resize((int) (log_length + 1));
        glGetShaderInfoLog(handle, log_length, nullptr, (GLchar *) buf.begin());
        fprintf(stderr, "%s\n", buf.begin());
    }
    return (GLboolean) status == GL_TRUE;
}

// If you get an error please report on GitHub. You may try different GL context version or GLSL version.
static bool CheckProgram(GLuint handle, const char *desc) {
    ImGui_ImplOpenGL3_Data *bd = ImGui_ImplOpenGL3_GetBackendData();
    GLint status = 0, log_length = 0;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
    if ((GLboolean) status == GL_FALSE)
        fprintf(stderr,
                "ERROR: ImGui_ImplOpenGL3_CreateDeviceObjects: failed to link %s! With GLSL %s\n",
                desc, bd->GlslVersionString);
    if (log_length > 1) {
        ImVector<char> buf;
        buf.resize((int) (log_length + 1));
        glGetProgramInfoLog(handle, log_length, nullptr, (GLchar *) buf.begin());
        fprintf(stderr, "%s\n", buf.begin());
    }
    return (GLboolean) status == GL_TRUE;
}

bool ImGui_ImplOpenGL3_CreateDeviceObjects() {
    ImGui_ImplOpenGL3_Data *bd = ImGui_ImplOpenGL3_GetBackendData();

    // Backup GL state
    GLint last_texture, last_array_buffer;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    GLint last_vertex_array;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
#endif

    // Parse GLSL version string
    int glsl_version = 130;
    sscanf(bd->GlslVersionString, "#version %d", &glsl_version);

    std::string vertex_shader =
            FUtil::readFileString(METADOT_RESLOC_STR("data/shaders/imgui_common.vert"));
    std::string fragment_shader =
            FUtil::readFileString(METADOT_RESLOC_STR("data/shaders/imgui_common.frag"));

    const char *vertbuffer = vertex_shader.c_str();
    const char *fragbuffer = fragment_shader.c_str();

    // Create shaders
    GLuint vert_handle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_handle, 1, &vertbuffer, nullptr);
    glCompileShader(vert_handle);
    CheckShader(vert_handle, "vertex shader");

    GLuint frag_handle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_handle, 1, &fragbuffer, nullptr);
    glCompileShader(frag_handle);
    CheckShader(frag_handle, "fragment shader");

    // Link
    bd->ShaderHandle = glCreateProgram();
    glAttachShader(bd->ShaderHandle, vert_handle);
    glAttachShader(bd->ShaderHandle, frag_handle);
    glLinkProgram(bd->ShaderHandle);
    CheckProgram(bd->ShaderHandle, "shader program");

    glDetachShader(bd->ShaderHandle, vert_handle);
    glDetachShader(bd->ShaderHandle, frag_handle);
    glDeleteShader(vert_handle);
    glDeleteShader(frag_handle);

    bd->AttribLocationTex = glGetUniformLocation(bd->ShaderHandle, "Texture");
    bd->AttribLocationProjMtx = glGetUniformLocation(bd->ShaderHandle, "ProjMtx");
    bd->AttribLocationVtxPos = (GLuint) glGetAttribLocation(bd->ShaderHandle, "Position");
    bd->AttribLocationVtxUV = (GLuint) glGetAttribLocation(bd->ShaderHandle, "UV");
    bd->AttribLocationVtxColor = (GLuint) glGetAttribLocation(bd->ShaderHandle, "Color");

    // Create buffers
    glGenBuffers(1, &bd->VboHandle);
    glGenBuffers(1, &bd->ElementsHandle);

    ImGui_ImplOpenGL3_CreateFontsTexture();

    // Restore modified GL state
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
#ifdef IMGUI_IMPL_OPENGL_USE_VERTEX_ARRAY
    glBindVertexArray(last_vertex_array);
#endif

    return true;
}

void ImGui_ImplOpenGL3_DestroyDeviceObjects() {
    ImGui_ImplOpenGL3_Data *bd = ImGui_ImplOpenGL3_GetBackendData();
    if (bd->VboHandle) {
        glDeleteBuffers(1, &bd->VboHandle);
        bd->VboHandle = 0;
    }
    if (bd->ElementsHandle) {
        glDeleteBuffers(1, &bd->ElementsHandle);
        bd->ElementsHandle = 0;
    }
    if (bd->ShaderHandle) {
        glDeleteProgram(bd->ShaderHandle);
        bd->ShaderHandle = 0;
    }
    ImGui_ImplOpenGL3_DestroyFontsTexture();
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create and handle multiple viewports simultaneously.
// If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

static void ImGui_ImplOpenGL3_RenderWindow(ImGuiViewport *viewport, void *) {
    if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear)) {
        ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    ImGui_ImplOpenGL3_RenderDrawData(viewport->DrawData);
}

static void ImGui_ImplOpenGL3_InitPlatformInterface() {
    ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
    platform_io.Renderer_RenderWindow = ImGui_ImplOpenGL3_RenderWindow;
}

static void ImGui_ImplOpenGL3_ShutdownPlatformInterface() { ImGui::DestroyPlatformWindows(); }

#pragma endregion ImGuiImplementGL3

#pragma region ImGuiImplementSDL

// SDL Data
struct ImGui_ImplSDL2_Data
{
    C_Window *Window;
    C_Renderer *Renderer;
    Uint64 Time;
    Uint32 MouseWindowID;
    int MouseButtonsDown;
    C_Cursor *MouseCursors[ImGuiMouseCursor_COUNT];
    int PendingMouseLeaveFrame;
    char *ClipboardTextData;
    bool MouseCanUseGlobalState;
    bool MouseCanReportHoveredViewport;// This is hard to use/unreliable on SDL so we'll set ImGuiBackendFlags_HasMouseHoveredViewport dynamically based on state.
    bool UseVulkan;

    ImGui_ImplSDL2_Data() { memset((void *) this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
// FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled when using multi-context.
static ImGui_ImplSDL2_Data *ImGui_ImplSDL2_GetBackendData() {
    return ImGui::GetCurrentContext()
                   ? (ImGui_ImplSDL2_Data *) ImGui::GetIO().BackendPlatformUserData
                   : nullptr;
}

// Forward Declarations
static void ImGui_ImplSDL2_UpdateMonitors();
static void ImGui_ImplSDL2_InitPlatformInterface(SDL_Window *window, void *sdl_gl_context);
static void ImGui_ImplSDL2_ShutdownPlatformInterface();

// Functions
static const char *ImGui_ImplSDL2_GetClipboardText(void *) {
    ImGui_ImplSDL2_Data *bd = ImGui_ImplSDL2_GetBackendData();
    if (bd->ClipboardTextData) SDL_free(bd->ClipboardTextData);
    bd->ClipboardTextData = SDL_GetClipboardText();
    return bd->ClipboardTextData;
}

static void ImGui_ImplSDL2_SetClipboardText(void *, const char *text) {
    SDL_SetClipboardText(text);
}

static ImGuiKey ImGui_ImplSDL2_KeycodeToImGuiKey(int keycode) {
    switch (keycode) {
        case SDLK_TAB:
            return ImGuiKey_Tab;
        case SDLK_LEFT:
            return ImGuiKey_LeftArrow;
        case SDLK_RIGHT:
            return ImGuiKey_RightArrow;
        case SDLK_UP:
            return ImGuiKey_UpArrow;
        case SDLK_DOWN:
            return ImGuiKey_DownArrow;
        case SDLK_PAGEUP:
            return ImGuiKey_PageUp;
        case SDLK_PAGEDOWN:
            return ImGuiKey_PageDown;
        case SDLK_HOME:
            return ImGuiKey_Home;
        case SDLK_END:
            return ImGuiKey_End;
        case SDLK_INSERT:
            return ImGuiKey_Insert;
        case SDLK_DELETE:
            return ImGuiKey_Delete;
        case SDLK_BACKSPACE:
            return ImGuiKey_Backspace;
        case SDLK_SPACE:
            return ImGuiKey_Space;
        case SDLK_RETURN:
            return ImGuiKey_Enter;
        case SDLK_ESCAPE:
            return ImGuiKey_Escape;
        case SDLK_QUOTE:
            return ImGuiKey_Apostrophe;
        case SDLK_COMMA:
            return ImGuiKey_Comma;
        case SDLK_MINUS:
            return ImGuiKey_Minus;
        case SDLK_PERIOD:
            return ImGuiKey_Period;
        case SDLK_SLASH:
            return ImGuiKey_Slash;
        case SDLK_SEMICOLON:
            return ImGuiKey_Semicolon;
        case SDLK_EQUALS:
            return ImGuiKey_Equal;
        case SDLK_LEFTBRACKET:
            return ImGuiKey_LeftBracket;
        case SDLK_BACKSLASH:
            return ImGuiKey_Backslash;
        case SDLK_RIGHTBRACKET:
            return ImGuiKey_RightBracket;
        case SDLK_BACKQUOTE:
            return ImGuiKey_GraveAccent;
        case SDLK_CAPSLOCK:
            return ImGuiKey_CapsLock;
        case SDLK_SCROLLLOCK:
            return ImGuiKey_ScrollLock;
        case SDLK_NUMLOCKCLEAR:
            return ImGuiKey_NumLock;
        case SDLK_PRINTSCREEN:
            return ImGuiKey_PrintScreen;
        case SDLK_PAUSE:
            return ImGuiKey_Pause;
        case SDLK_KP_0:
            return ImGuiKey_Keypad0;
        case SDLK_KP_1:
            return ImGuiKey_Keypad1;
        case SDLK_KP_2:
            return ImGuiKey_Keypad2;
        case SDLK_KP_3:
            return ImGuiKey_Keypad3;
        case SDLK_KP_4:
            return ImGuiKey_Keypad4;
        case SDLK_KP_5:
            return ImGuiKey_Keypad5;
        case SDLK_KP_6:
            return ImGuiKey_Keypad6;
        case SDLK_KP_7:
            return ImGuiKey_Keypad7;
        case SDLK_KP_8:
            return ImGuiKey_Keypad8;
        case SDLK_KP_9:
            return ImGuiKey_Keypad9;
        case SDLK_KP_PERIOD:
            return ImGuiKey_KeypadDecimal;
        case SDLK_KP_DIVIDE:
            return ImGuiKey_KeypadDivide;
        case SDLK_KP_MULTIPLY:
            return ImGuiKey_KeypadMultiply;
        case SDLK_KP_MINUS:
            return ImGuiKey_KeypadSubtract;
        case SDLK_KP_PLUS:
            return ImGuiKey_KeypadAdd;
        case SDLK_KP_ENTER:
            return ImGuiKey_KeypadEnter;
        case SDLK_KP_EQUALS:
            return ImGuiKey_KeypadEqual;
        case SDLK_LCTRL:
            return ImGuiKey_LeftCtrl;
        case SDLK_LSHIFT:
            return ImGuiKey_LeftShift;
        case SDLK_LALT:
            return ImGuiKey_LeftAlt;
        case SDLK_LGUI:
            return ImGuiKey_LeftSuper;
        case SDLK_RCTRL:
            return ImGuiKey_RightCtrl;
        case SDLK_RSHIFT:
            return ImGuiKey_RightShift;
        case SDLK_RALT:
            return ImGuiKey_RightAlt;
        case SDLK_RGUI:
            return ImGuiKey_RightSuper;
        case SDLK_APPLICATION:
            return ImGuiKey_Menu;
        case SDLK_0:
            return ImGuiKey_0;
        case SDLK_1:
            return ImGuiKey_1;
        case SDLK_2:
            return ImGuiKey_2;
        case SDLK_3:
            return ImGuiKey_3;
        case SDLK_4:
            return ImGuiKey_4;
        case SDLK_5:
            return ImGuiKey_5;
        case SDLK_6:
            return ImGuiKey_6;
        case SDLK_7:
            return ImGuiKey_7;
        case SDLK_8:
            return ImGuiKey_8;
        case SDLK_9:
            return ImGuiKey_9;
        case SDLK_a:
            return ImGuiKey_A;
        case SDLK_b:
            return ImGuiKey_B;
        case SDLK_c:
            return ImGuiKey_C;
        case SDLK_d:
            return ImGuiKey_D;
        case SDLK_e:
            return ImGuiKey_E;
        case SDLK_f:
            return ImGuiKey_F;
        case SDLK_g:
            return ImGuiKey_G;
        case SDLK_h:
            return ImGuiKey_H;
        case SDLK_i:
            return ImGuiKey_I;
        case SDLK_j:
            return ImGuiKey_J;
        case SDLK_k:
            return ImGuiKey_K;
        case SDLK_l:
            return ImGuiKey_L;
        case SDLK_m:
            return ImGuiKey_M;
        case SDLK_n:
            return ImGuiKey_N;
        case SDLK_o:
            return ImGuiKey_O;
        case SDLK_p:
            return ImGuiKey_P;
        case SDLK_q:
            return ImGuiKey_Q;
        case SDLK_r:
            return ImGuiKey_R;
        case SDLK_s:
            return ImGuiKey_S;
        case SDLK_t:
            return ImGuiKey_T;
        case SDLK_u:
            return ImGuiKey_U;
        case SDLK_v:
            return ImGuiKey_V;
        case SDLK_w:
            return ImGuiKey_W;
        case SDLK_x:
            return ImGuiKey_X;
        case SDLK_y:
            return ImGuiKey_Y;
        case SDLK_z:
            return ImGuiKey_Z;
        case SDLK_F1:
            return ImGuiKey_F1;
        case SDLK_F2:
            return ImGuiKey_F2;
        case SDLK_F3:
            return ImGuiKey_F3;
        case SDLK_F4:
            return ImGuiKey_F4;
        case SDLK_F5:
            return ImGuiKey_F5;
        case SDLK_F6:
            return ImGuiKey_F6;
        case SDLK_F7:
            return ImGuiKey_F7;
        case SDLK_F8:
            return ImGuiKey_F8;
        case SDLK_F9:
            return ImGuiKey_F9;
        case SDLK_F10:
            return ImGuiKey_F10;
        case SDLK_F11:
            return ImGuiKey_F11;
        case SDLK_F12:
            return ImGuiKey_F12;
    }
    return ImGuiKey_None;
}

static void ImGui_ImplSDL2_UpdateKeyModifiers(SDL_Keymod sdl_key_mods) {
    ImGuiIO &io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, (sdl_key_mods & KMOD_CTRL) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (sdl_key_mods & KMOD_SHIFT) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (sdl_key_mods & KMOD_ALT) != 0);
    io.AddKeyEvent(ImGuiMod_Super, (sdl_key_mods & KMOD_GUI) != 0);
}

// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
// If you have multiple SDL events and some of them are not meant to be used by dear imgui, you may need to filter events based on their windowID field.
bool ImGui_ImplSDL2_ProcessEvent(const SDL_Event *event) {
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplSDL2_Data *bd = ImGui_ImplSDL2_GetBackendData();

    switch (event->type) {
        case SDL_MOUSEMOTION: {
            ImVec2 mouse_pos((float) event->motion.x, (float) event->motion.y);
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                int window_x, window_y;
                SDL_GetWindowPosition(SDL_GetWindowFromID(event->motion.windowID), &window_x,
                                      &window_y);
                mouse_pos.x += window_x;
                mouse_pos.y += window_y;
            }
            io.AddMousePosEvent(mouse_pos.x, mouse_pos.y);
            return true;
        }
        case SDL_MOUSEWHEEL: {
            float wheel_x = (event->wheel.x > 0) ? 1.0f : (event->wheel.x < 0) ? -1.0f : 0.0f;
            float wheel_y = (event->wheel.y > 0) ? 1.0f : (event->wheel.y < 0) ? -1.0f : 0.0f;
            io.AddMouseWheelEvent(wheel_x, wheel_y);
            return true;
        }
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            int mouse_button = -1;
            if (event->button.button == SDL_BUTTON_LEFT) { mouse_button = 0; }
            if (event->button.button == SDL_BUTTON_RIGHT) { mouse_button = 1; }
            if (event->button.button == SDL_BUTTON_MIDDLE) { mouse_button = 2; }
            if (event->button.button == SDL_BUTTON_X1) { mouse_button = 3; }
            if (event->button.button == SDL_BUTTON_X2) { mouse_button = 4; }
            if (mouse_button == -1) break;
            io.AddMouseButtonEvent(mouse_button, (event->type == SDL_MOUSEBUTTONDOWN));
            bd->MouseButtonsDown = (event->type == SDL_MOUSEBUTTONDOWN)
                                           ? (bd->MouseButtonsDown | (1 << mouse_button))
                                           : (bd->MouseButtonsDown & ~(1 << mouse_button));
            return true;
        }
        case SDL_TEXTINPUT: {
            io.AddInputCharactersUTF8(event->text.text);
            return true;
        }
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            ImGui_ImplSDL2_UpdateKeyModifiers((SDL_Keymod) event->key.keysym.mod);
            ImGuiKey key = ImGui_ImplSDL2_KeycodeToImGuiKey(event->key.keysym.sym);
            io.AddKeyEvent(key, (event->type == SDL_KEYDOWN));
            io.SetKeyEventNativeData(
                    key, event->key.keysym.sym, event->key.keysym.scancode,
                    event->key.keysym
                            .scancode);// To support legacy indexing (<1.87 user code). Legacy backend uses SDLK_*** as indices to IsKeyXXX() functions.
            return true;
        }
        case SDL_WINDOWEVENT: {
            // - When capturing mouse, SDL will send a bunch of conflicting LEAVE/ENTER event on every mouse move, but the final ENTER tends to be right.
            // - However we won't get a correct LEAVE event for a captured window.
            // - In some cases, when detaching a window from main viewport SDL may send SDL_WINDOWEVENT_ENTER one frame too late,
            //   causing SDL_WINDOWEVENT_LEAVE on previous frame to interrupt drag operation by clear mouse position. This is why
            //   we delay process the SDL_WINDOWEVENT_LEAVE events by one frame. See issue #5012 for details.
            Uint8 window_event = event->window.event;
            if (window_event == SDL_WINDOWEVENT_ENTER) {
                bd->MouseWindowID = event->window.windowID;
                bd->PendingMouseLeaveFrame = 0;
            }
            if (window_event == SDL_WINDOWEVENT_LEAVE)
                bd->PendingMouseLeaveFrame = ImGui::GetFrameCount() + 1;
            if (window_event == SDL_WINDOWEVENT_FOCUS_GAINED) io.AddFocusEvent(true);
            else if (window_event == SDL_WINDOWEVENT_FOCUS_LOST)
                io.AddFocusEvent(false);
            if (window_event == SDL_WINDOWEVENT_CLOSE || window_event == SDL_WINDOWEVENT_MOVED ||
                window_event == SDL_WINDOWEVENT_RESIZED)
                if (ImGuiViewport *viewport = ImGui::FindViewportByPlatformHandle(
                            (void *) SDL_GetWindowFromID(event->window.windowID))) {
                    if (window_event == SDL_WINDOWEVENT_CLOSE)
                        viewport->PlatformRequestClose = true;
                    if (window_event == SDL_WINDOWEVENT_MOVED) viewport->PlatformRequestMove = true;
                    if (window_event == SDL_WINDOWEVENT_RESIZED)
                        viewport->PlatformRequestResize = true;
                    return true;
                }
            return true;
        }
    }
    return false;
}

static bool ImGui_ImplSDL2_Init(SDL_Window *window, SDL_Renderer *renderer, void *sdl_gl_context) {
    ImGuiIO &io = ImGui::GetIO();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

    // Check and store if we are on a SDL backend that supports global mouse position
    // ("wayland" and "rpi" don't support it, but we chose to use a white-list instead of a black-list)
    bool mouse_can_use_global_state = false;
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
    const char *sdl_backend = SDL_GetCurrentVideoDriver();
    const char *global_mouse_whitelist[] = {"windows", "cocoa", "x11", "DIVE", "VMAN"};
    for (int n = 0; n < IM_ARRAYSIZE(global_mouse_whitelist); n++)
        if (strncmp(sdl_backend, global_mouse_whitelist[n], strlen(global_mouse_whitelist[n])) == 0)
            mouse_can_use_global_state = true;
#endif

    // Setup backend capabilities flags
    ImGui_ImplSDL2_Data *bd = IM_NEW(ImGui_ImplSDL2_Data)();
    io.BackendPlatformUserData = (void *) bd;
    io.BackendPlatformName = "imgui_impl_sdl";
    io.BackendFlags |=
            ImGuiBackendFlags_HasMouseCursors;// We can honor GetMouseCursor() values (optional)
    io.BackendFlags |=
            ImGuiBackendFlags_HasSetMousePos;// We can honor io.WantSetMousePos requests (optional, rarely used)
    if (mouse_can_use_global_state)
        io.BackendFlags |=
                ImGuiBackendFlags_PlatformHasViewports;// We can create multi-viewports on the Platform side (optional)

    bd->Window = window;
    bd->Renderer = renderer;

    // SDL on Linux/OSX doesn't report events for unfocused windows (see https://github.com/ocornut/imgui/issues/4960)
    // We will use 'MouseCanReportHoveredViewport' to set 'ImGuiBackendFlags_HasMouseHoveredViewport' dynamically each frame.
    bd->MouseCanUseGlobalState = mouse_can_use_global_state;
#ifndef __APPLE__
    bd->MouseCanReportHoveredViewport = bd->MouseCanUseGlobalState;
#else
    bd->MouseCanReportHoveredViewport = false;
#endif

    io.SetClipboardTextFn = ImGui_ImplSDL2_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplSDL2_GetClipboardText;
    io.ClipboardUserData = nullptr;

    // Load mouse cursors
    bd->MouseCursors[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    bd->MouseCursors[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    bd->MouseCursors[ImGuiMouseCursor_ResizeAll] =
            SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    bd->MouseCursors[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNESW] =
            SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNWSE] =
            SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    bd->MouseCursors[ImGuiMouseCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    bd->MouseCursors[ImGuiMouseCursor_NotAllowed] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);

    // Set platform dependent data in viewport
    // Our mouse update function expect PlatformHandle to be filled for the main viewport
    ImGuiViewport *main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = (void *) window;
    main_viewport->PlatformHandleRaw = nullptr;
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info)) {
#ifdef _WIN32
        main_viewport->PlatformHandleRaw = (void *) info.info.win.window;
#elif defined(__APPLE__) && defined(SDL_VIDEO_DRIVER_COCOA)
        main_viewport->PlatformHandleRaw = (void *) info.info.cocoa.window;
#endif
    }

    // From 2.0.5: Set SDL hint to receive mouse click events on window focus, otherwise SDL doesn't emit the event.
    // Without this, when clicking to gain focus, our widgets wouldn't activate even though they showed as hovered.
    // (This is unfortunately a global SDL setting, so enabling it might have a side-effect on your application.
    // It is unlikely to make a difference, but if your app absolutely needs to ignore the initial on-focus click:
    // you can ignore SDL_MOUSEBUTTONDOWN events coming right after a SDL_WINDOWEVENT_FOCUS_GAINED)
#ifdef SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
#endif

    // From 2.0.22: Disable auto-capture, this is preventing drag and drop across multiple windows (see #5710)
#ifdef SDL_HINT_MOUSE_AUTO_CAPTURE
    SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");
#endif

    // Update monitors
    ImGui_ImplSDL2_UpdateMonitors();

    // We need SDL_CaptureMouse(), SDL_GetGlobalMouseState() from SDL 2.0.4+ to support multiple viewports.
    // We left the call to ImGui_ImplSDL2_InitPlatformInterface() outside of #ifdef to avoid unused-function warnings.
    if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) &&
        (io.BackendFlags & ImGuiBackendFlags_PlatformHasViewports))
        ImGui_ImplSDL2_InitPlatformInterface(window, sdl_gl_context);

    return true;
}

bool ImGui_ImplSDL2_InitForOpenGL(SDL_Window *window, void *sdl_gl_context) {
    return ImGui_ImplSDL2_Init(window, nullptr, sdl_gl_context);
}

bool ImGui_ImplSDL2_InitForVulkan(SDL_Window *window) {
#if !SDL_HAS_VULKAN
    IM_ASSERT(0 && "Unsupported");
#endif
    if (!ImGui_ImplSDL2_Init(window, nullptr, nullptr)) return false;
    ImGui_ImplSDL2_Data *bd = ImGui_ImplSDL2_GetBackendData();
    bd->UseVulkan = true;
    return true;
}

bool ImGui_ImplSDL2_InitForD3D(SDL_Window *window) {
#if !defined(_WIN32)
    IM_ASSERT(0 && "Unsupported");
#endif
    return ImGui_ImplSDL2_Init(window, nullptr, nullptr);
}

bool ImGui_ImplSDL2_InitForMetal(SDL_Window *window) {
    return ImGui_ImplSDL2_Init(window, nullptr, nullptr);
}

bool ImGui_ImplSDL2_InitForSDLRenderer(SDL_Window *window, SDL_Renderer *renderer) {
    return ImGui_ImplSDL2_Init(window, renderer, nullptr);
}

void ImGui_ImplSDL2_Shutdown() {
    ImGui_ImplSDL2_Data *bd = ImGui_ImplSDL2_GetBackendData();
    IM_ASSERT(bd != nullptr && "No platform backend to shutdown, or already shutdown?");
    ImGuiIO &io = ImGui::GetIO();

    ImGui_ImplSDL2_ShutdownPlatformInterface();

    if (bd->ClipboardTextData) SDL_free(bd->ClipboardTextData);
    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++)
        SDL_FreeCursor(bd->MouseCursors[cursor_n]);

    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    IM_DELETE(bd);
}

// This code is incredibly messy because some of the functions we need for full viewport support are not available in SDL < 2.0.4.
static void ImGui_ImplSDL2_UpdateMouseData() {
    ImGui_ImplSDL2_Data *bd = ImGui_ImplSDL2_GetBackendData();
    ImGuiIO &io = ImGui::GetIO();

    // We forward mouse input when hovered or captured (via SDL_MOUSEMOTION) or when focused (below)
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
    // SDL_CaptureMouse() let the OS know e.g. that our imgui drag outside the SDL window boundaries shouldn't e.g. trigger other operations outside
    SDL_CaptureMouse((bd->MouseButtonsDown != 0) ? SDL_TRUE : SDL_FALSE);
    SDL_Window *focused_window = SDL_GetKeyboardFocus();
    const bool is_app_focused =
            (focused_window && (bd->Window == focused_window ||
                                ImGui::FindViewportByPlatformHandle((void *) focused_window)));
#else
    SDL_Window *focused_window = bd->Window;
    const bool is_app_focused = (SDL_GetWindowFlags(bd->Window) & SDL_WINDOW_INPUT_FOCUS) !=
                                0;// SDL 2.0.3 and non-windowed systems: single-viewport only
#endif

    if (is_app_focused) {
        // (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
        if (io.WantSetMousePos) {
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
                SDL_WarpMouseGlobal((int) io.MousePos.x, (int) io.MousePos.y);
            else
#endif
                SDL_WarpMouseInWindow(bd->Window, (int) io.MousePos.x, (int) io.MousePos.y);
        }

        // (Optional) Fallback to provide mouse position when focused (SDL_MOUSEMOTION already provides this when hovered or captured)
        if (bd->MouseCanUseGlobalState && bd->MouseButtonsDown == 0) {
            // Single-viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window)
            // Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePos is (0,0) when the mouse is on the upper-left of the primary monitor)
            int mouse_x, mouse_y, window_x, window_y;
            SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
            if (!(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)) {
                SDL_GetWindowPosition(focused_window, &window_x, &window_y);
                mouse_x -= window_x;
                mouse_y -= window_y;
            }
            io.AddMousePosEvent((float) mouse_x, (float) mouse_y);
        }
    }

    // (Optional) When using multiple viewports: call io.AddMouseViewportEvent() with the viewport the OS mouse cursor is hovering.
    // If ImGuiBackendFlags_HasMouseHoveredViewport is not set by the backend, Dear imGui will ignore this field and infer the information using its flawed heuristic.
    // - [!] SDL backend does NOT correctly ignore viewports with the _NoInputs flag.
    //       Some backend are not able to handle that correctly. If a backend report an hovered viewport that has the _NoInputs flag (e.g. when dragging a window
    //       for docking, the viewport has the _NoInputs flag in order to allow us to find the viewport under), then Dear ImGui is forced to ignore the value reported
    //       by the backend, and use its flawed heuristic to guess the viewport behind.
    // - [X] SDL backend correctly reports this regardless of another viewport behind focused and dragged from (we need this to find a useful drag and drop target).
    if (io.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport) {
        ImGuiID mouse_viewport_id = 0;
        if (SDL_Window *sdl_mouse_window = SDL_GetWindowFromID(bd->MouseWindowID))
            if (ImGuiViewport *mouse_viewport =
                        ImGui::FindViewportByPlatformHandle((void *) sdl_mouse_window))
                mouse_viewport_id = mouse_viewport->ID;
        io.AddMouseViewportEvent(mouse_viewport_id);
    }
}

static void ImGui_ImplSDL2_UpdateMouseCursor() {
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) return;
    ImGui_ImplSDL2_Data *bd = ImGui_ImplSDL2_GetBackendData();

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None) {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        SDL_ShowCursor(SDL_FALSE);
    } else {
        // Show OS mouse cursor
        SDL_SetCursor(bd->MouseCursors[imgui_cursor] ? bd->MouseCursors[imgui_cursor]
                                                     : bd->MouseCursors[ImGuiMouseCursor_Arrow]);
        SDL_ShowCursor(SDL_TRUE);
    }
}

static void ImGui_ImplSDL2_UpdateGamepads() {
    ImGuiIO &io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) ==
        0)// FIXME: Technically feeding gamepad shouldn't depend on this now that they are regular inputs.
        return;

    // Get gamepad
    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    SDL_GameController *game_controller = SDL_GameControllerOpen(0);
    if (!game_controller) return;
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

// Update gamepad inputs
#define IM_SATURATE(V) (V < 0.0f ? 0.0f : V > 1.0f ? 1.0f : V)
#define MAP_BUTTON(KEY_NO, BUTTON_NO)                                                              \
    { io.AddKeyEvent(KEY_NO, SDL_GameControllerGetButton(game_controller, BUTTON_NO) != 0); }
#define MAP_ANALOG(KEY_NO, AXIS_NO, V0, V1)                                                        \
    {                                                                                              \
        float vn = (float) (SDL_GameControllerGetAxis(game_controller, AXIS_NO) - V0) /            \
                   (float) (V1 - V0);                                                              \
        vn = IM_SATURATE(vn);                                                                      \
        io.AddKeyAnalogEvent(KEY_NO, vn > 0.1f, vn);                                               \
    }
    const int thumb_dead_zone = 8000;// SDL_gamecontroller.h suggests using this value.
    MAP_BUTTON(ImGuiKey_GamepadStart, SDL_CONTROLLER_BUTTON_START);
    MAP_BUTTON(ImGuiKey_GamepadBack, SDL_CONTROLLER_BUTTON_BACK);
    MAP_BUTTON(ImGuiKey_GamepadFaceLeft, SDL_CONTROLLER_BUTTON_X); // Xbox X, PS Square
    MAP_BUTTON(ImGuiKey_GamepadFaceRight, SDL_CONTROLLER_BUTTON_B);// Xbox B, PS Circle
    MAP_BUTTON(ImGuiKey_GamepadFaceUp, SDL_CONTROLLER_BUTTON_Y);   // Xbox Y, PS Triangle
    MAP_BUTTON(ImGuiKey_GamepadFaceDown, SDL_CONTROLLER_BUTTON_A); // Xbox A, PS Cross
    MAP_BUTTON(ImGuiKey_GamepadDpadLeft, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    MAP_BUTTON(ImGuiKey_GamepadDpadRight, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    MAP_BUTTON(ImGuiKey_GamepadDpadUp, SDL_CONTROLLER_BUTTON_DPAD_UP);
    MAP_BUTTON(ImGuiKey_GamepadDpadDown, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    MAP_BUTTON(ImGuiKey_GamepadL1, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    MAP_BUTTON(ImGuiKey_GamepadR1, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    MAP_ANALOG(ImGuiKey_GamepadL2, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 0.0f, 32767);
    MAP_ANALOG(ImGuiKey_GamepadR2, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 0.0f, 32767);
    MAP_BUTTON(ImGuiKey_GamepadL3, SDL_CONTROLLER_BUTTON_LEFTSTICK);
    MAP_BUTTON(ImGuiKey_GamepadR3, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    MAP_ANALOG(ImGuiKey_GamepadLStickLeft, SDL_CONTROLLER_AXIS_LEFTX, -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadLStickRight, SDL_CONTROLLER_AXIS_LEFTX, +thumb_dead_zone, +32767);
    MAP_ANALOG(ImGuiKey_GamepadLStickUp, SDL_CONTROLLER_AXIS_LEFTY, -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadLStickDown, SDL_CONTROLLER_AXIS_LEFTY, +thumb_dead_zone, +32767);
    MAP_ANALOG(ImGuiKey_GamepadRStickLeft, SDL_CONTROLLER_AXIS_RIGHTX, -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadRStickRight, SDL_CONTROLLER_AXIS_RIGHTX, +thumb_dead_zone, +32767);
    MAP_ANALOG(ImGuiKey_GamepadRStickUp, SDL_CONTROLLER_AXIS_RIGHTY, -thumb_dead_zone, -32768);
    MAP_ANALOG(ImGuiKey_GamepadRStickDown, SDL_CONTROLLER_AXIS_RIGHTY, +thumb_dead_zone, +32767);
#undef MAP_BUTTON
#undef MAP_ANALOG
}

// FIXME-PLATFORM: SDL doesn't have an event to notify the application of display/monitor changes
static void ImGui_ImplSDL2_UpdateMonitors() {
    ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
    platform_io.Monitors.resize(0);
    int display_count = SDL_GetNumVideoDisplays();
    for (int n = 0; n < display_count; n++) {
        // Warning: the validity of monitor DPI information on Windows depends on the application DPI awareness settings, which generally needs to be set in the manifest or at runtime.
        ImGuiPlatformMonitor monitor;
        C_Rect r;
        SDL_GetDisplayBounds(n, &r);
        monitor.MainPos = monitor.WorkPos = ImVec2((float) r.x, (float) r.y);
        monitor.MainSize = monitor.WorkSize = ImVec2((float) r.w, (float) r.h);
#if SDL_HAS_USABLE_DISPLAY_BOUNDS
        SDL_GetDisplayUsableBounds(n, &r);
        monitor.WorkPos = ImVec2((float) r.x, (float) r.y);
        monitor.WorkSize = ImVec2((float) r.w, (float) r.h);
#endif
#if SDL_HAS_PER_MONITOR_DPI
        // FIXME-VIEWPORT: On MacOS SDL reports actual monitor DPI scale, ignoring OS configuration. We may want to set
        //  DpiScale to cocoa_window.backingScaleFactor here.
        float dpi = 0.0f;
        if (!SDL_GetDisplayDPI(n, &dpi, nullptr, nullptr)) monitor.DpiScale = dpi / 96.0f;
#endif
        platform_io.Monitors.push_back(monitor);
    }
}

void ImGui_ImplSDL2_NewFrame() {
    ImGui_ImplSDL2_Data *bd = ImGui_ImplSDL2_GetBackendData();
    IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplSDL2_Init()?");
    ImGuiIO &io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    SDL_GetWindowSize(bd->Window, &w, &h);
    if (SDL_GetWindowFlags(bd->Window) & SDL_WINDOW_MINIMIZED) w = h = 0;
    if (bd->Renderer != nullptr) SDL_GetRendererOutputSize(bd->Renderer, &display_w, &display_h);
    else
        SDL_GL_GetDrawableSize(bd->Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float) w, (float) h);
    if (w > 0 && h > 0)
        io.DisplayFramebufferScale = ImVec2((float) display_w / w, (float) display_h / h);

    // Setup time step (we don't use SDL_GetTicks() because it is using millisecond resolution)
    static Uint64 frequency = SDL_GetPerformanceFrequency();
    Uint64 current_time = SDL_GetPerformanceCounter();
    io.DeltaTime = bd->Time > 0 ? (float) ((double) (current_time - bd->Time) / frequency)
                                : (float) (1.0f / 60.0f);
    bd->Time = current_time;

    if (bd->PendingMouseLeaveFrame && bd->PendingMouseLeaveFrame >= ImGui::GetFrameCount() &&
        bd->MouseButtonsDown == 0) {
        bd->MouseWindowID = 0;
        bd->PendingMouseLeaveFrame = 0;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }

    // Our io.AddMouseViewportEvent() calls will only be valid when not capturing.
    // Technically speaking testing for 'bd->MouseButtonsDown == 0' would be more rygorous, but testing for payload reduces noise and potential side-effects.
    if (bd->MouseCanReportHoveredViewport && ImGui::GetDragDropPayload() == nullptr)
        io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;
    else
        io.BackendFlags &= ~ImGuiBackendFlags_HasMouseHoveredViewport;

    ImGui_ImplSDL2_UpdateMouseData();
    ImGui_ImplSDL2_UpdateMouseCursor();

    // Update game controllers (if enabled and available)
    ImGui_ImplSDL2_UpdateGamepads();
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create and handle multiple viewports simultaneously.
// If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

// Helper structure we store in the void* RenderUserData field of each ImGuiViewport to easily retrieve our backend data.
struct ImGui_ImplSDL2_ViewportData
{
    C_Window *Window;
    Uint32 WindowID;
    bool WindowOwned;
    C_GLContext GLContext;

    ImGui_ImplSDL2_ViewportData() {
        Window = nullptr;
        WindowID = 0;
        WindowOwned = false;
        GLContext = nullptr;
    }
    ~ImGui_ImplSDL2_ViewportData() { IM_ASSERT(Window == nullptr && GLContext == nullptr); }
};

static void ImGui_ImplSDL2_CreateWindow(ImGuiViewport *viewport) {
    ImGui_ImplSDL2_Data *bd = ImGui_ImplSDL2_GetBackendData();
    ImGui_ImplSDL2_ViewportData *vd = IM_NEW(ImGui_ImplSDL2_ViewportData)();
    viewport->PlatformUserData = vd;

    ImGuiViewport *main_viewport = ImGui::GetMainViewport();
    ImGui_ImplSDL2_ViewportData *main_viewport_data =
            (ImGui_ImplSDL2_ViewportData *) main_viewport->PlatformUserData;

    // Share GL resources with main context
    bool use_opengl = (main_viewport_data->GLContext != nullptr);
    C_GLContext backup_context = nullptr;
    if (use_opengl) {
        backup_context = SDL_GL_GetCurrentContext();
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
        SDL_GL_MakeCurrent(main_viewport_data->Window, main_viewport_data->GLContext);
    }

    Uint32 sdl_flags = 0;
    sdl_flags |= use_opengl ? SDL_WINDOW_OPENGL : (bd->UseVulkan ? SDL_WINDOW_VULKAN : 0);
    sdl_flags |= SDL_GetWindowFlags(bd->Window) & SDL_WINDOW_ALLOW_HIGHDPI;
    sdl_flags |= SDL_WINDOW_HIDDEN;
    sdl_flags |= (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? SDL_WINDOW_BORDERLESS : 0;
    sdl_flags |= (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? 0 : SDL_WINDOW_RESIZABLE;
#if !defined(_WIN32)
    // See SDL hack in ImGui_ImplSDL2_ShowWindow().
    sdl_flags |= (viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon) ? SDL_WINDOW_SKIP_TASKBAR : 0;
#endif
#if SDL_HAS_ALWAYS_ON_TOP
    sdl_flags |= (viewport->Flags & ImGuiViewportFlags_TopMost) ? SDL_WINDOW_ALWAYS_ON_TOP : 0;
#endif
    vd->Window = SDL_CreateWindow("No Title Yet", (int) viewport->Pos.x, (int) viewport->Pos.y,
                                  (int) viewport->Size.x, (int) viewport->Size.y, sdl_flags);
    vd->WindowOwned = true;
    if (use_opengl) {
        vd->GLContext = SDL_GL_CreateContext(vd->Window);
        SDL_GL_SetSwapInterval(0);
    }
    if (use_opengl && backup_context) SDL_GL_MakeCurrent(vd->Window, backup_context);

    viewport->PlatformHandle = (void *) vd->Window;
    viewport->PlatformHandleRaw = nullptr;
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(vd->Window, &info)) {
#if defined(_WIN32)
        viewport->PlatformHandleRaw = info.info.win.window;
#elif defined(__APPLE__) && defined(SDL_VIDEO_DRIVER_COCOA)
        viewport->PlatformHandleRaw = (void *) info.info.cocoa.window;
#endif
    }
}

static void ImGui_ImplSDL2_DestroyWindow(ImGuiViewport *viewport) {
    if (ImGui_ImplSDL2_ViewportData *vd =
                (ImGui_ImplSDL2_ViewportData *) viewport->PlatformUserData) {
        if (vd->GLContext && vd->WindowOwned) SDL_GL_DeleteContext(vd->GLContext);
        if (vd->Window && vd->WindowOwned) SDL_DestroyWindow(vd->Window);
        vd->GLContext = nullptr;
        vd->Window = nullptr;
        IM_DELETE(vd);
    }
    viewport->PlatformUserData = viewport->PlatformHandle = nullptr;
}

static void ImGui_ImplSDL2_ShowWindow(ImGuiViewport *viewport) {
    ImGui_ImplSDL2_ViewportData *vd = (ImGui_ImplSDL2_ViewportData *) viewport->PlatformUserData;
#if defined(_WIN32)
    HWND hwnd = (HWND) viewport->PlatformHandleRaw;

    // SDL hack: Hide icon from task bar
    // Note: SDL 2.0.6+ has a SDL_WINDOW_SKIP_TASKBAR flag which is supported under Windows but the way it create the window breaks our seamless transition.
    if (viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon) {
        LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
        ex_style &= ~WS_EX_APPWINDOW;
        ex_style |= WS_EX_TOOLWINDOW;
        ::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
    }

    // SDL hack: SDL always activate/focus windows :/
    if (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing) {
        ::ShowWindow(hwnd, SW_SHOWNA);
        return;
    }
#endif

    SDL_ShowWindow(vd->Window);
}

static ImVec2 ImGui_ImplSDL2_GetWindowPos(ImGuiViewport *viewport) {
    ImGui_ImplSDL2_ViewportData *vd = (ImGui_ImplSDL2_ViewportData *) viewport->PlatformUserData;
    int x = 0, y = 0;
    SDL_GetWindowPosition(vd->Window, &x, &y);
    return ImVec2((float) x, (float) y);
}

static void ImGui_ImplSDL2_SetWindowPos(ImGuiViewport *viewport, ImVec2 pos) {
    ImGui_ImplSDL2_ViewportData *vd = (ImGui_ImplSDL2_ViewportData *) viewport->PlatformUserData;
    SDL_SetWindowPosition(vd->Window, (int) pos.x, (int) pos.y);
}

static ImVec2 ImGui_ImplSDL2_GetWindowSize(ImGuiViewport *viewport) {
    ImGui_ImplSDL2_ViewportData *vd = (ImGui_ImplSDL2_ViewportData *) viewport->PlatformUserData;
    int w = 0, h = 0;
    SDL_GetWindowSize(vd->Window, &w, &h);
    return ImVec2((float) w, (float) h);
}

static void ImGui_ImplSDL2_SetWindowSize(ImGuiViewport *viewport, ImVec2 size) {
    ImGui_ImplSDL2_ViewportData *vd = (ImGui_ImplSDL2_ViewportData *) viewport->PlatformUserData;
    SDL_SetWindowSize(vd->Window, (int) size.x, (int) size.y);
}

static void ImGui_ImplSDL2_SetWindowTitle(ImGuiViewport *viewport, const char *title) {
    ImGui_ImplSDL2_ViewportData *vd = (ImGui_ImplSDL2_ViewportData *) viewport->PlatformUserData;
    SDL_SetWindowTitle(vd->Window, title);
}

#if SDL_HAS_WINDOW_ALPHA
static void ImGui_ImplSDL2_SetWindowAlpha(ImGuiViewport *viewport, float alpha) {
    ImGui_ImplSDL2_ViewportData *vd = (ImGui_ImplSDL2_ViewportData *) viewport->PlatformUserData;
    SDL_SetWindowOpacity(vd->Window, alpha);
}
#endif

static void ImGui_ImplSDL2_SetWindowFocus(ImGuiViewport *viewport) {
    ImGui_ImplSDL2_ViewportData *vd = (ImGui_ImplSDL2_ViewportData *) viewport->PlatformUserData;
    SDL_RaiseWindow(vd->Window);
}

static bool ImGui_ImplSDL2_GetWindowFocus(ImGuiViewport *viewport) {
    ImGui_ImplSDL2_ViewportData *vd = (ImGui_ImplSDL2_ViewportData *) viewport->PlatformUserData;
    return (SDL_GetWindowFlags(vd->Window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

static bool ImGui_ImplSDL2_GetWindowMinimized(ImGuiViewport *viewport) {
    ImGui_ImplSDL2_ViewportData *vd = (ImGui_ImplSDL2_ViewportData *) viewport->PlatformUserData;
    return (SDL_GetWindowFlags(vd->Window) & SDL_WINDOW_MINIMIZED) != 0;
}

static void ImGui_ImplSDL2_RenderWindow(ImGuiViewport *viewport, void *) {
    ImGui_ImplSDL2_ViewportData *vd = (ImGui_ImplSDL2_ViewportData *) viewport->PlatformUserData;
    if (vd->GLContext) SDL_GL_MakeCurrent(vd->Window, vd->GLContext);
}

static void ImGui_ImplSDL2_SwapBuffers(ImGuiViewport *viewport, void *) {
    ImGui_ImplSDL2_ViewportData *vd = (ImGui_ImplSDL2_ViewportData *) viewport->PlatformUserData;
    if (vd->GLContext) {
        SDL_GL_MakeCurrent(vd->Window, vd->GLContext);
        SDL_GL_SwapWindow(vd->Window);
    }
}

// Vulkan support (the Vulkan renderer needs to call a platform-side support function to create the surface)
// SDL is graceful enough to _not_ need <vulkan/vulkan.h> so we can safely include this.
#if SDL_HAS_VULKAN
static int ImGui_ImplSDL2_CreateVkSurface(ImGuiViewport *viewport, ImU64 vk_instance,
                                          const void *vk_allocator, ImU64 *out_vk_surface) {
    ImGui_ImplSDL2_ViewportData *vd = (ImGui_ImplSDL2_ViewportData *) viewport->PlatformUserData;
    (void) vk_allocator;
    SDL_bool ret = SDL_Vulkan_CreateSurface(vd->Window, (VkInstance) vk_instance,
                                            (VkSurfaceKHR *) out_vk_surface);
    return ret ? 0 : 1;// ret ? VK_SUCCESS : VK_NOT_READY
}
#endif// SDL_HAS_VULKAN

static void ImGui_ImplSDL2_InitPlatformInterface(SDL_Window *window, void *sdl_gl_context) {
    // Register platform interface (will be coupled with a renderer interface)
    ImGuiPlatformIO &platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_CreateWindow = ImGui_ImplSDL2_CreateWindow;
    platform_io.Platform_DestroyWindow = ImGui_ImplSDL2_DestroyWindow;
    platform_io.Platform_ShowWindow = ImGui_ImplSDL2_ShowWindow;
    platform_io.Platform_SetWindowPos = ImGui_ImplSDL2_SetWindowPos;
    platform_io.Platform_GetWindowPos = ImGui_ImplSDL2_GetWindowPos;
    platform_io.Platform_SetWindowSize = ImGui_ImplSDL2_SetWindowSize;
    platform_io.Platform_GetWindowSize = ImGui_ImplSDL2_GetWindowSize;
    platform_io.Platform_SetWindowFocus = ImGui_ImplSDL2_SetWindowFocus;
    platform_io.Platform_GetWindowFocus = ImGui_ImplSDL2_GetWindowFocus;
    platform_io.Platform_GetWindowMinimized = ImGui_ImplSDL2_GetWindowMinimized;
    platform_io.Platform_SetWindowTitle = ImGui_ImplSDL2_SetWindowTitle;
    platform_io.Platform_RenderWindow = ImGui_ImplSDL2_RenderWindow;
    platform_io.Platform_SwapBuffers = ImGui_ImplSDL2_SwapBuffers;
#if SDL_HAS_WINDOW_ALPHA
    platform_io.Platform_SetWindowAlpha = ImGui_ImplSDL2_SetWindowAlpha;
#endif
#if SDL_HAS_VULKAN
    platform_io.Platform_CreateVkSurface = ImGui_ImplSDL2_CreateVkSurface;
#endif

    // Register main window handle (which is owned by the main application, not by us)
    // This is mostly for simplicity and consistency, so that our code (e.g. mouse handling etc.) can use same logic for main and secondary viewports.
    ImGuiViewport *main_viewport = ImGui::GetMainViewport();
    ImGui_ImplSDL2_ViewportData *vd = IM_NEW(ImGui_ImplSDL2_ViewportData)();
    vd->Window = window;
    vd->WindowID = SDL_GetWindowID(window);
    vd->WindowOwned = false;
    vd->GLContext = sdl_gl_context;
    main_viewport->PlatformUserData = vd;
    main_viewport->PlatformHandle = vd->Window;
}

static void ImGui_ImplSDL2_ShutdownPlatformInterface() { ImGui::DestroyPlatformWindows(); }

#pragma endregion ImGuiImplementSDL

#pragma region ImString

ImString::ImString() : mData(0), mRefCount(0) {}

ImString::ImString(size_t len) : mData(0), mRefCount(0) { reserve(len); }

ImString::ImString(char *string) : mData(string), mRefCount(0) { ref(); }

ImString::ImString(const char *string) : mData(0), mRefCount(0) {
    if (string) {
        mData = ImStrdup(string);
        ref();
    }
}

ImString::ImString(const ImString &other) {
    mRefCount = other.mRefCount;
    mData = other.mData;
    ref();
}

ImString::~ImString() { unref(); }

char &ImString::operator[](size_t pos) { return mData[pos]; }

ImString::operator char *() { return mData; }

bool ImString::operator==(const char *string) { return strcmp(string, mData) == 0; }

bool ImString::operator!=(const char *string) { return strcmp(string, mData) != 0; }

bool ImString::operator==(ImString &string) { return strcmp(string.c_str(), mData) == 0; }

bool ImString::operator!=(const ImString &string) { return strcmp(string.c_str(), mData) != 0; }

ImString &ImString::operator=(const char *string) {
    if (mData) unref();
    mData = ImStrdup(string);
    ref();
    return *this;
}

ImString &ImString::operator=(const ImString &other) {
    if (mData && mData != other.mData) unref();
    mRefCount = other.mRefCount;
    mData = other.mData;
    ref();
    return *this;
}

void ImString::reserve(size_t len) {
    if (mData) unref();
    mData = (char *) ImGui::MemAlloc(len + 1);
    mData[len] = '\0';
    ref();
}

char *ImString::get() { return mData; }

const char *ImString::c_str() const { return mData; }

bool ImString::empty() const { return mData == 0 || mData[0] == '\0'; }

int ImString::refcount() const { return *mRefCount; }

void ImString::ref() {
    if (!mRefCount) {
        mRefCount = new int();
        (*mRefCount) = 0;
    }
    (*mRefCount)++;
}

void ImString::unref() {
    if (mRefCount) {
        (*mRefCount)--;
        if (*mRefCount == 0) {
            ImGui::MemFree(mData);
            mData = 0;
            delete mRefCount;
        }
        mRefCount = 0;
    }
}

#pragma endregion ImString

ImGuiMarkdown::ImGuiMarkdown() {

    m_table_last_pos = ImVec2(0, 0);

    m_md.abi_version = 0;
    m_md.syntax = nullptr;
    m_md.debug_log = nullptr;

    m_md.flags = MD_FLAG_TABLES | MD_FLAG_UNDERLINE | MD_FLAG_STRIKETHROUGH;
    m_md.enter_block = [](MD_BLOCKTYPE type, void *detail, void *userdata) {
        return ((ImGuiMarkdown *) userdata)->block(type, detail, true);
    };

    m_md.leave_block = [](MD_BLOCKTYPE type, void *detail, void *userdata) {
        return ((ImGuiMarkdown *) userdata)->block(type, detail, false);
    };

    m_md.enter_span = [](MD_SPANTYPE type, void *detail, void *userdata) {
        return ((ImGuiMarkdown *) userdata)->span(type, detail, true);
    };

    m_md.leave_span = [](MD_SPANTYPE type, void *detail, void *userdata) {
        return ((ImGuiMarkdown *) userdata)->span(type, detail, false);
    };

    m_md.text = [](MD_TEXTTYPE type, const MD_CHAR *text, MD_SIZE size, void *userdata) {
        return ((ImGuiMarkdown *) userdata)->text(type, text, text + size);
    };
}

void ImGuiMarkdown::BLOCK_UL(const MD_BLOCK_UL_DETAIL *d, bool e) {
    if (e) {
        m_list_stack.push_back(list_info{false, d->mark, 0});
    } else {
        m_list_stack.pop_back();
        if (m_list_stack.empty()) ImGui::NewLine();
    }
}

void ImGuiMarkdown::BLOCK_OL(const MD_BLOCK_OL_DETAIL *d, bool e) {
    if (e) {
        m_list_stack.push_back(list_info{true, d->mark_delimiter, d->start});
    } else {
        m_list_stack.pop_back();
        if (m_list_stack.empty()) ImGui::NewLine();
    }
}

void ImGuiMarkdown::BLOCK_LI(const MD_BLOCK_LI_DETAIL *, bool e) {
    if (e) {
        ImGui::NewLine();

        list_info &nfo = m_list_stack.back();
        if (nfo.is_ol) {
            ImGui::Text("%d%c", nfo.cur_ol++, nfo.delim);
            ImGui::SameLine();
        } else {
            if (nfo.delim == '*') {
                float cx = ImGui::GetCursorPosX();
                cx -= ImGui::GetStyle().FramePadding.x * 2;
                ImGui::SetCursorPosX(cx);
                ImGui::Bullet();
            } else {
                ImGui::Text("%c", nfo.delim);
                ImGui::SameLine();
            }
        }

        ImGui::Indent();
    } else {
        ImGui::Unindent();
    }
}

void ImGuiMarkdown::BLOCK_HR(bool e) {
    if (!e) {
        ImGui::NewLine();
        ImGui::Separator();
    }
}

void ImGuiMarkdown::BLOCK_H(const MD_BLOCK_H_DETAIL *d, bool e) {
    if (e) {
        m_hlevel = d->level;
        ImGui::NewLine();
    } else {
        m_hlevel = 0;
    }

    set_font(e);

    if (!e) {
        if (d->level <= 2) {
            ImGui::NewLine();
            ImGui::Separator();
        }
    }
}

void ImGuiMarkdown::BLOCK_DOC(bool) {}

void ImGuiMarkdown::BLOCK_QUOTE(bool) {}
void ImGuiMarkdown::BLOCK_CODE(const MD_BLOCK_CODE_DETAIL *, bool) {}

void ImGuiMarkdown::BLOCK_HTML(bool) {}

void ImGuiMarkdown::BLOCK_P(bool) {
    if (!m_list_stack.empty()) return;
    ImGui::NewLine();
}

void ImGuiMarkdown::BLOCK_TABLE(const MD_BLOCK_TABLE_DETAIL *, bool e) {
    if (e) {
        m_table_row_pos.clear();
        m_table_col_pos.clear();

        m_table_last_pos = ImGui::GetCursorPos();
    } else {

        ImGui::NewLine();
        m_table_last_pos.y = ImGui::GetCursorPos().y;

        m_table_col_pos.push_back(m_table_last_pos.x);
        m_table_row_pos.push_back(m_table_last_pos.y);

        const ImVec2 wp = ImGui::GetWindowPos();
        const ImVec2 sp = ImGui::GetStyle().ItemSpacing;
        const float wx = wp.x + sp.x / 2;
        const float wy = wp.y - sp.y / 2 - ImGui::GetScrollY();

        for (int i = 0; i < m_table_col_pos.size(); ++i) { m_table_col_pos[i] += wx; }

        for (int i = 0; i < m_table_row_pos.size(); ++i) { m_table_row_pos[i] += wy; }

        const ImColor c = ImGui::GetStyle().Colors[ImGuiCol_Separator];
        ////////////////////////////////////////////////////////////////////////

        ImDrawList *dl = ImGui::GetWindowDrawList();

        const float xmin = m_table_col_pos.front();
        const float xmax = m_table_col_pos.back();
        for (int i = 0; i < m_table_row_pos.size(); ++i) {
            const float p = m_table_row_pos[i];
            dl->AddLine(ImVec2(xmin, p), ImVec2(xmax, p), c, i == 1 ? 2.0f : 1.0f);
        }

        const float ymin = m_table_row_pos.front();
        const float ymax = m_table_row_pos.back();
        for (int i = 0; i < m_table_col_pos.size(); ++i) {
            const float p = m_table_col_pos[i];
            dl->AddLine(ImVec2(p, ymin), ImVec2(p, ymax), c);
        }
    }
}

void ImGuiMarkdown::BLOCK_THEAD(bool e) {
    m_is_table_header = e;
    set_font(e);
}

void ImGuiMarkdown::BLOCK_TBODY(bool e) { m_is_table_body = e; }

void ImGuiMarkdown::BLOCK_TR(bool e) {
    ImGui::SetCursorPosY(m_table_last_pos.y);

    if (e) {
        m_table_next_column = 0;
        ImGui::NewLine();
        m_table_row_pos.push_back(ImGui::GetCursorPosY());
    }
}

void ImGuiMarkdown::BLOCK_TH(const MD_BLOCK_TD_DETAIL *d, bool e) { BLOCK_TD(d, e); }

void ImGuiMarkdown::BLOCK_TD(const MD_BLOCK_TD_DETAIL *, bool e) {
    if (e) {

        if (m_table_next_column < m_table_col_pos.size()) {
            ImGui::SetCursorPosX(m_table_col_pos[m_table_next_column]);
        } else {
            m_table_col_pos.push_back(m_table_last_pos.x);
        }

        ++m_table_next_column;

        ImGui::Indent(m_table_col_pos[m_table_next_column - 1]);
        ImGui::SetCursorPos(
                ImVec2(m_table_col_pos[m_table_next_column - 1], m_table_row_pos.back()));

    } else {
        const ImVec2 p = ImGui::GetCursorPos();
        ImGui::Unindent(m_table_col_pos[m_table_next_column - 1]);
        ImGui::SetCursorPosX(p.x);
        if (p.y > m_table_last_pos.y) m_table_last_pos.y = p.y;
    }
    ImGui::TextUnformatted("");
    ImGui::SameLine();
}

////////////////////////////////////////////////////////////////////////////////
void ImGuiMarkdown::set_href(bool e, const MD_ATTRIBUTE &src) {
    if (e) {
        m_href.assign(src.text, src.size);
    } else {
        m_href.clear();
    }
}

void ImGuiMarkdown::set_font(bool e) {
    if (e) {
        ImGui::PushFont(get_font());
    } else {
        ImGui::PopFont();
    }
}

void ImGuiMarkdown::set_color(bool e) {
    if (e) {
        ImGui::PushStyleColor(ImGuiCol_Text, get_color());
    } else {
        ImGui::PopStyleColor();
    }
}

void ImGuiMarkdown::line(ImColor c, bool under) {
    ImVec2 mi = ImGui::GetItemRectMin();
    ImVec2 ma = ImGui::GetItemRectMax();

    if (!under) { ma.y -= ImGui::GetFontSize() / 2; }

    mi.y = ma.y;

    ImGui::GetWindowDrawList()->AddLine(mi, ma, c, 1.0f);
}

void ImGuiMarkdown::SPAN_A(const MD_SPAN_A_DETAIL *d, bool e) {
    set_href(e, d->href);
    set_color(e);
}

void ImGuiMarkdown::SPAN_EM(bool e) {
    m_is_em = e;
    set_font(e);
}

void ImGuiMarkdown::SPAN_STRONG(bool e) {
    m_is_strong = e;
    set_font(e);
}

void ImGuiMarkdown::SPAN_IMG(const MD_SPAN_IMG_DETAIL *d, bool e) {
    m_is_image = e;

    set_href(e, d->src);

    if (e) {

        image_info nfo;
        if (get_image(nfo)) {

            const float scale = ImGui::GetIO().FontGlobalScale;
            nfo.size.x *= scale;
            nfo.size.y *= scale;

            ImVec2 const csz = ImGui::GetContentRegionAvail();
            if (nfo.size.x > csz.x) {
                const float r = nfo.size.y / nfo.size.x;
                nfo.size.x = csz.x;
                nfo.size.y = csz.x * r;
            }

            ImGui::Image(nfo.texture_id, nfo.size, nfo.uv0, nfo.uv1, nfo.col_tint, nfo.col_border);

            if (ImGui::IsItemHovered()) {

                //if (d->title.size) {
                //	ImGui::SetTooltip("%.*s", (int)d->title.size, d->title.text);
                //}

                if (ImGui::IsMouseReleased(0)) { open_url(); }
            }
        }
    }
}

void ImGuiMarkdown::SPAN_CODE(bool) {}

void ImGuiMarkdown::SPAN_LATEXMATH(bool) {}

void ImGuiMarkdown::SPAN_LATEXMATH_DISPLAY(bool) {}

void ImGuiMarkdown::SPAN_WIKILINK(const MD_SPAN_WIKILINK_DETAIL *, bool) {}

void ImGuiMarkdown::SPAN_U(bool e) { m_is_underline = e; }

void ImGuiMarkdown::SPAN_DEL(bool e) { m_is_strikethrough = e; }

void ImGuiMarkdown::render_text(const char *str, const char *str_end) {
    const float scale = ImGui::GetIO().FontGlobalScale;
    const ImGuiStyle &s = ImGui::GetStyle();

    while (!m_is_image && str < str_end) {

        const char *te = str_end;

        if (!m_is_table_header) {

            float wl = ImGui::GetContentRegionAvail().x;

            if (m_is_table_body) {
                wl = (m_table_next_column < m_table_col_pos.size()
                              ? m_table_col_pos[m_table_next_column]
                              : m_table_last_pos.x);
                wl -= ImGui::GetCursorPosX();
            }

            te = ImGui::GetFont()->CalcWordWrapPositionA(scale, str, str_end, wl);

            if (te == str) ++te;
        }

        ImGui::TextUnformatted(str, te);

        if (!m_href.empty()) {

            ImVec4 c;
            if (ImGui::IsItemHovered()) {

                ImGui::SetTooltip("%s", m_href.c_str());

                c = s.Colors[ImGuiCol_ButtonHovered];
                if (ImGui::IsMouseReleased(0)) { open_url(); }
            } else {
                c = s.Colors[ImGuiCol_Button];
            }
            line(c, true);
        }
        if (m_is_underline) { line(s.Colors[ImGuiCol_Text], true); }
        if (m_is_strikethrough) { line(s.Colors[ImGuiCol_Text], false); }

        str = te;

        while (str < str_end && *str == ' ') ++str;
    }

    ImGui::SameLine(0.0f, 0.0f);
}

bool ImGuiMarkdown::render_entity(const char *str, const char *str_end) {
    const size_t sz = str_end - str;
    if (strncmp(str, "&nbsp;", sz) == 0) {
        ImGui::TextUnformatted("");
        ImGui::SameLine();
        return true;
    }
    return false;
}

bool ImGuiMarkdown::render_html(const char *str, const char *str_end) {
    const size_t sz = str_end - str;

    if (strncmp(str, "<br>", sz) == 0) {
        ImGui::NewLine();
        return true;
    }
    if (strncmp(str, "<hr>", sz) == 0) {
        ImGui::Separator();
        return true;
    }
    if (strncmp(str, "<u>", sz) == 0) {
        m_is_underline = true;
        return true;
    }
    if (strncmp(str, "</u>", sz) == 0) {
        m_is_underline = false;
        return true;
    }
    return false;
}

int ImGuiMarkdown::text(MD_TEXTTYPE type, const char *str, const char *str_end) {
    switch (type) {
        case MD_TEXT_NORMAL:
            render_text(str, str_end);
            break;
        case MD_TEXT_CODE:
            render_text(str, str_end);
            break;
        case MD_TEXT_NULLCHAR:
            break;
        case MD_TEXT_BR:
            ImGui::NewLine();
            break;
        case MD_TEXT_SOFTBR:
            soft_break();
            break;
        case MD_TEXT_ENTITY:
            if (!render_entity(str, str_end)) { render_text(str, str_end); };
            break;
        case MD_TEXT_HTML:
            if (!render_html(str, str_end)) { render_text(str, str_end); }
            break;
        case MD_TEXT_LATEXMATH:
            render_text(str, str_end);
            break;
        default:
            break;
    }

    if (m_is_table_header) {
        const float x = ImGui::GetCursorPosX();
        if (x > m_table_last_pos.x) m_table_last_pos.x = x;
    }

    return 0;
}

int ImGuiMarkdown::block(MD_BLOCKTYPE type, void *d, bool e) {
    switch (type) {
        case MD_BLOCK_DOC:
            BLOCK_DOC(e);
            break;
        case MD_BLOCK_QUOTE:
            BLOCK_QUOTE(e);
            break;
        case MD_BLOCK_UL:
            BLOCK_UL((MD_BLOCK_UL_DETAIL *) d, e);
            break;
        case MD_BLOCK_OL:
            BLOCK_OL((MD_BLOCK_OL_DETAIL *) d, e);
            break;
        case MD_BLOCK_LI:
            BLOCK_LI((MD_BLOCK_LI_DETAIL *) d, e);
            break;
        case MD_BLOCK_HR:
            BLOCK_HR(e);
            break;
        case MD_BLOCK_H:
            BLOCK_H((MD_BLOCK_H_DETAIL *) d, e);
            break;
        case MD_BLOCK_CODE:
            BLOCK_CODE((MD_BLOCK_CODE_DETAIL *) d, e);
            break;
        case MD_BLOCK_HTML:
            BLOCK_HTML(e);
            break;
        case MD_BLOCK_P:
            BLOCK_P(e);
            break;
        case MD_BLOCK_TABLE:
            BLOCK_TABLE((MD_BLOCK_TABLE_DETAIL *) d, e);
            break;
        case MD_BLOCK_THEAD:
            BLOCK_THEAD(e);
            break;
        case MD_BLOCK_TBODY:
            BLOCK_TBODY(e);
            break;
        case MD_BLOCK_TR:
            BLOCK_TR(e);
            break;
        case MD_BLOCK_TH:
            BLOCK_TH((MD_BLOCK_TD_DETAIL *) d, e);
            break;
        case MD_BLOCK_TD:
            BLOCK_TD((MD_BLOCK_TD_DETAIL *) d, e);
            break;
        default:
            assert(false);
            break;
    }

    return 0;
}

int ImGuiMarkdown::span(MD_SPANTYPE type, void *d, bool e) {
    switch (type) {
        case MD_SPAN_EM:
            SPAN_EM(e);
            break;
        case MD_SPAN_STRONG:
            SPAN_STRONG(e);
            break;
        case MD_SPAN_A:
            SPAN_A((MD_SPAN_A_DETAIL *) d, e);
            break;
        case MD_SPAN_IMG:
            SPAN_IMG((MD_SPAN_IMG_DETAIL *) d, e);
            break;
        case MD_SPAN_CODE:
            SPAN_CODE(e);
            break;
        case MD_SPAN_DEL:
            SPAN_DEL(e);
            break;
        case MD_SPAN_LATEXMATH:
            SPAN_LATEXMATH(e);
            break;
        case MD_SPAN_LATEXMATH_DISPLAY:
            SPAN_LATEXMATH_DISPLAY(e);
            break;
        case MD_SPAN_WIKILINK:
            SPAN_WIKILINK((MD_SPAN_WIKILINK_DETAIL *) d, e);
            break;
        case MD_SPAN_U:
            SPAN_U(e);
            break;
        default:
            assert(false);
            break;
    }

    return 0;
}

int ImGuiMarkdown::print(const std::string &text) {
    return md_parse(text.c_str(), text.size() + 1, &m_md, this);
}

////////////////////////////////////////////////////////////////////////////////

ImFont *ImGuiMarkdown::get_font() const {
    return nullptr;//default font

    //Example:
#if 0
    if (m_is_table_header) {
        return g_font_bold;
    }

    switch (m_hlevel)
    {
    case 0:
        return m_is_strong ? g_font_bold : g_font_regular;
    case 1:
        return g_font_bold_large;
    default:
        return g_font_bold;
    }
#endif
};

ImVec4 ImGuiMarkdown::get_color() const {
    if (!m_href.empty()) { return ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]; }
    return ImGui::GetStyle().Colors[ImGuiCol_Text];
}

bool ImGuiMarkdown::get_image(image_info &nfo) const {
    nfo.texture_id = ImGui::GetIO().Fonts->TexID;
    nfo.size = {100, 50};
    nfo.uv0 = {0, 0};
    nfo.uv1 = {1, 1};
    nfo.col_tint = {1, 1, 1, 1};
    nfo.col_border = {0, 0, 0, 0};

    return true;
};

void ImGuiMarkdown::open_url() const {
    if (!m_is_image) {
        SDL_OpenURL(m_href.c_str());
    } else {
    }
}

void ImGuiMarkdown::soft_break() { ImGui::NewLine(); }

#if defined(_METADOT_IMM32)

#include <commctrl.h>
#include <tchar.h>
#include <windows.h>

#include <algorithm>
#include <cassert>

#include "imgui.h"
#include "imgui_internal.h"

#pragma comment(                                                                                   \
        linker,                                                                                    \
        "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

void ImGUIIMMCommunication::operator()() {
    ImGuiIO &io = ImGui::GetIO();

    static ImVec2 window_pos = ImVec2();
    static ImVec2 window_pos_pivot = ImVec2();

    static ImGuiID candidate_window_root_id = 0;

    static ImGuiWindow *lastTextInputNavWindow = nullptr;
    static ImGuiID lastTextInputActiveId = 0;
    static ImGuiID lastTextInputFocusId = 0;

    if (!(candidate_window_root_id &&
          ((ImGui::GetCurrentContext()->NavWindow
                    ? ImGui::GetCurrentContext()->NavWindow->RootWindow->ID
                    : 0u) == candidate_window_root_id))) {

        window_pos = ImVec2(ImGui::GetCurrentContext()->PlatformImeData.InputPos.x + 1.0f,
                            ImGui::GetCurrentContext()->PlatformImeData.InputPos.y);//
        window_pos_pivot = ImVec2(0.0f, 0.0f);

        const ImGuiContext *const currentContext = ImGui::GetCurrentContext();
        IM_ASSERT(currentContext || !"ImGui::GetCurrentContext() return nullptr.");
        if (currentContext) {
            if (!ImGui::IsMouseClicked(0)) {
                if ((currentContext->WantTextInputNextFrame != -1)
                            ? (!!(currentContext->WantTextInputNextFrame))
                            : false) {
                    if ((!!currentContext->NavWindow) &&
                        (currentContext->NavWindow->RootWindow->ID != candidate_window_root_id) &&
                        (ImGui::GetActiveID() != lastTextInputActiveId)) {
                        OutputDebugStringW(L"update lastTextInputActiveId\n");
                        lastTextInputNavWindow = ImGui::GetCurrentContext()->NavWindow;
                        lastTextInputActiveId = ImGui::GetActiveID();
                        lastTextInputFocusId = ImGui::GetFocusID();
                    }
                } else {
                    if (lastTextInputActiveId != 0) {
                        if (currentContext->WantTextInputNextFrame) {
                            OutputDebugStringW(L"update lastTextInputActiveId disabled\n");
                        } else {
                            OutputDebugStringW(L"update lastTextInputActiveId disabled update\n");
                        }
                    }
                    lastTextInputNavWindow = nullptr;
                    lastTextInputActiveId = 0;
                    lastTextInputFocusId = 0;
                }
            }
        }
    }

    ImVec2 target_screen_pos = ImVec2(0.0f, 0.0f);// IME Candidate List Window position.

    if (this->is_open) {
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        if (ImGui::Begin("IME Composition Window", nullptr,
                         ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoNav |
                                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                 ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoSavedSettings)) {

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.78125f, 1.0f, 0.1875f, 1.0f));
            ImGui::Text(static_cast<bool>(comp_conved_utf8) ? comp_conved_utf8.get() : "");
            ImGui::PopStyleColor();

            if (static_cast<bool>(comp_target_utf8)) {
                ImGui::SameLine(0.0f, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_Text,
                                      ImVec4(0.203125f, 0.91796875f, 0.35546875f, 1.0f));

                target_screen_pos = ImGui::GetCursorScreenPos();
                target_screen_pos.y += ImGui::GetTextLineHeightWithSpacing();

                ImGui::Text(static_cast<bool>(comp_target_utf8) ? comp_target_utf8.get() : "");
                ImGui::PopStyleColor();
            }
            if (static_cast<bool>(comp_unconv_utf8)) {
                ImGui::SameLine(0.0f, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.78125f, 1.0f, 0.1875f, 1.0f));
                ImGui::Text(static_cast<bool>(comp_unconv_utf8) ? comp_unconv_utf8.get() : "");
                ImGui::PopStyleColor();
            }
            /*
              ImGui::SameLine();
              ImGui::Text("%u %u",
              candidate_window_root_id ,
              ImGui::GetCurrentContext()->NavWindow ? ImGui::GetCurrentContext()->NavWindow->RootWindow->ID : 0u);
            */
            ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
        }
        ImGui::End();
        ImGui::PopStyleVar();

        /* Draw Candidate List */
        if (show_ime_candidate_list && !candidate_list.list_utf8.empty()) {

            std::vector<const char *> listbox_items = {};

            IM_ASSERT(candidate_window_num);
            int candidate_page = ((int) candidate_list.selection) / candidate_window_num;
            int candidate_selection = ((int) candidate_list.selection) % candidate_window_num;

            auto begin_ite = std::begin(candidate_list.list_utf8);
            std::advance(begin_ite, candidate_page * candidate_window_num);
            auto end_ite = begin_ite;
            {
                auto the_end = std::end(candidate_list.list_utf8);
                for (int i = 0; end_ite != the_end && i < candidate_window_num; ++i) {
                    std::advance(end_ite, 1);
                }
            }

            std::for_each(begin_ite, end_ite,
                          [&](auto &item) { listbox_items.push_back(item.c_str()); });

            const float candidate_window_height = ((ImGui::GetStyle().FramePadding.y * 2) +
                                                   ((ImGui::GetTextLineHeightWithSpacing()) *
                                                    ((int) std::size(listbox_items) + 2)));

            if (io.DisplaySize.y < (target_screen_pos.y + candidate_window_height)) {
                target_screen_pos.y -=
                        ImGui::GetTextLineHeightWithSpacing() + candidate_window_height;
            }

            ImGui::SetNextWindowPos(target_screen_pos, ImGuiCond_Always, window_pos_pivot);

            if (ImGui::Begin("##Overlay-IME-Candidate-List-Window", &show_ime_candidate_list,
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration |
                                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
                                     ImGuiWindowFlags_NoSavedSettings |
                                     ImGuiWindowFlags_NoFocusOnAppearing |
                                     ImGuiWindowFlags_NoNav)) {
                if (ImGui::ListBoxHeader("##IMECandidateListWindow",
                                         static_cast<int>(std::size(listbox_items)),
                                         static_cast<int>(std::size(listbox_items)))) {

                    int i = 0;
                    for (const char *&listbox_item: listbox_items) {
                        if (ImGui::Selectable(listbox_item, (i == candidate_selection))) {

                            /* candidate list selection */

                            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                                !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
                                if (lastTextInputActiveId && lastTextInputFocusId) {
                                    ImGui::SetActiveID(lastTextInputActiveId,
                                                       lastTextInputNavWindow);
                                    ImGui::SetFocusID(lastTextInputFocusId, lastTextInputNavWindow);
                                }
                            }

                            /*
                                我想做 ImmNotifyIME (hImc, NI_SELECTCANDIDATESTR, 0, Candidate_page * Candidate_window_num + i);
                              由于 Vista ImmNotifyIME 不支持 NI_SELECTCANDIDATESTR。
                              @ 请参阅 Microsoft 的 IMM32 兼容性信息.doc

                              因此，DXUTguiIME.cpp（曾经使用过的 DXUT 的 gui IME 处理单元现在被视为已弃用。
                              您可以在 https://github.com/microsoft/DXUT 查看
                              有问题的代码是 https://github.com/microsoft/DXUT/blob/master/Optional/DXUTguiIME.cpp）
                              我确认过了

                              从 L.380 单击候选列表时有一个代码

                              原因是 SendKey 用于通过发送箭头光标键从候选列表中进行选择。
                              （你是什么意思 ？！）

                              基于此，使用 SendKey 创建代码。
                            */
                            {
                                if (candidate_selection == i) {
                                    OutputDebugStringW(L"complete\n");
                                    this->request_candidate_list_str_commit = 1;
                                } else {
                                    const BYTE nVirtualKey =
                                            (candidate_selection < i) ? VK_DOWN : VK_UP;
                                    const size_t nNumToHit = abs(candidate_selection - i);
                                    for (size_t hit = 0; hit < nNumToHit; ++hit) {
                                        keybd_event(nVirtualKey, 0, 0, 0);
                                        keybd_event(nVirtualKey, 0, KEYEVENTF_KEYUP, 0);
                                    }
                                    this->request_candidate_list_str_commit = (int) nNumToHit;
                                }
                            }
                        }
                        ++i;
                    }
                    ImGui::ListBoxFooter();
                }
                ImGui::Text("%d/%d", candidate_list.selection + 1,
                            static_cast<int>(std::size(candidate_list.list_utf8)));
#if defined(_DEBUG)
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s",
#if defined(UNICODE)
                                   u8"DEBUG (UNICODE)"
#else
                                   u8"DEBUG (MBCS)"
#endif /* defined( UNICODE ) */
                );
#endif /* defined( DEBUG ) */
                // #1 ここで作るウィンドウがフォーカスを持ったときには、ウィンドウの位置を変更してはいけない。
                candidate_window_root_id = ImGui::GetCurrentWindowRead()->RootWindow->ID;
                ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
            }
            ImGui::End();
        }
    }

    /*
      io.WantTextInput 根本不可靠。
      在文本输入小部件中，在另一个窗口上按下鼠标按钮以
      其他窗口和小部件在启用 WantTextInput 的情况下处于活动状态
      （特别是当涉及到 IME 候选窗口的小部件时，
      Press 焦点移动到启用了 WantTextInput 的 IME 候选窗口。
      在下一帧
      WantTextInput 已关闭，因为 IME 候选窗口未启用文本输入
      然后，这里就出现了IME失效的现象。 )

      这与直觉行为相反，所以我们会修复它，但是当禁用它时，
      如果任何小部件没有获得窗口焦点
      当 WantTextInput 为真时启用
      不对称的形状减少了不适。
    */
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
        if (io.ImeWindowHandle) {
            IM_ASSERT(IsWindow(static_cast<HWND>(io.ImeWindowHandle)));
            (void) (ImmAssociateContext(static_cast<HWND>(io.ImeWindowHandle), HIMC(0)));
        }
    }
    if (io.WantTextInput) {
        if (io.ImeWindowHandle) {
            IM_ASSERT(IsWindow(static_cast<HWND>(io.ImeWindowHandle)));
            /*
              The second argument of ImmAssociateContextEx is
              Changed from IN to _In_ between 10.0.10586.0-Windows 10 and 10.0.14393.0-Windows.

              https://abi-laboratory.pro/compatibility/Windows_10_1511_10586.494_to_Windows_10_1607_14393.0/x86_64/headers_diff/imm32.dll/diff.html

              This is strange, but HIMC is probably not zero because this change was intentional.
              However, the document states that HIMC is ignored if the flag IACE_DEFAULT is used.

              https://docs.microsoft.com/en-us/windows/win32/api/immdev/nf-immdev-immassociatecontextex

              Passing HIMC appropriately when using IACE_DEFAULT causes an error and returns to 0
            */
            VERIFY(ImmAssociateContextEx(static_cast<HWND>(io.ImeWindowHandle), HIMC(0),
                                         IACE_DEFAULT));
        }
    }

    return;
}

ImGUIIMMCommunication::IMMCandidateList ImGUIIMMCommunication::IMMCandidateList::cocreate(
        const CANDIDATELIST *const src, const size_t src_size) {
    IM_ASSERT(nullptr != src);
    IM_ASSERT(sizeof(CANDIDATELIST) <= src->dwSize);
    IM_ASSERT(src->dwSelection < src->dwCount);

    IMMCandidateList dst{};
    if (!(sizeof(CANDIDATELIST) < src->dwSize)) { return dst; }
    if (!(src->dwSelection < src->dwCount)) { return dst; }
    const char *const baseaddr = reinterpret_cast<const char *>(src);

    for (size_t i = 0; i < src->dwCount; ++i) {
        const wchar_t *const item = reinterpret_cast<const wchar_t *>(baseaddr + src->dwOffset[i]);
        const int require_byte = WideCharToMultiByte(CP_UTF8, 0, item, -1, nullptr, 0, NULL, NULL);
        if (0 < require_byte) {
            std::unique_ptr<char[]> utf8buf{new char[require_byte]};
            if (require_byte == WideCharToMultiByte(CP_UTF8, 0, item, -1, utf8buf.get(),
                                                    require_byte, NULL, NULL)) {
                dst.list_utf8.emplace_back(utf8buf.get());
                continue;
            }
        }
        dst.list_utf8.emplace_back("??");
    }
    dst.selection = src->dwSelection;
    return dst;
}

bool ImGUIIMMCommunication::update_candidate_window(HWND hWnd) {
    IM_ASSERT(IsWindow(hWnd));
    bool result = false;
    HIMC const hImc = ImmGetContext(hWnd);
    if (hImc) {
        DWORD dwSize = ImmGetCandidateListW(hImc, 0, NULL, 0);

        if (dwSize) {
            IM_ASSERT(sizeof(CANDIDATELIST) <= dwSize);
            if (sizeof(CANDIDATELIST) <= dwSize) {

                std::vector<char> candidatelist((size_t) dwSize);
                if ((DWORD) (std::size(candidatelist) *
                             sizeof(typename decltype(candidatelist)::value_type)) ==
                    ImmGetCandidateListW(
                            hImc, 0, reinterpret_cast<CANDIDATELIST *>(candidatelist.data()),
                            (DWORD) (std::size(candidatelist) *
                                     sizeof(typename decltype(candidatelist)::value_type)))) {
                    const CANDIDATELIST *const cl =
                            reinterpret_cast<CANDIDATELIST *>(candidatelist.data());
                    candidate_list = std::move(IMMCandidateList::cocreate(cl, dwSize));
                    result = true;
#if 0  /* for IMM candidate window debug BEGIN*/
                    {
                        wchar_t dbgbuf[1024];
                        _snwprintf_s(dbgbuf, sizeof(dbgbuf) / sizeof(dbgbuf[0]),
                            L" dwSize = %d , dwCount = %d , dwSelection = %d\n",
                            cl->dwSize,
                            cl->dwCount,
                            cl->dwSelection);
                        OutputDebugStringW(dbgbuf);
                        for (DWORD i = 0; i < cl->dwCount; ++i) {
                            _snwprintf_s(dbgbuf, sizeof(dbgbuf) / sizeof(dbgbuf[0]),
                                L"%d%s: %s\n",
                                i, (cl->dwSelection == i ? L"*" : L" "),
                                (wchar_t*)(candidatelist.data() + cl->dwOffset[i]));
                            OutputDebugStringW(dbgbuf);
                        }
                    }
#endif /* for IMM candidate window debug END */
                }
            }
        }
        VERIFY(ImmReleaseContext(hWnd, hImc));
    }
    return result;
}

LRESULT
ImGUIIMMCommunication::imm_communication_subClassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                                      LPARAM lParam, UINT_PTR uIdSubclass,
                                                      DWORD_PTR dwRefData) {
    switch (uMsg) {
        case WM_DESTROY: {
            VERIFY(ImmAssociateContextEx(hWnd, HIMC(0), IACE_DEFAULT));
            if (!RemoveWindowSubclass(hWnd, reinterpret_cast<SUBCLASSPROC>(uIdSubclass),
                                      uIdSubclass)) {
                IM_ASSERT(!"RemoveWindowSubclass() failed\n");
            }
        } break;
        default:
            if (dwRefData) {
                return imm_communication_subClassProc_implement(
                        hWnd, uMsg, wParam, lParam, uIdSubclass,
                        *reinterpret_cast<ImGUIIMMCommunication *>(dwRefData));
            }
    }
    return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT
ImGUIIMMCommunication::imm_communication_subClassProc_implement(HWND hWnd, UINT uMsg, WPARAM wParam,
                                                                LPARAM lParam, UINT_PTR uIdSubclass,
                                                                ImGUIIMMCommunication &comm) {
    switch (uMsg) {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            if (comm.is_open) { return 0; }
            break;

        case WM_IME_SETCONTEXT: { /* 各ビットを落とす */
            lParam &= ~(ISC_SHOWUICOMPOSITIONWINDOW | (ISC_SHOWUICANDIDATEWINDOW) |
                        (ISC_SHOWUICANDIDATEWINDOW << 1) | (ISC_SHOWUICANDIDATEWINDOW << 2) |
                        (ISC_SHOWUICANDIDATEWINDOW << 3));
        }
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
        case WM_IME_STARTCOMPOSITION: {
            /* 此消息通知应用程序请求“IME”显示转换窗口。
            应用程序在处理转换字符串的显示时必须处理此消息。
            如果您让 DefWindowProc 函数处理此消息，则该消息将被传递到默认 IME 窗口。
            （即自己绘制转换窗口时，不要传给DefWindowPrc）
        */
            comm.is_open = true;
        }
            return 1;
        case WM_IME_ENDCOMPOSITION: {
            comm.is_open = false;
        }
            return DefSubclassProc(hWnd, uMsg, wParam, lParam);
        case WM_IME_COMPOSITION: {
            HIMC const hImc = ImmGetContext(hWnd);
            if (hImc) {
                if (lParam & GCS_RESULTSTR) {
                    comm.comp_conved_utf8 = nullptr;
                    comm.comp_target_utf8 = nullptr;
                    comm.comp_unconv_utf8 = nullptr;
                    comm.show_ime_candidate_list = false;
                }
                if (lParam & GCS_COMPSTR) {

                    const LONG compstr_length_in_byte =
                            ImmGetCompositionStringW(hImc, GCS_COMPSTR, nullptr, 0);
                    switch (compstr_length_in_byte) {
                        case IMM_ERROR_NODATA:
                        case IMM_ERROR_GENERAL:
                            break;
                        default: {
                            size_t const buf_length_in_wchar =
                                    (size_t(compstr_length_in_byte) / sizeof(wchar_t)) + 1;
                            IM_ASSERT(0 < buf_length_in_wchar);
                            std::unique_ptr<wchar_t[]> buf{new wchar_t[buf_length_in_wchar]};
                            if (buf) {
                                //std::fill( &buf[0] , &buf[buf_length_in_wchar-1] , L'\0' );
                                const LONG buf_length_in_byte =
                                        LONG(buf_length_in_wchar * sizeof(wchar_t));
                                const DWORD l = ImmGetCompositionStringW(hImc, GCS_COMPSTR,
                                                                         (LPVOID) (buf.get()),
                                                                         buf_length_in_byte);

                                const DWORD attribute_size =
                                        ImmGetCompositionStringW(hImc, GCS_COMPATTR, NULL, 0);
                                std::vector<char> attribute_vec(attribute_size, 0);
                                const DWORD attribute_end = ImmGetCompositionStringW(
                                        hImc, GCS_COMPATTR, attribute_vec.data(),
                                        (DWORD) std::size(attribute_vec));
                                IM_ASSERT(attribute_end == (DWORD) (std::size(attribute_vec)));
                                {
                                    std::wstring comp_converted;
                                    std::wstring comp_target;
                                    std::wstring comp_unconveted;
                                    size_t begin = 0;
                                    size_t end = 0;

                                    for (end = begin; end < attribute_end; ++end) {
                                        if ((ATTR_TARGET_CONVERTED == attribute_vec[end] ||
                                             ATTR_TARGET_NOTCONVERTED == attribute_vec[end])) {
                                            break;
                                        } else {
                                            comp_converted.push_back(buf[end]);
                                        }
                                    }

                                    for (begin = end; end < attribute_end; ++end) {
                                        if (!(ATTR_TARGET_CONVERTED == attribute_vec[end] ||
                                              ATTR_TARGET_NOTCONVERTED == attribute_vec[end])) {
                                            break;
                                        } else {
                                            comp_target.push_back(buf[end]);
                                        }
                                    }

                                    for (; end < attribute_end; ++end) {
                                        comp_unconveted.push_back(buf[end]);
                                    }

#if 0
                            {
                                wchar_t dbgbuf[1024];
                                _snwprintf_s(dbgbuf, sizeof(dbgbuf) / sizeof(dbgbuf[0]),
                                    L"attribute_size = %d \"%s[%s]%s\"\n",
                                    attribute_size,
                                    comp_converted.c_str(),
                                    comp_target.c_str(),
                                    comp_unconveted.c_str());
                                OutputDebugStringW(dbgbuf);
                            }
#endif
                                    // 准备一个 lambda 函数以将每个函数转换为 UTF-8
                                    /*
                                将 std::wstring 转换为 std::unique_ptr <char[]> 作为 UTF - 8 空终止字符串
                                如果参数是空字符串，则输入 nullptr
                                */
                                    auto to_utf8_pointer =
                                            [](const std::wstring &arg) -> std::unique_ptr<char[]> {
                                        if (arg.empty()) {
                                            return std::unique_ptr<char[]>(nullptr);
                                        }
                                        const int require_byte =
                                                WideCharToMultiByte(CP_UTF8, 0, arg.c_str(), -1,
                                                                    nullptr, 0, NULL, NULL);
                                        if (0 == require_byte) {
                                            const DWORD lastError = GetLastError();
                                            (void) (lastError);
                                            IM_ASSERT(ERROR_INSUFFICIENT_BUFFER != lastError);
                                            IM_ASSERT(ERROR_INVALID_FLAGS != lastError);
                                            IM_ASSERT(ERROR_INVALID_PARAMETER != lastError);
                                            IM_ASSERT(ERROR_NO_UNICODE_TRANSLATION != lastError);
                                        }
                                        IM_ASSERT(0 != require_byte);
                                        if (!(0 < require_byte)) {
                                            return std::unique_ptr<char[]>(nullptr);
                                        }

                                        std::unique_ptr<char[]> utf8buf{new char[require_byte]};

                                        const int conversion_result = WideCharToMultiByte(
                                                CP_UTF8, 0, arg.c_str(), -1, utf8buf.get(),
                                                require_byte, NULL, NULL);
                                        if (conversion_result == 0) {
                                            const DWORD lastError = GetLastError();
                                            (void) (lastError);
                                            IM_ASSERT(ERROR_INSUFFICIENT_BUFFER != lastError);
                                            IM_ASSERT(ERROR_INVALID_FLAGS != lastError);
                                            IM_ASSERT(ERROR_INVALID_PARAMETER != lastError);
                                            IM_ASSERT(ERROR_NO_UNICODE_TRANSLATION != lastError);
                                        }

                                        IM_ASSERT(require_byte == conversion_result);
                                        if (require_byte != conversion_result) {
                                            utf8buf.reset(nullptr);
                                        }
                                        return utf8buf;
                                    };

                                    comm.comp_conved_utf8 = to_utf8_pointer(comp_converted);
                                    comm.comp_target_utf8 = to_utf8_pointer(comp_target);
                                    comm.comp_unconv_utf8 = to_utf8_pointer(comp_unconveted);

                                    /*当 GCS_COMPSTR 更新时，Google IME 当然会 IMN_CHANGECANDIDATE
                                不会像完成一样发送。
                                所以，我请求你在这里更新候选人名单 */

                                    // 如果comp_target为空则删除
                                    if (!static_cast<bool>(comm.comp_target_utf8)) {
                                        comm.candidate_list.clear();
                                    } else {
                                        // candidate_list の取得に失敗したときは消去する
                                        if (!comm.update_candidate_window(hWnd)) {
                                            comm.candidate_list.clear();
                                        }
                                    }
                                }
                            }
                        } break;
                    }
                }
                VERIFY(ImmReleaseContext(hWnd, hImc));
            }

        }// end of WM_IME_COMPOSITION

#if defined(UNICODE)
        // 在UNICODE配置的情况下，直接用DefWindowProc吸收进IME就可以了
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
            // 在多字节配置中，Window 子类的过程处理它，所以需要 DefSubclassProc。
#else
            return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
#endif

        case WM_IME_NOTIFY: {
            switch (wParam) {

                case IMN_OPENCANDIDATE:
                    // 通常，在发送 IMN_OPENCANDIDATE 时会设置 show_ime_candidate_list 标志。
                    // Google IME 不发送 IMN_OPENCANDIDATE（IMN_CHANGECANDIDATE 发送）
                    // 为此，在启动 IMN_CHANGECANDIDATE 时进行更改
                    comm.show_ime_candidate_list = true;
#if 0
            if (IMN_OPENCANDIDATE == wParam) {
                OutputDebugStringW(L"IMN_OPENCANDIDATE\n");
            }
#endif
                    ;// tear down;
                case IMN_CHANGECANDIDATE: {
#if 0
            if (IMN_CHANGECANDIDATE == wParam) {
                OutputDebugStringW(L"IMN_CHANGECANDIDATE\n");
            }
#endif

                    // Google IME BEGIN 的代码 有关详细信息，请参阅有关 IMN_OPENCANDIDATE 的评论
                    if (!comm.show_ime_candidate_list) { comm.show_ime_candidate_list = true; }
                    // 谷歌输入法结束代码

                    HIMC const hImc = ImmGetContext(hWnd);
                    if (hImc) {
                        DWORD dwSize = ImmGetCandidateListW(hImc, 0, NULL, 0);
                        if (dwSize) {
                            IM_ASSERT(sizeof(CANDIDATELIST) <= dwSize);
                            if (sizeof(CANDIDATELIST) <=
                                dwSize) {// dwSize は最低でも struct CANDIDATELIST より大きくなければならない

                                (void) (lParam);
                                std::vector<char> candidatelist((size_t) dwSize);
                                if ((DWORD) (std::size(candidatelist) *
                                             sizeof(typename decltype(candidatelist)::
                                                            value_type)) ==
                                    ImmGetCandidateListW(
                                            hImc, 0,
                                            reinterpret_cast<CANDIDATELIST *>(candidatelist.data()),
                                            (DWORD) (std::size(candidatelist) *
                                                     sizeof(typename decltype(candidatelist)::
                                                                    value_type)))) {
                                    const CANDIDATELIST *const cl =
                                            reinterpret_cast<CANDIDATELIST *>(candidatelist.data());
                                    comm.candidate_list =
                                            std::move(IMMCandidateList::cocreate(cl, dwSize));

#if 0  /* for IMM candidate window debug BEGIN*/
                            {
                                wchar_t dbgbuf[1024];
                                _snwprintf_s(dbgbuf, sizeof(dbgbuf) / sizeof(dbgbuf[0]),
                                    L"lParam = %lld, dwSize = %d , dwCount = %d , dwSelection = %d\n",
                                    lParam,
                                    cl->dwSize,
                                    cl->dwCount,
                                    cl->dwSelection);
                                OutputDebugStringW(dbgbuf);
                                for (DWORD i = 0; i < cl->dwCount; ++i) {
                                    _snwprintf_s(dbgbuf, sizeof(dbgbuf) / sizeof(dbgbuf[0]),
                                        L"%d%s: %s\n",
                                        i, (cl->dwSelection == i ? L"*" : L" "),
                                        (wchar_t*)(candidatelist.data() + cl->dwOffset[i]));
                                    OutputDebugStringW(dbgbuf);
                                }
                            }
#endif /* for IMM candidate window debug END */
                                }
                            }
                        }
                        VERIFY(ImmReleaseContext(hWnd, hImc));
                    }
                }

                    IM_ASSERT(0 <= comm.request_candidate_list_str_commit);
                    if (comm.request_candidate_list_str_commit) {
                        if (comm.request_candidate_list_str_commit == 1) {
                            VERIFY(PostMessage(hWnd, WM_IMGUI_IMM32_COMMAND,
                                               WM_IMGUI_IMM32_COMMAND_COMPOSITION_COMPLETE, 0));
                        }
                        --(comm.request_candidate_list_str_commit);
                    }

                    break;
                case IMN_CLOSECANDIDATE: {
                    //OutputDebugStringW(L"IMN_CLOSECANDIDATE\n");
                    comm.show_ime_candidate_list = false;
                } break;
                default:
                    break;
            }
        }
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);

        case WM_IME_REQUEST:
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);

        case WM_INPUTLANGCHANGE:
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);

        case WM_IMGUI_IMM32_COMMAND: {
            switch (wParam) {
                case WM_IMGUI_IMM32_COMMAND_NOP:// NOP
                    return 1;
                case WM_IMGUI_IMM32_COMMAND_SUBCLASSIFY: {
                    ImGuiIO &io = ImGui::GetIO();
                    if (io.ImeWindowHandle) {
                        IM_ASSERT(IsWindow(static_cast<HWND>(io.ImeWindowHandle)));
                        VERIFY(ImmAssociateContextEx(static_cast<HWND>(io.ImeWindowHandle), nullptr,
                                                     IACE_IGNORENOCONTEXT));
                    }
                }
                    return 1;
                case WM_IMGUI_IMM32_COMMAND_COMPOSITION_COMPLETE:
                    if (!static_cast<bool>(comm.comp_unconv_utf8) ||
                        '\0' == *(comm.comp_unconv_utf8.get())) {
                        /*
                   There is probably no unconverted string after the conversion target.
                   However, since there is now a cursor operation in the
                   keyboard buffer, the confirmation operation needs to be performed
                   after the cursor operation is completed.  For this purpose, an enter
                   key, which is a decision operation, is added to the end of the
                   keyboard buffer.
                */
#if 0
                /*
                  Here, the expected behavior is to perform the conversion completion
                  operation.
                  However, there is a bug that the TextInput loses the focus
                  and the conversion result is lost when the conversion
                  complete operation is performed.

                  To work around this bug, disable it.
                  This is because the movement of the focus of the widget and
                  the accompanying timing are complicated relationships.
                */
                HIMC const hImc = ImmGetContext(hWnd);
                if (hImc) {
                    VERIFY(ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0));
                    VERIFY(ImmReleaseContext(hWnd, hImc));
                }
#else
                        keybd_event(VK_RETURN, 0, 0, 0);
                        keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
#endif
                    } else {
                        /* Do this to close the candidate window without ending composition. */
                        /*
                  keybd_event (VK_RIGHT, 0, 0, 0);
                  keybd_event (VK_RIGHT, 0, KEYEVENTF_KEYUP, 0);

                  keybd_event (VK_LEFT, 0, 0, 0);
                  keybd_event (VK_LEFT, 0, KEYEVENTF_KEYUP, 0);
                */
                        /*
                   Since there is an unconverted string after the conversion
                   target, press the right key of the keyboard to convert the
                   next clause to IME.
                */
                        keybd_event(VK_RIGHT, 0, 0, 0);
                        keybd_event(VK_RIGHT, 0, KEYEVENTF_KEYUP, 0);
                    }
                    return 1;
                default:
                    break;
            }
        }
            return 1;
        default:
            break;
    }
    return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

BOOL ImGUIIMMCommunication::subclassify_impl(HWND hWnd) {
    IM_ASSERT(IsWindow(hWnd));
    if (!IsWindow(hWnd)) { return FALSE; }

    /* 在 imgui_imm32_onthespot 中进行 IME 控制，
            因为当 TextWidget 失去焦点时 io.WantTextInput 变为 true-> off
            这时候检查一下IME的状态，如果打开就关闭。
            @see ImGUIIMMCommunication :: operator () () 开头

            亲爱的ImGui的ImGui::IO::ImeWindowHandle原本用来指定CompositionWindow的位置
            我用了它，所以它符合目的

            但是，这种方法不好，因为当有多个目标操作系统窗口时它会崩溃。
    */
    ImGui::GetIO().ImeWindowHandle = static_cast<void *>(hWnd);
    if (::SetWindowSubclass(
                hWnd, ImGUIIMMCommunication::imm_communication_subClassProc,
                reinterpret_cast<UINT_PTR>(ImGUIIMMCommunication::imm_communication_subClassProc),
                reinterpret_cast<DWORD_PTR>(this))) {
        /*
           I want to close IME once by calling imgex::imm_associate_context_disable()

           However, at this point, default IMM Context may not have been initialized by User32.dll yet.
           Therefore, a strategy is to post the message and set the IMMContext after the message pump is turned on.
        */
        return PostMessage(hWnd, WM_IMGUI_IMM32_COMMAND, WM_IMGUI_IMM32_COMMAND_SUBCLASSIFY, 0u);
    }
    return FALSE;
}

#endif

void ImGuiWidget::PlotFlame(const char *label,
                            void (*values_getter)(float *start, float *end, ImU8 *level,
                                                  const char **caption, const void *data, int idx),
                            const void *data, int values_count, int values_offset,
                            const char *overlay_text, float scale_min, float scale_max,
                            ImVec2 graph_size) {
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;

    ImGuiContext &g = *GImGui;
    const ImGuiStyle &style = g.Style;

    // Find the maximum depth
    ImU8 maxDepth = 0;
    for (int i = values_offset; i < values_count; ++i) {
        ImU8 depth;
        values_getter(nullptr, nullptr, &depth, nullptr, data, i);
        maxDepth = ImMax(maxDepth, depth);
    }

    const auto blockHeight = ImGui::GetTextLineHeight() + (style.FramePadding.y * 2);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    if (graph_size.x == 0.0f) graph_size.x = ImGui::CalcItemWidth();
    if (graph_size.y == 0.0f)
        graph_size.y = label_size.y + (style.FramePadding.y * 3) + blockHeight * (maxDepth + 1);

    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + graph_size);
    const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
    const ImRect total_bb(frame_bb.Min,
                          frame_bb.Max + ImVec2(label_size.x > 0.0f
                                                        ? style.ItemInnerSpacing.x + label_size.x
                                                        : 0.0f,
                                                0));
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, 0, &frame_bb)) return;

    // Determine scale from values if not specified
    if (scale_min == FLT_MAX || scale_max == FLT_MAX) {
        float v_min = FLT_MAX;
        float v_max = -FLT_MAX;
        for (int i = values_offset; i < values_count; i++) {
            float v_start, v_end;
            values_getter(&v_start, &v_end, nullptr, nullptr, data, i);
            if (v_start == v_start)// Check non-NaN values
                v_min = ImMin(v_min, v_start);
            if (v_end == v_end)// Check non-NaN values
                v_max = ImMax(v_max, v_end);
        }
        if (scale_min == FLT_MAX) scale_min = v_min;
        if (scale_max == FLT_MAX) scale_max = v_max;
    }

    ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true,
                       style.FrameRounding);

    bool any_hovered = false;
    if (values_count - values_offset >= 1) {
        const ImU32 col_base = ImGui::GetColorU32(ImGuiCol_PlotHistogram) & 0x77FFFFFF;
        const ImU32 col_hovered = ImGui::GetColorU32(ImGuiCol_PlotHistogramHovered) & 0x77FFFFFF;
        const ImU32 col_outline_base = ImGui::GetColorU32(ImGuiCol_PlotHistogram) & 0x7FFFFFFF;
        const ImU32 col_outline_hovered =
                ImGui::GetColorU32(ImGuiCol_PlotHistogramHovered) & 0x7FFFFFFF;

        for (int i = values_offset; i < values_count; ++i) {
            float stageStart, stageEnd;
            ImU8 depth;
            const char *caption;

            values_getter(&stageStart, &stageEnd, &depth, &caption, data, i);

            auto duration = scale_max - scale_min;
            if (duration == 0) { return; }

            auto start = stageStart - scale_min;
            auto end = stageEnd - scale_min;

            auto startX = static_cast<float>(start / (double) duration);
            auto endX = static_cast<float>(end / (double) duration);

            float width = inner_bb.Max.x - inner_bb.Min.x;
            float height = blockHeight * (maxDepth - depth + 1) - style.FramePadding.y;

            auto pos0 = inner_bb.Min + ImVec2(startX * width, height);
            auto pos1 = inner_bb.Min + ImVec2(endX * width, height + blockHeight);

            bool v_hovered = false;
            if (ImGui::IsMouseHoveringRect(pos0, pos1)) {
                ImGui::SetTooltip("%s: %8.4g", caption, stageEnd - stageStart);
                v_hovered = true;
                any_hovered = v_hovered;
            }

            window->DrawList->AddRectFilled(pos0, pos1, v_hovered ? col_hovered : col_base);
            window->DrawList->AddRect(pos0, pos1,
                                      v_hovered ? col_outline_hovered : col_outline_base);
            auto textSize = ImGui::CalcTextSize(caption);
            auto boxSize = (pos1 - pos0);
            auto textOffset = ImVec2(0.0f, 0.0f);
            if (textSize.x < boxSize.x) {
                textOffset = ImVec2(0.5f, 0.5f) * (boxSize - textSize);
                ImGui::RenderText(pos0 + textOffset, caption);
            }
        }

        // Text overlay
        if (overlay_text)
            ImGui::RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y),
                                     frame_bb.Max, overlay_text, NULL, NULL, ImVec2(0.5f, 0.0f));

        if (label_size.x > 0.0f)
            ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y),
                              label);
    }

    if (!any_hovered && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Total: %8.4g", scale_max - scale_min);
    }
}

/**
	Define METAENGINE_GUI_DISABLE_TEST_WINDOW to disable this feature.
*/

#ifndef METAENGINE_GUI_DISABLE_TEST_WINDOW
#include <array>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <tuple>
#include <vector>
#endif

void ShowAutoTestWindow() {

    auto myCollapsingHeader = [](const char *name) -> bool {
        ImGuiStyle &style = ImGui::GetStyle();
        ImGui::PushStyleColor(ImGuiCol_Header, style.Colors[ImGuiCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, style.Colors[ImGuiCol_ButtonHovered]);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, style.Colors[ImGuiCol_ButtonActive]);
        bool b = ImGui::CollapsingHeader(name);
        ImGui::PopStyleColor(3);
        return b;
    };
    if (myCollapsingHeader("About ImGui::Auto()")) {
        ImGui::Auto(R"comment(
ImGui::Auto() is one simple function that can create GUI for almost any data structure using ImGui functions.
a. Your data is presented in tree-like structure that is defined by your type.
    The generated code is highly efficient.
b. Const types are display only, the user cannot modify them.
    Use ImGui::as_cost() to permit the user to modify a non-const type.
The following types are supported (with nesting):
1 Strings. Flavours of std::string and char*. Use std::string for input.
2 Numbers. Integers, Floating points, ImVec{2,4}
3 STL Containers. Standard containers are supported. (std::vector, std::map,...)
The contained type has to be supported. If it is not, see 8.
4 Pointers, arrays. Pointed type must be supported.
5 std::pair, std::tuple. Types used must be supported.
6 structs and simple classes! The struct is converted to a tuple, and displayed as such.
    * Requirements: C++14 compiler (GCC-5.0+, Clang, Visual Studio 2017 with /std:c++17, ...)
    * How? I'm using this libary https://github.com/apolukhin/magic_get (ImGui::Auto() is only VS2017 C++17 tested.)
7 Functions. A void(void) type function becomes simple button.
For other types, you can input the arguments and calculate the result.
Not yet implemented!
8 You can define ImGui::Auto() for your own type! Use the following macros:
    * METAENGINE_GUI_DEFINE_INLINE(template_spec, type_spec, code)	single line definition, teplates shouldn't have commas inside!
    * METAENGINE_GUI_DEFINE_BEGIN(template_spec, type_spec)        start multiple line definition, no commas in arguments!
    * METAENGINE_GUI_DEFINE_BEGIN_P(template_spec, type_spec)      start multiple line definition, can have commas, use parentheses!
    * METAENGINE_GUI_DEFINE_END                                    end multiple line definition with this.
where
    * template_spec   describes how the type is templated. For fully specialized, use "template<>" only
    * type_spec       is the type for witch you define the ImGui::Auto() function.
	* var             will be the generated function argument of type type_spec.
	* name            is the const std::string& given by the user, and/or generated by the caller ImGui::Auto function
Example:           METAENGINE_GUI_DEFINE_INLINE(template<>, bool, ImGui::Checkbox(name.c_str(), &var);)
Tipps: - You may use ImGui::Auto_t<type>::Auto(var, name) functions directly.
        - Check imgui::detail namespace for other helper functions.

The libary uses partial template specialization, so definitions can overlap, the most specialized will be chosen.
However, using large, nested structs can lead to slow compilation.
Container data, large structs and tuples are hidden by default.
)comment");
        if (ImGui::TreeNode("TODO-s")) {
            ImGui::Auto(R"todos(
	1	Insert items to (non-const) set, map, list, ect
	2	Call any function or function object.
	3	Deduce function arguments as a tuple. User can edit the arguments, call the function (button), and view return value.
	4	All of the above needs self ImGui::Auto() allocated memmory. Current plan is to prioritize low memory usage
		over user experiance. (Beacuse if you want good UI, you code it, not generate it.)
		Plan A:
			a)	Data is stored in a map. Key can be the presneted object's address.
			b)	Data is allocated when not already present in the map
			c)	std::unique_ptr<void, deleter> is used in the map with deleter function manually added (or equivalent something)
			d)	The first call for ImGui::Auto at every few frames should delete
				(some of, or) all data that was not accesed in the last (few) frames. How?
		This means after closing a TreeNode, and opening it, the temporary values might be deleted and recreated.
)todos");
            ImGui::TreePop();
        }
    }
    if (myCollapsingHeader("1. String")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	ImGui::Auto("Hello Imgui::Auto() !"); //This is how this text is written as well.)code");
        ImGui::Auto("Hello Imgui::Auto() !");//This is how this text is written as well.

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::string str = "Hello ImGui::Auto() for strings!";
	ImGui::Auto(str, "asd");)code");
        static std::string str = "Hello ImGui::Auto() for strings!";
        ImGui::Auto(str, "str");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::string str2 = "ImGui::Auto()\n Automatically uses multiline input for strings!\n:)";
	ImGui::Auto(str2, "str2");)code");
        static std::string str2 =
                "ImGui::Auto()\n Automatically uses multiline input for strings!\n:)";
        ImGui::Auto(str2, "str2");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static const std::string conststr = "Const types are not to be changed!";
	ImGui::Auto(conststr, "conststr");)code");
        static const std::string conststr = "Const types are not to be changed!";
        ImGui::Auto(conststr, "conststr");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	char * buffer = "To edit a string use std::string. Manual buffers are unwelcome here.";
	ImGui::Auto(buffer, "buffer");)code");
        char *buffer = "To edit a string use std::string. Manual buffers are unwelcome here.";
        ImGui::Auto(buffer, "buffer");

        ImGui::Unindent();
    }
    if (myCollapsingHeader("2. Numbers")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	static int i = 42;
	ImGui::Auto(i, "i");)code");
        static int i = 42;
        ImGui::Auto(i, "i");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static float f = 3.14;
	ImGui::Auto(f, "f");)code");
        static float f = 3.14;
        ImGui::Auto(f, "f");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static ImVec4 f4 = {1.5f,2.1f,3.4f,4.3f};
	ImGui::Auto(f4, "f4");)code");
        static ImVec4 f4 = {1.5f, 2.1f, 3.4f, 4.3f};
        ImGui::Auto(f4, "f4");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static const ImVec2 f2 = {1.f,2.f};
	ImGui::Auto(f2, "f2");)code");
        static const ImVec2 f2 = {1.f, 2.f};
        ImGui::Auto(f2, "f2");

        ImGui::Unindent();
    }
    if (myCollapsingHeader("3. Containers")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	static std::vector<std::string> vec = { "First string","Second str",":)" };
	ImGui::Auto(vec,"vec");)code");
        static std::vector<std::string> vec = {"First string", "Second str", ":)"};
        ImGui::Auto(vec, "vec");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static const std::vector<float> constvec = { 3,1,2.1f,4,3,4,5 };
	ImGui::Auto(constvec,"constvec");	//Cannot change vector, nor values)code");
        static const std::vector<float> constvec = {3, 1, 2.1f, 4, 3, 4, 5};
        ImGui::Auto(constvec, "constvec");//Cannot change vector, nor values

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::vector<bool> bvec = { false, true, false, false };
	ImGui::Auto(bvec,"bvec");)code");
        static std::vector<bool> bvec = {false, true, false, false};
        ImGui::Auto(bvec, "bvec");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static const std::vector<bool> constbvec = { false, true, false, false };
	ImGui::Auto(constbvec,"constbvec");
	)code");
        static const std::vector<bool> constbvec = {false, true, false, false};
        ImGui::Auto(constbvec, "constbvec");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::map<int, float> map = { {3,2},{1,2} };
	ImGui::Auto(map, "map");	// insert and other operations)code");
        static std::map<int, float> map = {{3, 2}, {1, 2}};
        ImGui::Auto(map, "map");// insert and other operations

        if (ImGui::TreeNode("All cases")) {
            ImGui::Auto(R"code(
	static std::deque<bool> deque = { false, true, false, false };
	ImGui::Auto(deque,"deque");)code");
            static std::deque<bool> deque = {false, true, false, false};
            ImGui::Auto(deque, "deque");

            ImGui::NewLine();
            ImGui::Separator();

            ImGui::Auto(R"code(
	static std::set<char*> set = { "set","with","char*" };
	ImGui::Auto(set,"set");)code");
            static std::set<char *> set = {"set", "with",
                                           "char*"};//for some reason, this does not work
            ImGui::Auto(set, "set");                // the problem is with the const iterator, but

            ImGui::NewLine();
            ImGui::Separator();

            ImGui::Auto(R"code(
	static std::map<char*, std::string> map = { {"asd","somevalue"},{"bsd","value"} };
	ImGui::Auto(map, "map");	// insert and other operations)code");
            static std::map<char *, std::string> map = {{"asd", "somevalue"}, {"bsd", "value"}};
            ImGui::Auto(map, "map");// insert and other operations

            ImGui::TreePop();
        }
        ImGui::Unindent();
    }
    if (myCollapsingHeader("4. Pointers and Arrays")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	static float *pf = nullptr;
	ImGui::Auto(pf, "pf");)code");
        static float *pf = nullptr;
        ImGui::Auto(pf, "pf");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static int i=10, *pi=&i;
	ImGui::Auto(pi, "pi");)code");
        static int i = 10, *pi = &i;
        ImGui::Auto(pi, "pi");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static const std::string cs= "I cannot be changed!", * cps=&cs;
	ImGui::Auto(cps, "cps");)code");
        static const std::string cs = "I cannot be changed!", *cps = &cs;
        ImGui::Auto(cps, "cps");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::string str = "I can be changed! (my pointee cannot)";
	static std::string *const strpc = &str;)code");
        static std::string str = "I can be changed! (my pointee cannot)";
        static std::string *const strpc = &str;
        ImGui::Auto(strpc, "strpc");

        ImGui::NewLine();
        ImGui::Separator();
        ImGui::Auto(R"code(
	static std::array<float,5> farray = { 1.2, 3.4, 5.6, 7.8, 9.0 };
	ImGui::Auto(farray, "std::array");)code");
        static std::array<float, 5> farray = {1.2, 3.4, 5.6, 7.8, 9.0};
        ImGui::Auto(farray, "std::array");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static float farr[5] = { 1.2, 3.4, 5.6, 7.8, 9.0 };
	ImGui::Auto(farr, "float[5]");)code");
        static float farr[5] = {11.2, 3.4, 5.6, 7.8, 911.0};
        ImGui::Auto(farr, "float[5]");

        ImGui::Unindent();
    }
    if (myCollapsingHeader("5. Pairs and Tuples")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	static std::pair<bool, ImVec2> pair = { true,{2.1f,3.2f} };
	ImGui::Auto(pair, "pair");)code");
        static std::pair<bool, ImVec2> pair = {true, {2.1f, 3.2f}};
        ImGui::Auto(pair, "pair");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::pair<int, std::string> pair2 = { -3,"simple types appear next to each other in a pair" };
	ImGui::Auto(pair2, "pair2");)code");
        static std::pair<int, std::string> pair2 = {
                -3, "simple types appear next to each other in a pair"};
        ImGui::Auto(pair2, "pair2");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	ImGui::Auto(ImGui::as_const(pair), "as_const(pair)"); //easy way to view as const)code");
        ImGui::Auto(ImGui::as_const(pair), "as_const(pair)");//easy way to view as const

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	std::tuple<const int, std::string, ImVec2> tuple = { 42, "string in tuple", {3.1f,3.2f} };
	ImGui::Auto(tuple, "tuple");)code");
        std::tuple<const int, std::string, ImVec2> tuple = {42, "string in tuple", {3.1f, 3.2f}};
        ImGui::Auto(tuple, "tuple");

        ImGui::NewLine();
        ImGui::Separator();

        // 	ImGui::Auto(R"code(
        // const std::tuple<int, const char*, ImVec2> consttuple = { 42, "smaller tuples are inlined", {3.1f,3.2f} };
        // ImGui::Auto(consttuple, "consttuple");)code");
        // 	const std::tuple<int, const char*, ImVec2> consttuple = { 42, "Smaller tuples are inlined", {3.1f,3.2f} };
        // 	ImGui::Auto(consttuple, "consttuple");

        ImGui::Unindent();
    }
    if (myCollapsingHeader("6. Structs!!")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	struct A //Structs are automagically converted to tuples!
	{
		int i = 216;
		bool b = true;
	};
	static A a;
	ImGui::Auto("a", a);)code");
        struct A
        {
            int i = 216;
            bool b = true;
        };
        static A a;
        ImGui::Auto(a, "a");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	ImGui::Auto(ImGui::as_const(a), "as_const(a)");// const structs are possible)code");
        ImGui::Auto(ImGui::as_const(a), "as_const(a)");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	struct B
	{
		std::string str = "Unfortunatelly, cannot deduce const-ness from within a struct";
		const A a = A();
	};
	static B b;
	ImGui::Auto(b, "b");)code");
        struct B
        {
            std::string str = "Unfortunatelly, cannot deduce const-ness from within a struct";
            const A a = A();
        };
        static B b;
        ImGui::Auto(b, "b");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::vector<B> vec = { {"vector of structs!", A()}, B() };
	ImGui::Auto(vec, "vec");)code");
        static std::vector<B> vec = {{"vector of structs!", A()}, B()};
        ImGui::Auto(vec, "vec");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	struct C
	{
		std::list<B> vec;
		A *a;
	};
	static C c = { {{"Container inside a struct!", A() }}, &a };
	ImGui::Auto(c, "c");)code");
        struct C
        {
            std::list<B> vec;
            A *a;
        };
        static C c = {{{"Container inside a struct!", A()}}, &a};
        ImGui::Auto(c, "c");

        ImGui::Unindent();
    }
    if (myCollapsingHeader("Functions")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	void (*func)() = []() { ImGui::TextColored(ImVec4(0.5, 1, 0.5, 1.0), "Button pressed, function called :)"); };
	ImGui::Auto(func, "void(void) function");)code");
        void (*func)() = []() {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5, 1, 0.5, 1.0), "Button pressed, function called :)");
        };
        ImGui::Auto(func, "void(void) function");

        ImGui::Unindent();
    }
}
