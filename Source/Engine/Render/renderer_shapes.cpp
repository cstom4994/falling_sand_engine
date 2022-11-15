#include "renderer_gpu.h"
#include "renderer_RendererImpl.h"
#include <string.h>

#ifdef _MSC_VER
// Disable warning: selection for inlining
#pragma warning(disable: 4514 4711)
// Disable warning: Spectre mitigation
#pragma warning(disable: 5045)
#endif

#define CHECK_RENDERER() \
METAENGINE_Render_Renderer* renderer = METAENGINE_Render_GetCurrentRenderer(); \
if(renderer == NULL) \
    return;

#define CHECK_RENDERER_1(ret) \
METAENGINE_Render_Renderer* renderer = METAENGINE_Render_GetCurrentRenderer(); \
if(renderer == NULL) \
    return ret;


float METAENGINE_Render_SetLineThickness(float thickness)
{
	CHECK_RENDERER_1(1.0f);
	return renderer->impl->SetLineThickness(renderer, thickness);
}

float METAENGINE_Render_GetLineThickness(void)
{
	CHECK_RENDERER_1(1.0f);
	return renderer->impl->GetLineThickness(renderer);
}

void METAENGINE_Render_Pixel(METAENGINE_Render_Target* target, float x, float y, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->Pixel(renderer, target, x, y, color);
}

void METAENGINE_Render_Line(METAENGINE_Render_Target* target, float x1, float y1, float x2, float y2, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->Line(renderer, target, x1, y1, x2, y2, color);
}


void METAENGINE_Render_Arc(METAENGINE_Render_Target* target, float x, float y, float radius, float start_angle, float end_angle, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->Arc(renderer, target, x, y, radius, start_angle, end_angle, color);
}


void METAENGINE_Render_ArcFilled(METAENGINE_Render_Target* target, float x, float y, float radius, float start_angle, float end_angle, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->ArcFilled(renderer, target, x, y, radius, start_angle, end_angle, color);
}

void METAENGINE_Render_Circle(METAENGINE_Render_Target* target, float x, float y, float radius, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->Circle(renderer, target, x, y, radius, color);
}

void METAENGINE_Render_CircleFilled(METAENGINE_Render_Target* target, float x, float y, float radius, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->CircleFilled(renderer, target, x, y, radius, color);
}

void METAENGINE_Render_Ellipse(METAENGINE_Render_Target* target, float x, float y, float rx, float ry, float degrees, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->Ellipse(renderer, target, x, y, rx, ry, degrees, color);
}

void METAENGINE_Render_EllipseFilled(METAENGINE_Render_Target* target, float x, float y, float rx, float ry, float degrees, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->EllipseFilled(renderer, target, x, y, rx, ry, degrees, color);
}

void METAENGINE_Render_Sector(METAENGINE_Render_Target* target, float x, float y, float inner_radius, float outer_radius, float start_angle, float end_angle, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->Sector(renderer, target, x, y, inner_radius, outer_radius, start_angle, end_angle, color);
}

void METAENGINE_Render_SectorFilled(METAENGINE_Render_Target* target, float x, float y, float inner_radius, float outer_radius, float start_angle, float end_angle, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->SectorFilled(renderer, target, x, y, inner_radius, outer_radius, start_angle, end_angle, color);
}

void METAENGINE_Render_Tri(METAENGINE_Render_Target* target, float x1, float y1, float x2, float y2, float x3, float y3, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->Tri(renderer, target, x1, y1, x2, y2, x3, y3, color);
}

void METAENGINE_Render_TriFilled(METAENGINE_Render_Target* target, float x1, float y1, float x2, float y2, float x3, float y3, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->TriFilled(renderer, target, x1, y1, x2, y2, x3, y3, color);
}

void METAENGINE_Render_Rectangle(METAENGINE_Render_Target* target, float x1, float y1, float x2, float y2, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->Rectangle(renderer, target, x1, y1, x2, y2, color);
}

void METAENGINE_Render_Rectangle2(METAENGINE_Render_Target* target, METAENGINE_Render_Rect rect, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->Rectangle(renderer, target, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, color);
}

void METAENGINE_Render_RectangleFilled(METAENGINE_Render_Target* target, float x1, float y1, float x2, float y2, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->RectangleFilled(renderer, target, x1, y1, x2, y2, color);
}

void METAENGINE_Render_RectangleFilled2(METAENGINE_Render_Target* target, METAENGINE_Render_Rect rect, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->RectangleFilled(renderer, target, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, color);
}

void METAENGINE_Render_RectangleRound(METAENGINE_Render_Target* target, float x1, float y1, float x2, float y2, float radius, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->RectangleRound(renderer, target, x1, y1, x2, y2, radius, color);
}

void METAENGINE_Render_RectangleRound2(METAENGINE_Render_Target* target, METAENGINE_Render_Rect rect, float radius, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->RectangleRound(renderer, target, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, radius, color);
}

void METAENGINE_Render_RectangleRoundFilled(METAENGINE_Render_Target* target, float x1, float y1, float x2, float y2, float radius, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->RectangleRoundFilled(renderer, target, x1, y1, x2, y2, radius, color);
}

void METAENGINE_Render_RectangleRoundFilled2(METAENGINE_Render_Target* target, METAENGINE_Render_Rect rect, float radius, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->RectangleRoundFilled(renderer, target, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, radius, color);
}

void METAENGINE_Render_Polygon(METAENGINE_Render_Target* target, unsigned int num_vertices, float* vertices, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->Polygon(renderer, target, num_vertices, vertices, color);
}

void METAENGINE_Render_Polyline(METAENGINE_Render_Target* target, unsigned int num_vertices, float* vertices, SDL_Color color, METAENGINE_Render_bool close_loop)
{
	CHECK_RENDERER();
	renderer->impl->Polyline(renderer, target, num_vertices, vertices, color, close_loop );
}

void METAENGINE_Render_PolygonFilled(METAENGINE_Render_Target* target, unsigned int num_vertices, float* vertices, SDL_Color color)
{
	CHECK_RENDERER();
	renderer->impl->PolygonFilled(renderer, target, num_vertices, vertices, color);
}

