
#ifndef ME_SURFACE_GL_H
#define ME_SURFACE_GL_H

#include "engine/renderer/gpu.hpp"
#include "surface.h"

// Create flags

enum MEsurface_createFlags {
    // Flag indicating if geometry based anti-aliasing is used (may not be needed when using MSAA).
    ME_SURFACE_ANTIALIAS = 1 << 0,
    // Flag indicating if strokes should be drawn using stencil buffer. The rendering will be a little
    // slower, but path overlaps (i.e. self-intersecting or sharp turns) will be drawn just once.
    ME_SURFACE_STENCIL_STROKES = 1 << 1,
    // Flag indicating that additional debug checks are done.
    ME_SURFACE_DEBUG = 1 << 2,
};

MEsurface_context* ME_surface_CreateGL3(int flags);
void ME_surface_DeleteGL3(MEsurface_context* ctx);

int ME_surface_CreateImageFromHandleGL3(MEsurface_context* ctx, GLuint textureId, int w, int h, int flags);
GLuint ME_surface_ImageHandleGL3(MEsurface_context* ctx, int image);

// These are additional flags on top of MEsurface_imageFlags.
enum MEsurface_imageFlagsGL {
    ME_SURFACE_IMAGE_NODELETE = 1 << 16,  // Do not delete GL texture handle.
};

struct MEsurface_GLframebuffer {
    MEsurface_context* ctx;
    GLuint fbo;
    GLuint rbo;
    GLuint texture;
    int image;
};
typedef struct MEsurface_GLframebuffer MEsurface_GLframebuffer;

// Helper function to create GL frame buffer to render to.
void ME_surface_gl_BindFramebuffer(MEsurface_GLframebuffer* fb);
MEsurface_GLframebuffer* ME_surface_gl_CreateFramebuffer(MEsurface_context* ctx, int w, int h, int imageFlags);
void ME_surface_gl_DeleteFramebuffer(MEsurface_GLframebuffer* fb);

#endif
