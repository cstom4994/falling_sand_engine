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
#include "engine/internal/builtin_box2d.h"
#include "engine/mathlib.h"

F32 math_perlin(F32 x, F32 y, F32 z, int x_wrap = 0, int y_wrap = 0, int z_wrap = 0);

#pragma region NewMATH

namespace NewMaths {
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

#endif
