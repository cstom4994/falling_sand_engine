

#ifndef METADOT_NUKLEAR_H
#define METADOT_NUKLEAR_H

#include "Libs/raylib/raylib.h"

#ifndef NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_VARARGS
#endif

#include "nuklear.h"

#ifdef __cplusplus
extern "C" {
#endif

struct nk_context* InitNuklear(int fontSize);                // Initialize the Nuklear GUI context
struct nk_context* InitNuklearEx(Font font, float fontSize); // Initialize the Nuklear GUI context, with a custom font
void UpdateNuklear(struct nk_context * ctx);                 // Update the input state and internal components for Nuklear
void DrawNuklear(struct nk_context * ctx);                   // Render the Nuklear GUI on the screen
void UnloadNuklear(struct nk_context * ctx);                 // Deinitialize the Nuklear context
struct nk_color ColorToNuklear(Color color);                 // Convert a MetaDot Color to a Nuklear color object
struct nk_colorf ColorToNuklearF(Color color);               // Convert a MetaDot Color to a Nuklear floating color
struct Color ColorFromNuklear(struct nk_color color);        // Convert a Nuklear color to a MetaDot Color
struct Color ColorFromNuklearF(struct nk_colorf color);      // Convert a Nuklear floating color to a MetaDot Color
struct Rectangle RectangleFromNuklear(struct nk_rect rect);  // Convert a Nuklear rectangle to a MetaDot Rectangle
struct nk_rect RectangleToNuklear(Rectangle rect);           // Convert a MetaDot Rectangle to a Nuklear Rectangle
struct nk_image TextureToNuklear(Texture tex);               // Convert a MetaDot Texture to A Nuklear image
struct Texture TextureFromNuklear(struct nk_image img);      // Convert a Nuklear image to a MetaDot Texture
struct nk_image LoadNuklearImage(const char* path);          // Load a Nuklear image
void UnloadNuklearImage(struct nk_image img);                // Unload a Nuklear image. And free its data
void CleanupNuklearImage(struct nk_image img);               // Frees the data stored by the Nuklear image

#ifdef __cplusplus
}
#endif

#endif  // METADOT_NUKLEAR_H
