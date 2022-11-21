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

#include "box2d/b2_block_allocator.h"
#include "box2d/b2_broad_phase.h"
#include "box2d/b2_chain_shape.h"
#include "box2d/b2_circle_shape.h"
#include "box2d/b2_collision.h"
#include "box2d/b2_distance.h"
#include "box2d/b2_dynamic_tree.h"
#include "box2d/b2_edge_shape.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_time_of_impact.h"
#include "box2d/b2_timer.h"
#include <new>
#include <stdio.h>
#include <string.h>

b2ChainShape::~b2ChainShape() {
    Clear();
}

void b2ChainShape::Clear() {
    b2Free(m_vertices);
    m_vertices = nullptr;
    m_count = 0;
}

void b2ChainShape::CreateLoop(const b2Vec2 *vertices, int32 count) {
    b2Assert(m_vertices == nullptr && m_count == 0);
    b2Assert(count >= 3);
    if (count < 3) {
        return;
    }

    for (int32 i = 1; i < count; ++i) {
        b2Vec2 v1 = vertices[i - 1];
        b2Vec2 v2 = vertices[i];
        // If the code crashes here, it means your vertices are too close together.
        b2Assert(b2DistanceSquared(v1, v2) > b2_linearSlop * b2_linearSlop);
    }

    m_count = count + 1;
    m_vertices = (b2Vec2 *) b2Alloc(m_count * sizeof(b2Vec2));
    memcpy(m_vertices, vertices, count * sizeof(b2Vec2));
    m_vertices[count] = m_vertices[0];
    m_prevVertex = m_vertices[m_count - 2];
    m_nextVertex = m_vertices[1];
}

void b2ChainShape::CreateChain(const b2Vec2 *vertices, int32 count, const b2Vec2 &prevVertex, const b2Vec2 &nextVertex) {
    b2Assert(m_vertices == nullptr && m_count == 0);
    b2Assert(count >= 2);
    for (int32 i = 1; i < count; ++i) {
        // If the code crashes here, it means your vertices are too close together.
        b2Assert(b2DistanceSquared(vertices[i - 1], vertices[i]) > b2_linearSlop * b2_linearSlop);
    }

    m_count = count;
    m_vertices = (b2Vec2 *) b2Alloc(count * sizeof(b2Vec2));
    memcpy(m_vertices, vertices, m_count * sizeof(b2Vec2));

    m_prevVertex = prevVertex;
    m_nextVertex = nextVertex;
}

b2Shape *b2ChainShape::Clone(b2BlockAllocator *allocator) const {
    void *mem = allocator->Allocate(sizeof(b2ChainShape));
    b2ChainShape *clone = new (mem) b2ChainShape;
    clone->CreateChain(m_vertices, m_count, m_prevVertex, m_nextVertex);
    return clone;
}

int32 b2ChainShape::GetChildCount() const {
    // edge count = vertex count - 1
    return m_count - 1;
}

void b2ChainShape::GetChildEdge(b2EdgeShape *edge, int32 index) const {
    b2Assert(0 <= index && index < m_count - 1);
    edge->m_type = b2Shape::e_edge;
    edge->m_radius = m_radius;

    edge->m_vertex1 = m_vertices[index + 0];
    edge->m_vertex2 = m_vertices[index + 1];
    edge->m_oneSided = true;

    if (index > 0) {
        edge->m_vertex0 = m_vertices[index - 1];
    } else {
        edge->m_vertex0 = m_prevVertex;
    }

    if (index < m_count - 2) {
        edge->m_vertex3 = m_vertices[index + 2];
    } else {
        edge->m_vertex3 = m_nextVertex;
    }
}

bool b2ChainShape::TestPoint(const b2Transform &xf, const b2Vec2 &p) const {
    B2_NOT_USED(xf);
    B2_NOT_USED(p);
    return false;
}

bool b2ChainShape::RayCast(b2RayCastOutput *output, const b2RayCastInput &input,
                           const b2Transform &xf, int32 childIndex) const {
    b2Assert(childIndex < m_count);

    b2EdgeShape edgeShape;

    int32 i1 = childIndex;
    int32 i2 = childIndex + 1;
    if (i2 == m_count) {
        i2 = 0;
    }

    edgeShape.m_vertex1 = m_vertices[i1];
    edgeShape.m_vertex2 = m_vertices[i2];

    return edgeShape.RayCast(output, input, xf, 0);
}

void b2ChainShape::ComputeAABB(b2AABB *aabb, const b2Transform &xf, int32 childIndex) const {
    b2Assert(childIndex < m_count);

    int32 i1 = childIndex;
    int32 i2 = childIndex + 1;
    if (i2 == m_count) {
        i2 = 0;
    }

    b2Vec2 v1 = b2Mul(xf, m_vertices[i1]);
    b2Vec2 v2 = b2Mul(xf, m_vertices[i2]);

    b2Vec2 lower = b2Min(v1, v2);
    b2Vec2 upper = b2Max(v1, v2);

    b2Vec2 r(m_radius, m_radius);
    aabb->lowerBound = lower - r;
    aabb->upperBound = upper + r;
}

void b2ChainShape::ComputeMass(b2MassData *massData, float density) const {
    B2_NOT_USED(density);

    massData->mass = 0.0f;
    massData->center.SetZero();
    massData->I = 0.0f;
}


b2BroadPhase::b2BroadPhase() {
    m_proxyCount = 0;

    m_pairCapacity = 16;
    m_pairCount = 0;
    m_pairBuffer = (b2Pair *) b2Alloc(m_pairCapacity * sizeof(b2Pair));

    m_moveCapacity = 16;
    m_moveCount = 0;
    m_moveBuffer = (int32 *) b2Alloc(m_moveCapacity * sizeof(int32));
}

b2BroadPhase::~b2BroadPhase() {
    b2Free(m_moveBuffer);
    b2Free(m_pairBuffer);
}

int32 b2BroadPhase::CreateProxy(const b2AABB &aabb, void *userData) {
    int32 proxyId = m_tree.CreateProxy(aabb, userData);
    ++m_proxyCount;
    BufferMove(proxyId);
    return proxyId;
}

void b2BroadPhase::DestroyProxy(int32 proxyId) {
    UnBufferMove(proxyId);
    --m_proxyCount;
    m_tree.DestroyProxy(proxyId);
}

void b2BroadPhase::MoveProxy(int32 proxyId, const b2AABB &aabb, const b2Vec2 &displacement) {
    bool buffer = m_tree.MoveProxy(proxyId, aabb, displacement);
    if (buffer) {
        BufferMove(proxyId);
    }
}

void b2BroadPhase::TouchProxy(int32 proxyId) {
    BufferMove(proxyId);
}

void b2BroadPhase::BufferMove(int32 proxyId) {
    if (m_moveCount == m_moveCapacity) {
        int32 *oldBuffer = m_moveBuffer;
        m_moveCapacity *= 2;
        m_moveBuffer = (int32 *) b2Alloc(m_moveCapacity * sizeof(int32));
        memcpy(m_moveBuffer, oldBuffer, m_moveCount * sizeof(int32));
        b2Free(oldBuffer);
    }

    m_moveBuffer[m_moveCount] = proxyId;
    ++m_moveCount;
}

void b2BroadPhase::UnBufferMove(int32 proxyId) {
    for (int32 i = 0; i < m_moveCount; ++i) {
        if (m_moveBuffer[i] == proxyId) {
            m_moveBuffer[i] = e_nullProxy;
        }
    }
}

// This is called from b2DynamicTree::Query when we are gathering pairs.
bool b2BroadPhase::QueryCallback(int32 proxyId) {
    // A proxy cannot form a pair with itself.
    if (proxyId == m_queryProxyId) {
        return true;
    }

    const bool moved = m_tree.WasMoved(proxyId);
    if (moved && proxyId > m_queryProxyId) {
        // Both proxies are moving. Avoid duplicate pairs.
        return true;
    }

    // Grow the pair buffer as needed.
    if (m_pairCount == m_pairCapacity) {
        b2Pair *oldBuffer = m_pairBuffer;
        m_pairCapacity = m_pairCapacity + (m_pairCapacity >> 1);
        m_pairBuffer = (b2Pair *) b2Alloc(m_pairCapacity * sizeof(b2Pair));
        memcpy(m_pairBuffer, oldBuffer, m_pairCount * sizeof(b2Pair));
        b2Free(oldBuffer);
    }

    m_pairBuffer[m_pairCount].proxyIdA = b2Min(proxyId, m_queryProxyId);
    m_pairBuffer[m_pairCount].proxyIdB = b2Max(proxyId, m_queryProxyId);
    ++m_pairCount;

    return true;
}


b2Shape *b2CircleShape::Clone(b2BlockAllocator *allocator) const {
    void *mem = allocator->Allocate(sizeof(b2CircleShape));
    b2CircleShape *clone = new (mem) b2CircleShape;
    *clone = *this;
    return clone;
}

int32 b2CircleShape::GetChildCount() const {
    return 1;
}

bool b2CircleShape::TestPoint(const b2Transform &transform, const b2Vec2 &p) const {
    b2Vec2 center = transform.p + b2Mul(transform.q, m_p);
    b2Vec2 d = p - center;
    return b2Dot(d, d) <= m_radius * m_radius;
}

// Collision Detection in Interactive 3D Environments by Gino van den Bergen
// From Section 3.1.2
// x = s + a * r
// norm(x) = radius
bool b2CircleShape::RayCast(b2RayCastOutput *output, const b2RayCastInput &input,
                            const b2Transform &transform, int32 childIndex) const {
    B2_NOT_USED(childIndex);

    b2Vec2 position = transform.p + b2Mul(transform.q, m_p);
    b2Vec2 s = input.p1 - position;
    float b = b2Dot(s, s) - m_radius * m_radius;

    // Solve quadratic equation.
    b2Vec2 r = input.p2 - input.p1;
    float c = b2Dot(s, r);
    float rr = b2Dot(r, r);
    float sigma = c * c - rr * b;

    // Check for negative discriminant and short segment.
    if (sigma < 0.0f || rr < b2_epsilon) {
        return false;
    }

    // Find the point of intersection of the line with the circle.
    float a = -(c + b2Sqrt(sigma));

    // Is the intersection point on the segment?
    if (0.0f <= a && a <= input.maxFraction * rr) {
        a /= rr;
        output->fraction = a;
        output->normal = s + a * r;
        output->normal.Normalize();
        return true;
    }

    return false;
}

void b2CircleShape::ComputeAABB(b2AABB *aabb, const b2Transform &transform, int32 childIndex) const {
    B2_NOT_USED(childIndex);

    b2Vec2 p = transform.p + b2Mul(transform.q, m_p);
    aabb->lowerBound.Set(p.x - m_radius, p.y - m_radius);
    aabb->upperBound.Set(p.x + m_radius, p.y + m_radius);
}

void b2CircleShape::ComputeMass(b2MassData *massData, float density) const {
    massData->mass = density * b2_pi * m_radius * m_radius;
    massData->center = m_p;

    // inertia about the local origin
    massData->I = massData->mass * (0.5f * m_radius * m_radius + b2Dot(m_p, m_p));
}


void b2CollideCircles(
        b2Manifold *manifold,
        const b2CircleShape *circleA, const b2Transform &xfA,
        const b2CircleShape *circleB, const b2Transform &xfB) {
    manifold->pointCount = 0;

    b2Vec2 pA = b2Mul(xfA, circleA->m_p);
    b2Vec2 pB = b2Mul(xfB, circleB->m_p);

    b2Vec2 d = pB - pA;
    float distSqr = b2Dot(d, d);
    float rA = circleA->m_radius, rB = circleB->m_radius;
    float radius = rA + rB;
    if (distSqr > radius * radius) {
        return;
    }

    manifold->type = b2Manifold::e_circles;
    manifold->localPoint = circleA->m_p;
    manifold->localNormal.SetZero();
    manifold->pointCount = 1;

    manifold->points[0].localPoint = circleB->m_p;
    manifold->points[0].id.key = 0;
}

void b2CollidePolygonAndCircle(
        b2Manifold *manifold,
        const b2PolygonShape *polygonA, const b2Transform &xfA,
        const b2CircleShape *circleB, const b2Transform &xfB) {
    manifold->pointCount = 0;

    // Compute circle position in the frame of the polygon.
    b2Vec2 c = b2Mul(xfB, circleB->m_p);
    b2Vec2 cLocal = b2MulT(xfA, c);

    // Find the min separating edge.
    int32 normalIndex = 0;
    float separation = -b2_maxFloat;
    float radius = polygonA->m_radius + circleB->m_radius;
    int32 vertexCount = polygonA->m_count;
    const b2Vec2 *vertices = polygonA->m_vertices;
    const b2Vec2 *normals = polygonA->m_normals;

    for (int32 i = 0; i < vertexCount; ++i) {
        float s = b2Dot(normals[i], cLocal - vertices[i]);

        if (s > radius) {
            // Early out.
            return;
        }

        if (s > separation) {
            separation = s;
            normalIndex = i;
        }
    }

    // Vertices that subtend the incident face.
    int32 vertIndex1 = normalIndex;
    int32 vertIndex2 = vertIndex1 + 1 < vertexCount ? vertIndex1 + 1 : 0;
    b2Vec2 v1 = vertices[vertIndex1];
    b2Vec2 v2 = vertices[vertIndex2];

    // If the center is inside the polygon ...
    if (separation < b2_epsilon) {
        manifold->pointCount = 1;
        manifold->type = b2Manifold::e_faceA;
        manifold->localNormal = normals[normalIndex];
        manifold->localPoint = 0.5f * (v1 + v2);
        manifold->points[0].localPoint = circleB->m_p;
        manifold->points[0].id.key = 0;
        return;
    }

    // Compute barycentric coordinates
    float u1 = b2Dot(cLocal - v1, v2 - v1);
    float u2 = b2Dot(cLocal - v2, v1 - v2);
    if (u1 <= 0.0f) {
        if (b2DistanceSquared(cLocal, v1) > radius * radius) {
            return;
        }

        manifold->pointCount = 1;
        manifold->type = b2Manifold::e_faceA;
        manifold->localNormal = cLocal - v1;
        manifold->localNormal.Normalize();
        manifold->localPoint = v1;
        manifold->points[0].localPoint = circleB->m_p;
        manifold->points[0].id.key = 0;
    } else if (u2 <= 0.0f) {
        if (b2DistanceSquared(cLocal, v2) > radius * radius) {
            return;
        }

        manifold->pointCount = 1;
        manifold->type = b2Manifold::e_faceA;
        manifold->localNormal = cLocal - v2;
        manifold->localNormal.Normalize();
        manifold->localPoint = v2;
        manifold->points[0].localPoint = circleB->m_p;
        manifold->points[0].id.key = 0;
    } else {
        b2Vec2 faceCenter = 0.5f * (v1 + v2);
        float s = b2Dot(cLocal - faceCenter, normals[vertIndex1]);
        if (s > radius) {
            return;
        }

        manifold->pointCount = 1;
        manifold->type = b2Manifold::e_faceA;
        manifold->localNormal = normals[vertIndex1];
        manifold->localPoint = faceCenter;
        manifold->points[0].localPoint = circleB->m_p;
        manifold->points[0].id.key = 0;
    }
}


// Compute contact points for edge versus circle.
// This accounts for edge connectivity.
void b2CollideEdgeAndCircle(b2Manifold *manifold,
                            const b2EdgeShape *edgeA, const b2Transform &xfA,
                            const b2CircleShape *circleB, const b2Transform &xfB) {
    manifold->pointCount = 0;

    // Compute circle in frame of edge
    b2Vec2 Q = b2MulT(xfA, b2Mul(xfB, circleB->m_p));

    b2Vec2 A = edgeA->m_vertex1, B = edgeA->m_vertex2;
    b2Vec2 e = B - A;

    // Normal points to the right for a CCW winding
    b2Vec2 n(e.y, -e.x);
    float offset = b2Dot(n, Q - A);

    bool oneSided = edgeA->m_oneSided;
    if (oneSided && offset < 0.0f) {
        return;
    }

    // Barycentric coordinates
    float u = b2Dot(e, B - Q);
    float v = b2Dot(e, Q - A);

    float radius = edgeA->m_radius + circleB->m_radius;

    b2ContactFeature cf;
    cf.indexB = 0;
    cf.typeB = b2ContactFeature::e_vertex;

    // Region A
    if (v <= 0.0f) {
        b2Vec2 P = A;
        b2Vec2 d = Q - P;
        float dd = b2Dot(d, d);
        if (dd > radius * radius) {
            return;
        }

        // Is there an edge connected to A?
        if (edgeA->m_oneSided) {
            b2Vec2 A1 = edgeA->m_vertex0;
            b2Vec2 B1 = A;
            b2Vec2 e1 = B1 - A1;
            float u1 = b2Dot(e1, B1 - Q);

            // Is the circle in Region AB of the previous edge?
            if (u1 > 0.0f) {
                return;
            }
        }

        cf.indexA = 0;
        cf.typeA = b2ContactFeature::e_vertex;
        manifold->pointCount = 1;
        manifold->type = b2Manifold::e_circles;
        manifold->localNormal.SetZero();
        manifold->localPoint = P;
        manifold->points[0].id.key = 0;
        manifold->points[0].id.cf = cf;
        manifold->points[0].localPoint = circleB->m_p;
        return;
    }

    // Region B
    if (u <= 0.0f) {
        b2Vec2 P = B;
        b2Vec2 d = Q - P;
        float dd = b2Dot(d, d);
        if (dd > radius * radius) {
            return;
        }

        // Is there an edge connected to B?
        if (edgeA->m_oneSided) {
            b2Vec2 B2 = edgeA->m_vertex3;
            b2Vec2 A2 = B;
            b2Vec2 e2 = B2 - A2;
            float v2 = b2Dot(e2, Q - A2);

            // Is the circle in Region AB of the next edge?
            if (v2 > 0.0f) {
                return;
            }
        }

        cf.indexA = 1;
        cf.typeA = b2ContactFeature::e_vertex;
        manifold->pointCount = 1;
        manifold->type = b2Manifold::e_circles;
        manifold->localNormal.SetZero();
        manifold->localPoint = P;
        manifold->points[0].id.key = 0;
        manifold->points[0].id.cf = cf;
        manifold->points[0].localPoint = circleB->m_p;
        return;
    }

    // Region AB
    float den = b2Dot(e, e);
    b2Assert(den > 0.0f);
    b2Vec2 P = (1.0f / den) * (u * A + v * B);
    b2Vec2 d = Q - P;
    float dd = b2Dot(d, d);
    if (dd > radius * radius) {
        return;
    }

    if (offset < 0.0f) {
        n.Set(-n.x, -n.y);
    }
    n.Normalize();

    cf.indexA = 0;
    cf.typeA = b2ContactFeature::e_face;
    manifold->pointCount = 1;
    manifold->type = b2Manifold::e_faceA;
    manifold->localNormal = n;
    manifold->localPoint = A;
    manifold->points[0].id.key = 0;
    manifold->points[0].id.cf = cf;
    manifold->points[0].localPoint = circleB->m_p;
}

// This structure is used to keep track of the best separating axis.
struct b2EPAxis
{
    enum Type {
        e_unknown,
        e_edgeA,
        e_edgeB
    };

    b2Vec2 normal;
    Type type;
    int32 index;
    float separation;
};

// This holds polygon B expressed in frame A.
struct b2TempPolygon
{
    b2Vec2 vertices[b2_maxPolygonVertices];
    b2Vec2 normals[b2_maxPolygonVertices];
    int32 count;
};

// Reference face used for clipping
struct b2ReferenceFace
{
    int32 i1, i2;
    b2Vec2 v1, v2;
    b2Vec2 normal;

    b2Vec2 sideNormal1;
    float sideOffset1;

    b2Vec2 sideNormal2;
    float sideOffset2;
};

static b2EPAxis b2ComputeEdgeSeparation(const b2TempPolygon &polygonB, const b2Vec2 &v1, const b2Vec2 &normal1) {
    b2EPAxis axis;
    axis.type = b2EPAxis::e_edgeA;
    axis.index = -1;
    axis.separation = -FLT_MAX;
    axis.normal.SetZero();

    b2Vec2 axes[2] = {normal1, -normal1};

    // Find axis with least overlap (min-max problem)
    for (int32 j = 0; j < 2; ++j) {
        float sj = FLT_MAX;

        // Find deepest polygon vertex along axis j
        for (int32 i = 0; i < polygonB.count; ++i) {
            float si = b2Dot(axes[j], polygonB.vertices[i] - v1);
            if (si < sj) {
                sj = si;
            }
        }

        if (sj > axis.separation) {
            axis.index = j;
            axis.separation = sj;
            axis.normal = axes[j];
        }
    }

    return axis;
}

static b2EPAxis b2ComputePolygonSeparation(const b2TempPolygon &polygonB, const b2Vec2 &v1, const b2Vec2 &v2) {
    b2EPAxis axis;
    axis.type = b2EPAxis::e_unknown;
    axis.index = -1;
    axis.separation = -FLT_MAX;
    axis.normal.SetZero();

    for (int32 i = 0; i < polygonB.count; ++i) {
        b2Vec2 n = -polygonB.normals[i];

        float s1 = b2Dot(n, polygonB.vertices[i] - v1);
        float s2 = b2Dot(n, polygonB.vertices[i] - v2);
        float s = b2Min(s1, s2);

        if (s > axis.separation) {
            axis.type = b2EPAxis::e_edgeB;
            axis.index = i;
            axis.separation = s;
            axis.normal = n;
        }
    }

    return axis;
}

void b2CollideEdgeAndPolygon(b2Manifold *manifold,
                             const b2EdgeShape *edgeA, const b2Transform &xfA,
                             const b2PolygonShape *polygonB, const b2Transform &xfB) {
    manifold->pointCount = 0;

    b2Transform xf = b2MulT(xfA, xfB);

    b2Vec2 centroidB = b2Mul(xf, polygonB->m_centroid);

    b2Vec2 v1 = edgeA->m_vertex1;
    b2Vec2 v2 = edgeA->m_vertex2;

    b2Vec2 edge1 = v2 - v1;
    edge1.Normalize();

    // Normal points to the right for a CCW winding
    b2Vec2 normal1(edge1.y, -edge1.x);
    float offset1 = b2Dot(normal1, centroidB - v1);

    bool oneSided = edgeA->m_oneSided;
    if (oneSided && offset1 < 0.0f) {
        return;
    }

    // Get polygonB in frameA
    b2TempPolygon tempPolygonB;
    tempPolygonB.count = polygonB->m_count;
    for (int32 i = 0; i < polygonB->m_count; ++i) {
        tempPolygonB.vertices[i] = b2Mul(xf, polygonB->m_vertices[i]);
        tempPolygonB.normals[i] = b2Mul(xf.q, polygonB->m_normals[i]);
    }

    float radius = polygonB->m_radius + edgeA->m_radius;

    b2EPAxis edgeAxis = b2ComputeEdgeSeparation(tempPolygonB, v1, normal1);
    if (edgeAxis.separation > radius) {
        return;
    }

    b2EPAxis polygonAxis = b2ComputePolygonSeparation(tempPolygonB, v1, v2);
    if (polygonAxis.separation > radius) {
        return;
    }

    // Use hysteresis for jitter reduction.
    const float k_relativeTol = 0.98f;
    const float k_absoluteTol = 0.001f;

    b2EPAxis primaryAxis;
    if (polygonAxis.separation - radius > k_relativeTol * (edgeAxis.separation - radius) + k_absoluteTol) {
        primaryAxis = polygonAxis;
    } else {
        primaryAxis = edgeAxis;
    }

    if (oneSided) {
        // Smooth collision
        // See https://box2d.org/posts/2020/06/ghost-collisions/

        b2Vec2 edge0 = v1 - edgeA->m_vertex0;
        edge0.Normalize();
        b2Vec2 normal0(edge0.y, -edge0.x);
        bool convex1 = b2Cross(edge0, edge1) >= 0.0f;

        b2Vec2 edge2 = edgeA->m_vertex3 - v2;
        edge2.Normalize();
        b2Vec2 normal2(edge2.y, -edge2.x);
        bool convex2 = b2Cross(edge1, edge2) >= 0.0f;

        const float sinTol = 0.1f;
        bool side1 = b2Dot(primaryAxis.normal, edge1) <= 0.0f;

        // Check Gauss Map
        if (side1) {
            if (convex1) {
                if (b2Cross(primaryAxis.normal, normal0) > sinTol) {
                    // Skip region
                    return;
                }

                // Admit region
            } else {
                // Snap region
                primaryAxis = edgeAxis;
            }
        } else {
            if (convex2) {
                if (b2Cross(normal2, primaryAxis.normal) > sinTol) {
                    // Skip region
                    return;
                }

                // Admit region
            } else {
                // Snap region
                primaryAxis = edgeAxis;
            }
        }
    }

    b2ClipVertex clipPoints[2];
    b2ReferenceFace ref;
    if (primaryAxis.type == b2EPAxis::e_edgeA) {
        manifold->type = b2Manifold::e_faceA;

        // Search for the polygon normal that is most anti-parallel to the edge normal.
        int32 bestIndex = 0;
        float bestValue = b2Dot(primaryAxis.normal, tempPolygonB.normals[0]);
        for (int32 i = 1; i < tempPolygonB.count; ++i) {
            float value = b2Dot(primaryAxis.normal, tempPolygonB.normals[i]);
            if (value < bestValue) {
                bestValue = value;
                bestIndex = i;
            }
        }

        int32 i1 = bestIndex;
        int32 i2 = i1 + 1 < tempPolygonB.count ? i1 + 1 : 0;

        clipPoints[0].v = tempPolygonB.vertices[i1];
        clipPoints[0].id.cf.indexA = 0;
        clipPoints[0].id.cf.indexB = static_cast<uint8>(i1);
        clipPoints[0].id.cf.typeA = b2ContactFeature::e_face;
        clipPoints[0].id.cf.typeB = b2ContactFeature::e_vertex;

        clipPoints[1].v = tempPolygonB.vertices[i2];
        clipPoints[1].id.cf.indexA = 0;
        clipPoints[1].id.cf.indexB = static_cast<uint8>(i2);
        clipPoints[1].id.cf.typeA = b2ContactFeature::e_face;
        clipPoints[1].id.cf.typeB = b2ContactFeature::e_vertex;

        ref.i1 = 0;
        ref.i2 = 1;
        ref.v1 = v1;
        ref.v2 = v2;
        ref.normal = primaryAxis.normal;
        ref.sideNormal1 = -edge1;
        ref.sideNormal2 = edge1;
    } else {
        manifold->type = b2Manifold::e_faceB;

        clipPoints[0].v = v2;
        clipPoints[0].id.cf.indexA = 1;
        clipPoints[0].id.cf.indexB = static_cast<uint8>(primaryAxis.index);
        clipPoints[0].id.cf.typeA = b2ContactFeature::e_vertex;
        clipPoints[0].id.cf.typeB = b2ContactFeature::e_face;

        clipPoints[1].v = v1;
        clipPoints[1].id.cf.indexA = 0;
        clipPoints[1].id.cf.indexB = static_cast<uint8>(primaryAxis.index);
        clipPoints[1].id.cf.typeA = b2ContactFeature::e_vertex;
        clipPoints[1].id.cf.typeB = b2ContactFeature::e_face;

        ref.i1 = primaryAxis.index;
        ref.i2 = ref.i1 + 1 < tempPolygonB.count ? ref.i1 + 1 : 0;
        ref.v1 = tempPolygonB.vertices[ref.i1];
        ref.v2 = tempPolygonB.vertices[ref.i2];
        ref.normal = tempPolygonB.normals[ref.i1];

        // CCW winding
        ref.sideNormal1.Set(ref.normal.y, -ref.normal.x);
        ref.sideNormal2 = -ref.sideNormal1;
    }

    ref.sideOffset1 = b2Dot(ref.sideNormal1, ref.v1);
    ref.sideOffset2 = b2Dot(ref.sideNormal2, ref.v2);

    // Clip incident edge against reference face side planes
    b2ClipVertex clipPoints1[2];
    b2ClipVertex clipPoints2[2];
    int32 np;

    // Clip to side 1
    np = b2ClipSegmentToLine(clipPoints1, clipPoints, ref.sideNormal1, ref.sideOffset1, ref.i1);

    if (np < b2_maxManifoldPoints) {
        return;
    }

    // Clip to side 2
    np = b2ClipSegmentToLine(clipPoints2, clipPoints1, ref.sideNormal2, ref.sideOffset2, ref.i2);

    if (np < b2_maxManifoldPoints) {
        return;
    }

    // Now clipPoints2 contains the clipped points.
    if (primaryAxis.type == b2EPAxis::e_edgeA) {
        manifold->localNormal = ref.normal;
        manifold->localPoint = ref.v1;
    } else {
        manifold->localNormal = polygonB->m_normals[ref.i1];
        manifold->localPoint = polygonB->m_vertices[ref.i1];
    }

    int32 pointCount = 0;
    for (int32 i = 0; i < b2_maxManifoldPoints; ++i) {
        float separation;

        separation = b2Dot(ref.normal, clipPoints2[i].v - ref.v1);

        if (separation <= radius) {
            b2ManifoldPoint *cp = manifold->points + pointCount;

            if (primaryAxis.type == b2EPAxis::e_edgeA) {
                cp->localPoint = b2MulT(xf, clipPoints2[i].v);
                cp->id = clipPoints2[i].id;
            } else {
                cp->localPoint = clipPoints2[i].v;
                cp->id.cf.typeA = clipPoints2[i].id.cf.typeB;
                cp->id.cf.typeB = clipPoints2[i].id.cf.typeA;
                cp->id.cf.indexA = clipPoints2[i].id.cf.indexB;
                cp->id.cf.indexB = clipPoints2[i].id.cf.indexA;
            }

            ++pointCount;
        }
    }

    manifold->pointCount = pointCount;
}


// Find the max separation between poly1 and poly2 using edge normals from poly1.
static float b2FindMaxSeparation(int32 *edgeIndex,
                                 const b2PolygonShape *poly1, const b2Transform &xf1,
                                 const b2PolygonShape *poly2, const b2Transform &xf2) {
    int32 count1 = poly1->m_count;
    int32 count2 = poly2->m_count;
    const b2Vec2 *n1s = poly1->m_normals;
    const b2Vec2 *v1s = poly1->m_vertices;
    const b2Vec2 *v2s = poly2->m_vertices;
    b2Transform xf = b2MulT(xf2, xf1);

    int32 bestIndex = 0;
    float maxSeparation = -b2_maxFloat;
    for (int32 i = 0; i < count1; ++i) {
        // Get poly1 normal in frame2.
        b2Vec2 n = b2Mul(xf.q, n1s[i]);
        b2Vec2 v1 = b2Mul(xf, v1s[i]);

        // Find deepest point for normal i.
        float si = b2_maxFloat;
        for (int32 j = 0; j < count2; ++j) {
            float sij = b2Dot(n, v2s[j] - v1);
            if (sij < si) {
                si = sij;
            }
        }

        if (si > maxSeparation) {
            maxSeparation = si;
            bestIndex = i;
        }
    }

    *edgeIndex = bestIndex;
    return maxSeparation;
}

static void b2FindIncidentEdge(b2ClipVertex c[2],
                               const b2PolygonShape *poly1, const b2Transform &xf1, int32 edge1,
                               const b2PolygonShape *poly2, const b2Transform &xf2) {
    const b2Vec2 *normals1 = poly1->m_normals;

    int32 count2 = poly2->m_count;
    const b2Vec2 *vertices2 = poly2->m_vertices;
    const b2Vec2 *normals2 = poly2->m_normals;

    b2Assert(0 <= edge1 && edge1 < poly1->m_count);

    // Get the normal of the reference edge in poly2's frame.
    b2Vec2 normal1 = b2MulT(xf2.q, b2Mul(xf1.q, normals1[edge1]));

    // Find the incident edge on poly2.
    int32 index = 0;
    float minDot = b2_maxFloat;
    for (int32 i = 0; i < count2; ++i) {
        float dot = b2Dot(normal1, normals2[i]);
        if (dot < minDot) {
            minDot = dot;
            index = i;
        }
    }

    // Build the clip vertices for the incident edge.
    int32 i1 = index;
    int32 i2 = i1 + 1 < count2 ? i1 + 1 : 0;

    c[0].v = b2Mul(xf2, vertices2[i1]);
    c[0].id.cf.indexA = (uint8) edge1;
    c[0].id.cf.indexB = (uint8) i1;
    c[0].id.cf.typeA = b2ContactFeature::e_face;
    c[0].id.cf.typeB = b2ContactFeature::e_vertex;

    c[1].v = b2Mul(xf2, vertices2[i2]);
    c[1].id.cf.indexA = (uint8) edge1;
    c[1].id.cf.indexB = (uint8) i2;
    c[1].id.cf.typeA = b2ContactFeature::e_face;
    c[1].id.cf.typeB = b2ContactFeature::e_vertex;
}

// Find edge normal of max separation on A - return if separating axis is found
// Find edge normal of max separation on B - return if separation axis is found
// Choose reference edge as min(minA, minB)
// Find incident edge
// Clip

// The normal points from 1 to 2
void b2CollidePolygons(b2Manifold *manifold,
                       const b2PolygonShape *polyA, const b2Transform &xfA,
                       const b2PolygonShape *polyB, const b2Transform &xfB) {
    manifold->pointCount = 0;
    float totalRadius = polyA->m_radius + polyB->m_radius;

    int32 edgeA = 0;
    float separationA = b2FindMaxSeparation(&edgeA, polyA, xfA, polyB, xfB);
    if (separationA > totalRadius)
        return;

    int32 edgeB = 0;
    float separationB = b2FindMaxSeparation(&edgeB, polyB, xfB, polyA, xfA);
    if (separationB > totalRadius)
        return;

    const b2PolygonShape *poly1;// reference polygon
    const b2PolygonShape *poly2;// incident polygon
    b2Transform xf1, xf2;
    int32 edge1;// reference edge
    uint8 flip;
    const float k_tol = 0.1f * b2_linearSlop;

    if (separationB > separationA + k_tol) {
        poly1 = polyB;
        poly2 = polyA;
        xf1 = xfB;
        xf2 = xfA;
        edge1 = edgeB;
        manifold->type = b2Manifold::e_faceB;
        flip = 1;
    } else {
        poly1 = polyA;
        poly2 = polyB;
        xf1 = xfA;
        xf2 = xfB;
        edge1 = edgeA;
        manifold->type = b2Manifold::e_faceA;
        flip = 0;
    }

    b2ClipVertex incidentEdge[2];
    b2FindIncidentEdge(incidentEdge, poly1, xf1, edge1, poly2, xf2);

    int32 count1 = poly1->m_count;
    const b2Vec2 *vertices1 = poly1->m_vertices;

    int32 iv1 = edge1;
    int32 iv2 = edge1 + 1 < count1 ? edge1 + 1 : 0;

    b2Vec2 v11 = vertices1[iv1];
    b2Vec2 v12 = vertices1[iv2];

    b2Vec2 localTangent = v12 - v11;
    localTangent.Normalize();

    b2Vec2 localNormal = b2Cross(localTangent, 1.0f);
    b2Vec2 planePoint = 0.5f * (v11 + v12);

    b2Vec2 tangent = b2Mul(xf1.q, localTangent);
    b2Vec2 normal = b2Cross(tangent, 1.0f);

    v11 = b2Mul(xf1, v11);
    v12 = b2Mul(xf1, v12);

    // Face offset.
    float frontOffset = b2Dot(normal, v11);

    // Side offsets, extended by polytope skin thickness.
    float sideOffset1 = -b2Dot(tangent, v11) + totalRadius;
    float sideOffset2 = b2Dot(tangent, v12) + totalRadius;

    // Clip incident edge against extruded edge1 side edges.
    b2ClipVertex clipPoints1[2];
    b2ClipVertex clipPoints2[2];
    int np;

    // Clip to box side 1
    np = b2ClipSegmentToLine(clipPoints1, incidentEdge, -tangent, sideOffset1, iv1);

    if (np < 2)
        return;

    // Clip to negative box side 1
    np = b2ClipSegmentToLine(clipPoints2, clipPoints1, tangent, sideOffset2, iv2);

    if (np < 2) {
        return;
    }

    // Now clipPoints2 contains the clipped points.
    manifold->localNormal = localNormal;
    manifold->localPoint = planePoint;

    int32 pointCount = 0;
    for (int32 i = 0; i < b2_maxManifoldPoints; ++i) {
        float separation = b2Dot(normal, clipPoints2[i].v) - frontOffset;

        if (separation <= totalRadius) {
            b2ManifoldPoint *cp = manifold->points + pointCount;
            cp->localPoint = b2MulT(xf2, clipPoints2[i].v);
            cp->id = clipPoints2[i].id;
            if (flip) {
                // Swap features
                b2ContactFeature cf = cp->id.cf;
                cp->id.cf.indexA = cf.indexB;
                cp->id.cf.indexB = cf.indexA;
                cp->id.cf.typeA = cf.typeB;
                cp->id.cf.typeB = cf.typeA;
            }
            ++pointCount;
        }
    }

    manifold->pointCount = pointCount;
}


void b2WorldManifold::Initialize(const b2Manifold *manifold,
                                 const b2Transform &xfA, float radiusA,
                                 const b2Transform &xfB, float radiusB) {
    if (manifold->pointCount == 0) {
        return;
    }

    switch (manifold->type) {
        case b2Manifold::e_circles: {
            normal.Set(1.0f, 0.0f);
            b2Vec2 pointA = b2Mul(xfA, manifold->localPoint);
            b2Vec2 pointB = b2Mul(xfB, manifold->points[0].localPoint);
            if (b2DistanceSquared(pointA, pointB) > b2_epsilon * b2_epsilon) {
                normal = pointB - pointA;
                normal.Normalize();
            }

            b2Vec2 cA = pointA + radiusA * normal;
            b2Vec2 cB = pointB - radiusB * normal;
            points[0] = 0.5f * (cA + cB);
            separations[0] = b2Dot(cB - cA, normal);
        } break;

        case b2Manifold::e_faceA: {
            normal = b2Mul(xfA.q, manifold->localNormal);
            b2Vec2 planePoint = b2Mul(xfA, manifold->localPoint);

            for (int32 i = 0; i < manifold->pointCount; ++i) {
                b2Vec2 clipPoint = b2Mul(xfB, manifold->points[i].localPoint);
                b2Vec2 cA = clipPoint + (radiusA - b2Dot(clipPoint - planePoint, normal)) * normal;
                b2Vec2 cB = clipPoint - radiusB * normal;
                points[i] = 0.5f * (cA + cB);
                separations[i] = b2Dot(cB - cA, normal);
            }
        } break;

        case b2Manifold::e_faceB: {
            normal = b2Mul(xfB.q, manifold->localNormal);
            b2Vec2 planePoint = b2Mul(xfB, manifold->localPoint);

            for (int32 i = 0; i < manifold->pointCount; ++i) {
                b2Vec2 clipPoint = b2Mul(xfA, manifold->points[i].localPoint);
                b2Vec2 cB = clipPoint + (radiusB - b2Dot(clipPoint - planePoint, normal)) * normal;
                b2Vec2 cA = clipPoint - radiusA * normal;
                points[i] = 0.5f * (cA + cB);
                separations[i] = b2Dot(cA - cB, normal);
            }

            // Ensure normal points from A to B.
            normal = -normal;
        } break;
    }
}

void b2GetPointStates(b2PointState state1[b2_maxManifoldPoints], b2PointState state2[b2_maxManifoldPoints],
                      const b2Manifold *manifold1, const b2Manifold *manifold2) {
    for (int32 i = 0; i < b2_maxManifoldPoints; ++i) {
        state1[i] = b2_nullState;
        state2[i] = b2_nullState;
    }

    // Detect persists and removes.
    for (int32 i = 0; i < manifold1->pointCount; ++i) {
        b2ContactID id = manifold1->points[i].id;

        state1[i] = b2_removeState;

        for (int32 j = 0; j < manifold2->pointCount; ++j) {
            if (manifold2->points[j].id.key == id.key) {
                state1[i] = b2_persistState;
                break;
            }
        }
    }

    // Detect persists and adds.
    for (int32 i = 0; i < manifold2->pointCount; ++i) {
        b2ContactID id = manifold2->points[i].id;

        state2[i] = b2_addState;

        for (int32 j = 0; j < manifold1->pointCount; ++j) {
            if (manifold1->points[j].id.key == id.key) {
                state2[i] = b2_persistState;
                break;
            }
        }
    }
}

// From Real-time Collision Detection, p179.
bool b2AABB::RayCast(b2RayCastOutput *output, const b2RayCastInput &input) const {
    float tmin = -b2_maxFloat;
    float tmax = b2_maxFloat;

    b2Vec2 p = input.p1;
    b2Vec2 d = input.p2 - input.p1;
    b2Vec2 absD = b2Abs(d);

    b2Vec2 normal;

    for (int32 i = 0; i < 2; ++i) {
        if (absD(i) < b2_epsilon) {
            // Parallel.
            if (p(i) < lowerBound(i) || upperBound(i) < p(i)) {
                return false;
            }
        } else {
            float inv_d = 1.0f / d(i);
            float t1 = (lowerBound(i) - p(i)) * inv_d;
            float t2 = (upperBound(i) - p(i)) * inv_d;

            // Sign of the normal vector.
            float s = -1.0f;

            if (t1 > t2) {
                b2Swap(t1, t2);
                s = 1.0f;
            }

            // Push the min up
            if (t1 > tmin) {
                normal.SetZero();
                normal(i) = s;
                tmin = t1;
            }

            // Pull the max down
            tmax = b2Min(tmax, t2);

            if (tmin > tmax) {
                return false;
            }
        }
    }

    // Does the ray start inside the box?
    // Does the ray intersect beyond the max fraction?
    if (tmin < 0.0f || input.maxFraction < tmin) {
        return false;
    }

    // Intersection.
    output->fraction = tmin;
    output->normal = normal;
    return true;
}

// Sutherland-Hodgman clipping.
int32 b2ClipSegmentToLine(b2ClipVertex vOut[2], const b2ClipVertex vIn[2],
                          const b2Vec2 &normal, float offset, int32 vertexIndexA) {
    // Start with no output points
    int32 count = 0;

    // Calculate the distance of end points to the line
    float distance0 = b2Dot(normal, vIn[0].v) - offset;
    float distance1 = b2Dot(normal, vIn[1].v) - offset;

    // If the points are behind the plane
    if (distance0 <= 0.0f) vOut[count++] = vIn[0];
    if (distance1 <= 0.0f) vOut[count++] = vIn[1];

    // If the points are on different sides of the plane
    if (distance0 * distance1 < 0.0f) {
        // Find intersection point of edge and plane
        float interp = distance0 / (distance0 - distance1);
        vOut[count].v = vIn[0].v + interp * (vIn[1].v - vIn[0].v);

        // VertexA is hitting edgeB.
        vOut[count].id.cf.indexA = static_cast<uint8>(vertexIndexA);
        vOut[count].id.cf.indexB = vIn[0].id.cf.indexB;
        vOut[count].id.cf.typeA = b2ContactFeature::e_vertex;
        vOut[count].id.cf.typeB = b2ContactFeature::e_face;
        ++count;

        b2Assert(count == 2);
    }

    return count;
}

bool b2TestOverlap(const b2Shape *shapeA, int32 indexA,
                   const b2Shape *shapeB, int32 indexB,
                   const b2Transform &xfA, const b2Transform &xfB) {
    b2DistanceInput input;
    input.proxyA.Set(shapeA, indexA);
    input.proxyB.Set(shapeB, indexB);
    input.transformA = xfA;
    input.transformB = xfB;
    input.useRadii = true;

    b2SimplexCache cache;
    cache.count = 0;

    b2DistanceOutput output;

    b2Distance(&output, &cache, &input);

    return output.distance < 10.0f * b2_epsilon;
}


// GJK using Voronoi regions (Christer Ericson) and Barycentric coordinates.
int32 b2_gjkCalls, b2_gjkIters, b2_gjkMaxIters;

void b2DistanceProxy::Set(const b2Shape *shape, int32 index) {
    switch (shape->GetType()) {
        case b2Shape::e_circle: {
            const b2CircleShape *circle = static_cast<const b2CircleShape *>(shape);
            m_vertices = &circle->m_p;
            m_count = 1;
            m_radius = circle->m_radius;
        } break;

        case b2Shape::e_polygon: {
            const b2PolygonShape *polygon = static_cast<const b2PolygonShape *>(shape);
            m_vertices = polygon->m_vertices;
            m_count = polygon->m_count;
            m_radius = polygon->m_radius;
        } break;

        case b2Shape::e_chain: {
            const b2ChainShape *chain = static_cast<const b2ChainShape *>(shape);
            b2Assert(0 <= index && index < chain->m_count);

            m_buffer[0] = chain->m_vertices[index];
            if (index + 1 < chain->m_count) {
                m_buffer[1] = chain->m_vertices[index + 1];
            } else {
                m_buffer[1] = chain->m_vertices[0];
            }

            m_vertices = m_buffer;
            m_count = 2;
            m_radius = chain->m_radius;
        } break;

        case b2Shape::e_edge: {
            const b2EdgeShape *edge = static_cast<const b2EdgeShape *>(shape);
            m_vertices = &edge->m_vertex1;
            m_count = 2;
            m_radius = edge->m_radius;
        } break;

        default:
            b2Assert(false);
    }
}

void b2DistanceProxy::Set(const b2Vec2 *vertices, int32 count, float radius) {
    m_vertices = vertices;
    m_count = count;
    m_radius = radius;
}

struct b2SimplexVertex
{
    b2Vec2 wA;   // support point in proxyA
    b2Vec2 wB;   // support point in proxyB
    b2Vec2 w;    // wB - wA
    float a;     // barycentric coordinate for closest point
    int32 indexA;// wA index
    int32 indexB;// wB index
};

struct b2Simplex
{
    void ReadCache(const b2SimplexCache *cache,
                   const b2DistanceProxy *proxyA, const b2Transform &transformA,
                   const b2DistanceProxy *proxyB, const b2Transform &transformB) {
        b2Assert(cache->count <= 3);

        // Copy data from cache.
        m_count = cache->count;
        b2SimplexVertex *vertices = &m_v1;
        for (int32 i = 0; i < m_count; ++i) {
            b2SimplexVertex *v = vertices + i;
            v->indexA = cache->indexA[i];
            v->indexB = cache->indexB[i];
            b2Vec2 wALocal = proxyA->GetVertex(v->indexA);
            b2Vec2 wBLocal = proxyB->GetVertex(v->indexB);
            v->wA = b2Mul(transformA, wALocal);
            v->wB = b2Mul(transformB, wBLocal);
            v->w = v->wB - v->wA;
            v->a = 0.0f;
        }

        // Compute the new simplex metric, if it is substantially different than
        // old metric then flush the simplex.
        if (m_count > 1) {
            float metric1 = cache->metric;
            float metric2 = GetMetric();
            if (metric2 < 0.5f * metric1 || 2.0f * metric1 < metric2 || metric2 < b2_epsilon) {
                // Reset the simplex.
                m_count = 0;
            }
        }

        // If the cache is empty or invalid ...
        if (m_count == 0) {
            b2SimplexVertex *v = vertices + 0;
            v->indexA = 0;
            v->indexB = 0;
            b2Vec2 wALocal = proxyA->GetVertex(0);
            b2Vec2 wBLocal = proxyB->GetVertex(0);
            v->wA = b2Mul(transformA, wALocal);
            v->wB = b2Mul(transformB, wBLocal);
            v->w = v->wB - v->wA;
            v->a = 1.0f;
            m_count = 1;
        }
    }

    void WriteCache(b2SimplexCache *cache) const {
        cache->metric = GetMetric();
        cache->count = uint16(m_count);
        const b2SimplexVertex *vertices = &m_v1;
        for (int32 i = 0; i < m_count; ++i) {
            cache->indexA[i] = uint8(vertices[i].indexA);
            cache->indexB[i] = uint8(vertices[i].indexB);
        }
    }

    b2Vec2 GetSearchDirection() const {
        switch (m_count) {
            case 1:
                return -m_v1.w;

            case 2: {
                b2Vec2 e12 = m_v2.w - m_v1.w;
                float sgn = b2Cross(e12, -m_v1.w);
                if (sgn > 0.0f) {
                    // Origin is left of e12.
                    return b2Cross(1.0f, e12);
                } else {
                    // Origin is right of e12.
                    return b2Cross(e12, 1.0f);
                }
            }

            default:
                b2Assert(false);
                return b2Vec2_zero;
        }
    }

    b2Vec2 GetClosestPoint() const {
        switch (m_count) {
            case 0:
                b2Assert(false);
                return b2Vec2_zero;

            case 1:
                return m_v1.w;

            case 2:
                return m_v1.a * m_v1.w + m_v2.a * m_v2.w;

            case 3:
                return b2Vec2_zero;

            default:
                b2Assert(false);
                return b2Vec2_zero;
        }
    }

    void GetWitnessPoints(b2Vec2 *pA, b2Vec2 *pB) const {
        switch (m_count) {
            case 0:
                b2Assert(false);
                break;

            case 1:
                *pA = m_v1.wA;
                *pB = m_v1.wB;
                break;

            case 2:
                *pA = m_v1.a * m_v1.wA + m_v2.a * m_v2.wA;
                *pB = m_v1.a * m_v1.wB + m_v2.a * m_v2.wB;
                break;

            case 3:
                *pA = m_v1.a * m_v1.wA + m_v2.a * m_v2.wA + m_v3.a * m_v3.wA;
                *pB = *pA;
                break;

            default:
                b2Assert(false);
                break;
        }
    }

    float GetMetric() const {
        switch (m_count) {
            case 0:
                b2Assert(false);
                return 0.0f;

            case 1:
                return 0.0f;

            case 2:
                return b2Distance(m_v1.w, m_v2.w);

            case 3:
                return b2Cross(m_v2.w - m_v1.w, m_v3.w - m_v1.w);

            default:
                b2Assert(false);
                return 0.0f;
        }
    }

    void Solve2();
    void Solve3();

    b2SimplexVertex m_v1, m_v2, m_v3;
    int32 m_count;
};


// Solve a line segment using barycentric coordinates.
//
// p = a1 * w1 + a2 * w2
// a1 + a2 = 1
//
// The vector from the origin to the closest point on the line is
// perpendicular to the line.
// e12 = w2 - w1
// dot(p, e) = 0
// a1 * dot(w1, e) + a2 * dot(w2, e) = 0
//
// 2-by-2 linear system
// [1      1     ][a1] = [1]
// [w1.e12 w2.e12][a2] = [0]
//
// Define
// d12_1 =  dot(w2, e12)
// d12_2 = -dot(w1, e12)
// d12 = d12_1 + d12_2
//
// Solution
// a1 = d12_1 / d12
// a2 = d12_2 / d12
void b2Simplex::Solve2() {
    b2Vec2 w1 = m_v1.w;
    b2Vec2 w2 = m_v2.w;
    b2Vec2 e12 = w2 - w1;

    // w1 region
    float d12_2 = -b2Dot(w1, e12);
    if (d12_2 <= 0.0f) {
        // a2 <= 0, so we clamp it to 0
        m_v1.a = 1.0f;
        m_count = 1;
        return;
    }

    // w2 region
    float d12_1 = b2Dot(w2, e12);
    if (d12_1 <= 0.0f) {
        // a1 <= 0, so we clamp it to 0
        m_v2.a = 1.0f;
        m_count = 1;
        m_v1 = m_v2;
        return;
    }

    // Must be in e12 region.
    float inv_d12 = 1.0f / (d12_1 + d12_2);
    m_v1.a = d12_1 * inv_d12;
    m_v2.a = d12_2 * inv_d12;
    m_count = 2;
}

// Possible regions:
// - points[2]
// - edge points[0]-points[2]
// - edge points[1]-points[2]
// - inside the triangle
void b2Simplex::Solve3() {
    b2Vec2 w1 = m_v1.w;
    b2Vec2 w2 = m_v2.w;
    b2Vec2 w3 = m_v3.w;

    // Edge12
    // [1      1     ][a1] = [1]
    // [w1.e12 w2.e12][a2] = [0]
    // a3 = 0
    b2Vec2 e12 = w2 - w1;
    float w1e12 = b2Dot(w1, e12);
    float w2e12 = b2Dot(w2, e12);
    float d12_1 = w2e12;
    float d12_2 = -w1e12;

    // Edge13
    // [1      1     ][a1] = [1]
    // [w1.e13 w3.e13][a3] = [0]
    // a2 = 0
    b2Vec2 e13 = w3 - w1;
    float w1e13 = b2Dot(w1, e13);
    float w3e13 = b2Dot(w3, e13);
    float d13_1 = w3e13;
    float d13_2 = -w1e13;

    // Edge23
    // [1      1     ][a2] = [1]
    // [w2.e23 w3.e23][a3] = [0]
    // a1 = 0
    b2Vec2 e23 = w3 - w2;
    float w2e23 = b2Dot(w2, e23);
    float w3e23 = b2Dot(w3, e23);
    float d23_1 = w3e23;
    float d23_2 = -w2e23;

    // Triangle123
    float n123 = b2Cross(e12, e13);

    float d123_1 = n123 * b2Cross(w2, w3);
    float d123_2 = n123 * b2Cross(w3, w1);
    float d123_3 = n123 * b2Cross(w1, w2);

    // w1 region
    if (d12_2 <= 0.0f && d13_2 <= 0.0f) {
        m_v1.a = 1.0f;
        m_count = 1;
        return;
    }

    // e12
    if (d12_1 > 0.0f && d12_2 > 0.0f && d123_3 <= 0.0f) {
        float inv_d12 = 1.0f / (d12_1 + d12_2);
        m_v1.a = d12_1 * inv_d12;
        m_v2.a = d12_2 * inv_d12;
        m_count = 2;
        return;
    }

    // e13
    if (d13_1 > 0.0f && d13_2 > 0.0f && d123_2 <= 0.0f) {
        float inv_d13 = 1.0f / (d13_1 + d13_2);
        m_v1.a = d13_1 * inv_d13;
        m_v3.a = d13_2 * inv_d13;
        m_count = 2;
        m_v2 = m_v3;
        return;
    }

    // w2 region
    if (d12_1 <= 0.0f && d23_2 <= 0.0f) {
        m_v2.a = 1.0f;
        m_count = 1;
        m_v1 = m_v2;
        return;
    }

    // w3 region
    if (d13_1 <= 0.0f && d23_1 <= 0.0f) {
        m_v3.a = 1.0f;
        m_count = 1;
        m_v1 = m_v3;
        return;
    }

    // e23
    if (d23_1 > 0.0f && d23_2 > 0.0f && d123_1 <= 0.0f) {
        float inv_d23 = 1.0f / (d23_1 + d23_2);
        m_v2.a = d23_1 * inv_d23;
        m_v3.a = d23_2 * inv_d23;
        m_count = 2;
        m_v1 = m_v3;
        return;
    }

    // Must be in triangle123
    float inv_d123 = 1.0f / (d123_1 + d123_2 + d123_3);
    m_v1.a = d123_1 * inv_d123;
    m_v2.a = d123_2 * inv_d123;
    m_v3.a = d123_3 * inv_d123;
    m_count = 3;
}

void b2Distance(b2DistanceOutput *output,
                b2SimplexCache *cache,
                const b2DistanceInput *input) {
    ++b2_gjkCalls;

    const b2DistanceProxy *proxyA = &input->proxyA;
    const b2DistanceProxy *proxyB = &input->proxyB;

    b2Transform transformA = input->transformA;
    b2Transform transformB = input->transformB;

    // Initialize the simplex.
    b2Simplex simplex;
    simplex.ReadCache(cache, proxyA, transformA, proxyB, transformB);

    // Get simplex vertices as an array.
    b2SimplexVertex *vertices = &simplex.m_v1;
    const int32 k_maxIters = 20;

    // These store the vertices of the last simplex so that we
    // can check for duplicates and prevent cycling.
    int32 saveA[3], saveB[3];
    int32 saveCount = 0;

    // Main iteration loop.
    int32 iter = 0;
    while (iter < k_maxIters) {
        // Copy simplex so we can identify duplicates.
        saveCount = simplex.m_count;
        for (int32 i = 0; i < saveCount; ++i) {
            saveA[i] = vertices[i].indexA;
            saveB[i] = vertices[i].indexB;
        }

        switch (simplex.m_count) {
            case 1:
                break;

            case 2:
                simplex.Solve2();
                break;

            case 3:
                simplex.Solve3();
                break;

            default:
                b2Assert(false);
        }

        // If we have 3 points, then the origin is in the corresponding triangle.
        if (simplex.m_count == 3) {
            break;
        }

        // Get search direction.
        b2Vec2 d = simplex.GetSearchDirection();

        // Ensure the search direction is numerically fit.
        if (d.LengthSquared() < b2_epsilon * b2_epsilon) {
            // The origin is probably contained by a line segment
            // or triangle. Thus the shapes are overlapped.

            // We can't return zero here even though there may be overlap.
            // In case the simplex is a point, segment, or triangle it is difficult
            // to determine if the origin is contained in the CSO or very close to it.
            break;
        }

        // Compute a tentative new simplex vertex using support points.
        b2SimplexVertex *vertex = vertices + simplex.m_count;
        vertex->indexA = proxyA->GetSupport(b2MulT(transformA.q, -d));
        vertex->wA = b2Mul(transformA, proxyA->GetVertex(vertex->indexA));
        vertex->indexB = proxyB->GetSupport(b2MulT(transformB.q, d));
        vertex->wB = b2Mul(transformB, proxyB->GetVertex(vertex->indexB));
        vertex->w = vertex->wB - vertex->wA;

        // Iteration count is equated to the number of support point calls.
        ++iter;
        ++b2_gjkIters;

        // Check for duplicate support points. This is the main termination criteria.
        bool duplicate = false;
        for (int32 i = 0; i < saveCount; ++i) {
            if (vertex->indexA == saveA[i] && vertex->indexB == saveB[i]) {
                duplicate = true;
                break;
            }
        }

        // If we found a duplicate support point we must exit to avoid cycling.
        if (duplicate) {
            break;
        }

        // New vertex is ok and needed.
        ++simplex.m_count;
    }

    b2_gjkMaxIters = b2Max(b2_gjkMaxIters, iter);

    // Prepare output.
    simplex.GetWitnessPoints(&output->pointA, &output->pointB);
    output->distance = b2Distance(output->pointA, output->pointB);
    output->iterations = iter;

    // Cache the simplex.
    simplex.WriteCache(cache);

    // Apply radii if requested
    if (input->useRadii) {
        if (output->distance < b2_epsilon) {
            // Shapes are too close to safely compute normal
            b2Vec2 p = 0.5f * (output->pointA + output->pointB);
            output->pointA = p;
            output->pointB = p;
            output->distance = 0.0f;
        } else {
            // Keep closest points on perimeter even if overlapped, this way
            // the points move smoothly.
            float rA = proxyA->m_radius;
            float rB = proxyB->m_radius;
            b2Vec2 normal = output->pointB - output->pointA;
            normal.Normalize();
            output->distance = b2Max(0.0f, output->distance - rA - rB);
            output->pointA += rA * normal;
            output->pointB -= rB * normal;
        }
    }
}

// GJK-raycast
// Algorithm by Gino van den Bergen.
// "Smooth Mesh Contacts with GJK" in Game Physics Pearls. 2010
bool b2ShapeCast(b2ShapeCastOutput *output, const b2ShapeCastInput *input) {
    output->iterations = 0;
    output->lambda = 1.0f;
    output->normal.SetZero();
    output->point.SetZero();

    const b2DistanceProxy *proxyA = &input->proxyA;
    const b2DistanceProxy *proxyB = &input->proxyB;

    float radiusA = b2Max(proxyA->m_radius, b2_polygonRadius);
    float radiusB = b2Max(proxyB->m_radius, b2_polygonRadius);
    float radius = radiusA + radiusB;

    b2Transform xfA = input->transformA;
    b2Transform xfB = input->transformB;

    b2Vec2 r = input->translationB;
    b2Vec2 n(0.0f, 0.0f);
    float lambda = 0.0f;

    // Initial simplex
    b2Simplex simplex;
    simplex.m_count = 0;

    // Get simplex vertices as an array.
    b2SimplexVertex *vertices = &simplex.m_v1;

    // Get support point in -r direction
    int32 indexA = proxyA->GetSupport(b2MulT(xfA.q, -r));
    b2Vec2 wA = b2Mul(xfA, proxyA->GetVertex(indexA));
    int32 indexB = proxyB->GetSupport(b2MulT(xfB.q, r));
    b2Vec2 wB = b2Mul(xfB, proxyB->GetVertex(indexB));
    b2Vec2 v = wA - wB;

    // Sigma is the target distance between polygons
    float sigma = b2Max(b2_polygonRadius, radius - b2_polygonRadius);
    const float tolerance = 0.5f * b2_linearSlop;

    // Main iteration loop.
    const int32 k_maxIters = 20;
    int32 iter = 0;
    while (iter < k_maxIters && v.Length() - sigma > tolerance) {
        b2Assert(simplex.m_count < 3);

        output->iterations += 1;

        // Support in direction -v (A - B)
        indexA = proxyA->GetSupport(b2MulT(xfA.q, -v));
        wA = b2Mul(xfA, proxyA->GetVertex(indexA));
        indexB = proxyB->GetSupport(b2MulT(xfB.q, v));
        wB = b2Mul(xfB, proxyB->GetVertex(indexB));
        b2Vec2 p = wA - wB;

        // -v is a normal at p
        v.Normalize();

        // Intersect ray with plane
        float vp = b2Dot(v, p);
        float vr = b2Dot(v, r);
        if (vp - sigma > lambda * vr) {
            if (vr <= 0.0f) {
                return false;
            }

            lambda = (vp - sigma) / vr;
            if (lambda > 1.0f) {
                return false;
            }

            n = -v;
            simplex.m_count = 0;
        }

        // Reverse simplex since it works with B - A.
        // Shift by lambda * r because we want the closest point to the current clip point.
        // Note that the support point p is not shifted because we want the plane equation
        // to be formed in unshifted space.
        b2SimplexVertex *vertex = vertices + simplex.m_count;
        vertex->indexA = indexB;
        vertex->wA = wB + lambda * r;
        vertex->indexB = indexA;
        vertex->wB = wA;
        vertex->w = vertex->wB - vertex->wA;
        vertex->a = 1.0f;
        simplex.m_count += 1;

        switch (simplex.m_count) {
            case 1:
                break;

            case 2:
                simplex.Solve2();
                break;

            case 3:
                simplex.Solve3();
                break;

            default:
                b2Assert(false);
        }

        // If we have 3 points, then the origin is in the corresponding triangle.
        if (simplex.m_count == 3) {
            // Overlap
            return false;
        }

        // Get search direction.
        v = simplex.GetClosestPoint();

        // Iteration count is equated to the number of support point calls.
        ++iter;
    }

    if (iter == 0) {
        // Initial overlap
        return false;
    }

    // Prepare output.
    b2Vec2 pointA, pointB;
    simplex.GetWitnessPoints(&pointB, &pointA);

    if (v.LengthSquared() > 0.0f) {
        n = -v;
        n.Normalize();
    }

    output->point = pointA + radiusA * n;
    output->normal = n;
    output->lambda = lambda;
    output->iterations = iter;
    return true;
}


b2DynamicTree::b2DynamicTree() {
    m_root = b2_nullNode;

    m_nodeCapacity = 16;
    m_nodeCount = 0;
    m_nodes = (b2TreeNode *) b2Alloc(m_nodeCapacity * sizeof(b2TreeNode));
    memset(m_nodes, 0, m_nodeCapacity * sizeof(b2TreeNode));

    // Build a linked list for the free list.
    for (int32 i = 0; i < m_nodeCapacity - 1; ++i) {
        m_nodes[i].next = i + 1;
        m_nodes[i].height = -1;
    }
    m_nodes[m_nodeCapacity - 1].next = b2_nullNode;
    m_nodes[m_nodeCapacity - 1].height = -1;
    m_freeList = 0;

    m_insertionCount = 0;
}

b2DynamicTree::~b2DynamicTree() {
    // This frees the entire tree in one shot.
    b2Free(m_nodes);
}

// Allocate a node from the pool. Grow the pool if necessary.
int32 b2DynamicTree::AllocateNode() {
    // Expand the node pool as needed.
    if (m_freeList == b2_nullNode) {
        b2Assert(m_nodeCount == m_nodeCapacity);

        // The free list is empty. Rebuild a bigger pool.
        b2TreeNode *oldNodes = m_nodes;
        m_nodeCapacity *= 2;
        m_nodes = (b2TreeNode *) b2Alloc(m_nodeCapacity * sizeof(b2TreeNode));
        memcpy(m_nodes, oldNodes, m_nodeCount * sizeof(b2TreeNode));
        b2Free(oldNodes);

        // Build a linked list for the free list. The parent
        // pointer becomes the "next" pointer.
        for (int32 i = m_nodeCount; i < m_nodeCapacity - 1; ++i) {
            m_nodes[i].next = i + 1;
            m_nodes[i].height = -1;
        }
        m_nodes[m_nodeCapacity - 1].next = b2_nullNode;
        m_nodes[m_nodeCapacity - 1].height = -1;
        m_freeList = m_nodeCount;
    }

    // Peel a node off the free list.
    int32 nodeId = m_freeList;
    m_freeList = m_nodes[nodeId].next;
    m_nodes[nodeId].parent = b2_nullNode;
    m_nodes[nodeId].child1 = b2_nullNode;
    m_nodes[nodeId].child2 = b2_nullNode;
    m_nodes[nodeId].height = 0;
    m_nodes[nodeId].userData = nullptr;
    m_nodes[nodeId].moved = false;
    ++m_nodeCount;
    return nodeId;
}

// Return a node to the pool.
void b2DynamicTree::FreeNode(int32 nodeId) {
    b2Assert(0 <= nodeId && nodeId < m_nodeCapacity);
    b2Assert(0 < m_nodeCount);
    m_nodes[nodeId].next = m_freeList;
    m_nodes[nodeId].height = -1;
    m_freeList = nodeId;
    --m_nodeCount;
}

// Create a proxy in the tree as a leaf node. We return the index
// of the node instead of a pointer so that we can grow
// the node pool.
int32 b2DynamicTree::CreateProxy(const b2AABB &aabb, void *userData) {
    int32 proxyId = AllocateNode();

    // Fatten the aabb.
    b2Vec2 r(b2_aabbExtension, b2_aabbExtension);
    m_nodes[proxyId].aabb.lowerBound = aabb.lowerBound - r;
    m_nodes[proxyId].aabb.upperBound = aabb.upperBound + r;
    m_nodes[proxyId].userData = userData;
    m_nodes[proxyId].height = 0;
    m_nodes[proxyId].moved = true;

    InsertLeaf(proxyId);

    return proxyId;
}

void b2DynamicTree::DestroyProxy(int32 proxyId) {
    b2Assert(0 <= proxyId && proxyId < m_nodeCapacity);
    b2Assert(m_nodes[proxyId].IsLeaf());

    RemoveLeaf(proxyId);
    FreeNode(proxyId);
}

bool b2DynamicTree::MoveProxy(int32 proxyId, const b2AABB &aabb, const b2Vec2 &displacement) {
    b2Assert(0 <= proxyId && proxyId < m_nodeCapacity);

    b2Assert(m_nodes[proxyId].IsLeaf());

    // Extend AABB
    b2AABB fatAABB;
    b2Vec2 r(b2_aabbExtension, b2_aabbExtension);
    fatAABB.lowerBound = aabb.lowerBound - r;
    fatAABB.upperBound = aabb.upperBound + r;

    // Predict AABB movement
    b2Vec2 d = b2_aabbMultiplier * displacement;

    if (d.x < 0.0f) {
        fatAABB.lowerBound.x += d.x;
    } else {
        fatAABB.upperBound.x += d.x;
    }

    if (d.y < 0.0f) {
        fatAABB.lowerBound.y += d.y;
    } else {
        fatAABB.upperBound.y += d.y;
    }

    const b2AABB &treeAABB = m_nodes[proxyId].aabb;
    if (treeAABB.Contains(aabb)) {
        // The tree AABB still contains the object, but it might be too large.
        // Perhaps the object was moving fast but has since gone to sleep.
        // The huge AABB is larger than the new fat AABB.
        b2AABB hugeAABB;
        hugeAABB.lowerBound = fatAABB.lowerBound - 4.0f * r;
        hugeAABB.upperBound = fatAABB.upperBound + 4.0f * r;

        if (hugeAABB.Contains(treeAABB)) {
            // The tree AABB contains the object AABB and the tree AABB is
            // not too large. No tree update needed.
            return false;
        }

        // Otherwise the tree AABB is huge and needs to be shrunk
    }

    RemoveLeaf(proxyId);

    m_nodes[proxyId].aabb = fatAABB;

    InsertLeaf(proxyId);

    m_nodes[proxyId].moved = true;

    return true;
}

void b2DynamicTree::InsertLeaf(int32 leaf) {
    ++m_insertionCount;

    if (m_root == b2_nullNode) {
        m_root = leaf;
        m_nodes[m_root].parent = b2_nullNode;
        return;
    }

    // Find the best sibling for this node
    b2AABB leafAABB = m_nodes[leaf].aabb;
    int32 index = m_root;
    while (m_nodes[index].IsLeaf() == false) {
        int32 child1 = m_nodes[index].child1;
        int32 child2 = m_nodes[index].child2;

        float area = m_nodes[index].aabb.GetPerimeter();

        b2AABB combinedAABB;
        combinedAABB.Combine(m_nodes[index].aabb, leafAABB);
        float combinedArea = combinedAABB.GetPerimeter();

        // Cost of creating a new parent for this node and the new leaf
        float cost = 2.0f * combinedArea;

        // Minimum cost of pushing the leaf further down the tree
        float inheritanceCost = 2.0f * (combinedArea - area);

        // Cost of descending into child1
        float cost1;
        if (m_nodes[child1].IsLeaf()) {
            b2AABB aabb;
            aabb.Combine(leafAABB, m_nodes[child1].aabb);
            cost1 = aabb.GetPerimeter() + inheritanceCost;
        } else {
            b2AABB aabb;
            aabb.Combine(leafAABB, m_nodes[child1].aabb);
            float oldArea = m_nodes[child1].aabb.GetPerimeter();
            float newArea = aabb.GetPerimeter();
            cost1 = (newArea - oldArea) + inheritanceCost;
        }

        // Cost of descending into child2
        float cost2;
        if (m_nodes[child2].IsLeaf()) {
            b2AABB aabb;
            aabb.Combine(leafAABB, m_nodes[child2].aabb);
            cost2 = aabb.GetPerimeter() + inheritanceCost;
        } else {
            b2AABB aabb;
            aabb.Combine(leafAABB, m_nodes[child2].aabb);
            float oldArea = m_nodes[child2].aabb.GetPerimeter();
            float newArea = aabb.GetPerimeter();
            cost2 = newArea - oldArea + inheritanceCost;
        }

        // Descend according to the minimum cost.
        if (cost < cost1 && cost < cost2) {
            break;
        }

        // Descend
        if (cost1 < cost2) {
            index = child1;
        } else {
            index = child2;
        }
    }

    int32 sibling = index;

    // Create a new parent.
    int32 oldParent = m_nodes[sibling].parent;
    int32 newParent = AllocateNode();
    m_nodes[newParent].parent = oldParent;
    m_nodes[newParent].userData = nullptr;
    m_nodes[newParent].aabb.Combine(leafAABB, m_nodes[sibling].aabb);
    m_nodes[newParent].height = m_nodes[sibling].height + 1;

    if (oldParent != b2_nullNode) {
        // The sibling was not the root.
        if (m_nodes[oldParent].child1 == sibling) {
            m_nodes[oldParent].child1 = newParent;
        } else {
            m_nodes[oldParent].child2 = newParent;
        }

        m_nodes[newParent].child1 = sibling;
        m_nodes[newParent].child2 = leaf;
        m_nodes[sibling].parent = newParent;
        m_nodes[leaf].parent = newParent;
    } else {
        // The sibling was the root.
        m_nodes[newParent].child1 = sibling;
        m_nodes[newParent].child2 = leaf;
        m_nodes[sibling].parent = newParent;
        m_nodes[leaf].parent = newParent;
        m_root = newParent;
    }

    // Walk back up the tree fixing heights and AABBs
    index = m_nodes[leaf].parent;
    while (index != b2_nullNode) {
        index = Balance(index);

        int32 child1 = m_nodes[index].child1;
        int32 child2 = m_nodes[index].child2;

        b2Assert(child1 != b2_nullNode);
        b2Assert(child2 != b2_nullNode);

        m_nodes[index].height = 1 + b2Max(m_nodes[child1].height, m_nodes[child2].height);
        m_nodes[index].aabb.Combine(m_nodes[child1].aabb, m_nodes[child2].aabb);

        index = m_nodes[index].parent;
    }

    //Validate();
}

void b2DynamicTree::RemoveLeaf(int32 leaf) {
    if (leaf == m_root) {
        m_root = b2_nullNode;
        return;
    }

    int32 parent = m_nodes[leaf].parent;
    int32 grandParent = m_nodes[parent].parent;
    int32 sibling;
    if (m_nodes[parent].child1 == leaf) {
        sibling = m_nodes[parent].child2;
    } else {
        sibling = m_nodes[parent].child1;
    }

    if (grandParent != b2_nullNode) {
        // Destroy parent and connect sibling to grandParent.
        if (m_nodes[grandParent].child1 == parent) {
            m_nodes[grandParent].child1 = sibling;
        } else {
            m_nodes[grandParent].child2 = sibling;
        }
        m_nodes[sibling].parent = grandParent;
        FreeNode(parent);

        // Adjust ancestor bounds.
        int32 index = grandParent;
        while (index != b2_nullNode) {
            index = Balance(index);

            int32 child1 = m_nodes[index].child1;
            int32 child2 = m_nodes[index].child2;

            m_nodes[index].aabb.Combine(m_nodes[child1].aabb, m_nodes[child2].aabb);
            m_nodes[index].height = 1 + b2Max(m_nodes[child1].height, m_nodes[child2].height);

            index = m_nodes[index].parent;
        }
    } else {
        m_root = sibling;
        m_nodes[sibling].parent = b2_nullNode;
        FreeNode(parent);
    }

    //Validate();
}

// Perform a left or right rotation if node A is imbalanced.
// Returns the new root index.
int32 b2DynamicTree::Balance(int32 iA) {
    b2Assert(iA != b2_nullNode);

    b2TreeNode *A = m_nodes + iA;
    if (A->IsLeaf() || A->height < 2) {
        return iA;
    }

    int32 iB = A->child1;
    int32 iC = A->child2;
    b2Assert(0 <= iB && iB < m_nodeCapacity);
    b2Assert(0 <= iC && iC < m_nodeCapacity);

    b2TreeNode *B = m_nodes + iB;
    b2TreeNode *C = m_nodes + iC;

    int32 balance = C->height - B->height;

    // Rotate C up
    if (balance > 1) {
        int32 iF = C->child1;
        int32 iG = C->child2;
        b2TreeNode *F = m_nodes + iF;
        b2TreeNode *G = m_nodes + iG;
        b2Assert(0 <= iF && iF < m_nodeCapacity);
        b2Assert(0 <= iG && iG < m_nodeCapacity);

        // Swap A and C
        C->child1 = iA;
        C->parent = A->parent;
        A->parent = iC;

        // A's old parent should point to C
        if (C->parent != b2_nullNode) {
            if (m_nodes[C->parent].child1 == iA) {
                m_nodes[C->parent].child1 = iC;
            } else {
                b2Assert(m_nodes[C->parent].child2 == iA);
                m_nodes[C->parent].child2 = iC;
            }
        } else {
            m_root = iC;
        }

        // Rotate
        if (F->height > G->height) {
            C->child2 = iF;
            A->child2 = iG;
            G->parent = iA;
            A->aabb.Combine(B->aabb, G->aabb);
            C->aabb.Combine(A->aabb, F->aabb);

            A->height = 1 + b2Max(B->height, G->height);
            C->height = 1 + b2Max(A->height, F->height);
        } else {
            C->child2 = iG;
            A->child2 = iF;
            F->parent = iA;
            A->aabb.Combine(B->aabb, F->aabb);
            C->aabb.Combine(A->aabb, G->aabb);

            A->height = 1 + b2Max(B->height, F->height);
            C->height = 1 + b2Max(A->height, G->height);
        }

        return iC;
    }

    // Rotate B up
    if (balance < -1) {
        int32 iD = B->child1;
        int32 iE = B->child2;
        b2TreeNode *D = m_nodes + iD;
        b2TreeNode *E = m_nodes + iE;
        b2Assert(0 <= iD && iD < m_nodeCapacity);
        b2Assert(0 <= iE && iE < m_nodeCapacity);

        // Swap A and B
        B->child1 = iA;
        B->parent = A->parent;
        A->parent = iB;

        // A's old parent should point to B
        if (B->parent != b2_nullNode) {
            if (m_nodes[B->parent].child1 == iA) {
                m_nodes[B->parent].child1 = iB;
            } else {
                b2Assert(m_nodes[B->parent].child2 == iA);
                m_nodes[B->parent].child2 = iB;
            }
        } else {
            m_root = iB;
        }

        // Rotate
        if (D->height > E->height) {
            B->child2 = iD;
            A->child1 = iE;
            E->parent = iA;
            A->aabb.Combine(C->aabb, E->aabb);
            B->aabb.Combine(A->aabb, D->aabb);

            A->height = 1 + b2Max(C->height, E->height);
            B->height = 1 + b2Max(A->height, D->height);
        } else {
            B->child2 = iE;
            A->child1 = iD;
            D->parent = iA;
            A->aabb.Combine(C->aabb, D->aabb);
            B->aabb.Combine(A->aabb, E->aabb);

            A->height = 1 + b2Max(C->height, D->height);
            B->height = 1 + b2Max(A->height, E->height);
        }

        return iB;
    }

    return iA;
}

int32 b2DynamicTree::GetHeight() const {
    if (m_root == b2_nullNode) {
        return 0;
    }

    return m_nodes[m_root].height;
}

//
float b2DynamicTree::GetAreaRatio() const {
    if (m_root == b2_nullNode) {
        return 0.0f;
    }

    const b2TreeNode *root = m_nodes + m_root;
    float rootArea = root->aabb.GetPerimeter();

    float totalArea = 0.0f;
    for (int32 i = 0; i < m_nodeCapacity; ++i) {
        const b2TreeNode *node = m_nodes + i;
        if (node->height < 0) {
            // Free node in pool
            continue;
        }

        totalArea += node->aabb.GetPerimeter();
    }

    return totalArea / rootArea;
}

// Compute the height of a sub-tree.
int32 b2DynamicTree::ComputeHeight(int32 nodeId) const {
    b2Assert(0 <= nodeId && nodeId < m_nodeCapacity);
    b2TreeNode *node = m_nodes + nodeId;

    if (node->IsLeaf()) {
        return 0;
    }

    int32 height1 = ComputeHeight(node->child1);
    int32 height2 = ComputeHeight(node->child2);
    return 1 + b2Max(height1, height2);
}

int32 b2DynamicTree::ComputeHeight() const {
    int32 height = ComputeHeight(m_root);
    return height;
}

void b2DynamicTree::ValidateStructure(int32 index) const {
    if (index == b2_nullNode) {
        return;
    }

    if (index == m_root) {
        b2Assert(m_nodes[index].parent == b2_nullNode);
    }

    const b2TreeNode *node = m_nodes + index;

    int32 child1 = node->child1;
    int32 child2 = node->child2;

    if (node->IsLeaf()) {
        b2Assert(child1 == b2_nullNode);
        b2Assert(child2 == b2_nullNode);
        b2Assert(node->height == 0);
        return;
    }

    b2Assert(0 <= child1 && child1 < m_nodeCapacity);
    b2Assert(0 <= child2 && child2 < m_nodeCapacity);

    b2Assert(m_nodes[child1].parent == index);
    b2Assert(m_nodes[child2].parent == index);

    ValidateStructure(child1);
    ValidateStructure(child2);
}

void b2DynamicTree::ValidateMetrics(int32 index) const {
    if (index == b2_nullNode) {
        return;
    }

    const b2TreeNode *node = m_nodes + index;

    int32 child1 = node->child1;
    int32 child2 = node->child2;

    if (node->IsLeaf()) {
        b2Assert(child1 == b2_nullNode);
        b2Assert(child2 == b2_nullNode);
        b2Assert(node->height == 0);
        return;
    }

    b2Assert(0 <= child1 && child1 < m_nodeCapacity);
    b2Assert(0 <= child2 && child2 < m_nodeCapacity);

    int32 height1 = m_nodes[child1].height;
    int32 height2 = m_nodes[child2].height;
    int32 height;
    height = 1 + b2Max(height1, height2);
    b2Assert(node->height == height);

    b2AABB aabb;
    aabb.Combine(m_nodes[child1].aabb, m_nodes[child2].aabb);

    b2Assert(aabb.lowerBound == node->aabb.lowerBound);
    b2Assert(aabb.upperBound == node->aabb.upperBound);

    ValidateMetrics(child1);
    ValidateMetrics(child2);
}

void b2DynamicTree::Validate() const {
#if defined(b2DEBUG)
    ValidateStructure(m_root);
    ValidateMetrics(m_root);

    int32 freeCount = 0;
    int32 freeIndex = m_freeList;
    while (freeIndex != b2_nullNode) {
        b2Assert(0 <= freeIndex && freeIndex < m_nodeCapacity);
        freeIndex = m_nodes[freeIndex].next;
        ++freeCount;
    }

    b2Assert(GetHeight() == ComputeHeight());

    b2Assert(m_nodeCount + freeCount == m_nodeCapacity);
#endif
}

int32 b2DynamicTree::GetMaxBalance() const {
    int32 maxBalance = 0;
    for (int32 i = 0; i < m_nodeCapacity; ++i) {
        const b2TreeNode *node = m_nodes + i;
        if (node->height <= 1) {
            continue;
        }

        b2Assert(node->IsLeaf() == false);

        int32 child1 = node->child1;
        int32 child2 = node->child2;
        int32 balance = b2Abs(m_nodes[child2].height - m_nodes[child1].height);
        maxBalance = b2Max(maxBalance, balance);
    }

    return maxBalance;
}

void b2DynamicTree::RebuildBottomUp() {
    int32 *nodes = (int32 *) b2Alloc(m_nodeCount * sizeof(int32));
    int32 count = 0;

    // Build array of leaves. Free the rest.
    for (int32 i = 0; i < m_nodeCapacity; ++i) {
        if (m_nodes[i].height < 0) {
            // free node in pool
            continue;
        }

        if (m_nodes[i].IsLeaf()) {
            m_nodes[i].parent = b2_nullNode;
            nodes[count] = i;
            ++count;
        } else {
            FreeNode(i);
        }
    }

    while (count > 1) {
        float minCost = b2_maxFloat;
        int32 iMin = -1, jMin = -1;
        for (int32 i = 0; i < count; ++i) {
            b2AABB aabbi = m_nodes[nodes[i]].aabb;

            for (int32 j = i + 1; j < count; ++j) {
                b2AABB aabbj = m_nodes[nodes[j]].aabb;
                b2AABB b;
                b.Combine(aabbi, aabbj);
                float cost = b.GetPerimeter();
                if (cost < minCost) {
                    iMin = i;
                    jMin = j;
                    minCost = cost;
                }
            }
        }

        int32 index1 = nodes[iMin];
        int32 index2 = nodes[jMin];
        b2TreeNode *child1 = m_nodes + index1;
        b2TreeNode *child2 = m_nodes + index2;

        int32 parentIndex = AllocateNode();
        b2TreeNode *parent = m_nodes + parentIndex;
        parent->child1 = index1;
        parent->child2 = index2;
        parent->height = 1 + b2Max(child1->height, child2->height);
        parent->aabb.Combine(child1->aabb, child2->aabb);
        parent->parent = b2_nullNode;

        child1->parent = parentIndex;
        child2->parent = parentIndex;

        nodes[jMin] = nodes[count - 1];
        nodes[iMin] = parentIndex;
        --count;
    }

    m_root = nodes[0];
    b2Free(nodes);

    Validate();
}

void b2DynamicTree::ShiftOrigin(const b2Vec2 &newOrigin) {
    // Build array of leaves. Free the rest.
    for (int32 i = 0; i < m_nodeCapacity; ++i) {
        m_nodes[i].aabb.lowerBound -= newOrigin;
        m_nodes[i].aabb.upperBound -= newOrigin;
    }
}


void b2EdgeShape::SetOneSided(const b2Vec2 &v0, const b2Vec2 &v1, const b2Vec2 &v2, const b2Vec2 &v3) {
    m_vertex0 = v0;
    m_vertex1 = v1;
    m_vertex2 = v2;
    m_vertex3 = v3;
    m_oneSided = true;
}

void b2EdgeShape::SetTwoSided(const b2Vec2 &v1, const b2Vec2 &v2) {
    m_vertex1 = v1;
    m_vertex2 = v2;
    m_oneSided = false;
}

b2Shape *b2EdgeShape::Clone(b2BlockAllocator *allocator) const {
    void *mem = allocator->Allocate(sizeof(b2EdgeShape));
    b2EdgeShape *clone = new (mem) b2EdgeShape;
    *clone = *this;
    return clone;
}

int32 b2EdgeShape::GetChildCount() const {
    return 1;
}

bool b2EdgeShape::TestPoint(const b2Transform &xf, const b2Vec2 &p) const {
    B2_NOT_USED(xf);
    B2_NOT_USED(p);
    return false;
}

// p = p1 + t * d
// v = v1 + s * e
// p1 + t * d = v1 + s * e
// s * e - t * d = p1 - v1
bool b2EdgeShape::RayCast(b2RayCastOutput *output, const b2RayCastInput &input,
                          const b2Transform &xf, int32 childIndex) const {
    B2_NOT_USED(childIndex);

    // Put the ray into the edge's frame of reference.
    b2Vec2 p1 = b2MulT(xf.q, input.p1 - xf.p);
    b2Vec2 p2 = b2MulT(xf.q, input.p2 - xf.p);
    b2Vec2 d = p2 - p1;

    b2Vec2 v1 = m_vertex1;
    b2Vec2 v2 = m_vertex2;
    b2Vec2 e = v2 - v1;

    // Normal points to the right, looking from v1 at v2
    b2Vec2 normal(e.y, -e.x);
    normal.Normalize();

    // q = p1 + t * d
    // dot(normal, q - v1) = 0
    // dot(normal, p1 - v1) + t * dot(normal, d) = 0
    float numerator = b2Dot(normal, v1 - p1);
    if (m_oneSided && numerator > 0.0f) {
        return false;
    }

    float denominator = b2Dot(normal, d);

    if (denominator == 0.0f) {
        return false;
    }

    float t = numerator / denominator;
    if (t < 0.0f || input.maxFraction < t) {
        return false;
    }

    b2Vec2 q = p1 + t * d;

    // q = v1 + s * r
    // s = dot(q - v1, r) / dot(r, r)
    b2Vec2 r = v2 - v1;
    float rr = b2Dot(r, r);
    if (rr == 0.0f) {
        return false;
    }

    float s = b2Dot(q - v1, r) / rr;
    if (s < 0.0f || 1.0f < s) {
        return false;
    }

    output->fraction = t;
    if (numerator > 0.0f) {
        output->normal = -b2Mul(xf.q, normal);
    } else {
        output->normal = b2Mul(xf.q, normal);
    }
    return true;
}

void b2EdgeShape::ComputeAABB(b2AABB *aabb, const b2Transform &xf, int32 childIndex) const {
    B2_NOT_USED(childIndex);

    b2Vec2 v1 = b2Mul(xf, m_vertex1);
    b2Vec2 v2 = b2Mul(xf, m_vertex2);

    b2Vec2 lower = b2Min(v1, v2);
    b2Vec2 upper = b2Max(v1, v2);

    b2Vec2 r(m_radius, m_radius);
    aabb->lowerBound = lower - r;
    aabb->upperBound = upper + r;
}

void b2EdgeShape::ComputeMass(b2MassData *massData, float density) const {
    B2_NOT_USED(density);

    massData->mass = 0.0f;
    massData->center = 0.5f * (m_vertex1 + m_vertex2);
    massData->I = 0.0f;
}


b2Shape *b2PolygonShape::Clone(b2BlockAllocator *allocator) const {
    void *mem = allocator->Allocate(sizeof(b2PolygonShape));
    b2PolygonShape *clone = new (mem) b2PolygonShape;
    *clone = *this;
    return clone;
}

void b2PolygonShape::SetAsBox(float hx, float hy) {
    m_count = 4;
    m_vertices[0].Set(-hx, -hy);
    m_vertices[1].Set(hx, -hy);
    m_vertices[2].Set(hx, hy);
    m_vertices[3].Set(-hx, hy);
    m_normals[0].Set(0.0f, -1.0f);
    m_normals[1].Set(1.0f, 0.0f);
    m_normals[2].Set(0.0f, 1.0f);
    m_normals[3].Set(-1.0f, 0.0f);
    m_centroid.SetZero();
}

void b2PolygonShape::SetAsBox(float hx, float hy, const b2Vec2 &center, float angle) {
    m_count = 4;
    m_vertices[0].Set(-hx, -hy);
    m_vertices[1].Set(hx, -hy);
    m_vertices[2].Set(hx, hy);
    m_vertices[3].Set(-hx, hy);
    m_normals[0].Set(0.0f, -1.0f);
    m_normals[1].Set(1.0f, 0.0f);
    m_normals[2].Set(0.0f, 1.0f);
    m_normals[3].Set(-1.0f, 0.0f);
    m_centroid = center;

    b2Transform xf;
    xf.p = center;
    xf.q.Set(angle);

    // Transform vertices and normals.
    for (int32 i = 0; i < m_count; ++i) {
        m_vertices[i] = b2Mul(xf, m_vertices[i]);
        m_normals[i] = b2Mul(xf.q, m_normals[i]);
    }
}

int32 b2PolygonShape::GetChildCount() const {
    return 1;
}

static b2Vec2 ComputeCentroid(const b2Vec2 *vs, int32 count) {
    b2Assert(count >= 3);

    b2Vec2 c(0.0f, 0.0f);
    float area = 0.0f;

    // Get a reference point for forming triangles.
    // Use the first vertex to reduce round-off errors.
    b2Vec2 s = vs[0];

    const float inv3 = 1.0f / 3.0f;

    for (int32 i = 0; i < count; ++i) {
        // Triangle vertices.
        b2Vec2 p1 = vs[0] - s;
        b2Vec2 p2 = vs[i] - s;
        b2Vec2 p3 = i + 1 < count ? vs[i + 1] - s : vs[0] - s;

        b2Vec2 e1 = p2 - p1;
        b2Vec2 e2 = p3 - p1;

        float D = b2Cross(e1, e2);

        float triangleArea = 0.5f * D;
        area += triangleArea;

        // Area weighted centroid
        c += triangleArea * inv3 * (p1 + p2 + p3);
    }

    // Centroid
    b2Assert(area > b2_epsilon);
    c = (1.0f / area) * c + s;
    return c;
}

void b2PolygonShape::Set(const b2Vec2 *vertices, int32 count) {
    b2Assert(3 <= count && count <= b2_maxPolygonVertices);
    if (count < 3) {
        SetAsBox(1.0f, 1.0f);
        return;
    }

    int32 n = b2Min(count, b2_maxPolygonVertices);

    // Perform welding and copy vertices into local buffer.
    b2Vec2 ps[b2_maxPolygonVertices];
    int32 tempCount = 0;
    for (int32 i = 0; i < n; ++i) {
        b2Vec2 v = vertices[i];

        bool unique = true;
        for (int32 j = 0; j < tempCount; ++j) {
            if (b2DistanceSquared(v, ps[j]) < ((0.5f * b2_linearSlop) * (0.5f * b2_linearSlop))) {
                unique = false;
                break;
            }
        }

        if (unique) {
            ps[tempCount++] = v;
        }
    }

    n = tempCount;
    if (n < 3) {
        // Polygon is degenerate.
        b2Assert(false);
        SetAsBox(1.0f, 1.0f);
        return;
    }

    // Create the convex hull using the Gift wrapping algorithm
    // http://en.wikipedia.org/wiki/Gift_wrapping_algorithm

    // Find the right most point on the hull
    int32 i0 = 0;
    float x0 = ps[0].x;
    for (int32 i = 1; i < n; ++i) {
        float x = ps[i].x;
        if (x > x0 || (x == x0 && ps[i].y < ps[i0].y)) {
            i0 = i;
            x0 = x;
        }
    }

    int32 hull[b2_maxPolygonVertices];
    int32 m = 0;
    int32 ih = i0;

    for (;;) {
        b2Assert(m < b2_maxPolygonVertices);
        hull[m] = ih;

        int32 ie = 0;
        for (int32 j = 1; j < n; ++j) {
            if (ie == ih) {
                ie = j;
                continue;
            }

            b2Vec2 r = ps[ie] - ps[hull[m]];
            b2Vec2 v = ps[j] - ps[hull[m]];
            float c = b2Cross(r, v);
            if (c < 0.0f) {
                ie = j;
            }

            // Collinearity check
            if (c == 0.0f && v.LengthSquared() > r.LengthSquared()) {
                ie = j;
            }
        }

        ++m;
        ih = ie;

        if (ie == i0) {
            break;
        }
    }

    if (m < 3) {
        // Polygon is degenerate.
        b2Assert(false);
        SetAsBox(1.0f, 1.0f);
        return;
    }

    m_count = m;

    // Copy vertices.
    for (int32 i = 0; i < m; ++i) {
        m_vertices[i] = ps[hull[i]];
    }

    // Compute normals. Ensure the edges have non-zero length.
    for (int32 i = 0; i < m; ++i) {
        int32 i1 = i;
        int32 i2 = i + 1 < m ? i + 1 : 0;
        b2Vec2 edge = m_vertices[i2] - m_vertices[i1];
        b2Assert(edge.LengthSquared() > b2_epsilon * b2_epsilon);
        m_normals[i] = b2Cross(edge, 1.0f);
        m_normals[i].Normalize();
    }

    // Compute the polygon centroid.
    m_centroid = ComputeCentroid(m_vertices, m);
}

bool b2PolygonShape::TestPoint(const b2Transform &xf, const b2Vec2 &p) const {
    b2Vec2 pLocal = b2MulT(xf.q, p - xf.p);

    for (int32 i = 0; i < m_count; ++i) {
        float dot = b2Dot(m_normals[i], pLocal - m_vertices[i]);
        if (dot > 0.0f) {
            return false;
        }
    }

    return true;
}

bool b2PolygonShape::RayCast(b2RayCastOutput *output, const b2RayCastInput &input,
                             const b2Transform &xf, int32 childIndex) const {
    B2_NOT_USED(childIndex);

    // Put the ray into the polygon's frame of reference.
    b2Vec2 p1 = b2MulT(xf.q, input.p1 - xf.p);
    b2Vec2 p2 = b2MulT(xf.q, input.p2 - xf.p);
    b2Vec2 d = p2 - p1;

    float lower = 0.0f, upper = input.maxFraction;

    int32 index = -1;

    for (int32 i = 0; i < m_count; ++i) {
        // p = p1 + a * d
        // dot(normal, p - v) = 0
        // dot(normal, p1 - v) + a * dot(normal, d) = 0
        float numerator = b2Dot(m_normals[i], m_vertices[i] - p1);
        float denominator = b2Dot(m_normals[i], d);

        if (denominator == 0.0f) {
            if (numerator < 0.0f) {
                return false;
            }
        } else {
            // Note: we want this predicate without division:
            // lower < numerator / denominator, where denominator < 0
            // Since denominator < 0, we have to flip the inequality:
            // lower < numerator / denominator <==> denominator * lower > numerator.
            if (denominator < 0.0f && numerator < lower * denominator) {
                // Increase lower.
                // The segment enters this half-space.
                lower = numerator / denominator;
                index = i;
            } else if (denominator > 0.0f && numerator < upper * denominator) {
                // Decrease upper.
                // The segment exits this half-space.
                upper = numerator / denominator;
            }
        }

        // The use of epsilon here causes the assert on lower to trip
        // in some cases. Apparently the use of epsilon was to make edge
        // shapes work, but now those are handled separately.
        //if (upper < lower - b2_epsilon)
        if (upper < lower) {
            return false;
        }
    }

    b2Assert(0.0f <= lower && lower <= input.maxFraction);

    if (index >= 0) {
        output->fraction = lower;
        output->normal = b2Mul(xf.q, m_normals[index]);
        return true;
    }

    return false;
}

void b2PolygonShape::ComputeAABB(b2AABB *aabb, const b2Transform &xf, int32 childIndex) const {
    B2_NOT_USED(childIndex);

    b2Vec2 lower = b2Mul(xf, m_vertices[0]);
    b2Vec2 upper = lower;

    for (int32 i = 1; i < m_count; ++i) {
        b2Vec2 v = b2Mul(xf, m_vertices[i]);
        lower = b2Min(lower, v);
        upper = b2Max(upper, v);
    }

    b2Vec2 r(m_radius, m_radius);
    aabb->lowerBound = lower - r;
    aabb->upperBound = upper + r;
}

void b2PolygonShape::ComputeMass(b2MassData *massData, float density) const {
    // Polygon mass, centroid, and inertia.
    // Let rho be the polygon density in mass per unit area.
    // Then:
    // mass = rho * int(dA)
    // centroid.x = (1/mass) * rho * int(x * dA)
    // centroid.y = (1/mass) * rho * int(y * dA)
    // I = rho * int((x*x + y*y) * dA)
    //
    // We can compute these integrals by summing all the integrals
    // for each triangle of the polygon. To evaluate the integral
    // for a single triangle, we make a change of variables to
    // the (u,v) coordinates of the triangle:
    // x = x0 + e1x * u + e2x * v
    // y = y0 + e1y * u + e2y * v
    // where 0 <= u && 0 <= v && u + v <= 1.
    //
    // We integrate u from [0,1-v] and then v from [0,1].
    // We also need to use the Jacobian of the transformation:
    // D = cross(e1, e2)
    //
    // Simplification: triangle centroid = (1/3) * (p1 + p2 + p3)
    //
    // The rest of the derivation is handled by computer algebra.

    b2Assert(m_count >= 3);

    b2Vec2 center(0.0f, 0.0f);
    float area = 0.0f;
    float I = 0.0f;

    // Get a reference point for forming triangles.
    // Use the first vertex to reduce round-off errors.
    b2Vec2 s = m_vertices[0];

    const float k_inv3 = 1.0f / 3.0f;

    for (int32 i = 0; i < m_count; ++i) {
        // Triangle vertices.
        b2Vec2 e1 = m_vertices[i] - s;
        b2Vec2 e2 = i + 1 < m_count ? m_vertices[i + 1] - s : m_vertices[0] - s;

        float D = b2Cross(e1, e2);

        float triangleArea = 0.5f * D;
        area += triangleArea;

        // Area weighted centroid
        center += triangleArea * k_inv3 * (e1 + e2);

        float ex1 = e1.x, ey1 = e1.y;
        float ex2 = e2.x, ey2 = e2.y;

        float intx2 = ex1 * ex1 + ex2 * ex1 + ex2 * ex2;
        float inty2 = ey1 * ey1 + ey2 * ey1 + ey2 * ey2;

        I += (0.25f * k_inv3 * D) * (intx2 + inty2);
    }

    // Total mass
    massData->mass = density * area;

    // Center of mass
    b2Assert(area > b2_epsilon);
    center *= 1.0f / area;
    massData->center = center + s;

    // Inertia tensor relative to the local origin (point s).
    massData->I = density * I;

    // Shift to center of mass then to original body origin.
    massData->I += massData->mass * (b2Dot(massData->center, massData->center) - b2Dot(center, center));
}

bool b2PolygonShape::Validate() const {
    for (int32 i = 0; i < m_count; ++i) {
        int32 i1 = i;
        int32 i2 = i < m_count - 1 ? i1 + 1 : 0;
        b2Vec2 p = m_vertices[i1];
        b2Vec2 e = m_vertices[i2] - p;

        for (int32 j = 0; j < m_count; ++j) {
            if (j == i1 || j == i2) {
                continue;
            }

            b2Vec2 v = m_vertices[j] - p;
            float c = b2Cross(e, v);
            if (c < 0.0f) {
                return false;
            }
        }
    }

    return true;
}


float b2_toiTime, b2_toiMaxTime;
int32 b2_toiCalls, b2_toiIters, b2_toiMaxIters;
int32 b2_toiRootIters, b2_toiMaxRootIters;

//
struct b2SeparationFunction
{
    enum Type {
        e_points,
        e_faceA,
        e_faceB
    };

    // TODO_ERIN might not need to return the separation

    float Initialize(const b2SimplexCache *cache,
                     const b2DistanceProxy *proxyA, const b2Sweep &sweepA,
                     const b2DistanceProxy *proxyB, const b2Sweep &sweepB,
                     float t1) {
        m_proxyA = proxyA;
        m_proxyB = proxyB;
        int32 count = cache->count;
        b2Assert(0 < count && count < 3);

        m_sweepA = sweepA;
        m_sweepB = sweepB;

        b2Transform xfA, xfB;
        m_sweepA.GetTransform(&xfA, t1);
        m_sweepB.GetTransform(&xfB, t1);

        if (count == 1) {
            m_type = e_points;
            b2Vec2 localPointA = m_proxyA->GetVertex(cache->indexA[0]);
            b2Vec2 localPointB = m_proxyB->GetVertex(cache->indexB[0]);
            b2Vec2 pointA = b2Mul(xfA, localPointA);
            b2Vec2 pointB = b2Mul(xfB, localPointB);
            m_axis = pointB - pointA;
            float s = m_axis.Normalize();
            return s;
        } else if (cache->indexA[0] == cache->indexA[1]) {
            // Two points on B and one on A.
            m_type = e_faceB;
            b2Vec2 localPointB1 = proxyB->GetVertex(cache->indexB[0]);
            b2Vec2 localPointB2 = proxyB->GetVertex(cache->indexB[1]);

            m_axis = b2Cross(localPointB2 - localPointB1, 1.0f);
            m_axis.Normalize();
            b2Vec2 normal = b2Mul(xfB.q, m_axis);

            m_localPoint = 0.5f * (localPointB1 + localPointB2);
            b2Vec2 pointB = b2Mul(xfB, m_localPoint);

            b2Vec2 localPointA = proxyA->GetVertex(cache->indexA[0]);
            b2Vec2 pointA = b2Mul(xfA, localPointA);

            float s = b2Dot(pointA - pointB, normal);
            if (s < 0.0f) {
                m_axis = -m_axis;
                s = -s;
            }
            return s;
        } else {
            // Two points on A and one or two points on B.
            m_type = e_faceA;
            b2Vec2 localPointA1 = m_proxyA->GetVertex(cache->indexA[0]);
            b2Vec2 localPointA2 = m_proxyA->GetVertex(cache->indexA[1]);

            m_axis = b2Cross(localPointA2 - localPointA1, 1.0f);
            m_axis.Normalize();
            b2Vec2 normal = b2Mul(xfA.q, m_axis);

            m_localPoint = 0.5f * (localPointA1 + localPointA2);
            b2Vec2 pointA = b2Mul(xfA, m_localPoint);

            b2Vec2 localPointB = m_proxyB->GetVertex(cache->indexB[0]);
            b2Vec2 pointB = b2Mul(xfB, localPointB);

            float s = b2Dot(pointB - pointA, normal);
            if (s < 0.0f) {
                m_axis = -m_axis;
                s = -s;
            }
            return s;
        }
    }

    //
    float FindMinSeparation(int32 *indexA, int32 *indexB, float t) const {
        b2Transform xfA, xfB;
        m_sweepA.GetTransform(&xfA, t);
        m_sweepB.GetTransform(&xfB, t);

        switch (m_type) {
            case e_points: {
                b2Vec2 axisA = b2MulT(xfA.q, m_axis);
                b2Vec2 axisB = b2MulT(xfB.q, -m_axis);

                *indexA = m_proxyA->GetSupport(axisA);
                *indexB = m_proxyB->GetSupport(axisB);

                b2Vec2 localPointA = m_proxyA->GetVertex(*indexA);
                b2Vec2 localPointB = m_proxyB->GetVertex(*indexB);

                b2Vec2 pointA = b2Mul(xfA, localPointA);
                b2Vec2 pointB = b2Mul(xfB, localPointB);

                float separation = b2Dot(pointB - pointA, m_axis);
                return separation;
            }

            case e_faceA: {
                b2Vec2 normal = b2Mul(xfA.q, m_axis);
                b2Vec2 pointA = b2Mul(xfA, m_localPoint);

                b2Vec2 axisB = b2MulT(xfB.q, -normal);

                *indexA = -1;
                *indexB = m_proxyB->GetSupport(axisB);

                b2Vec2 localPointB = m_proxyB->GetVertex(*indexB);
                b2Vec2 pointB = b2Mul(xfB, localPointB);

                float separation = b2Dot(pointB - pointA, normal);
                return separation;
            }

            case e_faceB: {
                b2Vec2 normal = b2Mul(xfB.q, m_axis);
                b2Vec2 pointB = b2Mul(xfB, m_localPoint);

                b2Vec2 axisA = b2MulT(xfA.q, -normal);

                *indexB = -1;
                *indexA = m_proxyA->GetSupport(axisA);

                b2Vec2 localPointA = m_proxyA->GetVertex(*indexA);
                b2Vec2 pointA = b2Mul(xfA, localPointA);

                float separation = b2Dot(pointA - pointB, normal);
                return separation;
            }

            default:
                b2Assert(false);
                *indexA = -1;
                *indexB = -1;
                return 0.0f;
        }
    }

    //
    float Evaluate(int32 indexA, int32 indexB, float t) const {
        b2Transform xfA, xfB;
        m_sweepA.GetTransform(&xfA, t);
        m_sweepB.GetTransform(&xfB, t);

        switch (m_type) {
            case e_points: {
                b2Vec2 localPointA = m_proxyA->GetVertex(indexA);
                b2Vec2 localPointB = m_proxyB->GetVertex(indexB);

                b2Vec2 pointA = b2Mul(xfA, localPointA);
                b2Vec2 pointB = b2Mul(xfB, localPointB);
                float separation = b2Dot(pointB - pointA, m_axis);

                return separation;
            }

            case e_faceA: {
                b2Vec2 normal = b2Mul(xfA.q, m_axis);
                b2Vec2 pointA = b2Mul(xfA, m_localPoint);

                b2Vec2 localPointB = m_proxyB->GetVertex(indexB);
                b2Vec2 pointB = b2Mul(xfB, localPointB);

                float separation = b2Dot(pointB - pointA, normal);
                return separation;
            }

            case e_faceB: {
                b2Vec2 normal = b2Mul(xfB.q, m_axis);
                b2Vec2 pointB = b2Mul(xfB, m_localPoint);

                b2Vec2 localPointA = m_proxyA->GetVertex(indexA);
                b2Vec2 pointA = b2Mul(xfA, localPointA);

                float separation = b2Dot(pointA - pointB, normal);
                return separation;
            }

            default:
                b2Assert(false);
                return 0.0f;
        }
    }

    const b2DistanceProxy *m_proxyA;
    const b2DistanceProxy *m_proxyB;
    b2Sweep m_sweepA, m_sweepB;
    Type m_type;
    b2Vec2 m_localPoint;
    b2Vec2 m_axis;
};

// CCD via the local separating axis method. This seeks progression
// by computing the largest time at which separation is maintained.
void b2TimeOfImpact(b2TOIOutput *output, const b2TOIInput *input) {
    b2Timer timer;

    ++b2_toiCalls;

    output->state = b2TOIOutput::e_unknown;
    output->t = input->tMax;

    const b2DistanceProxy *proxyA = &input->proxyA;
    const b2DistanceProxy *proxyB = &input->proxyB;

    b2Sweep sweepA = input->sweepA;
    b2Sweep sweepB = input->sweepB;

    // Large rotations can make the root finder fail, so we normalize the
    // sweep angles.
    sweepA.Normalize();
    sweepB.Normalize();

    float tMax = input->tMax;

    float totalRadius = proxyA->m_radius + proxyB->m_radius;
    float target = b2Max(b2_linearSlop, totalRadius - 3.0f * b2_linearSlop);
    float tolerance = 0.25f * b2_linearSlop;
    b2Assert(target > tolerance);

    float t1 = 0.0f;
    const int32 k_maxIterations = 20;// TODO_ERIN b2Settings
    int32 iter = 0;

    // Prepare input for distance query.
    b2SimplexCache cache;
    cache.count = 0;
    b2DistanceInput distanceInput;
    distanceInput.proxyA = input->proxyA;
    distanceInput.proxyB = input->proxyB;
    distanceInput.useRadii = false;

    // The outer loop progressively attempts to compute new separating axes.
    // This loop terminates when an axis is repeated (no progress is made).
    for (;;) {
        b2Transform xfA, xfB;
        sweepA.GetTransform(&xfA, t1);
        sweepB.GetTransform(&xfB, t1);

        // Get the distance between shapes. We can also use the results
        // to get a separating axis.
        distanceInput.transformA = xfA;
        distanceInput.transformB = xfB;
        b2DistanceOutput distanceOutput;
        b2Distance(&distanceOutput, &cache, &distanceInput);

        // If the shapes are overlapped, we give up on continuous collision.
        if (distanceOutput.distance <= 0.0f) {
            // Failure!
            output->state = b2TOIOutput::e_overlapped;
            output->t = 0.0f;
            break;
        }

        if (distanceOutput.distance < target + tolerance) {
            // Victory!
            output->state = b2TOIOutput::e_touching;
            output->t = t1;
            break;
        }

        // Initialize the separating axis.
        b2SeparationFunction fcn;
        fcn.Initialize(&cache, proxyA, sweepA, proxyB, sweepB, t1);
#if 0
		// Dump the curve seen by the root finder
		{
			const int32 N = 100;
			float dx = 1.0f / N;
			float xs[N+1];
			float fs[N+1];

			float x = 0.0f;

			for (int32 i = 0; i <= N; ++i)
			{
				sweepA.GetTransform(&xfA, x);
				sweepB.GetTransform(&xfB, x);
				float f = fcn.Evaluate(xfA, xfB) - target;

				printf("%g %g\n", x, f);

				xs[i] = x;
				fs[i] = f;

				x += dx;
			}
		}
#endif

        // Compute the TOI on the separating axis. We do this by successively
        // resolving the deepest point. This loop is bounded by the number of vertices.
        bool done = false;
        float t2 = tMax;
        int32 pushBackIter = 0;
        for (;;) {
            // Find the deepest point at t2. Store the witness point indices.
            int32 indexA, indexB;
            float s2 = fcn.FindMinSeparation(&indexA, &indexB, t2);

            // Is the final configuration separated?
            if (s2 > target + tolerance) {
                // Victory!
                output->state = b2TOIOutput::e_separated;
                output->t = tMax;
                done = true;
                break;
            }

            // Has the separation reached tolerance?
            if (s2 > target - tolerance) {
                // Advance the sweeps
                t1 = t2;
                break;
            }

            // Compute the initial separation of the witness points.
            float s1 = fcn.Evaluate(indexA, indexB, t1);

            // Check for initial overlap. This might happen if the root finder
            // runs out of iterations.
            if (s1 < target - tolerance) {
                output->state = b2TOIOutput::e_failed;
                output->t = t1;
                done = true;
                break;
            }

            // Check for touching
            if (s1 <= target + tolerance) {
                // Victory! t1 should hold the TOI (could be 0.0).
                output->state = b2TOIOutput::e_touching;
                output->t = t1;
                done = true;
                break;
            }

            // Compute 1D root of: f(x) - target = 0
            int32 rootIterCount = 0;
            float a1 = t1, a2 = t2;
            for (;;) {
                // Use a mix of the secant rule and bisection.
                float t;
                if (rootIterCount & 1) {
                    // Secant rule to improve convergence.
                    t = a1 + (target - s1) * (a2 - a1) / (s2 - s1);
                } else {
                    // Bisection to guarantee progress.
                    t = 0.5f * (a1 + a2);
                }

                ++rootIterCount;
                ++b2_toiRootIters;

                float s = fcn.Evaluate(indexA, indexB, t);

                if (b2Abs(s - target) < tolerance) {
                    // t2 holds a tentative value for t1
                    t2 = t;
                    break;
                }

                // Ensure we continue to bracket the root.
                if (s > target) {
                    a1 = t;
                    s1 = s;
                } else {
                    a2 = t;
                    s2 = s;
                }

                if (rootIterCount == 50) {
                    break;
                }
            }

            b2_toiMaxRootIters = b2Max(b2_toiMaxRootIters, rootIterCount);

            ++pushBackIter;

            if (pushBackIter == b2_maxPolygonVertices) {
                break;
            }
        }

        ++iter;
        ++b2_toiIters;

        if (done) {
            break;
        }

        if (iter == k_maxIterations) {
            // Root finder got stuck. Semi-victory.
            output->state = b2TOIOutput::e_failed;
            output->t = t1;
            break;
        }
    }

    b2_toiMaxIters = b2Max(b2_toiMaxIters, iter);

    float time = timer.GetMilliseconds();
    b2_toiMaxTime = b2Max(b2_toiMaxTime, time);
    b2_toiTime += time;
}
