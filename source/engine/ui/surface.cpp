

#include "surface.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "libs/external/stb_image.h"

#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"

#define ME_SURFACE_INIT_FONTIMAGE_SIZE 512
#define ME_SURFACE_MAX_FONTIMAGE_SIZE 2048
#define ME_SURFACE_MAX_FONTIMAGES 4

#define ME_SURFACE_INIT_COMMANDS_SIZE 256
#define ME_SURFACE_INIT_POINTS_SIZE 128
#define ME_SURFACE_INIT_PATHS_SIZE 16
#define ME_SURFACE_INIT_VERTS_SIZE 256

#ifndef ME_SURFACE_MAX_STATES
#define ME_SURFACE_MAX_STATES 32
#endif

#define ME_SURFACE_KAPPA90 0.5522847493f  // Length proportional to radius of a cubic bezier handle for 90deg arcs.

#define ME_SURFACE_COUNTOF(arr) (sizeof(arr) / sizeof(0 [arr]))

enum MEsurface_commands {
    ME_SURFACE_MOVETO = 0,
    ME_SURFACE_LINETO = 1,
    ME_SURFACE_BEZIERTO = 2,
    ME_SURFACE_CLOSE = 3,
    ME_SURFACE_WINDING = 4,
};

enum MEsurface_pointFlags {
    ME_SURFACE_PT_CORNER = 0x01,
    ME_SURFACE_PT_LEFT = 0x02,
    ME_SURFACE_PT_BEVEL = 0x04,
    ME_SURFACE_PR_INNERBEVEL = 0x08,
};

struct MEsurface_state {
    MEsurface_compositeOperationState compositeOperation;
    int shapeAntiAlias;
    MEsurface_paint fill;
    MEsurface_paint stroke;
    float strokeWidth;
    float miterLimit;
    int lineJoin;
    int lineCap;
    float alpha;
    float xform[6];
    MEsurface_scissor scissor;
    float fontSize;
    float letterSpacing;
    float lineHeight;
    float fontBlur;
    int textAlign;
    int fontId;
};
typedef struct MEsurface_state MEsurface_state;

struct MEsurface_point {
    float x, y;
    float dx, dy;
    float len;
    float dmx, dmy;
    unsigned char flags;
};
typedef struct MEsurface_point MEsurface_point;

struct MEsurface_pathCache {
    MEsurface_point* points;
    int npoints;
    int cpoints;
    MEsurface_path* paths;
    int npaths;
    int cpaths;
    MEsurface_vertex* verts;
    int nverts;
    int cverts;
    float bounds[4];
};
typedef struct MEsurface_pathCache MEsurface_pathCache;

struct MEsurface_context {
    MEsurface_funcs params;
    float* commands;
    int ccommands;
    int ncommands;
    float commandx, commandy;
    MEsurface_state states[ME_SURFACE_MAX_STATES];
    int nstates;
    MEsurface_pathCache* cache;
    float tessTol;
    float distTol;
    float fringeWidth;
    float devicePxRatio;
    struct FONScontext* fs;
    int fontImages[ME_SURFACE_MAX_FONTIMAGES];
    int fontImageIdx;
    int drawCallCount;
    int fillTriCount;
    int strokeTriCount;
    int textTriCount;
};

static float ME_surface___sqrtf(float a) { return sqrtf(a); }
static float ME_surface___modf(float a, float b) { return fmodf(a, b); }
static float ME_surface___sinf(float a) { return sinf(a); }
static float ME_surface___cosf(float a) { return cosf(a); }
static float ME_surface___tanf(float a) { return tanf(a); }
static float ME_surface___atan2f(float a, float b) { return atan2f(a, b); }
static float ME_surface___acosf(float a) { return acosf(a); }

static int ME_surface___mini(int a, int b) { return a < b ? a : b; }
static int ME_surface___maxi(int a, int b) { return a > b ? a : b; }
static int ME_surface___clampi(int a, int mn, int mx) { return a < mn ? mn : (a > mx ? mx : a); }
static float ME_surface___minf(float a, float b) { return a < b ? a : b; }
static float ME_surface___maxf(float a, float b) { return a > b ? a : b; }
static float ME_surface___absf(float a) { return a >= 0.0f ? a : -a; }
static float ME_surface___signf(float a) { return a >= 0.0f ? 1.0f : -1.0f; }
static float ME_surface___clampf(float a, float mn, float mx) { return a < mn ? mn : (a > mx ? mx : a); }
static float ME_surface___cross(float dx0, float dy0, float dx1, float dy1) { return dx1 * dy0 - dx0 * dy1; }

static float ME_surface___normalize(float* x, float* y) {
    float d = ME_surface___sqrtf((*x) * (*x) + (*y) * (*y));
    if (d > 1e-6f) {
        float id = 1.0f / d;
        *x *= id;
        *y *= id;
    }
    return d;
}

static void ME_surface___deletePathCache(MEsurface_pathCache* c) {
    if (c == NULL) return;
    if (c->points != NULL) free(c->points);
    if (c->paths != NULL) free(c->paths);
    if (c->verts != NULL) free(c->verts);
    free(c);
}

static MEsurface_pathCache* ME_surface___allocPathCache(void) {
    MEsurface_pathCache* c = (MEsurface_pathCache*)malloc(sizeof(MEsurface_pathCache));
    if (c == NULL) goto error;
    memset(c, 0, sizeof(MEsurface_pathCache));

    c->points = (MEsurface_point*)malloc(sizeof(MEsurface_point) * ME_SURFACE_INIT_POINTS_SIZE);
    if (!c->points) goto error;
    c->npoints = 0;
    c->cpoints = ME_SURFACE_INIT_POINTS_SIZE;

    c->paths = (MEsurface_path*)malloc(sizeof(MEsurface_path) * ME_SURFACE_INIT_PATHS_SIZE);
    if (!c->paths) goto error;
    c->npaths = 0;
    c->cpaths = ME_SURFACE_INIT_PATHS_SIZE;

    c->verts = (MEsurface_vertex*)malloc(sizeof(MEsurface_vertex) * ME_SURFACE_INIT_VERTS_SIZE);
    if (!c->verts) goto error;
    c->nverts = 0;
    c->cverts = ME_SURFACE_INIT_VERTS_SIZE;

    return c;
error:
    ME_surface___deletePathCache(c);
    return NULL;
}

static void ME_surface___setDevicePixelRatio(MEsurface_context* ctx, float ratio) {
    ctx->tessTol = 0.25f / ratio;
    ctx->distTol = 0.01f / ratio;
    ctx->fringeWidth = 1.0f / ratio;
    ctx->devicePxRatio = ratio;
}

static MEsurface_compositeOperationState ME_surface___compositeOperationState(int op) {
    int sfactor, dfactor;

    if (op == ME_SURFACE_SOURCE_OVER) {
        sfactor = ME_SURFACE_ONE;
        dfactor = ME_SURFACE_ONE_MINUS_SRC_ALPHA;
    } else if (op == ME_SURFACE_SOURCE_IN) {
        sfactor = ME_SURFACE_DST_ALPHA;
        dfactor = ME_SURFACE_ZERO;
    } else if (op == ME_SURFACE_SOURCE_OUT) {
        sfactor = ME_SURFACE_ONE_MINUS_DST_ALPHA;
        dfactor = ME_SURFACE_ZERO;
    } else if (op == ME_SURFACE_ATOP) {
        sfactor = ME_SURFACE_DST_ALPHA;
        dfactor = ME_SURFACE_ONE_MINUS_SRC_ALPHA;
    } else if (op == ME_SURFACE_DESTINATION_OVER) {
        sfactor = ME_SURFACE_ONE_MINUS_DST_ALPHA;
        dfactor = ME_SURFACE_ONE;
    } else if (op == ME_SURFACE_DESTINATION_IN) {
        sfactor = ME_SURFACE_ZERO;
        dfactor = ME_SURFACE_SRC_ALPHA;
    } else if (op == ME_SURFACE_DESTINATION_OUT) {
        sfactor = ME_SURFACE_ZERO;
        dfactor = ME_SURFACE_ONE_MINUS_SRC_ALPHA;
    } else if (op == ME_SURFACE_DESTINATION_ATOP) {
        sfactor = ME_SURFACE_ONE_MINUS_DST_ALPHA;
        dfactor = ME_SURFACE_SRC_ALPHA;
    } else if (op == ME_SURFACE_LIGHTER) {
        sfactor = ME_SURFACE_ONE;
        dfactor = ME_SURFACE_ONE;
    } else if (op == ME_SURFACE_COPY) {
        sfactor = ME_SURFACE_ONE;
        dfactor = ME_SURFACE_ZERO;
    } else if (op == ME_SURFACE_XOR) {
        sfactor = ME_SURFACE_ONE_MINUS_DST_ALPHA;
        dfactor = ME_SURFACE_ONE_MINUS_SRC_ALPHA;
    } else {
        sfactor = ME_SURFACE_ONE;
        dfactor = ME_SURFACE_ZERO;
    }

    MEsurface_compositeOperationState state;
    state.srcRGB = sfactor;
    state.dstRGB = dfactor;
    state.srcAlpha = sfactor;
    state.dstAlpha = dfactor;
    return state;
}

static MEsurface_state* ME_surface___getState(MEsurface_context* ctx) { return &ctx->states[ctx->nstates - 1]; }

MEsurface_context* ME_surface_CreateInternal(MEsurface_funcs* params) {
    FONSparams fontParams;
    MEsurface_context* ctx = (MEsurface_context*)malloc(sizeof(MEsurface_context));
    int i;
    if (ctx == NULL) goto error;
    memset(ctx, 0, sizeof(MEsurface_context));

    ctx->params = *params;
    for (i = 0; i < ME_SURFACE_MAX_FONTIMAGES; i++) ctx->fontImages[i] = 0;

    ctx->commands = (float*)malloc(sizeof(float) * ME_SURFACE_INIT_COMMANDS_SIZE);
    if (!ctx->commands) goto error;
    ctx->ncommands = 0;
    ctx->ccommands = ME_SURFACE_INIT_COMMANDS_SIZE;

    ctx->cache = ME_surface___allocPathCache();
    if (ctx->cache == NULL) goto error;

    ME_surface_Save(ctx);
    ME_surface_Reset(ctx);

    ME_surface___setDevicePixelRatio(ctx, 1.0f);

    if (ctx->params.renderCreate(ctx->params.userPtr) == 0) goto error;

    // Init font rendering
    memset(&fontParams, 0, sizeof(fontParams));
    fontParams.width = ME_SURFACE_INIT_FONTIMAGE_SIZE;
    fontParams.height = ME_SURFACE_INIT_FONTIMAGE_SIZE;
    fontParams.flags = FONS_ZERO_TOPLEFT;
    fontParams.renderCreate = NULL;
    fontParams.renderUpdate = NULL;
    fontParams.renderDraw = NULL;
    fontParams.renderDelete = NULL;
    fontParams.userPtr = NULL;
    ctx->fs = fonsCreateInternal(&fontParams);
    if (ctx->fs == NULL) goto error;

    // Create font texture
    ctx->fontImages[0] = ctx->params.renderCreateTexture(ctx->params.userPtr, ME_SURFACE_TEXTURE_ALPHA, fontParams.width, fontParams.height, 0, NULL);
    if (ctx->fontImages[0] == 0) goto error;
    ctx->fontImageIdx = 0;

    return ctx;

error:
    ME_surface_DeleteInternal(ctx);
    return 0;
}

MEsurface_funcs* ME_surface_InternalParams(MEsurface_context* ctx) { return &ctx->params; }

void ME_surface_DeleteInternal(MEsurface_context* ctx) {
    int i;
    if (ctx == NULL) return;
    if (ctx->commands != NULL) free(ctx->commands);
    if (ctx->cache != NULL) ME_surface___deletePathCache(ctx->cache);

    if (ctx->fs) fonsDeleteInternal(ctx->fs);

    for (i = 0; i < ME_SURFACE_MAX_FONTIMAGES; i++) {
        if (ctx->fontImages[i] != 0) {
            ME_surface_DeleteImage(ctx, ctx->fontImages[i]);
            ctx->fontImages[i] = 0;
        }
    }

    if (ctx->params.renderDelete != NULL) ctx->params.renderDelete(ctx->params.userPtr);

    free(ctx);
}

void ME_surface_BeginFrame(MEsurface_context* ctx, float windowWidth, float windowHeight, float devicePixelRatio) {
    /*  printf("Tris: draws:%d  fill:%d  stroke:%d  text:%d  TOT:%d\n",
            ctx->drawCallCount, ctx->fillTriCount, ctx->strokeTriCount, ctx->textTriCount,
            ctx->fillTriCount+ctx->strokeTriCount+ctx->textTriCount);*/

    ctx->nstates = 0;
    ME_surface_Save(ctx);
    ME_surface_Reset(ctx);

    ME_surface___setDevicePixelRatio(ctx, devicePixelRatio);

    ctx->params.renderViewport(ctx->params.userPtr, windowWidth, windowHeight, devicePixelRatio);

    ctx->drawCallCount = 0;
    ctx->fillTriCount = 0;
    ctx->strokeTriCount = 0;
    ctx->textTriCount = 0;
}

void ME_surface_CancelFrame(MEsurface_context* ctx) { ctx->params.renderCancel(ctx->params.userPtr); }

void ME_surface_EndFrame(MEsurface_context* ctx) {
    ctx->params.renderFlush(ctx->params.userPtr);
    if (ctx->fontImageIdx != 0) {
        int fontImage = ctx->fontImages[ctx->fontImageIdx];
        ctx->fontImages[ctx->fontImageIdx] = 0;
        int i, j, iw, ih;
        // delete images that smaller than current one
        if (fontImage == 0) return;
        ME_surface_ImageSize(ctx, fontImage, &iw, &ih);
        for (i = j = 0; i < ctx->fontImageIdx; i++) {
            if (ctx->fontImages[i] != 0) {
                int nw, nh;
                int image = ctx->fontImages[i];
                ctx->fontImages[i] = 0;
                ME_surface_ImageSize(ctx, image, &nw, &nh);
                if (nw < iw || nh < ih)
                    ME_surface_DeleteImage(ctx, image);
                else
                    ctx->fontImages[j++] = image;
            }
        }
        // make current font image to first
        ctx->fontImages[j] = ctx->fontImages[0];
        ctx->fontImages[0] = fontImage;
        ctx->fontImageIdx = 0;
    }
}

MEsurface_color ME_surface_RGB(unsigned char r, unsigned char g, unsigned char b) { return ME_surface_RGBA(r, g, b, 255); }

MEsurface_color ME_surface_RGBf(float r, float g, float b) { return ME_surface_RGBAf(r, g, b, 1.0f); }

MEsurface_color ME_surface_RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    MEsurface_color color;
    // Use longer initialization to suppress warning.
    color.r = r / 255.0f;
    color.g = g / 255.0f;
    color.b = b / 255.0f;
    color.a = a / 255.0f;
    return color;
}

MEsurface_color ME_surface_RGBAf(float r, float g, float b, float a) {
    MEsurface_color color;
    // Use longer initialization to suppress warning.
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;
    return color;
}

MEsurface_color ME_surface_TransRGBA(MEsurface_color c, unsigned char a) {
    c.a = a / 255.0f;
    return c;
}

MEsurface_color ME_surface_TransRGBAf(MEsurface_color c, float a) {
    c.a = a;
    return c;
}

MEsurface_color ME_surface_LerpRGBA(MEsurface_color c0, MEsurface_color c1, float u) {
    int i;
    float oneminu;
    MEsurface_color cint = {{{0}}};

    u = ME_surface___clampf(u, 0.0f, 1.0f);
    oneminu = 1.0f - u;
    for (i = 0; i < 4; i++) {
        cint.rgba[i] = c0.rgba[i] * oneminu + c1.rgba[i] * u;
    }

    return cint;
}

MEsurface_color ME_surface_HSL(float h, float s, float l) { return ME_surface_HSLA(h, s, l, 255); }

static float ME_surface___hue(float h, float m1, float m2) {
    if (h < 0) h += 1;
    if (h > 1) h -= 1;
    if (h < 1.0f / 6.0f)
        return m1 + (m2 - m1) * h * 6.0f;
    else if (h < 3.0f / 6.0f)
        return m2;
    else if (h < 4.0f / 6.0f)
        return m1 + (m2 - m1) * (2.0f / 3.0f - h) * 6.0f;
    return m1;
}

MEsurface_color ME_surface_HSLA(float h, float s, float l, unsigned char a) {
    float m1, m2;
    MEsurface_color col;
    h = ME_surface___modf(h, 1.0f);
    if (h < 0.0f) h += 1.0f;
    s = ME_surface___clampf(s, 0.0f, 1.0f);
    l = ME_surface___clampf(l, 0.0f, 1.0f);
    m2 = l <= 0.5f ? (l * (1 + s)) : (l + s - l * s);
    m1 = 2 * l - m2;
    col.r = ME_surface___clampf(ME_surface___hue(h + 1.0f / 3.0f, m1, m2), 0.0f, 1.0f);
    col.g = ME_surface___clampf(ME_surface___hue(h, m1, m2), 0.0f, 1.0f);
    col.b = ME_surface___clampf(ME_surface___hue(h - 1.0f / 3.0f, m1, m2), 0.0f, 1.0f);
    col.a = a / 255.0f;
    return col;
}

void ME_surface_TransformIdentity(float* t) {
    t[0] = 1.0f;
    t[1] = 0.0f;
    t[2] = 0.0f;
    t[3] = 1.0f;
    t[4] = 0.0f;
    t[5] = 0.0f;
}

void ME_surface_TransformTranslate(float* t, float tx, float ty) {
    t[0] = 1.0f;
    t[1] = 0.0f;
    t[2] = 0.0f;
    t[3] = 1.0f;
    t[4] = tx;
    t[5] = ty;
}

void ME_surface_TransformScale(float* t, float sx, float sy) {
    t[0] = sx;
    t[1] = 0.0f;
    t[2] = 0.0f;
    t[3] = sy;
    t[4] = 0.0f;
    t[5] = 0.0f;
}

void ME_surface_TransformRotate(float* t, float a) {
    float cs = ME_surface___cosf(a), sn = ME_surface___sinf(a);
    t[0] = cs;
    t[1] = sn;
    t[2] = -sn;
    t[3] = cs;
    t[4] = 0.0f;
    t[5] = 0.0f;
}

void ME_surface_TransformSkewX(float* t, float a) {
    t[0] = 1.0f;
    t[1] = 0.0f;
    t[2] = ME_surface___tanf(a);
    t[3] = 1.0f;
    t[4] = 0.0f;
    t[5] = 0.0f;
}

void ME_surface_TransformSkewY(float* t, float a) {
    t[0] = 1.0f;
    t[1] = ME_surface___tanf(a);
    t[2] = 0.0f;
    t[3] = 1.0f;
    t[4] = 0.0f;
    t[5] = 0.0f;
}

void ME_surface_TransformMultiply(float* t, const float* s) {
    float t0 = t[0] * s[0] + t[1] * s[2];
    float t2 = t[2] * s[0] + t[3] * s[2];
    float t4 = t[4] * s[0] + t[5] * s[2] + s[4];
    t[1] = t[0] * s[1] + t[1] * s[3];
    t[3] = t[2] * s[1] + t[3] * s[3];
    t[5] = t[4] * s[1] + t[5] * s[3] + s[5];
    t[0] = t0;
    t[2] = t2;
    t[4] = t4;
}

void ME_surface_TransformPremultiply(float* t, const float* s) {
    float s2[6];
    memcpy(s2, s, sizeof(float) * 6);
    ME_surface_TransformMultiply(s2, t);
    memcpy(t, s2, sizeof(float) * 6);
}

int ME_surface_TransformInverse(float* inv, const float* t) {
    double invdet, det = (double)t[0] * t[3] - (double)t[2] * t[1];
    if (det > -1e-6 && det < 1e-6) {
        ME_surface_TransformIdentity(inv);
        return 0;
    }
    invdet = 1.0 / det;
    inv[0] = (float)(t[3] * invdet);
    inv[2] = (float)(-t[2] * invdet);
    inv[4] = (float)(((double)t[2] * t[5] - (double)t[3] * t[4]) * invdet);
    inv[1] = (float)(-t[1] * invdet);
    inv[3] = (float)(t[0] * invdet);
    inv[5] = (float)(((double)t[1] * t[4] - (double)t[0] * t[5]) * invdet);
    return 1;
}

void ME_surface_TransformPoint(float* dx, float* dy, const float* t, float sx, float sy) {
    *dx = sx * t[0] + sy * t[2] + t[4];
    *dy = sx * t[1] + sy * t[3] + t[5];
}

float ME_surface_DegToRad(float deg) { return deg / 180.0f * ME_SURFACE_PI; }

float ME_surface_RadToDeg(float rad) { return rad / ME_SURFACE_PI * 180.0f; }

static void ME_surface___setPaintColor(MEsurface_paint* p, MEsurface_color color) {
    memset(p, 0, sizeof(*p));
    ME_surface_TransformIdentity(p->xform);
    p->radius = 0.0f;
    p->feather = 1.0f;
    p->innerColor = color;
    p->outerColor = color;
}

// State handling
void ME_surface_Save(MEsurface_context* ctx) {
    if (ctx->nstates >= ME_SURFACE_MAX_STATES) return;
    if (ctx->nstates > 0) memcpy(&ctx->states[ctx->nstates], &ctx->states[ctx->nstates - 1], sizeof(MEsurface_state));
    ctx->nstates++;
}

void ME_surface_Restore(MEsurface_context* ctx) {
    if (ctx->nstates <= 1) return;
    ctx->nstates--;
}

void ME_surface_Reset(MEsurface_context* ctx) {
    MEsurface_state* state = ME_surface___getState(ctx);
    memset(state, 0, sizeof(*state));

    ME_surface___setPaintColor(&state->fill, ME_surface_RGBA(255, 255, 255, 255));
    ME_surface___setPaintColor(&state->stroke, ME_surface_RGBA(0, 0, 0, 255));
    state->compositeOperation = ME_surface___compositeOperationState(ME_SURFACE_SOURCE_OVER);
    state->shapeAntiAlias = 1;
    state->strokeWidth = 1.0f;
    state->miterLimit = 10.0f;
    state->lineCap = ME_SURFACE_BUTT;
    state->lineJoin = ME_SURFACE_MITER;
    state->alpha = 1.0f;
    ME_surface_TransformIdentity(state->xform);

    state->scissor.extent[0] = -1.0f;
    state->scissor.extent[1] = -1.0f;

    state->fontSize = 16.0f;
    state->letterSpacing = 0.0f;
    state->lineHeight = 1.0f;
    state->fontBlur = 0.0f;
    state->textAlign = ME_SURFACE_ALIGN_LEFT | ME_SURFACE_ALIGN_BASELINE;
    state->fontId = 0;
}

// State setting
void ME_surface_ShapeAntiAlias(MEsurface_context* ctx, int enabled) {
    MEsurface_state* state = ME_surface___getState(ctx);
    state->shapeAntiAlias = enabled;
}

void ME_surface_StrokeWidth(MEsurface_context* ctx, float width) {
    MEsurface_state* state = ME_surface___getState(ctx);
    state->strokeWidth = width;
}

void ME_surface_MiterLimit(MEsurface_context* ctx, float limit) {
    MEsurface_state* state = ME_surface___getState(ctx);
    state->miterLimit = limit;
}

void ME_surface_LineCap(MEsurface_context* ctx, int cap) {
    MEsurface_state* state = ME_surface___getState(ctx);
    state->lineCap = cap;
}

void ME_surface_LineJoin(MEsurface_context* ctx, int join) {
    MEsurface_state* state = ME_surface___getState(ctx);
    state->lineJoin = join;
}

void ME_surface_GlobalAlpha(MEsurface_context* ctx, float alpha) {
    MEsurface_state* state = ME_surface___getState(ctx);
    state->alpha = alpha;
}

void ME_surface_Transform(MEsurface_context* ctx, float a, float b, float c, float d, float e, float f) {
    MEsurface_state* state = ME_surface___getState(ctx);
    float t[6] = {a, b, c, d, e, f};
    ME_surface_TransformPremultiply(state->xform, t);
}

void ME_surface_ResetTransform(MEsurface_context* ctx) {
    MEsurface_state* state = ME_surface___getState(ctx);
    ME_surface_TransformIdentity(state->xform);
}

void ME_surface_Translate(MEsurface_context* ctx, float x, float y) {
    MEsurface_state* state = ME_surface___getState(ctx);
    float t[6];
    ME_surface_TransformTranslate(t, x, y);
    ME_surface_TransformPremultiply(state->xform, t);
}

void ME_surface_Rotate(MEsurface_context* ctx, float angle) {
    MEsurface_state* state = ME_surface___getState(ctx);
    float t[6];
    ME_surface_TransformRotate(t, angle);
    ME_surface_TransformPremultiply(state->xform, t);
}

void ME_surface_SkewX(MEsurface_context* ctx, float angle) {
    MEsurface_state* state = ME_surface___getState(ctx);
    float t[6];
    ME_surface_TransformSkewX(t, angle);
    ME_surface_TransformPremultiply(state->xform, t);
}

void ME_surface_SkewY(MEsurface_context* ctx, float angle) {
    MEsurface_state* state = ME_surface___getState(ctx);
    float t[6];
    ME_surface_TransformSkewY(t, angle);
    ME_surface_TransformPremultiply(state->xform, t);
}

void ME_surface_Scale(MEsurface_context* ctx, float x, float y) {
    MEsurface_state* state = ME_surface___getState(ctx);
    float t[6];
    ME_surface_TransformScale(t, x, y);
    ME_surface_TransformPremultiply(state->xform, t);
}

void ME_surface_CurrentTransform(MEsurface_context* ctx, float* xform) {
    MEsurface_state* state = ME_surface___getState(ctx);
    if (xform == NULL) return;
    memcpy(xform, state->xform, sizeof(float) * 6);
}

void ME_surface_StrokeColor(MEsurface_context* ctx, MEsurface_color color) {
    MEsurface_state* state = ME_surface___getState(ctx);
    ME_surface___setPaintColor(&state->stroke, color);
}

void ME_surface_StrokePaint(MEsurface_context* ctx, MEsurface_paint paint) {
    MEsurface_state* state = ME_surface___getState(ctx);
    state->stroke = paint;
    ME_surface_TransformMultiply(state->stroke.xform, state->xform);
}

void ME_surface_FillColor(MEsurface_context* ctx, MEsurface_color color) {
    MEsurface_state* state = ME_surface___getState(ctx);
    ME_surface___setPaintColor(&state->fill, color);
}

void ME_surface_FillPaint(MEsurface_context* ctx, MEsurface_paint paint) {
    MEsurface_state* state = ME_surface___getState(ctx);
    state->fill = paint;
    ME_surface_TransformMultiply(state->fill.xform, state->xform);
}

#ifndef ME_SURFACE_NO_STB
int ME_surface_CreateImage(MEsurface_context* ctx, const char* filename, int imageFlags) {
    int w, h, n, image;
    unsigned char* img;
    stbi_set_unpremultiply_on_load(1);
    stbi_convert_iphone_png_to_rgb(1);
    img = stbi_load(filename, &w, &h, &n, 4);
    if (img == NULL) {
        //      printf("Failed to load %s - %s\n", filename, stbi_failure_reason());
        return 0;
    }
    image = ME_surface_CreateImageRGBA(ctx, w, h, imageFlags, img);
    stbi_image_free(img);
    return image;
}

int ME_surface_CreateImageMem(MEsurface_context* ctx, int imageFlags, unsigned char* data, int ndata) {
    int w, h, n, image;
    unsigned char* img = stbi_load_from_memory(data, ndata, &w, &h, &n, 4);
    if (img == NULL) {
        //      printf("Failed to load %s - %s\n", filename, stbi_failure_reason());
        return 0;
    }
    image = ME_surface_CreateImageRGBA(ctx, w, h, imageFlags, img);
    stbi_image_free(img);
    return image;
}
#endif

int ME_surface_CreateImageRGBA(MEsurface_context* ctx, int w, int h, int imageFlags, const unsigned char* data) {
    return ctx->params.renderCreateTexture(ctx->params.userPtr, ME_SURFACE_TEXTURE_RGBA, w, h, imageFlags, data);
}

void ME_surface_UpdateImage(MEsurface_context* ctx, int image, const unsigned char* data) {
    int w, h;
    ctx->params.renderGetTextureSize(ctx->params.userPtr, image, &w, &h);
    ctx->params.renderUpdateTexture(ctx->params.userPtr, image, 0, 0, w, h, data);
}

void ME_surface_ImageSize(MEsurface_context* ctx, int image, int* w, int* h) { ctx->params.renderGetTextureSize(ctx->params.userPtr, image, w, h); }

void ME_surface_DeleteImage(MEsurface_context* ctx, int image) { ctx->params.renderDeleteTexture(ctx->params.userPtr, image); }

MEsurface_paint ME_surface_LinearGradient(MEsurface_context* ctx, float sx, float sy, float ex, float ey, MEsurface_color icol, MEsurface_color ocol) {
    MEsurface_paint p;
    float dx, dy, d;
    const float large = 1e5;

    memset(&p, 0, sizeof(p));

    // Calculate transform aligned to the line
    dx = ex - sx;
    dy = ey - sy;
    d = sqrtf(dx * dx + dy * dy);
    if (d > 0.0001f) {
        dx /= d;
        dy /= d;
    } else {
        dx = 0;
        dy = 1;
    }

    p.xform[0] = dy;
    p.xform[1] = -dx;
    p.xform[2] = dx;
    p.xform[3] = dy;
    p.xform[4] = sx - dx * large;
    p.xform[5] = sy - dy * large;

    p.extent[0] = large;
    p.extent[1] = large + d * 0.5f;

    p.radius = 0.0f;

    p.feather = ME_surface___maxf(1.0f, d);

    p.innerColor = icol;
    p.outerColor = ocol;

    return p;
}

MEsurface_paint ME_surface_RadialGradient(MEsurface_context* ctx, float cx, float cy, float inr, float outr, MEsurface_color icol, MEsurface_color ocol) {
    MEsurface_paint p;
    float r = (inr + outr) * 0.5f;
    float f = (outr - inr);

    memset(&p, 0, sizeof(p));

    ME_surface_TransformIdentity(p.xform);
    p.xform[4] = cx;
    p.xform[5] = cy;

    p.extent[0] = r;
    p.extent[1] = r;

    p.radius = r;

    p.feather = ME_surface___maxf(1.0f, f);

    p.innerColor = icol;
    p.outerColor = ocol;

    return p;
}

MEsurface_paint ME_surface_BoxGradient(MEsurface_context* ctx, float x, float y, float w, float h, float r, float f, MEsurface_color icol, MEsurface_color ocol) {
    MEsurface_paint p;

    memset(&p, 0, sizeof(p));

    ME_surface_TransformIdentity(p.xform);
    p.xform[4] = x + w * 0.5f;
    p.xform[5] = y + h * 0.5f;

    p.extent[0] = w * 0.5f;
    p.extent[1] = h * 0.5f;

    p.radius = r;

    p.feather = ME_surface___maxf(1.0f, f);

    p.innerColor = icol;
    p.outerColor = ocol;

    return p;
}

MEsurface_paint ME_surface_ImagePattern(MEsurface_context* ctx, float cx, float cy, float w, float h, float angle, int image, float alpha) {
    MEsurface_paint p;

    memset(&p, 0, sizeof(p));

    ME_surface_TransformRotate(p.xform, angle);
    p.xform[4] = cx;
    p.xform[5] = cy;

    p.extent[0] = w;
    p.extent[1] = h;

    p.image = image;

    p.innerColor = p.outerColor = ME_surface_RGBAf(1, 1, 1, alpha);

    return p;
}

// Scissoring
void ME_surface_Scissor(MEsurface_context* ctx, float x, float y, float w, float h) {
    MEsurface_state* state = ME_surface___getState(ctx);

    w = ME_surface___maxf(0.0f, w);
    h = ME_surface___maxf(0.0f, h);

    ME_surface_TransformIdentity(state->scissor.xform);
    state->scissor.xform[4] = x + w * 0.5f;
    state->scissor.xform[5] = y + h * 0.5f;
    ME_surface_TransformMultiply(state->scissor.xform, state->xform);

    state->scissor.extent[0] = w * 0.5f;
    state->scissor.extent[1] = h * 0.5f;
}

static void ME_surface___isectRects(float* dst, float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    float minx = ME_surface___maxf(ax, bx);
    float miny = ME_surface___maxf(ay, by);
    float maxx = ME_surface___minf(ax + aw, bx + bw);
    float maxy = ME_surface___minf(ay + ah, by + bh);
    dst[0] = minx;
    dst[1] = miny;
    dst[2] = ME_surface___maxf(0.0f, maxx - minx);
    dst[3] = ME_surface___maxf(0.0f, maxy - miny);
}

void ME_surface_IntersectScissor(MEsurface_context* ctx, float x, float y, float w, float h) {
    MEsurface_state* state = ME_surface___getState(ctx);
    float pxform[6], invxorm[6];
    float rect[4];
    float ex, ey, tex, tey;

    // If no previous scissor has been set, set the scissor as current scissor.
    if (state->scissor.extent[0] < 0) {
        ME_surface_Scissor(ctx, x, y, w, h);
        return;
    }

    // Transform the current scissor rect into current transform space.
    // If there is difference in rotation, this will be approximation.
    memcpy(pxform, state->scissor.xform, sizeof(float) * 6);
    ex = state->scissor.extent[0];
    ey = state->scissor.extent[1];
    ME_surface_TransformInverse(invxorm, state->xform);
    ME_surface_TransformMultiply(pxform, invxorm);
    tex = ex * ME_surface___absf(pxform[0]) + ey * ME_surface___absf(pxform[2]);
    tey = ex * ME_surface___absf(pxform[1]) + ey * ME_surface___absf(pxform[3]);

    // Intersect rects.
    ME_surface___isectRects(rect, pxform[4] - tex, pxform[5] - tey, tex * 2, tey * 2, x, y, w, h);

    ME_surface_Scissor(ctx, rect[0], rect[1], rect[2], rect[3]);
}

void ME_surface_ResetScissor(MEsurface_context* ctx) {
    MEsurface_state* state = ME_surface___getState(ctx);
    memset(state->scissor.xform, 0, sizeof(state->scissor.xform));
    state->scissor.extent[0] = -1.0f;
    state->scissor.extent[1] = -1.0f;
}

// Global composite operation.
void ME_surface_GlobalCompositeOperation(MEsurface_context* ctx, int op) {
    MEsurface_state* state = ME_surface___getState(ctx);
    state->compositeOperation = ME_surface___compositeOperationState(op);
}

void ME_surface_GlobalCompositeBlendFunc(MEsurface_context* ctx, int sfactor, int dfactor) { ME_surface_GlobalCompositeBlendFuncSeparate(ctx, sfactor, dfactor, sfactor, dfactor); }

void ME_surface_GlobalCompositeBlendFuncSeparate(MEsurface_context* ctx, int srcRGB, int dstRGB, int srcAlpha, int dstAlpha) {
    MEsurface_compositeOperationState op;
    op.srcRGB = srcRGB;
    op.dstRGB = dstRGB;
    op.srcAlpha = srcAlpha;
    op.dstAlpha = dstAlpha;

    MEsurface_state* state = ME_surface___getState(ctx);
    state->compositeOperation = op;
}

static int ME_surface___ptEquals(float x1, float y1, float x2, float y2, float tol) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy < tol * tol;
}

static float ME_surface___distPtSeg(float x, float y, float px, float py, float qx, float qy) {
    float pqx, pqy, dx, dy, d, t;
    pqx = qx - px;
    pqy = qy - py;
    dx = x - px;
    dy = y - py;
    d = pqx * pqx + pqy * pqy;
    t = pqx * dx + pqy * dy;
    if (d > 0) t /= d;
    if (t < 0)
        t = 0;
    else if (t > 1)
        t = 1;
    dx = px + t * pqx - x;
    dy = py + t * pqy - y;
    return dx * dx + dy * dy;
}

static void ME_surface___appendCommands(MEsurface_context* ctx, float* vals, int nvals) {
    MEsurface_state* state = ME_surface___getState(ctx);
    int i;

    if (ctx->ncommands + nvals > ctx->ccommands) {
        float* commands;
        int ccommands = ctx->ncommands + nvals + ctx->ccommands / 2;
        commands = (float*)realloc(ctx->commands, sizeof(float) * ccommands);
        if (commands == NULL) return;
        ctx->commands = commands;
        ctx->ccommands = ccommands;
    }

    if ((int)vals[0] != ME_SURFACE_CLOSE && (int)vals[0] != ME_SURFACE_WINDING) {
        ctx->commandx = vals[nvals - 2];
        ctx->commandy = vals[nvals - 1];
    }

    // transform commands
    i = 0;
    while (i < nvals) {
        int cmd = (int)vals[i];
        switch (cmd) {
            case ME_SURFACE_MOVETO:
                ME_surface_TransformPoint(&vals[i + 1], &vals[i + 2], state->xform, vals[i + 1], vals[i + 2]);
                i += 3;
                break;
            case ME_SURFACE_LINETO:
                ME_surface_TransformPoint(&vals[i + 1], &vals[i + 2], state->xform, vals[i + 1], vals[i + 2]);
                i += 3;
                break;
            case ME_SURFACE_BEZIERTO:
                ME_surface_TransformPoint(&vals[i + 1], &vals[i + 2], state->xform, vals[i + 1], vals[i + 2]);
                ME_surface_TransformPoint(&vals[i + 3], &vals[i + 4], state->xform, vals[i + 3], vals[i + 4]);
                ME_surface_TransformPoint(&vals[i + 5], &vals[i + 6], state->xform, vals[i + 5], vals[i + 6]);
                i += 7;
                break;
            case ME_SURFACE_CLOSE:
                i++;
                break;
            case ME_SURFACE_WINDING:
                i += 2;
                break;
            default:
                i++;
        }
    }

    memcpy(&ctx->commands[ctx->ncommands], vals, nvals * sizeof(float));

    ctx->ncommands += nvals;
}

static void ME_surface___clearPathCache(MEsurface_context* ctx) {
    ctx->cache->npoints = 0;
    ctx->cache->npaths = 0;
}

static MEsurface_path* ME_surface___lastPath(MEsurface_context* ctx) {
    if (ctx->cache->npaths > 0) return &ctx->cache->paths[ctx->cache->npaths - 1];
    return NULL;
}

static void ME_surface___addPath(MEsurface_context* ctx) {
    MEsurface_path* path;
    if (ctx->cache->npaths + 1 > ctx->cache->cpaths) {
        MEsurface_path* paths;
        int cpaths = ctx->cache->npaths + 1 + ctx->cache->cpaths / 2;
        paths = (MEsurface_path*)realloc(ctx->cache->paths, sizeof(MEsurface_path) * cpaths);
        if (paths == NULL) return;
        ctx->cache->paths = paths;
        ctx->cache->cpaths = cpaths;
    }
    path = &ctx->cache->paths[ctx->cache->npaths];
    memset(path, 0, sizeof(*path));
    path->first = ctx->cache->npoints;
    path->winding = ME_SURFACE_CCW;

    ctx->cache->npaths++;
}

static MEsurface_point* ME_surface___lastPoint(MEsurface_context* ctx) {
    if (ctx->cache->npoints > 0) return &ctx->cache->points[ctx->cache->npoints - 1];
    return NULL;
}

static void ME_surface___addPoint(MEsurface_context* ctx, float x, float y, int flags) {
    MEsurface_path* path = ME_surface___lastPath(ctx);
    MEsurface_point* pt;
    if (path == NULL) return;

    if (path->count > 0 && ctx->cache->npoints > 0) {
        pt = ME_surface___lastPoint(ctx);
        if (ME_surface___ptEquals(pt->x, pt->y, x, y, ctx->distTol)) {
            pt->flags |= flags;
            return;
        }
    }

    if (ctx->cache->npoints + 1 > ctx->cache->cpoints) {
        MEsurface_point* points;
        int cpoints = ctx->cache->npoints + 1 + ctx->cache->cpoints / 2;
        points = (MEsurface_point*)realloc(ctx->cache->points, sizeof(MEsurface_point) * cpoints);
        if (points == NULL) return;
        ctx->cache->points = points;
        ctx->cache->cpoints = cpoints;
    }

    pt = &ctx->cache->points[ctx->cache->npoints];
    memset(pt, 0, sizeof(*pt));
    pt->x = x;
    pt->y = y;
    pt->flags = (unsigned char)flags;

    ctx->cache->npoints++;
    path->count++;
}

static void ME_surface___closePath(MEsurface_context* ctx) {
    MEsurface_path* path = ME_surface___lastPath(ctx);
    if (path == NULL) return;
    path->closed = 1;
}

static void ME_surface___pathWinding(MEsurface_context* ctx, int winding) {
    MEsurface_path* path = ME_surface___lastPath(ctx);
    if (path == NULL) return;
    path->winding = winding;
}

static float ME_surface___getAverageScale(float* t) {
    float sx = sqrtf(t[0] * t[0] + t[2] * t[2]);
    float sy = sqrtf(t[1] * t[1] + t[3] * t[3]);
    return (sx + sy) * 0.5f;
}

static MEsurface_vertex* ME_surface___allocTempVerts(MEsurface_context* ctx, int nverts) {
    if (nverts > ctx->cache->cverts) {
        MEsurface_vertex* verts;
        int cverts = (nverts + 0xff) & ~0xff;  // Round up to prevent allocations when things change just slightly.
        verts = (MEsurface_vertex*)realloc(ctx->cache->verts, sizeof(MEsurface_vertex) * cverts);
        if (verts == NULL) return NULL;
        ctx->cache->verts = verts;
        ctx->cache->cverts = cverts;
    }

    return ctx->cache->verts;
}

static float ME_surface___triarea2(float ax, float ay, float bx, float by, float cx, float cy) {
    float abx = bx - ax;
    float aby = by - ay;
    float acx = cx - ax;
    float acy = cy - ay;
    return acx * aby - abx * acy;
}

static float ME_surface___polyArea(MEsurface_point* pts, int npts) {
    int i;
    float area = 0;
    for (i = 2; i < npts; i++) {
        MEsurface_point* a = &pts[0];
        MEsurface_point* b = &pts[i - 1];
        MEsurface_point* c = &pts[i];
        area += ME_surface___triarea2(a->x, a->y, b->x, b->y, c->x, c->y);
    }
    return area * 0.5f;
}

static void ME_surface___polyReverse(MEsurface_point* pts, int npts) {
    MEsurface_point tmp;
    int i = 0, j = npts - 1;
    while (i < j) {
        tmp = pts[i];
        pts[i] = pts[j];
        pts[j] = tmp;
        i++;
        j--;
    }
}

static void ME_surface___vset(MEsurface_vertex* vtx, float x, float y, float u, float v) {
    vtx->x = x;
    vtx->y = y;
    vtx->u = u;
    vtx->v = v;
}

static void ME_surface___tesselateBezier(MEsurface_context* ctx, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int level, int type) {
    float x12, y12, x23, y23, x34, y34, x123, y123, x234, y234, x1234, y1234;
    float dx, dy, d2, d3;

    if (level > 10) return;

    x12 = (x1 + x2) * 0.5f;
    y12 = (y1 + y2) * 0.5f;
    x23 = (x2 + x3) * 0.5f;
    y23 = (y2 + y3) * 0.5f;
    x34 = (x3 + x4) * 0.5f;
    y34 = (y3 + y4) * 0.5f;
    x123 = (x12 + x23) * 0.5f;
    y123 = (y12 + y23) * 0.5f;

    dx = x4 - x1;
    dy = y4 - y1;
    d2 = ME_surface___absf(((x2 - x4) * dy - (y2 - y4) * dx));
    d3 = ME_surface___absf(((x3 - x4) * dy - (y3 - y4) * dx));

    if ((d2 + d3) * (d2 + d3) < ctx->tessTol * (dx * dx + dy * dy)) {
        ME_surface___addPoint(ctx, x4, y4, type);
        return;
    }

    /*  if (ME_surface___absf(x1+x3-x2-x2) + ME_surface___absf(y1+y3-y2-y2) + ME_surface___absf(x2+x4-x3-x3) + ME_surface___absf(y2+y4-y3-y3) < ctx->tessTol) {
            ME_surface___addPoint(ctx, x4, y4, type);
            return;
        }*/

    x234 = (x23 + x34) * 0.5f;
    y234 = (y23 + y34) * 0.5f;
    x1234 = (x123 + x234) * 0.5f;
    y1234 = (y123 + y234) * 0.5f;

    ME_surface___tesselateBezier(ctx, x1, y1, x12, y12, x123, y123, x1234, y1234, level + 1, 0);
    ME_surface___tesselateBezier(ctx, x1234, y1234, x234, y234, x34, y34, x4, y4, level + 1, type);
}

static void ME_surface___flattenPaths(MEsurface_context* ctx) {
    MEsurface_pathCache* cache = ctx->cache;
    //  MEsurface_state* state = ME_surface___getState(ctx);
    MEsurface_point* last;
    MEsurface_point* p0;
    MEsurface_point* p1;
    MEsurface_point* pts;
    MEsurface_path* path;
    int i, j;
    float* cp1;
    float* cp2;
    float* p;
    float area;

    if (cache->npaths > 0) return;

    // Flatten
    i = 0;
    while (i < ctx->ncommands) {
        int cmd = (int)ctx->commands[i];
        switch (cmd) {
            case ME_SURFACE_MOVETO:
                ME_surface___addPath(ctx);
                p = &ctx->commands[i + 1];
                ME_surface___addPoint(ctx, p[0], p[1], ME_SURFACE_PT_CORNER);
                i += 3;
                break;
            case ME_SURFACE_LINETO:
                p = &ctx->commands[i + 1];
                ME_surface___addPoint(ctx, p[0], p[1], ME_SURFACE_PT_CORNER);
                i += 3;
                break;
            case ME_SURFACE_BEZIERTO:
                last = ME_surface___lastPoint(ctx);
                if (last != NULL) {
                    cp1 = &ctx->commands[i + 1];
                    cp2 = &ctx->commands[i + 3];
                    p = &ctx->commands[i + 5];
                    ME_surface___tesselateBezier(ctx, last->x, last->y, cp1[0], cp1[1], cp2[0], cp2[1], p[0], p[1], 0, ME_SURFACE_PT_CORNER);
                }
                i += 7;
                break;
            case ME_SURFACE_CLOSE:
                ME_surface___closePath(ctx);
                i++;
                break;
            case ME_SURFACE_WINDING:
                ME_surface___pathWinding(ctx, (int)ctx->commands[i + 1]);
                i += 2;
                break;
            default:
                i++;
        }
    }

    cache->bounds[0] = cache->bounds[1] = 1e6f;
    cache->bounds[2] = cache->bounds[3] = -1e6f;

    // Calculate the direction and length of line segments.
    for (j = 0; j < cache->npaths; j++) {
        path = &cache->paths[j];
        pts = &cache->points[path->first];

        // If the first and last points are the same, remove the last, mark as closed path.
        p0 = &pts[path->count - 1];
        p1 = &pts[0];
        if (ME_surface___ptEquals(p0->x, p0->y, p1->x, p1->y, ctx->distTol)) {
            path->count--;
            p0 = &pts[path->count - 1];
            path->closed = 1;
        }

        // Enforce winding.
        if (path->count > 2) {
            area = ME_surface___polyArea(pts, path->count);
            if (path->winding == ME_SURFACE_CCW && area < 0.0f) ME_surface___polyReverse(pts, path->count);
            if (path->winding == ME_SURFACE_CW && area > 0.0f) ME_surface___polyReverse(pts, path->count);
        }

        for (i = 0; i < path->count; i++) {
            // Calculate segment direction and length
            p0->dx = p1->x - p0->x;
            p0->dy = p1->y - p0->y;
            p0->len = ME_surface___normalize(&p0->dx, &p0->dy);
            // Update bounds
            cache->bounds[0] = ME_surface___minf(cache->bounds[0], p0->x);
            cache->bounds[1] = ME_surface___minf(cache->bounds[1], p0->y);
            cache->bounds[2] = ME_surface___maxf(cache->bounds[2], p0->x);
            cache->bounds[3] = ME_surface___maxf(cache->bounds[3], p0->y);
            // Advance
            p0 = p1++;
        }
    }
}

static int ME_surface___curveDivs(float r, float arc, float tol) {
    float da = acosf(r / (r + tol)) * 2.0f;
    return ME_surface___maxi(2, (int)ceilf(arc / da));
}

static void ME_surface___chooseBevel(int bevel, MEsurface_point* p0, MEsurface_point* p1, float w, float* x0, float* y0, float* x1, float* y1) {
    if (bevel) {
        *x0 = p1->x + p0->dy * w;
        *y0 = p1->y - p0->dx * w;
        *x1 = p1->x + p1->dy * w;
        *y1 = p1->y - p1->dx * w;
    } else {
        *x0 = p1->x + p1->dmx * w;
        *y0 = p1->y + p1->dmy * w;
        *x1 = p1->x + p1->dmx * w;
        *y1 = p1->y + p1->dmy * w;
    }
}

static MEsurface_vertex* ME_surface___roundJoin(MEsurface_vertex* dst, MEsurface_point* p0, MEsurface_point* p1, float lw, float rw, float lu, float ru, int ncap, float fringe) {
    int i, n;
    float dlx0 = p0->dy;
    float dly0 = -p0->dx;
    float dlx1 = p1->dy;
    float dly1 = -p1->dx;

    if (p1->flags & ME_SURFACE_PT_LEFT) {
        float lx0, ly0, lx1, ly1, a0, a1;
        ME_surface___chooseBevel(p1->flags & ME_SURFACE_PR_INNERBEVEL, p0, p1, lw, &lx0, &ly0, &lx1, &ly1);
        a0 = atan2f(-dly0, -dlx0);
        a1 = atan2f(-dly1, -dlx1);
        if (a1 > a0) a1 -= ME_SURFACE_PI * 2;

        ME_surface___vset(dst, lx0, ly0, lu, 1);
        dst++;
        ME_surface___vset(dst, p1->x - dlx0 * rw, p1->y - dly0 * rw, ru, 1);
        dst++;

        n = ME_surface___clampi((int)ceilf(((a0 - a1) / ME_SURFACE_PI) * ncap), 2, ncap);
        for (i = 0; i < n; i++) {
            float u = i / (float)(n - 1);
            float a = a0 + u * (a1 - a0);
            float rx = p1->x + cosf(a) * rw;
            float ry = p1->y + sinf(a) * rw;
            ME_surface___vset(dst, p1->x, p1->y, 0.5f, 1);
            dst++;
            ME_surface___vset(dst, rx, ry, ru, 1);
            dst++;
        }

        ME_surface___vset(dst, lx1, ly1, lu, 1);
        dst++;
        ME_surface___vset(dst, p1->x - dlx1 * rw, p1->y - dly1 * rw, ru, 1);
        dst++;

    } else {
        float rx0, ry0, rx1, ry1, a0, a1;
        ME_surface___chooseBevel(p1->flags & ME_SURFACE_PR_INNERBEVEL, p0, p1, -rw, &rx0, &ry0, &rx1, &ry1);
        a0 = atan2f(dly0, dlx0);
        a1 = atan2f(dly1, dlx1);
        if (a1 < a0) a1 += ME_SURFACE_PI * 2;

        ME_surface___vset(dst, p1->x + dlx0 * rw, p1->y + dly0 * rw, lu, 1);
        dst++;
        ME_surface___vset(dst, rx0, ry0, ru, 1);
        dst++;

        n = ME_surface___clampi((int)ceilf(((a1 - a0) / ME_SURFACE_PI) * ncap), 2, ncap);
        for (i = 0; i < n; i++) {
            float u = i / (float)(n - 1);
            float a = a0 + u * (a1 - a0);
            float lx = p1->x + cosf(a) * lw;
            float ly = p1->y + sinf(a) * lw;
            ME_surface___vset(dst, lx, ly, lu, 1);
            dst++;
            ME_surface___vset(dst, p1->x, p1->y, 0.5f, 1);
            dst++;
        }

        ME_surface___vset(dst, p1->x + dlx1 * rw, p1->y + dly1 * rw, lu, 1);
        dst++;
        ME_surface___vset(dst, rx1, ry1, ru, 1);
        dst++;
    }
    return dst;
}

static MEsurface_vertex* ME_surface___bevelJoin(MEsurface_vertex* dst, MEsurface_point* p0, MEsurface_point* p1, float lw, float rw, float lu, float ru, float fringe) {
    float rx0, ry0, rx1, ry1;
    float lx0, ly0, lx1, ly1;
    float dlx0 = p0->dy;
    float dly0 = -p0->dx;
    float dlx1 = p1->dy;
    float dly1 = -p1->dx;

    if (p1->flags & ME_SURFACE_PT_LEFT) {
        ME_surface___chooseBevel(p1->flags & ME_SURFACE_PR_INNERBEVEL, p0, p1, lw, &lx0, &ly0, &lx1, &ly1);

        ME_surface___vset(dst, lx0, ly0, lu, 1);
        dst++;
        ME_surface___vset(dst, p1->x - dlx0 * rw, p1->y - dly0 * rw, ru, 1);
        dst++;

        if (p1->flags & ME_SURFACE_PT_BEVEL) {
            ME_surface___vset(dst, lx0, ly0, lu, 1);
            dst++;
            ME_surface___vset(dst, p1->x - dlx0 * rw, p1->y - dly0 * rw, ru, 1);
            dst++;

            ME_surface___vset(dst, lx1, ly1, lu, 1);
            dst++;
            ME_surface___vset(dst, p1->x - dlx1 * rw, p1->y - dly1 * rw, ru, 1);
            dst++;
        } else {
            rx0 = p1->x - p1->dmx * rw;
            ry0 = p1->y - p1->dmy * rw;

            ME_surface___vset(dst, p1->x, p1->y, 0.5f, 1);
            dst++;
            ME_surface___vset(dst, p1->x - dlx0 * rw, p1->y - dly0 * rw, ru, 1);
            dst++;

            ME_surface___vset(dst, rx0, ry0, ru, 1);
            dst++;
            ME_surface___vset(dst, rx0, ry0, ru, 1);
            dst++;

            ME_surface___vset(dst, p1->x, p1->y, 0.5f, 1);
            dst++;
            ME_surface___vset(dst, p1->x - dlx1 * rw, p1->y - dly1 * rw, ru, 1);
            dst++;
        }

        ME_surface___vset(dst, lx1, ly1, lu, 1);
        dst++;
        ME_surface___vset(dst, p1->x - dlx1 * rw, p1->y - dly1 * rw, ru, 1);
        dst++;

    } else {
        ME_surface___chooseBevel(p1->flags & ME_SURFACE_PR_INNERBEVEL, p0, p1, -rw, &rx0, &ry0, &rx1, &ry1);

        ME_surface___vset(dst, p1->x + dlx0 * lw, p1->y + dly0 * lw, lu, 1);
        dst++;
        ME_surface___vset(dst, rx0, ry0, ru, 1);
        dst++;

        if (p1->flags & ME_SURFACE_PT_BEVEL) {
            ME_surface___vset(dst, p1->x + dlx0 * lw, p1->y + dly0 * lw, lu, 1);
            dst++;
            ME_surface___vset(dst, rx0, ry0, ru, 1);
            dst++;

            ME_surface___vset(dst, p1->x + dlx1 * lw, p1->y + dly1 * lw, lu, 1);
            dst++;
            ME_surface___vset(dst, rx1, ry1, ru, 1);
            dst++;
        } else {
            lx0 = p1->x + p1->dmx * lw;
            ly0 = p1->y + p1->dmy * lw;

            ME_surface___vset(dst, p1->x + dlx0 * lw, p1->y + dly0 * lw, lu, 1);
            dst++;
            ME_surface___vset(dst, p1->x, p1->y, 0.5f, 1);
            dst++;

            ME_surface___vset(dst, lx0, ly0, lu, 1);
            dst++;
            ME_surface___vset(dst, lx0, ly0, lu, 1);
            dst++;

            ME_surface___vset(dst, p1->x + dlx1 * lw, p1->y + dly1 * lw, lu, 1);
            dst++;
            ME_surface___vset(dst, p1->x, p1->y, 0.5f, 1);
            dst++;
        }

        ME_surface___vset(dst, p1->x + dlx1 * lw, p1->y + dly1 * lw, lu, 1);
        dst++;
        ME_surface___vset(dst, rx1, ry1, ru, 1);
        dst++;
    }

    return dst;
}

static MEsurface_vertex* ME_surface___buttCapStart(MEsurface_vertex* dst, MEsurface_point* p, float dx, float dy, float w, float d, float aa, float u0, float u1) {
    float px = p->x - dx * d;
    float py = p->y - dy * d;
    float dlx = dy;
    float dly = -dx;
    ME_surface___vset(dst, px + dlx * w - dx * aa, py + dly * w - dy * aa, u0, 0);
    dst++;
    ME_surface___vset(dst, px - dlx * w - dx * aa, py - dly * w - dy * aa, u1, 0);
    dst++;
    ME_surface___vset(dst, px + dlx * w, py + dly * w, u0, 1);
    dst++;
    ME_surface___vset(dst, px - dlx * w, py - dly * w, u1, 1);
    dst++;
    return dst;
}

static MEsurface_vertex* ME_surface___buttCapEnd(MEsurface_vertex* dst, MEsurface_point* p, float dx, float dy, float w, float d, float aa, float u0, float u1) {
    float px = p->x + dx * d;
    float py = p->y + dy * d;
    float dlx = dy;
    float dly = -dx;
    ME_surface___vset(dst, px + dlx * w, py + dly * w, u0, 1);
    dst++;
    ME_surface___vset(dst, px - dlx * w, py - dly * w, u1, 1);
    dst++;
    ME_surface___vset(dst, px + dlx * w + dx * aa, py + dly * w + dy * aa, u0, 0);
    dst++;
    ME_surface___vset(dst, px - dlx * w + dx * aa, py - dly * w + dy * aa, u1, 0);
    dst++;
    return dst;
}

static MEsurface_vertex* ME_surface___roundCapStart(MEsurface_vertex* dst, MEsurface_point* p, float dx, float dy, float w, int ncap, float aa, float u0, float u1) {
    int i;
    float px = p->x;
    float py = p->y;
    float dlx = dy;
    float dly = -dx;

    for (i = 0; i < ncap; i++) {
        float a = i / (float)(ncap - 1) * ME_SURFACE_PI;
        float ax = cosf(a) * w, ay = sinf(a) * w;
        ME_surface___vset(dst, px - dlx * ax - dx * ay, py - dly * ax - dy * ay, u0, 1);
        dst++;
        ME_surface___vset(dst, px, py, 0.5f, 1);
        dst++;
    }
    ME_surface___vset(dst, px + dlx * w, py + dly * w, u0, 1);
    dst++;
    ME_surface___vset(dst, px - dlx * w, py - dly * w, u1, 1);
    dst++;
    return dst;
}

static MEsurface_vertex* ME_surface___roundCapEnd(MEsurface_vertex* dst, MEsurface_point* p, float dx, float dy, float w, int ncap, float aa, float u0, float u1) {
    int i;
    float px = p->x;
    float py = p->y;
    float dlx = dy;
    float dly = -dx;

    ME_surface___vset(dst, px + dlx * w, py + dly * w, u0, 1);
    dst++;
    ME_surface___vset(dst, px - dlx * w, py - dly * w, u1, 1);
    dst++;
    for (i = 0; i < ncap; i++) {
        float a = i / (float)(ncap - 1) * ME_SURFACE_PI;
        float ax = cosf(a) * w, ay = sinf(a) * w;
        ME_surface___vset(dst, px, py, 0.5f, 1);
        dst++;
        ME_surface___vset(dst, px - dlx * ax + dx * ay, py - dly * ax + dy * ay, u0, 1);
        dst++;
    }
    return dst;
}

static void ME_surface___calculateJoins(MEsurface_context* ctx, float w, int lineJoin, float miterLimit) {
    MEsurface_pathCache* cache = ctx->cache;
    int i, j;
    float iw = 0.0f;

    if (w > 0.0f) iw = 1.0f / w;

    // Calculate which joins needs extra vertices to append, and gather vertex count.
    for (i = 0; i < cache->npaths; i++) {
        MEsurface_path* path = &cache->paths[i];
        MEsurface_point* pts = &cache->points[path->first];
        MEsurface_point* p0 = &pts[path->count - 1];
        MEsurface_point* p1 = &pts[0];
        int nleft = 0;

        path->nbevel = 0;

        for (j = 0; j < path->count; j++) {
            float dlx0, dly0, dlx1, dly1, dmr2, cross, limit;
            dlx0 = p0->dy;
            dly0 = -p0->dx;
            dlx1 = p1->dy;
            dly1 = -p1->dx;
            // Calculate extrusions
            p1->dmx = (dlx0 + dlx1) * 0.5f;
            p1->dmy = (dly0 + dly1) * 0.5f;
            dmr2 = p1->dmx * p1->dmx + p1->dmy * p1->dmy;
            if (dmr2 > 0.000001f) {
                float scale = 1.0f / dmr2;
                if (scale > 600.0f) {
                    scale = 600.0f;
                }
                p1->dmx *= scale;
                p1->dmy *= scale;
            }

            // Clear flags, but keep the corner.
            p1->flags = (p1->flags & ME_SURFACE_PT_CORNER) ? ME_SURFACE_PT_CORNER : 0;

            // Keep track of left turns.
            cross = p1->dx * p0->dy - p0->dx * p1->dy;
            if (cross > 0.0f) {
                nleft++;
                p1->flags |= ME_SURFACE_PT_LEFT;
            }

            // Calculate if we should use bevel or miter for inner join.
            limit = ME_surface___maxf(1.01f, ME_surface___minf(p0->len, p1->len) * iw);
            if ((dmr2 * limit * limit) < 1.0f) p1->flags |= ME_SURFACE_PR_INNERBEVEL;

            // Check to see if the corner needs to be beveled.
            if (p1->flags & ME_SURFACE_PT_CORNER) {
                if ((dmr2 * miterLimit * miterLimit) < 1.0f || lineJoin == ME_SURFACE_BEVEL || lineJoin == ME_SURFACE_ROUND) {
                    p1->flags |= ME_SURFACE_PT_BEVEL;
                }
            }

            if ((p1->flags & (ME_SURFACE_PT_BEVEL | ME_SURFACE_PR_INNERBEVEL)) != 0) path->nbevel++;

            p0 = p1++;
        }

        path->convex = (nleft == path->count) ? 1 : 0;
    }
}

static int ME_surface___expandStroke(MEsurface_context* ctx, float w, float fringe, int lineCap, int lineJoin, float miterLimit) {
    MEsurface_pathCache* cache = ctx->cache;
    MEsurface_vertex* verts;
    MEsurface_vertex* dst;
    int cverts, i, j;
    float aa = fringe;  // ctx->fringeWidth;
    float u0 = 0.0f, u1 = 1.0f;
    int ncap = ME_surface___curveDivs(w, ME_SURFACE_PI, ctx->tessTol);  // Calculate divisions per half circle.

    w += aa * 0.5f;

    // Disable the gradient used for antialiasing when antialiasing is not used.
    if (aa == 0.0f) {
        u0 = 0.5f;
        u1 = 0.5f;
    }

    ME_surface___calculateJoins(ctx, w, lineJoin, miterLimit);

    // Calculate max vertex usage.
    cverts = 0;
    for (i = 0; i < cache->npaths; i++) {
        MEsurface_path* path = &cache->paths[i];
        int loop = (path->closed == 0) ? 0 : 1;
        if (lineJoin == ME_SURFACE_ROUND)
            cverts += (path->count + path->nbevel * (ncap + 2) + 1) * 2;  // plus one for loop
        else
            cverts += (path->count + path->nbevel * 5 + 1) * 2;  // plus one for loop
        if (loop == 0) {
            // space for caps
            if (lineCap == ME_SURFACE_ROUND) {
                cverts += (ncap * 2 + 2) * 2;
            } else {
                cverts += (3 + 3) * 2;
            }
        }
    }

    verts = ME_surface___allocTempVerts(ctx, cverts);
    if (verts == NULL) return 0;

    for (i = 0; i < cache->npaths; i++) {
        MEsurface_path* path = &cache->paths[i];
        MEsurface_point* pts = &cache->points[path->first];
        MEsurface_point* p0;
        MEsurface_point* p1;
        int s, e, loop;
        float dx, dy;

        path->fill = 0;
        path->nfill = 0;

        // Calculate fringe or stroke
        loop = (path->closed == 0) ? 0 : 1;
        dst = verts;
        path->stroke = dst;

        if (loop) {
            // Looping
            p0 = &pts[path->count - 1];
            p1 = &pts[0];
            s = 0;
            e = path->count;
        } else {
            // Add cap
            p0 = &pts[0];
            p1 = &pts[1];
            s = 1;
            e = path->count - 1;
        }

        if (loop == 0) {
            // Add cap
            dx = p1->x - p0->x;
            dy = p1->y - p0->y;
            ME_surface___normalize(&dx, &dy);
            if (lineCap == ME_SURFACE_BUTT)
                dst = ME_surface___buttCapStart(dst, p0, dx, dy, w, -aa * 0.5f, aa, u0, u1);
            else if (lineCap == ME_SURFACE_BUTT || lineCap == ME_SURFACE_SQUARE)
                dst = ME_surface___buttCapStart(dst, p0, dx, dy, w, w - aa, aa, u0, u1);
            else if (lineCap == ME_SURFACE_ROUND)
                dst = ME_surface___roundCapStart(dst, p0, dx, dy, w, ncap, aa, u0, u1);
        }

        for (j = s; j < e; ++j) {
            if ((p1->flags & (ME_SURFACE_PT_BEVEL | ME_SURFACE_PR_INNERBEVEL)) != 0) {
                if (lineJoin == ME_SURFACE_ROUND) {
                    dst = ME_surface___roundJoin(dst, p0, p1, w, w, u0, u1, ncap, aa);
                } else {
                    dst = ME_surface___bevelJoin(dst, p0, p1, w, w, u0, u1, aa);
                }
            } else {
                ME_surface___vset(dst, p1->x + (p1->dmx * w), p1->y + (p1->dmy * w), u0, 1);
                dst++;
                ME_surface___vset(dst, p1->x - (p1->dmx * w), p1->y - (p1->dmy * w), u1, 1);
                dst++;
            }
            p0 = p1++;
        }

        if (loop) {
            // Loop it
            ME_surface___vset(dst, verts[0].x, verts[0].y, u0, 1);
            dst++;
            ME_surface___vset(dst, verts[1].x, verts[1].y, u1, 1);
            dst++;
        } else {
            // Add cap
            dx = p1->x - p0->x;
            dy = p1->y - p0->y;
            ME_surface___normalize(&dx, &dy);
            if (lineCap == ME_SURFACE_BUTT)
                dst = ME_surface___buttCapEnd(dst, p1, dx, dy, w, -aa * 0.5f, aa, u0, u1);
            else if (lineCap == ME_SURFACE_BUTT || lineCap == ME_SURFACE_SQUARE)
                dst = ME_surface___buttCapEnd(dst, p1, dx, dy, w, w - aa, aa, u0, u1);
            else if (lineCap == ME_SURFACE_ROUND)
                dst = ME_surface___roundCapEnd(dst, p1, dx, dy, w, ncap, aa, u0, u1);
        }

        path->nstroke = (int)(dst - verts);

        verts = dst;
    }

    return 1;
}

static int ME_surface___expandFill(MEsurface_context* ctx, float w, int lineJoin, float miterLimit) {
    MEsurface_pathCache* cache = ctx->cache;
    MEsurface_vertex* verts;
    MEsurface_vertex* dst;
    int cverts, convex, i, j;
    float aa = ctx->fringeWidth;
    int fringe = w > 0.0f;

    ME_surface___calculateJoins(ctx, w, lineJoin, miterLimit);

    // Calculate max vertex usage.
    cverts = 0;
    for (i = 0; i < cache->npaths; i++) {
        MEsurface_path* path = &cache->paths[i];
        cverts += path->count + path->nbevel + 1;
        if (fringe) cverts += (path->count + path->nbevel * 5 + 1) * 2;  // plus one for loop
    }

    verts = ME_surface___allocTempVerts(ctx, cverts);
    if (verts == NULL) return 0;

    convex = cache->npaths == 1 && cache->paths[0].convex;

    for (i = 0; i < cache->npaths; i++) {
        MEsurface_path* path = &cache->paths[i];
        MEsurface_point* pts = &cache->points[path->first];
        MEsurface_point* p0;
        MEsurface_point* p1;
        float rw, lw, woff;
        float ru, lu;

        // Calculate shape vertices.
        woff = 0.5f * aa;
        dst = verts;
        path->fill = dst;

        if (fringe) {
            // Looping
            p0 = &pts[path->count - 1];
            p1 = &pts[0];
            for (j = 0; j < path->count; ++j) {
                if (p1->flags & ME_SURFACE_PT_BEVEL) {
                    float dlx0 = p0->dy;
                    float dly0 = -p0->dx;
                    float dlx1 = p1->dy;
                    float dly1 = -p1->dx;
                    if (p1->flags & ME_SURFACE_PT_LEFT) {
                        float lx = p1->x + p1->dmx * woff;
                        float ly = p1->y + p1->dmy * woff;
                        ME_surface___vset(dst, lx, ly, 0.5f, 1);
                        dst++;
                    } else {
                        float lx0 = p1->x + dlx0 * woff;
                        float ly0 = p1->y + dly0 * woff;
                        float lx1 = p1->x + dlx1 * woff;
                        float ly1 = p1->y + dly1 * woff;
                        ME_surface___vset(dst, lx0, ly0, 0.5f, 1);
                        dst++;
                        ME_surface___vset(dst, lx1, ly1, 0.5f, 1);
                        dst++;
                    }
                } else {
                    ME_surface___vset(dst, p1->x + (p1->dmx * woff), p1->y + (p1->dmy * woff), 0.5f, 1);
                    dst++;
                }
                p0 = p1++;
            }
        } else {
            for (j = 0; j < path->count; ++j) {
                ME_surface___vset(dst, pts[j].x, pts[j].y, 0.5f, 1);
                dst++;
            }
        }

        path->nfill = (int)(dst - verts);
        verts = dst;

        // Calculate fringe
        if (fringe) {
            lw = w + woff;
            rw = w - woff;
            lu = 0;
            ru = 1;
            dst = verts;
            path->stroke = dst;

            // Create only half a fringe for convex shapes so that
            // the shape can be rendered without stenciling.
            if (convex) {
                lw = woff;  // This should generate the same vertex as fill inset above.
                lu = 0.5f;  // Set outline fade at middle.
            }

            // Looping
            p0 = &pts[path->count - 1];
            p1 = &pts[0];

            for (j = 0; j < path->count; ++j) {
                if ((p1->flags & (ME_SURFACE_PT_BEVEL | ME_SURFACE_PR_INNERBEVEL)) != 0) {
                    dst = ME_surface___bevelJoin(dst, p0, p1, lw, rw, lu, ru, ctx->fringeWidth);
                } else {
                    ME_surface___vset(dst, p1->x + (p1->dmx * lw), p1->y + (p1->dmy * lw), lu, 1);
                    dst++;
                    ME_surface___vset(dst, p1->x - (p1->dmx * rw), p1->y - (p1->dmy * rw), ru, 1);
                    dst++;
                }
                p0 = p1++;
            }

            // Loop it
            ME_surface___vset(dst, verts[0].x, verts[0].y, lu, 1);
            dst++;
            ME_surface___vset(dst, verts[1].x, verts[1].y, ru, 1);
            dst++;

            path->nstroke = (int)(dst - verts);
            verts = dst;
        } else {
            path->stroke = NULL;
            path->nstroke = 0;
        }
    }

    return 1;
}

// Draw
void ME_surface_BeginPath(MEsurface_context* ctx) {
    ctx->ncommands = 0;
    ME_surface___clearPathCache(ctx);
}

void ME_surface_MoveTo(MEsurface_context* ctx, float x, float y) {
    float vals[] = {ME_SURFACE_MOVETO, x, y};
    ME_surface___appendCommands(ctx, vals, ME_SURFACE_COUNTOF(vals));
}

void ME_surface_LineTo(MEsurface_context* ctx, float x, float y) {
    float vals[] = {ME_SURFACE_LINETO, x, y};
    ME_surface___appendCommands(ctx, vals, ME_SURFACE_COUNTOF(vals));
}

void ME_surface_BezierTo(MEsurface_context* ctx, float c1x, float c1y, float c2x, float c2y, float x, float y) {
    float vals[] = {ME_SURFACE_BEZIERTO, c1x, c1y, c2x, c2y, x, y};
    ME_surface___appendCommands(ctx, vals, ME_SURFACE_COUNTOF(vals));
}

void ME_surface_QuadTo(MEsurface_context* ctx, float cx, float cy, float x, float y) {
    float x0 = ctx->commandx;
    float y0 = ctx->commandy;
    float vals[] = {ME_SURFACE_BEZIERTO, x0 + 2.0f / 3.0f * (cx - x0), y0 + 2.0f / 3.0f * (cy - y0), x + 2.0f / 3.0f * (cx - x), y + 2.0f / 3.0f * (cy - y), x, y};
    ME_surface___appendCommands(ctx, vals, ME_SURFACE_COUNTOF(vals));
}

void ME_surface_ArcTo(MEsurface_context* ctx, float x1, float y1, float x2, float y2, float radius) {
    float x0 = ctx->commandx;
    float y0 = ctx->commandy;
    float dx0, dy0, dx1, dy1, a, d, cx, cy, a0, a1;
    int dir;

    if (ctx->ncommands == 0) {
        return;
    }

    // Handle degenerate cases.
    if (ME_surface___ptEquals(x0, y0, x1, y1, ctx->distTol) || ME_surface___ptEquals(x1, y1, x2, y2, ctx->distTol) || ME_surface___distPtSeg(x1, y1, x0, y0, x2, y2) < ctx->distTol * ctx->distTol ||
        radius < ctx->distTol) {
        ME_surface_LineTo(ctx, x1, y1);
        return;
    }

    // Calculate tangential circle to lines (x0,y0)-(x1,y1) and (x1,y1)-(x2,y2).
    dx0 = x0 - x1;
    dy0 = y0 - y1;
    dx1 = x2 - x1;
    dy1 = y2 - y1;
    ME_surface___normalize(&dx0, &dy0);
    ME_surface___normalize(&dx1, &dy1);
    a = ME_surface___acosf(dx0 * dx1 + dy0 * dy1);
    d = radius / ME_surface___tanf(a / 2.0f);

    //  printf("a=%f° d=%f\n", a/ME_SURFACE_PI*180.0f, d);

    if (d > 10000.0f) {
        ME_surface_LineTo(ctx, x1, y1);
        return;
    }

    if (ME_surface___cross(dx0, dy0, dx1, dy1) > 0.0f) {
        cx = x1 + dx0 * d + dy0 * radius;
        cy = y1 + dy0 * d + -dx0 * radius;
        a0 = ME_surface___atan2f(dx0, -dy0);
        a1 = ME_surface___atan2f(-dx1, dy1);
        dir = ME_SURFACE_CW;
        //      printf("CW c=(%f, %f) a0=%f° a1=%f°\n", cx, cy, a0/ME_SURFACE_PI*180.0f, a1/ME_SURFACE_PI*180.0f);
    } else {
        cx = x1 + dx0 * d + -dy0 * radius;
        cy = y1 + dy0 * d + dx0 * radius;
        a0 = ME_surface___atan2f(-dx0, dy0);
        a1 = ME_surface___atan2f(dx1, -dy1);
        dir = ME_SURFACE_CCW;
        //      printf("CCW c=(%f, %f) a0=%f° a1=%f°\n", cx, cy, a0/ME_SURFACE_PI*180.0f, a1/ME_SURFACE_PI*180.0f);
    }

    ME_surface_Arc(ctx, cx, cy, radius, a0, a1, dir);
}

void ME_surface_ClosePath(MEsurface_context* ctx) {
    float vals[] = {ME_SURFACE_CLOSE};
    ME_surface___appendCommands(ctx, vals, ME_SURFACE_COUNTOF(vals));
}

void ME_surface_PathWinding(MEsurface_context* ctx, int dir) {
    float vals[] = {ME_SURFACE_WINDING, (float)dir};
    ME_surface___appendCommands(ctx, vals, ME_SURFACE_COUNTOF(vals));
}

void ME_surface_Arc(MEsurface_context* ctx, float cx, float cy, float r, float a0, float a1, int dir) {
    float a = 0, da = 0, hda = 0, kappa = 0;
    float dx = 0, dy = 0, x = 0, y = 0, tanx = 0, tany = 0;
    float px = 0, py = 0, ptanx = 0, ptany = 0;
    float vals[3 + 5 * 7 + 100];
    int i, ndivs, nvals;
    int move = ctx->ncommands > 0 ? ME_SURFACE_LINETO : ME_SURFACE_MOVETO;

    // Clamp angles
    da = a1 - a0;
    if (dir == ME_SURFACE_CW) {
        if (ME_surface___absf(da) >= ME_SURFACE_PI * 2) {
            da = ME_SURFACE_PI * 2;
        } else {
            while (da < 0.0f) da += ME_SURFACE_PI * 2;
        }
    } else {
        if (ME_surface___absf(da) >= ME_SURFACE_PI * 2) {
            da = -ME_SURFACE_PI * 2;
        } else {
            while (da > 0.0f) da -= ME_SURFACE_PI * 2;
        }
    }

    // Split arc into max 90 degree segments.
    ndivs = ME_surface___maxi(1, ME_surface___mini((int)(ME_surface___absf(da) / (ME_SURFACE_PI * 0.5f) + 0.5f), 5));
    hda = (da / (float)ndivs) / 2.0f;
    kappa = ME_surface___absf(4.0f / 3.0f * (1.0f - ME_surface___cosf(hda)) / ME_surface___sinf(hda));

    if (dir == ME_SURFACE_CCW) kappa = -kappa;

    nvals = 0;
    for (i = 0; i <= ndivs; i++) {
        a = a0 + da * (i / (float)ndivs);
        dx = ME_surface___cosf(a);
        dy = ME_surface___sinf(a);
        x = cx + dx * r;
        y = cy + dy * r;
        tanx = -dy * r * kappa;
        tany = dx * r * kappa;

        if (i == 0) {
            vals[nvals++] = (float)move;
            vals[nvals++] = x;
            vals[nvals++] = y;
        } else {
            vals[nvals++] = ME_SURFACE_BEZIERTO;
            vals[nvals++] = px + ptanx;
            vals[nvals++] = py + ptany;
            vals[nvals++] = x - tanx;
            vals[nvals++] = y - tany;
            vals[nvals++] = x;
            vals[nvals++] = y;
        }
        px = x;
        py = y;
        ptanx = tanx;
        ptany = tany;
    }

    ME_surface___appendCommands(ctx, vals, nvals);
}

void ME_surface_Rect(MEsurface_context* ctx, float x, float y, float w, float h) {
    float vals[] = {ME_SURFACE_MOVETO, x, y, ME_SURFACE_LINETO, x, y + h, ME_SURFACE_LINETO, x + w, y + h, ME_SURFACE_LINETO, x + w, y, ME_SURFACE_CLOSE};
    ME_surface___appendCommands(ctx, vals, ME_SURFACE_COUNTOF(vals));
}

void ME_surface_RoundedRect(MEsurface_context* ctx, float x, float y, float w, float h, float r) { ME_surface_RoundedRectVarying(ctx, x, y, w, h, r, r, r, r); }

void ME_surface_RoundedRectVarying(MEsurface_context* ctx, float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft) {
    if (radTopLeft < 0.1f && radTopRight < 0.1f && radBottomRight < 0.1f && radBottomLeft < 0.1f) {
        ME_surface_Rect(ctx, x, y, w, h);
        return;
    } else {
        float halfw = ME_surface___absf(w) * 0.5f;
        float halfh = ME_surface___absf(h) * 0.5f;
        float rxBL = ME_surface___minf(radBottomLeft, halfw) * ME_surface___signf(w), ryBL = ME_surface___minf(radBottomLeft, halfh) * ME_surface___signf(h);
        float rxBR = ME_surface___minf(radBottomRight, halfw) * ME_surface___signf(w), ryBR = ME_surface___minf(radBottomRight, halfh) * ME_surface___signf(h);
        float rxTR = ME_surface___minf(radTopRight, halfw) * ME_surface___signf(w), ryTR = ME_surface___minf(radTopRight, halfh) * ME_surface___signf(h);
        float rxTL = ME_surface___minf(radTopLeft, halfw) * ME_surface___signf(w), ryTL = ME_surface___minf(radTopLeft, halfh) * ME_surface___signf(h);
        float vals[] = {ME_SURFACE_MOVETO,
                        x,
                        y + ryTL,
                        ME_SURFACE_LINETO,
                        x,
                        y + h - ryBL,
                        ME_SURFACE_BEZIERTO,
                        x,
                        y + h - ryBL * (1 - ME_SURFACE_KAPPA90),
                        x + rxBL * (1 - ME_SURFACE_KAPPA90),
                        y + h,
                        x + rxBL,
                        y + h,
                        ME_SURFACE_LINETO,
                        x + w - rxBR,
                        y + h,
                        ME_SURFACE_BEZIERTO,
                        x + w - rxBR * (1 - ME_SURFACE_KAPPA90),
                        y + h,
                        x + w,
                        y + h - ryBR * (1 - ME_SURFACE_KAPPA90),
                        x + w,
                        y + h - ryBR,
                        ME_SURFACE_LINETO,
                        x + w,
                        y + ryTR,
                        ME_SURFACE_BEZIERTO,
                        x + w,
                        y + ryTR * (1 - ME_SURFACE_KAPPA90),
                        x + w - rxTR * (1 - ME_SURFACE_KAPPA90),
                        y,
                        x + w - rxTR,
                        y,
                        ME_SURFACE_LINETO,
                        x + rxTL,
                        y,
                        ME_SURFACE_BEZIERTO,
                        x + rxTL * (1 - ME_SURFACE_KAPPA90),
                        y,
                        x,
                        y + ryTL * (1 - ME_SURFACE_KAPPA90),
                        x,
                        y + ryTL,
                        ME_SURFACE_CLOSE};
        ME_surface___appendCommands(ctx, vals, ME_SURFACE_COUNTOF(vals));
    }
}

void ME_surface_Ellipse(MEsurface_context* ctx, float cx, float cy, float rx, float ry) {
    float vals[] = {ME_SURFACE_MOVETO,
                    cx - rx,
                    cy,
                    ME_SURFACE_BEZIERTO,
                    cx - rx,
                    cy + ry * ME_SURFACE_KAPPA90,
                    cx - rx * ME_SURFACE_KAPPA90,
                    cy + ry,
                    cx,
                    cy + ry,
                    ME_SURFACE_BEZIERTO,
                    cx + rx * ME_SURFACE_KAPPA90,
                    cy + ry,
                    cx + rx,
                    cy + ry * ME_SURFACE_KAPPA90,
                    cx + rx,
                    cy,
                    ME_SURFACE_BEZIERTO,
                    cx + rx,
                    cy - ry * ME_SURFACE_KAPPA90,
                    cx + rx * ME_SURFACE_KAPPA90,
                    cy - ry,
                    cx,
                    cy - ry,
                    ME_SURFACE_BEZIERTO,
                    cx - rx * ME_SURFACE_KAPPA90,
                    cy - ry,
                    cx - rx,
                    cy - ry * ME_SURFACE_KAPPA90,
                    cx - rx,
                    cy,
                    ME_SURFACE_CLOSE};
    ME_surface___appendCommands(ctx, vals, ME_SURFACE_COUNTOF(vals));
}

void ME_surface_Circle(MEsurface_context* ctx, float cx, float cy, float r) { ME_surface_Ellipse(ctx, cx, cy, r, r); }

void ME_surface_DebugDumpPathCache(MEsurface_context* ctx) {
    const MEsurface_path* path;
    int i, j;

    printf("Dumping %d cached paths\n", ctx->cache->npaths);
    for (i = 0; i < ctx->cache->npaths; i++) {
        path = &ctx->cache->paths[i];
        printf(" - Path %d\n", i);
        if (path->nfill) {
            printf("   - fill: %d\n", path->nfill);
            for (j = 0; j < path->nfill; j++) printf("%f\t%f\n", path->fill[j].x, path->fill[j].y);
        }
        if (path->nstroke) {
            printf("   - stroke: %d\n", path->nstroke);
            for (j = 0; j < path->nstroke; j++) printf("%f\t%f\n", path->stroke[j].x, path->stroke[j].y);
        }
    }
}

void ME_surface_Fill(MEsurface_context* ctx) {
    MEsurface_state* state = ME_surface___getState(ctx);
    const MEsurface_path* path;
    MEsurface_paint fillPaint = state->fill;
    int i;

    ME_surface___flattenPaths(ctx);
    if (ctx->params.edgeAntiAlias && state->shapeAntiAlias)
        ME_surface___expandFill(ctx, ctx->fringeWidth, ME_SURFACE_MITER, 2.4f);
    else
        ME_surface___expandFill(ctx, 0.0f, ME_SURFACE_MITER, 2.4f);

    // Apply global alpha
    fillPaint.innerColor.a *= state->alpha;
    fillPaint.outerColor.a *= state->alpha;

    ctx->params.renderFill(ctx->params.userPtr, &fillPaint, state->compositeOperation, &state->scissor, ctx->fringeWidth, ctx->cache->bounds, ctx->cache->paths, ctx->cache->npaths);

    // Count triangles
    for (i = 0; i < ctx->cache->npaths; i++) {
        path = &ctx->cache->paths[i];
        ctx->fillTriCount += path->nfill - 2;
        ctx->fillTriCount += path->nstroke - 2;
        ctx->drawCallCount += 2;
    }
}

void ME_surface_Stroke(MEsurface_context* ctx) {
    MEsurface_state* state = ME_surface___getState(ctx);
    float scale = ME_surface___getAverageScale(state->xform);
    float strokeWidth = ME_surface___clampf(state->strokeWidth * scale, 0.0f, 200.0f);
    MEsurface_paint strokePaint = state->stroke;
    const MEsurface_path* path;
    int i;

    if (strokeWidth < ctx->fringeWidth) {
        // If the stroke width is less than pixel size, use alpha to emulate coverage.
        // Since coverage is area, scale by alpha*alpha.
        float alpha = ME_surface___clampf(strokeWidth / ctx->fringeWidth, 0.0f, 1.0f);
        strokePaint.innerColor.a *= alpha * alpha;
        strokePaint.outerColor.a *= alpha * alpha;
        strokeWidth = ctx->fringeWidth;
    }

    // Apply global alpha
    strokePaint.innerColor.a *= state->alpha;
    strokePaint.outerColor.a *= state->alpha;

    ME_surface___flattenPaths(ctx);

    if (ctx->params.edgeAntiAlias && state->shapeAntiAlias)
        ME_surface___expandStroke(ctx, strokeWidth * 0.5f, ctx->fringeWidth, state->lineCap, state->lineJoin, state->miterLimit);
    else
        ME_surface___expandStroke(ctx, strokeWidth * 0.5f, 0.0f, state->lineCap, state->lineJoin, state->miterLimit);

    ctx->params.renderStroke(ctx->params.userPtr, &strokePaint, state->compositeOperation, &state->scissor, ctx->fringeWidth, strokeWidth, ctx->cache->paths, ctx->cache->npaths);

    // Count triangles
    for (i = 0; i < ctx->cache->npaths; i++) {
        path = &ctx->cache->paths[i];
        ctx->strokeTriCount += path->nstroke - 2;
        ctx->drawCallCount++;
    }
}

// Add fonts
int ME_surface_CreateFont(MEsurface_context* ctx, const char* name, const char* filename) { return fonsAddFont(ctx->fs, name, filename, 0); }

int ME_surface_CreateFontAtIndex(MEsurface_context* ctx, const char* name, const char* filename, const int fontIndex) { return fonsAddFont(ctx->fs, name, filename, fontIndex); }

int ME_surface_CreateFontMem(MEsurface_context* ctx, const char* name, unsigned char* data, int ndata, int freeData) { return fonsAddFontMem(ctx->fs, name, data, ndata, freeData, 0); }

int ME_surface_CreateFontMemAtIndex(MEsurface_context* ctx, const char* name, unsigned char* data, int ndata, int freeData, const int fontIndex) {
    return fonsAddFontMem(ctx->fs, name, data, ndata, freeData, fontIndex);
}

int ME_surface_FindFont(MEsurface_context* ctx, const char* name) {
    if (name == NULL) return -1;
    return fonsGetFontByName(ctx->fs, name);
}

int ME_surface_AddFallbackFontId(MEsurface_context* ctx, int baseFont, int fallbackFont) {
    if (baseFont == -1 || fallbackFont == -1) return 0;
    return fonsAddFallbackFont(ctx->fs, baseFont, fallbackFont);
}

int ME_surface_AddFallbackFont(MEsurface_context* ctx, const char* baseFont, const char* fallbackFont) {
    return ME_surface_AddFallbackFontId(ctx, ME_surface_FindFont(ctx, baseFont), ME_surface_FindFont(ctx, fallbackFont));
}

void ME_surface_ResetFallbackFontsId(MEsurface_context* ctx, int baseFont) { fonsResetFallbackFont(ctx->fs, baseFont); }

void ME_surface_ResetFallbackFonts(MEsurface_context* ctx, const char* baseFont) { ME_surface_ResetFallbackFontsId(ctx, ME_surface_FindFont(ctx, baseFont)); }

// State setting
void ME_surface_FontSize(MEsurface_context* ctx, float size) {
    MEsurface_state* state = ME_surface___getState(ctx);
    state->fontSize = size;
}

void ME_surface_FontBlur(MEsurface_context* ctx, float blur) {
    MEsurface_state* state = ME_surface___getState(ctx);
    state->fontBlur = blur;
}

void ME_surface_TextLetterSpacing(MEsurface_context* ctx, float spacing) {
    MEsurface_state* state = ME_surface___getState(ctx);
    state->letterSpacing = spacing;
}

void ME_surface_TextLineHeight(MEsurface_context* ctx, float lineHeight) {
    MEsurface_state* state = ME_surface___getState(ctx);
    state->lineHeight = lineHeight;
}

void ME_surface_TextAlign(MEsurface_context* ctx, int align) {
    MEsurface_state* state = ME_surface___getState(ctx);
    state->textAlign = align;
}

void ME_surface_FontFaceId(MEsurface_context* ctx, int font) {
    MEsurface_state* state = ME_surface___getState(ctx);
    state->fontId = font;
}

void ME_surface_FontFace(MEsurface_context* ctx, const char* font) {
    MEsurface_state* state = ME_surface___getState(ctx);
    state->fontId = fonsGetFontByName(ctx->fs, font);
}

static float ME_surface___quantize(float a, float d) { return ((int)(a / d + 0.5f)) * d; }

static float ME_surface___getFontScale(MEsurface_state* state) { return ME_surface___minf(ME_surface___quantize(ME_surface___getAverageScale(state->xform), 0.01f), 4.0f); }

static void ME_surface___flushTextTexture(MEsurface_context* ctx) {
    int dirty[4];

    if (fonsValidateTexture(ctx->fs, dirty)) {
        int fontImage = ctx->fontImages[ctx->fontImageIdx];
        // Update texture
        if (fontImage != 0) {
            int iw, ih;
            const unsigned char* data = fonsGetTextureData(ctx->fs, &iw, &ih);
            int x = dirty[0];
            int y = dirty[1];
            int w = dirty[2] - dirty[0];
            int h = dirty[3] - dirty[1];
            ctx->params.renderUpdateTexture(ctx->params.userPtr, fontImage, x, y, w, h, data);
        }
    }
}

static int ME_surface___allocTextAtlas(MEsurface_context* ctx) {
    int iw, ih;
    ME_surface___flushTextTexture(ctx);
    if (ctx->fontImageIdx >= ME_SURFACE_MAX_FONTIMAGES - 1) return 0;
    // if next fontImage already have a texture
    if (ctx->fontImages[ctx->fontImageIdx + 1] != 0)
        ME_surface_ImageSize(ctx, ctx->fontImages[ctx->fontImageIdx + 1], &iw, &ih);
    else {  // calculate the new font image size and create it.
        ME_surface_ImageSize(ctx, ctx->fontImages[ctx->fontImageIdx], &iw, &ih);
        if (iw > ih)
            ih *= 2;
        else
            iw *= 2;
        if (iw > ME_SURFACE_MAX_FONTIMAGE_SIZE || ih > ME_SURFACE_MAX_FONTIMAGE_SIZE) iw = ih = ME_SURFACE_MAX_FONTIMAGE_SIZE;
        ctx->fontImages[ctx->fontImageIdx + 1] = ctx->params.renderCreateTexture(ctx->params.userPtr, ME_SURFACE_TEXTURE_ALPHA, iw, ih, 0, NULL);
    }
    ++ctx->fontImageIdx;
    fonsResetAtlas(ctx->fs, iw, ih);
    return 1;
}

static void ME_surface___renderText(MEsurface_context* ctx, MEsurface_vertex* verts, int nverts) {
    MEsurface_state* state = ME_surface___getState(ctx);
    MEsurface_paint paint = state->fill;

    // ENGINE()-> triangles.
    paint.image = ctx->fontImages[ctx->fontImageIdx];

    // Apply global alpha
    paint.innerColor.a *= state->alpha;
    paint.outerColor.a *= state->alpha;

    ctx->params.renderTriangles(ctx->params.userPtr, &paint, state->compositeOperation, &state->scissor, verts, nverts, ctx->fringeWidth);

    ctx->drawCallCount++;
    ctx->textTriCount += nverts / 3;
}

static int ME_surface___isTransformFlipped(const float* xform) {
    float det = xform[0] * xform[3] - xform[2] * xform[1];
    return (det < 0);
}

float ME_surface_Text(MEsurface_context* ctx, float x, float y, const char* string, const char* end) {
    MEsurface_state* state = ME_surface___getState(ctx);
    FONStextIter iter, prevIter;
    FONSquad q;
    MEsurface_vertex* verts;
    float scale = ME_surface___getFontScale(state) * ctx->devicePxRatio;
    float invscale = 1.0f / scale;
    int cverts = 0;
    int nverts = 0;
    int isFlipped = ME_surface___isTransformFlipped(state->xform);

    if (end == NULL) end = string + strlen(string);

    if (state->fontId == FONS_INVALID) return x;

    fonsSetSize(ctx->fs, state->fontSize * scale);
    fonsSetSpacing(ctx->fs, state->letterSpacing * scale);
    fonsSetBlur(ctx->fs, state->fontBlur * scale);
    fonsSetAlign(ctx->fs, state->textAlign);
    fonsSetFont(ctx->fs, state->fontId);

    cverts = ME_surface___maxi(2, (int)(end - string)) * 6;  // conservative estimate.
    verts = ME_surface___allocTempVerts(ctx, cverts);
    if (verts == NULL) return x;

    fonsTextIterInit(ctx->fs, &iter, x * scale, y * scale, string, end, FONS_GLYPH_BITMAP_REQUIRED);
    prevIter = iter;
    while (fonsTextIterNext(ctx->fs, &iter, &q)) {
        float c[4 * 2];
        if (iter.prevGlyphIndex == -1) {  // can not retrieve glyph?
            if (nverts != 0) {
                ME_surface___renderText(ctx, verts, nverts);
                nverts = 0;
            }
            if (!ME_surface___allocTextAtlas(ctx)) break;  // no memory :(
            iter = prevIter;
            fonsTextIterNext(ctx->fs, &iter, &q);  // try again
            if (iter.prevGlyphIndex == -1)         // still can not find glyph?
                break;
        }
        prevIter = iter;
        if (isFlipped) {
            float tmp;

            tmp = q.y0;
            q.y0 = q.y1;
            q.y1 = tmp;
            tmp = q.t0;
            q.t0 = q.t1;
            q.t1 = tmp;
        }
        // Transform corners.
        ME_surface_TransformPoint(&c[0], &c[1], state->xform, q.x0 * invscale, q.y0 * invscale);
        ME_surface_TransformPoint(&c[2], &c[3], state->xform, q.x1 * invscale, q.y0 * invscale);
        ME_surface_TransformPoint(&c[4], &c[5], state->xform, q.x1 * invscale, q.y1 * invscale);
        ME_surface_TransformPoint(&c[6], &c[7], state->xform, q.x0 * invscale, q.y1 * invscale);
        // Create triangles
        if (nverts + 6 <= cverts) {
            ME_surface___vset(&verts[nverts], c[0], c[1], q.s0, q.t0);
            nverts++;
            ME_surface___vset(&verts[nverts], c[4], c[5], q.s1, q.t1);
            nverts++;
            ME_surface___vset(&verts[nverts], c[2], c[3], q.s1, q.t0);
            nverts++;
            ME_surface___vset(&verts[nverts], c[0], c[1], q.s0, q.t0);
            nverts++;
            ME_surface___vset(&verts[nverts], c[6], c[7], q.s0, q.t1);
            nverts++;
            ME_surface___vset(&verts[nverts], c[4], c[5], q.s1, q.t1);
            nverts++;
        }
    }

    // TODO: add back-end bit to do this just once per frame.
    ME_surface___flushTextTexture(ctx);

    ME_surface___renderText(ctx, verts, nverts);

    return iter.nextx / scale;
}

void ME_surface_TextBox(MEsurface_context* ctx, float x, float y, float breakRowWidth, const char* string, const char* end) {
    MEsurface_state* state = ME_surface___getState(ctx);
    MEsurface_textRow rows[2];
    int nrows = 0, i;
    int oldAlign = state->textAlign;
    int haling = state->textAlign & (ME_SURFACE_ALIGN_LEFT | ME_SURFACE_ALIGN_CENTER | ME_SURFACE_ALIGN_RIGHT);
    int valign = state->textAlign & (ME_SURFACE_ALIGN_TOP | ME_SURFACE_ALIGN_MIDDLE | ME_SURFACE_ALIGN_BOTTOM | ME_SURFACE_ALIGN_BASELINE);
    float lineh = 0;

    if (state->fontId == FONS_INVALID) return;

    ME_surface_TextMetrics(ctx, NULL, NULL, &lineh);

    state->textAlign = ME_SURFACE_ALIGN_LEFT | valign;

    while ((nrows = ME_surface_TextBreakLines(ctx, string, end, breakRowWidth, rows, 2))) {
        for (i = 0; i < nrows; i++) {
            MEsurface_textRow* row = &rows[i];
            if (haling & ME_SURFACE_ALIGN_LEFT)
                ME_surface_Text(ctx, x, y, row->start, row->end);
            else if (haling & ME_SURFACE_ALIGN_CENTER)
                ME_surface_Text(ctx, x + breakRowWidth * 0.5f - row->width * 0.5f, y, row->start, row->end);
            else if (haling & ME_SURFACE_ALIGN_RIGHT)
                ME_surface_Text(ctx, x + breakRowWidth - row->width, y, row->start, row->end);
            y += lineh * state->lineHeight;
        }
        string = rows[nrows - 1].next;
    }

    state->textAlign = oldAlign;
}

int ME_surface_TextGlyphPositions(MEsurface_context* ctx, float x, float y, const char* string, const char* end, MEsurface_glyphPosition* positions, int maxPositions) {
    MEsurface_state* state = ME_surface___getState(ctx);
    float scale = ME_surface___getFontScale(state) * ctx->devicePxRatio;
    float invscale = 1.0f / scale;
    FONStextIter iter, prevIter;
    FONSquad q;
    int npos = 0;

    if (state->fontId == FONS_INVALID) return 0;

    if (end == NULL) end = string + strlen(string);

    if (string == end) return 0;

    fonsSetSize(ctx->fs, state->fontSize * scale);
    fonsSetSpacing(ctx->fs, state->letterSpacing * scale);
    fonsSetBlur(ctx->fs, state->fontBlur * scale);
    fonsSetAlign(ctx->fs, state->textAlign);
    fonsSetFont(ctx->fs, state->fontId);

    fonsTextIterInit(ctx->fs, &iter, x * scale, y * scale, string, end, FONS_GLYPH_BITMAP_OPTIONAL);
    prevIter = iter;
    while (fonsTextIterNext(ctx->fs, &iter, &q)) {
        if (iter.prevGlyphIndex < 0 && ME_surface___allocTextAtlas(ctx)) {  // can not retrieve glyph?
            iter = prevIter;
            fonsTextIterNext(ctx->fs, &iter, &q);  // try again
        }
        prevIter = iter;
        positions[npos].str = iter.str;
        positions[npos].x = iter.x * invscale;
        positions[npos].minx = ME_surface___minf(iter.x, q.x0) * invscale;
        positions[npos].maxx = ME_surface___maxf(iter.nextx, q.x1) * invscale;
        npos++;
        if (npos >= maxPositions) break;
    }

    return npos;
}

enum MEsurface_codepointType {
    ME_SURFACE_SPACE,
    ME_SURFACE_NEWLINE,
    ME_SURFACE_CHAR,
    ME_SURFACE_CJK_CHAR,
};

int ME_surface_TextBreakLines(MEsurface_context* ctx, const char* string, const char* end, float breakRowWidth, MEsurface_textRow* rows, int maxRows) {
    MEsurface_state* state = ME_surface___getState(ctx);
    float scale = ME_surface___getFontScale(state) * ctx->devicePxRatio;
    float invscale = 1.0f / scale;
    FONStextIter iter, prevIter;
    FONSquad q;
    int nrows = 0;
    float rowStartX = 0;
    float rowWidth = 0;
    float rowMinX = 0;
    float rowMaxX = 0;
    const char* rowStart = NULL;
    const char* rowEnd = NULL;
    const char* wordStart = NULL;
    float wordStartX = 0;
    float wordMinX = 0;
    const char* breakEnd = NULL;
    float breakWidth = 0;
    float breakMaxX = 0;
    int type = ME_SURFACE_SPACE, ptype = ME_SURFACE_SPACE;
    unsigned int pcodepoint = 0;

    if (maxRows == 0) return 0;
    if (state->fontId == FONS_INVALID) return 0;

    if (end == NULL) end = string + strlen(string);

    if (string == end) return 0;

    fonsSetSize(ctx->fs, state->fontSize * scale);
    fonsSetSpacing(ctx->fs, state->letterSpacing * scale);
    fonsSetBlur(ctx->fs, state->fontBlur * scale);
    fonsSetAlign(ctx->fs, state->textAlign);
    fonsSetFont(ctx->fs, state->fontId);

    breakRowWidth *= scale;

    fonsTextIterInit(ctx->fs, &iter, 0, 0, string, end, FONS_GLYPH_BITMAP_OPTIONAL);
    prevIter = iter;
    while (fonsTextIterNext(ctx->fs, &iter, &q)) {
        if (iter.prevGlyphIndex < 0 && ME_surface___allocTextAtlas(ctx)) {  // can not retrieve glyph?
            iter = prevIter;
            fonsTextIterNext(ctx->fs, &iter, &q);  // try again
        }
        prevIter = iter;
        switch (iter.codepoint) {
            case 9:       // \t
            case 11:      // \v
            case 12:      // \f
            case 32:      // space
            case 0x00a0:  // NBSP
                type = ME_SURFACE_SPACE;
                break;
            case 10:  // \n
                type = pcodepoint == 13 ? ME_SURFACE_SPACE : ME_SURFACE_NEWLINE;
                break;
            case 13:  // \r
                type = pcodepoint == 10 ? ME_SURFACE_SPACE : ME_SURFACE_NEWLINE;
                break;
            case 0x0085:  // NEL
                type = ME_SURFACE_NEWLINE;
                break;
            default:
                if ((iter.codepoint >= 0x4E00 && iter.codepoint <= 0x9FFF) || (iter.codepoint >= 0x3000 && iter.codepoint <= 0x30FF) || (iter.codepoint >= 0xFF00 && iter.codepoint <= 0xFFEF) ||
                    (iter.codepoint >= 0x1100 && iter.codepoint <= 0x11FF) || (iter.codepoint >= 0x3130 && iter.codepoint <= 0x318F) || (iter.codepoint >= 0xAC00 && iter.codepoint <= 0xD7AF))
                    type = ME_SURFACE_CJK_CHAR;
                else
                    type = ME_SURFACE_CHAR;
                break;
        }

        if (type == ME_SURFACE_NEWLINE) {
            // Always handle new lines.
            rows[nrows].start = rowStart != NULL ? rowStart : iter.str;
            rows[nrows].end = rowEnd != NULL ? rowEnd : iter.str;
            rows[nrows].width = rowWidth * invscale;
            rows[nrows].minx = rowMinX * invscale;
            rows[nrows].maxx = rowMaxX * invscale;
            rows[nrows].next = iter.next;
            nrows++;
            if (nrows >= maxRows) return nrows;
            // Set null break point
            breakEnd = rowStart;
            breakWidth = 0.0;
            breakMaxX = 0.0;
            // Indicate to skip the white space at the beginning of the row.
            rowStart = NULL;
            rowEnd = NULL;
            rowWidth = 0;
            rowMinX = rowMaxX = 0;
        } else {
            if (rowStart == NULL) {
                // Skip white space until the beginning of the line
                if (type == ME_SURFACE_CHAR || type == ME_SURFACE_CJK_CHAR) {
                    // The current char is the row so far
                    rowStartX = iter.x;
                    rowStart = iter.str;
                    rowEnd = iter.next;
                    rowWidth = iter.nextx - rowStartX;
                    rowMinX = q.x0 - rowStartX;
                    rowMaxX = q.x1 - rowStartX;
                    wordStart = iter.str;
                    wordStartX = iter.x;
                    wordMinX = q.x0 - rowStartX;
                    // Set null break point
                    breakEnd = rowStart;
                    breakWidth = 0.0;
                    breakMaxX = 0.0;
                }
            } else {
                float nextWidth = iter.nextx - rowStartX;

                // track last non-white space character
                if (type == ME_SURFACE_CHAR || type == ME_SURFACE_CJK_CHAR) {
                    rowEnd = iter.next;
                    rowWidth = iter.nextx - rowStartX;
                    rowMaxX = q.x1 - rowStartX;
                }
                // track last end of a word
                if (((ptype == ME_SURFACE_CHAR || ptype == ME_SURFACE_CJK_CHAR) && type == ME_SURFACE_SPACE) || type == ME_SURFACE_CJK_CHAR) {
                    breakEnd = iter.str;
                    breakWidth = rowWidth;
                    breakMaxX = rowMaxX;
                }
                // track last beginning of a word
                if ((ptype == ME_SURFACE_SPACE && (type == ME_SURFACE_CHAR || type == ME_SURFACE_CJK_CHAR)) || type == ME_SURFACE_CJK_CHAR) {
                    wordStart = iter.str;
                    wordStartX = iter.x;
                    wordMinX = q.x0;
                }

                // Break to new line when a character is beyond break width.
                if ((type == ME_SURFACE_CHAR || type == ME_SURFACE_CJK_CHAR) && nextWidth > breakRowWidth) {
                    // The run length is too long, need to break to new line.
                    if (breakEnd == rowStart) {
                        // The current word is longer than the row length, just break it from here.
                        rows[nrows].start = rowStart;
                        rows[nrows].end = iter.str;
                        rows[nrows].width = rowWidth * invscale;
                        rows[nrows].minx = rowMinX * invscale;
                        rows[nrows].maxx = rowMaxX * invscale;
                        rows[nrows].next = iter.str;
                        nrows++;
                        if (nrows >= maxRows) return nrows;
                        rowStartX = iter.x;
                        rowStart = iter.str;
                        rowEnd = iter.next;
                        rowWidth = iter.nextx - rowStartX;
                        rowMinX = q.x0 - rowStartX;
                        rowMaxX = q.x1 - rowStartX;
                        wordStart = iter.str;
                        wordStartX = iter.x;
                        wordMinX = q.x0 - rowStartX;
                    } else {
                        // Break the line from the end of the last word, and start new line from the beginning of the new.
                        rows[nrows].start = rowStart;
                        rows[nrows].end = breakEnd;
                        rows[nrows].width = breakWidth * invscale;
                        rows[nrows].minx = rowMinX * invscale;
                        rows[nrows].maxx = breakMaxX * invscale;
                        rows[nrows].next = wordStart;
                        nrows++;
                        if (nrows >= maxRows) return nrows;
                        // Update row
                        rowStartX = wordStartX;
                        rowStart = wordStart;
                        rowEnd = iter.next;
                        rowWidth = iter.nextx - rowStartX;
                        rowMinX = wordMinX - rowStartX;
                        rowMaxX = q.x1 - rowStartX;
                    }
                    // Set null break point
                    breakEnd = rowStart;
                    breakWidth = 0.0;
                    breakMaxX = 0.0;
                }
            }
        }

        pcodepoint = iter.codepoint;
        ptype = type;
    }

    // Break the line from the end of the last word, and start new line from the beginning of the new.
    if (rowStart != NULL) {
        rows[nrows].start = rowStart;
        rows[nrows].end = rowEnd;
        rows[nrows].width = rowWidth * invscale;
        rows[nrows].minx = rowMinX * invscale;
        rows[nrows].maxx = rowMaxX * invscale;
        rows[nrows].next = end;
        nrows++;
    }

    return nrows;
}

float ME_surface_TextBounds(MEsurface_context* ctx, float x, float y, const char* string, const char* end, float* bounds) {
    MEsurface_state* state = ME_surface___getState(ctx);
    float scale = ME_surface___getFontScale(state) * ctx->devicePxRatio;
    float invscale = 1.0f / scale;
    float width;

    if (state->fontId == FONS_INVALID) return 0;

    fonsSetSize(ctx->fs, state->fontSize * scale);
    fonsSetSpacing(ctx->fs, state->letterSpacing * scale);
    fonsSetBlur(ctx->fs, state->fontBlur * scale);
    fonsSetAlign(ctx->fs, state->textAlign);
    fonsSetFont(ctx->fs, state->fontId);

    width = fonsTextBounds(ctx->fs, x * scale, y * scale, string, end, bounds);
    if (bounds != NULL) {
        // Use line bounds for height.
        fonsLineBounds(ctx->fs, y * scale, &bounds[1], &bounds[3]);
        bounds[0] *= invscale;
        bounds[1] *= invscale;
        bounds[2] *= invscale;
        bounds[3] *= invscale;
    }
    return width * invscale;
}

void ME_surface_TextBoxBounds(MEsurface_context* ctx, float x, float y, float breakRowWidth, const char* string, const char* end, float* bounds) {
    MEsurface_state* state = ME_surface___getState(ctx);
    MEsurface_textRow rows[2];
    float scale = ME_surface___getFontScale(state) * ctx->devicePxRatio;
    float invscale = 1.0f / scale;
    int nrows = 0, i;
    int oldAlign = state->textAlign;
    int haling = state->textAlign & (ME_SURFACE_ALIGN_LEFT | ME_SURFACE_ALIGN_CENTER | ME_SURFACE_ALIGN_RIGHT);
    int valign = state->textAlign & (ME_SURFACE_ALIGN_TOP | ME_SURFACE_ALIGN_MIDDLE | ME_SURFACE_ALIGN_BOTTOM | ME_SURFACE_ALIGN_BASELINE);
    float lineh = 0, rminy = 0, rmaxy = 0;
    float minx, miny, maxx, maxy;

    if (state->fontId == FONS_INVALID) {
        if (bounds != NULL) bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0f;
        return;
    }

    ME_surface_TextMetrics(ctx, NULL, NULL, &lineh);

    state->textAlign = ME_SURFACE_ALIGN_LEFT | valign;

    minx = maxx = x;
    miny = maxy = y;

    fonsSetSize(ctx->fs, state->fontSize * scale);
    fonsSetSpacing(ctx->fs, state->letterSpacing * scale);
    fonsSetBlur(ctx->fs, state->fontBlur * scale);
    fonsSetAlign(ctx->fs, state->textAlign);
    fonsSetFont(ctx->fs, state->fontId);
    fonsLineBounds(ctx->fs, 0, &rminy, &rmaxy);
    rminy *= invscale;
    rmaxy *= invscale;

    while ((nrows = ME_surface_TextBreakLines(ctx, string, end, breakRowWidth, rows, 2))) {
        for (i = 0; i < nrows; i++) {
            MEsurface_textRow* row = &rows[i];
            float rminx, rmaxx, dx = 0;
            // Horizontal bounds
            if (haling & ME_SURFACE_ALIGN_LEFT)
                dx = 0;
            else if (haling & ME_SURFACE_ALIGN_CENTER)
                dx = breakRowWidth * 0.5f - row->width * 0.5f;
            else if (haling & ME_SURFACE_ALIGN_RIGHT)
                dx = breakRowWidth - row->width;
            rminx = x + row->minx + dx;
            rmaxx = x + row->maxx + dx;
            minx = ME_surface___minf(minx, rminx);
            maxx = ME_surface___maxf(maxx, rmaxx);
            // Vertical bounds.
            miny = ME_surface___minf(miny, y + rminy);
            maxy = ME_surface___maxf(maxy, y + rmaxy);

            y += lineh * state->lineHeight;
        }
        string = rows[nrows - 1].next;
    }

    state->textAlign = oldAlign;

    if (bounds != NULL) {
        bounds[0] = minx;
        bounds[1] = miny;
        bounds[2] = maxx;
        bounds[3] = maxy;
    }
}

void ME_surface_TextMetrics(MEsurface_context* ctx, float* ascender, float* descender, float* lineh) {
    MEsurface_state* state = ME_surface___getState(ctx);
    float scale = ME_surface___getFontScale(state) * ctx->devicePxRatio;
    float invscale = 1.0f / scale;

    if (state->fontId == FONS_INVALID) return;

    fonsSetSize(ctx->fs, state->fontSize * scale);
    fonsSetSpacing(ctx->fs, state->letterSpacing * scale);
    fonsSetBlur(ctx->fs, state->fontBlur * scale);
    fonsSetAlign(ctx->fs, state->textAlign);
    fonsSetFont(ctx->fs, state->fontId);

    fonsVertMetrics(ctx->fs, ascender, descender, lineh);
    if (ascender != NULL) *ascender *= invscale;
    if (descender != NULL) *descender *= invscale;
    if (lineh != NULL) *lineh *= invscale;
}
// vim: ft=c nu noet ts=4
