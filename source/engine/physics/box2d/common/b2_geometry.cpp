// Metadot physics engine is enhanced based on box2d modification
// Metadot code Copyright(c) 2022-2023, KaoruXun All rights reserved.
// Box2d code by Erin Catto licensed under the MIT License
// https://github.com/erincatto/box2d

// MIT License
// Copyright (c) 2022-2023 KaoruXun
// Copyright (c) 2019 Erin Catto

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "engine/physics/box2d/inc/b2_geometry.h"

#include "engine/physics/box2d/inc/b2_edge_shape.h"

void CreateLinks(b2Body* b, b2FixtureDef* fd, const b2Vec2* vertices, int32 count, bool closed, const b2Vec2 prevVertex, const b2Vec2 nextVertex) {
    b2Assert(closed ? count >= 3 : count >= 2);

    const b2Shape* oldShape = fd->shape;

    for (int32 i = 1; i < count; ++i) {
        // If the code crashes here, it means your vertices are too close together.
        b2Assert(b2DistanceSquared(vertices[i - 1], vertices[i]) > b2_linearSlop * b2_linearSlop);

        b2Vec2 v0 = (i > 1) ? vertices[i - 2] : prevVertex;
        b2Vec2 v3 = (i < count - 1) ? vertices[i + 1] : nextVertex;

        b2EdgeShape shape;
        shape.SetOneSided(v0, vertices[i - 1], vertices[i], v3);

        fd->shape = &shape;
        b->CreateFixture(fd);
    }

    if (closed) {
        b2EdgeShape shape;
        shape.SetOneSided(vertices[count - 2], vertices[count - 1], vertices[0], vertices[1]);

        fd->shape = &shape;
        b->CreateFixture(fd);
    }

    fd->shape = oldShape;
}

void b2CreateLoop(b2Body* b, b2FixtureDef* fd, const b2Vec2* vertices, int32 count) { CreateLinks(b, fd, vertices, count, true, vertices[count - 1], vertices[0]); }

void b2CreateChain(b2Body* b, b2FixtureDef* fd, const b2Vec2* vertices, int32 count, const b2Vec2& prevVertex, const b2Vec2& nextVertex) {
    CreateLinks(b, fd, vertices, count, false, prevVertex, nextVertex);
}

void b2CreateLoop(b2Body* b, const b2Vec2* vertices, int32 count) {
    b2FixtureDef fd;
    CreateLinks(b, &fd, vertices, count, true, vertices[count - 1], vertices[0]);
}

void b2CreateChain(b2Body* b, const b2Vec2* vertices, int32 count, const b2Vec2& prevVertex, const b2Vec2& nextVertex) {
    b2FixtureDef fd;
    CreateLinks(b, &fd, vertices, count, false, prevVertex, nextVertex);
}
