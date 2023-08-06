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

#ifndef BOX2D_H
#define BOX2D_H

// These include files constitute the main Box2D API

#include "b2_body.h"
#include "b2_broad_phase.h"
#include "b2_circle_shape.h"
#include "b2_contact.h"
#include "b2_edge_shape.h"
#include "b2_fixture.h"
#include "b2_geometry.h"
#include "b2_polygon_shape.h"
#include "b2_settings.h"
#include "b2_time_step.h"
#include "b2_timer.h"
#include "b2_world.h"
#include "b2_world_callbacks.h"

#ifdef ENABLE_LIQUID
#include "b2_particle.h"
#include "b2_particle_group.h"
#include "b2_particle_system.h"
#endif  // ENABLE_LIQUID

#include "b2_distance_joint.h"
#include "b2_friction_joint.h"
#include "b2_gear_joint.h"
#include "b2_motor_joint.h"
#include "b2_mouse_joint.h"
#include "b2_prismatic_joint.h"
#include "b2_pulley_joint.h"
#include "b2_revolute_joint.h"
#include "b2_weld_joint.h"
#include "b2_wheel_joint.h"

#endif