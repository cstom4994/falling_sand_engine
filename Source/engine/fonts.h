
#ifndef FONS_H
#define FONS_H

#ifdef __cplusplus
extern "C" {
#endif

#define FONS_INVALID -1

enum FONSflags {
    FONS_ZERO_TOPLEFT = 1,
    FONS_ZERO_BOTTOMLEFT = 2,
};

enum FONSalign {
    // Horizontal align
    FONS_ALIGN_LEFT = 1 << 0,  // Default
    FONS_ALIGN_CENTER = 1 << 1,
    FONS_ALIGN_RIGHT = 1 << 2,
    // Vertical align
    FONS_ALIGN_TOP = 1 << 3,
    FONS_ALIGN_MIDDLE = 1 << 4,
    FONS_ALIGN_BOTTOM = 1 << 5,
    FONS_ALIGN_BASELINE = 1 << 6,  // Default
};

enum FONSerrorCode {
    // Font atlas is full.
    FONS_ATLAS_FULL = 1,
    // Scratch memory used to render glyphs is full, requested size reported in 'val', you may need to bump up FONS_SCRATCH_BUF_SIZE.
    FONS_SCRATCH_FULL = 2,
    // Calls to fonsPushState has created too large stack, if you need deep state stack bump up FONS_MAX_STATES.
    FONS_STATES_OVERFLOW = 3,
    // Trying to pop too many states fonsPopState().
    FONS_STATES_UNDERFLOW = 4,
};

struct FONSparams {
    int width, height;
    unsigned char flags;
    void* userPtr;
    int (*renderCreate)(void* uptr, int width, int height);
    int (*renderResize)(void* uptr, int width, int height);
    void (*renderUpdate)(void* uptr, int* rect, const unsigned char* data);
    void (*renderDraw)(void* uptr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts);
    void (*renderDelete)(void* uptr);
};
typedef struct FONSparams FONSparams;

struct FONSquad {
    float x0, y0, s0, t0;
    float x1, y1, s1, t1;
};
typedef struct FONSquad FONSquad;

struct FONStextIter {
    float x, y, nextx, nexty, scale, spacing;
    unsigned int codepoint;
    short isize, iblur;
    struct FONSfont* font;
    int prevGlyphIndex;
    const char* str;
    const char* next;
    const char* end;
    unsigned int utf8state;
};
typedef struct FONStextIter FONStextIter;

typedef struct FONScontext FONScontext;

// Contructor and destructor.
FONScontext* fonsCreateInternal(FONSparams* params);
void fonsDeleteInternal(FONScontext* s);

void fonsSetErrorCallback(FONScontext* s, void (*callback)(void* uptr, int error, int val), void* uptr);
// Returns current atlas size.
void fonsGetAtlasSize(FONScontext* s, int* width, int* height);
// Expands the atlas size.
int fonsExpandAtlas(FONScontext* s, int width, int height);
// Resets the whole stash.
int fonsResetAtlas(FONScontext* stash, int width, int height);

// Add fonts
int fonsAddFont(FONScontext* s, const char* name, const char* path);
int fonsAddFontMem(FONScontext* s, const char* name, unsigned char* data, int ndata, int freeData);
int fonsGetFontByName(FONScontext* s, const char* name);
int fonsAddFallbackFont(FONScontext* stash, int base, int fallback);

// State handling
void fonsPushState(FONScontext* s);
void fonsPopState(FONScontext* s);
void fonsClearState(FONScontext* s);

// State setting
void fonsSetSize(FONScontext* s, float size);
void fonsSetColor(FONScontext* s, unsigned int color);
void fonsSetSpacing(FONScontext* s, float spacing);
void fonsSetBlur(FONScontext* s, float blur);
void fonsSetAlign(FONScontext* s, int align);
void fonsSetFont(FONScontext* s, int font);

// Draw text
float fonsDrawText(FONScontext* s, float x, float y, const char* string, const char* end);

// Measure text
float fonsTextBounds(FONScontext* s, float x, float y, const char* string, const char* end, float* bounds);
void fonsLineBounds(FONScontext* s, float y, float* miny, float* maxy);
void fonsVertMetrics(FONScontext* s, float* ascender, float* descender, float* lineh);

// Text iterator
int fonsTextIterInit(FONScontext* stash, FONStextIter* iter, float x, float y, const char* str, const char* end);
int fonsTextIterNext(FONScontext* stash, FONStextIter* iter, struct FONSquad* quad);

// Pull texture changes
const unsigned char* fonsGetTextureData(FONScontext* stash, int* width, int* height);
int fonsValidateTexture(FONScontext* s, int* dirty);

// Draws the stash texture for debugging
void fonsDrawDebug(FONScontext* s, float x, float y);

#pragma region FontRender

FONScontext* glfonsCreate(int width, int height, int flags);
void glfonsDelete(FONScontext* ctx);

unsigned int glfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

#pragma endregion FontRender

#ifdef __cplusplus
}
#endif

#endif  // FONS_H