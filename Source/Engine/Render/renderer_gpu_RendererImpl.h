#ifndef _METAENGINE_Render_RENDERERIMPL_H__
#define _METAENGINE_Render_RENDERERIMPL_H__

#include "renderer_gpu.h"

// Internal API for managing window mappings
void METAENGINE_Render_AddWindowMapping(METAENGINE_Render_Target *target);
void METAENGINE_Render_RemoveWindowMapping(Uint32 windowID);
void METAENGINE_Render_RemoveWindowMappingByTarget(METAENGINE_Render_Target *target);

/*! Private implementation of renderer members. */
typedef struct METAENGINE_Render_RendererImpl
{
    /*! \see METAENGINE_Render_Init()
	 *  \see METAENGINE_Render_InitRenderer()
	 *  \see METAENGINE_Render_InitRendererByID()
	 */
    METAENGINE_Render_Target *(*Init)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_RendererID renderer_request, Uint16 w, Uint16 h, METAENGINE_Render_WindowFlagEnum SDL_flags);

    METAENGINE_Render_Target *(*CreateTargetFromWindow)(METAENGINE_Render_Renderer *renderer, Uint32 windowID, METAENGINE_Render_Target *target);
    bool (*SetActiveTarget)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);
    METAENGINE_Render_Target *(*CreateAliasTarget)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);
    void (*MakeCurrent)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, Uint32 windowID);
    void (*SetAsCurrent)(METAENGINE_Render_Renderer *renderer);// Sets up this renderer to act as the current renderer.  Called automatically by METAENGINE_Render_SetCurrentRenderer().
    void (*ResetRendererState)(METAENGINE_Render_Renderer *renderer);
    bool (*AddDepthBuffer)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);
    bool (*SetWindowResolution)(METAENGINE_Render_Renderer *renderer, Uint16 w, Uint16 h);
    void (*SetVirtualResolution)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, Uint16 w, Uint16 h);
    void (*UnsetVirtualResolution)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);
    void (*Quit)(METAENGINE_Render_Renderer *renderer);
    bool (*SetFullscreen)(METAENGINE_Render_Renderer *renderer, bool enable_fullscreen, bool use_desktop_resolution);
    METAENGINE_Render_Camera (*SetCamera)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, METAENGINE_Render_Camera *cam);
    METAENGINE_Render_Image *(*CreateImage)(METAENGINE_Render_Renderer *renderer, Uint16 w, Uint16 h, METAENGINE_Render_FormatEnum format);
    METAENGINE_Render_Image *(*CreateImageUsingTexture)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_TextureHandle handle, bool take_ownership);
    METAENGINE_Render_Image *(*CreateAliasImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image);
    bool (*SaveImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, const char *filename, METAENGINE_Render_FileFormatEnum format);
    METAENGINE_Render_Image *(*CopyImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image);
    void (*UpdateImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_Rect *image_rect, SDL_Surface *surface, METAENGINE_Render_Rect *surface_rect);
    void (*UpdateImageBytes)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_Rect *image_rect, const unsigned char *bytes, int bytes_per_row);
    bool (*ReplaceImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, SDL_Surface *surface, METAENGINE_Render_Rect *surface_rect);
    METAENGINE_Render_Image *(*CopyImageFromSurface)(METAENGINE_Render_Renderer *renderer, SDL_Surface *surface, METAENGINE_Render_Rect *surface_rect);
    METAENGINE_Render_Image *(*CopyImageFromTarget)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);
    SDL_Surface *(*CopySurfaceFromTarget)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);
    SDL_Surface *(*CopySurfaceFromImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image);
    void (*FreeImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image);
    METAENGINE_Render_Target *(*GetTarget)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image);
    void (*FreeTarget)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);
    void (*Blit)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect, METAENGINE_Render_Target *target, float x, float y);
    void (*BlitRotate)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect, METAENGINE_Render_Target *target, float x, float y, float degrees);
    void (*BlitScale)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect, METAENGINE_Render_Target *target, float x, float y, float scaleX, float scaleY);
    void (*BlitTransform)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect, METAENGINE_Render_Target *target, float x, float y, float degrees, float scaleX, float scaleY);
    void (*BlitTransformX)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect, METAENGINE_Render_Target *target, float x, float y, float pivot_x, float pivot_y, float degrees, float scaleX, float scaleY);
    void (*PrimitiveBatchV)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_Target *target, METAENGINE_Render_PrimitiveEnum primitive_type, unsigned short num_vertices, void *values, unsigned int num_indices, unsigned short *indices, METAENGINE_Render_BatchFlagEnum flags);
    void (*GenerateMipmaps)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image);
    METAENGINE_Render_Rect (*SetClip)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, Sint16 x, Sint16 y, Uint16 w, Uint16 h);
    void (*UnsetClip)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);
    SDL_Color (*GetPixel)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, Sint16 x, Sint16 y);
    void (*SetImageFilter)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_FilterEnum filter);
    void (*SetWrapMode)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_WrapEnum wrap_mode_x, METAENGINE_Render_WrapEnum wrap_mode_y);
    METAENGINE_Render_TextureHandle (*GetTextureHandle)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image);
    void (*ClearRGBA)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    void (*FlushBlitBuffer)(METAENGINE_Render_Renderer *renderer);
    void (*Flip)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);
    Uint32 (*CreateShaderProgram)(METAENGINE_Render_Renderer *renderer);
    void (*FreeShaderProgram)(METAENGINE_Render_Renderer *renderer, Uint32 program_object);
    Uint32 (*CompileShader_RW)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_ShaderEnum shader_type, SDL_RWops *shader_source, bool free_rwops);
    Uint32 (*CompileShader)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_ShaderEnum shader_type, const char *shader_source);
    void (*FreeShader)(METAENGINE_Render_Renderer *renderer, Uint32 shader_object);
    void (*AttachShader)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, Uint32 shader_object);
    void (*DetachShader)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, Uint32 shader_object);
    bool (*LinkShaderProgram)(METAENGINE_Render_Renderer *renderer, Uint32 program_object);
    void (*ActivateShaderProgram)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, METAENGINE_Render_ShaderBlock *block);
    void (*DeactivateShaderProgram)(METAENGINE_Render_Renderer *renderer);
    const char *(*GetShaderMessage)(METAENGINE_Render_Renderer *renderer);
    int (*GetAttributeLocation)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, const char *attrib_name);
    int (*GetUniformLocation)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, const char *uniform_name);
    METAENGINE_Render_ShaderBlock (*LoadShaderBlock)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, const char *position_name, const char *texcoord_name, const char *color_name, const char *modelViewMatrix_name);
    void (*SetShaderBlock)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_ShaderBlock block);
    void (*SetShaderImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, int location, int image_unit);
    void (*GetUniformiv)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, int location, int *values);
    void (*SetUniformi)(METAENGINE_Render_Renderer *renderer, int location, int value);
    void (*SetUniformiv)(METAENGINE_Render_Renderer *renderer, int location, int num_elements_per_value, int num_values, int *values);
    void (*GetUniformuiv)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, int location, unsigned int *values);
    void (*SetUniformui)(METAENGINE_Render_Renderer *renderer, int location, unsigned int value);
    void (*SetUniformuiv)(METAENGINE_Render_Renderer *renderer, int location, int num_elements_per_value, int num_values, unsigned int *values);
    void (*GetUniformfv)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, int location, float *values);
    void (*SetUniformf)(METAENGINE_Render_Renderer *renderer, int location, float value);
    void (*SetUniformfv)(METAENGINE_Render_Renderer *renderer, int location, int num_elements_per_value, int num_values, float *values);
    void (*SetUniformMatrixfv)(METAENGINE_Render_Renderer *renderer, int location, int num_matrices, int num_rows, int num_columns, bool transpose, float *values);
    void (*SetAttributef)(METAENGINE_Render_Renderer *renderer, int location, float value);
    void (*SetAttributei)(METAENGINE_Render_Renderer *renderer, int location, int value);
    void (*SetAttributeui)(METAENGINE_Render_Renderer *renderer, int location, unsigned int value);
    void (*SetAttributefv)(METAENGINE_Render_Renderer *renderer, int location, int num_elements, float *value);
    void (*SetAttributeiv)(METAENGINE_Render_Renderer *renderer, int location, int num_elements, int *value);
    void (*SetAttributeuiv)(METAENGINE_Render_Renderer *renderer, int location, int num_elements, unsigned int *value);
    void (*SetAttributeSource)(METAENGINE_Render_Renderer *renderer, int num_values, METAENGINE_Render_Attribute source);


    // Shapes
    float (*SetLineThickness)(METAENGINE_Render_Renderer *renderer, float thickness);
    float (*GetLineThickness)(METAENGINE_Render_Renderer *renderer);
    void (*Pixel)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, SDL_Color color);
    void (*Line)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2, SDL_Color color);
    void (*Arc)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, float radius, float start_angle, float end_angle, SDL_Color color);
    void (*ArcFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, float radius, float start_angle, float end_angle, SDL_Color color);
    void (*Circle)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, float radius, SDL_Color color);
    void (*CircleFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, float radius, SDL_Color color);
    void (*Ellipse)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, float rx, float ry, float degrees, SDL_Color color);
    void (*EllipseFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, float rx, float ry, float degrees, SDL_Color color);
    void (*Sector)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, float inner_radius, float outer_radius, float start_angle, float end_angle, SDL_Color color);
    void (*SectorFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, float inner_radius, float outer_radius, float start_angle, float end_angle, SDL_Color color);
    void (*Tri)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2, float x3, float y3, SDL_Color color);
    void (*TriFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2, float x3, float y3, SDL_Color color);
    void (*Rectangle)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2, SDL_Color color);
    void (*RectangleFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2, SDL_Color color);
    void (*RectangleRound)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2, float radius, SDL_Color color);
    void (*RectangleRoundFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2, float radius, SDL_Color color);
    void (*Polygon)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, unsigned int num_vertices, float *vertices, SDL_Color color);
    void (*Polyline)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, unsigned int num_vertices, float *vertices, SDL_Color color, bool close_loop);
    void (*PolygonFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, unsigned int num_vertices, float *vertices, SDL_Color color);

} METAENGINE_Render_RendererImpl;


#endif
