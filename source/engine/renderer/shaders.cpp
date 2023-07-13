// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "shaders.hpp"

#include "engine/core/core.hpp"
#include "engine/utils/name.hpp"
#include "engine/utils/utility.hpp"

u32 ME_Shaders_LoadShader(R_ShaderEnum thisype, const char* filename) {

    u32 shader;
    R_Renderer* renderer = R_GetCurrentRenderer();

    char* source = ME_fs_readfilestring(filename);

    if (!source) {
        R_PushErrorCode("load_shader", R_ERROR_FILE_NOT_FOUND, "Shader file \"%s\" not found", filename);
        ME_fs_freestring(source);
        return METADOT_FAILED;
    }

    // Compile the shader
    shader = R_CompileShader(thisype, source);

    ME_fs_freestring(source);

    return shader;
}

R_ShaderBlock ME_Shaders_LoadShaderProgram(u32* p, const char* vertex_shader_file, const char* fragment_shader_file) {
    u32 v, f;
    v = ME_Shaders_LoadShader(R_VERTEX_SHADER, vertex_shader_file);

    if (!v) METADOT_ERROR("Failed to load vertex shader (%s): %s", vertex_shader_file, R_GetShaderMessage());

    f = ME_Shaders_LoadShader(R_FRAGMENT_SHADER, fragment_shader_file);

    if (!f) METADOT_ERROR("Failed to load fragment shader (%s): %s", fragment_shader_file, R_GetShaderMessage());

    *p = R_LinkShaders(v, f);

    if (!*p) {
        R_ShaderBlock b = {-1, -1, -1, -1};
        METADOT_ERROR("Failed to link shader program (%s + %s): %s", vertex_shader_file, fragment_shader_file, R_GetShaderMessage());
        return b;
    }

    {
        R_ShaderBlock block = R_LoadShaderBlock(*p, "gpu_Vertex", "gpu_TexCoord", "gpu_Color", "gpu_ModelViewProjectionMatrix");
        R_ActivateShaderProgram(*p, &block);

        return block;
    }
}

void ME_Shaders_FreeShader(u32 p) { R_FreeShaderProgram(p); }

u32 ShaderBase::Init() {
    this->shader = 0;
    this->block = ME_Shaders_LoadShaderProgram(&this->shader, this->vertex_shader_file.c_str(), this->fragment_shader_file.c_str());
    if (this->shader_name.empty()) {
        METADOT_WARN("Shader program load with cbase");
    } else {
        METADOT_BUG(std::format("Load shader program {0}", this->shader_name).c_str());
    }
    return this->shader;
}

void ShaderBase::Unload() { ME_Shaders_FreeShader(this->shader); }

void ShaderBase::Activate() { R_ActivateShaderProgram(this->shader, &this->block); }

//--------------------------------------------------------------------------------------------------------------------------------//

// Adds includeSource to the beginning of baseSource and returns the total string
static char* ME_add_include_file(char* baseSource, const char* includePath);

// Loads the all contents of a file into a buffer. Allocates but DOES NOT free memory
static bool ME_load_into_buffer(const char* path, char** buffer);

int ME_shader_load(GLenum type, const char* path, const char* includePath) {
    // load raw code into memory:
    char* source = 0;
    if (!ME_load_into_buffer(path, &source)) return -1;

    // add included code to original:
    source = ME_add_include_file(source, includePath);
    if (source == NULL) return -1;

    // compile:
    GLshader shader;
    int success;

    shader = glCreateShader(type);
    glShaderSource(shader, 1, (const GLchar* const*)&source, NULL);
    glCompileShader(shader);

    ME_FREE(source);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLsizei logLength;
        char message[1024];
        glGetShaderInfoLog(shader, 1024, &logLength, message);

        char messageFinal[1024 + 256];
        sprintf(messageFinal, "failed to compile shader at \"%s\" with the following info log:\n%s", path, message);
        g_engine_message_callback(ME_MESSAGE_SHADER, ME_MESSAGE_ERROR, messageFinal);

        glDeleteShader(shader);
        return -1;
    }

    return shader;
}

void ME_shader_free(GLshader id) { glDeleteShader(id); }

//--------------------------------------------------------------------------------------------------------------------------------//

int ME_program_load(const char* vertexPath, const char* vertexIncludePath, const char* fragmentPath, const char* fragmentIncludePath) {
    // load and compile shaders:
    int vertex = ME_shader_load(GL_VERTEX_SHADER, vertexPath, vertexIncludePath);
    int fragment = ME_shader_load(GL_FRAGMENT_SHADER, fragmentPath, fragmentIncludePath);
    if (vertex < 0 || fragment < 0) {
        glDeleteShader(vertex >= 0 ? vertex : 0);  // only delete if was actually created to avoid extra errors
        glDeleteShader(fragment >= 0 ? fragment : 0);
        return -1;
    }

    // link shaders:
    GLprogram program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        ME_shader_free(vertex);
        ME_shader_free(fragment);
        glDeleteProgram(program);
        return -1;
    }

    // delete shaders:
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}

int ME_compute_program_load(const char* path, const char* includePath) {
    int compute = ME_shader_load(GL_COMPUTE_SHADER, path, includePath);
    if (compute < 0) return -1;

    // link into program:
    GLprogram program = glCreateProgram();
    glAttachShader(program, compute);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        ME_shader_free(compute);
        glDeleteProgram(program);
        return -1;
    }

    // delete shaders:
    glDeleteShader(compute);

    return program;
}

void ME_program_free(GLprogram id) { glDeleteProgram(id); }

void ME_program_activate(GLprogram id) { glUseProgram(id); }

//--------------------------------------------------------------------------------------------------------------------------------//

void ME_program_uniform_int(GLprogram id, const char* name, GLint val) { glUniform1i(glGetUniformLocation(id, name), (GLint)val); }

void ME_program_uniform_uint(GLprogram id, const char* name, GLuint val) { glUniform1ui(glGetUniformLocation(id, name), (GLuint)val); }

void ME_program_uniform_float(GLprogram id, const char* name, GLfloat val) { glUniform1f(glGetUniformLocation(id, name), (GLfloat)val); }

void ME_program_uniform_double(GLprogram id, const char* name, GLdouble val) { glUniform1d(glGetUniformLocation(id, name), (GLdouble)val); }

void ME_program_uniform_vec2(GLprogram id, const char* name, MEvec2* val) { glUniform2fv(glGetUniformLocation(id, name), 1, (GLfloat*)val); }

void ME_program_uniform_vec3(GLprogram id, const char* name, MEvec3* val) { glUniform3fv(glGetUniformLocation(id, name), 1, (GLfloat*)val); }

void ME_program_uniform_vec4(GLprogram id, const char* name, MEvec4* val) { glUniform4fv(glGetUniformLocation(id, name), 1, (GLfloat*)val); }

void ME_program_uniform_mat3(GLprogram id, const char* name, MEmat3* val) { glUniformMatrix3fv(glGetUniformLocation(id, name), 1, GL_FALSE, (GLfloat*)&val->m[0][0]); }

void ME_program_uniform_mat4(GLprogram id, const char* name, MEmat4* val) { glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE, (GLfloat*)&val->m[0][0]); }

//--------------------------------------------------------------------------------------------------------------------------------//

static char* ME_add_include_file(char* baseSource, const char* includePath) {
    if (includePath == NULL) return baseSource;

    char* includeSource = 0;
    if (!ME_load_into_buffer(includePath, &includeSource)) {
        ME_FREE(baseSource);
        return NULL;
    }

    size_t baseLen = strlen(baseSource);
    size_t includeLen = strlen(includeSource);

    char* versionStart = strstr(baseSource, "#version");
    if (versionStart == NULL) {
        g_engine_message_callback(ME_MESSAGE_SHADER, ME_MESSAGE_ERROR, "shader source file did not contain a #version, unable to include another shader");

        ME_FREE(baseSource);
        ME_FREE(includeSource);
        return NULL;
    }

    unsigned int i = versionStart - baseSource;
    while (baseSource[i] != '\n') {
        i++;
        if (i >= baseLen) {
            g_engine_message_callback(ME_MESSAGE_SHADER, ME_MESSAGE_ERROR, "end of shader source file was reached before end of #version was found");
            ME_FREE(baseSource);
            ME_FREE(includeSource);
            return NULL;
        }
    }

    char* combinedSource = (char*)ME_MALLOC(sizeof(char) * (baseLen + includeLen + 1));
    memcpy(combinedSource, baseSource, sizeof(char) * i);
    memcpy(&combinedSource[i], includeSource, sizeof(char) * includeLen);
    memcpy(&combinedSource[i + includeLen], &baseSource[i + 1], sizeof(char) * (baseLen - i + 1));

    ME_FREE(baseSource);
    ME_FREE(includeSource);
    return combinedSource;
}

static bool ME_load_into_buffer(const char* path, char** buffer) {
    *buffer = 0;
    long length;
    FILE* file = fopen(path, "rb");

    if (file) {
        bool result = false;

        fseek(file, 0, SEEK_END);
        length = ftell(file);
        fseek(file, 0, SEEK_SET);
        *buffer = (char*)ME_MALLOC(length + 1);

        if (*buffer) {
            fread(*buffer, length, 1, file);
            (*buffer)[length] = '\0';
            result = true;
        } else {
            g_engine_message_callback(ME_MESSAGE_CPU_MEMORY, ME_MESSAGE_ERROR, "failed to allocate memory for shader source code");
            result = false;
        }

        fclose(file);
        return result;
    } else {
        char message[256];
        sprintf(message, "failed to open file \"%s\" for reading", path);
        g_engine_message_callback(ME_MESSAGE_FILE_IO, ME_MESSAGE_ERROR, message);
        return false;
    }
}

static uint64_t get_shader_file_timestamp(const char* filename) {
    uint64_t timestamp = 0;

#ifdef _WIN32
    struct __stat64 stFileInfo;
    if (_stat64(filename, &stFileInfo) == 0) {
        timestamp = stFileInfo.st_mtime;
    }
#else
    struct stat fileStat;

    if (stat(filename, &fileStat) == -1) {
        perror(filename);
        return 0;
    }

    timestamp = fileStat.st_mtime;
#endif

    return timestamp;
}

static std::string shader_string_from_file(const char* filename) {
    std::ifstream fs(filename);
    if (!fs) {
        return "";
    }

    std::string s(std::istreambuf_iterator<char>{fs}, std::istreambuf_iterator<char>{});

    return s;
}

ME_shader_set::~ME_shader_set() {
    for (std::pair<const shader_name_type_pair, shader_t>& shader : m_shaders) {
        glDeleteShader(shader.second.handle);
    }

    for (std::pair<const std::vector<const shader_name_type_pair*>, program_t>& program : m_programs) {
        glDeleteProgram(program.second.internal_handle);
    }
}

void ME_shader_set::set_version(const std::string& version) { m_version = version; }

void ME_shader_set::set_preamble(const std::string& preamble) { m_preamble = preamble; }

GLprogram* ME_shader_set::add_program(const std::vector<std::pair<std::string, GLenum>>& typedShaders) {
    std::vector<const shader_name_type_pair*> shaderNameTypes;

    // find references to existing shaders, and create ones that didn't exist previously.
    for (const std::pair<std::string, GLenum>& shaderNameType : typedShaders) {
        shader_name_type_pair tmpShaderNameType;
        std::tie(tmpShaderNameType.name, tmpShaderNameType.type) = shaderNameType;

        // test that the file can be opened (to catch typos or missing file bugs)
        {
            std::ifstream ifs(tmpShaderNameType.name);
            if (!ifs) {
                METADOT_ERROR("[Render] Failed to open shader %s", tmpShaderNameType.name.c_str());
            }
        }

        auto foundShader = m_shaders.emplace(std::move(tmpShaderNameType), shader_t{}).first;
        if (!foundShader->second.handle) {
            foundShader->second.handle = glCreateShader(shaderNameType.second);
            // Mask the hash to 16 bits because some implementations are limited to that number of bits.
            // The sign bit is masked out, since some shader compilers treat the #line as signed, and others treat it unsigned.
            foundShader->second.hashname = (int32_t)std::hash<std::string>()(shaderNameType.first) & 0x7FFF;
        }
        shaderNameTypes.push_back(&foundShader->first);
    }

    // ensure the programs have a canonical order
    std::sort(begin(shaderNameTypes), end(shaderNameTypes));
    shaderNameTypes.erase(std::unique(begin(shaderNameTypes), end(shaderNameTypes)), end(shaderNameTypes));

    // find the program associated to these shaders (or create it if missing)
    auto foundProgram = m_programs.emplace(shaderNameTypes, program_t{}).first;
    if (!foundProgram->second.internal_handle) {
        // public handle is 0 until the program has linked without error
        foundProgram->second.public_handle = 0;

        foundProgram->second.internal_handle = glCreateProgram();
        for (const shader_name_type_pair* shader : shaderNameTypes) {
            glAttachShader(foundProgram->second.internal_handle, m_shaders[*shader].handle);
        }
    }

    return &foundProgram->second.public_handle;
}

void ME_shader_set::update_programs() {
    // find all shaders with updated timestamps
    std::set<std::pair<const shader_name_type_pair, shader_t>*> updatedShaders;
    for (std::pair<const shader_name_type_pair, shader_t>& shader : m_shaders) {
        uint64_t timestamp = get_shader_file_timestamp(shader.first.name.c_str());
        if (timestamp > shader.second.timestamp) {
            shader.second.timestamp = timestamp;
            updatedShaders.insert(&shader);
        }
    }

    // recompile all updated shaders
    for (std::pair<const shader_name_type_pair, shader_t>* shader : updatedShaders) {
        // the #line prefix ensures error messages have the right line number for their file
        // the #line directive also allows specifying a "file name" number, which makes it possible to identify which file the error came from.
        std::string version = "#version " + m_version + "\n";

        std::string defines;
        switch (shader->first.type) {
            case GL_VERTEX_SHADER:
                defines += "#define VERTEX_SHADER\n";
                break;
            case GL_FRAGMENT_SHADER:
                defines += "#define FRAGMENT_SHADER\n";
                break;
            case GL_GEOMETRY_SHADER:
                defines += "#define GEOMETRY_SHADER\n";
                break;
            case GL_TESS_CONTROL_SHADER:
                defines += "#define TESS_CONTROL_SHADER\n";
                break;
            case GL_TESS_EVALUATION_SHADER:
                defines += "#define TESS_EVALUATION_SHADER\n";
                break;
            case GL_COMPUTE_SHADER:
                defines += "#define COMPUTE_SHADER\n";
                break;
        }

        std::string preamble_hash = std::to_string((int32_t)std::hash<std::string>()("preamble") & 0x7FFF);
        std::string preamble = "#line 1 " + preamble_hash + "\n" + m_preamble + "\n";

        std::string source_hash = std::to_string(shader->second.hashname);
        std::string source = "#line 1 " + source_hash + "\n" + shader_string_from_file(shader->first.name.c_str()) + "\n";

        const char* strings[] = {version.c_str(), defines.c_str(), preamble.c_str(), source.c_str()};
        GLint lengths[] = {(GLint)version.length(), (GLint)defines.length(), (GLint)preamble.length(), (GLint)source.length()};

        glShaderSource(shader->second.handle, sizeof(strings) / sizeof(*strings), strings, lengths);
        glCompileShader(shader->second.handle);

        GLint status;
        glGetShaderiv(shader->second.handle, GL_COMPILE_STATUS, &status);
        if (!status) {
            GLint logLength;
            glGetShaderiv(shader->second.handle, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<char> log(logLength + 1);
            glGetShaderInfoLog(shader->second.handle, logLength, NULL, log.data());

            std::string log_s = log.data();

            // replace all filename hashes in the error messages with actual filenames
            for (size_t found_preamble; (found_preamble = log_s.find(preamble_hash)) != std::string::npos;) {
                log_s.replace(found_preamble, preamble_hash.size(), "preamble");
            }
            for (size_t found_source; (found_source = log_s.find(source_hash)) != std::string::npos;) {
                log_s.replace(found_source, source_hash.size(), shader->first.name);
            }

            METADOT_ERROR("[Render] Error compiling ", shader->first.name.c_str(), "\n    ", log_s.c_str());
        }
    }

    // relink all programs that had their shaders updated and have all their shaders compiling successfully
    for (std::pair<const std::vector<const shader_name_type_pair*>, program_t>& program : m_programs) {
        bool programNeedsRelink = false;
        for (const shader_name_type_pair* programShader : program.first) {
            for (std::pair<const shader_name_type_pair, shader_t>* shader : updatedShaders) {
                if (&shader->first == programShader) {
                    programNeedsRelink = true;
                    break;
                }
            }

            if (programNeedsRelink) break;
        }

        // Don't attempt to link shaders that didn't compile successfully
        bool canRelink = true;
        if (programNeedsRelink) {
            for (const shader_name_type_pair* programShader : program.first) {
                GLint status;
                glGetShaderiv(m_shaders[*programShader].handle, GL_COMPILE_STATUS, &status);
                if (!status) {
                    canRelink = false;
                    break;
                }
            }
        }

        if (programNeedsRelink && canRelink) {
            glLinkProgram(program.second.internal_handle);

            GLint logLength;
            glGetProgramiv(program.second.internal_handle, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<char> log(logLength + 1);
            glGetProgramInfoLog(program.second.internal_handle, logLength, NULL, log.data());

            std::string log_s = log.data();

            // replace all filename hashes in the error messages with actual filenames
            std::string preamble_hash = std::to_string((int32_t)std::hash<std::string>()("preamble"));
            for (size_t found_preamble; (found_preamble = log_s.find(preamble_hash)) != std::string::npos;) {
                log_s.replace(found_preamble, preamble_hash.size(), "preamble");
            }
            for (const shader_name_type_pair* shaderInProgram : program.first) {
                std::string source_hash = std::to_string(m_shaders[*shaderInProgram].hashname);
                for (size_t found_source; (found_source = log_s.find(source_hash)) != std::string::npos;) {
                    log_s.replace(found_source, source_hash.size(), shaderInProgram->name);
                }
            }

            GLint status;
            glGetProgramiv(program.second.internal_handle, GL_LINK_STATUS, &status);

            std::string shader_names;

            for (const shader_name_type_pair* shader : program.first) {
                if (shader != program.first.front()) {
                    shader_names += ", ";
                }

                shader_names += shader->name;
            }

            if (!status) {
                METADOT_ERROR("[Render] Error linking (", shader_names, ")");
            } else {
                METADOT_INFO("[Render] Successfully linked (", shader_names, ")");
            }

            if (log[0] != '\0') {
                METADOT_INFO(log_s.c_str());
            }

            if (!status) {
                program.second.public_handle = 0;
            } else {
                program.second.public_handle = program.second.internal_handle;
            }
        }
    }
}

void ME_shader_set::set_preamble_file(const std::string& preambleFilename) { set_preamble(shader_string_from_file(preambleFilename.c_str())); }

GLprogram* ME_shader_set::add_program_from_exts(const std::vector<std::string>& shaders) {
    std::vector<std::pair<std::string, GLenum>> typedShaders;
    for (const std::string& shader : shaders) {
        size_t extLoc = shader.find_last_of('.');
        if (extLoc == std::string::npos) {
            return nullptr;
        }

        GLenum shaderType;

        std::string ext = shader.substr(extLoc + 1);
        if (ext == "vert")
            shaderType = GL_VERTEX_SHADER;
        else if (ext == "frag")
            shaderType = GL_FRAGMENT_SHADER;
        else if (ext == "geom")
            shaderType = GL_GEOMETRY_SHADER;
        else if (ext == "tesc")
            shaderType = GL_TESS_CONTROL_SHADER;
        else if (ext == "tese")
            shaderType = GL_TESS_EVALUATION_SHADER;
        else if (ext == "comp")
            shaderType = GL_COMPUTE_SHADER;
        else
            return nullptr;

        typedShaders.emplace_back(shader, shaderType);
    }

    return add_program(typedShaders);
}

GLprogram* ME_shader_set::add_program_from_combined_file(const std::string& filename, const std::vector<GLenum>& shaderTypes) {
    std::vector<std::pair<std::string, GLenum>> typedShaders;

    for (auto type : shaderTypes) typedShaders.emplace_back(filename, type);

    return add_program(typedShaders);
}
