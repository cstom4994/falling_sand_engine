

#ifndef _FONTCACHE_H_
#define _FONTCACHE_H_

#include <stdarg.h>

#include "engine/renderer/renderer_gpu.h"
#include "engine/sdl_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

// Let's pretend this exists...
#define TTF_STYLE_OUTLINE 16

#define FontCache_Rect metadot_rect
#define FontCache_Target R_Target
#define FontCache_Image R_Image

// SDL_FontCache types

typedef enum { FontCache_ALIGN_LEFT, FontCache_ALIGN_CENTER, FontCache_ALIGN_RIGHT } FontCache_AlignEnum;

typedef enum { FontCache_FILTER_NEAREST, FontCache_FILTER_LINEAR } FontCache_FilterEnum;

typedef struct FontCache_Scale {
    float x;
    float y;

} FontCache_Scale;

typedef struct FontCache_Effect {
    FontCache_AlignEnum alignment;
    FontCache_Scale scale;
    METAENGINE_Color color;

} FontCache_Effect;

// Opaque type
typedef struct FontCache_Font FontCache_Font;

typedef struct FontCache_GlyphData {
    C_Rect rect;
    int cache_level;

} FontCache_GlyphData;

// Object creation

FontCache_Rect FontCache_MakeRect(float x, float y, float w, float h);

FontCache_Scale FontCache_MakeScale(float x, float y);

METAENGINE_Color FontCache_MakeColor(U8 r, U8 g, U8 b, U8 a);

FontCache_Effect FontCache_MakeEffect(FontCache_AlignEnum alignment, FontCache_Scale scale, METAENGINE_Color color);

FontCache_GlyphData FontCache_MakeGlyphData(int cache_level, Sint16 x, Sint16 y, U16 w, U16 h);

// Font object

FontCache_Font* FontCache_CreateFont(void);

U8 FontCache_LoadFont(FontCache_Font* font, const char* filename_ttf, U32 pointSize, METAENGINE_Color color, int style);

U8 FontCache_LoadFontFromTTF(FontCache_Font* font, TTF_Font* ttf, METAENGINE_Color color);

U8 FontCache_LoadFont_RW(FontCache_Font* font, SDL_RWops* file_rwops_ttf, U8 own_rwops, U32 pointSize, METAENGINE_Color color, int style);

void FontCache_ClearFont(FontCache_Font* font);

void FontCache_FreeFont(FontCache_Font* font);

// Built-in loading strings

char* FontCache_GetStringASCII(void);

char* FontCache_GetStringLatin1(void);

char* FontCache_GetStringASCII_Latin1(void);

// UTF-8 to SDL_FontCache codepoint conversion

/*!
Returns the U32 codepoint (not UTF-32) parsed from the given UTF-8 string.
\param c A pointer to a string of proper UTF-8 character values.
\param advance_pointer If true, the source pointer will be incremented to skip the extra bytes from multibyte codepoints.
*/
U32 FontCache_GetCodepointFromUTF8(const char** c, U8 advance_pointer);

/*!
Parses the given codepoint and stores the UTF-8 bytes in 'result'.  The result is NULL terminated.
\param result A memory buffer for the UTF-8 values.  Must be at least 5 bytes long.
\param codepoint The U32 codepoint to parse (not UTF-32).
*/
void FontCache_GetUTF8FromCodepoint(char* result, U32 codepoint);

// UTF-8 string operations

/*! Allocates a new string of 'size' bytes that is already NULL-terminated.  The NULL byte counts toward the size limit, as usual.  Returns NULL if size is 0. */
char* U8_alloc(unsigned int size);

/*! Deallocates the given string. */
void U8_free(char* string);

/*! Allocates a copy of the given string. */
char* U8_strdup(const char* string);

/*! Returns the number of UTF-8 characters in the given string. */
int U8_strlen(const char* string);

/*! Returns the number of bytes in the UTF-8 multibyte character pointed at by 'character'. */
int U8_charsize(const char* character);

/*! Copies the source multibyte character into the given buffer without overrunning it.  Returns 0 on failure. */
int U8_charcpy(char* buffer, const char* source, int buffer_size);

/*! Returns a pointer to the next UTF-8 character. */
const char* U8_next(const char* string);

/*! Inserts a UTF-8 string into 'string' at the given position.  Use a position of -1 to append.  Returns 0 when unable to insert the string. */
int U8_strinsert(char* string, int position, const char* source, int max_bytes);

/*! Erases the UTF-8 character at the given position, moving the subsequent characters down. */
void U8_strdel(char* string, int position);

// Internal settings

/*! Sets the string from which to load the initial glyphs.  Use this if you need upfront loading for any reason (such as lack of render-target support). */
void FontCache_SetLoadingString(FontCache_Font* font, const char* string);

/*! Returns the size of the internal buffer which is used for unpacking variadic text data.  This buffer is shared by all FontCache_Fonts. */
unsigned int FontCache_GetBufferSize(void);

/*! Changes the size of the internal buffer which is used for unpacking variadic text data.  This buffer is shared by all FontCache_Fonts. */
void FontCache_SetBufferSize(unsigned int size);

void FontCache_SetRenderCallback(FontCache_Rect (*callback)(FontCache_Image* src, FontCache_Rect* srcrect, FontCache_Target* dest, float x, float y, float xscale, float yscale));

FontCache_Rect FontCache_DefaultRenderCallback(FontCache_Image* src, FontCache_Rect* srcrect, FontCache_Target* dest, float x, float y, float xscale, float yscale);

// Custom caching

/*! Returns the number of cache levels that are active. */
int FontCache_GetNumCacheLevels(FontCache_Font* font);

/*! Returns the cache source texture at the given cache level. */
FontCache_Image* FontCache_GetGlyphCacheLevel(FontCache_Font* font, int cache_level);

// TODO: Specify ownership of the texture (should be shareable)
/*! Sets a cache source texture for rendering.  New cache levels must be sequential. */
U8 FontCache_SetGlyphCacheLevel(FontCache_Font* font, int cache_level, FontCache_Image* cache_texture);

/*! Copies the given surface to the given cache level as a texture.  New cache levels must be sequential. */
U8 FontCache_UploadGlyphCache(FontCache_Font* font, int cache_level, C_Surface* data_surface);

/*! Returns the number of codepoints that are stored in the font's glyph data map. */
unsigned int FontCache_GetNumCodepoints(FontCache_Font* font);

/*! Copies the stored codepoints into the given array. */
void FontCache_GetCodepoints(FontCache_Font* font, U32* result);

/*! Stores the glyph data for the given codepoint in 'result'.  Returns 0 if the codepoint was not found in the cache. */
U8 FontCache_GetGlyphData(FontCache_Font* font, FontCache_GlyphData* result, U32 codepoint);

/*! Sets the glyph data for the given codepoint.  Duplicates are not checked.  Returns a pointer to the stored data. */
FontCache_GlyphData* FontCache_SetGlyphData(FontCache_Font* font, U32 codepoint, FontCache_GlyphData glyph_data);

// Rendering

FontCache_Rect FontCache_Draw(FontCache_Font* font, FontCache_Target* dest, float x, float y, const char* formatted_text, ...);
FontCache_Rect FontCache_DrawAlign(FontCache_Font* font, FontCache_Target* dest, float x, float y, FontCache_AlignEnum align, const char* formatted_text, ...);
FontCache_Rect FontCache_DrawScale(FontCache_Font* font, FontCache_Target* dest, float x, float y, FontCache_Scale scale, const char* formatted_text, ...);
FontCache_Rect FontCache_DrawColor(FontCache_Font* font, FontCache_Target* dest, float x, float y, METAENGINE_Color color, const char* formatted_text, ...);
FontCache_Rect FontCache_DrawEffect(FontCache_Font* font, FontCache_Target* dest, float x, float y, FontCache_Effect effect, const char* formatted_text, ...);

FontCache_Rect FontCache_DrawBox(FontCache_Font* font, FontCache_Target* dest, FontCache_Rect box, const char* formatted_text, ...);
FontCache_Rect FontCache_DrawBoxAlign(FontCache_Font* font, FontCache_Target* dest, FontCache_Rect box, FontCache_AlignEnum align, const char* formatted_text, ...);
FontCache_Rect FontCache_DrawBoxScale(FontCache_Font* font, FontCache_Target* dest, FontCache_Rect box, FontCache_Scale scale, const char* formatted_text, ...);
FontCache_Rect FontCache_DrawBoxColor(FontCache_Font* font, FontCache_Target* dest, FontCache_Rect box, METAENGINE_Color color, const char* formatted_text, ...);
FontCache_Rect FontCache_DrawBoxEffect(FontCache_Font* font, FontCache_Target* dest, FontCache_Rect box, FontCache_Effect effect, const char* formatted_text, ...);

FontCache_Rect FontCache_DrawColumn(FontCache_Font* font, FontCache_Target* dest, float x, float y, U16 width, const char* formatted_text, ...);
FontCache_Rect FontCache_DrawColumnAlign(FontCache_Font* font, FontCache_Target* dest, float x, float y, U16 width, FontCache_AlignEnum align, const char* formatted_text, ...);
FontCache_Rect FontCache_DrawColumnScale(FontCache_Font* font, FontCache_Target* dest, float x, float y, U16 width, FontCache_Scale scale, const char* formatted_text, ...);
FontCache_Rect FontCache_DrawColumnColor(FontCache_Font* font, FontCache_Target* dest, float x, float y, U16 width, METAENGINE_Color color, const char* formatted_text, ...);
FontCache_Rect FontCache_DrawColumnEffect(FontCache_Font* font, FontCache_Target* dest, float x, float y, U16 width, FontCache_Effect effect, const char* formatted_text, ...);

// Getters

FontCache_FilterEnum FontCache_GetFilterMode(FontCache_Font* font);
U16 FontCache_GetLineHeight(FontCache_Font* font);
U16 FontCache_GetHeight(FontCache_Font* font, const char* formatted_text, ...);
U16 FontCache_GetWidth(FontCache_Font* font, const char* formatted_text, ...);

// Returns a 1-pixel wide box in front of the character in the given position (index)
FontCache_Rect FontCache_GetCharacterOffset(FontCache_Font* font, U16 position_index, int column_width, const char* formatted_text, ...);
U16 FontCache_GetColumnHeight(FontCache_Font* font, U16 width, const char* formatted_text, ...);

int FontCache_GetAscent(FontCache_Font* font, const char* formatted_text, ...);
int FontCache_GetDescent(FontCache_Font* font, const char* formatted_text, ...);
int FontCache_GetBaseline(FontCache_Font* font);
int FontCache_GetSpacing(FontCache_Font* font);
int FontCache_GetLineSpacing(FontCache_Font* font);
U16 FontCache_GetMaxWidth(FontCache_Font* font);
METAENGINE_Color FontCache_GetDefaultColor(FontCache_Font* font);

FontCache_Rect FontCache_GetBounds(FontCache_Font* font, float x, float y, FontCache_AlignEnum align, FontCache_Scale scale, const char* formatted_text, ...);

U8 FontCache_InRect(float x, float y, FontCache_Rect input_rect);
// Given an offset (x,y) from the text draw position (the upper-left corner), returns the character position (UTF-8 index)
U16 FontCache_GetPositionFromOffset(FontCache_Font* font, float x, float y, int column_width, FontCache_AlignEnum align, const char* formatted_text, ...);

// Returns the number of characters in the new wrapped text written into `result`.
int FontCache_GetWrappedText(FontCache_Font* font, char* result, int max_result_size, U16 width, const char* formatted_text, ...);

// Setters

void FontCache_SetFilterMode(FontCache_Font* font, FontCache_FilterEnum filter);
void FontCache_SetSpacing(FontCache_Font* font, int LetterSpacing);
void FontCache_SetLineSpacing(FontCache_Font* font, int LineSpacing);
void FontCache_SetDefaultColor(FontCache_Font* font, METAENGINE_Color color);

#ifdef __cplusplus
}
#endif

#endif
