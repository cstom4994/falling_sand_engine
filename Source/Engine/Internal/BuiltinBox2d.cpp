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

#include "BuiltinBox2d.h"
#include "Engine/Renderer/RendererGPU.h"

#include <new>
#include <stdio.h>

b2Body::b2Body(const b2BodyDef *bd, b2World *world) {
    METADOT_ASSERT_E(bd->position.IsValid());
    METADOT_ASSERT_E(bd->linearVelocity.IsValid());
    METADOT_ASSERT_E(b2IsValid(bd->angle));
    METADOT_ASSERT_E(b2IsValid(bd->angularVelocity));
    METADOT_ASSERT_E(b2IsValid(bd->angularDamping) && bd->angularDamping >= 0.0f);
    METADOT_ASSERT_E(b2IsValid(bd->linearDamping) && bd->linearDamping >= 0.0f);

    m_flags = 0;

    if (bd->bullet) { m_flags |= e_bulletFlag; }
    if (bd->fixedRotation) { m_flags |= e_fixedRotationFlag; }
    if (bd->allowSleep) { m_flags |= e_autoSleepFlag; }
    if (bd->awake && bd->type != b2_staticBody) { m_flags |= e_awakeFlag; }
    if (bd->enabled) { m_flags |= e_enabledFlag; }

    m_world = world;

    m_xf.p = bd->position;
    m_xf.q.Set(bd->angle);

    m_sweep.localCenter.SetZero();
    m_sweep.c0 = m_xf.p;
    m_sweep.c = m_xf.p;
    m_sweep.a0 = bd->angle;
    m_sweep.a = bd->angle;
    m_sweep.alpha0 = 0.0f;

    m_jointList = nullptr;
    m_contactList = nullptr;
    m_prev = nullptr;
    m_next = nullptr;

    m_linearVelocity = bd->linearVelocity;
    m_angularVelocity = bd->angularVelocity;

    m_linearDamping = bd->linearDamping;
    m_angularDamping = bd->angularDamping;
    m_gravityScale = bd->gravityScale;

    m_force.SetZero();
    m_torque = 0.0f;

    m_sleepTime = 0.0f;

    m_type = bd->type;

    m_mass = 0.0f;
    m_invMass = 0.0f;

    m_I = 0.0f;
    m_invI = 0.0f;

    m_userData = bd->userData;

    m_fixtureList = nullptr;
    m_fixtureCount = 0;
}

b2Body::~b2Body() {
    // shapes and joints are destroyed in b2World::Destroy
}

void b2Body::SetType(b2BodyType type) {
    METADOT_ASSERT_E(m_world->IsLocked() == false);
    if (m_world->IsLocked() == true) { return; }

    if (m_type == type) { return; }

    m_type = type;

    ResetMassData();

    if (m_type == b2_staticBody) {
        m_linearVelocity.SetZero();
        m_angularVelocity = 0.0f;
        m_sweep.a0 = m_sweep.a;
        m_sweep.c0 = m_sweep.c;
        m_flags &= ~e_awakeFlag;
        SynchronizeFixtures();
    }

    SetAwake(true);

    m_force.SetZero();
    m_torque = 0.0f;

    // Delete the attached contacts.
    b2ContactEdge *ce = m_contactList;
    while (ce) {
        b2ContactEdge *ce0 = ce;
        ce = ce->next;
        m_world->m_contactManager.Destroy(ce0->contact);
    }
    m_contactList = nullptr;

    // Touch the proxies so that new contacts will be created (when appropriate)
    b2BroadPhase *broadPhase = &m_world->m_contactManager.m_broadPhase;
    for (b2Fixture *f = m_fixtureList; f; f = f->m_next) {
        int32 proxyCount = f->m_proxyCount;
        for (int32 i = 0; i < proxyCount; ++i) { broadPhase->TouchProxy(f->m_proxies[i].proxyId); }
    }
}

b2Fixture *b2Body::CreateFixture(const b2FixtureDef *def) {
    METADOT_ASSERT_E(m_world->IsLocked() == false);
    if (m_world->IsLocked() == true) { return nullptr; }

    b2BlockAllocator *allocator = &m_world->m_blockAllocator;

    void *memory = allocator->Allocate(sizeof(b2Fixture));
    b2Fixture *fixture = new (memory) b2Fixture;
    fixture->Create(allocator, this, def);

    if (m_flags & e_enabledFlag) {
        b2BroadPhase *broadPhase = &m_world->m_contactManager.m_broadPhase;
        fixture->CreateProxies(broadPhase, m_xf);
    }

    fixture->m_next = m_fixtureList;
    m_fixtureList = fixture;
    ++m_fixtureCount;

    fixture->m_body = this;

    // Adjust mass properties if needed.
    if (fixture->m_density > 0.0f) { ResetMassData(); }

    // Let the world know we have a new fixture. This will cause new contacts
    // to be created at the beginning of the next time step.
    m_world->m_newContacts = true;

    return fixture;
}

b2Fixture *b2Body::CreateFixture(const b2Shape *shape, float density) {
    b2FixtureDef def;
    def.shape = shape;
    def.density = density;

    return CreateFixture(&def);
}

void b2Body::DestroyFixture(b2Fixture *fixture) {
    if (fixture == NULL) { return; }

    METADOT_ASSERT_E(m_world->IsLocked() == false);
    if (m_world->IsLocked() == true) { return; }

    METADOT_ASSERT_E(fixture->m_body == this);

    // Remove the fixture from this body's singly linked list.
    METADOT_ASSERT_E(m_fixtureCount > 0);
    b2Fixture **node = &m_fixtureList;
    bool found = false;
    while (*node != nullptr) {
        if (*node == fixture) {
            *node = fixture->m_next;
            found = true;
            break;
        }

        node = &(*node)->m_next;
    }

    // You tried to remove a shape that is not attached to this body.
    METADOT_ASSERT_E(found);

    const float density = fixture->m_density;

    // Destroy any contacts associated with the fixture.
    b2ContactEdge *edge = m_contactList;
    while (edge) {
        b2Contact *c = edge->contact;
        edge = edge->next;

        b2Fixture *fixtureA = c->GetFixtureA();
        b2Fixture *fixtureB = c->GetFixtureB();

        if (fixture == fixtureA || fixture == fixtureB) {
            // This destroys the contact and removes it from
            // this body's contact list.
            m_world->m_contactManager.Destroy(c);
        }
    }

    b2BlockAllocator *allocator = &m_world->m_blockAllocator;

    if (m_flags & e_enabledFlag) {
        b2BroadPhase *broadPhase = &m_world->m_contactManager.m_broadPhase;
        fixture->DestroyProxies(broadPhase);
    }

    fixture->m_body = nullptr;
    fixture->m_next = nullptr;
    fixture->Destroy(allocator);
    fixture->~b2Fixture();
    allocator->Free(fixture, sizeof(b2Fixture));

    --m_fixtureCount;

    // Reset the mass data
    if (density > 0.0f) { ResetMassData(); }
}

void b2Body::ResetMassData() {
    // Compute mass data from shapes. Each shape has its own density.
    m_mass = 0.0f;
    m_invMass = 0.0f;
    m_I = 0.0f;
    m_invI = 0.0f;
    m_sweep.localCenter.SetZero();

    // Static and kinematic bodies have zero mass.
    if (m_type == b2_staticBody || m_type == b2_kinematicBody) {
        m_sweep.c0 = m_xf.p;
        m_sweep.c = m_xf.p;
        m_sweep.a0 = m_sweep.a;
        return;
    }

    METADOT_ASSERT_E(m_type == b2_dynamicBody);

    // Accumulate mass over all fixtures.
    b2Vec2 localCenter = b2Vec2_zero;
    for (b2Fixture *f = m_fixtureList; f; f = f->m_next) {
        if (f->m_density == 0.0f) { continue; }

        b2MassData massData;
        f->GetMassData(&massData);
        m_mass += massData.mass;
        localCenter += massData.mass * massData.center;
        m_I += massData.I;
    }

    // Compute center of mass.
    if (m_mass > 0.0f) {
        m_invMass = 1.0f / m_mass;
        localCenter *= m_invMass;
    }

    if (m_I > 0.0f && (m_flags & e_fixedRotationFlag) == 0) {
        // Center the inertia about the center of mass.
        m_I -= m_mass * b2Dot(localCenter, localCenter);
        METADOT_ASSERT_E(m_I > 0.0f);
        m_invI = 1.0f / m_I;

    } else {
        m_I = 0.0f;
        m_invI = 0.0f;
    }

    // Move center of mass.
    b2Vec2 oldCenter = m_sweep.c;
    m_sweep.localCenter = localCenter;
    m_sweep.c0 = m_sweep.c = b2Mul(m_xf, m_sweep.localCenter);

    // Update center of mass velocity.
    m_linearVelocity += b2Cross(m_angularVelocity, m_sweep.c - oldCenter);
}

void b2Body::SetMassData(const b2MassData *massData) {
    METADOT_ASSERT_E(m_world->IsLocked() == false);
    if (m_world->IsLocked() == true) { return; }

    if (m_type != b2_dynamicBody) { return; }

    m_invMass = 0.0f;
    m_I = 0.0f;
    m_invI = 0.0f;

    m_mass = massData->mass;
    if (m_mass <= 0.0f) { m_mass = 1.0f; }

    m_invMass = 1.0f / m_mass;

    if (massData->I > 0.0f && (m_flags & b2Body::e_fixedRotationFlag) == 0) {
        m_I = massData->I - m_mass * b2Dot(massData->center, massData->center);
        METADOT_ASSERT_E(m_I > 0.0f);
        m_invI = 1.0f / m_I;
    }

    // Move center of mass.
    b2Vec2 oldCenter = m_sweep.c;
    m_sweep.localCenter = massData->center;
    m_sweep.c0 = m_sweep.c = b2Mul(m_xf, m_sweep.localCenter);

    // Update center of mass velocity.
    m_linearVelocity += b2Cross(m_angularVelocity, m_sweep.c - oldCenter);
}

bool b2Body::ShouldCollide(const b2Body *other) const {
    // At least one body should be dynamic.
    if (m_type != b2_dynamicBody && other->m_type != b2_dynamicBody) { return false; }

    // Does a joint prevent collision?
    for (b2JointEdge *jn = m_jointList; jn; jn = jn->next) {
        if (jn->other == other) {
            if (jn->joint->m_collideConnected == false) { return false; }
        }
    }

    return true;
}

void b2Body::SetTransform(const b2Vec2 &position, float angle) {
    METADOT_ASSERT_E(m_world->IsLocked() == false);
    if (m_world->IsLocked() == true) { return; }

    m_xf.q.Set(angle);
    m_xf.p = position;

    m_sweep.c = b2Mul(m_xf, m_sweep.localCenter);
    m_sweep.a = angle;

    m_sweep.c0 = m_sweep.c;
    m_sweep.a0 = angle;

    b2BroadPhase *broadPhase = &m_world->m_contactManager.m_broadPhase;
    for (b2Fixture *f = m_fixtureList; f; f = f->m_next) { f->Synchronize(broadPhase, m_xf, m_xf); }

    // Check for new contacts the next step
    m_world->m_newContacts = true;
}

void b2Body::SynchronizeFixtures() {
    b2BroadPhase *broadPhase = &m_world->m_contactManager.m_broadPhase;

    if (m_flags & b2Body::e_awakeFlag) {
        b2Transform xf1;
        xf1.q.Set(m_sweep.a0);
        xf1.p = m_sweep.c0 - b2Mul(xf1.q, m_sweep.localCenter);

        for (b2Fixture *f = m_fixtureList; f; f = f->m_next) {
            f->Synchronize(broadPhase, xf1, m_xf);
        }
    } else {
        for (b2Fixture *f = m_fixtureList; f; f = f->m_next) {
            f->Synchronize(broadPhase, m_xf, m_xf);
        }
    }
}

void b2Body::SetEnabled(bool flag) {
    METADOT_ASSERT_E(m_world->IsLocked() == false);

    if (flag == IsEnabled()) { return; }

    if (flag) {
        m_flags |= e_enabledFlag;

        // Create all proxies.
        b2BroadPhase *broadPhase = &m_world->m_contactManager.m_broadPhase;
        for (b2Fixture *f = m_fixtureList; f; f = f->m_next) { f->CreateProxies(broadPhase, m_xf); }

        // Contacts are created at the beginning of the next
        m_world->m_newContacts = true;
    } else {
        m_flags &= ~e_enabledFlag;

        // Destroy all proxies.
        b2BroadPhase *broadPhase = &m_world->m_contactManager.m_broadPhase;
        for (b2Fixture *f = m_fixtureList; f; f = f->m_next) { f->DestroyProxies(broadPhase); }

        // Destroy the attached contacts.
        b2ContactEdge *ce = m_contactList;
        while (ce) {
            b2ContactEdge *ce0 = ce;
            ce = ce->next;
            m_world->m_contactManager.Destroy(ce0->contact);
        }
        m_contactList = nullptr;
    }
}

void b2Body::SetFixedRotation(bool flag) {
    bool status = (m_flags & e_fixedRotationFlag) == e_fixedRotationFlag;
    if (status == flag) { return; }

    if (flag) {
        m_flags |= e_fixedRotationFlag;
    } else {
        m_flags &= ~e_fixedRotationFlag;
    }

    m_angularVelocity = 0.0f;

    ResetMassData();
}

void b2Body::Dump() {
    int32 bodyIndex = m_islandIndex;

    // %.9g is sufficient to save and load the same value using text
    // FLT_DECIMAL_DIG == 9

    b2Dump("{\n");
    b2Dump("  b2BodyDef bd;\n");
    b2Dump("  bd.type = b2BodyType(%d);\n", m_type);
    b2Dump("  bd.position.Set(%.9g, %.9g);\n", m_xf.p.x, m_xf.p.y);
    b2Dump("  bd.angle = %.9g;\n", m_sweep.a);
    b2Dump("  bd.linearVelocity.Set(%.9g, %.9g);\n", m_linearVelocity.x, m_linearVelocity.y);
    b2Dump("  bd.angularVelocity = %.9g;\n", m_angularVelocity);
    b2Dump("  bd.linearDamping = %.9g;\n", m_linearDamping);
    b2Dump("  bd.angularDamping = %.9g;\n", m_angularDamping);
    b2Dump("  bd.allowSleep = bool(%d);\n", m_flags & e_autoSleepFlag);
    b2Dump("  bd.awake = bool(%d);\n", m_flags & e_awakeFlag);
    b2Dump("  bd.fixedRotation = bool(%d);\n", m_flags & e_fixedRotationFlag);
    b2Dump("  bd.bullet = bool(%d);\n", m_flags & e_bulletFlag);
    b2Dump("  bd.enabled = bool(%d);\n", m_flags & e_enabledFlag);
    b2Dump("  bd.gravityScale = %.9g;\n", m_gravityScale);
    b2Dump("  bodies[%d] = m_world->CreateBody(&bd);\n", m_islandIndex);
    b2Dump("\n");
    for (b2Fixture *f = m_fixtureList; f; f = f->m_next) {
        b2Dump("  {\n");
        f->Dump(bodyIndex);
        b2Dump("  }\n");
    }
    b2Dump("}\n");
}

b2World::b2World(const b2Vec2 &gravity) {
    m_destructionListener = nullptr;
    m_debugDraw = nullptr;

    m_bodyList = nullptr;
    m_jointList = nullptr;

    m_bodyCount = 0;
    m_jointCount = 0;

    m_warmStarting = true;
    m_continuousPhysics = true;
    m_subStepping = false;

    m_stepComplete = true;

    m_allowSleep = true;
    m_gravity = gravity;

    m_newContacts = false;
    m_locked = false;
    m_clearForces = true;

    m_inv_dt0 = 0.0f;

    m_contactManager.m_allocator = &m_blockAllocator;

    memset(&m_profile, 0, sizeof(b2Profile));
}

b2World::~b2World() {
    // Some shapes allocate using b2Alloc.
    b2Body *b = m_bodyList;
    while (b) {
        b2Body *bNext = b->m_next;

        b2Fixture *f = b->m_fixtureList;
        while (f) {
            b2Fixture *fNext = f->m_next;
            f->m_proxyCount = 0;
            f->Destroy(&m_blockAllocator);
            f = fNext;
        }

        b = bNext;
    }
}

void b2World::SetDestructionListener(b2DestructionListener *listener) {
    m_destructionListener = listener;
}

void b2World::SetContactFilter(b2ContactFilter *filter) {
    m_contactManager.m_contactFilter = filter;
}

void b2World::SetContactListener(b2ContactListener *listener) {
    m_contactManager.m_contactListener = listener;
}

void b2World::SetDebugDraw(class DebugDraw *debugDraw) { m_debugDraw = debugDraw; }

b2Body *b2World::CreateBody(const b2BodyDef *def) {
    METADOT_ASSERT_E(IsLocked() == false);
    if (IsLocked()) { return nullptr; }

    void *mem = m_blockAllocator.Allocate(sizeof(b2Body));
    b2Body *b = new (mem) b2Body(def, this);

    // Add to world doubly linked list.
    b->m_prev = nullptr;
    b->m_next = m_bodyList;
    if (m_bodyList) { m_bodyList->m_prev = b; }
    m_bodyList = b;
    ++m_bodyCount;

    return b;
}

void b2World::DestroyBody(b2Body *b) {
    METADOT_ASSERT_E(m_bodyCount > 0);
    METADOT_ASSERT_E(IsLocked() == false);
    if (IsLocked()) { return; }

    // Delete the attached joints.
    b2JointEdge *je = b->m_jointList;
    while (je) {
        b2JointEdge *je0 = je;
        je = je->next;

        if (m_destructionListener) { m_destructionListener->SayGoodbye(je0->joint); }

        DestroyJoint(je0->joint);

        b->m_jointList = je;
    }
    b->m_jointList = nullptr;

    // Delete the attached contacts.
    b2ContactEdge *ce = b->m_contactList;
    while (ce) {
        b2ContactEdge *ce0 = ce;
        ce = ce->next;
        m_contactManager.Destroy(ce0->contact);
    }
    b->m_contactList = nullptr;

    // Delete the attached fixtures. This destroys broad-phase proxies.
    b2Fixture *f = b->m_fixtureList;
    while (f) {
        b2Fixture *f0 = f;
        f = f->m_next;

        if (m_destructionListener) { m_destructionListener->SayGoodbye(f0); }

        f0->DestroyProxies(&m_contactManager.m_broadPhase);
        f0->Destroy(&m_blockAllocator);
        f0->~b2Fixture();
        m_blockAllocator.Free(f0, sizeof(b2Fixture));

        b->m_fixtureList = f;
        b->m_fixtureCount -= 1;
    }
    b->m_fixtureList = nullptr;
    b->m_fixtureCount = 0;

    // Remove world body list.
    if (b->m_prev) { b->m_prev->m_next = b->m_next; }

    if (b->m_next) { b->m_next->m_prev = b->m_prev; }

    if (b == m_bodyList) { m_bodyList = b->m_next; }

    --m_bodyCount;
    b->~b2Body();
    m_blockAllocator.Free(b, sizeof(b2Body));
}

b2Joint *b2World::CreateJoint(const b2JointDef *def) {
    METADOT_ASSERT_E(IsLocked() == false);
    if (IsLocked()) { return nullptr; }

    b2Joint *j = b2Joint::Create(def, &m_blockAllocator);

    // Connect to the world list.
    j->m_prev = nullptr;
    j->m_next = m_jointList;
    if (m_jointList) { m_jointList->m_prev = j; }
    m_jointList = j;
    ++m_jointCount;

    // Connect to the bodies' doubly linked lists.
    j->m_edgeA.joint = j;
    j->m_edgeA.other = j->m_bodyB;
    j->m_edgeA.prev = nullptr;
    j->m_edgeA.next = j->m_bodyA->m_jointList;
    if (j->m_bodyA->m_jointList) j->m_bodyA->m_jointList->prev = &j->m_edgeA;
    j->m_bodyA->m_jointList = &j->m_edgeA;

    j->m_edgeB.joint = j;
    j->m_edgeB.other = j->m_bodyA;
    j->m_edgeB.prev = nullptr;
    j->m_edgeB.next = j->m_bodyB->m_jointList;
    if (j->m_bodyB->m_jointList) j->m_bodyB->m_jointList->prev = &j->m_edgeB;
    j->m_bodyB->m_jointList = &j->m_edgeB;

    b2Body *bodyA = def->bodyA;
    b2Body *bodyB = def->bodyB;

    // If the joint prevents collisions, then flag any contacts for filtering.
    if (def->collideConnected == false) {
        b2ContactEdge *edge = bodyB->GetContactList();
        while (edge) {
            if (edge->other == bodyA) {
                // Flag the contact for filtering at the next time step (where either
                // body is awake).
                edge->contact->FlagForFiltering();
            }

            edge = edge->next;
        }
    }

    // Note: creating a joint doesn't wake the bodies.

    return j;
}

void b2World::DestroyJoint(b2Joint *j) {
    METADOT_ASSERT_E(IsLocked() == false);
    if (IsLocked()) { return; }

    bool collideConnected = j->m_collideConnected;

    // Remove from the doubly linked list.
    if (j->m_prev) { j->m_prev->m_next = j->m_next; }

    if (j->m_next) { j->m_next->m_prev = j->m_prev; }

    if (j == m_jointList) { m_jointList = j->m_next; }

    // Disconnect from island graph.
    b2Body *bodyA = j->m_bodyA;
    b2Body *bodyB = j->m_bodyB;

    // Wake up connected bodies.
    bodyA->SetAwake(true);
    bodyB->SetAwake(true);

    // Remove from body 1.
    if (j->m_edgeA.prev) { j->m_edgeA.prev->next = j->m_edgeA.next; }

    if (j->m_edgeA.next) { j->m_edgeA.next->prev = j->m_edgeA.prev; }

    if (&j->m_edgeA == bodyA->m_jointList) { bodyA->m_jointList = j->m_edgeA.next; }

    j->m_edgeA.prev = nullptr;
    j->m_edgeA.next = nullptr;

    // Remove from body 2
    if (j->m_edgeB.prev) { j->m_edgeB.prev->next = j->m_edgeB.next; }

    if (j->m_edgeB.next) { j->m_edgeB.next->prev = j->m_edgeB.prev; }

    if (&j->m_edgeB == bodyB->m_jointList) { bodyB->m_jointList = j->m_edgeB.next; }

    j->m_edgeB.prev = nullptr;
    j->m_edgeB.next = nullptr;

    b2Joint::Destroy(j, &m_blockAllocator);

    METADOT_ASSERT_E(m_jointCount > 0);
    --m_jointCount;

    // If the joint prevents collisions, then flag any contacts for filtering.
    if (collideConnected == false) {
        b2ContactEdge *edge = bodyB->GetContactList();
        while (edge) {
            if (edge->other == bodyA) {
                // Flag the contact for filtering at the next time step (where either
                // body is awake).
                edge->contact->FlagForFiltering();
            }

            edge = edge->next;
        }
    }
}

//
void b2World::SetAllowSleeping(bool flag) {
    if (flag == m_allowSleep) { return; }

    m_allowSleep = flag;
    if (m_allowSleep == false) {
        for (b2Body *b = m_bodyList; b; b = b->m_next) { b->SetAwake(true); }
    }
}

// Find islands, integrate and solve constraints, solve position constraints
void b2World::Solve(const b2TimeStep &step) {
    m_profile.solveInit = 0.0f;
    m_profile.solveVelocity = 0.0f;
    m_profile.solvePosition = 0.0f;

    // Size the island for the worst case.
    b2Island island(m_bodyCount, m_contactManager.m_contactCount, m_jointCount, &m_stackAllocator,
                    m_contactManager.m_contactListener);

    // Clear all the island flags.
    for (b2Body *b = m_bodyList; b; b = b->m_next) { b->m_flags &= ~b2Body::e_islandFlag; }
    for (b2Contact *c = m_contactManager.m_contactList; c; c = c->m_next) {
        c->m_flags &= ~b2Contact::e_islandFlag;
    }
    for (b2Joint *j = m_jointList; j; j = j->m_next) { j->m_islandFlag = false; }

    // Build and simulate all awake islands.
    int32 stackSize = m_bodyCount;
    b2Body **stack = (b2Body **) m_stackAllocator.Allocate(stackSize * sizeof(b2Body *));
    for (b2Body *seed = m_bodyList; seed; seed = seed->m_next) {
        if (seed->m_flags & b2Body::e_islandFlag) { continue; }

        if (seed->IsAwake() == false || seed->IsEnabled() == false) { continue; }

        // The seed can be dynamic or kinematic.
        if (seed->GetType() == b2_staticBody) { continue; }

        // Reset island and stack.
        island.Clear();
        int32 stackCount = 0;
        stack[stackCount++] = seed;
        seed->m_flags |= b2Body::e_islandFlag;

        // Perform a depth first search (DFS) on the constraint graph.
        while (stackCount > 0) {
            // Grab the next body off the stack and add it to the island.
            b2Body *b = stack[--stackCount];
            METADOT_ASSERT_E(b->IsEnabled() == true);
            island.Add(b);

            // To keep islands as small as possible, we don't
            // propagate islands across static bodies.
            if (b->GetType() == b2_staticBody) { continue; }

            // Make sure the body is awake (without resetting sleep timer).
            b->m_flags |= b2Body::e_awakeFlag;

            // Search all contacts connected to this body.
            for (b2ContactEdge *ce = b->m_contactList; ce; ce = ce->next) {
                b2Contact *contact = ce->contact;

                // Has this contact already been added to an island?
                if (contact->m_flags & b2Contact::e_islandFlag) { continue; }

                // Is this contact solid and touching?
                if (contact->IsEnabled() == false || contact->IsTouching() == false) { continue; }

                // Skip sensors.
                bool sensorA = contact->m_fixtureA->m_isSensor;
                bool sensorB = contact->m_fixtureB->m_isSensor;
                if (sensorA || sensorB) { continue; }

                island.Add(contact);
                contact->m_flags |= b2Contact::e_islandFlag;

                b2Body *other = ce->other;

                // Was the other body already added to this island?
                if (other->m_flags & b2Body::e_islandFlag) { continue; }

                METADOT_ASSERT_E(stackCount < stackSize);
                stack[stackCount++] = other;
                other->m_flags |= b2Body::e_islandFlag;
            }

            // Search all joints connect to this body.
            for (b2JointEdge *je = b->m_jointList; je; je = je->next) {
                if (je->joint->m_islandFlag == true) { continue; }

                b2Body *other = je->other;

                // Don't simulate joints connected to disabled bodies.
                if (other->IsEnabled() == false) { continue; }

                island.Add(je->joint);
                je->joint->m_islandFlag = true;

                if (other->m_flags & b2Body::e_islandFlag) { continue; }

                METADOT_ASSERT_E(stackCount < stackSize);
                stack[stackCount++] = other;
                other->m_flags |= b2Body::e_islandFlag;
            }
        }

        b2Profile profile;
        island.Solve(&profile, step, m_gravity, m_allowSleep);
        m_profile.solveInit += profile.solveInit;
        m_profile.solveVelocity += profile.solveVelocity;
        m_profile.solvePosition += profile.solvePosition;

        // Post solve cleanup.
        for (int32 i = 0; i < island.m_bodyCount; ++i) {
            // Allow static bodies to participate in other islands.
            b2Body *b = island.m_bodies[i];
            if (b->GetType() == b2_staticBody) { b->m_flags &= ~b2Body::e_islandFlag; }
        }
    }

    m_stackAllocator.Free(stack);

    {
        b2Timer timer;
        // Synchronize fixtures, check for out of range bodies.
        for (b2Body *b = m_bodyList; b; b = b->GetNext()) {
            // If a body was not in an island then it did not move.
            if ((b->m_flags & b2Body::e_islandFlag) == 0) { continue; }

            if (b->GetType() == b2_staticBody) { continue; }

            // Update fixtures (for broad-phase).
            b->SynchronizeFixtures();
        }

        // Look for new contacts.
        m_contactManager.FindNewContacts();
        m_profile.broadphase = timer.GetMilliseconds();
    }
}

// Find TOI contacts and solve them.
void b2World::SolveTOI(const b2TimeStep &step) {
    b2Island island(2 * b2_maxTOIContacts, b2_maxTOIContacts, 0, &m_stackAllocator,
                    m_contactManager.m_contactListener);

    if (m_stepComplete) {
        for (b2Body *b = m_bodyList; b; b = b->m_next) {
            b->m_flags &= ~b2Body::e_islandFlag;
            b->m_sweep.alpha0 = 0.0f;
        }

        for (b2Contact *c = m_contactManager.m_contactList; c; c = c->m_next) {
            // Invalidate TOI
            c->m_flags &= ~(b2Contact::e_toiFlag | b2Contact::e_islandFlag);
            c->m_toiCount = 0;
            c->m_toi = 1.0f;
        }
    }

    // Find TOI events and solve them.
    for (;;) {
        // Find the first TOI.
        b2Contact *minContact = nullptr;
        float minAlpha = 1.0f;

        for (b2Contact *c = m_contactManager.m_contactList; c; c = c->m_next) {
            // Is this contact disabled?
            if (c->IsEnabled() == false) { continue; }

            // Prevent excessive sub-stepping.
            if (c->m_toiCount > b2_maxSubSteps) { continue; }

            float alpha = 1.0f;
            if (c->m_flags & b2Contact::e_toiFlag) {
                // This contact has a valid cached TOI.
                alpha = c->m_toi;
            } else {
                b2Fixture *fA = c->GetFixtureA();
                b2Fixture *fB = c->GetFixtureB();

                // Is there a sensor?
                if (fA->IsSensor() || fB->IsSensor()) { continue; }

                b2Body *bA = fA->GetBody();
                b2Body *bB = fB->GetBody();

                b2BodyType typeA = bA->m_type;
                b2BodyType typeB = bB->m_type;
                METADOT_ASSERT_E(typeA == b2_dynamicBody || typeB == b2_dynamicBody);

                bool activeA = bA->IsAwake() && typeA != b2_staticBody;
                bool activeB = bB->IsAwake() && typeB != b2_staticBody;

                // Is at least one body active (awake and dynamic or kinematic)?
                if (activeA == false && activeB == false) { continue; }

                bool collideA = bA->IsBullet() || typeA != b2_dynamicBody;
                bool collideB = bB->IsBullet() || typeB != b2_dynamicBody;

                // Are these two non-bullet dynamic bodies?
                if (collideA == false && collideB == false) { continue; }

                // Compute the TOI for this contact.
                // Put the sweeps onto the same time interval.
                float alpha0 = bA->m_sweep.alpha0;

                if (bA->m_sweep.alpha0 < bB->m_sweep.alpha0) {
                    alpha0 = bB->m_sweep.alpha0;
                    bA->m_sweep.Advance(alpha0);
                } else if (bB->m_sweep.alpha0 < bA->m_sweep.alpha0) {
                    alpha0 = bA->m_sweep.alpha0;
                    bB->m_sweep.Advance(alpha0);
                }

                METADOT_ASSERT_E(alpha0 < 1.0f);

                int32 indexA = c->GetChildIndexA();
                int32 indexB = c->GetChildIndexB();

                // Compute the time of impact in interval [0, minTOI]
                b2TOIInput input;
                input.proxyA.Set(fA->GetShape(), indexA);
                input.proxyB.Set(fB->GetShape(), indexB);
                input.sweepA = bA->m_sweep;
                input.sweepB = bB->m_sweep;
                input.tMax = 1.0f;

                b2TOIOutput output;
                b2TimeOfImpact(&output, &input);

                // Beta is the fraction of the remaining portion of the .
                float beta = output.t;
                if (output.state == b2TOIOutput::e_touching) {
                    alpha = b2Min(alpha0 + (1.0f - alpha0) * beta, 1.0f);
                } else {
                    alpha = 1.0f;
                }

                c->m_toi = alpha;
                c->m_flags |= b2Contact::e_toiFlag;
            }

            if (alpha < minAlpha) {
                // This is the minimum TOI found so far.
                minContact = c;
                minAlpha = alpha;
            }
        }

        if (minContact == nullptr || 1.0f - 10.0f * b2_epsilon < minAlpha) {
            // No more TOI events. Done!
            m_stepComplete = true;
            break;
        }

        // Advance the bodies to the TOI.
        b2Fixture *fA = minContact->GetFixtureA();
        b2Fixture *fB = minContact->GetFixtureB();
        b2Body *bA = fA->GetBody();
        b2Body *bB = fB->GetBody();

        b2Sweep backup1 = bA->m_sweep;
        b2Sweep backup2 = bB->m_sweep;

        bA->Advance(minAlpha);
        bB->Advance(minAlpha);

        // The TOI contact likely has some new contact points.
        minContact->Update(m_contactManager.m_contactListener);
        minContact->m_flags &= ~b2Contact::e_toiFlag;
        ++minContact->m_toiCount;

        // Is the contact solid?
        if (minContact->IsEnabled() == false || minContact->IsTouching() == false) {
            // Restore the sweeps.
            minContact->SetEnabled(false);
            bA->m_sweep = backup1;
            bB->m_sweep = backup2;
            bA->SynchronizeTransform();
            bB->SynchronizeTransform();
            continue;
        }

        bA->SetAwake(true);
        bB->SetAwake(true);

        // Build the island
        island.Clear();
        island.Add(bA);
        island.Add(bB);
        island.Add(minContact);

        bA->m_flags |= b2Body::e_islandFlag;
        bB->m_flags |= b2Body::e_islandFlag;
        minContact->m_flags |= b2Contact::e_islandFlag;

        // Get contacts on bodyA and bodyB.
        b2Body *bodies[2] = {bA, bB};
        for (int32 i = 0; i < 2; ++i) {
            b2Body *body = bodies[i];
            if (body->m_type == b2_dynamicBody) {
                for (b2ContactEdge *ce = body->m_contactList; ce; ce = ce->next) {
                    if (island.m_bodyCount == island.m_bodyCapacity) { break; }

                    if (island.m_contactCount == island.m_contactCapacity) { break; }

                    b2Contact *contact = ce->contact;

                    // Has this contact already been added to the island?
                    if (contact->m_flags & b2Contact::e_islandFlag) { continue; }

                    // Only add static, kinematic, or bullet bodies.
                    b2Body *other = ce->other;
                    if (other->m_type == b2_dynamicBody && body->IsBullet() == false &&
                        other->IsBullet() == false) {
                        continue;
                    }

                    // Skip sensors.
                    bool sensorA = contact->m_fixtureA->m_isSensor;
                    bool sensorB = contact->m_fixtureB->m_isSensor;
                    if (sensorA || sensorB) { continue; }

                    // Tentatively advance the body to the TOI.
                    b2Sweep backup = other->m_sweep;
                    if ((other->m_flags & b2Body::e_islandFlag) == 0) { other->Advance(minAlpha); }

                    // Update the contact points
                    contact->Update(m_contactManager.m_contactListener);

                    // Was the contact disabled by the user?
                    if (contact->IsEnabled() == false) {
                        other->m_sweep = backup;
                        other->SynchronizeTransform();
                        continue;
                    }

                    // Are there contact points?
                    if (contact->IsTouching() == false) {
                        other->m_sweep = backup;
                        other->SynchronizeTransform();
                        continue;
                    }

                    // Add the contact to the island
                    contact->m_flags |= b2Contact::e_islandFlag;
                    island.Add(contact);

                    // Has the other body already been added to the island?
                    if (other->m_flags & b2Body::e_islandFlag) { continue; }

                    // Add the other body to the island.
                    other->m_flags |= b2Body::e_islandFlag;

                    if (other->m_type != b2_staticBody) { other->SetAwake(true); }

                    island.Add(other);
                }
            }
        }

        b2TimeStep subStep;
        subStep.dt = (1.0f - minAlpha) * step.dt;
        subStep.inv_dt = 1.0f / subStep.dt;
        subStep.dtRatio = 1.0f;
        subStep.positionIterations = 20;
        subStep.velocityIterations = step.velocityIterations;
        subStep.warmStarting = false;
        island.SolveTOI(subStep, bA->m_islandIndex, bB->m_islandIndex);

        // Reset island flags and synchronize broad-phase proxies.
        for (int32 i = 0; i < island.m_bodyCount; ++i) {
            b2Body *body = island.m_bodies[i];
            body->m_flags &= ~b2Body::e_islandFlag;

            if (body->m_type != b2_dynamicBody) { continue; }

            body->SynchronizeFixtures();

            // Invalidate all contact TOIs on this displaced body.
            for (b2ContactEdge *ce = body->m_contactList; ce; ce = ce->next) {
                ce->contact->m_flags &= ~(b2Contact::e_toiFlag | b2Contact::e_islandFlag);
            }
        }

        // Commit fixture proxy movements to the broad-phase so that new contacts are created.
        // Also, some contacts can be destroyed.
        m_contactManager.FindNewContacts();

        if (m_subStepping) {
            m_stepComplete = false;
            break;
        }
    }
}

void b2World::Step(float dt, int32 velocityIterations, int32 positionIterations) {
    b2Timer stepTimer;

    // If new fixtures were added, we need to find the new contacts.
    if (m_newContacts) {
        m_contactManager.FindNewContacts();
        m_newContacts = false;
    }

    m_locked = true;

    b2TimeStep step;
    step.dt = dt;
    step.velocityIterations = velocityIterations;
    step.positionIterations = positionIterations;
    if (dt > 0.0f) {
        step.inv_dt = 1.0f / dt;
    } else {
        step.inv_dt = 0.0f;
    }

    step.dtRatio = m_inv_dt0 * dt;

    step.warmStarting = m_warmStarting;

    // Update contacts. This is where some contacts are destroyed.
    {
        b2Timer timer;
        m_contactManager.Collide();
        m_profile.collide = timer.GetMilliseconds();
    }

    // Integrate velocities, solve velocity constraints, and integrate positions.
    if (m_stepComplete && step.dt > 0.0f) {
        b2Timer timer;
        Solve(step);
        m_profile.solve = timer.GetMilliseconds();
    }

    // Handle TOI events.
    if (m_continuousPhysics && step.dt > 0.0f) {
        b2Timer timer;
        SolveTOI(step);
        m_profile.solveTOI = timer.GetMilliseconds();
    }

    if (step.dt > 0.0f) { m_inv_dt0 = step.inv_dt; }

    if (m_clearForces) { ClearForces(); }

    m_locked = false;

    m_profile.step = stepTimer.GetMilliseconds();
}

void b2World::ClearForces() {
    for (b2Body *body = m_bodyList; body; body = body->GetNext()) {
        body->m_force.SetZero();
        body->m_torque = 0.0f;
    }
}

struct b2WorldQueryWrapper
{
    bool QueryCallback(int32 proxyId) {
        b2FixtureProxy *proxy = (b2FixtureProxy *) broadPhase->GetUserData(proxyId);
        return callback->ReportFixture(proxy->fixture);
    }

    const b2BroadPhase *broadPhase;
    b2QueryCallback *callback;
};

void b2World::QueryAABB(b2QueryCallback *callback, const b2AABB &aabb) const {
    b2WorldQueryWrapper wrapper;
    wrapper.broadPhase = &m_contactManager.m_broadPhase;
    wrapper.callback = callback;
    m_contactManager.m_broadPhase.Query(&wrapper, aabb);
}

struct b2WorldRayCastWrapper
{
    float RayCastCallback(const b2RayCastInput &input, int32 proxyId) {
        void *userData = broadPhase->GetUserData(proxyId);
        b2FixtureProxy *proxy = (b2FixtureProxy *) userData;
        b2Fixture *fixture = proxy->fixture;
        int32 index = proxy->childIndex;
        b2RayCastOutput output;
        bool hit = fixture->RayCast(&output, input, index);

        if (hit) {
            float fraction = output.fraction;
            b2Vec2 point = (1.0f - fraction) * input.p1 + fraction * input.p2;
            return callback->ReportFixture(fixture, point, output.normal, fraction);
        }

        return input.maxFraction;
    }

    const b2BroadPhase *broadPhase;
    b2RayCastCallback *callback;
};

void b2World::RayCast(b2RayCastCallback *callback, const b2Vec2 &point1,
                      const b2Vec2 &point2) const {
    b2WorldRayCastWrapper wrapper;
    wrapper.broadPhase = &m_contactManager.m_broadPhase;
    wrapper.callback = callback;
    b2RayCastInput input;
    input.maxFraction = 1.0f;
    input.p1 = point1;
    input.p2 = point2;
    m_contactManager.m_broadPhase.RayCast(&wrapper, input);
}

void b2World::DrawShape(b2Fixture *fixture, const b2Transform &xf, const b2Color &color) {
    switch (fixture->GetType()) {
        case b2Shape::e_circle: {
            b2CircleShape *circle = (b2CircleShape *) fixture->GetShape();

            b2Vec2 center = b2Mul(xf, circle->m_p);
            float radius = circle->m_radius;
            b2Vec2 axis = b2Mul(xf.q, b2Vec2(1.0f, 0.0f));

            m_debugDraw->DrawSolidCircle(center, radius, axis, color);
        } break;

        case b2Shape::e_edge: {
            b2EdgeShape *edge = (b2EdgeShape *) fixture->GetShape();
            b2Vec2 v1 = b2Mul(xf, edge->m_vertex1);
            b2Vec2 v2 = b2Mul(xf, edge->m_vertex2);
            m_debugDraw->DrawSegment(v1, v2, color);

            if (edge->m_oneSided == false) {
                m_debugDraw->DrawPoint(v1, 4.0f, color);
                m_debugDraw->DrawPoint(v2, 4.0f, color);
            }
        } break;

        case b2Shape::e_chain: {
            b2ChainShape *chain = (b2ChainShape *) fixture->GetShape();
            int32 count = chain->m_count;
            const b2Vec2 *vertices = chain->m_vertices;

            b2Vec2 v1 = b2Mul(xf, vertices[0]);
            for (int32 i = 1; i < count; ++i) {
                b2Vec2 v2 = b2Mul(xf, vertices[i]);
                m_debugDraw->DrawSegment(v1, v2, color);
                v1 = v2;
            }
        } break;

        case b2Shape::e_polygon: {
            b2PolygonShape *poly = (b2PolygonShape *) fixture->GetShape();
            int32 vertexCount = poly->m_count;
            METADOT_ASSERT_E(vertexCount <= b2_maxPolygonVertices);
            b2Vec2 vertices[b2_maxPolygonVertices];

            for (int32 i = 0; i < vertexCount; ++i) {
                vertices[i] = b2Mul(xf, poly->m_vertices[i]);
            }

            m_debugDraw->DrawSolidPolygon(vertices, vertexCount, color);
        } break;

        default:
            break;
    }
}

void b2World::DebugDraw() {
    if (m_debugDraw == nullptr) { return; }

    uint32 flags = m_debugDraw->GetFlags();

    if (flags & DebugDraw::e_shapeBit) {
        for (b2Body *b = m_bodyList; b; b = b->GetNext()) {
            const b2Transform &xf = b->GetTransform();
            for (b2Fixture *f = b->GetFixtureList(); f; f = f->GetNext()) {
                if (b->GetType() == b2_dynamicBody && b->m_mass == 0.0f) {
                    // Bad body
                    DrawShape(f, xf, b2Color(1.0f, 0.0f, 0.0f));
                } else if (b->IsEnabled() == false) {
                    DrawShape(f, xf, b2Color(0.5f, 0.5f, 0.3f));
                } else if (b->GetType() == b2_staticBody) {
                    DrawShape(f, xf, b2Color(0.5f, 0.9f, 0.5f));
                } else if (b->GetType() == b2_kinematicBody) {
                    DrawShape(f, xf, b2Color(0.5f, 0.5f, 0.9f));
                } else if (b->IsAwake() == false) {
                    DrawShape(f, xf, b2Color(0.6f, 0.6f, 0.6f));
                } else {
                    DrawShape(f, xf, b2Color(0.9f, 0.7f, 0.7f));
                }
            }
        }
    }

    if (flags & DebugDraw::e_jointBit) {
        for (b2Joint *j = m_jointList; j; j = j->GetNext()) { j->Draw(m_debugDraw); }
    }

    if (flags & DebugDraw::e_pairBit) {
        b2Color color(0.3f, 0.9f, 0.9f);
        for (b2Contact *c = m_contactManager.m_contactList; c; c = c->GetNext()) {
            b2Fixture *fixtureA = c->GetFixtureA();
            b2Fixture *fixtureB = c->GetFixtureB();
            int32 indexA = c->GetChildIndexA();
            int32 indexB = c->GetChildIndexB();
            b2Vec2 cA = fixtureA->GetAABB(indexA).GetCenter();
            b2Vec2 cB = fixtureB->GetAABB(indexB).GetCenter();

            m_debugDraw->DrawSegment(cA, cB, color);
        }
    }

    if (flags & DebugDraw::e_aabbBit) {
        b2Color color(0.9f, 0.3f, 0.9f);
        b2BroadPhase *bp = &m_contactManager.m_broadPhase;

        for (b2Body *b = m_bodyList; b; b = b->GetNext()) {
            if (b->IsEnabled() == false) { continue; }

            for (b2Fixture *f = b->GetFixtureList(); f; f = f->GetNext()) {
                for (int32 i = 0; i < f->m_proxyCount; ++i) {
                    b2FixtureProxy *proxy = f->m_proxies + i;
                    b2AABB aabb = bp->GetFatAABB(proxy->proxyId);
                    b2Vec2 vs[4];
                    vs[0].Set(aabb.lowerBound.x, aabb.lowerBound.y);
                    vs[1].Set(aabb.upperBound.x, aabb.lowerBound.y);
                    vs[2].Set(aabb.upperBound.x, aabb.upperBound.y);
                    vs[3].Set(aabb.lowerBound.x, aabb.upperBound.y);

                    m_debugDraw->DrawPolygon(vs, 4, color);
                }
            }
        }
    }

    if (flags & DebugDraw::e_centerOfMassBit) {
        for (b2Body *b = m_bodyList; b; b = b->GetNext()) {
            b2Transform xf = b->GetTransform();
            xf.p = b->GetWorldCenter();
            m_debugDraw->DrawTransform(xf);
        }
    }
}

int32 b2World::GetProxyCount() const { return m_contactManager.m_broadPhase.GetProxyCount(); }

int32 b2World::GetTreeHeight() const { return m_contactManager.m_broadPhase.GetTreeHeight(); }

int32 b2World::GetTreeBalance() const { return m_contactManager.m_broadPhase.GetTreeBalance(); }

float b2World::GetTreeQuality() const { return m_contactManager.m_broadPhase.GetTreeQuality(); }

void b2World::ShiftOrigin(const b2Vec2 &newOrigin) {
    METADOT_ASSERT_E(m_locked == false);
    if (m_locked) { return; }

    for (b2Body *b = m_bodyList; b; b = b->m_next) {
        b->m_xf.p -= newOrigin;
        b->m_sweep.c0 -= newOrigin;
        b->m_sweep.c -= newOrigin;
    }

    for (b2Joint *j = m_jointList; j; j = j->m_next) { j->ShiftOrigin(newOrigin); }

    m_contactManager.m_broadPhase.ShiftOrigin(newOrigin);
}

void b2World::Dump() {
    if (m_locked) { return; }

    b2OpenDump("box2d_dump.inl");

    b2Dump("b2Vec2 g(%.9g, %.9g);\n", m_gravity.x, m_gravity.y);
    b2Dump("m_world->SetGravity(g);\n");

    b2Dump("b2Body** bodies = (b2Body**)b2Alloc(%d * sizeof(b2Body*));\n", m_bodyCount);
    b2Dump("b2Joint** joints = (b2Joint**)b2Alloc(%d * sizeof(b2Joint*));\n", m_jointCount);

    int32 i = 0;
    for (b2Body *b = m_bodyList; b; b = b->m_next) {
        b->m_islandIndex = i;
        b->Dump();
        ++i;
    }

    i = 0;
    for (b2Joint *j = m_jointList; j; j = j->m_next) {
        j->m_index = i;
        ++i;
    }

    // First pass on joints, skip gear joints.
    for (b2Joint *j = m_jointList; j; j = j->m_next) {
        if (j->m_type == e_gearJoint) { continue; }

        b2Dump("{\n");
        j->Dump();
        b2Dump("}\n");
    }

    // Second pass on joints, only gear joints.
    for (b2Joint *j = m_jointList; j; j = j->m_next) {
        if (j->m_type != e_gearJoint) { continue; }

        b2Dump("{\n");
        j->Dump();
        b2Dump("}\n");
    }

    b2Dump("b2Free(joints);\n");
    b2Dump("b2Free(bodies);\n");
    b2Dump("joints = nullptr;\n");
    b2Dump("bodies = nullptr;\n");

    b2CloseDump();
}

// Return true if contact calculations should be performed between these two shapes.
// If you implement your own collision filter you may want to build from this implementation.
bool b2ContactFilter::ShouldCollide(b2Fixture *fixtureA, b2Fixture *fixtureB) {
    const b2Filter &filterA = fixtureA->GetFilterData();
    const b2Filter &filterB = fixtureB->GetFilterData();

    if (filterA.groupIndex == filterB.groupIndex && filterA.groupIndex != 0) {
        return filterA.groupIndex > 0;
    }

    bool collide = (filterA.maskBits & filterB.categoryBits) != 0 &&
                   (filterA.categoryBits & filterB.maskBits) != 0;
    return collide;
}

// Linear constraint (point-to-line)
// d = pB - pA = xB + rB - xA - rA
// C = dot(ay, d)
// Cdot = dot(d, cross(wA, ay)) + dot(ay, vB + cross(wB, rB) - vA - cross(wA, rA))
//      = -dot(ay, vA) - dot(cross(d + rA, ay), wA) + dot(ay, vB) + dot(cross(rB, ay), vB)
// J = [-ay, -cross(d + rA, ay), ay, cross(rB, ay)]

// Spring linear constraint
// C = dot(ax, d)
// Cdot = = -dot(ax, vA) - dot(cross(d + rA, ax), wA) + dot(ax, vB) + dot(cross(rB, ax), vB)
// J = [-ax -cross(d+rA, ax) ax cross(rB, ax)]

// Motor rotational constraint
// Cdot = wB - wA
// J = [0 0 -1 0 0 1]

void b2WheelJointDef::Initialize(b2Body *bA, b2Body *bB, const b2Vec2 &anchor, const b2Vec2 &axis) {
    bodyA = bA;
    bodyB = bB;
    localAnchorA = bodyA->GetLocalPoint(anchor);
    localAnchorB = bodyB->GetLocalPoint(anchor);
    localAxisA = bodyA->GetLocalVector(axis);
}

b2WheelJoint::b2WheelJoint(const b2WheelJointDef *def) : b2Joint(def) {
    m_localAnchorA = def->localAnchorA;
    m_localAnchorB = def->localAnchorB;
    m_localXAxisA = def->localAxisA;
    m_localYAxisA = b2Cross(1.0f, m_localXAxisA);

    m_mass = 0.0f;
    m_impulse = 0.0f;
    m_motorMass = 0.0f;
    m_motorImpulse = 0.0f;
    m_springMass = 0.0f;
    m_springImpulse = 0.0f;

    m_axialMass = 0.0f;
    m_lowerImpulse = 0.0f;
    m_upperImpulse = 0.0f;
    m_lowerTranslation = def->lowerTranslation;
    m_upperTranslation = def->upperTranslation;
    m_enableLimit = def->enableLimit;

    m_maxMotorTorque = def->maxMotorTorque;
    m_motorSpeed = def->motorSpeed;
    m_enableMotor = def->enableMotor;

    m_bias = 0.0f;
    m_gamma = 0.0f;

    m_ax.SetZero();
    m_ay.SetZero();

    m_stiffness = def->stiffness;
    m_damping = def->damping;
}

void b2WheelJoint::InitVelocityConstraints(const b2SolverData &data) {
    m_indexA = m_bodyA->m_islandIndex;
    m_indexB = m_bodyB->m_islandIndex;
    m_localCenterA = m_bodyA->m_sweep.localCenter;
    m_localCenterB = m_bodyB->m_sweep.localCenter;
    m_invMassA = m_bodyA->m_invMass;
    m_invMassB = m_bodyB->m_invMass;
    m_invIA = m_bodyA->m_invI;
    m_invIB = m_bodyB->m_invI;

    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    b2Vec2 cA = data.positions[m_indexA].c;
    float aA = data.positions[m_indexA].a;
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;

    b2Vec2 cB = data.positions[m_indexB].c;
    float aB = data.positions[m_indexB].a;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    b2Rot qA(aA), qB(aB);

    // Compute the effective masses.
    b2Vec2 rA = b2Mul(qA, m_localAnchorA - m_localCenterA);
    b2Vec2 rB = b2Mul(qB, m_localAnchorB - m_localCenterB);
    b2Vec2 d = cB + rB - cA - rA;

    // Point to line constraint
    {
        m_ay = b2Mul(qA, m_localYAxisA);
        m_sAy = b2Cross(d + rA, m_ay);
        m_sBy = b2Cross(rB, m_ay);

        m_mass = mA + mB + iA * m_sAy * m_sAy + iB * m_sBy * m_sBy;

        if (m_mass > 0.0f) { m_mass = 1.0f / m_mass; }
    }

    // Spring constraint
    m_ax = b2Mul(qA, m_localXAxisA);
    m_sAx = b2Cross(d + rA, m_ax);
    m_sBx = b2Cross(rB, m_ax);

    const float invMass = mA + mB + iA * m_sAx * m_sAx + iB * m_sBx * m_sBx;
    if (invMass > 0.0f) {
        m_axialMass = 1.0f / invMass;
    } else {
        m_axialMass = 0.0f;
    }

    m_springMass = 0.0f;
    m_bias = 0.0f;
    m_gamma = 0.0f;

    if (m_stiffness > 0.0f && invMass > 0.0f) {
        m_springMass = 1.0f / invMass;

        float C = b2Dot(d, m_ax);

        // magic formulas
        float h = data.step.dt;
        m_gamma = h * (m_damping + h * m_stiffness);
        if (m_gamma > 0.0f) { m_gamma = 1.0f / m_gamma; }

        m_bias = C * h * m_stiffness * m_gamma;

        m_springMass = invMass + m_gamma;
        if (m_springMass > 0.0f) { m_springMass = 1.0f / m_springMass; }
    } else {
        m_springImpulse = 0.0f;
    }

    if (m_enableLimit) {
        m_translation = b2Dot(m_ax, d);
    } else {
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }

    if (m_enableMotor) {
        m_motorMass = iA + iB;
        if (m_motorMass > 0.0f) { m_motorMass = 1.0f / m_motorMass; }
    } else {
        m_motorMass = 0.0f;
        m_motorImpulse = 0.0f;
    }

    if (data.step.warmStarting) {
        // Account for variable time step.
        m_impulse *= data.step.dtRatio;
        m_springImpulse *= data.step.dtRatio;
        m_motorImpulse *= data.step.dtRatio;

        float axialImpulse = m_springImpulse + m_lowerImpulse - m_upperImpulse;
        b2Vec2 P = m_impulse * m_ay + axialImpulse * m_ax;
        float LA = m_impulse * m_sAy + axialImpulse * m_sAx + m_motorImpulse;
        float LB = m_impulse * m_sBy + axialImpulse * m_sBx + m_motorImpulse;

        vA -= m_invMassA * P;
        wA -= m_invIA * LA;

        vB += m_invMassB * P;
        wB += m_invIB * LB;
    } else {
        m_impulse = 0.0f;
        m_springImpulse = 0.0f;
        m_motorImpulse = 0.0f;
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

void b2WheelJoint::SolveVelocityConstraints(const b2SolverData &data) {
    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    // Solve spring constraint
    {
        float Cdot = b2Dot(m_ax, vB - vA) + m_sBx * wB - m_sAx * wA;
        float impulse = -m_springMass * (Cdot + m_bias + m_gamma * m_springImpulse);
        m_springImpulse += impulse;

        b2Vec2 P = impulse * m_ax;
        float LA = impulse * m_sAx;
        float LB = impulse * m_sBx;

        vA -= mA * P;
        wA -= iA * LA;

        vB += mB * P;
        wB += iB * LB;
    }

    // Solve rotational motor constraint
    {
        float Cdot = wB - wA - m_motorSpeed;
        float impulse = -m_motorMass * Cdot;

        float oldImpulse = m_motorImpulse;
        float maxImpulse = data.step.dt * m_maxMotorTorque;
        m_motorImpulse = b2Clamp(m_motorImpulse + impulse, -maxImpulse, maxImpulse);
        impulse = m_motorImpulse - oldImpulse;

        wA -= iA * impulse;
        wB += iB * impulse;
    }

    if (m_enableLimit) {
        // Lower limit
        {
            float C = m_translation - m_lowerTranslation;
            float Cdot = b2Dot(m_ax, vB - vA) + m_sBx * wB - m_sAx * wA;
            float impulse = -m_axialMass * (Cdot + b2Max(C, 0.0f) * data.step.inv_dt);
            float oldImpulse = m_lowerImpulse;
            m_lowerImpulse = b2Max(m_lowerImpulse + impulse, 0.0f);
            impulse = m_lowerImpulse - oldImpulse;

            b2Vec2 P = impulse * m_ax;
            float LA = impulse * m_sAx;
            float LB = impulse * m_sBx;

            vA -= mA * P;
            wA -= iA * LA;
            vB += mB * P;
            wB += iB * LB;
        }

        // Upper limit
        // Note: signs are flipped to keep C positive when the constraint is satisfied.
        // This also keeps the impulse positive when the limit is active.
        {
            float C = m_upperTranslation - m_translation;
            float Cdot = b2Dot(m_ax, vA - vB) + m_sAx * wA - m_sBx * wB;
            float impulse = -m_axialMass * (Cdot + b2Max(C, 0.0f) * data.step.inv_dt);
            float oldImpulse = m_upperImpulse;
            m_upperImpulse = b2Max(m_upperImpulse + impulse, 0.0f);
            impulse = m_upperImpulse - oldImpulse;

            b2Vec2 P = impulse * m_ax;
            float LA = impulse * m_sAx;
            float LB = impulse * m_sBx;

            vA += mA * P;
            wA += iA * LA;
            vB -= mB * P;
            wB -= iB * LB;
        }
    }

    // Solve point to line constraint
    {
        float Cdot = b2Dot(m_ay, vB - vA) + m_sBy * wB - m_sAy * wA;
        float impulse = -m_mass * Cdot;
        m_impulse += impulse;

        b2Vec2 P = impulse * m_ay;
        float LA = impulse * m_sAy;
        float LB = impulse * m_sBy;

        vA -= mA * P;
        wA -= iA * LA;

        vB += mB * P;
        wB += iB * LB;
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

bool b2WheelJoint::SolvePositionConstraints(const b2SolverData &data) {
    b2Vec2 cA = data.positions[m_indexA].c;
    float aA = data.positions[m_indexA].a;
    b2Vec2 cB = data.positions[m_indexB].c;
    float aB = data.positions[m_indexB].a;

    float linearError = 0.0f;

    if (m_enableLimit) {
        b2Rot qA(aA), qB(aB);

        b2Vec2 rA = b2Mul(qA, m_localAnchorA - m_localCenterA);
        b2Vec2 rB = b2Mul(qB, m_localAnchorB - m_localCenterB);
        b2Vec2 d = (cB - cA) + rB - rA;

        b2Vec2 ax = b2Mul(qA, m_localXAxisA);
        float sAx = b2Cross(d + rA, m_ax);
        float sBx = b2Cross(rB, m_ax);

        float C = 0.0f;
        float translation = b2Dot(ax, d);
        if (b2Abs(m_upperTranslation - m_lowerTranslation) < 2.0f * b2_linearSlop) {
            C = translation;
        } else if (translation <= m_lowerTranslation) {
            C = b2Min(translation - m_lowerTranslation, 0.0f);
        } else if (translation >= m_upperTranslation) {
            C = b2Max(translation - m_upperTranslation, 0.0f);
        }

        if (C != 0.0f) {

            float invMass = m_invMassA + m_invMassB + m_invIA * sAx * sAx + m_invIB * sBx * sBx;
            float impulse = 0.0f;
            if (invMass != 0.0f) { impulse = -C / invMass; }

            b2Vec2 P = impulse * ax;
            float LA = impulse * sAx;
            float LB = impulse * sBx;

            cA -= m_invMassA * P;
            aA -= m_invIA * LA;
            cB += m_invMassB * P;
            aB += m_invIB * LB;

            linearError = b2Abs(C);
        }
    }

    // Solve perpendicular constraint
    {
        b2Rot qA(aA), qB(aB);

        b2Vec2 rA = b2Mul(qA, m_localAnchorA - m_localCenterA);
        b2Vec2 rB = b2Mul(qB, m_localAnchorB - m_localCenterB);
        b2Vec2 d = (cB - cA) + rB - rA;

        b2Vec2 ay = b2Mul(qA, m_localYAxisA);

        float sAy = b2Cross(d + rA, ay);
        float sBy = b2Cross(rB, ay);

        float C = b2Dot(d, ay);

        float invMass = m_invMassA + m_invMassB + m_invIA * m_sAy * m_sAy + m_invIB * m_sBy * m_sBy;

        float impulse = 0.0f;
        if (invMass != 0.0f) { impulse = -C / invMass; }

        b2Vec2 P = impulse * ay;
        float LA = impulse * sAy;
        float LB = impulse * sBy;

        cA -= m_invMassA * P;
        aA -= m_invIA * LA;
        cB += m_invMassB * P;
        aB += m_invIB * LB;

        linearError = b2Max(linearError, b2Abs(C));
    }

    data.positions[m_indexA].c = cA;
    data.positions[m_indexA].a = aA;
    data.positions[m_indexB].c = cB;
    data.positions[m_indexB].a = aB;

    return linearError <= b2_linearSlop;
}

b2Vec2 b2WheelJoint::GetAnchorA() const { return m_bodyA->GetWorldPoint(m_localAnchorA); }

b2Vec2 b2WheelJoint::GetAnchorB() const { return m_bodyB->GetWorldPoint(m_localAnchorB); }

b2Vec2 b2WheelJoint::GetReactionForce(float inv_dt) const {
    return inv_dt * (m_impulse * m_ay + (m_springImpulse + m_lowerImpulse - m_upperImpulse) * m_ax);
}

float b2WheelJoint::GetReactionTorque(float inv_dt) const { return inv_dt * m_motorImpulse; }

float b2WheelJoint::GetJointTranslation() const {
    b2Body *bA = m_bodyA;
    b2Body *bB = m_bodyB;

    b2Vec2 pA = bA->GetWorldPoint(m_localAnchorA);
    b2Vec2 pB = bB->GetWorldPoint(m_localAnchorB);
    b2Vec2 d = pB - pA;
    b2Vec2 axis = bA->GetWorldVector(m_localXAxisA);

    float translation = b2Dot(d, axis);
    return translation;
}

float b2WheelJoint::GetJointLinearSpeed() const {
    b2Body *bA = m_bodyA;
    b2Body *bB = m_bodyB;

    b2Vec2 rA = b2Mul(bA->m_xf.q, m_localAnchorA - bA->m_sweep.localCenter);
    b2Vec2 rB = b2Mul(bB->m_xf.q, m_localAnchorB - bB->m_sweep.localCenter);
    b2Vec2 p1 = bA->m_sweep.c + rA;
    b2Vec2 p2 = bB->m_sweep.c + rB;
    b2Vec2 d = p2 - p1;
    b2Vec2 axis = b2Mul(bA->m_xf.q, m_localXAxisA);

    b2Vec2 vA = bA->m_linearVelocity;
    b2Vec2 vB = bB->m_linearVelocity;
    float wA = bA->m_angularVelocity;
    float wB = bB->m_angularVelocity;

    float speed =
            b2Dot(d, b2Cross(wA, axis)) + b2Dot(axis, vB + b2Cross(wB, rB) - vA - b2Cross(wA, rA));
    return speed;
}

float b2WheelJoint::GetJointAngle() const {
    b2Body *bA = m_bodyA;
    b2Body *bB = m_bodyB;
    return bB->m_sweep.a - bA->m_sweep.a;
}

float b2WheelJoint::GetJointAngularSpeed() const {
    float wA = m_bodyA->m_angularVelocity;
    float wB = m_bodyB->m_angularVelocity;
    return wB - wA;
}

bool b2WheelJoint::IsLimitEnabled() const { return m_enableLimit; }

void b2WheelJoint::EnableLimit(bool flag) {
    if (flag != m_enableLimit) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_enableLimit = flag;
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }
}

float b2WheelJoint::GetLowerLimit() const { return m_lowerTranslation; }

float b2WheelJoint::GetUpperLimit() const { return m_upperTranslation; }

void b2WheelJoint::SetLimits(float lower, float upper) {
    METADOT_ASSERT_E(lower <= upper);
    if (lower != m_lowerTranslation || upper != m_upperTranslation) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_lowerTranslation = lower;
        m_upperTranslation = upper;
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }
}

bool b2WheelJoint::IsMotorEnabled() const { return m_enableMotor; }

void b2WheelJoint::EnableMotor(bool flag) {
    if (flag != m_enableMotor) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_enableMotor = flag;
    }
}

void b2WheelJoint::SetMotorSpeed(float speed) {
    if (speed != m_motorSpeed) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_motorSpeed = speed;
    }
}

void b2WheelJoint::SetMaxMotorTorque(float torque) {
    if (torque != m_maxMotorTorque) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_maxMotorTorque = torque;
    }
}

float b2WheelJoint::GetMotorTorque(float inv_dt) const { return inv_dt * m_motorImpulse; }

void b2WheelJoint::SetStiffness(float stiffness) { m_stiffness = stiffness; }

float b2WheelJoint::GetStiffness() const { return m_stiffness; }

void b2WheelJoint::SetDamping(float damping) { m_damping = damping; }

float b2WheelJoint::GetDamping() const { return m_damping; }

void b2WheelJoint::Dump() {
    // FLT_DECIMAL_DIG == 9

    int32 indexA = m_bodyA->m_islandIndex;
    int32 indexB = m_bodyB->m_islandIndex;

    b2Dump("  b2WheelJointDef jd;\n");
    b2Dump("  jd.bodyA = bodies[%d];\n", indexA);
    b2Dump("  jd.bodyB = bodies[%d];\n", indexB);
    b2Dump("  jd.collideConnected = bool(%d);\n", m_collideConnected);
    b2Dump("  jd.localAnchorA.Set(%.9g, %.9g);\n", m_localAnchorA.x, m_localAnchorA.y);
    b2Dump("  jd.localAnchorB.Set(%.9g, %.9g);\n", m_localAnchorB.x, m_localAnchorB.y);
    b2Dump("  jd.localAxisA.Set(%.9g, %.9g);\n", m_localXAxisA.x, m_localXAxisA.y);
    b2Dump("  jd.enableMotor = bool(%d);\n", m_enableMotor);
    b2Dump("  jd.motorSpeed = %.9g;\n", m_motorSpeed);
    b2Dump("  jd.maxMotorTorque = %.9g;\n", m_maxMotorTorque);
    b2Dump("  jd.stiffness = %.9g;\n", m_stiffness);
    b2Dump("  jd.damping = %.9g;\n", m_damping);
    b2Dump("  joints[%d] = m_world->CreateJoint(&jd);\n", m_index);
}

///
void b2WheelJoint::Draw(DebugDraw *draw) const {
    const b2Transform &xfA = m_bodyA->GetTransform();
    const b2Transform &xfB = m_bodyB->GetTransform();
    b2Vec2 pA = b2Mul(xfA, m_localAnchorA);
    b2Vec2 pB = b2Mul(xfB, m_localAnchorB);

    b2Vec2 axis = b2Mul(xfA.q, m_localXAxisA);

    b2Color c1(0.7f, 0.7f, 0.7f);
    b2Color c2(0.3f, 0.9f, 0.3f);
    b2Color c3(0.9f, 0.3f, 0.3f);
    b2Color c4(0.3f, 0.3f, 0.9f);
    b2Color c5(0.4f, 0.4f, 0.4f);

    draw->DrawSegment(pA, pB, c5);

    if (m_enableLimit) {
        b2Vec2 lower = pA + m_lowerTranslation * axis;
        b2Vec2 upper = pA + m_upperTranslation * axis;
        b2Vec2 perp = b2Mul(xfA.q, m_localYAxisA);
        draw->DrawSegment(lower, upper, c1);
        draw->DrawSegment(lower - 0.5f * perp, lower + 0.5f * perp, c2);
        draw->DrawSegment(upper - 0.5f * perp, upper + 0.5f * perp, c3);
    } else {
        draw->DrawSegment(pA - 1.0f * axis, pA + 1.0f * axis, c1);
    }

    draw->DrawPoint(pA, 5.0f, c1);
    draw->DrawPoint(pB, 5.0f, c4);
}

// Point-to-point constraint
// C = p2 - p1
// Cdot = v2 - v1
//      = v2 + cross(w2, r2) - v1 - cross(w1, r1)
// J = [-I -r1_skew I r2_skew ]
// Identity used:
// w k % (rx i + ry j) = w * (-ry i + rx j)

// Angle constraint
// C = angle2 - angle1 - referenceAngle
// Cdot = w2 - w1
// J = [0 0 -1 0 0 1]
// K = invI1 + invI2

void b2WeldJointDef::Initialize(b2Body *bA, b2Body *bB, const b2Vec2 &anchor) {
    bodyA = bA;
    bodyB = bB;
    localAnchorA = bodyA->GetLocalPoint(anchor);
    localAnchorB = bodyB->GetLocalPoint(anchor);
    referenceAngle = bodyB->GetAngle() - bodyA->GetAngle();
}

b2WeldJoint::b2WeldJoint(const b2WeldJointDef *def) : b2Joint(def) {
    m_localAnchorA = def->localAnchorA;
    m_localAnchorB = def->localAnchorB;
    m_referenceAngle = def->referenceAngle;
    m_stiffness = def->stiffness;
    m_damping = def->damping;

    m_impulse.SetZero();
}

void b2WeldJoint::InitVelocityConstraints(const b2SolverData &data) {
    m_indexA = m_bodyA->m_islandIndex;
    m_indexB = m_bodyB->m_islandIndex;
    m_localCenterA = m_bodyA->m_sweep.localCenter;
    m_localCenterB = m_bodyB->m_sweep.localCenter;
    m_invMassA = m_bodyA->m_invMass;
    m_invMassB = m_bodyB->m_invMass;
    m_invIA = m_bodyA->m_invI;
    m_invIB = m_bodyB->m_invI;

    float aA = data.positions[m_indexA].a;
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;

    float aB = data.positions[m_indexB].a;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    b2Rot qA(aA), qB(aB);

    m_rA = b2Mul(qA, m_localAnchorA - m_localCenterA);
    m_rB = b2Mul(qB, m_localAnchorB - m_localCenterB);

    // J = [-I -r1_skew I r2_skew]
    //     [ 0       -1 0       1]
    // r_skew = [-ry; rx]

    // Matlab
    // K = [ mA+r1y^2*iA+mB+r2y^2*iB,  -r1y*iA*r1x-r2y*iB*r2x,          -r1y*iA-r2y*iB]
    //     [  -r1y*iA*r1x-r2y*iB*r2x, mA+r1x^2*iA+mB+r2x^2*iB,           r1x*iA+r2x*iB]
    //     [          -r1y*iA-r2y*iB,           r1x*iA+r2x*iB,                   iA+iB]

    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    b2Mat33 K;
    K.ex.x = mA + mB + m_rA.y * m_rA.y * iA + m_rB.y * m_rB.y * iB;
    K.ey.x = -m_rA.y * m_rA.x * iA - m_rB.y * m_rB.x * iB;
    K.ez.x = -m_rA.y * iA - m_rB.y * iB;
    K.ex.y = K.ey.x;
    K.ey.y = mA + mB + m_rA.x * m_rA.x * iA + m_rB.x * m_rB.x * iB;
    K.ez.y = m_rA.x * iA + m_rB.x * iB;
    K.ex.z = K.ez.x;
    K.ey.z = K.ez.y;
    K.ez.z = iA + iB;

    if (m_stiffness > 0.0f) {
        K.GetInverse22(&m_mass);

        float invM = iA + iB;

        float C = aB - aA - m_referenceAngle;

        // Damping coefficient
        float d = m_damping;

        // Spring stiffness
        float k = m_stiffness;

        // magic formulas
        float h = data.step.dt;
        m_gamma = h * (d + h * k);
        m_gamma = m_gamma != 0.0f ? 1.0f / m_gamma : 0.0f;
        m_bias = C * h * k * m_gamma;

        invM += m_gamma;
        m_mass.ez.z = invM != 0.0f ? 1.0f / invM : 0.0f;
    } else if (K.ez.z == 0.0f) {
        K.GetInverse22(&m_mass);
        m_gamma = 0.0f;
        m_bias = 0.0f;
    } else {
        K.GetSymInverse33(&m_mass);
        m_gamma = 0.0f;
        m_bias = 0.0f;
    }

    if (data.step.warmStarting) {
        // Scale impulses to support a variable time step.
        m_impulse *= data.step.dtRatio;

        b2Vec2 P(m_impulse.x, m_impulse.y);

        vA -= mA * P;
        wA -= iA * (b2Cross(m_rA, P) + m_impulse.z);

        vB += mB * P;
        wB += iB * (b2Cross(m_rB, P) + m_impulse.z);
    } else {
        m_impulse.SetZero();
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

void b2WeldJoint::SolveVelocityConstraints(const b2SolverData &data) {
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    if (m_stiffness > 0.0f) {
        float Cdot2 = wB - wA;

        float impulse2 = -m_mass.ez.z * (Cdot2 + m_bias + m_gamma * m_impulse.z);
        m_impulse.z += impulse2;

        wA -= iA * impulse2;
        wB += iB * impulse2;

        b2Vec2 Cdot1 = vB + b2Cross(wB, m_rB) - vA - b2Cross(wA, m_rA);

        b2Vec2 impulse1 = -b2Mul22(m_mass, Cdot1);
        m_impulse.x += impulse1.x;
        m_impulse.y += impulse1.y;

        b2Vec2 P = impulse1;

        vA -= mA * P;
        wA -= iA * b2Cross(m_rA, P);

        vB += mB * P;
        wB += iB * b2Cross(m_rB, P);
    } else {
        b2Vec2 Cdot1 = vB + b2Cross(wB, m_rB) - vA - b2Cross(wA, m_rA);
        float Cdot2 = wB - wA;
        b2Vec3 Cdot(Cdot1.x, Cdot1.y, Cdot2);

        b2Vec3 impulse = -b2Mul(m_mass, Cdot);
        m_impulse += impulse;

        b2Vec2 P(impulse.x, impulse.y);

        vA -= mA * P;
        wA -= iA * (b2Cross(m_rA, P) + impulse.z);

        vB += mB * P;
        wB += iB * (b2Cross(m_rB, P) + impulse.z);
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

bool b2WeldJoint::SolvePositionConstraints(const b2SolverData &data) {
    b2Vec2 cA = data.positions[m_indexA].c;
    float aA = data.positions[m_indexA].a;
    b2Vec2 cB = data.positions[m_indexB].c;
    float aB = data.positions[m_indexB].a;

    b2Rot qA(aA), qB(aB);

    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    b2Vec2 rA = b2Mul(qA, m_localAnchorA - m_localCenterA);
    b2Vec2 rB = b2Mul(qB, m_localAnchorB - m_localCenterB);

    float positionError, angularError;

    b2Mat33 K;
    K.ex.x = mA + mB + rA.y * rA.y * iA + rB.y * rB.y * iB;
    K.ey.x = -rA.y * rA.x * iA - rB.y * rB.x * iB;
    K.ez.x = -rA.y * iA - rB.y * iB;
    K.ex.y = K.ey.x;
    K.ey.y = mA + mB + rA.x * rA.x * iA + rB.x * rB.x * iB;
    K.ez.y = rA.x * iA + rB.x * iB;
    K.ex.z = K.ez.x;
    K.ey.z = K.ez.y;
    K.ez.z = iA + iB;

    if (m_stiffness > 0.0f) {
        b2Vec2 C1 = cB + rB - cA - rA;

        positionError = C1.Length();
        angularError = 0.0f;

        b2Vec2 P = -K.Solve22(C1);

        cA -= mA * P;
        aA -= iA * b2Cross(rA, P);

        cB += mB * P;
        aB += iB * b2Cross(rB, P);
    } else {
        b2Vec2 C1 = cB + rB - cA - rA;
        float C2 = aB - aA - m_referenceAngle;

        positionError = C1.Length();
        angularError = b2Abs(C2);

        b2Vec3 C(C1.x, C1.y, C2);

        b2Vec3 impulse;
        if (K.ez.z > 0.0f) {
            impulse = -K.Solve33(C);
        } else {
            b2Vec2 impulse2 = -K.Solve22(C1);
            impulse.Set(impulse2.x, impulse2.y, 0.0f);
        }

        b2Vec2 P(impulse.x, impulse.y);

        cA -= mA * P;
        aA -= iA * (b2Cross(rA, P) + impulse.z);

        cB += mB * P;
        aB += iB * (b2Cross(rB, P) + impulse.z);
    }

    data.positions[m_indexA].c = cA;
    data.positions[m_indexA].a = aA;
    data.positions[m_indexB].c = cB;
    data.positions[m_indexB].a = aB;

    return positionError <= b2_linearSlop && angularError <= b2_angularSlop;
}

b2Vec2 b2WeldJoint::GetAnchorA() const { return m_bodyA->GetWorldPoint(m_localAnchorA); }

b2Vec2 b2WeldJoint::GetAnchorB() const { return m_bodyB->GetWorldPoint(m_localAnchorB); }

b2Vec2 b2WeldJoint::GetReactionForce(float inv_dt) const {
    b2Vec2 P(m_impulse.x, m_impulse.y);
    return inv_dt * P;
}

float b2WeldJoint::GetReactionTorque(float inv_dt) const { return inv_dt * m_impulse.z; }

void b2WeldJoint::Dump() {
    int32 indexA = m_bodyA->m_islandIndex;
    int32 indexB = m_bodyB->m_islandIndex;

    b2Dump("  b2WeldJointDef jd;\n");
    b2Dump("  jd.bodyA = bodies[%d];\n", indexA);
    b2Dump("  jd.bodyB = bodies[%d];\n", indexB);
    b2Dump("  jd.collideConnected = bool(%d);\n", m_collideConnected);
    b2Dump("  jd.localAnchorA.Set(%.9g, %.9g);\n", m_localAnchorA.x, m_localAnchorA.y);
    b2Dump("  jd.localAnchorB.Set(%.9g, %.9g);\n", m_localAnchorB.x, m_localAnchorB.y);
    b2Dump("  jd.referenceAngle = %.9g;\n", m_referenceAngle);
    b2Dump("  jd.stiffness = %.9g;\n", m_stiffness);
    b2Dump("  jd.damping = %.9g;\n", m_damping);
    b2Dump("  joints[%d] = m_world->CreateJoint(&jd);\n", m_index);
}

// Point-to-point constraint
// C = p2 - p1
// Cdot = v2 - v1
//      = v2 + cross(w2, r2) - v1 - cross(w1, r1)
// J = [-I -r1_skew I r2_skew ]
// Identity used:
// w k % (rx i + ry j) = w * (-ry i + rx j)

// Motor constraint
// Cdot = w2 - w1
// J = [0 0 -1 0 0 1]
// K = invI1 + invI2

void b2RevoluteJointDef::Initialize(b2Body *bA, b2Body *bB, const b2Vec2 &anchor) {
    bodyA = bA;
    bodyB = bB;
    localAnchorA = bodyA->GetLocalPoint(anchor);
    localAnchorB = bodyB->GetLocalPoint(anchor);
    referenceAngle = bodyB->GetAngle() - bodyA->GetAngle();
}

b2RevoluteJoint::b2RevoluteJoint(const b2RevoluteJointDef *def) : b2Joint(def) {
    m_localAnchorA = def->localAnchorA;
    m_localAnchorB = def->localAnchorB;
    m_referenceAngle = def->referenceAngle;

    m_impulse.SetZero();
    m_axialMass = 0.0f;
    m_motorImpulse = 0.0f;
    m_lowerImpulse = 0.0f;
    m_upperImpulse = 0.0f;

    m_lowerAngle = def->lowerAngle;
    m_upperAngle = def->upperAngle;
    m_maxMotorTorque = def->maxMotorTorque;
    m_motorSpeed = def->motorSpeed;
    m_enableLimit = def->enableLimit;
    m_enableMotor = def->enableMotor;

    m_angle = 0.0f;
}

void b2RevoluteJoint::InitVelocityConstraints(const b2SolverData &data) {
    m_indexA = m_bodyA->m_islandIndex;
    m_indexB = m_bodyB->m_islandIndex;
    m_localCenterA = m_bodyA->m_sweep.localCenter;
    m_localCenterB = m_bodyB->m_sweep.localCenter;
    m_invMassA = m_bodyA->m_invMass;
    m_invMassB = m_bodyB->m_invMass;
    m_invIA = m_bodyA->m_invI;
    m_invIB = m_bodyB->m_invI;

    float aA = data.positions[m_indexA].a;
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;

    float aB = data.positions[m_indexB].a;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    b2Rot qA(aA), qB(aB);

    m_rA = b2Mul(qA, m_localAnchorA - m_localCenterA);
    m_rB = b2Mul(qB, m_localAnchorB - m_localCenterB);

    // J = [-I -r1_skew I r2_skew]
    // r_skew = [-ry; rx]

    // Matlab
    // K = [ mA+r1y^2*iA+mB+r2y^2*iB,  -r1y*iA*r1x-r2y*iB*r2x]
    //     [  -r1y*iA*r1x-r2y*iB*r2x, mA+r1x^2*iA+mB+r2x^2*iB]

    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    m_K.ex.x = mA + mB + m_rA.y * m_rA.y * iA + m_rB.y * m_rB.y * iB;
    m_K.ey.x = -m_rA.y * m_rA.x * iA - m_rB.y * m_rB.x * iB;
    m_K.ex.y = m_K.ey.x;
    m_K.ey.y = mA + mB + m_rA.x * m_rA.x * iA + m_rB.x * m_rB.x * iB;

    m_axialMass = iA + iB;
    bool fixedRotation;
    if (m_axialMass > 0.0f) {
        m_axialMass = 1.0f / m_axialMass;
        fixedRotation = false;
    } else {
        fixedRotation = true;
    }

    m_angle = aB - aA - m_referenceAngle;
    if (m_enableLimit == false || fixedRotation) {
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }

    if (m_enableMotor == false || fixedRotation) { m_motorImpulse = 0.0f; }

    if (data.step.warmStarting) {
        // Scale impulses to support a variable time step.
        m_impulse *= data.step.dtRatio;
        m_motorImpulse *= data.step.dtRatio;
        m_lowerImpulse *= data.step.dtRatio;
        m_upperImpulse *= data.step.dtRatio;

        float axialImpulse = m_motorImpulse + m_lowerImpulse - m_upperImpulse;
        b2Vec2 P(m_impulse.x, m_impulse.y);

        vA -= mA * P;
        wA -= iA * (b2Cross(m_rA, P) + axialImpulse);

        vB += mB * P;
        wB += iB * (b2Cross(m_rB, P) + axialImpulse);
    } else {
        m_impulse.SetZero();
        m_motorImpulse = 0.0f;
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

void b2RevoluteJoint::SolveVelocityConstraints(const b2SolverData &data) {
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    bool fixedRotation = (iA + iB == 0.0f);

    // Solve motor constraint.
    if (m_enableMotor && fixedRotation == false) {
        float Cdot = wB - wA - m_motorSpeed;
        float impulse = -m_axialMass * Cdot;
        float oldImpulse = m_motorImpulse;
        float maxImpulse = data.step.dt * m_maxMotorTorque;
        m_motorImpulse = b2Clamp(m_motorImpulse + impulse, -maxImpulse, maxImpulse);
        impulse = m_motorImpulse - oldImpulse;

        wA -= iA * impulse;
        wB += iB * impulse;
    }

    if (m_enableLimit && fixedRotation == false) {
        // Lower limit
        {
            float C = m_angle - m_lowerAngle;
            float Cdot = wB - wA;
            float impulse = -m_axialMass * (Cdot + b2Max(C, 0.0f) * data.step.inv_dt);
            float oldImpulse = m_lowerImpulse;
            m_lowerImpulse = b2Max(m_lowerImpulse + impulse, 0.0f);
            impulse = m_lowerImpulse - oldImpulse;

            wA -= iA * impulse;
            wB += iB * impulse;
        }

        // Upper limit
        // Note: signs are flipped to keep C positive when the constraint is satisfied.
        // This also keeps the impulse positive when the limit is active.
        {
            float C = m_upperAngle - m_angle;
            float Cdot = wA - wB;
            float impulse = -m_axialMass * (Cdot + b2Max(C, 0.0f) * data.step.inv_dt);
            float oldImpulse = m_upperImpulse;
            m_upperImpulse = b2Max(m_upperImpulse + impulse, 0.0f);
            impulse = m_upperImpulse - oldImpulse;

            wA += iA * impulse;
            wB -= iB * impulse;
        }
    }

    // Solve point-to-point constraint
    {
        b2Vec2 Cdot = vB + b2Cross(wB, m_rB) - vA - b2Cross(wA, m_rA);
        b2Vec2 impulse = m_K.Solve(-Cdot);

        m_impulse.x += impulse.x;
        m_impulse.y += impulse.y;

        vA -= mA * impulse;
        wA -= iA * b2Cross(m_rA, impulse);

        vB += mB * impulse;
        wB += iB * b2Cross(m_rB, impulse);
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

bool b2RevoluteJoint::SolvePositionConstraints(const b2SolverData &data) {
    b2Vec2 cA = data.positions[m_indexA].c;
    float aA = data.positions[m_indexA].a;
    b2Vec2 cB = data.positions[m_indexB].c;
    float aB = data.positions[m_indexB].a;

    b2Rot qA(aA), qB(aB);

    float angularError = 0.0f;
    float positionError = 0.0f;

    bool fixedRotation = (m_invIA + m_invIB == 0.0f);

    // Solve angular limit constraint
    if (m_enableLimit && fixedRotation == false) {
        float angle = aB - aA - m_referenceAngle;
        float C = 0.0f;

        if (b2Abs(m_upperAngle - m_lowerAngle) < 2.0f * b2_angularSlop) {
            // Prevent large angular corrections
            C = b2Clamp(angle - m_lowerAngle, -b2_maxAngularCorrection, b2_maxAngularCorrection);
        } else if (angle <= m_lowerAngle) {
            // Prevent large angular corrections and allow some slop.
            C = b2Clamp(angle - m_lowerAngle + b2_angularSlop, -b2_maxAngularCorrection, 0.0f);
        } else if (angle >= m_upperAngle) {
            // Prevent large angular corrections and allow some slop.
            C = b2Clamp(angle - m_upperAngle - b2_angularSlop, 0.0f, b2_maxAngularCorrection);
        }

        float limitImpulse = -m_axialMass * C;
        aA -= m_invIA * limitImpulse;
        aB += m_invIB * limitImpulse;
        angularError = b2Abs(C);
    }

    // Solve point-to-point constraint.
    {
        qA.Set(aA);
        qB.Set(aB);
        b2Vec2 rA = b2Mul(qA, m_localAnchorA - m_localCenterA);
        b2Vec2 rB = b2Mul(qB, m_localAnchorB - m_localCenterB);

        b2Vec2 C = cB + rB - cA - rA;
        positionError = C.Length();

        float mA = m_invMassA, mB = m_invMassB;
        float iA = m_invIA, iB = m_invIB;

        b2Mat22 K;
        K.ex.x = mA + mB + iA * rA.y * rA.y + iB * rB.y * rB.y;
        K.ex.y = -iA * rA.x * rA.y - iB * rB.x * rB.y;
        K.ey.x = K.ex.y;
        K.ey.y = mA + mB + iA * rA.x * rA.x + iB * rB.x * rB.x;

        b2Vec2 impulse = -K.Solve(C);

        cA -= mA * impulse;
        aA -= iA * b2Cross(rA, impulse);

        cB += mB * impulse;
        aB += iB * b2Cross(rB, impulse);
    }

    data.positions[m_indexA].c = cA;
    data.positions[m_indexA].a = aA;
    data.positions[m_indexB].c = cB;
    data.positions[m_indexB].a = aB;

    return positionError <= b2_linearSlop && angularError <= b2_angularSlop;
}

b2Vec2 b2RevoluteJoint::GetAnchorA() const { return m_bodyA->GetWorldPoint(m_localAnchorA); }

b2Vec2 b2RevoluteJoint::GetAnchorB() const { return m_bodyB->GetWorldPoint(m_localAnchorB); }

b2Vec2 b2RevoluteJoint::GetReactionForce(float inv_dt) const {
    b2Vec2 P(m_impulse.x, m_impulse.y);
    return inv_dt * P;
}

float b2RevoluteJoint::GetReactionTorque(float inv_dt) const {
    return inv_dt * (m_motorImpulse + m_lowerImpulse - m_upperImpulse);
}

float b2RevoluteJoint::GetJointAngle() const {
    b2Body *bA = m_bodyA;
    b2Body *bB = m_bodyB;
    return bB->m_sweep.a - bA->m_sweep.a - m_referenceAngle;
}

float b2RevoluteJoint::GetJointSpeed() const {
    b2Body *bA = m_bodyA;
    b2Body *bB = m_bodyB;
    return bB->m_angularVelocity - bA->m_angularVelocity;
}

bool b2RevoluteJoint::IsMotorEnabled() const { return m_enableMotor; }

void b2RevoluteJoint::EnableMotor(bool flag) {
    if (flag != m_enableMotor) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_enableMotor = flag;
    }
}

float b2RevoluteJoint::GetMotorTorque(float inv_dt) const { return inv_dt * m_motorImpulse; }

void b2RevoluteJoint::SetMotorSpeed(float speed) {
    if (speed != m_motorSpeed) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_motorSpeed = speed;
    }
}

void b2RevoluteJoint::SetMaxMotorTorque(float torque) {
    if (torque != m_maxMotorTorque) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_maxMotorTorque = torque;
    }
}

bool b2RevoluteJoint::IsLimitEnabled() const { return m_enableLimit; }

void b2RevoluteJoint::EnableLimit(bool flag) {
    if (flag != m_enableLimit) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_enableLimit = flag;
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }
}

float b2RevoluteJoint::GetLowerLimit() const { return m_lowerAngle; }

float b2RevoluteJoint::GetUpperLimit() const { return m_upperAngle; }

void b2RevoluteJoint::SetLimits(float lower, float upper) {
    METADOT_ASSERT_E(lower <= upper);

    if (lower != m_lowerAngle || upper != m_upperAngle) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
        m_lowerAngle = lower;
        m_upperAngle = upper;
    }
}

void b2RevoluteJoint::Dump() {
    int32 indexA = m_bodyA->m_islandIndex;
    int32 indexB = m_bodyB->m_islandIndex;

    b2Dump("  b2RevoluteJointDef jd;\n");
    b2Dump("  jd.bodyA = bodies[%d];\n", indexA);
    b2Dump("  jd.bodyB = bodies[%d];\n", indexB);
    b2Dump("  jd.collideConnected = bool(%d);\n", m_collideConnected);
    b2Dump("  jd.localAnchorA.Set(%.9g, %.9g);\n", m_localAnchorA.x, m_localAnchorA.y);
    b2Dump("  jd.localAnchorB.Set(%.9g, %.9g);\n", m_localAnchorB.x, m_localAnchorB.y);
    b2Dump("  jd.referenceAngle = %.9g;\n", m_referenceAngle);
    b2Dump("  jd.enableLimit = bool(%d);\n", m_enableLimit);
    b2Dump("  jd.lowerAngle = %.9g;\n", m_lowerAngle);
    b2Dump("  jd.upperAngle = %.9g;\n", m_upperAngle);
    b2Dump("  jd.enableMotor = bool(%d);\n", m_enableMotor);
    b2Dump("  jd.motorSpeed = %.9g;\n", m_motorSpeed);
    b2Dump("  jd.maxMotorTorque = %.9g;\n", m_maxMotorTorque);
    b2Dump("  joints[%d] = m_world->CreateJoint(&jd);\n", m_index);
}

///
void b2RevoluteJoint::Draw(DebugDraw *draw) const {
    const b2Transform &xfA = m_bodyA->GetTransform();
    const b2Transform &xfB = m_bodyB->GetTransform();
    b2Vec2 pA = b2Mul(xfA, m_localAnchorA);
    b2Vec2 pB = b2Mul(xfB, m_localAnchorB);

    b2Color c1(0.7f, 0.7f, 0.7f);
    b2Color c2(0.3f, 0.9f, 0.3f);
    b2Color c3(0.9f, 0.3f, 0.3f);
    b2Color c4(0.3f, 0.3f, 0.9f);
    b2Color c5(0.4f, 0.4f, 0.4f);

    draw->DrawPoint(pA, 5.0f, c4);
    draw->DrawPoint(pB, 5.0f, c5);

    float aA = m_bodyA->GetAngle();
    float aB = m_bodyB->GetAngle();
    float angle = aB - aA - m_referenceAngle;

    const float L = 0.5f;

    b2Vec2 r = L * b2Vec2(cosf(angle), sinf(angle));
    draw->DrawSegment(pB, pB + r, c1);
    draw->DrawCircle(pB, L, c1);

    if (m_enableLimit) {
        b2Vec2 rlo = L * b2Vec2(cosf(m_lowerAngle), sinf(m_lowerAngle));
        b2Vec2 rhi = L * b2Vec2(cosf(m_upperAngle), sinf(m_upperAngle));

        draw->DrawSegment(pB, pB + rlo, c2);
        draw->DrawSegment(pB, pB + rhi, c3);
    }

    b2Color color(0.5f, 0.8f, 0.8f);
    draw->DrawSegment(xfA.p, pA, color);
    draw->DrawSegment(pA, pB, color);
    draw->DrawSegment(xfB.p, pB, color);
}

// Pulley:
// length1 = norm(p1 - s1)
// length2 = norm(p2 - s2)
// C0 = (length1 + ratio * length2)_initial
// C = C0 - (length1 + ratio * length2)
// u1 = (p1 - s1) / norm(p1 - s1)
// u2 = (p2 - s2) / norm(p2 - s2)
// Cdot = -dot(u1, v1 + cross(w1, r1)) - ratio * dot(u2, v2 + cross(w2, r2))
// J = -[u1 cross(r1, u1) ratio * u2  ratio * cross(r2, u2)]
// K = J * invM * JT
//   = invMass1 + invI1 * cross(r1, u1)^2 + ratio^2 * (invMass2 + invI2 * cross(r2, u2)^2)

void b2PulleyJointDef::Initialize(b2Body *bA, b2Body *bB, const b2Vec2 &groundA,
                                  const b2Vec2 &groundB, const b2Vec2 &anchorA,
                                  const b2Vec2 &anchorB, float r) {
    bodyA = bA;
    bodyB = bB;
    groundAnchorA = groundA;
    groundAnchorB = groundB;
    localAnchorA = bodyA->GetLocalPoint(anchorA);
    localAnchorB = bodyB->GetLocalPoint(anchorB);
    b2Vec2 dA = anchorA - groundA;
    lengthA = dA.Length();
    b2Vec2 dB = anchorB - groundB;
    lengthB = dB.Length();
    ratio = r;
    METADOT_ASSERT_E(ratio > b2_epsilon);
}

b2PulleyJoint::b2PulleyJoint(const b2PulleyJointDef *def) : b2Joint(def) {
    m_groundAnchorA = def->groundAnchorA;
    m_groundAnchorB = def->groundAnchorB;
    m_localAnchorA = def->localAnchorA;
    m_localAnchorB = def->localAnchorB;

    m_lengthA = def->lengthA;
    m_lengthB = def->lengthB;

    METADOT_ASSERT_E(def->ratio != 0.0f);
    m_ratio = def->ratio;

    m_constant = def->lengthA + m_ratio * def->lengthB;

    m_impulse = 0.0f;
}

void b2PulleyJoint::InitVelocityConstraints(const b2SolverData &data) {
    m_indexA = m_bodyA->m_islandIndex;
    m_indexB = m_bodyB->m_islandIndex;
    m_localCenterA = m_bodyA->m_sweep.localCenter;
    m_localCenterB = m_bodyB->m_sweep.localCenter;
    m_invMassA = m_bodyA->m_invMass;
    m_invMassB = m_bodyB->m_invMass;
    m_invIA = m_bodyA->m_invI;
    m_invIB = m_bodyB->m_invI;

    b2Vec2 cA = data.positions[m_indexA].c;
    float aA = data.positions[m_indexA].a;
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;

    b2Vec2 cB = data.positions[m_indexB].c;
    float aB = data.positions[m_indexB].a;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    b2Rot qA(aA), qB(aB);

    m_rA = b2Mul(qA, m_localAnchorA - m_localCenterA);
    m_rB = b2Mul(qB, m_localAnchorB - m_localCenterB);

    // Get the pulley axes.
    m_uA = cA + m_rA - m_groundAnchorA;
    m_uB = cB + m_rB - m_groundAnchorB;

    float lengthA = m_uA.Length();
    float lengthB = m_uB.Length();

    if (lengthA > 10.0f * b2_linearSlop) {
        m_uA *= 1.0f / lengthA;
    } else {
        m_uA.SetZero();
    }

    if (lengthB > 10.0f * b2_linearSlop) {
        m_uB *= 1.0f / lengthB;
    } else {
        m_uB.SetZero();
    }

    // Compute effective mass.
    float ruA = b2Cross(m_rA, m_uA);
    float ruB = b2Cross(m_rB, m_uB);

    float mA = m_invMassA + m_invIA * ruA * ruA;
    float mB = m_invMassB + m_invIB * ruB * ruB;

    m_mass = mA + m_ratio * m_ratio * mB;

    if (m_mass > 0.0f) { m_mass = 1.0f / m_mass; }

    if (data.step.warmStarting) {
        // Scale impulses to support variable time steps.
        m_impulse *= data.step.dtRatio;

        // Warm starting.
        b2Vec2 PA = -(m_impulse) *m_uA;
        b2Vec2 PB = (-m_ratio * m_impulse) * m_uB;

        vA += m_invMassA * PA;
        wA += m_invIA * b2Cross(m_rA, PA);
        vB += m_invMassB * PB;
        wB += m_invIB * b2Cross(m_rB, PB);
    } else {
        m_impulse = 0.0f;
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

void b2PulleyJoint::SolveVelocityConstraints(const b2SolverData &data) {
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    b2Vec2 vpA = vA + b2Cross(wA, m_rA);
    b2Vec2 vpB = vB + b2Cross(wB, m_rB);

    float Cdot = -b2Dot(m_uA, vpA) - m_ratio * b2Dot(m_uB, vpB);
    float impulse = -m_mass * Cdot;
    m_impulse += impulse;

    b2Vec2 PA = -impulse * m_uA;
    b2Vec2 PB = -m_ratio * impulse * m_uB;
    vA += m_invMassA * PA;
    wA += m_invIA * b2Cross(m_rA, PA);
    vB += m_invMassB * PB;
    wB += m_invIB * b2Cross(m_rB, PB);

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

bool b2PulleyJoint::SolvePositionConstraints(const b2SolverData &data) {
    b2Vec2 cA = data.positions[m_indexA].c;
    float aA = data.positions[m_indexA].a;
    b2Vec2 cB = data.positions[m_indexB].c;
    float aB = data.positions[m_indexB].a;

    b2Rot qA(aA), qB(aB);

    b2Vec2 rA = b2Mul(qA, m_localAnchorA - m_localCenterA);
    b2Vec2 rB = b2Mul(qB, m_localAnchorB - m_localCenterB);

    // Get the pulley axes.
    b2Vec2 uA = cA + rA - m_groundAnchorA;
    b2Vec2 uB = cB + rB - m_groundAnchorB;

    float lengthA = uA.Length();
    float lengthB = uB.Length();

    if (lengthA > 10.0f * b2_linearSlop) {
        uA *= 1.0f / lengthA;
    } else {
        uA.SetZero();
    }

    if (lengthB > 10.0f * b2_linearSlop) {
        uB *= 1.0f / lengthB;
    } else {
        uB.SetZero();
    }

    // Compute effective mass.
    float ruA = b2Cross(rA, uA);
    float ruB = b2Cross(rB, uB);

    float mA = m_invMassA + m_invIA * ruA * ruA;
    float mB = m_invMassB + m_invIB * ruB * ruB;

    float mass = mA + m_ratio * m_ratio * mB;

    if (mass > 0.0f) { mass = 1.0f / mass; }

    float C = m_constant - lengthA - m_ratio * lengthB;
    float linearError = b2Abs(C);

    float impulse = -mass * C;

    b2Vec2 PA = -impulse * uA;
    b2Vec2 PB = -m_ratio * impulse * uB;

    cA += m_invMassA * PA;
    aA += m_invIA * b2Cross(rA, PA);
    cB += m_invMassB * PB;
    aB += m_invIB * b2Cross(rB, PB);

    data.positions[m_indexA].c = cA;
    data.positions[m_indexA].a = aA;
    data.positions[m_indexB].c = cB;
    data.positions[m_indexB].a = aB;

    return linearError < b2_linearSlop;
}

b2Vec2 b2PulleyJoint::GetAnchorA() const { return m_bodyA->GetWorldPoint(m_localAnchorA); }

b2Vec2 b2PulleyJoint::GetAnchorB() const { return m_bodyB->GetWorldPoint(m_localAnchorB); }

b2Vec2 b2PulleyJoint::GetReactionForce(float inv_dt) const {
    b2Vec2 P = m_impulse * m_uB;
    return inv_dt * P;
}

float b2PulleyJoint::GetReactionTorque(float inv_dt) const {
    B2_NOT_USED(inv_dt);
    return 0.0f;
}

b2Vec2 b2PulleyJoint::GetGroundAnchorA() const { return m_groundAnchorA; }

b2Vec2 b2PulleyJoint::GetGroundAnchorB() const { return m_groundAnchorB; }

float b2PulleyJoint::GetLengthA() const { return m_lengthA; }

float b2PulleyJoint::GetLengthB() const { return m_lengthB; }

float b2PulleyJoint::GetRatio() const { return m_ratio; }

float b2PulleyJoint::GetCurrentLengthA() const {
    b2Vec2 p = m_bodyA->GetWorldPoint(m_localAnchorA);
    b2Vec2 s = m_groundAnchorA;
    b2Vec2 d = p - s;
    return d.Length();
}

float b2PulleyJoint::GetCurrentLengthB() const {
    b2Vec2 p = m_bodyB->GetWorldPoint(m_localAnchorB);
    b2Vec2 s = m_groundAnchorB;
    b2Vec2 d = p - s;
    return d.Length();
}

void b2PulleyJoint::Dump() {
    int32 indexA = m_bodyA->m_islandIndex;
    int32 indexB = m_bodyB->m_islandIndex;

    b2Dump("  b2PulleyJointDef jd;\n");
    b2Dump("  jd.bodyA = bodies[%d];\n", indexA);
    b2Dump("  jd.bodyB = bodies[%d];\n", indexB);
    b2Dump("  jd.collideConnected = bool(%d);\n", m_collideConnected);
    b2Dump("  jd.groundAnchorA.Set(%.9g, %.9g);\n", m_groundAnchorA.x, m_groundAnchorA.y);
    b2Dump("  jd.groundAnchorB.Set(%.9g, %.9g);\n", m_groundAnchorB.x, m_groundAnchorB.y);
    b2Dump("  jd.localAnchorA.Set(%.9g, %.9g);\n", m_localAnchorA.x, m_localAnchorA.y);
    b2Dump("  jd.localAnchorB.Set(%.9g, %.9g);\n", m_localAnchorB.x, m_localAnchorB.y);
    b2Dump("  jd.lengthA = %.9g;\n", m_lengthA);
    b2Dump("  jd.lengthB = %.9g;\n", m_lengthB);
    b2Dump("  jd.ratio = %.9g;\n", m_ratio);
    b2Dump("  joints[%d] = m_world->CreateJoint(&jd);\n", m_index);
}

void b2PulleyJoint::ShiftOrigin(const b2Vec2 &newOrigin) {
    m_groundAnchorA -= newOrigin;
    m_groundAnchorB -= newOrigin;
}

// Linear constraint (point-to-line)
// d = p2 - p1 = x2 + r2 - x1 - r1
// C = dot(perp, d)
// Cdot = dot(d, cross(w1, perp)) + dot(perp, v2 + cross(w2, r2) - v1 - cross(w1, r1))
//      = -dot(perp, v1) - dot(cross(d + r1, perp), w1) + dot(perp, v2) + dot(cross(r2, perp), v2)
// J = [-perp, -cross(d + r1, perp), perp, cross(r2,perp)]
//
// Angular constraint
// C = a2 - a1 + a_initial
// Cdot = w2 - w1
// J = [0 0 -1 0 0 1]
//
// K = J * invM * JT
//
// J = [-a -s1 a s2]
//     [0  -1  0  1]
// a = perp
// s1 = cross(d + r1, a) = cross(p2 - x1, a)
// s2 = cross(r2, a) = cross(p2 - x2, a)

// Motor/Limit linear constraint
// C = dot(ax1, d)
// Cdot = -dot(ax1, v1) - dot(cross(d + r1, ax1), w1) + dot(ax1, v2) + dot(cross(r2, ax1), v2)
// J = [-ax1 -cross(d+r1,ax1) ax1 cross(r2,ax1)]

// Predictive limit is applied even when the limit is not active.
// Prevents a constraint speed that can lead to a constraint error in one time step.
// Want C2 = C1 + h * Cdot >= 0
// Or:
// Cdot + C1/h >= 0
// I do not apply a negative constraint error because that is handled in position correction.
// So:
// Cdot + max(C1, 0)/h >= 0

// Block Solver
// We develop a block solver that includes the angular and linear constraints. This makes the limit stiffer.
//
// The Jacobian has 2 rows:
// J = [-uT -s1 uT s2] // linear
//     [0   -1   0  1] // angular
//
// u = perp
// s1 = cross(d + r1, u), s2 = cross(r2, u)
// a1 = cross(d + r1, v), a2 = cross(r2, v)

void b2PrismaticJointDef::Initialize(b2Body *bA, b2Body *bB, const b2Vec2 &anchor,
                                     const b2Vec2 &axis) {
    bodyA = bA;
    bodyB = bB;
    localAnchorA = bodyA->GetLocalPoint(anchor);
    localAnchorB = bodyB->GetLocalPoint(anchor);
    localAxisA = bodyA->GetLocalVector(axis);
    referenceAngle = bodyB->GetAngle() - bodyA->GetAngle();
}

b2PrismaticJoint::b2PrismaticJoint(const b2PrismaticJointDef *def) : b2Joint(def) {
    m_localAnchorA = def->localAnchorA;
    m_localAnchorB = def->localAnchorB;
    m_localXAxisA = def->localAxisA;
    m_localXAxisA.Normalize();
    m_localYAxisA = b2Cross(1.0f, m_localXAxisA);
    m_referenceAngle = def->referenceAngle;

    m_impulse.SetZero();
    m_axialMass = 0.0f;
    m_motorImpulse = 0.0f;
    m_lowerImpulse = 0.0f;
    m_upperImpulse = 0.0f;

    m_lowerTranslation = def->lowerTranslation;
    m_upperTranslation = def->upperTranslation;

    METADOT_ASSERT_E(m_lowerTranslation <= m_upperTranslation);

    m_maxMotorForce = def->maxMotorForce;
    m_motorSpeed = def->motorSpeed;
    m_enableLimit = def->enableLimit;
    m_enableMotor = def->enableMotor;

    m_translation = 0.0f;
    m_axis.SetZero();
    m_perp.SetZero();
}

void b2PrismaticJoint::InitVelocityConstraints(const b2SolverData &data) {
    m_indexA = m_bodyA->m_islandIndex;
    m_indexB = m_bodyB->m_islandIndex;
    m_localCenterA = m_bodyA->m_sweep.localCenter;
    m_localCenterB = m_bodyB->m_sweep.localCenter;
    m_invMassA = m_bodyA->m_invMass;
    m_invMassB = m_bodyB->m_invMass;
    m_invIA = m_bodyA->m_invI;
    m_invIB = m_bodyB->m_invI;

    b2Vec2 cA = data.positions[m_indexA].c;
    float aA = data.positions[m_indexA].a;
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;

    b2Vec2 cB = data.positions[m_indexB].c;
    float aB = data.positions[m_indexB].a;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    b2Rot qA(aA), qB(aB);

    // Compute the effective masses.
    b2Vec2 rA = b2Mul(qA, m_localAnchorA - m_localCenterA);
    b2Vec2 rB = b2Mul(qB, m_localAnchorB - m_localCenterB);
    b2Vec2 d = (cB - cA) + rB - rA;

    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    // Compute motor Jacobian and effective mass.
    {
        m_axis = b2Mul(qA, m_localXAxisA);
        m_a1 = b2Cross(d + rA, m_axis);
        m_a2 = b2Cross(rB, m_axis);

        m_axialMass = mA + mB + iA * m_a1 * m_a1 + iB * m_a2 * m_a2;
        if (m_axialMass > 0.0f) { m_axialMass = 1.0f / m_axialMass; }
    }

    // Prismatic constraint.
    {
        m_perp = b2Mul(qA, m_localYAxisA);

        m_s1 = b2Cross(d + rA, m_perp);
        m_s2 = b2Cross(rB, m_perp);

        float k11 = mA + mB + iA * m_s1 * m_s1 + iB * m_s2 * m_s2;
        float k12 = iA * m_s1 + iB * m_s2;
        float k22 = iA + iB;
        if (k22 == 0.0f) {
            // For bodies with fixed rotation.
            k22 = 1.0f;
        }

        m_K.ex.Set(k11, k12);
        m_K.ey.Set(k12, k22);
    }

    if (m_enableLimit) {
        m_translation = b2Dot(m_axis, d);
    } else {
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }

    if (m_enableMotor == false) { m_motorImpulse = 0.0f; }

    if (data.step.warmStarting) {
        // Account for variable time step.
        m_impulse *= data.step.dtRatio;
        m_motorImpulse *= data.step.dtRatio;
        m_lowerImpulse *= data.step.dtRatio;
        m_upperImpulse *= data.step.dtRatio;

        float axialImpulse = m_motorImpulse + m_lowerImpulse - m_upperImpulse;
        b2Vec2 P = m_impulse.x * m_perp + axialImpulse * m_axis;
        float LA = m_impulse.x * m_s1 + m_impulse.y + axialImpulse * m_a1;
        float LB = m_impulse.x * m_s2 + m_impulse.y + axialImpulse * m_a2;

        vA -= mA * P;
        wA -= iA * LA;

        vB += mB * P;
        wB += iB * LB;
    } else {
        m_impulse.SetZero();
        m_motorImpulse = 0.0f;
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

void b2PrismaticJoint::SolveVelocityConstraints(const b2SolverData &data) {
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    // Solve linear motor constraint
    if (m_enableMotor) {
        float Cdot = b2Dot(m_axis, vB - vA) + m_a2 * wB - m_a1 * wA;
        float impulse = m_axialMass * (m_motorSpeed - Cdot);
        float oldImpulse = m_motorImpulse;
        float maxImpulse = data.step.dt * m_maxMotorForce;
        m_motorImpulse = b2Clamp(m_motorImpulse + impulse, -maxImpulse, maxImpulse);
        impulse = m_motorImpulse - oldImpulse;

        b2Vec2 P = impulse * m_axis;
        float LA = impulse * m_a1;
        float LB = impulse * m_a2;

        vA -= mA * P;
        wA -= iA * LA;
        vB += mB * P;
        wB += iB * LB;
    }

    if (m_enableLimit) {
        // Lower limit
        {
            float C = m_translation - m_lowerTranslation;
            float Cdot = b2Dot(m_axis, vB - vA) + m_a2 * wB - m_a1 * wA;
            float impulse = -m_axialMass * (Cdot + b2Max(C, 0.0f) * data.step.inv_dt);
            float oldImpulse = m_lowerImpulse;
            m_lowerImpulse = b2Max(m_lowerImpulse + impulse, 0.0f);
            impulse = m_lowerImpulse - oldImpulse;

            b2Vec2 P = impulse * m_axis;
            float LA = impulse * m_a1;
            float LB = impulse * m_a2;

            vA -= mA * P;
            wA -= iA * LA;
            vB += mB * P;
            wB += iB * LB;
        }

        // Upper limit
        // Note: signs are flipped to keep C positive when the constraint is satisfied.
        // This also keeps the impulse positive when the limit is active.
        {
            float C = m_upperTranslation - m_translation;
            float Cdot = b2Dot(m_axis, vA - vB) + m_a1 * wA - m_a2 * wB;
            float impulse = -m_axialMass * (Cdot + b2Max(C, 0.0f) * data.step.inv_dt);
            float oldImpulse = m_upperImpulse;
            m_upperImpulse = b2Max(m_upperImpulse + impulse, 0.0f);
            impulse = m_upperImpulse - oldImpulse;

            b2Vec2 P = impulse * m_axis;
            float LA = impulse * m_a1;
            float LB = impulse * m_a2;

            vA += mA * P;
            wA += iA * LA;
            vB -= mB * P;
            wB -= iB * LB;
        }
    }

    // Solve the prismatic constraint in block form.
    {
        b2Vec2 Cdot;
        Cdot.x = b2Dot(m_perp, vB - vA) + m_s2 * wB - m_s1 * wA;
        Cdot.y = wB - wA;

        b2Vec2 df = m_K.Solve(-Cdot);
        m_impulse += df;

        b2Vec2 P = df.x * m_perp;
        float LA = df.x * m_s1 + df.y;
        float LB = df.x * m_s2 + df.y;

        vA -= mA * P;
        wA -= iA * LA;

        vB += mB * P;
        wB += iB * LB;
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

// A velocity based solver computes reaction forces(impulses) using the velocity constraint solver.Under this context,
// the position solver is not there to resolve forces.It is only there to cope with integration error.
//
// Therefore, the pseudo impulses in the position solver do not have any physical meaning.Thus it is okay if they suck.
//
// We could take the active state from the velocity solver.However, the joint might push past the limit when the velocity
// solver indicates the limit is inactive.
bool b2PrismaticJoint::SolvePositionConstraints(const b2SolverData &data) {
    b2Vec2 cA = data.positions[m_indexA].c;
    float aA = data.positions[m_indexA].a;
    b2Vec2 cB = data.positions[m_indexB].c;
    float aB = data.positions[m_indexB].a;

    b2Rot qA(aA), qB(aB);

    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    // Compute fresh Jacobians
    b2Vec2 rA = b2Mul(qA, m_localAnchorA - m_localCenterA);
    b2Vec2 rB = b2Mul(qB, m_localAnchorB - m_localCenterB);
    b2Vec2 d = cB + rB - cA - rA;

    b2Vec2 axis = b2Mul(qA, m_localXAxisA);
    float a1 = b2Cross(d + rA, axis);
    float a2 = b2Cross(rB, axis);
    b2Vec2 perp = b2Mul(qA, m_localYAxisA);

    float s1 = b2Cross(d + rA, perp);
    float s2 = b2Cross(rB, perp);

    b2Vec3 impulse;
    b2Vec2 C1;
    C1.x = b2Dot(perp, d);
    C1.y = aB - aA - m_referenceAngle;

    float linearError = b2Abs(C1.x);
    float angularError = b2Abs(C1.y);

    bool active = false;
    float C2 = 0.0f;
    if (m_enableLimit) {
        float translation = b2Dot(axis, d);
        if (b2Abs(m_upperTranslation - m_lowerTranslation) < 2.0f * b2_linearSlop) {
            C2 = translation;
            linearError = b2Max(linearError, b2Abs(translation));
            active = true;
        } else if (translation <= m_lowerTranslation) {
            C2 = b2Min(translation - m_lowerTranslation, 0.0f);
            linearError = b2Max(linearError, m_lowerTranslation - translation);
            active = true;
        } else if (translation >= m_upperTranslation) {
            C2 = b2Max(translation - m_upperTranslation, 0.0f);
            linearError = b2Max(linearError, translation - m_upperTranslation);
            active = true;
        }
    }

    if (active) {
        float k11 = mA + mB + iA * s1 * s1 + iB * s2 * s2;
        float k12 = iA * s1 + iB * s2;
        float k13 = iA * s1 * a1 + iB * s2 * a2;
        float k22 = iA + iB;
        if (k22 == 0.0f) {
            // For fixed rotation
            k22 = 1.0f;
        }
        float k23 = iA * a1 + iB * a2;
        float k33 = mA + mB + iA * a1 * a1 + iB * a2 * a2;

        b2Mat33 K;
        K.ex.Set(k11, k12, k13);
        K.ey.Set(k12, k22, k23);
        K.ez.Set(k13, k23, k33);

        b2Vec3 C;
        C.x = C1.x;
        C.y = C1.y;
        C.z = C2;

        impulse = K.Solve33(-C);
    } else {
        float k11 = mA + mB + iA * s1 * s1 + iB * s2 * s2;
        float k12 = iA * s1 + iB * s2;
        float k22 = iA + iB;
        if (k22 == 0.0f) { k22 = 1.0f; }

        b2Mat22 K;
        K.ex.Set(k11, k12);
        K.ey.Set(k12, k22);

        b2Vec2 impulse1 = K.Solve(-C1);
        impulse.x = impulse1.x;
        impulse.y = impulse1.y;
        impulse.z = 0.0f;
    }

    b2Vec2 P = impulse.x * perp + impulse.z * axis;
    float LA = impulse.x * s1 + impulse.y + impulse.z * a1;
    float LB = impulse.x * s2 + impulse.y + impulse.z * a2;

    cA -= mA * P;
    aA -= iA * LA;
    cB += mB * P;
    aB += iB * LB;

    data.positions[m_indexA].c = cA;
    data.positions[m_indexA].a = aA;
    data.positions[m_indexB].c = cB;
    data.positions[m_indexB].a = aB;

    return linearError <= b2_linearSlop && angularError <= b2_angularSlop;
}

b2Vec2 b2PrismaticJoint::GetAnchorA() const { return m_bodyA->GetWorldPoint(m_localAnchorA); }

b2Vec2 b2PrismaticJoint::GetAnchorB() const { return m_bodyB->GetWorldPoint(m_localAnchorB); }

b2Vec2 b2PrismaticJoint::GetReactionForce(float inv_dt) const {
    return inv_dt *
           (m_impulse.x * m_perp + (m_motorImpulse + m_lowerImpulse - m_upperImpulse) * m_axis);
}

float b2PrismaticJoint::GetReactionTorque(float inv_dt) const { return inv_dt * m_impulse.y; }

float b2PrismaticJoint::GetJointTranslation() const {
    b2Vec2 pA = m_bodyA->GetWorldPoint(m_localAnchorA);
    b2Vec2 pB = m_bodyB->GetWorldPoint(m_localAnchorB);
    b2Vec2 d = pB - pA;
    b2Vec2 axis = m_bodyA->GetWorldVector(m_localXAxisA);

    float translation = b2Dot(d, axis);
    return translation;
}

float b2PrismaticJoint::GetJointSpeed() const {
    b2Body *bA = m_bodyA;
    b2Body *bB = m_bodyB;

    b2Vec2 rA = b2Mul(bA->m_xf.q, m_localAnchorA - bA->m_sweep.localCenter);
    b2Vec2 rB = b2Mul(bB->m_xf.q, m_localAnchorB - bB->m_sweep.localCenter);
    b2Vec2 p1 = bA->m_sweep.c + rA;
    b2Vec2 p2 = bB->m_sweep.c + rB;
    b2Vec2 d = p2 - p1;
    b2Vec2 axis = b2Mul(bA->m_xf.q, m_localXAxisA);

    b2Vec2 vA = bA->m_linearVelocity;
    b2Vec2 vB = bB->m_linearVelocity;
    float wA = bA->m_angularVelocity;
    float wB = bB->m_angularVelocity;

    float speed =
            b2Dot(d, b2Cross(wA, axis)) + b2Dot(axis, vB + b2Cross(wB, rB) - vA - b2Cross(wA, rA));
    return speed;
}

bool b2PrismaticJoint::IsLimitEnabled() const { return m_enableLimit; }

void b2PrismaticJoint::EnableLimit(bool flag) {
    if (flag != m_enableLimit) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_enableLimit = flag;
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }
}

float b2PrismaticJoint::GetLowerLimit() const { return m_lowerTranslation; }

float b2PrismaticJoint::GetUpperLimit() const { return m_upperTranslation; }

void b2PrismaticJoint::SetLimits(float lower, float upper) {
    METADOT_ASSERT_E(lower <= upper);
    if (lower != m_lowerTranslation || upper != m_upperTranslation) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_lowerTranslation = lower;
        m_upperTranslation = upper;
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }
}

bool b2PrismaticJoint::IsMotorEnabled() const { return m_enableMotor; }

void b2PrismaticJoint::EnableMotor(bool flag) {
    if (flag != m_enableMotor) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_enableMotor = flag;
    }
}

void b2PrismaticJoint::SetMotorSpeed(float speed) {
    if (speed != m_motorSpeed) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_motorSpeed = speed;
    }
}

void b2PrismaticJoint::SetMaxMotorForce(float force) {
    if (force != m_maxMotorForce) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_maxMotorForce = force;
    }
}

float b2PrismaticJoint::GetMotorForce(float inv_dt) const { return inv_dt * m_motorImpulse; }

void b2PrismaticJoint::Dump() {
    // FLT_DECIMAL_DIG == 9

    int32 indexA = m_bodyA->m_islandIndex;
    int32 indexB = m_bodyB->m_islandIndex;

    b2Dump("  b2PrismaticJointDef jd;\n");
    b2Dump("  jd.bodyA = bodies[%d];\n", indexA);
    b2Dump("  jd.bodyB = bodies[%d];\n", indexB);
    b2Dump("  jd.collideConnected = bool(%d);\n", m_collideConnected);
    b2Dump("  jd.localAnchorA.Set(%.9g, %.9g);\n", m_localAnchorA.x, m_localAnchorA.y);
    b2Dump("  jd.localAnchorB.Set(%.9g, %.9g);\n", m_localAnchorB.x, m_localAnchorB.y);
    b2Dump("  jd.localAxisA.Set(%.9g, %.9g);\n", m_localXAxisA.x, m_localXAxisA.y);
    b2Dump("  jd.referenceAngle = %.9g;\n", m_referenceAngle);
    b2Dump("  jd.enableLimit = bool(%d);\n", m_enableLimit);
    b2Dump("  jd.lowerTranslation = %.9g;\n", m_lowerTranslation);
    b2Dump("  jd.upperTranslation = %.9g;\n", m_upperTranslation);
    b2Dump("  jd.enableMotor = bool(%d);\n", m_enableMotor);
    b2Dump("  jd.motorSpeed = %.9g;\n", m_motorSpeed);
    b2Dump("  jd.maxMotorForce = %.9g;\n", m_maxMotorForce);
    b2Dump("  joints[%d] = m_world->CreateJoint(&jd);\n", m_index);
}

void b2PrismaticJoint::Draw(DebugDraw *draw) const {
    const b2Transform &xfA = m_bodyA->GetTransform();
    const b2Transform &xfB = m_bodyB->GetTransform();
    b2Vec2 pA = b2Mul(xfA, m_localAnchorA);
    b2Vec2 pB = b2Mul(xfB, m_localAnchorB);

    b2Vec2 axis = b2Mul(xfA.q, m_localXAxisA);

    b2Color c1(0.7f, 0.7f, 0.7f);
    b2Color c2(0.3f, 0.9f, 0.3f);
    b2Color c3(0.9f, 0.3f, 0.3f);
    b2Color c4(0.3f, 0.3f, 0.9f);
    b2Color c5(0.4f, 0.4f, 0.4f);

    draw->DrawSegment(pA, pB, c5);

    if (m_enableLimit) {
        b2Vec2 lower = pA + m_lowerTranslation * axis;
        b2Vec2 upper = pA + m_upperTranslation * axis;
        b2Vec2 perp = b2Mul(xfA.q, m_localYAxisA);
        draw->DrawSegment(lower, upper, c1);
        draw->DrawSegment(lower - 0.5f * perp, lower + 0.5f * perp, c2);
        draw->DrawSegment(upper - 0.5f * perp, upper + 0.5f * perp, c3);
    } else {
        draw->DrawSegment(pA - 1.0f * axis, pA + 1.0f * axis, c1);
    }

    draw->DrawPoint(pA, 5.0f, c1);
    draw->DrawPoint(pB, 5.0f, c4);
}

b2Contact *b2PolygonContact::Create(b2Fixture *fixtureA, int32, b2Fixture *fixtureB, int32,
                                    b2BlockAllocator *allocator) {
    void *mem = allocator->Allocate(sizeof(b2PolygonContact));
    return new (mem) b2PolygonContact(fixtureA, fixtureB);
}

void b2PolygonContact::Destroy(b2Contact *contact, b2BlockAllocator *allocator) {
    ((b2PolygonContact *) contact)->~b2PolygonContact();
    allocator->Free(contact, sizeof(b2PolygonContact));
}

b2PolygonContact::b2PolygonContact(b2Fixture *fixtureA, b2Fixture *fixtureB)
    : b2Contact(fixtureA, 0, fixtureB, 0) {
    METADOT_ASSERT_E(m_fixtureA->GetType() == b2Shape::e_polygon);
    METADOT_ASSERT_E(m_fixtureB->GetType() == b2Shape::e_polygon);
}

void b2PolygonContact::Evaluate(b2Manifold *manifold, const b2Transform &xfA,
                                const b2Transform &xfB) {
    b2CollidePolygons(manifold, (b2PolygonShape *) m_fixtureA->GetShape(), xfA,
                      (b2PolygonShape *) m_fixtureB->GetShape(), xfB);
}

b2Contact *b2PolygonAndCircleContact::Create(b2Fixture *fixtureA, int32, b2Fixture *fixtureB, int32,
                                             b2BlockAllocator *allocator) {
    void *mem = allocator->Allocate(sizeof(b2PolygonAndCircleContact));
    return new (mem) b2PolygonAndCircleContact(fixtureA, fixtureB);
}

void b2PolygonAndCircleContact::Destroy(b2Contact *contact, b2BlockAllocator *allocator) {
    ((b2PolygonAndCircleContact *) contact)->~b2PolygonAndCircleContact();
    allocator->Free(contact, sizeof(b2PolygonAndCircleContact));
}

b2PolygonAndCircleContact::b2PolygonAndCircleContact(b2Fixture *fixtureA, b2Fixture *fixtureB)
    : b2Contact(fixtureA, 0, fixtureB, 0) {
    METADOT_ASSERT_E(m_fixtureA->GetType() == b2Shape::e_polygon);
    METADOT_ASSERT_E(m_fixtureB->GetType() == b2Shape::e_circle);
}

void b2PolygonAndCircleContact::Evaluate(b2Manifold *manifold, const b2Transform &xfA,
                                         const b2Transform &xfB) {
    b2CollidePolygonAndCircle(manifold, (b2PolygonShape *) m_fixtureA->GetShape(), xfA,
                              (b2CircleShape *) m_fixtureB->GetShape(), xfB);
}

// p = attached point, m = mouse point
// C = p - m
// Cdot = v
//      = v + cross(w, r)
// J = [I r_skew]
// Identity used:
// w k % (rx i + ry j) = w * (-ry i + rx j)

b2MouseJoint::b2MouseJoint(const b2MouseJointDef *def) : b2Joint(def) {
    m_targetA = def->target;
    m_localAnchorB = b2MulT(m_bodyB->GetTransform(), m_targetA);
    m_maxForce = def->maxForce;
    m_stiffness = def->stiffness;
    m_damping = def->damping;

    m_impulse.SetZero();
    m_beta = 0.0f;
    m_gamma = 0.0f;
}

void b2MouseJoint::SetTarget(const b2Vec2 &target) {
    if (target != m_targetA) {
        m_bodyB->SetAwake(true);
        m_targetA = target;
    }
}

const b2Vec2 &b2MouseJoint::GetTarget() const { return m_targetA; }

void b2MouseJoint::SetMaxForce(float force) { m_maxForce = force; }

float b2MouseJoint::GetMaxForce() const { return m_maxForce; }

void b2MouseJoint::InitVelocityConstraints(const b2SolverData &data) {
    m_indexB = m_bodyB->m_islandIndex;
    m_localCenterB = m_bodyB->m_sweep.localCenter;
    m_invMassB = m_bodyB->m_invMass;
    m_invIB = m_bodyB->m_invI;

    b2Vec2 cB = data.positions[m_indexB].c;
    float aB = data.positions[m_indexB].a;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    b2Rot qB(aB);

    float d = m_damping;
    float k = m_stiffness;

    // magic formulas
    // gamma has units of inverse mass.
    // beta has units of inverse time.
    float h = data.step.dt;
    m_gamma = h * (d + h * k);
    if (m_gamma != 0.0f) { m_gamma = 1.0f / m_gamma; }
    m_beta = h * k * m_gamma;

    // Compute the effective mass matrix.
    m_rB = b2Mul(qB, m_localAnchorB - m_localCenterB);

    // K    = [(1/m1 + 1/m2) * eye(2) - skew(r1) * invI1 * skew(r1) - skew(r2) * invI2 * skew(r2)]
    //      = [1/m1+1/m2     0    ] + invI1 * [r1.y*r1.y -r1.x*r1.y] + invI2 * [r1.y*r1.y -r1.x*r1.y]
    //        [    0     1/m1+1/m2]           [-r1.x*r1.y r1.x*r1.x]           [-r1.x*r1.y r1.x*r1.x]
    b2Mat22 K;
    K.ex.x = m_invMassB + m_invIB * m_rB.y * m_rB.y + m_gamma;
    K.ex.y = -m_invIB * m_rB.x * m_rB.y;
    K.ey.x = K.ex.y;
    K.ey.y = m_invMassB + m_invIB * m_rB.x * m_rB.x + m_gamma;

    m_mass = K.GetInverse();

    m_C = cB + m_rB - m_targetA;
    m_C *= m_beta;

    // Cheat with some damping
    wB *= 0.98f;

    if (data.step.warmStarting) {
        m_impulse *= data.step.dtRatio;
        vB += m_invMassB * m_impulse;
        wB += m_invIB * b2Cross(m_rB, m_impulse);
    } else {
        m_impulse.SetZero();
    }

    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

void b2MouseJoint::SolveVelocityConstraints(const b2SolverData &data) {
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    // Cdot = v + cross(w, r)
    b2Vec2 Cdot = vB + b2Cross(wB, m_rB);
    b2Vec2 impulse = b2Mul(m_mass, -(Cdot + m_C + m_gamma * m_impulse));

    b2Vec2 oldImpulse = m_impulse;
    m_impulse += impulse;
    float maxImpulse = data.step.dt * m_maxForce;
    if (m_impulse.LengthSquared() > maxImpulse * maxImpulse) {
        m_impulse *= maxImpulse / m_impulse.Length();
    }
    impulse = m_impulse - oldImpulse;

    vB += m_invMassB * impulse;
    wB += m_invIB * b2Cross(m_rB, impulse);

    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

bool b2MouseJoint::SolvePositionConstraints(const b2SolverData &data) {
    B2_NOT_USED(data);
    return true;
}

b2Vec2 b2MouseJoint::GetAnchorA() const { return m_targetA; }

b2Vec2 b2MouseJoint::GetAnchorB() const { return m_bodyB->GetWorldPoint(m_localAnchorB); }

b2Vec2 b2MouseJoint::GetReactionForce(float inv_dt) const { return inv_dt * m_impulse; }

float b2MouseJoint::GetReactionTorque(float inv_dt) const { return inv_dt * 0.0f; }

void b2MouseJoint::ShiftOrigin(const b2Vec2 &newOrigin) { m_targetA -= newOrigin; }

// Point-to-point constraint
// Cdot = v2 - v1
//      = v2 + cross(w2, r2) - v1 - cross(w1, r1)
// J = [-I -r1_skew I r2_skew ]
// Identity used:
// w k % (rx i + ry j) = w * (-ry i + rx j)
//
// r1 = offset - c1
// r2 = -c2

// Angle constraint
// Cdot = w2 - w1
// J = [0 0 -1 0 0 1]
// K = invI1 + invI2

void b2MotorJointDef::Initialize(b2Body *bA, b2Body *bB) {
    bodyA = bA;
    bodyB = bB;
    b2Vec2 xB = bodyB->GetPosition();
    linearOffset = bodyA->GetLocalPoint(xB);

    float angleA = bodyA->GetAngle();
    float angleB = bodyB->GetAngle();
    angularOffset = angleB - angleA;
}

b2MotorJoint::b2MotorJoint(const b2MotorJointDef *def) : b2Joint(def) {
    m_linearOffset = def->linearOffset;
    m_angularOffset = def->angularOffset;

    m_linearImpulse.SetZero();
    m_angularImpulse = 0.0f;

    m_maxForce = def->maxForce;
    m_maxTorque = def->maxTorque;
    m_correctionFactor = def->correctionFactor;
}

void b2MotorJoint::InitVelocityConstraints(const b2SolverData &data) {
    m_indexA = m_bodyA->m_islandIndex;
    m_indexB = m_bodyB->m_islandIndex;
    m_localCenterA = m_bodyA->m_sweep.localCenter;
    m_localCenterB = m_bodyB->m_sweep.localCenter;
    m_invMassA = m_bodyA->m_invMass;
    m_invMassB = m_bodyB->m_invMass;
    m_invIA = m_bodyA->m_invI;
    m_invIB = m_bodyB->m_invI;

    b2Vec2 cA = data.positions[m_indexA].c;
    float aA = data.positions[m_indexA].a;
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;

    b2Vec2 cB = data.positions[m_indexB].c;
    float aB = data.positions[m_indexB].a;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    b2Rot qA(aA), qB(aB);

    // Compute the effective mass matrix.
    m_rA = b2Mul(qA, m_linearOffset - m_localCenterA);
    m_rB = b2Mul(qB, -m_localCenterB);

    // J = [-I -r1_skew I r2_skew]
    // r_skew = [-ry; rx]

    // Matlab
    // K = [ mA+r1y^2*iA+mB+r2y^2*iB,  -r1y*iA*r1x-r2y*iB*r2x,          -r1y*iA-r2y*iB]
    //     [  -r1y*iA*r1x-r2y*iB*r2x, mA+r1x^2*iA+mB+r2x^2*iB,           r1x*iA+r2x*iB]
    //     [          -r1y*iA-r2y*iB,           r1x*iA+r2x*iB,                   iA+iB]

    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    // Upper 2 by 2 of K for point to point
    b2Mat22 K;
    K.ex.x = mA + mB + iA * m_rA.y * m_rA.y + iB * m_rB.y * m_rB.y;
    K.ex.y = -iA * m_rA.x * m_rA.y - iB * m_rB.x * m_rB.y;
    K.ey.x = K.ex.y;
    K.ey.y = mA + mB + iA * m_rA.x * m_rA.x + iB * m_rB.x * m_rB.x;

    m_linearMass = K.GetInverse();

    m_angularMass = iA + iB;
    if (m_angularMass > 0.0f) { m_angularMass = 1.0f / m_angularMass; }

    m_linearError = cB + m_rB - cA - m_rA;
    m_angularError = aB - aA - m_angularOffset;

    if (data.step.warmStarting) {
        // Scale impulses to support a variable time step.
        m_linearImpulse *= data.step.dtRatio;
        m_angularImpulse *= data.step.dtRatio;

        b2Vec2 P(m_linearImpulse.x, m_linearImpulse.y);
        vA -= mA * P;
        wA -= iA * (b2Cross(m_rA, P) + m_angularImpulse);
        vB += mB * P;
        wB += iB * (b2Cross(m_rB, P) + m_angularImpulse);
    } else {
        m_linearImpulse.SetZero();
        m_angularImpulse = 0.0f;
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

void b2MotorJoint::SolveVelocityConstraints(const b2SolverData &data) {
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    float h = data.step.dt;
    float inv_h = data.step.inv_dt;

    // Solve angular friction
    {
        float Cdot = wB - wA + inv_h * m_correctionFactor * m_angularError;
        float impulse = -m_angularMass * Cdot;

        float oldImpulse = m_angularImpulse;
        float maxImpulse = h * m_maxTorque;
        m_angularImpulse = b2Clamp(m_angularImpulse + impulse, -maxImpulse, maxImpulse);
        impulse = m_angularImpulse - oldImpulse;

        wA -= iA * impulse;
        wB += iB * impulse;
    }

    // Solve linear friction
    {
        b2Vec2 Cdot = vB + b2Cross(wB, m_rB) - vA - b2Cross(wA, m_rA) +
                      inv_h * m_correctionFactor * m_linearError;

        b2Vec2 impulse = -b2Mul(m_linearMass, Cdot);
        b2Vec2 oldImpulse = m_linearImpulse;
        m_linearImpulse += impulse;

        float maxImpulse = h * m_maxForce;

        if (m_linearImpulse.LengthSquared() > maxImpulse * maxImpulse) {
            m_linearImpulse.Normalize();
            m_linearImpulse *= maxImpulse;
        }

        impulse = m_linearImpulse - oldImpulse;

        vA -= mA * impulse;
        wA -= iA * b2Cross(m_rA, impulse);

        vB += mB * impulse;
        wB += iB * b2Cross(m_rB, impulse);
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

bool b2MotorJoint::SolvePositionConstraints(const b2SolverData &data) {
    B2_NOT_USED(data);

    return true;
}

b2Vec2 b2MotorJoint::GetAnchorA() const { return m_bodyA->GetPosition(); }

b2Vec2 b2MotorJoint::GetAnchorB() const { return m_bodyB->GetPosition(); }

b2Vec2 b2MotorJoint::GetReactionForce(float inv_dt) const { return inv_dt * m_linearImpulse; }

float b2MotorJoint::GetReactionTorque(float inv_dt) const { return inv_dt * m_angularImpulse; }

void b2MotorJoint::SetMaxForce(float force) {
    METADOT_ASSERT_E(b2IsValid(force) && force >= 0.0f);
    m_maxForce = force;
}

float b2MotorJoint::GetMaxForce() const { return m_maxForce; }

void b2MotorJoint::SetMaxTorque(float torque) {
    METADOT_ASSERT_E(b2IsValid(torque) && torque >= 0.0f);
    m_maxTorque = torque;
}

float b2MotorJoint::GetMaxTorque() const { return m_maxTorque; }

void b2MotorJoint::SetCorrectionFactor(float factor) {
    METADOT_ASSERT_E(b2IsValid(factor) && 0.0f <= factor && factor <= 1.0f);
    m_correctionFactor = factor;
}

float b2MotorJoint::GetCorrectionFactor() const { return m_correctionFactor; }

void b2MotorJoint::SetLinearOffset(const b2Vec2 &linearOffset) {
    if (linearOffset.x != m_linearOffset.x || linearOffset.y != m_linearOffset.y) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_linearOffset = linearOffset;
    }
}

const b2Vec2 &b2MotorJoint::GetLinearOffset() const { return m_linearOffset; }

void b2MotorJoint::SetAngularOffset(float angularOffset) {
    if (angularOffset != m_angularOffset) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_angularOffset = angularOffset;
    }
}

float b2MotorJoint::GetAngularOffset() const { return m_angularOffset; }

void b2MotorJoint::Dump() {
    int32 indexA = m_bodyA->m_islandIndex;
    int32 indexB = m_bodyB->m_islandIndex;

    b2Dump("  b2MotorJointDef jd;\n");
    b2Dump("  jd.bodyA = bodies[%d];\n", indexA);
    b2Dump("  jd.bodyB = bodies[%d];\n", indexB);
    b2Dump("  jd.collideConnected = bool(%d);\n", m_collideConnected);
    b2Dump("  jd.linearOffset.Set(%.9g, %.9g);\n", m_linearOffset.x, m_linearOffset.y);
    b2Dump("  jd.angularOffset = %.9g;\n", m_angularOffset);
    b2Dump("  jd.maxForce = %.9g;\n", m_maxForce);
    b2Dump("  jd.maxTorque = %.9g;\n", m_maxTorque);
    b2Dump("  jd.correctionFactor = %.9g;\n", m_correctionFactor);
    b2Dump("  joints[%d] = m_world->CreateJoint(&jd);\n", m_index);
}

void b2LinearStiffness(float &stiffness, float &damping, float frequencyHertz, float dampingRatio,
                       const b2Body *bodyA, const b2Body *bodyB) {
    float massA = bodyA->GetMass();
    float massB = bodyB->GetMass();
    float mass;
    if (massA > 0.0f && massB > 0.0f) {
        mass = massA * massB / (massA + massB);
    } else if (massA > 0.0f) {
        mass = massA;
    } else {
        mass = massB;
    }

    float omega = 2.0f * b2_pi * frequencyHertz;
    stiffness = mass * omega * omega;
    damping = 2.0f * mass * dampingRatio * omega;
}

void b2AngularStiffness(float &stiffness, float &damping, float frequencyHertz, float dampingRatio,
                        const b2Body *bodyA, const b2Body *bodyB) {
    float IA = bodyA->GetInertia();
    float IB = bodyB->GetInertia();
    float I;
    if (IA > 0.0f && IB > 0.0f) {
        I = IA * IB / (IA + IB);
    } else if (IA > 0.0f) {
        I = IA;
    } else {
        I = IB;
    }

    float omega = 2.0f * b2_pi * frequencyHertz;
    stiffness = I * omega * omega;
    damping = 2.0f * I * dampingRatio * omega;
}

b2Joint *b2Joint::Create(const b2JointDef *def, b2BlockAllocator *allocator) {
    b2Joint *joint = nullptr;

    switch (def->type) {
        case e_distanceJoint: {
            void *mem = allocator->Allocate(sizeof(b2DistanceJoint));
            joint = new (mem) b2DistanceJoint(static_cast<const b2DistanceJointDef *>(def));
        } break;

        case e_mouseJoint: {
            void *mem = allocator->Allocate(sizeof(b2MouseJoint));
            joint = new (mem) b2MouseJoint(static_cast<const b2MouseJointDef *>(def));
        } break;

        case e_prismaticJoint: {
            void *mem = allocator->Allocate(sizeof(b2PrismaticJoint));
            joint = new (mem) b2PrismaticJoint(static_cast<const b2PrismaticJointDef *>(def));
        } break;

        case e_revoluteJoint: {
            void *mem = allocator->Allocate(sizeof(b2RevoluteJoint));
            joint = new (mem) b2RevoluteJoint(static_cast<const b2RevoluteJointDef *>(def));
        } break;

        case e_pulleyJoint: {
            void *mem = allocator->Allocate(sizeof(b2PulleyJoint));
            joint = new (mem) b2PulleyJoint(static_cast<const b2PulleyJointDef *>(def));
        } break;

        case e_gearJoint: {
            void *mem = allocator->Allocate(sizeof(b2GearJoint));
            joint = new (mem) b2GearJoint(static_cast<const b2GearJointDef *>(def));
        } break;

        case e_wheelJoint: {
            void *mem = allocator->Allocate(sizeof(b2WheelJoint));
            joint = new (mem) b2WheelJoint(static_cast<const b2WheelJointDef *>(def));
        } break;

        case e_weldJoint: {
            void *mem = allocator->Allocate(sizeof(b2WeldJoint));
            joint = new (mem) b2WeldJoint(static_cast<const b2WeldJointDef *>(def));
        } break;

        case e_frictionJoint: {
            void *mem = allocator->Allocate(sizeof(b2FrictionJoint));
            joint = new (mem) b2FrictionJoint(static_cast<const b2FrictionJointDef *>(def));
        } break;

        case e_motorJoint: {
            void *mem = allocator->Allocate(sizeof(b2MotorJoint));
            joint = new (mem) b2MotorJoint(static_cast<const b2MotorJointDef *>(def));
        } break;

        default:
            METADOT_ASSERT_E(false);
            break;
    }

    return joint;
}

void b2Joint::Destroy(b2Joint *joint, b2BlockAllocator *allocator) {
    joint->~b2Joint();
    switch (joint->m_type) {
        case e_distanceJoint:
            allocator->Free(joint, sizeof(b2DistanceJoint));
            break;

        case e_mouseJoint:
            allocator->Free(joint, sizeof(b2MouseJoint));
            break;

        case e_prismaticJoint:
            allocator->Free(joint, sizeof(b2PrismaticJoint));
            break;

        case e_revoluteJoint:
            allocator->Free(joint, sizeof(b2RevoluteJoint));
            break;

        case e_pulleyJoint:
            allocator->Free(joint, sizeof(b2PulleyJoint));
            break;

        case e_gearJoint:
            allocator->Free(joint, sizeof(b2GearJoint));
            break;

        case e_wheelJoint:
            allocator->Free(joint, sizeof(b2WheelJoint));
            break;

        case e_weldJoint:
            allocator->Free(joint, sizeof(b2WeldJoint));
            break;

        case e_frictionJoint:
            allocator->Free(joint, sizeof(b2FrictionJoint));
            break;

        case e_motorJoint:
            allocator->Free(joint, sizeof(b2MotorJoint));
            break;

        default:
            METADOT_ASSERT_E(false);
            break;
    }
}

b2Joint::b2Joint(const b2JointDef *def) {
    METADOT_ASSERT_E(def->bodyA != def->bodyB);

    m_type = def->type;
    m_prev = nullptr;
    m_next = nullptr;
    m_bodyA = def->bodyA;
    m_bodyB = def->bodyB;
    m_index = 0;
    m_collideConnected = def->collideConnected;
    m_islandFlag = false;
    m_userData = def->userData;

    m_edgeA.joint = nullptr;
    m_edgeA.other = nullptr;
    m_edgeA.prev = nullptr;
    m_edgeA.next = nullptr;

    m_edgeB.joint = nullptr;
    m_edgeB.other = nullptr;
    m_edgeB.prev = nullptr;
    m_edgeB.next = nullptr;
}

bool b2Joint::IsEnabled() const { return m_bodyA->IsEnabled() && m_bodyB->IsEnabled(); }

void b2Joint::Draw(DebugDraw *draw) const {
    const b2Transform &xf1 = m_bodyA->GetTransform();
    const b2Transform &xf2 = m_bodyB->GetTransform();
    b2Vec2 x1 = xf1.p;
    b2Vec2 x2 = xf2.p;
    b2Vec2 p1 = GetAnchorA();
    b2Vec2 p2 = GetAnchorB();

    b2Color color(0.5f, 0.8f, 0.8f);

    switch (m_type) {
        case e_distanceJoint:
            draw->DrawSegment(p1, p2, color);
            break;

        case e_pulleyJoint: {
            b2PulleyJoint *pulley = (b2PulleyJoint *) this;
            b2Vec2 s1 = pulley->GetGroundAnchorA();
            b2Vec2 s2 = pulley->GetGroundAnchorB();
            draw->DrawSegment(s1, p1, color);
            draw->DrawSegment(s2, p2, color);
            draw->DrawSegment(s1, s2, color);
        } break;

        case e_mouseJoint: {
            b2Color c;
            c.Set(0.0f, 1.0f, 0.0f);
            draw->DrawPoint(p1, 4.0f, c);
            draw->DrawPoint(p2, 4.0f, c);

            c.Set(0.8f, 0.8f, 0.8f);
            draw->DrawSegment(p1, p2, c);

        } break;

        default:
            draw->DrawSegment(x1, p1, color);
            draw->DrawSegment(p1, p2, color);
            draw->DrawSegment(x2, p2, color);
    }
}

/*
Position Correction Notes
=========================
I tried the several algorithms for position correction of the 2D revolute joint.
I looked at these systems:
- simple pendulum (1m diameter sphere on massless 5m stick) with initial angular velocity of 100 rad/s.
- suspension bridge with 30 1m long planks of length 1m.
- multi-link chain with 30 1m long links.

Here are the algorithms:

Baumgarte - A fraction of the position error is added to the velocity error. There is no
separate position solver.

Pseudo Velocities - After the velocity solver and position integration,
the position error, Jacobian, and effective mass are recomputed. Then
the velocity constraints are solved with pseudo velocities and a fraction
of the position error is added to the pseudo velocity error. The pseudo
velocities are initialized to zero and there is no warm-starting. After
the position solver, the pseudo velocities are added to the positions.
This is also called the First Order World method or the Position LCP method.

Modified Nonlinear Gauss-Seidel (NGS) - Like Pseudo Velocities except the
position error is re-computed for each constraint and the positions are updated
after the constraint is solved. The radius vectors (aka Jacobians) are
re-computed too (otherwise the algorithm has horrible instability). The pseudo
velocity states are not needed because they are effectively zero at the beginning
of each iteration. Since we have the current position error, we allow the
iterations to terminate early if the error becomes smaller than b2_linearSlop.

Full NGS or just NGS - Like Modified NGS except the effective mass are re-computed
each time a constraint is solved.

Here are the results:
Baumgarte - this is the cheapest algorithm but it has some stability problems,
especially with the bridge. The chain links separate easily close to the root
and they jitter as they struggle to pull together. This is one of the most common
methods in the field. The big drawback is that the position correction artificially
affects the momentum, thus leading to instabilities and false bounce. I used a
bias factor of 0.2. A larger bias factor makes the bridge less stable, a smaller
factor makes joints and contacts more spongy.

Pseudo Velocities - the is more stable than the Baumgarte method. The bridge is
stable. However, joints still separate with large angular velocities. Drag the
simple pendulum in a circle quickly and the joint will separate. The chain separates
easily and does not recover. I used a bias factor of 0.2. A larger value lead to
the bridge collapsing when a heavy cube drops on it.

Modified NGS - this algorithm is better in some ways than Baumgarte and Pseudo
Velocities, but in other ways it is worse. The bridge and chain are much more
stable, but the simple pendulum goes unstable at high angular velocities.

Full NGS - stable in all tests. The joints display good stiffness. The bridge
still sags, but this is better than infinite forces.

Recommendations
Pseudo Velocities are not really worthwhile because the bridge and chain cannot
recover from joint separation. In other cases the benefit over Baumgarte is small.

Modified NGS is not a robust method for the revolute joint due to the violent
instability seen in the simple pendulum. Perhaps it is viable with other constraint
types, especially scalar constraints where the effective mass is a scalar.

This leaves Baumgarte and Full NGS. Baumgarte has small, but manageable instabilities
and is very fast. I don't think we can escape Baumgarte, especially in highly
demanding cases where high constraint fidelity is not needed.

Full NGS is robust and easy on the eyes. I recommend this as an option for
higher fidelity simulation and certainly for suspension bridges and long chains.
Full NGS might be a good choice for ragdolls, especially motorized ragdolls where
joint separation can be problematic. The number of NGS iterations can be reduced
for better performance without harming robustness much.

Each joint in a can be handled differently in the position solver. So I recommend
a system where the user can select the algorithm on a per joint basis. I would
probably default to the slower Full NGS and let the user select the faster
Baumgarte method in performance critical scenarios.
*/

/*
Cache Performance

The Box2D solvers are dominated by cache misses. Data structures are designed
to increase the number of cache hits. Much of misses are due to random access
to body data. The constraint structures are iterated over linearly, which leads
to few cache misses.

The bodies are not accessed during iteration. Instead read only data, such as
the mass values are stored with the constraints. The mutable data are the constraint
impulses and the bodies velocities/positions. The impulses are held inside the
constraint structures. The body velocities/positions are held in compact, temporary
arrays to increase the number of cache hits. Linear and angular velocity are
stored in a single array since multiple arrays lead to multiple misses.
*/

/*
2D Rotation

R = [cos(theta) -sin(theta)]
    [sin(theta) cos(theta) ]

thetaDot = omega

Let q1 = cos(theta), q2 = sin(theta).
R = [q1 -q2]
    [q2  q1]

q1Dot = -thetaDot * q2
q2Dot = thetaDot * q1

q1_new = q1_old - dt * w * q2
q2_new = q2_old + dt * w * q1
then normalize.

This might be faster than computing sin+cos.
However, we can compute sin+cos of the same angle fast.
*/

b2Island::b2Island(int32 bodyCapacity, int32 contactCapacity, int32 jointCapacity,
                   b2StackAllocator *allocator, b2ContactListener *listener) {
    m_bodyCapacity = bodyCapacity;
    m_contactCapacity = contactCapacity;
    m_jointCapacity = jointCapacity;
    m_bodyCount = 0;
    m_contactCount = 0;
    m_jointCount = 0;

    m_allocator = allocator;
    m_listener = listener;

    m_bodies = (b2Body **) m_allocator->Allocate(bodyCapacity * sizeof(b2Body *));
    m_contacts = (b2Contact **) m_allocator->Allocate(contactCapacity * sizeof(b2Contact *));
    m_joints = (b2Joint **) m_allocator->Allocate(jointCapacity * sizeof(b2Joint *));

    m_velocities = (b2Velocity *) m_allocator->Allocate(m_bodyCapacity * sizeof(b2Velocity));
    m_positions = (b2Position *) m_allocator->Allocate(m_bodyCapacity * sizeof(b2Position));
}

b2Island::~b2Island() {
    // Warning: the order should reverse the constructor order.
    m_allocator->Free(m_positions);
    m_allocator->Free(m_velocities);
    m_allocator->Free(m_joints);
    m_allocator->Free(m_contacts);
    m_allocator->Free(m_bodies);
}

void b2Island::Solve(b2Profile *profile, const b2TimeStep &step, const b2Vec2 &gravity,
                     bool allowSleep) {
    b2Timer timer;

    float h = step.dt;

    // Integrate velocities and apply damping. Initialize the body state.
    for (int32 i = 0; i < m_bodyCount; ++i) {
        b2Body *b = m_bodies[i];

        b2Vec2 c = b->m_sweep.c;
        float a = b->m_sweep.a;
        b2Vec2 v = b->m_linearVelocity;
        float w = b->m_angularVelocity;

        // Store positions for continuous collision.
        b->m_sweep.c0 = b->m_sweep.c;
        b->m_sweep.a0 = b->m_sweep.a;

        if (b->m_type == b2_dynamicBody) {
            // Integrate velocities.
            v += h * b->m_invMass * (b->m_gravityScale * b->m_mass * gravity + b->m_force);
            w += h * b->m_invI * b->m_torque;

            // Apply damping.
            // ODE: dv/dt + c * v = 0
            // Solution: v(t) = v0 * exp(-c * t)
            // Time step: v(t + dt) = v0 * exp(-c * (t + dt)) = v0 * exp(-c * t) * exp(-c * dt) = v * exp(-c * dt)
            // v2 = exp(-c * dt) * v1
            // Pade approximation:
            // v2 = v1 * 1 / (1 + c * dt)
            v *= 1.0f / (1.0f + h * b->m_linearDamping);
            w *= 1.0f / (1.0f + h * b->m_angularDamping);
        }

        m_positions[i].c = c;
        m_positions[i].a = a;
        m_velocities[i].v = v;
        m_velocities[i].w = w;
    }

    timer.Reset();

    // Solver data
    b2SolverData solverData;
    solverData.step = step;
    solverData.positions = m_positions;
    solverData.velocities = m_velocities;

    // Initialize velocity constraints.
    b2ContactSolverDef contactSolverDef;
    contactSolverDef.step = step;
    contactSolverDef.contacts = m_contacts;
    contactSolverDef.count = m_contactCount;
    contactSolverDef.positions = m_positions;
    contactSolverDef.velocities = m_velocities;
    contactSolverDef.allocator = m_allocator;

    b2ContactSolver contactSolver(&contactSolverDef);
    contactSolver.InitializeVelocityConstraints();

    if (step.warmStarting) { contactSolver.WarmStart(); }

    for (int32 i = 0; i < m_jointCount; ++i) { m_joints[i]->InitVelocityConstraints(solverData); }

    profile->solveInit = timer.GetMilliseconds();

    // Solve velocity constraints
    timer.Reset();
    for (int32 i = 0; i < step.velocityIterations; ++i) {
        for (int32 j = 0; j < m_jointCount; ++j) {
            m_joints[j]->SolveVelocityConstraints(solverData);
        }

        contactSolver.SolveVelocityConstraints();
    }

    // Store impulses for warm starting
    contactSolver.StoreImpulses();
    profile->solveVelocity = timer.GetMilliseconds();

    // Integrate positions
    for (int32 i = 0; i < m_bodyCount; ++i) {
        b2Vec2 c = m_positions[i].c;
        float a = m_positions[i].a;
        b2Vec2 v = m_velocities[i].v;
        float w = m_velocities[i].w;

        // Check for large velocities
        b2Vec2 translation = h * v;
        if (b2Dot(translation, translation) > b2_maxTranslationSquared) {
            float ratio = b2_maxTranslation / translation.Length();
            v *= ratio;
        }

        float rotation = h * w;
        if (rotation * rotation > b2_maxRotationSquared) {
            float ratio = b2_maxRotation / b2Abs(rotation);
            w *= ratio;
        }

        // Integrate
        c += h * v;
        a += h * w;

        m_positions[i].c = c;
        m_positions[i].a = a;
        m_velocities[i].v = v;
        m_velocities[i].w = w;
    }

    // Solve position constraints
    timer.Reset();
    bool positionSolved = false;
    for (int32 i = 0; i < step.positionIterations; ++i) {
        bool contactsOkay = contactSolver.SolvePositionConstraints();

        bool jointsOkay = true;
        for (int32 j = 0; j < m_jointCount; ++j) {
            bool jointOkay = m_joints[j]->SolvePositionConstraints(solverData);
            jointsOkay = jointsOkay && jointOkay;
        }

        if (contactsOkay && jointsOkay) {
            // Exit early if the position errors are small.
            positionSolved = true;
            break;
        }
    }

    // Copy state buffers back to the bodies
    for (int32 i = 0; i < m_bodyCount; ++i) {
        b2Body *body = m_bodies[i];
        body->m_sweep.c = m_positions[i].c;
        body->m_sweep.a = m_positions[i].a;
        body->m_linearVelocity = m_velocities[i].v;
        body->m_angularVelocity = m_velocities[i].w;
        body->SynchronizeTransform();
    }

    profile->solvePosition = timer.GetMilliseconds();

    Report(contactSolver.m_velocityConstraints);

    if (allowSleep) {
        float minSleepTime = b2_maxFloat;

        const float linTolSqr = b2_linearSleepTolerance * b2_linearSleepTolerance;
        const float angTolSqr = b2_angularSleepTolerance * b2_angularSleepTolerance;

        for (int32 i = 0; i < m_bodyCount; ++i) {
            b2Body *b = m_bodies[i];
            if (b->GetType() == b2_staticBody) { continue; }

            if ((b->m_flags & b2Body::e_autoSleepFlag) == 0 ||
                b->m_angularVelocity * b->m_angularVelocity > angTolSqr ||
                b2Dot(b->m_linearVelocity, b->m_linearVelocity) > linTolSqr) {
                b->m_sleepTime = 0.0f;
                minSleepTime = 0.0f;
            } else {
                b->m_sleepTime += h;
                minSleepTime = b2Min(minSleepTime, b->m_sleepTime);
            }
        }

        if (minSleepTime >= b2_timeToSleep && positionSolved) {
            for (int32 i = 0; i < m_bodyCount; ++i) {
                b2Body *b = m_bodies[i];
                b->SetAwake(false);
            }
        }
    }
}

void b2Island::SolveTOI(const b2TimeStep &subStep, int32 toiIndexA, int32 toiIndexB) {
    METADOT_ASSERT_E(toiIndexA < m_bodyCount);
    METADOT_ASSERT_E(toiIndexB < m_bodyCount);

    // Initialize the body state.
    for (int32 i = 0; i < m_bodyCount; ++i) {
        b2Body *b = m_bodies[i];
        m_positions[i].c = b->m_sweep.c;
        m_positions[i].a = b->m_sweep.a;
        m_velocities[i].v = b->m_linearVelocity;
        m_velocities[i].w = b->m_angularVelocity;
    }

    b2ContactSolverDef contactSolverDef;
    contactSolverDef.contacts = m_contacts;
    contactSolverDef.count = m_contactCount;
    contactSolverDef.allocator = m_allocator;
    contactSolverDef.step = subStep;
    contactSolverDef.positions = m_positions;
    contactSolverDef.velocities = m_velocities;
    b2ContactSolver contactSolver(&contactSolverDef);

    // Solve position constraints.
    for (int32 i = 0; i < subStep.positionIterations; ++i) {
        bool contactsOkay = contactSolver.SolveTOIPositionConstraints(toiIndexA, toiIndexB);
        if (contactsOkay) { break; }
    }

#if 0
	// Is the new position really safe?
	for (int32 i = 0; i < m_contactCount; ++i)
	{
		b2Contact* c = m_contacts[i];
		b2Fixture* fA = c->GetFixtureA();
		b2Fixture* fB = c->GetFixtureB();

		b2Body* bA = fA->GetBody();
		b2Body* bB = fB->GetBody();

		int32 indexA = c->GetChildIndexA();
		int32 indexB = c->GetChildIndexB();

		b2DistanceInput input;
		input.proxyA.Set(fA->GetShape(), indexA);
		input.proxyB.Set(fB->GetShape(), indexB);
		input.transformA = bA->GetTransform();
		input.transformB = bB->GetTransform();
		input.useRadii = false;

		b2DistanceOutput output;
		b2SimplexCache cache;
		cache.count = 0;
		b2Distance(&output, &cache, &input);

		if (output.distance == 0 || cache.count == 3)
		{
			cache.count += 0;
		}
	}
#endif

    // Leap of faith to new safe state.
    m_bodies[toiIndexA]->m_sweep.c0 = m_positions[toiIndexA].c;
    m_bodies[toiIndexA]->m_sweep.a0 = m_positions[toiIndexA].a;
    m_bodies[toiIndexB]->m_sweep.c0 = m_positions[toiIndexB].c;
    m_bodies[toiIndexB]->m_sweep.a0 = m_positions[toiIndexB].a;

    // No warm starting is needed for TOI events because warm
    // starting impulses were applied in the discrete solver.
    contactSolver.InitializeVelocityConstraints();

    // Solve velocity constraints.
    for (int32 i = 0; i < subStep.velocityIterations; ++i) {
        contactSolver.SolveVelocityConstraints();
    }

    // Don't store the TOI contact forces for warm starting
    // because they can be quite large.

    float h = subStep.dt;

    // Integrate positions
    for (int32 i = 0; i < m_bodyCount; ++i) {
        b2Vec2 c = m_positions[i].c;
        float a = m_positions[i].a;
        b2Vec2 v = m_velocities[i].v;
        float w = m_velocities[i].w;

        // Check for large velocities
        b2Vec2 translation = h * v;
        if (b2Dot(translation, translation) > b2_maxTranslationSquared) {
            float ratio = b2_maxTranslation / translation.Length();
            v *= ratio;
        }

        float rotation = h * w;
        if (rotation * rotation > b2_maxRotationSquared) {
            float ratio = b2_maxRotation / b2Abs(rotation);
            w *= ratio;
        }

        // Integrate
        c += h * v;
        a += h * w;

        m_positions[i].c = c;
        m_positions[i].a = a;
        m_velocities[i].v = v;
        m_velocities[i].w = w;

        // Sync bodies
        b2Body *body = m_bodies[i];
        body->m_sweep.c = c;
        body->m_sweep.a = a;
        body->m_linearVelocity = v;
        body->m_angularVelocity = w;
        body->SynchronizeTransform();
    }

    Report(contactSolver.m_velocityConstraints);
}

void b2Island::Report(const b2ContactVelocityConstraint *constraints) {
    if (m_listener == nullptr) { return; }

    for (int32 i = 0; i < m_contactCount; ++i) {
        b2Contact *c = m_contacts[i];

        const b2ContactVelocityConstraint *vc = constraints + i;

        b2ContactImpulse impulse;
        impulse.count = vc->pointCount;
        for (int32 j = 0; j < vc->pointCount; ++j) {
            impulse.normalImpulses[j] = vc->points[j].normalImpulse;
            impulse.tangentImpulses[j] = vc->points[j].tangentImpulse;
        }

        m_listener->PostSolve(c, &impulse);
    }
}

// Gear Joint:
// C0 = (coordinate1 + ratio * coordinate2)_initial
// C = (coordinate1 + ratio * coordinate2) - C0 = 0
// J = [J1 ratio * J2]
// K = J * invM * JT
//   = J1 * invM1 * J1T + ratio * ratio * J2 * invM2 * J2T
//
// Revolute:
// coordinate = rotation
// Cdot = angularVelocity
// J = [0 0 1]
// K = J * invM * JT = invI
//
// Prismatic:
// coordinate = dot(p - pg, ug)
// Cdot = dot(v + cross(w, r), ug)
// J = [ug cross(r, ug)]
// K = J * invM * JT = invMass + invI * cross(r, ug)^2

b2GearJoint::b2GearJoint(const b2GearJointDef *def) : b2Joint(def) {
    m_joint1 = def->joint1;
    m_joint2 = def->joint2;

    m_typeA = m_joint1->GetType();
    m_typeB = m_joint2->GetType();

    METADOT_ASSERT_E(m_typeA == e_revoluteJoint || m_typeA == e_prismaticJoint);
    METADOT_ASSERT_E(m_typeB == e_revoluteJoint || m_typeB == e_prismaticJoint);

    float coordinateA, coordinateB;

    // TODO_ERIN there might be some problem with the joint edges in b2Joint.

    m_bodyC = m_joint1->GetBodyA();
    m_bodyA = m_joint1->GetBodyB();

    // Body B on joint1 must be dynamic
    METADOT_ASSERT_E(m_bodyA->m_type == b2_dynamicBody);

    // Get geometry of joint1
    b2Transform xfA = m_bodyA->m_xf;
    float aA = m_bodyA->m_sweep.a;
    b2Transform xfC = m_bodyC->m_xf;
    float aC = m_bodyC->m_sweep.a;

    if (m_typeA == e_revoluteJoint) {
        b2RevoluteJoint *revolute = (b2RevoluteJoint *) def->joint1;
        m_localAnchorC = revolute->m_localAnchorA;
        m_localAnchorA = revolute->m_localAnchorB;
        m_referenceAngleA = revolute->m_referenceAngle;
        m_localAxisC.SetZero();

        coordinateA = aA - aC - m_referenceAngleA;

        // position error is measured in radians
        m_tolerance = b2_angularSlop;
    } else {
        b2PrismaticJoint *prismatic = (b2PrismaticJoint *) def->joint1;
        m_localAnchorC = prismatic->m_localAnchorA;
        m_localAnchorA = prismatic->m_localAnchorB;
        m_referenceAngleA = prismatic->m_referenceAngle;
        m_localAxisC = prismatic->m_localXAxisA;

        b2Vec2 pC = m_localAnchorC;
        b2Vec2 pA = b2MulT(xfC.q, b2Mul(xfA.q, m_localAnchorA) + (xfA.p - xfC.p));
        coordinateA = b2Dot(pA - pC, m_localAxisC);

        // position error is measured in meters
        m_tolerance = b2_linearSlop;
    }

    m_bodyD = m_joint2->GetBodyA();
    m_bodyB = m_joint2->GetBodyB();

    // Body B on joint2 must be dynamic
    METADOT_ASSERT_E(m_bodyB->m_type == b2_dynamicBody);

    // Get geometry of joint2
    b2Transform xfB = m_bodyB->m_xf;
    float aB = m_bodyB->m_sweep.a;
    b2Transform xfD = m_bodyD->m_xf;
    float aD = m_bodyD->m_sweep.a;

    if (m_typeB == e_revoluteJoint) {
        b2RevoluteJoint *revolute = (b2RevoluteJoint *) def->joint2;
        m_localAnchorD = revolute->m_localAnchorA;
        m_localAnchorB = revolute->m_localAnchorB;
        m_referenceAngleB = revolute->m_referenceAngle;
        m_localAxisD.SetZero();

        coordinateB = aB - aD - m_referenceAngleB;
    } else {
        b2PrismaticJoint *prismatic = (b2PrismaticJoint *) def->joint2;
        m_localAnchorD = prismatic->m_localAnchorA;
        m_localAnchorB = prismatic->m_localAnchorB;
        m_referenceAngleB = prismatic->m_referenceAngle;
        m_localAxisD = prismatic->m_localXAxisA;

        b2Vec2 pD = m_localAnchorD;
        b2Vec2 pB = b2MulT(xfD.q, b2Mul(xfB.q, m_localAnchorB) + (xfB.p - xfD.p));
        coordinateB = b2Dot(pB - pD, m_localAxisD);
    }

    m_ratio = def->ratio;

    m_constant = coordinateA + m_ratio * coordinateB;

    m_impulse = 0.0f;
}

void b2GearJoint::InitVelocityConstraints(const b2SolverData &data) {
    m_indexA = m_bodyA->m_islandIndex;
    m_indexB = m_bodyB->m_islandIndex;
    m_indexC = m_bodyC->m_islandIndex;
    m_indexD = m_bodyD->m_islandIndex;
    m_lcA = m_bodyA->m_sweep.localCenter;
    m_lcB = m_bodyB->m_sweep.localCenter;
    m_lcC = m_bodyC->m_sweep.localCenter;
    m_lcD = m_bodyD->m_sweep.localCenter;
    m_mA = m_bodyA->m_invMass;
    m_mB = m_bodyB->m_invMass;
    m_mC = m_bodyC->m_invMass;
    m_mD = m_bodyD->m_invMass;
    m_iA = m_bodyA->m_invI;
    m_iB = m_bodyB->m_invI;
    m_iC = m_bodyC->m_invI;
    m_iD = m_bodyD->m_invI;

    float aA = data.positions[m_indexA].a;
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;

    float aB = data.positions[m_indexB].a;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    float aC = data.positions[m_indexC].a;
    b2Vec2 vC = data.velocities[m_indexC].v;
    float wC = data.velocities[m_indexC].w;

    float aD = data.positions[m_indexD].a;
    b2Vec2 vD = data.velocities[m_indexD].v;
    float wD = data.velocities[m_indexD].w;

    b2Rot qA(aA), qB(aB), qC(aC), qD(aD);

    m_mass = 0.0f;

    if (m_typeA == e_revoluteJoint) {
        m_JvAC.SetZero();
        m_JwA = 1.0f;
        m_JwC = 1.0f;
        m_mass += m_iA + m_iC;
    } else {
        b2Vec2 u = b2Mul(qC, m_localAxisC);
        b2Vec2 rC = b2Mul(qC, m_localAnchorC - m_lcC);
        b2Vec2 rA = b2Mul(qA, m_localAnchorA - m_lcA);
        m_JvAC = u;
        m_JwC = b2Cross(rC, u);
        m_JwA = b2Cross(rA, u);
        m_mass += m_mC + m_mA + m_iC * m_JwC * m_JwC + m_iA * m_JwA * m_JwA;
    }

    if (m_typeB == e_revoluteJoint) {
        m_JvBD.SetZero();
        m_JwB = m_ratio;
        m_JwD = m_ratio;
        m_mass += m_ratio * m_ratio * (m_iB + m_iD);
    } else {
        b2Vec2 u = b2Mul(qD, m_localAxisD);
        b2Vec2 rD = b2Mul(qD, m_localAnchorD - m_lcD);
        b2Vec2 rB = b2Mul(qB, m_localAnchorB - m_lcB);
        m_JvBD = m_ratio * u;
        m_JwD = m_ratio * b2Cross(rD, u);
        m_JwB = m_ratio * b2Cross(rB, u);
        m_mass += m_ratio * m_ratio * (m_mD + m_mB) + m_iD * m_JwD * m_JwD + m_iB * m_JwB * m_JwB;
    }

    // Compute effective mass.
    m_mass = m_mass > 0.0f ? 1.0f / m_mass : 0.0f;

    if (data.step.warmStarting) {
        vA += (m_mA * m_impulse) * m_JvAC;
        wA += m_iA * m_impulse * m_JwA;
        vB += (m_mB * m_impulse) * m_JvBD;
        wB += m_iB * m_impulse * m_JwB;
        vC -= (m_mC * m_impulse) * m_JvAC;
        wC -= m_iC * m_impulse * m_JwC;
        vD -= (m_mD * m_impulse) * m_JvBD;
        wD -= m_iD * m_impulse * m_JwD;
    } else {
        m_impulse = 0.0f;
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
    data.velocities[m_indexC].v = vC;
    data.velocities[m_indexC].w = wC;
    data.velocities[m_indexD].v = vD;
    data.velocities[m_indexD].w = wD;
}

void b2GearJoint::SolveVelocityConstraints(const b2SolverData &data) {
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;
    b2Vec2 vC = data.velocities[m_indexC].v;
    float wC = data.velocities[m_indexC].w;
    b2Vec2 vD = data.velocities[m_indexD].v;
    float wD = data.velocities[m_indexD].w;

    float Cdot = b2Dot(m_JvAC, vA - vC) + b2Dot(m_JvBD, vB - vD);
    Cdot += (m_JwA * wA - m_JwC * wC) + (m_JwB * wB - m_JwD * wD);

    float impulse = -m_mass * Cdot;
    m_impulse += impulse;

    vA += (m_mA * impulse) * m_JvAC;
    wA += m_iA * impulse * m_JwA;
    vB += (m_mB * impulse) * m_JvBD;
    wB += m_iB * impulse * m_JwB;
    vC -= (m_mC * impulse) * m_JvAC;
    wC -= m_iC * impulse * m_JwC;
    vD -= (m_mD * impulse) * m_JvBD;
    wD -= m_iD * impulse * m_JwD;

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
    data.velocities[m_indexC].v = vC;
    data.velocities[m_indexC].w = wC;
    data.velocities[m_indexD].v = vD;
    data.velocities[m_indexD].w = wD;
}

bool b2GearJoint::SolvePositionConstraints(const b2SolverData &data) {
    b2Vec2 cA = data.positions[m_indexA].c;
    float aA = data.positions[m_indexA].a;
    b2Vec2 cB = data.positions[m_indexB].c;
    float aB = data.positions[m_indexB].a;
    b2Vec2 cC = data.positions[m_indexC].c;
    float aC = data.positions[m_indexC].a;
    b2Vec2 cD = data.positions[m_indexD].c;
    float aD = data.positions[m_indexD].a;

    b2Rot qA(aA), qB(aB), qC(aC), qD(aD);

    float coordinateA, coordinateB;

    b2Vec2 JvAC, JvBD;
    float JwA, JwB, JwC, JwD;
    float mass = 0.0f;

    if (m_typeA == e_revoluteJoint) {
        JvAC.SetZero();
        JwA = 1.0f;
        JwC = 1.0f;
        mass += m_iA + m_iC;

        coordinateA = aA - aC - m_referenceAngleA;
    } else {
        b2Vec2 u = b2Mul(qC, m_localAxisC);
        b2Vec2 rC = b2Mul(qC, m_localAnchorC - m_lcC);
        b2Vec2 rA = b2Mul(qA, m_localAnchorA - m_lcA);
        JvAC = u;
        JwC = b2Cross(rC, u);
        JwA = b2Cross(rA, u);
        mass += m_mC + m_mA + m_iC * JwC * JwC + m_iA * JwA * JwA;

        b2Vec2 pC = m_localAnchorC - m_lcC;
        b2Vec2 pA = b2MulT(qC, rA + (cA - cC));
        coordinateA = b2Dot(pA - pC, m_localAxisC);
    }

    if (m_typeB == e_revoluteJoint) {
        JvBD.SetZero();
        JwB = m_ratio;
        JwD = m_ratio;
        mass += m_ratio * m_ratio * (m_iB + m_iD);

        coordinateB = aB - aD - m_referenceAngleB;
    } else {
        b2Vec2 u = b2Mul(qD, m_localAxisD);
        b2Vec2 rD = b2Mul(qD, m_localAnchorD - m_lcD);
        b2Vec2 rB = b2Mul(qB, m_localAnchorB - m_lcB);
        JvBD = m_ratio * u;
        JwD = m_ratio * b2Cross(rD, u);
        JwB = m_ratio * b2Cross(rB, u);
        mass += m_ratio * m_ratio * (m_mD + m_mB) + m_iD * JwD * JwD + m_iB * JwB * JwB;

        b2Vec2 pD = m_localAnchorD - m_lcD;
        b2Vec2 pB = b2MulT(qD, rB + (cB - cD));
        coordinateB = b2Dot(pB - pD, m_localAxisD);
    }

    float C = (coordinateA + m_ratio * coordinateB) - m_constant;

    float impulse = 0.0f;
    if (mass > 0.0f) { impulse = -C / mass; }

    cA += m_mA * impulse * JvAC;
    aA += m_iA * impulse * JwA;
    cB += m_mB * impulse * JvBD;
    aB += m_iB * impulse * JwB;
    cC -= m_mC * impulse * JvAC;
    aC -= m_iC * impulse * JwC;
    cD -= m_mD * impulse * JvBD;
    aD -= m_iD * impulse * JwD;

    data.positions[m_indexA].c = cA;
    data.positions[m_indexA].a = aA;
    data.positions[m_indexB].c = cB;
    data.positions[m_indexB].a = aB;
    data.positions[m_indexC].c = cC;
    data.positions[m_indexC].a = aC;
    data.positions[m_indexD].c = cD;
    data.positions[m_indexD].a = aD;

    if (b2Abs(C) < m_tolerance) { return true; }

    return false;
}

b2Vec2 b2GearJoint::GetAnchorA() const { return m_bodyA->GetWorldPoint(m_localAnchorA); }

b2Vec2 b2GearJoint::GetAnchorB() const { return m_bodyB->GetWorldPoint(m_localAnchorB); }

b2Vec2 b2GearJoint::GetReactionForce(float inv_dt) const {
    b2Vec2 P = m_impulse * m_JvAC;
    return inv_dt * P;
}

float b2GearJoint::GetReactionTorque(float inv_dt) const {
    float L = m_impulse * m_JwA;
    return inv_dt * L;
}

void b2GearJoint::SetRatio(float ratio) {
    METADOT_ASSERT_E(b2IsValid(ratio));
    m_ratio = ratio;
}

float b2GearJoint::GetRatio() const { return m_ratio; }

void b2GearJoint::Dump() {
    int32 indexA = m_bodyA->m_islandIndex;
    int32 indexB = m_bodyB->m_islandIndex;

    int32 index1 = m_joint1->m_index;
    int32 index2 = m_joint2->m_index;

    b2Dump("  b2GearJointDef jd;\n");
    b2Dump("  jd.bodyA = bodies[%d];\n", indexA);
    b2Dump("  jd.bodyB = bodies[%d];\n", indexB);
    b2Dump("  jd.collideConnected = bool(%d);\n", m_collideConnected);
    b2Dump("  jd.joint1 = joints[%d];\n", index1);
    b2Dump("  jd.joint2 = joints[%d];\n", index2);
    b2Dump("  jd.ratio = %.9g;\n", m_ratio);
    b2Dump("  joints[%d] = m_world->CreateJoint(&jd);\n", m_index);
}

// Point-to-point constraint
// Cdot = v2 - v1
//      = v2 + cross(w2, r2) - v1 - cross(w1, r1)
// J = [-I -r1_skew I r2_skew ]
// Identity used:
// w k % (rx i + ry j) = w * (-ry i + rx j)

// Angle constraint
// Cdot = w2 - w1
// J = [0 0 -1 0 0 1]
// K = invI1 + invI2

void b2FrictionJointDef::Initialize(b2Body *bA, b2Body *bB, const b2Vec2 &anchor) {
    bodyA = bA;
    bodyB = bB;
    localAnchorA = bodyA->GetLocalPoint(anchor);
    localAnchorB = bodyB->GetLocalPoint(anchor);
}

b2FrictionJoint::b2FrictionJoint(const b2FrictionJointDef *def) : b2Joint(def) {
    m_localAnchorA = def->localAnchorA;
    m_localAnchorB = def->localAnchorB;

    m_linearImpulse.SetZero();
    m_angularImpulse = 0.0f;

    m_maxForce = def->maxForce;
    m_maxTorque = def->maxTorque;
}

void b2FrictionJoint::InitVelocityConstraints(const b2SolverData &data) {
    m_indexA = m_bodyA->m_islandIndex;
    m_indexB = m_bodyB->m_islandIndex;
    m_localCenterA = m_bodyA->m_sweep.localCenter;
    m_localCenterB = m_bodyB->m_sweep.localCenter;
    m_invMassA = m_bodyA->m_invMass;
    m_invMassB = m_bodyB->m_invMass;
    m_invIA = m_bodyA->m_invI;
    m_invIB = m_bodyB->m_invI;

    float aA = data.positions[m_indexA].a;
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;

    float aB = data.positions[m_indexB].a;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    b2Rot qA(aA), qB(aB);

    // Compute the effective mass matrix.
    m_rA = b2Mul(qA, m_localAnchorA - m_localCenterA);
    m_rB = b2Mul(qB, m_localAnchorB - m_localCenterB);

    // J = [-I -r1_skew I r2_skew]
    //     [ 0       -1 0       1]
    // r_skew = [-ry; rx]

    // Matlab
    // K = [ mA+r1y^2*iA+mB+r2y^2*iB,  -r1y*iA*r1x-r2y*iB*r2x,          -r1y*iA-r2y*iB]
    //     [  -r1y*iA*r1x-r2y*iB*r2x, mA+r1x^2*iA+mB+r2x^2*iB,           r1x*iA+r2x*iB]
    //     [          -r1y*iA-r2y*iB,           r1x*iA+r2x*iB,                   iA+iB]

    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    b2Mat22 K;
    K.ex.x = mA + mB + iA * m_rA.y * m_rA.y + iB * m_rB.y * m_rB.y;
    K.ex.y = -iA * m_rA.x * m_rA.y - iB * m_rB.x * m_rB.y;
    K.ey.x = K.ex.y;
    K.ey.y = mA + mB + iA * m_rA.x * m_rA.x + iB * m_rB.x * m_rB.x;

    m_linearMass = K.GetInverse();

    m_angularMass = iA + iB;
    if (m_angularMass > 0.0f) { m_angularMass = 1.0f / m_angularMass; }

    if (data.step.warmStarting) {
        // Scale impulses to support a variable time step.
        m_linearImpulse *= data.step.dtRatio;
        m_angularImpulse *= data.step.dtRatio;

        b2Vec2 P(m_linearImpulse.x, m_linearImpulse.y);
        vA -= mA * P;
        wA -= iA * (b2Cross(m_rA, P) + m_angularImpulse);
        vB += mB * P;
        wB += iB * (b2Cross(m_rB, P) + m_angularImpulse);
    } else {
        m_linearImpulse.SetZero();
        m_angularImpulse = 0.0f;
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

void b2FrictionJoint::SolveVelocityConstraints(const b2SolverData &data) {
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    float mA = m_invMassA, mB = m_invMassB;
    float iA = m_invIA, iB = m_invIB;

    float h = data.step.dt;

    // Solve angular friction
    {
        float Cdot = wB - wA;
        float impulse = -m_angularMass * Cdot;

        float oldImpulse = m_angularImpulse;
        float maxImpulse = h * m_maxTorque;
        m_angularImpulse = b2Clamp(m_angularImpulse + impulse, -maxImpulse, maxImpulse);
        impulse = m_angularImpulse - oldImpulse;

        wA -= iA * impulse;
        wB += iB * impulse;
    }

    // Solve linear friction
    {
        b2Vec2 Cdot = vB + b2Cross(wB, m_rB) - vA - b2Cross(wA, m_rA);

        b2Vec2 impulse = -b2Mul(m_linearMass, Cdot);
        b2Vec2 oldImpulse = m_linearImpulse;
        m_linearImpulse += impulse;

        float maxImpulse = h * m_maxForce;

        if (m_linearImpulse.LengthSquared() > maxImpulse * maxImpulse) {
            m_linearImpulse.Normalize();
            m_linearImpulse *= maxImpulse;
        }

        impulse = m_linearImpulse - oldImpulse;

        vA -= mA * impulse;
        wA -= iA * b2Cross(m_rA, impulse);

        vB += mB * impulse;
        wB += iB * b2Cross(m_rB, impulse);
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

bool b2FrictionJoint::SolvePositionConstraints(const b2SolverData &data) {
    B2_NOT_USED(data);

    return true;
}

b2Vec2 b2FrictionJoint::GetAnchorA() const { return m_bodyA->GetWorldPoint(m_localAnchorA); }

b2Vec2 b2FrictionJoint::GetAnchorB() const { return m_bodyB->GetWorldPoint(m_localAnchorB); }

b2Vec2 b2FrictionJoint::GetReactionForce(float inv_dt) const { return inv_dt * m_linearImpulse; }

float b2FrictionJoint::GetReactionTorque(float inv_dt) const { return inv_dt * m_angularImpulse; }

void b2FrictionJoint::SetMaxForce(float force) {
    METADOT_ASSERT_E(b2IsValid(force) && force >= 0.0f);
    m_maxForce = force;
}

float b2FrictionJoint::GetMaxForce() const { return m_maxForce; }

void b2FrictionJoint::SetMaxTorque(float torque) {
    METADOT_ASSERT_E(b2IsValid(torque) && torque >= 0.0f);
    m_maxTorque = torque;
}

float b2FrictionJoint::GetMaxTorque() const { return m_maxTorque; }

void b2FrictionJoint::Dump() {
    int32 indexA = m_bodyA->m_islandIndex;
    int32 indexB = m_bodyB->m_islandIndex;

    b2Dump("  b2FrictionJointDef jd;\n");
    b2Dump("  jd.bodyA = bodies[%d];\n", indexA);
    b2Dump("  jd.bodyB = bodies[%d];\n", indexB);
    b2Dump("  jd.collideConnected = bool(%d);\n", m_collideConnected);
    b2Dump("  jd.localAnchorA.Set(%.9g, %.9g);\n", m_localAnchorA.x, m_localAnchorA.y);
    b2Dump("  jd.localAnchorB.Set(%.9g, %.9g);\n", m_localAnchorB.x, m_localAnchorB.y);
    b2Dump("  jd.maxForce = %.9g;\n", m_maxForce);
    b2Dump("  jd.maxTorque = %.9g;\n", m_maxTorque);
    b2Dump("  joints[%d] = m_world->CreateJoint(&jd);\n", m_index);
}

b2Fixture::b2Fixture() {
    m_body = nullptr;
    m_next = nullptr;
    m_proxies = nullptr;
    m_proxyCount = 0;
    m_shape = nullptr;
    m_density = 0.0f;
}

void b2Fixture::Create(b2BlockAllocator *allocator, b2Body *body, const b2FixtureDef *def) {
    m_userData = def->userData;
    m_friction = def->friction;
    m_restitution = def->restitution;
    m_restitutionThreshold = def->restitutionThreshold;

    m_body = body;
    m_next = nullptr;

    m_filter = def->filter;

    m_isSensor = def->isSensor;

    m_shape = def->shape->Clone(allocator);

    // Reserve proxy space
    int32 childCount = m_shape->GetChildCount();
    m_proxies = (b2FixtureProxy *) allocator->Allocate(childCount * sizeof(b2FixtureProxy));
    for (int32 i = 0; i < childCount; ++i) {
        m_proxies[i].fixture = nullptr;
        m_proxies[i].proxyId = b2BroadPhase::e_nullProxy;
    }
    m_proxyCount = 0;

    m_density = def->density;
}

void b2Fixture::Destroy(b2BlockAllocator *allocator) {
    // The proxies must be destroyed before calling this.
    METADOT_ASSERT_E(m_proxyCount == 0);

    // Free the proxy array.
    int32 childCount = m_shape->GetChildCount();
    allocator->Free(m_proxies, childCount * sizeof(b2FixtureProxy));
    m_proxies = nullptr;

    // Free the child shape.
    switch (m_shape->m_type) {
        case b2Shape::e_circle: {
            b2CircleShape *s = (b2CircleShape *) m_shape;
            s->~b2CircleShape();
            allocator->Free(s, sizeof(b2CircleShape));
        } break;

        case b2Shape::e_edge: {
            b2EdgeShape *s = (b2EdgeShape *) m_shape;
            s->~b2EdgeShape();
            allocator->Free(s, sizeof(b2EdgeShape));
        } break;

        case b2Shape::e_polygon: {
            b2PolygonShape *s = (b2PolygonShape *) m_shape;
            s->~b2PolygonShape();
            allocator->Free(s, sizeof(b2PolygonShape));
        } break;

        case b2Shape::e_chain: {
            b2ChainShape *s = (b2ChainShape *) m_shape;
            s->~b2ChainShape();
            allocator->Free(s, sizeof(b2ChainShape));
        } break;

        default:
            METADOT_ASSERT_E(false);
            break;
    }

    m_shape = nullptr;
}

void b2Fixture::CreateProxies(b2BroadPhase *broadPhase, const b2Transform &xf) {
    METADOT_ASSERT_E(m_proxyCount == 0);

    // Create proxies in the broad-phase.
    m_proxyCount = m_shape->GetChildCount();

    for (int32 i = 0; i < m_proxyCount; ++i) {
        b2FixtureProxy *proxy = m_proxies + i;
        m_shape->ComputeAABB(&proxy->aabb, xf, i);
        proxy->proxyId = broadPhase->CreateProxy(proxy->aabb, proxy);
        proxy->fixture = this;
        proxy->childIndex = i;
    }
}

void b2Fixture::DestroyProxies(b2BroadPhase *broadPhase) {
    // Destroy proxies in the broad-phase.
    for (int32 i = 0; i < m_proxyCount; ++i) {
        b2FixtureProxy *proxy = m_proxies + i;
        broadPhase->DestroyProxy(proxy->proxyId);
        proxy->proxyId = b2BroadPhase::e_nullProxy;
    }

    m_proxyCount = 0;
}

void b2Fixture::Synchronize(b2BroadPhase *broadPhase, const b2Transform &transform1,
                            const b2Transform &transform2) {
    if (m_proxyCount == 0) { return; }

    for (int32 i = 0; i < m_proxyCount; ++i) {
        b2FixtureProxy *proxy = m_proxies + i;

        // Compute an AABB that covers the swept shape (may miss some rotation effect).
        b2AABB aabb1, aabb2;
        m_shape->ComputeAABB(&aabb1, transform1, proxy->childIndex);
        m_shape->ComputeAABB(&aabb2, transform2, proxy->childIndex);

        proxy->aabb.Combine(aabb1, aabb2);

        b2Vec2 displacement = aabb2.GetCenter() - aabb1.GetCenter();

        broadPhase->MoveProxy(proxy->proxyId, proxy->aabb, displacement);
    }
}

void b2Fixture::SetFilterData(const b2Filter &filter) {
    m_filter = filter;

    Refilter();
}

void b2Fixture::Refilter() {
    if (m_body == nullptr) { return; }

    // Flag associated contacts for filtering.
    b2ContactEdge *edge = m_body->GetContactList();
    while (edge) {
        b2Contact *contact = edge->contact;
        b2Fixture *fixtureA = contact->GetFixtureA();
        b2Fixture *fixtureB = contact->GetFixtureB();
        if (fixtureA == this || fixtureB == this) { contact->FlagForFiltering(); }

        edge = edge->next;
    }

    b2World *world = m_body->GetWorld();

    if (world == nullptr) { return; }

    // Touch each proxy so that new pairs may be created
    b2BroadPhase *broadPhase = &world->m_contactManager.m_broadPhase;
    for (int32 i = 0; i < m_proxyCount; ++i) { broadPhase->TouchProxy(m_proxies[i].proxyId); }
}

void b2Fixture::SetSensor(bool sensor) {
    if (sensor != m_isSensor) {
        m_body->SetAwake(true);
        m_isSensor = sensor;
    }
}

void b2Fixture::Dump(int32 bodyIndex) {
    b2Dump("    b2FixtureDef fd;\n");
    b2Dump("    fd.friction = %.9g;\n", m_friction);
    b2Dump("    fd.restitution = %.9g;\n", m_restitution);
    b2Dump("    fd.restitutionThreshold = %.9g;\n", m_restitutionThreshold);
    b2Dump("    fd.density = %.9g;\n", m_density);
    b2Dump("    fd.isSensor = bool(%d);\n", m_isSensor);
    b2Dump("    fd.filter.categoryBits = uint16(%d);\n", m_filter.categoryBits);
    b2Dump("    fd.filter.maskBits = uint16(%d);\n", m_filter.maskBits);
    b2Dump("    fd.filter.groupIndex = int16(%d);\n", m_filter.groupIndex);

    switch (m_shape->m_type) {
        case b2Shape::e_circle: {
            b2CircleShape *s = (b2CircleShape *) m_shape;
            b2Dump("    b2CircleShape shape;\n");
            b2Dump("    shape.m_radius = %.9g;\n", s->m_radius);
            b2Dump("    shape.m_p.Set(%.9g, %.9g);\n", s->m_p.x, s->m_p.y);
        } break;

        case b2Shape::e_edge: {
            b2EdgeShape *s = (b2EdgeShape *) m_shape;
            b2Dump("    b2EdgeShape shape;\n");
            b2Dump("    shape.m_radius = %.9g;\n", s->m_radius);
            b2Dump("    shape.m_vertex0.Set(%.9g, %.9g);\n", s->m_vertex0.x, s->m_vertex0.y);
            b2Dump("    shape.m_vertex1.Set(%.9g, %.9g);\n", s->m_vertex1.x, s->m_vertex1.y);
            b2Dump("    shape.m_vertex2.Set(%.9g, %.9g);\n", s->m_vertex2.x, s->m_vertex2.y);
            b2Dump("    shape.m_vertex3.Set(%.9g, %.9g);\n", s->m_vertex3.x, s->m_vertex3.y);
            b2Dump("    shape.m_oneSided = bool(%d);\n", s->m_oneSided);
        } break;

        case b2Shape::e_polygon: {
            b2PolygonShape *s = (b2PolygonShape *) m_shape;
            b2Dump("    b2PolygonShape shape;\n");
            b2Dump("    b2Vec2 vs[%d];\n", b2_maxPolygonVertices);
            for (int32 i = 0; i < s->m_count; ++i) {
                b2Dump("    vs[%d].Set(%.9g, %.9g);\n", i, s->m_vertices[i].x, s->m_vertices[i].y);
            }
            b2Dump("    shape.Set(vs, %d);\n", s->m_count);
        } break;

        case b2Shape::e_chain: {
            b2ChainShape *s = (b2ChainShape *) m_shape;
            b2Dump("    b2ChainShape shape;\n");
            b2Dump("    b2Vec2 vs[%d];\n", s->m_count);
            for (int32 i = 0; i < s->m_count; ++i) {
                b2Dump("    vs[%d].Set(%.9g, %.9g);\n", i, s->m_vertices[i].x, s->m_vertices[i].y);
            }
            b2Dump("    shape.CreateChain(vs, %d);\n", s->m_count);
            b2Dump("    shape.m_prevVertex.Set(%.9g, %.9g);\n", s->m_prevVertex.x,
                   s->m_prevVertex.y);
            b2Dump("    shape.m_nextVertex.Set(%.9g, %.9g);\n", s->m_nextVertex.x,
                   s->m_nextVertex.y);
        } break;

        default:
            return;
    }

    b2Dump("\n");
    b2Dump("    fd.shape = &shape;\n");
    b2Dump("\n");
    b2Dump("    bodies[%d]->CreateFixture(&fd);\n", bodyIndex);
}

b2Contact *b2EdgeAndPolygonContact::Create(b2Fixture *fixtureA, int32, b2Fixture *fixtureB, int32,
                                           b2BlockAllocator *allocator) {
    void *mem = allocator->Allocate(sizeof(b2EdgeAndPolygonContact));
    return new (mem) b2EdgeAndPolygonContact(fixtureA, fixtureB);
}

void b2EdgeAndPolygonContact::Destroy(b2Contact *contact, b2BlockAllocator *allocator) {
    ((b2EdgeAndPolygonContact *) contact)->~b2EdgeAndPolygonContact();
    allocator->Free(contact, sizeof(b2EdgeAndPolygonContact));
}

b2EdgeAndPolygonContact::b2EdgeAndPolygonContact(b2Fixture *fixtureA, b2Fixture *fixtureB)
    : b2Contact(fixtureA, 0, fixtureB, 0) {
    METADOT_ASSERT_E(m_fixtureA->GetType() == b2Shape::e_edge);
    METADOT_ASSERT_E(m_fixtureB->GetType() == b2Shape::e_polygon);
}

void b2EdgeAndPolygonContact::Evaluate(b2Manifold *manifold, const b2Transform &xfA,
                                       const b2Transform &xfB) {
    b2CollideEdgeAndPolygon(manifold, (b2EdgeShape *) m_fixtureA->GetShape(), xfA,
                            (b2PolygonShape *) m_fixtureB->GetShape(), xfB);
}

b2Contact *b2EdgeAndCircleContact::Create(b2Fixture *fixtureA, int32, b2Fixture *fixtureB, int32,
                                          b2BlockAllocator *allocator) {
    void *mem = allocator->Allocate(sizeof(b2EdgeAndCircleContact));
    return new (mem) b2EdgeAndCircleContact(fixtureA, fixtureB);
}

void b2EdgeAndCircleContact::Destroy(b2Contact *contact, b2BlockAllocator *allocator) {
    ((b2EdgeAndCircleContact *) contact)->~b2EdgeAndCircleContact();
    allocator->Free(contact, sizeof(b2EdgeAndCircleContact));
}

b2EdgeAndCircleContact::b2EdgeAndCircleContact(b2Fixture *fixtureA, b2Fixture *fixtureB)
    : b2Contact(fixtureA, 0, fixtureB, 0) {
    METADOT_ASSERT_E(m_fixtureA->GetType() == b2Shape::e_edge);
    METADOT_ASSERT_E(m_fixtureB->GetType() == b2Shape::e_circle);
}

void b2EdgeAndCircleContact::Evaluate(b2Manifold *manifold, const b2Transform &xfA,
                                      const b2Transform &xfB) {
    b2CollideEdgeAndCircle(manifold, (b2EdgeShape *) m_fixtureA->GetShape(), xfA,
                           (b2CircleShape *) m_fixtureB->GetShape(), xfB);
}

// 1-D constrained system
// m (v2 - v1) = lambda
// v2 + (beta/h) * x1 + gamma * lambda = 0, gamma has units of inverse mass.
// x2 = x1 + h * v2

// 1-D mass-damper-spring system
// m (v2 - v1) + h * d * v2 + h * k *

// C = norm(p2 - p1) - L
// u = (p2 - p1) / norm(p2 - p1)
// Cdot = dot(u, v2 + cross(w2, r2) - v1 - cross(w1, r1))
// J = [-u -cross(r1, u) u cross(r2, u)]
// K = J * invM * JT
//   = invMass1 + invI1 * cross(r1, u)^2 + invMass2 + invI2 * cross(r2, u)^2

void b2DistanceJointDef::Initialize(b2Body *b1, b2Body *b2, const b2Vec2 &anchor1,
                                    const b2Vec2 &anchor2) {
    bodyA = b1;
    bodyB = b2;
    localAnchorA = bodyA->GetLocalPoint(anchor1);
    localAnchorB = bodyB->GetLocalPoint(anchor2);
    b2Vec2 d = anchor2 - anchor1;
    length = b2Max(d.Length(), b2_linearSlop);
    minLength = length;
    maxLength = length;
}

b2DistanceJoint::b2DistanceJoint(const b2DistanceJointDef *def) : b2Joint(def) {
    m_localAnchorA = def->localAnchorA;
    m_localAnchorB = def->localAnchorB;
    m_length = b2Max(def->length, b2_linearSlop);
    m_minLength = b2Max(def->minLength, b2_linearSlop);
    m_maxLength = b2Max(def->maxLength, m_minLength);
    m_stiffness = def->stiffness;
    m_damping = def->damping;

    m_gamma = 0.0f;
    m_bias = 0.0f;
    m_impulse = 0.0f;
    m_lowerImpulse = 0.0f;
    m_upperImpulse = 0.0f;
    m_currentLength = 0.0f;
}

void b2DistanceJoint::InitVelocityConstraints(const b2SolverData &data) {
    m_indexA = m_bodyA->m_islandIndex;
    m_indexB = m_bodyB->m_islandIndex;
    m_localCenterA = m_bodyA->m_sweep.localCenter;
    m_localCenterB = m_bodyB->m_sweep.localCenter;
    m_invMassA = m_bodyA->m_invMass;
    m_invMassB = m_bodyB->m_invMass;
    m_invIA = m_bodyA->m_invI;
    m_invIB = m_bodyB->m_invI;

    b2Vec2 cA = data.positions[m_indexA].c;
    float aA = data.positions[m_indexA].a;
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;

    b2Vec2 cB = data.positions[m_indexB].c;
    float aB = data.positions[m_indexB].a;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    b2Rot qA(aA), qB(aB);

    m_rA = b2Mul(qA, m_localAnchorA - m_localCenterA);
    m_rB = b2Mul(qB, m_localAnchorB - m_localCenterB);
    m_u = cB + m_rB - cA - m_rA;

    // Handle singularity.
    m_currentLength = m_u.Length();
    if (m_currentLength > b2_linearSlop) {
        m_u *= 1.0f / m_currentLength;
    } else {
        m_u.Set(0.0f, 0.0f);
        m_mass = 0.0f;
        m_impulse = 0.0f;
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }

    float crAu = b2Cross(m_rA, m_u);
    float crBu = b2Cross(m_rB, m_u);
    float invMass = m_invMassA + m_invIA * crAu * crAu + m_invMassB + m_invIB * crBu * crBu;
    m_mass = invMass != 0.0f ? 1.0f / invMass : 0.0f;

    if (m_stiffness > 0.0f && m_minLength < m_maxLength) {
        // soft
        float C = m_currentLength - m_length;

        float d = m_damping;
        float k = m_stiffness;

        // magic formulas
        float h = data.step.dt;

        // gamma = 1 / (h * (d + h * k))
        // the extra factor of h in the denominator is since the lambda is an impulse, not a force
        m_gamma = h * (d + h * k);
        m_gamma = m_gamma != 0.0f ? 1.0f / m_gamma : 0.0f;
        m_bias = C * h * k * m_gamma;

        invMass += m_gamma;
        m_softMass = invMass != 0.0f ? 1.0f / invMass : 0.0f;
    } else {
        // rigid
        m_gamma = 0.0f;
        m_bias = 0.0f;
        m_softMass = m_mass;
    }

    if (data.step.warmStarting) {
        // Scale the impulse to support a variable time step.
        m_impulse *= data.step.dtRatio;
        m_lowerImpulse *= data.step.dtRatio;
        m_upperImpulse *= data.step.dtRatio;

        b2Vec2 P = (m_impulse + m_lowerImpulse - m_upperImpulse) * m_u;
        vA -= m_invMassA * P;
        wA -= m_invIA * b2Cross(m_rA, P);
        vB += m_invMassB * P;
        wB += m_invIB * b2Cross(m_rB, P);
    } else {
        m_impulse = 0.0f;
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

void b2DistanceJoint::SolveVelocityConstraints(const b2SolverData &data) {
    b2Vec2 vA = data.velocities[m_indexA].v;
    float wA = data.velocities[m_indexA].w;
    b2Vec2 vB = data.velocities[m_indexB].v;
    float wB = data.velocities[m_indexB].w;

    if (m_minLength < m_maxLength) {
        if (m_stiffness > 0.0f) {
            // Cdot = dot(u, v + cross(w, r))
            b2Vec2 vpA = vA + b2Cross(wA, m_rA);
            b2Vec2 vpB = vB + b2Cross(wB, m_rB);
            float Cdot = b2Dot(m_u, vpB - vpA);

            float impulse = -m_softMass * (Cdot + m_bias + m_gamma * m_impulse);
            m_impulse += impulse;

            b2Vec2 P = impulse * m_u;
            vA -= m_invMassA * P;
            wA -= m_invIA * b2Cross(m_rA, P);
            vB += m_invMassB * P;
            wB += m_invIB * b2Cross(m_rB, P);
        }

        // lower
        {
            float C = m_currentLength - m_minLength;
            float bias = b2Max(0.0f, C) * data.step.inv_dt;

            b2Vec2 vpA = vA + b2Cross(wA, m_rA);
            b2Vec2 vpB = vB + b2Cross(wB, m_rB);
            float Cdot = b2Dot(m_u, vpB - vpA);

            float impulse = -m_mass * (Cdot + bias);
            float oldImpulse = m_lowerImpulse;
            m_lowerImpulse = b2Max(0.0f, m_lowerImpulse + impulse);
            impulse = m_lowerImpulse - oldImpulse;
            b2Vec2 P = impulse * m_u;

            vA -= m_invMassA * P;
            wA -= m_invIA * b2Cross(m_rA, P);
            vB += m_invMassB * P;
            wB += m_invIB * b2Cross(m_rB, P);
        }

        // upper
        {
            float C = m_maxLength - m_currentLength;
            float bias = b2Max(0.0f, C) * data.step.inv_dt;

            b2Vec2 vpA = vA + b2Cross(wA, m_rA);
            b2Vec2 vpB = vB + b2Cross(wB, m_rB);
            float Cdot = b2Dot(m_u, vpA - vpB);

            float impulse = -m_mass * (Cdot + bias);
            float oldImpulse = m_upperImpulse;
            m_upperImpulse = b2Max(0.0f, m_upperImpulse + impulse);
            impulse = m_upperImpulse - oldImpulse;
            b2Vec2 P = -impulse * m_u;

            vA -= m_invMassA * P;
            wA -= m_invIA * b2Cross(m_rA, P);
            vB += m_invMassB * P;
            wB += m_invIB * b2Cross(m_rB, P);
        }
    } else {
        // Equal limits

        // Cdot = dot(u, v + cross(w, r))
        b2Vec2 vpA = vA + b2Cross(wA, m_rA);
        b2Vec2 vpB = vB + b2Cross(wB, m_rB);
        float Cdot = b2Dot(m_u, vpB - vpA);

        float impulse = -m_mass * Cdot;
        m_impulse += impulse;

        b2Vec2 P = impulse * m_u;
        vA -= m_invMassA * P;
        wA -= m_invIA * b2Cross(m_rA, P);
        vB += m_invMassB * P;
        wB += m_invIB * b2Cross(m_rB, P);
    }

    data.velocities[m_indexA].v = vA;
    data.velocities[m_indexA].w = wA;
    data.velocities[m_indexB].v = vB;
    data.velocities[m_indexB].w = wB;
}

bool b2DistanceJoint::SolvePositionConstraints(const b2SolverData &data) {
    b2Vec2 cA = data.positions[m_indexA].c;
    float aA = data.positions[m_indexA].a;
    b2Vec2 cB = data.positions[m_indexB].c;
    float aB = data.positions[m_indexB].a;

    b2Rot qA(aA), qB(aB);

    b2Vec2 rA = b2Mul(qA, m_localAnchorA - m_localCenterA);
    b2Vec2 rB = b2Mul(qB, m_localAnchorB - m_localCenterB);
    b2Vec2 u = cB + rB - cA - rA;

    float length = u.Normalize();
    float C;
    if (m_minLength == m_maxLength) {
        C = length - m_minLength;
    } else if (length < m_minLength) {
        C = length - m_minLength;
    } else if (m_maxLength < length) {
        C = length - m_maxLength;
    } else {
        return true;
    }

    float impulse = -m_mass * C;
    b2Vec2 P = impulse * u;

    cA -= m_invMassA * P;
    aA -= m_invIA * b2Cross(rA, P);
    cB += m_invMassB * P;
    aB += m_invIB * b2Cross(rB, P);

    data.positions[m_indexA].c = cA;
    data.positions[m_indexA].a = aA;
    data.positions[m_indexB].c = cB;
    data.positions[m_indexB].a = aB;

    return b2Abs(C) < b2_linearSlop;
}

b2Vec2 b2DistanceJoint::GetAnchorA() const { return m_bodyA->GetWorldPoint(m_localAnchorA); }

b2Vec2 b2DistanceJoint::GetAnchorB() const { return m_bodyB->GetWorldPoint(m_localAnchorB); }

b2Vec2 b2DistanceJoint::GetReactionForce(float inv_dt) const {
    b2Vec2 F = inv_dt * (m_impulse + m_lowerImpulse - m_upperImpulse) * m_u;
    return F;
}

float b2DistanceJoint::GetReactionTorque(float inv_dt) const {
    B2_NOT_USED(inv_dt);
    return 0.0f;
}

float b2DistanceJoint::SetLength(float length) {
    m_impulse = 0.0f;
    m_length = b2Max(b2_linearSlop, length);
    return m_length;
}

float b2DistanceJoint::SetMinLength(float minLength) {
    m_lowerImpulse = 0.0f;
    m_minLength = b2Clamp(minLength, b2_linearSlop, m_maxLength);
    return m_minLength;
}

float b2DistanceJoint::SetMaxLength(float maxLength) {
    m_upperImpulse = 0.0f;
    m_maxLength = b2Max(maxLength, m_minLength);
    return m_maxLength;
}

float b2DistanceJoint::GetCurrentLength() const {
    b2Vec2 pA = m_bodyA->GetWorldPoint(m_localAnchorA);
    b2Vec2 pB = m_bodyB->GetWorldPoint(m_localAnchorB);
    b2Vec2 d = pB - pA;
    float length = d.Length();
    return length;
}

void b2DistanceJoint::Dump() {
    int32 indexA = m_bodyA->m_islandIndex;
    int32 indexB = m_bodyB->m_islandIndex;

    b2Dump("  b2DistanceJointDef jd;\n");
    b2Dump("  jd.bodyA = bodies[%d];\n", indexA);
    b2Dump("  jd.bodyB = bodies[%d];\n", indexB);
    b2Dump("  jd.collideConnected = bool(%d);\n", m_collideConnected);
    b2Dump("  jd.localAnchorA.Set(%.9g, %.9g);\n", m_localAnchorA.x, m_localAnchorA.y);
    b2Dump("  jd.localAnchorB.Set(%.9g, %.9g);\n", m_localAnchorB.x, m_localAnchorB.y);
    b2Dump("  jd.length = %.9g;\n", m_length);
    b2Dump("  jd.minLength = %.9g;\n", m_minLength);
    b2Dump("  jd.maxLength = %.9g;\n", m_maxLength);
    b2Dump("  jd.stiffness = %.9g;\n", m_stiffness);
    b2Dump("  jd.damping = %.9g;\n", m_damping);
    b2Dump("  joints[%d] = m_world->CreateJoint(&jd);\n", m_index);
}

void b2DistanceJoint::Draw(DebugDraw *draw) const {
    const b2Transform &xfA = m_bodyA->GetTransform();
    const b2Transform &xfB = m_bodyB->GetTransform();
    b2Vec2 pA = b2Mul(xfA, m_localAnchorA);
    b2Vec2 pB = b2Mul(xfB, m_localAnchorB);

    b2Vec2 axis = pB - pA;
    axis.Normalize();

    b2Color c1(0.7f, 0.7f, 0.7f);
    b2Color c2(0.3f, 0.9f, 0.3f);
    b2Color c3(0.9f, 0.3f, 0.3f);
    b2Color c4(0.4f, 0.4f, 0.4f);

    draw->DrawSegment(pA, pB, c4);

    b2Vec2 pRest = pA + m_length * axis;
    draw->DrawPoint(pRest, 8.0f, c1);

    if (m_minLength != m_maxLength) {
        if (m_minLength > b2_linearSlop) {
            b2Vec2 pMin = pA + m_minLength * axis;
            draw->DrawPoint(pMin, 4.0f, c2);
        }

        if (m_maxLength < FLT_MAX) {
            b2Vec2 pMax = pA + m_maxLength * axis;
            draw->DrawPoint(pMax, 4.0f, c3);
        }
    }
}

b2ContactRegister b2Contact::s_registers[b2Shape::e_typeCount][b2Shape::e_typeCount];
bool b2Contact::s_initialized = false;

void b2Contact::InitializeRegisters() {
    AddType(b2CircleContact::Create, b2CircleContact::Destroy, b2Shape::e_circle,
            b2Shape::e_circle);
    AddType(b2PolygonAndCircleContact::Create, b2PolygonAndCircleContact::Destroy,
            b2Shape::e_polygon, b2Shape::e_circle);
    AddType(b2PolygonContact::Create, b2PolygonContact::Destroy, b2Shape::e_polygon,
            b2Shape::e_polygon);
    AddType(b2EdgeAndCircleContact::Create, b2EdgeAndCircleContact::Destroy, b2Shape::e_edge,
            b2Shape::e_circle);
    AddType(b2EdgeAndPolygonContact::Create, b2EdgeAndPolygonContact::Destroy, b2Shape::e_edge,
            b2Shape::e_polygon);
    AddType(b2ChainAndCircleContact::Create, b2ChainAndCircleContact::Destroy, b2Shape::e_chain,
            b2Shape::e_circle);
    AddType(b2ChainAndPolygonContact::Create, b2ChainAndPolygonContact::Destroy, b2Shape::e_chain,
            b2Shape::e_polygon);
}

void b2Contact::AddType(b2ContactCreateFcn *createFcn, b2ContactDestroyFcn *destoryFcn,
                        b2Shape::Type type1, b2Shape::Type type2) {
    METADOT_ASSERT_E(0 <= type1 && type1 < b2Shape::e_typeCount);
    METADOT_ASSERT_E(0 <= type2 && type2 < b2Shape::e_typeCount);

    s_registers[type1][type2].createFcn = createFcn;
    s_registers[type1][type2].destroyFcn = destoryFcn;
    s_registers[type1][type2].primary = true;

    if (type1 != type2) {
        s_registers[type2][type1].createFcn = createFcn;
        s_registers[type2][type1].destroyFcn = destoryFcn;
        s_registers[type2][type1].primary = false;
    }
}

b2Contact *b2Contact::Create(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB,
                             b2BlockAllocator *allocator) {
    if (s_initialized == false) {
        InitializeRegisters();
        s_initialized = true;
    }

    b2Shape::Type type1 = fixtureA->GetType();
    b2Shape::Type type2 = fixtureB->GetType();

    METADOT_ASSERT_E(0 <= type1 && type1 < b2Shape::e_typeCount);
    METADOT_ASSERT_E(0 <= type2 && type2 < b2Shape::e_typeCount);

    b2ContactCreateFcn *createFcn = s_registers[type1][type2].createFcn;
    if (createFcn) {
        if (s_registers[type1][type2].primary) {
            return createFcn(fixtureA, indexA, fixtureB, indexB, allocator);
        } else {
            return createFcn(fixtureB, indexB, fixtureA, indexA, allocator);
        }
    } else {
        return nullptr;
    }
}

void b2Contact::Destroy(b2Contact *contact, b2BlockAllocator *allocator) {
    METADOT_ASSERT_E(s_initialized == true);

    b2Fixture *fixtureA = contact->m_fixtureA;
    b2Fixture *fixtureB = contact->m_fixtureB;

    if (contact->m_manifold.pointCount > 0 && fixtureA->IsSensor() == false &&
        fixtureB->IsSensor() == false) {
        fixtureA->GetBody()->SetAwake(true);
        fixtureB->GetBody()->SetAwake(true);
    }

    b2Shape::Type typeA = fixtureA->GetType();
    b2Shape::Type typeB = fixtureB->GetType();

    METADOT_ASSERT_E(0 <= typeA && typeA < b2Shape::e_typeCount);
    METADOT_ASSERT_E(0 <= typeB && typeB < b2Shape::e_typeCount);

    b2ContactDestroyFcn *destroyFcn = s_registers[typeA][typeB].destroyFcn;
    destroyFcn(contact, allocator);
}

b2Contact::b2Contact(b2Fixture *fA, int32 indexA, b2Fixture *fB, int32 indexB) {
    m_flags = e_enabledFlag;

    m_fixtureA = fA;
    m_fixtureB = fB;

    m_indexA = indexA;
    m_indexB = indexB;

    m_manifold.pointCount = 0;

    m_prev = nullptr;
    m_next = nullptr;

    m_nodeA.contact = nullptr;
    m_nodeA.prev = nullptr;
    m_nodeA.next = nullptr;
    m_nodeA.other = nullptr;

    m_nodeB.contact = nullptr;
    m_nodeB.prev = nullptr;
    m_nodeB.next = nullptr;
    m_nodeB.other = nullptr;

    m_toiCount = 0;

    m_friction = b2MixFriction(m_fixtureA->m_friction, m_fixtureB->m_friction);
    m_restitution = b2MixRestitution(m_fixtureA->m_restitution, m_fixtureB->m_restitution);
    m_restitutionThreshold = b2MixRestitutionThreshold(m_fixtureA->m_restitutionThreshold,
                                                       m_fixtureB->m_restitutionThreshold);

    m_tangentSpeed = 0.0f;
}

// Update the contact manifold and touching status.
// Note: do not assume the fixture AABBs are overlapping or are valid.
void b2Contact::Update(b2ContactListener *listener) {
    b2Manifold oldManifold = m_manifold;

    // Re-enable this contact.
    m_flags |= e_enabledFlag;

    bool touching = false;
    bool wasTouching = (m_flags & e_touchingFlag) == e_touchingFlag;

    bool sensorA = m_fixtureA->IsSensor();
    bool sensorB = m_fixtureB->IsSensor();
    bool sensor = sensorA || sensorB;

    b2Body *bodyA = m_fixtureA->GetBody();
    b2Body *bodyB = m_fixtureB->GetBody();
    const b2Transform &xfA = bodyA->GetTransform();
    const b2Transform &xfB = bodyB->GetTransform();

    // Is this contact a sensor?
    if (sensor) {
        const b2Shape *shapeA = m_fixtureA->GetShape();
        const b2Shape *shapeB = m_fixtureB->GetShape();
        touching = b2TestOverlap(shapeA, m_indexA, shapeB, m_indexB, xfA, xfB);

        // Sensors don't generate manifolds.
        m_manifold.pointCount = 0;
    } else {
        Evaluate(&m_manifold, xfA, xfB);
        touching = m_manifold.pointCount > 0;

        // Match old contact ids to new contact ids and copy the
        // stored impulses to warm start the solver.
        for (int32 i = 0; i < m_manifold.pointCount; ++i) {
            b2ManifoldPoint *mp2 = m_manifold.points + i;
            mp2->normalImpulse = 0.0f;
            mp2->tangentImpulse = 0.0f;
            b2ContactID id2 = mp2->id;

            for (int32 j = 0; j < oldManifold.pointCount; ++j) {
                b2ManifoldPoint *mp1 = oldManifold.points + j;

                if (mp1->id.key == id2.key) {
                    mp2->normalImpulse = mp1->normalImpulse;
                    mp2->tangentImpulse = mp1->tangentImpulse;
                    break;
                }
            }
        }

        if (touching != wasTouching) {
            bodyA->SetAwake(true);
            bodyB->SetAwake(true);
        }
    }

    if (touching) {
        m_flags |= e_touchingFlag;
    } else {
        m_flags &= ~e_touchingFlag;
    }

    if (wasTouching == false && touching == true && listener) { listener->BeginContact(this); }

    if (wasTouching == true && touching == false && listener) { listener->EndContact(this); }

    if (sensor == false && touching && listener) { listener->PreSolve(this, &oldManifold); }
}

// Solver debugging is normally disabled because the block solver sometimes has to deal with a poorly conditioned effective mass matrix.
#define B2_DEBUG_SOLVER 0

bool g_blockSolve = true;

struct b2ContactPositionConstraint
{
    b2Vec2 localPoints[b2_maxManifoldPoints];
    b2Vec2 localNormal;
    b2Vec2 localPoint;
    int32 indexA;
    int32 indexB;
    float invMassA, invMassB;
    b2Vec2 localCenterA, localCenterB;
    float invIA, invIB;
    b2Manifold::Type type;
    float radiusA, radiusB;
    int32 pointCount;
};

b2ContactSolver::b2ContactSolver(b2ContactSolverDef *def) {
    m_step = def->step;
    m_allocator = def->allocator;
    m_count = def->count;
    m_positionConstraints = (b2ContactPositionConstraint *) m_allocator->Allocate(
            m_count * sizeof(b2ContactPositionConstraint));
    m_velocityConstraints = (b2ContactVelocityConstraint *) m_allocator->Allocate(
            m_count * sizeof(b2ContactVelocityConstraint));
    m_positions = def->positions;
    m_velocities = def->velocities;
    m_contacts = def->contacts;

    // Initialize position independent portions of the constraints.
    for (int32 i = 0; i < m_count; ++i) {
        b2Contact *contact = m_contacts[i];

        b2Fixture *fixtureA = contact->m_fixtureA;
        b2Fixture *fixtureB = contact->m_fixtureB;
        b2Shape *shapeA = fixtureA->GetShape();
        b2Shape *shapeB = fixtureB->GetShape();
        float radiusA = shapeA->m_radius;
        float radiusB = shapeB->m_radius;
        b2Body *bodyA = fixtureA->GetBody();
        b2Body *bodyB = fixtureB->GetBody();
        b2Manifold *manifold = contact->GetManifold();

        int32 pointCount = manifold->pointCount;
        METADOT_ASSERT_E(pointCount > 0);

        b2ContactVelocityConstraint *vc = m_velocityConstraints + i;
        vc->friction = contact->m_friction;
        vc->restitution = contact->m_restitution;
        vc->threshold = contact->m_restitutionThreshold;
        vc->tangentSpeed = contact->m_tangentSpeed;
        vc->indexA = bodyA->m_islandIndex;
        vc->indexB = bodyB->m_islandIndex;
        vc->invMassA = bodyA->m_invMass;
        vc->invMassB = bodyB->m_invMass;
        vc->invIA = bodyA->m_invI;
        vc->invIB = bodyB->m_invI;
        vc->contactIndex = i;
        vc->pointCount = pointCount;
        vc->K.SetZero();
        vc->normalMass.SetZero();

        b2ContactPositionConstraint *pc = m_positionConstraints + i;
        pc->indexA = bodyA->m_islandIndex;
        pc->indexB = bodyB->m_islandIndex;
        pc->invMassA = bodyA->m_invMass;
        pc->invMassB = bodyB->m_invMass;
        pc->localCenterA = bodyA->m_sweep.localCenter;
        pc->localCenterB = bodyB->m_sweep.localCenter;
        pc->invIA = bodyA->m_invI;
        pc->invIB = bodyB->m_invI;
        pc->localNormal = manifold->localNormal;
        pc->localPoint = manifold->localPoint;
        pc->pointCount = pointCount;
        pc->radiusA = radiusA;
        pc->radiusB = radiusB;
        pc->type = manifold->type;

        for (int32 j = 0; j < pointCount; ++j) {
            b2ManifoldPoint *cp = manifold->points + j;
            b2VelocityConstraintPoint *vcp = vc->points + j;

            if (m_step.warmStarting) {
                vcp->normalImpulse = m_step.dtRatio * cp->normalImpulse;
                vcp->tangentImpulse = m_step.dtRatio * cp->tangentImpulse;
            } else {
                vcp->normalImpulse = 0.0f;
                vcp->tangentImpulse = 0.0f;
            }

            vcp->rA.SetZero();
            vcp->rB.SetZero();
            vcp->normalMass = 0.0f;
            vcp->tangentMass = 0.0f;
            vcp->velocityBias = 0.0f;

            pc->localPoints[j] = cp->localPoint;
        }
    }
}

b2ContactSolver::~b2ContactSolver() {
    m_allocator->Free(m_velocityConstraints);
    m_allocator->Free(m_positionConstraints);
}

// Initialize position dependent portions of the velocity constraints.
void b2ContactSolver::InitializeVelocityConstraints() {
    for (int32 i = 0; i < m_count; ++i) {
        b2ContactVelocityConstraint *vc = m_velocityConstraints + i;
        b2ContactPositionConstraint *pc = m_positionConstraints + i;

        float radiusA = pc->radiusA;
        float radiusB = pc->radiusB;
        b2Manifold *manifold = m_contacts[vc->contactIndex]->GetManifold();

        int32 indexA = vc->indexA;
        int32 indexB = vc->indexB;

        float mA = vc->invMassA;
        float mB = vc->invMassB;
        float iA = vc->invIA;
        float iB = vc->invIB;
        b2Vec2 localCenterA = pc->localCenterA;
        b2Vec2 localCenterB = pc->localCenterB;

        b2Vec2 cA = m_positions[indexA].c;
        float aA = m_positions[indexA].a;
        b2Vec2 vA = m_velocities[indexA].v;
        float wA = m_velocities[indexA].w;

        b2Vec2 cB = m_positions[indexB].c;
        float aB = m_positions[indexB].a;
        b2Vec2 vB = m_velocities[indexB].v;
        float wB = m_velocities[indexB].w;

        METADOT_ASSERT_E(manifold->pointCount > 0);

        b2Transform xfA, xfB;
        xfA.q.Set(aA);
        xfB.q.Set(aB);
        xfA.p = cA - b2Mul(xfA.q, localCenterA);
        xfB.p = cB - b2Mul(xfB.q, localCenterB);

        b2WorldManifold worldManifold;
        worldManifold.Initialize(manifold, xfA, radiusA, xfB, radiusB);

        vc->normal = worldManifold.normal;

        int32 pointCount = vc->pointCount;
        for (int32 j = 0; j < pointCount; ++j) {
            b2VelocityConstraintPoint *vcp = vc->points + j;

            vcp->rA = worldManifold.points[j] - cA;
            vcp->rB = worldManifold.points[j] - cB;

            float rnA = b2Cross(vcp->rA, vc->normal);
            float rnB = b2Cross(vcp->rB, vc->normal);

            float kNormal = mA + mB + iA * rnA * rnA + iB * rnB * rnB;

            vcp->normalMass = kNormal > 0.0f ? 1.0f / kNormal : 0.0f;

            b2Vec2 tangent = b2Cross(vc->normal, 1.0f);

            float rtA = b2Cross(vcp->rA, tangent);
            float rtB = b2Cross(vcp->rB, tangent);

            float kTangent = mA + mB + iA * rtA * rtA + iB * rtB * rtB;

            vcp->tangentMass = kTangent > 0.0f ? 1.0f / kTangent : 0.0f;

            // Setup a velocity bias for restitution.
            vcp->velocityBias = 0.0f;
            float vRel = b2Dot(vc->normal, vB + b2Cross(wB, vcp->rB) - vA - b2Cross(wA, vcp->rA));
            if (vRel < -vc->threshold) { vcp->velocityBias = -vc->restitution * vRel; }
        }

        // If we have two points, then prepare the block solver.
        if (vc->pointCount == 2 && g_blockSolve) {
            b2VelocityConstraintPoint *vcp1 = vc->points + 0;
            b2VelocityConstraintPoint *vcp2 = vc->points + 1;

            float rn1A = b2Cross(vcp1->rA, vc->normal);
            float rn1B = b2Cross(vcp1->rB, vc->normal);
            float rn2A = b2Cross(vcp2->rA, vc->normal);
            float rn2B = b2Cross(vcp2->rB, vc->normal);

            float k11 = mA + mB + iA * rn1A * rn1A + iB * rn1B * rn1B;
            float k22 = mA + mB + iA * rn2A * rn2A + iB * rn2B * rn2B;
            float k12 = mA + mB + iA * rn1A * rn2A + iB * rn1B * rn2B;

            // Ensure a reasonable condition number.
            const float k_maxConditionNumber = 1000.0f;
            if (k11 * k11 < k_maxConditionNumber * (k11 * k22 - k12 * k12)) {
                // K is safe to invert.
                vc->K.ex.Set(k11, k12);
                vc->K.ey.Set(k12, k22);
                vc->normalMass = vc->K.GetInverse();
            } else {
                // The constraints are redundant, just use one.
                // TODO_ERIN use deepest?
                vc->pointCount = 1;
            }
        }
    }
}

void b2ContactSolver::WarmStart() {
    // Warm start.
    for (int32 i = 0; i < m_count; ++i) {
        b2ContactVelocityConstraint *vc = m_velocityConstraints + i;

        int32 indexA = vc->indexA;
        int32 indexB = vc->indexB;
        float mA = vc->invMassA;
        float iA = vc->invIA;
        float mB = vc->invMassB;
        float iB = vc->invIB;
        int32 pointCount = vc->pointCount;

        b2Vec2 vA = m_velocities[indexA].v;
        float wA = m_velocities[indexA].w;
        b2Vec2 vB = m_velocities[indexB].v;
        float wB = m_velocities[indexB].w;

        b2Vec2 normal = vc->normal;
        b2Vec2 tangent = b2Cross(normal, 1.0f);

        for (int32 j = 0; j < pointCount; ++j) {
            b2VelocityConstraintPoint *vcp = vc->points + j;
            b2Vec2 P = vcp->normalImpulse * normal + vcp->tangentImpulse * tangent;
            wA -= iA * b2Cross(vcp->rA, P);
            vA -= mA * P;
            wB += iB * b2Cross(vcp->rB, P);
            vB += mB * P;
        }

        m_velocities[indexA].v = vA;
        m_velocities[indexA].w = wA;
        m_velocities[indexB].v = vB;
        m_velocities[indexB].w = wB;
    }
}

void b2ContactSolver::SolveVelocityConstraints() {
    for (int32 i = 0; i < m_count; ++i) {
        b2ContactVelocityConstraint *vc = m_velocityConstraints + i;

        int32 indexA = vc->indexA;
        int32 indexB = vc->indexB;
        float mA = vc->invMassA;
        float iA = vc->invIA;
        float mB = vc->invMassB;
        float iB = vc->invIB;
        int32 pointCount = vc->pointCount;

        b2Vec2 vA = m_velocities[indexA].v;
        float wA = m_velocities[indexA].w;
        b2Vec2 vB = m_velocities[indexB].v;
        float wB = m_velocities[indexB].w;

        b2Vec2 normal = vc->normal;
        b2Vec2 tangent = b2Cross(normal, 1.0f);
        float friction = vc->friction;

        METADOT_ASSERT_E(pointCount == 1 || pointCount == 2);

        // Solve tangent constraints first because non-penetration is more important
        // than friction.
        for (int32 j = 0; j < pointCount; ++j) {
            b2VelocityConstraintPoint *vcp = vc->points + j;

            // Relative velocity at contact
            b2Vec2 dv = vB + b2Cross(wB, vcp->rB) - vA - b2Cross(wA, vcp->rA);

            // Compute tangent force
            float vt = b2Dot(dv, tangent) - vc->tangentSpeed;
            float lambda = vcp->tangentMass * (-vt);

            // b2Clamp the accumulated force
            float maxFriction = friction * vcp->normalImpulse;
            float newImpulse = b2Clamp(vcp->tangentImpulse + lambda, -maxFriction, maxFriction);
            lambda = newImpulse - vcp->tangentImpulse;
            vcp->tangentImpulse = newImpulse;

            // Apply contact impulse
            b2Vec2 P = lambda * tangent;

            vA -= mA * P;
            wA -= iA * b2Cross(vcp->rA, P);

            vB += mB * P;
            wB += iB * b2Cross(vcp->rB, P);
        }

        // Solve normal constraints
        if (pointCount == 1 || g_blockSolve == false) {
            for (int32 j = 0; j < pointCount; ++j) {
                b2VelocityConstraintPoint *vcp = vc->points + j;

                // Relative velocity at contact
                b2Vec2 dv = vB + b2Cross(wB, vcp->rB) - vA - b2Cross(wA, vcp->rA);

                // Compute normal impulse
                float vn = b2Dot(dv, normal);
                float lambda = -vcp->normalMass * (vn - vcp->velocityBias);

                // b2Clamp the accumulated impulse
                float newImpulse = b2Max(vcp->normalImpulse + lambda, 0.0f);
                lambda = newImpulse - vcp->normalImpulse;
                vcp->normalImpulse = newImpulse;

                // Apply contact impulse
                b2Vec2 P = lambda * normal;
                vA -= mA * P;
                wA -= iA * b2Cross(vcp->rA, P);

                vB += mB * P;
                wB += iB * b2Cross(vcp->rB, P);
            }
        } else {
            // Block solver developed in collaboration with Dirk Gregorius (back in 01/07 on Box2D_Lite).
            // Build the mini LCP for this contact patch
            //
            // vn = A * x + b, vn >= 0, x >= 0 and vn_i * x_i = 0 with i = 1..2
            //
            // A = J * W * JT and J = ( -n, -r1 x n, n, r2 x n )
            // b = vn0 - velocityBias
            //
            // The system is solved using the "Total enumeration method" (s. Murty). The complementary constraint vn_i * x_i
            // implies that we must have in any solution either vn_i = 0 or x_i = 0. So for the 2D contact problem the cases
            // vn1 = 0 and vn2 = 0, x1 = 0 and x2 = 0, x1 = 0 and vn2 = 0, x2 = 0 and vn1 = 0 need to be tested. The first valid
            // solution that satisfies the problem is chosen.
            //
            // In order to account of the accumulated impulse 'a' (because of the iterative nature of the solver which only requires
            // that the accumulated impulse is clamped and not the incremental impulse) we change the impulse variable (x_i).
            //
            // Substitute:
            //
            // x = a + d
            //
            // a := old total impulse
            // x := new total impulse
            // d := incremental impulse
            //
            // For the current iteration we extend the formula for the incremental impulse
            // to compute the new total impulse:
            //
            // vn = A * d + b
            //    = A * (x - a) + b
            //    = A * x + b - A * a
            //    = A * x + b'
            // b' = b - A * a;

            b2VelocityConstraintPoint *cp1 = vc->points + 0;
            b2VelocityConstraintPoint *cp2 = vc->points + 1;

            b2Vec2 a(cp1->normalImpulse, cp2->normalImpulse);
            METADOT_ASSERT_E(a.x >= 0.0f && a.y >= 0.0f);

            // Relative velocity at contact
            b2Vec2 dv1 = vB + b2Cross(wB, cp1->rB) - vA - b2Cross(wA, cp1->rA);
            b2Vec2 dv2 = vB + b2Cross(wB, cp2->rB) - vA - b2Cross(wA, cp2->rA);

            // Compute normal velocity
            float vn1 = b2Dot(dv1, normal);
            float vn2 = b2Dot(dv2, normal);

            b2Vec2 b;
            b.x = vn1 - cp1->velocityBias;
            b.y = vn2 - cp2->velocityBias;

            // Compute b'
            b -= b2Mul(vc->K, a);

            const float k_errorTol = 1e-3f;
            B2_NOT_USED(k_errorTol);

            for (;;) {
                //
                // Case 1: vn = 0
                //
                // 0 = A * x + b'
                //
                // Solve for x:
                //
                // x = - inv(A) * b'
                //
                b2Vec2 x = -b2Mul(vc->normalMass, b);

                if (x.x >= 0.0f && x.y >= 0.0f) {
                    // Get the incremental impulse
                    b2Vec2 d = x - a;

                    // Apply incremental impulse
                    b2Vec2 P1 = d.x * normal;
                    b2Vec2 P2 = d.y * normal;
                    vA -= mA * (P1 + P2);
                    wA -= iA * (b2Cross(cp1->rA, P1) + b2Cross(cp2->rA, P2));

                    vB += mB * (P1 + P2);
                    wB += iB * (b2Cross(cp1->rB, P1) + b2Cross(cp2->rB, P2));

                    // Accumulate
                    cp1->normalImpulse = x.x;
                    cp2->normalImpulse = x.y;

#if B2_DEBUG_SOLVER == 1
                    // Postconditions
                    dv1 = vB + b2Cross(wB, cp1->rB) - vA - b2Cross(wA, cp1->rA);
                    dv2 = vB + b2Cross(wB, cp2->rB) - vA - b2Cross(wA, cp2->rA);

                    // Compute normal velocity
                    vn1 = b2Dot(dv1, normal);
                    vn2 = b2Dot(dv2, normal);

                    METADOT_ASSERT_E(b2Abs(vn1 - cp1->velocityBias) < k_errorTol);
                    METADOT_ASSERT_E(b2Abs(vn2 - cp2->velocityBias) < k_errorTol);
#endif
                    break;
                }

                //
                // Case 2: vn1 = 0 and x2 = 0
                //
                //   0 = a11 * x1 + a12 * 0 + b1'
                // vn2 = a21 * x1 + a22 * 0 + b2'
                //
                x.x = -cp1->normalMass * b.x;
                x.y = 0.0f;
                vn1 = 0.0f;
                vn2 = vc->K.ex.y * x.x + b.y;
                if (x.x >= 0.0f && vn2 >= 0.0f) {
                    // Get the incremental impulse
                    b2Vec2 d = x - a;

                    // Apply incremental impulse
                    b2Vec2 P1 = d.x * normal;
                    b2Vec2 P2 = d.y * normal;
                    vA -= mA * (P1 + P2);
                    wA -= iA * (b2Cross(cp1->rA, P1) + b2Cross(cp2->rA, P2));

                    vB += mB * (P1 + P2);
                    wB += iB * (b2Cross(cp1->rB, P1) + b2Cross(cp2->rB, P2));

                    // Accumulate
                    cp1->normalImpulse = x.x;
                    cp2->normalImpulse = x.y;

#if B2_DEBUG_SOLVER == 1
                    // Postconditions
                    dv1 = vB + b2Cross(wB, cp1->rB) - vA - b2Cross(wA, cp1->rA);

                    // Compute normal velocity
                    vn1 = b2Dot(dv1, normal);

                    METADOT_ASSERT_E(b2Abs(vn1 - cp1->velocityBias) < k_errorTol);
#endif
                    break;
                }

                //
                // Case 3: vn2 = 0 and x1 = 0
                //
                // vn1 = a11 * 0 + a12 * x2 + b1'
                //   0 = a21 * 0 + a22 * x2 + b2'
                //
                x.x = 0.0f;
                x.y = -cp2->normalMass * b.y;
                vn1 = vc->K.ey.x * x.y + b.x;
                vn2 = 0.0f;

                if (x.y >= 0.0f && vn1 >= 0.0f) {
                    // Resubstitute for the incremental impulse
                    b2Vec2 d = x - a;

                    // Apply incremental impulse
                    b2Vec2 P1 = d.x * normal;
                    b2Vec2 P2 = d.y * normal;
                    vA -= mA * (P1 + P2);
                    wA -= iA * (b2Cross(cp1->rA, P1) + b2Cross(cp2->rA, P2));

                    vB += mB * (P1 + P2);
                    wB += iB * (b2Cross(cp1->rB, P1) + b2Cross(cp2->rB, P2));

                    // Accumulate
                    cp1->normalImpulse = x.x;
                    cp2->normalImpulse = x.y;

#if B2_DEBUG_SOLVER == 1
                    // Postconditions
                    dv2 = vB + b2Cross(wB, cp2->rB) - vA - b2Cross(wA, cp2->rA);

                    // Compute normal velocity
                    vn2 = b2Dot(dv2, normal);

                    METADOT_ASSERT_E(b2Abs(vn2 - cp2->velocityBias) < k_errorTol);
#endif
                    break;
                }

                //
                // Case 4: x1 = 0 and x2 = 0
                //
                // vn1 = b1
                // vn2 = b2;
                x.x = 0.0f;
                x.y = 0.0f;
                vn1 = b.x;
                vn2 = b.y;

                if (vn1 >= 0.0f && vn2 >= 0.0f) {
                    // Resubstitute for the incremental impulse
                    b2Vec2 d = x - a;

                    // Apply incremental impulse
                    b2Vec2 P1 = d.x * normal;
                    b2Vec2 P2 = d.y * normal;
                    vA -= mA * (P1 + P2);
                    wA -= iA * (b2Cross(cp1->rA, P1) + b2Cross(cp2->rA, P2));

                    vB += mB * (P1 + P2);
                    wB += iB * (b2Cross(cp1->rB, P1) + b2Cross(cp2->rB, P2));

                    // Accumulate
                    cp1->normalImpulse = x.x;
                    cp2->normalImpulse = x.y;

                    break;
                }

                // No solution, give up. This is hit sometimes, but it doesn't seem to matter.
                break;
            }
        }

        m_velocities[indexA].v = vA;
        m_velocities[indexA].w = wA;
        m_velocities[indexB].v = vB;
        m_velocities[indexB].w = wB;
    }
}

void b2ContactSolver::StoreImpulses() {
    for (int32 i = 0; i < m_count; ++i) {
        b2ContactVelocityConstraint *vc = m_velocityConstraints + i;
        b2Manifold *manifold = m_contacts[vc->contactIndex]->GetManifold();

        for (int32 j = 0; j < vc->pointCount; ++j) {
            manifold->points[j].normalImpulse = vc->points[j].normalImpulse;
            manifold->points[j].tangentImpulse = vc->points[j].tangentImpulse;
        }
    }
}

struct b2PositionSolverManifold
{
    void Initialize(b2ContactPositionConstraint *pc, const b2Transform &xfA, const b2Transform &xfB,
                    int32 index) {
        METADOT_ASSERT_E(pc->pointCount > 0);

        switch (pc->type) {
            case b2Manifold::e_circles: {
                b2Vec2 pointA = b2Mul(xfA, pc->localPoint);
                b2Vec2 pointB = b2Mul(xfB, pc->localPoints[0]);
                normal = pointB - pointA;
                normal.Normalize();
                point = 0.5f * (pointA + pointB);
                separation = b2Dot(pointB - pointA, normal) - pc->radiusA - pc->radiusB;
            } break;

            case b2Manifold::e_faceA: {
                normal = b2Mul(xfA.q, pc->localNormal);
                b2Vec2 planePoint = b2Mul(xfA, pc->localPoint);

                b2Vec2 clipPoint = b2Mul(xfB, pc->localPoints[index]);
                separation = b2Dot(clipPoint - planePoint, normal) - pc->radiusA - pc->radiusB;
                point = clipPoint;
            } break;

            case b2Manifold::e_faceB: {
                normal = b2Mul(xfB.q, pc->localNormal);
                b2Vec2 planePoint = b2Mul(xfB, pc->localPoint);

                b2Vec2 clipPoint = b2Mul(xfA, pc->localPoints[index]);
                separation = b2Dot(clipPoint - planePoint, normal) - pc->radiusA - pc->radiusB;
                point = clipPoint;

                // Ensure normal points from A to B
                normal = -normal;
            } break;
        }
    }

    b2Vec2 normal;
    b2Vec2 point;
    float separation;
};

// Sequential solver.
bool b2ContactSolver::SolvePositionConstraints() {
    float minSeparation = 0.0f;

    for (int32 i = 0; i < m_count; ++i) {
        b2ContactPositionConstraint *pc = m_positionConstraints + i;

        int32 indexA = pc->indexA;
        int32 indexB = pc->indexB;
        b2Vec2 localCenterA = pc->localCenterA;
        float mA = pc->invMassA;
        float iA = pc->invIA;
        b2Vec2 localCenterB = pc->localCenterB;
        float mB = pc->invMassB;
        float iB = pc->invIB;
        int32 pointCount = pc->pointCount;

        b2Vec2 cA = m_positions[indexA].c;
        float aA = m_positions[indexA].a;

        b2Vec2 cB = m_positions[indexB].c;
        float aB = m_positions[indexB].a;

        // Solve normal constraints
        for (int32 j = 0; j < pointCount; ++j) {
            b2Transform xfA, xfB;
            xfA.q.Set(aA);
            xfB.q.Set(aB);
            xfA.p = cA - b2Mul(xfA.q, localCenterA);
            xfB.p = cB - b2Mul(xfB.q, localCenterB);

            b2PositionSolverManifold psm;
            psm.Initialize(pc, xfA, xfB, j);
            b2Vec2 normal = psm.normal;

            b2Vec2 point = psm.point;
            float separation = psm.separation;

            b2Vec2 rA = point - cA;
            b2Vec2 rB = point - cB;

            // Track max constraint error.
            minSeparation = b2Min(minSeparation, separation);

            // Prevent large corrections and allow slop.
            float C = b2Clamp(b2_baumgarte * (separation + b2_linearSlop), -b2_maxLinearCorrection,
                              0.0f);

            // Compute the effective mass.
            float rnA = b2Cross(rA, normal);
            float rnB = b2Cross(rB, normal);
            float K = mA + mB + iA * rnA * rnA + iB * rnB * rnB;

            // Compute normal impulse
            float impulse = K > 0.0f ? -C / K : 0.0f;

            b2Vec2 P = impulse * normal;

            cA -= mA * P;
            aA -= iA * b2Cross(rA, P);

            cB += mB * P;
            aB += iB * b2Cross(rB, P);
        }

        m_positions[indexA].c = cA;
        m_positions[indexA].a = aA;

        m_positions[indexB].c = cB;
        m_positions[indexB].a = aB;
    }

    // We can't expect minSpeparation >= -b2_linearSlop because we don't
    // push the separation above -b2_linearSlop.
    return minSeparation >= -3.0f * b2_linearSlop;
}

// Sequential position solver for position constraints.
bool b2ContactSolver::SolveTOIPositionConstraints(int32 toiIndexA, int32 toiIndexB) {
    float minSeparation = 0.0f;

    for (int32 i = 0; i < m_count; ++i) {
        b2ContactPositionConstraint *pc = m_positionConstraints + i;

        int32 indexA = pc->indexA;
        int32 indexB = pc->indexB;
        b2Vec2 localCenterA = pc->localCenterA;
        b2Vec2 localCenterB = pc->localCenterB;
        int32 pointCount = pc->pointCount;

        float mA = 0.0f;
        float iA = 0.0f;
        if (indexA == toiIndexA || indexA == toiIndexB) {
            mA = pc->invMassA;
            iA = pc->invIA;
        }

        float mB = 0.0f;
        float iB = 0.;
        if (indexB == toiIndexA || indexB == toiIndexB) {
            mB = pc->invMassB;
            iB = pc->invIB;
        }

        b2Vec2 cA = m_positions[indexA].c;
        float aA = m_positions[indexA].a;

        b2Vec2 cB = m_positions[indexB].c;
        float aB = m_positions[indexB].a;

        // Solve normal constraints
        for (int32 j = 0; j < pointCount; ++j) {
            b2Transform xfA, xfB;
            xfA.q.Set(aA);
            xfB.q.Set(aB);
            xfA.p = cA - b2Mul(xfA.q, localCenterA);
            xfB.p = cB - b2Mul(xfB.q, localCenterB);

            b2PositionSolverManifold psm;
            psm.Initialize(pc, xfA, xfB, j);
            b2Vec2 normal = psm.normal;

            b2Vec2 point = psm.point;
            float separation = psm.separation;

            b2Vec2 rA = point - cA;
            b2Vec2 rB = point - cB;

            // Track max constraint error.
            minSeparation = b2Min(minSeparation, separation);

            // Prevent large corrections and allow slop.
            float C = b2Clamp(b2_toiBaumgarte * (separation + b2_linearSlop),
                              -b2_maxLinearCorrection, 0.0f);

            // Compute the effective mass.
            float rnA = b2Cross(rA, normal);
            float rnB = b2Cross(rB, normal);
            float K = mA + mB + iA * rnA * rnA + iB * rnB * rnB;

            // Compute normal impulse
            float impulse = K > 0.0f ? -C / K : 0.0f;

            b2Vec2 P = impulse * normal;

            cA -= mA * P;
            aA -= iA * b2Cross(rA, P);

            cB += mB * P;
            aB += iB * b2Cross(rB, P);
        }

        m_positions[indexA].c = cA;
        m_positions[indexA].a = aA;

        m_positions[indexB].c = cB;
        m_positions[indexB].a = aB;
    }

    // We can't expect minSpeparation >= -b2_linearSlop because we don't
    // push the separation above -b2_linearSlop.
    return minSeparation >= -1.5f * b2_linearSlop;
}

b2ContactFilter b2_defaultFilter;
b2ContactListener b2_defaultListener;

b2ContactManager::b2ContactManager() {
    m_contactList = nullptr;
    m_contactCount = 0;
    m_contactFilter = &b2_defaultFilter;
    m_contactListener = &b2_defaultListener;
    m_allocator = nullptr;
}

void b2ContactManager::Destroy(b2Contact *c) {
    b2Fixture *fixtureA = c->GetFixtureA();
    b2Fixture *fixtureB = c->GetFixtureB();
    b2Body *bodyA = fixtureA->GetBody();
    b2Body *bodyB = fixtureB->GetBody();

    if (m_contactListener && c->IsTouching()) { m_contactListener->EndContact(c); }

    // Remove from the world.
    if (c->m_prev) { c->m_prev->m_next = c->m_next; }

    if (c->m_next) { c->m_next->m_prev = c->m_prev; }

    if (c == m_contactList) { m_contactList = c->m_next; }

    // Remove from body 1
    if (c->m_nodeA.prev) { c->m_nodeA.prev->next = c->m_nodeA.next; }

    if (c->m_nodeA.next) { c->m_nodeA.next->prev = c->m_nodeA.prev; }

    if (&c->m_nodeA == bodyA->m_contactList) { bodyA->m_contactList = c->m_nodeA.next; }

    // Remove from body 2
    if (c->m_nodeB.prev) { c->m_nodeB.prev->next = c->m_nodeB.next; }

    if (c->m_nodeB.next) { c->m_nodeB.next->prev = c->m_nodeB.prev; }

    if (&c->m_nodeB == bodyB->m_contactList) { bodyB->m_contactList = c->m_nodeB.next; }

    // Call the factory.
    b2Contact::Destroy(c, m_allocator);
    --m_contactCount;
}

// This is the top level collision call for the time step. Here
// all the narrow phase collision is processed for the world
// contact list.
void b2ContactManager::Collide() {
    // Update awake contacts.
    b2Contact *c = m_contactList;
    while (c) {
        b2Fixture *fixtureA = c->GetFixtureA();
        b2Fixture *fixtureB = c->GetFixtureB();
        int32 indexA = c->GetChildIndexA();
        int32 indexB = c->GetChildIndexB();
        b2Body *bodyA = fixtureA->GetBody();
        b2Body *bodyB = fixtureB->GetBody();

        // Is this contact flagged for filtering?
        if (c->m_flags & b2Contact::e_filterFlag) {
            // Should these bodies collide?
            if (bodyB->ShouldCollide(bodyA) == false) {
                b2Contact *cNuke = c;
                c = cNuke->GetNext();
                Destroy(cNuke);
                continue;
            }

            // Check user filtering.
            if (m_contactFilter && m_contactFilter->ShouldCollide(fixtureA, fixtureB) == false) {
                b2Contact *cNuke = c;
                c = cNuke->GetNext();
                Destroy(cNuke);
                continue;
            }

            // Clear the filtering flag.
            c->m_flags &= ~b2Contact::e_filterFlag;
        }

        bool activeA = bodyA->IsAwake() && bodyA->m_type != b2_staticBody;
        bool activeB = bodyB->IsAwake() && bodyB->m_type != b2_staticBody;

        // At least one body must be awake and it must be dynamic or kinematic.
        if (activeA == false && activeB == false) {
            c = c->GetNext();
            continue;
        }

        int32 proxyIdA = fixtureA->m_proxies[indexA].proxyId;
        int32 proxyIdB = fixtureB->m_proxies[indexB].proxyId;
        bool overlap = m_broadPhase.TestOverlap(proxyIdA, proxyIdB);

        // Here we destroy contacts that cease to overlap in the broad-phase.
        if (overlap == false) {
            b2Contact *cNuke = c;
            c = cNuke->GetNext();
            Destroy(cNuke);
            continue;
        }

        // The contact persists.
        c->Update(m_contactListener);
        c = c->GetNext();
    }
}

void b2ContactManager::FindNewContacts() { m_broadPhase.UpdatePairs(this); }

void b2ContactManager::AddPair(void *proxyUserDataA, void *proxyUserDataB) {
    b2FixtureProxy *proxyA = (b2FixtureProxy *) proxyUserDataA;
    b2FixtureProxy *proxyB = (b2FixtureProxy *) proxyUserDataB;

    b2Fixture *fixtureA = proxyA->fixture;
    b2Fixture *fixtureB = proxyB->fixture;

    int32 indexA = proxyA->childIndex;
    int32 indexB = proxyB->childIndex;

    b2Body *bodyA = fixtureA->GetBody();
    b2Body *bodyB = fixtureB->GetBody();

    // Are the fixtures on the same body?
    if (bodyA == bodyB) { return; }

    // TODO_ERIN use a hash table to remove a potential bottleneck when both
    // bodies have a lot of contacts.
    // Does a contact already exist?
    b2ContactEdge *edge = bodyB->GetContactList();
    while (edge) {
        if (edge->other == bodyA) {
            b2Fixture *fA = edge->contact->GetFixtureA();
            b2Fixture *fB = edge->contact->GetFixtureB();
            int32 iA = edge->contact->GetChildIndexA();
            int32 iB = edge->contact->GetChildIndexB();

            if (fA == fixtureA && fB == fixtureB && iA == indexA && iB == indexB) {
                // A contact already exists.
                return;
            }

            if (fA == fixtureB && fB == fixtureA && iA == indexB && iB == indexA) {
                // A contact already exists.
                return;
            }
        }

        edge = edge->next;
    }

    // Does a joint override collision? Is at least one body dynamic?
    if (bodyB->ShouldCollide(bodyA) == false) { return; }

    // Check user filtering.
    if (m_contactFilter && m_contactFilter->ShouldCollide(fixtureA, fixtureB) == false) { return; }

    // Call the factory.
    b2Contact *c = b2Contact::Create(fixtureA, indexA, fixtureB, indexB, m_allocator);
    if (c == nullptr) { return; }

    // Contact creation may swap fixtures.
    fixtureA = c->GetFixtureA();
    fixtureB = c->GetFixtureB();
    indexA = c->GetChildIndexA();
    indexB = c->GetChildIndexB();
    bodyA = fixtureA->GetBody();
    bodyB = fixtureB->GetBody();

    // Insert into the world.
    c->m_prev = nullptr;
    c->m_next = m_contactList;
    if (m_contactList != nullptr) { m_contactList->m_prev = c; }
    m_contactList = c;

    // Connect to island graph.

    // Connect to body A
    c->m_nodeA.contact = c;
    c->m_nodeA.other = bodyB;

    c->m_nodeA.prev = nullptr;
    c->m_nodeA.next = bodyA->m_contactList;
    if (bodyA->m_contactList != nullptr) { bodyA->m_contactList->prev = &c->m_nodeA; }
    bodyA->m_contactList = &c->m_nodeA;

    // Connect to body B
    c->m_nodeB.contact = c;
    c->m_nodeB.other = bodyA;

    c->m_nodeB.prev = nullptr;
    c->m_nodeB.next = bodyB->m_contactList;
    if (bodyB->m_contactList != nullptr) { bodyB->m_contactList->prev = &c->m_nodeB; }
    bodyB->m_contactList = &c->m_nodeB;

    ++m_contactCount;
}

b2Contact *b2CircleContact::Create(b2Fixture *fixtureA, int32, b2Fixture *fixtureB, int32,
                                   b2BlockAllocator *allocator) {
    void *mem = allocator->Allocate(sizeof(b2CircleContact));
    return new (mem) b2CircleContact(fixtureA, fixtureB);
}

void b2CircleContact::Destroy(b2Contact *contact, b2BlockAllocator *allocator) {
    ((b2CircleContact *) contact)->~b2CircleContact();
    allocator->Free(contact, sizeof(b2CircleContact));
}

b2CircleContact::b2CircleContact(b2Fixture *fixtureA, b2Fixture *fixtureB)
    : b2Contact(fixtureA, 0, fixtureB, 0) {
    METADOT_ASSERT_E(m_fixtureA->GetType() == b2Shape::e_circle);
    METADOT_ASSERT_E(m_fixtureB->GetType() == b2Shape::e_circle);
}

void b2CircleContact::Evaluate(b2Manifold *manifold, const b2Transform &xfA,
                               const b2Transform &xfB) {
    b2CollideCircles(manifold, (b2CircleShape *) m_fixtureA->GetShape(), xfA,
                     (b2CircleShape *) m_fixtureB->GetShape(), xfB);
}

b2Contact *b2ChainAndPolygonContact::Create(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB,
                                            int32 indexB, b2BlockAllocator *allocator) {
    void *mem = allocator->Allocate(sizeof(b2ChainAndPolygonContact));
    return new (mem) b2ChainAndPolygonContact(fixtureA, indexA, fixtureB, indexB);
}

void b2ChainAndPolygonContact::Destroy(b2Contact *contact, b2BlockAllocator *allocator) {
    ((b2ChainAndPolygonContact *) contact)->~b2ChainAndPolygonContact();
    allocator->Free(contact, sizeof(b2ChainAndPolygonContact));
}

b2ChainAndPolygonContact::b2ChainAndPolygonContact(b2Fixture *fixtureA, int32 indexA,
                                                   b2Fixture *fixtureB, int32 indexB)
    : b2Contact(fixtureA, indexA, fixtureB, indexB) {
    METADOT_ASSERT_E(m_fixtureA->GetType() == b2Shape::e_chain);
    METADOT_ASSERT_E(m_fixtureB->GetType() == b2Shape::e_polygon);
}

void b2ChainAndPolygonContact::Evaluate(b2Manifold *manifold, const b2Transform &xfA,
                                        const b2Transform &xfB) {
    b2ChainShape *chain = (b2ChainShape *) m_fixtureA->GetShape();
    b2EdgeShape edge;
    chain->GetChildEdge(&edge, m_indexA);
    b2CollideEdgeAndPolygon(manifold, &edge, xfA, (b2PolygonShape *) m_fixtureB->GetShape(), xfB);
}

b2Contact *b2ChainAndCircleContact::Create(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB,
                                           int32 indexB, b2BlockAllocator *allocator) {
    void *mem = allocator->Allocate(sizeof(b2ChainAndCircleContact));
    return new (mem) b2ChainAndCircleContact(fixtureA, indexA, fixtureB, indexB);
}

void b2ChainAndCircleContact::Destroy(b2Contact *contact, b2BlockAllocator *allocator) {
    ((b2ChainAndCircleContact *) contact)->~b2ChainAndCircleContact();
    allocator->Free(contact, sizeof(b2ChainAndCircleContact));
}

b2ChainAndCircleContact::b2ChainAndCircleContact(b2Fixture *fixtureA, int32 indexA,
                                                 b2Fixture *fixtureB, int32 indexB)
    : b2Contact(fixtureA, indexA, fixtureB, indexB) {
    METADOT_ASSERT_E(m_fixtureA->GetType() == b2Shape::e_chain);
    METADOT_ASSERT_E(m_fixtureB->GetType() == b2Shape::e_circle);
}

void b2ChainAndCircleContact::Evaluate(b2Manifold *manifold, const b2Transform &xfA,
                                       const b2Transform &xfB) {
    b2ChainShape *chain = (b2ChainShape *) m_fixtureA->GetShape();
    b2EdgeShape edge;
    chain->GetChildEdge(&edge, m_indexA);
    b2CollideEdgeAndCircle(manifold, &edge, xfA, (b2CircleShape *) m_fixtureB->GetShape(), xfB);
}

struct b2RopeStretch
{
    int32 i1, i2;
    float invMass1, invMass2;
    float L;
    float lambda;
    float spring;
    float damper;
};

struct b2RopeBend
{
    int32 i1, i2, i3;
    float invMass1, invMass2, invMass3;
    float invEffectiveMass;
    float lambda;
    float L1, L2;
    float alpha1, alpha2;
    float spring;
    float damper;
};

b2Rope::b2Rope() {
    m_position.SetZero();
    m_count = 0;
    m_stretchCount = 0;
    m_bendCount = 0;
    m_stretchConstraints = nullptr;
    m_bendConstraints = nullptr;
    m_bindPositions = nullptr;
    m_ps = nullptr;
    m_p0s = nullptr;
    m_vs = nullptr;
    m_invMasses = nullptr;
    m_gravity.SetZero();
}

b2Rope::~b2Rope() {
    b2Free(m_stretchConstraints);
    b2Free(m_bendConstraints);
    b2Free(m_bindPositions);
    b2Free(m_ps);
    b2Free(m_p0s);
    b2Free(m_vs);
    b2Free(m_invMasses);
}

void b2Rope::Create(const b2RopeDef &def) {
    METADOT_ASSERT_E(def.count >= 3);
    m_position = def.position;
    m_count = def.count;
    m_bindPositions = (b2Vec2 *) b2Alloc(m_count * sizeof(b2Vec2));
    m_ps = (b2Vec2 *) b2Alloc(m_count * sizeof(b2Vec2));
    m_p0s = (b2Vec2 *) b2Alloc(m_count * sizeof(b2Vec2));
    m_vs = (b2Vec2 *) b2Alloc(m_count * sizeof(b2Vec2));
    m_invMasses = (float *) b2Alloc(m_count * sizeof(float));

    for (int32 i = 0; i < m_count; ++i) {
        m_bindPositions[i] = def.vertices[i];
        m_ps[i] = def.vertices[i] + m_position;
        m_p0s[i] = def.vertices[i] + m_position;
        m_vs[i].SetZero();

        float m = def.masses[i];
        if (m > 0.0f) {
            m_invMasses[i] = 1.0f / m;
        } else {
            m_invMasses[i] = 0.0f;
        }
    }

    m_stretchCount = m_count - 1;
    m_bendCount = m_count - 2;

    m_stretchConstraints = (b2RopeStretch *) b2Alloc(m_stretchCount * sizeof(b2RopeStretch));
    m_bendConstraints = (b2RopeBend *) b2Alloc(m_bendCount * sizeof(b2RopeBend));

    for (int32 i = 0; i < m_stretchCount; ++i) {
        b2RopeStretch &c = m_stretchConstraints[i];

        b2Vec2 p1 = m_ps[i];
        b2Vec2 p2 = m_ps[i + 1];

        c.i1 = i;
        c.i2 = i + 1;
        c.L = b2Distance(p1, p2);
        c.invMass1 = m_invMasses[i];
        c.invMass2 = m_invMasses[i + 1];
        c.lambda = 0.0f;
        c.damper = 0.0f;
        c.spring = 0.0f;
    }

    for (int32 i = 0; i < m_bendCount; ++i) {
        b2RopeBend &c = m_bendConstraints[i];

        b2Vec2 p1 = m_ps[i];
        b2Vec2 p2 = m_ps[i + 1];
        b2Vec2 p3 = m_ps[i + 2];

        c.i1 = i;
        c.i2 = i + 1;
        c.i3 = i + 2;
        c.invMass1 = m_invMasses[i];
        c.invMass2 = m_invMasses[i + 1];
        c.invMass3 = m_invMasses[i + 2];
        c.invEffectiveMass = 0.0f;
        c.L1 = b2Distance(p1, p2);
        c.L2 = b2Distance(p2, p3);
        c.lambda = 0.0f;

        // Pre-compute effective mass (TODO use flattened config)
        b2Vec2 e1 = p2 - p1;
        b2Vec2 e2 = p3 - p2;
        float L1sqr = e1.LengthSquared();
        float L2sqr = e2.LengthSquared();

        if (L1sqr * L2sqr == 0.0f) { continue; }

        b2Vec2 Jd1 = (-1.0f / L1sqr) * e1.Skew();
        b2Vec2 Jd2 = (1.0f / L2sqr) * e2.Skew();

        b2Vec2 J1 = -Jd1;
        b2Vec2 J2 = Jd1 - Jd2;
        b2Vec2 J3 = Jd2;

        c.invEffectiveMass = c.invMass1 * b2Dot(J1, J1) + c.invMass2 * b2Dot(J2, J2) +
                             c.invMass3 * b2Dot(J3, J3);

        b2Vec2 r = p3 - p1;

        float rr = r.LengthSquared();
        if (rr == 0.0f) { continue; }

        // a1 = h2 / (h1 + h2)
        // a2 = h1 / (h1 + h2)
        c.alpha1 = b2Dot(e2, r) / rr;
        c.alpha2 = b2Dot(e1, r) / rr;
    }

    m_gravity = def.gravity;

    SetTuning(def.tuning);
}

void b2Rope::SetTuning(const b2RopeTuning &tuning) {
    m_tuning = tuning;

    // Pre-compute spring and damper values based on tuning

    const float bendOmega = 2.0f * b2_pi * m_tuning.bendHertz;

    for (int32 i = 0; i < m_bendCount; ++i) {
        b2RopeBend &c = m_bendConstraints[i];

        float L1sqr = c.L1 * c.L1;
        float L2sqr = c.L2 * c.L2;

        if (L1sqr * L2sqr == 0.0f) {
            c.spring = 0.0f;
            c.damper = 0.0f;
            continue;
        }

        // Flatten the triangle formed by the two edges
        float J2 = 1.0f / c.L1 + 1.0f / c.L2;
        float sum = c.invMass1 / L1sqr + c.invMass2 * J2 * J2 + c.invMass3 / L2sqr;
        if (sum == 0.0f) {
            c.spring = 0.0f;
            c.damper = 0.0f;
            continue;
        }

        float mass = 1.0f / sum;

        c.spring = mass * bendOmega * bendOmega;
        c.damper = 2.0f * mass * m_tuning.bendDamping * bendOmega;
    }

    const float stretchOmega = 2.0f * b2_pi * m_tuning.stretchHertz;

    for (int32 i = 0; i < m_stretchCount; ++i) {
        b2RopeStretch &c = m_stretchConstraints[i];

        float sum = c.invMass1 + c.invMass2;
        if (sum == 0.0f) { continue; }

        float mass = 1.0f / sum;

        c.spring = mass * stretchOmega * stretchOmega;
        c.damper = 2.0f * mass * m_tuning.stretchDamping * stretchOmega;
    }
}

void b2Rope::Step(float dt, int32 iterations, const b2Vec2 &position) {
    if (dt == 0.0) { return; }

    const float inv_dt = 1.0f / dt;
    float d = expf(-dt * m_tuning.damping);

    // Apply gravity and damping
    for (int32 i = 0; i < m_count; ++i) {
        if (m_invMasses[i] > 0.0f) {
            m_vs[i] *= d;
            m_vs[i] += dt * m_gravity;
        } else {
            m_vs[i] = inv_dt * (m_bindPositions[i] + position - m_p0s[i]);
        }
    }

    // Apply bending spring
    if (m_tuning.bendingModel == b2_springAngleBendingModel) { ApplyBendForces(dt); }

    for (int32 i = 0; i < m_bendCount; ++i) { m_bendConstraints[i].lambda = 0.0f; }

    for (int32 i = 0; i < m_stretchCount; ++i) { m_stretchConstraints[i].lambda = 0.0f; }

    // Update position
    for (int32 i = 0; i < m_count; ++i) { m_ps[i] += dt * m_vs[i]; }

    // Solve constraints
    for (int32 i = 0; i < iterations; ++i) {
        if (m_tuning.bendingModel == b2_pbdAngleBendingModel) {
            SolveBend_PBD_Angle();
        } else if (m_tuning.bendingModel == b2_xpbdAngleBendingModel) {
            SolveBend_XPBD_Angle(dt);
        } else if (m_tuning.bendingModel == b2_pbdDistanceBendingModel) {
            SolveBend_PBD_Distance();
        } else if (m_tuning.bendingModel == b2_pbdHeightBendingModel) {
            SolveBend_PBD_Height();
        } else if (m_tuning.bendingModel == b2_pbdTriangleBendingModel) {
            SolveBend_PBD_Triangle();
        }

        if (m_tuning.stretchingModel == b2_pbdStretchingModel) {
            SolveStretch_PBD();
        } else if (m_tuning.stretchingModel == b2_xpbdStretchingModel) {
            SolveStretch_XPBD(dt);
        }
    }

    // Constrain velocity
    for (int32 i = 0; i < m_count; ++i) {
        m_vs[i] = inv_dt * (m_ps[i] - m_p0s[i]);
        m_p0s[i] = m_ps[i];
    }
}

void b2Rope::Reset(const b2Vec2 &position) {
    m_position = position;

    for (int32 i = 0; i < m_count; ++i) {
        m_ps[i] = m_bindPositions[i] + m_position;
        m_p0s[i] = m_bindPositions[i] + m_position;
        m_vs[i].SetZero();
    }

    for (int32 i = 0; i < m_bendCount; ++i) { m_bendConstraints[i].lambda = 0.0f; }

    for (int32 i = 0; i < m_stretchCount; ++i) { m_stretchConstraints[i].lambda = 0.0f; }
}

void b2Rope::SolveStretch_PBD() {
    const float stiffness = m_tuning.stretchStiffness;

    for (int32 i = 0; i < m_stretchCount; ++i) {
        const b2RopeStretch &c = m_stretchConstraints[i];

        b2Vec2 p1 = m_ps[c.i1];
        b2Vec2 p2 = m_ps[c.i2];

        b2Vec2 d = p2 - p1;
        float L = d.Normalize();

        float sum = c.invMass1 + c.invMass2;
        if (sum == 0.0f) { continue; }

        float s1 = c.invMass1 / sum;
        float s2 = c.invMass2 / sum;

        p1 -= stiffness * s1 * (c.L - L) * d;
        p2 += stiffness * s2 * (c.L - L) * d;

        m_ps[c.i1] = p1;
        m_ps[c.i2] = p2;
    }
}

void b2Rope::SolveStretch_XPBD(float dt) {
    METADOT_ASSERT_E(dt > 0.0f);

    for (int32 i = 0; i < m_stretchCount; ++i) {
        b2RopeStretch &c = m_stretchConstraints[i];

        b2Vec2 p1 = m_ps[c.i1];
        b2Vec2 p2 = m_ps[c.i2];

        b2Vec2 dp1 = p1 - m_p0s[c.i1];
        b2Vec2 dp2 = p2 - m_p0s[c.i2];

        b2Vec2 u = p2 - p1;
        float L = u.Normalize();

        b2Vec2 J1 = -u;
        b2Vec2 J2 = u;

        float sum = c.invMass1 + c.invMass2;
        if (sum == 0.0f) { continue; }

        const float alpha = 1.0f / (c.spring * dt * dt);// 1 / kg
        const float beta = dt * dt * c.damper;          // kg * s
        const float sigma = alpha * beta / dt;          // non-dimensional
        float C = L - c.L;

        // This is using the initial velocities
        float Cdot = b2Dot(J1, dp1) + b2Dot(J2, dp2);

        float B = C + alpha * c.lambda + sigma * Cdot;
        float sum2 = (1.0f + sigma) * sum + alpha;

        float impulse = -B / sum2;

        p1 += (c.invMass1 * impulse) * J1;
        p2 += (c.invMass2 * impulse) * J2;

        m_ps[c.i1] = p1;
        m_ps[c.i2] = p2;
        c.lambda += impulse;
    }
}

void b2Rope::SolveBend_PBD_Angle() {
    const float stiffness = m_tuning.bendStiffness;

    for (int32 i = 0; i < m_bendCount; ++i) {
        const b2RopeBend &c = m_bendConstraints[i];

        b2Vec2 p1 = m_ps[c.i1];
        b2Vec2 p2 = m_ps[c.i2];
        b2Vec2 p3 = m_ps[c.i3];

        b2Vec2 d1 = p2 - p1;
        b2Vec2 d2 = p3 - p2;
        float a = b2Cross(d1, d2);
        float b = b2Dot(d1, d2);

        float angle = b2Atan2(a, b);

        float L1sqr, L2sqr;

        if (m_tuning.isometric) {
            L1sqr = c.L1 * c.L1;
            L2sqr = c.L2 * c.L2;
        } else {
            L1sqr = d1.LengthSquared();
            L2sqr = d2.LengthSquared();
        }

        if (L1sqr * L2sqr == 0.0f) { continue; }

        b2Vec2 Jd1 = (-1.0f / L1sqr) * d1.Skew();
        b2Vec2 Jd2 = (1.0f / L2sqr) * d2.Skew();

        b2Vec2 J1 = -Jd1;
        b2Vec2 J2 = Jd1 - Jd2;
        b2Vec2 J3 = Jd2;

        float sum;
        if (m_tuning.fixedEffectiveMass) {
            sum = c.invEffectiveMass;
        } else {
            sum = c.invMass1 * b2Dot(J1, J1) + c.invMass2 * b2Dot(J2, J2) +
                  c.invMass3 * b2Dot(J3, J3);
        }

        if (sum == 0.0f) { sum = c.invEffectiveMass; }

        float impulse = -stiffness * angle / sum;

        p1 += (c.invMass1 * impulse) * J1;
        p2 += (c.invMass2 * impulse) * J2;
        p3 += (c.invMass3 * impulse) * J3;

        m_ps[c.i1] = p1;
        m_ps[c.i2] = p2;
        m_ps[c.i3] = p3;
    }
}

void b2Rope::SolveBend_XPBD_Angle(float dt) {
    METADOT_ASSERT_E(dt > 0.0f);

    for (int32 i = 0; i < m_bendCount; ++i) {
        b2RopeBend &c = m_bendConstraints[i];

        b2Vec2 p1 = m_ps[c.i1];
        b2Vec2 p2 = m_ps[c.i2];
        b2Vec2 p3 = m_ps[c.i3];

        b2Vec2 dp1 = p1 - m_p0s[c.i1];
        b2Vec2 dp2 = p2 - m_p0s[c.i2];
        b2Vec2 dp3 = p3 - m_p0s[c.i3];

        b2Vec2 d1 = p2 - p1;
        b2Vec2 d2 = p3 - p2;

        float L1sqr, L2sqr;

        if (m_tuning.isometric) {
            L1sqr = c.L1 * c.L1;
            L2sqr = c.L2 * c.L2;
        } else {
            L1sqr = d1.LengthSquared();
            L2sqr = d2.LengthSquared();
        }

        if (L1sqr * L2sqr == 0.0f) { continue; }

        float a = b2Cross(d1, d2);
        float b = b2Dot(d1, d2);

        float angle = b2Atan2(a, b);

        b2Vec2 Jd1 = (-1.0f / L1sqr) * d1.Skew();
        b2Vec2 Jd2 = (1.0f / L2sqr) * d2.Skew();

        b2Vec2 J1 = -Jd1;
        b2Vec2 J2 = Jd1 - Jd2;
        b2Vec2 J3 = Jd2;

        float sum;
        if (m_tuning.fixedEffectiveMass) {
            sum = c.invEffectiveMass;
        } else {
            sum = c.invMass1 * b2Dot(J1, J1) + c.invMass2 * b2Dot(J2, J2) +
                  c.invMass3 * b2Dot(J3, J3);
        }

        if (sum == 0.0f) { continue; }

        const float alpha = 1.0f / (c.spring * dt * dt);
        const float beta = dt * dt * c.damper;
        const float sigma = alpha * beta / dt;
        float C = angle;

        // This is using the initial velocities
        float Cdot = b2Dot(J1, dp1) + b2Dot(J2, dp2) + b2Dot(J3, dp3);

        float B = C + alpha * c.lambda + sigma * Cdot;
        float sum2 = (1.0f + sigma) * sum + alpha;

        float impulse = -B / sum2;

        p1 += (c.invMass1 * impulse) * J1;
        p2 += (c.invMass2 * impulse) * J2;
        p3 += (c.invMass3 * impulse) * J3;

        m_ps[c.i1] = p1;
        m_ps[c.i2] = p2;
        m_ps[c.i3] = p3;
        c.lambda += impulse;
    }
}

void b2Rope::ApplyBendForces(float dt) {
    // omega = 2 * pi * hz
    const float omega = 2.0f * b2_pi * m_tuning.bendHertz;

    for (int32 i = 0; i < m_bendCount; ++i) {
        const b2RopeBend &c = m_bendConstraints[i];

        b2Vec2 p1 = m_ps[c.i1];
        b2Vec2 p2 = m_ps[c.i2];
        b2Vec2 p3 = m_ps[c.i3];

        b2Vec2 v1 = m_vs[c.i1];
        b2Vec2 v2 = m_vs[c.i2];
        b2Vec2 v3 = m_vs[c.i3];

        b2Vec2 d1 = p2 - p1;
        b2Vec2 d2 = p3 - p2;

        float L1sqr, L2sqr;

        if (m_tuning.isometric) {
            L1sqr = c.L1 * c.L1;
            L2sqr = c.L2 * c.L2;
        } else {
            L1sqr = d1.LengthSquared();
            L2sqr = d2.LengthSquared();
        }

        if (L1sqr * L2sqr == 0.0f) { continue; }

        float a = b2Cross(d1, d2);
        float b = b2Dot(d1, d2);

        float angle = b2Atan2(a, b);

        b2Vec2 Jd1 = (-1.0f / L1sqr) * d1.Skew();
        b2Vec2 Jd2 = (1.0f / L2sqr) * d2.Skew();

        b2Vec2 J1 = -Jd1;
        b2Vec2 J2 = Jd1 - Jd2;
        b2Vec2 J3 = Jd2;

        float sum;
        if (m_tuning.fixedEffectiveMass) {
            sum = c.invEffectiveMass;
        } else {
            sum = c.invMass1 * b2Dot(J1, J1) + c.invMass2 * b2Dot(J2, J2) +
                  c.invMass3 * b2Dot(J3, J3);
        }

        if (sum == 0.0f) { continue; }

        float mass = 1.0f / sum;

        const float spring = mass * omega * omega;
        const float damper = 2.0f * mass * m_tuning.bendDamping * omega;

        float C = angle;
        float Cdot = b2Dot(J1, v1) + b2Dot(J2, v2) + b2Dot(J3, v3);

        float impulse = -dt * (spring * C + damper * Cdot);

        m_vs[c.i1] += (c.invMass1 * impulse) * J1;
        m_vs[c.i2] += (c.invMass2 * impulse) * J2;
        m_vs[c.i3] += (c.invMass3 * impulse) * J3;
    }
}

void b2Rope::SolveBend_PBD_Distance() {
    const float stiffness = m_tuning.bendStiffness;

    for (int32 i = 0; i < m_bendCount; ++i) {
        const b2RopeBend &c = m_bendConstraints[i];

        int32 i1 = c.i1;
        int32 i2 = c.i3;

        b2Vec2 p1 = m_ps[i1];
        b2Vec2 p2 = m_ps[i2];

        b2Vec2 d = p2 - p1;
        float L = d.Normalize();

        float sum = c.invMass1 + c.invMass3;
        if (sum == 0.0f) { continue; }

        float s1 = c.invMass1 / sum;
        float s2 = c.invMass3 / sum;

        p1 -= stiffness * s1 * (c.L1 + c.L2 - L) * d;
        p2 += stiffness * s2 * (c.L1 + c.L2 - L) * d;

        m_ps[i1] = p1;
        m_ps[i2] = p2;
    }
}

// Constraint based implementation of:
// P. Volino: Simple Linear Bending Stiffness in Particle Systems
void b2Rope::SolveBend_PBD_Height() {
    const float stiffness = m_tuning.bendStiffness;

    for (int32 i = 0; i < m_bendCount; ++i) {
        const b2RopeBend &c = m_bendConstraints[i];

        b2Vec2 p1 = m_ps[c.i1];
        b2Vec2 p2 = m_ps[c.i2];
        b2Vec2 p3 = m_ps[c.i3];

        // Barycentric coordinates are held constant
        b2Vec2 d = c.alpha1 * p1 + c.alpha2 * p3 - p2;
        float dLen = d.Length();

        if (dLen == 0.0f) { continue; }

        b2Vec2 dHat = (1.0f / dLen) * d;

        b2Vec2 J1 = c.alpha1 * dHat;
        b2Vec2 J2 = -dHat;
        b2Vec2 J3 = c.alpha2 * dHat;

        float sum =
                c.invMass1 * c.alpha1 * c.alpha1 + c.invMass2 + c.invMass3 * c.alpha2 * c.alpha2;

        if (sum == 0.0f) { continue; }

        float C = dLen;
        float mass = 1.0f / sum;
        float impulse = -stiffness * mass * C;

        p1 += (c.invMass1 * impulse) * J1;
        p2 += (c.invMass2 * impulse) * J2;
        p3 += (c.invMass3 * impulse) * J3;

        m_ps[c.i1] = p1;
        m_ps[c.i2] = p2;
        m_ps[c.i3] = p3;
    }
}

// M. Kelager: A Triangle Bending Constraint Model for PBD
void b2Rope::SolveBend_PBD_Triangle() {
    const float stiffness = m_tuning.bendStiffness;

    for (int32 i = 0; i < m_bendCount; ++i) {
        const b2RopeBend &c = m_bendConstraints[i];

        b2Vec2 b0 = m_ps[c.i1];
        b2Vec2 v = m_ps[c.i2];
        b2Vec2 b1 = m_ps[c.i3];

        float wb0 = c.invMass1;
        float wv = c.invMass2;
        float wb1 = c.invMass3;

        float W = wb0 + wb1 + 2.0f * wv;
        float invW = stiffness / W;

        b2Vec2 d = v - (1.0f / 3.0f) * (b0 + v + b1);

        b2Vec2 db0 = 2.0f * wb0 * invW * d;
        b2Vec2 dv = -4.0f * wv * invW * d;
        b2Vec2 db1 = 2.0f * wb1 * invW * d;

        b0 += db0;
        v += dv;
        b1 += db1;

        m_ps[c.i1] = b0;
        m_ps[c.i2] = v;
        m_ps[c.i3] = b1;
    }
}

void b2Rope::Draw(DebugDraw *draw) const {
    b2Color c(0.4f, 0.5f, 0.7f);
    b2Color pg(0.1f, 0.8f, 0.1f);
    b2Color pd(0.7f, 0.2f, 0.4f);

    for (int32 i = 0; i < m_count - 1; ++i) {
        draw->DrawSegment(m_ps[i], m_ps[i + 1], c);

        const b2Color &pc = m_invMasses[i] > 0.0f ? pd : pg;
        draw->DrawPoint(m_ps[i], 5.0f, pc);
    }

    const b2Color &pc = m_invMasses[m_count - 1] > 0.0f ? pd : pg;
    draw->DrawPoint(m_ps[m_count - 1], 5.0f, pc);
}

static const int32 b2_chunkSize = 16 * 1024;
static const int32 b2_maxBlockSize = 640;
static const int32 b2_chunkArrayIncrement = 128;

// These are the supported object sizes. Actual allocations are rounded up the next size.
static const int32 b2_blockSizes[b2_blockSizeCount] = {
        16, // 0
        32, // 1
        64, // 2
        96, // 3
        128,// 4
        160,// 5
        192,// 6
        224,// 7
        256,// 8
        320,// 9
        384,// 10
        448,// 11
        512,// 12
        640,// 13
};

// This maps an arbitrary allocation size to a suitable slot in b2_blockSizes.
struct b2SizeMap
{
    b2SizeMap() {
        int32 j = 0;
        values[0] = 0;
        for (int32 i = 1; i <= b2_maxBlockSize; ++i) {
            METADOT_ASSERT_E(j < b2_blockSizeCount);
            if (i <= b2_blockSizes[j]) {
                values[i] = (uint8) j;
            } else {
                ++j;
                values[i] = (uint8) j;
            }
        }
    }

    uint8 values[b2_maxBlockSize + 1];
};

static const b2SizeMap b2_sizeMap;

struct b2Chunk
{
    int32 blockSize;
    b2Block *blocks;
};

struct b2Block
{
    b2Block *next;
};

b2BlockAllocator::b2BlockAllocator() {
    METADOT_ASSERT_E(b2_blockSizeCount < UCHAR_MAX);

    m_chunkSpace = b2_chunkArrayIncrement;
    m_chunkCount = 0;
    m_chunks = (b2Chunk *) b2Alloc(m_chunkSpace * sizeof(b2Chunk));

    memset(m_chunks, 0, m_chunkSpace * sizeof(b2Chunk));
    memset(m_freeLists, 0, sizeof(m_freeLists));
}

b2BlockAllocator::~b2BlockAllocator() {
    for (int32 i = 0; i < m_chunkCount; ++i) { b2Free(m_chunks[i].blocks); }

    b2Free(m_chunks);
}

void *b2BlockAllocator::Allocate(int32 size) {
    if (size == 0) { return nullptr; }

    METADOT_ASSERT_E(0 < size);

    if (size > b2_maxBlockSize) { return b2Alloc(size); }

    int32 index = b2_sizeMap.values[size];
    METADOT_ASSERT_E(0 <= index && index < b2_blockSizeCount);

    if (m_freeLists[index]) {
        b2Block *block = m_freeLists[index];
        m_freeLists[index] = block->next;
        return block;
    } else {
        if (m_chunkCount == m_chunkSpace) {
            b2Chunk *oldChunks = m_chunks;
            m_chunkSpace += b2_chunkArrayIncrement;
            m_chunks = (b2Chunk *) b2Alloc(m_chunkSpace * sizeof(b2Chunk));
            memcpy(m_chunks, oldChunks, m_chunkCount * sizeof(b2Chunk));
            memset(m_chunks + m_chunkCount, 0, b2_chunkArrayIncrement * sizeof(b2Chunk));
            b2Free(oldChunks);
        }

        b2Chunk *chunk = m_chunks + m_chunkCount;
        chunk->blocks = (b2Block *) b2Alloc(b2_chunkSize);
#if defined(_DEBUG)
        memset(chunk->blocks, 0xcd, b2_chunkSize);
#endif
        int32 blockSize = b2_blockSizes[index];
        chunk->blockSize = blockSize;
        int32 blockCount = b2_chunkSize / blockSize;
        METADOT_ASSERT_E(blockCount * blockSize <= b2_chunkSize);
        for (int32 i = 0; i < blockCount - 1; ++i) {
            b2Block *block = (b2Block *) ((int8 *) chunk->blocks + blockSize * i);
            b2Block *next = (b2Block *) ((int8 *) chunk->blocks + blockSize * (i + 1));
            block->next = next;
        }
        b2Block *last = (b2Block *) ((int8 *) chunk->blocks + blockSize * (blockCount - 1));
        last->next = nullptr;

        m_freeLists[index] = chunk->blocks->next;
        ++m_chunkCount;

        return chunk->blocks;
    }
}

void b2BlockAllocator::Free(void *p, int32 size) {
    if (size == 0) { return; }

    METADOT_ASSERT_E(0 < size);

    if (size > b2_maxBlockSize) {
        b2Free(p);
        return;
    }

    int32 index = b2_sizeMap.values[size];
    METADOT_ASSERT_E(0 <= index && index < b2_blockSizeCount);

#if defined(_DEBUG)
    // Verify the memory address and size is valid.
    int32 blockSize = b2_blockSizes[index];
    bool found = false;
    for (int32 i = 0; i < m_chunkCount; ++i) {
        b2Chunk *chunk = m_chunks + i;
        if (chunk->blockSize != blockSize) {
            METADOT_ASSERT_E((int8 *) p + blockSize <= (int8 *) chunk->blocks ||
                     (int8 *) chunk->blocks + b2_chunkSize <= (int8 *) p);
        } else {
            if ((int8 *) chunk->blocks <= (int8 *) p &&
                (int8 *) p + blockSize <= (int8 *) chunk->blocks + b2_chunkSize) {
                found = true;
            }
        }
    }

    METADOT_ASSERT_E(found);

    memset(p, 0xfd, blockSize);
#endif

    b2Block *block = (b2Block *) p;
    block->next = m_freeLists[index];
    m_freeLists[index] = block;
}

void b2BlockAllocator::Clear() {
    for (int32 i = 0; i < m_chunkCount; ++i) { b2Free(m_chunks[i].blocks); }

    m_chunkCount = 0;
    memset(m_chunks, 0, m_chunkSpace * sizeof(b2Chunk));
    memset(m_freeLists, 0, sizeof(m_freeLists));
}

const b2Vec2 b2Vec2_zero(0.0f, 0.0f);

/// Solve A * x = b, where b is a column vector. This is more efficient
/// than computing the inverse in one-shot cases.
b2Vec3 b2Mat33::Solve33(const b2Vec3 &b) const {
    float det = b2Dot(ex, b2Cross(ey, ez));
    if (det != 0.0f) { det = 1.0f / det; }
    b2Vec3 x;
    x.x = det * b2Dot(b, b2Cross(ey, ez));
    x.y = det * b2Dot(ex, b2Cross(b, ez));
    x.z = det * b2Dot(ex, b2Cross(ey, b));
    return x;
}

/// Solve A * x = b, where b is a column vector. This is more efficient
/// than computing the inverse in one-shot cases.
b2Vec2 b2Mat33::Solve22(const b2Vec2 &b) const {
    float a11 = ex.x, a12 = ey.x, a21 = ex.y, a22 = ey.y;
    float det = a11 * a22 - a12 * a21;
    if (det != 0.0f) { det = 1.0f / det; }
    b2Vec2 x;
    x.x = det * (a22 * b.x - a12 * b.y);
    x.y = det * (a11 * b.y - a21 * b.x);
    return x;
}

///
void b2Mat33::GetInverse22(b2Mat33 *M) const {
    float a = ex.x, b = ey.x, c = ex.y, d = ey.y;
    float det = a * d - b * c;
    if (det != 0.0f) { det = 1.0f / det; }

    M->ex.x = det * d;
    M->ey.x = -det * b;
    M->ex.z = 0.0f;
    M->ex.y = -det * c;
    M->ey.y = det * a;
    M->ey.z = 0.0f;
    M->ez.x = 0.0f;
    M->ez.y = 0.0f;
    M->ez.z = 0.0f;
}

/// Returns the zero matrix if singular.
void b2Mat33::GetSymInverse33(b2Mat33 *M) const {
    float det = b2Dot(ex, b2Cross(ey, ez));
    if (det != 0.0f) { det = 1.0f / det; }

    float a11 = ex.x, a12 = ey.x, a13 = ez.x;
    float a22 = ey.y, a23 = ez.y;
    float a33 = ez.z;

    M->ex.x = det * (a22 * a33 - a23 * a23);
    M->ex.y = det * (a13 * a23 - a12 * a33);
    M->ex.z = det * (a12 * a23 - a13 * a22);

    M->ey.x = M->ex.y;
    M->ey.y = det * (a11 * a33 - a13 * a13);
    M->ey.z = det * (a13 * a12 - a11 * a23);

    M->ez.x = M->ex.z;
    M->ez.y = M->ey.z;
    M->ez.z = det * (a11 * a22 - a12 * a12);
}

b2Version b2_version = {2, 4, 1};

// Memory allocators. Modify these to use your own allocator.
void *b2Alloc_Default(int32 size) { return malloc(size); }

void b2Free_Default(void *mem) { free(mem); }

// You can modify this to use your logging facility.
void b2Log_Default(const char *string, va_list args) { vprintf(string, args); }

FILE *b2_dumpFile = nullptr;

void b2OpenDump(const char *fileName) {
    METADOT_ASSERT_E(b2_dumpFile == nullptr);
    b2_dumpFile = fopen(fileName, "w");
}

void b2Dump(const char *string, ...) {
    if (b2_dumpFile == nullptr) { return; }

    va_list args;
    va_start(args, string);
    vfprintf(b2_dumpFile, string, args);
    va_end(args);
}

void b2CloseDump() {
    fclose(b2_dumpFile);
    b2_dumpFile = nullptr;
}

b2StackAllocator::b2StackAllocator() {
    m_index = 0;
    m_allocation = 0;
    m_maxAllocation = 0;
    m_entryCount = 0;
}

b2StackAllocator::~b2StackAllocator() {
    METADOT_ASSERT_E(m_index == 0);
    METADOT_ASSERT_E(m_entryCount == 0);
}

void *b2StackAllocator::Allocate(int32 size) {
    METADOT_ASSERT_E(m_entryCount < b2_maxStackEntries);

    b2StackEntry *entry = m_entries + m_entryCount;
    entry->size = size;
    if (m_index + size > b2_stackSize) {
        entry->data = (char *) b2Alloc(size);
        entry->usedMalloc = true;
    } else {
        entry->data = m_data + m_index;
        entry->usedMalloc = false;
        m_index += size;
    }

    m_allocation += size;
    m_maxAllocation = b2Max(m_maxAllocation, m_allocation);
    ++m_entryCount;

    return entry->data;
}

void b2StackAllocator::Free(void *p) {
    METADOT_ASSERT_E(m_entryCount > 0);
    b2StackEntry *entry = m_entries + m_entryCount - 1;
    METADOT_ASSERT_E(p == entry->data);
    if (entry->usedMalloc) {
        b2Free(p);
    } else {
        m_index -= entry->size;
    }
    m_allocation -= entry->size;
    --m_entryCount;

    p = nullptr;
}

int32 b2StackAllocator::GetMaxAllocation() const { return m_maxAllocation; }

#if defined(_WIN32)

double b2Timer::s_invFrequency = 0.0;

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

b2Timer::b2Timer() {
    LARGE_INTEGER largeInteger;

    if (s_invFrequency == 0.0) {
        QueryPerformanceFrequency(&largeInteger);
        s_invFrequency = double(largeInteger.QuadPart);
        if (s_invFrequency > 0.0) { s_invFrequency = 1000.0 / s_invFrequency; }
    }

    QueryPerformanceCounter(&largeInteger);
    m_start = double(largeInteger.QuadPart);
}

void b2Timer::Reset() {
    LARGE_INTEGER largeInteger;
    QueryPerformanceCounter(&largeInteger);
    m_start = double(largeInteger.QuadPart);
}

float b2Timer::GetMilliseconds() const {
    LARGE_INTEGER largeInteger;
    QueryPerformanceCounter(&largeInteger);
    double count = double(largeInteger.QuadPart);
    float ms = float(s_invFrequency * (count - m_start));
    return ms;
}

#elif defined(__linux__) || defined(__APPLE__)

#include <sys/time.h>

b2Timer::b2Timer() { Reset(); }

void b2Timer::Reset() {
    timeval t;
    gettimeofday(&t, 0);
    m_start_sec = t.tv_sec;
    m_start_usec = t.tv_usec;
}

float b2Timer::GetMilliseconds() const {
    timeval t;
    gettimeofday(&t, 0);
    time_t start_sec = m_start_sec;
    suseconds_t start_usec = m_start_usec;

    // http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
    if (t.tv_usec < start_usec) {
        int nsec = (start_usec - t.tv_usec) / 1000000 + 1;
        start_usec -= 1000000 * nsec;
        start_sec += nsec;
    }

    if (t.tv_usec - start_usec > 1000000) {
        int nsec = (t.tv_usec - start_usec) / 1000000;
        start_usec += 1000000 * nsec;
        start_sec -= nsec;
    }
    return 1000.0f * (t.tv_sec - start_sec) + 0.001f * (t.tv_usec - start_usec);
}

#else

b2Timer::b2Timer() {}

void b2Timer::Reset() {}

float b2Timer::GetMilliseconds() const { return 0.0f; }

#endif

b2ChainShape::~b2ChainShape() { Clear(); }

void b2ChainShape::Clear() {
    b2Free(m_vertices);
    m_vertices = nullptr;
    m_count = 0;
}

void b2ChainShape::CreateLoop(const b2Vec2 *vertices, int32 count) {
    METADOT_ASSERT_E(m_vertices == nullptr && m_count == 0);
    METADOT_ASSERT_E(count >= 3);
    if (count < 3) { return; }

    for (int32 i = 1; i < count; ++i) {
        b2Vec2 v1 = vertices[i - 1];
        b2Vec2 v2 = vertices[i];
        // If the code crashes here, it means your vertices are too close together.
        METADOT_ASSERT_E(b2DistanceSquared(v1, v2) > b2_linearSlop * b2_linearSlop);
    }

    m_count = count + 1;
    m_vertices = (b2Vec2 *) b2Alloc(m_count * sizeof(b2Vec2));
    memcpy(m_vertices, vertices, count * sizeof(b2Vec2));
    m_vertices[count] = m_vertices[0];
    m_prevVertex = m_vertices[m_count - 2];
    m_nextVertex = m_vertices[1];
}

void b2ChainShape::CreateChain(const b2Vec2 *vertices, int32 count, const b2Vec2 &prevVertex,
                               const b2Vec2 &nextVertex) {
    METADOT_ASSERT_E(m_vertices == nullptr && m_count == 0);
    METADOT_ASSERT_E(count >= 2);
    for (int32 i = 1; i < count; ++i) {
        // If the code crashes here, it means your vertices are too close together.
        METADOT_ASSERT_E(b2DistanceSquared(vertices[i - 1], vertices[i]) > b2_linearSlop * b2_linearSlop);
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
    METADOT_ASSERT_E(0 <= index && index < m_count - 1);
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
    METADOT_ASSERT_E(childIndex < m_count);

    b2EdgeShape edgeShape;

    int32 i1 = childIndex;
    int32 i2 = childIndex + 1;
    if (i2 == m_count) { i2 = 0; }

    edgeShape.m_vertex1 = m_vertices[i1];
    edgeShape.m_vertex2 = m_vertices[i2];

    return edgeShape.RayCast(output, input, xf, 0);
}

void b2ChainShape::ComputeAABB(b2AABB *aabb, const b2Transform &xf, int32 childIndex) const {
    METADOT_ASSERT_E(childIndex < m_count);

    int32 i1 = childIndex;
    int32 i2 = childIndex + 1;
    if (i2 == m_count) { i2 = 0; }

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
    if (buffer) { BufferMove(proxyId); }
}

void b2BroadPhase::TouchProxy(int32 proxyId) { BufferMove(proxyId); }

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
        if (m_moveBuffer[i] == proxyId) { m_moveBuffer[i] = e_nullProxy; }
    }
}

// This is called from b2DynamicTree::Query when we are gathering pairs.
bool b2BroadPhase::QueryCallback(int32 proxyId) {
    // A proxy cannot form a pair with itself.
    if (proxyId == m_queryProxyId) { return true; }

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

int32 b2CircleShape::GetChildCount() const { return 1; }

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
    if (sigma < 0.0f || rr < b2_epsilon) { return false; }

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

void b2CircleShape::ComputeAABB(b2AABB *aabb, const b2Transform &transform,
                                int32 childIndex) const {
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

void b2CollideCircles(b2Manifold *manifold, const b2CircleShape *circleA, const b2Transform &xfA,
                      const b2CircleShape *circleB, const b2Transform &xfB) {
    manifold->pointCount = 0;

    b2Vec2 pA = b2Mul(xfA, circleA->m_p);
    b2Vec2 pB = b2Mul(xfB, circleB->m_p);

    b2Vec2 d = pB - pA;
    float distSqr = b2Dot(d, d);
    float rA = circleA->m_radius, rB = circleB->m_radius;
    float radius = rA + rB;
    if (distSqr > radius * radius) { return; }

    manifold->type = b2Manifold::e_circles;
    manifold->localPoint = circleA->m_p;
    manifold->localNormal.SetZero();
    manifold->pointCount = 1;

    manifold->points[0].localPoint = circleB->m_p;
    manifold->points[0].id.key = 0;
}

void b2CollidePolygonAndCircle(b2Manifold *manifold, const b2PolygonShape *polygonA,
                               const b2Transform &xfA, const b2CircleShape *circleB,
                               const b2Transform &xfB) {
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
        if (b2DistanceSquared(cLocal, v1) > radius * radius) { return; }

        manifold->pointCount = 1;
        manifold->type = b2Manifold::e_faceA;
        manifold->localNormal = cLocal - v1;
        manifold->localNormal.Normalize();
        manifold->localPoint = v1;
        manifold->points[0].localPoint = circleB->m_p;
        manifold->points[0].id.key = 0;
    } else if (u2 <= 0.0f) {
        if (b2DistanceSquared(cLocal, v2) > radius * radius) { return; }

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
        if (s > radius) { return; }

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
void b2CollideEdgeAndCircle(b2Manifold *manifold, const b2EdgeShape *edgeA, const b2Transform &xfA,
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
    if (oneSided && offset < 0.0f) { return; }

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
        if (dd > radius * radius) { return; }

        // Is there an edge connected to A?
        if (edgeA->m_oneSided) {
            b2Vec2 A1 = edgeA->m_vertex0;
            b2Vec2 B1 = A;
            b2Vec2 e1 = B1 - A1;
            float u1 = b2Dot(e1, B1 - Q);

            // Is the circle in Region AB of the previous edge?
            if (u1 > 0.0f) { return; }
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
        if (dd > radius * radius) { return; }

        // Is there an edge connected to B?
        if (edgeA->m_oneSided) {
            b2Vec2 B2 = edgeA->m_vertex3;
            b2Vec2 A2 = B;
            b2Vec2 e2 = B2 - A2;
            float v2 = b2Dot(e2, Q - A2);

            // Is the circle in Region AB of the next edge?
            if (v2 > 0.0f) { return; }
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
    METADOT_ASSERT_E(den > 0.0f);
    b2Vec2 P = (1.0f / den) * (u * A + v * B);
    b2Vec2 d = Q - P;
    float dd = b2Dot(d, d);
    if (dd > radius * radius) { return; }

    if (offset < 0.0f) { n.Set(-n.x, -n.y); }
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

static b2EPAxis b2ComputeEdgeSeparation(const b2TempPolygon &polygonB, const b2Vec2 &v1,
                                        const b2Vec2 &normal1) {
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
            if (si < sj) { sj = si; }
        }

        if (sj > axis.separation) {
            axis.index = j;
            axis.separation = sj;
            axis.normal = axes[j];
        }
    }

    return axis;
}

static b2EPAxis b2ComputePolygonSeparation(const b2TempPolygon &polygonB, const b2Vec2 &v1,
                                           const b2Vec2 &v2) {
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

void b2CollideEdgeAndPolygon(b2Manifold *manifold, const b2EdgeShape *edgeA, const b2Transform &xfA,
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
    if (oneSided && offset1 < 0.0f) { return; }

    // Get polygonB in frameA
    b2TempPolygon tempPolygonB;
    tempPolygonB.count = polygonB->m_count;
    for (int32 i = 0; i < polygonB->m_count; ++i) {
        tempPolygonB.vertices[i] = b2Mul(xf, polygonB->m_vertices[i]);
        tempPolygonB.normals[i] = b2Mul(xf.q, polygonB->m_normals[i]);
    }

    float radius = polygonB->m_radius + edgeA->m_radius;

    b2EPAxis edgeAxis = b2ComputeEdgeSeparation(tempPolygonB, v1, normal1);
    if (edgeAxis.separation > radius) { return; }

    b2EPAxis polygonAxis = b2ComputePolygonSeparation(tempPolygonB, v1, v2);
    if (polygonAxis.separation > radius) { return; }

    // Use hysteresis for jitter reduction.
    const float k_relativeTol = 0.98f;
    const float k_absoluteTol = 0.001f;

    b2EPAxis primaryAxis;
    if (polygonAxis.separation - radius >
        k_relativeTol * (edgeAxis.separation - radius) + k_absoluteTol) {
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

    if (np < b2_maxManifoldPoints) { return; }

    // Clip to side 2
    np = b2ClipSegmentToLine(clipPoints2, clipPoints1, ref.sideNormal2, ref.sideOffset2, ref.i2);

    if (np < b2_maxManifoldPoints) { return; }

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
static float b2FindMaxSeparation(int32 *edgeIndex, const b2PolygonShape *poly1,
                                 const b2Transform &xf1, const b2PolygonShape *poly2,
                                 const b2Transform &xf2) {
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
            if (sij < si) { si = sij; }
        }

        if (si > maxSeparation) {
            maxSeparation = si;
            bestIndex = i;
        }
    }

    *edgeIndex = bestIndex;
    return maxSeparation;
}

static void b2FindIncidentEdge(b2ClipVertex c[2], const b2PolygonShape *poly1,
                               const b2Transform &xf1, int32 edge1, const b2PolygonShape *poly2,
                               const b2Transform &xf2) {
    const b2Vec2 *normals1 = poly1->m_normals;

    int32 count2 = poly2->m_count;
    const b2Vec2 *vertices2 = poly2->m_vertices;
    const b2Vec2 *normals2 = poly2->m_normals;

    METADOT_ASSERT_E(0 <= edge1 && edge1 < poly1->m_count);

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
void b2CollidePolygons(b2Manifold *manifold, const b2PolygonShape *polyA, const b2Transform &xfA,
                       const b2PolygonShape *polyB, const b2Transform &xfB) {
    manifold->pointCount = 0;
    float totalRadius = polyA->m_radius + polyB->m_radius;

    int32 edgeA = 0;
    float separationA = b2FindMaxSeparation(&edgeA, polyA, xfA, polyB, xfB);
    if (separationA > totalRadius) return;

    int32 edgeB = 0;
    float separationB = b2FindMaxSeparation(&edgeB, polyB, xfB, polyA, xfA);
    if (separationB > totalRadius) return;

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

    if (np < 2) return;

    // Clip to negative box side 1
    np = b2ClipSegmentToLine(clipPoints2, clipPoints1, tangent, sideOffset2, iv2);

    if (np < 2) { return; }

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

void b2WorldManifold::Initialize(const b2Manifold *manifold, const b2Transform &xfA, float radiusA,
                                 const b2Transform &xfB, float radiusB) {
    if (manifold->pointCount == 0) { return; }

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

void b2GetPointStates(b2PointState state1[b2_maxManifoldPoints],
                      b2PointState state2[b2_maxManifoldPoints], const b2Manifold *manifold1,
                      const b2Manifold *manifold2) {
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
            if (p(i) < lowerBound(i) || upperBound(i) < p(i)) { return false; }
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

            if (tmin > tmax) { return false; }
        }
    }

    // Does the ray start inside the box?
    // Does the ray intersect beyond the max fraction?
    if (tmin < 0.0f || input.maxFraction < tmin) { return false; }

    // Intersection.
    output->fraction = tmin;
    output->normal = normal;
    return true;
}

// Sutherland-Hodgman clipping.
int32 b2ClipSegmentToLine(b2ClipVertex vOut[2], const b2ClipVertex vIn[2], const b2Vec2 &normal,
                          float offset, int32 vertexIndexA) {
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

        METADOT_ASSERT_E(count == 2);
    }

    return count;
}

bool b2TestOverlap(const b2Shape *shapeA, int32 indexA, const b2Shape *shapeB, int32 indexB,
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
            METADOT_ASSERT_E(0 <= index && index < chain->m_count);

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
            METADOT_ASSERT_E(false);
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
    void ReadCache(const b2SimplexCache *cache, const b2DistanceProxy *proxyA,
                   const b2Transform &transformA, const b2DistanceProxy *proxyB,
                   const b2Transform &transformB) {
        METADOT_ASSERT_E(cache->count <= 3);

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
                METADOT_ASSERT_E(false);
                return b2Vec2_zero;
        }
    }

    b2Vec2 GetClosestPoint() const {
        switch (m_count) {
            case 0:
                METADOT_ASSERT_E(false);
                return b2Vec2_zero;

            case 1:
                return m_v1.w;

            case 2:
                return m_v1.a * m_v1.w + m_v2.a * m_v2.w;

            case 3:
                return b2Vec2_zero;

            default:
                METADOT_ASSERT_E(false);
                return b2Vec2_zero;
        }
    }

    void GetWitnessPoints(b2Vec2 *pA, b2Vec2 *pB) const {
        switch (m_count) {
            case 0:
                METADOT_ASSERT_E(false);
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
                METADOT_ASSERT_E(false);
                break;
        }
    }

    float GetMetric() const {
        switch (m_count) {
            case 0:
                METADOT_ASSERT_E(false);
                return 0.0f;

            case 1:
                return 0.0f;

            case 2:
                return b2Distance(m_v1.w, m_v2.w);

            case 3:
                return b2Cross(m_v2.w - m_v1.w, m_v3.w - m_v1.w);

            default:
                METADOT_ASSERT_E(false);
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

void b2Distance(b2DistanceOutput *output, b2SimplexCache *cache, const b2DistanceInput *input) {
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
                METADOT_ASSERT_E(false);
        }

        // If we have 3 points, then the origin is in the corresponding triangle.
        if (simplex.m_count == 3) { break; }

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
        if (duplicate) { break; }

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
        METADOT_ASSERT_E(simplex.m_count < 3);

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
            if (vr <= 0.0f) { return false; }

            lambda = (vp - sigma) / vr;
            if (lambda > 1.0f) { return false; }

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
                METADOT_ASSERT_E(false);
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
        METADOT_ASSERT_E(m_nodeCount == m_nodeCapacity);

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
    METADOT_ASSERT_E(0 <= nodeId && nodeId < m_nodeCapacity);
    METADOT_ASSERT_E(0 < m_nodeCount);
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
    METADOT_ASSERT_E(0 <= proxyId && proxyId < m_nodeCapacity);
    METADOT_ASSERT_E(m_nodes[proxyId].IsLeaf());

    RemoveLeaf(proxyId);
    FreeNode(proxyId);
}

bool b2DynamicTree::MoveProxy(int32 proxyId, const b2AABB &aabb, const b2Vec2 &displacement) {
    METADOT_ASSERT_E(0 <= proxyId && proxyId < m_nodeCapacity);

    METADOT_ASSERT_E(m_nodes[proxyId].IsLeaf());

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
        if (cost < cost1 && cost < cost2) { break; }

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

        METADOT_ASSERT_E(child1 != b2_nullNode);
        METADOT_ASSERT_E(child2 != b2_nullNode);

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
    METADOT_ASSERT_E(iA != b2_nullNode);

    b2TreeNode *A = m_nodes + iA;
    if (A->IsLeaf() || A->height < 2) { return iA; }

    int32 iB = A->child1;
    int32 iC = A->child2;
    METADOT_ASSERT_E(0 <= iB && iB < m_nodeCapacity);
    METADOT_ASSERT_E(0 <= iC && iC < m_nodeCapacity);

    b2TreeNode *B = m_nodes + iB;
    b2TreeNode *C = m_nodes + iC;

    int32 balance = C->height - B->height;

    // Rotate C up
    if (balance > 1) {
        int32 iF = C->child1;
        int32 iG = C->child2;
        b2TreeNode *F = m_nodes + iF;
        b2TreeNode *G = m_nodes + iG;
        METADOT_ASSERT_E(0 <= iF && iF < m_nodeCapacity);
        METADOT_ASSERT_E(0 <= iG && iG < m_nodeCapacity);

        // Swap A and C
        C->child1 = iA;
        C->parent = A->parent;
        A->parent = iC;

        // A's old parent should point to C
        if (C->parent != b2_nullNode) {
            if (m_nodes[C->parent].child1 == iA) {
                m_nodes[C->parent].child1 = iC;
            } else {
                METADOT_ASSERT_E(m_nodes[C->parent].child2 == iA);
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
        METADOT_ASSERT_E(0 <= iD && iD < m_nodeCapacity);
        METADOT_ASSERT_E(0 <= iE && iE < m_nodeCapacity);

        // Swap A and B
        B->child1 = iA;
        B->parent = A->parent;
        A->parent = iB;

        // A's old parent should point to B
        if (B->parent != b2_nullNode) {
            if (m_nodes[B->parent].child1 == iA) {
                m_nodes[B->parent].child1 = iB;
            } else {
                METADOT_ASSERT_E(m_nodes[B->parent].child2 == iA);
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
    if (m_root == b2_nullNode) { return 0; }

    return m_nodes[m_root].height;
}

//
float b2DynamicTree::GetAreaRatio() const {
    if (m_root == b2_nullNode) { return 0.0f; }

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
    METADOT_ASSERT_E(0 <= nodeId && nodeId < m_nodeCapacity);
    b2TreeNode *node = m_nodes + nodeId;

    if (node->IsLeaf()) { return 0; }

    int32 height1 = ComputeHeight(node->child1);
    int32 height2 = ComputeHeight(node->child2);
    return 1 + b2Max(height1, height2);
}

int32 b2DynamicTree::ComputeHeight() const {
    int32 height = ComputeHeight(m_root);
    return height;
}

void b2DynamicTree::ValidateStructure(int32 index) const {
    if (index == b2_nullNode) { return; }

    if (index == m_root) { METADOT_ASSERT_E(m_nodes[index].parent == b2_nullNode); }

    const b2TreeNode *node = m_nodes + index;

    int32 child1 = node->child1;
    int32 child2 = node->child2;

    if (node->IsLeaf()) {
        METADOT_ASSERT_E(child1 == b2_nullNode);
        METADOT_ASSERT_E(child2 == b2_nullNode);
        METADOT_ASSERT_E(node->height == 0);
        return;
    }

    METADOT_ASSERT_E(0 <= child1 && child1 < m_nodeCapacity);
    METADOT_ASSERT_E(0 <= child2 && child2 < m_nodeCapacity);

    METADOT_ASSERT_E(m_nodes[child1].parent == index);
    METADOT_ASSERT_E(m_nodes[child2].parent == index);

    ValidateStructure(child1);
    ValidateStructure(child2);
}

void b2DynamicTree::ValidateMetrics(int32 index) const {
    if (index == b2_nullNode) { return; }

    const b2TreeNode *node = m_nodes + index;

    int32 child1 = node->child1;
    int32 child2 = node->child2;

    if (node->IsLeaf()) {
        METADOT_ASSERT_E(child1 == b2_nullNode);
        METADOT_ASSERT_E(child2 == b2_nullNode);
        METADOT_ASSERT_E(node->height == 0);
        return;
    }

    METADOT_ASSERT_E(0 <= child1 && child1 < m_nodeCapacity);
    METADOT_ASSERT_E(0 <= child2 && child2 < m_nodeCapacity);

    int32 height1 = m_nodes[child1].height;
    int32 height2 = m_nodes[child2].height;
    int32 height;
    height = 1 + b2Max(height1, height2);
    METADOT_ASSERT_E(node->height == height);

    b2AABB aabb;
    aabb.Combine(m_nodes[child1].aabb, m_nodes[child2].aabb);

    METADOT_ASSERT_E(aabb.lowerBound == node->aabb.lowerBound);
    METADOT_ASSERT_E(aabb.upperBound == node->aabb.upperBound);

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
        METADOT_ASSERT_E(0 <= freeIndex && freeIndex < m_nodeCapacity);
        freeIndex = m_nodes[freeIndex].next;
        ++freeCount;
    }

    METADOT_ASSERT_E(GetHeight() == ComputeHeight());

    METADOT_ASSERT_E(m_nodeCount + freeCount == m_nodeCapacity);
#endif
}

int32 b2DynamicTree::GetMaxBalance() const {
    int32 maxBalance = 0;
    for (int32 i = 0; i < m_nodeCapacity; ++i) {
        const b2TreeNode *node = m_nodes + i;
        if (node->height <= 1) { continue; }

        METADOT_ASSERT_E(node->IsLeaf() == false);

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

void b2EdgeShape::SetOneSided(const b2Vec2 &v0, const b2Vec2 &v1, const b2Vec2 &v2,
                              const b2Vec2 &v3) {
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

int32 b2EdgeShape::GetChildCount() const { return 1; }

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
    if (m_oneSided && numerator > 0.0f) { return false; }

    float denominator = b2Dot(normal, d);

    if (denominator == 0.0f) { return false; }

    float t = numerator / denominator;
    if (t < 0.0f || input.maxFraction < t) { return false; }

    b2Vec2 q = p1 + t * d;

    // q = v1 + s * r
    // s = dot(q - v1, r) / dot(r, r)
    b2Vec2 r = v2 - v1;
    float rr = b2Dot(r, r);
    if (rr == 0.0f) { return false; }

    float s = b2Dot(q - v1, r) / rr;
    if (s < 0.0f || 1.0f < s) { return false; }

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

int32 b2PolygonShape::GetChildCount() const { return 1; }

static b2Vec2 ComputeCentroid(const b2Vec2 *vs, int32 count) {
    METADOT_ASSERT_E(count >= 3);

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
    METADOT_ASSERT_E(area > b2_epsilon);
    c = (1.0f / area) * c + s;
    return c;
}

void b2PolygonShape::Set(const b2Vec2 *vertices, int32 count) {
    METADOT_ASSERT_E(3 <= count && count <= b2_maxPolygonVertices);
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

        if (unique) { ps[tempCount++] = v; }
    }

    n = tempCount;
    if (n < 3) {
        // Polygon is degenerate.
        METADOT_ASSERT_E(false);
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
        METADOT_ASSERT_E(m < b2_maxPolygonVertices);
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
            if (c < 0.0f) { ie = j; }

            // Collinearity check
            if (c == 0.0f && v.LengthSquared() > r.LengthSquared()) { ie = j; }
        }

        ++m;
        ih = ie;

        if (ie == i0) { break; }
    }

    if (m < 3) {
        // Polygon is degenerate.
        METADOT_ASSERT_E(false);
        SetAsBox(1.0f, 1.0f);
        return;
    }

    m_count = m;

    // Copy vertices.
    for (int32 i = 0; i < m; ++i) { m_vertices[i] = ps[hull[i]]; }

    // Compute normals. Ensure the edges have non-zero length.
    for (int32 i = 0; i < m; ++i) {
        int32 i1 = i;
        int32 i2 = i + 1 < m ? i + 1 : 0;
        b2Vec2 edge = m_vertices[i2] - m_vertices[i1];
        METADOT_ASSERT_E(edge.LengthSquared() > b2_epsilon * b2_epsilon);
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
        if (dot > 0.0f) { return false; }
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
            if (numerator < 0.0f) { return false; }
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
        if (upper < lower) { return false; }
    }

    METADOT_ASSERT_E(0.0f <= lower && lower <= input.maxFraction);

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

    METADOT_ASSERT_E(m_count >= 3);

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
    METADOT_ASSERT_E(area > b2_epsilon);
    center *= 1.0f / area;
    massData->center = center + s;

    // Inertia tensor relative to the local origin (point s).
    massData->I = density * I;

    // Shift to center of mass then to original body origin.
    massData->I +=
            massData->mass * (b2Dot(massData->center, massData->center) - b2Dot(center, center));
}

bool b2PolygonShape::Validate() const {
    for (int32 i = 0; i < m_count; ++i) {
        int32 i1 = i;
        int32 i2 = i < m_count - 1 ? i1 + 1 : 0;
        b2Vec2 p = m_vertices[i1];
        b2Vec2 e = m_vertices[i2] - p;

        for (int32 j = 0; j < m_count; ++j) {
            if (j == i1 || j == i2) { continue; }

            b2Vec2 v = m_vertices[j] - p;
            float c = b2Cross(e, v);
            if (c < 0.0f) { return false; }
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

    float Initialize(const b2SimplexCache *cache, const b2DistanceProxy *proxyA,
                     const b2Sweep &sweepA, const b2DistanceProxy *proxyB, const b2Sweep &sweepB,
                     float t1) {
        m_proxyA = proxyA;
        m_proxyB = proxyB;
        int32 count = cache->count;
        METADOT_ASSERT_E(0 < count && count < 3);

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
                METADOT_ASSERT_E(false);
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
                METADOT_ASSERT_E(false);
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
    METADOT_ASSERT_E(target > tolerance);

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

                if (rootIterCount == 50) { break; }
            }

            b2_toiMaxRootIters = b2Max(b2_toiMaxRootIters, rootIterCount);

            ++pushBackIter;

            if (pushBackIter == b2_maxPolygonVertices) { break; }
        }

        ++iter;
        ++b2_toiIters;

        if (done) { break; }

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
