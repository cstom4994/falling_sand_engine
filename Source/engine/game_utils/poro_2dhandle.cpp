
#include "poro_2dhandle.h"

namespace BaseEngine {
/*
template< class T >
bool CRect< T >::operator <=( const CRect< T >& other ) const
{
return CRectIsInside( *this, other );
}

template< class T >
bool CRect< T >::operator >=( const CRect< T >& other ) const
{
return CRectIsInside( other, *this );
}
*/
}

#ifndef INC_CRECT_FUNCTIONS_CPP
#define INC_CRECT_FUNCTIONS_CPP

#include <assert.h>

#include "core/debug_impl.hpp"

namespace BaseEngine {

namespace {
template <class TRect>
bool RectBooleanPrivate(const TRect &first, TRect &second, TRect &third) {

    // If the second is bigge we cannot do a thing
    if (second.x <= first.x && second.x + second.w > first.x + first.w && second.y <= first.y && second.y + second.h > first.y + first.h) {
        return false;
    }

    // First well check if the second is inside the first
    if (first.x <= second.x && first.x + first.w > second.x + second.w && first.y <= second.y && first.y + first.h > second.y + second.h) {
        RectMakeEmpty(second);
        return true;
    }

    int used = 0;

    third = second;

    TRect tmp;

    // Vertical motherfuckers
    if (third.y < first.y) {
        second = TRect(third.x, third.y, third.w, first.y - third.y);
        third.h -= (first.y - third.y);
        third.y = first.y;

        used++;
    }

    if (third.y + third.h > first.y + first.h) {
        // if ( used == 2 ) return false;
        if (used == 1) {
            tmp = TRect(third.x, first.y + first.h, third.w, (third.y + third.h) - (first.y + first.h));
        } else {
            second = TRect(third.x, first.y + first.h, third.w, (third.y + third.h) - (first.y + first.h));
        }

        third.h -= (third.y + third.h) - (first.y + first.h);

        used++;
    }

    // Horizontal motherfuckerss
    // Then lets try to crack this mother fucker
    if (third.x < first.x)  // this means that most left wont ever hit the target
    {
        switch (used) {
            case 2:
                return false;
                break;

            case 1:
                tmp = TRect(third.x, third.y, first.x - third.x, third.h);
                break;

            case 0:
                second = TRect(third.x, third.y, first.x - third.x, third.h);
                break;

            default:
                assert(false && "This should not be possible");
        }

        third.w -= (first.x - third.x);
        third.x = first.x;

        used++;
    }

    if (third.x + third.w > first.x + first.w)  // this means that most of the right wont ever hit the target
    {
        switch (used) {
            case 2:
                return false;
                break;

            case 1:
                tmp = TRect(first.x + first.w, third.y, (third.x + third.w) - (first.x + first.w), third.h);
                break;

            case 0:
                second = TRect(first.x + first.w, third.y, (third.x + third.w) - (first.x + first.w), third.h);
                break;

            default:
                assert(false && "This should not be possible");
        }

        third.w -= (third.x + third.w) - (first.x + first.w);
        used++;
    }

    switch (used) {
        case 2:
            third = tmp;
            break;

        case 1:
            RectMakeEmpty(third);
            break;

        default:
            assert(false && "This should no be possible");
    }

    return true;
}

}  // end of anonymous namespace

template <class TRect>
void RectMakeEmpty(TRect &rect) {
    rect.x = 0;
    rect.y = 0;
    rect.w = 0;
    rect.h = 0;
}

template <class TRect>
bool RectHit(const TRect &first, const TRect &second) {

    return ((first.x > second.x - first.w && first.x < second.x + second.w) && (first.y > second.y - first.h && first.y < second.y + second.h));
}

template <class TRect>
bool RectIsInside(const TRect &inside, const TRect &thiss) {
    return (inside.x >= thiss.x && inside.y >= thiss.y && inside.x + inside.w <= thiss.x + thiss.w && inside.y + inside.h <= thiss.y + thiss.h);
}

template <class TRect, class PType>
bool RectIsInside(const TRect &inside, PType x, PType y) {
    return (inside.x <= x && inside.x + inside.w > x && inside.y <= y && inside.y + inside.h > y);
}

template <class TRect, class TPoint>
bool IsPointInsideRect(const TPoint &point, const TRect &rect) {
    return RectIsInside(rect, point.x, point.y);
}

template <class TRect>
void RectAddToList(std::list<TRect> &list, TRect rect) {
    if (list.empty()) {
        list.push_back(rect);
        return;
    }

    typename std::list<TRect>::iterator i;
    TRect rect_2;
    TRect rect_3;

    for (i = list.begin(); i != list.end(); ++i) {
        if (RectHit(*i, rect)) {
            if (RectIsInside(rect, *i)) return;
            rect_2 = *i;
            i = list.erase(i);
            RectMakeEmpty(rect_3);

            RectBoolean(rect, rect_2, rect_3);
            if (!rect_2.empty()) RectAddToList(list, rect_2);
            if (!rect_3.empty()) RectAddToList(list, rect_3);
        }
    }

    list.push_back(rect);
}

template <class TRect>
void RectClampInside(TRect &inside, const TRect &borders) {
    if (inside.x < borders.x) inside.x = borders.x;
    if (inside.y < borders.y) inside.y = borders.y;
    if (inside.x + inside.w > borders.x + borders.w) inside.x = (borders.x + borders.w) - inside.w;
    if (inside.y + inside.h > borders.y + borders.h) inside.y = (borders.y + borders.h) - inside.h;
}

template <class TRect>
void RectKeepInside(TRect &inside, const TRect &borders) {

    if (!RectHit(inside, borders)) {
        RectMakeEmpty(inside);
        return;
    }

    TRect return_value(inside);

    if (inside.x < borders.x) {
        return_value.x = borders.x;
        return_value.w -= (borders.x - inside.x);
    }
    if (inside.y < borders.y) {
        return_value.y = borders.y;
        return_value.h -= (borders.y - inside.y);
    }
    if (return_value.x + return_value.w > borders.x + borders.w) {
        return_value.w = (borders.x + borders.w) - return_value.x;
    }
    if (return_value.y + return_value.h > borders.y + borders.h) {
        return_value.h = (borders.y + borders.h) - return_value.y;
    }

    inside = return_value;
}

template <class TRect>
void RectBoolean(TRect &first, TRect &second, TRect &third) {

    if (RectHit(first, second) == false) {
        // std::cout << "jees" << std::endl;
        return;
    }

    if (first == second) {
        RectMakeEmpty(second);
        return;
    }

    if (first.w * first.h >= second.w * second.h) {
        TRect tmp_second(second);

        if (RectBooleanPrivate(first, second, third))
            return;
        else {
            second = tmp_second;

            // for testing...
            bool crap = RectBooleanPrivate(second, first, third);

            assert(crap);

            return;
        }
    } else {

        TRect tmp_first(first);

        if (RectBooleanPrivate(second, first, third))
            return;
        else {
            first = tmp_first;

            // for testing... the crap i mean
            bool crap = RectBooleanPrivate(first, second, third);

            assert(crap);
            return;
        }
    }
}

template <class TRect>
void RectBooleanConst(const TRect &first, const TRect &second, std::vector<TRect> &left_overs) {

    assert(false && "This may contain some bugs");
    /*
// If the second is bigge we cannot do a thing
if ( second.x <= first.x && second.x + second.w >= first.x + first.w &&
     second.y <= first.y && second.y + second.h >= first.y + first.h )
{
    return;
}

// First well check if the second is inside the first
if ( first.x <= second.x && first.x + first.w >= second.x + second.w &&
    first.y <= second.y && first.y + first.h >= second.y + second.h )
{
    types::rectMakeEmpty( second );
    return;
}
*/

    // types::rect Leftovers( second );
    TRect active(second);
    // std::vector< types::rect > left_overs;

    // Vertical motherfuckers
    if (active.y < first.y) {
        left_overs.push_back(TRect(active.x, active.y, active.w, first.y - active.y));
        active.h -= (first.y - active.y);
        active.y = first.y;
    }

    if (active.y + active.h > first.y + first.h) {
        left_overs.push_back(TRect(active.x, first.y + first.h, active.w, (active.y + active.h) - (first.y + first.h)));
        active.h = (active.y + active.h) - (first.y + first.h);
    }

    // Horizontal motherfuckerss
    // Then lets try to crack this mother fucker
    if (active.x < first.x)  // this means that most left wont ever hit the target
    {
        left_overs.push_back(TRect(active.x, active.y, first.x - active.x, active.h));
        active.w -= (first.x - active.x);
        active.x = first.x;
    }

    if (active.x + active.w > first.x + first.w)  // this means that most of the right wont ever hit the target
    {
        left_overs.push_back(TRect(first.x + first.w, active.y, (active.x + active.w) - (first.x + first.w), active.h));
        active.w -= (active.x + active.w) - (first.x + first.w);
    }
}

template <class TRect>
void RectBooleanConst(const TRect &first, const TRect &second, std::list<TRect> &left_overs) {

    /*
// If the second is bigge we cannot do a thing
if ( second.x <= first.x && second.x + second.w >= first.x + first.w &&
     second.y <= first.y && second.y + second.h >= first.y + first.h )
{
    return;
}

// First well check if the second is inside the first
if ( first.x <= second.x && first.x + first.w >= second.x + second.w &&
    first.y <= second.y && first.y + first.h >= second.y + second.h )
{
    types::rectMakeEmpty( second );
    return;
}
*/

    // types::rect Leftovers( second );
    TRect active(second);
    // std::vector< types::rect > left_overs;

    // Vertical motherfuckers
    if (active.y < first.y) {
        left_overs.push_back(TRect(active.x, active.y, active.w, first.y - active.y));
        active.h -= (first.y - active.y);
        active.y = first.y;
    }

    if (active.y + active.h >= first.y + first.h) {
        // assert( false && "This gets called" );
        left_overs.push_back(TRect(active.x, first.y + first.h, active.w, (active.y + active.h) - (first.y + first.h)));
        active.h -= (active.y + active.h) - (first.y + first.h);
    }

    // Horizontal motherfuckerss
    // Then lets try to crack this mother fucker
    if (active.x < first.x)  // this means that most left wont ever hit the target
    {
        left_overs.push_back(TRect(active.x, active.y, first.x - active.x, active.h));
        active.w -= (first.x - active.x);
        active.x = first.x;
    }

    if (active.x + active.w >= first.x + first.w)  // this means that most of the right wont ever hit the target
    {
        // assert( false && "This gets called" );
        left_overs.push_back(TRect(first.x + first.w, active.y, (active.x + active.w) - (first.x + first.w), active.h));
        active.w -= (active.x + active.w) - (first.x + first.w);
    }

    // left_overs.push_back( active );
}

template <class TRect, class TPoint>
void RectFindCrossPoints(const TRect &first, const TRect &second, std::vector<TPoint> &points) {
    if (!RectHit(first, second)) return;
    TPoint tmp_coord;

    //
    //              a1             a2
    //   b1   b2    +--------------+
    //   +----+     |              |
    //   |    |     |              |
    //   |    |     +--------------+
    //   |    |     a3             a4
    //   |    |
    //   +----+
    //   b3   b4
    //

    // The upper horizontal lines against the vertical lines...

    // a1, a2 == b1, b3
    tmp_coord = RectFindCrossPointsLine(TPoint(first.x, first.y), TPoint(first.x + first.w, first.y), TPoint(second.x, second.y), TPoint(second.x, second.y + second.h));
    if (!tmp_coord.empty()) points.push_back(tmp_coord);

    // a1, a2 == b2, b4
    tmp_coord = RectFindCrossPointsLine(TPoint(first.x, first.y), TPoint(first.x + first.w, first.y), TPoint(second.x + second.w, second.y), TPoint(second.x + second.w, second.y + second.h));
    if (!tmp_coord.empty()) points.push_back(tmp_coord);

    // a3, a4 == b1, b3
    tmp_coord = RectFindCrossPointsLine(TPoint(first.x, first.y + first.h), TPoint(first.x + first.w, first.y + first.h), TPoint(second.x, second.y), TPoint(second.x, second.y + second.h));
    if (!tmp_coord.empty()) points.push_back(tmp_coord);

    // a3, a4 == b2, b4
    tmp_coord = RectFindCrossPointsLine(TPoint(first.x, first.y + first.h), TPoint(first.x + first.w, first.y + first.h), TPoint(second.x + second.w, second.y),
                                        TPoint(second.x + second.w, second.y + second.h));
    if (!tmp_coord.empty()) points.push_back(tmp_coord);

    // a1, a3 == b1, b2
    tmp_coord = RectFindCrossPointsLine(TPoint(first.x, first.y), TPoint(first.x, first.y + first.h), TPoint(second.x, second.y), TPoint(second.x + second.w, second.y));
    if (!tmp_coord.empty()) points.push_back(tmp_coord);

    // a1, a3 == b3, b4
    tmp_coord = RectFindCrossPointsLine(TPoint(first.x, first.y), TPoint(first.x, first.y + first.h), TPoint(second.x, second.y + second.h), TPoint(second.x + second.w, second.y + second.h));
    if (!tmp_coord.empty()) points.push_back(tmp_coord);

    // a2, a4 == b1, b2
    tmp_coord = RectFindCrossPointsLine(TPoint(first.x + first.w, first.y), TPoint(first.x + first.w, first.y + first.h), TPoint(second.x, second.y), TPoint(second.x + second.w, second.y));
    if (!tmp_coord.empty()) points.push_back(tmp_coord);

    // a2, a4 == b3, b4
    tmp_coord = RectFindCrossPointsLine(TPoint(first.x + first.w, first.y), TPoint(first.x + first.w, first.y + first.h), TPoint(second.x, second.y + second.h),
                                        TPoint(second.x + second.w, second.y + second.h));
    if (!tmp_coord.empty()) points.push_back(tmp_coord);
}

template <class TPoint>
TPoint RectFindCrossPointsLine(const TPoint &first_begin, const TPoint &first_end, const TPoint &second_begin, const TPoint &second_end) {
    TPoint return_value;

    // First we'll check if we hit each other or not
    // in the horizontal sense
    if (first_begin.x <= second_begin.x && first_end.x >= second_end.x && second_begin.y <= first_begin.y && second_end.y >= first_end.y) {
        // We hit in the vertical sense
        return_value.x = second_begin.x;
        return_value.y = first_begin.y;

        return return_value;
    }

    if (first_begin.x >= second_begin.x && first_end.x <= second_end.x && second_begin.y >= first_begin.y && second_end.y <= first_end.y) {
        // We hit in the vertical sense
        return_value.x = first_begin.x;
        return_value.y = second_begin.y;

        return return_value;
    }

    // The horizontal stuff
    // if ( first_begin.y

    return return_value;
}

}  // namespace BaseEngine

#endif

#include <math.h>

#include <string>

#ifndef PI
#define PI 3.14159265
#endif

namespace BaseEngine {
namespace easing {

//-----------------------------------------------------------------------------

Back::EaseIn Back::easeIn;
Back::EaseOut Back::easeOut;
Back::EaseInOut Back::easeInOut;

float Back::impl_easeIn(float t, float b, float c, float d) {
    float s = 1.70158f;
    float postFix = t /= d;
    return c * (postFix)*t * ((s + 1) * t - s) + b;
}
float Back::impl_easeOut(float t, float b, float c, float d) {
    float s = 1.70158f;
    return c * ((t = t / d - 1) * t * ((s + 1) * t + s) + 1) + b;
}

float Back::impl_easeInOut(float t, float b, float c, float d) {
    float s = 1.70158f;
    if ((t /= d / 2) < 1) return c / 2 * (t * t * (((s *= (1.525f)) + 1) * t - s)) + b;
    float postFix = t -= 2;
    return c / 2 * ((postFix)*t * (((s *= (1.525f)) + 1) * t + s) + 2) + b;
}

//-----------------------------------------------------------------------------

Bounce::EaseIn Bounce::easeIn;
Bounce::EaseOut Bounce::easeOut;
Bounce::EaseInOut Bounce::easeInOut;

float Bounce::impl_easeIn(float t, float b, float c, float d) { return c - impl_easeOut(d - t, 0, c, d) + b; }
float Bounce::impl_easeOut(float t, float b, float c, float d) {
    if ((t /= d) < (1 / 2.75f)) {
        return c * (7.5625f * t * t) + b;
    } else if (t < (2 / 2.75f)) {
        float postFix = t -= (1.5f / 2.75f);
        return c * (7.5625f * (postFix)*t + .75f) + b;
    } else if (t < (2.5 / 2.75)) {
        float postFix = t -= (2.25f / 2.75f);
        return c * (7.5625f * (postFix)*t + .9375f) + b;
    } else {
        float postFix = t -= (2.625f / 2.75f);
        return c * (7.5625f * (postFix)*t + .984375f) + b;
    }
}

float Bounce::impl_easeInOut(float t, float b, float c, float d) {
    if (t < d / 2)
        return impl_easeIn(t * 2, 0, c, d) * .5f + b;
    else
        return impl_easeOut(t * 2 - d, 0, c, d) * .5f + c * .5f + b;
}

//-----------------------------------------------------------------------------

Circ::EaseIn Circ::easeIn;
Circ::EaseOut Circ::easeOut;
Circ::EaseInOut Circ::easeInOut;

float Circ::impl_easeIn(float t, float b, float c, float d) { return -c * (sqrt(1 - (t /= d) * t) - 1) + b; }
float Circ::impl_easeOut(float t, float b, float c, float d) { return c * sqrt(1 - (t = t / d - 1) * t) + b; }

float Circ::impl_easeInOut(float t, float b, float c, float d) {
    if ((t /= d / 2) < 1) return -c / 2 * (sqrt(1 - t * t) - 1) + b;
    return c / 2 * (sqrt(1 - t * (t -= 2)) + 1) + b;
}

//-----------------------------------------------------------------------------

Cubic::EaseIn Cubic::easeIn;
Cubic::EaseOut Cubic::easeOut;
Cubic::EaseInOut Cubic::easeInOut;

float Cubic::impl_easeIn(float t, float b, float c, float d) { return c * (t /= d) * t * t + b; }
float Cubic::impl_easeOut(float t, float b, float c, float d) { return c * ((t = t / d - 1) * t * t + 1) + b; }

float Cubic::impl_easeInOut(float t, float b, float c, float d) {
    if ((t /= d / 2) < 1) return c / 2 * t * t * t + b;
    return c / 2 * ((t -= 2) * t * t + 2) + b;
}

//-----------------------------------------------------------------------------

Elastic::EaseIn Elastic::easeIn;
Elastic::EaseOut Elastic::easeOut;
Elastic::EaseInOut Elastic::easeInOut;

float Elastic::impl_easeIn(float t, float b, float c, float d) {
    if (t == 0) return b;
    if ((t /= d) == 1) return b + c;
    float p = d * .3f;
    float a = c;
    float s = p / 4;
    float postFix = a * powf(2, 10 * (t -= 1));  // this is a fix, again, with post-increment operators
    return (float)-(postFix * sin((t * d - s) * (2 * PI) / p)) + b;
}

float Elastic::impl_easeOut(float t, float b, float c, float d) {
    if (t == 0) return b;
    if ((t /= d) == 1) return b + c;
    float p = d * .3f;
    float a = c;
    float s = p / 4;
    return (float)(a * pow(2, -10 * t) * sin((t * d - s) * (2 * PI) / p) + c + b);
}

float Elastic::impl_easeInOut(float t, float b, float c, float d) {
    if (t == 0) return b;
    if ((t /= d / 2) == 2) return b + c;
    float p = d * (.3f * 1.5f);
    float a = c;
    float s = p / 4;

    if (t < 1) {
        float postFix = a * powf(2, 10 * (t -= 1));  // postIncrement is evil
        return (float)(-.5f * (postFix * sin((t * d - s) * (2 * PI) / p)) + b);
    }
    float postFix = a * powf(2, -10 * (t -= 1));  // postIncrement is evil
    return (float)(postFix * sin((t * d - s) * (2 * PI) / p) * .5f + c + b);
}

//-----------------------------------------------------------------------------

Expo::EaseIn Expo::easeIn;
Expo::EaseOut Expo::easeOut;
Expo::EaseInOut Expo::easeInOut;

float Expo::impl_easeIn(float t, float b, float c, float d) { return (t == 0) ? b : c * powf(2, 10 * (t / d - 1)) + b; }
float Expo::impl_easeOut(float t, float b, float c, float d) { return (t == d) ? b + c : c * (-powf(2, -10 * t / d) + 1) + b; }

float Expo::impl_easeInOut(float t, float b, float c, float d) {
    if (t == 0) return b;
    if (t == d) return b + c;
    if ((t /= d / 2) < 1) return c / 2 * powf(2, 10 * (t - 1)) + b;
    return c / 2 * (-powf(2, -10 * --t) + 2) + b;
}

//-----------------------------------------------------------------------------

Linear::EaseNone Linear::easeNone;
Linear::EaseIn Linear::easeIn;
Linear::EaseOut Linear::easeOut;
Linear::EaseInOut Linear::easeInOut;

float Linear::impl_easeNone(float t, float b, float c, float d) { return c * t / d + b; }
float Linear::impl_easeIn(float t, float b, float c, float d) { return c * t / d + b; }
float Linear::impl_easeOut(float t, float b, float c, float d) { return c * t / d + b; }

float Linear::impl_easeInOut(float t, float b, float c, float d) { return c * t / d + b; }

//-----------------------------------------------------------------------------

Quad::EaseIn Quad::easeIn;
Quad::EaseOut Quad::easeOut;
Quad::EaseInOut Quad::easeInOut;

float Quad::impl_easeIn(float t, float b, float c, float d) { return c * (t /= d) * t + b; }
float Quad::impl_easeOut(float t, float b, float c, float d) { return -c * (t /= d) * (t - 2) + b; }

float Quad::impl_easeInOut(float t, float b, float c, float d) {
    if ((t /= d / 2) < 1) return ((c / 2) * (t * t)) + b;
    return -c / 2 * (((t - 2) * (--t)) - 1) + b;
    /*
originally return -c/2 * (((t-2)*(--t)) - 1) + b;

I've had to swap (--t)*(t-2) due to diffence in behaviour in
pre-increment operators between java and c++, after hours
of joy
*/
}

//-----------------------------------------------------------------------------

Quart::EaseIn Quart::easeIn;
Quart::EaseOut Quart::easeOut;
Quart::EaseInOut Quart::easeInOut;

float Quart::impl_easeIn(float t, float b, float c, float d) { return c * (t /= d) * t * t * t + b; }
float Quart::impl_easeOut(float t, float b, float c, float d) { return -c * ((t = t / d - 1) * t * t * t - 1) + b; }

float Quart::impl_easeInOut(float t, float b, float c, float d) {
    if ((t /= d / 2) < 1) return c / 2 * t * t * t * t + b;
    return -c / 2 * ((t -= 2) * t * t * t - 2) + b;
}

//-----------------------------------------------------------------------------

Quint::EaseIn Quint::easeIn;
Quint::EaseOut Quint::easeOut;
Quint::EaseInOut Quint::easeInOut;

float Quint::impl_easeIn(float t, float b, float c, float d) { return c * (t /= d) * t * t * t * t + b; }
float Quint::impl_easeOut(float t, float b, float c, float d) { return c * ((t = t / d - 1) * t * t * t * t + 1) + b; }

float Quint::impl_easeInOut(float t, float b, float c, float d) {
    if ((t /= d / 2) < 1) return c / 2 * t * t * t * t * t + b;
    return c / 2 * ((t -= 2) * t * t * t * t + 2) + b;
}

//-----------------------------------------------------------------------------

Sine::EaseIn Sine::easeIn;
Sine::EaseOut Sine::easeOut;
Sine::EaseInOut Sine::easeInOut;

float Sine::impl_easeIn(float t, float b, float c, float d) { return (float)(-c * cos(t / d * (PI / 2)) + c + b); }
float Sine::impl_easeOut(float t, float b, float c, float d) { return (float)(c * sin(t / d * (PI / 2)) + b); }

float Sine::impl_easeInOut(float t, float b, float c, float d) { return (float)(-c / 2 * (cos(PI * t / d) - 1) + b); }

//-----------------------------------------------------------------------------

float PetriHacks::SinGoTo2AndBack::f(float x) { return sin(x * 3.141596f) * 2.f; }

float PetriHacks::LittleBackAndForth::f(float x) { return ((pow(x + 0.17f, 1.3f) * sin(1.f / (x + 0.17f))) * 1.043f) + 0.035f; }

float PetriHacks::DimishingShakeBack::f(float x) { return ((cos(x * 3.1415f) + 1) * 0.5f) * sin(x * 30.0f); }

float PetriHacks::DimishingShake::f(float x) { return ((cos(x * 3.1415f) + 1) * 0.5f) * sin(x * 30.0f) + x; }

PetriHacks::SinGoTo2AndBack PetriHacks::sinGoTo2AndBack;
PetriHacks::DimishingShakeBack PetriHacks::dimishingShakeBack;
PetriHacks::DimishingShakeBack PetriHacks::dimishingShake;
PetriHacks::LittleBackAndForth PetriHacks::littleBackAndForth;

//-----------------------------------------------------------------------------

IEasingFunc &GetEasing(const char *ename) {
    std::string easing_name = ename;

#define STR_RETURN_EASING(x, y) \
    if (easing_name == #x "::" #y) return x::y;

    STR_RETURN_EASING(Back, easeIn);
    STR_RETURN_EASING(Back, easeOut);
    STR_RETURN_EASING(Back, easeInOut);

    STR_RETURN_EASING(Bounce, easeIn);
    STR_RETURN_EASING(Bounce, easeOut);
    STR_RETURN_EASING(Bounce, easeInOut);

    STR_RETURN_EASING(Circ, easeIn);
    STR_RETURN_EASING(Circ, easeOut);
    STR_RETURN_EASING(Circ, easeInOut);

    STR_RETURN_EASING(Cubic, easeIn);
    STR_RETURN_EASING(Cubic, easeOut);
    STR_RETURN_EASING(Cubic, easeInOut);

    STR_RETURN_EASING(Elastic, easeIn);
    STR_RETURN_EASING(Elastic, easeOut);
    STR_RETURN_EASING(Elastic, easeInOut);

    STR_RETURN_EASING(Expo, easeIn);
    STR_RETURN_EASING(Expo, easeOut);
    STR_RETURN_EASING(Expo, easeInOut);

    STR_RETURN_EASING(Linear, easeIn);
    STR_RETURN_EASING(Linear, easeOut);
    STR_RETURN_EASING(Linear, easeInOut);

    STR_RETURN_EASING(Quad, easeIn);
    STR_RETURN_EASING(Quad, easeOut);
    STR_RETURN_EASING(Quad, easeInOut);

    STR_RETURN_EASING(Quart, easeIn);
    STR_RETURN_EASING(Quart, easeOut);
    STR_RETURN_EASING(Quart, easeInOut);

    STR_RETURN_EASING(Quint, easeIn);
    STR_RETURN_EASING(Quint, easeOut);
    STR_RETURN_EASING(Quint, easeInOut);

    STR_RETURN_EASING(Sine, easeIn);
    STR_RETURN_EASING(Sine, easeOut);
    STR_RETURN_EASING(Sine, easeInOut);

    STR_RETURN_EASING(PetriHacks, sinGoTo2AndBack);
    STR_RETURN_EASING(PetriHacks, dimishingShakeBack);
    STR_RETURN_EASING(PetriHacks, dimishingShake);
    STR_RETURN_EASING(PetriHacks, littleBackAndForth);

#undef STR_RETURN_EASING

    return Linear::easeIn;
}

//-----------------------------------------------------------------------------

}  // end of namespace easing
}  // end of namespace BaseEngine
