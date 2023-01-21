
#include "fonts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL_pixels.h"
#include "core/core.h"
#include "renderer/renderer_opengl.h"

#if SDL_VERSION_ATLEAST(2, 0, 0)
#define FontCache_GET_ALPHA(sdl_color) ((sdl_color).a)
#else
#define FontCache_GET_ALPHA(sdl_color) ((sdl_color).unused)
#endif

// Need SDL_RenderIsClipEnabled() for proper clipping support
#if SDL_VERSION_ATLEAST(2, 0, 4)
#define ENABLE_SDL_CLIPPING
#endif

#define FontCache_MIN(a, b) ((a) < (b) ? (a) : (b))
#define FontCache_MAX(a, b) ((a) > (b) ? (a) : (b))

#define FontCache_EXTRACT_VARARGS(buffer, start_args)       \
    {                                                       \
        va_list lst;                                        \
        va_start(lst, start_args);                          \
        vsnprintf(buffer, fc_buffer_size, start_args, lst); \
        va_end(lst);                                        \
    }

// Extra pixels of padding around each glyph to avoid linear filtering artifacts
#define FontCache_CACHE_PADDING 1

static U8 has_clip(FontCache_Target* dest) { return dest->use_clip_rect; }

static FontCache_Rect get_clip(FontCache_Target* dest) { return dest->clip_rect; }

static void set_clip(FontCache_Target* dest, FontCache_Rect* rect) {
    if (rect != NULL)
        R_SetClipRect(dest, *rect);
    else
        R_UnsetClip(dest);
}

static void set_color(FontCache_Image* src, U8 r, U8 g, U8 b, U8 a) { R_SetRGBA(src, r, g, b, a); }

static char* new_concat(const char* a, const char* b) {
    // Create new buffer
    unsigned int size = strlen(a) + strlen(b);
    char* new_string = (char*)malloc(size + 1);

    // Concatenate strings in the new buffer
    strcpy(new_string, a);
    strcat(new_string, b);

    return new_string;
}

static char* replace_concat(char** a, const char* b) {
    char* new_string = new_concat(*a, b);
    free(*a);
    *a = new_string;
    return *a;
}

// Shared buffer for variadic text
static char* fc_buffer = NULL;
static unsigned int fc_buffer_size = 1024;

static U8 fc_has_render_target_support = 0;

char* FontCache_GetStringASCII(void) {
    static char* buffer = NULL;
    if (buffer == NULL) {
        int i;
        char c;
        buffer = (char*)malloc(512);
        memset(buffer, 0, 512);
        i = 0;
        c = 32;
        while (1) {
            buffer[i] = c;
            if (c == 126) break;
            ++i;
            ++c;
        }
    }
    return U8_strdup(buffer);
}

char* FontCache_GetStringLatin1(void) {
    static char* buffer = NULL;
    if (buffer == NULL) {
        int i;
        unsigned char c;
        buffer = (char*)malloc(512);
        memset(buffer, 0, 512);
        i = 0;
        c = 0xA0;
        while (1) {
            buffer[i] = 0xC2;
            buffer[i + 1] = c;
            if (c == 0xBF) break;
            i += 2;
            ++c;
        }
        i += 2;
        c = 0x80;
        while (1) {
            buffer[i] = 0xC3;
            buffer[i + 1] = c;
            if (c == 0xBF) break;
            i += 2;
            ++c;
        }
    }
    return U8_strdup(buffer);
}

char* FontCache_GetStringASCII_Latin1(void) {
    static char* buffer = NULL;
    if (buffer == NULL) buffer = new_concat(FontCache_GetStringASCII(), FontCache_GetStringLatin1());

    return U8_strdup(buffer);
}

FontCache_Rect FontCache_MakeRect(float x, float y, float w, float h) {
    FontCache_Rect r = {x, y, w, h};
    return r;
}

FontCache_Scale FontCache_MakeScale(float x, float y) {
    FontCache_Scale s = {x, y};

    return s;
}

METAENGINE_Color FontCache_MakeColor(U8 r, U8 g, U8 b, U8 a) {
    METAENGINE_Color c = {r, g, b, a};
    return c;
}

FontCache_Effect FontCache_MakeEffect(FontCache_AlignEnum alignment, FontCache_Scale scale, METAENGINE_Color color) {
    FontCache_Effect e;

    e.alignment = alignment;
    e.scale = scale;
    e.color = color;

    return e;
}

FontCache_GlyphData FontCache_MakeGlyphData(int cache_level, Sint16 x, Sint16 y, U16 w, U16 h) {
    FontCache_GlyphData gd;

    gd.rect.x = x;
    gd.rect.y = y;
    gd.rect.w = w;
    gd.rect.h = h;
    gd.cache_level = cache_level;

    return gd;
}

// Enough to hold all of the ascii characters and some.
#define FontCache_DEFAULT_NUM_BUCKETS 300

typedef struct FontCache_MapNode {
    U32 key;
    FontCache_GlyphData value;
    struct FontCache_MapNode* next;

} FontCache_MapNode;

typedef struct FontCache_Map {
    int num_buckets;
    FontCache_MapNode** buckets;
} FontCache_Map;

static FontCache_Map* FontCache_MapCreate(int num_buckets) {
    int i;
    FontCache_Map* map = (FontCache_Map*)malloc(sizeof(FontCache_Map));

    map->num_buckets = num_buckets;
    map->buckets = (FontCache_MapNode**)malloc(num_buckets * sizeof(FontCache_MapNode*));

    for (i = 0; i < num_buckets; ++i) {
        map->buckets[i] = NULL;
    }

    return map;
}

/*static void FontCache_MapClear(FontCache_Map* map)
{
    int i;
    if(map == NULL)
        return;

    // Go through each bucket
    for(i = 0; i < map->num_buckets; ++i)
    {
        // Delete the nodes in order
        FontCache_MapNode* node = map->buckets[i];
        while(node != NULL)
        {
            FontCache_MapNode* last = node;
            node = node->next;
            free(last);
        }
        // Set the bucket to empty
        map->buckets[i] = NULL;
    }
}*/

static void FontCache_MapFree(FontCache_Map* map) {
    int i;
    if (map == NULL) return;

    // Go through each bucket
    for (i = 0; i < map->num_buckets; ++i) {
        // Delete the nodes in order
        FontCache_MapNode* node = map->buckets[i];
        while (node != NULL) {
            FontCache_MapNode* last = node;
            node = node->next;
            free(last);
        }
    }

    free(map->buckets);
    free(map);
}

// Note: Does not handle duplicates in any special way.
static FontCache_GlyphData* FontCache_MapInsert(FontCache_Map* map, U32 codepoint, FontCache_GlyphData glyph) {
    U32 index;
    FontCache_MapNode* node;
    if (map == NULL) return NULL;

    // Get index for bucket
    index = codepoint % map->num_buckets;

    // If this bucket is empty, create a node and return its value
    if (map->buckets[index] == NULL) {
        node = map->buckets[index] = (FontCache_MapNode*)malloc(sizeof(FontCache_MapNode));
        node->key = codepoint;
        node->value = glyph;
        node->next = NULL;
        return &node->value;
    }

    for (node = map->buckets[index]; node != NULL; node = node->next) {
        // Find empty node and add a new one on.
        if (node->next == NULL) {
            node->next = (FontCache_MapNode*)malloc(sizeof(FontCache_MapNode));
            node = node->next;

            node->key = codepoint;
            node->value = glyph;
            node->next = NULL;
            return &node->value;
        }
    }

    return NULL;
}

static FontCache_GlyphData* FontCache_MapFind(FontCache_Map* map, U32 codepoint) {
    U32 index;
    FontCache_MapNode* node;
    if (map == NULL) return NULL;

    // Get index for bucket
    index = codepoint % map->num_buckets;

    // Go through list until we find a match
    for (node = map->buckets[index]; node != NULL; node = node->next) {
        if (node->key == codepoint) return &node->value;
    }

    return NULL;
}

struct FontCache_Font {

    TTF_Font* ttf_source;  // TTF_Font source of characters
    U8 owns_ttf_source;    // Can we delete the TTF_Font ourselves?

    FontCache_FilterEnum filter;

    METAENGINE_Color default_color;
    U16 height;

    U16 maxWidth;
    U16 baseline;
    int ascent;
    int descent;

    int lineSpacing;
    int letterSpacing;

    // Uses 32-bit (4-byte) Unicode codepoints to refer to each glyph
    // Codepoints are little endian (reversed from UTF-8) so that something like 0x00000005 is ASCII 5 and the map can be indexed by ASCII values
    FontCache_Map* glyphs;

    FontCache_GlyphData last_glyph;  // Texture packing cursor
    int glyph_cache_size;
    int glyph_cache_count;
    FontCache_Image** glyph_cache;

    char* loading_string;
};

// Private
static FontCache_GlyphData* FontCache_PackGlyphData(FontCache_Font* font, U32 codepoint, U16 width, U16 maxWidth, U16 maxHeight);

static FontCache_Rect FontCache_RenderLeft(FontCache_Font* font, FontCache_Target* dest, float x, float y, FontCache_Scale scale, const char* text);
static FontCache_Rect FontCache_RenderCenter(FontCache_Font* font, FontCache_Target* dest, float x, float y, FontCache_Scale scale, const char* text);
static FontCache_Rect FontCache_RenderRight(FontCache_Font* font, FontCache_Target* dest, float x, float y, FontCache_Scale scale, const char* text);

static_inline C_Surface* FontCache_CreateSurface32(U32 width, U32 height) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    return SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
#else
    return SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
#endif
}

char* U8_alloc(unsigned int size) {
    char* result;
    if (size == 0) return NULL;

    result = (char*)malloc(size);
    result[0] = '\0';

    return result;
}

void U8_free(char* string) { free(string); }

char* U8_strdup(const char* string) {
    char* result;
    if (string == NULL) return NULL;

    result = (char*)malloc(strlen(string) + 1);
    strcpy(result, string);

    return result;
}

int U8_strlen(const char* string) {
    int length = 0;
    if (string == NULL) return 0;

    while (*string != '\0') {
        string = U8_next(string);
        ++length;
    }

    return length;
}

int U8_charsize(const char* character) {
    if (character == NULL) return 0;

    if ((unsigned char)*character <= 0x7F)
        return 1;
    else if ((unsigned char)*character < 0xE0)
        return 2;
    else if ((unsigned char)*character < 0xF0)
        return 3;
    else
        return 4;
    return 1;
}

int U8_charcpy(char* buffer, const char* source, int buffer_size) {
    int charsize;
    if (buffer == NULL || source == NULL || buffer_size < 1) return 0;

    charsize = U8_charsize(source);
    if (charsize > buffer_size) return 0;

    memcpy(buffer, source, charsize);
    return charsize;
}

const char* U8_next(const char* string) { return string + U8_charsize(string); }

int U8_strinsert(char* string, int position, const char* source, int max_bytes) {
    int pos_bytes;
    int len;
    int add_len;
    int ulen;

    if (string == NULL || source == NULL) return 0;

    len = strlen(string);
    add_len = strlen(source);
    ulen = U8_strlen(string);

    if (position == -1) position = ulen;

    if (position < 0 || position > ulen || len + add_len + 1 > max_bytes) return 0;

    // Move string pointer to the proper position
    pos_bytes = 0;
    while (*string != '\0' && pos_bytes < position) {
        string = (char*)U8_next(string);
        ++pos_bytes;
    }

    // Move the rest of the string out of the way
    memmove(string + add_len, string, len - pos_bytes + 1);

    // Copy in the new characters
    memcpy(string, source, add_len);

    return 1;
}

void U8_strdel(char* string, int position) {
    if (string == NULL || position < 0) return;

    while (*string != '\0') {
        if (position == 0) {
            int chars_to_erase = U8_charsize(string);
            int remaining_bytes = strlen(string) + 1;
            memmove(string, string + chars_to_erase, remaining_bytes);
            break;
        }

        string = (char*)U8_next(string);
        --position;
    }
}

static_inline FontCache_Rect FontCache_RectUnion(FontCache_Rect A, FontCache_Rect B) {
    float x, x2, y, y2;
    x = FontCache_MIN(A.x, B.x);
    y = FontCache_MIN(A.y, B.y);
    x2 = FontCache_MAX(A.x + A.w, B.x + B.w);
    y2 = FontCache_MAX(A.y + A.h, B.y + B.h);
    {
        FontCache_Rect result = {x, y, FontCache_MAX(0, x2 - x), FontCache_MAX(0, y2 - y)};
        return result;
    }
}

// Adapted from SDL_IntersectRect
static_inline FontCache_Rect FontCache_RectIntersect(FontCache_Rect A, FontCache_Rect B) {
    FontCache_Rect result;
    float Amin, Amax, Bmin, Bmax;

    // Horizontal intersection
    Amin = A.x;
    Amax = Amin + A.w;
    Bmin = B.x;
    Bmax = Bmin + B.w;
    if (Bmin > Amin) Amin = Bmin;
    result.x = Amin;
    if (Bmax < Amax) Amax = Bmax;
    result.w = Amax - Amin > 0 ? Amax - Amin : 0;

    // Vertical intersection
    Amin = A.y;
    Amax = Amin + A.h;
    Bmin = B.y;
    Bmax = Bmin + B.h;
    if (Bmin > Amin) Amin = Bmin;
    result.y = Amin;
    if (Bmax < Amax) Amax = Bmax;
    result.h = Amax - Amin > 0 ? Amax - Amin : 0;

    return result;
}

FontCache_Rect FontCache_DefaultRenderCallback(FontCache_Image* src, FontCache_Rect* srcrect, FontCache_Target* dest, float x, float y, float xscale, float yscale) {
    float w = srcrect->w * xscale;
    float h = srcrect->h * yscale;
    FontCache_Rect result;

    // FIXME: Why does the scaled offset look so wrong?
    {
        metadot_rect r = *srcrect;
        R_BlitScale(src, &r, dest, x + xscale * r.w / 2.0f, y + r.h / 2.0f, xscale, yscale);
    }

    result.x = x;
    result.y = y;
    result.w = w;
    result.h = h;
    return result;
}

static FontCache_Rect (*fc_render_callback)(FontCache_Image* src, FontCache_Rect* srcrect, FontCache_Target* dest, float x, float y, float xscale, float yscale) = &FontCache_DefaultRenderCallback;

void FontCache_SetRenderCallback(FontCache_Rect (*callback)(FontCache_Image* src, FontCache_Rect* srcrect, FontCache_Target* dest, float x, float y, float xscale, float yscale)) {
    if (callback == NULL)
        fc_render_callback = &FontCache_DefaultRenderCallback;
    else
        fc_render_callback = callback;
}

void FontCache_GetUTF8FromCodepoint(char* result, U32 codepoint) {
    char a, b, c, d;

    if (result == NULL) return;

    a = (codepoint >> 24) & 0xFF;
    b = (codepoint >> 16) & 0xFF;
    c = (codepoint >> 8) & 0xFF;
    d = codepoint & 0xFF;

    if (a == 0) {
        if (b == 0) {
            if (c == 0) {
                result[0] = d;
                result[1] = '\0';
            } else {
                result[0] = c;
                result[1] = d;
                result[2] = '\0';
            }
        } else {
            result[0] = b;
            result[1] = c;
            result[2] = d;
            result[3] = '\0';
        }
    } else {
        result[0] = a;
        result[1] = b;
        result[2] = c;
        result[3] = d;
        result[4] = '\0';
    }
}

U32 FontCache_GetCodepointFromUTF8(const char** c, U8 advance_pointer) {
    U32 result = 0;
    const char* str;
    if (c == NULL || *c == NULL) return 0;

    str = *c;
    if ((unsigned char)*str <= 0x7F)
        result = *str;
    else if ((unsigned char)*str < 0xE0) {
        result |= (unsigned char)(*str) << 8;
        result |= (unsigned char)(*(str + 1));
        if (advance_pointer) *c += 1;
    } else if ((unsigned char)*str < 0xF0) {
        result |= (unsigned char)(*str) << 16;
        result |= (unsigned char)(*(str + 1)) << 8;
        result |= (unsigned char)(*(str + 2));
        if (advance_pointer) *c += 2;
    } else {
        result |= (unsigned char)(*str) << 24;
        result |= (unsigned char)(*(str + 1)) << 16;
        result |= (unsigned char)(*(str + 2)) << 8;
        result |= (unsigned char)(*(str + 3));
        if (advance_pointer) *c += 3;
    }
    return result;
}

void FontCache_SetLoadingString(FontCache_Font* font, const char* string) {
    if (font == NULL) return;

    free(font->loading_string);
    font->loading_string = U8_strdup(string);
}

unsigned int FontCache_GetBufferSize(void) { return fc_buffer_size; }

void FontCache_SetBufferSize(unsigned int size) {
    free(fc_buffer);
    if (size > 0) {
        fc_buffer_size = size;
        fc_buffer = (char*)malloc(fc_buffer_size);
    } else
        fc_buffer = (char*)malloc(fc_buffer_size);
}

// Constructors

static void FontCache_Init(FontCache_Font* font) {
    if (font == NULL) return;

    font->ttf_source = NULL;
    font->owns_ttf_source = 0;

    font->filter = FontCache_FILTER_NEAREST;

    font->default_color.r = 0;
    font->default_color.g = 0;
    font->default_color.b = 0;
    FontCache_GET_ALPHA(font->default_color) = 255;

    font->height = 0;  // ascent+descent

    font->maxWidth = 0;
    font->baseline = 0;
    font->ascent = 0;
    font->descent = 0;

    font->lineSpacing = 0;
    font->letterSpacing = 0;

    // Give a little offset for when filtering/mipmaps are used.  Depending on mipmap level, this will still not be enough.
    font->last_glyph.rect.x = FontCache_CACHE_PADDING;
    font->last_glyph.rect.y = FontCache_CACHE_PADDING;
    font->last_glyph.rect.w = 0;
    font->last_glyph.rect.h = 0;
    font->last_glyph.cache_level = 0;

    if (font->glyphs != NULL) FontCache_MapFree(font->glyphs);

    font->glyphs = FontCache_MapCreate(FontCache_DEFAULT_NUM_BUCKETS);

    font->glyph_cache_size = 3;
    font->glyph_cache_count = 0;

    font->glyph_cache = (FontCache_Image**)malloc(font->glyph_cache_size * sizeof(FontCache_Image*));

    if (font->loading_string == NULL) font->loading_string = FontCache_GetStringASCII();

    if (fc_buffer == NULL) fc_buffer = (char*)malloc(fc_buffer_size);
}

static U8 FontCache_GrowGlyphCache(FontCache_Font* font) {
    if (font == NULL) return 0;
    R_Image* new_level = R_CreateImage(font->height * 12, font->height * 12, R_FORMAT_RGBA);
    R_SetAnchor(new_level, 0.5f, 0.5f);  // Just in case the default is different
    if (new_level == NULL || !FontCache_SetGlyphCacheLevel(font, font->glyph_cache_count, new_level)) {
        METADOT_ERROR("Error: SDL_FontCache ran out of packing space and could not add another cache level.");
        SDL_DestroyTexture(new_level);
        return 0;
    }
    return 1;
}

U8 FontCache_UploadGlyphCache(FontCache_Font* font, int cache_level, C_Surface* data_surface) {
    if (font == NULL || data_surface == NULL) return 0;

    R_Image* new_level = R_CopyImageFromSurface(data_surface);
    R_SetAnchor(new_level, 0.5f, 0.5f);  // Just in case the default is different
    if (FontCache_GetFilterMode(font) == FontCache_FILTER_LINEAR)
        R_SetImageFilter(new_level, R_FILTER_LINEAR);
    else
        R_SetImageFilter(new_level, R_FILTER_NEAREST);
    if (new_level == NULL || !FontCache_SetGlyphCacheLevel(font, cache_level, new_level)) {
        METADOT_ERROR("Error: SDL_FontCache ran out of packing space and could not add another cache level.");

        R_FreeImage(new_level);

        return 0;
    }
    return 1;
}

static FontCache_GlyphData* FontCache_PackGlyphData(FontCache_Font* font, U32 codepoint, U16 width, U16 maxWidth, U16 maxHeight) {
    FontCache_Map* glyphs = font->glyphs;
    FontCache_GlyphData* last_glyph = &font->last_glyph;
    U16 height = font->height + FontCache_CACHE_PADDING;

    if (last_glyph->rect.x + last_glyph->rect.w + width >= maxWidth - FontCache_CACHE_PADDING) {
        if (last_glyph->rect.y + height + height >= maxHeight - FontCache_CACHE_PADDING) {
            // Get ready to pack on the next cache level when it is ready
            last_glyph->cache_level = font->glyph_cache_count;
            last_glyph->rect.x = FontCache_CACHE_PADDING;
            last_glyph->rect.y = FontCache_CACHE_PADDING;
            last_glyph->rect.w = 0;
            return NULL;
        } else {
            // Go to next row
            last_glyph->rect.x = FontCache_CACHE_PADDING;
            last_glyph->rect.y += height;
            last_glyph->rect.w = 0;
        }
    }

    // Move to next space
    last_glyph->rect.x += last_glyph->rect.w + 1 + FontCache_CACHE_PADDING;
    last_glyph->rect.w = width;

    return FontCache_MapInsert(glyphs, codepoint, FontCache_MakeGlyphData(last_glyph->cache_level, last_glyph->rect.x, last_glyph->rect.y, last_glyph->rect.w, last_glyph->rect.h));
}

FontCache_Image* FontCache_GetGlyphCacheLevel(FontCache_Font* font, int cache_level) {
    if (font == NULL || cache_level < 0 || cache_level > font->glyph_cache_count) return NULL;

    return font->glyph_cache[cache_level];
}

U8 FontCache_SetGlyphCacheLevel(FontCache_Font* font, int cache_level, FontCache_Image* cache_texture) {
    if (font == NULL || cache_level < 0) return 0;

    // Must be sequentially added
    if (cache_level > font->glyph_cache_count + 1) return 0;

    if (cache_level == font->glyph_cache_count) {
        font->glyph_cache_count++;

        // Grow cache?
        if (font->glyph_cache_count > font->glyph_cache_size) {
            // Copy old cache to new one
            int i;
            FontCache_Image** new_cache;
            new_cache = (FontCache_Image**)malloc(font->glyph_cache_count * sizeof(FontCache_Image*));
            for (i = 0; i < font->glyph_cache_size; ++i) new_cache[i] = font->glyph_cache[i];

            // Save new cache
            free(font->glyph_cache);
            font->glyph_cache_size = font->glyph_cache_count;
            font->glyph_cache = new_cache;
        }
    }

    font->glyph_cache[cache_level] = cache_texture;
    return 1;
}

FontCache_Font* FontCache_CreateFont(void) {
    FontCache_Font* font;

    font = (FontCache_Font*)malloc(sizeof(FontCache_Font));
    memset(font, 0, sizeof(FontCache_Font));

    FontCache_Init(font);

    return font;
}

// Assume this many will be enough...
#define FontCache_LOAD_MAX_SURFACES 10

U8 FontCache_LoadFontFromTTF(FontCache_Font* font, TTF_Font* ttf, METAENGINE_Color color) {
    if (font == NULL || ttf == NULL) return 0;

    FontCache_ClearFont(font);

    // Might as well check render target support here

    fc_has_render_target_support = R_IsFeatureEnabled(R_FEATURE_RENDER_TARGETS);

    font->ttf_source = ttf;

    // font->line_height = TTF_FontLineSkip(ttf);
    font->height = TTF_FontHeight(ttf);
    font->ascent = TTF_FontAscent(ttf);
    font->descent = -TTF_FontDescent(ttf);

    // Some bug for certain fonts can result in an incorrect height.
    if (font->height < font->ascent - font->descent) font->height = font->ascent - font->descent;

    font->baseline = font->height - font->descent;

    font->default_color = color;

    {
        METAENGINE_Color white = {255, 255, 255, 255};
        C_Surface* glyph_surf;
        char buff[5];
        const char* buff_ptr = buff;
        const char* source_string;
        U8 packed = 0;

        // Copy glyphs from the surface to the font texture and store the position data
        // Pack row by row into a square texture
        // Try figuring out dimensions that make sense for the font size.
        unsigned int w = font->height * 12;
        unsigned int h = font->height * 12;
        C_Surface* surfaces[FontCache_LOAD_MAX_SURFACES];
        int num_surfaces = 1;
        surfaces[0] = FontCache_CreateSurface32(w, h);
        font->last_glyph.rect.x = FontCache_CACHE_PADDING;
        font->last_glyph.rect.y = FontCache_CACHE_PADDING;
        font->last_glyph.rect.w = 0;
        font->last_glyph.rect.h = font->height;

        memset(buff, 0, 5);
        source_string = font->loading_string;
        for (; *source_string != '\0'; source_string = U8_next(source_string)) {
            if (!U8_charcpy(buff, source_string, 5)) continue;
            glyph_surf = TTF_RenderUTF8_Blended(ttf, buff, ToSDLColor(white));
            if (glyph_surf == NULL) continue;

            // Try packing.  If it fails, create a new surface for the next cache level.
            packed = (FontCache_PackGlyphData(font, FontCache_GetCodepointFromUTF8(&buff_ptr, 0), glyph_surf->w, surfaces[num_surfaces - 1]->w, surfaces[num_surfaces - 1]->h) != NULL);
            if (!packed) {
                int i = num_surfaces - 1;
                if (num_surfaces >= FontCache_LOAD_MAX_SURFACES) {
                    // Can't do any more!
                    METADOT_ERROR("SDL_FontCache error: Could not create enough cache surfaces to fit all of the loading string!");
                    SDL_FreeSurface(glyph_surf);
                    break;
                }

                // Upload the current surface to the glyph cache now so we can keep the cache level packing cursor up to date as we go.
                FontCache_UploadGlyphCache(font, i, surfaces[i]);
                SDL_FreeSurface(surfaces[i]);
                // Update the glyph cursor to the new cache level.  We need to do this here because the actual cache lags behind our use of the packing above.
                font->last_glyph.cache_level = num_surfaces;

                surfaces[num_surfaces] = FontCache_CreateSurface32(w, h);
                num_surfaces++;
            }

            // Try packing for the new surface, then blit onto it.
            if (packed || FontCache_PackGlyphData(font, FontCache_GetCodepointFromUTF8(&buff_ptr, 0), glyph_surf->w, surfaces[num_surfaces - 1]->w, surfaces[num_surfaces - 1]->h) != NULL) {
                SDL_SetSurfaceBlendMode(glyph_surf, SDL_BLENDMODE_NONE);
                SDL_Rect srcRect = {0, 0, glyph_surf->w, glyph_surf->h};
                SDL_Rect destrect = font->last_glyph.rect;
                SDL_BlitSurface(glyph_surf, &srcRect, surfaces[num_surfaces - 1], &destrect);
            }

            SDL_FreeSurface(glyph_surf);
        }

        {
            int i = num_surfaces - 1;
            FontCache_UploadGlyphCache(font, i, surfaces[i]);
            SDL_FreeSurface(surfaces[i]);
        }
    }

    return 1;
}

U8 FontCache_LoadFont(FontCache_Font* font, const char* filename_ttf, U32 pointSize, METAENGINE_Color color, int style) {
    SDL_RWops* rwops;

    if (font == NULL) return 0;

    rwops = SDL_RWFromFile(filename_ttf, "rb");

    if (rwops == NULL) {
        METADOT_ERROR("Unable to open file for reading: %s", SDL_GetError());
        return 0;
    }

    return FontCache_LoadFont_RW(font, rwops, 1, pointSize, color, style);
}

U8 FontCache_LoadFont_RW(FontCache_Font* font, SDL_RWops* file_rwops_ttf, U8 own_rwops, U32 pointSize, METAENGINE_Color color, int style) {
    U8 result;
    TTF_Font* ttf;
    U8 outline;

    if (font == NULL) return 0;

    if (!TTF_WasInit() && TTF_Init() < 0) {
        METADOT_ERROR("Unable to initialize SDL_ttf: %s", TTF_GetError());
        if (own_rwops) SDL_RWclose(file_rwops_ttf);
        return 0;
    }

    ttf = TTF_OpenFontRW(file_rwops_ttf, own_rwops, pointSize);

    if (ttf == NULL) {
        METADOT_ERROR("Unable to load TrueType font: %s", TTF_GetError());
        if (own_rwops) SDL_RWclose(file_rwops_ttf);
        return 0;
    }

    outline = (style & TTF_STYLE_OUTLINE);
    if (outline) {
        style &= ~TTF_STYLE_OUTLINE;
        TTF_SetFontOutline(ttf, 1);
    }
    TTF_SetFontStyle(ttf, style);

    result = FontCache_LoadFontFromTTF(font, ttf, color);

    // Can only load new (uncached) glyphs if we can keep the SDL_RWops open.
    font->owns_ttf_source = own_rwops;
    if (!own_rwops) {
        TTF_CloseFont(font->ttf_source);
        font->ttf_source = NULL;
    }

    return result;
}

void FontCache_ClearFont(FontCache_Font* font) {
    int i;
    if (font == NULL) return;

    // Release resources
    if (font->owns_ttf_source) TTF_CloseFont(font->ttf_source);

    font->owns_ttf_source = 0;
    font->ttf_source = NULL;

    // Delete glyph map
    FontCache_MapFree(font->glyphs);
    font->glyphs = NULL;

    // Delete glyph cache
    for (i = 0; i < font->glyph_cache_count; ++i) {

        R_FreeImage(font->glyph_cache[i]);
    }
    free(font->glyph_cache);
    font->glyph_cache = NULL;

    // Reset font
    FontCache_Init(font);
}

void FontCache_FreeFont(FontCache_Font* font) {
    int i;
    if (font == NULL) return;

    // Release resources
    if (font->owns_ttf_source) TTF_CloseFont(font->ttf_source);

    // Delete glyph map
    FontCache_MapFree(font->glyphs);

    // Delete glyph cache
    for (i = 0; i < font->glyph_cache_count; ++i) {

        R_FreeImage(font->glyph_cache[i]);
    }
    free(font->glyph_cache);

    free(font->loading_string);

    free(font);
}

int FontCache_GetNumCacheLevels(FontCache_Font* font) { return font->glyph_cache_count; }

U8 FontCache_AddGlyphToCache(FontCache_Font* font, C_Surface* glyph_surface) {
    if (font == NULL || glyph_surface == NULL) return 0;

    SDL_SetSurfaceBlendMode(glyph_surface, SDL_BLENDMODE_NONE);
    FontCache_Image* dest = FontCache_GetGlyphCacheLevel(font, font->last_glyph.cache_level);
    if (dest == NULL) return 0;

    {
        R_Target* target = R_LoadTarget(dest);
        if (target == NULL) return 0;
        R_Image* img = R_CopyImageFromSurface(glyph_surface);
        R_SetAnchor(img, 0.5f, 0.5f);  // Just in case the default is different
        R_SetImageFilter(img, R_FILTER_NEAREST);
        R_SetBlendMode(img, R_BLEND_SET);

        SDL_Rect destrect = font->last_glyph.rect;
        R_Blit(img, NULL, target, destrect.x + destrect.w / 2, destrect.y + destrect.h / 2);

        R_FreeImage(img);
        R_FreeTarget(target);
    }

    return 1;
}

unsigned int FontCache_GetNumCodepoints(FontCache_Font* font) {
    FontCache_Map* glyphs;
    int i;
    unsigned int result = 0;
    if (font == NULL || font->glyphs == NULL) return 0;

    glyphs = font->glyphs;

    for (i = 0; i < glyphs->num_buckets; ++i) {
        FontCache_MapNode* node;
        for (node = glyphs->buckets[i]; node != NULL; node = node->next) {
            result++;
        }
    }

    return result;
}

void FontCache_GetCodepoints(FontCache_Font* font, U32* result) {
    FontCache_Map* glyphs;
    int i;
    unsigned int count = 0;
    if (font == NULL || font->glyphs == NULL) return;

    glyphs = font->glyphs;

    for (i = 0; i < glyphs->num_buckets; ++i) {
        FontCache_MapNode* node;
        for (node = glyphs->buckets[i]; node != NULL; node = node->next) {
            result[count] = node->key;
            count++;
        }
    }
}

U8 FontCache_GetGlyphData(FontCache_Font* font, FontCache_GlyphData* result, U32 codepoint) {
    FontCache_GlyphData* e = FontCache_MapFind(font->glyphs, codepoint);
    if (e == NULL) {
        char buff[5];
        int w, h;
        METAENGINE_Color white = {255, 255, 255, 255};
        C_Surface* surf;
        FontCache_Image* cache_image;

        if (font->ttf_source == NULL) return 0;

        FontCache_GetUTF8FromCodepoint(buff, codepoint);

        cache_image = FontCache_GetGlyphCacheLevel(font, font->last_glyph.cache_level);
        if (cache_image == NULL) {
            METADOT_ERROR("SDL_FontCache: Failed to load cache image, so cannot add new glyphs!");
            return 0;
        }

        w = cache_image->w;
        h = cache_image->h;

        surf = TTF_RenderUTF8_Blended(font->ttf_source, buff, ToSDLColor(white));
        if (surf == NULL) {
            return 0;
        }

        e = FontCache_PackGlyphData(font, codepoint, surf->w, w, h);
        if (e == NULL) {
            // Grow the cache
            FontCache_GrowGlyphCache(font);

            // Try packing again
            e = FontCache_PackGlyphData(font, codepoint, surf->w, w, h);
            if (e == NULL) {
                SDL_FreeSurface(surf);
                return 0;
            }
        }

        // Render onto the cache texture
        FontCache_AddGlyphToCache(font, surf);

        SDL_FreeSurface(surf);
    }

    if (result != NULL && e != NULL) *result = *e;

    return 1;
}

FontCache_GlyphData* FontCache_SetGlyphData(FontCache_Font* font, U32 codepoint, FontCache_GlyphData glyph_data) { return FontCache_MapInsert(font->glyphs, codepoint, glyph_data); }

// Drawing
static FontCache_Rect FontCache_RenderLeft(FontCache_Font* font, FontCache_Target* dest, float x, float y, FontCache_Scale scale, const char* text) {
    const char* c = text;
    FontCache_Rect srcRect;
    FontCache_Rect dstRect;
    FontCache_Rect dirtyRect = FontCache_MakeRect(x, y, 0, 0);

    FontCache_GlyphData glyph;
    U32 codepoint;

    float destX = x;
    float destY = y;
    float destH;
    float destLineSpacing;
    float destLetterSpacing;

    if (font == NULL) return dirtyRect;

    destH = font->height * scale.y;
    destLineSpacing = font->lineSpacing * scale.y;
    destLetterSpacing = font->letterSpacing * scale.x;

    if (c == NULL || font->glyph_cache_count == 0 || dest == NULL) return dirtyRect;

    int newlineX = x;

    for (; *c != '\0'; c++) {
        if (*c == '\n') {
            destX = newlineX;
            destY += destH + destLineSpacing;
            continue;
        }

        codepoint = FontCache_GetCodepointFromUTF8(&c, 1);  // Increments 'c' to skip the extra UTF-8 bytes
        if (!FontCache_GetGlyphData(font, &glyph, codepoint)) {
            codepoint = ' ';
            if (!FontCache_GetGlyphData(font, &glyph, codepoint)) continue;  // Skip bad characters
        }

        if (codepoint == ' ') {
            destX += glyph.rect.w * scale.x + destLetterSpacing;
            continue;
        }
        /*if(destX >= dest->w)
            continue;
        if(destY >= dest->h)
            continue;*/

        srcRect.x = glyph.rect.x;
        srcRect.y = glyph.rect.y;
        srcRect.w = glyph.rect.w;
        srcRect.h = glyph.rect.h;

        dstRect = fc_render_callback(FontCache_GetGlyphCacheLevel(font, glyph.cache_level), &srcRect, dest, destX, destY, scale.x, scale.y);
        if (dirtyRect.w == 0 || dirtyRect.h == 0)
            dirtyRect = dstRect;
        else
            dirtyRect = FontCache_RectUnion(dirtyRect, dstRect);

        destX += glyph.rect.w * scale.x + destLetterSpacing;
    }

    return dirtyRect;
}

static void set_color_for_all_caches(FontCache_Font* font, METAENGINE_Color color) {
    // TODO: How can I predict which glyph caches are to be used?
    FontCache_Image* img;
    int i;
    int num_levels = FontCache_GetNumCacheLevels(font);
    for (i = 0; i < num_levels; ++i) {
        img = FontCache_GetGlyphCacheLevel(font, i);
        set_color(img, color.r, color.g, color.b, FontCache_GET_ALPHA(color));
    }
}

FontCache_Rect FontCache_Draw(FontCache_Font* font, FontCache_Target* dest, float x, float y, const char* formatted_text, ...) {
    if (formatted_text == NULL || font == NULL) return FontCache_MakeRect(x, y, 0, 0);

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    set_color_for_all_caches(font, font->default_color);

    return FontCache_RenderLeft(font, dest, x, y, FontCache_MakeScale(1, 1), fc_buffer);
}

typedef struct FontCache_StringList {
    char* value;
    struct FontCache_StringList* next;
} FontCache_StringList;

void FontCache_StringListFree(FontCache_StringList* node) {
    // Delete the nodes in order
    while (node != NULL) {
        FontCache_StringList* last = node;
        node = node->next;

        free(last->value);
        free(last);
    }
}

FontCache_StringList** FontCache_StringListPushBack(FontCache_StringList** node, char* value, U8 copy) {
    if (node == NULL) {
        return node;
    }

    // Get to the last node
    while (*node != NULL) {
        node = &(*node)->next;
    }

    *node = (FontCache_StringList*)malloc(sizeof(FontCache_StringList));

    (*node)->value = (copy ? U8_strdup(value) : value);
    (*node)->next = NULL;

    return node;
}

static FontCache_StringList* FontCache_Explode(const char* text, char delimiter) {
    FontCache_StringList* head;
    FontCache_StringList* new_node;
    FontCache_StringList** node;
    const char* start;
    const char* end;
    unsigned int size;
    if (text == NULL) return NULL;

    head = NULL;
    node = &head;

    // Doesn't technically support UTF-8, but it's probably fine, right?
    size = 0;
    start = end = text;
    while (1) {
        if (*end == delimiter || *end == '\0') {
            *node = (FontCache_StringList*)malloc(sizeof(FontCache_StringList));
            new_node = *node;

            new_node->value = (char*)malloc(size + 1);
            memcpy(new_node->value, start, size);
            new_node->value[size] = '\0';

            new_node->next = NULL;

            if (*end == '\0') break;

            node = &((*node)->next);
            start = end + 1;
            size = 0;
        } else
            ++size;

        ++end;
    }

    return head;
}

static FontCache_StringList* FontCache_ExplodeAndKeep(const char* text, char delimiter) {
    FontCache_StringList* head;
    FontCache_StringList* new_node;
    FontCache_StringList** node;
    const char* start;
    const char* end;
    unsigned int size;
    if (text == NULL) return NULL;

    head = NULL;
    node = &head;

    // Doesn't technically support UTF-8, but it's probably fine, right?
    size = 0;
    start = end = text;
    while (1) {
        if (*end == delimiter || *end == '\0') {
            *node = (FontCache_StringList*)malloc(sizeof(FontCache_StringList));
            new_node = *node;

            new_node->value = (char*)malloc(size + 1);
            memcpy(new_node->value, start, size);
            new_node->value[size] = '\0';

            new_node->next = NULL;

            if (*end == '\0') break;

            node = &((*node)->next);
            start = end;
            size = 1;
        } else
            ++size;

        ++end;
    }

    return head;
}

static void FontCache_RenderAlign(FontCache_Font* font, FontCache_Target* dest, float x, float y, int width, FontCache_Scale scale, FontCache_AlignEnum align, const char* text) {
    switch (align) {
        case FontCache_ALIGN_LEFT:
            FontCache_RenderLeft(font, dest, x, y, scale, text);
            break;
        case FontCache_ALIGN_CENTER:
            FontCache_RenderCenter(font, dest, x + width / 2, y, scale, text);
            break;
        case FontCache_ALIGN_RIGHT:
            FontCache_RenderRight(font, dest, x + width, y, scale, text);
            break;
    }
}

static FontCache_StringList* FontCache_GetBufferFitToColumn(FontCache_Font* font, int width, FontCache_Scale scale, U8 keep_newlines) {
    FontCache_StringList* result = NULL;
    FontCache_StringList** current = &result;

    FontCache_StringList *ls, *iter;

    ls = (keep_newlines ? FontCache_ExplodeAndKeep(fc_buffer, '\n') : FontCache_Explode(fc_buffer, '\n'));
    for (iter = ls; iter != NULL; iter = iter->next) {
        char* line = iter->value;

        // If line is too long, then add words one at a time until we go over.
        if (width > 0 && FontCache_GetWidth(font, "%s", line) > width) {
            FontCache_StringList *words, *word_iter;

            words = FontCache_Explode(line, ' ');
            // Skip the first word for the iterator, so there will always be at least one word per line
            line = new_concat(words->value, " ");
            for (word_iter = words->next; word_iter != NULL; word_iter = word_iter->next) {
                char* line_plus_word = new_concat(line, word_iter->value);
                char* word_plus_space = new_concat(word_iter->value, " ");
                if (FontCache_GetWidth(font, "%s", line_plus_word) > width) {
                    current = FontCache_StringListPushBack(current, line, 0);

                    line = word_plus_space;
                } else {
                    replace_concat(&line, word_plus_space);
                    free(word_plus_space);
                }
                free(line_plus_word);
            }
            current = FontCache_StringListPushBack(current, line, 0);
            FontCache_StringListFree(words);
        } else {
            current = FontCache_StringListPushBack(current, line, 0);
            iter->value = NULL;
        }
    }
    FontCache_StringListFree(ls);

    return result;
}

static void FontCache_DrawColumnFromBuffer(FontCache_Font* font, FontCache_Target* dest, FontCache_Rect box, int* total_height, FontCache_Scale scale, FontCache_AlignEnum align) {
    int y = box.y;
    FontCache_StringList *ls, *iter;

    ls = FontCache_GetBufferFitToColumn(font, box.w, scale, 0);
    for (iter = ls; iter != NULL; iter = iter->next) {
        FontCache_RenderAlign(font, dest, box.x, y, box.w, scale, align, iter->value);
        y += FontCache_GetLineHeight(font);
    }
    FontCache_StringListFree(ls);

    if (total_height != NULL) *total_height = y - box.y;
}

FontCache_Rect FontCache_DrawBox(FontCache_Font* font, FontCache_Target* dest, FontCache_Rect box, const char* formatted_text, ...) {
    U8 useClip;
    if (formatted_text == NULL || font == NULL) return FontCache_MakeRect(box.x, box.y, 0, 0);

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    useClip = has_clip(dest);
    FontCache_Rect oldclip, newclip;
    if (useClip) {
        oldclip = get_clip(dest);
        newclip = FontCache_RectIntersect(oldclip, box);
    } else
        newclip = box;

    set_clip(dest, &newclip);

    set_color_for_all_caches(font, font->default_color);

    FontCache_DrawColumnFromBuffer(font, dest, box, NULL, FontCache_MakeScale(1, 1), FontCache_ALIGN_LEFT);

    if (useClip)
        set_clip(dest, &oldclip);
    else
        set_clip(dest, NULL);

    return box;
}

FontCache_Rect FontCache_DrawBoxAlign(FontCache_Font* font, FontCache_Target* dest, FontCache_Rect box, FontCache_AlignEnum align, const char* formatted_text, ...) {
    U8 useClip;
    if (formatted_text == NULL || font == NULL) return FontCache_MakeRect(box.x, box.y, 0, 0);

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    useClip = has_clip(dest);
    FontCache_Rect oldclip, newclip;
    if (useClip) {
        oldclip = get_clip(dest);
        newclip = FontCache_RectIntersect(oldclip, box);
    } else
        newclip = box;
    set_clip(dest, &newclip);

    set_color_for_all_caches(font, font->default_color);

    FontCache_DrawColumnFromBuffer(font, dest, box, NULL, FontCache_MakeScale(1, 1), align);

    if (useClip)
        set_clip(dest, &oldclip);
    else
        set_clip(dest, NULL);

    return box;
}

FontCache_Rect FontCache_DrawBoxScale(FontCache_Font* font, FontCache_Target* dest, FontCache_Rect box, FontCache_Scale scale, const char* formatted_text, ...) {
    U8 useClip;
    if (formatted_text == NULL || font == NULL) return FontCache_MakeRect(box.x, box.y, 0, 0);

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    useClip = has_clip(dest);
    FontCache_Rect oldclip, newclip;
    if (useClip) {
        oldclip = get_clip(dest);
        newclip = FontCache_RectIntersect(oldclip, box);
    } else
        newclip = box;
    set_clip(dest, &newclip);

    set_color_for_all_caches(font, font->default_color);

    FontCache_DrawColumnFromBuffer(font, dest, box, NULL, scale, FontCache_ALIGN_LEFT);

    if (useClip)
        set_clip(dest, &oldclip);
    else
        set_clip(dest, NULL);

    return box;
}

FontCache_Rect FontCache_DrawBoxColor(FontCache_Font* font, FontCache_Target* dest, FontCache_Rect box, METAENGINE_Color color, const char* formatted_text, ...) {
    U8 useClip;
    if (formatted_text == NULL || font == NULL) return FontCache_MakeRect(box.x, box.y, 0, 0);

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    useClip = has_clip(dest);
    FontCache_Rect oldclip, newclip;
    if (useClip) {
        oldclip = get_clip(dest);
        newclip = FontCache_RectIntersect(oldclip, box);
    } else
        newclip = box;
    set_clip(dest, &newclip);

    set_color_for_all_caches(font, color);

    FontCache_DrawColumnFromBuffer(font, dest, box, NULL, FontCache_MakeScale(1, 1), FontCache_ALIGN_LEFT);

    if (useClip)
        set_clip(dest, &oldclip);
    else
        set_clip(dest, NULL);

    return box;
}

FontCache_Rect FontCache_DrawBoxEffect(FontCache_Font* font, FontCache_Target* dest, FontCache_Rect box, FontCache_Effect effect, const char* formatted_text, ...) {
    U8 useClip;
    if (formatted_text == NULL || font == NULL) return FontCache_MakeRect(box.x, box.y, 0, 0);

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    useClip = has_clip(dest);
    FontCache_Rect oldclip, newclip;
    if (useClip) {
        oldclip = get_clip(dest);
        newclip = FontCache_RectIntersect(oldclip, box);
    } else
        newclip = box;
    set_clip(dest, &newclip);

    set_color_for_all_caches(font, effect.color);

    FontCache_DrawColumnFromBuffer(font, dest, box, NULL, effect.scale, effect.alignment);

    if (useClip)
        set_clip(dest, &oldclip);
    else
        set_clip(dest, NULL);

    return box;
}

FontCache_Rect FontCache_DrawColumn(FontCache_Font* font, FontCache_Target* dest, float x, float y, U16 width, const char* formatted_text, ...) {
    FontCache_Rect box = {x, y, width, 0};
    int total_height;

    if (formatted_text == NULL || font == NULL) return FontCache_MakeRect(x, y, 0, 0);

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    set_color_for_all_caches(font, font->default_color);

    FontCache_DrawColumnFromBuffer(font, dest, box, &total_height, FontCache_MakeScale(1, 1), FontCache_ALIGN_LEFT);

    return FontCache_MakeRect(box.x, box.y, width, total_height);
}

FontCache_Rect FontCache_DrawColumnAlign(FontCache_Font* font, FontCache_Target* dest, float x, float y, U16 width, FontCache_AlignEnum align, const char* formatted_text, ...) {
    FontCache_Rect box = {x, y, width, 0};
    int total_height;

    if (formatted_text == NULL || font == NULL) return FontCache_MakeRect(x, y, 0, 0);

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    set_color_for_all_caches(font, font->default_color);

    switch (align) {
        case FontCache_ALIGN_CENTER:
            box.x -= width / 2;
            break;
        case FontCache_ALIGN_RIGHT:
            box.x -= width;
            break;
        default:
            break;
    }

    FontCache_DrawColumnFromBuffer(font, dest, box, &total_height, FontCache_MakeScale(1, 1), align);

    return FontCache_MakeRect(box.x, box.y, width, total_height);
}

FontCache_Rect FontCache_DrawColumnScale(FontCache_Font* font, FontCache_Target* dest, float x, float y, U16 width, FontCache_Scale scale, const char* formatted_text, ...) {
    FontCache_Rect box = {x, y, width, 0};
    int total_height;

    if (formatted_text == NULL || font == NULL) return FontCache_MakeRect(x, y, 0, 0);

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    set_color_for_all_caches(font, font->default_color);

    FontCache_DrawColumnFromBuffer(font, dest, box, &total_height, scale, FontCache_ALIGN_LEFT);

    return FontCache_MakeRect(box.x, box.y, width, total_height);
}

FontCache_Rect FontCache_DrawColumnColor(FontCache_Font* font, FontCache_Target* dest, float x, float y, U16 width, METAENGINE_Color color, const char* formatted_text, ...) {
    FontCache_Rect box = {x, y, width, 0};
    int total_height;

    if (formatted_text == NULL || font == NULL) return FontCache_MakeRect(x, y, 0, 0);

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    set_color_for_all_caches(font, color);

    FontCache_DrawColumnFromBuffer(font, dest, box, &total_height, FontCache_MakeScale(1, 1), FontCache_ALIGN_LEFT);

    return FontCache_MakeRect(box.x, box.y, width, total_height);
}

FontCache_Rect FontCache_DrawColumnEffect(FontCache_Font* font, FontCache_Target* dest, float x, float y, U16 width, FontCache_Effect effect, const char* formatted_text, ...) {
    FontCache_Rect box = {x, y, width, 0};
    int total_height;

    if (formatted_text == NULL || font == NULL) return FontCache_MakeRect(x, y, 0, 0);

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    set_color_for_all_caches(font, effect.color);

    switch (effect.alignment) {
        case FontCache_ALIGN_CENTER:
            box.x -= width / 2;
            break;
        case FontCache_ALIGN_RIGHT:
            box.x -= width;
            break;
        default:
            break;
    }

    FontCache_DrawColumnFromBuffer(font, dest, box, &total_height, effect.scale, effect.alignment);

    return FontCache_MakeRect(box.x, box.y, width, total_height);
}

static FontCache_Rect FontCache_RenderCenter(FontCache_Font* font, FontCache_Target* dest, float x, float y, FontCache_Scale scale, const char* text) {
    FontCache_Rect result = {x, y, 0, 0};
    if (text == NULL || font == NULL) return result;

    char* str = U8_strdup(text);
    char* del = str;
    char* c;

    // Go through str, when you find a \n, replace it with \0 and print it
    // then move down, back, and continue.
    for (c = str; *c != '\0';) {
        if (*c == '\n') {
            *c = '\0';
            result = FontCache_RectUnion(FontCache_RenderLeft(font, dest, x - scale.x * FontCache_GetWidth(font, "%s", str) / 2.0f, y, scale, str), result);
            *c = '\n';
            c++;
            str = c;
            y += scale.y * font->height;
        } else
            c++;
    }

    result = FontCache_RectUnion(FontCache_RenderLeft(font, dest, x - scale.x * FontCache_GetWidth(font, "%s", str) / 2.0f, y, scale, str), result);

    free(del);
    return result;
}

static FontCache_Rect FontCache_RenderRight(FontCache_Font* font, FontCache_Target* dest, float x, float y, FontCache_Scale scale, const char* text) {
    FontCache_Rect result = {x, y, 0, 0};
    if (text == NULL || font == NULL) return result;

    char* str = U8_strdup(text);
    char* del = str;
    char* c;

    for (c = str; *c != '\0';) {
        if (*c == '\n') {
            *c = '\0';
            result = FontCache_RectUnion(FontCache_RenderLeft(font, dest, x - scale.x * FontCache_GetWidth(font, "%s", str), y, scale, str), result);
            *c = '\n';
            c++;
            str = c;
            y += scale.y * font->height;
        } else
            c++;
    }

    result = FontCache_RectUnion(FontCache_RenderLeft(font, dest, x - scale.x * FontCache_GetWidth(font, "%s", str), y, scale, str), result);

    free(del);
    return result;
}

FontCache_Rect FontCache_DrawScale(FontCache_Font* font, FontCache_Target* dest, float x, float y, FontCache_Scale scale, const char* formatted_text, ...) {
    if (formatted_text == NULL || font == NULL) return FontCache_MakeRect(x, y, 0, 0);

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    set_color_for_all_caches(font, font->default_color);

    return FontCache_RenderLeft(font, dest, x, y, scale, fc_buffer);
}

FontCache_Rect FontCache_DrawAlign(FontCache_Font* font, FontCache_Target* dest, float x, float y, FontCache_AlignEnum align, const char* formatted_text, ...) {
    if (formatted_text == NULL || font == NULL) return FontCache_MakeRect(x, y, 0, 0);

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    set_color_for_all_caches(font, font->default_color);

    FontCache_Rect result;
    switch (align) {
        case FontCache_ALIGN_LEFT:
            result = FontCache_RenderLeft(font, dest, x, y, FontCache_MakeScale(1, 1), fc_buffer);
            break;
        case FontCache_ALIGN_CENTER:
            result = FontCache_RenderCenter(font, dest, x, y, FontCache_MakeScale(1, 1), fc_buffer);
            break;
        case FontCache_ALIGN_RIGHT:
            result = FontCache_RenderRight(font, dest, x, y, FontCache_MakeScale(1, 1), fc_buffer);
            break;
        default:
            result = FontCache_MakeRect(x, y, 0, 0);
            break;
    }

    return result;
}

FontCache_Rect FontCache_DrawColor(FontCache_Font* font, FontCache_Target* dest, float x, float y, METAENGINE_Color color, const char* formatted_text, ...) {
    if (formatted_text == NULL || font == NULL) return FontCache_MakeRect(x, y, 0, 0);

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    set_color_for_all_caches(font, color);

    return FontCache_RenderLeft(font, dest, x, y, FontCache_MakeScale(1, 1), fc_buffer);
}

FontCache_Rect FontCache_DrawEffect(FontCache_Font* font, FontCache_Target* dest, float x, float y, FontCache_Effect effect, const char* formatted_text, ...) {
    if (formatted_text == NULL || font == NULL) return FontCache_MakeRect(x, y, 0, 0);

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    set_color_for_all_caches(font, effect.color);

    FontCache_Rect result;
    switch (effect.alignment) {
        case FontCache_ALIGN_LEFT:
            result = FontCache_RenderLeft(font, dest, x, y, effect.scale, fc_buffer);
            break;
        case FontCache_ALIGN_CENTER:
            result = FontCache_RenderCenter(font, dest, x, y, effect.scale, fc_buffer);
            break;
        case FontCache_ALIGN_RIGHT:
            result = FontCache_RenderRight(font, dest, x, y, effect.scale, fc_buffer);
            break;
        default:
            result = FontCache_MakeRect(x, y, 0, 0);
            break;
    }

    return result;
}

// Getters

FontCache_FilterEnum FontCache_GetFilterMode(FontCache_Font* font) {
    if (font == NULL) return FontCache_FILTER_NEAREST;

    return font->filter;
}

U16 FontCache_GetLineHeight(FontCache_Font* font) {
    if (font == NULL) return 0;

    return font->height;
}

U16 FontCache_GetHeight(FontCache_Font* font, const char* formatted_text, ...) {
    if (formatted_text == NULL || font == NULL) return 0;

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    U16 numLines = 1;
    const char* c;

    for (c = fc_buffer; *c != '\0'; c++) {
        if (*c == '\n') numLines++;
    }

    //   Actual height of letter region + line spacing
    return font->height * numLines + font->lineSpacing * (numLines - 1);  // height*numLines;
}

U16 FontCache_GetWidth(FontCache_Font* font, const char* formatted_text, ...) {
    if (formatted_text == NULL || font == NULL) return 0;

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    const char* c;
    U16 width = 0;
    U16 bigWidth = 0;  // Allows for multi-line strings

    for (c = fc_buffer; *c != '\0'; c++) {
        if (*c == '\n') {
            bigWidth = bigWidth >= width ? bigWidth : width;
            width = 0;
            continue;
        }

        FontCache_GlyphData glyph;
        U32 codepoint = FontCache_GetCodepointFromUTF8(&c, 1);
        if (FontCache_GetGlyphData(font, &glyph, codepoint) || FontCache_GetGlyphData(font, &glyph, ' ')) width += glyph.rect.w;
    }
    bigWidth = bigWidth >= width ? bigWidth : width;

    return bigWidth;
}

// If width == -1, use no width limit
FontCache_Rect FontCache_GetCharacterOffset(FontCache_Font* font, U16 position_index, int column_width, const char* formatted_text, ...) {
    FontCache_Rect result = {0, 0, 1, FontCache_GetLineHeight(font)};
    FontCache_StringList *ls, *iter;
    int num_lines = 0;
    U8 done = 0;

    if (formatted_text == NULL || column_width == 0 || position_index == 0 || font == NULL) return result;

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    ls = FontCache_GetBufferFitToColumn(font, column_width, FontCache_MakeScale(1, 1), 1);
    for (iter = ls; iter != NULL;) {
        char* line;
        int i = 0;
        FontCache_StringList* next_iter = iter->next;

        ++num_lines;
        for (line = iter->value; line != NULL && *line != '\0'; line = (char*)U8_next(line)) {
            ++i;
            --position_index;
            if (position_index == 0) {
                // FIXME: Doesn't handle box-wrapped newlines correctly
                line = (char*)U8_next(line);
                line[0] = '\0';
                result.x = FontCache_GetWidth(font, "%s", iter->value);
                done = 1;
                break;
            }
        }
        if (done) break;

        // Prevent line wrapping if there are no more lines
        if (next_iter == NULL && !done) result.x = FontCache_GetWidth(font, "%s", iter->value);
        iter = next_iter;
    }
    FontCache_StringListFree(ls);

    if (num_lines > 1) {
        result.y = (num_lines - 1) * FontCache_GetLineHeight(font);
    }

    return result;
}

U16 FontCache_GetColumnHeight(FontCache_Font* font, U16 width, const char* formatted_text, ...) {
    int y = 0;

    FontCache_StringList *ls, *iter;

    if (font == NULL) return 0;

    if (formatted_text == NULL || width == 0) return font->height;

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    ls = FontCache_GetBufferFitToColumn(font, width, FontCache_MakeScale(1, 1), 0);
    for (iter = ls; iter != NULL; iter = iter->next) {
        y += FontCache_GetLineHeight(font);
    }
    FontCache_StringListFree(ls);

    return y;
}

static int FontCache_GetAscentFromCodepoint(FontCache_Font* font, U32 codepoint) {
    FontCache_GlyphData glyph;

    if (font == NULL) return 0;

    // FIXME: Store ascent so we can return it here
    FontCache_GetGlyphData(font, &glyph, codepoint);
    return glyph.rect.h;
}

static int FontCache_GetDescentFromCodepoint(FontCache_Font* font, U32 codepoint) {
    FontCache_GlyphData glyph;

    if (font == NULL) return 0;

    // FIXME: Store descent so we can return it here
    FontCache_GetGlyphData(font, &glyph, codepoint);
    return glyph.rect.h;
}

int FontCache_GetAscent(FontCache_Font* font, const char* formatted_text, ...) {
    U32 codepoint;
    int max, ascent;
    const char* c;

    if (font == NULL) return 0;

    if (formatted_text == NULL) return font->ascent;

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    max = 0;
    c = fc_buffer;

    while (*c != '\0') {
        codepoint = FontCache_GetCodepointFromUTF8(&c, 1);
        if (codepoint != 0) {
            ascent = FontCache_GetAscentFromCodepoint(font, codepoint);
            if (ascent > max) max = ascent;
        }
        ++c;
    }
    return max;
}

int FontCache_GetDescent(FontCache_Font* font, const char* formatted_text, ...) {
    U32 codepoint;
    int max, descent;
    const char* c;

    if (font == NULL) return 0;

    if (formatted_text == NULL) return font->descent;

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    max = 0;
    c = fc_buffer;

    while (*c != '\0') {
        codepoint = FontCache_GetCodepointFromUTF8(&c, 1);
        if (codepoint != 0) {
            descent = FontCache_GetDescentFromCodepoint(font, codepoint);
            if (descent > max) max = descent;
        }
        ++c;
    }
    return max;
}

int FontCache_GetBaseline(FontCache_Font* font) {
    if (font == NULL) return 0;

    return font->baseline;
}

int FontCache_GetSpacing(FontCache_Font* font) {
    if (font == NULL) return 0;

    return font->letterSpacing;
}

int FontCache_GetLineSpacing(FontCache_Font* font) {
    if (font == NULL) return 0;

    return font->lineSpacing;
}

U16 FontCache_GetMaxWidth(FontCache_Font* font) {
    if (font == NULL) return 0;

    return font->maxWidth;
}

METAENGINE_Color FontCache_GetDefaultColor(FontCache_Font* font) {
    if (font == NULL) {
        METAENGINE_Color c = {0, 0, 0, 255};
        return c;
    }

    return font->default_color;
}

FontCache_Rect FontCache_GetBounds(FontCache_Font* font, float x, float y, FontCache_AlignEnum align, FontCache_Scale scale, const char* formatted_text, ...) {
    FontCache_Rect result = {x, y, 0, 0};

    if (formatted_text == NULL) return result;

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    result.w = FontCache_GetWidth(font, "%s", fc_buffer) * scale.x;
    result.h = FontCache_GetHeight(font, "%s", fc_buffer) * scale.y;

    switch (align) {
        case FontCache_ALIGN_LEFT:
            break;
        case FontCache_ALIGN_CENTER:
            result.x -= result.w / 2;
            break;
        case FontCache_ALIGN_RIGHT:
            result.x -= result.w;
            break;
        default:
            break;
    }

    return result;
}

U8 FontCache_InRect(float x, float y, FontCache_Rect input_rect) { return (input_rect.x <= x && x <= input_rect.x + input_rect.w && input_rect.y <= y && y <= input_rect.y + input_rect.h); }

// TODO: Make it work with alignment
U16 FontCache_GetPositionFromOffset(FontCache_Font* font, float x, float y, int column_width, FontCache_AlignEnum align, const char* formatted_text, ...) {
    FontCache_StringList *ls, *iter;
    U8 done = 0;
    int height = FontCache_GetLineHeight(font);
    U16 position = 0;
    int current_x = 0;
    int current_y = 0;
    FontCache_GlyphData glyph_data;

    if (formatted_text == NULL || column_width == 0 || font == NULL) return 0;

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    ls = FontCache_GetBufferFitToColumn(font, column_width, FontCache_MakeScale(1, 1), 1);
    for (iter = ls; iter != NULL; iter = iter->next) {
        char* line;

        for (line = iter->value; line != NULL && *line != '\0'; line = (char*)U8_next(line)) {
            if (FontCache_GetGlyphData(font, &glyph_data, FontCache_GetCodepointFromUTF8((const char**)&line, 0))) {
                if (FontCache_InRect(x, y, FontCache_MakeRect(current_x, current_y, glyph_data.rect.w, glyph_data.rect.h))) {
                    done = 1;
                    break;
                }

                current_x += glyph_data.rect.w;
            }
            position++;
        }
        if (done) break;

        current_x = 0;
        current_y += height;
        if (y < current_y) break;
    }
    FontCache_StringListFree(ls);

    return position;
}

int FontCache_GetWrappedText(FontCache_Font* font, char* result, int max_result_size, U16 width, const char* formatted_text, ...) {
    FontCache_StringList *ls, *iter;

    if (font == NULL) return 0;

    if (formatted_text == NULL || width == 0) return 0;

    FontCache_EXTRACT_VARARGS(fc_buffer, formatted_text);

    ls = FontCache_GetBufferFitToColumn(font, width, FontCache_MakeScale(1, 1), 0);
    int size_so_far = 0;
    int size_remaining = max_result_size - 1;  // reserve for \0
    for (iter = ls; iter != NULL && size_remaining > 0; iter = iter->next) {
        // Copy as much of this line as we can
        int len = strlen(iter->value);
        int num_bytes = FontCache_MIN(len, size_remaining);
        memcpy(&result[size_so_far], iter->value, num_bytes);
        size_so_far += num_bytes;

        // If there's another line, add newline character
        if (size_remaining > 0 && iter->next != NULL) {
            --size_remaining;
            result[size_so_far] = '\n';
            ++size_so_far;
        }
    }
    FontCache_StringListFree(ls);

    result[size_so_far] = '\0';

    return size_so_far;
}

// Setters

void FontCache_SetFilterMode(FontCache_Font* font, FontCache_FilterEnum filter) {
    if (font == NULL) return;

    if (font->filter != filter) {
        font->filter = filter;

        // Update each texture to use this filter mode
        {
            int i;
            R_FilterEnum gpu_filter = R_FILTER_NEAREST;
            if (FontCache_GetFilterMode(font) == FontCache_FILTER_LINEAR) gpu_filter = R_FILTER_LINEAR;

            for (i = 0; i < font->glyph_cache_count; ++i) {
                R_SetImageFilter(font->glyph_cache[i], gpu_filter);
            }
        }
    }
}

void FontCache_SetSpacing(FontCache_Font* font, int LetterSpacing) {
    if (font == NULL) return;

    font->letterSpacing = LetterSpacing;
}

void FontCache_SetLineSpacing(FontCache_Font* font, int LineSpacing) {
    if (font == NULL) return;

    font->lineSpacing = LineSpacing;
}

void FontCache_SetDefaultColor(FontCache_Font* font, METAENGINE_Color color) {
    if (font == NULL) return;

    font->default_color = color;
}
