

#ifndef INC_MATHUTILS_H
#define INC_MATHUTILS_H

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

#include "core/const.h"

#ifndef INC_CVECTOR2_H
#define INC_CVECTOR2_H

#include <math.h>

namespace BaseEngine {
namespace math {

//=============================================================================
// Still in the works templated vector class

template <class Type>
class CVector2 {
public:
    typedef Type unit_type;
    //=========================================================================

    CVector2() : x(Type()), y(Type()) {}

    CVector2(Type x, Type y) : x(x), y(y) {}

    template <class T>
    explicit CVector2(const T &other) : x((Type)(other.x)), y((Type)(other.y)) {}

    void Set(Type x_, Type y_) {
        x = x_;
        y = y_;
    }

    //=========================================================================

    bool operator==(const CVector2<Type> &other) const { return (this->x == other.x && this->y == other.y); }
    bool operator!=(const CVector2<Type> &other) const { return !operator==(other); }

    //=========================================================================

    CVector2<Type> operator-() const { return CVector2<Type>(-x, -y); }

    CVector2<Type> &operator+=(const CVector2<Type> &v) {
        x += v.x;
        y += v.y;

        return *this;
    }

    CVector2<Type> &operator-=(const CVector2<Type> &v) {
        x -= v.x;
        y -= v.y;

        return *this;
    }

    CVector2<Type> &operator*=(float a) {
        x = (Type)(x * a);
        y = (Type)(y * a);

        return *this;
    }

    CVector2<Type> &operator/=(float a) {
        x = (Type)(x / a);
        y = (Type)(y / a);

        return *this;
    }

    //-------------------------------------------------------------------------

    CVector2<Type> operator+(const CVector2<Type> &other) const { return CVector2<Type>(this->x + other.x, this->y + other.y); }

    CVector2<Type> operator-(const CVector2<Type> &other) const { return CVector2<Type>(this->x - other.x, this->y - other.y); }

    CVector2<Type> operator*(float t) const { return CVector2<Type>((Type)(this->x * t), (Type)(this->y * t)); }

    CVector2<Type> operator/(float t) const { return CVector2<Type>((Type)(this->x / t), (Type)(this->y / t)); }

    //=========================================================================

    Type Dot(const CVector2<Type> &a, const CVector2<Type> &b) const { return a.x * b.x + a.y * b.y; }

    Type Cross(const CVector2<Type> &a, const CVector2<Type> &b) const { return a.x * b.y - a.y * b.x; }

    CVector2<Type> Cross(const CVector2<Type> &a, const Type &s) const { return CVector2<Type>(s * a.y, -s * a.x); }

    CVector2<Type> Cross(const Type &s, const CVector2<Type> &a) const { return CVector2<Type>(-s * a.y, s * a.x); }

    //=========================================================================

    Type LengthSquared() const { return (x * x + y * y); }

    float Length() const { return sqrtf((float)LengthSquared()); }

    CVector2<Type> Normalize() const {
        float d = Length();

        if (d > 0)
            return CVector2<Type>(Type(x / d), Type(y / d));
        else
            return CVector2<Type>(0, 0);
    }

    //=========================================================================
    // ripped from:
    // http://forums.indiegamer.com/showthread.php?t=10459

    Type Angle() const {
        CVector2<Type> normal = Normalize();
        Type angle = atan2(normal.y, normal.x);

        return angle;
    }

    Type Angle(const CVector2<Type> &x) const {
        Type dot = Dot(*this, x);
        Type cross = Cross(*this, x);

        // angle between segments
        Type angle = (Type)atan2(cross, dot);

        return angle;
    }

    CVector2<Type> &Rotate(Type angle_rad) {
        Type tx = x;
        x = (Type)x * (Type)cos(angle_rad) - y * (Type)sin(angle_rad);
        y = (Type)tx * (Type)sin(angle_rad) + y * (Type)cos(angle_rad);

        return *this;
    }

    CVector2<Type> &Rotate(const CVector2<Type> &centre, Type angle_rad) {
        CVector2<Type> D = *this - centre;
        D.Rotate(angle_rad);

        // *this = xCentre + D;
        D += centre;
        Set(D.x, D.y);

        return *this;
    }

    //=========================================================================

    Type x;
    Type y;
};

}  // namespace math
}  // end of namespace BaseEngine

// ---------- types ----------
namespace types {
typedef BaseEngine::math::CVector2<float> vector2;
typedef BaseEngine::math::CVector2<double> dvector2;
typedef BaseEngine::math::CVector2<int> ivector2;

typedef BaseEngine::math::CVector2<int> point;

}  // end of namespace types

#endif

#ifndef INC_CENG_MATH_CMAT22_H
#define INC_CENG_MATH_CMAT22_H

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

namespace BaseEngine {
namespace math {

// typedef CVector2< float > Vec2;

//-----------------------------------------------------------------------------
template <class Type>
struct CMat22 {
    CMat22() { SetIdentity(); }
    CMat22(float angle) {
        Type c = (Type)cos(angle), s = (Type)sin(angle);
        col1.x = c;
        col2.x = -s;
        col1.y = s;
        col2.y = c;
    }

    CMat22(const CVector2<Type> &col1, const CVector2<Type> &col2) : col1(col1), col2(col2) {}

    CMat22(Type c1x, Type c1y, Type c2x, Type c2y) : col1(c1x, c1y), col2(c2x, c2y) {}

    CMat22<Type> Transpose() const { return CMat22<Type>(CVector2<Type>(col1.x, col2.x), CVector2<Type>(col1.y, col2.y)); }

    /// Initialize this matrix using columns.
    void Set(const CVector2<Type> &c1, const CVector2<Type> &c2) {
        col1 = c1;
        col2 = c2;
    }

    /// Initialize this matrix using an angle. This matrix becomes
    /// an orthonormal rotation matrix.
    void Set(float angle) {
        float c = cosf((float)angle), s = sinf((float)angle);
        col1.x = (Type)c;
        col2.x = (Type)(-s);
        col1.y = (Type)s;
        col2.y = (Type)c;
    }

    /// Set this to the identity matrix.
    void SetIdentity() {
        col1.x = (Type)1;
        col2.x = (Type)0;
        col1.y = (Type)0;
        col2.y = (Type)1;
    }

    /// Set this matrix to all zeros.
    void SetZero() {
        col1.x = 0;
        col2.x = 0;
        col1.y = 0;
        col2.y = 0;
    }

    /// Extract the angle from this matrix (assumed to be
    /// a rotation matrix).
    float GetAngle() const { return static_cast<float>(atan2(col1.y, col1.x)); }

    void SetAngle(float angle) { Set(angle); }

    CMat22<Type> Invert() const {
        Type a = col1.x, b = col2.x, c = col1.y, d = col2.y;
        CMat22<Type> B;
        Type det = a * d - b * c;
        assert(det != 0.0f);
        float fdet = 1.0f / (float)det;
        B.col1.x = Type(fdet * d);
        B.col2.x = Type(-fdet * b);
        B.col1.y = Type(-fdet * c);
        B.col2.y = Type(fdet * a);
        return B;
    }

    CVector2<Type> col1, col2;
};

}  // end of namespace math
}  // end of namespace BaseEngine

//----------------- types --------------------------------------------

namespace types {
typedef BaseEngine::math::CMat22<float> mat22;
}  // end of namespace types

#endif

#ifndef INC_CENG_MATH_CXFORM_H
#define INC_CENG_MATH_CXFORM_H

namespace BaseEngine {
namespace math {

/// A transform contains translation and rotation. It is used to represent
/// the position and orientation of rigid frames.
template <typename Type>
struct CXForm {
    /// The default constructor does nothing (for performance).
    CXForm() : scale((Type)1, (Type)1) {}

    /// Initialize using a position vector and a rotation matrix.
    CXForm(const CVector2<Type> &position, const CMat22<Type> &R, const CVector2<Type> &scale) : position(position), R(R), scale(scale) {}

    const CXForm<Type> &operator=(const CXForm<Type> &other) {
        position = other.position;
        R = other.R;
        scale = other.scale;
        return *this;
    }

    /// Set this to the identity transform.
    void SetIdentity() {
        position.Set((Type)0, (Type)0);
        R.SetIdentity();
        scale.Set((Type)1, (Type)1);
    }

    CVector2<Type> position;
    CMat22<Type> R;
    CVector2<Type> scale;
};

}  // end of namespace math
}  // End of namespace BaseEngine

// -------------- types --------------------------

namespace types {
typedef BaseEngine::math::CXForm<float> xform;
}  // end of namespace types

#endif

#ifndef INC_MATH_FUNCTIONS_H
#define INC_MATH_FUNCTIONS_H

namespace BaseEngine {
namespace math {

//-----------------------------------------------------------------------------
// returns a rounded value, if the value is less than 0, rounds it "down"
// otherwise rounds it rounds it up by 0.5f
float Round(float value);
double Round(double value);

inline float Round(float value) {
    if (value < 0)
        value -= 0.5f;
    else
        value += 0.5f;
    return floor(value);
}

inline double Round(double value) {
    if (value < 0)
        value -= 0.5;
    else
        value += 0.5;
    return floor(value);
}

//=============================================================================
// Min -function returns the smaller one
//
// Required operators: < -operator with the same type
//
// Basically done because VC6 breaks the std::min and std::max, that fucker
//.............................................................................

template <typename Type>
inline const Type &Min(const Type &c1, const Type &c2) {
    return (c1 < c2) ? c1 : c2;
}

template <typename Type>
inline const Type &Min(const Type &c1, const Type &c2, const Type &c3) {
    return (c1 < Min(c2, c3)) ? c1 : Min(c2, c3);
}

template <typename Type>
inline const Type &Min(const Type &c1, const Type &c2, const Type &c3, const Type &c4) {
    return (c1 < Min(c2, c3, c4)) ? c1 : Min(c2, c3, c4);
}

//=============================================================================
// Max -function returns the bigger one
//
// Required operators: < -operator with the same type
//
// Basically done because VC6 breaks the std::min and std::max, that fucker
//.............................................................................

template <typename Type>
inline const Type &Max(const Type &c1, const Type &c2) {
    return (c1 < c2) ? c2 : c1;
}

template <typename Type>
inline const Type &Max(const Type &c1, const Type &c2, const Type &c3) {
    return (c1 < Max(c2, c3)) ? Max(c2, c3) : c1;
}

template <typename Type>
inline const Type &Max(const Type &c1, const Type &c2, const Type &c3, const Type &c4) {
    return (c1 < Max(c2, c3, c4)) ? Max(c2, c3, c4) : c1;
}

//=============================================================================
// Returns the absolute value of the variable
//
// Required operators: < operator and - operator.
//.............................................................................

template <typename Type>
inline Type Absolute(const Type &n) {
    /*
if( n < 0 )
return -n;
else
return n;
*/

    return BaseEngine::math::Max(n, -n);
}

//=============================================================================
// Clams a give value to the desired
//
// Required operators: < operator
//.............................................................................

template <typename Type>
inline Type Clamp(const Type &a, const Type &low, const Type &high) {
    return BaseEngine::math::Max(low, BaseEngine::math::Min(a, high));
}

//=============================================================================
// Swaps to elements with each other.
//
// Required operators: = operator
//.............................................................................

template <typename Type>
inline void Swap(Type &a, Type &b) {
    Type temp = a;
    a = b;
    b = temp;
}

//=============================================================================
// Returns Square value of the value
//
// Required operators: * operator
//.............................................................................

template <typename Type>
inline Type Square(Type x) {
    return x * x;
}

//=============================================================================

template <typename Type>
inline Type ConvertRadToAngle(const Type &rad) {
    return rad * (180.0f / PI);
}

template <typename Type>
inline Type ConvertAngleToRad(const Type &angle) {
    return angle * (PI / 180.0f);
}

//=============================================================================

template <typename Type>
inline float Distance(const Type &vector1, const Type &vector2) {
    Type d;
    d.x = vector1.x - vector2.x;
    d.y = vector1.y - vector2.y;

    return sqrtf((float)(d.x * d.x + d.y * d.y));
}

template <typename Type>
inline float Distance(const Type &vector1) {
    return sqrtf((float)(vector1.x * vector1.x + vector1.y * vector1.y));
}

template <typename Type>
inline float DistanceSquared(const Type &vector1, const Type &vector2) {
    Type d;
    d.x = vector1.x - vector2.x;
    d.y = vector1.y - vector2.y;

    return (float)(d.x * d.x + d.y * d.y);
}

template <typename Type>
inline float DistanceSquared(const Type &vector1) {
    return (float)(vector1.x * vector1.x + vector1.y * vector1.y);
}

//=============================================================================

template <typename Type>
inline Type Lerp(const Type &a, const Type &b, float t) {
    return (Type)(a + t * (b - a));
}

//=============================================================================

}  // end of namespace math
}  // end of namespace BaseEngine

#endif

namespace BaseEngine {
namespace math {

//-----------------------------------------------------------------------------

template <class Type>
Type Dot(const CVector2<Type> &a, const CVector2<Type> &b) {
    return a.x * b.x + a.y * b.y;
}

template <class Type>
Type Cross(const CVector2<Type> &a, const CVector2<Type> &b) {
    return a.x * b.y - a.y * b.x;
}

template <class Type>
CVector2<Type> Cross(const CVector2<Type> &a, Type s) {
    return CVector2<Type>(s * a.y, -s * a.x);
}
template <class Type>
CVector2<Type> Cross(Type s, const CVector2<Type> &a) {
    return CVector2<Type>(-s * a.y, s * a.x);
}

// ----------------------------------------------------------------------------

template <class Type>
CVector2<Type> operator*(const CMat22<Type> &A, const CVector2<Type> &v) {
    return CVector2<Type>(A.col1.x * v.x + A.col2.x * v.y, A.col1.y * v.x + A.col2.y * v.y);
}

template <class Type>
CVector2<Type> operator*(Type s, const CVector2<Type> &v) {
    return CVector2<Type>(s * v.x, s * v.y);
}

template <class Type>
CMat22<Type> operator+(const CMat22<Type> &A, const CMat22<Type> &B) {
    return CMat22<Type>(A.col1 + B.col1, A.col2 + B.col2);
}

template <class Type>
CMat22<Type> operator*(const CMat22<Type> &A, const CMat22<Type> &B) {
    return CMat22<Type>(A * B.col1, A * B.col2);
}

// ----------------------------------------------------------------------------

template <class Type>
CVector2<Type> Abs(const CVector2<Type> &a) {
    return CVector2<Type>(fabsf(a.x), fabsf(a.y));
}

template <class Type>
CMat22<Type> Abs(const CMat22<Type> &A) {
    return CMat22<Type>(Abs(A.col1), Abs(A.col2));
}

// ----------------------------------------------------------------------------

template <class Type>
inline Type Sign(Type value) {
    return value < (Type)0 ? (Type)-1 : (Type)1;
}

// ----------------------------------------------------------------------------

template <class Type>
CVector2<Type> Mul(const CVector2<Type> &a, const CVector2<Type> &b) {
    CVector2<Type> result(a.x * b.x, a.y * b.y);
    return result;
}

/// Multiply a matrix times a vector. If a rotation matrix is provided,
/// then this transforms the vector from one frame to another.
template <class Type, class Vector2>
Vector2 Mul(const CMat22<Type> &A, const Vector2 &v) {
    Vector2 u;
    u.x = A.col1.x * v.x + A.col2.x * v.y;
    u.y = A.col1.y * v.x + A.col2.y * v.y;
    // u.Set( A.col1.x * v.x + A.col2.x * v.y, A.col1.y * v.x + A.col2.y * v.y );
    return u;
}

/// Multiply a matrix transpose times a vector. If a rotation matrix is provided,
/// then this transforms the vector from one frame to another (inverse transform).
template <class Type>
CVector2<Type> MulT(const CMat22<Type> &A, const CVector2<Type> &v) {
    CVector2<Type> u;
    u.Set(Dot(v, A.col1), Dot(v, A.col2));
    return u;
}

template <class Type>
CVector2<Type> Mul(const CXForm<Type> &T, const CVector2<Type> &v) {
    return T.position + Mul(T.R, v);
}

#if 0
template< class Type, class Vector2 >
Vector2 Mul( const CXForm< Type >& T, const Vector2& v )
{
	Vector2 result;
	result.x = v.x * T.scale.x;
	result.y = v.y * T.scale.y;
	result = Mul( T.R, result );
	result.x += T.position.x;
	result.y += T.position.y;
	return result;
}
#endif

template <class Type, class Vector2>
Vector2 MulWithScale(const CXForm<Type> &T, const Vector2 &v) {
    Vector2 result;
    result.x = v.x * T.scale.x;
    result.y = v.y * T.scale.y;
    result = Mul(T.R, result);
    result.x += T.position.x;
    result.y += T.position.y;
    return result;
}

template <class Type>
CXForm<Type> Mul(const CXForm<Type> &T, const CXForm<Type> &v) {

    CXForm<Type> result;

    result.position.x = T.scale.x * v.position.x;
    result.position.y = T.scale.y * v.position.y;
    result.position = Mul(T.R, result.position);
    result.position += T.position;

    result.R = Mul(T.R, v.R);
    result.scale.x = T.scale.x * v.scale.x;
    result.scale.y = T.scale.y * v.scale.y;

    return result;
}

template <class Type>
CVector2<Type> MulT(const CXForm<Type> &T, const CVector2<Type> &v) {
    return MulT(T.R, v - T.position);
}

template <class Type>
CXForm<Type> MulT(const CXForm<Type> &T, const CXForm<Type> &v) {

    CXForm<Type> result;

    result.scale.x = v.scale.x / T.scale.x;
    result.scale.y = v.scale.y / T.scale.y;
    result.R = Mul(T.R.Invert(), v.R);

    result.position.x = (v.position.x - T.position.x) / T.scale.x;
    result.position.y = (v.position.y - T.position.y) / T.scale.y;
    // result.position = v.position - T.position;
    result.position = MulT(T.R, result.position);

    return result;
}

template <class Type>
CVector2<Type> MulTWithScale(const CXForm<Type> &T, const CVector2<Type> &v) {
    CVector2<Type> result(v);
    if (T.scale.x != 0) result.x /= T.scale.x;
    if (T.scale.y != 0) result.y /= T.scale.y;
    return MulT(T.R, result - T.position);
}

// A * B
template <class Type>
inline CMat22<Type> Mul(const CMat22<Type> &A, const CMat22<Type> &B) {
    CMat22<Type> C;
    C.Set(Mul(A, B.col1), Mul(A, B.col2));
    return C;
}

// ----------------------------------------------------------------------------

template <class Type>
inline CVector2<float> ClosestPointOnLineSegment(const CVector2<Type> &a, const CVector2<Type> &b, const CVector2<Type> &p) {
    CVector2<float> c(p - a);
    CVector2<float> v(b - a);
    float distance = v.Length();

    // optimized normalized
    // v = v.Normalise();
    if (distance != 0) {
        v.x /= distance;
        v.y /= distance;
    }

    float t = (float)Dot(v, c);

    if (t < 0) return CVector2<float>(a);

    if (t > distance) return CVector2<float>(b);

    v *= t;

    return CVector2<float>(a) + v;
}

// ----------------------------------------------------------------------------

template <class Type>
inline float DistanceFromLineSquared(const CVector2<Type> &a, const CVector2<Type> &b, const CVector2<Type> &p) {
    CVector2<float> delta = ClosestPointOnLineSegment(a, b, p) - CVector2<float>(p);

    return delta.LengthSquared();
}

// ----------------------------------------------------------------------------

template <class Type>
inline float DistanceFromLine(const CVector2<Type> &a, const CVector2<Type> &b, const CVector2<Type> &p) {
    return sqrtf((float)DistanceFromLineSquared(a, b, p));
}

// ----------------------------------------------------------------------------

template <class T>
bool LineIntersection(const CVector2<T> &startA, const CVector2<T> &endA, const CVector2<T> &startB, const CVector2<T> &endB, CVector2<T> &result) {

    // TODO: reuse mathutil.intersect
    float d = (endB.y - startB.y) * (endA.x - startA.x) - (endB.x - startB.x) * (endA.y - startA.y);

    if (d == 0)  // parallel lines
        return false;

    float uA = (endB.x - startB.x) * (startA.y - startB.y) - (endB.y - startB.y) * (startA.x - startB.x);
    uA /= d;
    float uB = (endA.x - startA.x) * (startA.y - startB.y) - (endA.y - startA.y) * (startA.x - startB.x);
    uB /= d;

    if (uA < 0 || uA > 1 || uB < 0 || uB > 1) return false;  // intersection point isn't between the start and endpoints

    result.Set(startA.x + uA * (endA.x - startA.x), startA.y + uA * (endA.y - startA.y));

    // result = position;
    return true;
}

// ----------------------------------------------------------------------------

template <class PType>
float DistanceFromAABB(const CVector2<PType> &point, const CVector2<PType> &aabb_min, const CVector2<PType> &aabb_max) {
    if (IsPointInsideAABB(point, aabb_min, aabb_max)) return 0;

    float lowest = 0;
    float temp = BaseEngine::math::DistanceFromLineSquared(aabb_min, CVector2<PType>(aabb_max.x, aabb_min.y), point);
    if (temp < lowest) lowest = temp;

    temp = BaseEngine::math::DistanceFromLineSquared(CVector2<PType>(aabb_max.x, aabb_min.y), CVector2<PType>(aabb_max.x, aabb_max.y), point);
    if (temp < lowest) lowest = temp;

    temp = BaseEngine::math::DistanceFromLineSquared(CVector2<PType>(aabb_max.x, aabb_max.y), CVector2<PType>(aabb_min.x, aabb_max.y), point);
    if (temp < lowest) lowest = temp;

    temp = BaseEngine::math::DistanceFromLineSquared(CVector2<PType>(aabb_min.x, aabb_max.y), CVector2<PType>(aabb_min.x, aabb_min.y), point);
    if (temp < lowest) lowest = temp;

    return sqrtf(lowest);
}

// ----------------------------------------------------------------------------
}  // end of namespace math
}  // end of namespace BaseEngine

#ifndef INC_MATH_CAVERAGER_H
#define INC_MATH_CAVERAGER_H

namespace BaseEngine {
namespace math {

///////////////////////////////////////////////////////////////////////////////

template <class T>
class CAverager {
public:
    CAverager() : value(T()), current_value(T()), count(0) {}

    virtual ~CAverager() {}

    virtual void Reset() {
        value = T();
        count = 0;
    }

    virtual T Add(const T &other) {
        current_value = other;
        value += other;
        count++;

        return GetAverage();
    }

    T operator+=(const T &other) { return Add(other); }

    T GetAverage() const {
        if (count == 0) return T();

        return (T)(value / (T)count);
    }

    T GetCurrent() const { return current_value; }
    T GetTotal() const { return value; }
    unsigned int GetCount() const { return count; }

private:
    T value;
    T current_value;
    unsigned int count;
};

///////////////////////////////////////////////////////////////////////////////

}  // end of namespace math
}  // end of namespace BaseEngine

#endif

#ifndef INC_POINTINSIDE_H
#define INC_POINTINSIDE_H

#include <vector>

struct b2AABB;

namespace BaseEngine {
namespace math {

typedef float PointType;

bool IsPointInsideRect(const CVector2<PointType> &point, const CVector2<PointType> &position, const CVector2<PointType> &width, float rotation);

bool IsPointInsideCircle(const CVector2<PointType> &point, const CVector2<PointType> &position, float d);

bool IsPointInsidePolygon(const CVector2<PointType> &point, const std::vector<CVector2<PointType>> &polygon);
bool IsPointInsidePolygon(const CVector2<int> &point, const std::vector<CVector2<int>> &polygon);

bool IsPointInsidePolygon_Better(const CVector2<PointType> &point, const std::vector<CVector2<PointType>> &pgon);

template <class Type>
bool IsPointInsideAABB(const Type &point, const Type &rect_low, const Type &rect_high) {
    return (rect_low.x <= point.x && rect_high.x > point.x && rect_low.y <= point.y && rect_high.y > point.y);
}

bool DoesLineAndBoxCollide(const CVector2<PointType> &p1, const CVector2<PointType> &p2, const CVector2<PointType> &rect_low, const CVector2<PointType> &rect_high);

bool TestLineAABB(const CVector2<PointType> &p1, const CVector2<PointType> &p2, const CVector2<PointType> &rect_low, const CVector2<PointType> &rect_high);

template <class PType>
bool TestAABBAABBIntersection(const CVector2<PType> &a_min, const CVector2<PType> &a_max, const CVector2<PType> &b_min, const CVector2<PType> &b_max) {
    if (a_max.x < b_min.x || a_max.y < b_min.y || a_min.x > b_max.x || a_min.y > b_max.y) {
        return false;
    }
    return true;
}

}  // end of namespace math
}  // end of namespace BaseEngine

#endif

#ifndef INC_MATH_CSTATISTICSHELPER_H
#define INC_MATH_CSTATISTICSHELPER_H

#include <limits>

namespace BaseEngine {
namespace math {

///////////////////////////////////////////////////////////////////////////////

template <class T>
class CStatisticsHelper : public math::CAverager<T> {
public:
    CStatisticsHelper() : CAverager<T>(), minValue(std::numeric_limits<T>::max()), maxValue(std::numeric_limits<T>::min()) {}

    virtual void Reset() {
        minValue = std::numeric_limits<T>::max();
        maxValue = std::numeric_limits<T>::min();

        CAverager<T>::Reset();
    }

    virtual T Add(const T &other) {
        if (other < minValue) minValue = other;
        if (other > maxValue) maxValue = other;

        return CAverager<T>::Add(other);
    }

    T GetMin() const { return minValue; }

    T GetMax() const { return maxValue; }

private:
    T minValue;
    T maxValue;
};
///////////////////////////////////////////////////////////////////////////////

}  // namespace math
}  // end of namespace BaseEngine

#endif

#ifndef INC_CANGLE_H
#define INC_CANGLE_H

#include <math.h>

namespace BaseEngine {
namespace math {

template <typename T>
class CAngle {
    static const T TWO_PI;

    T AngleMod(T value) {
        while (value < 0) {
            value += TWO_PI;
        }

        value = static_cast<T>(fmod((double)value, (double)TWO_PI));

        return value;
    }

public:
    CAngle() : mAng(T()) {}
    CAngle(const CAngle &other) : mAng(other.mAng) {}
    CAngle(T value) { SetValue(value); }

    T GetValue() const { return mAng; }
    void SetValue(T value) { mAng = AngleMod(value); }

    void operator+=(const CAngle &other) {
        mAng += other.mAng;
        if (mAng >= TWO_PI) mAng -= TWO_PI;

        METADOT_ASSERT_E(mAng >= 0 && mAng <= TWO_PI);
    }

    void operator-=(const CAngle &other) {
        mAng -= other.mAng;
        if (mAng < 0) mAng += TWO_PI;

        METADOT_ASSERT_E(mAng >= 0 && mAng <= TWO_PI);
    }

    CAngle operator+(const CAngle &other) const {
        CAngle result;
        result.mAng = mAng + other.mAng;
        if (result.mAng >= TWO_PI) result.mAng -= TWO_PI;

        return result;
    }

    CAngle operator-(const CAngle &other) const {
        CAngle result;
        result.mAng = mAng - other.mAng;
        if (result.mAng < 0) result.mAng += TWO_PI;

        return result;
    }

    bool operator==(const CAngle &other) const { return GetValue() == other.GetValue(); }

private:
    T mAng;
};

template <typename T>
const T CAngle<T>::TWO_PI = (T)(2 * PI);

}  // end of namespace math
}  // namespace BaseEngine

// ---------------- types ---------------------

namespace types {
typedef BaseEngine::math::CAngle<float> angle;
}  // end of namespace types

#endif

#endif
