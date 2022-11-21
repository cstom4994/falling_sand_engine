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
#include "box2d/b2_body.h"
#include "box2d/b2_broad_phase.h"
#include "box2d/b2_chain_circle_contact.h"
#include "box2d/b2_chain_polygon_contact.h"
#include "box2d/b2_chain_shape.h"
#include "box2d/b2_circle_contact.h"
#include "box2d/b2_circle_shape.h"
#include "box2d/b2_collision.h"
#include "box2d/b2_contact.h"
#include "box2d/b2_contact_manager.h"
#include "box2d/b2_contact_solver.h"
#include "box2d/b2_distance.h"
#include "box2d/b2_distance_joint.h"
#include "box2d/b2_draw.h"
#include "box2d/b2_edge_circle_contact.h"
#include "box2d/b2_edge_polygon_contact.h"
#include "box2d/b2_edge_shape.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_friction_joint.h"
#include "box2d/b2_gear_joint.h"
#include "box2d/b2_island.h"
#include "box2d/b2_joint.h"
#include "box2d/b2_motor_joint.h"
#include "box2d/b2_mouse_joint.h"
#include "box2d/b2_polygon_circle_contact.h"
#include "box2d/b2_polygon_contact.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_prismatic_joint.h"
#include "box2d/b2_pulley_joint.h"
#include "box2d/b2_revolute_joint.h"
#include "box2d/b2_rope.h"
#include "box2d/b2_shape.h"
#include "box2d/b2_stack_allocator.h"
#include "box2d/b2_time_of_impact.h"
#include "box2d/b2_time_step.h"
#include "box2d/b2_timer.h"
#include "box2d/b2_weld_joint.h"
#include "box2d/b2_wheel_joint.h"
#include "box2d/b2_world.h"
#include "box2d/b2_world_callbacks.h"
#include <new>
#include <stdio.h>

b2Body::b2Body(const b2BodyDef *bd, b2World *world) {
    b2Assert(bd->position.IsValid());
    b2Assert(bd->linearVelocity.IsValid());
    b2Assert(b2IsValid(bd->angle));
    b2Assert(b2IsValid(bd->angularVelocity));
    b2Assert(b2IsValid(bd->angularDamping) && bd->angularDamping >= 0.0f);
    b2Assert(b2IsValid(bd->linearDamping) && bd->linearDamping >= 0.0f);

    m_flags = 0;

    if (bd->bullet) {
        m_flags |= e_bulletFlag;
    }
    if (bd->fixedRotation) {
        m_flags |= e_fixedRotationFlag;
    }
    if (bd->allowSleep) {
        m_flags |= e_autoSleepFlag;
    }
    if (bd->awake && bd->type != b2_staticBody) {
        m_flags |= e_awakeFlag;
    }
    if (bd->enabled) {
        m_flags |= e_enabledFlag;
    }

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
    b2Assert(m_world->IsLocked() == false);
    if (m_world->IsLocked() == true) {
        return;
    }

    if (m_type == type) {
        return;
    }

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
        for (int32 i = 0; i < proxyCount; ++i) {
            broadPhase->TouchProxy(f->m_proxies[i].proxyId);
        }
    }
}

b2Fixture *b2Body::CreateFixture(const b2FixtureDef *def) {
    b2Assert(m_world->IsLocked() == false);
    if (m_world->IsLocked() == true) {
        return nullptr;
    }

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
    if (fixture->m_density > 0.0f) {
        ResetMassData();
    }

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
    if (fixture == NULL) {
        return;
    }

    b2Assert(m_world->IsLocked() == false);
    if (m_world->IsLocked() == true) {
        return;
    }

    b2Assert(fixture->m_body == this);

    // Remove the fixture from this body's singly linked list.
    b2Assert(m_fixtureCount > 0);
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
    b2Assert(found);

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
    if (density > 0.0f) {
        ResetMassData();
    }
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

    b2Assert(m_type == b2_dynamicBody);

    // Accumulate mass over all fixtures.
    b2Vec2 localCenter = b2Vec2_zero;
    for (b2Fixture *f = m_fixtureList; f; f = f->m_next) {
        if (f->m_density == 0.0f) {
            continue;
        }

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
        b2Assert(m_I > 0.0f);
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
    b2Assert(m_world->IsLocked() == false);
    if (m_world->IsLocked() == true) {
        return;
    }

    if (m_type != b2_dynamicBody) {
        return;
    }

    m_invMass = 0.0f;
    m_I = 0.0f;
    m_invI = 0.0f;

    m_mass = massData->mass;
    if (m_mass <= 0.0f) {
        m_mass = 1.0f;
    }

    m_invMass = 1.0f / m_mass;

    if (massData->I > 0.0f && (m_flags & b2Body::e_fixedRotationFlag) == 0) {
        m_I = massData->I - m_mass * b2Dot(massData->center, massData->center);
        b2Assert(m_I > 0.0f);
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
    if (m_type != b2_dynamicBody && other->m_type != b2_dynamicBody) {
        return false;
    }

    // Does a joint prevent collision?
    for (b2JointEdge *jn = m_jointList; jn; jn = jn->next) {
        if (jn->other == other) {
            if (jn->joint->m_collideConnected == false) {
                return false;
            }
        }
    }

    return true;
}

void b2Body::SetTransform(const b2Vec2 &position, float angle) {
    b2Assert(m_world->IsLocked() == false);
    if (m_world->IsLocked() == true) {
        return;
    }

    m_xf.q.Set(angle);
    m_xf.p = position;

    m_sweep.c = b2Mul(m_xf, m_sweep.localCenter);
    m_sweep.a = angle;

    m_sweep.c0 = m_sweep.c;
    m_sweep.a0 = angle;

    b2BroadPhase *broadPhase = &m_world->m_contactManager.m_broadPhase;
    for (b2Fixture *f = m_fixtureList; f; f = f->m_next) {
        f->Synchronize(broadPhase, m_xf, m_xf);
    }

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
    b2Assert(m_world->IsLocked() == false);

    if (flag == IsEnabled()) {
        return;
    }

    if (flag) {
        m_flags |= e_enabledFlag;

        // Create all proxies.
        b2BroadPhase *broadPhase = &m_world->m_contactManager.m_broadPhase;
        for (b2Fixture *f = m_fixtureList; f; f = f->m_next) {
            f->CreateProxies(broadPhase, m_xf);
        }

        // Contacts are created at the beginning of the next
        m_world->m_newContacts = true;
    } else {
        m_flags &= ~e_enabledFlag;

        // Destroy all proxies.
        b2BroadPhase *broadPhase = &m_world->m_contactManager.m_broadPhase;
        for (b2Fixture *f = m_fixtureList; f; f = f->m_next) {
            f->DestroyProxies(broadPhase);
        }

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
    if (status == flag) {
        return;
    }

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

void b2World::SetDebugDraw(b2Draw *debugDraw) {
    m_debugDraw = debugDraw;
}

b2Body *b2World::CreateBody(const b2BodyDef *def) {
    b2Assert(IsLocked() == false);
    if (IsLocked()) {
        return nullptr;
    }

    void *mem = m_blockAllocator.Allocate(sizeof(b2Body));
    b2Body *b = new (mem) b2Body(def, this);

    // Add to world doubly linked list.
    b->m_prev = nullptr;
    b->m_next = m_bodyList;
    if (m_bodyList) {
        m_bodyList->m_prev = b;
    }
    m_bodyList = b;
    ++m_bodyCount;

    return b;
}

void b2World::DestroyBody(b2Body *b) {
    b2Assert(m_bodyCount > 0);
    b2Assert(IsLocked() == false);
    if (IsLocked()) {
        return;
    }

    // Delete the attached joints.
    b2JointEdge *je = b->m_jointList;
    while (je) {
        b2JointEdge *je0 = je;
        je = je->next;

        if (m_destructionListener) {
            m_destructionListener->SayGoodbye(je0->joint);
        }

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

        if (m_destructionListener) {
            m_destructionListener->SayGoodbye(f0);
        }

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
    if (b->m_prev) {
        b->m_prev->m_next = b->m_next;
    }

    if (b->m_next) {
        b->m_next->m_prev = b->m_prev;
    }

    if (b == m_bodyList) {
        m_bodyList = b->m_next;
    }

    --m_bodyCount;
    b->~b2Body();
    m_blockAllocator.Free(b, sizeof(b2Body));
}

b2Joint *b2World::CreateJoint(const b2JointDef *def) {
    b2Assert(IsLocked() == false);
    if (IsLocked()) {
        return nullptr;
    }

    b2Joint *j = b2Joint::Create(def, &m_blockAllocator);

    // Connect to the world list.
    j->m_prev = nullptr;
    j->m_next = m_jointList;
    if (m_jointList) {
        m_jointList->m_prev = j;
    }
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
    b2Assert(IsLocked() == false);
    if (IsLocked()) {
        return;
    }

    bool collideConnected = j->m_collideConnected;

    // Remove from the doubly linked list.
    if (j->m_prev) {
        j->m_prev->m_next = j->m_next;
    }

    if (j->m_next) {
        j->m_next->m_prev = j->m_prev;
    }

    if (j == m_jointList) {
        m_jointList = j->m_next;
    }

    // Disconnect from island graph.
    b2Body *bodyA = j->m_bodyA;
    b2Body *bodyB = j->m_bodyB;

    // Wake up connected bodies.
    bodyA->SetAwake(true);
    bodyB->SetAwake(true);

    // Remove from body 1.
    if (j->m_edgeA.prev) {
        j->m_edgeA.prev->next = j->m_edgeA.next;
    }

    if (j->m_edgeA.next) {
        j->m_edgeA.next->prev = j->m_edgeA.prev;
    }

    if (&j->m_edgeA == bodyA->m_jointList) {
        bodyA->m_jointList = j->m_edgeA.next;
    }

    j->m_edgeA.prev = nullptr;
    j->m_edgeA.next = nullptr;

    // Remove from body 2
    if (j->m_edgeB.prev) {
        j->m_edgeB.prev->next = j->m_edgeB.next;
    }

    if (j->m_edgeB.next) {
        j->m_edgeB.next->prev = j->m_edgeB.prev;
    }

    if (&j->m_edgeB == bodyB->m_jointList) {
        bodyB->m_jointList = j->m_edgeB.next;
    }

    j->m_edgeB.prev = nullptr;
    j->m_edgeB.next = nullptr;

    b2Joint::Destroy(j, &m_blockAllocator);

    b2Assert(m_jointCount > 0);
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
    if (flag == m_allowSleep) {
        return;
    }

    m_allowSleep = flag;
    if (m_allowSleep == false) {
        for (b2Body *b = m_bodyList; b; b = b->m_next) {
            b->SetAwake(true);
        }
    }
}

// Find islands, integrate and solve constraints, solve position constraints
void b2World::Solve(const b2TimeStep &step) {
    m_profile.solveInit = 0.0f;
    m_profile.solveVelocity = 0.0f;
    m_profile.solvePosition = 0.0f;

    // Size the island for the worst case.
    b2Island island(m_bodyCount,
                    m_contactManager.m_contactCount,
                    m_jointCount,
                    &m_stackAllocator,
                    m_contactManager.m_contactListener);

    // Clear all the island flags.
    for (b2Body *b = m_bodyList; b; b = b->m_next) {
        b->m_flags &= ~b2Body::e_islandFlag;
    }
    for (b2Contact *c = m_contactManager.m_contactList; c; c = c->m_next) {
        c->m_flags &= ~b2Contact::e_islandFlag;
    }
    for (b2Joint *j = m_jointList; j; j = j->m_next) {
        j->m_islandFlag = false;
    }

    // Build and simulate all awake islands.
    int32 stackSize = m_bodyCount;
    b2Body **stack = (b2Body **) m_stackAllocator.Allocate(stackSize * sizeof(b2Body *));
    for (b2Body *seed = m_bodyList; seed; seed = seed->m_next) {
        if (seed->m_flags & b2Body::e_islandFlag) {
            continue;
        }

        if (seed->IsAwake() == false || seed->IsEnabled() == false) {
            continue;
        }

        // The seed can be dynamic or kinematic.
        if (seed->GetType() == b2_staticBody) {
            continue;
        }

        // Reset island and stack.
        island.Clear();
        int32 stackCount = 0;
        stack[stackCount++] = seed;
        seed->m_flags |= b2Body::e_islandFlag;

        // Perform a depth first search (DFS) on the constraint graph.
        while (stackCount > 0) {
            // Grab the next body off the stack and add it to the island.
            b2Body *b = stack[--stackCount];
            b2Assert(b->IsEnabled() == true);
            island.Add(b);

            // To keep islands as small as possible, we don't
            // propagate islands across static bodies.
            if (b->GetType() == b2_staticBody) {
                continue;
            }

            // Make sure the body is awake (without resetting sleep timer).
            b->m_flags |= b2Body::e_awakeFlag;

            // Search all contacts connected to this body.
            for (b2ContactEdge *ce = b->m_contactList; ce; ce = ce->next) {
                b2Contact *contact = ce->contact;

                // Has this contact already been added to an island?
                if (contact->m_flags & b2Contact::e_islandFlag) {
                    continue;
                }

                // Is this contact solid and touching?
                if (contact->IsEnabled() == false ||
                    contact->IsTouching() == false) {
                    continue;
                }

                // Skip sensors.
                bool sensorA = contact->m_fixtureA->m_isSensor;
                bool sensorB = contact->m_fixtureB->m_isSensor;
                if (sensorA || sensorB) {
                    continue;
                }

                island.Add(contact);
                contact->m_flags |= b2Contact::e_islandFlag;

                b2Body *other = ce->other;

                // Was the other body already added to this island?
                if (other->m_flags & b2Body::e_islandFlag) {
                    continue;
                }

                b2Assert(stackCount < stackSize);
                stack[stackCount++] = other;
                other->m_flags |= b2Body::e_islandFlag;
            }

            // Search all joints connect to this body.
            for (b2JointEdge *je = b->m_jointList; je; je = je->next) {
                if (je->joint->m_islandFlag == true) {
                    continue;
                }

                b2Body *other = je->other;

                // Don't simulate joints connected to disabled bodies.
                if (other->IsEnabled() == false) {
                    continue;
                }

                island.Add(je->joint);
                je->joint->m_islandFlag = true;

                if (other->m_flags & b2Body::e_islandFlag) {
                    continue;
                }

                b2Assert(stackCount < stackSize);
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
            if (b->GetType() == b2_staticBody) {
                b->m_flags &= ~b2Body::e_islandFlag;
            }
        }
    }

    m_stackAllocator.Free(stack);

    {
        b2Timer timer;
        // Synchronize fixtures, check for out of range bodies.
        for (b2Body *b = m_bodyList; b; b = b->GetNext()) {
            // If a body was not in an island then it did not move.
            if ((b->m_flags & b2Body::e_islandFlag) == 0) {
                continue;
            }

            if (b->GetType() == b2_staticBody) {
                continue;
            }

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
    b2Island island(2 * b2_maxTOIContacts, b2_maxTOIContacts, 0, &m_stackAllocator, m_contactManager.m_contactListener);

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
            if (c->IsEnabled() == false) {
                continue;
            }

            // Prevent excessive sub-stepping.
            if (c->m_toiCount > b2_maxSubSteps) {
                continue;
            }

            float alpha = 1.0f;
            if (c->m_flags & b2Contact::e_toiFlag) {
                // This contact has a valid cached TOI.
                alpha = c->m_toi;
            } else {
                b2Fixture *fA = c->GetFixtureA();
                b2Fixture *fB = c->GetFixtureB();

                // Is there a sensor?
                if (fA->IsSensor() || fB->IsSensor()) {
                    continue;
                }

                b2Body *bA = fA->GetBody();
                b2Body *bB = fB->GetBody();

                b2BodyType typeA = bA->m_type;
                b2BodyType typeB = bB->m_type;
                b2Assert(typeA == b2_dynamicBody || typeB == b2_dynamicBody);

                bool activeA = bA->IsAwake() && typeA != b2_staticBody;
                bool activeB = bB->IsAwake() && typeB != b2_staticBody;

                // Is at least one body active (awake and dynamic or kinematic)?
                if (activeA == false && activeB == false) {
                    continue;
                }

                bool collideA = bA->IsBullet() || typeA != b2_dynamicBody;
                bool collideB = bB->IsBullet() || typeB != b2_dynamicBody;

                // Are these two non-bullet dynamic bodies?
                if (collideA == false && collideB == false) {
                    continue;
                }

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

                b2Assert(alpha0 < 1.0f);

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
                    if (island.m_bodyCount == island.m_bodyCapacity) {
                        break;
                    }

                    if (island.m_contactCount == island.m_contactCapacity) {
                        break;
                    }

                    b2Contact *contact = ce->contact;

                    // Has this contact already been added to the island?
                    if (contact->m_flags & b2Contact::e_islandFlag) {
                        continue;
                    }

                    // Only add static, kinematic, or bullet bodies.
                    b2Body *other = ce->other;
                    if (other->m_type == b2_dynamicBody &&
                        body->IsBullet() == false && other->IsBullet() == false) {
                        continue;
                    }

                    // Skip sensors.
                    bool sensorA = contact->m_fixtureA->m_isSensor;
                    bool sensorB = contact->m_fixtureB->m_isSensor;
                    if (sensorA || sensorB) {
                        continue;
                    }

                    // Tentatively advance the body to the TOI.
                    b2Sweep backup = other->m_sweep;
                    if ((other->m_flags & b2Body::e_islandFlag) == 0) {
                        other->Advance(minAlpha);
                    }

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
                    if (other->m_flags & b2Body::e_islandFlag) {
                        continue;
                    }

                    // Add the other body to the island.
                    other->m_flags |= b2Body::e_islandFlag;

                    if (other->m_type != b2_staticBody) {
                        other->SetAwake(true);
                    }

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

            if (body->m_type != b2_dynamicBody) {
                continue;
            }

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

    if (step.dt > 0.0f) {
        m_inv_dt0 = step.inv_dt;
    }

    if (m_clearForces) {
        ClearForces();
    }

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

void b2World::RayCast(b2RayCastCallback *callback, const b2Vec2 &point1, const b2Vec2 &point2) const {
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
            b2Assert(vertexCount <= b2_maxPolygonVertices);
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
    if (m_debugDraw == nullptr) {
        return;
    }

    uint32 flags = m_debugDraw->GetFlags();

    if (flags & b2Draw::e_shapeBit) {
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

    if (flags & b2Draw::e_jointBit) {
        for (b2Joint *j = m_jointList; j; j = j->GetNext()) {
            j->Draw(m_debugDraw);
        }
    }

    if (flags & b2Draw::e_pairBit) {
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

    if (flags & b2Draw::e_aabbBit) {
        b2Color color(0.9f, 0.3f, 0.9f);
        b2BroadPhase *bp = &m_contactManager.m_broadPhase;

        for (b2Body *b = m_bodyList; b; b = b->GetNext()) {
            if (b->IsEnabled() == false) {
                continue;
            }

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

    if (flags & b2Draw::e_centerOfMassBit) {
        for (b2Body *b = m_bodyList; b; b = b->GetNext()) {
            b2Transform xf = b->GetTransform();
            xf.p = b->GetWorldCenter();
            m_debugDraw->DrawTransform(xf);
        }
    }
}

int32 b2World::GetProxyCount() const {
    return m_contactManager.m_broadPhase.GetProxyCount();
}

int32 b2World::GetTreeHeight() const {
    return m_contactManager.m_broadPhase.GetTreeHeight();
}

int32 b2World::GetTreeBalance() const {
    return m_contactManager.m_broadPhase.GetTreeBalance();
}

float b2World::GetTreeQuality() const {
    return m_contactManager.m_broadPhase.GetTreeQuality();
}

void b2World::ShiftOrigin(const b2Vec2 &newOrigin) {
    b2Assert(m_locked == false);
    if (m_locked) {
        return;
    }

    for (b2Body *b = m_bodyList; b; b = b->m_next) {
        b->m_xf.p -= newOrigin;
        b->m_sweep.c0 -= newOrigin;
        b->m_sweep.c -= newOrigin;
    }

    for (b2Joint *j = m_jointList; j; j = j->m_next) {
        j->ShiftOrigin(newOrigin);
    }

    m_contactManager.m_broadPhase.ShiftOrigin(newOrigin);
}

void b2World::Dump() {
    if (m_locked) {
        return;
    }

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
        if (j->m_type == e_gearJoint) {
            continue;
        }

        b2Dump("{\n");
        j->Dump();
        b2Dump("}\n");
    }

    // Second pass on joints, only gear joints.
    for (b2Joint *j = m_jointList; j; j = j->m_next) {
        if (j->m_type != e_gearJoint) {
            continue;
        }

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

    bool collide = (filterA.maskBits & filterB.categoryBits) != 0 && (filterA.categoryBits & filterB.maskBits) != 0;
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

b2WheelJoint::b2WheelJoint(const b2WheelJointDef *def)
    : b2Joint(def) {
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

        if (m_mass > 0.0f) {
            m_mass = 1.0f / m_mass;
        }
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
        if (m_gamma > 0.0f) {
            m_gamma = 1.0f / m_gamma;
        }

        m_bias = C * h * m_stiffness * m_gamma;

        m_springMass = invMass + m_gamma;
        if (m_springMass > 0.0f) {
            m_springMass = 1.0f / m_springMass;
        }
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
        if (m_motorMass > 0.0f) {
            m_motorMass = 1.0f / m_motorMass;
        }
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
            if (invMass != 0.0f) {
                impulse = -C / invMass;
            }

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
        if (invMass != 0.0f) {
            impulse = -C / invMass;
        }

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

b2Vec2 b2WheelJoint::GetAnchorA() const {
    return m_bodyA->GetWorldPoint(m_localAnchorA);
}

b2Vec2 b2WheelJoint::GetAnchorB() const {
    return m_bodyB->GetWorldPoint(m_localAnchorB);
}

b2Vec2 b2WheelJoint::GetReactionForce(float inv_dt) const {
    return inv_dt * (m_impulse * m_ay + (m_springImpulse + m_lowerImpulse - m_upperImpulse) * m_ax);
}

float b2WheelJoint::GetReactionTorque(float inv_dt) const {
    return inv_dt * m_motorImpulse;
}

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

    float speed = b2Dot(d, b2Cross(wA, axis)) + b2Dot(axis, vB + b2Cross(wB, rB) - vA - b2Cross(wA, rA));
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

bool b2WheelJoint::IsLimitEnabled() const {
    return m_enableLimit;
}

void b2WheelJoint::EnableLimit(bool flag) {
    if (flag != m_enableLimit) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_enableLimit = flag;
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }
}

float b2WheelJoint::GetLowerLimit() const {
    return m_lowerTranslation;
}

float b2WheelJoint::GetUpperLimit() const {
    return m_upperTranslation;
}

void b2WheelJoint::SetLimits(float lower, float upper) {
    b2Assert(lower <= upper);
    if (lower != m_lowerTranslation || upper != m_upperTranslation) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_lowerTranslation = lower;
        m_upperTranslation = upper;
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }
}

bool b2WheelJoint::IsMotorEnabled() const {
    return m_enableMotor;
}

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

float b2WheelJoint::GetMotorTorque(float inv_dt) const {
    return inv_dt * m_motorImpulse;
}

void b2WheelJoint::SetStiffness(float stiffness) {
    m_stiffness = stiffness;
}

float b2WheelJoint::GetStiffness() const {
    return m_stiffness;
}

void b2WheelJoint::SetDamping(float damping) {
    m_damping = damping;
}

float b2WheelJoint::GetDamping() const {
    return m_damping;
}

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
void b2WheelJoint::Draw(b2Draw *draw) const {
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

b2WeldJoint::b2WeldJoint(const b2WeldJointDef *def)
    : b2Joint(def) {
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

b2Vec2 b2WeldJoint::GetAnchorA() const {
    return m_bodyA->GetWorldPoint(m_localAnchorA);
}

b2Vec2 b2WeldJoint::GetAnchorB() const {
    return m_bodyB->GetWorldPoint(m_localAnchorB);
}

b2Vec2 b2WeldJoint::GetReactionForce(float inv_dt) const {
    b2Vec2 P(m_impulse.x, m_impulse.y);
    return inv_dt * P;
}

float b2WeldJoint::GetReactionTorque(float inv_dt) const {
    return inv_dt * m_impulse.z;
}

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

b2RevoluteJoint::b2RevoluteJoint(const b2RevoluteJointDef *def)
    : b2Joint(def) {
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

    if (m_enableMotor == false || fixedRotation) {
        m_motorImpulse = 0.0f;
    }

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

b2Vec2 b2RevoluteJoint::GetAnchorA() const {
    return m_bodyA->GetWorldPoint(m_localAnchorA);
}

b2Vec2 b2RevoluteJoint::GetAnchorB() const {
    return m_bodyB->GetWorldPoint(m_localAnchorB);
}

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

bool b2RevoluteJoint::IsMotorEnabled() const {
    return m_enableMotor;
}

void b2RevoluteJoint::EnableMotor(bool flag) {
    if (flag != m_enableMotor) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_enableMotor = flag;
    }
}

float b2RevoluteJoint::GetMotorTorque(float inv_dt) const {
    return inv_dt * m_motorImpulse;
}

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

bool b2RevoluteJoint::IsLimitEnabled() const {
    return m_enableLimit;
}

void b2RevoluteJoint::EnableLimit(bool flag) {
    if (flag != m_enableLimit) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_enableLimit = flag;
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }
}

float b2RevoluteJoint::GetLowerLimit() const {
    return m_lowerAngle;
}

float b2RevoluteJoint::GetUpperLimit() const {
    return m_upperAngle;
}

void b2RevoluteJoint::SetLimits(float lower, float upper) {
    b2Assert(lower <= upper);

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
void b2RevoluteJoint::Draw(b2Draw *draw) const {
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

void b2PulleyJointDef::Initialize(b2Body *bA, b2Body *bB,
                                  const b2Vec2 &groundA, const b2Vec2 &groundB,
                                  const b2Vec2 &anchorA, const b2Vec2 &anchorB,
                                  float r) {
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
    b2Assert(ratio > b2_epsilon);
}

b2PulleyJoint::b2PulleyJoint(const b2PulleyJointDef *def)
    : b2Joint(def) {
    m_groundAnchorA = def->groundAnchorA;
    m_groundAnchorB = def->groundAnchorB;
    m_localAnchorA = def->localAnchorA;
    m_localAnchorB = def->localAnchorB;

    m_lengthA = def->lengthA;
    m_lengthB = def->lengthB;

    b2Assert(def->ratio != 0.0f);
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

    if (m_mass > 0.0f) {
        m_mass = 1.0f / m_mass;
    }

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

    if (mass > 0.0f) {
        mass = 1.0f / mass;
    }

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

b2Vec2 b2PulleyJoint::GetAnchorA() const {
    return m_bodyA->GetWorldPoint(m_localAnchorA);
}

b2Vec2 b2PulleyJoint::GetAnchorB() const {
    return m_bodyB->GetWorldPoint(m_localAnchorB);
}

b2Vec2 b2PulleyJoint::GetReactionForce(float inv_dt) const {
    b2Vec2 P = m_impulse * m_uB;
    return inv_dt * P;
}

float b2PulleyJoint::GetReactionTorque(float inv_dt) const {
    B2_NOT_USED(inv_dt);
    return 0.0f;
}

b2Vec2 b2PulleyJoint::GetGroundAnchorA() const {
    return m_groundAnchorA;
}

b2Vec2 b2PulleyJoint::GetGroundAnchorB() const {
    return m_groundAnchorB;
}

float b2PulleyJoint::GetLengthA() const {
    return m_lengthA;
}

float b2PulleyJoint::GetLengthB() const {
    return m_lengthB;
}

float b2PulleyJoint::GetRatio() const {
    return m_ratio;
}

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

void b2PrismaticJointDef::Initialize(b2Body *bA, b2Body *bB, const b2Vec2 &anchor, const b2Vec2 &axis) {
    bodyA = bA;
    bodyB = bB;
    localAnchorA = bodyA->GetLocalPoint(anchor);
    localAnchorB = bodyB->GetLocalPoint(anchor);
    localAxisA = bodyA->GetLocalVector(axis);
    referenceAngle = bodyB->GetAngle() - bodyA->GetAngle();
}

b2PrismaticJoint::b2PrismaticJoint(const b2PrismaticJointDef *def)
    : b2Joint(def) {
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

    b2Assert(m_lowerTranslation <= m_upperTranslation);

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
        if (m_axialMass > 0.0f) {
            m_axialMass = 1.0f / m_axialMass;
        }
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

    if (m_enableMotor == false) {
        m_motorImpulse = 0.0f;
    }

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
        if (k22 == 0.0f) {
            k22 = 1.0f;
        }

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

b2Vec2 b2PrismaticJoint::GetAnchorA() const {
    return m_bodyA->GetWorldPoint(m_localAnchorA);
}

b2Vec2 b2PrismaticJoint::GetAnchorB() const {
    return m_bodyB->GetWorldPoint(m_localAnchorB);
}

b2Vec2 b2PrismaticJoint::GetReactionForce(float inv_dt) const {
    return inv_dt * (m_impulse.x * m_perp + (m_motorImpulse + m_lowerImpulse - m_upperImpulse) * m_axis);
}

float b2PrismaticJoint::GetReactionTorque(float inv_dt) const {
    return inv_dt * m_impulse.y;
}

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

    float speed = b2Dot(d, b2Cross(wA, axis)) + b2Dot(axis, vB + b2Cross(wB, rB) - vA - b2Cross(wA, rA));
    return speed;
}

bool b2PrismaticJoint::IsLimitEnabled() const {
    return m_enableLimit;
}

void b2PrismaticJoint::EnableLimit(bool flag) {
    if (flag != m_enableLimit) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_enableLimit = flag;
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }
}

float b2PrismaticJoint::GetLowerLimit() const {
    return m_lowerTranslation;
}

float b2PrismaticJoint::GetUpperLimit() const {
    return m_upperTranslation;
}

void b2PrismaticJoint::SetLimits(float lower, float upper) {
    b2Assert(lower <= upper);
    if (lower != m_lowerTranslation || upper != m_upperTranslation) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_lowerTranslation = lower;
        m_upperTranslation = upper;
        m_lowerImpulse = 0.0f;
        m_upperImpulse = 0.0f;
    }
}

bool b2PrismaticJoint::IsMotorEnabled() const {
    return m_enableMotor;
}

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

float b2PrismaticJoint::GetMotorForce(float inv_dt) const {
    return inv_dt * m_motorImpulse;
}

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

void b2PrismaticJoint::Draw(b2Draw *draw) const {
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


b2Contact *b2PolygonContact::Create(b2Fixture *fixtureA, int32, b2Fixture *fixtureB, int32, b2BlockAllocator *allocator) {
    void *mem = allocator->Allocate(sizeof(b2PolygonContact));
    return new (mem) b2PolygonContact(fixtureA, fixtureB);
}

void b2PolygonContact::Destroy(b2Contact *contact, b2BlockAllocator *allocator) {
    ((b2PolygonContact *) contact)->~b2PolygonContact();
    allocator->Free(contact, sizeof(b2PolygonContact));
}

b2PolygonContact::b2PolygonContact(b2Fixture *fixtureA, b2Fixture *fixtureB)
    : b2Contact(fixtureA, 0, fixtureB, 0) {
    b2Assert(m_fixtureA->GetType() == b2Shape::e_polygon);
    b2Assert(m_fixtureB->GetType() == b2Shape::e_polygon);
}

void b2PolygonContact::Evaluate(b2Manifold *manifold, const b2Transform &xfA, const b2Transform &xfB) {
    b2CollidePolygons(manifold,
                      (b2PolygonShape *) m_fixtureA->GetShape(), xfA,
                      (b2PolygonShape *) m_fixtureB->GetShape(), xfB);
}


b2Contact *b2PolygonAndCircleContact::Create(b2Fixture *fixtureA, int32, b2Fixture *fixtureB, int32, b2BlockAllocator *allocator) {
    void *mem = allocator->Allocate(sizeof(b2PolygonAndCircleContact));
    return new (mem) b2PolygonAndCircleContact(fixtureA, fixtureB);
}

void b2PolygonAndCircleContact::Destroy(b2Contact *contact, b2BlockAllocator *allocator) {
    ((b2PolygonAndCircleContact *) contact)->~b2PolygonAndCircleContact();
    allocator->Free(contact, sizeof(b2PolygonAndCircleContact));
}

b2PolygonAndCircleContact::b2PolygonAndCircleContact(b2Fixture *fixtureA, b2Fixture *fixtureB)
    : b2Contact(fixtureA, 0, fixtureB, 0) {
    b2Assert(m_fixtureA->GetType() == b2Shape::e_polygon);
    b2Assert(m_fixtureB->GetType() == b2Shape::e_circle);
}

void b2PolygonAndCircleContact::Evaluate(b2Manifold *manifold, const b2Transform &xfA, const b2Transform &xfB) {
    b2CollidePolygonAndCircle(manifold,
                              (b2PolygonShape *) m_fixtureA->GetShape(), xfA,
                              (b2CircleShape *) m_fixtureB->GetShape(), xfB);
}


// p = attached point, m = mouse point
// C = p - m
// Cdot = v
//      = v + cross(w, r)
// J = [I r_skew]
// Identity used:
// w k % (rx i + ry j) = w * (-ry i + rx j)

b2MouseJoint::b2MouseJoint(const b2MouseJointDef *def)
    : b2Joint(def) {
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

const b2Vec2 &b2MouseJoint::GetTarget() const {
    return m_targetA;
}

void b2MouseJoint::SetMaxForce(float force) {
    m_maxForce = force;
}

float b2MouseJoint::GetMaxForce() const {
    return m_maxForce;
}

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
    if (m_gamma != 0.0f) {
        m_gamma = 1.0f / m_gamma;
    }
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

b2Vec2 b2MouseJoint::GetAnchorA() const {
    return m_targetA;
}

b2Vec2 b2MouseJoint::GetAnchorB() const {
    return m_bodyB->GetWorldPoint(m_localAnchorB);
}

b2Vec2 b2MouseJoint::GetReactionForce(float inv_dt) const {
    return inv_dt * m_impulse;
}

float b2MouseJoint::GetReactionTorque(float inv_dt) const {
    return inv_dt * 0.0f;
}

void b2MouseJoint::ShiftOrigin(const b2Vec2 &newOrigin) {
    m_targetA -= newOrigin;
}


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

b2MotorJoint::b2MotorJoint(const b2MotorJointDef *def)
    : b2Joint(def) {
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
    if (m_angularMass > 0.0f) {
        m_angularMass = 1.0f / m_angularMass;
    }

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
        b2Vec2 Cdot = vB + b2Cross(wB, m_rB) - vA - b2Cross(wA, m_rA) + inv_h * m_correctionFactor * m_linearError;

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

b2Vec2 b2MotorJoint::GetAnchorA() const {
    return m_bodyA->GetPosition();
}

b2Vec2 b2MotorJoint::GetAnchorB() const {
    return m_bodyB->GetPosition();
}

b2Vec2 b2MotorJoint::GetReactionForce(float inv_dt) const {
    return inv_dt * m_linearImpulse;
}

float b2MotorJoint::GetReactionTorque(float inv_dt) const {
    return inv_dt * m_angularImpulse;
}

void b2MotorJoint::SetMaxForce(float force) {
    b2Assert(b2IsValid(force) && force >= 0.0f);
    m_maxForce = force;
}

float b2MotorJoint::GetMaxForce() const {
    return m_maxForce;
}

void b2MotorJoint::SetMaxTorque(float torque) {
    b2Assert(b2IsValid(torque) && torque >= 0.0f);
    m_maxTorque = torque;
}

float b2MotorJoint::GetMaxTorque() const {
    return m_maxTorque;
}

void b2MotorJoint::SetCorrectionFactor(float factor) {
    b2Assert(b2IsValid(factor) && 0.0f <= factor && factor <= 1.0f);
    m_correctionFactor = factor;
}

float b2MotorJoint::GetCorrectionFactor() const {
    return m_correctionFactor;
}

void b2MotorJoint::SetLinearOffset(const b2Vec2 &linearOffset) {
    if (linearOffset.x != m_linearOffset.x || linearOffset.y != m_linearOffset.y) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_linearOffset = linearOffset;
    }
}

const b2Vec2 &b2MotorJoint::GetLinearOffset() const {
    return m_linearOffset;
}

void b2MotorJoint::SetAngularOffset(float angularOffset) {
    if (angularOffset != m_angularOffset) {
        m_bodyA->SetAwake(true);
        m_bodyB->SetAwake(true);
        m_angularOffset = angularOffset;
    }
}

float b2MotorJoint::GetAngularOffset() const {
    return m_angularOffset;
}

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


void b2LinearStiffness(float &stiffness, float &damping,
                       float frequencyHertz, float dampingRatio,
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

void b2AngularStiffness(float &stiffness, float &damping,
                        float frequencyHertz, float dampingRatio,
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
            b2Assert(false);
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
            b2Assert(false);
            break;
    }
}

b2Joint::b2Joint(const b2JointDef *def) {
    b2Assert(def->bodyA != def->bodyB);

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

bool b2Joint::IsEnabled() const {
    return m_bodyA->IsEnabled() && m_bodyB->IsEnabled();
}

void b2Joint::Draw(b2Draw *draw) const {
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

b2Island::b2Island(
        int32 bodyCapacity,
        int32 contactCapacity,
        int32 jointCapacity,
        b2StackAllocator *allocator,
        b2ContactListener *listener) {
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

void b2Island::Solve(b2Profile *profile, const b2TimeStep &step, const b2Vec2 &gravity, bool allowSleep) {
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

    if (step.warmStarting) {
        contactSolver.WarmStart();
    }

    for (int32 i = 0; i < m_jointCount; ++i) {
        m_joints[i]->InitVelocityConstraints(solverData);
    }

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
            if (b->GetType() == b2_staticBody) {
                continue;
            }

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
    b2Assert(toiIndexA < m_bodyCount);
    b2Assert(toiIndexB < m_bodyCount);

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
        if (contactsOkay) {
            break;
        }
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
    if (m_listener == nullptr) {
        return;
    }

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

b2GearJoint::b2GearJoint(const b2GearJointDef *def)
    : b2Joint(def) {
    m_joint1 = def->joint1;
    m_joint2 = def->joint2;

    m_typeA = m_joint1->GetType();
    m_typeB = m_joint2->GetType();

    b2Assert(m_typeA == e_revoluteJoint || m_typeA == e_prismaticJoint);
    b2Assert(m_typeB == e_revoluteJoint || m_typeB == e_prismaticJoint);

    float coordinateA, coordinateB;

    // TODO_ERIN there might be some problem with the joint edges in b2Joint.

    m_bodyC = m_joint1->GetBodyA();
    m_bodyA = m_joint1->GetBodyB();

    // Body B on joint1 must be dynamic
    b2Assert(m_bodyA->m_type == b2_dynamicBody);

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
    b2Assert(m_bodyB->m_type == b2_dynamicBody);

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
    if (mass > 0.0f) {
        impulse = -C / mass;
    }

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

    if (b2Abs(C) < m_tolerance) {
        return true;
    }

    return false;
}

b2Vec2 b2GearJoint::GetAnchorA() const {
    return m_bodyA->GetWorldPoint(m_localAnchorA);
}

b2Vec2 b2GearJoint::GetAnchorB() const {
    return m_bodyB->GetWorldPoint(m_localAnchorB);
}

b2Vec2 b2GearJoint::GetReactionForce(float inv_dt) const {
    b2Vec2 P = m_impulse * m_JvAC;
    return inv_dt * P;
}

float b2GearJoint::GetReactionTorque(float inv_dt) const {
    float L = m_impulse * m_JwA;
    return inv_dt * L;
}

void b2GearJoint::SetRatio(float ratio) {
    b2Assert(b2IsValid(ratio));
    m_ratio = ratio;
}

float b2GearJoint::GetRatio() const {
    return m_ratio;
}

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

b2FrictionJoint::b2FrictionJoint(const b2FrictionJointDef *def)
    : b2Joint(def) {
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
    if (m_angularMass > 0.0f) {
        m_angularMass = 1.0f / m_angularMass;
    }

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

b2Vec2 b2FrictionJoint::GetAnchorA() const {
    return m_bodyA->GetWorldPoint(m_localAnchorA);
}

b2Vec2 b2FrictionJoint::GetAnchorB() const {
    return m_bodyB->GetWorldPoint(m_localAnchorB);
}

b2Vec2 b2FrictionJoint::GetReactionForce(float inv_dt) const {
    return inv_dt * m_linearImpulse;
}

float b2FrictionJoint::GetReactionTorque(float inv_dt) const {
    return inv_dt * m_angularImpulse;
}

void b2FrictionJoint::SetMaxForce(float force) {
    b2Assert(b2IsValid(force) && force >= 0.0f);
    m_maxForce = force;
}

float b2FrictionJoint::GetMaxForce() const {
    return m_maxForce;
}

void b2FrictionJoint::SetMaxTorque(float torque) {
    b2Assert(b2IsValid(torque) && torque >= 0.0f);
    m_maxTorque = torque;
}

float b2FrictionJoint::GetMaxTorque() const {
    return m_maxTorque;
}

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
    b2Assert(m_proxyCount == 0);

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
            b2Assert(false);
            break;
    }

    m_shape = nullptr;
}

void b2Fixture::CreateProxies(b2BroadPhase *broadPhase, const b2Transform &xf) {
    b2Assert(m_proxyCount == 0);

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

void b2Fixture::Synchronize(b2BroadPhase *broadPhase, const b2Transform &transform1, const b2Transform &transform2) {
    if (m_proxyCount == 0) {
        return;
    }

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
    if (m_body == nullptr) {
        return;
    }

    // Flag associated contacts for filtering.
    b2ContactEdge *edge = m_body->GetContactList();
    while (edge) {
        b2Contact *contact = edge->contact;
        b2Fixture *fixtureA = contact->GetFixtureA();
        b2Fixture *fixtureB = contact->GetFixtureB();
        if (fixtureA == this || fixtureB == this) {
            contact->FlagForFiltering();
        }

        edge = edge->next;
    }

    b2World *world = m_body->GetWorld();

    if (world == nullptr) {
        return;
    }

    // Touch each proxy so that new pairs may be created
    b2BroadPhase *broadPhase = &world->m_contactManager.m_broadPhase;
    for (int32 i = 0; i < m_proxyCount; ++i) {
        broadPhase->TouchProxy(m_proxies[i].proxyId);
    }
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
            b2Dump("    shape.m_prevVertex.Set(%.9g, %.9g);\n", s->m_prevVertex.x, s->m_prevVertex.y);
            b2Dump("    shape.m_nextVertex.Set(%.9g, %.9g);\n", s->m_nextVertex.x, s->m_nextVertex.y);
        } break;

        default:
            return;
    }

    b2Dump("\n");
    b2Dump("    fd.shape = &shape;\n");
    b2Dump("\n");
    b2Dump("    bodies[%d]->CreateFixture(&fd);\n", bodyIndex);
}


b2Contact *b2EdgeAndPolygonContact::Create(b2Fixture *fixtureA, int32, b2Fixture *fixtureB, int32, b2BlockAllocator *allocator) {
    void *mem = allocator->Allocate(sizeof(b2EdgeAndPolygonContact));
    return new (mem) b2EdgeAndPolygonContact(fixtureA, fixtureB);
}

void b2EdgeAndPolygonContact::Destroy(b2Contact *contact, b2BlockAllocator *allocator) {
    ((b2EdgeAndPolygonContact *) contact)->~b2EdgeAndPolygonContact();
    allocator->Free(contact, sizeof(b2EdgeAndPolygonContact));
}

b2EdgeAndPolygonContact::b2EdgeAndPolygonContact(b2Fixture *fixtureA, b2Fixture *fixtureB)
    : b2Contact(fixtureA, 0, fixtureB, 0) {
    b2Assert(m_fixtureA->GetType() == b2Shape::e_edge);
    b2Assert(m_fixtureB->GetType() == b2Shape::e_polygon);
}

void b2EdgeAndPolygonContact::Evaluate(b2Manifold *manifold, const b2Transform &xfA, const b2Transform &xfB) {
    b2CollideEdgeAndPolygon(manifold,
                            (b2EdgeShape *) m_fixtureA->GetShape(), xfA,
                            (b2PolygonShape *) m_fixtureB->GetShape(), xfB);
}


b2Contact *b2EdgeAndCircleContact::Create(b2Fixture *fixtureA, int32, b2Fixture *fixtureB, int32, b2BlockAllocator *allocator) {
    void *mem = allocator->Allocate(sizeof(b2EdgeAndCircleContact));
    return new (mem) b2EdgeAndCircleContact(fixtureA, fixtureB);
}

void b2EdgeAndCircleContact::Destroy(b2Contact *contact, b2BlockAllocator *allocator) {
    ((b2EdgeAndCircleContact *) contact)->~b2EdgeAndCircleContact();
    allocator->Free(contact, sizeof(b2EdgeAndCircleContact));
}

b2EdgeAndCircleContact::b2EdgeAndCircleContact(b2Fixture *fixtureA, b2Fixture *fixtureB)
    : b2Contact(fixtureA, 0, fixtureB, 0) {
    b2Assert(m_fixtureA->GetType() == b2Shape::e_edge);
    b2Assert(m_fixtureB->GetType() == b2Shape::e_circle);
}

void b2EdgeAndCircleContact::Evaluate(b2Manifold *manifold, const b2Transform &xfA, const b2Transform &xfB) {
    b2CollideEdgeAndCircle(manifold,
                           (b2EdgeShape *) m_fixtureA->GetShape(), xfA,
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


void b2DistanceJointDef::Initialize(b2Body *b1, b2Body *b2,
                                    const b2Vec2 &anchor1, const b2Vec2 &anchor2) {
    bodyA = b1;
    bodyB = b2;
    localAnchorA = bodyA->GetLocalPoint(anchor1);
    localAnchorB = bodyB->GetLocalPoint(anchor2);
    b2Vec2 d = anchor2 - anchor1;
    length = b2Max(d.Length(), b2_linearSlop);
    minLength = length;
    maxLength = length;
}

b2DistanceJoint::b2DistanceJoint(const b2DistanceJointDef *def)
    : b2Joint(def) {
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

b2Vec2 b2DistanceJoint::GetAnchorA() const {
    return m_bodyA->GetWorldPoint(m_localAnchorA);
}

b2Vec2 b2DistanceJoint::GetAnchorB() const {
    return m_bodyB->GetWorldPoint(m_localAnchorB);
}

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

void b2DistanceJoint::Draw(b2Draw *draw) const {
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
    AddType(b2CircleContact::Create, b2CircleContact::Destroy, b2Shape::e_circle, b2Shape::e_circle);
    AddType(b2PolygonAndCircleContact::Create, b2PolygonAndCircleContact::Destroy, b2Shape::e_polygon, b2Shape::e_circle);
    AddType(b2PolygonContact::Create, b2PolygonContact::Destroy, b2Shape::e_polygon, b2Shape::e_polygon);
    AddType(b2EdgeAndCircleContact::Create, b2EdgeAndCircleContact::Destroy, b2Shape::e_edge, b2Shape::e_circle);
    AddType(b2EdgeAndPolygonContact::Create, b2EdgeAndPolygonContact::Destroy, b2Shape::e_edge, b2Shape::e_polygon);
    AddType(b2ChainAndCircleContact::Create, b2ChainAndCircleContact::Destroy, b2Shape::e_chain, b2Shape::e_circle);
    AddType(b2ChainAndPolygonContact::Create, b2ChainAndPolygonContact::Destroy, b2Shape::e_chain, b2Shape::e_polygon);
}

void b2Contact::AddType(b2ContactCreateFcn *createFcn, b2ContactDestroyFcn *destoryFcn,
                        b2Shape::Type type1, b2Shape::Type type2) {
    b2Assert(0 <= type1 && type1 < b2Shape::e_typeCount);
    b2Assert(0 <= type2 && type2 < b2Shape::e_typeCount);

    s_registers[type1][type2].createFcn = createFcn;
    s_registers[type1][type2].destroyFcn = destoryFcn;
    s_registers[type1][type2].primary = true;

    if (type1 != type2) {
        s_registers[type2][type1].createFcn = createFcn;
        s_registers[type2][type1].destroyFcn = destoryFcn;
        s_registers[type2][type1].primary = false;
    }
}

b2Contact *b2Contact::Create(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB, b2BlockAllocator *allocator) {
    if (s_initialized == false) {
        InitializeRegisters();
        s_initialized = true;
    }

    b2Shape::Type type1 = fixtureA->GetType();
    b2Shape::Type type2 = fixtureB->GetType();

    b2Assert(0 <= type1 && type1 < b2Shape::e_typeCount);
    b2Assert(0 <= type2 && type2 < b2Shape::e_typeCount);

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
    b2Assert(s_initialized == true);

    b2Fixture *fixtureA = contact->m_fixtureA;
    b2Fixture *fixtureB = contact->m_fixtureB;

    if (contact->m_manifold.pointCount > 0 &&
        fixtureA->IsSensor() == false &&
        fixtureB->IsSensor() == false) {
        fixtureA->GetBody()->SetAwake(true);
        fixtureB->GetBody()->SetAwake(true);
    }

    b2Shape::Type typeA = fixtureA->GetType();
    b2Shape::Type typeB = fixtureB->GetType();

    b2Assert(0 <= typeA && typeA < b2Shape::e_typeCount);
    b2Assert(0 <= typeB && typeB < b2Shape::e_typeCount);

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
    m_restitutionThreshold = b2MixRestitutionThreshold(m_fixtureA->m_restitutionThreshold, m_fixtureB->m_restitutionThreshold);

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

    if (wasTouching == false && touching == true && listener) {
        listener->BeginContact(this);
    }

    if (wasTouching == true && touching == false && listener) {
        listener->EndContact(this);
    }

    if (sensor == false && touching && listener) {
        listener->PreSolve(this, &oldManifold);
    }
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
    m_positionConstraints = (b2ContactPositionConstraint *) m_allocator->Allocate(m_count * sizeof(b2ContactPositionConstraint));
    m_velocityConstraints = (b2ContactVelocityConstraint *) m_allocator->Allocate(m_count * sizeof(b2ContactVelocityConstraint));
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
        b2Assert(pointCount > 0);

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

        b2Assert(manifold->pointCount > 0);

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
            if (vRel < -vc->threshold) {
                vcp->velocityBias = -vc->restitution * vRel;
            }
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

        b2Assert(pointCount == 1 || pointCount == 2);

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
            b2Assert(a.x >= 0.0f && a.y >= 0.0f);

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

                    b2Assert(b2Abs(vn1 - cp1->velocityBias) < k_errorTol);
                    b2Assert(b2Abs(vn2 - cp2->velocityBias) < k_errorTol);
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

                    b2Assert(b2Abs(vn1 - cp1->velocityBias) < k_errorTol);
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

                    b2Assert(b2Abs(vn2 - cp2->velocityBias) < k_errorTol);
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
    void Initialize(b2ContactPositionConstraint *pc, const b2Transform &xfA, const b2Transform &xfB, int32 index) {
        b2Assert(pc->pointCount > 0);

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
            float C = b2Clamp(b2_baumgarte * (separation + b2_linearSlop), -b2_maxLinearCorrection, 0.0f);

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
            float C = b2Clamp(b2_toiBaumgarte * (separation + b2_linearSlop), -b2_maxLinearCorrection, 0.0f);

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

    if (m_contactListener && c->IsTouching()) {
        m_contactListener->EndContact(c);
    }

    // Remove from the world.
    if (c->m_prev) {
        c->m_prev->m_next = c->m_next;
    }

    if (c->m_next) {
        c->m_next->m_prev = c->m_prev;
    }

    if (c == m_contactList) {
        m_contactList = c->m_next;
    }

    // Remove from body 1
    if (c->m_nodeA.prev) {
        c->m_nodeA.prev->next = c->m_nodeA.next;
    }

    if (c->m_nodeA.next) {
        c->m_nodeA.next->prev = c->m_nodeA.prev;
    }

    if (&c->m_nodeA == bodyA->m_contactList) {
        bodyA->m_contactList = c->m_nodeA.next;
    }

    // Remove from body 2
    if (c->m_nodeB.prev) {
        c->m_nodeB.prev->next = c->m_nodeB.next;
    }

    if (c->m_nodeB.next) {
        c->m_nodeB.next->prev = c->m_nodeB.prev;
    }

    if (&c->m_nodeB == bodyB->m_contactList) {
        bodyB->m_contactList = c->m_nodeB.next;
    }

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

void b2ContactManager::FindNewContacts() {
    m_broadPhase.UpdatePairs(this);
}

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
    if (bodyA == bodyB) {
        return;
    }

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
    if (bodyB->ShouldCollide(bodyA) == false) {
        return;
    }

    // Check user filtering.
    if (m_contactFilter && m_contactFilter->ShouldCollide(fixtureA, fixtureB) == false) {
        return;
    }

    // Call the factory.
    b2Contact *c = b2Contact::Create(fixtureA, indexA, fixtureB, indexB, m_allocator);
    if (c == nullptr) {
        return;
    }

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
    if (m_contactList != nullptr) {
        m_contactList->m_prev = c;
    }
    m_contactList = c;

    // Connect to island graph.

    // Connect to body A
    c->m_nodeA.contact = c;
    c->m_nodeA.other = bodyB;

    c->m_nodeA.prev = nullptr;
    c->m_nodeA.next = bodyA->m_contactList;
    if (bodyA->m_contactList != nullptr) {
        bodyA->m_contactList->prev = &c->m_nodeA;
    }
    bodyA->m_contactList = &c->m_nodeA;

    // Connect to body B
    c->m_nodeB.contact = c;
    c->m_nodeB.other = bodyA;

    c->m_nodeB.prev = nullptr;
    c->m_nodeB.next = bodyB->m_contactList;
    if (bodyB->m_contactList != nullptr) {
        bodyB->m_contactList->prev = &c->m_nodeB;
    }
    bodyB->m_contactList = &c->m_nodeB;

    ++m_contactCount;
}


b2Contact *b2CircleContact::Create(b2Fixture *fixtureA, int32, b2Fixture *fixtureB, int32, b2BlockAllocator *allocator) {
    void *mem = allocator->Allocate(sizeof(b2CircleContact));
    return new (mem) b2CircleContact(fixtureA, fixtureB);
}

void b2CircleContact::Destroy(b2Contact *contact, b2BlockAllocator *allocator) {
    ((b2CircleContact *) contact)->~b2CircleContact();
    allocator->Free(contact, sizeof(b2CircleContact));
}

b2CircleContact::b2CircleContact(b2Fixture *fixtureA, b2Fixture *fixtureB)
    : b2Contact(fixtureA, 0, fixtureB, 0) {
    b2Assert(m_fixtureA->GetType() == b2Shape::e_circle);
    b2Assert(m_fixtureB->GetType() == b2Shape::e_circle);
}

void b2CircleContact::Evaluate(b2Manifold *manifold, const b2Transform &xfA, const b2Transform &xfB) {
    b2CollideCircles(manifold,
                     (b2CircleShape *) m_fixtureA->GetShape(), xfA,
                     (b2CircleShape *) m_fixtureB->GetShape(), xfB);
}


b2Contact *b2ChainAndPolygonContact::Create(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB, b2BlockAllocator *allocator) {
    void *mem = allocator->Allocate(sizeof(b2ChainAndPolygonContact));
    return new (mem) b2ChainAndPolygonContact(fixtureA, indexA, fixtureB, indexB);
}

void b2ChainAndPolygonContact::Destroy(b2Contact *contact, b2BlockAllocator *allocator) {
    ((b2ChainAndPolygonContact *) contact)->~b2ChainAndPolygonContact();
    allocator->Free(contact, sizeof(b2ChainAndPolygonContact));
}

b2ChainAndPolygonContact::b2ChainAndPolygonContact(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB)
    : b2Contact(fixtureA, indexA, fixtureB, indexB) {
    b2Assert(m_fixtureA->GetType() == b2Shape::e_chain);
    b2Assert(m_fixtureB->GetType() == b2Shape::e_polygon);
}

void b2ChainAndPolygonContact::Evaluate(b2Manifold *manifold, const b2Transform &xfA, const b2Transform &xfB) {
    b2ChainShape *chain = (b2ChainShape *) m_fixtureA->GetShape();
    b2EdgeShape edge;
    chain->GetChildEdge(&edge, m_indexA);
    b2CollideEdgeAndPolygon(manifold, &edge, xfA,
                            (b2PolygonShape *) m_fixtureB->GetShape(), xfB);
}


b2Contact *b2ChainAndCircleContact::Create(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB, b2BlockAllocator *allocator) {
    void *mem = allocator->Allocate(sizeof(b2ChainAndCircleContact));
    return new (mem) b2ChainAndCircleContact(fixtureA, indexA, fixtureB, indexB);
}

void b2ChainAndCircleContact::Destroy(b2Contact *contact, b2BlockAllocator *allocator) {
    ((b2ChainAndCircleContact *) contact)->~b2ChainAndCircleContact();
    allocator->Free(contact, sizeof(b2ChainAndCircleContact));
}

b2ChainAndCircleContact::b2ChainAndCircleContact(b2Fixture *fixtureA, int32 indexA, b2Fixture *fixtureB, int32 indexB)
    : b2Contact(fixtureA, indexA, fixtureB, indexB) {
    b2Assert(m_fixtureA->GetType() == b2Shape::e_chain);
    b2Assert(m_fixtureB->GetType() == b2Shape::e_circle);
}

void b2ChainAndCircleContact::Evaluate(b2Manifold *manifold, const b2Transform &xfA, const b2Transform &xfB) {
    b2ChainShape *chain = (b2ChainShape *) m_fixtureA->GetShape();
    b2EdgeShape edge;
    chain->GetChildEdge(&edge, m_indexA);
    b2CollideEdgeAndCircle(manifold, &edge, xfA,
                           (b2CircleShape *) m_fixtureB->GetShape(), xfB);
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
    b2Assert(def.count >= 3);
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

        if (L1sqr * L2sqr == 0.0f) {
            continue;
        }

        b2Vec2 Jd1 = (-1.0f / L1sqr) * e1.Skew();
        b2Vec2 Jd2 = (1.0f / L2sqr) * e2.Skew();

        b2Vec2 J1 = -Jd1;
        b2Vec2 J2 = Jd1 - Jd2;
        b2Vec2 J3 = Jd2;

        c.invEffectiveMass = c.invMass1 * b2Dot(J1, J1) + c.invMass2 * b2Dot(J2, J2) + c.invMass3 * b2Dot(J3, J3);

        b2Vec2 r = p3 - p1;

        float rr = r.LengthSquared();
        if (rr == 0.0f) {
            continue;
        }

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
        if (sum == 0.0f) {
            continue;
        }

        float mass = 1.0f / sum;

        c.spring = mass * stretchOmega * stretchOmega;
        c.damper = 2.0f * mass * m_tuning.stretchDamping * stretchOmega;
    }
}

void b2Rope::Step(float dt, int32 iterations, const b2Vec2 &position) {
    if (dt == 0.0) {
        return;
    }

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
    if (m_tuning.bendingModel == b2_springAngleBendingModel) {
        ApplyBendForces(dt);
    }

    for (int32 i = 0; i < m_bendCount; ++i) {
        m_bendConstraints[i].lambda = 0.0f;
    }

    for (int32 i = 0; i < m_stretchCount; ++i) {
        m_stretchConstraints[i].lambda = 0.0f;
    }

    // Update position
    for (int32 i = 0; i < m_count; ++i) {
        m_ps[i] += dt * m_vs[i];
    }

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

    for (int32 i = 0; i < m_bendCount; ++i) {
        m_bendConstraints[i].lambda = 0.0f;
    }

    for (int32 i = 0; i < m_stretchCount; ++i) {
        m_stretchConstraints[i].lambda = 0.0f;
    }
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
        if (sum == 0.0f) {
            continue;
        }

        float s1 = c.invMass1 / sum;
        float s2 = c.invMass2 / sum;

        p1 -= stiffness * s1 * (c.L - L) * d;
        p2 += stiffness * s2 * (c.L - L) * d;

        m_ps[c.i1] = p1;
        m_ps[c.i2] = p2;
    }
}

void b2Rope::SolveStretch_XPBD(float dt) {
    b2Assert(dt > 0.0f);

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
        if (sum == 0.0f) {
            continue;
        }

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

        if (L1sqr * L2sqr == 0.0f) {
            continue;
        }

        b2Vec2 Jd1 = (-1.0f / L1sqr) * d1.Skew();
        b2Vec2 Jd2 = (1.0f / L2sqr) * d2.Skew();

        b2Vec2 J1 = -Jd1;
        b2Vec2 J2 = Jd1 - Jd2;
        b2Vec2 J3 = Jd2;

        float sum;
        if (m_tuning.fixedEffectiveMass) {
            sum = c.invEffectiveMass;
        } else {
            sum = c.invMass1 * b2Dot(J1, J1) + c.invMass2 * b2Dot(J2, J2) + c.invMass3 * b2Dot(J3, J3);
        }

        if (sum == 0.0f) {
            sum = c.invEffectiveMass;
        }

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
    b2Assert(dt > 0.0f);

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

        if (L1sqr * L2sqr == 0.0f) {
            continue;
        }

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
            sum = c.invMass1 * b2Dot(J1, J1) + c.invMass2 * b2Dot(J2, J2) + c.invMass3 * b2Dot(J3, J3);
        }

        if (sum == 0.0f) {
            continue;
        }

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

        if (L1sqr * L2sqr == 0.0f) {
            continue;
        }

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
            sum = c.invMass1 * b2Dot(J1, J1) + c.invMass2 * b2Dot(J2, J2) + c.invMass3 * b2Dot(J3, J3);
        }

        if (sum == 0.0f) {
            continue;
        }

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
        if (sum == 0.0f) {
            continue;
        }

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

        if (dLen == 0.0f) {
            continue;
        }

        b2Vec2 dHat = (1.0f / dLen) * d;

        b2Vec2 J1 = c.alpha1 * dHat;
        b2Vec2 J2 = -dHat;
        b2Vec2 J3 = c.alpha2 * dHat;

        float sum = c.invMass1 * c.alpha1 * c.alpha1 + c.invMass2 + c.invMass3 * c.alpha2 * c.alpha2;

        if (sum == 0.0f) {
            continue;
        }

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

void b2Rope::Draw(b2Draw *draw) const {
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
