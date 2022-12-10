/***************************************************************************
 *
 * Copyright (c) 2003 - 2011 Petri Purho
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ***************************************************************************/

#include "color_utils.h"

#include "Poro/utils/PoroMath.hpp"

namespace CEngine {
    namespace {}// namespace
    // ----------------------------------------------------------------------------

    // ----------------------------------------------------------------------------

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

        return types::fcolor((float) r / 255.f, (float) g / 255.f, (float) b / 255.f);
    }

    types::uint32 ConvertToHex(const types::color &c) {
        types::uint32 r32, g32, b32;

        //types::uint32 RMask( 0x00FF0000 );
        //types::uint32 GMask( 0x0000FF00);
        //types::uint32 BMask( 0x000000FF );
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
        float t = (float) (CEngine::math::Absolute(c1.GetR() - c2.GetR()) +
                           CEngine::math::Absolute(c1.GetG() - c2.GetG()) +
                           CEngine::math::Absolute(c1.GetB() - c2.GetB()));

        return t / 3.f;
    }

    // ----------------------------------------------------------------------------

    float ColorDistance(const types::color &c1, const types::color &c2) {
        float t = (float) (CEngine::math::Absolute(c1.GetR() - c2.GetR()) +
                           CEngine::math::Absolute(c1.GetG() - c2.GetG()) +
                           CEngine::math::Absolute(c1.GetB() - c2.GetB()));

        return t / (255.f * 3);
    }

    // ----------------------------------------------------------------------------

    types::color DoDarker(const types::color &o, float dark) {
        types::color result;
        result.SetFloat(o.GetRf() * dark, o.GetGf() * dark, o.GetBf() * dark, 1.f);

        return result;
    }

    //-----------------------------------------------------------------------------

    types::uint32 Blend2Colors(types::uint32 c1, types::uint32 c2, float how_much_of_1,
                               bool ignore_alpha) {
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

}// namespace CEngine
