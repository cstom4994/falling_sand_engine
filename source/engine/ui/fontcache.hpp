
#pragma once

#include <cstdio>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "libs/external/stb_truetype.h"

#ifndef VE_FONTCACHE_CURVE_QUALITY
#define VE_FONTCACHE_CURVE_QUALITY 6
#endif  // VE_FONTCACHE_CURVE_QUALITY

#include "libs/external/utf8.h"

/*
---------------------------------- Font Atlas Caching Strategy --------------------------------------------

                        2k
                        --------------------
                        |         |        |
                        |    A    |        |
                        |         |        | 2
                        |---------|    C   | k
                        |         |        |
                     1k |    B    |        |
                        |         |        |
                        --------------------
                        |                  |
                        |                  |
                        |                  | 2
                        |        D         | k
                        |                  |
                        |                  |
                        |                  |
                        --------------------

                        Region A = 32x32 caches, 1024 glyphs
                        Region B = 32x64 caches, 512 glyphs
                        Region C = 64x64 caches, 512 glyphs
                        Region D = 128x128 caches, 256 glyphs
*/

#define VE_FONTCACHE_ATLAS_WIDTH 4096
#define VE_FONTCACHE_ATLAS_HEIGHT 2048
#define VE_FONTCACHE_ATLAS_GLYPH_PADDING 1

#define VE_FONTCACHE_ATLAS_REGION_A_WIDTH 32
#define VE_FONTCACHE_ATLAS_REGION_A_HEIGHT 32
#define VE_FONTCACHE_ATLAS_REGION_A_XSIZE (VE_FONTCACHE_ATLAS_WIDTH / 4)
#define VE_FONTCACHE_ATLAS_REGION_A_YSIZE (VE_FONTCACHE_ATLAS_HEIGHT / 2)
#define VE_FONTCACHE_ATLAS_REGION_A_XCAPACITY (VE_FONTCACHE_ATLAS_REGION_A_XSIZE / VE_FONTCACHE_ATLAS_REGION_A_WIDTH)
#define VE_FONTCACHE_ATLAS_REGION_A_YCAPACITY (VE_FONTCACHE_ATLAS_REGION_A_YSIZE / VE_FONTCACHE_ATLAS_REGION_A_HEIGHT)
#define VE_FONTCACHE_ATLAS_REGION_A_CAPACITY (VE_FONTCACHE_ATLAS_REGION_A_XCAPACITY * VE_FONTCACHE_ATLAS_REGION_A_YCAPACITY)
#define VE_FONTCACHE_ATLAS_REGION_A_XOFFSET 0
#define VE_FONTCACHE_ATLAS_REGION_A_YOFFSET 0

#define VE_FONTCACHE_ATLAS_REGION_B_WIDTH 32
#define VE_FONTCACHE_ATLAS_REGION_B_HEIGHT 64
#define VE_FONTCACHE_ATLAS_REGION_B_XSIZE (VE_FONTCACHE_ATLAS_WIDTH / 4)
#define VE_FONTCACHE_ATLAS_REGION_B_YSIZE (VE_FONTCACHE_ATLAS_HEIGHT / 2)
#define VE_FONTCACHE_ATLAS_REGION_B_XCAPACITY (VE_FONTCACHE_ATLAS_REGION_B_XSIZE / VE_FONTCACHE_ATLAS_REGION_B_WIDTH)
#define VE_FONTCACHE_ATLAS_REGION_B_YCAPACITY (VE_FONTCACHE_ATLAS_REGION_B_YSIZE / VE_FONTCACHE_ATLAS_REGION_B_HEIGHT)
#define VE_FONTCACHE_ATLAS_REGION_B_CAPACITY (VE_FONTCACHE_ATLAS_REGION_B_XCAPACITY * VE_FONTCACHE_ATLAS_REGION_B_YCAPACITY)
#define VE_FONTCACHE_ATLAS_REGION_B_XOFFSET 0
#define VE_FONTCACHE_ATLAS_REGION_B_YOFFSET VE_FONTCACHE_ATLAS_REGION_A_YSIZE

#define VE_FONTCACHE_ATLAS_REGION_C_WIDTH 64
#define VE_FONTCACHE_ATLAS_REGION_C_HEIGHT 64
#define VE_FONTCACHE_ATLAS_REGION_C_XSIZE (VE_FONTCACHE_ATLAS_WIDTH / 4)
#define VE_FONTCACHE_ATLAS_REGION_C_YSIZE (VE_FONTCACHE_ATLAS_HEIGHT)
#define VE_FONTCACHE_ATLAS_REGION_C_XCAPACITY (VE_FONTCACHE_ATLAS_REGION_C_XSIZE / VE_FONTCACHE_ATLAS_REGION_C_WIDTH)
#define VE_FONTCACHE_ATLAS_REGION_C_YCAPACITY (VE_FONTCACHE_ATLAS_REGION_C_YSIZE / VE_FONTCACHE_ATLAS_REGION_C_HEIGHT)
#define VE_FONTCACHE_ATLAS_REGION_C_CAPACITY (VE_FONTCACHE_ATLAS_REGION_C_XCAPACITY * VE_FONTCACHE_ATLAS_REGION_C_YCAPACITY)
#define VE_FONTCACHE_ATLAS_REGION_C_XOFFSET VE_FONTCACHE_ATLAS_REGION_A_XSIZE
#define VE_FONTCACHE_ATLAS_REGION_C_YOFFSET 0

#define VE_FONTCACHE_ATLAS_REGION_D_WIDTH 128
#define VE_FONTCACHE_ATLAS_REGION_D_HEIGHT 128
#define VE_FONTCACHE_ATLAS_REGION_D_XSIZE (VE_FONTCACHE_ATLAS_WIDTH / 2)
#define VE_FONTCACHE_ATLAS_REGION_D_YSIZE (VE_FONTCACHE_ATLAS_HEIGHT)
#define VE_FONTCACHE_ATLAS_REGION_D_XCAPACITY (VE_FONTCACHE_ATLAS_REGION_D_XSIZE / VE_FONTCACHE_ATLAS_REGION_D_WIDTH)
#define VE_FONTCACHE_ATLAS_REGION_D_YCAPACITY (VE_FONTCACHE_ATLAS_REGION_D_YSIZE / VE_FONTCACHE_ATLAS_REGION_D_HEIGHT)
#define VE_FONTCACHE_ATLAS_REGION_D_CAPACITY (VE_FONTCACHE_ATLAS_REGION_D_XCAPACITY * VE_FONTCACHE_ATLAS_REGION_D_YCAPACITY)
#define VE_FONTCACHE_ATLAS_REGION_D_XOFFSET (VE_FONTCACHE_ATLAS_WIDTH / 2)
#define VE_FONTCACHE_ATLAS_REGION_D_YOFFSET 0

static_assert(VE_FONTCACHE_ATLAS_REGION_A_CAPACITY == 1024, "VE FontCache Atlas Sanity check fail. Please update this assert if you changed atlas packing strategy.");
static_assert(VE_FONTCACHE_ATLAS_REGION_B_CAPACITY == 512, "VE FontCache Atlas Sanity check fail. Please update this assert if you changed atlas packing strategy.");
static_assert(VE_FONTCACHE_ATLAS_REGION_C_CAPACITY == 512, "VE FontCache Atlas Sanity check fail. Please update this assert if you changed atlas packing strategy.");
static_assert(VE_FONTCACHE_ATLAS_REGION_D_CAPACITY == 256, "VE FontCache Atlas Sanity check fail. Please update this assert if you changed atlas packing strategy.");

#define VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_X 4
#define VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_Y 4
#define VE_FONTCACHE_GLYPHDRAW_BUFFER_BATCH 4
#define VE_FONTCACHE_GLYPHDRAW_BUFFER_WIDTH (VE_FONTCACHE_ATLAS_REGION_D_WIDTH * VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_X * VE_FONTCACHE_GLYPHDRAW_BUFFER_BATCH)
#define VE_FONTCACHE_GLYPHDRAW_BUFFER_HEIGHT (VE_FONTCACHE_ATLAS_REGION_D_HEIGHT * VE_FONTCACHE_GLYPHDRAW_OVERSAMPLE_Y)

// Set to same value as VE_FONTCACHE_ATLAS_GLYPH_PADDING for best results!
#define VE_FONTCACHE_GLYPHDRAW_PADDING VE_FONTCACHE_ATLAS_GLYPH_PADDING

#define VE_FONTCACHE_FRAMEBUFFER_PASS_GLYPH 1
#define VE_FONTCACHE_FRAMEBUFFER_PASS_ATLAS 2
#define VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET 3
#define VE_FONTCACHE_FRAMEBUFFER_PASS_TARGET_UNCACHED 4

// How many to store in text shaping cache. Shaping cache is also stored in LRU format.
#define VE_FONTCACHE_SHAPECACHE_SIZE 256

// How much to reserve for each shape cache. This adds up to a ~0.768mb cache.
#define VE_FONTCACHE_SHAPECACHE_RESERVE_LENGTH 64

// Max. text size for caching. This means the cache has ~3.072mb upper bound.
#define VE_FONTCACHE_SHAPECACHE_MAX_LENGTH 256

// Sizes below this snap their advance to next pixel boundary.
#define VE_FONTCACHE_ADVANCE_SNAP_SMALLFONT_SIZE 12

// --------------------------------------------------------------- Data Types ---------------------------------------------------

typedef int64_t ve_font_id;
typedef int32_t ve_codepoint;
typedef int32_t ve_glyph;
typedef char ve_atlas_region;

struct ve_fontcache_entry {
    ve_font_id font_id = 0;
    stbtt_fontinfo info;
    bool used = false;
    float size = 24.0f;
    float size_scale = 1.0f;
};

struct ve_fontcache_vertex {
    float x;
    float y;
    float u;
    float v;
};

struct ve_fontcache_vec2 {
    float x;
    float y;
};
inline ve_fontcache_vec2 ve_fontcache_make_vec2(float x_, float y_) {
    ve_fontcache_vec2 v;
    v.x = x_;
    v.y = y_;
    return v;
}

struct ve_fontcache_draw {
    uint32_t pass = 0;  // One of VE_FONTCACHE_FRAMEBUFFER_PASS_* values.
    uint32_t start_index = 0;
    uint32_t end_index = 0;
    bool clear_before_draw = false;
    uint32_t region = 0;
    float colour[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};

struct ve_fontcache_drawlist {
    std::vector<ve_fontcache_vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<ve_fontcache_draw> dcalls;
};

typedef uint32_t ve_fontcache_poollist_itr;
typedef uint64_t ve_fontcache_poollist_value;

struct ve_fontcache_poollist_item {
    ve_fontcache_poollist_itr prev = -1;
    ve_fontcache_poollist_itr next = -1;
    ve_fontcache_poollist_value value = 0;
};

struct ve_fontcache_poollist {
    std::vector<ve_fontcache_poollist_item> pool;
    std::vector<ve_fontcache_poollist_itr> freelist;
    ve_fontcache_poollist_itr front = -1;
    ve_fontcache_poollist_itr back = -1;
    size_t size = 0;
    size_t capacity = 0;
};

struct ve_fontcache_LRU_link {
    int value = 0;
    ve_fontcache_poollist_itr ptr;
};

struct ve_fontcache_LRU {
    int capacity = 0;
    std::unordered_map<uint64_t, ve_fontcache_LRU_link> cache;
    ve_fontcache_poollist key_queue;
};

struct ve_fontcache_atlas {
    uint32_t next_atlas_idx_A = 0;
    uint32_t next_atlas_idx_B = 0;
    uint32_t next_atlas_idx_C = 0;
    uint32_t next_atlas_idx_D = 0;

    ve_fontcache_LRU stateA;
    ve_fontcache_LRU stateB;
    ve_fontcache_LRU stateC;
    ve_fontcache_LRU stateD;

    uint32_t glyph_update_batch_x = 0;
    ve_fontcache_drawlist glyph_update_batch_clear_drawlist;
    ve_fontcache_drawlist glyph_update_batch_drawlist;
};

struct ve_fontcache_shaped_text {
    std::vector<ve_glyph> glyphs;
    std::vector<ve_fontcache_vec2> pos;
    ve_fontcache_vec2 end_cursor_pos;
};

struct ve_fontcache_shaped_text_cache {
    std::vector<ve_fontcache_shaped_text> storage;
    ve_fontcache_LRU state;
    uint32_t next_cache_idx = 0;
};

struct ve_fontcache {
    std::vector<ve_fontcache_entry> entry;

    std::vector<ve_fontcache_vec2> temp_path;
    std::unordered_map<uint64_t, bool> temp_codepoint_seen;

    uint32_t snap_width = 0;
    uint32_t snap_height = 0;
    float colour[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    ve_fontcache_vec2 cursor_pos;

    ve_fontcache_drawlist drawlist;
    ve_fontcache_atlas atlas;
    ve_fontcache_shaped_text_cache shape_cache;

    bool text_shape_advanced = true;
};

// --------------------------------------------------------------------- VEFontCache Function Declarations -------------------------------------------------------------

// Call this to initialise a fontcache.
void ve_fontcache_init(ve_fontcache* cache);

// Call this to shutdown everything.
void ve_fontcache_shutdown(ve_fontcache* cache);

// Load hb_font from in-memory buffer. Supports otf, ttf, everything STB_truetype supports.
//     Caller still owns data and must hold onto it; This buffer cannot be freed while hb_font is still being used.
//     SBTTT will keep track of weak pointers into this memory.
//     If you're loading the same hb_font at different size_px values, it is OK to share the same data buffer amongst them.
//
ve_font_id ve_fontcache_load(ve_fontcache* cache, const void* data, size_t data_size, float size_px = 24.0f);

// Load hb_font from file. Supports otf, ttf, everything STB_truetype supports.
//     Caller still owns given buffer and must hold onto it. This buffer cannot be freed while hb_font is still being used.
//     SBTTT will keep track of weak pointers into this memory.
//     If you're loading the same hb_font at different size_px values, it is OK to share the same buffer amongst them.
//
ve_font_id ve_fontcache_loadfile(ve_fontcache* cache, const char* filename, std::vector<uint8_t>& buffer, float size_px = 24.0f);

// Unload a font and relase memory. Calling ve_fontcache_shutdown already does this on all loaded fonts.
void ve_fontcache_unload(ve_fontcache* cache, ve_font_id id);

// Configure snapping glyphs to pixel border when hb_font is rendered to 2D screen. May affect kerning. This may be changed at any time.
// Set both to zero to disable pixel snapping.
void ve_fontcache_configure_snap(ve_fontcache* cache, uint32_t snap_width = 0, uint32_t snap_height = 0);

// Call this per-frame after draw list has been executed. This will clear the drawlist for next frame.
void ve_fontcache_flush_drawlist(ve_fontcache* cache);

// Main draw text function. This batches caches both shape and glyphs, and uses fallback path when not available.
//     Note that this function immediately appends everything needed to render the next to the cache->drawlist.
//     If you want to draw to multiple unrelated targets, simply draw_text, then loop through execute draw list, draw_text again, and loop through execute draw list again.
//     Suggest scalex = 1 / screen_width and scaley = 1 / screen height! scalex and scaley will need to account for aspect ratio.
//
bool ve_fontcache_draw_text(ve_fontcache* cache, ve_font_id font, const std::string& text_utf8, float posx = 0.0f, float posy = 0.0f, float scalex = 1.0f, float scaley = 1.0f);

// Get where the last ve_fontcache_draw_text call left off.
ve_fontcache_vec2 ve_fontcache_get_cursor_pos(ve_fontcache* cache);

// Merges drawcalls. Significantly improves drawcall overhead, highly recommended. Call this before looping through and executing drawlist.
void ve_fontcache_optimise_drawlist(ve_fontcache* cache);

// Retrieves current drawlist from cache.
ve_fontcache_drawlist* ve_fontcache_get_drawlist(ve_fontcache* cache);

// Set text colour of subsequent text draws.
void ve_fontcache_set_colour(ve_fontcache* cache, float c[4]);

inline void ve_fontcache_enable_advanced_text_shaping(ve_fontcache* cache, bool enabled = true) { cache->text_shape_advanced = enabled; }

// --------------------------------------------------------------------- Generic Data Structure Declarations -------------------------------------------------------------

// Generic pool list for alloc-free LRU implementation.
void ve_fontcache_poollist_init(ve_fontcache_poollist& plist, int capacity);
void ve_fontcache_poollist_push_front(ve_fontcache_poollist& plist, ve_fontcache_poollist_value v);
void ve_fontcache_poollist_erase(ve_fontcache_poollist& plist, ve_fontcache_poollist_itr it);
ve_fontcache_poollist_value ve_fontcache_poollist_peek_back(ve_fontcache_poollist& plist);
ve_fontcache_poollist_value ve_fontcache_poollist_pop_back(ve_fontcache_poollist& plist);

// Generic LRU ( Least-Recently-Used ) cache implementation, reused for both atlas and shape cache.
void ve_fontcache_LRU_init(ve_fontcache_LRU& LRU, int capacity);
int ve_fontcache_LRU_get(ve_fontcache_LRU& LRU, uint64_t key);
int ve_fontcache_LRU_peek(ve_fontcache_LRU& LRU, uint64_t key);
uint64_t ve_fontcache_LRU_put(ve_fontcache_LRU& LRU, uint64_t key, int val);
void ve_fontcache_LRU_refresh(ve_fontcache_LRU& LRU, uint64_t key);
uint64_t ve_fontcache_LRU_get_next_evicted(ve_fontcache_LRU& LRU);

bool ve_fontcache_cache_glyph(ve_fontcache* cache, ve_font_id font, ve_glyph glyph_index, float scaleX = 1.0f, float scaleY = 1.0f, float translateX = 0.0f, float translateY = 0.0f);
