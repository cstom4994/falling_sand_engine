
#include "surface_gl.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "surface.h"

enum ME_SURFACE_GLuniformLoc { ME_SURFACE_GL_LOC_VIEWSIZE, ME_SURFACE_GL_LOC_TEX, ME_SURFACE_GL_LOC_FRAG, ME_SURFACE_GL_MAX_LOCS };

enum ME_SURFACE_GLshaderType { ME_SURFACE_SVG_SHADER_FILLGRAD, ME_SURFACE_SVG_SHADER_FILLIMG, ME_SURFACE_SVG_SHADER_SIMPLE, ME_SURFACE_SVG_SHADER_IMG };

enum ME_SURFACE_GLuniformBindings {
    ME_SURFACE_GL_FRAG_BINDING = 0,
};

struct ME_SURFACE_GLshader {
    GLuint prog;
    GLuint frag;
    GLuint vert;
    GLint loc[ME_SURFACE_GL_MAX_LOCS];
};
typedef struct ME_SURFACE_GLshader ME_SURFACE_GLshader;

struct ME_SURFACE_GLtexture {
    int id;
    GLuint tex;
    int width, height;
    int type;
    int flags;
};
typedef struct ME_SURFACE_GLtexture ME_SURFACE_GLtexture;

struct ME_SURFACE_GLblend {
    GLenum srcRGB;
    GLenum dstRGB;
    GLenum srcAlpha;
    GLenum dstAlpha;
};
typedef struct ME_SURFACE_GLblend ME_SURFACE_GLblend;

enum ME_SURFACE_GLcallType {
    ME_SURFACE_GL_NONE = 0,
    ME_SURFACE_GL_FILL,
    ME_SURFACE_GL_CONVEXFILL,
    ME_SURFACE_GL_STROKE,
    ME_SURFACE_GL_TRIANGLES,
};

struct ME_SURFACE_GLcall {
    int type;
    int image;
    int pathOffset;
    int pathCount;
    int triangleOffset;
    int triangleCount;
    int uniformOffset;
    ME_SURFACE_GLblend blendFunc;
};
typedef struct ME_SURFACE_GLcall ME_SURFACE_GLcall;

struct ME_SURFACE_GLpath {
    int fillOffset;
    int fillCount;
    int strokeOffset;
    int strokeCount;
};
typedef struct ME_SURFACE_GLpath ME_SURFACE_GLpath;

struct ME_SURFACE_GLfragUniforms {
    float scissorMat[12];  // matrices are actually 3 vec4s
    float paintMat[12];
    struct MEsurface_color innerCol;
    struct MEsurface_color outerCol;
    float scissorExt[2];
    float scissorScale[2];
    float extent[2];
    float radius;
    float feather;
    float strokeMult;
    float strokeThr;
    int texType;
    int type;
};
typedef struct ME_SURFACE_GLfragUniforms ME_SURFACE_GLfragUniforms;

struct ME_SURFACE_GLcontext {
    ME_SURFACE_GLshader shader;
    ME_SURFACE_GLtexture* textures;
    float view[2];
    int ntextures;
    int ctextures;
    int textureId;
    GLuint vertBuf;
    GLuint vertArr;
    GLuint fragBuf;
    int fragSize;
    int flags;

    // Per frame buffers
    ME_SURFACE_GLcall* calls;
    int ccalls;
    int ncalls;
    ME_SURFACE_GLpath* paths;
    int cpaths;
    int npaths;
    struct MEsurface_vertex* verts;
    int cverts;
    int nverts;
    unsigned char* uniforms;
    int cuniforms;
    int nuniforms;

    // cached state
    GLuint boundTexture;
    GLuint stencilMask;
    GLenum stencilFunc;
    GLint stencilFuncRef;
    GLuint stencilFuncMask;
    ME_SURFACE_GLblend blendFunc;

    int dummyTex;
};
typedef struct ME_SURFACE_GLcontext ME_SURFACE_GLcontext;

static int ME_surface_gl_maxi(int a, int b) { return a > b ? a : b; }

static void ME_surface_gl_bindTexture(ME_SURFACE_GLcontext* gl, GLuint tex) {
    if (gl->boundTexture != tex) {
        gl->boundTexture = tex;
        glBindTexture(GL_TEXTURE_2D, tex);
    }
}

static void ME_surface_gl_stencilMask(ME_SURFACE_GLcontext* gl, GLuint mask) {
    if (gl->stencilMask != mask) {
        gl->stencilMask = mask;
        glStencilMask(mask);
    }
}

static void ME_surface_gl_stencilFunc(ME_SURFACE_GLcontext* gl, GLenum func, GLint ref, GLuint mask) {
    if ((gl->stencilFunc != func) || (gl->stencilFuncRef != ref) || (gl->stencilFuncMask != mask)) {

        gl->stencilFunc = func;
        gl->stencilFuncRef = ref;
        gl->stencilFuncMask = mask;
        glStencilFunc(func, ref, mask);
    }
}

static void ME_surface_gl_blendFuncSeparate(ME_SURFACE_GLcontext* gl, const ME_SURFACE_GLblend* blend) {
    if ((gl->blendFunc.srcRGB != blend->srcRGB) || (gl->blendFunc.dstRGB != blend->dstRGB) || (gl->blendFunc.srcAlpha != blend->srcAlpha) || (gl->blendFunc.dstAlpha != blend->dstAlpha)) {

        gl->blendFunc = *blend;
        glBlendFuncSeparate(blend->srcRGB, blend->dstRGB, blend->srcAlpha, blend->dstAlpha);
    }
}

static ME_SURFACE_GLtexture* ME_surface_gl_allocTexture(ME_SURFACE_GLcontext* gl) {
    ME_SURFACE_GLtexture* tex = NULL;
    int i;

    for (i = 0; i < gl->ntextures; i++) {
        if (gl->textures[i].id == 0) {
            tex = &gl->textures[i];
            break;
        }
    }
    if (tex == NULL) {
        if (gl->ntextures + 1 > gl->ctextures) {
            ME_SURFACE_GLtexture* textures;
            int ctextures = ME_surface_gl_maxi(gl->ntextures + 1, 4) + gl->ctextures / 2;  // 1.5x Overallocate
            textures = (ME_SURFACE_GLtexture*)realloc(gl->textures, sizeof(ME_SURFACE_GLtexture) * ctextures);
            if (textures == NULL) return NULL;
            gl->textures = textures;
            gl->ctextures = ctextures;
        }
        tex = &gl->textures[gl->ntextures++];
    }

    memset(tex, 0, sizeof(*tex));
    tex->id = ++gl->textureId;

    return tex;
}

static ME_SURFACE_GLtexture* ME_surface_gl_findTexture(ME_SURFACE_GLcontext* gl, int id) {
    int i;
    for (i = 0; i < gl->ntextures; i++)
        if (gl->textures[i].id == id) return &gl->textures[i];
    return NULL;
}

static int ME_surface_gl_deleteTexture(ME_SURFACE_GLcontext* gl, int id) {
    int i;
    for (i = 0; i < gl->ntextures; i++) {
        if (gl->textures[i].id == id) {
            if (gl->textures[i].tex != 0 && (gl->textures[i].flags & ME_SURFACE_IMAGE_NODELETE) == 0) glDeleteTextures(1, &gl->textures[i].tex);
            memset(&gl->textures[i], 0, sizeof(gl->textures[i]));
            return 1;
        }
    }
    return 0;
}

static void ME_surface_gl_dumpShaderError(GLuint shader, const char* name, const char* type) {
    GLchar str[512 + 1];
    GLsizei len = 0;
    glGetShaderInfoLog(shader, 512, &len, str);
    if (len > 512) len = 512;
    str[len] = '\0';
    printf("Shader %s/%s error:\n%s\n", name, type, str);
}

static void ME_surface_gl_dumpProgramError(GLuint prog, const char* name) {
    GLchar str[512 + 1];
    GLsizei len = 0;
    glGetProgramInfoLog(prog, 512, &len, str);
    if (len > 512) len = 512;
    str[len] = '\0';
    printf("Program %s error:\n%s\n", name, str);
}

static void ME_surface_gl_checkError(ME_SURFACE_GLcontext* gl, const char* str) {
    GLenum err;
    if ((gl->flags & ME_SURFACE_DEBUG) == 0) return;
    err = glGetError();
    if (err != GL_NO_ERROR) {
        printf("Error %08x after %s\n", err, str);
        return;
    }
}

static int ME_surface_gl_createShader(ME_SURFACE_GLshader* shader, const char* name, const char* header, const char* opts, const char* vshader, const char* fshader) {
    GLint status;
    GLuint prog, vert, frag;
    const char* str[3];
    str[0] = header;
    str[1] = opts != NULL ? opts : "";

    memset(shader, 0, sizeof(*shader));

    prog = glCreateProgram();
    vert = glCreateShader(GL_VERTEX_SHADER);
    frag = glCreateShader(GL_FRAGMENT_SHADER);
    str[2] = vshader;
    glShaderSource(vert, 3, str, 0);
    str[2] = fshader;
    glShaderSource(frag, 3, str, 0);

    glCompileShader(vert);
    glGetShaderiv(vert, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        ME_surface_gl_dumpShaderError(vert, name, "vert");
        return 0;
    }

    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        ME_surface_gl_dumpShaderError(frag, name, "frag");
        return 0;
    }

    glAttachShader(prog, vert);
    glAttachShader(prog, frag);

    glBindAttribLocation(prog, 0, "vertex");
    glBindAttribLocation(prog, 1, "tcoord");

    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        ME_surface_gl_dumpProgramError(prog, name);
        return 0;
    }

    shader->prog = prog;
    shader->vert = vert;
    shader->frag = frag;

    return 1;
}

static void ME_surface_gl_deleteShader(ME_SURFACE_GLshader* shader) {
    if (shader->prog != 0) glDeleteProgram(shader->prog);
    if (shader->vert != 0) glDeleteShader(shader->vert);
    if (shader->frag != 0) glDeleteShader(shader->frag);
}

static void ME_surface_gl_getUniforms(ME_SURFACE_GLshader* shader) {
    shader->loc[ME_SURFACE_GL_LOC_VIEWSIZE] = glGetUniformLocation(shader->prog, "viewSize");
    shader->loc[ME_SURFACE_GL_LOC_TEX] = glGetUniformLocation(shader->prog, "tex");
    shader->loc[ME_SURFACE_GL_LOC_FRAG] = glGetUniformBlockIndex(shader->prog, "frag");
}

static int ME_surface_gl_renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data);

static int ME_surface_gl_renderCreate(void* uptr) {
    ME_SURFACE_GLcontext* gl = (ME_SURFACE_GLcontext*)uptr;
    int align = 4;

    static const char* shaderHeader =
            "#version 430 core\n"
            "#define ME_SURFACE_GL3 1\n"
            "#define USE_UNIFORMBUFFER 1\n"
            "\n";

    static const char* fillVertShader =
            "#ifdef ME_SURFACE_GL3\n"
            "   uniform vec2 viewSize;\n"
            "   in vec2 vertex;\n"
            "   in vec2 tcoord;\n"
            "   out vec2 ftcoord;\n"
            "   out vec2 fpos;\n"
            "#else\n"
            "   uniform vec2 viewSize;\n"
            "   attribute vec2 vertex;\n"
            "   attribute vec2 tcoord;\n"
            "   varying vec2 ftcoord;\n"
            "   varying vec2 fpos;\n"
            "#endif\n"
            "void main(void) {\n"
            "   ftcoord = tcoord;\n"
            "   fpos = vertex;\n"
            "   gl_Position = vec4(2.0*vertex.x/viewSize.x - 1.0, 1.0 - 2.0*vertex.y/viewSize.y, 0, 1);\n"
            "}\n";

    static const char* fillFragShader =
            "#ifdef GL_ES\n"
            "#if defined(GL_FRAGMENT_PRECISION_HIGH) || defined(ME_SURFACE_GL3)\n"
            " precision highp float;\n"
            "#else\n"
            " precision mediump float;\n"
            "#endif\n"
            "#endif\n"
            "#ifdef ME_SURFACE_GL3\n"
            "#ifdef USE_UNIFORMBUFFER\n"
            "   layout(std140) uniform frag {\n"
            "       mat3 scissorMat;\n"
            "       mat3 paintMat;\n"
            "       vec4 innerCol;\n"
            "       vec4 outerCol;\n"
            "       vec2 scissorExt;\n"
            "       vec2 scissorScale;\n"
            "       vec2 extent;\n"
            "       float radius;\n"
            "       float feather;\n"
            "       float strokeMult;\n"
            "       float strokeThr;\n"
            "       int texType;\n"
            "       int type;\n"
            "   };\n"
            "#else\n"  // ME_SURFACE_GL3 && !USE_UNIFORMBUFFER
            "   uniform vec4 frag[UNIFORMARRAY_SIZE];\n"
            "#endif\n"
            "   uniform sampler2D tex;\n"
            "   in vec2 ftcoord;\n"
            "   in vec2 fpos;\n"
            "   out vec4 outColor;\n"
            "#else\n"  // !ME_SURFACE_GL3
            "   uniform vec4 frag[UNIFORMARRAY_SIZE];\n"
            "   uniform sampler2D tex;\n"
            "   varying vec2 ftcoord;\n"
            "   varying vec2 fpos;\n"
            "#endif\n"
            "#ifndef USE_UNIFORMBUFFER\n"
            "   #define scissorMat mat3(frag[0].xyz, frag[1].xyz, frag[2].xyz)\n"
            "   #define paintMat mat3(frag[3].xyz, frag[4].xyz, frag[5].xyz)\n"
            "   #define innerCol frag[6]\n"
            "   #define outerCol frag[7]\n"
            "   #define scissorExt frag[8].xy\n"
            "   #define scissorScale frag[8].zw\n"
            "   #define extent frag[9].xy\n"
            "   #define radius frag[9].z\n"
            "   #define feather frag[9].w\n"
            "   #define strokeMult frag[10].x\n"
            "   #define strokeThr frag[10].y\n"
            "   #define texType int(frag[10].z)\n"
            "   #define type int(frag[10].w)\n"
            "#endif\n"
            "\n"
            "float sdroundrect(vec2 pt, vec2 ext, float rad) {\n"
            "   vec2 ext2 = ext - vec2(rad,rad);\n"
            "   vec2 d = abs(pt) - ext2;\n"
            "   return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rad;\n"
            "}\n"
            "\n"
            "// Scissoring\n"
            "float scissorMask(vec2 p) {\n"
            "   vec2 sc = (abs((scissorMat * vec3(p,1.0)).xy) - scissorExt);\n"
            "   sc = vec2(0.5,0.5) - sc * scissorScale;\n"
            "   return clamp(sc.x,0.0,1.0) * clamp(sc.y,0.0,1.0);\n"
            "}\n"
            "#ifdef EDGE_AA\n"
            "// Stroke - from [0..1] to clipped pyramid, where the slope is 1px.\n"
            "float strokeMask() {\n"
            "   return min(1.0, (1.0-abs(ftcoord.x*2.0-1.0))*strokeMult) * min(1.0, ftcoord.y);\n"
            "}\n"
            "#endif\n"
            "\n"
            "void main(void) {\n"
            "   vec4 result;\n"
            "   float scissor = scissorMask(fpos);\n"
            "#ifdef EDGE_AA\n"
            "   float strokeAlpha = strokeMask();\n"
            "   if (strokeAlpha < strokeThr) discard;\n"
            "#else\n"
            "   float strokeAlpha = 1.0;\n"
            "#endif\n"
            "   if (type == 0) {            // Gradient\n"
            "       // Calculate gradient color using box gradient\n"
            "       vec2 pt = (paintMat * vec3(fpos,1.0)).xy;\n"
            "       float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);\n"
            "       vec4 color = mix(innerCol,outerCol,d);\n"
            "       // Combine alpha\n"
            "       color *= strokeAlpha * scissor;\n"
            "       result = color;\n"
            "   } else if (type == 1) {     // Image\n"
            "       // Calculate color fron texture\n"
            "       vec2 pt = (paintMat * vec3(fpos,1.0)).xy / extent;\n"
            "#ifdef ME_SURFACE_GL3\n"
            "       vec4 color = texture(tex, pt);\n"
            "#else\n"
            "       vec4 color = texture2D(tex, pt);\n"
            "#endif\n"
            "       if (texType == 1) color = vec4(color.xyz*color.w,color.w);"
            "       if (texType == 2) color = vec4(color.x);"
            "       // Apply color tint and alpha.\n"
            "       color *= innerCol;\n"
            "       // Combine alpha\n"
            "       color *= strokeAlpha * scissor;\n"
            "       result = color;\n"
            "   } else if (type == 2) {     // Stencil fill\n"
            "       result = vec4(1,1,1,1);\n"
            "   } else if (type == 3) {     // Textured tris\n"
            "#ifdef ME_SURFACE_GL3\n"
            "       vec4 color = texture(tex, ftcoord);\n"
            "#else\n"
            "       vec4 color = texture2D(tex, ftcoord);\n"
            "#endif\n"
            "       if (texType == 1) color = vec4(color.xyz*color.w,color.w);"
            "       if (texType == 2) color = vec4(color.x);"
            "       color *= scissor;\n"
            "       result = color * innerCol;\n"
            "   }\n"
            "#ifdef ME_SURFACE_GL3\n"
            "   outColor = result;\n"
            "#else\n"
            "   gl_FragColor = result;\n"
            "#endif\n"
            "}\n";

    ME_surface_gl_checkError(gl, "init");

    if (gl->flags & ME_SURFACE_ANTIALIAS) {
        if (ME_surface_gl_createShader(&gl->shader, "shader", shaderHeader, "#define EDGE_AA 1\n", fillVertShader, fillFragShader) == 0) return 0;
    } else {
        if (ME_surface_gl_createShader(&gl->shader, "shader", shaderHeader, NULL, fillVertShader, fillFragShader) == 0) return 0;
    }

    ME_surface_gl_checkError(gl, "uniform locations");
    ME_surface_gl_getUniforms(&gl->shader);

    // Create dynamic vertex array
    glGenVertexArrays(1, &gl->vertArr);
    glGenBuffers(1, &gl->vertBuf);

    // Create UBOs
    glUniformBlockBinding(gl->shader.prog, gl->shader.loc[ME_SURFACE_GL_LOC_FRAG], ME_SURFACE_GL_FRAG_BINDING);
    glGenBuffers(1, &gl->fragBuf);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &align);

    gl->fragSize = sizeof(ME_SURFACE_GLfragUniforms) + align - sizeof(ME_SURFACE_GLfragUniforms) % align;

    // Some platforms does not allow to have samples to unset textures.
    // Create empty one which is bound when there's no texture specified.
    gl->dummyTex = ME_surface_gl_renderCreateTexture(gl, ME_SURFACE_TEXTURE_ALPHA, 1, 1, 0, NULL);

    ME_surface_gl_checkError(gl, "create done");

    glFinish();

    return 1;
}

static int ME_surface_gl_renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data) {
    ME_SURFACE_GLcontext* gl = (ME_SURFACE_GLcontext*)uptr;
    ME_SURFACE_GLtexture* tex = ME_surface_gl_allocTexture(gl);

    if (tex == NULL) return 0;

    glGenTextures(1, &tex->tex);
    tex->width = w;
    tex->height = h;
    tex->type = type;
    tex->flags = imageFlags;
    ME_surface_gl_bindTexture(gl, tex->tex);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->width);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    if (type == ME_SURFACE_TEXTURE_RGBA)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, data);

    if (imageFlags & ME_SURFACE_IMAGE_GENERATE_MIPMAPS) {
        if (imageFlags & ME_SURFACE_IMAGE_NEAREST) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
    } else {
        if (imageFlags & ME_SURFACE_IMAGE_NEAREST) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
    }

    if (imageFlags & ME_SURFACE_IMAGE_NEAREST) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    if (imageFlags & ME_SURFACE_IMAGE_REPEATX)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    if (imageFlags & ME_SURFACE_IMAGE_REPEATY)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    // The new way to build mipmaps on GL3
    if (imageFlags & ME_SURFACE_IMAGE_GENERATE_MIPMAPS) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    ME_surface_gl_checkError(gl, "create tex");
    ME_surface_gl_bindTexture(gl, 0);

    return tex->id;
}

static int ME_surface_gl_renderDeleteTexture(void* uptr, int image) {
    ME_SURFACE_GLcontext* gl = (ME_SURFACE_GLcontext*)uptr;
    return ME_surface_gl_deleteTexture(gl, image);
}

static int ME_surface_gl_renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data) {
    ME_SURFACE_GLcontext* gl = (ME_SURFACE_GLcontext*)uptr;
    ME_SURFACE_GLtexture* tex = ME_surface_gl_findTexture(gl, image);

    if (tex == NULL) return 0;
    ME_surface_gl_bindTexture(gl, tex->tex);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->width);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, x);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, y);

    if (tex->type == ME_SURFACE_TEXTURE_RGBA)
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
    else
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RED, GL_UNSIGNED_BYTE, data);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    ME_surface_gl_bindTexture(gl, 0);

    return 1;
}

static int ME_surface_gl_renderGetTextureSize(void* uptr, int image, int* w, int* h) {
    ME_SURFACE_GLcontext* gl = (ME_SURFACE_GLcontext*)uptr;
    ME_SURFACE_GLtexture* tex = ME_surface_gl_findTexture(gl, image);
    if (tex == NULL) return 0;
    *w = tex->width;
    *h = tex->height;
    return 1;
}

static void ME_surface_gl_xformToMat3x4(float* m3, float* t) {
    m3[0] = t[0];
    m3[1] = t[1];
    m3[2] = 0.0f;
    m3[3] = 0.0f;
    m3[4] = t[2];
    m3[5] = t[3];
    m3[6] = 0.0f;
    m3[7] = 0.0f;
    m3[8] = t[4];
    m3[9] = t[5];
    m3[10] = 1.0f;
    m3[11] = 0.0f;
}

static MEsurface_color ME_surface_gl_premulColor(MEsurface_color c) {
    c.r *= c.a;
    c.g *= c.a;
    c.b *= c.a;
    return c;
}

static int ME_surface_gl_convertPaint(ME_SURFACE_GLcontext* gl, ME_SURFACE_GLfragUniforms* frag, MEsurface_paint* paint, MEsurface_scissor* scissor, float width, float fringe, float strokeThr) {
    ME_SURFACE_GLtexture* tex = NULL;
    float invxform[6];

    memset(frag, 0, sizeof(*frag));

    frag->innerCol = ME_surface_gl_premulColor(paint->innerColor);
    frag->outerCol = ME_surface_gl_premulColor(paint->outerColor);

    if (scissor->extent[0] < -0.5f || scissor->extent[1] < -0.5f) {
        memset(frag->scissorMat, 0, sizeof(frag->scissorMat));
        frag->scissorExt[0] = 1.0f;
        frag->scissorExt[1] = 1.0f;
        frag->scissorScale[0] = 1.0f;
        frag->scissorScale[1] = 1.0f;
    } else {
        ME_surface_TransformInverse(invxform, scissor->xform);
        ME_surface_gl_xformToMat3x4(frag->scissorMat, invxform);
        frag->scissorExt[0] = scissor->extent[0];
        frag->scissorExt[1] = scissor->extent[1];
        frag->scissorScale[0] = sqrtf(scissor->xform[0] * scissor->xform[0] + scissor->xform[2] * scissor->xform[2]) / fringe;
        frag->scissorScale[1] = sqrtf(scissor->xform[1] * scissor->xform[1] + scissor->xform[3] * scissor->xform[3]) / fringe;
    }

    memcpy(frag->extent, paint->extent, sizeof(frag->extent));
    frag->strokeMult = (width * 0.5f + fringe * 0.5f) / fringe;
    frag->strokeThr = strokeThr;

    if (paint->image != 0) {
        tex = ME_surface_gl_findTexture(gl, paint->image);
        if (tex == NULL) return 0;
        if ((tex->flags & ME_SURFACE_IMAGE_FLIPY) != 0) {
            float m1[6], m2[6];
            ME_surface_TransformTranslate(m1, 0.0f, frag->extent[1] * 0.5f);
            ME_surface_TransformMultiply(m1, paint->xform);
            ME_surface_TransformScale(m2, 1.0f, -1.0f);
            ME_surface_TransformMultiply(m2, m1);
            ME_surface_TransformTranslate(m1, 0.0f, -frag->extent[1] * 0.5f);
            ME_surface_TransformMultiply(m1, m2);
            ME_surface_TransformInverse(invxform, m1);
        } else {
            ME_surface_TransformInverse(invxform, paint->xform);
        }
        frag->type = ME_SURFACE_SVG_SHADER_FILLIMG;

        if (tex->type == ME_SURFACE_TEXTURE_RGBA)
            frag->texType = (tex->flags & ME_SURFACE_IMAGE_PREMULTIPLIED) ? 0 : 1;
        else
            frag->texType = 2;
        //      printf("frag->texType = %d\n", frag->texType);
    } else {
        frag->type = ME_SURFACE_SVG_SHADER_FILLGRAD;
        frag->radius = paint->radius;
        frag->feather = paint->feather;
        ME_surface_TransformInverse(invxform, paint->xform);
    }

    ME_surface_gl_xformToMat3x4(frag->paintMat, invxform);

    return 1;
}

static ME_SURFACE_GLfragUniforms* ME_surface___fragUniformPtr(ME_SURFACE_GLcontext* gl, int i);

static void ME_surface_gl_setUniforms(ME_SURFACE_GLcontext* gl, int uniformOffset, int image) {
    ME_SURFACE_GLtexture* tex = NULL;
    glBindBufferRange(GL_UNIFORM_BUFFER, ME_SURFACE_GL_FRAG_BINDING, gl->fragBuf, uniformOffset, sizeof(ME_SURFACE_GLfragUniforms));

    if (image != 0) {
        tex = ME_surface_gl_findTexture(gl, image);
    }
    // If no image is set, use empty texture
    if (tex == NULL) {
        tex = ME_surface_gl_findTexture(gl, gl->dummyTex);
    }
    ME_surface_gl_bindTexture(gl, tex != NULL ? tex->tex : 0);
    ME_surface_gl_checkError(gl, "tex paint tex");
}

static void ME_surface_gl_renderViewport(void* uptr, float width, float height, float devicePixelRatio) {
    ME_SURFACE_GLcontext* gl = (ME_SURFACE_GLcontext*)uptr;
    gl->view[0] = width;
    gl->view[1] = height;
}

static void ME_surface_gl_fill(ME_SURFACE_GLcontext* gl, ME_SURFACE_GLcall* call) {
    ME_SURFACE_GLpath* paths = &gl->paths[call->pathOffset];
    int i, npaths = call->pathCount;

    // Draw shapes
    glEnable(GL_STENCIL_TEST);
    ME_surface_gl_stencilMask(gl, 0xff);
    ME_surface_gl_stencilFunc(gl, GL_ALWAYS, 0, 0xff);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // set bindpoint for solid loc
    ME_surface_gl_setUniforms(gl, call->uniformOffset, 0);
    ME_surface_gl_checkError(gl, "fill simple");

    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
    glDisable(GL_CULL_FACE);
    for (i = 0; i < npaths; i++) glDrawArrays(GL_TRIANGLE_FAN, paths[i].fillOffset, paths[i].fillCount);
    glEnable(GL_CULL_FACE);

    // Draw anti-aliased pixels
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    ME_surface_gl_setUniforms(gl, call->uniformOffset + gl->fragSize, call->image);
    ME_surface_gl_checkError(gl, "fill fill");

    if (gl->flags & ME_SURFACE_ANTIALIAS) {
        ME_surface_gl_stencilFunc(gl, GL_EQUAL, 0x00, 0xff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        // Draw fringes
        for (i = 0; i < npaths; i++) glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
    }

    // Draw fill
    ME_surface_gl_stencilFunc(gl, GL_NOTEQUAL, 0x0, 0xff);
    glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
    glDrawArrays(GL_TRIANGLE_STRIP, call->triangleOffset, call->triangleCount);

    glDisable(GL_STENCIL_TEST);
}

static void ME_surface_gl_convexFill(ME_SURFACE_GLcontext* gl, ME_SURFACE_GLcall* call) {
    ME_SURFACE_GLpath* paths = &gl->paths[call->pathOffset];
    int i, npaths = call->pathCount;

    ME_surface_gl_setUniforms(gl, call->uniformOffset, call->image);
    ME_surface_gl_checkError(gl, "convex fill");

    for (i = 0; i < npaths; i++) {
        glDrawArrays(GL_TRIANGLE_FAN, paths[i].fillOffset, paths[i].fillCount);
        // Draw fringes
        if (paths[i].strokeCount > 0) {
            glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
        }
    }
}

static void ME_surface_gl_stroke(ME_SURFACE_GLcontext* gl, ME_SURFACE_GLcall* call) {
    ME_SURFACE_GLpath* paths = &gl->paths[call->pathOffset];
    int npaths = call->pathCount, i;

    if (gl->flags & ME_SURFACE_STENCIL_STROKES) {

        glEnable(GL_STENCIL_TEST);
        ME_surface_gl_stencilMask(gl, 0xff);

        // Fill the stroke base without overlap
        ME_surface_gl_stencilFunc(gl, GL_EQUAL, 0x0, 0xff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
        ME_surface_gl_setUniforms(gl, call->uniformOffset + gl->fragSize, call->image);
        ME_surface_gl_checkError(gl, "stroke fill 0");
        for (i = 0; i < npaths; i++) glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);

        // Draw anti-aliased pixels.
        ME_surface_gl_setUniforms(gl, call->uniformOffset, call->image);
        ME_surface_gl_stencilFunc(gl, GL_EQUAL, 0x00, 0xff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        for (i = 0; i < npaths; i++) glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);

        // Clear stencil buffer.
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        ME_surface_gl_stencilFunc(gl, GL_ALWAYS, 0x0, 0xff);
        glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
        ME_surface_gl_checkError(gl, "stroke fill 1");
        for (i = 0; i < npaths; i++) glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        glDisable(GL_STENCIL_TEST);

        //      ME_surface_gl_convertPaint(gl, ME_surface___fragUniformPtr(gl, call->uniformOffset + gl->fragSize), paint, scissor, strokeWidth, fringe, 1.0f - 0.5f/255.0f);

    } else {
        ME_surface_gl_setUniforms(gl, call->uniformOffset, call->image);
        ME_surface_gl_checkError(gl, "stroke fill");
        // Draw Strokes
        for (i = 0; i < npaths; i++) glDrawArrays(GL_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
    }
}

static void ME_surface_gl_triangles(ME_SURFACE_GLcontext* gl, ME_SURFACE_GLcall* call) {
    ME_surface_gl_setUniforms(gl, call->uniformOffset, call->image);
    ME_surface_gl_checkError(gl, "triangles fill");

    glDrawArrays(GL_TRIANGLES, call->triangleOffset, call->triangleCount);
}

static void ME_surface_gl_renderCancel(void* uptr) {
    ME_SURFACE_GLcontext* gl = (ME_SURFACE_GLcontext*)uptr;
    gl->nverts = 0;
    gl->npaths = 0;
    gl->ncalls = 0;
    gl->nuniforms = 0;
}

static GLenum ME_surface_gl___convertBlendFuncFactor(int factor) {
    if (factor == ME_SURFACE_ZERO) return GL_ZERO;
    if (factor == ME_SURFACE_ONE) return GL_ONE;
    if (factor == ME_SURFACE_SRC_COLOR) return GL_SRC_COLOR;
    if (factor == ME_SURFACE_ONE_MINUS_SRC_COLOR) return GL_ONE_MINUS_SRC_COLOR;
    if (factor == ME_SURFACE_DST_COLOR) return GL_DST_COLOR;
    if (factor == ME_SURFACE_ONE_MINUS_DST_COLOR) return GL_ONE_MINUS_DST_COLOR;
    if (factor == ME_SURFACE_SRC_ALPHA) return GL_SRC_ALPHA;
    if (factor == ME_SURFACE_ONE_MINUS_SRC_ALPHA) return GL_ONE_MINUS_SRC_ALPHA;
    if (factor == ME_SURFACE_DST_ALPHA) return GL_DST_ALPHA;
    if (factor == ME_SURFACE_ONE_MINUS_DST_ALPHA) return GL_ONE_MINUS_DST_ALPHA;
    if (factor == ME_SURFACE_SRC_ALPHA_SATURATE) return GL_SRC_ALPHA_SATURATE;
    return GL_INVALID_ENUM;
}

static ME_SURFACE_GLblend ME_surface_gl_blendCompositeOperation(MEsurface_compositeOperationState op) {
    ME_SURFACE_GLblend blend;
    blend.srcRGB = ME_surface_gl___convertBlendFuncFactor(op.srcRGB);
    blend.dstRGB = ME_surface_gl___convertBlendFuncFactor(op.dstRGB);
    blend.srcAlpha = ME_surface_gl___convertBlendFuncFactor(op.srcAlpha);
    blend.dstAlpha = ME_surface_gl___convertBlendFuncFactor(op.dstAlpha);
    if (blend.srcRGB == GL_INVALID_ENUM || blend.dstRGB == GL_INVALID_ENUM || blend.srcAlpha == GL_INVALID_ENUM || blend.dstAlpha == GL_INVALID_ENUM) {
        blend.srcRGB = GL_ONE;
        blend.dstRGB = GL_ONE_MINUS_SRC_ALPHA;
        blend.srcAlpha = GL_ONE;
        blend.dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
    }
    return blend;
}

static void ME_surface_gl_renderFlush(void* uptr) {
    ME_SURFACE_GLcontext* gl = (ME_SURFACE_GLcontext*)uptr;
    int i;

    if (gl->ncalls > 0) {

        // Setup require GL state.
        glUseProgram(gl->shader.prog);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glStencilMask(0xffffffff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        gl->boundTexture = 0;
        gl->stencilMask = 0xffffffff;
        gl->stencilFunc = GL_ALWAYS;
        gl->stencilFuncRef = 0;
        gl->stencilFuncMask = 0xffffffff;
        gl->blendFunc.srcRGB = GL_INVALID_ENUM;
        gl->blendFunc.srcAlpha = GL_INVALID_ENUM;
        gl->blendFunc.dstRGB = GL_INVALID_ENUM;
        gl->blendFunc.dstAlpha = GL_INVALID_ENUM;

        // Upload ubo for frag shaders
        glBindBuffer(GL_UNIFORM_BUFFER, gl->fragBuf);
        glBufferData(GL_UNIFORM_BUFFER, gl->nuniforms * gl->fragSize, gl->uniforms, GL_STREAM_DRAW);

        // Upload vertex data
        glBindVertexArray(gl->vertArr);
        glBindBuffer(GL_ARRAY_BUFFER, gl->vertBuf);
        glBufferData(GL_ARRAY_BUFFER, gl->nverts * sizeof(MEsurface_vertex), gl->verts, GL_STREAM_DRAW);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(MEsurface_vertex), (const GLvoid*)(size_t)0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(MEsurface_vertex), (const GLvoid*)(0 + 2 * sizeof(float)));

        // Set view and texture just once per frame.
        glUniform1i(gl->shader.loc[ME_SURFACE_GL_LOC_TEX], 0);
        glUniform2fv(gl->shader.loc[ME_SURFACE_GL_LOC_VIEWSIZE], 1, gl->view);

        glBindBuffer(GL_UNIFORM_BUFFER, gl->fragBuf);

        for (i = 0; i < gl->ncalls; i++) {
            ME_SURFACE_GLcall* call = &gl->calls[i];
            ME_surface_gl_blendFuncSeparate(gl, &call->blendFunc);
            if (call->type == ME_SURFACE_GL_FILL)
                ME_surface_gl_fill(gl, call);
            else if (call->type == ME_SURFACE_GL_CONVEXFILL)
                ME_surface_gl_convexFill(gl, call);
            else if (call->type == ME_SURFACE_GL_STROKE)
                ME_surface_gl_stroke(gl, call);
            else if (call->type == ME_SURFACE_GL_TRIANGLES)
                ME_surface_gl_triangles(gl, call);
        }

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        glBindVertexArray(0);

        glDisable(GL_CULL_FACE);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(0);
        ME_surface_gl_bindTexture(gl, 0);
    }

    // Reset calls
    gl->nverts = 0;
    gl->npaths = 0;
    gl->ncalls = 0;
    gl->nuniforms = 0;
}

static int ME_surface_gl_maxVertCount(const MEsurface_path* paths, int npaths) {
    int i, count = 0;
    for (i = 0; i < npaths; i++) {
        count += paths[i].nfill;
        count += paths[i].nstroke;
    }
    return count;
}

static ME_SURFACE_GLcall* ME_surface_gl_allocCall(ME_SURFACE_GLcontext* gl) {
    ME_SURFACE_GLcall* ret = NULL;
    if (gl->ncalls + 1 > gl->ccalls) {
        ME_SURFACE_GLcall* calls;
        int ccalls = ME_surface_gl_maxi(gl->ncalls + 1, 128) + gl->ccalls / 2;  // 1.5x Overallocate
        calls = (ME_SURFACE_GLcall*)realloc(gl->calls, sizeof(ME_SURFACE_GLcall) * ccalls);
        if (calls == NULL) return NULL;
        gl->calls = calls;
        gl->ccalls = ccalls;
    }
    ret = &gl->calls[gl->ncalls++];
    memset(ret, 0, sizeof(ME_SURFACE_GLcall));
    return ret;
}

static int ME_surface_gl_allocPaths(ME_SURFACE_GLcontext* gl, int n) {
    int ret = 0;
    if (gl->npaths + n > gl->cpaths) {
        ME_SURFACE_GLpath* paths;
        int cpaths = ME_surface_gl_maxi(gl->npaths + n, 128) + gl->cpaths / 2;  // 1.5x Overallocate
        paths = (ME_SURFACE_GLpath*)realloc(gl->paths, sizeof(ME_SURFACE_GLpath) * cpaths);
        if (paths == NULL) return -1;
        gl->paths = paths;
        gl->cpaths = cpaths;
    }
    ret = gl->npaths;
    gl->npaths += n;
    return ret;
}

static int ME_surface_gl_allocVerts(ME_SURFACE_GLcontext* gl, int n) {
    int ret = 0;
    if (gl->nverts + n > gl->cverts) {
        MEsurface_vertex* verts;
        int cverts = ME_surface_gl_maxi(gl->nverts + n, 4096) + gl->cverts / 2;  // 1.5x Overallocate
        verts = (MEsurface_vertex*)realloc(gl->verts, sizeof(MEsurface_vertex) * cverts);
        if (verts == NULL) return -1;
        gl->verts = verts;
        gl->cverts = cverts;
    }
    ret = gl->nverts;
    gl->nverts += n;
    return ret;
}

static int ME_surface_gl_allocFragUniforms(ME_SURFACE_GLcontext* gl, int n) {
    int ret = 0, structSize = gl->fragSize;
    if (gl->nuniforms + n > gl->cuniforms) {
        unsigned char* uniforms;
        int cuniforms = ME_surface_gl_maxi(gl->nuniforms + n, 128) + gl->cuniforms / 2;  // 1.5x Overallocate
        uniforms = (unsigned char*)realloc(gl->uniforms, structSize * cuniforms);
        if (uniforms == NULL) return -1;
        gl->uniforms = uniforms;
        gl->cuniforms = cuniforms;
    }
    ret = gl->nuniforms * structSize;
    gl->nuniforms += n;
    return ret;
}

static ME_SURFACE_GLfragUniforms* ME_surface___fragUniformPtr(ME_SURFACE_GLcontext* gl, int i) { return (ME_SURFACE_GLfragUniforms*)&gl->uniforms[i]; }

static void ME_surface_gl_vset(MEsurface_vertex* vtx, float x, float y, float u, float v) {
    vtx->x = x;
    vtx->y = y;
    vtx->u = u;
    vtx->v = v;
}

static void ME_surface_gl_renderFill(void* uptr, MEsurface_paint* paint, MEsurface_compositeOperationState compositeOperation, MEsurface_scissor* scissor, float fringe, const float* bounds,
                                     const MEsurface_path* paths, int npaths) {
    ME_SURFACE_GLcontext* gl = (ME_SURFACE_GLcontext*)uptr;
    ME_SURFACE_GLcall* call = ME_surface_gl_allocCall(gl);
    MEsurface_vertex* quad;
    ME_SURFACE_GLfragUniforms* frag;
    int i, maxverts, offset;

    if (call == NULL) return;

    call->type = ME_SURFACE_GL_FILL;
    call->triangleCount = 4;
    call->pathOffset = ME_surface_gl_allocPaths(gl, npaths);
    if (call->pathOffset == -1) goto error;
    call->pathCount = npaths;
    call->image = paint->image;
    call->blendFunc = ME_surface_gl_blendCompositeOperation(compositeOperation);

    if (npaths == 1 && paths[0].convex) {
        call->type = ME_SURFACE_GL_CONVEXFILL;
        call->triangleCount = 0;  // Bounding box fill quad not needed for convex fill
    }

    // Allocate vertices for all the paths.
    maxverts = ME_surface_gl_maxVertCount(paths, npaths) + call->triangleCount;
    offset = ME_surface_gl_allocVerts(gl, maxverts);
    if (offset == -1) goto error;

    for (i = 0; i < npaths; i++) {
        ME_SURFACE_GLpath* copy = &gl->paths[call->pathOffset + i];
        const MEsurface_path* path = &paths[i];
        memset(copy, 0, sizeof(ME_SURFACE_GLpath));
        if (path->nfill > 0) {
            copy->fillOffset = offset;
            copy->fillCount = path->nfill;
            memcpy(&gl->verts[offset], path->fill, sizeof(MEsurface_vertex) * path->nfill);
            offset += path->nfill;
        }
        if (path->nstroke > 0) {
            copy->strokeOffset = offset;
            copy->strokeCount = path->nstroke;
            memcpy(&gl->verts[offset], path->stroke, sizeof(MEsurface_vertex) * path->nstroke);
            offset += path->nstroke;
        }
    }

    // Setup uniforms for draw calls
    if (call->type == ME_SURFACE_GL_FILL) {
        // Quad
        call->triangleOffset = offset;
        quad = &gl->verts[call->triangleOffset];
        ME_surface_gl_vset(&quad[0], bounds[2], bounds[3], 0.5f, 1.0f);
        ME_surface_gl_vset(&quad[1], bounds[2], bounds[1], 0.5f, 1.0f);
        ME_surface_gl_vset(&quad[2], bounds[0], bounds[3], 0.5f, 1.0f);
        ME_surface_gl_vset(&quad[3], bounds[0], bounds[1], 0.5f, 1.0f);

        call->uniformOffset = ME_surface_gl_allocFragUniforms(gl, 2);
        if (call->uniformOffset == -1) goto error;
        // Simple shader for stencil
        frag = ME_surface___fragUniformPtr(gl, call->uniformOffset);
        memset(frag, 0, sizeof(*frag));
        frag->strokeThr = -1.0f;
        frag->type = ME_SURFACE_SVG_SHADER_SIMPLE;
        // Fill shader
        ME_surface_gl_convertPaint(gl, ME_surface___fragUniformPtr(gl, call->uniformOffset + gl->fragSize), paint, scissor, fringe, fringe, -1.0f);
    } else {
        call->uniformOffset = ME_surface_gl_allocFragUniforms(gl, 1);
        if (call->uniformOffset == -1) goto error;
        // Fill shader
        ME_surface_gl_convertPaint(gl, ME_surface___fragUniformPtr(gl, call->uniformOffset), paint, scissor, fringe, fringe, -1.0f);
    }

    return;

error:
    // We get here if call alloc was ok, but something else is not.
    // Roll back the last call to prevent drawing it.
    if (gl->ncalls > 0) gl->ncalls--;
}

static void ME_surface_gl_renderStroke(void* uptr, MEsurface_paint* paint, MEsurface_compositeOperationState compositeOperation, MEsurface_scissor* scissor, float fringe, float strokeWidth,
                                       const MEsurface_path* paths, int npaths) {
    ME_SURFACE_GLcontext* gl = (ME_SURFACE_GLcontext*)uptr;
    ME_SURFACE_GLcall* call = ME_surface_gl_allocCall(gl);
    int i, maxverts, offset;

    if (call == NULL) return;

    call->type = ME_SURFACE_GL_STROKE;
    call->pathOffset = ME_surface_gl_allocPaths(gl, npaths);
    if (call->pathOffset == -1) goto error;
    call->pathCount = npaths;
    call->image = paint->image;
    call->blendFunc = ME_surface_gl_blendCompositeOperation(compositeOperation);

    // Allocate vertices for all the paths.
    maxverts = ME_surface_gl_maxVertCount(paths, npaths);
    offset = ME_surface_gl_allocVerts(gl, maxverts);
    if (offset == -1) goto error;

    for (i = 0; i < npaths; i++) {
        ME_SURFACE_GLpath* copy = &gl->paths[call->pathOffset + i];
        const MEsurface_path* path = &paths[i];
        memset(copy, 0, sizeof(ME_SURFACE_GLpath));
        if (path->nstroke) {
            copy->strokeOffset = offset;
            copy->strokeCount = path->nstroke;
            memcpy(&gl->verts[offset], path->stroke, sizeof(MEsurface_vertex) * path->nstroke);
            offset += path->nstroke;
        }
    }

    if (gl->flags & ME_SURFACE_STENCIL_STROKES) {
        // Fill shader
        call->uniformOffset = ME_surface_gl_allocFragUniforms(gl, 2);
        if (call->uniformOffset == -1) goto error;

        ME_surface_gl_convertPaint(gl, ME_surface___fragUniformPtr(gl, call->uniformOffset), paint, scissor, strokeWidth, fringe, -1.0f);
        ME_surface_gl_convertPaint(gl, ME_surface___fragUniformPtr(gl, call->uniformOffset + gl->fragSize), paint, scissor, strokeWidth, fringe, 1.0f - 0.5f / 255.0f);

    } else {
        // Fill shader
        call->uniformOffset = ME_surface_gl_allocFragUniforms(gl, 1);
        if (call->uniformOffset == -1) goto error;
        ME_surface_gl_convertPaint(gl, ME_surface___fragUniformPtr(gl, call->uniformOffset), paint, scissor, strokeWidth, fringe, -1.0f);
    }

    return;

error:
    // We get here if call alloc was ok, but something else is not.
    // Roll back the last call to prevent drawing it.
    if (gl->ncalls > 0) gl->ncalls--;
}

static void ME_surface_gl_renderTriangles(void* uptr, MEsurface_paint* paint, MEsurface_compositeOperationState compositeOperation, MEsurface_scissor* scissor, const MEsurface_vertex* verts,
                                          int nverts, float fringe) {
    ME_SURFACE_GLcontext* gl = (ME_SURFACE_GLcontext*)uptr;
    ME_SURFACE_GLcall* call = ME_surface_gl_allocCall(gl);
    ME_SURFACE_GLfragUniforms* frag;

    if (call == NULL) return;

    call->type = ME_SURFACE_GL_TRIANGLES;
    call->image = paint->image;
    call->blendFunc = ME_surface_gl_blendCompositeOperation(compositeOperation);

    // Allocate vertices for all the paths.
    call->triangleOffset = ME_surface_gl_allocVerts(gl, nverts);
    if (call->triangleOffset == -1) goto error;
    call->triangleCount = nverts;

    memcpy(&gl->verts[call->triangleOffset], verts, sizeof(MEsurface_vertex) * nverts);

    // Fill shader
    call->uniformOffset = ME_surface_gl_allocFragUniforms(gl, 1);
    if (call->uniformOffset == -1) goto error;
    frag = ME_surface___fragUniformPtr(gl, call->uniformOffset);
    ME_surface_gl_convertPaint(gl, frag, paint, scissor, 1.0f, fringe, -1.0f);
    frag->type = ME_SURFACE_SVG_SHADER_IMG;

    return;

error:
    // We get here if call alloc was ok, but something else is not.
    // Roll back the last call to prevent drawing it.
    if (gl->ncalls > 0) gl->ncalls--;
}

static void ME_surface_gl_renderDelete(void* uptr) {
    ME_SURFACE_GLcontext* gl = (ME_SURFACE_GLcontext*)uptr;
    int i;
    if (gl == NULL) return;

    ME_surface_gl_deleteShader(&gl->shader);

    if (gl->fragBuf != 0) glDeleteBuffers(1, &gl->fragBuf);

    if (gl->vertArr != 0) glDeleteVertexArrays(1, &gl->vertArr);

    if (gl->vertBuf != 0) glDeleteBuffers(1, &gl->vertBuf);

    for (i = 0; i < gl->ntextures; i++) {
        if (gl->textures[i].tex != 0 && (gl->textures[i].flags & ME_SURFACE_IMAGE_NODELETE) == 0) glDeleteTextures(1, &gl->textures[i].tex);
    }
    free(gl->textures);

    free(gl->paths);
    free(gl->verts);
    free(gl->uniforms);
    free(gl->calls);

    free(gl);
}

MEsurface_context* g_ctx = NULL;

MEsurface_context* ME_surface_CreateGL3(int flags) {
    MEsurface_funcs params;
    MEsurface_context* ctx = NULL;
    ME_SURFACE_GLcontext* gl = (ME_SURFACE_GLcontext*)malloc(sizeof(ME_SURFACE_GLcontext));
    if (gl == NULL) goto error;
    memset(gl, 0, sizeof(ME_SURFACE_GLcontext));

    memset(&params, 0, sizeof(params));
    params.renderCreate = ME_surface_gl_renderCreate;
    params.renderCreateTexture = ME_surface_gl_renderCreateTexture;
    params.renderDeleteTexture = ME_surface_gl_renderDeleteTexture;
    params.renderUpdateTexture = ME_surface_gl_renderUpdateTexture;
    params.renderGetTextureSize = ME_surface_gl_renderGetTextureSize;
    params.renderViewport = ME_surface_gl_renderViewport;
    params.renderCancel = ME_surface_gl_renderCancel;
    params.renderFlush = ME_surface_gl_renderFlush;
    params.renderFill = ME_surface_gl_renderFill;
    params.renderStroke = ME_surface_gl_renderStroke;
    params.renderTriangles = ME_surface_gl_renderTriangles;
    params.renderDelete = ME_surface_gl_renderDelete;
    params.userPtr = gl;
    params.edgeAntiAlias = flags & ME_SURFACE_ANTIALIAS ? 1 : 0;

    gl->flags = flags;

    ctx = ME_surface_CreateInternal(&params);
    if (ctx == NULL) goto error;

    g_ctx = ctx;

    return ctx;

error:
    // 'gl' is freed by ME_surface_DeleteInternal.
    if (ctx != NULL) ME_surface_DeleteInternal(ctx);
    return NULL;
}

void ME_surface_DeleteGL3(MEsurface_context* ctx) { ME_surface_DeleteInternal(ctx); }

int ME_surface_CreateImageFromHandleGL3(MEsurface_context* ctx, GLuint textureId, int w, int h, int imageFlags) {
    ME_SURFACE_GLcontext* gl = (ME_SURFACE_GLcontext*)ME_surface_InternalParams(ctx)->userPtr;
    ME_SURFACE_GLtexture* tex = ME_surface_gl_allocTexture(gl);

    if (tex == NULL) return 0;

    tex->type = ME_SURFACE_TEXTURE_RGBA;
    tex->tex = textureId;
    tex->flags = imageFlags;
    tex->width = w;
    tex->height = h;

    return tex->id;
}

GLuint ME_surface_ImageHandleGL3(MEsurface_context* ctx, int image) {
    ME_SURFACE_GLcontext* gl = (ME_SURFACE_GLcontext*)ME_surface_InternalParams(ctx)->userPtr;
    ME_SURFACE_GLtexture* tex = ME_surface_gl_findTexture(gl, image);
    return tex->tex;
}

static GLint defaultFBO = -1;

MEsurface_GLframebuffer* ME_surface_gl_CreateFramebuffer(MEsurface_context* ctx, int w, int h, int imageFlags) {
    GLint defaultFBO;
    GLint defaultRBO;
    MEsurface_GLframebuffer* fb = NULL;

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFBO);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &defaultRBO);

    fb = (MEsurface_GLframebuffer*)malloc(sizeof(MEsurface_GLframebuffer));
    if (fb == NULL) goto error;
    memset(fb, 0, sizeof(MEsurface_GLframebuffer));

    fb->image = ME_surface_CreateImageRGBA(ctx, w, h, imageFlags | ME_SURFACE_IMAGE_FLIPY | ME_SURFACE_IMAGE_PREMULTIPLIED, NULL);

    fb->texture = ME_surface_ImageHandleGL3(ctx, fb->image);

    fb->ctx = ctx;

    // frame buffer object
    glGenFramebuffers(1, &fb->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);

    // render buffer object
    glGenRenderbuffers(1, &fb->rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, fb->rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, w, h);

    // combine all
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb->rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
#ifdef GL_DEPTH24_STENCIL8
        // If GL_STENCIL_INDEX8 is not supported, try GL_DEPTH24_STENCIL8 as a fallback.
        // Some graphics cards require a depth buffer along with a stencil.
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->texture, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb->rbo);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
#endif  // GL_DEPTH24_STENCIL8
            goto error;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, defaultRBO);
    return fb;
error:
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, defaultRBO);
    ME_surface_gl_DeleteFramebuffer(fb);
    return NULL;
}

void ME_surface_gl_BindFramebuffer(MEsurface_GLframebuffer* fb) {
    if (defaultFBO == -1) glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, fb != NULL ? fb->fbo : defaultFBO);
}

void ME_surface_gl_DeleteFramebuffer(MEsurface_GLframebuffer* fb) {
    if (fb == NULL) return;
    if (fb->fbo != 0) glDeleteFramebuffers(1, &fb->fbo);
    if (fb->rbo != 0) glDeleteRenderbuffers(1, &fb->rbo);
    if (fb->image >= 0) ME_surface_DeleteImage(fb->ctx, fb->image);
    fb->ctx = NULL;
    fb->fbo = 0;
    fb->rbo = 0;
    fb->texture = 0;
    fb->image = -1;
    free(fb);
}