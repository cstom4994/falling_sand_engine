// Metadot physics engine is enhanced based on box2d modification
// Metadot code copyright(c) 2022, KaoruXun All rights reserved.
// Box2d code by Erin Catto licensed under the MIT License
// https://github.com/erincatto/box2d

/*
MIT License
Copyright (c) 2019 Erin Catto

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef BOX2D_H
#define BOX2D_H

// These include files constitute the main Box2D API

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

#pragma region COMMON

#include <assert.h>
#include <cmath>
#include <float.h>
#include <stddef.h>
#include <stdlib.h>

#if !defined(NDEBUG)
#define b2DEBUG
#endif

#define B2_NOT_USED(x) ((void) (x))
#define b2Assert(A) assert(A)

#define b2_maxFloat FLT_MAX
#define b2_epsilon FLT_EPSILON
#define b2_pi 3.14159265359f

/// @file
/// Global tuning constants based on meters-kilograms-seconds (MKS) units.
///

// Collision

/// The maximum number of contact points between two convex shapes. Do
/// not change this value.
#define b2_maxManifoldPoints 2

/// This is used to fatten AABBs in the dynamic tree. This allows proxies
/// to move by a small amount without triggering a tree adjustment.
/// This is in meters.
#define b2_aabbExtension (0.1f * b2_lengthUnitsPerMeter)

/// This is used to fatten AABBs in the dynamic tree. This is used to predict
/// the future position based on the current displacement.
/// This is a dimensionless multiplier.
#define b2_aabbMultiplier 4.0f

/// A small length used as a collision and constraint tolerance. Usually it is
/// chosen to be numerically significant, but visually insignificant. In meters.
#define b2_linearSlop (0.005f * b2_lengthUnitsPerMeter)

/// A small angle used as a collision and constraint tolerance. Usually it is
/// chosen to be numerically significant, but visually insignificant.
#define b2_angularSlop (2.0f / 180.0f * b2_pi)

/// The radius of the polygon/edge shape skin. This should not be modified. Making
/// this smaller means polygons will have an insufficient buffer for continuous collision.
/// Making it larger may create artifacts for vertex collision.
#define b2_polygonRadius (2.0f * b2_linearSlop)

/// Maximum number of sub-steps per contact in continuous physics simulation.
#define b2_maxSubSteps 8

// Dynamics

/// Maximum number of contacts to be handled to solve a TOI impact.
#define b2_maxTOIContacts 32

/// The maximum linear position correction used when solving constraints. This helps to
/// prevent overshoot. Meters.
#define b2_maxLinearCorrection (0.2f * b2_lengthUnitsPerMeter)

/// The maximum angular position correction used when solving constraints. This helps to
/// prevent overshoot.
#define b2_maxAngularCorrection (8.0f / 180.0f * b2_pi)

/// The maximum linear translation of a body per step. This limit is very large and is used
/// to prevent numerical problems. You shouldn't need to adjust this. Meters.
#define b2_maxTranslation (2.0f * b2_lengthUnitsPerMeter)
#define b2_maxTranslationSquared (b2_maxTranslation * b2_maxTranslation)

/// The maximum angular velocity of a body. This limit is very large and is used
/// to prevent numerical problems. You shouldn't need to adjust this.
#define b2_maxRotation (0.5f * b2_pi)
#define b2_maxRotationSquared (b2_maxRotation * b2_maxRotation)

/// This scale factor controls how fast overlap is resolved. Ideally this would be 1 so
/// that overlap is removed in one time step. However using values close to 1 often lead
/// to overshoot.
#define b2_baumgarte 0.2f
#define b2_toiBaumgarte 0.75f

// Sleep

/// The time that a body must be still before it will go to sleep.
#define b2_timeToSleep 0.5f

/// A body cannot sleep if its linear velocity is above this tolerance.
#define b2_linearSleepTolerance (0.01f * b2_lengthUnitsPerMeter)

/// A body cannot sleep if its angular velocity is above this tolerance.
#define b2_angularSleepTolerance (2.0f / 180.0f * b2_pi)

/// Dump to a file. Only one dump file allowed at a time.
void b2OpenDump(const char *fileName);
void b2Dump(const char *string, ...);
void b2CloseDump();

/// Version numbering scheme.
/// See http://en.wikipedia.org/wiki/Software_versioning
struct b2Version
{
    int32 major;   ///< significant changes
    int32 minor;   ///< incremental changes
    int32 revision;///< bug fixes
};

/// Current version.
extern b2Version b2_version;

#pragma endregion COMMON

/// @file
/// Settings that can be overriden for your application
///

/// Define this macro in your build if you want to override settings
#ifdef B2_USER_SETTINGS

/// This is a user file that includes custom definitions of the macros, structs, and functions
/// defined below.
#include "b2_user_settings.h"

#else

#include <stdarg.h>
#include <stdint.h>

// Tunable Constants

/// You can use this to change the length scale used by your game.
/// For example for inches you could use 39.4.
#define b2_lengthUnitsPerMeter 1.0f

/// The maximum number of vertices on a convex polygon. You cannot increase
/// this too much because b2BlockAllocator has a maximum object size.
#define b2_maxPolygonVertices 8

// User data

/// You can define this to inject whatever data you want in b2Body
struct b2BodyUserData
{
    b2BodyUserData() { pointer = 0; }

    /// For legacy compatibility
    uintptr_t pointer;
};

/// You can define this to inject whatever data you want in b2Fixture
struct b2FixtureUserData
{
    b2FixtureUserData() { pointer = 0; }

    /// For legacy compatibility
    uintptr_t pointer;
};

/// You can define this to inject whatever data you want in b2Joint
struct b2JointUserData
{
    b2JointUserData() { pointer = 0; }

    /// For legacy compatibility
    uintptr_t pointer;
};

// Memory Allocation

/// Default allocation functions
void *b2Alloc_Default(int32 size);
void b2Free_Default(void *mem);

/// Implement this function to use your own memory allocator.
inline void *b2Alloc(int32 size) { return b2Alloc_Default(size); }

/// If you implement b2Alloc, you should also implement this function.
inline void b2Free(void *mem) { b2Free_Default(mem); }

/// Default logging function
void b2Log_Default(const char *string, va_list args);

/// Implement this to use your own logging.
inline void b2Log(const char *string, ...) {
    va_list args;
    va_start(args, string);
    b2Log_Default(string, args);
    va_end(args);
}

#endif// B2_USER_SETTINGS

const int32 b2_blockSizeCount = 14;

struct b2Block;
struct b2Chunk;

/// This is a small object allocator used for allocating small
/// objects that persist for more than one time step.
/// See: http://www.codeproject.com/useritems/Small_Block_Allocator.asp
class b2BlockAllocator {
public:
    b2BlockAllocator();
    ~b2BlockAllocator();

    /// Allocate memory. This will use b2Alloc if the size is larger than b2_maxBlockSize.
    void *Allocate(int32 size);

    /// Free memory. This will use b2Free if the size is larger than b2_maxBlockSize.
    void Free(void *p, int32 size);

    void Clear();

private:
    b2Chunk *m_chunks;
    int32 m_chunkCount;
    int32 m_chunkSpace;

    b2Block *m_freeLists[b2_blockSizeCount];
};

#pragma region MATH

/// This function is used to ensure that a floating point number is not a NaN or infinity.
inline bool b2IsValid(float x) { return std::isfinite(x); }

#define b2Sqrt(x) sqrtf(x)
#define b2Atan2(y, x) atan2f(y, x)

/// A 2D column vector.
struct b2Vec2
{
    /// Default constructor does nothing (for performance).
    b2Vec2() = default;

    /// Construct using coordinates.
    b2Vec2(float xIn, float yIn) : x(xIn), y(yIn) {}

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
    b2Vec2 operator-() const {
        b2Vec2 v;
        v.Set(-x, -y);
        return v;
    }

    /// Read from and indexed element.
    float operator()(int32 i) const { return (&x)[i]; }

    /// Write to an indexed element.
    float &operator()(int32 i) { return (&x)[i]; }

    /// Add a vector to this vector.
    void operator+=(const b2Vec2 &v) {
        x += v.x;
        y += v.y;
    }

    /// Subtract a vector from this vector.
    void operator-=(const b2Vec2 &v) {
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
        if (length < b2_epsilon) { return 0.0f; }
        float invLength = 1.0f / length;
        x *= invLength;
        y *= invLength;

        return length;
    }

    /// Does this vector contain finite coordinates?
    bool IsValid() const { return b2IsValid(x) && b2IsValid(y); }

    /// Get the skew vector such that dot(skew_vec, other) == cross(vec, other)
    b2Vec2 Skew() const { return b2Vec2(-y, x); }

    float x, y;
};

/// A 2D column vector with 3 elements.
struct b2Vec3
{
    /// Default constructor does nothing (for performance).
    b2Vec3() = default;

    /// Construct using coordinates.
    b2Vec3(float xIn, float yIn, float zIn) : x(xIn), y(yIn), z(zIn) {}

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
    b2Vec3 operator-() const {
        b2Vec3 v;
        v.Set(-x, -y, -z);
        return v;
    }

    /// Add a vector to this vector.
    void operator+=(const b2Vec3 &v) {
        x += v.x;
        y += v.y;
        z += v.z;
    }

    /// Subtract a vector from this vector.
    void operator-=(const b2Vec3 &v) {
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
struct b2Mat22
{
    /// The default constructor does nothing (for performance).
    b2Mat22() = default;

    /// Construct this matrix using columns.
    b2Mat22(const b2Vec2 &c1, const b2Vec2 &c2) {
        ex = c1;
        ey = c2;
    }

    /// Construct this matrix using scalars.
    b2Mat22(float a11, float a12, float a21, float a22) {
        ex.x = a11;
        ex.y = a21;
        ey.x = a12;
        ey.y = a22;
    }

    /// Initialize this matrix using columns.
    void Set(const b2Vec2 &c1, const b2Vec2 &c2) {
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

    b2Mat22 GetInverse() const {
        float a = ex.x, b = ey.x, c = ex.y, d = ey.y;
        b2Mat22 B;
        float det = a * d - b * c;
        if (det != 0.0f) { det = 1.0f / det; }
        B.ex.x = det * d;
        B.ey.x = -det * b;
        B.ex.y = -det * c;
        B.ey.y = det * a;
        return B;
    }

    /// Solve A * x = b, where b is a column vector. This is more efficient
    /// than computing the inverse in one-shot cases.
    b2Vec2 Solve(const b2Vec2 &b) const {
        float a11 = ex.x, a12 = ey.x, a21 = ex.y, a22 = ey.y;
        float det = a11 * a22 - a12 * a21;
        if (det != 0.0f) { det = 1.0f / det; }
        b2Vec2 x;
        x.x = det * (a22 * b.x - a12 * b.y);
        x.y = det * (a11 * b.y - a21 * b.x);
        return x;
    }

    b2Vec2 ex, ey;
};

/// A 3-by-3 matrix. Stored in column-major order.
struct b2Mat33
{
    /// The default constructor does nothing (for performance).
    b2Mat33() = default;

    /// Construct this matrix using columns.
    b2Mat33(const b2Vec3 &c1, const b2Vec3 &c2, const b2Vec3 &c3) {
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
    b2Vec3 Solve33(const b2Vec3 &b) const;

    /// Solve A * x = b, where b is a column vector. This is more efficient
    /// than computing the inverse in one-shot cases. Solve only the upper
    /// 2-by-2 matrix equation.
    b2Vec2 Solve22(const b2Vec2 &b) const;

    /// Get the inverse of this matrix as a 2-by-2.
    /// Returns the zero matrix if singular.
    void GetInverse22(b2Mat33 *M) const;

    /// Get the symmetric inverse of this matrix as a 3-by-3.
    /// Returns the zero matrix if singular.
    void GetSymInverse33(b2Mat33 *M) const;

    b2Vec3 ex, ey, ez;
};

/// Rotation
struct b2Rot
{
    b2Rot() = default;

    /// Initialize from an angle in radians
    explicit b2Rot(float angle) {
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
    b2Vec2 GetXAxis() const { return b2Vec2(c, s); }

    /// Get the u-axis
    b2Vec2 GetYAxis() const { return b2Vec2(-s, c); }

    /// Sine and cosine
    float s, c;
};

/// A transform contains translation and rotation. It is used to represent
/// the position and orientation of rigid frames.
struct b2Transform
{
    /// The default constructor does nothing.
    b2Transform() = default;

    /// Initialize using a position vector and a rotation.
    b2Transform(const b2Vec2 &position, const b2Rot &rotation) : p(position), q(rotation) {}

    /// Set this to the identity transform.
    void SetIdentity() {
        p.SetZero();
        q.SetIdentity();
    }

    /// Set this based on the position and angle.
    void Set(const b2Vec2 &position, float angle) {
        p = position;
        q.Set(angle);
    }

    b2Vec2 p;
    b2Rot q;
};

/// This describes the motion of a body/shape for TOI computation.
/// Shapes are defined with respect to the body origin, which may
/// no coincide with the center of mass. However, to support dynamics
/// we must interpolate the center of mass position.
struct b2Sweep
{
    b2Sweep() = default;

    /// Get the interpolated transform at a specific time.
    /// @param transform the output transform
    /// @param beta is a factor in [0,1], where 0 indicates alpha0.
    void GetTransform(b2Transform *transform, float beta) const;

    /// Advance the sweep forward, yielding a new initial state.
    /// @param alpha the new initial time.
    void Advance(float alpha);

    /// Normalize the angles.
    void Normalize();

    b2Vec2 localCenter;///< local center of mass position
    b2Vec2 c0, c;      ///< center world positions
    float a0, a;       ///< world angles

    /// Fraction of the current time step in the range [0,1]
    /// c0 and a0 are the positions at alpha0.
    float alpha0;
};

/// Useful constant
extern const b2Vec2 b2Vec2_zero;

/// Perform the dot product on two vectors.
inline float b2Dot(const b2Vec2 &a, const b2Vec2 &b) { return a.x * b.x + a.y * b.y; }

/// Perform the cross product on two vectors. In 2D this produces a scalar.
inline float b2Cross(const b2Vec2 &a, const b2Vec2 &b) { return a.x * b.y - a.y * b.x; }

/// Perform the cross product on a vector and a scalar. In 2D this produces
/// a vector.
inline b2Vec2 b2Cross(const b2Vec2 &a, float s) { return b2Vec2(s * a.y, -s * a.x); }

/// Perform the cross product on a scalar and a vector. In 2D this produces
/// a vector.
inline b2Vec2 b2Cross(float s, const b2Vec2 &a) { return b2Vec2(-s * a.y, s * a.x); }

/// Multiply a matrix times a vector. If a rotation matrix is provided,
/// then this transforms the vector from one frame to another.
inline b2Vec2 b2Mul(const b2Mat22 &A, const b2Vec2 &v) {
    return b2Vec2(A.ex.x * v.x + A.ey.x * v.y, A.ex.y * v.x + A.ey.y * v.y);
}

/// Multiply a matrix transpose times a vector. If a rotation matrix is provided,
/// then this transforms the vector from one frame to another (inverse transform).
inline b2Vec2 b2MulT(const b2Mat22 &A, const b2Vec2 &v) {
    return b2Vec2(b2Dot(v, A.ex), b2Dot(v, A.ey));
}

/// Add two vectors component-wise.
inline b2Vec2 operator+(const b2Vec2 &a, const b2Vec2 &b) { return b2Vec2(a.x + b.x, a.y + b.y); }

/// Subtract two vectors component-wise.
inline b2Vec2 operator-(const b2Vec2 &a, const b2Vec2 &b) { return b2Vec2(a.x - b.x, a.y - b.y); }

inline b2Vec2 operator*(float s, const b2Vec2 &a) { return b2Vec2(s * a.x, s * a.y); }

inline bool operator==(const b2Vec2 &a, const b2Vec2 &b) { return a.x == b.x && a.y == b.y; }

inline bool operator!=(const b2Vec2 &a, const b2Vec2 &b) { return a.x != b.x || a.y != b.y; }

inline float b2Distance(const b2Vec2 &a, const b2Vec2 &b) {
    b2Vec2 c = a - b;
    return c.Length();
}

inline float b2DistanceSquared(const b2Vec2 &a, const b2Vec2 &b) {
    b2Vec2 c = a - b;
    return b2Dot(c, c);
}

inline b2Vec3 operator*(float s, const b2Vec3 &a) { return b2Vec3(s * a.x, s * a.y, s * a.z); }

/// Add two vectors component-wise.
inline b2Vec3 operator+(const b2Vec3 &a, const b2Vec3 &b) {
    return b2Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

/// Subtract two vectors component-wise.
inline b2Vec3 operator-(const b2Vec3 &a, const b2Vec3 &b) {
    return b2Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

/// Perform the dot product on two vectors.
inline float b2Dot(const b2Vec3 &a, const b2Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

/// Perform the cross product on two vectors.
inline b2Vec3 b2Cross(const b2Vec3 &a, const b2Vec3 &b) {
    return b2Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

inline b2Mat22 operator+(const b2Mat22 &A, const b2Mat22 &B) {
    return b2Mat22(A.ex + B.ex, A.ey + B.ey);
}

// A * B
inline b2Mat22 b2Mul(const b2Mat22 &A, const b2Mat22 &B) {
    return b2Mat22(b2Mul(A, B.ex), b2Mul(A, B.ey));
}

// A^T * B
inline b2Mat22 b2MulT(const b2Mat22 &A, const b2Mat22 &B) {
    b2Vec2 c1(b2Dot(A.ex, B.ex), b2Dot(A.ey, B.ex));
    b2Vec2 c2(b2Dot(A.ex, B.ey), b2Dot(A.ey, B.ey));
    return b2Mat22(c1, c2);
}

/// Multiply a matrix times a vector.
inline b2Vec3 b2Mul(const b2Mat33 &A, const b2Vec3 &v) {
    return v.x * A.ex + v.y * A.ey + v.z * A.ez;
}

/// Multiply a matrix times a vector.
inline b2Vec2 b2Mul22(const b2Mat33 &A, const b2Vec2 &v) {
    return b2Vec2(A.ex.x * v.x + A.ey.x * v.y, A.ex.y * v.x + A.ey.y * v.y);
}

/// Multiply two rotations: q * r
inline b2Rot b2Mul(const b2Rot &q, const b2Rot &r) {
    // [qc -qs] * [rc -rs] = [qc*rc-qs*rs -qc*rs-qs*rc]
    // [qs  qc]   [rs  rc]   [qs*rc+qc*rs -qs*rs+qc*rc]
    // s = qs * rc + qc * rs
    // c = qc * rc - qs * rs
    b2Rot qr;
    qr.s = q.s * r.c + q.c * r.s;
    qr.c = q.c * r.c - q.s * r.s;
    return qr;
}

/// Transpose multiply two rotations: qT * r
inline b2Rot b2MulT(const b2Rot &q, const b2Rot &r) {
    // [ qc qs] * [rc -rs] = [qc*rc+qs*rs -qc*rs+qs*rc]
    // [-qs qc]   [rs  rc]   [-qs*rc+qc*rs qs*rs+qc*rc]
    // s = qc * rs - qs * rc
    // c = qc * rc + qs * rs
    b2Rot qr;
    qr.s = q.c * r.s - q.s * r.c;
    qr.c = q.c * r.c + q.s * r.s;
    return qr;
}

/// Rotate a vector
inline b2Vec2 b2Mul(const b2Rot &q, const b2Vec2 &v) {
    return b2Vec2(q.c * v.x - q.s * v.y, q.s * v.x + q.c * v.y);
}

/// Inverse rotate a vector
inline b2Vec2 b2MulT(const b2Rot &q, const b2Vec2 &v) {
    return b2Vec2(q.c * v.x + q.s * v.y, -q.s * v.x + q.c * v.y);
}

inline b2Vec2 b2Mul(const b2Transform &T, const b2Vec2 &v) {
    float x = (T.q.c * v.x - T.q.s * v.y) + T.p.x;
    float y = (T.q.s * v.x + T.q.c * v.y) + T.p.y;

    return b2Vec2(x, y);
}

inline b2Vec2 b2MulT(const b2Transform &T, const b2Vec2 &v) {
    float px = v.x - T.p.x;
    float py = v.y - T.p.y;
    float x = (T.q.c * px + T.q.s * py);
    float y = (-T.q.s * px + T.q.c * py);

    return b2Vec2(x, y);
}

// v2 = A.q.Rot(B.q.Rot(v1) + B.p) + A.p
//    = (A.q * B.q).Rot(v1) + A.q.Rot(B.p) + A.p
inline b2Transform b2Mul(const b2Transform &A, const b2Transform &B) {
    b2Transform C;
    C.q = b2Mul(A.q, B.q);
    C.p = b2Mul(A.q, B.p) + A.p;
    return C;
}

// v2 = A.q' * (B.q * v1 + B.p - A.p)
//    = A.q' * B.q * v1 + A.q' * (B.p - A.p)
inline b2Transform b2MulT(const b2Transform &A, const b2Transform &B) {
    b2Transform C;
    C.q = b2MulT(A.q, B.q);
    C.p = b2MulT(A.q, B.p - A.p);
    return C;
}

template<typename T>
inline T b2Abs(T a) {
    return a > T(0) ? a : -a;
}

inline b2Vec2 b2Abs(const b2Vec2 &a) { return b2Vec2(b2Abs(a.x), b2Abs(a.y)); }

inline b2Mat22 b2Abs(const b2Mat22 &A) { return b2Mat22(b2Abs(A.ex), b2Abs(A.ey)); }

template<typename T>
inline T b2Min(T a, T b) {
    return a < b ? a : b;
}

inline b2Vec2 b2Min(const b2Vec2 &a, const b2Vec2 &b) {
    return b2Vec2(b2Min(a.x, b.x), b2Min(a.y, b.y));
}

template<typename T>
inline T b2Max(T a, T b) {
    return a > b ? a : b;
}

inline b2Vec2 b2Max(const b2Vec2 &a, const b2Vec2 &b) {
    return b2Vec2(b2Max(a.x, b.x), b2Max(a.y, b.y));
}

template<typename T>
inline T b2Clamp(T a, T low, T high) {
    return b2Max(low, b2Min(a, high));
}

inline b2Vec2 b2Clamp(const b2Vec2 &a, const b2Vec2 &low, const b2Vec2 &high) {
    return b2Max(low, b2Min(a, high));
}

template<typename T>
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
inline uint32 b2NextPowerOfTwo(uint32 x) {
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return x + 1;
}

inline bool b2IsPowerOfTwo(uint32 x) {
    bool result = x > 0 && (x & (x - 1)) == 0;
    return result;
}

// https://fgiesen.wordpress.com/2012/08/15/linear-interpolation-past-present-and-future/
inline void b2Sweep::GetTransform(b2Transform *xf, float beta) const {
    xf->p = (1.0f - beta) * c0 + beta * c;
    float angle = (1.0f - beta) * a0 + beta * a;
    xf->q.Set(angle);

    // Shift to origin
    xf->p -= b2Mul(xf->q, localCenter);
}

inline void b2Sweep::Advance(float alpha) {
    b2Assert(alpha0 < 1.0f);
    float beta = (alpha - alpha0) / (1.0f - alpha0);
    c0 += beta * (c - c0);
    a0 += beta * (a - a0);
    alpha0 = alpha;
}

/// Normalize an angle in radians to be between -pi and pi
inline void b2Sweep::Normalize() {
    float twoPi = 2.0f * b2_pi;
    float d = twoPi * floorf(a0 / twoPi);
    a0 -= d;
    a -= d;
}

#pragma endregion MATH

#pragma region DRAW

/// Color for debug drawing. Each value has the range [0,1].
struct b2Color
{
    b2Color() {}
    b2Color(float rIn, float gIn, float bIn, float aIn = 1.0f) {
        r = rIn;
        g = gIn;
        b = bIn;
        a = aIn;
    }

    void Set(float rIn, float gIn, float bIn, float aIn = 1.0f) {
        r = rIn;
        g = gIn;
        b = bIn;
        a = aIn;
    }

    float r, g, b, a;
};

/// Implement and register this class with a b2World to provide debug drawing of physics
/// entities in your game.
class b2Draw {
public:
    b2Draw();

    virtual ~b2Draw() {}

    enum {
        e_shapeBit = 0x0001,      ///< draw shapes
        e_jointBit = 0x0002,      ///< draw joint connections
        e_aabbBit = 0x0004,       ///< draw axis aligned bounding boxes
        e_pairBit = 0x0008,       ///< draw broad-phase pairs
        e_centerOfMassBit = 0x0010///< draw center of mass frame
    };

    /// Set the drawing flags.
    void SetFlags(uint32 flags);

    /// Get the drawing flags.
    uint32 GetFlags() const;

    /// Append flags to the current flags.
    void AppendFlags(uint32 flags);

    /// Clear flags from the current flags.
    void ClearFlags(uint32 flags);

    /// Draw a closed polygon provided in CCW order.
    virtual void DrawPolygon(const b2Vec2 *vertices, int32 vertexCount, const b2Color &color) = 0;

    /// Draw a solid closed polygon provided in CCW order.
    virtual void DrawSolidPolygon(const b2Vec2 *vertices, int32 vertexCount,
                                  const b2Color &color) = 0;

    /// Draw a circle.
    virtual void DrawCircle(const b2Vec2 &center, float radius, const b2Color &color) = 0;

    /// Draw a solid circle.
    virtual void DrawSolidCircle(const b2Vec2 &center, float radius, const b2Vec2 &axis,
                                 const b2Color &color) = 0;

    /// Draw a line segment.
    virtual void DrawSegment(const b2Vec2 &p1, const b2Vec2 &p2, const b2Color &color) = 0;

    /// Draw a transform. Choose your own length scale.
    /// @param xf a transform.
    virtual void DrawTransform(const b2Transform &xf) = 0;

    /// Draw a point.
    virtual void DrawPoint(const b2Vec2 &p, float size, const b2Color &color) = 0;

protected:
    uint32 m_drawFlags;
};

#pragma endregion DRAW

#pragma region TIMER

/// Timer for profiling. This has platform specific code and may
/// not work on every platform.
class b2Timer {
public:
    /// Constructor
    b2Timer();

    /// Reset the timer.
    void Reset();

    /// Get the time since construction or the last reset.
    float GetMilliseconds() const;

private:
#if defined(_WIN32)
    double m_start;
    static double s_invFrequency;
#elif defined(__linux__) || defined(__APPLE__)
    unsigned long long m_start_sec;
    unsigned long long m_start_usec;
#endif
};

#pragma endregion TIMER

#pragma region JOINT

class b2Body;
class b2Draw;
class b2Joint;
struct b2SolverData;
class b2BlockAllocator;

enum b2JointType {
    e_unknownJoint,
    e_revoluteJoint,
    e_prismaticJoint,
    e_distanceJoint,
    e_pulleyJoint,
    e_mouseJoint,
    e_gearJoint,
    e_wheelJoint,
    e_weldJoint,
    e_frictionJoint,
    e_motorJoint
};

struct b2Jacobian
{
    b2Vec2 linear;
    float angularA;
    float angularB;
};

/// A joint edge is used to connect bodies and joints together
/// in a joint graph where each body is a node and each joint
/// is an edge. A joint edge belongs to a doubly linked list
/// maintained in each attached body. Each joint has two joint
/// nodes, one for each attached body.
struct b2JointEdge
{
    b2Body *other;    ///< provides quick access to the other body attached.
    b2Joint *joint;   ///< the joint
    b2JointEdge *prev;///< the previous joint edge in the body's joint list
    b2JointEdge *next;///< the next joint edge in the body's joint list
};

/// Joint definitions are used to construct joints.
struct b2JointDef
{
    b2JointDef() {
        type = e_unknownJoint;
        bodyA = nullptr;
        bodyB = nullptr;
        collideConnected = false;
    }

    /// The joint type is set automatically for concrete joint types.
    b2JointType type;

    /// Use this to attach application specific data to your joints.
    b2JointUserData userData;

    /// The first attached body.
    b2Body *bodyA;

    /// The second attached body.
    b2Body *bodyB;

    /// Set this flag to true if the attached bodies should collide.
    bool collideConnected;
};

/// Utility to compute linear stiffness values from frequency and damping ratio
void b2LinearStiffness(float &stiffness, float &damping, float frequencyHertz, float dampingRatio,
                       const b2Body *bodyA, const b2Body *bodyB);

/// Utility to compute rotational stiffness values frequency and damping ratio
void b2AngularStiffness(float &stiffness, float &damping, float frequencyHertz, float dampingRatio,
                        const b2Body *bodyA, const b2Body *bodyB);

/// The base joint class. Joints are used to constraint two bodies together in
/// various fashions. Some joints also feature limits and motors.
class b2Joint {
public:
    /// Get the type of the concrete joint.
    b2JointType GetType() const;

    /// Get the first body attached to this joint.
    b2Body *GetBodyA();

    /// Get the second body attached to this joint.
    b2Body *GetBodyB();

    /// Get the anchor point on bodyA in world coordinates.
    virtual b2Vec2 GetAnchorA() const = 0;

    /// Get the anchor point on bodyB in world coordinates.
    virtual b2Vec2 GetAnchorB() const = 0;

    /// Get the reaction force on bodyB at the joint anchor in Newtons.
    virtual b2Vec2 GetReactionForce(float inv_dt) const = 0;

    /// Get the reaction torque on bodyB in N*m.
    virtual float GetReactionTorque(float inv_dt) const = 0;

    /// Get the next joint the world joint list.
    b2Joint *GetNext();
    const b2Joint *GetNext() const;

    /// Get the user data pointer.
    b2JointUserData &GetUserData();
    const b2JointUserData &GetUserData() const;

    /// Short-cut function to determine if either body is enabled.
    bool IsEnabled() const;

    /// Get collide connected.
    /// Note: modifying the collide connect flag won't work correctly because
    /// the flag is only checked when fixture AABBs begin to overlap.
    bool GetCollideConnected() const;

    /// Dump this joint to the log file.
    virtual void Dump() { b2Dump("// Dump is not supported for this joint type.\n"); }

    /// Shift the origin for any points stored in world coordinates.
    virtual void ShiftOrigin(const b2Vec2 &newOrigin) { B2_NOT_USED(newOrigin); }

    /// Debug draw this joint
    virtual void Draw(b2Draw *draw) const;

protected:
    friend class b2World;
    friend class b2Body;
    friend class b2Island;
    friend class b2GearJoint;

    static b2Joint *Create(const b2JointDef *def, b2BlockAllocator *allocator);
    static void Destroy(b2Joint *joint, b2BlockAllocator *allocator);

    b2Joint(const b2JointDef *def);
    virtual ~b2Joint() {}

    virtual void InitVelocityConstraints(const b2SolverData &data) = 0;
    virtual void SolveVelocityConstraints(const b2SolverData &data) = 0;

    // This returns true if the position errors are within tolerance.
    virtual bool SolvePositionConstraints(const b2SolverData &data) = 0;

    b2JointType m_type;
    b2Joint *m_prev;
    b2Joint *m_next;
    b2JointEdge m_edgeA;
    b2JointEdge m_edgeB;
    b2Body *m_bodyA;
    b2Body *m_bodyB;

    int32 m_index;

    bool m_islandFlag;
    bool m_collideConnected;

    b2JointUserData m_userData;
};

inline b2JointType b2Joint::GetType() const { return m_type; }

inline b2Body *b2Joint::GetBodyA() { return m_bodyA; }

inline b2Body *b2Joint::GetBodyB() { return m_bodyB; }

inline b2Joint *b2Joint::GetNext() { return m_next; }

inline const b2Joint *b2Joint::GetNext() const { return m_next; }

inline b2JointUserData &b2Joint::GetUserData() { return m_userData; }

inline const b2JointUserData &b2Joint::GetUserData() const { return m_userData; }

inline bool b2Joint::GetCollideConnected() const { return m_collideConnected; }

#pragma endregion JOINT

#pragma region

#include <limits.h>

/// @file
/// Structures and functions used for computing contact points, distance
/// queries, and TOI queries.

class b2Shape;
class b2CircleShape;
class b2EdgeShape;
class b2PolygonShape;

const uint8 b2_nullFeature = UCHAR_MAX;

/// The features that intersect to form the contact point
/// This must be 4 bytes or less.
struct b2ContactFeature
{
    enum Type {
        e_vertex = 0,
        e_face = 1
    };

    uint8 indexA;///< Feature index on shapeA
    uint8 indexB;///< Feature index on shapeB
    uint8 typeA; ///< The feature type on shapeA
    uint8 typeB; ///< The feature type on shapeB
};

/// Contact ids to facilitate warm starting.
union b2ContactID {
    b2ContactFeature cf;
    uint32 key;///< Used to quickly compare contact ids.
};

/// A manifold point is a contact point belonging to a contact
/// manifold. It holds details related to the geometry and dynamics
/// of the contact points.
/// The local point usage depends on the manifold type:
/// -e_circles: the local center of circleB
/// -e_faceA: the local center of cirlceB or the clip point of polygonB
/// -e_faceB: the clip point of polygonA
/// This structure is stored across time steps, so we keep it small.
/// Note: the impulses are used for internal caching and may not
/// provide reliable contact forces, especially for high speed collisions.
struct b2ManifoldPoint
{
    b2Vec2 localPoint;   ///< usage depends on manifold type
    float normalImpulse; ///< the non-penetration impulse
    float tangentImpulse;///< the friction impulse
    b2ContactID id;      ///< uniquely identifies a contact point between two shapes
};

/// A manifold for two touching convex shapes.
/// Box2D supports multiple types of contact:
/// - clip point versus plane with radius
/// - point versus point with radius (circles)
/// The local point usage depends on the manifold type:
/// -e_circles: the local center of circleA
/// -e_faceA: the center of faceA
/// -e_faceB: the center of faceB
/// Similarly the local normal usage:
/// -e_circles: not used
/// -e_faceA: the normal on polygonA
/// -e_faceB: the normal on polygonB
/// We store contacts in this way so that position correction can
/// account for movement, which is critical for continuous physics.
/// All contact scenarios must be expressed in one of these types.
/// This structure is stored across time steps, so we keep it small.
struct b2Manifold
{
    enum Type {
        e_circles,
        e_faceA,
        e_faceB
    };

    b2ManifoldPoint points[b2_maxManifoldPoints];///< the points of contact
    b2Vec2 localNormal;                          ///< not use for Type::e_points
    b2Vec2 localPoint;                           ///< usage depends on manifold type
    Type type;
    int32 pointCount;///< the number of manifold points
};

/// This is used to compute the current state of a contact manifold.
struct b2WorldManifold
{
    /// Evaluate the manifold with supplied transforms. This assumes
    /// modest motion from the original state. This does not change the
    /// point count, impulses, etc. The radii must come from the shapes
    /// that generated the manifold.
    void Initialize(const b2Manifold *manifold, const b2Transform &xfA, float radiusA,
                    const b2Transform &xfB, float radiusB);

    b2Vec2 normal;                          ///< world vector pointing from A to B
    b2Vec2 points[b2_maxManifoldPoints];    ///< world contact point (point of intersection)
    float separations[b2_maxManifoldPoints];///< a negative value indicates overlap, in meters
};

/// This is used for determining the state of contact points.
enum b2PointState {
    b2_nullState,   ///< point does not exist
    b2_addState,    ///< point was added in the update
    b2_persistState,///< point persisted across the update
    b2_removeState  ///< point was removed in the update
};

/// Compute the point states given two manifolds. The states pertain to the transition from manifold1
/// to manifold2. So state1 is either persist or remove while state2 is either add or persist.
void b2GetPointStates(b2PointState state1[b2_maxManifoldPoints],
                      b2PointState state2[b2_maxManifoldPoints], const b2Manifold *manifold1,
                      const b2Manifold *manifold2);

/// Used for computing contact manifolds.
struct b2ClipVertex
{
    b2Vec2 v;
    b2ContactID id;
};

/// Ray-cast input data. The ray extends from p1 to p1 + maxFraction * (p2 - p1).
struct b2RayCastInput
{
    b2Vec2 p1, p2;
    float maxFraction;
};

/// Ray-cast output data. The ray hits at p1 + fraction * (p2 - p1), where p1 and p2
/// come from b2RayCastInput.
struct b2RayCastOutput
{
    b2Vec2 normal;
    float fraction;
};

/// An axis aligned bounding box.
struct b2AABB
{
    /// Verify that the bounds are sorted.
    bool IsValid() const;

    /// Get the center of the AABB.
    b2Vec2 GetCenter() const { return 0.5f * (lowerBound + upperBound); }

    /// Get the extents of the AABB (half-widths).
    b2Vec2 GetExtents() const { return 0.5f * (upperBound - lowerBound); }

    /// Get the perimeter length
    float GetPerimeter() const {
        float wx = upperBound.x - lowerBound.x;
        float wy = upperBound.y - lowerBound.y;
        return 2.0f * (wx + wy);
    }

    /// Combine an AABB into this one.
    void Combine(const b2AABB &aabb) {
        lowerBound = b2Min(lowerBound, aabb.lowerBound);
        upperBound = b2Max(upperBound, aabb.upperBound);
    }

    /// Combine two AABBs into this one.
    void Combine(const b2AABB &aabb1, const b2AABB &aabb2) {
        lowerBound = b2Min(aabb1.lowerBound, aabb2.lowerBound);
        upperBound = b2Max(aabb1.upperBound, aabb2.upperBound);
    }

    /// Does this aabb contain the provided AABB.
    bool Contains(const b2AABB &aabb) const {
        bool result = true;
        result = result && lowerBound.x <= aabb.lowerBound.x;
        result = result && lowerBound.y <= aabb.lowerBound.y;
        result = result && aabb.upperBound.x <= upperBound.x;
        result = result && aabb.upperBound.y <= upperBound.y;
        return result;
    }

    bool RayCast(b2RayCastOutput *output, const b2RayCastInput &input) const;

    b2Vec2 lowerBound;///< the lower vertex
    b2Vec2 upperBound;///< the upper vertex
};

/// Compute the collision manifold between two circles.
void b2CollideCircles(b2Manifold *manifold, const b2CircleShape *circleA, const b2Transform &xfA,
                      const b2CircleShape *circleB, const b2Transform &xfB);

/// Compute the collision manifold between a polygon and a circle.
void b2CollidePolygonAndCircle(b2Manifold *manifold, const b2PolygonShape *polygonA,
                               const b2Transform &xfA, const b2CircleShape *circleB,
                               const b2Transform &xfB);

/// Compute the collision manifold between two polygons.
void b2CollidePolygons(b2Manifold *manifold, const b2PolygonShape *polygonA, const b2Transform &xfA,
                       const b2PolygonShape *polygonB, const b2Transform &xfB);

/// Compute the collision manifold between an edge and a circle.
void b2CollideEdgeAndCircle(b2Manifold *manifold, const b2EdgeShape *polygonA,
                            const b2Transform &xfA, const b2CircleShape *circleB,
                            const b2Transform &xfB);

/// Compute the collision manifold between an edge and a polygon.
void b2CollideEdgeAndPolygon(b2Manifold *manifold, const b2EdgeShape *edgeA, const b2Transform &xfA,
                             const b2PolygonShape *circleB, const b2Transform &xfB);

/// Clipping for contact manifolds.
int32 b2ClipSegmentToLine(b2ClipVertex vOut[2], const b2ClipVertex vIn[2], const b2Vec2 &normal,
                          float offset, int32 vertexIndexA);

/// Determine if two generic shapes overlap.
bool b2TestOverlap(const b2Shape *shapeA, int32 indexA, const b2Shape *shapeB, int32 indexB,
                   const b2Transform &xfA, const b2Transform &xfB);

// ---------------- Inline Functions ------------------------------------------

inline bool b2AABB::IsValid() const {
    b2Vec2 d = upperBound - lowerBound;
    bool valid = d.x >= 0.0f && d.y >= 0.0f;
    valid = valid && lowerBound.IsValid() && upperBound.IsValid();
    return valid;
}

inline bool b2TestOverlap(const b2AABB &a, const b2AABB &b) {
    b2Vec2 d1, d2;
    d1 = b.lowerBound - a.upperBound;
    d2 = a.lowerBound - b.upperBound;

    if (d1.x > 0.0f || d1.y > 0.0f) return false;

    if (d2.x > 0.0f || d2.y > 0.0f) return false;

    return true;
}

#pragma endregion

#pragma region

class b2BlockAllocator;

/// This holds the mass data computed for a shape.
struct b2MassData
{
    /// The mass of the shape, usually in kilograms.
    float mass;

    /// The position of the shape's centroid relative to the shape's origin.
    b2Vec2 center;

    /// The rotational inertia of the shape about the local origin.
    float I;
};

/// A shape is used for collision detection. You can create a shape however you like.
/// Shapes used for simulation in b2World are created automatically when a b2Fixture
/// is created. Shapes may encapsulate a one or more child shapes.
class b2Shape {
public:
    enum Type {
        e_circle = 0,
        e_edge = 1,
        e_polygon = 2,
        e_chain = 3,
        e_typeCount = 4
    };

    virtual ~b2Shape() {}

    /// Clone the concrete shape using the provided allocator.
    virtual b2Shape *Clone(b2BlockAllocator *allocator) const = 0;

    /// Get the type of this shape. You can use this to down cast to the concrete shape.
    /// @return the shape type.
    Type GetType() const;

    /// Get the number of child primitives.
    virtual int32 GetChildCount() const = 0;

    /// Test a point for containment in this shape. This only works for convex shapes.
    /// @param xf the shape world transform.
    /// @param p a point in world coordinates.
    virtual bool TestPoint(const b2Transform &xf, const b2Vec2 &p) const = 0;

    /// Cast a ray against a child shape.
    /// @param output the ray-cast results.
    /// @param input the ray-cast input parameters.
    /// @param transform the transform to be applied to the shape.
    /// @param childIndex the child shape index
    virtual bool RayCast(b2RayCastOutput *output, const b2RayCastInput &input,
                         const b2Transform &transform, int32 childIndex) const = 0;

    /// Given a transform, compute the associated axis aligned bounding box for a child shape.
    /// @param aabb returns the axis aligned box.
    /// @param xf the world transform of the shape.
    /// @param childIndex the child shape
    virtual void ComputeAABB(b2AABB *aabb, const b2Transform &xf, int32 childIndex) const = 0;

    /// Compute the mass properties of this shape using its dimensions and density.
    /// The inertia tensor is computed about the local origin.
    /// @param massData returns the mass data for this shape.
    /// @param density the density in kilograms per meter squared.
    virtual void ComputeMass(b2MassData *massData, float density) const = 0;

    Type m_type;

    /// Radius of a shape. For polygonal shapes this must be b2_polygonRadius. There is no support for
    /// making rounded polygons.
    float m_radius;
};

inline b2Shape::Type b2Shape::GetType() const { return m_type; }

#pragma endregion

#pragma region

class b2Fixture;
class b2Joint;
class b2Contact;
class b2Controller;
class b2World;
struct b2FixtureDef;
struct b2JointEdge;
struct b2ContactEdge;

/// The body type.
/// static: zero mass, zero velocity, may be manually moved
/// kinematic: zero mass, non-zero velocity set by user, moved by solver
/// dynamic: positive mass, non-zero velocity determined by forces, moved by solver
enum b2BodyType {
    b2_staticBody = 0,
    b2_kinematicBody,
    b2_dynamicBody
};

/// A body definition holds all the data needed to construct a rigid body.
/// You can safely re-use body definitions. Shapes are added to a body after construction.
struct b2BodyDef
{
    /// This constructor sets the body definition default values.
    b2BodyDef() {
        position.Set(0.0f, 0.0f);
        angle = 0.0f;
        linearVelocity.Set(0.0f, 0.0f);
        angularVelocity = 0.0f;
        linearDamping = 0.0f;
        angularDamping = 0.0f;
        allowSleep = true;
        awake = true;
        fixedRotation = false;
        bullet = false;
        type = b2_staticBody;
        enabled = true;
        gravityScale = 1.0f;
    }

    /// The body type: static, kinematic, or dynamic.
    /// Note: if a dynamic body would have zero mass, the mass is set to one.
    b2BodyType type;

    /// The world position of the body. Avoid creating bodies at the origin
    /// since this can lead to many overlapping shapes.
    b2Vec2 position;

    /// The world angle of the body in radians.
    float angle;

    /// The linear velocity of the body's origin in world co-ordinates.
    b2Vec2 linearVelocity;

    /// The angular velocity of the body.
    float angularVelocity;

    /// Linear damping is use to reduce the linear velocity. The damping parameter
    /// can be larger than 1.0f but the damping effect becomes sensitive to the
    /// time step when the damping parameter is large.
    /// Units are 1/time
    float linearDamping;

    /// Angular damping is use to reduce the angular velocity. The damping parameter
    /// can be larger than 1.0f but the damping effect becomes sensitive to the
    /// time step when the damping parameter is large.
    /// Units are 1/time
    float angularDamping;

    /// Set this flag to false if this body should never fall asleep. Note that
    /// this increases CPU usage.
    bool allowSleep;

    /// Is this body initially awake or sleeping?
    bool awake;

    /// Should this body be prevented from rotating? Useful for characters.
    bool fixedRotation;

    /// Is this a fast moving body that should be prevented from tunneling through
    /// other moving bodies? Note that all bodies are prevented from tunneling through
    /// kinematic and static bodies. This setting is only considered on dynamic bodies.
    /// @warning You should use this flag sparingly since it increases processing time.
    bool bullet;

    /// Does this body start out enabled?
    bool enabled;

    /// Use this to store application specific body data.
    b2BodyUserData userData;

    /// Scale the gravity applied to this body.
    float gravityScale;
};

/// A rigid body. These are created via b2World::CreateBody.
class b2Body {
public:
    /// Creates a fixture and attach it to this body. Use this function if you need
    /// to set some fixture parameters, like friction. Otherwise you can create the
    /// fixture directly from a shape.
    /// If the density is non-zero, this function automatically updates the mass of the body.
    /// Contacts are not created until the next time step.
    /// @param def the fixture definition.
    /// @warning This function is locked during callbacks.
    b2Fixture *CreateFixture(const b2FixtureDef *def);

    /// Creates a fixture from a shape and attach it to this body.
    /// This is a convenience function. Use b2FixtureDef if you need to set parameters
    /// like friction, restitution, user data, or filtering.
    /// If the density is non-zero, this function automatically updates the mass of the body.
    /// @param shape the shape to be cloned.
    /// @param density the shape density (set to zero for static bodies).
    /// @warning This function is locked during callbacks.
    b2Fixture *CreateFixture(const b2Shape *shape, float density);

    /// Destroy a fixture. This removes the fixture from the broad-phase and
    /// destroys all contacts associated with this fixture. This will
    /// automatically adjust the mass of the body if the body is dynamic and the
    /// fixture has positive density.
    /// All fixtures attached to a body are implicitly destroyed when the body is destroyed.
    /// @param fixture the fixture to be removed.
    /// @warning This function is locked during callbacks.
    void DestroyFixture(b2Fixture *fixture);

    /// Set the position of the body's origin and rotation.
    /// Manipulating a body's transform may cause non-physical behavior.
    /// Note: contacts are updated on the next call to b2World::Step.
    /// @param position the world position of the body's local origin.
    /// @param angle the world rotation in radians.
    void SetTransform(const b2Vec2 &position, float angle);

    /// Get the body transform for the body's origin.
    /// @return the world transform of the body's origin.
    const b2Transform &GetTransform() const;

    /// Get the world body origin position.
    /// @return the world position of the body's origin.
    const b2Vec2 &GetPosition() const;

    /// Get the angle in radians.
    /// @return the current world rotation angle in radians.
    float GetAngle() const;

    /// Get the world position of the center of mass.
    const b2Vec2 &GetWorldCenter() const;

    /// Get the local position of the center of mass.
    const b2Vec2 &GetLocalCenter() const;

    /// Set the linear velocity of the center of mass.
    /// @param v the new linear velocity of the center of mass.
    void SetLinearVelocity(const b2Vec2 &v);

    /// Get the linear velocity of the center of mass.
    /// @return the linear velocity of the center of mass.
    const b2Vec2 &GetLinearVelocity() const;

    /// Set the angular velocity.
    /// @param omega the new angular velocity in radians/second.
    void SetAngularVelocity(float omega);

    /// Get the angular velocity.
    /// @return the angular velocity in radians/second.
    float GetAngularVelocity() const;

    /// Apply a force at a world point. If the force is not
    /// applied at the center of mass, it will generate a torque and
    /// affect the angular velocity. This wakes up the body.
    /// @param force the world force vector, usually in Newtons (N).
    /// @param point the world position of the point of application.
    /// @param wake also wake up the body
    void ApplyForce(const b2Vec2 &force, const b2Vec2 &point, bool wake);

    /// Apply a force to the center of mass. This wakes up the body.
    /// @param force the world force vector, usually in Newtons (N).
    /// @param wake also wake up the body
    void ApplyForceToCenter(const b2Vec2 &force, bool wake);

    /// Apply a torque. This affects the angular velocity
    /// without affecting the linear velocity of the center of mass.
    /// @param torque about the z-axis (out of the screen), usually in N-m.
    /// @param wake also wake up the body
    void ApplyTorque(float torque, bool wake);

    /// Apply an impulse at a point. This immediately modifies the velocity.
    /// It also modifies the angular velocity if the point of application
    /// is not at the center of mass. This wakes up the body.
    /// @param impulse the world impulse vector, usually in N-seconds or kg-m/s.
    /// @param point the world position of the point of application.
    /// @param wake also wake up the body
    void ApplyLinearImpulse(const b2Vec2 &impulse, const b2Vec2 &point, bool wake);

    /// Apply an impulse to the center of mass. This immediately modifies the velocity.
    /// @param impulse the world impulse vector, usually in N-seconds or kg-m/s.
    /// @param wake also wake up the body
    void ApplyLinearImpulseToCenter(const b2Vec2 &impulse, bool wake);

    /// Apply an angular impulse.
    /// @param impulse the angular impulse in units of kg*m*m/s
    /// @param wake also wake up the body
    void ApplyAngularImpulse(float impulse, bool wake);

    /// Get the total mass of the body.
    /// @return the mass, usually in kilograms (kg).
    float GetMass() const;

    /// Get the rotational inertia of the body about the local origin.
    /// @return the rotational inertia, usually in kg-m^2.
    float GetInertia() const;

    /// Get the mass data of the body.
    /// @return a struct containing the mass, inertia and center of the body.
    b2MassData GetMassData() const;

    /// Set the mass properties to override the mass properties of the fixtures.
    /// Note that this changes the center of mass position.
    /// Note that creating or destroying fixtures can also alter the mass.
    /// This function has no effect if the body isn't dynamic.
    /// @param data the mass properties.
    void SetMassData(const b2MassData *data);

    /// This resets the mass properties to the sum of the mass properties of the fixtures.
    /// This normally does not need to be called unless you called SetMassData to override
    /// the mass and you later want to reset the mass.
    void ResetMassData();

    /// Get the world coordinates of a point given the local coordinates.
    /// @param localPoint a point on the body measured relative the the body's origin.
    /// @return the same point expressed in world coordinates.
    b2Vec2 GetWorldPoint(const b2Vec2 &localPoint) const;

    /// Get the world coordinates of a vector given the local coordinates.
    /// @param localVector a vector fixed in the body.
    /// @return the same vector expressed in world coordinates.
    b2Vec2 GetWorldVector(const b2Vec2 &localVector) const;

    /// Gets a local point relative to the body's origin given a world point.
    /// @param worldPoint a point in world coordinates.
    /// @return the corresponding local point relative to the body's origin.
    b2Vec2 GetLocalPoint(const b2Vec2 &worldPoint) const;

    /// Gets a local vector given a world vector.
    /// @param worldVector a vector in world coordinates.
    /// @return the corresponding local vector.
    b2Vec2 GetLocalVector(const b2Vec2 &worldVector) const;

    /// Get the world linear velocity of a world point attached to this body.
    /// @param worldPoint a point in world coordinates.
    /// @return the world velocity of a point.
    b2Vec2 GetLinearVelocityFromWorldPoint(const b2Vec2 &worldPoint) const;

    /// Get the world velocity of a local point.
    /// @param localPoint a point in local coordinates.
    /// @return the world velocity of a point.
    b2Vec2 GetLinearVelocityFromLocalPoint(const b2Vec2 &localPoint) const;

    /// Get the linear damping of the body.
    float GetLinearDamping() const;

    /// Set the linear damping of the body.
    void SetLinearDamping(float linearDamping);

    /// Get the angular damping of the body.
    float GetAngularDamping() const;

    /// Set the angular damping of the body.
    void SetAngularDamping(float angularDamping);

    /// Get the gravity scale of the body.
    float GetGravityScale() const;

    /// Set the gravity scale of the body.
    void SetGravityScale(float scale);

    /// Set the type of this body. This may alter the mass and velocity.
    void SetType(b2BodyType type);

    /// Get the type of this body.
    b2BodyType GetType() const;

    /// Should this body be treated like a bullet for continuous collision detection?
    void SetBullet(bool flag);

    /// Is this body treated like a bullet for continuous collision detection?
    bool IsBullet() const;

    /// You can disable sleeping on this body. If you disable sleeping, the
    /// body will be woken.
    void SetSleepingAllowed(bool flag);

    /// Is this body allowed to sleep
    bool IsSleepingAllowed() const;

    /// Set the sleep state of the body. A sleeping body has very
    /// low CPU cost.
    /// @param flag set to true to wake the body, false to put it to sleep.
    void SetAwake(bool flag);

    /// Get the sleeping state of this body.
    /// @return true if the body is awake.
    bool IsAwake() const;

    /// Allow a body to be disabled. A disabled body is not simulated and cannot
    /// be collided with or woken up.
    /// If you pass a flag of true, all fixtures will be added to the broad-phase.
    /// If you pass a flag of false, all fixtures will be removed from the
    /// broad-phase and all contacts will be destroyed.
    /// Fixtures and joints are otherwise unaffected. You may continue
    /// to create/destroy fixtures and joints on disabled bodies.
    /// Fixtures on a disabled body are implicitly disabled and will
    /// not participate in collisions, ray-casts, or queries.
    /// Joints connected to a disabled body are implicitly disabled.
    /// An diabled body is still owned by a b2World object and remains
    /// in the body list.
    void SetEnabled(bool flag);

    /// Get the active state of the body.
    bool IsEnabled() const;

    /// Set this body to have fixed rotation. This causes the mass
    /// to be reset.
    void SetFixedRotation(bool flag);

    /// Does this body have fixed rotation?
    bool IsFixedRotation() const;

    /// Get the list of all fixtures attached to this body.
    b2Fixture *GetFixtureList();
    const b2Fixture *GetFixtureList() const;

    /// Get the list of all joints attached to this body.
    b2JointEdge *GetJointList();
    const b2JointEdge *GetJointList() const;

    /// Get the list of all contacts attached to this body.
    /// @warning this list changes during the time step and you may
    /// miss some collisions if you don't use b2ContactListener.
    b2ContactEdge *GetContactList();
    const b2ContactEdge *GetContactList() const;

    /// Get the next body in the world's body list.
    b2Body *GetNext();
    const b2Body *GetNext() const;

    /// Get the user data pointer that was provided in the body definition.
    b2BodyUserData &GetUserData();
    const b2BodyUserData &GetUserData() const;

    /// Get the parent world of this body.
    b2World *GetWorld();
    const b2World *GetWorld() const;

    /// Dump this body to a file
    void Dump();

private:
    friend class b2World;
    friend class b2Island;
    friend class b2ContactManager;
    friend class b2ContactSolver;
    friend class b2Contact;

    friend class b2DistanceJoint;
    friend class b2FrictionJoint;
    friend class b2GearJoint;
    friend class b2MotorJoint;
    friend class b2MouseJoint;
    friend class b2PrismaticJoint;
    friend class b2PulleyJoint;
    friend class b2RevoluteJoint;
    friend class b2WeldJoint;
    friend class b2WheelJoint;

    // m_flags
    enum {
        e_islandFlag = 0x0001,
        e_awakeFlag = 0x0002,
        e_autoSleepFlag = 0x0004,
        e_bulletFlag = 0x0008,
        e_fixedRotationFlag = 0x0010,
        e_enabledFlag = 0x0020,
        e_toiFlag = 0x0040
    };

    b2Body(const b2BodyDef *bd, b2World *world);
    ~b2Body();

    void SynchronizeFixtures();
    void SynchronizeTransform();

    // This is used to prevent connected bodies from colliding.
    // It may lie, depending on the collideConnected flag.
    bool ShouldCollide(const b2Body *other) const;

    void Advance(float t);

    b2BodyType m_type;

    uint16 m_flags;

    int32 m_islandIndex;

    b2Transform m_xf;// the body origin transform
    b2Sweep m_sweep; // the swept motion for CCD

    b2Vec2 m_linearVelocity;
    float m_angularVelocity;

    b2Vec2 m_force;
    float m_torque;

    b2World *m_world;
    b2Body *m_prev;
    b2Body *m_next;

    b2Fixture *m_fixtureList;
    int32 m_fixtureCount;

    b2JointEdge *m_jointList;
    b2ContactEdge *m_contactList;

    float m_mass, m_invMass;

    // Rotational inertia about the center of mass.
    float m_I, m_invI;

    float m_linearDamping;
    float m_angularDamping;
    float m_gravityScale;

    float m_sleepTime;

    b2BodyUserData m_userData;
};

inline b2BodyType b2Body::GetType() const { return m_type; }

inline const b2Transform &b2Body::GetTransform() const { return m_xf; }

inline const b2Vec2 &b2Body::GetPosition() const { return m_xf.p; }

inline float b2Body::GetAngle() const { return m_sweep.a; }

inline const b2Vec2 &b2Body::GetWorldCenter() const { return m_sweep.c; }

inline const b2Vec2 &b2Body::GetLocalCenter() const { return m_sweep.localCenter; }

inline void b2Body::SetLinearVelocity(const b2Vec2 &v) {
    if (m_type == b2_staticBody) { return; }

    if (b2Dot(v, v) > 0.0f) { SetAwake(true); }

    m_linearVelocity = v;
}

inline const b2Vec2 &b2Body::GetLinearVelocity() const { return m_linearVelocity; }

inline void b2Body::SetAngularVelocity(float w) {
    if (m_type == b2_staticBody) { return; }

    if (w * w > 0.0f) { SetAwake(true); }

    m_angularVelocity = w;
}

inline float b2Body::GetAngularVelocity() const { return m_angularVelocity; }

inline float b2Body::GetMass() const { return m_mass; }

inline float b2Body::GetInertia() const {
    return m_I + m_mass * b2Dot(m_sweep.localCenter, m_sweep.localCenter);
}

inline b2MassData b2Body::GetMassData() const {
    b2MassData data;
    data.mass = m_mass;
    data.I = m_I + m_mass * b2Dot(m_sweep.localCenter, m_sweep.localCenter);
    data.center = m_sweep.localCenter;
    return data;
}

inline b2Vec2 b2Body::GetWorldPoint(const b2Vec2 &localPoint) const {
    return b2Mul(m_xf, localPoint);
}

inline b2Vec2 b2Body::GetWorldVector(const b2Vec2 &localVector) const {
    return b2Mul(m_xf.q, localVector);
}

inline b2Vec2 b2Body::GetLocalPoint(const b2Vec2 &worldPoint) const {
    return b2MulT(m_xf, worldPoint);
}

inline b2Vec2 b2Body::GetLocalVector(const b2Vec2 &worldVector) const {
    return b2MulT(m_xf.q, worldVector);
}

inline b2Vec2 b2Body::GetLinearVelocityFromWorldPoint(const b2Vec2 &worldPoint) const {
    return m_linearVelocity + b2Cross(m_angularVelocity, worldPoint - m_sweep.c);
}

inline b2Vec2 b2Body::GetLinearVelocityFromLocalPoint(const b2Vec2 &localPoint) const {
    return GetLinearVelocityFromWorldPoint(GetWorldPoint(localPoint));
}

inline float b2Body::GetLinearDamping() const { return m_linearDamping; }

inline void b2Body::SetLinearDamping(float linearDamping) { m_linearDamping = linearDamping; }

inline float b2Body::GetAngularDamping() const { return m_angularDamping; }

inline void b2Body::SetAngularDamping(float angularDamping) { m_angularDamping = angularDamping; }

inline float b2Body::GetGravityScale() const { return m_gravityScale; }

inline void b2Body::SetGravityScale(float scale) { m_gravityScale = scale; }

inline void b2Body::SetBullet(bool flag) {
    if (flag) {
        m_flags |= e_bulletFlag;
    } else {
        m_flags &= ~e_bulletFlag;
    }
}

inline bool b2Body::IsBullet() const { return (m_flags & e_bulletFlag) == e_bulletFlag; }

inline void b2Body::SetAwake(bool flag) {
    if (m_type == b2_staticBody) { return; }

    if (flag) {
        m_flags |= e_awakeFlag;
        m_sleepTime = 0.0f;
    } else {
        m_flags &= ~e_awakeFlag;
        m_sleepTime = 0.0f;
        m_linearVelocity.SetZero();
        m_angularVelocity = 0.0f;
        m_force.SetZero();
        m_torque = 0.0f;
    }
}

inline bool b2Body::IsAwake() const { return (m_flags & e_awakeFlag) == e_awakeFlag; }

inline bool b2Body::IsEnabled() const { return (m_flags & e_enabledFlag) == e_enabledFlag; }

inline bool b2Body::IsFixedRotation() const {
    return (m_flags & e_fixedRotationFlag) == e_fixedRotationFlag;
}

inline void b2Body::SetSleepingAllowed(bool flag) {
    if (flag) {
        m_flags |= e_autoSleepFlag;
    } else {
        m_flags &= ~e_autoSleepFlag;
        SetAwake(true);
    }
}

inline bool b2Body::IsSleepingAllowed() const {
    return (m_flags & e_autoSleepFlag) == e_autoSleepFlag;
}

inline b2Fixture *b2Body::GetFixtureList() { return m_fixtureList; }

inline const b2Fixture *b2Body::GetFixtureList() const { return m_fixtureList; }

inline b2JointEdge *b2Body::GetJointList() { return m_jointList; }

inline const b2JointEdge *b2Body::GetJointList() const { return m_jointList; }

inline b2ContactEdge *b2Body::GetContactList() { return m_contactList; }

inline const b2ContactEdge *b2Body::GetContactList() const { return m_contactList; }

inline b2Body *b2Body::GetNext() { return m_next; }

inline const b2Body *b2Body::GetNext() const { return m_next; }

inline b2BodyUserData &b2Body::GetUserData() { return m_userData; }

inline const b2BodyUserData &b2Body::GetUserData() const { return m_userData; }

inline void b2Body::ApplyForce(const b2Vec2 &force, const b2Vec2 &point, bool wake) {
    if (m_type != b2_dynamicBody) { return; }

    if (wake && (m_flags & e_awakeFlag) == 0) { SetAwake(true); }

    // Don't accumulate a force if the body is sleeping.
    if (m_flags & e_awakeFlag) {
        m_force += force;
        m_torque += b2Cross(point - m_sweep.c, force);
    }
}

inline void b2Body::ApplyForceToCenter(const b2Vec2 &force, bool wake) {
    if (m_type != b2_dynamicBody) { return; }

    if (wake && (m_flags & e_awakeFlag) == 0) { SetAwake(true); }

    // Don't accumulate a force if the body is sleeping
    if (m_flags & e_awakeFlag) { m_force += force; }
}

inline void b2Body::ApplyTorque(float torque, bool wake) {
    if (m_type != b2_dynamicBody) { return; }

    if (wake && (m_flags & e_awakeFlag) == 0) { SetAwake(true); }

    // Don't accumulate a force if the body is sleeping
    if (m_flags & e_awakeFlag) { m_torque += torque; }
}

inline void b2Body::ApplyLinearImpulse(const b2Vec2 &impulse, const b2Vec2 &point, bool wake) {
    if (m_type != b2_dynamicBody) { return; }

    if (wake && (m_flags & e_awakeFlag) == 0) { SetAwake(true); }

    // Don't accumulate velocity if the body is sleeping
    if (m_flags & e_awakeFlag) {
        m_linearVelocity += m_invMass * impulse;
        m_angularVelocity += m_invI * b2Cross(point - m_sweep.c, impulse);
    }
}

inline void b2Body::ApplyLinearImpulseToCenter(const b2Vec2 &impulse, bool wake) {
    if (m_type != b2_dynamicBody) { return; }

    if (wake && (m_flags & e_awakeFlag) == 0) { SetAwake(true); }

    // Don't accumulate velocity if the body is sleeping
    if (m_flags & e_awakeFlag) { m_linearVelocity += m_invMass * impulse; }
}

inline void b2Body::ApplyAngularImpulse(float impulse, bool wake) {
    if (m_type != b2_dynamicBody) { return; }

    if (wake && (m_flags & e_awakeFlag) == 0) { SetAwake(true); }

    // Don't accumulate velocity if the body is sleeping
    if (m_flags & e_awakeFlag) { m_angularVelocity += m_invI * impulse; }
}

inline void b2Body::SynchronizeTransform() {
    m_xf.q.Set(m_sweep.a);
    m_xf.p = m_sweep.c - b2Mul(m_xf.q, m_sweep.localCenter);
}

inline void b2Body::Advance(float alpha) {
    // Advance to the new safe time. This doesn't sync the broad-phase.
    m_sweep.Advance(alpha);
    m_sweep.c = m_sweep.c0;
    m_sweep.a = m_sweep.a0;
    m_xf.q.Set(m_sweep.a);
    m_xf.p = m_sweep.c - b2Mul(m_xf.q, m_sweep.localCenter);
}

inline b2World *b2Body::GetWorld() { return m_world; }

inline const b2World *b2Body::GetWorld() const { return m_world; }

#pragma endregion

#pragma region

class b2BlockAllocator;
class b2Body;
class b2BroadPhase;
class b2Fixture;

/// This holds contact filtering data.
struct b2Filter
{
    b2Filter() {
        categoryBits = 0x0001;
        maskBits = 0xFFFF;
        groupIndex = 0;
    }

    /// The collision category bits. Normally you would just set one bit.
    uint16 categoryBits;

    /// The collision mask bits. This states the categories that this
    /// shape would accept for collision.
    uint16 maskBits;

    /// Collision groups allow a certain group of objects to never collide (negative)
    /// or always collide (positive). Zero means no collision group. Non-zero group
    /// filtering always wins against the mask bits.
    int16 groupIndex;
};

/// A fixture definition is used to create a fixture. This class defines an
/// abstract fixture definition. You can reuse fixture definitions safely.
struct b2FixtureDef
{
    /// The constructor sets the default fixture definition values.
    b2FixtureDef() {
        shape = nullptr;
        friction = 0.2f;
        restitution = 0.0f;
        restitutionThreshold = 1.0f * b2_lengthUnitsPerMeter;
        density = 0.0f;
        isSensor = false;
    }

    /// The shape, this must be set. The shape will be cloned, so you
    /// can create the shape on the stack.
    const b2Shape *shape;

    /// Use this to store application specific fixture data.
    b2FixtureUserData userData;

    /// The friction coefficient, usually in the range [0,1].
    float friction;

    /// The restitution (elasticity) usually in the range [0,1].
    float restitution;

    /// Restitution velocity threshold, usually in m/s. Collisions above this
    /// speed have restitution applied (will bounce).
    float restitutionThreshold;

    /// The density, usually in kg/m^2.
    float density;

    /// A sensor shape collects contact information but never generates a collision
    /// response.
    bool isSensor;

    /// Contact filtering data.
    b2Filter filter;
};

/// This proxy is used internally to connect fixtures to the broad-phase.
struct b2FixtureProxy
{
    b2AABB aabb;
    b2Fixture *fixture;
    int32 childIndex;
    int32 proxyId;
};

/// A fixture is used to attach a shape to a body for collision detection. A fixture
/// inherits its transform from its parent. Fixtures hold additional non-geometric data
/// such as friction, collision filters, etc.
/// Fixtures are created via b2Body::CreateFixture.
/// @warning you cannot reuse fixtures.
class b2Fixture {
public:
    /// Get the type of the child shape. You can use this to down cast to the concrete shape.
    /// @return the shape type.
    b2Shape::Type GetType() const;

    /// Get the child shape. You can modify the child shape, however you should not change the
    /// number of vertices because this will crash some collision caching mechanisms.
    /// Manipulating the shape may lead to non-physical behavior.
    b2Shape *GetShape();
    const b2Shape *GetShape() const;

    /// Set if this fixture is a sensor.
    void SetSensor(bool sensor);

    /// Is this fixture a sensor (non-solid)?
    /// @return the true if the shape is a sensor.
    bool IsSensor() const;

    /// Set the contact filtering data. This will not update contacts until the next time
    /// step when either parent body is active and awake.
    /// This automatically calls Refilter.
    void SetFilterData(const b2Filter &filter);

    /// Get the contact filtering data.
    const b2Filter &GetFilterData() const;

    /// Call this if you want to establish collision that was previously disabled by b2ContactFilter::ShouldCollide.
    void Refilter();

    /// Get the parent body of this fixture. This is nullptr if the fixture is not attached.
    /// @return the parent body.
    b2Body *GetBody();
    const b2Body *GetBody() const;

    /// Get the next fixture in the parent body's fixture list.
    /// @return the next shape.
    b2Fixture *GetNext();
    const b2Fixture *GetNext() const;

    /// Get the user data that was assigned in the fixture definition. Use this to
    /// store your application specific data.
    b2FixtureUserData &GetUserData();
    const b2FixtureUserData &GetUserData() const;

    /// Test a point for containment in this fixture.
    /// @param p a point in world coordinates.
    bool TestPoint(const b2Vec2 &p) const;

    /// Cast a ray against this shape.
    /// @param output the ray-cast results.
    /// @param input the ray-cast input parameters.
    /// @param childIndex the child shape index (e.g. edge index)
    bool RayCast(b2RayCastOutput *output, const b2RayCastInput &input, int32 childIndex) const;

    /// Get the mass data for this fixture. The mass data is based on the density and
    /// the shape. The rotational inertia is about the shape's origin. This operation
    /// may be expensive.
    void GetMassData(b2MassData *massData) const;

    /// Set the density of this fixture. This will _not_ automatically adjust the mass
    /// of the body. You must call b2Body::ResetMassData to update the body's mass.
    void SetDensity(float density);

    /// Get the density of this fixture.
    float GetDensity() const;

    /// Get the coefficient of friction.
    float GetFriction() const;

    /// Set the coefficient of friction. This will _not_ change the friction of
    /// existing contacts.
    void SetFriction(float friction);

    /// Get the coefficient of restitution.
    float GetRestitution() const;

    /// Set the coefficient of restitution. This will _not_ change the restitution of
    /// existing contacts.
    void SetRestitution(float restitution);

    /// Get the restitution velocity threshold.
    float GetRestitutionThreshold() const;

    /// Set the restitution threshold. This will _not_ change the restitution threshold of
    /// existing contacts.
    void SetRestitutionThreshold(float threshold);

    /// Get the fixture's AABB. This AABB may be enlarge and/or stale.
    /// If you need a more accurate AABB, compute it using the shape and
    /// the body transform.
    const b2AABB &GetAABB(int32 childIndex) const;

    /// Dump this fixture to the log file.
    void Dump(int32 bodyIndex);

protected:
    friend class b2Body;
    friend class b2World;
    friend class b2Contact;
    friend class b2ContactManager;

    b2Fixture();

    // We need separation create/destroy functions from the constructor/destructor because
    // the destructor cannot access the allocator (no destructor arguments allowed by C++).
    void Create(b2BlockAllocator *allocator, b2Body *body, const b2FixtureDef *def);
    void Destroy(b2BlockAllocator *allocator);

    // These support body activation/deactivation.
    void CreateProxies(b2BroadPhase *broadPhase, const b2Transform &xf);
    void DestroyProxies(b2BroadPhase *broadPhase);

    void Synchronize(b2BroadPhase *broadPhase, const b2Transform &xf1, const b2Transform &xf2);

    float m_density;

    b2Fixture *m_next;
    b2Body *m_body;

    b2Shape *m_shape;

    float m_friction;
    float m_restitution;
    float m_restitutionThreshold;

    b2FixtureProxy *m_proxies;
    int32 m_proxyCount;

    b2Filter m_filter;

    bool m_isSensor;

    b2FixtureUserData m_userData;
};

inline b2Shape::Type b2Fixture::GetType() const { return m_shape->GetType(); }

inline b2Shape *b2Fixture::GetShape() { return m_shape; }

inline const b2Shape *b2Fixture::GetShape() const { return m_shape; }

inline bool b2Fixture::IsSensor() const { return m_isSensor; }

inline const b2Filter &b2Fixture::GetFilterData() const { return m_filter; }

inline b2FixtureUserData &b2Fixture::GetUserData() { return m_userData; }

inline const b2FixtureUserData &b2Fixture::GetUserData() const { return m_userData; }

inline b2Body *b2Fixture::GetBody() { return m_body; }

inline const b2Body *b2Fixture::GetBody() const { return m_body; }

inline b2Fixture *b2Fixture::GetNext() { return m_next; }

inline const b2Fixture *b2Fixture::GetNext() const { return m_next; }

inline void b2Fixture::SetDensity(float density) {
    b2Assert(b2IsValid(density) && density >= 0.0f);
    m_density = density;
}

inline float b2Fixture::GetDensity() const { return m_density; }

inline float b2Fixture::GetFriction() const { return m_friction; }

inline void b2Fixture::SetFriction(float friction) { m_friction = friction; }

inline float b2Fixture::GetRestitution() const { return m_restitution; }

inline void b2Fixture::SetRestitution(float restitution) { m_restitution = restitution; }

inline float b2Fixture::GetRestitutionThreshold() const { return m_restitutionThreshold; }

inline void b2Fixture::SetRestitutionThreshold(float threshold) {
    m_restitutionThreshold = threshold;
}

inline bool b2Fixture::TestPoint(const b2Vec2 &p) const {
    return m_shape->TestPoint(m_body->GetTransform(), p);
}

inline bool b2Fixture::RayCast(b2RayCastOutput *output, const b2RayCastInput &input,
                               int32 childIndex) const {
    return m_shape->RayCast(output, input, m_body->GetTransform(), childIndex);
}

inline void b2Fixture::GetMassData(b2MassData *massData) const {
    m_shape->ComputeMass(massData, m_density);
}

inline const b2AABB &b2Fixture::GetAABB(int32 childIndex) const {
    b2Assert(0 <= childIndex && childIndex < m_proxyCount);
    return m_proxies[childIndex].aabb;
}

#pragma endregion

#pragma region

class b2Body;
class b2Contact;
class b2Fixture;
class b2World;
class b2BlockAllocator;
class b2StackAllocator;
class b2ContactListener;

/// Friction mixing law. The idea is to allow either fixture to drive the friction to zero.
/// For example, anything slides on ice.
inline float b2MixFriction(float friction1, float friction2) {
    return b2Sqrt(friction1 * friction2);
}

/// Restitution mixing law. The idea is allow for anything to bounce off an inelastic surface.
/// For example, a superball bounces on anything.
inline float b2MixRestitution(float restitution1, float restitution2) {
    return restitution1 > restitution2 ? restitution1 : restitution2;
}

/// Restitution mixing law. This picks the lowest value.
inline float b2MixRestitutionThreshold(float threshold1, float threshold2) {
    return threshold1 < threshold2 ? threshold1 : threshold2;
}

typedef b2Contact *b2ContactCreateFcn(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB,
                                      int32 indexB, b2BlockAllocator *allocator);
typedef void b2ContactDestroyFcn(b2Contact *contact, b2BlockAllocator *allocator);

struct b2ContactRegister
{
    b2ContactCreateFcn *createFcn;
    b2ContactDestroyFcn *destroyFcn;
    bool primary;
};

/// A contact edge is used to connect bodies and contacts together
/// in a contact graph where each body is a node and each contact
/// is an edge. A contact edge belongs to a doubly linked list
/// maintained in each attached body. Each contact has two contact
/// nodes, one for each attached body.
struct b2ContactEdge
{
    b2Body *other;      ///< provides quick access to the other body attached.
    b2Contact *contact; ///< the contact
    b2ContactEdge *prev;///< the previous contact edge in the body's contact list
    b2ContactEdge *next;///< the next contact edge in the body's contact list
};

/// The class manages contact between two shapes. A contact exists for each overlapping
/// AABB in the broad-phase (except if filtered). Therefore a contact object may exist
/// that has no contact points.
class b2Contact {
public:
    /// Get the contact manifold. Do not modify the manifold unless you understand the
    /// internals of Box2D.
    b2Manifold *GetManifold();
    const b2Manifold *GetManifold() const;

    /// Get the world manifold.
    void GetWorldManifold(b2WorldManifold *worldManifold) const;

    /// Is this contact touching?
    bool IsTouching() const;

    /// Enable/disable this contact. This can be used inside the pre-solve
    /// contact listener. The contact is only disabled for the current
    /// time step (or sub-step in continuous collisions).
    void SetEnabled(bool flag);

    /// Has this contact been disabled?
    bool IsEnabled() const;

    /// Get the next contact in the world's contact list.
    b2Contact *GetNext();
    const b2Contact *GetNext() const;

    /// Get fixture A in this contact.
    b2Fixture *GetFixtureA();
    const b2Fixture *GetFixtureA() const;

    /// Get the child primitive index for fixture A.
    int32 GetChildIndexA() const;

    /// Get fixture B in this contact.
    b2Fixture *GetFixtureB();
    const b2Fixture *GetFixtureB() const;

    /// Get the child primitive index for fixture B.
    int32 GetChildIndexB() const;

    /// Override the default friction mixture. You can call this in b2ContactListener::PreSolve.
    /// This value persists until set or reset.
    void SetFriction(float friction);

    /// Get the friction.
    float GetFriction() const;

    /// Reset the friction mixture to the default value.
    void ResetFriction();

    /// Override the default restitution mixture. You can call this in b2ContactListener::PreSolve.
    /// The value persists until you set or reset.
    void SetRestitution(float restitution);

    /// Get the restitution.
    float GetRestitution() const;

    /// Reset the restitution to the default value.
    void ResetRestitution();

    /// Override the default restitution velocity threshold mixture. You can call this in b2ContactListener::PreSolve.
    /// The value persists until you set or reset.
    void SetRestitutionThreshold(float threshold);

    /// Get the restitution threshold.
    float GetRestitutionThreshold() const;

    /// Reset the restitution threshold to the default value.
    void ResetRestitutionThreshold();

    /// Set the desired tangent speed for a conveyor belt behavior. In meters per second.
    void SetTangentSpeed(float speed);

    /// Get the desired tangent speed. In meters per second.
    float GetTangentSpeed() const;

    /// Evaluate this contact with your own manifold and transforms.
    virtual void Evaluate(b2Manifold *manifold, const b2Transform &xfA, const b2Transform &xfB) = 0;

protected:
    friend class b2ContactManager;
    friend class b2World;
    friend class b2ContactSolver;
    friend class b2Body;
    friend class b2Fixture;

    // Flags stored in m_flags
    enum {
        // Used when crawling contact graph when forming islands.
        e_islandFlag = 0x0001,

        // Set when the shapes are touching.
        e_touchingFlag = 0x0002,

        // This contact can be disabled (by user)
        e_enabledFlag = 0x0004,

        // This contact needs filtering because a fixture filter was changed.
        e_filterFlag = 0x0008,

        // This bullet contact had a TOI event
        e_bulletHitFlag = 0x0010,

        // This contact has a valid TOI in m_toi
        e_toiFlag = 0x0020
    };

    /// Flag this contact for filtering. Filtering will occur the next time step.
    void FlagForFiltering();

    static void AddType(b2ContactCreateFcn *createFcn, b2ContactDestroyFcn *destroyFcn,
                        b2Shape::Type typeA, b2Shape::Type typeB);
    static void InitializeRegisters();
    static b2Contact *Create(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB,
                             b2BlockAllocator *allocator);
    static void Destroy(b2Contact *contact, b2Shape::Type typeA, b2Shape::Type typeB,
                        b2BlockAllocator *allocator);
    static void Destroy(b2Contact *contact, b2BlockAllocator *allocator);

    b2Contact() : m_fixtureA(nullptr), m_fixtureB(nullptr) {}
    b2Contact(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB);
    virtual ~b2Contact() {}

    void Update(b2ContactListener *listener);

    static b2ContactRegister s_registers[b2Shape::e_typeCount][b2Shape::e_typeCount];
    static bool s_initialized;

    uint32 m_flags;

    // World pool and list pointers.
    b2Contact *m_prev;
    b2Contact *m_next;

    // Nodes for connecting bodies.
    b2ContactEdge m_nodeA;
    b2ContactEdge m_nodeB;

    b2Fixture *m_fixtureA;
    b2Fixture *m_fixtureB;

    int32 m_indexA;
    int32 m_indexB;

    b2Manifold m_manifold;

    int32 m_toiCount;
    float m_toi;

    float m_friction;
    float m_restitution;
    float m_restitutionThreshold;

    float m_tangentSpeed;
};

inline b2Manifold *b2Contact::GetManifold() { return &m_manifold; }

inline const b2Manifold *b2Contact::GetManifold() const { return &m_manifold; }

inline void b2Contact::GetWorldManifold(b2WorldManifold *worldManifold) const {
    const b2Body *bodyA = m_fixtureA->GetBody();
    const b2Body *bodyB = m_fixtureB->GetBody();
    const b2Shape *shapeA = m_fixtureA->GetShape();
    const b2Shape *shapeB = m_fixtureB->GetShape();

    worldManifold->Initialize(&m_manifold, bodyA->GetTransform(), shapeA->m_radius,
                              bodyB->GetTransform(), shapeB->m_radius);
}

inline void b2Contact::SetEnabled(bool flag) {
    if (flag) {
        m_flags |= e_enabledFlag;
    } else {
        m_flags &= ~e_enabledFlag;
    }
}

inline bool b2Contact::IsEnabled() const { return (m_flags & e_enabledFlag) == e_enabledFlag; }

inline bool b2Contact::IsTouching() const { return (m_flags & e_touchingFlag) == e_touchingFlag; }

inline b2Contact *b2Contact::GetNext() { return m_next; }

inline const b2Contact *b2Contact::GetNext() const { return m_next; }

inline b2Fixture *b2Contact::GetFixtureA() { return m_fixtureA; }

inline const b2Fixture *b2Contact::GetFixtureA() const { return m_fixtureA; }

inline b2Fixture *b2Contact::GetFixtureB() { return m_fixtureB; }

inline int32 b2Contact::GetChildIndexA() const { return m_indexA; }

inline const b2Fixture *b2Contact::GetFixtureB() const { return m_fixtureB; }

inline int32 b2Contact::GetChildIndexB() const { return m_indexB; }

inline void b2Contact::FlagForFiltering() { m_flags |= e_filterFlag; }

inline void b2Contact::SetFriction(float friction) { m_friction = friction; }

inline float b2Contact::GetFriction() const { return m_friction; }

inline void b2Contact::ResetFriction() {
    m_friction = b2MixFriction(m_fixtureA->m_friction, m_fixtureB->m_friction);
}

inline void b2Contact::SetRestitution(float restitution) { m_restitution = restitution; }

inline float b2Contact::GetRestitution() const { return m_restitution; }

inline void b2Contact::ResetRestitution() {
    m_restitution = b2MixRestitution(m_fixtureA->m_restitution, m_fixtureB->m_restitution);
}

inline void b2Contact::SetRestitutionThreshold(float threshold) {
    m_restitutionThreshold = threshold;
}

inline float b2Contact::GetRestitutionThreshold() const { return m_restitutionThreshold; }

inline void b2Contact::ResetRestitutionThreshold() {
    m_restitutionThreshold = b2MixRestitutionThreshold(m_fixtureA->m_restitutionThreshold,
                                                       m_fixtureB->m_restitutionThreshold);
}

inline void b2Contact::SetTangentSpeed(float speed) { m_tangentSpeed = speed; }

inline float b2Contact::GetTangentSpeed() const { return m_tangentSpeed; }

#pragma endregion

#pragma region

/// Distance joint definition. This requires defining an anchor point on both
/// bodies and the non-zero distance of the distance joint. The definition uses
/// local anchor points so that the initial configuration can violate the
/// constraint slightly. This helps when saving and loading a game.
struct b2DistanceJointDef : public b2JointDef
{
    b2DistanceJointDef() {
        type = e_distanceJoint;
        localAnchorA.Set(0.0f, 0.0f);
        localAnchorB.Set(0.0f, 0.0f);
        length = 1.0f;
        minLength = 0.0f;
        maxLength = FLT_MAX;
        stiffness = 0.0f;
        damping = 0.0f;
    }

    /// Initialize the bodies, anchors, and rest length using world space anchors.
    /// The minimum and maximum lengths are set to the rest length.
    void Initialize(b2Body *bodyA, b2Body *bodyB, const b2Vec2 &anchorA, const b2Vec2 &anchorB);

    /// The local anchor point relative to bodyA's origin.
    b2Vec2 localAnchorA;

    /// The local anchor point relative to bodyB's origin.
    b2Vec2 localAnchorB;

    /// The rest length of this joint. Clamped to a stable minimum value.
    float length;

    /// Minimum length. Clamped to a stable minimum value.
    float minLength;

    /// Maximum length. Must be greater than or equal to the minimum length.
    float maxLength;

    /// The linear stiffness in N/m.
    float stiffness;

    /// The linear damping in N*s/m.
    float damping;
};

/// A distance joint constrains two points on two bodies to remain at a fixed
/// distance from each other. You can view this as a massless, rigid rod.
class b2DistanceJoint : public b2Joint {
public:
    b2Vec2 GetAnchorA() const override;
    b2Vec2 GetAnchorB() const override;

    /// Get the reaction force given the inverse time step.
    /// Unit is N.
    b2Vec2 GetReactionForce(float inv_dt) const override;

    /// Get the reaction torque given the inverse time step.
    /// Unit is N*m. This is always zero for a distance joint.
    float GetReactionTorque(float inv_dt) const override;

    /// The local anchor point relative to bodyA's origin.
    const b2Vec2 &GetLocalAnchorA() const { return m_localAnchorA; }

    /// The local anchor point relative to bodyB's origin.
    const b2Vec2 &GetLocalAnchorB() const { return m_localAnchorB; }

    /// Get the rest length
    float GetLength() const { return m_length; }

    /// Set the rest length
    /// @returns clamped rest length
    float SetLength(float length);

    /// Get the minimum length
    float GetMinLength() const { return m_minLength; }

    /// Set the minimum length
    /// @returns the clamped minimum length
    float SetMinLength(float minLength);

    /// Get the maximum length
    float GetMaxLength() const { return m_maxLength; }

    /// Set the maximum length
    /// @returns the clamped maximum length
    float SetMaxLength(float maxLength);

    /// Get the current length
    float GetCurrentLength() const;

    /// Set/get the linear stiffness in N/m
    void SetStiffness(float stiffness) { m_stiffness = stiffness; }
    float GetStiffness() const { return m_stiffness; }

    /// Set/get linear damping in N*s/m
    void SetDamping(float damping) { m_damping = damping; }
    float GetDamping() const { return m_damping; }

    /// Dump joint to dmLog
    void Dump() override;

    ///
    void Draw(b2Draw *draw) const override;

protected:
    friend class b2Joint;
    b2DistanceJoint(const b2DistanceJointDef *data);

    void InitVelocityConstraints(const b2SolverData &data) override;
    void SolveVelocityConstraints(const b2SolverData &data) override;
    bool SolvePositionConstraints(const b2SolverData &data) override;

    float m_stiffness;
    float m_damping;
    float m_bias;
    float m_length;
    float m_minLength;
    float m_maxLength;

    // Solver shared
    b2Vec2 m_localAnchorA;
    b2Vec2 m_localAnchorB;
    float m_gamma;
    float m_impulse;
    float m_lowerImpulse;
    float m_upperImpulse;

    // Solver temp
    int32 m_indexA;
    int32 m_indexB;
    b2Vec2 m_u;
    b2Vec2 m_rA;
    b2Vec2 m_rB;
    b2Vec2 m_localCenterA;
    b2Vec2 m_localCenterB;
    float m_currentLength;
    float m_invMassA;
    float m_invMassB;
    float m_invIA;
    float m_invIB;
    float m_softMass;
    float m_mass;
};

#pragma endregion

#pragma region

/// Friction joint definition.
struct b2FrictionJointDef : public b2JointDef
{
    b2FrictionJointDef() {
        type = e_frictionJoint;
        localAnchorA.SetZero();
        localAnchorB.SetZero();
        maxForce = 0.0f;
        maxTorque = 0.0f;
    }

    /// Initialize the bodies, anchors, axis, and reference angle using the world
    /// anchor and world axis.
    void Initialize(b2Body *bodyA, b2Body *bodyB, const b2Vec2 &anchor);

    /// The local anchor point relative to bodyA's origin.
    b2Vec2 localAnchorA;

    /// The local anchor point relative to bodyB's origin.
    b2Vec2 localAnchorB;

    /// The maximum friction force in N.
    float maxForce;

    /// The maximum friction torque in N-m.
    float maxTorque;
};

/// Friction joint. This is used for top-down friction.
/// It provides 2D translational friction and angular friction.
class b2FrictionJoint : public b2Joint {
public:
    b2Vec2 GetAnchorA() const override;
    b2Vec2 GetAnchorB() const override;

    b2Vec2 GetReactionForce(float inv_dt) const override;
    float GetReactionTorque(float inv_dt) const override;

    /// The local anchor point relative to bodyA's origin.
    const b2Vec2 &GetLocalAnchorA() const { return m_localAnchorA; }

    /// The local anchor point relative to bodyB's origin.
    const b2Vec2 &GetLocalAnchorB() const { return m_localAnchorB; }

    /// Set the maximum friction force in N.
    void SetMaxForce(float force);

    /// Get the maximum friction force in N.
    float GetMaxForce() const;

    /// Set the maximum friction torque in N*m.
    void SetMaxTorque(float torque);

    /// Get the maximum friction torque in N*m.
    float GetMaxTorque() const;

    /// Dump joint to dmLog
    void Dump() override;

protected:
    friend class b2Joint;

    b2FrictionJoint(const b2FrictionJointDef *def);

    void InitVelocityConstraints(const b2SolverData &data) override;
    void SolveVelocityConstraints(const b2SolverData &data) override;
    bool SolvePositionConstraints(const b2SolverData &data) override;

    b2Vec2 m_localAnchorA;
    b2Vec2 m_localAnchorB;

    // Solver shared
    b2Vec2 m_linearImpulse;
    float m_angularImpulse;
    float m_maxForce;
    float m_maxTorque;

    // Solver temp
    int32 m_indexA;
    int32 m_indexB;
    b2Vec2 m_rA;
    b2Vec2 m_rB;
    b2Vec2 m_localCenterA;
    b2Vec2 m_localCenterB;
    float m_invMassA;
    float m_invMassB;
    float m_invIA;
    float m_invIB;
    b2Mat22 m_linearMass;
    float m_angularMass;
};

#pragma endregion

#pragma region

/// Gear joint definition. This definition requires two existing
/// revolute or prismatic joints (any combination will work).
/// @warning bodyB on the input joints must both be dynamic
struct b2GearJointDef : public b2JointDef
{
    b2GearJointDef() {
        type = e_gearJoint;
        joint1 = nullptr;
        joint2 = nullptr;
        ratio = 1.0f;
    }

    /// The first revolute/prismatic joint attached to the gear joint.
    b2Joint *joint1;

    /// The second revolute/prismatic joint attached to the gear joint.
    b2Joint *joint2;

    /// The gear ratio.
    /// @see b2GearJoint for explanation.
    float ratio;
};

/// A gear joint is used to connect two joints together. Either joint
/// can be a revolute or prismatic joint. You specify a gear ratio
/// to bind the motions together:
/// coordinate1 + ratio * coordinate2 = constant
/// The ratio can be negative or positive. If one joint is a revolute joint
/// and the other joint is a prismatic joint, then the ratio will have units
/// of length or units of 1/length.
/// @warning You have to manually destroy the gear joint if joint1 or joint2
/// is destroyed.
class b2GearJoint : public b2Joint {
public:
    b2Vec2 GetAnchorA() const override;
    b2Vec2 GetAnchorB() const override;

    b2Vec2 GetReactionForce(float inv_dt) const override;
    float GetReactionTorque(float inv_dt) const override;

    /// Get the first joint.
    b2Joint *GetJoint1() { return m_joint1; }

    /// Get the second joint.
    b2Joint *GetJoint2() { return m_joint2; }

    /// Set/Get the gear ratio.
    void SetRatio(float ratio);
    float GetRatio() const;

    /// Dump joint to dmLog
    void Dump() override;

protected:
    friend class b2Joint;
    b2GearJoint(const b2GearJointDef *data);

    void InitVelocityConstraints(const b2SolverData &data) override;
    void SolveVelocityConstraints(const b2SolverData &data) override;
    bool SolvePositionConstraints(const b2SolverData &data) override;

    b2Joint *m_joint1;
    b2Joint *m_joint2;

    b2JointType m_typeA;
    b2JointType m_typeB;

    // Body A is connected to body C
    // Body B is connected to body D
    b2Body *m_bodyC;
    b2Body *m_bodyD;

    // Solver shared
    b2Vec2 m_localAnchorA;
    b2Vec2 m_localAnchorB;
    b2Vec2 m_localAnchorC;
    b2Vec2 m_localAnchorD;

    b2Vec2 m_localAxisC;
    b2Vec2 m_localAxisD;

    float m_referenceAngleA;
    float m_referenceAngleB;

    float m_constant;
    float m_ratio;
    float m_tolerance;

    float m_impulse;

    // Solver temp
    int32 m_indexA, m_indexB, m_indexC, m_indexD;
    b2Vec2 m_lcA, m_lcB, m_lcC, m_lcD;
    float m_mA, m_mB, m_mC, m_mD;
    float m_iA, m_iB, m_iC, m_iD;
    b2Vec2 m_JvAC, m_JvBD;
    float m_JwA, m_JwB, m_JwC, m_JwD;
    float m_mass;
};

#pragma endregion

#pragma region

/// Motor joint definition.
struct b2MotorJointDef : public b2JointDef
{
    b2MotorJointDef() {
        type = e_motorJoint;
        linearOffset.SetZero();
        angularOffset = 0.0f;
        maxForce = 1.0f;
        maxTorque = 1.0f;
        correctionFactor = 0.3f;
    }

    /// Initialize the bodies and offsets using the current transforms.
    void Initialize(b2Body *bodyA, b2Body *bodyB);

    /// Position of bodyB minus the position of bodyA, in bodyA's frame, in meters.
    b2Vec2 linearOffset;

    /// The bodyB angle minus bodyA angle in radians.
    float angularOffset;

    /// The maximum motor force in N.
    float maxForce;

    /// The maximum motor torque in N-m.
    float maxTorque;

    /// Position correction factor in the range [0,1].
    float correctionFactor;
};

/// A motor joint is used to control the relative motion
/// between two bodies. A typical usage is to control the movement
/// of a dynamic body with respect to the ground.
class b2MotorJoint : public b2Joint {
public:
    b2Vec2 GetAnchorA() const override;
    b2Vec2 GetAnchorB() const override;

    b2Vec2 GetReactionForce(float inv_dt) const override;
    float GetReactionTorque(float inv_dt) const override;

    /// Set/get the target linear offset, in frame A, in meters.
    void SetLinearOffset(const b2Vec2 &linearOffset);
    const b2Vec2 &GetLinearOffset() const;

    /// Set/get the target angular offset, in radians.
    void SetAngularOffset(float angularOffset);
    float GetAngularOffset() const;

    /// Set the maximum friction force in N.
    void SetMaxForce(float force);

    /// Get the maximum friction force in N.
    float GetMaxForce() const;

    /// Set the maximum friction torque in N*m.
    void SetMaxTorque(float torque);

    /// Get the maximum friction torque in N*m.
    float GetMaxTorque() const;

    /// Set the position correction factor in the range [0,1].
    void SetCorrectionFactor(float factor);

    /// Get the position correction factor in the range [0,1].
    float GetCorrectionFactor() const;

    /// Dump to b2Log
    void Dump() override;

protected:
    friend class b2Joint;

    b2MotorJoint(const b2MotorJointDef *def);

    void InitVelocityConstraints(const b2SolverData &data) override;
    void SolveVelocityConstraints(const b2SolverData &data) override;
    bool SolvePositionConstraints(const b2SolverData &data) override;

    // Solver shared
    b2Vec2 m_linearOffset;
    float m_angularOffset;
    b2Vec2 m_linearImpulse;
    float m_angularImpulse;
    float m_maxForce;
    float m_maxTorque;
    float m_correctionFactor;

    // Solver temp
    int32 m_indexA;
    int32 m_indexB;
    b2Vec2 m_rA;
    b2Vec2 m_rB;
    b2Vec2 m_localCenterA;
    b2Vec2 m_localCenterB;
    b2Vec2 m_linearError;
    float m_angularError;
    float m_invMassA;
    float m_invMassB;
    float m_invIA;
    float m_invIB;
    b2Mat22 m_linearMass;
    float m_angularMass;
};

#pragma endregion

#pragma region

/// Mouse joint definition. This requires a world target point,
/// tuning parameters, and the time step.
struct b2MouseJointDef : public b2JointDef
{
    b2MouseJointDef() {
        type = e_mouseJoint;
        target.Set(0.0f, 0.0f);
        maxForce = 0.0f;
        stiffness = 0.0f;
        damping = 0.0f;
    }

    /// The initial world target point. This is assumed
    /// to coincide with the body anchor initially.
    b2Vec2 target;

    /// The maximum constraint force that can be exerted
    /// to move the candidate body. Usually you will express
    /// as some multiple of the weight (multiplier * mass * gravity).
    float maxForce;

    /// The linear stiffness in N/m
    float stiffness;

    /// The linear damping in N*s/m
    float damping;
};

/// A mouse joint is used to make a point on a body track a
/// specified world point. This a soft constraint with a maximum
/// force. This allows the constraint to stretch and without
/// applying huge forces.
/// NOTE: this joint is not documented in the manual because it was
/// developed to be used in the testbed. If you want to learn how to
/// use the mouse joint, look at the testbed.
class b2MouseJoint : public b2Joint {
public:
    /// Implements b2Joint.
    b2Vec2 GetAnchorA() const override;

    /// Implements b2Joint.
    b2Vec2 GetAnchorB() const override;

    /// Implements b2Joint.
    b2Vec2 GetReactionForce(float inv_dt) const override;

    /// Implements b2Joint.
    float GetReactionTorque(float inv_dt) const override;

    /// Use this to update the target point.
    void SetTarget(const b2Vec2 &target);
    const b2Vec2 &GetTarget() const;

    /// Set/get the maximum force in Newtons.
    void SetMaxForce(float force);
    float GetMaxForce() const;

    /// Set/get the linear stiffness in N/m
    void SetStiffness(float stiffness) { m_stiffness = stiffness; }
    float GetStiffness() const { return m_stiffness; }

    /// Set/get linear damping in N*s/m
    void SetDamping(float damping) { m_damping = damping; }
    float GetDamping() const { return m_damping; }

    /// The mouse joint does not support dumping.
    void Dump() override { b2Log("Mouse joint dumping is not supported.\n"); }

    /// Implement b2Joint::ShiftOrigin
    void ShiftOrigin(const b2Vec2 &newOrigin) override;

protected:
    friend class b2Joint;

    b2MouseJoint(const b2MouseJointDef *def);

    void InitVelocityConstraints(const b2SolverData &data) override;
    void SolveVelocityConstraints(const b2SolverData &data) override;
    bool SolvePositionConstraints(const b2SolverData &data) override;

    b2Vec2 m_localAnchorB;
    b2Vec2 m_targetA;
    float m_stiffness;
    float m_damping;
    float m_beta;

    // Solver shared
    b2Vec2 m_impulse;
    float m_maxForce;
    float m_gamma;

    // Solver temp
    int32 m_indexA;
    int32 m_indexB;
    b2Vec2 m_rB;
    b2Vec2 m_localCenterB;
    float m_invMassB;
    float m_invIB;
    b2Mat22 m_mass;
    b2Vec2 m_C;
};

#pragma endregion

#pragma region

/// Prismatic joint definition. This requires defining a line of
/// motion using an axis and an anchor point. The definition uses local
/// anchor points and a local axis so that the initial configuration
/// can violate the constraint slightly. The joint translation is zero
/// when the local anchor points coincide in world space. Using local
/// anchors and a local axis helps when saving and loading a game.
struct b2PrismaticJointDef : public b2JointDef
{
    b2PrismaticJointDef() {
        type = e_prismaticJoint;
        localAnchorA.SetZero();
        localAnchorB.SetZero();
        localAxisA.Set(1.0f, 0.0f);
        referenceAngle = 0.0f;
        enableLimit = false;
        lowerTranslation = 0.0f;
        upperTranslation = 0.0f;
        enableMotor = false;
        maxMotorForce = 0.0f;
        motorSpeed = 0.0f;
    }

    /// Initialize the bodies, anchors, axis, and reference angle using the world
    /// anchor and unit world axis.
    void Initialize(b2Body *bodyA, b2Body *bodyB, const b2Vec2 &anchor, const b2Vec2 &axis);

    /// The local anchor point relative to bodyA's origin.
    b2Vec2 localAnchorA;

    /// The local anchor point relative to bodyB's origin.
    b2Vec2 localAnchorB;

    /// The local translation unit axis in bodyA.
    b2Vec2 localAxisA;

    /// The constrained angle between the bodies: bodyB_angle - bodyA_angle.
    float referenceAngle;

    /// Enable/disable the joint limit.
    bool enableLimit;

    /// The lower translation limit, usually in meters.
    float lowerTranslation;

    /// The upper translation limit, usually in meters.
    float upperTranslation;

    /// Enable/disable the joint motor.
    bool enableMotor;

    /// The maximum motor torque, usually in N-m.
    float maxMotorForce;

    /// The desired motor speed in radians per second.
    float motorSpeed;
};

/// A prismatic joint. This joint provides one degree of freedom: translation
/// along an axis fixed in bodyA. Relative rotation is prevented. You can
/// use a joint limit to restrict the range of motion and a joint motor to
/// drive the motion or to model joint friction.
class b2PrismaticJoint : public b2Joint {
public:
    b2Vec2 GetAnchorA() const override;
    b2Vec2 GetAnchorB() const override;

    b2Vec2 GetReactionForce(float inv_dt) const override;
    float GetReactionTorque(float inv_dt) const override;

    /// The local anchor point relative to bodyA's origin.
    const b2Vec2 &GetLocalAnchorA() const { return m_localAnchorA; }

    /// The local anchor point relative to bodyB's origin.
    const b2Vec2 &GetLocalAnchorB() const { return m_localAnchorB; }

    /// The local joint axis relative to bodyA.
    const b2Vec2 &GetLocalAxisA() const { return m_localXAxisA; }

    /// Get the reference angle.
    float GetReferenceAngle() const { return m_referenceAngle; }

    /// Get the current joint translation, usually in meters.
    float GetJointTranslation() const;

    /// Get the current joint translation speed, usually in meters per second.
    float GetJointSpeed() const;

    /// Is the joint limit enabled?
    bool IsLimitEnabled() const;

    /// Enable/disable the joint limit.
    void EnableLimit(bool flag);

    /// Get the lower joint limit, usually in meters.
    float GetLowerLimit() const;

    /// Get the upper joint limit, usually in meters.
    float GetUpperLimit() const;

    /// Set the joint limits, usually in meters.
    void SetLimits(float lower, float upper);

    /// Is the joint motor enabled?
    bool IsMotorEnabled() const;

    /// Enable/disable the joint motor.
    void EnableMotor(bool flag);

    /// Set the motor speed, usually in meters per second.
    void SetMotorSpeed(float speed);

    /// Get the motor speed, usually in meters per second.
    float GetMotorSpeed() const;

    /// Set the maximum motor force, usually in N.
    void SetMaxMotorForce(float force);
    float GetMaxMotorForce() const { return m_maxMotorForce; }

    /// Get the current motor force given the inverse time step, usually in N.
    float GetMotorForce(float inv_dt) const;

    /// Dump to b2Log
    void Dump() override;

    ///
    void Draw(b2Draw *draw) const override;

protected:
    friend class b2Joint;
    friend class b2GearJoint;
    b2PrismaticJoint(const b2PrismaticJointDef *def);

    void InitVelocityConstraints(const b2SolverData &data) override;
    void SolveVelocityConstraints(const b2SolverData &data) override;
    bool SolvePositionConstraints(const b2SolverData &data) override;

    b2Vec2 m_localAnchorA;
    b2Vec2 m_localAnchorB;
    b2Vec2 m_localXAxisA;
    b2Vec2 m_localYAxisA;
    float m_referenceAngle;
    b2Vec2 m_impulse;
    float m_motorImpulse;
    float m_lowerImpulse;
    float m_upperImpulse;
    float m_lowerTranslation;
    float m_upperTranslation;
    float m_maxMotorForce;
    float m_motorSpeed;
    bool m_enableLimit;
    bool m_enableMotor;

    // Solver temp
    int32 m_indexA;
    int32 m_indexB;
    b2Vec2 m_localCenterA;
    b2Vec2 m_localCenterB;
    float m_invMassA;
    float m_invMassB;
    float m_invIA;
    float m_invIB;
    b2Vec2 m_axis, m_perp;
    float m_s1, m_s2;
    float m_a1, m_a2;
    b2Mat22 m_K;
    float m_translation;
    float m_axialMass;
};

inline float b2PrismaticJoint::GetMotorSpeed() const { return m_motorSpeed; }

#pragma endregion

#pragma region

const float b2_minPulleyLength = 2.0f;

/// Pulley joint definition. This requires two ground anchors,
/// two dynamic body anchor points, and a pulley ratio.
struct b2PulleyJointDef : public b2JointDef
{
    b2PulleyJointDef() {
        type = e_pulleyJoint;
        groundAnchorA.Set(-1.0f, 1.0f);
        groundAnchorB.Set(1.0f, 1.0f);
        localAnchorA.Set(-1.0f, 0.0f);
        localAnchorB.Set(1.0f, 0.0f);
        lengthA = 0.0f;
        lengthB = 0.0f;
        ratio = 1.0f;
        collideConnected = true;
    }

    /// Initialize the bodies, anchors, lengths, max lengths, and ratio using the world anchors.
    void Initialize(b2Body *bodyA, b2Body *bodyB, const b2Vec2 &groundAnchorA,
                    const b2Vec2 &groundAnchorB, const b2Vec2 &anchorA, const b2Vec2 &anchorB,
                    float ratio);

    /// The first ground anchor in world coordinates. This point never moves.
    b2Vec2 groundAnchorA;

    /// The second ground anchor in world coordinates. This point never moves.
    b2Vec2 groundAnchorB;

    /// The local anchor point relative to bodyA's origin.
    b2Vec2 localAnchorA;

    /// The local anchor point relative to bodyB's origin.
    b2Vec2 localAnchorB;

    /// The a reference length for the segment attached to bodyA.
    float lengthA;

    /// The a reference length for the segment attached to bodyB.
    float lengthB;

    /// The pulley ratio, used to simulate a block-and-tackle.
    float ratio;
};

/// The pulley joint is connected to two bodies and two fixed ground points.
/// The pulley supports a ratio such that:
/// length1 + ratio * length2 <= constant
/// Yes, the force transmitted is scaled by the ratio.
/// Warning: the pulley joint can get a bit squirrelly by itself. They often
/// work better when combined with prismatic joints. You should also cover the
/// the anchor points with static shapes to prevent one side from going to
/// zero length.
class b2PulleyJoint : public b2Joint {
public:
    b2Vec2 GetAnchorA() const override;
    b2Vec2 GetAnchorB() const override;

    b2Vec2 GetReactionForce(float inv_dt) const override;
    float GetReactionTorque(float inv_dt) const override;

    /// Get the first ground anchor.
    b2Vec2 GetGroundAnchorA() const;

    /// Get the second ground anchor.
    b2Vec2 GetGroundAnchorB() const;

    /// Get the current length of the segment attached to bodyA.
    float GetLengthA() const;

    /// Get the current length of the segment attached to bodyB.
    float GetLengthB() const;

    /// Get the pulley ratio.
    float GetRatio() const;

    /// Get the current length of the segment attached to bodyA.
    float GetCurrentLengthA() const;

    /// Get the current length of the segment attached to bodyB.
    float GetCurrentLengthB() const;

    /// Dump joint to dmLog
    void Dump() override;

    /// Implement b2Joint::ShiftOrigin
    void ShiftOrigin(const b2Vec2 &newOrigin) override;

protected:
    friend class b2Joint;
    b2PulleyJoint(const b2PulleyJointDef *data);

    void InitVelocityConstraints(const b2SolverData &data) override;
    void SolveVelocityConstraints(const b2SolverData &data) override;
    bool SolvePositionConstraints(const b2SolverData &data) override;

    b2Vec2 m_groundAnchorA;
    b2Vec2 m_groundAnchorB;
    float m_lengthA;
    float m_lengthB;

    // Solver shared
    b2Vec2 m_localAnchorA;
    b2Vec2 m_localAnchorB;
    float m_constant;
    float m_ratio;
    float m_impulse;

    // Solver temp
    int32 m_indexA;
    int32 m_indexB;
    b2Vec2 m_uA;
    b2Vec2 m_uB;
    b2Vec2 m_rA;
    b2Vec2 m_rB;
    b2Vec2 m_localCenterA;
    b2Vec2 m_localCenterB;
    float m_invMassA;
    float m_invMassB;
    float m_invIA;
    float m_invIB;
    float m_mass;
};

#pragma endregion

#pragma region

/// Revolute joint definition. This requires defining an anchor point where the
/// bodies are joined. The definition uses local anchor points so that the
/// initial configuration can violate the constraint slightly. You also need to
/// specify the initial relative angle for joint limits. This helps when saving
/// and loading a game.
/// The local anchor points are measured from the body's origin
/// rather than the center of mass because:
/// 1. you might not know where the center of mass will be.
/// 2. if you add/remove shapes from a body and recompute the mass,
///    the joints will be broken.
struct b2RevoluteJointDef : public b2JointDef
{
    b2RevoluteJointDef() {
        type = e_revoluteJoint;
        localAnchorA.Set(0.0f, 0.0f);
        localAnchorB.Set(0.0f, 0.0f);
        referenceAngle = 0.0f;
        lowerAngle = 0.0f;
        upperAngle = 0.0f;
        maxMotorTorque = 0.0f;
        motorSpeed = 0.0f;
        enableLimit = false;
        enableMotor = false;
    }

    /// Initialize the bodies, anchors, and reference angle using a world
    /// anchor point.
    void Initialize(b2Body *bodyA, b2Body *bodyB, const b2Vec2 &anchor);

    /// The local anchor point relative to bodyA's origin.
    b2Vec2 localAnchorA;

    /// The local anchor point relative to bodyB's origin.
    b2Vec2 localAnchorB;

    /// The bodyB angle minus bodyA angle in the reference state (radians).
    float referenceAngle;

    /// A flag to enable joint limits.
    bool enableLimit;

    /// The lower angle for the joint limit (radians).
    float lowerAngle;

    /// The upper angle for the joint limit (radians).
    float upperAngle;

    /// A flag to enable the joint motor.
    bool enableMotor;

    /// The desired motor speed. Usually in radians per second.
    float motorSpeed;

    /// The maximum motor torque used to achieve the desired motor speed.
    /// Usually in N-m.
    float maxMotorTorque;
};

/// A revolute joint constrains two bodies to share a common point while they
/// are free to rotate about the point. The relative rotation about the shared
/// point is the joint angle. You can limit the relative rotation with
/// a joint limit that specifies a lower and upper angle. You can use a motor
/// to drive the relative rotation about the shared point. A maximum motor torque
/// is provided so that infinite forces are not generated.
class b2RevoluteJoint : public b2Joint {
public:
    b2Vec2 GetAnchorA() const override;
    b2Vec2 GetAnchorB() const override;

    /// The local anchor point relative to bodyA's origin.
    const b2Vec2 &GetLocalAnchorA() const { return m_localAnchorA; }

    /// The local anchor point relative to bodyB's origin.
    const b2Vec2 &GetLocalAnchorB() const { return m_localAnchorB; }

    /// Get the reference angle.
    float GetReferenceAngle() const { return m_referenceAngle; }

    /// Get the current joint angle in radians.
    float GetJointAngle() const;

    /// Get the current joint angle speed in radians per second.
    float GetJointSpeed() const;

    /// Is the joint limit enabled?
    bool IsLimitEnabled() const;

    /// Enable/disable the joint limit.
    void EnableLimit(bool flag);

    /// Get the lower joint limit in radians.
    float GetLowerLimit() const;

    /// Get the upper joint limit in radians.
    float GetUpperLimit() const;

    /// Set the joint limits in radians.
    void SetLimits(float lower, float upper);

    /// Is the joint motor enabled?
    bool IsMotorEnabled() const;

    /// Enable/disable the joint motor.
    void EnableMotor(bool flag);

    /// Set the motor speed in radians per second.
    void SetMotorSpeed(float speed);

    /// Get the motor speed in radians per second.
    float GetMotorSpeed() const;

    /// Set the maximum motor torque, usually in N-m.
    void SetMaxMotorTorque(float torque);
    float GetMaxMotorTorque() const { return m_maxMotorTorque; }

    /// Get the reaction force given the inverse time step.
    /// Unit is N.
    b2Vec2 GetReactionForce(float inv_dt) const override;

    /// Get the reaction torque due to the joint limit given the inverse time step.
    /// Unit is N*m.
    float GetReactionTorque(float inv_dt) const override;

    /// Get the current motor torque given the inverse time step.
    /// Unit is N*m.
    float GetMotorTorque(float inv_dt) const;

    /// Dump to b2Log.
    void Dump() override;

    ///
    void Draw(b2Draw *draw) const override;

protected:
    friend class b2Joint;
    friend class b2GearJoint;

    b2RevoluteJoint(const b2RevoluteJointDef *def);

    void InitVelocityConstraints(const b2SolverData &data) override;
    void SolveVelocityConstraints(const b2SolverData &data) override;
    bool SolvePositionConstraints(const b2SolverData &data) override;

    // Solver shared
    b2Vec2 m_localAnchorA;
    b2Vec2 m_localAnchorB;
    b2Vec2 m_impulse;
    float m_motorImpulse;
    float m_lowerImpulse;
    float m_upperImpulse;
    bool m_enableMotor;
    float m_maxMotorTorque;
    float m_motorSpeed;
    bool m_enableLimit;
    float m_referenceAngle;
    float m_lowerAngle;
    float m_upperAngle;

    // Solver temp
    int32 m_indexA;
    int32 m_indexB;
    b2Vec2 m_rA;
    b2Vec2 m_rB;
    b2Vec2 m_localCenterA;
    b2Vec2 m_localCenterB;
    float m_invMassA;
    float m_invMassB;
    float m_invIA;
    float m_invIB;
    b2Mat22 m_K;
    float m_angle;
    float m_axialMass;
};

inline float b2RevoluteJoint::GetMotorSpeed() const { return m_motorSpeed; }

#pragma endregion

#pragma region

/// Weld joint definition. You need to specify local anchor points
/// where they are attached and the relative body angle. The position
/// of the anchor points is important for computing the reaction torque.
struct b2WeldJointDef : public b2JointDef
{
    b2WeldJointDef() {
        type = e_weldJoint;
        localAnchorA.Set(0.0f, 0.0f);
        localAnchorB.Set(0.0f, 0.0f);
        referenceAngle = 0.0f;
        stiffness = 0.0f;
        damping = 0.0f;
    }

    /// Initialize the bodies, anchors, reference angle, stiffness, and damping.
    /// @param bodyA the first body connected by this joint
    /// @param bodyB the second body connected by this joint
    /// @param anchor the point of connection in world coordinates
    void Initialize(b2Body *bodyA, b2Body *bodyB, const b2Vec2 &anchor);

    /// The local anchor point relative to bodyA's origin.
    b2Vec2 localAnchorA;

    /// The local anchor point relative to bodyB's origin.
    b2Vec2 localAnchorB;

    /// The bodyB angle minus bodyA angle in the reference state (radians).
    float referenceAngle;

    /// The rotational stiffness in N*m
    /// Disable softness with a value of 0
    float stiffness;

    /// The rotational damping in N*m*s
    float damping;
};

/// A weld joint essentially glues two bodies together. A weld joint may
/// distort somewhat because the island constraint solver is approximate.
class b2WeldJoint : public b2Joint {
public:
    b2Vec2 GetAnchorA() const override;
    b2Vec2 GetAnchorB() const override;

    b2Vec2 GetReactionForce(float inv_dt) const override;
    float GetReactionTorque(float inv_dt) const override;

    /// The local anchor point relative to bodyA's origin.
    const b2Vec2 &GetLocalAnchorA() const { return m_localAnchorA; }

    /// The local anchor point relative to bodyB's origin.
    const b2Vec2 &GetLocalAnchorB() const { return m_localAnchorB; }

    /// Get the reference angle.
    float GetReferenceAngle() const { return m_referenceAngle; }

    /// Set/get stiffness in N*m
    void SetStiffness(float stiffness) { m_stiffness = stiffness; }
    float GetStiffness() const { return m_stiffness; }

    /// Set/get damping in N*m*s
    void SetDamping(float damping) { m_damping = damping; }
    float GetDamping() const { return m_damping; }

    /// Dump to b2Log
    void Dump() override;

protected:
    friend class b2Joint;

    b2WeldJoint(const b2WeldJointDef *def);

    void InitVelocityConstraints(const b2SolverData &data) override;
    void SolveVelocityConstraints(const b2SolverData &data) override;
    bool SolvePositionConstraints(const b2SolverData &data) override;

    float m_stiffness;
    float m_damping;
    float m_bias;

    // Solver shared
    b2Vec2 m_localAnchorA;
    b2Vec2 m_localAnchorB;
    float m_referenceAngle;
    float m_gamma;
    b2Vec3 m_impulse;

    // Solver temp
    int32 m_indexA;
    int32 m_indexB;
    b2Vec2 m_rA;
    b2Vec2 m_rB;
    b2Vec2 m_localCenterA;
    b2Vec2 m_localCenterB;
    float m_invMassA;
    float m_invMassB;
    float m_invIA;
    float m_invIB;
    b2Mat33 m_mass;
};

#pragma endregion

#pragma region

/// Wheel joint definition. This requires defining a line of
/// motion using an axis and an anchor point. The definition uses local
/// anchor points and a local axis so that the initial configuration
/// can violate the constraint slightly. The joint translation is zero
/// when the local anchor points coincide in world space. Using local
/// anchors and a local axis helps when saving and loading a game.
struct b2WheelJointDef : public b2JointDef
{
    b2WheelJointDef() {
        type = e_wheelJoint;
        localAnchorA.SetZero();
        localAnchorB.SetZero();
        localAxisA.Set(1.0f, 0.0f);
        enableLimit = false;
        lowerTranslation = 0.0f;
        upperTranslation = 0.0f;
        enableMotor = false;
        maxMotorTorque = 0.0f;
        motorSpeed = 0.0f;
        stiffness = 0.0f;
        damping = 0.0f;
    }

    /// Initialize the bodies, anchors, axis, and reference angle using the world
    /// anchor and world axis.
    void Initialize(b2Body *bodyA, b2Body *bodyB, const b2Vec2 &anchor, const b2Vec2 &axis);

    /// The local anchor point relative to bodyA's origin.
    b2Vec2 localAnchorA;

    /// The local anchor point relative to bodyB's origin.
    b2Vec2 localAnchorB;

    /// The local translation axis in bodyA.
    b2Vec2 localAxisA;

    /// Enable/disable the joint limit.
    bool enableLimit;

    /// The lower translation limit, usually in meters.
    float lowerTranslation;

    /// The upper translation limit, usually in meters.
    float upperTranslation;

    /// Enable/disable the joint motor.
    bool enableMotor;

    /// The maximum motor torque, usually in N-m.
    float maxMotorTorque;

    /// The desired motor speed in radians per second.
    float motorSpeed;

    /// Suspension stiffness. Typically in units N/m.
    float stiffness;

    /// Suspension damping. Typically in units of N*s/m.
    float damping;
};

/// A wheel joint. This joint provides two degrees of freedom: translation
/// along an axis fixed in bodyA and rotation in the plane. In other words, it is a point to
/// line constraint with a rotational motor and a linear spring/damper. The spring/damper is
/// initialized upon creation. This joint is designed for vehicle suspensions.
class b2WheelJoint : public b2Joint {
public:
    b2Vec2 GetAnchorA() const override;
    b2Vec2 GetAnchorB() const override;

    b2Vec2 GetReactionForce(float inv_dt) const override;
    float GetReactionTorque(float inv_dt) const override;

    /// The local anchor point relative to bodyA's origin.
    const b2Vec2 &GetLocalAnchorA() const { return m_localAnchorA; }

    /// The local anchor point relative to bodyB's origin.
    const b2Vec2 &GetLocalAnchorB() const { return m_localAnchorB; }

    /// The local joint axis relative to bodyA.
    const b2Vec2 &GetLocalAxisA() const { return m_localXAxisA; }

    /// Get the current joint translation, usually in meters.
    float GetJointTranslation() const;

    /// Get the current joint linear speed, usually in meters per second.
    float GetJointLinearSpeed() const;

    /// Get the current joint angle in radians.
    float GetJointAngle() const;

    /// Get the current joint angular speed in radians per second.
    float GetJointAngularSpeed() const;

    /// Is the joint limit enabled?
    bool IsLimitEnabled() const;

    /// Enable/disable the joint translation limit.
    void EnableLimit(bool flag);

    /// Get the lower joint translation limit, usually in meters.
    float GetLowerLimit() const;

    /// Get the upper joint translation limit, usually in meters.
    float GetUpperLimit() const;

    /// Set the joint translation limits, usually in meters.
    void SetLimits(float lower, float upper);

    /// Is the joint motor enabled?
    bool IsMotorEnabled() const;

    /// Enable/disable the joint motor.
    void EnableMotor(bool flag);

    /// Set the motor speed, usually in radians per second.
    void SetMotorSpeed(float speed);

    /// Get the motor speed, usually in radians per second.
    float GetMotorSpeed() const;

    /// Set/Get the maximum motor force, usually in N-m.
    void SetMaxMotorTorque(float torque);
    float GetMaxMotorTorque() const;

    /// Get the current motor torque given the inverse time step, usually in N-m.
    float GetMotorTorque(float inv_dt) const;

    /// Access spring stiffness
    void SetStiffness(float stiffness);
    float GetStiffness() const;

    /// Access damping
    void SetDamping(float damping);
    float GetDamping() const;

    /// Dump to b2Log
    void Dump() override;

    ///
    void Draw(b2Draw *draw) const override;

protected:
    friend class b2Joint;
    b2WheelJoint(const b2WheelJointDef *def);

    void InitVelocityConstraints(const b2SolverData &data) override;
    void SolveVelocityConstraints(const b2SolverData &data) override;
    bool SolvePositionConstraints(const b2SolverData &data) override;

    b2Vec2 m_localAnchorA;
    b2Vec2 m_localAnchorB;
    b2Vec2 m_localXAxisA;
    b2Vec2 m_localYAxisA;

    float m_impulse;
    float m_motorImpulse;
    float m_springImpulse;

    float m_lowerImpulse;
    float m_upperImpulse;
    float m_translation;
    float m_lowerTranslation;
    float m_upperTranslation;

    float m_maxMotorTorque;
    float m_motorSpeed;

    bool m_enableLimit;
    bool m_enableMotor;

    float m_stiffness;
    float m_damping;

    // Solver temp
    int32 m_indexA;
    int32 m_indexB;
    b2Vec2 m_localCenterA;
    b2Vec2 m_localCenterB;
    float m_invMassA;
    float m_invMassB;
    float m_invIA;
    float m_invIB;

    b2Vec2 m_ax, m_ay;
    float m_sAx, m_sBx;
    float m_sAy, m_sBy;

    float m_mass;
    float m_motorMass;
    float m_axialMass;
    float m_springMass;

    float m_bias;
    float m_gamma;
};

inline float b2WheelJoint::GetMotorSpeed() const { return m_motorSpeed; }

inline float b2WheelJoint::GetMaxMotorTorque() const { return m_maxMotorTorque; }

#pragma endregion

#pragma region

class b2EdgeShape;

/// A chain shape is a free form sequence of line segments.
/// The chain has one-sided collision, with the surface normal pointing to the right of the edge.
/// This provides a counter-clockwise winding like the polygon shape.
/// Connectivity information is used to create smooth collisions.
/// @warning the chain will not collide properly if there are self-intersections.
class b2ChainShape : public b2Shape {
public:
    b2ChainShape();

    /// The destructor frees the vertices using b2Free.
    ~b2ChainShape();

    /// Clear all data.
    void Clear();

    /// Create a loop. This automatically adjusts connectivity.
    /// @param vertices an array of vertices, these are copied
    /// @param count the vertex count
    void CreateLoop(const b2Vec2 *vertices, int32 count);

    /// Create a chain with ghost vertices to connect multiple chains together.
    /// @param vertices an array of vertices, these are copied
    /// @param count the vertex count
    /// @param prevVertex previous vertex from chain that connects to the start
    /// @param nextVertex next vertex from chain that connects to the end
    void CreateChain(const b2Vec2 *vertices, int32 count, const b2Vec2 &prevVertex,
                     const b2Vec2 &nextVertex);

    /// Implement b2Shape. Vertices are cloned using b2Alloc.
    b2Shape *Clone(b2BlockAllocator *allocator) const override;

    /// @see b2Shape::GetChildCount
    int32 GetChildCount() const override;

    /// Get a child edge.
    void GetChildEdge(b2EdgeShape *edge, int32 index) const;

    /// This always return false.
    /// @see b2Shape::TestPoint
    bool TestPoint(const b2Transform &transform, const b2Vec2 &p) const override;

    /// Implement b2Shape.
    bool RayCast(b2RayCastOutput *output, const b2RayCastInput &input, const b2Transform &transform,
                 int32 childIndex) const override;

    /// @see b2Shape::ComputeAABB
    void ComputeAABB(b2AABB *aabb, const b2Transform &transform, int32 childIndex) const override;

    /// Chains have zero mass.
    /// @see b2Shape::ComputeMass
    void ComputeMass(b2MassData *massData, float density) const override;

    /// The vertices. Owned by this class.
    b2Vec2 *m_vertices;

    /// The vertex count.
    int32 m_count;

    b2Vec2 m_prevVertex, m_nextVertex;
};

inline b2ChainShape::b2ChainShape() {
    m_type = e_chain;
    m_radius = b2_polygonRadius;
    m_vertices = nullptr;
    m_count = 0;
}

#pragma endregion

#pragma region

/// A solid circle shape
class b2CircleShape : public b2Shape {
public:
    b2CircleShape();

    /// Implement b2Shape.
    b2Shape *Clone(b2BlockAllocator *allocator) const override;

    /// @see b2Shape::GetChildCount
    int32 GetChildCount() const override;

    /// Implement b2Shape.
    bool TestPoint(const b2Transform &transform, const b2Vec2 &p) const override;

    /// Implement b2Shape.
    /// @note because the circle is solid, rays that start inside do not hit because the normal is
    /// not defined.
    bool RayCast(b2RayCastOutput *output, const b2RayCastInput &input, const b2Transform &transform,
                 int32 childIndex) const override;

    /// @see b2Shape::ComputeAABB
    void ComputeAABB(b2AABB *aabb, const b2Transform &transform, int32 childIndex) const override;

    /// @see b2Shape::ComputeMass
    void ComputeMass(b2MassData *massData, float density) const override;

    /// Position
    b2Vec2 m_p;
};

inline b2CircleShape::b2CircleShape() {
    m_type = e_circle;
    m_radius = 0.0f;
    m_p.SetZero();
}

#pragma endregion

#pragma region

/// A line segment (edge) shape. These can be connected in chains or loops
/// to other edge shapes. Edges created independently are two-sided and do
/// no provide smooth movement across junctions.
class b2EdgeShape : public b2Shape {
public:
    b2EdgeShape();

    /// Set this as a part of a sequence. Vertex v0 precedes the edge and vertex v3
    /// follows. These extra vertices are used to provide smooth movement
    /// across junctions. This also makes the collision one-sided. The edge
    /// normal points to the right looking from v1 to v2.
    void SetOneSided(const b2Vec2 &v0, const b2Vec2 &v1, const b2Vec2 &v2, const b2Vec2 &v3);

    /// Set this as an isolated edge. Collision is two-sided.
    void SetTwoSided(const b2Vec2 &v1, const b2Vec2 &v2);

    /// Implement b2Shape.
    b2Shape *Clone(b2BlockAllocator *allocator) const override;

    /// @see b2Shape::GetChildCount
    int32 GetChildCount() const override;

    /// @see b2Shape::TestPoint
    bool TestPoint(const b2Transform &transform, const b2Vec2 &p) const override;

    /// Implement b2Shape.
    bool RayCast(b2RayCastOutput *output, const b2RayCastInput &input, const b2Transform &transform,
                 int32 childIndex) const override;

    /// @see b2Shape::ComputeAABB
    void ComputeAABB(b2AABB *aabb, const b2Transform &transform, int32 childIndex) const override;

    /// @see b2Shape::ComputeMass
    void ComputeMass(b2MassData *massData, float density) const override;

    /// These are the edge vertices
    b2Vec2 m_vertex1, m_vertex2;

    /// Optional adjacent vertices. These are used for smooth collision.
    b2Vec2 m_vertex0, m_vertex3;

    /// Uses m_vertex0 and m_vertex3 to create smooth collision.
    bool m_oneSided;
};

inline b2EdgeShape::b2EdgeShape() {
    m_type = e_edge;
    m_radius = b2_polygonRadius;
    m_vertex0.x = 0.0f;
    m_vertex0.y = 0.0f;
    m_vertex3.x = 0.0f;
    m_vertex3.y = 0.0f;
    m_oneSided = false;
}

#pragma endregion

#pragma region

/// A solid convex polygon. It is assumed that the interior of the polygon is to
/// the left of each edge.
/// Polygons have a maximum number of vertices equal to b2_maxPolygonVertices.
/// In most cases you should not need many vertices for a convex polygon.
class b2PolygonShape : public b2Shape {
public:
    b2PolygonShape();

    /// Implement b2Shape.
    b2Shape *Clone(b2BlockAllocator *allocator) const override;

    /// @see b2Shape::GetChildCount
    int32 GetChildCount() const override;

    /// Create a convex hull from the given array of local points.
    /// The count must be in the range [3, b2_maxPolygonVertices].
    /// @warning the points may be re-ordered, even if they form a convex polygon
    /// @warning collinear points are handled but not removed. Collinear points
    /// may lead to poor stacking behavior.
    void Set(const b2Vec2 *points, int32 count);

    /// Build vertices to represent an axis-aligned box centered on the local origin.
    /// @param hx the half-width.
    /// @param hy the half-height.
    void SetAsBox(float hx, float hy);

    /// Build vertices to represent an oriented box.
    /// @param hx the half-width.
    /// @param hy the half-height.
    /// @param center the center of the box in local coordinates.
    /// @param angle the rotation of the box in local coordinates.
    void SetAsBox(float hx, float hy, const b2Vec2 &center, float angle);

    /// @see b2Shape::TestPoint
    bool TestPoint(const b2Transform &transform, const b2Vec2 &p) const override;

    /// Implement b2Shape.
    /// @note because the polygon is solid, rays that start inside do not hit because the normal is
    /// not defined.
    bool RayCast(b2RayCastOutput *output, const b2RayCastInput &input, const b2Transform &transform,
                 int32 childIndex) const override;

    /// @see b2Shape::ComputeAABB
    void ComputeAABB(b2AABB *aabb, const b2Transform &transform, int32 childIndex) const override;

    /// @see b2Shape::ComputeMass
    void ComputeMass(b2MassData *massData, float density) const override;

    /// Validate convexity. This is a very time consuming operation.
    /// @returns true if valid
    bool Validate() const;

    b2Vec2 m_centroid;
    b2Vec2 m_vertices[b2_maxPolygonVertices];
    b2Vec2 m_normals[b2_maxPolygonVertices];
    int32 m_count;
};

inline b2PolygonShape::b2PolygonShape() {
    m_type = e_polygon;
    m_radius = b2_polygonRadius;
    m_count = 0;
    m_centroid.SetZero();
}
#pragma endregion

#pragma region

#include <string.h>

/// This is a growable LIFO stack with an initial capacity of N.
/// If the stack size exceeds the initial capacity, the heap is used
/// to increase the size of the stack.
template<typename T, int32 N>
class b2GrowableStack {
public:
    b2GrowableStack() {
        m_stack = m_array;
        m_count = 0;
        m_capacity = N;
    }

    ~b2GrowableStack() {
        if (m_stack != m_array) {
            b2Free(m_stack);
            m_stack = nullptr;
        }
    }

    void Push(const T &element) {
        if (m_count == m_capacity) {
            T *old = m_stack;
            m_capacity *= 2;
            m_stack = (T *) b2Alloc(m_capacity * sizeof(T));
            memcpy(m_stack, old, m_count * sizeof(T));
            if (old != m_array) { b2Free(old); }
        }

        m_stack[m_count] = element;
        ++m_count;
    }

    T Pop() {
        b2Assert(m_count > 0);
        --m_count;
        return m_stack[m_count];
    }

    int32 GetCount() { return m_count; }

private:
    T *m_stack;
    T m_array[N];
    int32 m_count;
    int32 m_capacity;
};

#pragma endregion

#pragma region

#define b2_nullNode (-1)

/// A node in the dynamic tree. The client does not interact with this directly.
struct b2TreeNode
{
    bool IsLeaf() const { return child1 == b2_nullNode; }

    /// Enlarged AABB
    b2AABB aabb;

    void *userData;

    union {
        int32 parent;
        int32 next;
    };

    int32 child1;
    int32 child2;

    // leaf = 0, free node = -1
    int32 height;

    bool moved;
};

/// A dynamic AABB tree broad-phase, inspired by Nathanael Presson's btDbvt.
/// A dynamic tree arranges data in a binary tree to accelerate
/// queries such as volume queries and ray casts. Leafs are proxies
/// with an AABB. In the tree we expand the proxy AABB by b2_fatAABBFactor
/// so that the proxy AABB is bigger than the client object. This allows the client
/// object to move by small amounts without triggering a tree update.
///
/// Nodes are pooled and relocatable, so we use node indices rather than pointers.
class b2DynamicTree {
public:
    /// Constructing the tree initializes the node pool.
    b2DynamicTree();

    /// Destroy the tree, freeing the node pool.
    ~b2DynamicTree();

    /// Create a proxy. Provide a tight fitting AABB and a userData pointer.
    int32 CreateProxy(const b2AABB &aabb, void *userData);

    /// Destroy a proxy. This asserts if the id is invalid.
    void DestroyProxy(int32 proxyId);

    /// Move a proxy with a swepted AABB. If the proxy has moved outside of its fattened AABB,
    /// then the proxy is removed from the tree and re-inserted. Otherwise
    /// the function returns immediately.
    /// @return true if the proxy was re-inserted.
    bool MoveProxy(int32 proxyId, const b2AABB &aabb1, const b2Vec2 &displacement);

    /// Get proxy user data.
    /// @return the proxy user data or 0 if the id is invalid.
    void *GetUserData(int32 proxyId) const;

    bool WasMoved(int32 proxyId) const;
    void ClearMoved(int32 proxyId);

    /// Get the fat AABB for a proxy.
    const b2AABB &GetFatAABB(int32 proxyId) const;

    /// Query an AABB for overlapping proxies. The callback class
    /// is called for each proxy that overlaps the supplied AABB.
    template<typename T>
    void Query(T *callback, const b2AABB &aabb) const;

    /// Ray-cast against the proxies in the tree. This relies on the callback
    /// to perform a exact ray-cast in the case were the proxy contains a shape.
    /// The callback also performs the any collision filtering. This has performance
    /// roughly equal to k * log(n), where k is the number of collisions and n is the
    /// number of proxies in the tree.
    /// @param input the ray-cast input data. The ray extends from p1 to p1 + maxFraction * (p2 - p1).
    /// @param callback a callback class that is called for each proxy that is hit by the ray.
    template<typename T>
    void RayCast(T *callback, const b2RayCastInput &input) const;

    /// Validate this tree. For testing.
    void Validate() const;

    /// Compute the height of the binary tree in O(N) time. Should not be
    /// called often.
    int32 GetHeight() const;

    /// Get the maximum balance of an node in the tree. The balance is the difference
    /// in height of the two children of a node.
    int32 GetMaxBalance() const;

    /// Get the ratio of the sum of the node areas to the root area.
    float GetAreaRatio() const;

    /// Build an optimal tree. Very expensive. For testing.
    void RebuildBottomUp();

    /// Shift the world origin. Useful for large worlds.
    /// The shift formula is: position -= newOrigin
    /// @param newOrigin the new origin with respect to the old origin
    void ShiftOrigin(const b2Vec2 &newOrigin);

private:
    int32 AllocateNode();
    void FreeNode(int32 node);

    void InsertLeaf(int32 node);
    void RemoveLeaf(int32 node);

    int32 Balance(int32 index);

    int32 ComputeHeight() const;
    int32 ComputeHeight(int32 nodeId) const;

    void ValidateStructure(int32 index) const;
    void ValidateMetrics(int32 index) const;

    int32 m_root;

    b2TreeNode *m_nodes;
    int32 m_nodeCount;
    int32 m_nodeCapacity;

    int32 m_freeList;

    int32 m_insertionCount;
};

inline void *b2DynamicTree::GetUserData(int32 proxyId) const {
    b2Assert(0 <= proxyId && proxyId < m_nodeCapacity);
    return m_nodes[proxyId].userData;
}

inline bool b2DynamicTree::WasMoved(int32 proxyId) const {
    b2Assert(0 <= proxyId && proxyId < m_nodeCapacity);
    return m_nodes[proxyId].moved;
}

inline void b2DynamicTree::ClearMoved(int32 proxyId) {
    b2Assert(0 <= proxyId && proxyId < m_nodeCapacity);
    m_nodes[proxyId].moved = false;
}

inline const b2AABB &b2DynamicTree::GetFatAABB(int32 proxyId) const {
    b2Assert(0 <= proxyId && proxyId < m_nodeCapacity);
    return m_nodes[proxyId].aabb;
}

template<typename T>
inline void b2DynamicTree::Query(T *callback, const b2AABB &aabb) const {
    b2GrowableStack<int32, 256> stack;
    stack.Push(m_root);

    while (stack.GetCount() > 0) {
        int32 nodeId = stack.Pop();
        if (nodeId == b2_nullNode) { continue; }

        const b2TreeNode *node = m_nodes + nodeId;

        if (b2TestOverlap(node->aabb, aabb)) {
            if (node->IsLeaf()) {
                bool proceed = callback->QueryCallback(nodeId);
                if (proceed == false) { return; }
            } else {
                stack.Push(node->child1);
                stack.Push(node->child2);
            }
        }
    }
}

template<typename T>
inline void b2DynamicTree::RayCast(T *callback, const b2RayCastInput &input) const {
    b2Vec2 p1 = input.p1;
    b2Vec2 p2 = input.p2;
    b2Vec2 r = p2 - p1;
    b2Assert(r.LengthSquared() > 0.0f);
    r.Normalize();

    // v is perpendicular to the segment.
    b2Vec2 v = b2Cross(1.0f, r);
    b2Vec2 abs_v = b2Abs(v);

    // Separating axis for segment (Gino, p80).
    // |dot(v, p1 - c)| > dot(|v|, h)

    float maxFraction = input.maxFraction;

    // Build a bounding box for the segment.
    b2AABB segmentAABB;
    {
        b2Vec2 t = p1 + maxFraction * (p2 - p1);
        segmentAABB.lowerBound = b2Min(p1, t);
        segmentAABB.upperBound = b2Max(p1, t);
    }

    b2GrowableStack<int32, 256> stack;
    stack.Push(m_root);

    while (stack.GetCount() > 0) {
        int32 nodeId = stack.Pop();
        if (nodeId == b2_nullNode) { continue; }

        const b2TreeNode *node = m_nodes + nodeId;

        if (b2TestOverlap(node->aabb, segmentAABB) == false) { continue; }

        // Separating axis for segment (Gino, p80).
        // |dot(v, p1 - c)| > dot(|v|, h)
        b2Vec2 c = node->aabb.GetCenter();
        b2Vec2 h = node->aabb.GetExtents();
        float separation = b2Abs(b2Dot(v, p1 - c)) - b2Dot(abs_v, h);
        if (separation > 0.0f) { continue; }

        if (node->IsLeaf()) {
            b2RayCastInput subInput;
            subInput.p1 = input.p1;
            subInput.p2 = input.p2;
            subInput.maxFraction = maxFraction;

            float value = callback->RayCastCallback(subInput, nodeId);

            if (value == 0.0f) {
                // The client has terminated the ray cast.
                return;
            }

            if (value > 0.0f) {
                // Update segment bounding box.
                maxFraction = value;
                b2Vec2 t = p1 + maxFraction * (p2 - p1);
                segmentAABB.lowerBound = b2Min(p1, t);
                segmentAABB.upperBound = b2Max(p1, t);
            }
        } else {
            stack.Push(node->child1);
            stack.Push(node->child2);
        }
    }
}

#pragma endregion

#pragma region

struct b2Pair
{
    int32 proxyIdA;
    int32 proxyIdB;
};

/// The broad-phase is used for computing pairs and performing volume queries and ray casts.
/// This broad-phase does not persist pairs. Instead, this reports potentially new pairs.
/// It is up to the client to consume the new pairs and to track subsequent overlap.
class b2BroadPhase {
public:
    enum {
        e_nullProxy = -1
    };

    b2BroadPhase();
    ~b2BroadPhase();

    /// Create a proxy with an initial AABB. Pairs are not reported until
    /// UpdatePairs is called.
    int32 CreateProxy(const b2AABB &aabb, void *userData);

    /// Destroy a proxy. It is up to the client to remove any pairs.
    void DestroyProxy(int32 proxyId);

    /// Call MoveProxy as many times as you like, then when you are done
    /// call UpdatePairs to finalized the proxy pairs (for your time step).
    void MoveProxy(int32 proxyId, const b2AABB &aabb, const b2Vec2 &displacement);

    /// Call to trigger a re-processing of it's pairs on the next call to UpdatePairs.
    void TouchProxy(int32 proxyId);

    /// Get the fat AABB for a proxy.
    const b2AABB &GetFatAABB(int32 proxyId) const;

    /// Get user data from a proxy. Returns nullptr if the id is invalid.
    void *GetUserData(int32 proxyId) const;

    /// Test overlap of fat AABBs.
    bool TestOverlap(int32 proxyIdA, int32 proxyIdB) const;

    /// Get the number of proxies.
    int32 GetProxyCount() const;

    /// Update the pairs. This results in pair callbacks. This can only add pairs.
    template<typename T>
    void UpdatePairs(T *callback);

    /// Query an AABB for overlapping proxies. The callback class
    /// is called for each proxy that overlaps the supplied AABB.
    template<typename T>
    void Query(T *callback, const b2AABB &aabb) const;

    /// Ray-cast against the proxies in the tree. This relies on the callback
    /// to perform a exact ray-cast in the case were the proxy contains a shape.
    /// The callback also performs the any collision filtering. This has performance
    /// roughly equal to k * log(n), where k is the number of collisions and n is the
    /// number of proxies in the tree.
    /// @param input the ray-cast input data. The ray extends from p1 to p1 + maxFraction * (p2 - p1).
    /// @param callback a callback class that is called for each proxy that is hit by the ray.
    template<typename T>
    void RayCast(T *callback, const b2RayCastInput &input) const;

    /// Get the height of the embedded tree.
    int32 GetTreeHeight() const;

    /// Get the balance of the embedded tree.
    int32 GetTreeBalance() const;

    /// Get the quality metric of the embedded tree.
    float GetTreeQuality() const;

    /// Shift the world origin. Useful for large worlds.
    /// The shift formula is: position -= newOrigin
    /// @param newOrigin the new origin with respect to the old origin
    void ShiftOrigin(const b2Vec2 &newOrigin);

private:
    friend class b2DynamicTree;

    void BufferMove(int32 proxyId);
    void UnBufferMove(int32 proxyId);

    bool QueryCallback(int32 proxyId);

    b2DynamicTree m_tree;

    int32 m_proxyCount;

    int32 *m_moveBuffer;
    int32 m_moveCapacity;
    int32 m_moveCount;

    b2Pair *m_pairBuffer;
    int32 m_pairCapacity;
    int32 m_pairCount;

    int32 m_queryProxyId;
};

inline void *b2BroadPhase::GetUserData(int32 proxyId) const { return m_tree.GetUserData(proxyId); }

inline bool b2BroadPhase::TestOverlap(int32 proxyIdA, int32 proxyIdB) const {
    const b2AABB &aabbA = m_tree.GetFatAABB(proxyIdA);
    const b2AABB &aabbB = m_tree.GetFatAABB(proxyIdB);
    return b2TestOverlap(aabbA, aabbB);
}

inline const b2AABB &b2BroadPhase::GetFatAABB(int32 proxyId) const {
    return m_tree.GetFatAABB(proxyId);
}

inline int32 b2BroadPhase::GetProxyCount() const { return m_proxyCount; }

inline int32 b2BroadPhase::GetTreeHeight() const { return m_tree.GetHeight(); }

inline int32 b2BroadPhase::GetTreeBalance() const { return m_tree.GetMaxBalance(); }

inline float b2BroadPhase::GetTreeQuality() const { return m_tree.GetAreaRatio(); }

template<typename T>
void b2BroadPhase::UpdatePairs(T *callback) {
    // Reset pair buffer
    m_pairCount = 0;

    // Perform tree queries for all moving proxies.
    for (int32 i = 0; i < m_moveCount; ++i) {
        m_queryProxyId = m_moveBuffer[i];
        if (m_queryProxyId == e_nullProxy) { continue; }

        // We have to query the tree with the fat AABB so that
        // we don't fail to create a pair that may touch later.
        const b2AABB &fatAABB = m_tree.GetFatAABB(m_queryProxyId);

        // Query tree, create pairs and add them pair buffer.
        m_tree.Query(this, fatAABB);
    }

    // Send pairs to caller
    for (int32 i = 0; i < m_pairCount; ++i) {
        b2Pair *primaryPair = m_pairBuffer + i;
        void *userDataA = m_tree.GetUserData(primaryPair->proxyIdA);
        void *userDataB = m_tree.GetUserData(primaryPair->proxyIdB);

        callback->AddPair(userDataA, userDataB);
    }

    // Clear move flags
    for (int32 i = 0; i < m_moveCount; ++i) {
        int32 proxyId = m_moveBuffer[i];
        if (proxyId == e_nullProxy) { continue; }

        m_tree.ClearMoved(proxyId);
    }

    // Reset move buffer
    m_moveCount = 0;
}

template<typename T>
inline void b2BroadPhase::Query(T *callback, const b2AABB &aabb) const {
    m_tree.Query(callback, aabb);
}

template<typename T>
inline void b2BroadPhase::RayCast(T *callback, const b2RayCastInput &input) const {
    m_tree.RayCast(callback, input);
}

inline void b2BroadPhase::ShiftOrigin(const b2Vec2 &newOrigin) { m_tree.ShiftOrigin(newOrigin); }
#pragma endregion

#pragma region

/// Profiling data. Times are in milliseconds.
struct b2Profile
{
    float step;
    float collide;
    float solve;
    float solveInit;
    float solveVelocity;
    float solvePosition;
    float broadphase;
    float solveTOI;
};

/// This is an internal structure.
struct b2TimeStep
{
    float dt;     // time step
    float inv_dt; // inverse time step (0 if dt == 0).
    float dtRatio;// dt * inv_dt0
    int32 velocityIterations;
    int32 positionIterations;
    bool warmStarting;
};

/// This is an internal structure.
struct b2Position
{
    b2Vec2 c;
    float a;
};

/// This is an internal structure.
struct b2Velocity
{
    b2Vec2 v;
    float w;
};

/// Solver Data
struct b2SolverData
{
    b2TimeStep step;
    b2Position *positions;
    b2Velocity *velocities;
};

#pragma endregion

#pragma region

class b2Contact;
class b2ContactFilter;
class b2ContactListener;
class b2BlockAllocator;

// Delegate of b2World.
class b2ContactManager {
public:
    b2ContactManager();

    // Broad-phase callback.
    void AddPair(void *proxyUserDataA, void *proxyUserDataB);

    void FindNewContacts();

    void Destroy(b2Contact *c);

    void Collide();

    b2BroadPhase m_broadPhase;
    b2Contact *m_contactList;
    int32 m_contactCount;
    b2ContactFilter *m_contactFilter;
    b2ContactListener *m_contactListener;
    b2BlockAllocator *m_allocator;
};

#pragma endregion

#pragma region

const int32 b2_stackSize = 100 * 1024;// 100k
const int32 b2_maxStackEntries = 32;

struct b2StackEntry
{
    char *data;
    int32 size;
    bool usedMalloc;
};

// This is a stack allocator used for fast per step allocations.
// You must nest allocate/free pairs. The code will assert
// if you try to interleave multiple allocate/free pairs.
class b2StackAllocator {
public:
    b2StackAllocator();
    ~b2StackAllocator();

    void *Allocate(int32 size);
    void Free(void *p);

    int32 GetMaxAllocation() const;

private:
    char m_data[b2_stackSize];
    int32 m_index;

    int32 m_allocation;
    int32 m_maxAllocation;

    b2StackEntry m_entries[b2_maxStackEntries];
    int32 m_entryCount;
};

#pragma endregion

#pragma region

struct b2Vec2;
struct b2Transform;
class b2Fixture;
class b2Body;
class b2Joint;
class b2Contact;
struct b2ContactResult;
struct b2Manifold;

/// Joints and fixtures are destroyed when their associated
/// body is destroyed. Implement this listener so that you
/// may nullify references to these joints and shapes.
class b2DestructionListener {
public:
    virtual ~b2DestructionListener() {}

    /// Called when any joint is about to be destroyed due
    /// to the destruction of one of its attached bodies.
    virtual void SayGoodbye(b2Joint *joint) = 0;

    /// Called when any fixture is about to be destroyed due
    /// to the destruction of its parent body.
    virtual void SayGoodbye(b2Fixture *fixture) = 0;
};

/// Implement this class to provide collision filtering. In other words, you can implement
/// this class if you want finer control over contact creation.
class b2ContactFilter {
public:
    virtual ~b2ContactFilter() {}

    /// Return true if contact calculations should be performed between these two shapes.
    /// @warning for performance reasons this is only called when the AABBs begin to overlap.
    virtual bool ShouldCollide(b2Fixture *fixtureA, b2Fixture *fixtureB);
};

/// Contact impulses for reporting. Impulses are used instead of forces because
/// sub-step forces may approach infinity for rigid body collisions. These
/// match up one-to-one with the contact points in b2Manifold.
struct b2ContactImpulse
{
    float normalImpulses[b2_maxManifoldPoints];
    float tangentImpulses[b2_maxManifoldPoints];
    int32 count;
};

/// Implement this class to get contact information. You can use these results for
/// things like sounds and game logic. You can also get contact results by
/// traversing the contact lists after the time step. However, you might miss
/// some contacts because continuous physics leads to sub-stepping.
/// Additionally you may receive multiple callbacks for the same contact in a
/// single time step.
/// You should strive to make your callbacks efficient because there may be
/// many callbacks per time step.
/// @warning You cannot create/destroy Box2D entities inside these callbacks.
class b2ContactListener {
public:
    virtual ~b2ContactListener() {}

    /// Called when two fixtures begin to touch.
    virtual void BeginContact(b2Contact *contact) { B2_NOT_USED(contact); }

    /// Called when two fixtures cease to touch.
    virtual void EndContact(b2Contact *contact) { B2_NOT_USED(contact); }

    /// This is called after a contact is updated. This allows you to inspect a
    /// contact before it goes to the solver. If you are careful, you can modify the
    /// contact manifold (e.g. disable contact).
    /// A copy of the old manifold is provided so that you can detect changes.
    /// Note: this is called only for awake bodies.
    /// Note: this is called even when the number of contact points is zero.
    /// Note: this is not called for sensors.
    /// Note: if you set the number of contact points to zero, you will not
    /// get an EndContact callback. However, you may get a BeginContact callback
    /// the next step.
    virtual void PreSolve(b2Contact *contact, const b2Manifold *oldManifold) {
        B2_NOT_USED(contact);
        B2_NOT_USED(oldManifold);
    }

    /// This lets you inspect a contact after the solver is finished. This is useful
    /// for inspecting impulses.
    /// Note: the contact manifold does not include time of impact impulses, which can be
    /// arbitrarily large if the sub-step is small. Hence the impulse is provided explicitly
    /// in a separate data structure.
    /// Note: this is only called for contacts that are touching, solid, and awake.
    virtual void PostSolve(b2Contact *contact, const b2ContactImpulse *impulse) {
        B2_NOT_USED(contact);
        B2_NOT_USED(impulse);
    }
};

/// Callback class for AABB queries.
/// See b2World::Query
class b2QueryCallback {
public:
    virtual ~b2QueryCallback() {}

    /// Called for each fixture found in the query AABB.
    /// @return false to terminate the query.
    virtual bool ReportFixture(b2Fixture *fixture) = 0;
};

/// Callback class for ray casts.
/// See b2World::RayCast
class b2RayCastCallback {
public:
    virtual ~b2RayCastCallback() {}

    /// Called for each fixture found in the query. You control how the ray cast
    /// proceeds by returning a float:
    /// return -1: ignore this fixture and continue
    /// return 0: terminate the ray cast
    /// return fraction: clip the ray to this point
    /// return 1: don't clip the ray and continue
    /// @param fixture the fixture hit by the ray
    /// @param point the point of initial intersection
    /// @param normal the normal vector at the point of intersection
    /// @param fraction the fraction along the ray at the point of intersection
    /// @return -1 to filter, 0 to terminate, fraction to clip the ray for
    /// closest hit, 1 to continue
    virtual float ReportFixture(b2Fixture *fixture, const b2Vec2 &point, const b2Vec2 &normal,
                                float fraction) = 0;
};

#pragma endregion

#pragma region

struct b2AABB;
struct b2BodyDef;
struct b2Color;
struct b2JointDef;
class b2Body;
class b2Draw;
class b2Fixture;
class b2Joint;

/// The world class manages all physics entities, dynamic simulation,
/// and asynchronous queries. The world also contains efficient memory
/// management facilities.
class b2World {
public:
    /// Construct a world object.
    /// @param gravity the world gravity vector.
    b2World(const b2Vec2 &gravity);

    /// Destruct the world. All physics entities are destroyed and all heap memory is released.
    ~b2World();

    /// Register a destruction listener. The listener is owned by you and must
    /// remain in scope.
    void SetDestructionListener(b2DestructionListener *listener);

    /// Register a contact filter to provide specific control over collision.
    /// Otherwise the default filter is used (b2_defaultFilter). The listener is
    /// owned by you and must remain in scope.
    void SetContactFilter(b2ContactFilter *filter);

    /// Register a contact event listener. The listener is owned by you and must
    /// remain in scope.
    void SetContactListener(b2ContactListener *listener);

    /// Register a routine for debug drawing. The debug draw functions are called
    /// inside with b2World::DebugDraw method. The debug draw object is owned
    /// by you and must remain in scope.
    void SetDebugDraw(b2Draw *debugDraw);

    /// Create a rigid body given a definition. No reference to the definition
    /// is retained.
    /// @warning This function is locked during callbacks.
    b2Body *CreateBody(const b2BodyDef *def);

    /// Destroy a rigid body given a definition. No reference to the definition
    /// is retained. This function is locked during callbacks.
    /// @warning This automatically deletes all associated shapes and joints.
    /// @warning This function is locked during callbacks.
    void DestroyBody(b2Body *body);

    /// Create a joint to constrain bodies together. No reference to the definition
    /// is retained. This may cause the connected bodies to cease colliding.
    /// @warning This function is locked during callbacks.
    b2Joint *CreateJoint(const b2JointDef *def);

    /// Destroy a joint. This may cause the connected bodies to begin colliding.
    /// @warning This function is locked during callbacks.
    void DestroyJoint(b2Joint *joint);

    /// Take a time step. This performs collision detection, integration,
    /// and constraint solution.
    /// @param timeStep the amount of time to simulate, this should not vary.
    /// @param velocityIterations for the velocity constraint solver.
    /// @param positionIterations for the position constraint solver.
    void Step(float timeStep, int32 velocityIterations, int32 positionIterations);

    /// Manually clear the force buffer on all bodies. By default, forces are cleared automatically
    /// after each call to Step. The default behavior is modified by calling SetAutoClearForces.
    /// The purpose of this function is to support sub-stepping. Sub-stepping is often used to maintain
    /// a fixed sized time step under a variable frame-rate.
    /// When you perform sub-stepping you will disable auto clearing of forces and instead call
    /// ClearForces after all sub-steps are complete in one pass of your game loop.
    /// @see SetAutoClearForces
    void ClearForces();

    /// Call this to draw shapes and other debug draw data. This is intentionally non-const.
    void DebugDraw();

    /// Query the world for all fixtures that potentially overlap the
    /// provided AABB.
    /// @param callback a user implemented callback class.
    /// @param aabb the query box.
    void QueryAABB(b2QueryCallback *callback, const b2AABB &aabb) const;

    /// Ray-cast the world for all fixtures in the path of the ray. Your callback
    /// controls whether you get the closest point, any point, or n-points.
    /// The ray-cast ignores shapes that contain the starting point.
    /// @param callback a user implemented callback class.
    /// @param point1 the ray starting point
    /// @param point2 the ray ending point
    void RayCast(b2RayCastCallback *callback, const b2Vec2 &point1, const b2Vec2 &point2) const;

    /// Get the world body list. With the returned body, use b2Body::GetNext to get
    /// the next body in the world list. A nullptr body indicates the end of the list.
    /// @return the head of the world body list.
    b2Body *GetBodyList();
    const b2Body *GetBodyList() const;

    /// Get the world joint list. With the returned joint, use b2Joint::GetNext to get
    /// the next joint in the world list. A nullptr joint indicates the end of the list.
    /// @return the head of the world joint list.
    b2Joint *GetJointList();
    const b2Joint *GetJointList() const;

    /// Get the world contact list. With the returned contact, use b2Contact::GetNext to get
    /// the next contact in the world list. A nullptr contact indicates the end of the list.
    /// @return the head of the world contact list.
    /// @warning contacts are created and destroyed in the middle of a time step.
    /// Use b2ContactListener to avoid missing contacts.
    b2Contact *GetContactList();
    const b2Contact *GetContactList() const;

    /// Enable/disable sleep.
    void SetAllowSleeping(bool flag);
    bool GetAllowSleeping() const { return m_allowSleep; }

    /// Enable/disable warm starting. For testing.
    void SetWarmStarting(bool flag) { m_warmStarting = flag; }
    bool GetWarmStarting() const { return m_warmStarting; }

    /// Enable/disable continuous physics. For testing.
    void SetContinuousPhysics(bool flag) { m_continuousPhysics = flag; }
    bool GetContinuousPhysics() const { return m_continuousPhysics; }

    /// Enable/disable single stepped continuous physics. For testing.
    void SetSubStepping(bool flag) { m_subStepping = flag; }
    bool GetSubStepping() const { return m_subStepping; }

    /// Get the number of broad-phase proxies.
    int32 GetProxyCount() const;

    /// Get the number of bodies.
    int32 GetBodyCount() const;

    /// Get the number of joints.
    int32 GetJointCount() const;

    /// Get the number of contacts (each may have 0 or more contact points).
    int32 GetContactCount() const;

    /// Get the height of the dynamic tree.
    int32 GetTreeHeight() const;

    /// Get the balance of the dynamic tree.
    int32 GetTreeBalance() const;

    /// Get the quality metric of the dynamic tree. The smaller the better.
    /// The minimum is 1.
    float GetTreeQuality() const;

    /// Change the global gravity vector.
    void SetGravity(const b2Vec2 &gravity);

    /// Get the global gravity vector.
    b2Vec2 GetGravity() const;

    /// Is the world locked (in the middle of a time step).
    bool IsLocked() const;

    /// Set flag to control automatic clearing of forces after each time step.
    void SetAutoClearForces(bool flag);

    /// Get the flag that controls automatic clearing of forces after each time step.
    bool GetAutoClearForces() const;

    /// Shift the world origin. Useful for large worlds.
    /// The body shift formula is: position -= newOrigin
    /// @param newOrigin the new origin with respect to the old origin
    void ShiftOrigin(const b2Vec2 &newOrigin);

    /// Get the contact manager for testing.
    const b2ContactManager &GetContactManager() const;

    /// Get the current profile.
    const b2Profile &GetProfile() const;

    /// Dump the world into the log file.
    /// @warning this should be called outside of a time step.
    void Dump();

private:
    friend class b2Body;
    friend class b2Fixture;
    friend class b2ContactManager;
    friend class b2Controller;

    void Solve(const b2TimeStep &step);
    void SolveTOI(const b2TimeStep &step);

    void DrawShape(b2Fixture *shape, const b2Transform &xf, const b2Color &color);

    b2BlockAllocator m_blockAllocator;
    b2StackAllocator m_stackAllocator;

    b2ContactManager m_contactManager;

    b2Body *m_bodyList;
    b2Joint *m_jointList;

    int32 m_bodyCount;
    int32 m_jointCount;

    b2Vec2 m_gravity;
    bool m_allowSleep;

    b2DestructionListener *m_destructionListener;
    b2Draw *m_debugDraw;

    // This is used to compute the time step ratio to
    // support a variable time step.
    float m_inv_dt0;

    bool m_newContacts;
    bool m_locked;
    bool m_clearForces;

    // These are for debugging the solver.
    bool m_warmStarting;
    bool m_continuousPhysics;
    bool m_subStepping;

    bool m_stepComplete;

    b2Profile m_profile;
};

inline b2Body *b2World::GetBodyList() { return m_bodyList; }

inline const b2Body *b2World::GetBodyList() const { return m_bodyList; }

inline b2Joint *b2World::GetJointList() { return m_jointList; }

inline const b2Joint *b2World::GetJointList() const { return m_jointList; }

inline b2Contact *b2World::GetContactList() { return m_contactManager.m_contactList; }

inline const b2Contact *b2World::GetContactList() const { return m_contactManager.m_contactList; }

inline int32 b2World::GetBodyCount() const { return m_bodyCount; }

inline int32 b2World::GetJointCount() const { return m_jointCount; }

inline int32 b2World::GetContactCount() const { return m_contactManager.m_contactCount; }

inline void b2World::SetGravity(const b2Vec2 &gravity) { m_gravity = gravity; }

inline b2Vec2 b2World::GetGravity() const { return m_gravity; }

inline bool b2World::IsLocked() const { return m_locked; }

inline void b2World::SetAutoClearForces(bool flag) { m_clearForces = flag; }

/// Get the flag that controls automatic clearing of forces after each time step.
inline bool b2World::GetAutoClearForces() const { return m_clearForces; }

inline const b2ContactManager &b2World::GetContactManager() const { return m_contactManager; }

inline const b2Profile &b2World::GetProfile() const { return m_profile; }

#pragma endregion

#pragma region

class b2Contact;
class b2Joint;
class b2StackAllocator;
class b2ContactListener;
struct b2ContactVelocityConstraint;
struct b2Profile;

/// This is an internal class.
class b2Island {
public:
    b2Island(int32 bodyCapacity, int32 contactCapacity, int32 jointCapacity,
             b2StackAllocator *allocator, b2ContactListener *listener);
    ~b2Island();

    void Clear() {
        m_bodyCount = 0;
        m_contactCount = 0;
        m_jointCount = 0;
    }

    void Solve(b2Profile *profile, const b2TimeStep &step, const b2Vec2 &gravity, bool allowSleep);

    void SolveTOI(const b2TimeStep &subStep, int32 toiIndexA, int32 toiIndexB);

    void Add(b2Body *body) {
        b2Assert(m_bodyCount < m_bodyCapacity);
        body->m_islandIndex = m_bodyCount;
        m_bodies[m_bodyCount] = body;
        ++m_bodyCount;
    }

    void Add(b2Contact *contact) {
        b2Assert(m_contactCount < m_contactCapacity);
        m_contacts[m_contactCount++] = contact;
    }

    void Add(b2Joint *joint) {
        b2Assert(m_jointCount < m_jointCapacity);
        m_joints[m_jointCount++] = joint;
    }

    void Report(const b2ContactVelocityConstraint *constraints);

    b2StackAllocator *m_allocator;
    b2ContactListener *m_listener;

    b2Body **m_bodies;
    b2Contact **m_contacts;
    b2Joint **m_joints;

    b2Position *m_positions;
    b2Velocity *m_velocities;

    int32 m_bodyCount;
    int32 m_jointCount;
    int32 m_contactCount;

    int32 m_bodyCapacity;
    int32 m_contactCapacity;
    int32 m_jointCapacity;
};

#pragma endregion

#pragma region

class b2BlockAllocator;

class b2ChainAndCircleContact : public b2Contact {
public:
    static b2Contact *Create(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB,
                             b2BlockAllocator *allocator);
    static void Destroy(b2Contact *contact, b2BlockAllocator *allocator);

    b2ChainAndCircleContact(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB);
    ~b2ChainAndCircleContact() {}

    void Evaluate(b2Manifold *manifold, const b2Transform &xfA, const b2Transform &xfB) override;
};
#pragma endregion

#pragma region

class b2BlockAllocator;

class b2ChainAndPolygonContact : public b2Contact {
public:
    static b2Contact *Create(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB,
                             b2BlockAllocator *allocator);
    static void Destroy(b2Contact *contact, b2BlockAllocator *allocator);

    b2ChainAndPolygonContact(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB);
    ~b2ChainAndPolygonContact() {}

    void Evaluate(b2Manifold *manifold, const b2Transform &xfA, const b2Transform &xfB) override;
};

#pragma endregion

#pragma region

class b2BlockAllocator;

class b2CircleContact : public b2Contact {
public:
    static b2Contact *Create(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB,
                             b2BlockAllocator *allocator);
    static void Destroy(b2Contact *contact, b2BlockAllocator *allocator);

    b2CircleContact(b2Fixture *fixtureA, b2Fixture *fixtureB);
    ~b2CircleContact() {}

    void Evaluate(b2Manifold *manifold, const b2Transform &xfA, const b2Transform &xfB) override;
};
#pragma endregion

#pragma region

class b2Contact;
class b2Body;
class b2StackAllocator;
struct b2ContactPositionConstraint;

struct b2VelocityConstraintPoint
{
    b2Vec2 rA;
    b2Vec2 rB;
    float normalImpulse;
    float tangentImpulse;
    float normalMass;
    float tangentMass;
    float velocityBias;
};

struct b2ContactVelocityConstraint
{
    b2VelocityConstraintPoint points[b2_maxManifoldPoints];
    b2Vec2 normal;
    b2Mat22 normalMass;
    b2Mat22 K;
    int32 indexA;
    int32 indexB;
    float invMassA, invMassB;
    float invIA, invIB;
    float friction;
    float restitution;
    float threshold;
    float tangentSpeed;
    int32 pointCount;
    int32 contactIndex;
};

struct b2ContactSolverDef
{
    b2TimeStep step;
    b2Contact **contacts;
    int32 count;
    b2Position *positions;
    b2Velocity *velocities;
    b2StackAllocator *allocator;
};

class b2ContactSolver {
public:
    b2ContactSolver(b2ContactSolverDef *def);
    ~b2ContactSolver();

    void InitializeVelocityConstraints();

    void WarmStart();
    void SolveVelocityConstraints();
    void StoreImpulses();

    bool SolvePositionConstraints();
    bool SolveTOIPositionConstraints(int32 toiIndexA, int32 toiIndexB);

    b2TimeStep m_step;
    b2Position *m_positions;
    b2Velocity *m_velocities;
    b2StackAllocator *m_allocator;
    b2ContactPositionConstraint *m_positionConstraints;
    b2ContactVelocityConstraint *m_velocityConstraints;
    b2Contact **m_contacts;
    int m_count;
};

#pragma endregion

#pragma region

class b2Shape;

/// A distance proxy is used by the GJK algorithm.
/// It encapsulates any shape.
struct b2DistanceProxy
{
    b2DistanceProxy() : m_vertices(nullptr), m_count(0), m_radius(0.0f) {}

    /// Initialize the proxy using the given shape. The shape
    /// must remain in scope while the proxy is in use.
    void Set(const b2Shape *shape, int32 index);

    /// Initialize the proxy using a vertex cloud and radius. The vertices
    /// must remain in scope while the proxy is in use.
    void Set(const b2Vec2 *vertices, int32 count, float radius);

    /// Get the supporting vertex index in the given direction.
    int32 GetSupport(const b2Vec2 &d) const;

    /// Get the supporting vertex in the given direction.
    const b2Vec2 &GetSupportVertex(const b2Vec2 &d) const;

    /// Get the vertex count.
    int32 GetVertexCount() const;

    /// Get a vertex by index. Used by b2Distance.
    const b2Vec2 &GetVertex(int32 index) const;

    b2Vec2 m_buffer[2];
    const b2Vec2 *m_vertices;
    int32 m_count;
    float m_radius;
};

/// Used to warm start b2Distance.
/// Set count to zero on first call.
struct b2SimplexCache
{
    float metric;///< length or area
    uint16 count;
    uint8 indexA[3];///< vertices on shape A
    uint8 indexB[3];///< vertices on shape B
};

/// Input for b2Distance.
/// You have to option to use the shape radii
/// in the computation. Even
struct b2DistanceInput
{
    b2DistanceProxy proxyA;
    b2DistanceProxy proxyB;
    b2Transform transformA;
    b2Transform transformB;
    bool useRadii;
};

/// Output for b2Distance.
struct b2DistanceOutput
{
    b2Vec2 pointA;///< closest point on shapeA
    b2Vec2 pointB;///< closest point on shapeB
    float distance;
    int32 iterations;///< number of GJK iterations used
};

/// Compute the closest points between two shapes. Supports any combination of:
/// b2CircleShape, b2PolygonShape, b2EdgeShape. The simplex cache is input/output.
/// On the first call set b2SimplexCache.count to zero.
void b2Distance(b2DistanceOutput *output, b2SimplexCache *cache, const b2DistanceInput *input);

/// Input parameters for b2ShapeCast
struct b2ShapeCastInput
{
    b2DistanceProxy proxyA;
    b2DistanceProxy proxyB;
    b2Transform transformA;
    b2Transform transformB;
    b2Vec2 translationB;
};

/// Output results for b2ShapeCast
struct b2ShapeCastOutput
{
    b2Vec2 point;
    b2Vec2 normal;
    float lambda;
    int32 iterations;
};

/// Perform a linear shape cast of shape B moving and shape A fixed. Determines the hit point, normal, and translation fraction.
/// @returns true if hit, false if there is no hit or an initial overlap
bool b2ShapeCast(b2ShapeCastOutput *output, const b2ShapeCastInput *input);

//////////////////////////////////////////////////////////////////////////

inline int32 b2DistanceProxy::GetVertexCount() const { return m_count; }

inline const b2Vec2 &b2DistanceProxy::GetVertex(int32 index) const {
    b2Assert(0 <= index && index < m_count);
    return m_vertices[index];
}

inline int32 b2DistanceProxy::GetSupport(const b2Vec2 &d) const {
    int32 bestIndex = 0;
    float bestValue = b2Dot(m_vertices[0], d);
    for (int32 i = 1; i < m_count; ++i) {
        float value = b2Dot(m_vertices[i], d);
        if (value > bestValue) {
            bestIndex = i;
            bestValue = value;
        }
    }

    return bestIndex;
}

inline const b2Vec2 &b2DistanceProxy::GetSupportVertex(const b2Vec2 &d) const {
    int32 bestIndex = 0;
    float bestValue = b2Dot(m_vertices[0], d);
    for (int32 i = 1; i < m_count; ++i) {
        float value = b2Dot(m_vertices[i], d);
        if (value > bestValue) {
            bestIndex = i;
            bestValue = value;
        }
    }

    return m_vertices[bestIndex];
}

#pragma endregion

#pragma region

class b2BlockAllocator;

class b2EdgeAndCircleContact : public b2Contact {
public:
    static b2Contact *Create(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB,
                             b2BlockAllocator *allocator);
    static void Destroy(b2Contact *contact, b2BlockAllocator *allocator);

    b2EdgeAndCircleContact(b2Fixture *fixtureA, b2Fixture *fixtureB);
    ~b2EdgeAndCircleContact() {}

    void Evaluate(b2Manifold *manifold, const b2Transform &xfA, const b2Transform &xfB) override;
};
#pragma endregion

#pragma region

class b2BlockAllocator;

class b2EdgeAndPolygonContact : public b2Contact {
public:
    static b2Contact *Create(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB,
                             b2BlockAllocator *allocator);
    static void Destroy(b2Contact *contact, b2BlockAllocator *allocator);

    b2EdgeAndPolygonContact(b2Fixture *fixtureA, b2Fixture *fixtureB);
    ~b2EdgeAndPolygonContact() {}

    void Evaluate(b2Manifold *manifold, const b2Transform &xfA, const b2Transform &xfB) override;
};

#pragma endregion

#pragma region

class b2BlockAllocator;

class b2PolygonAndCircleContact : public b2Contact {
public:
    static b2Contact *Create(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB,
                             b2BlockAllocator *allocator);
    static void Destroy(b2Contact *contact, b2BlockAllocator *allocator);

    b2PolygonAndCircleContact(b2Fixture *fixtureA, b2Fixture *fixtureB);
    ~b2PolygonAndCircleContact() {}

    void Evaluate(b2Manifold *manifold, const b2Transform &xfA, const b2Transform &xfB) override;
};

#pragma endregion

#pragma region

class b2BlockAllocator;

class b2PolygonContact : public b2Contact {
public:
    static b2Contact *Create(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB,
                             b2BlockAllocator *allocator);
    static void Destroy(b2Contact *contact, b2BlockAllocator *allocator);

    b2PolygonContact(b2Fixture *fixtureA, b2Fixture *fixtureB);
    ~b2PolygonContact() {}

    void Evaluate(b2Manifold *manifold, const b2Transform &xfA, const b2Transform &xfB) override;
};

#pragma endregion

#pragma region

class b2Draw;
struct b2RopeStretch;
struct b2RopeBend;

enum b2StretchingModel {
    b2_pbdStretchingModel,
    b2_xpbdStretchingModel
};

enum b2BendingModel {
    b2_springAngleBendingModel = 0,
    b2_pbdAngleBendingModel,
    b2_xpbdAngleBendingModel,
    b2_pbdDistanceBendingModel,
    b2_pbdHeightBendingModel,
    b2_pbdTriangleBendingModel
};

///
struct b2RopeTuning
{
    b2RopeTuning() {
        stretchingModel = b2_pbdStretchingModel;
        bendingModel = b2_pbdAngleBendingModel;
        damping = 0.0f;
        stretchStiffness = 1.0f;
        stretchHertz = 1.0f;
        stretchDamping = 0.0f;
        bendStiffness = 0.5f;
        bendHertz = 1.0f;
        bendDamping = 0.0f;
        isometric = false;
        fixedEffectiveMass = false;
        warmStart = false;
    }

    b2StretchingModel stretchingModel;
    b2BendingModel bendingModel;
    float damping;
    float stretchStiffness;
    float stretchHertz;
    float stretchDamping;
    float bendStiffness;
    float bendHertz;
    float bendDamping;
    bool isometric;
    bool fixedEffectiveMass;
    bool warmStart;
};

///
struct b2RopeDef
{
    b2RopeDef() {
        position.SetZero();
        vertices = nullptr;
        count = 0;
        masses = nullptr;
        gravity.SetZero();
    }

    b2Vec2 position;
    b2Vec2 *vertices;
    int32 count;
    float *masses;
    b2Vec2 gravity;
    b2RopeTuning tuning;
};

///
class b2Rope {
public:
    b2Rope();
    ~b2Rope();

    ///
    void Create(const b2RopeDef &def);

    ///
    void SetTuning(const b2RopeTuning &tuning);

    ///
    void Step(float timeStep, int32 iterations, const b2Vec2 &position);

    ///
    void Reset(const b2Vec2 &position);

    ///
    void Draw(b2Draw *draw) const;

private:
    void SolveStretch_PBD();
    void SolveStretch_XPBD(float dt);
    void SolveBend_PBD_Angle();
    void SolveBend_XPBD_Angle(float dt);
    void SolveBend_PBD_Distance();
    void SolveBend_PBD_Height();
    void SolveBend_PBD_Triangle();
    void ApplyBendForces(float dt);

    b2Vec2 m_position;

    int32 m_count;
    int32 m_stretchCount;
    int32 m_bendCount;

    b2RopeStretch *m_stretchConstraints;
    b2RopeBend *m_bendConstraints;

    b2Vec2 *m_bindPositions;
    b2Vec2 *m_ps;
    b2Vec2 *m_p0s;
    b2Vec2 *m_vs;

    float *m_invMasses;
    b2Vec2 m_gravity;

    b2RopeTuning m_tuning;
};

#pragma endregion

#pragma region

/// Input parameters for b2TimeOfImpact
struct b2TOIInput
{
    b2DistanceProxy proxyA;
    b2DistanceProxy proxyB;
    b2Sweep sweepA;
    b2Sweep sweepB;
    float tMax;// defines sweep interval [0, tMax]
};

/// Output parameters for b2TimeOfImpact.
struct b2TOIOutput
{
    enum State {
        e_unknown,
        e_failed,
        e_overlapped,
        e_touching,
        e_separated
    };

    State state;
    float t;
};

/// Compute the upper bound on time before two shapes penetrate. Time is represented as
/// a fraction between [0,tMax]. This uses a swept separating axis and may miss some intermediate,
/// non-tunneling collisions. If you change the time interval, you should call this function
/// again.
/// Note: use b2Distance to compute the contact point and normal at the time of impact.
void b2TimeOfImpact(b2TOIOutput *output, const b2TOIInput *input);

#pragma endregion

#endif
