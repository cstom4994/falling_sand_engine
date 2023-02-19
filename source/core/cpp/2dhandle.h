
#ifndef INC_CRECT_H
#define INC_CRECT_H

// #include "Poro/utils/../config/cengtypes.h"
#include "core/math/mathlib.hpp"

namespace MetaEngine {

/*
template< class Ty >
CRect CreateRect( Ty& other );


template< class Ty >
CRect CreateRect( Ty* other );
*/

//! A class that defines a rectangle. Has x, y, w, h attributes. POD class
/*!
Used pretty much everywhere to indicate where a layer might be
*/
template <class T>
class CRect {
public:
    CRect() : x(0), y(0), w(T()), h(T()) {}

    CRect(const CRect<T> &other) : x(other.x), y(other.y), w(other.w), h(other.h) {}

    CRect(const T &inx, const T &iny, const T &inw, const T &inh) : x(inx), y(iny), w(inw), h(inh) {}

    bool operator==(const CRect<T> &other) const {
        if (x == other.x && y == other.y && w == other.w && h == other.h)
            return true;
        else
            return false;
    }

    bool operator!=(const CRect<T> &other) const { return !(operator==(other)); }

    /*
//! Tells us if this rect is inside the other rect
bool operator<=( const CRect< T >& other ) const;

//! Tells us if this rect is outside the other rect
bool operator>=( const CRect< T >& other ) const;
*/

    bool empty() const { return (w == T() && h == T()); }

    // for the use of resizing automagicly when screen is resized
    void Resize(const T &inw, const T &inh) {
        w = inw;
        h = inh;
    }

    //-------------------------------------------------------------------------

    CRect<T> operator-() const { return CRect<T>(-x, -y, -w, -h); }

    CRect<T> &operator+=(CRect<T> &v) {
        x += v.x;
        y += v.y;
        w += v.w;
        h += v.h;

        return *this;
    }

    CRect<T> &operator-=(const CRect<T> &v) {
        x -= v.x;
        y -= v.y;
        w -= v.w;
        h -= v.h;

        return *this;
    }

    CRect<T> &operator*=(const T &a) {
        x *= a;
        y *= a;
        w *= a;
        h *= a;

        return *this;
    }

    //-------------------------------------------------------------------------

    CRect<T> operator+(const CRect<T> &other) const { return CRect<T>(this->x + other.x, this->y + other.y, w + other.w, h + other.h); }

    CRect<T> operator-(const CRect<T> &other) const { return CRect<T>(this->x - other.x, this->y - other.y, w - other.w, h - other.h); }

    CRect<T> operator*(float t) const { return CRect<T>(this->x * t, this->y * t, w * t, h * t); }

    //-------------------------------------------------------------------------

    T GetLeft() const { return x; }
    T GetRight() const { return x + w; }
    T GetTop() const { return y; }
    T GetBottom() const { return y + h; }

    //-------------------------------------------------------------------------

    T x;
    T y;
    T w;
    T h;
};

template <class Tx, class Ty>
CRect<Tx> CreateRect(Ty &other) {
    return CRect<Tx>(other.GetX(), other.GetY(), other.GetW(), other.GetH());
}

template <class Tx, class Ty>
CRect<Tx> CreateRect(Ty *other) {
    return CRect<Tx>(other->GetX(), other->GetY(), other->GetW(), other->GetH());
}

template <class Tx, class Ty>
Ty CRectCast(const CRect<Tx> &other) {
    Ty tmp;
    tmp.x = other.x;
    tmp.y = other.y;
    tmp.w = other.w;
    tmp.h = other.h;

    return tmp;
}

}  // end of namespace MetaEngine

// ---------- types -------------------------

namespace types {
typedef MetaEngine::CRect<float> rect;
typedef MetaEngine::CRect<int> irect;
}  // end of namespace types

namespace MetaEngine {

//! Creates a empty rect
template <class TRect>
void RectMakeEmpty(TRect &rect);

//! Weather two rects hit each other or not
template <class TRect>
bool RectHit(const TRect &first, const TRect &second);

//! Tells us if the Rect a is inside the rect b and this meeans a complite inside operation
template <class TRect>
bool RectIsInside(const TRect &inside, const TRect &thiss);

//! Tells us if the point x, y is inside the rect
template <class TRect, class PType>
bool RectIsInside(const TRect &inside, PType x, PType y);

//! Tells us if the point p is inside the rect
template <class TRect, class TPoint>
bool IsPointInsideRect(const TPoint &point, const TRect &rect);

//! Adds a rect to a rect so that no rect is on top of any other rect in that list
template <class TRect>
void RectAddToList(std::list<TRect> &list, TRect rect);

//! Clamps the rect inside the borders, moves the rect so it doesn't go outside
template <class TRect>
void RectClampInside(TRect &inside, const TRect &borders);

//! Chops the first one so that it doesn't go outside the rect borders
template <class TRect>
void RectKeepInside(TRect &inside, const TRect &borders);

//! Makes a boolean operation for the first and second rect. uses the third if its neccisary.
/*!
 eeeee
 iooooo
 iooooo
 iooooo
 iooooo
  ooooo

 I found out that if you have two cubes and you want to boolean operate them
 ( chop them into other cubes so they dont touch each other ) you can always
 do it with max three cubes. The least is one and the max is three.

 So that is what this does
*/
template <class TRect>
void RectBoolean(TRect &first, TRect &second, TRect &third);

//! this chops the second into pieces around the first rect and pushes those pieces to left_overs
template <class TRect>
void RectBooleanConst(const TRect &first, const TRect &second, std::vector<TRect> &left_overs);

//! this chops the second into pieces around the first rect and pushes those pieces to left_overs
template <class TRect>
void RectBooleanConst(const TRect &first, const TRect &second, std::list<TRect> &left_overs);

//! Finds cross points of two rects and pushes them in to the vector. Uses CRectFindCrossPointsLine()
//! for finding the crosspoints
template <class TRect, class TPoint>
void RectFindCrossPoints(const TRect &first, const TRect &second, std::vector<TPoint> &points);

/*!
Works only with straight lines. Used by CRectFindCrossPoints(),
and this is a quite delicate piece of coding, the first has to first.
� mean it has to be with lower x and y values then the end. Doesn't
bother me cause, every line that comes from CRectFindCrossPoints
is sorted this way, but if you want use this generally i expect
you to sort things out. Or code a max in front of things. But
for optimation sake i use it this way.
*/
template <class TPoint>
TPoint RectFindCrossPointsLine(const TPoint &first_begin, const TPoint &first_end, const TPoint &second_begin, const TPoint &second_end);

///////////////////////////////////////////////////////////////////////////////

//! 2d coordination class. Basic POD class
template <class T>
class CPoint {
public:
    //=========================================================================

    CPoint() : x(T()), y(T()) {}

    CPoint(const T &inx, const T &iny) : x(inx), y(iny) {}

    CPoint(const CPoint<T> &other) : x(other.x), y(other.y) {}

    //=========================================================================

    bool operator==(const CPoint<T> &other) const { return (this->x == other.x && this->y == other.y); }

    bool operator!=(const CPoint<T> &other) const { return !operator==(other); }

    //=========================================================================

    CPoint<T> operator+(const CPoint<T> &other) const { return CPoint<T>(x + other.x, y + other.y); }

    CPoint<T> operator-(const CPoint<T> &other) const { return CPoint<T>(x - other.x, y - other.y); }

    CPoint<T> operator*(const CPoint<T> &other) const { return CPoint<T>(x * other.x, y * other.y); }

    CPoint<T> operator/(const CPoint<T> &other) const { return CPoint<T>(x / other.x, y / other.y); }

    //=========================================================================

    CPoint<T> &operator+=(const CPoint<T> &other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    CPoint<T> &operator-=(const CPoint<T> &other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    CPoint<T> &operator*=(const CPoint<T> &other) {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    CPoint<T> &operator/=(const CPoint<T> &other) {
        x /= other.x;
        y /= other.y;
        return *this;
    }

    //=========================================================================

    // bool empty() { return ( x == -1 && y == -1 ); }

    T x;
    T y;
};

typedef CPoint<int> CCoord;

template <class VectorT, class RectT>
bool CircleRectCollide(const VectorT &circle_p, float circle_r, const RectT &rect) {
    if (MetaEngine::RectHit(RectT(circle_p.x - circle_r, circle_p.y - circle_r, circle_r * 2.f, circle_r * 2.f), rect) == false) {
        return false;
    }

    // go through the lines of the box and check collision agains them
    std::vector<VectorT> rect_p(4);
    rect_p[0].Set(rect.x, rect.y);
    rect_p[1].Set(rect.x + rect.w, rect.y);
    rect_p[2].Set(rect.x + rect.w, rect.y + rect.h);
    rect_p[3].Set(rect.x, rect.y + rect.h);

    const float circle_r_squared = MetaEngine::math::Square(circle_r);
    float dist = 0;
    for (std::size_t i = 0; i < rect_p.size(); ++i) {
        dist = MetaEngine::math::DistanceFromLineSquared(rect_p[i], rect_p[(i + 1) % 4], circle_p);
        if (dist <= circle_r_squared) return true;
    }

    return false;
}

template <class VectorT, class RectT>
VectorT CircleRectResolveByPushingCircle(const VectorT &circle_p, float circle_r, const RectT &rect) {
    // go through the lines of the box and check collision agains them
    // we go through the lines and we find the closest position that we collide with
    std::vector<VectorT> rect_p(4);
    rect_p[0].Set(rect.x, rect.y);
    rect_p[1].Set(rect.x + rect.w, rect.y);
    rect_p[2].Set(rect.x + rect.w, rect.y + rect.h);
    rect_p[3].Set(rect.x, rect.y + rect.h);

    float dist = 0;
    VectorT point;
    float closest_dist = MetaEngine::math::Square(circle_r);
    VectorT closest_point;
    for (std::size_t i = 0; i < rect_p.size(); ++i) {
        point = MetaEngine::math::ClosestPointOnLineSegment(rect_p[i], rect_p[(i + 1) % 4], circle_p);
        dist = (circle_p - point).LengthSquared();

        if (dist < closest_dist) {
            closest_dist = dist;
            closest_point = point;
        }
    }

    const float extra_push = 0.1f;

    VectorT delta = circle_p - closest_point;
    delta = (delta.Normalize() * (circle_r + extra_push));

    // to fix it so that if the point is inside the box it's pushed outside
    if (MetaEngine::math::IsPointInsideAABB(circle_p, rect_p[0], rect_p[2])) delta = -delta;

    return closest_point + delta;
}

template <class VectorT>
bool CircleCircleCollide(const VectorT &p1, float r1, const VectorT &p2, float r2) {
    const VectorT delta = p2 - p1;
    if (delta.LengthSquared() <= MetaEngine::math::Square(r1 + r2)) {
        return true;
    }

    return false;
}

template <class VectorT>
VectorT CircleCircleResolveByPushingCircle(const VectorT &push_me_pos, float push_me_r, const VectorT &immovable_pos, float immovable_r) {
    const float extra_push = 0.1f;

    VectorT delta = push_me_pos - immovable_pos;
    delta = (delta.Normalize() * (push_me_r + immovable_r + extra_push));

    return immovable_pos + delta;
}

namespace easing {

//-------------------------------------------------------------------------

struct IEasingFunc {
    virtual ~IEasingFunc() {}

    float operator()(float x) { return f(x); }

    virtual float f(float x) = 0;
};

//-------------------------------------------------------------------------

IEasingFunc &GetEasing(const char *easing_name);

//-------------------------------------------------------------------------

class Back {
public:
    struct EaseIn : public IEasingFunc {
        virtual float f(float x) { return impl_easeIn(x, 0, 1, 1); }
    };

    struct EaseOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeOut(x, 0, 1, 1); }
    };

    struct EaseInOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeInOut(x, 0, 1, 1); }
    };

    static EaseIn easeIn;
    static EaseOut easeOut;
    static EaseInOut easeInOut;

    static float impl_easeIn(float t, float b, float c, float d);
    static float impl_easeOut(float t, float b, float c, float d);
    static float impl_easeInOut(float t, float b, float c, float d);
};

//-------------------------------------------------------------------------

class Bounce {
public:
    struct EaseIn : public IEasingFunc {
        virtual float f(float x) { return impl_easeIn(x, 0, 1, 1); }
    };

    struct EaseOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeOut(x, 0, 1, 1); }
    };

    struct EaseInOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeInOut(x, 0, 1, 1); }
    };

    static EaseIn easeIn;
    static EaseOut easeOut;
    static EaseInOut easeInOut;

    static float impl_easeIn(float t, float b, float c, float d);
    static float impl_easeOut(float t, float b, float c, float d);
    static float impl_easeInOut(float t, float b, float c, float d);
};

//-------------------------------------------------------------------------

class Circ {
public:
    struct EaseIn : public IEasingFunc {
        virtual float f(float x) { return impl_easeIn(x, 0, 1, 1); }
    };

    struct EaseOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeOut(x, 0, 1, 1); }
    };

    struct EaseInOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeInOut(x, 0, 1, 1); }
    };

    static EaseIn easeIn;
    static EaseOut easeOut;
    static EaseInOut easeInOut;

    static float impl_easeIn(float t, float b, float c, float d);
    static float impl_easeOut(float t, float b, float c, float d);
    static float impl_easeInOut(float t, float b, float c, float d);
};

//-------------------------------------------------------------------------

class Cubic {

public:
    struct EaseIn : public IEasingFunc {
        virtual float f(float x) { return impl_easeIn(x, 0, 1, 1); }
    };

    struct EaseOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeOut(x, 0, 1, 1); }
    };

    struct EaseInOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeInOut(x, 0, 1, 1); }
    };

    static EaseIn easeIn;
    static EaseOut easeOut;
    static EaseInOut easeInOut;

    static float impl_easeIn(float t, float b, float c, float d);
    static float impl_easeOut(float t, float b, float c, float d);
    static float impl_easeInOut(float t, float b, float c, float d);
};

//-------------------------------------------------------------------------

class Elastic {
public:
    struct EaseIn : public IEasingFunc {
        virtual float f(float x) { return impl_easeIn(x, 0, 1, 1); }
    };

    struct EaseOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeOut(x, 0, 1, 1); }
    };

    struct EaseInOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeInOut(x, 0, 1, 1); }
    };

    static EaseIn easeIn;
    static EaseOut easeOut;
    static EaseInOut easeInOut;

    static float impl_easeIn(float t, float b, float c, float d);
    static float impl_easeOut(float t, float b, float c, float d);
    static float impl_easeInOut(float t, float b, float c, float d);
};

//-------------------------------------------------------------------------

class Expo {
public:
    struct EaseIn : public IEasingFunc {
        virtual float f(float x) { return impl_easeIn(x, 0, 1, 1); }
    };

    struct EaseOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeOut(x, 0, 1, 1); }
    };

    struct EaseInOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeInOut(x, 0, 1, 1); }
    };

    static EaseIn easeIn;
    static EaseOut easeOut;
    static EaseInOut easeInOut;

    static float impl_easeIn(float t, float b, float c, float d);
    static float impl_easeOut(float t, float b, float c, float d);
    static float impl_easeInOut(float t, float b, float c, float d);
};

//-------------------------------------------------------------------------

class Linear {
public:
    struct EaseNone : public IEasingFunc {
        virtual float f(float x) { return impl_easeNone(x, 0, 1, 1); }
    };

    struct EaseIn : public IEasingFunc {
        virtual float f(float x) { return impl_easeIn(x, 0, 1, 1); }
    };

    struct EaseOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeOut(x, 0, 1, 1); }
    };

    struct EaseInOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeInOut(x, 0, 1, 1); }
    };

    static EaseNone easeNone;
    static EaseIn easeIn;
    static EaseOut easeOut;
    static EaseInOut easeInOut;

    static float impl_easeNone(float t, float b, float c, float d);  // ??
    static float impl_easeIn(float t, float b, float c, float d);
    static float impl_easeOut(float t, float b, float c, float d);
    static float impl_easeInOut(float t, float b, float c, float d);
};

//-------------------------------------------------------------------------

class Quad {

public:
    struct EaseIn : public IEasingFunc {
        virtual float f(float x) { return impl_easeIn(x, 0, 1, 1); }
    };

    struct EaseOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeOut(x, 0, 1, 1); }
    };

    struct EaseInOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeInOut(x, 0, 1, 1); }
    };

    static EaseIn easeIn;
    static EaseOut easeOut;
    static EaseInOut easeInOut;

    static float impl_easeIn(float t, float b, float c, float d);
    static float impl_easeOut(float t, float b, float c, float d);
    static float impl_easeInOut(float t, float b, float c, float d);
};

//-------------------------------------------------------------------------

class Quart {
public:
    struct EaseIn : public IEasingFunc {
        virtual float f(float x) { return impl_easeIn(x, 0, 1, 1); }
    };

    struct EaseOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeOut(x, 0, 1, 1); }
    };

    struct EaseInOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeInOut(x, 0, 1, 1); }
    };

    static EaseIn easeIn;
    static EaseOut easeOut;
    static EaseInOut easeInOut;

    static float impl_easeIn(float t, float b, float c, float d);
    static float impl_easeOut(float t, float b, float c, float d);
    static float impl_easeInOut(float t, float b, float c, float d);
};

//-------------------------------------------------------------------------

class Quint {

public:
    struct EaseIn : public IEasingFunc {
        virtual float f(float x) { return impl_easeIn(x, 0, 1, 1); }
    };

    struct EaseOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeOut(x, 0, 1, 1); }
    };

    struct EaseInOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeInOut(x, 0, 1, 1); }
    };

    static EaseIn easeIn;
    static EaseOut easeOut;
    static EaseInOut easeInOut;

    static float impl_easeIn(float t, float b, float c, float d);
    static float impl_easeOut(float t, float b, float c, float d);
    static float impl_easeInOut(float t, float b, float c, float d);
};

//-------------------------------------------------------------------------

class Sine {
public:
    struct EaseIn : public IEasingFunc {
        virtual float f(float x) { return impl_easeIn(x, 0, 1, 1); }
    };

    struct EaseOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeOut(x, 0, 1, 1); }
    };

    struct EaseInOut : public IEasingFunc {
        virtual float f(float x) { return impl_easeInOut(x, 0, 1, 1); }
    };

    static EaseIn easeIn;
    static EaseOut easeOut;
    static EaseInOut easeInOut;

    static float impl_easeIn(float t, float b, float c, float d);
    static float impl_easeOut(float t, float b, float c, float d);
    static float impl_easeInOut(float t, float b, float c, float d);
};

//-------------------------------------------------------------------------

class PetriHacks {
public:
    // Back functions are functions that return to 0,
    // Why? well sometimes you need things to return nicely as well

    // x goes to 2 and then down to 0
    // good for in and out style effects.
    // Want something to drop and then dissapper... use this baby
    // sin(x*3.14f)*2.f
    struct SinGoTo2AndBack : public IEasingFunc {
        virtual float f(float x);
    };

    // Shakes between 0.9933 and -0.9407  and returns to 0
    // does like 5 shakes, think of shaking the angle of a tree when player bumps into it
    // ( ( cos( x * 3.1415 ) + 1 ) * 0.5 )  * sin( x * 30.0 )
    struct DimishingShakeBack : public IEasingFunc {
        virtual float f(float x);
    };

    // Shakes between 1.102 and -0.7851 and goes to 1
    // same as DimishingShakeBack but actually ends up at the target
    // does like 5 shakes, don't really know what to use this for, but
    // I'm sure I'll figure something out
    // ( ( cos( x * 3.1415 ) + 1 ) * 0.5 )  * sin( x * 30.0 ) + x
    struct DimishingShake : public IEasingFunc {
        virtual float f(float x);
    };

    // A function that goes to -0.1 and then to 1.0, like taking a bit of speed before launching
    // ((pow(x+0.17f, 1.3f) * sin(1.f/(x+0.17f))) * 1.043f) + 0.035f
    struct LittleBackAndForth : public IEasingFunc {
        virtual float f(float x);
    };

    //----------------

    static SinGoTo2AndBack sinGoTo2AndBack;
    static DimishingShakeBack dimishingShakeBack;
    static DimishingShakeBack dimishingShake;
    static LittleBackAndForth littleBackAndForth;
};

//-------------------------------------------------------------------------

}  // namespace easing
}  // end of namespace MetaEngine

#endif
