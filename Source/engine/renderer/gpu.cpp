
#include "gpu.hpp"

#include "core/global.hpp"
#include "engine/imgui_core.hpp"

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

void Drawing::drawPolygon(R_Target *target, METAENGINE_Color col, b2Vec2 *verts, int x, int y, float scale, int count, float angle, float cx, float cy) {
    if (count < 2) return;
    b2Vec2 last = rotate_point(cx, cy, angle, verts[count - 1]);
    for (int i = 0; i < count; i++) {
        b2Vec2 rot = rotate_point(cx, cy, angle, verts[i]);
        R_Line(target, x + last.x * scale, y + last.y * scale, x + rot.x * scale, y + rot.y * scale, col);
        last = rot;
    }
}

U32 Drawing::darkenColor(U32 color, float brightness) {
    int a = (color >> 24) & 0xFF;
    int r = (int)(((color >> 16) & 0xFF) * brightness);
    int g = (int)(((color >> 8) & 0xFF) * brightness);
    int b = (int)((color & 0xFF) * brightness);

    return (a << 24) | (r << 16) | (g << 8) | b;
}

#define R_TO_STRING_GENERATOR(x) \
    case x:                      \
        return #x;               \
        break;

const char *METAENGINE::GLEnumToString(GLenum e) {
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
        // R_TO_STRING_GENERATOR(GL_DOUBLE_VEC2);
        // R_TO_STRING_GENERATOR(GL_DOUBLE_VEC3);
        // R_TO_STRING_GENERATOR(GL_DOUBLE_VEC4);
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
        // R_TO_STRING_GENERATOR(GL_DOUBLE_MAT2);
        // R_TO_STRING_GENERATOR(GL_DOUBLE_MAT3);
        // R_TO_STRING_GENERATOR(GL_DOUBLE_MAT4);
        // R_TO_STRING_GENERATOR(GL_DOUBLE_MAT2x3);
        // R_TO_STRING_GENERATOR(GL_DOUBLE_MAT2x4);
        // R_TO_STRING_GENERATOR(GL_DOUBLE_MAT3x2);
        // R_TO_STRING_GENERATOR(GL_DOUBLE_MAT3x4);
        // R_TO_STRING_GENERATOR(GL_DOUBLE_MAT4x2);
        // R_TO_STRING_GENERATOR(GL_DOUBLE_MAT4x3);
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

        // R_TO_STRING_GENERATOR(GL_IMAGE_1D);
        // R_TO_STRING_GENERATOR(GL_IMAGE_2D);
        // R_TO_STRING_GENERATOR(GL_IMAGE_3D);
        // R_TO_STRING_GENERATOR(GL_IMAGE_2D_RECT);
        // R_TO_STRING_GENERATOR(GL_IMAGE_CUBE);
        // R_TO_STRING_GENERATOR(GL_IMAGE_BUFFER);
        // R_TO_STRING_GENERATOR(GL_IMAGE_1D_ARRAY);
        // R_TO_STRING_GENERATOR(GL_IMAGE_2D_ARRAY);
        // R_TO_STRING_GENERATOR(GL_IMAGE_2D_MULTISAMPLE);
        // R_TO_STRING_GENERATOR(GL_IMAGE_2D_MULTISAMPLE_ARRAY);
        // R_TO_STRING_GENERATOR(GL_INT_IMAGE_1D);
        // R_TO_STRING_GENERATOR(GL_INT_IMAGE_2D);
        // R_TO_STRING_GENERATOR(GL_INT_IMAGE_3D);
        // R_TO_STRING_GENERATOR(GL_INT_IMAGE_2D_RECT);
        // R_TO_STRING_GENERATOR(GL_INT_IMAGE_CUBE);
        // R_TO_STRING_GENERATOR(GL_INT_IMAGE_BUFFER);
        // R_TO_STRING_GENERATOR(GL_INT_IMAGE_1D_ARRAY);
        // R_TO_STRING_GENERATOR(GL_INT_IMAGE_2D_ARRAY);
        // R_TO_STRING_GENERATOR(GL_INT_IMAGE_2D_MULTISAMPLE);
        // R_TO_STRING_GENERATOR(GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY);
        // R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_1D);
        // R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_2D);
        // R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_3D);
        // R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_2D_RECT);
        // R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_CUBE);
        // R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_BUFFER);
        // R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_1D_ARRAY);
        // R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_2D_ARRAY);
        // R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE);
        // R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY);
        // R_TO_STRING_GENERATOR(GL_UNSIGNED_INT_ATOMIC_COUNTER);
    }

    static char buffer[32];
    std::sprintf(buffer, "Unknown GLenum: (0x%04x)", e);
    return buffer;
}
#undef R_TO_STRING_GENERATOR

void METAENGINE::Detail::RenderUniformVariable(GLuint program, GLenum type, const char *name, GLint location) {
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

        default:
            ImGui::Text("%s has type %s, which isn't supported yet!", name, GLEnumToString(type));
            break;
    }
}

#undef R_INTROSPECTION_GENERATE_VARIABLE_RENDER
#undef R_INTROSPECTION_GENERATE_MATRIX_RENDER

float METAENGINE::Detail::GetScrollableHeight() { return ImGui::GetTextLineHeight() * 16; }

void METAENGINE::IntrospectShader(const char *label, GLuint program) {
    METADOT_ASSERT(label != nullptr, ("The label supplied with program: {} is nullptr", program));
    METADOT_ASSERT(glIsProgram(program), ("The program: {} is not a valid shader program", program));

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

            for (const auto &shader : attached_shaders) {
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
                    auto y_size = std::min(ImGui::CalcTextSize(source.data()).y, Detail::GetScrollableHeight());
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

void METAENGINE::IntrospectVertexArray(const char *label, GLuint vao) {
    METADOT_ASSERT(label != nullptr, ("The label supplied with VAO: %u is nullptr", vao));
    METADOT_ASSERT(glIsVertexArray(vao), ("The VAO: %u is not a valid vertex array object", vao));

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
                ImGui::Text("Offset in bytes: %" PRIdPTR "", (intptr_t)offset);

                GLint usage = 0;
                glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_USAGE, &usage);
                ImGui::Text("Usage: %s", GLEnumToString(usage));

                // Create table with indexes and actual contents
                if (ImGui::TreeNode("Buffer Contents")) {
                    ImGui::BeginChild(ImGui::GetID("vbo contents"), ImVec2(-1.0f, Detail::GetScrollableHeight()), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
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

DebugDraw::DebugDraw(R_Target *target) {
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

METAENGINE_Color DebugDraw::convertColor(const b2Color &color) { return {(U8)(color.r * 255), (U8)(color.g * 255), (U8)(color.b * 255), (U8)(color.a * 255)}; }

void DebugDraw::DrawPolygon(const b2Vec2 *vertices, I32 vertexCount, const b2Color &color) {
    b2Vec2 *verts = new b2Vec2[vertexCount];

    for (int i = 0; i < vertexCount; i++) {
        verts[i] = transform(vertices[i]);
    }

    // the "(float*)verts" assumes a b2Vec2 is equal to two floats (which it is)
    R_Polygon(target, vertexCount, (float *)verts, convertColor(color));

    delete[] verts;
}

void DebugDraw::DrawSolidPolygon(const b2Vec2 *vertices, I32 vertexCount, const b2Color &color) {
    b2Vec2 *verts = new b2Vec2[vertexCount];

    for (int i = 0; i < vertexCount; i++) {
        verts[i] = transform(vertices[i]);
    }

    // the "(float*)verts" assumes a b2Vec2 is equal to two floats (which it is)
    METAENGINE_Color c2 = convertColor(color);
    c2.a *= 0.25;
    R_PolygonFilled(target, vertexCount, (float *)verts, c2);
    R_Polygon(target, vertexCount, (float *)verts, convertColor(color));

    delete[] verts;
}

void DebugDraw::DrawCircle(const b2Vec2 &center, float radius, const b2Color &color) {
    b2Vec2 tr = transform(center);
    R_Circle(target, tr.x, tr.y, radius * scale, convertColor(color));
}

void DebugDraw::DrawSolidCircle(const b2Vec2 &center, float radius, const b2Vec2 &axis, const b2Color &color) {
    b2Vec2 tr = transform(center);
    R_CircleFilled(target, tr.x, tr.y, radius * scale, convertColor(color));
}

void DebugDraw::DrawSegment(const b2Vec2 &p1, const b2Vec2 &p2, const b2Color &color) {
    b2Vec2 tr1 = transform(p1);
    b2Vec2 tr2 = transform(p2);
    R_Line(target, tr1.x, tr1.y, tr2.x, tr2.y, convertColor(color));
}

void DebugDraw::DrawTransform(const b2Transform &xf) {
    const float k_axisScale = 8.0f;
    b2Vec2 p1 = xf.p, p2;
    b2Vec2 tr1 = transform(p1), tr2;

    p2 = p1 + k_axisScale * xf.q.GetXAxis();
    tr2 = transform(p2);
    R_Line(target, tr1.x, tr1.y, tr2.x, tr2.y, {0xff, 0x00, 0x00, 0xcc});

    p2 = p1 + k_axisScale * xf.q.GetYAxis();
    tr2 = transform(p2);
    R_Line(target, tr1.x, tr1.y, tr2.x, tr2.y, {0x00, 0xff, 0x00, 0xcc});
}

void DebugDraw::DrawPoint(const b2Vec2 &p, float size, const b2Color &color) {
    b2Vec2 tr = transform(p);
    R_CircleFilled(target, tr.x, tr.y, 2, convertColor(color));
}

void DebugDraw::DrawString(int x, int y, const char *string, ...) {}

void DebugDraw::DrawString(const b2Vec2 &p, const char *string, ...) {}

void DebugDraw::DrawAABB(b2AABB *aabb, const b2Color &color) {
    b2Vec2 tr1 = transform(aabb->lowerBound);
    b2Vec2 tr2 = transform(aabb->upperBound);
    R_Line(target, tr1.x, tr1.y, tr2.x, tr1.y, convertColor(color));
    R_Line(target, tr2.x, tr1.y, tr2.x, tr2.y, convertColor(color));
    R_Line(target, tr2.x, tr2.y, tr1.x, tr2.y, convertColor(color));
    R_Line(target, tr1.x, tr2.y, tr1.x, tr1.y, convertColor(color));
}

namespace SurfaceBase {

SDL_Texture *R_Create_texture_from_surface(SDL_Renderer *sdlRenderer, SDL_Surface *surf, int format = SDL_PIXELFORMAT_RGBA8888, bool destroySurface = true);
SDL_Surface *R_Create_filled_surface_rgba(int w, int h, Uint8 color_key_r, Uint8 color_key_g, Uint8 color_key_b, Uint8 alpha = 255);

Uint32 get_pixel32(SDL_Surface *surface, int x, int y);

Uint8 merge_channel(Uint8 a, Uint8 b, float amount);

void put_pixel32(SDL_Surface *surface, int x, int y, Uint32 pixel);

void R_Surface_horizontal_line_color_rgba(SDL_Surface *surface, int y, int x1, int x2, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
void R_Surface_vertical_line_color_rgba(SDL_Surface *surface, int x, int y1, int y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
bool R_Surface_circle_color_rgba(SDL_Surface *surface, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);

SDL_Surface *R_Surface_grayscale(SDL_Surface *surface);
SDL_Surface *R_Surface_invert(SDL_Surface *surface);
SDL_Surface *R_Surface_merge_color_rgba(SDL_Surface *surface, Uint8 color_key_r, Uint8 color_key_g, Uint8 color_key_b, float amount);
SDL_Surface *R_Surface_recolor_rgbar(SDL_Surface *surface, Uint8 color_key_r, Uint8 color_key_g, Uint8 color_key_b, float amount);
SDL_Surface *R_Surface_remove_color_rgba(SDL_Surface *surface, Uint8 color_key_r, Uint8 color_key_g, Uint8 color_key_b);
SDL_Surface *R_Surface_flip(SDL_Surface *surface, int flags);
};  // namespace SurfaceBase

namespace SurfaceBase {
Uint32 get_pixel32(SDL_Surface *surface, int x, int y) {
    if (surface != NULL) {
        if (x >= 0 && x < surface->w && y >= 0 && y < surface->h) {
            Uint32 *ptr = (Uint32 *)surface->pixels;
            int lineoffset = y * (surface->pitch / 4);
            return ptr[lineoffset + x];
        }
    }
    return 0;
}

Uint8 merge_channel(Uint8 a, Uint8 b, float amount) {
    float result = (b * amount) + (a * (1.0f - amount));
    return (Uint8)result;
}

void put_pixel32(SDL_Surface *surface, int x, int y, Uint32 pixel) {
    if (surface == NULL) {
        return;
    }
    if (x < surface->w && y < surface->h) {

        Uint32 *pixels = (Uint32 *)surface->pixels;

        pixels[(y * surface->w) + x] = pixel;
    }
}

SDL_Texture *R_Create_texture_from_surface(SDL_Renderer *sdlRenderer, SDL_Surface *surf, int format, bool destroySurface) {
    if (surf == NULL) {
        return NULL;
    }

    if (sdlRenderer == NULL) {
        if (destroySurface) {
            SDL_FreeSurface(surf);
            surf = NULL;
        }
        return NULL;
    }
    SDL_Surface *cast_img = SDL_ConvertSurfaceFormat(surf, format, 0);
    SDL_Rect rect = {0, 0, cast_img->w, cast_img->h};
    SDL_Texture *newTexture = SDL_CreateTexture(sdlRenderer, format, SDL_TEXTUREACCESS_STATIC, cast_img->w, cast_img->h);
    SDL_UpdateTexture(newTexture, &rect, cast_img->pixels, cast_img->w * 4);
    SDL_SetTextureBlendMode(newTexture, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(cast_img);
    if (destroySurface) {
        SDL_FreeSurface(surf);
        surf = NULL;
    }
    return newTexture;
}

SDL_Surface *R_Create_filled_surface_rgba(int w, int h, Uint8 color_key_r, Uint8 color_key_g, Uint8 color_key_b, Uint8 alpha) {
    if (w <= 0 || h <= 0) {
        return NULL;
    }
    SDL_Surface *newSurface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA8888);
    if (newSurface == NULL) {
        return NULL;
    }
    int x = 0, y = 0;
    bool surface_is_locked = SDL_MUSTLOCK(newSurface);
    if (surface_is_locked) {
        SDL_LockSurface(newSurface);
    }

    Uint8 rr = 0, bb = 0, gg = 0, aa = 0;
    Uint32 pixel;
    for (x = 0; x < newSurface->w; x++) {

        for (y = 0; y < newSurface->h; y++) {

            pixel = SDL_MapRGBA(newSurface->format, color_key_r, color_key_g, color_key_b, alpha);
            put_pixel32(newSurface, x, y, pixel);
        }
    }

    if (surface_is_locked) {
        SDL_UnlockSurface(newSurface);
    }
    return newSurface;
}

void surface_merge_color_rgba(SDL_Surface *surface, int y, int x1, int x2, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (surface == NULL) {
        return;
    }
    if (surface->w <= x1 && surface->h <= y) {
        return;
    }
    int temp = SDL_min(x1, x2);
    x2 = SDL_max(x1, x2);
    x1 = temp;
    Uint32 pixel;
    for (int i = x1; i < x2; i++) {
        pixel = SDL_MapRGBA(surface->format, r, g, b, a);
        put_pixel32(surface, i, y, pixel);
    }
}

void R_Surface_horizontal_line_color_rgba(SDL_Surface *surface, int y, int x1, int x2, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (surface == NULL) {
        return;
    }
    if (surface->w <= x1 && surface->h <= y) {
        return;
    }
    int temp = SDL_min(x1, x2);
    x2 = SDL_max(x1, x2);
    x1 = temp;
    Uint32 pixel;
    for (int i = x1; i < x2; i++) {
        pixel = SDL_MapRGBA(surface->format, r, g, b, a);
        put_pixel32(surface, i, y, pixel);
    }
}

void R_Surface_vertical_line_color_rgba(SDL_Surface *surface, int x, int y1, int y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (surface == NULL) {
        return;
    }
    if (surface->h <= y1 && surface->w <= x) {
        return;
    }
    int temp = SDL_min(y1, y2);
    y2 = SDL_max(y1, y2);
    y1 = temp;
    Uint32 pixel;
    for (int i = y1; i < y2; i++) {
        pixel = SDL_MapRGBA(surface->format, r, g, b, a);
        put_pixel32(surface, x, i, pixel);
    }
}

bool R_Surface_circle_color_rgba(SDL_Surface *surface, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (surface == NULL) {
        return false;
    }

    Sint16 cx = 0;
    Sint16 cy = rad;
    Sint16 ocx = (Sint16)0xffff;
    Sint16 ocy = (Sint16)0xffff;
    Sint16 df = 1 - rad;
    Sint16 d_e = 3;
    Sint16 d_se = -2 * rad + 5;
    Sint16 xpcx, xmcx, xpcy, xmcy;
    Sint16 ypcy, ymcy, ypcx, ymcx;

    float alphaRatio = 1;
    if (rad <= 0) {
        return false;
    }
    do {
        xpcx = x + cx;
        xmcx = x - cx;
        xpcy = x + cy;
        xmcy = x - cy;
        if (ocy != cy) {
            if (cy > 0) {
                ypcy = y + cy;
                ymcy = y - cy;
                R_Surface_horizontal_line_color_rgba(surface, ypcy, xmcx, xpcx, r, g, b, a);
                R_Surface_horizontal_line_color_rgba(surface, ymcy, xmcx, xpcx, r, g, b, a);
            }

            ocy = cy;
        }
        if (ocx != cx) {
            if (cx != cy) {
                if (cx > 0) {
                    ypcx = y + cx;
                    ymcx = y - cx;
                    R_Surface_horizontal_line_color_rgba(surface, ymcx, xmcy, xpcy, r, g, b, a);
                    R_Surface_horizontal_line_color_rgba(surface, ypcx, xmcy, xpcy, r, g, b, a);
                } else {
                    R_Surface_horizontal_line_color_rgba(surface, y, xmcy, xpcy, r, g, b, a);
                }
            }
            ocx = cx;
        }

        if (df < 0) {
            df += d_e;
            d_e += 2;
            d_se += 2;
        } else {
            df += d_se;
            d_e += 2;
            d_se += 4;
            cy--;
        }
        cx++;
    } while (cx <= cy);
    return true;
}

SDL_Surface *R_Surface_grayscale(SDL_Surface *surface) {
    if (surface == NULL) {
        return NULL;
    }

    SDL_Surface *coloredSurface = NULL;

    coloredSurface = SDL_CreateRGBSurface(0, surface->w, surface->h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask);
    if (coloredSurface != NULL) {
        bool surface_is_locked = SDL_MUSTLOCK(surface);
        if (surface_is_locked) {
            SDL_LockSurface(surface);
        }
        Uint8 rr = 0, bb = 0, gg = 0, aa = 0;
        for (int x = 0; x < coloredSurface->w; x++) {
            for (int y = 0; y < coloredSurface->h; y++) {
                Uint32 pixel = get_pixel32(surface, x, y);
                SDL_GetRGBA(pixel, surface->format, &rr, &gg, &bb, &aa);
                Uint8 grayedColor = (rr + gg + bb) / 3;
                pixel = SDL_MapRGBA(surface->format, grayedColor, grayedColor, grayedColor, aa);
                put_pixel32(coloredSurface, x, y, pixel);
            }
        }
        if (surface_is_locked) {
            SDL_UnlockSurface(surface);
        }
        return coloredSurface;
    }
    return NULL;
}

SDL_Surface *R_Surface_invert(SDL_Surface *surface) {
    if (surface != NULL) {
        SDL_Surface *tempSurface = NULL;
        SDL_Surface *coloredSurface = NULL;

        coloredSurface = SDL_CreateRGBSurface(0, surface->w, surface->h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask);
        if (coloredSurface != NULL) {
            bool surface_is_locked = SDL_MUSTLOCK(surface);
            if (surface_is_locked) {
                SDL_LockSurface(surface);
            }
            Uint8 rr = 0, bb = 0, gg = 0, aa = 0;
            for (int x = 0; x < coloredSurface->w; x++) {
                for (int y = 0; y < coloredSurface->h; y++) {
                    Uint32 pixel = get_pixel32(surface, x, y);
                    SDL_GetRGBA(pixel, surface->format, &rr, &gg, &bb, &aa);
                    pixel = SDL_MapRGBA(surface->format, 255 - rr, 255 - gg, 255 - bb, aa);
                    put_pixel32(coloredSurface, x, y, pixel);
                }
            }
            if (surface_is_locked) {
                SDL_UnlockSurface(surface);
            }
            return coloredSurface;
        } else if (tempSurface != NULL) {
            SDL_FreeSurface(tempSurface);
            tempSurface = NULL;
        }
    }
    return NULL;
}

SDL_Surface *surface_recolor(SDL_Surface *surface, Uint8 color_key_r, Uint8 color_key_g, Uint8 color_key_b, float amount) {
    if (surface != NULL) {

        SDL_Surface *tempSurface = NULL;
        SDL_Surface *coloredSurface = NULL;

        coloredSurface = SDL_CreateRGBSurface(0, surface->w, surface->h, surface->format->BitsPerPixel, surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, surface->format->Amask);
        if (coloredSurface != NULL) {

            bool surface_is_locked = SDL_MUSTLOCK(surface);
            if (surface_is_locked) {
                SDL_LockSurface(surface);
            }

            Uint8 rr = 0, bb = 0, gg = 0, aa = 0;

            for (int x = 0; x < coloredSurface->w; x++) {

                for (int y = 0; y < coloredSurface->h; y++) {

                    Uint32 pixel = get_pixel32(surface, x, y);
                    SDL_GetRGBA(pixel, surface->format, &rr, &gg, &bb, &aa);

                    rr = merge_channel(rr, color_key_r, amount);
                    gg = merge_channel(gg, color_key_g, amount);
                    bb = merge_channel(bb, color_key_b, amount);
                    pixel = SDL_MapRGBA(surface->format, rr, gg, bb, aa);
                    put_pixel32(coloredSurface, x, y, pixel);
                }
            }

            if (surface_is_locked) {
                SDL_UnlockSurface(surface);
            }

            return coloredSurface;
        }
    }
    return NULL;
}

SDL_Surface *R_Surface_remove_color_rgba(SDL_Surface *surface, Uint8 color_key_r, Uint8 color_key_g, Uint8 color_key_b) {
    if (surface != NULL) {

        SDL_Surface *coloredSurface = NULL;

        coloredSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA8888, 0);
        if (coloredSurface != NULL) {

            bool surface_is_locked = SDL_MUSTLOCK(coloredSurface);
            if (surface_is_locked) {
                SDL_LockSurface(coloredSurface);
            }

            Uint8 rr = 0, bb = 0, gg = 0, aa = 0;
            int y = 0;
            Uint32 pixel;

            for (int x = 0; x < coloredSurface->w; x++) {

                for (y = 0; y < coloredSurface->h; y++) {

                    pixel = get_pixel32(coloredSurface, x, y);
                    SDL_GetRGBA(pixel, coloredSurface->format, &rr, &gg, &bb, &aa);

                    if (rr == color_key_r && gg == color_key_g && bb == color_key_b) {
                        pixel = SDL_MapRGBA(coloredSurface->format, 255, 0, 255, 0);
                    }
                    put_pixel32(coloredSurface, x, y, pixel);
                }
            }

            if (surface_is_locked) {
                SDL_UnlockSurface(coloredSurface);
            }
            return coloredSurface;
        }
    }
    return NULL;
}

SDL_Surface *R_Surface_flip(SDL_Surface *surface, int flags) {
    if (surface != NULL) {

        SDL_Surface *flipped = NULL;

        flipped = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA8888, 0);
        if (flipped != NULL) {

            bool surface_is_locked = SDL_MUSTLOCK(flipped);
            if (surface_is_locked) {
                SDL_LockSurface(flipped);
            }

            for (int x = 0, rx = flipped->w - 1; x < flipped->w; x++, rx--) {

                for (int y = 0, ry = flipped->h - 1; y < flipped->h; y++, ry--) {

                    Uint32 pixel = get_pixel32(surface, x, y);

                    if ((flags & SDL_FLIP_VERTICAL) && (flags & SDL_FLIP_HORIZONTAL)) {
                        put_pixel32(flipped, rx, ry, pixel);
                    } else if (flags & SDL_FLIP_HORIZONTAL) {
                        put_pixel32(flipped, rx, y, pixel);
                    } else if (flags & SDL_FLIP_VERTICAL) {
                        put_pixel32(flipped, x, ry, pixel);
                    }
                }
            }

            if (surface_is_locked) {
                SDL_UnlockSurface(flipped);
            }

            return flipped;
        }
    }
    return NULL;
}
}  // namespace SurfaceBase
