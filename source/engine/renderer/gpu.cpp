
#include "gpu.hpp"

#include "engine/core/global.hpp"
#include "engine/core/mathlib_ext.hpp"
#include "engine/game.hpp"
#include "engine/game_shaders.hpp"
#include "engine/renderer/shaders.hpp"
#include "engine/ui/imgui_layer.hpp"
#include "engine/ui/surface.h"
#include "engine/ui/surface_gl.h"
#include "libs/imgui/imgui.h"

void ME_cam_push_matrix(Camera &cam, GLprogram shader, const char *uniform) {
    GLint camera_unif = glGetUniformLocation(shader, uniform);
    glUniformMatrix4fv(camera_unif, 1, GL_FALSE, ME_value_ptr(cam.cameraMatrix));
}

// Create a R_Image from a SurfaceUI Framebuffer
R_Image *generateFBO(MEsurface_context *_vg, const float _w, const float _h, void (*draw)(MEsurface_context *, const float, const float, const float, const float)) {
    // GPU_FlushBlitBuffer(); // call GPU_FlushBlitBuffer if you're doing this in the middle of SDL_gpu blitting
    MEsurface_GLframebuffer *fb = ME_surface_gl_CreateFramebuffer(
            _vg, _w, _h, ME_SURFACE_IMAGE_NODELETE);  // IMPORTANT: NVG_IMAGE_NODELETE allows us to run ME_surface_gl_DeleteFramebuffer without freeing the GPU_Image data
    ME_surface_gl_BindFramebuffer(fb);
    glViewport(0, 0, _w, _h);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    ME_surface_BeginFrame(_vg, _w, _h, 1.0f);
    draw(_vg, 0, 0, _w, _h);  // call the drawing function that was passed as parameter
    ME_surface_EndFrame(_vg);
    /* ME_surface_gl_BindFramebuffer(0); // official documentation says to unbind, but I haven't had issues not doing it */
    R_ResetRendererState();  // not calling GPU_ResetRendererState can cause problems with SDL_gpu depending on your order of operations
    // IMPORTANT: don't run ME_surface_gl_DeleteFramebuffer, GPU_CreateImageUsingTexture takes the handle
    R_Image *return_image = R_CreateImageUsingTexture(fb->texture, false);  // should take_ownership be true?
    ME_surface_gl_DeleteFramebuffer(fb);
    return return_image;
}

void surface_test_1(MEsurface_context *_vg, const float _x, const float _y, const float _w, const float _h) {
    const float square_r = 5.0f;
    ME_surface_BeginPath(_vg);
    ME_surface_RoundedRect(_vg, _x, _y, _w, _h, square_r);
    MEsurface_paint bg_paint = ME_surface_LinearGradient(_vg, _x, _y, _x + _w, _y + _h, ME_surface_RGBA(255, 255, 255, 255), ME_surface_RGBA(255, 255, 255, 155));
    ME_surface_FillPaint(_vg, bg_paint);
    ME_surface_Fill(_vg);
}

void surface_test_2(MEsurface_context *_vg, const float _x, const float _y, const float _w, const float _h) {
    float x = _x;
    float y = _y;
    ME_surface_BeginPath(_vg);
    ME_surface_MoveTo(_vg, x, y);
    for (unsigned i = 1; i < 50000; i++) {
        ME_surface_BezierTo(_vg, x - 10.0f, y + 10.0f, x + 25, y + 25, x, y);
        x += 10.0f;
        y += 5.0f;
        if (x > _w) x = 0.0f;
        if (y > _h) y = 0.0f;
    }
    MEsurface_paint stroke_paint = ME_surface_LinearGradient(_vg, _x, _y, _w, _h, ME_surface_RGBA(255, 255, 255, 20), ME_surface_RGBA(0, 255, 255, 10));
    ME_surface_StrokePaint(_vg, stroke_paint);
    ME_surface_Stroke(_vg);
}

void surface_test_3(MEsurface_context *_vg, const float _x, const float _y, const float _arc_radius) {
    const float pie_radius = 100.0f;
    ME_surface_BeginPath(_vg);
    ME_surface_MoveTo(_vg, _x, _y);
    ME_surface_Arc(_vg, _x, _y, pie_radius, 0.0f, ME_surface_DegToRad(_arc_radius), ME_SURFACE_CW);
    ME_surface_LineTo(_vg, _x, _y);
    ME_surface_FillColor(_vg, ME_surface_RGBA(0xFF, 0xFF, 0xFF, 0xFF));
    ME_surface_Fill(_vg);
}

void begin_3d(R_Target *screen) {
    R_FlushBlitBuffer();

    R_MatrixMode(screen, R_MODEL);
    R_PushMatrix();
    R_LoadIdentity();
    R_MatrixMode(screen, R_VIEW);
    R_PushMatrix();
    R_LoadIdentity();
    R_MatrixMode(screen, R_PROJECTION);
    R_PushMatrix();
    R_LoadIdentity();
}

void end_3d(R_Target *screen) {
    R_ResetRendererState();

    R_MatrixMode(screen, R_MODEL);
    R_PopMatrix();
    R_MatrixMode(screen, R_VIEW);
    R_PopMatrix();
    R_MatrixMode(screen, R_PROJECTION);
    R_PopMatrix();
}

void draw_spinning_triangle(R_Target *screen) {
    GLfloat gldata[21];
    float mvp[16];
    float t = SDL_GetTicks() / 1000.0f;

    R_Rotate(100 * t, 0, 0.707, 0.707);
    R_Rotate(20 * t, 0.707, 0.707, 0);

    gldata[0] = 0;
    gldata[1] = 0.2f;
    gldata[2] = 0;

    gldata[3] = 1.0f;
    gldata[4] = 0.0f;
    gldata[5] = 0.0f;
    gldata[6] = 1.0f;

    gldata[7] = -0.2f;
    gldata[8] = -0.2f;
    gldata[9] = 0;

    gldata[10] = 0.0f;
    gldata[11] = 1.0f;
    gldata[12] = 0.0f;
    gldata[13] = 1.0f;

    gldata[14] = 0.2f;
    gldata[15] = -0.2f;
    gldata[16] = 0;
    gldata[17] = 0.0f;
    gldata[18] = 0.0f;
    gldata[19] = 1.0f;
    gldata[20] = 1.0f;

    global.game->GameIsolate_.shaderworker->untexturedShader->Activate();
    global.game->GameIsolate_.shaderworker->untexturedShader->Update(mvp, gldata);
}

void draw_3d_stuff(R_Target *screen) {

    // R_Clear(ENGINE()->target);

    // ME_GL_STATE_BACKUP();

    begin_3d(screen);

    draw_spinning_triangle(screen);

    end_3d(screen);

    ME_CHECK_GL_ERROR();

    // ME_GL_STATE_RESTORE();
}

void draw_more_3d_stuff(R_Target *screen) {
    float t;
    begin_3d(screen);

    t = SDL_GetTicks() / 1000.0f;
    R_Rotate(t * 60, 0, 0, 1);
    R_Translate(0.4f, 0.4f, 0);
    draw_spinning_triangle(screen);

    end_3d(screen);
}

MEvec2 ME_draw_rotate_point(float cx, float cy, float angle, MEvec2 p) {
    float s = sin(angle);
    float c = cos(angle);

    // translate point back to origin:
    p.x -= cx;
    p.y -= cy;

    // rotate point
    float xnew = p.x * c - p.y * s;
    float ynew = p.x * s + p.y * c;

    // translate point back:
    return MEvec2(xnew + cx, ynew + cy);
}

void ME_draw_polygon(R_Target *target, ME_Color col, MEvec2 *verts, int x, int y, float scale, int count, float angle, float cx, float cy) {
    if (count < 2) return;
    MEvec2 last = ME_draw_rotate_point(cx, cy, angle, verts[count - 1]);
    for (int i = 0; i < count; i++) {
        MEvec2 rot = ME_draw_rotate_point(cx, cy, angle, verts[i]);
        R_Line(target, x + last.x * scale, y + last.y * scale, x + rot.x * scale, y + rot.y * scale, col);
        last = rot;
    }
}

u32 ME_draw_darken_color(u32 color, float brightness) {
    int a = (color >> 24) & 0xFF;
    int r = (int)(((color >> 16) & 0xFF) * brightness);
    int g = (int)(((color >> 8) & 0xFF) * brightness);
    int b = (int)((color & 0xFF) * brightness);

    return (a << 24) | (r << 16) | (g << 8) | b;
}

void ME_draw_text(std::string text, ME_Color col, int x, int y, bool outline, ME_Color outline_col) {
    ImDrawList *draw_list = ImGui::GetBackgroundDrawList();

    if (outline) {

        auto outline_col_im = ImColor(outline_col.r, outline_col.g, outline_col.b, col.a);

        draw_list->AddText(ImVec2(x + 0, y - 1), outline_col_im, text.c_str());  // up
        draw_list->AddText(ImVec2(x + 0, y + 1), outline_col_im, text.c_str());  // down
        draw_list->AddText(ImVec2(x + 1, y + 0), outline_col_im, text.c_str());  // right
        draw_list->AddText(ImVec2(x - 1, y + 0), outline_col_im, text.c_str());  // left

        draw_list->AddText(ImVec2(x + 1, y + 1), outline_col_im, text.c_str());  // down-right
        draw_list->AddText(ImVec2(x - 1, y + 1), outline_col_im, text.c_str());  // down-left

        draw_list->AddText(ImVec2(x + 1, y - 1), outline_col_im, text.c_str());  // up-right
        draw_list->AddText(ImVec2(x - 1, y - 1), outline_col_im, text.c_str());  // up-left
    }

    draw_list->AddText(ImVec2(x, y), ImColor(col.r, col.g, col.b, col.a), text.c_str());  // base
}

void ME_draw_text_plate(R_Target *target, std::string text, ME_Color col, int x, int y, ME_Color backcolor) {
    auto text_size = ImGui::CalcTextSize(text.c_str());
    R_RectangleFilled(target, x - 4, y - 4, x + text_size.x + 4, y + text_size.y + 4, backcolor);
    ME_draw_text(text, col, x, y);
}

#define R_TO_STRING_GENERATOR(x) \
    case x:                      \
        return #x;               \
        break;

const char *ME_gpu_glenum_string(GLenum e) {
    switch (e) {
        // shader:
        R_TO_STRING_GENERATOR(GL_VERTEX_SHADER);
        R_TO_STRING_GENERATOR(GL_GEOMETRY_SHADER);
        R_TO_STRING_GENERATOR(GL_FRAGMENT_SHADER);

        // buffer usage:
        R_TO_STRING_GENERATOR(GL_STREAM_DRAW);
        R_TO_STRING_GENERATOR(GL_STREAM_READ);
        R_TO_STRING_GENERATOR(GL_STREAM_COPY);
        R_TO_STRING_GENERATOR(GL_STATIC_DRAW);
        R_TO_STRING_GENERATOR(GL_STATIC_READ);
        R_TO_STRING_GENERATOR(GL_STATIC_COPY);
        R_TO_STRING_GENERATOR(GL_DYNAMIC_DRAW);
        R_TO_STRING_GENERATOR(GL_DYNAMIC_READ);
        R_TO_STRING_GENERATOR(GL_DYNAMIC_COPY);

        // errors:
        R_TO_STRING_GENERATOR(GL_NO_ERROR);
        R_TO_STRING_GENERATOR(GL_INVALID_ENUM);
        R_TO_STRING_GENERATOR(GL_INVALID_VALUE);
        R_TO_STRING_GENERATOR(GL_INVALID_OPERATION);
        R_TO_STRING_GENERATOR(GL_INVALID_FRAMEBUFFER_OPERATION);
        R_TO_STRING_GENERATOR(GL_OUT_OF_MEMORY);
        R_TO_STRING_GENERATOR(GL_STACK_UNDERFLOW);
        R_TO_STRING_GENERATOR(GL_STACK_OVERFLOW);

        // types:
        R_TO_STRING_GENERATOR(GL_BYTE);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_BYTE);
        R_TO_STRING_GENERATOR(GL_SHORT);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_SHORT);
        R_TO_STRING_GENERATOR(GL_FLOAT);
        R_TO_STRING_GENERATOR(GL_FLOAT_VEC2);
        R_TO_STRING_GENERATOR(GL_FLOAT_VEC3);
        R_TO_STRING_GENERATOR(GL_FLOAT_VEC4);
        R_TO_STRING_GENERATOR(GL_DOUBLE);
        R_TO_STRING_GENERATOR(GL_DOUBLE_VEC2);
        R_TO_STRING_GENERATOR(GL_DOUBLE_VEC3);
        R_TO_STRING_GENERATOR(GL_DOUBLE_VEC4);
        R_TO_STRING_GENERATOR(GL_INT);
        R_TO_STRING_GENERATOR(GL_INT_VEC2);
        R_TO_STRING_GENERATOR(GL_INT_VEC3);
        R_TO_STRING_GENERATOR(GL_INT_VEC4);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_VEC2);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_VEC3);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_VEC4);
        R_TO_STRING_GENERATOR(GL_BOOL);
        R_TO_STRING_GENERATOR(GL_BOOL_VEC2);
        R_TO_STRING_GENERATOR(GL_BOOL_VEC3);
        R_TO_STRING_GENERATOR(GL_BOOL_VEC4);
        R_TO_STRING_GENERATOR(GL_FLOAT_MAT2);
        R_TO_STRING_GENERATOR(GL_FLOAT_MAT3);
        R_TO_STRING_GENERATOR(GL_FLOAT_MAT4);
        R_TO_STRING_GENERATOR(GL_FLOAT_MAT2x3);
        R_TO_STRING_GENERATOR(GL_FLOAT_MAT2x4);
        R_TO_STRING_GENERATOR(GL_FLOAT_MAT3x2);
        R_TO_STRING_GENERATOR(GL_FLOAT_MAT3x4);
        R_TO_STRING_GENERATOR(GL_FLOAT_MAT4x2);
        R_TO_STRING_GENERATOR(GL_FLOAT_MAT4x3);
        R_TO_STRING_GENERATOR(GL_DOUBLE_MAT2);
        R_TO_STRING_GENERATOR(GL_DOUBLE_MAT3);
        R_TO_STRING_GENERATOR(GL_DOUBLE_MAT4);
        R_TO_STRING_GENERATOR(GL_DOUBLE_MAT2x3);
        R_TO_STRING_GENERATOR(GL_DOUBLE_MAT2x4);
        R_TO_STRING_GENERATOR(GL_DOUBLE_MAT3x2);
        R_TO_STRING_GENERATOR(GL_DOUBLE_MAT3x4);
        R_TO_STRING_GENERATOR(GL_DOUBLE_MAT4x2);
        R_TO_STRING_GENERATOR(GL_DOUBLE_MAT4x3);
        R_TO_STRING_GENERATOR(GL_SAMPLER_1D);
        R_TO_STRING_GENERATOR(GL_SAMPLER_2D);
        R_TO_STRING_GENERATOR(GL_SAMPLER_3D);
        R_TO_STRING_GENERATOR(GL_SAMPLER_CUBE);
        R_TO_STRING_GENERATOR(GL_SAMPLER_1D_SHADOW);
        R_TO_STRING_GENERATOR(GL_SAMPLER_2D_SHADOW);
        R_TO_STRING_GENERATOR(GL_SAMPLER_1D_ARRAY);
        R_TO_STRING_GENERATOR(GL_SAMPLER_2D_ARRAY);
        R_TO_STRING_GENERATOR(GL_SAMPLER_1D_ARRAY_SHADOW);
        R_TO_STRING_GENERATOR(GL_SAMPLER_2D_ARRAY_SHADOW);
        R_TO_STRING_GENERATOR(GL_SAMPLER_2D_MULTISAMPLE);
        R_TO_STRING_GENERATOR(GL_SAMPLER_2D_MULTISAMPLE_ARRAY);
        R_TO_STRING_GENERATOR(GL_SAMPLER_CUBE_SHADOW);
        R_TO_STRING_GENERATOR(GL_SAMPLER_BUFFER);
        R_TO_STRING_GENERATOR(GL_SAMPLER_2D_RECT);
        R_TO_STRING_GENERATOR(GL_SAMPLER_2D_RECT_SHADOW);
        R_TO_STRING_GENERATOR(GL_INT_SAMPLER_1D);
        R_TO_STRING_GENERATOR(GL_INT_SAMPLER_2D);
        R_TO_STRING_GENERATOR(GL_INT_SAMPLER_3D);
        R_TO_STRING_GENERATOR(GL_INT_SAMPLER_CUBE);
        R_TO_STRING_GENERATOR(GL_INT_SAMPLER_1D_ARRAY);
        R_TO_STRING_GENERATOR(GL_INT_SAMPLER_2D_ARRAY);
        R_TO_STRING_GENERATOR(GL_INT_SAMPLER_2D_MULTISAMPLE);
        R_TO_STRING_GENERATOR(GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY);
        R_TO_STRING_GENERATOR(GL_INT_SAMPLER_BUFFER);
        R_TO_STRING_GENERATOR(GL_INT_SAMPLER_2D_RECT);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_1D);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_2D);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_3D);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_CUBE);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_1D_ARRAY);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_2D_ARRAY);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_BUFFER);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_SAMPLER_2D_RECT);

        R_TO_STRING_GENERATOR(GL_IMAGE_1D);
        R_TO_STRING_GENERATOR(GL_IMAGE_2D);
        R_TO_STRING_GENERATOR(GL_IMAGE_3D);
        R_TO_STRING_GENERATOR(GL_IMAGE_2D_RECT);
        R_TO_STRING_GENERATOR(GL_IMAGE_CUBE);
        R_TO_STRING_GENERATOR(GL_IMAGE_BUFFER);
        R_TO_STRING_GENERATOR(GL_IMAGE_1D_ARRAY);
        R_TO_STRING_GENERATOR(GL_IMAGE_2D_ARRAY);
        R_TO_STRING_GENERATOR(GL_IMAGE_2D_MULTISAMPLE);
        R_TO_STRING_GENERATOR(GL_IMAGE_2D_MULTISAMPLE_ARRAY);
        R_TO_STRING_GENERATOR(GL_INT_IMAGE_1D);
        R_TO_STRING_GENERATOR(GL_INT_IMAGE_2D);
        R_TO_STRING_GENERATOR(GL_INT_IMAGE_3D);
        R_TO_STRING_GENERATOR(GL_INT_IMAGE_2D_RECT);
        R_TO_STRING_GENERATOR(GL_INT_IMAGE_CUBE);
        R_TO_STRING_GENERATOR(GL_INT_IMAGE_BUFFER);
        R_TO_STRING_GENERATOR(GL_INT_IMAGE_1D_ARRAY);
        R_TO_STRING_GENERATOR(GL_INT_IMAGE_2D_ARRAY);
        R_TO_STRING_GENERATOR(GL_INT_IMAGE_2D_MULTISAMPLE);
        R_TO_STRING_GENERATOR(GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_1D);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_2D);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_3D);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_2D_RECT);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_CUBE);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_BUFFER);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_1D_ARRAY);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_2D_ARRAY);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY);
        R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_ATOMIC_COUNTER);
    }

    static char buffer[32];
    std::sprintf(buffer, "Unknown GLenum: (0x%04x)", e);
    return buffer;
}

void ME::render_uniform_variable(GLuint program, GLenum type, const char *name, GLint location) {
    static bool is_color = false;
    switch (type) {
        case GL_FLOAT:
            R_INTROSPECTION_GENERATE_VARIABLE_RENDER(GLfloat, 1, GL_FLOAT, glGetUniformfv, glProgramUniform1fv, ImGui::DragFloat);
            break;

        case GL_FLOAT_VEC2:
            R_INTROSPECTION_GENERATE_VARIABLE_RENDER(GLfloat, 2, GL_FLOAT_VEC2, glGetUniformfv, glProgramUniform2fv, ImGui::DragFloat2);
            break;

        case GL_FLOAT_VEC3: {
            ImGui::Checkbox("##is_color", &is_color);
            ImGui::SameLine();
            ImGui::Text("GL_FLOAT_VEC3 %s", name);
            ImGui::SameLine();
            float value[3];
            glGetUniformfv(program, location, &value[0]);
            if ((!is_color && ImGui::DragFloat3("", &value[0])) || (is_color && ImGui::ColorEdit3("Color", &value[0], ImGuiColorEditFlags_NoLabel)))
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
                (is_color && ImGui::ColorEdit4("Color", &value[0], ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf)))
                glProgramUniform4fv(program, location, 1, &value[0]);
        } break;

        case GL_INT:
            R_INTROSPECTION_GENERATE_VARIABLE_RENDER(GLint, 1, GL_INT, glGetUniformiv, glProgramUniform1iv, ImGui::DragInt);
            break;

        case GL_INT_VEC2:
            R_INTROSPECTION_GENERATE_VARIABLE_RENDER(GLint, 2, GL_INT, glGetUniformiv, glProgramUniform2iv, ImGui::DragInt2);
            break;

        case GL_INT_VEC3:
            R_INTROSPECTION_GENERATE_VARIABLE_RENDER(GLint, 3, GL_INT, glGetUniformiv, glProgramUniform3iv, ImGui::DragInt3);
            break;

        case GL_INT_VEC4:
            R_INTROSPECTION_GENERATE_VARIABLE_RENDER(GLint, 4, GL_INT, glGetUniformiv, glProgramUniform4iv, ImGui::DragInt4);
            break;

        case GL_UNSIGNED_INT: {
            ImGui::Text("GL_UNSIGNED_INT %s:", name);
            ImGui::SameLine();
            GLuint value[1];
            glGetUniformuiv(program, location, &value[0]);
            if (ImGui::DragScalar("", ImGuiDataType_U32, &value[0], 0.25f)) glProgramUniform1uiv(program, location, 1, &value[0]);
        } break;

        case GL_UNSIGNED_INT_VEC3: {
            ImGui::Text("GL_UNSIGNED_INT_VEC3 %s:", name);
            ImGui::SameLine();
            GLuint value[1];
            glGetUniformuiv(program, location, &value[0]);
            if (ImGui::DragScalarN("", ImGuiDataType_U32, &value[0], 3, 0.25f)) glProgramUniform3uiv(program, location, 1, &value[0]);
        } break;

        case GL_SAMPLER_2D:
            R_INTROSPECTION_GENERATE_VARIABLE_RENDER(GLint, 1, GL_SAMPLER_2D, glGetUniformiv, glProgramUniform1iv, ImGui::DragInt);
            break;

        case GL_FLOAT_MAT2:
            R_INTROSPECTION_GENERATE_MATRIX_RENDER(GLfloat, 2, 2, GL_FLOAT_MAT2, glGetUniformfv, glProgramUniformMatrix2fv, ImGui::DragFloat2);
            break;

        case GL_FLOAT_MAT3:
            R_INTROSPECTION_GENERATE_MATRIX_RENDER(GLfloat, 3, 3, GL_FLOAT_MAT3, glGetUniformfv, glProgramUniformMatrix3fv, ImGui::DragFloat3);
            break;

        case GL_FLOAT_MAT4:
            R_INTROSPECTION_GENERATE_MATRIX_RENDER(GLfloat, 4, 4, GL_FLOAT_MAT4, glGetUniformfv, glProgramUniformMatrix4fv, ImGui::DragFloat4);
            break;

        case GL_FLOAT_MAT2x3:
            R_INTROSPECTION_GENERATE_MATRIX_RENDER(GLfloat, 3, 2, GL_FLOAT_MAT2x3, glGetUniformfv, glProgramUniformMatrix2x3fv, ImGui::DragFloat3);
            break;

        case GL_FLOAT_MAT2x4:
            R_INTROSPECTION_GENERATE_MATRIX_RENDER(GLfloat, 4, 2, GL_FLOAT_MAT2x4, glGetUniformfv, glProgramUniformMatrix2x4fv, ImGui::DragFloat4);
            break;

        case GL_FLOAT_MAT3x2:
            R_INTROSPECTION_GENERATE_MATRIX_RENDER(GLfloat, 2, 3, GL_FLOAT_MAT3x2, glGetUniformfv, glProgramUniformMatrix3x2fv, ImGui::DragFloat2);
            break;

        case GL_FLOAT_MAT3x4:
            R_INTROSPECTION_GENERATE_MATRIX_RENDER(GLfloat, 4, 3, GL_FLOAT_MAT3x4, glGetUniformfv, glProgramUniformMatrix3x2fv, ImGui::DragFloat4);
            break;

        case GL_BOOL: {
            ImGui::Text("GL_BOOL %s:", name);
            ImGui::SameLine();
            GLuint value;
            glGetUniformuiv(program, location, &value);
            if (ImGui::Checkbox("", (bool *)&value)) glProgramUniform1uiv(program, location, 1, &value);
        } break;

        case GL_IMAGE_2D: {
            ImGui::Text("GL_IMAGE_2D %s:", name);
            // ImGui::SameLine();
            GLuint value;
            glGetUniformuiv(program, location, &value);
            // if (ImGui::Checkbox("", (bool*)&value)) glProgramUniform1iv(program, location, 1, &value);
            ImGui::Image((void *)(intptr_t)value, ImVec2(256, 256));
        } break;

        case GL_SAMPLER_CUBE: {
            ImGui::Text("GL_SAMPLER_CUBE %s:", name);
            // ImGui::SameLine();
            GLuint value;
            glGetUniformuiv(program, location, &value);
            ImGui::Image((void *)(intptr_t)value, ImVec2(256, 256));
        } break;

        default:
            ImGui::TextColored(ME_rgba2imvec(255, 64, 64), "%s has type %s, which isn't supported yet!", name, ME_gpu_glenum_string(type));
            break;
    }
}

float ME::get_scrollable_height() { return ImGui::GetTextLineHeight() * 16; }

void ME::inspect_shader(const char *label, GLuint program) {
    ME_ASSERT(label != nullptr, ("The label supplied with program: {} is nullptr", program));
    ME_ASSERT(glIsProgram(program), ("The program: {} is not a valid shader program", program));

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
                glGetActiveUniform(program, i, max_name_length, nullptr, &ignored, &type, name.data());

                const auto location = glGetUniformLocation(program, name.data());
                ImGui::Indent();
                ImGui::PushID(i);
                ImGui::PushItemWidth(-1.0f);
                render_uniform_variable(program, type, name.data(), location);
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

            for (const auto &shader : attached_shaders) {
                GLint source_length = 0;
                glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &source_length);
                static std::vector<char> source;
                source.resize(source_length);
                glGetShaderSource(shader, source_length, nullptr, source.data());

                GLint type = 0;
                glGetShaderiv(shader, GL_SHADER_TYPE, &type);

                ImGui::Indent();
                auto string_type = ME_gpu_glenum_string(type);
                ImGui::PushID(string_type);
                if (ImGui::CollapsingHeader(string_type)) {
                    auto y_size = std::min(ImGui::CalcTextSize(source.data()).y, get_scrollable_height());
                    ImGui::InputTextMultiline("", source.data(), source.size(), ImVec2(-1.0f, y_size), ImGuiInputTextFlags_ReadOnly);
                }
                ImGui::PopID();
                ImGui::Unindent();
            }
        }
        ImGui::Unindent();
    }
    ImGui::PopID();
}

void ME::inspect_vertex_array(const char *label, GLuint vao) {
    ME_ASSERT(label != nullptr, ("The label supplied with VAO: %u is nullptr", vao));
    ME_ASSERT(glIsVertexArray(vao), ("The VAO: %u is not a valid vertex array object", vao));

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
                auto ptr = (GLuint *)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY);
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
                ImGui::Text("Type: %s", ME_gpu_glenum_string(type));

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
                ImGui::Text("Offset in bytes: %" PRIdPTR "", (intptr_t)offset);

                GLint usage = 0;
                glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_USAGE, &usage);
                ImGui::Text("Usage: %s", ME_gpu_glenum_string(usage));

                // Create table with indexes and actual contents
                if (ImGui::TreeNode("Buffer Contents")) {
                    ImGui::BeginChild(ImGui::GetID("vbo contents"), ImVec2(-1.0f, get_scrollable_height()), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                    ImGui::Columns(dimensions + 1);
                    const char *descriptors[] = {"index", "x", "y", "z", "w"};
                    for (int j = 0; j < dimensions + 1; j++) {
                        ImGui::Text("%s", descriptors[j]);
                        ImGui::NextColumn();
                    }
                    ImGui::Separator();

                    auto ptr = (char *)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY) + (intptr_t)offset;
                    for (int j = 0, c = 0; j < size; j += stride, c++) {
                        ImGui::Text("%d", c);
                        ImGui::NextColumn();
                        for (int k = 0; k < dimensions; k++) {
                            switch (type) {
                                case GL_BYTE:
                                    ImGui::Text("% d", *(GLbyte *)&ptr[j + k * sizeof(GLbyte)]);
                                    break;
                                case GL_UNSIGNED_BYTE:
                                    ImGui::Text("%u", *(GLubyte *)&ptr[j + k * sizeof(GLubyte)]);
                                    break;
                                case GL_SHORT:
                                    ImGui::Text("% d", *(GLshort *)&ptr[j + k * sizeof(GLshort)]);
                                    break;
                                case GL_UNSIGNED_SHORT:
                                    ImGui::Text("%u", *(GLushort *)&ptr[j + k * sizeof(GLushort)]);
                                    break;
                                case GL_INT:
                                    ImGui::Text("% d", *(GLint *)&ptr[j + k * sizeof(GLint)]);
                                    break;
                                case GL_UNSIGNED_INT:
                                    ImGui::Text("%u", *(GLuint *)&ptr[j + k * sizeof(GLuint)]);
                                    break;
                                case GL_FLOAT:
                                    ImGui::Text("% f", *(GLfloat *)&ptr[j + k * sizeof(GLfloat)]);
                                    break;
                                case GL_DOUBLE:
                                    ImGui::Text("% f", *(GLdouble *)&ptr[j + k * sizeof(GLdouble)]);
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

ME_debugdraw::ME_debugdraw(R_Target *target) {
    this->target = target;
    m_drawFlags = 0;
}

ME_debugdraw::~ME_debugdraw() {}

void ME_debugdraw::Create() {}

void ME_debugdraw::Destroy() {}

PVec2 ME_debugdraw::transform(const PVec2 &pt) {
    float x = ((pt.x) * scale + xOfs);
    float y = ((pt.y) * scale + yOfs);
    return PVec2(x, y);
}

void ME_debugdraw::DrawPolygon(const PVec2 *vertices, i32 vertexCount, const ME_Color &color) {
    PVec2 *verts = new PVec2[vertexCount];

    for (int i = 0; i < vertexCount; i++) {
        verts[i] = transform(vertices[i]);
    }

    // the "(float*)verts" assumes a b2Vec2 is equal to two floats (which it is)
    R_Polygon(target, vertexCount, (float *)verts, color);

    delete[] verts;
}

void ME_debugdraw::DrawSolidPolygon(const PVec2 *vertices, i32 vertexCount, const ME_Color &color) {
    PVec2 *verts = new PVec2[vertexCount];

    for (int i = 0; i < vertexCount; i++) {
        verts[i] = transform(vertices[i]);
    }

    // the "(float*)verts" assumes a b2Vec2 is equal to two floats (which it is)
    ME_Color c2 = color;
    c2.a *= 0.25;
    R_PolygonFilled(target, vertexCount, (float *)verts, c2);
    R_Polygon(target, vertexCount, (float *)verts, color);

    delete[] verts;
}

void ME_debugdraw::DrawCircle(const PVec2 &center, float radius, const ME_Color &color) {
    PVec2 tr = transform(center);
    R_Circle(target, tr.x, tr.y, radius * scale, color);
}

void ME_debugdraw::DrawSolidCircle(const PVec2 &center, float radius, const PVec2 &axis, const ME_Color &color) {
    PVec2 tr = transform(center);
    R_CircleFilled(target, tr.x, tr.y, radius * scale, color);
}

void ME_debugdraw::DrawSegment(const PVec2 &p1, const PVec2 &p2, const ME_Color &color) {
    PVec2 tr1 = transform(p1);
    PVec2 tr2 = transform(p2);
    R_Line(target, tr1.x, tr1.y, tr2.x, tr2.y, color);
}

void ME_debugdraw::DrawTransform(const PTransform &xf) {
    const float k_axisScale = 8.0f;
    PVec2 p1 = xf.p, p2;
    PVec2 tr1 = transform(p1), tr2;

    p2 = p1 + k_axisScale * xf.q.GetXAxis();
    tr2 = transform(p2);
    R_Line(target, tr1.x, tr1.y, tr2.x, tr2.y, {0xff, 0x00, 0x00, 0xcc});

    p2 = p1 + k_axisScale * xf.q.GetYAxis();
    tr2 = transform(p2);
    R_Line(target, tr1.x, tr1.y, tr2.x, tr2.y, {0x00, 0xff, 0x00, 0xcc});
}

void ME_debugdraw::DrawPoint(const PVec2 &p, float size, const ME_Color &color) {
    PVec2 tr = transform(p);
    R_CircleFilled(target, tr.x, tr.y, 2, color);
}

void ME_debugdraw::DrawString(int x, int y, const char *string, ...) {}

void ME_debugdraw::DrawString(const PVec2 &p, const char *string, ...) {}

void ME_debugdraw::DrawAABB(b2AABB *aabb, const ME_Color &color) {
    PVec2 tr1 = transform(aabb->lowerBound);
    PVec2 tr2 = transform(aabb->upperBound);
    R_Line(target, tr1.x, tr1.y, tr2.x, tr1.y, color);
    R_Line(target, tr2.x, tr1.y, tr2.x, tr2.y, color);
    R_Line(target, tr2.x, tr2.y, tr1.x, tr2.y, color);
    R_Line(target, tr1.x, tr2.y, tr1.x, tr1.y, color);
}
