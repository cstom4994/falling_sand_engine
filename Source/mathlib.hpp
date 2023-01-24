// Copyright(c) 2022-2023, KaoruXun All rights reserved.

// This source file may include
// Polypartition (MIT) by Ivan Fratric
// MarchingSquares(Tom Gibara) (MIT) by Juha Reunanen

#ifndef _METADOT_MATH_HPP_
#define _METADOT_MATH_HPP_

#include <cassert>
#include <list>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

#include "core/cpp/vector.hpp"
#include "internal/builtin_box2d.h"
#include "mathlib.h"

F32 math_perlin(F32 x, F32 y, F32 z, int x_wrap = 0, int y_wrap = 0, int z_wrap = 0);

template <class T>
constexpr T pi = T(3.1415926535897932385L);

#pragma region NewMATH

namespace NewMaths {
F32 vec22angle(metadot_v2 v2);
F32 clamp(F32 input, F32 min, F32 max);
int rand_range(int min, int max);
uint64_t rand_XOR();
inline F64 random_double();
inline F64 random_double(F64 min, F64 max);
struct v2;
struct RandState;
F32 v2_distance_2Points(v2 A, v2 B);
v2 unitvec_AtoB(v2 A, v2 B);
F32 signed_angle_v2(v2 A, v2 B);
v2 Rotate2D(v2 P, F32 sine, F32 cosine);
v2 Rotate2D(v2 P, F32 Angle);
v2 Rotate2D(v2 p, v2 o, F32 angle);
v2 Reflection2D(v2 P, v2 N);
bool PointInRectangle(v2 P, v2 A, v2 B, v2 C);
int sign(F32 x);
static F32 dot(v2 A, v2 B);
static F32 perpdot(v2 A, v2 B);
static bool operator==(v2 A, v2 B);

metadot_vec3 NormalizeVector(metadot_vec3 v);
metadot_vec3 Add(metadot_vec3 a, metadot_vec3 b);
metadot_vec3 Subtract(metadot_vec3 a, metadot_vec3 b);
metadot_vec3 ScalarMult(metadot_vec3 v, F32 s);
F64 Distance(metadot_vec3 a, metadot_vec3 b);
metadot_vec3 VectorProjection(metadot_vec3 a, metadot_vec3 b);
metadot_vec3 Reflection(metadot_vec3 *v1, metadot_vec3 *v2);

F64 DistanceFromPointToLine2D(metadot_vec3 lP1, metadot_vec3 lP2, metadot_vec3 p);

typedef struct Matrix3x3 {
    F32 m[3][3];
} Matrix3x3;

Matrix3x3 Transpose(Matrix3x3 m);
Matrix3x3 Identity();
metadot_vec3 Matrix3x3ToEulerAngles(Matrix3x3 m);
Matrix3x3 EulerAnglesToMatrix3x3(metadot_vec3 rotation);
metadot_vec3 RotateVector(metadot_vec3 v, Matrix3x3 m);
metadot_vec3 RotatePoint(metadot_vec3 p, metadot_vec3 r, metadot_vec3 pivot);
Matrix3x3 MultiplyMatrix3x3(Matrix3x3 a, Matrix3x3 b);

typedef struct Matrix4x4 {
    F32 m[4][4];
} Matrix4x4;

Matrix4x4 Identity4x4();
Matrix4x4 GetProjectionMatrix(F32 right, F32 left, F32 top, F32 bottom, F32 near, F32 far);

F32 Lerp(F64 t, F32 a, F32 b);
int Step(F32 edge, F32 x);
F32 Smoothstep(F32 edge0, F32 edge1, F32 x);
int Modulus(int a, int b);
F32 fModulus(F32 a, F32 b);
}  // namespace NewMaths

#pragma endregion NewMATH

#pragma region PCG

struct pcg_state_setseq_64 {  // Internals are *Private*.
    uint64_t state;           // RNG state.  All values are possible.
    uint64_t inc;             // Controls which RNG sequence (stream) is
                              // selected. Must *always* be odd.
};
typedef struct pcg_state_setseq_64 pcg32_random_t;

// If you *must* statically initialize it, here's one.

#define PCG32_INITIALIZER \
    { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL }

// pcg32_srandom(initstate, initseq)
// pcg32_srandom_r(rng, initstate, initseq):
//     Seed the rng.  Specified in two parts, state initializer and a
//     sequence selection constant (a.k.a. stream id)

void pcg32_srandom(uint64_t initstate, uint64_t initseq);
void pcg32_srandom_r(pcg32_random_t *rng, uint64_t initstate, uint64_t initseq);

// pcg32_random()
// pcg32_random_r(rng)
//     Generate a uniformly distributed 32-bit random number

uint32_t pcg32_random(void);
uint32_t pcg32_random_r(pcg32_random_t *rng);

// pcg32_boundedrand(bound):
// pcg32_boundedrand_r(rng, bound):
//     Generate a uniformly distributed number, r, where 0 <= r < bound

uint32_t pcg32_boundedrand(uint32_t bound);
uint32_t pcg32_boundedrand_r(pcg32_random_t *rng, uint32_t bound);

#pragma endregion PCG

void simplify_section(const MetaEngine::vector<b2Vec2> &pts, F32 tolerance, size_t i, size_t j, MetaEngine::vector<bool> *mark_map, size_t omitted = 0);
MetaEngine::vector<b2Vec2> simplify(const MetaEngine::vector<b2Vec2> &vertices, F32 tolerance);
F32 pDistance(F32 x, F32 y, F32 x1, F32 y1, F32 x2, F32 y2);

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
    Direction(b2Vec2 vec) : x(vec.x), y(vec.y) {}
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

typedef F64 tppl_float;

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

#pragma region PORO

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

#include "core/const.h"

namespace MetaEngine {
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
}  // end of namespace MetaEngine

// ---------- types ----------
namespace types {
typedef MetaEngine::math::CVector2<float> vector2;
typedef MetaEngine::math::CVector2<double> dvector2;
typedef MetaEngine::math::CVector2<int> ivector2;

typedef MetaEngine::math::CVector2<int> point;

}  // end of namespace types

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

namespace MetaEngine {
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
}  // end of namespace MetaEngine

//----------------- types --------------------------------------------

namespace types {
typedef MetaEngine::math::CMat22<float> mat22;
}  // end of namespace types

namespace MetaEngine {
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
}  // End of namespace MetaEngine

// -------------- types --------------------------

namespace types {
typedef MetaEngine::math::CXForm<float> xform;
}  // end of namespace types

namespace MetaEngine {
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

    return MetaEngine::math::Max(n, -n);
}

//=============================================================================
// Clams a give value to the desired
//
// Required operators: < operator
//.............................................................................

template <typename Type>
inline Type Clamp(const Type &a, const Type &low, const Type &high) {
    return MetaEngine::math::Max(low, MetaEngine::math::Min(a, high));
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
    float temp = MetaEngine::math::DistanceFromLineSquared(aabb_min, CVector2<PType>(aabb_max.x, aabb_min.y), point);
    if (temp < lowest) lowest = temp;

    temp = MetaEngine::math::DistanceFromLineSquared(CVector2<PType>(aabb_max.x, aabb_min.y), CVector2<PType>(aabb_max.x, aabb_max.y), point);
    if (temp < lowest) lowest = temp;

    temp = MetaEngine::math::DistanceFromLineSquared(CVector2<PType>(aabb_max.x, aabb_max.y), CVector2<PType>(aabb_min.x, aabb_max.y), point);
    if (temp < lowest) lowest = temp;

    temp = MetaEngine::math::DistanceFromLineSquared(CVector2<PType>(aabb_min.x, aabb_max.y), CVector2<PType>(aabb_min.x, aabb_min.y), point);
    if (temp < lowest) lowest = temp;

    return sqrtf(lowest);
}

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
}  // namespace MetaEngine

// ---------------- types ---------------------

namespace types {
typedef MetaEngine::math::CAngle<float> angle;
}  // end of namespace types

#pragma endregion PORO

#endif
