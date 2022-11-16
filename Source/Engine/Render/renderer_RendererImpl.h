#ifndef _METAENGINE_Render_RENDERERIMPL_H__
#define _METAENGINE_Render_RENDERERIMPL_H__

#include "renderer_gpu.h"

#ifdef __cplusplus
extern "C"
{
#endif

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
        METAENGINE_Render_Target *(SDLCALL *Init)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_RendererID renderer_request, Uint16 w, Uint16 h, METAENGINE_Render_WindowFlagEnum SDL_flags);

        /*! \see METAENGINE_Render_CreateTargetFromWindow
     * The extra parameter is used internally to reuse/reinit a target. */
        METAENGINE_Render_Target *(SDLCALL *CreateTargetFromWindow)(METAENGINE_Render_Renderer *renderer, Uint32 windowID, METAENGINE_Render_Target *target);

        /*! \see METAENGINE_Render_SetActiveTarget() */
        METAENGINE_Render_bool(SDLCALL *SetActiveTarget)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);

        /*! \see METAENGINE_Render_CreateAliasTarget() */
        METAENGINE_Render_Target *(SDLCALL *CreateAliasTarget)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);

        /*! \see METAENGINE_Render_MakeCurrent */
        void(SDLCALL *MakeCurrent)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, Uint32 windowID);

        /*! Sets up this renderer to act as the current renderer.  Called automatically by METAENGINE_Render_SetCurrentRenderer(). */
        void(SDLCALL *SetAsCurrent)(METAENGINE_Render_Renderer *renderer);

        /*! \see METAENGINE_Render_ResetRendererState() */
        void(SDLCALL *ResetRendererState)(METAENGINE_Render_Renderer *renderer);

        /*! \see METAENGINE_Render_AddDepthBuffer() */
        METAENGINE_Render_bool(SDLCALL *AddDepthBuffer)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);

        /*! \see METAENGINE_Render_SetWindowResolution() */
        METAENGINE_Render_bool(SDLCALL *SetWindowResolution)(METAENGINE_Render_Renderer *renderer, Uint16 w, Uint16 h);

        /*! \see METAENGINE_Render_SetVirtualResolution() */
        void(SDLCALL *SetVirtualResolution)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, Uint16 w, Uint16 h);

        /*! \see METAENGINE_Render_UnsetVirtualResolution() */
        void(SDLCALL *UnsetVirtualResolution)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);

        /*! Clean up the renderer state. */
        void(SDLCALL *Quit)(METAENGINE_Render_Renderer *renderer);

        /*! \see METAENGINE_Render_SetFullscreen() */
        METAENGINE_Render_bool(SDLCALL *SetFullscreen)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_bool enable_fullscreen, METAENGINE_Render_bool use_desktop_resolution);

        /*! \see METAENGINE_Render_SetCamera() */
        METAENGINE_Render_Camera(SDLCALL *SetCamera)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, METAENGINE_Render_Camera *cam);

        /*! \see METAENGINE_Render_CreateImage() */
        METAENGINE_Render_Image *(SDLCALL *CreateImage)(METAENGINE_Render_Renderer *renderer, Uint16 w, Uint16 h, METAENGINE_Render_FormatEnum format);

        /*! \see METAENGINE_Render_CreateImageUsingTexture() */
        METAENGINE_Render_Image *(SDLCALL *CreateImageUsingTexture)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_TextureHandle handle, METAENGINE_Render_bool take_ownership);

        /*! \see METAENGINE_Render_CreateAliasImage() */
        METAENGINE_Render_Image *(SDLCALL *CreateAliasImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image);

        /*! \see METAENGINE_Render_SaveImage() */
        METAENGINE_Render_bool(SDLCALL *SaveImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, const char *filename, METAENGINE_Render_FileFormatEnum format);

        /*! \see METAENGINE_Render_CopyImage() */
        METAENGINE_Render_Image *(SDLCALL *CopyImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image);

        /*! \see METAENGINE_Render_UpdateImage */
        void(SDLCALL *UpdateImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, const METAENGINE_Render_Rect *image_rect, SDL_Surface *surface, const METAENGINE_Render_Rect *surface_rect);

        /*! \see METAENGINE_Render_UpdateImageBytes */
        void(SDLCALL *UpdateImageBytes)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, const METAENGINE_Render_Rect *image_rect, const unsigned char *bytes, int bytes_per_row);

        /*! \see METAENGINE_Render_ReplaceImage */
        METAENGINE_Render_bool(SDLCALL *ReplaceImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, SDL_Surface *surface, const METAENGINE_Render_Rect *surface_rect);

        /*! \see METAENGINE_Render_CopyImageFromSurface() */
        METAENGINE_Render_Image *(SDLCALL *CopyImageFromSurface)(METAENGINE_Render_Renderer *renderer, SDL_Surface *surface, const METAENGINE_Render_Rect *surface_rect);

        /*! \see METAENGINE_Render_CopyImageFromTarget() */
        METAENGINE_Render_Image *(SDLCALL *CopyImageFromTarget)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);

        /*! \see METAENGINE_Render_CopySurfaceFromTarget() */
        SDL_Surface *(SDLCALL *CopySurfaceFromTarget)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);

        /*! \see METAENGINE_Render_CopySurfaceFromImage() */
        SDL_Surface *(SDLCALL *CopySurfaceFromImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image);

        /*! \see METAENGINE_Render_FreeImage() */
        void(SDLCALL *FreeImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image);

        /*! \see METAENGINE_Render_GetTarget() */
        METAENGINE_Render_Target *(SDLCALL *GetTarget)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image);

        /*! \see METAENGINE_Render_FreeTarget() */
        void(SDLCALL *FreeTarget)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);

        /*! \see METAENGINE_Render_Blit() */
        void(SDLCALL *Blit)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect, METAENGINE_Render_Target *target, float x, float y);

        /*! \see METAENGINE_Render_BlitRotate() */
        void(SDLCALL *BlitRotate)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect, METAENGINE_Render_Target *target, float x, float y, float degrees);

        /*! \see METAENGINE_Render_BlitScale() */
        void(SDLCALL *BlitScale)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect, METAENGINE_Render_Target *target, float x, float y, float scaleX, float scaleY);

        /*! \see METAENGINE_Render_BlitTransform */
        void(SDLCALL *BlitTransform)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect, METAENGINE_Render_Target *target, float x, float y, float degrees, float scaleX, float scaleY);

        /*! \see METAENGINE_Render_BlitTransformX() */
        void(SDLCALL *BlitTransformX)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect, METAENGINE_Render_Target *target, float x, float y, float pivot_x, float pivot_y, float degrees, float scaleX, float scaleY);

        /*! \see METAENGINE_Render_PrimitiveBatchV() */
        void(SDLCALL *PrimitiveBatchV)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_Target *target, METAENGINE_Render_PrimitiveEnum primitive_type, unsigned short num_vertices, void *values, unsigned int num_indices, unsigned short *indices, METAENGINE_Render_BatchFlagEnum flags);

        /*! \see METAENGINE_Render_GenerateMipmaps() */
        void(SDLCALL *GenerateMipmaps)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image);

        /*! \see METAENGINE_Render_SetClip() */
        METAENGINE_Render_Rect(SDLCALL *SetClip)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, Sint16 x, Sint16 y, Uint16 w, Uint16 h);

        /*! \see METAENGINE_Render_UnsetClip() */
        void(SDLCALL *UnsetClip)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);

        /*! \see METAENGINE_Render_GetPixel() */
        SDL_Color(SDLCALL *GetPixel)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, Sint16 x, Sint16 y);

        /*! \see METAENGINE_Render_SetImageFilter() */
        void(SDLCALL *SetImageFilter)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_FilterEnum filter);

        /*! \see METAENGINE_Render_SetWrapMode() */
        void(SDLCALL *SetWrapMode)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, METAENGINE_Render_WrapEnum wrap_mode_x, METAENGINE_Render_WrapEnum wrap_mode_y);

        /*! \see METAENGINE_Render_GetTextureHandle() */
        METAENGINE_Render_TextureHandle(SDLCALL *GetTextureHandle)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image);

        /*! \see METAENGINE_Render_ClearRGBA() */
        void(SDLCALL *ClearRGBA)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
        /*! \see METAENGINE_Render_FlushBlitBuffer() */
        void(SDLCALL *FlushBlitBuffer)(METAENGINE_Render_Renderer *renderer);
        /*! \see METAENGINE_Render_Flip() */
        void(SDLCALL *Flip)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);


        /*! \see METAENGINE_Render_CreateShaderProgram() */
        Uint32(SDLCALL *CreateShaderProgram)(METAENGINE_Render_Renderer *renderer);

        /*! \see METAENGINE_Render_FreeShaderProgram() */
        void(SDLCALL *FreeShaderProgram)(METAENGINE_Render_Renderer *renderer, Uint32 program_object);

        /*! \see METAENGINE_Render_CompileShader_RW() */
        Uint32(SDLCALL *CompileShader_RW)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_ShaderEnum shader_type, SDL_RWops *shader_source, METAENGINE_Render_bool free_rwops);

        /*! \see METAENGINE_Render_CompileShader() */
        Uint32(SDLCALL *CompileShader)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_ShaderEnum shader_type, const char *shader_source);

        /*! \see METAENGINE_Render_FreeShader() */
        void(SDLCALL *FreeShader)(METAENGINE_Render_Renderer *renderer, Uint32 shader_object);

        /*! \see METAENGINE_Render_AttachShader() */
        void(SDLCALL *AttachShader)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, Uint32 shader_object);

        /*! \see METAENGINE_Render_DetachShader() */
        void(SDLCALL *DetachShader)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, Uint32 shader_object);

        /*! \see METAENGINE_Render_LinkShaderProgram() */
        METAENGINE_Render_bool(SDLCALL *LinkShaderProgram)(METAENGINE_Render_Renderer *renderer, Uint32 program_object);

        /*! \see METAENGINE_Render_ActivateShaderProgram() */
        void(SDLCALL *ActivateShaderProgram)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, METAENGINE_Render_ShaderBlock *block);

        /*! \see METAENGINE_Render_DeactivateShaderProgram() */
        void(SDLCALL *DeactivateShaderProgram)(METAENGINE_Render_Renderer *renderer);

        /*! \see METAENGINE_Render_GetShaderMessage() */
        const char *(SDLCALL *GetShaderMessage)(METAENGINE_Render_Renderer *renderer);

        /*! \see METAENGINE_Render_GetAttribLocation() */
        int(SDLCALL *GetAttributeLocation)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, const char *attrib_name);

        /*! \see METAENGINE_Render_GetUniformLocation() */
        int(SDLCALL *GetUniformLocation)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, const char *uniform_name);

        /*! \see METAENGINE_Render_LoadShaderBlock() */
        METAENGINE_Render_ShaderBlock(SDLCALL *LoadShaderBlock)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, const char *position_name, const char *texcoord_name, const char *color_name, const char *modelViewMatrix_name);

        /*! \see METAENGINE_Render_SetShaderBlock() */
        void(SDLCALL *SetShaderBlock)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_ShaderBlock block);

        /*! \see METAENGINE_Render_SetShaderImage() */
        void(SDLCALL *SetShaderImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image, int location, int image_unit);

        /*! \see METAENGINE_Render_GetUniformiv() */
        void(SDLCALL *GetUniformiv)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, int location, int *values);

        /*! \see METAENGINE_Render_SetUniformi() */
        void(SDLCALL *SetUniformi)(METAENGINE_Render_Renderer *renderer, int location, int value);

        /*! \see METAENGINE_Render_SetUniformiv() */
        void(SDLCALL *SetUniformiv)(METAENGINE_Render_Renderer *renderer, int location, int num_elements_per_value, int num_values, int *values);

        /*! \see METAENGINE_Render_GetUniformuiv() */
        void(SDLCALL *GetUniformuiv)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, int location, unsigned int *values);

        /*! \see METAENGINE_Render_SetUniformui() */
        void(SDLCALL *SetUniformui)(METAENGINE_Render_Renderer *renderer, int location, unsigned int value);

        /*! \see METAENGINE_Render_SetUniformuiv() */
        void(SDLCALL *SetUniformuiv)(METAENGINE_Render_Renderer *renderer, int location, int num_elements_per_value, int num_values, unsigned int *values);

        /*! \see METAENGINE_Render_GetUniformfv() */
        void(SDLCALL *GetUniformfv)(METAENGINE_Render_Renderer *renderer, Uint32 program_object, int location, float *values);

        /*! \see METAENGINE_Render_SetUniformf() */
        void(SDLCALL *SetUniformf)(METAENGINE_Render_Renderer *renderer, int location, float value);

        /*! \see METAENGINE_Render_SetUniformfv() */
        void(SDLCALL *SetUniformfv)(METAENGINE_Render_Renderer *renderer, int location, int num_elements_per_value, int num_values, float *values);

        /*! \see METAENGINE_Render_SetUniformMatrixfv() */
        void(SDLCALL *SetUniformMatrixfv)(METAENGINE_Render_Renderer *renderer, int location, int num_matrices, int num_rows, int num_columns, METAENGINE_Render_bool transpose, float *values);

        /*! \see METAENGINE_Render_SetAttributef() */
        void(SDLCALL *SetAttributef)(METAENGINE_Render_Renderer *renderer, int location, float value);

        /*! \see METAENGINE_Render_SetAttributei() */
        void(SDLCALL *SetAttributei)(METAENGINE_Render_Renderer *renderer, int location, int value);

        /*! \see METAENGINE_Render_SetAttributeui() */
        void(SDLCALL *SetAttributeui)(METAENGINE_Render_Renderer *renderer, int location, unsigned int value);

        /*! \see METAENGINE_Render_SetAttributefv() */
        void(SDLCALL *SetAttributefv)(METAENGINE_Render_Renderer *renderer, int location, int num_elements, float *value);

        /*! \see METAENGINE_Render_SetAttributeiv() */
        void(SDLCALL *SetAttributeiv)(METAENGINE_Render_Renderer *renderer, int location, int num_elements, int *value);

        /*! \see METAENGINE_Render_SetAttributeuiv() */
        void(SDLCALL *SetAttributeuiv)(METAENGINE_Render_Renderer *renderer, int location, int num_elements, unsigned int *value);

        /*! \see METAENGINE_Render_SetAttributeSource() */
        void(SDLCALL *SetAttributeSource)(METAENGINE_Render_Renderer *renderer, int num_values, METAENGINE_Render_Attribute source);


        // Shapes

        /*! \see METAENGINE_Render_SetLineThickness() */
        float(SDLCALL *SetLineThickness)(METAENGINE_Render_Renderer *renderer, float thickness);

        /*! \see METAENGINE_Render_GetLineThickness() */
        float(SDLCALL *GetLineThickness)(METAENGINE_Render_Renderer *renderer);

        /*! \see METAENGINE_Render_Pixel() */
        void(SDLCALL *Pixel)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, SDL_Color color);

        /*! \see METAENGINE_Render_Line() */
        void(SDLCALL *Line)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2, SDL_Color color);

        /*! \see METAENGINE_Render_Arc() */
        void(SDLCALL *Arc)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, float radius, float start_angle, float end_angle, SDL_Color color);

        /*! \see METAENGINE_Render_ArcFilled() */
        void(SDLCALL *ArcFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, float radius, float start_angle, float end_angle, SDL_Color color);

        /*! \see METAENGINE_Render_Circle() */
        void(SDLCALL *Circle)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, float radius, SDL_Color color);

        /*! \see METAENGINE_Render_CircleFilled() */
        void(SDLCALL *CircleFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, float radius, SDL_Color color);

        /*! \see METAENGINE_Render_Ellipse() */
        void(SDLCALL *Ellipse)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, float rx, float ry, float degrees, SDL_Color color);

        /*! \see METAENGINE_Render_EllipseFilled() */
        void(SDLCALL *EllipseFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, float rx, float ry, float degrees, SDL_Color color);

        /*! \see METAENGINE_Render_Sector() */
        void(SDLCALL *Sector)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, float inner_radius, float outer_radius, float start_angle, float end_angle, SDL_Color color);

        /*! \see METAENGINE_Render_SectorFilled() */
        void(SDLCALL *SectorFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x, float y, float inner_radius, float outer_radius, float start_angle, float end_angle, SDL_Color color);

        /*! \see METAENGINE_Render_Tri() */
        void(SDLCALL *Tri)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2, float x3, float y3, SDL_Color color);

        /*! \see METAENGINE_Render_TriFilled() */
        void(SDLCALL *TriFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2, float x3, float y3, SDL_Color color);

        /*! \see METAENGINE_Render_Rectangle() */
        void(SDLCALL *Rectangle)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2, SDL_Color color);

        /*! \see METAENGINE_Render_RectangleFilled() */
        void(SDLCALL *RectangleFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2, SDL_Color color);

        /*! \see METAENGINE_Render_RectangleRound() */
        void(SDLCALL *RectangleRound)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2, float radius, SDL_Color color);

        /*! \see METAENGINE_Render_RectangleRoundFilled() */
        void(SDLCALL *RectangleRoundFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2, float radius, SDL_Color color);

        /*! \see METAENGINE_Render_Polygon() */
        void(SDLCALL *Polygon)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, unsigned int num_vertices, float *vertices, SDL_Color color);

        /*! \see METAENGINE_Render_Polyline() */
        void(SDLCALL *Polyline)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, unsigned int num_vertices, float *vertices, SDL_Color color, METAENGINE_Render_bool close_loop);

        /*! \see METAENGINE_Render_PolygonFilled() */
        void(SDLCALL *PolygonFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, unsigned int num_vertices, float *vertices, SDL_Color color);

    } METAENGINE_Render_RendererImpl;

#ifdef __cplusplus
}
#endif

#endif
