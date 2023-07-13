// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_SHADERS_HPP
#define ME_SHADERS_HPP

#include <stdlib.h>

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "engine/core/io/filesystem.h"
#include "engine/core/sdl_wrapper.h"
#include "engine/renderer/renderer_gpu.h"

// Loads a shader and prepends version/compatibility info before compiling it.
u32 ME_Shaders_LoadShader(R_ShaderEnum shader_type, const char* filename);
R_ShaderBlock ME_Shaders_LoadShaderProgram(u32* p, const char* vertex_shader_file, const char* fragment_shader_file);
void ME_Shaders_FreeShader(u32 p);

class ShaderBase {
public:
    u32 shader;
    R_ShaderBlock block;
    std::string vertex_shader_file;
    std::string fragment_shader_file;

    std::string_view shader_name;

    u32 Init();
    void Unload();
    void Activate();
};

#define ShaderBaseDecl()                                                 \
    u32 Init() {                                                         \
        this->shader_name = ME::cpp::type_name<decltype(this)>().View(); \
        return __super::Init();                                          \
    }

typedef GLuint GLshader;   // a handle to a GL shader
typedef GLuint GLprogram;  // a handle to a GL shader program

//--------------------------------------------------------------------------------------------------------------------------------//

/* Loads and compiles a shader
 * @param type the type of shader to be compiled. For example, GL_VERTEX_SHADER
 * @param path the path to the shader to be loaded
 * @param includePath the path to the shader to be included, if an include is not needed, set to NULL
 * @returns the handle to the shader, or -1 on failure
 */
int ME_shader_load(GLenum type, const char* path, const char* includePath);
/* Frees a shader
 * @param id the handle to the shader to free
 */
void ME_shader_free(GLshader id);

/* Generates a shader program with a vertex and a fragment shader
 * @param vertexPath the path to the vertex shader to use
 * @param vertexIncludePath the path to the file to be included in the vertex shader, or NULL if none is desired
 * @param fragmentPath the path to the fragment shader to use
 * @param fragmentIncludePath the path to the file to be included in the fragment shader, or NULL if none is desired
 * @returns the handle to the program, or -1 on failure
 */
int ME_program_load(const char* vertexPath, const char* vertexIncludePath, const char* fragmentPath, const char* fragmentIncludePath);
/* Generates a shader program with a compute shader
 * @param path the path to the compute shader to use
 * @param includePath the path to the file to be included in the compute shader, or NULL if none is desired
 * @returns the handle to the program, or -1 on failure
 */
int ME_compute_program_load(const char* path, const char* includePath);
/* Frees a shader program
 * @param id the handle to the shader program to free
 */
void ME_program_free(GLprogram id);
/* Activaes a shader program for drawing
 * @param id the handle to the program to activate
 */
void ME_program_activate(GLprogram id);

//--------------------------------------------------------------------------------------------------------------------------------//

// NOTE: uniform functions DO NOT activate the shader program, if the shader program is not first activated the uniform will not be set

// Sets an integer uniform
void ME_program_uniform_int(GLprogram id, const char* name, GLint val);
// Sets an unsigned integer uniform
void ME_program_uniform_uint(GLprogram id, const char* name, GLuint val);
// Sets a float uniform
void ME_program_uniform_float(GLprogram id, const char* name, GLfloat val);
// Sets a double uniform
void ME_program_uniform_double(GLprogram id, const char* name, GLdouble val);

// Sets a vector2 uniform
void ME_program_uniform_vec2(GLprogram id, const char* name, MEvec2* val);
// Sets a vector3 uniform
void ME_program_uniform_vec3(GLprogram id, const char* name, MEvec3* val);
// Sets a vector4 uniform
void ME_program_uniform_vec4(GLprogram id, const char* name, MEvec4* val);

// Sets a 3x3 matrix uniform
void ME_program_uniform_mat3(GLprogram id, const char* name, MEmat3* val);
// Sets a 4x4 matrix uniform
void ME_program_uniform_mat4(GLprogram id, const char* name, MEmat4* val);

class ME_shader_set {
    // typedefs for readability
    using shader_handle = GLshader;
    using program_handle = GLprogram;

    // filename and shader type
    struct shader_name_type_pair {
        std::string name;
        GLenum type;
        bool operator<(const shader_name_type_pair& rhs) const { return std::tie(name, type) < std::tie(rhs.name, rhs.type); }
    };

    // Shader in the ShaderSet system
    struct shader_t {
        shader_handle handle;
        // Timestamp of the last update of the shader
        uint64_t timestamp;
        // Hash of the name of the shader. This is used to recover the shader name from the GLSL compiler error messages.
        // It's not a perfect solution, but it's a miracle when it doesn't work.
        int32_t hashname;
    };

    // Program in the ShaderSet system.
    struct program_t {
        // The handle exposed externally ("public") and the most recent (succeeding/failed) linked program ("internal")
        // the public handle becomes 0 when a linking failure happens, until the linking error gets fixed.
        program_handle public_handle;
        program_handle internal_handle;
    };

    // the version in the version string that gets prepended to each shader
    std::string m_version;
    // the preamble which gets prepended to each shader (for eg. shared binding conventions)
    std::string m_preamble;
    // maps shader name/types to handles, in order to reuse shared shaders.
    std::map<shader_name_type_pair, shader_t> m_shaders;
    // allows looking up the program that represents a linked set of shaders
    std::map<std::vector<const shader_name_type_pair*>, program_t> m_programs;

public:
    ME_shader_set() = default;

    // Destructor releases all owned shaders
    ~ME_shader_set();

    // The version string to prepend to all shaders
    // Separated from the preamble because #version doesn't compile in C++
    void set_version(const std::string& version);

    // A string that gets prepended to every shader that gets compiled
    // Useful for compile-time constant #defines (like attrib locations)
    void set_preamble(const std::string& preamble);

    // Convenience for reading the preamble from a file
    // The preamble is NOT auto-reloaded.
    void set_preamble_file(const std::string& preambleFilename);

    // list of (file name, shader type) pairs
    // eg: AddProgram({ {"foo.vert", GL_VERTEX_SHADER}, {"bar.frag", GL_FRAGMENT_SHADER} });
    // To be const-correct, this should maybe return "const GLuint*". I'm trusting you not to write to that pointer.
    GLprogram* add_program(const std::vector<std::pair<std::string, GLenum>>& typedShaders);

    // Polls the timestamps of all the shaders and recompiles/relinks them if they changed
    void update_programs();

    // Convenience to add shaders based on extension file naming conventions
    // vertex shader: .vert
    // fragment shader: .frag
    // geometry shader: .geom
    // tessellation control shader: .tesc
    // tessellation evaluation shader: .tese
    // compute shader: .comp
    // eg: AddProgramFromExts({"foo.vert", "bar.frag"});
    // To be const-correct, this should maybe return "const GLuint*". I'm trusting you not to write to that pointer.
    GLprogram* add_program_from_exts(const std::vector<std::string>& shaders);

    // Convenience to add a single file that contains many shader stages.
    // Similar to what is explained here: https://software.intel.com/en-us/blogs/2012/03/26/using-ifdef-in-opengl-es-20-shaders
    // eg: AddProgramFromCombinedFile("shader.glsl", { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER });
    //
    // The shader will be compiled many times with a different #define based on which shader stage it's being used for, similarly to the Intel article above.
    // The defines are as follows:
    // vertex shader: VERTEX_SHADER
    // fragment shader: FRAGMENT_SHADER
    // geometry shader: GEOMETRY_SHADER
    // tessellation control shader: TESS_CONTROL_SHADER
    // tessellation evaluation shader: TESS_EVALUATION_SHADER
    // compute shader: COMPUTE_SHADER
    //
    // Note: These defines are not unique to the AddProgramFromCombinedFile API. The defines are also set with any other AddProgram*() API.
    // Note: You may use the defines from inside the preamble. (ie. the preamble is inserted after those defines.)
    //
    // Example combined file shader:
    //     #ifdef VERTEX_SHADER
    //     void main() { /* your vertex shader main */ }
    //     #endif
    //
    //     #ifdef FRAGMENT_SHADER
    //     void main() { /* your fragment shader main */ }
    //     #endif
    GLprogram* add_program_from_combined_file(const std::string& filename, const std::vector<GLenum>& shaderTypes);
};

#endif
