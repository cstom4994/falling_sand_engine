#include "pixel_color.h"

#include <SDL2/SDL.h>

#include "engine/mathlib.hpp"

namespace MetaEngine {

bool CColorUint8::masks_initialized = false;
CColorUint8::uint32 CColorUint8::RMask;
CColorUint8::uint32 CColorUint8::GMask;
CColorUint8::uint32 CColorUint8::BMask;
CColorUint8::uint32 CColorUint8::AMask;

CColorUint8::uint8 CColorUint8::RShift;
CColorUint8::uint8 CColorUint8::GShift;
CColorUint8::uint8 CColorUint8::BShift;
CColorUint8::uint8 CColorUint8::AShift;

void CColorUint8::InitMasks() {

    if (masks_initialized == false) {

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        RMask = (0xFF000000);
        GMask = (0x00FF0000);
        BMask = (0x0000FF00), AMask = (0x000000FF);
        RShift = (24);
        GShift = (16);
        BShift = (8);
        AShift = (0);
#else
        RMask = (0x000000FF);
        GMask = (0x0000FF00);
        BMask = (0x00FF0000);
        AMask = (0xFF000000);
        RShift = (0);
        GShift = (8);
        BShift = (16);
        AShift = (24);
#endif

        masks_initialized = true;
    }
}

// ---------------

bool CColorFloat::masks_initialized = false;
CColorFloat::uint32 CColorFloat::RMask;
CColorFloat::uint32 CColorFloat::GMask;
CColorFloat::uint32 CColorFloat::BMask;
CColorFloat::uint32 CColorFloat::AMask;

CColorFloat::uint8 CColorFloat::RShift;
CColorFloat::uint8 CColorFloat::GShift;
CColorFloat::uint8 CColorFloat::BShift;
CColorFloat::uint8 CColorFloat::AShift;

void CColorFloat::InitMasks() {

    if (masks_initialized == false) {

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        RMask = (0xFF000000);
        GMask = (0x00FF0000);
        BMask = (0x0000FF00), AMask = (0x000000FF);
        RShift = (24);
        GShift = (16);
        BShift = (8);
        AShift = (0);
#else
        RMask = (0x000000FF);
        GMask = (0x0000FF00);
        BMask = (0x00FF0000);
        AMask = (0xFF000000);
        RShift = (0);
        GShift = (8);
        BShift = (16);
        AShift = (24);
#endif

        masks_initialized = true;
    }
}

types::color GetColorFromHex(types::uint32 color) {
    types::uint32 r32, g32, b32;

    types::uint32 RMask(0x00FF0000);
    types::uint32 GMask(0x0000FF00);
    types::uint32 BMask(0x000000FF);
    types::uint32 RShift(16);
    types::uint32 GShift(8);
    types::uint32 BShift(0);
    r32 = color & RMask;
    g32 = color & GMask;
    b32 = color & BMask;

    int r = r32 >> RShift;
    int g = g32 >> GShift;
    int b = b32 >> BShift;

    return types::color(r, g, b);
}

types::fcolor GetFColorFromHex(types::uint32 color) {
    types::uint32 r32, g32, b32;

    types::uint32 RMask(0x00FF0000);
    types::uint32 GMask(0x0000FF00);
    types::uint32 BMask(0x000000FF);
    types::uint32 RShift(16);
    types::uint32 GShift(8);
    types::uint32 BShift(0);
    r32 = color & RMask;
    g32 = color & GMask;
    b32 = color & BMask;

    int r = r32 >> RShift;
    int g = g32 >> GShift;
    int b = b32 >> BShift;

    return types::fcolor((float)r / 255.f, (float)g / 255.f, (float)b / 255.f);
}

types::uint32 ConvertToHex(const types::color &c) {
    types::uint32 r32, g32, b32;

    // types::uint32 RMask( 0x00FF0000 );
    // types::uint32 GMask( 0x0000FF00);
    // types::uint32 BMask( 0x000000FF );
    types::uint32 RShift(16);
    types::uint32 GShift(8);
    types::uint32 BShift(0);

    r32 = c.GetR() << RShift;
    g32 = c.GetG() << GShift;
    b32 = c.GetB() << BShift;

    return (r32 | g32 | b32);
}
// ----------------------------------------------------------------------------

std::vector<types::uint32> ConvertColorPalette(const std::vector<types::color> &palette) {
    std::vector<types::uint32> result;
    for (unsigned int i = 0; i < palette.size(); ++i) {
        result.push_back(ConvertToHex(palette[i]));
    }
    return result;
}

std::vector<types::fcolor> ConvertColorPaletteF(const std::vector<types::uint32> &palette) {
    std::vector<types::fcolor> result;
    for (unsigned int i = 0; i < palette.size(); ++i) {
        types::color c = GetColorFromHex(palette[i]);
        types::fcolor fc(c.GetRf(), c.GetGf(), c.GetBf());
        result.push_back(fc);
    }
    return result;
}

// ----------------------------------------------------------------------------

types::color InvertColor(const types::color &c) {
    types::color result(255 - c.GetR(), 255 - c.GetG(), 255 - c.GetB());
    return result;
}

// ----------------------------------------------------------------------------

float ColorDistance(const types::fcolor &c1, const types::fcolor &c2) {
    float t = (float)(MetaEngine::math::Absolute(c1.GetR() - c2.GetR()) + MetaEngine::math::Absolute(c1.GetG() - c2.GetG()) + MetaEngine::math::Absolute(c1.GetB() - c2.GetB()));

    return t / 3.f;
}

// ----------------------------------------------------------------------------

float ColorDistance(const types::color &c1, const types::color &c2) {
    float t = (float)(MetaEngine::math::Absolute(c1.GetR() - c2.GetR()) + MetaEngine::math::Absolute(c1.GetG() - c2.GetG()) + MetaEngine::math::Absolute(c1.GetB() - c2.GetB()));

    return t / (255.f * 3);
}

// ----------------------------------------------------------------------------

types::color DoDarker(const types::color &o, float dark) {
    types::color result;
    result.SetFloat(o.GetRf() * dark, o.GetGf() * dark, o.GetBf() * dark, 1.f);

    return result;
}

//-----------------------------------------------------------------------------

types::uint32 Blend2Colors(types::uint32 c1, types::uint32 c2, float how_much_of_1, bool ignore_alpha) {
    if (c1 == c2) return c1;

    types::fcolor color1(c1);
    types::fcolor color2(c2);

    types::fcolor result = how_much_of_1 * color1 + (1.f - how_much_of_1) * color2;
    // alpha?
    // we set the alpha (R channel for twiched reasons) to be the highest alpha
    if (ignore_alpha) result.SetA(color2.GetA());

    // result.SetR( ( color1.GetR() > color2.GetR() )? color1.GetR() : color2.GetR() );
    return result.Get32();
}

//-----------------------------------------------------------------------------

}  // namespace MetaEngine
