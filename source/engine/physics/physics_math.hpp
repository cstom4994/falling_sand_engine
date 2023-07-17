

#ifndef ME_PM_HPP
#define ME_PM_HPP

#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <list>
#include <set>

#include "engine/core/core.hpp"
#include "engine/core/mathlib.hpp"
#include "engine/core/sdl_wrapper.h"
// #include "engine/physics/inc/physics2d.h"

#define B2_NOT_USED(x) ((void)(x))

#define b2_maxFloat FLT_MAX
#define b2_epsilon FLT_EPSILON
#define b2_pi 3.14159265359f

/// This function is used to ensure that a floating point number is not a NaN or infinity.
inline bool b2IsValid(float x) { return isfinite(x); }

#define b2Sqrt(x) sqrtf(x)
#define b2Atan2(y, x) atan2f(y, x)

ME_INLINE auto RectToPoint(const MEvec2 &v) {
    MEvec2 a1 = {v.x / 2.0f, v.y / 2.0f};
    MEvec2 a2 = {v.x / -2.0f, v.y / 2.0f};
    MEvec2 b1 = {v.x / -2.0f, v.y / -2.0f};
    MEvec2 b2 = {v.x / 2.0f, v.y / -2.0f};
    return std::initializer_list<MEvec2>{a1, a2, b1, b2};
}

/// A 2D column vector.
struct PVec2 {
    /// Default constructor does nothing (for performance).
    PVec2() = default;

    /// Construct using coordinates.
    PVec2(float xIn, float yIn) : x(xIn), y(yIn) {}

    PVec2(const MEvec2 &v2) : x(v2.x), y(v2.y) {}

    operator MEvec2() const { return MEvec2{x, y}; }

    /// Set this vector to all zeros.
    void SetZero() {
        x = 0.0f;
        y = 0.0f;
    }

    /// Set this vector to some specified coordinates.
    void Set(float x_, float y_) {
        x = x_;
        y = y_;
    }

    /// Negate this vector.
    PVec2 operator-() const {
        PVec2 v;
        v.Set(-x, -y);
        return v;
    }

    /// Read from and indexed element.
    float operator()(i32 i) const { return (&x)[i]; }

    /// Write to an indexed element.
    float &operator()(i32 i) { return (&x)[i]; }

    /// Add a vector to this vector.
    void operator+=(const PVec2 &v) {
        x += v.x;
        y += v.y;
    }

    /// Subtract a vector from this vector.
    void operator-=(const PVec2 &v) {
        x -= v.x;
        y -= v.y;
    }

    /// Multiply this vector by a scalar.
    void operator*=(float a) {
        x *= a;
        y *= a;
    }

    /// Get the length of this vector (the norm).
    float Length() const { return b2Sqrt(x * x + y * y); }

    /// Get the length squared. For performance, use this instead of
    /// b2Vec2::Length (if possible).
    float LengthSquared() const { return x * x + y * y; }

    /// Convert this vector into a unit vector. Returns the length.
    float Normalize() {
        float length = Length();
        if (length < b2_epsilon) {
            return 0.0f;
        }
        float invLength = 1.0f / length;
        x *= invLength;
        y *= invLength;

        return length;
    }

    /// Does this vector contain finite coordinates?
    bool IsValid() const { return b2IsValid(x) && b2IsValid(y); }

    /// Get the skew vector such that dot(skew_vec, other) == cross(vec, other)
    PVec2 Skew() const { return PVec2(-y, x); }

    float x, y;
};

/// A 2D column vector with 3 elements.
struct PVec3 {
    /// Default constructor does nothing (for performance).
    PVec3() = default;

    /// Construct using coordinates.
    PVec3(float xIn, float yIn, float zIn) : x(xIn), y(yIn), z(zIn) {}

    /// Set this vector to all zeros.
    void SetZero() {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }

    /// Set this vector to some specified coordinates.
    void Set(float x_, float y_, float z_) {
        x = x_;
        y = y_;
        z = z_;
    }

    /// Negate this vector.
    PVec3 operator-() const {
        PVec3 v;
        v.Set(-x, -y, -z);
        return v;
    }

    /// Add a vector to this vector.
    void operator+=(const PVec3 &v) {
        x += v.x;
        y += v.y;
        z += v.z;
    }

    /// Subtract a vector from this vector.
    void operator-=(const PVec3 &v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
    }

    /// Multiply this vector by a scalar.
    void operator*=(float s) {
        x *= s;
        y *= s;
        z *= s;
    }

    float x, y, z;
};

/// A 2-by-2 matrix. Stored in column-major order.
struct PMat22 {
    /// The default constructor does nothing (for performance).
    PMat22() = default;

    /// Construct this matrix using columns.
    PMat22(const PVec2 &c1, const PVec2 &c2) {
        ex = c1;
        ey = c2;
    }

    /// Construct this matrix using scalars.
    PMat22(float a11, float a12, float a21, float a22) {
        ex.x = a11;
        ex.y = a21;
        ey.x = a12;
        ey.y = a22;
    }

    /// Initialize this matrix using columns.
    void Set(const PVec2 &c1, const PVec2 &c2) {
        ex = c1;
        ey = c2;
    }

    /// Set this to the identity matrix.
    void SetIdentity() {
        ex.x = 1.0f;
        ey.x = 0.0f;
        ex.y = 0.0f;
        ey.y = 1.0f;
    }

    /// Set this matrix to all zeros.
    void SetZero() {
        ex.x = 0.0f;
        ey.x = 0.0f;
        ex.y = 0.0f;
        ey.y = 0.0f;
    }

    PMat22 GetInverse() const {
        float a = ex.x, b = ey.x, c = ex.y, d = ey.y;
        PMat22 B;
        float det = a * d - b * c;
        if (det != 0.0f) {
            det = 1.0f / det;
        }
        B.ex.x = det * d;
        B.ey.x = -det * b;
        B.ex.y = -det * c;
        B.ey.y = det * a;
        return B;
    }

    /// Solve A * x = b, where b is a column vector. This is more efficient
    /// than computing the inverse in one-shot cases.
    PVec2 Solve(const PVec2 &b) const {
        float a11 = ex.x, a12 = ey.x, a21 = ex.y, a22 = ey.y;
        float det = a11 * a22 - a12 * a21;
        if (det != 0.0f) {
            det = 1.0f / det;
        }
        PVec2 x;
        x.x = det * (a22 * b.x - a12 * b.y);
        x.y = det * (a11 * b.y - a21 * b.x);
        return x;
    }

    PVec2 ex, ey;
};

/// A 3-by-3 matrix. Stored in column-major order.
struct b2Mat33 {
    /// The default constructor does nothing (for performance).
    b2Mat33() = default;

    /// Construct this matrix using columns.
    b2Mat33(const PVec3 &c1, const PVec3 &c2, const PVec3 &c3) {
        ex = c1;
        ey = c2;
        ez = c3;
    }

    /// Set this matrix to all zeros.
    void SetZero() {
        ex.SetZero();
        ey.SetZero();
        ez.SetZero();
    }

    /// Solve A * x = b, where b is a column vector. This is more efficient
    /// than computing the inverse in one-shot cases.
    PVec3 Solve33(const PVec3 &b) const;

    /// Solve A * x = b, where b is a column vector. This is more efficient
    /// than computing the inverse in one-shot cases. Solve only the upper
    /// 2-by-2 matrix equation.
    PVec2 Solve22(const PVec2 &b) const;

    /// Get the inverse of this matrix as a 2-by-2.
    /// Returns the zero matrix if singular.
    void GetInverse22(b2Mat33 *M) const;

    /// Get the symmetric inverse of this matrix as a 3-by-3.
    /// Returns the zero matrix if singular.
    void GetSymInverse33(b2Mat33 *M) const;

    PVec3 ex, ey, ez;
};

/// Rotation
struct PRot {
    PRot() = default;

    /// Initialize from an angle in radians
    explicit PRot(float angle) {
        /// TODO_ERIN optimize
        s = sinf(angle);
        c = cosf(angle);
    }

    /// Set using an angle in radians.
    void Set(float angle) {
        /// TODO_ERIN optimize
        s = sinf(angle);
        c = cosf(angle);
    }

    /// Set to the identity rotation
    void SetIdentity() {
        s = 0.0f;
        c = 1.0f;
    }

    /// Get the angle in radians
    float GetAngle() const { return b2Atan2(s, c); }

    /// Get the x-axis
    PVec2 GetXAxis() const { return PVec2(c, s); }

    /// Get the u-axis
    PVec2 GetYAxis() const { return PVec2(-s, c); }

    /// Sine and cosine
    float s, c;
};

/// A transform contains translation and rotation. It is used to represent
/// the position and orientation of rigid frames.
struct PTransform {
    /// The default constructor does nothing.
    PTransform() = default;

    /// Initialize using a position vector and a rotation.
    PTransform(const PVec2 &position, const PRot &rotation) : p(position), q(rotation) {}

    /// Set this to the identity transform.
    void SetIdentity() {
        p.SetZero();
        q.SetIdentity();
    }

    /// Set this based on the position and angle.
    void Set(const PVec2 &position, float angle) {
        p = position;
        q.Set(angle);
    }

    PVec2 p;
    PRot q;
};

/// This describes the motion of a body/shape for TOI computation.
/// Shapes are defined with respect to the body origin, which may
/// no coincide with the center of mass. However, to support dynamics
/// we must interpolate the center of mass position.
struct PSweep {
    PSweep() = default;

    /// Get the interpolated transform at a specific time.
    /// @param transform the output transform
    /// @param beta is a factor in [0,1], where 0 indicates alpha0.
    void GetTransform(PTransform *transform, float beta) const;

    /// Advance the sweep forward, yielding a new initial state.
    /// @param alpha the new initial time.
    void Advance(float alpha);

    /// Normalize the angles.
    void Normalize();

    PVec2 localCenter;  ///< local center of mass position
    PVec2 c0, c;        ///< center world positions
    float a0, a;        ///< world angles

    /// Fraction of the current time step in the range [0,1]
    /// c0 and a0 are the positions at alpha0.
    float alpha0;
};

/// Useful constant
extern const PVec2 b2Vec2_zero;

/// Perform the dot product on two vectors.
inline float b2Dot(const PVec2 &a, const PVec2 &b) { return a.x * b.x + a.y * b.y; }

/// Perform the cross product on two vectors. In 2D this produces a scalar.
inline float b2Cross(const PVec2 &a, const PVec2 &b) { return a.x * b.y - a.y * b.x; }

/// Perform the cross product on a vector and a scalar. In 2D this produces
/// a vector.
inline PVec2 b2Cross(const PVec2 &a, float s) { return PVec2(s * a.y, -s * a.x); }

/// Perform the cross product on a scalar and a vector. In 2D this produces
/// a vector.
inline PVec2 b2Cross(float s, const PVec2 &a) { return PVec2(-s * a.y, s * a.x); }

/// Multiply a matrix times a vector. If a rotation matrix is provided,
/// then this transforms the vector from one frame to another.
inline PVec2 b2Mul(const PMat22 &A, const PVec2 &v) { return PVec2(A.ex.x * v.x + A.ey.x * v.y, A.ex.y * v.x + A.ey.y * v.y); }

/// Multiply a matrix transpose times a vector. If a rotation matrix is provided,
/// then this transforms the vector from one frame to another (inverse transform).
inline PVec2 b2MulT(const PMat22 &A, const PVec2 &v) { return PVec2(b2Dot(v, A.ex), b2Dot(v, A.ey)); }

/// Add two vectors component-wise.
inline PVec2 operator+(const PVec2 &a, const PVec2 &b) { return PVec2(a.x + b.x, a.y + b.y); }

/// Subtract two vectors component-wise.
inline PVec2 operator-(const PVec2 &a, const PVec2 &b) { return PVec2(a.x - b.x, a.y - b.y); }

inline PVec2 operator*(float s, const PVec2 &a) { return PVec2(s * a.x, s * a.y); }

inline bool operator==(const PVec2 &a, const PVec2 &b) { return a.x == b.x && a.y == b.y; }

inline bool operator!=(const PVec2 &a, const PVec2 &b) { return a.x != b.x || a.y != b.y; }

inline float b2Distance(const PVec2 &a, const PVec2 &b) {
    PVec2 c = a - b;
    return c.Length();
}

inline float b2DistanceSquared(const PVec2 &a, const PVec2 &b) {
    PVec2 c = a - b;
    return b2Dot(c, c);
}

inline PVec3 operator*(float s, const PVec3 &a) { return PVec3(s * a.x, s * a.y, s * a.z); }

/// Add two vectors component-wise.
inline PVec3 operator+(const PVec3 &a, const PVec3 &b) { return PVec3(a.x + b.x, a.y + b.y, a.z + b.z); }

/// Subtract two vectors component-wise.
inline PVec3 operator-(const PVec3 &a, const PVec3 &b) { return PVec3(a.x - b.x, a.y - b.y, a.z - b.z); }

/// Perform the dot product on two vectors.
inline float b2Dot(const PVec3 &a, const PVec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

/// Perform the cross product on two vectors.
inline PVec3 b2Cross(const PVec3 &a, const PVec3 &b) { return PVec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }

inline PMat22 operator+(const PMat22 &A, const PMat22 &B) { return PMat22(A.ex + B.ex, A.ey + B.ey); }

// A * B
inline PMat22 b2Mul(const PMat22 &A, const PMat22 &B) { return PMat22(b2Mul(A, B.ex), b2Mul(A, B.ey)); }

// A^T * B
inline PMat22 b2MulT(const PMat22 &A, const PMat22 &B) {
    PVec2 c1(b2Dot(A.ex, B.ex), b2Dot(A.ey, B.ex));
    PVec2 c2(b2Dot(A.ex, B.ey), b2Dot(A.ey, B.ey));
    return PMat22(c1, c2);
}

/// Multiply a matrix times a vector.
inline PVec3 b2Mul(const b2Mat33 &A, const PVec3 &v) { return v.x * A.ex + v.y * A.ey + v.z * A.ez; }

/// Multiply a matrix times a vector.
inline PVec2 b2Mul22(const b2Mat33 &A, const PVec2 &v) { return PVec2(A.ex.x * v.x + A.ey.x * v.y, A.ex.y * v.x + A.ey.y * v.y); }

/// Multiply two rotations: q * r
inline PRot b2Mul(const PRot &q, const PRot &r) {
    // [qc -qs] * [rc -rs] = [qc*rc-qs*rs -qc*rs-qs*rc]
    // [qs  qc]   [rs  rc]   [qs*rc+qc*rs -qs*rs+qc*rc]
    // s = qs * rc + qc * rs
    // c = qc * rc - qs * rs
    PRot qr;
    qr.s = q.s * r.c + q.c * r.s;
    qr.c = q.c * r.c - q.s * r.s;
    return qr;
}

/// Transpose multiply two rotations: qT * r
inline PRot b2MulT(const PRot &q, const PRot &r) {
    // [ qc qs] * [rc -rs] = [qc*rc+qs*rs -qc*rs+qs*rc]
    // [-qs qc]   [rs  rc]   [-qs*rc+qc*rs qs*rs+qc*rc]
    // s = qc * rs - qs * rc
    // c = qc * rc + qs * rs
    PRot qr;
    qr.s = q.c * r.s - q.s * r.c;
    qr.c = q.c * r.c + q.s * r.s;
    return qr;
}

/// Rotate a vector
inline PVec2 b2Mul(const PRot &q, const PVec2 &v) { return PVec2(q.c * v.x - q.s * v.y, q.s * v.x + q.c * v.y); }

/// Inverse rotate a vector
inline PVec2 b2MulT(const PRot &q, const PVec2 &v) { return PVec2(q.c * v.x + q.s * v.y, -q.s * v.x + q.c * v.y); }

inline PVec2 b2Mul(const PTransform &T, const PVec2 &v) {
    float x = (T.q.c * v.x - T.q.s * v.y) + T.p.x;
    float y = (T.q.s * v.x + T.q.c * v.y) + T.p.y;

    return PVec2(x, y);
}

inline PVec2 b2MulT(const PTransform &T, const PVec2 &v) {
    float px = v.x - T.p.x;
    float py = v.y - T.p.y;
    float x = (T.q.c * px + T.q.s * py);
    float y = (-T.q.s * px + T.q.c * py);

    return PVec2(x, y);
}

// v2 = A.q.Rot(B.q.Rot(v1) + B.p) + A.p
//    = (A.q * B.q).Rot(v1) + A.q.Rot(B.p) + A.p
inline PTransform b2Mul(const PTransform &A, const PTransform &B) {
    PTransform C;
    C.q = b2Mul(A.q, B.q);
    C.p = b2Mul(A.q, B.p) + A.p;
    return C;
}

// v2 = A.q' * (B.q * v1 + B.p - A.p)
//    = A.q' * B.q * v1 + A.q' * (B.p - A.p)
inline PTransform b2MulT(const PTransform &A, const PTransform &B) {
    PTransform C;
    C.q = b2MulT(A.q, B.q);
    C.p = b2MulT(A.q, B.p - A.p);
    return C;
}

template <typename T>
inline T b2Abs(T a) {
    return a > T(0) ? a : -a;
}

inline PVec2 b2Abs(const PVec2 &a) { return PVec2(b2Abs(a.x), b2Abs(a.y)); }

inline PMat22 b2Abs(const PMat22 &A) { return PMat22(b2Abs(A.ex), b2Abs(A.ey)); }

template <typename T>
inline T b2Min(T a, T b) {
    return a < b ? a : b;
}

inline PVec2 b2Min(const PVec2 &a, const PVec2 &b) { return PVec2(b2Min(a.x, b.x), b2Min(a.y, b.y)); }

template <typename T>
inline T b2Max(T a, T b) {
    return a > b ? a : b;
}

inline PVec2 b2Max(const PVec2 &a, const PVec2 &b) { return PVec2(b2Max(a.x, b.x), b2Max(a.y, b.y)); }

template <typename T>
inline T b2Clamp(T a, T low, T high) {
    return b2Max(low, b2Min(a, high));
}

inline PVec2 b2Clamp(const PVec2 &a, const PVec2 &low, const PVec2 &high) { return b2Max(low, b2Min(a, high)); }

template <typename T>
inline void b2Swap(T &a, T &b) {
    T tmp = a;
    a = b;
    b = tmp;
}

/// "Next Largest Power of 2
/// Given a binary integer value x, the next largest power of 2 can be computed by a SWAR algorithm
/// that recursively "folds" the upper bits into the lower bits. This process yields a bit vector with
/// the same most significant 1 as x, but all 1's below it. Adding 1 to that value yields the next
/// largest power of 2. For a 32-bit value:"
inline u32 b2NextPowerOfTwo(u32 x) {
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return x + 1;
}

inline bool b2IsPowerOfTwo(u32 x) {
    bool result = x > 0 && (x & (x - 1)) == 0;
    return result;
}

// https://fgiesen.wordpress.com/2012/08/15/linear-interpolation-past-present-and-future/
inline void PSweep::GetTransform(PTransform *xf, float beta) const {
    xf->p = (1.0f - beta) * c0 + beta * c;
    float angle = (1.0f - beta) * a0 + beta * a;
    xf->q.Set(angle);

    // Shift to origin
    xf->p -= b2Mul(xf->q, localCenter);
}

inline void PSweep::Advance(float alpha) {
    ME_ASSERT(alpha0 < 1.0f);
    float beta = (alpha - alpha0) / (1.0f - alpha0);
    c0 += beta * (c - c0);
    a0 += beta * (a - a0);
    alpha0 = alpha;
}

/// Normalize an angle in radians to be between -pi and pi
inline void PSweep::Normalize() {
    float twoPi = 2.0f * b2_pi;
    float d = twoPi * floorf(a0 / twoPi);
    a0 -= d;
    a -= d;
}

#pragma region TPPL

typedef f64 tppl_float;

#define TPPL_CCW 1
#define TPPL_CW -1

// 2D point structure
struct TPPLPoint {
    tppl_float x;
    tppl_float y;
    // User-specified vertex identifier.  Note that this isn't used internally
    // by the library, but will be faithfully copied around.
    int id;

    TPPLPoint operator+(const TPPLPoint &p) const {
        TPPLPoint r;
        r.x = x + p.x;
        r.y = y + p.y;
        return r;
    }

    TPPLPoint operator-(const TPPLPoint &p) const {
        TPPLPoint r;
        r.x = x - p.x;
        r.y = y - p.y;
        return r;
    }

    TPPLPoint operator*(const tppl_float f) const {
        TPPLPoint r;
        r.x = x * f;
        r.y = y * f;
        return r;
    }

    TPPLPoint operator/(const tppl_float f) const {
        TPPLPoint r;
        r.x = x / f;
        r.y = y / f;
        return r;
    }

    bool operator==(const TPPLPoint &p) const {
        if ((x == p.x) && (y == p.y))
            return true;
        else
            return false;
    }

    bool operator!=(const TPPLPoint &p) const {
        if ((x == p.x) && (y == p.y))
            return false;
        else
            return true;
    }
};

// Polygon implemented as an array of points with a 'hole' flag
class TPPLPoly {
protected:
    TPPLPoint *points;
    long numpoints;
    bool hole;

public:
    // constructors/destructors
    TPPLPoly();
    ~TPPLPoly();

    TPPLPoly(const TPPLPoly &src);
    TPPLPoly &operator=(const TPPLPoly &src);

    // getters and setters
    long GetNumPoints() const { return numpoints; }

    bool IsHole() const { return hole; }

    void SetHole(bool hole) { this->hole = hole; }

    TPPLPoint &GetPoint(long i) { return points[i]; }

    const TPPLPoint &GetPoint(long i) const { return points[i]; }

    TPPLPoint *GetPoints() { return points; }

    TPPLPoint &operator[](int i) { return points[i]; }

    const TPPLPoint &operator[](int i) const { return points[i]; }

    // clears the polygon points
    void Clear();

    // inits the polygon with numpoints vertices
    void Init(long numpoints);

    // creates a triangle with points p1,p2,p3
    void Triangle(TPPLPoint &p1, TPPLPoint &p2, TPPLPoint &p3);

    // inverts the orfer of vertices
    void Invert();

    // returns the orientation of the polygon
    // possible values:
    //    TPPL_CCW : polygon vertices are in counter-clockwise order
    //    TPPL_CW : polygon vertices are in clockwise order
    //        0 : the polygon has no (measurable) area
    int GetOrientation() const;

    // sets the polygon orientation
    // orientation can be
    //    TPPL_CCW : sets vertices in counter-clockwise order
    //    TPPL_CW : sets vertices in clockwise order
    void SetOrientation(int orientation);

    // checks whether a polygon is valid or not
    inline bool Valid() const { return this->numpoints >= 3; }
};

#ifdef TPPL_ALLOCATOR
typedef std::list<TPPLPoly, TPPL_ALLOCATOR(TPPLPoly)> TPPLPolyList;
#else
typedef std::list<TPPLPoly> TPPLPolyList;
#endif

class TPPLPartition {
protected:
    struct PartitionVertex {
        bool isActive;
        bool isConvex;
        bool isEar;

        TPPLPoint p;
        tppl_float angle;
        PartitionVertex *previous;
        PartitionVertex *next;

        PartitionVertex();
    };

    struct MonotoneVertex {
        TPPLPoint p;
        long previous;
        long next;
    };

    class VertexSorter {
        MonotoneVertex *vertices;

    public:
        VertexSorter(MonotoneVertex *v) : vertices(v) {}
        bool operator()(long index1, long index2);
    };

    struct Diagonal {
        long index1;
        long index2;
    };

#ifdef TPPL_ALLOCATOR
    typedef std::list<Diagonal, TPPL_ALLOCATOR(Diagonal)> DiagonalList;
#else
    typedef std::list<Diagonal> DiagonalList;
#endif

    // dynamic programming state for minimum-weight triangulation
    struct DPState {
        bool visible;
        tppl_float weight;
        long bestvertex;
    };

    // dynamic programming state for convex partitioning
    struct DPState2 {
        bool visible;
        long weight;
        DiagonalList pairs;
    };

    // edge that intersects the scanline
    struct ScanLineEdge {
        mutable long index;
        TPPLPoint p1;
        TPPLPoint p2;

        // determines if the edge is to the left of another edge
        bool operator<(const ScanLineEdge &other) const;

        bool IsConvex(const TPPLPoint &p1, const TPPLPoint &p2, const TPPLPoint &p3) const;
    };

    // standard helper functions
    bool IsConvex(TPPLPoint &p1, TPPLPoint &p2, TPPLPoint &p3);
    bool IsReflex(TPPLPoint &p1, TPPLPoint &p2, TPPLPoint &p3);
    bool IsInside(TPPLPoint &p1, TPPLPoint &p2, TPPLPoint &p3, TPPLPoint &p);

    bool InCone(TPPLPoint &p1, TPPLPoint &p2, TPPLPoint &p3, TPPLPoint &p);
    bool InCone(PartitionVertex *v, TPPLPoint &p);

    int Intersects(TPPLPoint &p11, TPPLPoint &p12, TPPLPoint &p21, TPPLPoint &p22);

    TPPLPoint Normalize(const TPPLPoint &p);
    tppl_float Distance(const TPPLPoint &p1, const TPPLPoint &p2);

    // helper functions for Triangulate_EC
    void UpdateVertexReflexity(PartitionVertex *v);
    void UpdateVertex(PartitionVertex *v, PartitionVertex *vertices, long numvertices);

    // helper functions for ConvexPartition_OPT
    void UpdateState(long a, long b, long w, long i, long j, DPState2 **dpstates);
    void TypeA(long i, long j, long k, PartitionVertex *vertices, DPState2 **dpstates);
    void TypeB(long i, long j, long k, PartitionVertex *vertices, DPState2 **dpstates);

    // helper functions for MonotonePartition
    bool PBelow(TPPLPoint &p1, TPPLPoint &p2);
    void AddDiagonal(MonotoneVertex *vertices, long *numvertices, long index1, long index2, char *vertextypes, std::set<ScanLineEdge>::iterator *edgeTreeIterators, std::set<ScanLineEdge> *edgeTree,
                     long *helpers);

    // triangulates a monotone polygon, used in Triangulate_MONO
    int TriangulateMonotone(TPPLPoly *inPoly, TPPLPolyList *triangles);

public:
    // simple heuristic procedure for removing holes from a list of polygons
    // works by creating a diagonal from the rightmost hole vertex to some visible vertex
    // time complexity: O(h*(n^2)), h is the number of holes, n is the number of vertices
    // space complexity: O(n)
    // params:
    //    inpolys : a list of polygons that can contain holes
    //              vertices of all non-hole polys have to be in counter-clockwise order
    //              vertices of all hole polys have to be in clockwise order
    //    outpolys : a list of polygons without holes
    // returns 1 on success, 0 on failure
    int RemoveHoles(TPPLPolyList *inpolys, TPPLPolyList *outpolys);

    // triangulates a polygon by ear clipping
    // time complexity O(n^2), n is the number of vertices
    // space complexity: O(n)
    // params:
    //    poly : an input polygon to be triangulated
    //           vertices have to be in counter-clockwise order
    //    triangles : a list of triangles (result)
    // returns 1 on success, 0 on failure
    int Triangulate_EC(TPPLPoly *poly, TPPLPolyList *triangles);

    // triangulates a list of polygons that may contain holes by ear clipping algorithm
    // first calls RemoveHoles to get rid of the holes, and then Triangulate_EC for each resulting polygon
    // time complexity: O(h*(n^2)), h is the number of holes, n is the number of vertices
    // space complexity: O(n)
    // params:
    //    inpolys : a list of polygons to be triangulated (can contain holes)
    //              vertices of all non-hole polys have to be in counter-clockwise order
    //              vertices of all hole polys have to be in clockwise order
    //    triangles : a list of triangles (result)
    // returns 1 on success, 0 on failure
    int Triangulate_EC(TPPLPolyList *inpolys, TPPLPolyList *triangles);

    // creates an optimal polygon triangulation in terms of minimal edge length
    // time complexity: O(n^3), n is the number of vertices
    // space complexity: O(n^2)
    // params:
    //    poly : an input polygon to be triangulated
    //           vertices have to be in counter-clockwise order
    //    triangles : a list of triangles (result)
    // returns 1 on success, 0 on failure
    int Triangulate_OPT(TPPLPoly *poly, TPPLPolyList *triangles);

    // triangulates a polygons by firstly partitioning it into monotone polygons
    // time complexity: O(n*log(n)), n is the number of vertices
    // space complexity: O(n)
    // params:
    //    poly : an input polygon to be triangulated
    //           vertices have to be in counter-clockwise order
    //    triangles : a list of triangles (result)
    // returns 1 on success, 0 on failure
    int Triangulate_MONO(TPPLPoly *poly, TPPLPolyList *triangles);

    // triangulates a list of polygons by firstly partitioning them into monotone polygons
    // time complexity: O(n*log(n)), n is the number of vertices
    // space complexity: O(n)
    // params:
    //    inpolys : a list of polygons to be triangulated (can contain holes)
    //              vertices of all non-hole polys have to be in counter-clockwise order
    //              vertices of all hole polys have to be in clockwise order
    //    triangles : a list of triangles (result)
    // returns 1 on success, 0 on failure
    int Triangulate_MONO(TPPLPolyList *inpolys, TPPLPolyList *triangles);

    // creates a monotone partition of a list of polygons that can contain holes
    // time complexity: O(n*log(n)), n is the number of vertices
    // space complexity: O(n)
    // params:
    //    inpolys : a list of polygons to be triangulated (can contain holes)
    //              vertices of all non-hole polys have to be in counter-clockwise order
    //              vertices of all hole polys have to be in clockwise order
    //    monotonePolys : a list of monotone polygons (result)
    // returns 1 on success, 0 on failure
    int MonotonePartition(TPPLPolyList *inpolys, TPPLPolyList *monotonePolys);

    // partitions a polygon into convex polygons by using Hertel-Mehlhorn algorithm
    // the algorithm gives at most four times the number of parts as the optimal algorithm
    // however, in practice it works much better than that and often gives optimal partition
    // uses triangulation obtained by ear clipping as intermediate result
    // time complexity O(n^2), n is the number of vertices
    // space complexity: O(n)
    // params:
    //    poly : an input polygon to be partitioned
    //           vertices have to be in counter-clockwise order
    //    parts : resulting list of convex polygons
    // returns 1 on success, 0 on failure
    int ConvexPartition_HM(TPPLPoly *poly, TPPLPolyList *parts);

    // partitions a list of polygons into convex parts by using Hertel-Mehlhorn algorithm
    // the algorithm gives at most four times the number of parts as the optimal algorithm
    // however, in practice it works much better than that and often gives optimal partition
    // uses triangulation obtained by ear clipping as intermediate result
    // time complexity O(n^2), n is the number of vertices
    // space complexity: O(n)
    // params:
    //    inpolys : an input list of polygons to be partitioned
    //              vertices of all non-hole polys have to be in counter-clockwise order
    //              vertices of all hole polys have to be in clockwise order
    //    parts : resulting list of convex polygons
    // returns 1 on success, 0 on failure
    int ConvexPartition_HM(TPPLPolyList *inpolys, TPPLPolyList *parts);

    // optimal convex partitioning (in terms of number of resulting convex polygons)
    // using the Keil-Snoeyink algorithm
    // M. Keil, J. Snoeyink, "On the time bound for convex decomposition of simple polygons", 1998
    // time complexity O(n^3), n is the number of vertices
    // space complexity: O(n^3)
    //    poly : an input polygon to be partitioned
    //           vertices have to be in counter-clockwise order
    //    parts : resulting list of convex polygons
    // returns 1 on success, 0 on failure
    int ConvexPartition_OPT(TPPLPoly *poly, TPPLPolyList *parts);
};

#pragma endregion TPPL

void simplify_section(const std::vector<MEvec2> &pts, f32 tolerance, size_t i, size_t j, std::vector<bool> *mark_map, size_t omitted = 0);
std::vector<MEvec2> simplify(const std::vector<MEvec2> &vertices, f32 tolerance);
f32 pDistance(f32 x, f32 y, f32 x1, f32 y1, f32 x2, f32 y2);

//  * A simple implementation of the marching squares algorithm that can identify
//  * perimeters in an supplied byte array. The array of data over which this
//  * instances of this class operate is not cloned by this class's constructor
//  * (for obvious efficiency reasons) and should therefore not be modified while
//  * the object is in use. It is expected that the data elements supplied to the
//  * algorithm have already been thresholded. The algorithm only distinguishes
//  * between zero and non-zero values.

namespace MarchingSquares {
struct Direction {
    Direction() : x(0), y(0) {}
    Direction(int x, int y) : x(x), y(y) {}
    Direction(MEvec2 vec) : x(vec.x), y(vec.y) {}
    int x;
    int y;
};

bool operator==(const Direction &a, const Direction &b);
Direction operator*(const Direction &direction, int multiplier);
Direction operator+(const Direction &a, const Direction &b);
Direction &operator+=(Direction &a, const Direction &b);

Direction MakeDirection(int x, int y);
Direction East();
Direction Northeast();
Direction North();
Direction Northwest();
Direction West();
Direction Southwest();
Direction South();
Direction Southeast();

bool isSet(int x, int y, int width, int height, unsigned char *data);
int value(int x, int y, int width, int height, unsigned char *data);

struct Result {
    int initialX = -1;
    int initialY = -1;
    std::vector<Direction> directions;
};

/**
 * Finds the perimeter between a set of zero and non-zero values which
 * begins at the specified data element. If no initial point is known,
 * consider using the convenience method supplied. The paths returned by
 * this method are always closed.
 *
 * The length of the supplied data array must exceed width * height,
 * with the data elements in row major order and the top-left-hand data
 * element at index zero.
 *
 * @param initialX
 *            the column of the data matrix at which to start tracing the
 *            perimeter
 * @param initialY
 *            the row of the data matrix at which to start tracing the
 *            perimeter
 * @param width
 *            the width of the data matrix
 * @param height
 *            the width of the data matrix
 * @param data
 *            the data elements
 *
 * @return a closed, anti-clockwise path that is a perimeter of between a
 *         set of zero and non-zero values in the data.
 * @throws std::runtime_error
 *             if there is no perimeter at the specified initial point.
 */
Result FindPerimeter(int initialX, int initialY, int width, int height, unsigned char *data);

/**
 * A convenience method that locates at least one perimeter in the data with
 * which this object was constructed. If there is no perimeter (ie. if all
 * elements of the supplied array are identically zero) then null is
 * returned.
 *
 * @return a perimeter path obtained from the data, or null
 */
Result FindPerimeter(int width, int height, unsigned char *data);
Result FindPerimeter(int width, int height, unsigned char *data, int lookX, int lookY);
Direction FindEdge(int width, int height, unsigned char *data, int lookX, int lookY);
}  // namespace MarchingSquares

#endif