#include "Engine/Renderer/RenderDraw.h"

#include "Engine/Renderer/Render.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Get world coordinates from screen coordinates
R_public R_vec3 R_unproject(R_vec3 source, R_mat proj, R_mat view) {
    R_vec3 result = {0.0f, 0.0f, 0.0f};

    // Calculate unproject matrix (multiply view patrix by R_ctx->gl_ctx.projection matrix) and invert it
    R_mat mat_viewProj = R_mat_mul(view, proj);
    mat_viewProj = R_mat_invert(mat_viewProj);

    // Create quaternion from source point
    R_quaternion quat = {source.x, source.y, source.z, 1.0f};

    // Multiply quat point by unproject matrix
    quat = R_quaternion_transform(quat, mat_viewProj);

    // Normalized world points in vectors
    result.x = quat.x / quat.w;
    result.y = quat.y / quat.w;
    result.z = quat.z / quat.w;

    return result;
}

// Returns a ray trace from mouse position
R_public R_ray R_get_mouse_ray(R_sizei screen_size, R_vec2 mouse_position, R_camera3d camera) {
    R_ray ray = {0};

    // Calculate normalized device coordinates
    // NOTE: y value is negative
    float x = (2.0f * mouse_position.x) / (float) screen_size.width - 1.0f;
    float y = 1.0f - (2.0f * mouse_position.y) / (float) screen_size.height;
    float z = 1.0f;

    // Store values in a vector
    R_vec3 device_coords = {x, y, z};

    // Calculate view matrix from camera look at
    R_mat mat_view = R_mat_look_at(camera.position, camera.target, camera.up);

    R_mat mat_proj = R_mat_identity();

    if (camera.type == RF_CAMERA_PERSPECTIVE) {
        // Calculate projection matrix from perspective
        mat_proj = R_mat_perspective(camera.fovy * R_deg2rad,
                                     ((double) screen_size.width / (double) screen_size.height),
                                     0.01, 1000.0);
    } else if (camera.type == RF_CAMERA_ORTHOGRAPHIC) {
        float aspect = (float) screen_size.width / (float) screen_size.height;
        double top = camera.fovy / 2.0;
        double right = top * aspect;

        // Calculate projection matrix from orthographic
        mat_proj = R_mat_ortho(-right, right, -top, top, 0.01, 1000.0);
    }

    // Unproject far/near points
    R_vec3 near_point =
            R_unproject((R_vec3){device_coords.x, device_coords.y, 0.0f}, mat_proj, mat_view);
    R_vec3 far_point =
            R_unproject((R_vec3){device_coords.x, device_coords.y, 1.0f}, mat_proj, mat_view);

    // Unproject the mouse cursor in the near plane.
    // We need this as the source position because orthographic projects, compared to perspect doesn't have a
    // convergence point, meaning that the "eye" of the camera is more like a plane than a point.
    R_vec3 camera_plane_pointer_pos =
            R_unproject((R_vec3){device_coords.x, device_coords.y, -1.0f}, mat_proj, mat_view);

    // Calculate normalized direction vector
    R_vec3 direction = R_vec3_normalize(R_vec3_sub(far_point, near_point));

    if (camera.type == RF_CAMERA_PERSPECTIVE) {
        ray.position = camera.position;
    } else if (camera.type == RF_CAMERA_ORTHOGRAPHIC) {
        ray.position = camera_plane_pointer_pos;
    }

    // Apply calculated vectors to ray
    ray.direction = direction;

    return ray;
}

// Get transform matrix for camera
R_public R_mat R_get_camera_matrix(R_camera3d camera) {
    return R_mat_look_at(camera.position, camera.target, camera.up);
}

// Returns camera 2d transform matrix
R_public R_mat R_get_camera_matrix2d(R_camera2d camera) {
    R_mat mat_transform = {0};
    // The camera in world-space is set by
    //   1. Move it to target
    //   2. Rotate by -rotation and scale by (1/zoom)
    //      When setting higher scale, it's more intuitive for the world to become bigger (= camera become smaller),
    //      not for the camera getting bigger, hence the invert. Same deal with rotation.
    //   3. Move it by (-offset);
    //      Offset defines target transform relative to screen, but since we're effectively "moving" screen (camera)
    //      we need to do it into opposite direction (inverse transform)

    // Having camera transform in world-space, inverse of it gives the R_gfxobal_model_view transform.
    // Since (A*B*C)' = C'*B'*A', the R_gfxobal_model_view is
    //   1. Move to offset
    //   2. Rotate and Scale
    //   3. Move by -target
    R_mat mat_origin = R_mat_translate(-camera.target.x, -camera.target.y, 0.0f);
    R_mat mat_rotation = R_mat_rotate((R_vec3){0.0f, 0.0f, 1.0f}, camera.rotation * R_deg2rad);
    R_mat mat_scale = R_mat_scale(camera.zoom, camera.zoom, 1.0f);
    R_mat mat_translation = R_mat_translate(camera.offset.x, camera.offset.y, 0.0f);

    mat_transform =
            R_mat_mul(R_mat_mul(mat_origin, R_mat_mul(mat_scale, mat_rotation)), mat_translation);

    return mat_transform;
}

// Returns the screen space position from a 3d world space position
R_public R_vec2 R_get_world_to_screen(R_sizei screen_size, R_vec3 position, R_camera3d camera) {
    // Calculate projection matrix from perspective instead of frustum
    R_mat mat_proj = R_mat_identity();

    if (camera.type == RF_CAMERA_PERSPECTIVE) {
        // Calculate projection matrix from perspective
        mat_proj = R_mat_perspective(camera.fovy * R_deg2rad,
                                     ((double) screen_size.width / (double) screen_size.height),
                                     0.01, 1000.0);
    } else if (camera.type == RF_CAMERA_ORTHOGRAPHIC) {
        float aspect = (float) screen_size.width / (float) screen_size.height;
        double top = camera.fovy / 2.0;
        double right = top * aspect;

        // Calculate projection matrix from orthographic
        mat_proj = R_mat_ortho(-right, right, -top, top, 0.01, 1000.0);
    }

    // Calculate view matrix from camera look at (and transpose it)
    R_mat mat_view = R_mat_look_at(camera.position, camera.target, camera.up);

    // Convert world position vector to quaternion
    R_quaternion world_pos = {position.x, position.y, position.z, 1.0f};

    // Transform world position to view
    world_pos = R_quaternion_transform(world_pos, mat_view);

    // Transform result to projection (clip space position)
    world_pos = R_quaternion_transform(world_pos, mat_proj);

    // Calculate normalized device coordinates (inverted y)
    R_vec3 ndc_pos = {world_pos.x / world_pos.w, -world_pos.y / world_pos.w,
                      world_pos.z / world_pos.w};

    // Calculate 2d screen position vector
    R_vec2 screen_position = {(ndc_pos.x + 1.0f) / 2.0f * (float) screen_size.width,
                              (ndc_pos.y + 1.0f) / 2.0f * (float) screen_size.height};

    return screen_position;
}

// Returns the screen space position for a 2d camera world space position
R_public R_vec2 R_get_world_to_screen2d(R_vec2 position, R_camera2d camera) {
    R_mat mat_camera = R_get_camera_matrix2d(camera);
    R_vec3 transform = R_vec3_transform((R_vec3){position.x, position.y, 0}, mat_camera);

    return (R_vec2){transform.x, transform.y};
}

// Returns the world space position for a 2d camera screen space position
R_public R_vec2 R_get_screen_to_world2d(R_vec2 position, R_camera2d camera) {
    R_mat inv_mat_camera = R_mat_invert(R_get_camera_matrix2d(camera));
    R_vec3 transform = R_vec3_transform((R_vec3){position.x, position.y, 0}, inv_mat_camera);

    return (R_vec2){transform.x, transform.y};
}

// Select camera mode (multiple camera modes available)
R_public void R_set_camera3d_mode(R_camera3d_state *state, R_camera3d camera,
                                  R_builtin_camera3d_mode mode) {
    R_vec3 v1 = camera.position;
    R_vec3 v2 = camera.target;

    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    float dz = v2.z - v1.z;

    state->camera_target_distance = sqrtf(dx * dx + dy * dy + dz * dz);

    R_vec2 distance = {0.0f, 0.0f};
    distance.x = sqrtf(dx * dx + dz * dz);
    distance.y = sqrtf(dx * dx + dy * dy);

    // R_camera3d angle calculation
    state->camera_angle.x =
            asinf((float) fabs(dx) /
                  distance.x);// R_camera3d angle in plane XZ (0 aligned with Z, move positive CCW)
    state->camera_angle.y =
            -asinf((float) fabs(dy) /
                   distance.y);// R_camera3d angle in plane XY (0 aligned with X, move positive CW)

    state->player_eyes_position = camera.position.y;

    // Lock cursor for first person and third person cameras
    // if ((mode == R_camera_first_person) || (mode == R_camera_third_person)) DisableCursor();
    // else EnableCursor();

    state->camera_mode = mode;
}

// Update camera depending on selected mode
// NOTE: R_camera3d controls depend on some raylib functions:
//       System: EnableCursor(), DisableCursor()
//       Mouse: IsMouseButtonDown(), GetMousePosition(), GetMouseWheelMove()
//       Keys:  IsKeyDown()
// TODO: Port to quaternion-based camera
R_public void R_update_camera3d(R_camera3d *camera, R_camera3d_state *state,
                                R_input_state_for_update_camera input_state) {
// R_camera3d mouse movement sensitivity
#define RF_CAMERA_MOUSE_MOVE_SENSITIVITY 0.003f
#define RF_CAMERA_MOUSE_SCROLL_SENSITIVITY 1.5f

// FREE_CAMERA
#define RF_CAMERA_FREE_MOUSE_SENSITIVITY 0.01f
#define RF_CAMERA_FREE_DISTANCE_MIN_CLAMP 0.3f
#define RF_CAMERA_FREE_DISTANCE_MAX_CLAMP 120.0f
#define RF_CAMERA_FREE_MIN_CLAMP 85.0f
#define RF_CAMERA_FREE_MAX_CLAMP -85.0f
#define RF_CAMERA_FREE_SMOOTH_ZOOM_SENSITIVITY 0.05f
#define RF_CAMERA_FREE_PANNING_DIVIDER 5.1f

// ORBITAL_CAMERA
#define RF_CAMERA_ORBITAL_SPEED 0.01f// Radians per frame

// FIRST_PERSON
//#define CAMERA_FIRST_PERSON_MOUSE_SENSITIVITY           0.003f
#define RF_CAMERA_FIRST_PERSON_FOCUS_DISTANCE 25.0f
#define RF_CAMERA_FIRST_PERSON_MIN_CLAMP 85.0f
#define RF_CAMERA_FIRST_PERSON_MAX_CLAMP -85.0f

#define RF_CAMERA_FIRST_PERSON_STEP_TRIGONOMETRIC_DIVIDER 5.0f
#define RF_CAMERA_FIRST_PERSON_STEP_DIVIDER 30.0f
#define RF_CAMERA_FIRST_PERSON_WAVING_DIVIDER 200.0f

// THIRD_PERSON
//#define CAMERA_THIRD_PERSON_MOUSE_SENSITIVITY           0.003f
#define RF_CAMERA_THIRD_PERSON_DISTANCE_CLAMP 1.2f
#define RF_CAMERA_THIRD_PERSON_MIN_CLAMP 5.0f
#define RF_CAMERA_THIRD_PERSON_MAX_CLAMP -85.0f
#define RF_CAMERA_THIRD_PERSON_OFFSET                                                              \
    (R_vec3) { 0.4f, 0.0f, 0.0f }

// PLAYER (used by camera)
#define RF_PLAYER_MOVEMENT_SENSITIVITY 20.0f

    // R_camera3d move modes (first person and third person cameras)
    typedef enum R_camera_move {
        R_move_front = 0,
        R_move_back,
        R_move_right,
        R_move_left,
        R_move_up,
        R_move_down
    } R_camera_move;

    // R_internal float player_eyes_position = 1.85f;

    // TODO: CRF_INTERNAL R_ctx->gl_ctx.camera_target_distance and R_ctx->gl_ctx.camera_angle here

    // Mouse movement detection
    R_vec2 mouse_position_delta = {0.0f, 0.0f};
    R_vec2 mouse_position = input_state.mouse_position;
    int mouse_wheel_move = input_state.mouse_wheel_move;

    // Keys input detection
    R_bool pan_key = input_state.is_camera_pan_control_key_down;
    R_bool alt_key = input_state.is_camera_alt_control_key_down;
    R_bool szoom_key = input_state.is_camera_smooth_zoom_control_key;

    R_bool direction[6];
    direction[0] = input_state.direction_keys[0];
    direction[1] = input_state.direction_keys[1];
    direction[2] = input_state.direction_keys[2];
    direction[3] = input_state.direction_keys[3];
    direction[4] = input_state.direction_keys[4];
    direction[5] = input_state.direction_keys[5];

    // TODO: Consider touch inputs for camera

    if (state->camera_mode != RF_CAMERA_CUSTOM) {
        mouse_position_delta.x = mouse_position.x - state->previous_mouse_position.x;
        mouse_position_delta.y = mouse_position.y - state->previous_mouse_position.y;

        state->previous_mouse_position = mouse_position;
    }

    // Support for multiple automatic camera modes
    switch (state->camera_mode) {
        case RF_CAMERA_FREE: {
            // Camera zoom
            if ((state->camera_target_distance < RF_CAMERA_FREE_DISTANCE_MAX_CLAMP) &&
                (mouse_wheel_move < 0)) {
                state->camera_target_distance -=
                        (mouse_wheel_move * RF_CAMERA_MOUSE_SCROLL_SENSITIVITY);

                if (state->camera_target_distance > RF_CAMERA_FREE_DISTANCE_MAX_CLAMP) {
                    state->camera_target_distance = RF_CAMERA_FREE_DISTANCE_MAX_CLAMP;
                }
            }
            // Camera looking down
            // TODO: Review, weird comparisson of R_ctx->gl_ctx.camera_target_distance == 120.0f?
            else if ((camera->position.y > camera->target.y) &&
                     (state->camera_target_distance == RF_CAMERA_FREE_DISTANCE_MAX_CLAMP) &&
                     (mouse_wheel_move < 0)) {
                camera->target.x += mouse_wheel_move * (camera->target.x - camera->position.x) *
                                    RF_CAMERA_MOUSE_SCROLL_SENSITIVITY /
                                    state->camera_target_distance;
                camera->target.y += mouse_wheel_move * (camera->target.y - camera->position.y) *
                                    RF_CAMERA_MOUSE_SCROLL_SENSITIVITY /
                                    state->camera_target_distance;
                camera->target.z += mouse_wheel_move * (camera->target.z - camera->position.z) *
                                    RF_CAMERA_MOUSE_SCROLL_SENSITIVITY /
                                    state->camera_target_distance;
            } else if ((camera->position.y > camera->target.y) && (camera->target.y >= 0)) {
                camera->target.x += mouse_wheel_move * (camera->target.x - camera->position.x) *
                                    RF_CAMERA_MOUSE_SCROLL_SENSITIVITY /
                                    state->camera_target_distance;
                camera->target.y += mouse_wheel_move * (camera->target.y - camera->position.y) *
                                    RF_CAMERA_MOUSE_SCROLL_SENSITIVITY /
                                    state->camera_target_distance;
                camera->target.z += mouse_wheel_move * (camera->target.z - camera->position.z) *
                                    RF_CAMERA_MOUSE_SCROLL_SENSITIVITY /
                                    state->camera_target_distance;

                // if (camera->target.y < 0) camera->target.y = -0.001;
            } else if ((camera->position.y > camera->target.y) && (camera->target.y < 0) &&
                       (mouse_wheel_move > 0)) {
                state->camera_target_distance -=
                        (mouse_wheel_move * RF_CAMERA_MOUSE_SCROLL_SENSITIVITY);
                if (state->camera_target_distance < RF_CAMERA_FREE_DISTANCE_MIN_CLAMP) {
                    state->camera_target_distance = RF_CAMERA_FREE_DISTANCE_MIN_CLAMP;
                }
            }
            // Camera looking up
            // TODO: Review, weird comparisson of R_ctx->gl_ctx.camera_target_distance == 120.0f?
            else if ((camera->position.y < camera->target.y) &&
                     (state->camera_target_distance == RF_CAMERA_FREE_DISTANCE_MAX_CLAMP) &&
                     (mouse_wheel_move < 0)) {
                camera->target.x += mouse_wheel_move * (camera->target.x - camera->position.x) *
                                    RF_CAMERA_MOUSE_SCROLL_SENSITIVITY /
                                    state->camera_target_distance;
                camera->target.y += mouse_wheel_move * (camera->target.y - camera->position.y) *
                                    RF_CAMERA_MOUSE_SCROLL_SENSITIVITY /
                                    state->camera_target_distance;
                camera->target.z += mouse_wheel_move * (camera->target.z - camera->position.z) *
                                    RF_CAMERA_MOUSE_SCROLL_SENSITIVITY /
                                    state->camera_target_distance;
            } else if ((camera->position.y < camera->target.y) && (camera->target.y <= 0)) {
                camera->target.x += mouse_wheel_move * (camera->target.x - camera->position.x) *
                                    RF_CAMERA_MOUSE_SCROLL_SENSITIVITY /
                                    state->camera_target_distance;
                camera->target.y += mouse_wheel_move * (camera->target.y - camera->position.y) *
                                    RF_CAMERA_MOUSE_SCROLL_SENSITIVITY /
                                    state->camera_target_distance;
                camera->target.z += mouse_wheel_move * (camera->target.z - camera->position.z) *
                                    RF_CAMERA_MOUSE_SCROLL_SENSITIVITY /
                                    state->camera_target_distance;

                // if (camera->target.y > 0) camera->target.y = 0.001;
            } else if ((camera->position.y < camera->target.y) && (camera->target.y > 0) &&
                       (mouse_wheel_move > 0)) {
                state->camera_target_distance -=
                        (mouse_wheel_move * RF_CAMERA_MOUSE_SCROLL_SENSITIVITY);
                if (state->camera_target_distance < RF_CAMERA_FREE_DISTANCE_MIN_CLAMP) {
                    state->camera_target_distance = RF_CAMERA_FREE_DISTANCE_MIN_CLAMP;
                }
            }

            // Input keys checks
            if (pan_key) {
                if (alt_key)// Alternative key behaviour
                {
                    if (szoom_key) {
                        // Camera smooth zoom
                        state->camera_target_distance +=
                                (mouse_position_delta.y * RF_CAMERA_FREE_SMOOTH_ZOOM_SENSITIVITY);
                    } else {
                        // Camera rotation
                        state->camera_angle.x +=
                                mouse_position_delta.x * -RF_CAMERA_FREE_MOUSE_SENSITIVITY;
                        state->camera_angle.y +=
                                mouse_position_delta.y * -RF_CAMERA_FREE_MOUSE_SENSITIVITY;

                        // Angle clamp
                        if (state->camera_angle.y > RF_CAMERA_FREE_MIN_CLAMP * R_deg2rad) {
                            state->camera_angle.y = RF_CAMERA_FREE_MIN_CLAMP * R_deg2rad;
                        } else if (state->camera_angle.y < RF_CAMERA_FREE_MAX_CLAMP * R_deg2rad) {
                            state->camera_angle.y = RF_CAMERA_FREE_MAX_CLAMP * R_deg2rad;
                        }
                    }
                } else {
                    // Camera panning
                    camera->target.x +=
                            ((mouse_position_delta.x * -RF_CAMERA_FREE_MOUSE_SENSITIVITY) *
                                     cosf(state->camera_angle.x) +
                             (mouse_position_delta.y * RF_CAMERA_FREE_MOUSE_SENSITIVITY) *
                                     sinf(state->camera_angle.x) * sinf(state->camera_angle.y)) *
                            (state->camera_target_distance / RF_CAMERA_FREE_PANNING_DIVIDER);
                    camera->target.y +=
                            ((mouse_position_delta.y * RF_CAMERA_FREE_MOUSE_SENSITIVITY) *
                             cosf(state->camera_angle.y)) *
                            (state->camera_target_distance / RF_CAMERA_FREE_PANNING_DIVIDER);
                    camera->target.z +=
                            ((mouse_position_delta.x * RF_CAMERA_FREE_MOUSE_SENSITIVITY) *
                                     sinf(state->camera_angle.x) +
                             (mouse_position_delta.y * RF_CAMERA_FREE_MOUSE_SENSITIVITY) *
                                     cosf(state->camera_angle.x) * sinf(state->camera_angle.y)) *
                            (state->camera_target_distance / RF_CAMERA_FREE_PANNING_DIVIDER);
                }
            }

            // Update camera position with changes
            camera->position.x = sinf(state->camera_angle.x) * state->camera_target_distance *
                                         cosf(state->camera_angle.y) +
                                 camera->target.x;
            camera->position.y =
                    ((state->camera_angle.y <= 0.0f) ? 1 : -1) * sinf(state->camera_angle.y) *
                            state->camera_target_distance * sinf(state->camera_angle.y) +
                    camera->target.y;
            camera->position.z = cosf(state->camera_angle.x) * state->camera_target_distance *
                                         cosf(state->camera_angle.y) +
                                 camera->target.z;

        } break;

        case RF_CAMERA_ORBITAL: {
            state->camera_angle.x += RF_CAMERA_ORBITAL_SPEED;// Camera orbit angle
            state->camera_target_distance -=
                    (mouse_wheel_move * RF_CAMERA_MOUSE_SCROLL_SENSITIVITY);// Camera zoom

            // Camera distance clamp
            if (state->camera_target_distance < RF_CAMERA_THIRD_PERSON_DISTANCE_CLAMP) {
                state->camera_target_distance = RF_CAMERA_THIRD_PERSON_DISTANCE_CLAMP;
            }

            // Update camera position with changes
            camera->position.x = sinf(state->camera_angle.x) * state->camera_target_distance *
                                         cosf(state->camera_angle.y) +
                                 camera->target.x;
            camera->position.y =
                    ((state->camera_angle.y <= 0.0f) ? 1 : -1) * sinf(state->camera_angle.y) *
                            state->camera_target_distance * sinf(state->camera_angle.y) +
                    camera->target.y;
            camera->position.z = cosf(state->camera_angle.x) * state->camera_target_distance *
                                         cosf(state->camera_angle.y) +
                                 camera->target.z;

        } break;

        case RF_CAMERA_FIRST_PERSON: {
            camera->position.x += (sinf(state->camera_angle.x) * direction[R_move_back] -
                                   sinf(state->camera_angle.x) * direction[R_move_front] -
                                   cosf(state->camera_angle.x) * direction[R_move_left] +
                                   cosf(state->camera_angle.x) * direction[R_move_right]) /
                                  RF_PLAYER_MOVEMENT_SENSITIVITY;

            camera->position.y += (sinf(state->camera_angle.y) * direction[R_move_front] -
                                   sinf(state->camera_angle.y) * direction[R_move_back] +
                                   1.0f * direction[R_move_up] - 1.0f * direction[R_move_down]) /
                                  RF_PLAYER_MOVEMENT_SENSITIVITY;

            camera->position.z += (cosf(state->camera_angle.x) * direction[R_move_back] -
                                   cosf(state->camera_angle.x) * direction[R_move_front] +
                                   sinf(state->camera_angle.x) * direction[R_move_left] -
                                   sinf(state->camera_angle.x) * direction[R_move_right]) /
                                  RF_PLAYER_MOVEMENT_SENSITIVITY;

            R_bool is_moving = 0;// Required for swinging

            for (R_int i = 0; i < 6; i++)
                if (direction[i]) {
                    is_moving = 1;
                    break;
                }

            // Camera orientation calculation
            state->camera_angle.x += (mouse_position_delta.x * -RF_CAMERA_MOUSE_MOVE_SENSITIVITY);
            state->camera_angle.y += (mouse_position_delta.y * -RF_CAMERA_MOUSE_MOVE_SENSITIVITY);

            // Angle clamp
            if (state->camera_angle.y > RF_CAMERA_FIRST_PERSON_MIN_CLAMP * R_deg2rad) {
                state->camera_angle.y = RF_CAMERA_FIRST_PERSON_MIN_CLAMP * R_deg2rad;
            } else if (state->camera_angle.y < RF_CAMERA_FIRST_PERSON_MAX_CLAMP * R_deg2rad) {
                state->camera_angle.y = RF_CAMERA_FIRST_PERSON_MAX_CLAMP * R_deg2rad;
            }

            // Camera is always looking at player
            camera->target.x = camera->position.x -
                               sinf(state->camera_angle.x) * RF_CAMERA_FIRST_PERSON_FOCUS_DISTANCE;
            camera->target.y = camera->position.y +
                               sinf(state->camera_angle.y) * RF_CAMERA_FIRST_PERSON_FOCUS_DISTANCE;
            camera->target.z = camera->position.z -
                               cosf(state->camera_angle.x) * RF_CAMERA_FIRST_PERSON_FOCUS_DISTANCE;

            if (is_moving) { state->swing_counter++; }

            // Camera position update
            // NOTE: On RF_CAMERA_FIRST_PERSON player Y-movement is limited to player 'eyes position'
            camera->position.y =
                    state->player_eyes_position -
                    sinf(state->swing_counter / RF_CAMERA_FIRST_PERSON_STEP_TRIGONOMETRIC_DIVIDER) /
                            RF_CAMERA_FIRST_PERSON_STEP_DIVIDER;

            camera->up.x = sinf(state->swing_counter /
                                (RF_CAMERA_FIRST_PERSON_STEP_TRIGONOMETRIC_DIVIDER * 2)) /
                           RF_CAMERA_FIRST_PERSON_WAVING_DIVIDER;
            camera->up.z = -sinf(state->swing_counter /
                                 (RF_CAMERA_FIRST_PERSON_STEP_TRIGONOMETRIC_DIVIDER * 2)) /
                           RF_CAMERA_FIRST_PERSON_WAVING_DIVIDER;

        } break;

        case RF_CAMERA_THIRD_PERSON: {
            camera->position.x += (sinf(state->camera_angle.x) * direction[R_move_back] -
                                   sinf(state->camera_angle.x) * direction[R_move_front] -
                                   cosf(state->camera_angle.x) * direction[R_move_left] +
                                   cosf(state->camera_angle.x) * direction[R_move_right]) /
                                  RF_PLAYER_MOVEMENT_SENSITIVITY;

            camera->position.y += (sinf(state->camera_angle.y) * direction[R_move_front] -
                                   sinf(state->camera_angle.y) * direction[R_move_back] +
                                   1.0f * direction[R_move_up] - 1.0f * direction[R_move_down]) /
                                  RF_PLAYER_MOVEMENT_SENSITIVITY;

            camera->position.z += (cosf(state->camera_angle.x) * direction[R_move_back] -
                                   cosf(state->camera_angle.x) * direction[R_move_front] +
                                   sinf(state->camera_angle.x) * direction[R_move_left] -
                                   sinf(state->camera_angle.x) * direction[R_move_right]) /
                                  RF_PLAYER_MOVEMENT_SENSITIVITY;

            // Camera orientation calculation
            state->camera_angle.x += (mouse_position_delta.x * -RF_CAMERA_MOUSE_MOVE_SENSITIVITY);
            state->camera_angle.y += (mouse_position_delta.y * -RF_CAMERA_MOUSE_MOVE_SENSITIVITY);

            // Angle clamp
            if (state->camera_angle.y > RF_CAMERA_THIRD_PERSON_MIN_CLAMP * R_deg2rad) {
                state->camera_angle.y = RF_CAMERA_THIRD_PERSON_MIN_CLAMP * R_deg2rad;
            } else if (state->camera_angle.y < RF_CAMERA_THIRD_PERSON_MAX_CLAMP * R_deg2rad) {
                state->camera_angle.y = RF_CAMERA_THIRD_PERSON_MAX_CLAMP * R_deg2rad;
            }

            // Camera zoom
            state->camera_target_distance -=
                    (mouse_wheel_move * RF_CAMERA_MOUSE_SCROLL_SENSITIVITY);

            // Camera distance clamp
            if (state->camera_target_distance < RF_CAMERA_THIRD_PERSON_DISTANCE_CLAMP) {
                state->camera_target_distance = RF_CAMERA_THIRD_PERSON_DISTANCE_CLAMP;
            }

            // TODO: It seems camera->position is not correctly updated or some rounding issue makes the camera move straight to camera->target...
            camera->position.x = sinf(state->camera_angle.x) * state->camera_target_distance *
                                         cosf(state->camera_angle.y) +
                                 camera->target.x;

            if (state->camera_angle.y <= 0.0f) {
                camera->position.y = sinf(state->camera_angle.y) * state->camera_target_distance *
                                             sinf(state->camera_angle.y) +
                                     camera->target.y;
            } else {
                camera->position.y = -sinf(state->camera_angle.y) * state->camera_target_distance *
                                             sinf(state->camera_angle.y) +
                                     camera->target.y;
            }

            camera->position.z = cosf(state->camera_angle.x) * state->camera_target_distance *
                                         cosf(state->camera_angle.y) +
                                 camera->target.z;

        } break;

        default:
            break;
    }
}

#pragma region pixel format

R_public const char *R_pixel_format_string(R_pixel_format format) {
    switch (format) {
        case R_pixel_format_grayscale:
            return "RF_UNCOMPRESSED_GRAYSCALE";
        case R_pixel_format_gray_alpha:
            return "RF_UNCOMPRESSED_GRAY_ALPHA";
        case R_pixel_format_r5g6b5:
            return "RF_UNCOMPRESSED_R5G6B5";
        case R_pixel_format_r8g8b8:
            return "RF_UNCOMPRESSED_R8G8B8";
        case R_pixel_format_r5g5b5a1:
            return "RF_UNCOMPRESSED_R5G5B5A1";
        case R_pixel_format_r4g4b4a4:
            return "RF_UNCOMPRESSED_R4G4B4A4";
        case R_pixel_format_r8g8b8a8:
            return "RF_UNCOMPRESSED_R8G8B8A8";
        case R_pixel_format_r32:
            return "RF_UNCOMPRESSED_R32";
        case R_pixel_format_r32g32b32:
            return "RF_UNCOMPRESSED_R32G32B32";
        case R_pixel_format_r32g32b32a32:
            return "RF_UNCOMPRESSED_R32G32B32A32";
        case R_pixel_format_dxt1_rgb:
            return "RF_COMPRESSED_DXT1_RGB";
        case R_pixel_format_dxt1_rgba:
            return "RF_COMPRESSED_DXT1_RGBA";
        case R_pixel_format_dxt3_rgba:
            return "RF_COMPRESSED_DXT3_RGBA";
        case R_pixel_format_dxt5_rgba:
            return "RF_COMPRESSED_DXT5_RGBA";
        case R_pixel_format_etc1_rgb:
            return "RF_COMPRESSED_ETC1_RGB";
        case R_pixel_format_etc2_rgb:
            return "RF_COMPRESSED_ETC2_RGB";
        case R_pixel_format_etc2_eac_rgba:
            return "RF_COMPRESSED_ETC2_EAC_RGBA";
        case R_pixel_format_pvrt_rgb:
            return "RF_COMPRESSED_PVRT_RGB";
        case R_pixel_format_prvt_rgba:
            return "RF_COMPRESSED_PVRT_RGBA";
        case R_pixel_format_astc_4x4_rgba:
            return "RF_COMPRESSED_ASTC_4x4_RGBA";
        case R_pixel_format_astc_8x8_rgba:
            return "RF_COMPRESSED_ASTC_8x8_RGBA";
        default:
            return NULL;
    }
}

R_public R_bool R_is_uncompressed_format(R_pixel_format format) {
    return format >= R_pixel_format_grayscale && format <= R_pixel_format_r32g32b32a32;
}

R_public R_bool R_is_compressed_format(R_pixel_format format) {
    return format >= R_pixel_format_dxt1_rgb && format <= R_pixel_format_astc_8x8_rgba;
}

R_public int R_bits_per_pixel(R_pixel_format format) {
    switch (format) {
        case R_pixel_format_grayscale:
            return 8;// 8 bit per pixel (no alpha)
        case R_pixel_format_gray_alpha:
            return 8 * 2;// 8 * 2 bpp (2 channels)
        case R_pixel_format_r5g6b5:
            return 16;// 16 bpp
        case R_pixel_format_r8g8b8:
            return 24;// 24 bpp
        case R_pixel_format_r5g5b5a1:
            return 16;// 16 bpp (1 bit alpha)
        case R_pixel_format_r4g4b4a4:
            return 16;// 16 bpp (4 bit alpha)
        case R_pixel_format_r8g8b8a8:
            return 32;// 32 bpp
        case R_pixel_format_r32:
            return 32;// 32 bpp (1 channel - float)
        case R_pixel_format_r32g32b32:
            return 32 * 3;// 32 * 3 bpp (3 channels - float)
        case R_pixel_format_r32g32b32a32:
            return 32 * 4;// 32 * 4 bpp (4 channels - float)
        case R_pixel_format_dxt1_rgb:
            return 4;// 4 bpp (no alpha)
        case R_pixel_format_dxt1_rgba:
            return 4;// 4 bpp (1 bit alpha)
        case R_pixel_format_dxt3_rgba:
            return 8;// 8 bpp
        case R_pixel_format_dxt5_rgba:
            return 8;// 8 bpp
        case R_pixel_format_etc1_rgb:
            return 4;// 4 bpp
        case R_pixel_format_etc2_rgb:
            return 4;// 4 bpp
        case R_pixel_format_etc2_eac_rgba:
            return 8;// 8 bpp
        case R_pixel_format_pvrt_rgb:
            return 4;// 4 bpp
        case R_pixel_format_prvt_rgba:
            return 4;// 4 bpp
        case R_pixel_format_astc_4x4_rgba:
            return 8;// 8 bpp
        case R_pixel_format_astc_8x8_rgba:
            return 2;// 2 bpp
        default:
            return 0;
    }
}

R_public int R_bytes_per_pixel(R_uncompressed_pixel_format format) {
    switch (format) {
        case R_pixel_format_grayscale:
            return 1;
        case R_pixel_format_gray_alpha:
            return 2;
        case R_pixel_format_r5g5b5a1:
            return 2;
        case R_pixel_format_r5g6b5:
            return 2;
        case R_pixel_format_r4g4b4a4:
            return 2;
        case R_pixel_format_r8g8b8a8:
            return 4;
        case R_pixel_format_r8g8b8:
            return 3;
        case R_pixel_format_r32:
            return 4;
        case R_pixel_format_r32g32b32:
            return 12;
        case R_pixel_format_r32g32b32a32:
            return 16;
        default:
            return 0;
    }
}

R_public int R_pixel_buffer_size(int width, int height, R_pixel_format format) {
    return width * height * R_bits_per_pixel(format) / 8;
}

R_public R_bool R_format_pixels_to_normalized(const void *src, R_int src_size,
                                              R_uncompressed_pixel_format src_format, R_vec4 *dst,
                                              R_int dst_size) {
    R_bool success = 0;

    R_int src_bpp = R_bytes_per_pixel(src_format);
    R_int src_pixel_count = src_size / src_bpp;
    R_int dst_pixel_count = dst_size / sizeof(R_vec4);

    if (dst_pixel_count >= src_pixel_count) {
        if (src_format == R_pixel_format_r32g32b32a32) {
            success = 1;
            memcpy(dst, src, src_size);
        } else {
            success = 1;

#define RF_FOR_EACH_PIXEL                                                                          \
    for (R_int dst_iter = 0, src_iter = 0; src_iter < src_size && dst_iter < dst_size;             \
         dst_iter++, src_iter += src_bpp)
            switch (src_format) {
                case R_pixel_format_grayscale:
                    RF_FOR_EACH_PIXEL {
                        float value = ((unsigned char *) src)[src_iter] / 255.0f;

                        dst[dst_iter].x = value;
                        dst[dst_iter].y = value;
                        dst[dst_iter].z = value;
                        dst[dst_iter].w = 1.0f;
                    }
                    break;

                case R_pixel_format_gray_alpha:
                    RF_FOR_EACH_PIXEL {
                        float value0 = (float) ((unsigned char *) src)[src_iter + 0] / 255.0f;
                        float value1 = (float) ((unsigned char *) src)[src_iter + 1] / 255.0f;

                        dst[dst_iter].x = value0;
                        dst[dst_iter].y = value0;
                        dst[dst_iter].z = value0;
                        dst[dst_iter].w = value1;
                    }
                    break;

                case R_pixel_format_r5g5b5a1:
                    RF_FOR_EACH_PIXEL {
                        unsigned short pixel = ((unsigned short *) src)[src_iter];

                        dst[dst_iter].x =
                                (float) ((pixel & 0b1111100000000000) >> 11) * (1.0f / 31);
                        dst[dst_iter].y = (float) ((pixel & 0b0000011111000000) >> 6) * (1.0f / 31);
                        dst[dst_iter].z = (float) ((pixel & 0b0000000000111110) >> 1) * (1.0f / 31);
                        dst[dst_iter].w = ((pixel & 0b0000000000000001) == 0) ? 0.0f : 1.0f;
                    }
                    break;

                case R_pixel_format_r5g6b5:
                    RF_FOR_EACH_PIXEL {
                        unsigned short pixel = ((unsigned short *) src)[src_iter];

                        dst[dst_iter].x =
                                (float) ((pixel & 0b1111100000000000) >> 11) * (1.0f / 31);
                        dst[dst_iter].y = (float) ((pixel & 0b0000011111100000) >> 5) * (1.0f / 63);
                        dst[dst_iter].z = (float) (pixel & 0b0000000000011111) * (1.0f / 31);
                        dst[dst_iter].w = 1.0f;
                    }
                    break;

                case R_pixel_format_r4g4b4a4:
                    RF_FOR_EACH_PIXEL {
                        unsigned short pixel = ((unsigned short *) src)[src_iter];

                        dst[dst_iter].x =
                                (float) ((pixel & 0b1111000000000000) >> 12) * (1.0f / 15);
                        dst[dst_iter].y = (float) ((pixel & 0b0000111100000000) >> 8) * (1.0f / 15);
                        dst[dst_iter].z = (float) ((pixel & 0b0000000011110000) >> 4) * (1.0f / 15);
                        dst[dst_iter].w = (float) (pixel & 0b0000000000001111) * (1.0f / 15);
                    }
                    break;

                case R_pixel_format_r8g8b8a8:
                    RF_FOR_EACH_PIXEL {
                        dst[dst_iter].x = (float) ((unsigned char *) src)[src_iter + 0] / 255.0f;
                        dst[dst_iter].y = (float) ((unsigned char *) src)[src_iter + 1] / 255.0f;
                        dst[dst_iter].z = (float) ((unsigned char *) src)[src_iter + 2] / 255.0f;
                        dst[dst_iter].w = (float) ((unsigned char *) src)[src_iter + 3] / 255.0f;
                    }
                    break;

                case R_pixel_format_r8g8b8:
                    RF_FOR_EACH_PIXEL {
                        dst[dst_iter].x = (float) ((unsigned char *) src)[src_iter + 0] / 255.0f;
                        dst[dst_iter].y = (float) ((unsigned char *) src)[src_iter + 1] / 255.0f;
                        dst[dst_iter].z = (float) ((unsigned char *) src)[src_iter + 2] / 255.0f;
                        dst[dst_iter].w = 1.0f;
                    }
                    break;

                case R_pixel_format_r32:
                    RF_FOR_EACH_PIXEL {
                        dst[dst_iter].x = ((float *) src)[src_iter];
                        dst[dst_iter].y = 0.0f;
                        dst[dst_iter].z = 0.0f;
                        dst[dst_iter].w = 1.0f;
                    }
                    break;

                case R_pixel_format_r32g32b32:
                    RF_FOR_EACH_PIXEL {
                        dst[dst_iter].x = ((float *) src)[src_iter + 0];
                        dst[dst_iter].y = ((float *) src)[src_iter + 1];
                        dst[dst_iter].z = ((float *) src)[src_iter + 2];
                        dst[dst_iter].w = 1.0f;
                    }
                    break;

                case R_pixel_format_r32g32b32a32:
                    RF_FOR_EACH_PIXEL {
                        dst[dst_iter].x = ((float *) src)[src_iter + 0];
                        dst[dst_iter].y = ((float *) src)[src_iter + 1];
                        dst[dst_iter].z = ((float *) src)[src_iter + 2];
                        dst[dst_iter].w = ((float *) src)[src_iter + 3];
                    }
                    break;

                default:
                    break;
            }
#undef RF_FOR_EACH_PIXEL
        }
    } else
        R_log_error(R_bad_buffer_size,
                    "Buffer is size %d but function expected a size of at least %d.", dst_size,
                    src_pixel_count * sizeof(R_vec4));

    return success;
}

R_public R_bool R_format_pixels_to_rgba32(const void *src, R_int src_size,
                                          R_uncompressed_pixel_format src_format, R_color *dst,
                                          R_int dst_size) {
    R_bool success = 0;

    R_int src_bpp = R_bytes_per_pixel(src_format);
    R_int src_pixel_count = src_size / src_bpp;
    R_int dst_pixel_count = dst_size / sizeof(R_color);

    if (dst_pixel_count >= src_pixel_count) {
        if (src_format == R_pixel_format_r8g8b8a8) {
            success = 1;
            memcpy(dst, src, src_size);
        } else {
            success = 1;
#define RF_FOR_EACH_PIXEL                                                                          \
    for (R_int dst_iter = 0, src_iter = 0; src_iter < src_size && dst_iter < dst_size;             \
         dst_iter++, src_iter += src_bpp)
            switch (src_format) {
                case R_pixel_format_grayscale:
                    RF_FOR_EACH_PIXEL {
                        unsigned char value = ((unsigned char *) src)[src_iter];
                        dst[dst_iter].r = value;
                        dst[dst_iter].g = value;
                        dst[dst_iter].b = value;
                        dst[dst_iter].a = 255;
                    }
                    break;

                case R_pixel_format_gray_alpha:
                    RF_FOR_EACH_PIXEL {
                        unsigned char value0 = ((unsigned char *) src)[src_iter + 0];
                        unsigned char value1 = ((unsigned char *) src)[src_iter + 1];

                        dst[dst_iter].r = value0;
                        dst[dst_iter].g = value0;
                        dst[dst_iter].b = value0;
                        dst[dst_iter].a = value1;
                    }
                    break;

                case R_pixel_format_r5g5b5a1:
                    RF_FOR_EACH_PIXEL {
                        unsigned short pixel = ((unsigned short *) src)[src_iter];

                        dst[dst_iter].r =
                                (unsigned char) ((float) ((pixel & 0b1111100000000000) >> 11) *
                                                 (255 / 31));
                        dst[dst_iter].g =
                                (unsigned char) ((float) ((pixel & 0b0000011111000000) >> 6) *
                                                 (255 / 31));
                        dst[dst_iter].b =
                                (unsigned char) ((float) ((pixel & 0b0000000000111110) >> 1) *
                                                 (255 / 31));
                        dst[dst_iter].a = (unsigned char) ((pixel & 0b0000000000000001) * 255);
                    }
                    break;

                case R_pixel_format_r5g6b5:
                    RF_FOR_EACH_PIXEL {
                        unsigned short pixel = ((unsigned short *) src)[src_iter];

                        dst[dst_iter].r =
                                (unsigned char) ((float) ((pixel & 0b1111100000000000) >> 11) *
                                                 (255 / 31));
                        dst[dst_iter].g =
                                (unsigned char) ((float) ((pixel & 0b0000011111100000) >> 5) *
                                                 (255 / 63));
                        dst[dst_iter].b =
                                (unsigned char) ((float) (pixel & 0b0000000000011111) * (255 / 31));
                        dst[dst_iter].a = 255;
                    }
                    break;

                case R_pixel_format_r4g4b4a4:
                    RF_FOR_EACH_PIXEL {
                        unsigned short pixel = ((unsigned short *) src)[src_iter];

                        dst[dst_iter].r =
                                (unsigned char) ((float) ((pixel & 0b1111000000000000) >> 12) *
                                                 (255 / 15));
                        dst[dst_iter].g =
                                (unsigned char) ((float) ((pixel & 0b0000111100000000) >> 8) *
                                                 (255 / 15));
                        dst[dst_iter].b =
                                (unsigned char) ((float) ((pixel & 0b0000000011110000) >> 4) *
                                                 (255 / 15));
                        dst[dst_iter].a =
                                (unsigned char) ((float) (pixel & 0b0000000000001111) * (255 / 15));
                    }
                    break;

                case R_pixel_format_r8g8b8a8:
                    RF_FOR_EACH_PIXEL {
                        dst[dst_iter].r = ((unsigned char *) src)[src_iter + 0];
                        dst[dst_iter].g = ((unsigned char *) src)[src_iter + 1];
                        dst[dst_iter].b = ((unsigned char *) src)[src_iter + 2];
                        dst[dst_iter].a = ((unsigned char *) src)[src_iter + 3];
                    }
                    break;

                case R_pixel_format_r8g8b8:
                    RF_FOR_EACH_PIXEL {
                        dst[dst_iter].r = (unsigned char) ((unsigned char *) src)[src_iter + 0];
                        dst[dst_iter].g = (unsigned char) ((unsigned char *) src)[src_iter + 1];
                        dst[dst_iter].b = (unsigned char) ((unsigned char *) src)[src_iter + 2];
                        dst[dst_iter].a = 255;
                    }
                    break;

                case R_pixel_format_r32:
                    RF_FOR_EACH_PIXEL {
                        dst[dst_iter].r = (unsigned char) (((float *) src)[src_iter + 0] * 255.0f);
                        dst[dst_iter].g = 0;
                        dst[dst_iter].b = 0;
                        dst[dst_iter].a = 255;
                    }
                    break;

                case R_pixel_format_r32g32b32:
                    RF_FOR_EACH_PIXEL {
                        dst[dst_iter].r = (unsigned char) (((float *) src)[src_iter + 0] * 255.0f);
                        dst[dst_iter].g = (unsigned char) (((float *) src)[src_iter + 1] * 255.0f);
                        dst[dst_iter].b = (unsigned char) (((float *) src)[src_iter + 2] * 255.0f);
                        dst[dst_iter].a = 255;
                    }
                    break;

                case R_pixel_format_r32g32b32a32:
                    RF_FOR_EACH_PIXEL {
                        dst[dst_iter].r = (unsigned char) (((float *) src)[src_iter + 0] * 255.0f);
                        dst[dst_iter].g = (unsigned char) (((float *) src)[src_iter + 1] * 255.0f);
                        dst[dst_iter].b = (unsigned char) (((float *) src)[src_iter + 2] * 255.0f);
                        dst[dst_iter].a = (unsigned char) (((float *) src)[src_iter + 3] * 255.0f);
                    }
                    break;

                default:
                    break;
            }
#undef RF_FOR_EACH_PIXEL
        }
    } else
        R_log_error(R_bad_buffer_size,
                    "Buffer is size %d but function expected a size of at least %d", dst_size,
                    src_pixel_count * sizeof(R_color));

    return success;
}

R_public R_bool R_format_pixels(const void *src, R_int src_size,
                                R_uncompressed_pixel_format src_format, void *dst, R_int dst_size,
                                R_uncompressed_pixel_format dst_format) {
    R_bool success = 0;

    if (R_is_uncompressed_format(src_format) && dst_format == R_pixel_format_r32g32b32a32) {
        success = R_format_pixels_to_normalized(src, src_size, src_format, dst, dst_size);
    } else if (R_is_uncompressed_format(src_format) && dst_format == R_pixel_format_r8g8b8a8) {
        success = R_format_pixels_to_rgba32(src, src_size, src_format, dst, dst_size);
    } else if (R_is_uncompressed_format(src_format) && R_is_uncompressed_format(dst_format)) {
        R_int src_bpp = R_bytes_per_pixel(src_format);
        R_int dst_bpp = R_bytes_per_pixel(dst_format);

        R_int src_pixel_count = src_size / src_bpp;
        R_int dst_pixel_count = dst_size / dst_bpp;

        if (dst_pixel_count >= src_pixel_count) {
            success = 1;

//Loop over both src and dst
#define RF_FOR_EACH_PIXEL                                                                          \
    for (R_int src_iter = 0, dst_iter = 0; src_iter < src_size && dst_iter < dst_size;             \
         src_iter += src_bpp, dst_iter += dst_bpp)
#define RF_COMPUTE_NORMALIZED_PIXEL()                                                              \
    R_format_one_pixel_to_normalized(((unsigned char *) src) + src_iter, src_format);
            if (src_format == dst_format) {
                memcpy(dst, src, src_size);
            } else {
                switch (dst_format) {
                    case R_pixel_format_grayscale:
                        RF_FOR_EACH_PIXEL {
                            R_vec4 normalized = RF_COMPUTE_NORMALIZED_PIXEL();
                            ((unsigned char *) dst)[dst_iter] =
                                    (unsigned char) ((normalized.x * 0.299f +
                                                      normalized.y * 0.587f +
                                                      normalized.z * 0.114f) *
                                                     255.0f);
                        }
                        break;

                    case R_pixel_format_gray_alpha:
                        RF_FOR_EACH_PIXEL {
                            R_vec4 normalized = RF_COMPUTE_NORMALIZED_PIXEL();

                            ((unsigned char *) dst)[dst_iter] =
                                    (unsigned char) ((normalized.x * 0.299f +
                                                      (float) normalized.y * 0.587f +
                                                      (float) normalized.z * 0.114f) *
                                                     255.0f);
                            ((unsigned char *) dst)[dst_iter + 1] =
                                    (unsigned char) (normalized.w * 255.0f);
                        }
                        break;

                    case R_pixel_format_r5g6b5:
                        RF_FOR_EACH_PIXEL {
                            R_vec4 normalized = RF_COMPUTE_NORMALIZED_PIXEL();

                            unsigned char r = (unsigned char) (round(normalized.x * 31.0f));
                            unsigned char g = (unsigned char) (round(normalized.y * 63.0f));
                            unsigned char b = (unsigned char) (round(normalized.z * 31.0f));

                            ((unsigned short *) dst)[dst_iter] = (unsigned short) r << 11 |
                                                                 (unsigned short) g << 5 |
                                                                 (unsigned short) b;
                        }
                        break;

                    case R_pixel_format_r8g8b8:
                        RF_FOR_EACH_PIXEL {
                            R_vec4 normalized = RF_COMPUTE_NORMALIZED_PIXEL();

                            ((unsigned char *) dst)[dst_iter] =
                                    (unsigned char) (normalized.x * 255.0f);
                            ((unsigned char *) dst)[dst_iter + 1] =
                                    (unsigned char) (normalized.y * 255.0f);
                            ((unsigned char *) dst)[dst_iter + 2] =
                                    (unsigned char) (normalized.z * 255.0f);
                        }
                        break;

                    case R_pixel_format_r5g5b5a1:
                        RF_FOR_EACH_PIXEL {
                            R_vec4 normalized = RF_COMPUTE_NORMALIZED_PIXEL();

                            int ALPHA_THRESHOLD = 50;
                            unsigned char r = (unsigned char) (round(normalized.x * 31.0f));
                            unsigned char g = (unsigned char) (round(normalized.y * 31.0f));
                            unsigned char b = (unsigned char) (round(normalized.z * 31.0f));
                            unsigned char a =
                                    (normalized.w > ((float) ALPHA_THRESHOLD / 255.0f)) ? 1 : 0;

                            ((unsigned short *) dst)[dst_iter] =
                                    (unsigned short) r << 11 | (unsigned short) g << 6 |
                                    (unsigned short) b << 1 | (unsigned short) a;
                        }
                        break;

                    case R_pixel_format_r4g4b4a4:
                        RF_FOR_EACH_PIXEL {
                            R_vec4 normalized = RF_COMPUTE_NORMALIZED_PIXEL();

                            unsigned char r = (unsigned char) (round(normalized.x * 15.0f));
                            unsigned char g = (unsigned char) (round(normalized.y * 15.0f));
                            unsigned char b = (unsigned char) (round(normalized.z * 15.0f));
                            unsigned char a = (unsigned char) (round(normalized.w * 15.0f));

                            ((unsigned short *) dst)[dst_iter] =
                                    (unsigned short) r << 12 | (unsigned short) g << 8 |
                                    (unsigned short) b << 4 | (unsigned short) a;
                        }
                        break;

                    case R_pixel_format_r32:
                        RF_FOR_EACH_PIXEL {
                            R_vec4 normalized = RF_COMPUTE_NORMALIZED_PIXEL();

                            ((float *) dst)[dst_iter] =
                                    (float) (normalized.x * 0.299f + normalized.y * 0.587f +
                                             normalized.z * 0.114f);
                        }
                        break;

                    case R_pixel_format_r32g32b32:
                        RF_FOR_EACH_PIXEL {
                            R_vec4 normalized = RF_COMPUTE_NORMALIZED_PIXEL();

                            ((float *) dst)[dst_iter] = normalized.x;
                            ((float *) dst)[dst_iter + 1] = normalized.y;
                            ((float *) dst)[dst_iter + 2] = normalized.z;
                        }
                        break;

                    default:
                        break;
                }
            }
#undef RF_FOR_EACH_PIXEL
#undef RF_COMPUTE_NORMALIZED_PIXEL
        } else
            R_log_error(R_bad_buffer_size,
                        "Buffer is size %d but function expected a size of at least %d.", dst_size,
                        src_pixel_count * dst_bpp);
    } else
        R_log_error(R_bad_argument,
                    "Function expected uncompressed pixel formats. Source format: %d, Destination "
                    "format: %d.",
                    src_format, dst_format);

    return success;
}

R_public R_vec4 R_format_one_pixel_to_normalized(const void *src,
                                                 R_uncompressed_pixel_format src_format) {
    R_vec4 result = {0};

    switch (src_format) {
        case R_pixel_format_grayscale: {
            float value = ((unsigned char *) src)[0] / 255.0f;

            result.x = value;
            result.y = value;
            result.z = value;
            result.w = 1.0f;
        } break;

        case R_pixel_format_gray_alpha: {
            float value0 = (float) ((unsigned char *) src)[0] / 255.0f;
            float value1 = (float) ((unsigned char *) src)[1] / 255.0f;

            result.x = value0;
            result.y = value0;
            result.z = value0;
            result.w = value1;
        } break;

        case R_pixel_format_r5g5b5a1: {
            unsigned short pixel = ((unsigned short *) src)[0];

            result.x = (float) ((pixel & 0b1111100000000000) >> 11) * (1.0f / 31);
            result.y = (float) ((pixel & 0b0000011111000000) >> 6) * (1.0f / 31);
            result.z = (float) ((pixel & 0b0000000000111110) >> 1) * (1.0f / 31);
            result.w = ((pixel & 0b0000000000000001) == 0) ? 0.0f : 1.0f;
        } break;

        case R_pixel_format_r5g6b5: {
            unsigned short pixel = ((unsigned short *) src)[0];

            result.x = (float) ((pixel & 0b1111100000000000) >> 11) * (1.0f / 31);
            result.y = (float) ((pixel & 0b0000011111100000) >> 5) * (1.0f / 63);
            result.z = (float) (pixel & 0b0000000000011111) * (1.0f / 31);
            result.w = 1.0f;
        } break;

        case R_pixel_format_r4g4b4a4: {
            unsigned short pixel = ((unsigned short *) src)[0];

            result.x = (float) ((pixel & 0b1111000000000000) >> 12) * (1.0f / 15);
            result.y = (float) ((pixel & 0b0000111100000000) >> 8) * (1.0f / 15);
            result.z = (float) ((pixel & 0b0000000011110000) >> 4) * (1.0f / 15);
            result.w = (float) (pixel & 0b0000000000001111) * (1.0f / 15);
        } break;

        case R_pixel_format_r8g8b8a8: {
            result.x = (float) ((unsigned char *) src)[0] / 255.0f;
            result.y = (float) ((unsigned char *) src)[1] / 255.0f;
            result.z = (float) ((unsigned char *) src)[2] / 255.0f;
            result.w = (float) ((unsigned char *) src)[3] / 255.0f;
        } break;

        case R_pixel_format_r8g8b8: {
            result.x = (float) ((unsigned char *) src)[0] / 255.0f;
            result.y = (float) ((unsigned char *) src)[1] / 255.0f;
            result.z = (float) ((unsigned char *) src)[2] / 255.0f;
            result.w = 1.0f;
        } break;

        case R_pixel_format_r32: {
            result.x = ((float *) src)[0];
            result.y = 0.0f;
            result.z = 0.0f;
            result.w = 1.0f;
        } break;

        case R_pixel_format_r32g32b32: {
            result.x = ((float *) src)[0];
            result.y = ((float *) src)[1];
            result.z = ((float *) src)[2];
            result.w = 1.0f;
        } break;

        case R_pixel_format_r32g32b32a32: {
            result.x = ((float *) src)[0];
            result.y = ((float *) src)[1];
            result.z = ((float *) src)[2];
            result.w = ((float *) src)[3];
        } break;

        default:
            break;
    }

    return result;
}

R_public R_color R_format_one_pixel_to_rgba32(const void *src,
                                              R_uncompressed_pixel_format src_format) {
    R_color result = {0};

    switch (src_format) {
        case R_pixel_format_grayscale: {
            unsigned char value = ((unsigned char *) src)[0];
            result.r = value;
            result.g = value;
            result.b = value;
            result.a = 255;
        } break;

        case R_pixel_format_gray_alpha: {
            unsigned char value0 = ((unsigned char *) src)[0];
            unsigned char value1 = ((unsigned char *) src)[1];

            result.r = value0;
            result.g = value0;
            result.b = value0;
            result.a = value1;
        } break;

        case R_pixel_format_r5g5b5a1: {
            unsigned short pixel = ((unsigned short *) src)[0];

            result.r = (unsigned char) ((float) ((pixel & 0b1111100000000000) >> 11) * (255 / 31));
            result.g = (unsigned char) ((float) ((pixel & 0b0000011111000000) >> 6) * (255 / 31));
            result.b = (unsigned char) ((float) ((pixel & 0b0000000000111110) >> 1) * (255 / 31));
            result.a = (unsigned char) ((pixel & 0b0000000000000001) * 255);
        } break;

        case R_pixel_format_r5g6b5: {
            unsigned short pixel = ((unsigned short *) src)[0];

            result.r = (unsigned char) ((float) ((pixel & 0b1111100000000000) >> 11) * (255 / 31));
            result.g = (unsigned char) ((float) ((pixel & 0b0000011111100000) >> 5) * (255 / 63));
            result.b = (unsigned char) ((float) (pixel & 0b0000000000011111) * (255 / 31));
            result.a = 255;
        } break;

        case R_pixel_format_r4g4b4a4: {
            unsigned short pixel = ((unsigned short *) src)[0];

            result.r = (unsigned char) ((float) ((pixel & 0b1111000000000000) >> 12) * (255 / 15));
            result.g = (unsigned char) ((float) ((pixel & 0b0000111100000000) >> 8) * (255 / 15));
            result.b = (unsigned char) ((float) ((pixel & 0b0000000011110000) >> 4) * (255 / 15));
            result.a = (unsigned char) ((float) (pixel & 0b0000000000001111) * (255 / 15));
        } break;

        case R_pixel_format_r8g8b8a8: {
            result.r = ((unsigned char *) src)[0];
            result.g = ((unsigned char *) src)[1];
            result.b = ((unsigned char *) src)[2];
            result.a = ((unsigned char *) src)[3];
        } break;

        case R_pixel_format_r8g8b8: {
            result.r = (unsigned char) ((unsigned char *) src)[0];
            result.g = (unsigned char) ((unsigned char *) src)[1];
            result.b = (unsigned char) ((unsigned char *) src)[2];
            result.a = 255;
        } break;

        case R_pixel_format_r32: {
            result.r = (unsigned char) (((float *) src)[0] * 255.0f);
            result.g = 0;
            result.b = 0;
            result.a = 255;
        } break;

        case R_pixel_format_r32g32b32: {
            result.r = (unsigned char) (((float *) src)[0] * 255.0f);
            result.g = (unsigned char) (((float *) src)[1] * 255.0f);
            result.b = (unsigned char) (((float *) src)[2] * 255.0f);
            result.a = 255;
        } break;

        case R_pixel_format_r32g32b32a32: {
            result.r = (unsigned char) (((float *) src)[0] * 255.0f);
            result.g = (unsigned char) (((float *) src)[1] * 255.0f);
            result.b = (unsigned char) (((float *) src)[2] * 255.0f);
            result.a = (unsigned char) (((float *) src)[3] * 255.0f);
        } break;

        default:
            break;
    }

    return result;
}

R_public void R_format_one_pixel(const void *src, R_uncompressed_pixel_format src_format, void *dst,
                                 R_uncompressed_pixel_format dst_format) {
    if (src_format == dst_format && R_is_uncompressed_format(src_format) &&
        R_is_uncompressed_format(dst_format)) {
        memcpy(dst, src, R_bytes_per_pixel(src_format));
    } else if (R_is_uncompressed_format(src_format) && dst_format == R_pixel_format_r32g32b32a32) {
        *((R_vec4 *) dst) = R_format_one_pixel_to_normalized(src, src_format);
    } else if (R_is_uncompressed_format(src_format) && dst_format == R_pixel_format_r8g8b8a8) {
        *((R_color *) dst) = R_format_one_pixel_to_rgba32(src, src_format);
    } else if (R_is_uncompressed_format(src_format) && R_is_uncompressed_format(dst_format)) {
        switch (dst_format) {
            case R_pixel_format_grayscale: {
                R_vec4 normalized = R_format_one_pixel_to_normalized(src, src_format);
                ((unsigned char *) dst)[0] =
                        (unsigned char) ((normalized.x * 0.299f + normalized.y * 0.587f +
                                          normalized.z * 0.114f) *
                                         255.0f);
            } break;

            case R_pixel_format_gray_alpha: {
                R_vec4 normalized = R_format_one_pixel_to_normalized(src, src_format);
                ((unsigned char *) dst)[0] =
                        (unsigned char) ((normalized.x * 0.299f + (float) normalized.y * 0.587f +
                                          (float) normalized.z * 0.114f) *
                                         255.0f);
                ((unsigned char *) dst)[0 + 1] = (unsigned char) (normalized.w * 255.0f);
            } break;

            case R_pixel_format_r5g6b5: {
                R_vec4 normalized = R_format_one_pixel_to_normalized(src, src_format);
                unsigned char r = (unsigned char) (round(normalized.x * 31.0f));
                unsigned char g = (unsigned char) (round(normalized.y * 63.0f));
                unsigned char b = (unsigned char) (round(normalized.z * 31.0f));

                ((unsigned short *) dst)[0] =
                        (unsigned short) r << 11 | (unsigned short) g << 5 | (unsigned short) b;
            } break;

            case R_pixel_format_r8g8b8: {
                R_vec4 normalized = R_format_one_pixel_to_normalized(src, src_format);
                ((unsigned char *) dst)[0] = (unsigned char) (normalized.x * 255.0f);
                ((unsigned char *) dst)[0 + 1] = (unsigned char) (normalized.y * 255.0f);
                ((unsigned char *) dst)[0 + 2] = (unsigned char) (normalized.z * 255.0f);
            } break;

            case R_pixel_format_r5g5b5a1: {
                R_vec4 normalized = R_format_one_pixel_to_normalized(src, src_format);
                int ALPHA_THRESHOLD = 50;
                unsigned char r = (unsigned char) (round(normalized.x * 31.0f));
                unsigned char g = (unsigned char) (round(normalized.y * 31.0f));
                unsigned char b = (unsigned char) (round(normalized.z * 31.0f));
                unsigned char a = (normalized.w > ((float) ALPHA_THRESHOLD / 255.0f)) ? 1 : 0;

                ((unsigned short *) dst)[0] = (unsigned short) r << 11 | (unsigned short) g << 6 |
                                              (unsigned short) b << 1 | (unsigned short) a;
            } break;

            case R_pixel_format_r4g4b4a4: {
                R_vec4 normalized = R_format_one_pixel_to_normalized(src, src_format);
                unsigned char r = (unsigned char) (round(normalized.x * 15.0f));
                unsigned char g = (unsigned char) (round(normalized.y * 15.0f));
                unsigned char b = (unsigned char) (round(normalized.z * 15.0f));
                unsigned char a = (unsigned char) (round(normalized.w * 15.0f));

                ((unsigned short *) dst)[0] = (unsigned short) r << 12 | (unsigned short) g << 8 |
                                              (unsigned short) b << 4 | (unsigned short) a;
            } break;

            case R_pixel_format_r8g8b8a8: {
                R_vec4 normalized = R_format_one_pixel_to_normalized(src, src_format);
                ((unsigned char *) dst)[0] = (unsigned char) (normalized.x * 255.0f);
                ((unsigned char *) dst)[0 + 1] = (unsigned char) (normalized.y * 255.0f);
                ((unsigned char *) dst)[0 + 2] = (unsigned char) (normalized.z * 255.0f);
                ((unsigned char *) dst)[0 + 3] = (unsigned char) (normalized.w * 255.0f);
            } break;

            case R_pixel_format_r32: {
                R_vec4 normalized = R_format_one_pixel_to_normalized(src, src_format);
                ((float *) dst)[0] = (float) (normalized.x * 0.299f + normalized.y * 0.587f +
                                              normalized.z * 0.114f);
            } break;

            case R_pixel_format_r32g32b32: {
                R_vec4 normalized = R_format_one_pixel_to_normalized(src, src_format);
                ((float *) dst)[0] = normalized.x;
                ((float *) dst)[0 + 1] = normalized.y;
                ((float *) dst)[0 + 2] = normalized.z;
            } break;

            case R_pixel_format_r32g32b32a32: {
                R_vec4 normalized = R_format_one_pixel_to_normalized(src, src_format);
                ((float *) dst)[0] = normalized.x;
                ((float *) dst)[0 + 1] = normalized.y;
                ((float *) dst)[0 + 2] = normalized.z;
                ((float *) dst)[0 + 3] = normalized.w;
            } break;

            default:
                break;
        }
    }
}

#pragma endregion

#pragma region color

// Returns 1 if the two colors have the same values for the rgb components
R_public R_bool R_color_match_rgb(R_color a, R_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

// Returns 1 if the two colors have the same values
R_public R_bool R_color_match(R_color a, R_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

// Returns hexadecimal value for a R_color
R_public int R_color_to_int(R_color color) {
    return (((int) color.r << 24) | ((int) color.g << 16) | ((int) color.b << 8) | (int) color.a);
}

// Returns color normalized as float [0..1]
R_public R_vec4 R_color_normalize(R_color color) {
    R_vec4 result;

    result.x = (float) color.r / 255.0f;
    result.y = (float) color.g / 255.0f;
    result.z = (float) color.b / 255.0f;
    result.w = (float) color.a / 255.0f;

    return result;
}

// Returns color from normalized values [0..1]
R_public R_color R_color_from_normalized(R_vec4 normalized) {
    R_color result;

    result.r = normalized.x * 255.0f;
    result.g = normalized.y * 255.0f;
    result.b = normalized.z * 255.0f;
    result.a = normalized.w * 255.0f;

    return result;
}

// Returns HSV values for a R_color. Hue is returned as degrees [0..360]
R_public R_vec3 R_color_to_hsv(R_color color) {
    R_vec3 rgb = {(float) color.r / 255.0f, (float) color.g / 255.0f, (float) color.b / 255.0f};
    R_vec3 hsv = {0.0f, 0.0f, 0.0f};
    float min, max, delta;

    min = rgb.x < rgb.y ? rgb.x : rgb.y;
    min = min < rgb.z ? min : rgb.z;

    max = rgb.x > rgb.y ? rgb.x : rgb.y;
    max = max > rgb.z ? max : rgb.z;

    hsv.z = max;// Value
    delta = max - min;

    if (delta < 0.00001f) {
        hsv.y = 0.0f;
        hsv.x = 0.0f;// Undefined, maybe NAN?
        return hsv;
    }

    if (max > 0.0f) {
        // NOTE: If max is 0, this divide would cause a crash
        hsv.y = (delta / max);// Saturation
    } else {
        // NOTE: If max is 0, then r = g = b = 0, s = 0, h is undefined
        hsv.y = 0.0f;
        hsv.x = NAN;// Undefined
        return hsv;
    }

    // NOTE: Comparing float values could not work properly
    if (rgb.x >= max) hsv.x = (rgb.y - rgb.z) / delta;// Between yellow & magenta
    else {
        if (rgb.y >= max) hsv.x = 2.0f + (rgb.z - rgb.x) / delta;// Between cyan & yellow
        else
            hsv.x = 4.0f + (rgb.x - rgb.y) / delta;// Between magenta & cyan
    }

    hsv.x *= 60.0f;// Convert to degrees

    if (hsv.x < 0.0f) hsv.x += 360.0f;

    return hsv;
}

// Returns a R_color from HSV values. R_color->HSV->R_color conversion will not yield exactly the same color due to rounding errors. Implementation reference: https://en.wikipedia.org/wiki/HSL_and_HSV#Alternative_HSV_conversion
R_public R_color R_color_from_hsv(R_vec3 hsv) {
    R_color color = {0, 0, 0, 255};
    float h = hsv.x, s = hsv.y, v = hsv.z;

    // Red channel
    float k = fmod((5.0f + h / 60.0f), 6);
    float t = 4.0f - k;
    k = (t < k) ? t : k;
    k = (k < 1) ? k : 1;
    k = (k > 0) ? k : 0;
    color.r = (v - v * s * k) * 255;

    // Green channel
    k = fmod((3.0f + h / 60.0f), 6);
    t = 4.0f - k;
    k = (t < k) ? t : k;
    k = (k < 1) ? k : 1;
    k = (k > 0) ? k : 0;
    color.g = (v - v * s * k) * 255;

    // Blue channel
    k = fmod((1.0f + h / 60.0f), 6);
    t = 4.0f - k;
    k = (t < k) ? t : k;
    k = (k < 1) ? k : 1;
    k = (k > 0) ? k : 0;
    color.b = (v - v * s * k) * 255;

    return color;
}

// Returns a R_color struct from hexadecimal value
R_public R_color R_color_from_int(int hex_value) {
    R_color color;

    color.r = (unsigned char) (hex_value >> 24) & 0xFF;
    color.g = (unsigned char) (hex_value >> 16) & 0xFF;
    color.b = (unsigned char) (hex_value >> 8) & 0xFF;
    color.a = (unsigned char) hex_value & 0xFF;

    return color;
}

// R_color fade-in or fade-out, alpha goes from 0.0f to 1.0f
R_public R_color R_fade(R_color color, float alpha) {
    if (alpha < 0.0f) alpha = 0.0f;
    else if (alpha > 1.0f)
        alpha = 1.0f;

    return (R_color){color.r, color.g, color.b, (unsigned char) (255.0f * alpha)};
}

#pragma endregion

#pragma region dependencies

#pragma region stb_image
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC(sz) R_alloc(R__global_allocator_for_dependencies, sz)
#define STBI_FREE(p) R_free(R__global_allocator_for_dependencies, p)
#define STBI_REALLOC_SIZED(p, oldsz, newsz)                                                        \
    R_default_realloc(R__global_allocator_for_dependencies, p, oldsz, newsz)
#define STBI_ASSERT(it) R_assert(it)
#define STBIDEF R_internal
#include "Libs/external/stb_image.h"
#pragma endregion

#pragma region stb_image_resize
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STBIR_MALLOC(sz, c) ((void) (c), R_alloc(R__global_allocator_for_dependencies, sz))
#define STBIR_FREE(p, c) ((void) (c), R_free(R__global_allocator_for_dependencies, p))
#define STBIR_ASSERT(it) R_assert(it)
#define STBIRDEF R_internal
#include "Libs/external/stb_image_resize.h"
#pragma endregion

#pragma region stb_rect_pack
#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_ASSERT R_assert
#define STBRP_STATIC
#include "Libs/external/stb_rect_pack.h"
#pragma endregion

#pragma region stb_perlin
#define STB_PERLIN_IMPLEMENTATION
#define STBPDEF R_internal
#include "Libs/external/stb_perlin.h"
#pragma endregion

#pragma endregion

#pragma region extract image data functions

R_public int R_image_size(R_image image) {
    return R_pixel_buffer_size(image.width, image.height, image.format);
}

R_public int R_image_size_in_format(R_image image, R_pixel_format format) {
    return image.width * image.height * R_bytes_per_pixel(format);
}

R_public R_bool R_image_get_pixels_as_rgba32_to_buffer(R_image image, R_color *dst,
                                                       R_int dst_size) {
    R_bool success = 0;

    if (R_is_uncompressed_format(image.format)) {
        if (image.format == R_pixel_format_r32 || image.format == R_pixel_format_r32g32b32 ||
            image.format == R_pixel_format_r32g32b32a32) {
            R_log(R_log_type_warning, "32bit pixel format converted to 8bit per channel.");
        }

        success = R_format_pixels_to_rgba32(image.data, R_image_size(image), image.format, dst,
                                            dst_size);
    } else
        R_log_error(R_bad_argument,
                    "Function only works for uncompressed formats but was called with format %d.",
                    image.format);

    return success;
}

R_public R_bool R_image_get_pixels_as_normalized_to_buffer(R_image image, R_vec4 *dst,
                                                           R_int dst_size) {
    R_bool success = 0;

    if (R_is_uncompressed_format(image.format)) {
        if ((image.format == R_pixel_format_r32) || (image.format == R_pixel_format_r32g32b32) ||
            (image.format == R_pixel_format_r32g32b32a32)) {
            R_log(R_log_type_warning, "32bit pixel format converted to 8bit per channel");
        }

        success = R_format_pixels_to_normalized(image.data, R_image_size(image),
                                                R_pixel_format_r32g32b32a32, dst, dst_size);
    } else
        R_log_error(R_bad_argument,
                    "Function only works for uncompressed formats but was called with format %d.",
                    image.format);

    return success;
}

// Get pixel data from image in the form of R_color struct array
R_public R_color *R_image_pixels_to_rgba32(R_image image, R_allocator allocator) {
    R_color *result = NULL;

    if (R_is_uncompressed_format(image.format)) {
        int size = image.width * image.height * sizeof(R_color);
        result = R_alloc(allocator, size);

        if (result) {
            R_bool success = R_image_get_pixels_as_rgba32_to_buffer(image, result, size);
            R_assert(success);
        } else
            R_log_error(R_bad_alloc, "Allocation of size %d failed.", size);
    } else
        R_log_error(R_bad_argument,
                    "Function only works for uncompressed formats but was called with format %d.",
                    image.format);

    return result;
}

// Get pixel data from image as R_vec4 array (float normalized)
R_public R_vec4 *R_image_compute_pixels_to_normalized(R_image image, R_allocator allocator) {
    R_vec4 *result = NULL;

    if (R_is_compressed_format(image.format)) {
        int size = image.width * image.height * sizeof(R_color);
        result = R_alloc(allocator, size);

        if (result) {
            R_bool success = R_image_get_pixels_as_normalized_to_buffer(image, result, size);
            R_assert(success);
        } else
            R_log_error(R_bad_alloc, "Allocation of size %d failed.", size);
    } else
        R_log_error(R_bad_argument,
                    "Function only works for uncompressed formats but was called with format %d.",
                    image.format);

    return result;
}

// Extract color palette from image to maximum size
R_public void R_image_extract_palette_to_buffer(R_image image, R_color *palette_dst,
                                                R_int palette_size) {
    if (R_is_uncompressed_format(image.format)) {
        if (palette_size > 0) {
            int img_size = R_image_size(image);
            int img_bpp = R_bytes_per_pixel(image.format);
            const unsigned char *img_data = (unsigned char *) image.data;

            for (R_int img_iter = 0, palette_iter = 0;
                 img_iter < img_size && palette_iter < palette_size; img_iter += img_bpp) {
                R_color color = R_format_one_pixel_to_rgba32(img_data, image.format);

                R_bool color_found = 0;

                for (R_int i = 0; i < palette_iter; i++) {
                    if (R_color_match(palette_dst[i], color)) {
                        color_found = 1;
                        break;
                    }
                }

                if (!color_found) {
                    palette_dst[palette_iter] = color;
                    palette_iter++;
                }
            }
        } else
            R_log(R_log_type_warning, "Palette size was 0.");
    } else
        R_log_error(R_bad_argument,
                    "Function only works for uncompressed formats but was called with format %d.",
                    image.format);
}

R_public R_palette R_image_extract_palette(R_image image, R_int palette_size,
                                           R_allocator allocator) {
    R_palette result = {0};

    if (R_is_uncompressed_format(image.format)) {
        R_color *dst = R_alloc(allocator, sizeof(R_color) * palette_size);
        R_image_extract_palette_to_buffer(image, dst, palette_size);

        result.colors = dst;
        result.count = palette_size;
    }

    return result;
}

// Get image alpha border rectangle
R_public R_rec R_image_alpha_border(R_image image, float threshold) {
    R_rec crop = {0};

    if (R_is_uncompressed_format(image.format)) {
        int x_min = 65536;// Define a big enough number
        int x_max = 0;
        int y_min = 65536;
        int y_max = 0;

        int src_bpp = R_bytes_per_pixel(image.format);
        int src_size = R_image_size(image);
        unsigned char *src = image.data;

        for (R_int y = 0; y < image.height; y++) {
            for (R_int x = 0; x < image.width; x++) {
                int src_pos = (y * image.width + x) * src_bpp;

                R_color rgba32_pixel = R_format_one_pixel_to_rgba32(&src[src_pos], image.format);

                if (rgba32_pixel.a > (unsigned char) (threshold * 255.0f)) {
                    if (x < x_min) x_min = x;
                    if (x > x_max) x_max = x;
                    if (y < y_min) y_min = y;
                    if (y > y_max) y_max = y;
                }
            }
        }

        crop = (R_rec){x_min, y_min, (x_max + 1) - x_min, (y_max + 1) - y_min};
    } else
        R_log_error(R_bad_argument,
                    "Function only works for uncompressed formats but was called with format %d.",
                    image.format);

    return crop;
}

#pragma endregion

#pragma region loading and unloading functions

R_public R_bool R_supports_image_file_type(const char *filename) {
    return R_is_file_extension(filename, ".png") || R_is_file_extension(filename, ".bmp") ||
           R_is_file_extension(filename, ".tga") || R_is_file_extension(filename, ".pic") ||
           R_is_file_extension(filename, ".psd") || R_is_file_extension(filename, ".hdr");
}

R_public R_image R_load_image_from_file_data_to_buffer(const void *src, R_int src_size, void *dst,
                                                       R_int dst_size, R_desired_channels channels,
                                                       R_allocator temp_allocator) {
    if (src == NULL || src_size <= 0) {
        R_log_error(R_bad_argument, "Argument `image` was invalid.");
        return (R_image){0};
    }

    R_image result = {0};

    int img_width = 0;
    int img_height = 0;
    int img_bpp = 0;

    R_set_global_dependencies_allocator(temp_allocator);
    void *output_buffer =
            stbi_load_from_memory(src, src_size, &img_width, &img_height, &img_bpp, channels);
    R_set_global_dependencies_allocator((R_allocator){0});

    if (output_buffer) {
        int output_buffer_size = img_width * img_height * img_bpp;

        if (dst_size >= output_buffer_size) {
            result.data = dst;
            result.width = img_width;
            result.height = img_height;
            result.valid = 1;

            switch (img_bpp) {
                case 1:
                    result.format = R_pixel_format_grayscale;
                    break;
                case 2:
                    result.format = R_pixel_format_gray_alpha;
                    break;
                case 3:
                    result.format = R_pixel_format_r8g8b8;
                    break;
                case 4:
                    result.format = R_pixel_format_r8g8b8a8;
                    break;
                default:
                    break;
            }

            memcpy(dst, output_buffer, output_buffer_size);
        } else
            R_log_error(R_bad_argument, "Buffer is not big enough", img_width, img_height, img_bpp);

        R_free(temp_allocator, output_buffer);
    } else
        R_log_error(R_stbi_failed,
                    "File format not supported or could not be loaded. STB Image returned { x: "
                    "%d, y: %d, channels: %d }",
                    img_width, img_height, img_bpp);

    return result;
}

R_public R_image R_load_image_from_file_data(const void *src, R_int src_size,
                                             R_desired_channels desired_channels,
                                             R_allocator allocator, R_allocator temp_allocator) {
    // Preconditions
    if (!src || src_size <= 0) {
        R_log_error(R_bad_argument, "Argument `src` was null.");
        return (R_image){0};
    }

    // Compute the result
    R_image result = {0};

    // Use stb image with the `temp_allocator` to decompress the image and get it's data
    int width = 0, height = 0, channels = 0;
    R_set_global_dependencies_allocator(temp_allocator);
    void *stbi_result =
            stbi_load_from_memory(src, src_size, &width, &height, &channels, desired_channels);
    R_set_global_dependencies_allocator((R_allocator){0});

    if (stbi_result && channels) {
        // Allocate a result buffer using the `allocator` and copy the data to it
        int stbi_result_size = width * height * channels;
        void *result_buffer = R_alloc(allocator, stbi_result_size);

        if (result_buffer) {
            result.data = result_buffer;
            result.width = width;
            result.height = height;
            result.valid = 1;

            // Set the format appropriately depending on the `channels` count
            switch (channels) {
                case 1:
                    result.format = R_pixel_format_grayscale;
                    break;
                case 2:
                    result.format = R_pixel_format_gray_alpha;
                    break;
                case 3:
                    result.format = R_pixel_format_r8g8b8;
                    break;
                case 4:
                    result.format = R_pixel_format_r8g8b8a8;
                    break;
                default:
                    break;
            }

            memcpy(result_buffer, stbi_result, stbi_result_size);
        } else
            R_log_error(R_bad_alloc, "Buffer is not big enough", width, height, channels);

        // Free the temp buffer allocated by stbi
        R_free(temp_allocator, stbi_result);
    } else
        R_log_error(R_stbi_failed,
                    "File format not supported or could not be loaded. STB Image returned { x: "
                    "%d, y: %d, channels: %d }",
                    width, height, channels);

    return result;
}

R_public R_image R_load_image_from_hdr_file_data_to_buffer(const void *src, R_int src_size,
                                                           void *dst, R_int dst_size,
                                                           R_desired_channels channels,
                                                           R_allocator temp_allocator) {
    R_image result = {0};

    if (src && src_size > 0) {
        int img_width = 0, img_height = 0, img_bpp = 0;

        // NOTE: Using stb_image to load images (Supports multiple image formats)
        R_set_global_dependencies_allocator(temp_allocator);
        void *output_buffer =
                stbi_load_from_memory(src, src_size, &img_width, &img_height, &img_bpp, channels);
        R_set_global_dependencies_allocator((R_allocator){0});

        if (output_buffer) {
            int output_buffer_size = img_width * img_height * img_bpp;

            if (dst_size >= output_buffer_size) {
                result.data = dst;
                result.width = img_width;
                result.height = img_height;
                result.valid = 1;

                if (img_bpp == 1) result.format = R_pixel_format_r32;
                else if (img_bpp == 3)
                    result.format = R_pixel_format_r32g32b32;
                else if (img_bpp == 4)
                    result.format = R_pixel_format_r32g32b32a32;

                memcpy(dst, output_buffer, output_buffer_size);
            } else
                R_log_error(R_bad_argument, "Buffer is not big enough", img_width, img_height,
                            img_bpp);

            R_free(temp_allocator, output_buffer);
        } else
            R_log_error(R_stbi_failed,
                        "File format not supported or could not be loaded. STB Image returned { "
                        "x: %d, y: %d, channels: %d }",
                        img_width, img_height, img_bpp);
    } else
        R_log_error(R_bad_argument, "Argument `image` was invalid.");

    return result;
}

R_public R_image R_load_image_from_hdr_file_data(const void *src, R_int src_size,
                                                 R_allocator allocator,
                                                 R_allocator temp_allocator) {
    R_image result = {0};

    if (src && src_size) {
        int width = 0, height = 0, bpp = 0;

        // NOTE: Using stb_image to load images (Supports multiple image formats)
        R_set_global_dependencies_allocator(temp_allocator);
        void *stbi_result =
                stbi_load_from_memory(src, src_size, &width, &height, &bpp, RF_ANY_CHANNELS);
        R_set_global_dependencies_allocator((R_allocator){0});

        if (stbi_result && bpp) {
            int stbi_result_size = width * height * bpp;
            void *result_buffer = R_alloc(allocator, stbi_result_size);

            if (result_buffer) {
                result.data = result_buffer;
                result.width = width;
                result.height = height;
                result.valid = 1;

                if (bpp == 1) result.format = R_pixel_format_r32;
                else if (bpp == 3)
                    result.format = R_pixel_format_r32g32b32;
                else if (bpp == 4)
                    result.format = R_pixel_format_r32g32b32a32;

                memcpy(result_buffer, stbi_result, stbi_result_size);
            } else
                R_log_error(R_bad_alloc, "Buffer is not big enough", width, height, bpp);

            R_free(temp_allocator, stbi_result);
        } else
            R_log_error(R_stbi_failed,
                        "File format not supported or could not be loaded. STB Image returned { "
                        "x: %d, y: %d, channels: %d }",
                        width, height, bpp);
    } else
        R_log_error(R_bad_argument, "Argument `image` was invalid.");

    return result;
}

R_public R_image R_load_image_from_format_to_buffer(const void *src, R_int src_size, int src_width,
                                                    int src_height,
                                                    R_uncompressed_pixel_format src_format,
                                                    void *dst, R_int dst_size,
                                                    R_uncompressed_pixel_format dst_format) {
    R_image result = {0};

    if (R_is_uncompressed_format(dst_format)) {
        result = (R_image){
                .data = dst,
                .width = src_width,
                .height = src_height,
                .format = dst_format,
        };

        R_bool success = R_format_pixels(src, src_size, src_format, dst, dst_size, dst_format);
        R_assert(success);
    }

    return result;
}

R_public R_image R_load_image_from_file(const char *filename, R_allocator allocator,
                                        R_allocator temp_allocator, R_io_callbacks io) {
    R_image image = {0};

    if (R_supports_image_file_type(filename)) {
        int file_size = R_file_size(io, filename);

        if (file_size > 0) {
            unsigned char *image_file_buffer = R_alloc(temp_allocator, file_size);

            if (image_file_buffer) {
                if (R_read_file(io, filename, image_file_buffer, file_size)) {
                    if (R_is_file_extension(filename, ".hdr")) {
                        image = R_load_image_from_hdr_file_data(image_file_buffer, file_size,
                                                                allocator, temp_allocator);
                    } else {
                        image = R_load_image_from_file_data(image_file_buffer, file_size,
                                                            RF_ANY_CHANNELS, allocator,
                                                            temp_allocator);
                    }
                } else
                    R_log_error(R_bad_io, "File size for %s is 0", filename);

                R_free(temp_allocator, image_file_buffer);
            } else
                R_log_error(R_bad_alloc, "Temporary allocation of size %d failed", file_size);
        } else
            R_log_error(R_bad_io, "File size for %s is 0", filename);
    } else
        R_log_error(R_unsupported, "Image fileformat not supported", filename);

    return image;
}

R_public void R_unload_image(R_image image, R_allocator allocator) {
    R_free(allocator, image.data);
}

#pragma endregion

#pragma region image manipulation

/**
 * Copy an existing image into a buffer.
 * @param image a valid image to copy from.
 * @param dst a buffer for the resulting image.
 * @param dst_size size of the `dst` buffer.
 * @return a deep copy of the image into the provided `dst` buffer.
 */
R_public R_image R_image_copy_to_buffer(R_image image, void *dst, R_int dst_size) {
    R_image result = {0};

    if (image.valid) {
        int width = image.width;
        int height = image.height;
        int size = width * height * R_bytes_per_pixel(image.format);

        if (dst_size >= size) {
            memcpy(dst, image.data, size);

            result.data = dst;
            result.width = image.width;
            result.height = image.height;
            result.format = image.format;
            result.valid = 1;
        } else
            R_log_error(R_bad_argument,
                        "Destination buffer is too small. Expected at least %d bytes but was %d",
                        size, dst_size);
    } else
        R_log_error(R_bad_argument, "Image was invalid.");

    return result;
}

/**
 * Copy an existing image.
 * @param image a valid image to copy from.
 * @param allocator used to allocate the new buffer for the resulting image.
 * @return a deep copy of the image.
 */
R_public R_image R_image_copy(R_image image, R_allocator allocator) {
    R_image result = {0};

    if (image.valid) {
        int size = image.width * image.height * R_bytes_per_pixel(image.format);
        void *dst = R_alloc(allocator, size);

        if (dst) {
            result = R_image_copy_to_buffer(image, dst, size);
        } else
            R_log_error(R_bad_alloc, "Failed to allocate %d bytes", size);
    } else
        R_log_error(R_bad_argument, "Image was invalid.");

    return result;
}

/**
 * Crop an image and store the result in a provided buffer.
 * @param image a valid image that we crop from.
 * @param crop a rectangle representing which part of the image to crop.
 * @param dst a buffer for the resulting image. Must be of size at least `R_pixel_buffer_size(RF_UNCOMPRESSED_R8G8B8A8, crop.width, crop.height)`.
 * @param dst_size size of the `dst` buffer.
 * @return a cropped image using the `dst` buffer in the same format as `image`.
 */
R_public R_image R_image_crop_to_buffer(R_image image, R_rec crop, void *dst, R_int dst_size,
                                        R_uncompressed_pixel_format dst_format) {
    R_image result = {0};

    if (image.valid) {
        // Security checks to validate crop rectangle
        if (crop.x < 0) {
            crop.width += crop.x;
            crop.x = 0;
        }
        if (crop.y < 0) {
            crop.height += crop.y;
            crop.y = 0;
        }
        if ((crop.x + crop.width) > image.width) { crop.width = image.width - crop.x; }
        if ((crop.y + crop.height) > image.height) { crop.height = image.height - crop.y; }

        if ((crop.x < image.width) && (crop.y < image.height)) {
            int expected_size = R_pixel_buffer_size(crop.width, crop.height, dst_format);
            if (dst_size >= expected_size) {
                R_pixel_format src_format = image.format;
                int src_size = R_image_size(image);

                unsigned char *src_ptr = image.data;
                unsigned char *dst_ptr = dst;

                int src_bpp = R_bytes_per_pixel(image.format);
                int dst_bpp = R_bytes_per_pixel(dst_format);

                int crop_y = crop.y;
                int crop_h = crop.height;
                int crop_x = crop.x;
                int crop_w = crop.width;

                for (R_int y = 0; y < crop_h; y++) {
                    for (R_int x = 0; x < crop_w; x++) {
                        int src_x = x + crop_x;
                        int src_y = y + crop_y;

                        int src_pixel = (src_y * image.width + src_x) * src_bpp;
                        int dst_pixel = (y * crop_w + x) * src_bpp;
                        R_assert(src_pixel < src_size);
                        R_assert(dst_pixel < dst_size);

                        R_format_one_pixel(&src_ptr[src_pixel], src_format, &dst_ptr[dst_pixel],
                                           dst_format);
                    }
                }

                result.data = dst;
                result.format = dst_format;
                result.width = crop.width;
                result.height = crop.height;
                result.valid = 1;
            } else
                R_log_error(
                        R_bad_argument,
                        "Destination buffer is too small. Expected at least %d bytes but was %d",
                        expected_size, dst_size);
        } else
            R_log_error(R_bad_argument, "Image can not be cropped, crop rectangle out of bounds.");
    } else
        R_log_error(R_bad_argument, "Image is invalid.");

    return result;
}

/**
 * Crop an image and store the result in a provided buffer.
 * @param image a valid image that we crop from.
 * @param crop a rectangle representing which part of the image to crop.
 * @param dst a buffer for the resulting image. Must be of size at least `R_pixel_buffer_size(RF_UNCOMPRESSED_R8G8B8A8, crop.width, crop.height)`.
 * @param dst_size size of the `dst` buffer.
 * @return a cropped image using the `dst` buffer in the same format as `image`.
 */
R_public R_image R_image_crop(R_image image, R_rec crop, R_allocator allocator) {
    R_image result = {0};

    if (image.valid) {
        int size = R_pixel_buffer_size(crop.width, crop.height, image.format);
        void *dst = R_alloc(allocator, size);

        if (dst) {
            result = R_image_crop_to_buffer(image, crop, dst, size, image.format);
        } else
            R_log_error(R_bad_alloc, "Allocation of size %d failed.", size);
    } else
        R_log_error(R_bad_argument, "Image is invalid.");

    return result;
}

R_internal int R_format_to_stb_channel_count(R_pixel_format format) {
    switch (format) {
        case R_pixel_format_grayscale:
            return 1;
        case R_pixel_format_gray_alpha:
            return 2;
        case R_pixel_format_r8g8b8:
            return 3;
        case R_pixel_format_r8g8b8a8:
            return 4;
        default:
            return 0;
    }
}

// Resize and image to new size.
// Note: Uses stb default scaling filters (both bicubic): STBIR_DEFAULT_FILTER_UPSAMPLE STBIR_FILTER_CATMULLROM STBIR_DEFAULT_FILTER_DOWNSAMPLE STBIR_FILTER_MITCHELL (high-quality Catmull-Rom)
R_public R_image R_image_resize_to_buffer(R_image image, int new_width, int new_height, void *dst,
                                          R_int dst_size, R_allocator temp_allocator) {
    if (!image.valid || dst_size < new_width * new_height * R_bytes_per_pixel(image.format))
        return (R_image){0};

    R_image result = {0};

    int stb_format = R_format_to_stb_channel_count(image.format);

    if (stb_format) {
        R_set_global_dependencies_allocator(temp_allocator);
        stbir_resize_uint8((unsigned char *) image.data, image.width, image.height, 0,
                           (unsigned char *) dst, new_width, new_height, 0, stb_format);
        R_set_global_dependencies_allocator((R_allocator){0});

        result.data = dst;
        result.width = new_width;
        result.height = new_height;
        result.format = image.format;
        result.valid = 1;
    } else// if the format of the image is not supported by stbir
    {
        int pixels_size = image.width * image.height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8);
        R_color *pixels = R_alloc(temp_allocator, pixels_size);

        if (pixels) {
            R_bool format_success = R_format_pixels_to_rgba32(image.data, R_image_size(image),
                                                              image.format, pixels, pixels_size);
            R_assert(format_success);

            R_set_global_dependencies_allocator(temp_allocator);
            stbir_resize_uint8((unsigned char *) pixels, image.width, image.height, 0,
                               (unsigned char *) dst, new_width, new_height, 0, 4);
            R_set_global_dependencies_allocator((R_allocator){0});

            format_success = R_format_pixels(pixels, pixels_size, R_pixel_format_r8g8b8a8, dst,
                                             dst_size, image.format);
            R_assert(format_success);

            result.data = dst;
            result.width = new_width;
            result.height = new_height;
            result.format = image.format;
            result.valid = 1;
        } else
            R_log_error(R_bad_alloc, "Allocation of size %d failed.",
                        image.width * image.height * sizeof(R_color));

        R_free(temp_allocator, pixels);
    }

    return result;
}

R_public R_image R_image_resize(R_image image, int new_width, int new_height, R_allocator allocator,
                                R_allocator temp_allocator) {
    R_image result = {0};

    if (image.valid) {
        int dst_size = new_width * new_height * R_bytes_per_pixel(image.format);
        void *dst = R_alloc(allocator, dst_size);

        if (dst) {
            result = R_image_resize_to_buffer(image, new_width, new_height, dst, dst_size,
                                              temp_allocator);
        } else
            R_log_error(R_bad_alloc, "Allocation of size %d failed.", dst_size);
    } else
        R_log_error(R_bad_argument, "Image is invalid.");

    return result;
}

/**
 * Resize and image to new size using Nearest-Neighbor scaling algorithm
 * @param image
 * @param new_width
 * @param new_height
 * @param dst
 * @param dst_size
 * @return a resized version of the `image`, with the `dst` buffer, in the same format.
 */
R_public R_image R_image_resize_nn_to_buffer(R_image image, int new_width, int new_height,
                                             void *dst, R_int dst_size) {
    R_image result = {0};

    if (image.valid) {
        int bpp = R_bytes_per_pixel(image.format);
        int expected_size = new_width * new_height * bpp;

        if (dst_size >= expected_size) {
            // EDIT: added +1 to account for an early rounding problem
            int x_ratio = (int) ((image.width << 16) / new_width) + 1;
            int y_ratio = (int) ((image.height << 16) / new_height) + 1;

            unsigned char *src = image.data;

            int x2, y2;
            for (R_int y = 0; y < new_height; y++) {
                for (R_int x = 0; x < new_width; x++) {
                    x2 = ((x * x_ratio) >> 16);
                    y2 = ((y * y_ratio) >> 16);

                    R_format_one_pixel(src + ((y2 * image.width) + x2) * bpp, image.format,
                                       ((unsigned char *) dst) + ((y * new_width) + x) * bpp,
                                       image.format);
                }
            }

            result.data = dst;
            result.width = new_width;
            result.height = new_height;
            result.format = image.format;
            result.valid = 1;
        } else
            R_log_error(R_bad_buffer_size,
                        "Expected `dst` to be at least %d bytes but was %d bytes", expected_size,
                        dst_size);
    } else
        R_log_error(R_bad_argument, "Image is invalid.");

    return result;
}

R_public R_image R_image_resize_nn(R_image image, int new_width, int new_height,
                                   R_allocator allocator) {
    R_image result = {0};

    if (image.valid) {
        int dst_size = new_width * new_height * R_bytes_per_pixel(image.format);
        void *dst = R_alloc(allocator, dst_size);

        if (dst) {
            result = R_image_resize_nn_to_buffer(image, new_width, new_height, dst, dst_size);
        } else
            R_log_error(R_bad_alloc, "Allocation of size %d failed.", dst_size);
    } else
        R_log_error(R_bad_argument, "Image is invalid.");

    return result;
}

// Convert image data to desired format
R_public R_image R_image_format_to_buffer(R_image image, R_uncompressed_pixel_format dst_format,
                                          void *dst, R_int dst_size) {
    R_image result = {0};

    if (image.valid) {
        if (R_is_uncompressed_format(dst_format) && R_is_uncompressed_format(image.format)) {
            R_bool success = R_format_pixels(image.data, R_image_size(image), image.format, dst,
                                             dst_size, dst_format);
            R_assert(success);

            result = (R_image){
                    .data = dst,
                    .width = image.width,
                    .height = image.height,
                    .format = dst_format,
                    .valid = 1,
            };
        } else
            R_log_error(R_bad_argument,
                        "Cannot format compressed pixel formats. Image format: %d, Destination "
                        "format: %d.",
                        image.format, dst_format);
    } else
        R_log_error(R_bad_argument, "Image is invalid.");

    return result;
}

R_public R_image R_image_format(R_image image, R_uncompressed_pixel_format new_format,
                                R_allocator allocator) {
    R_image result = {0};

    if (image.valid) {
        if (R_is_uncompressed_format(new_format) && R_is_uncompressed_format(image.format)) {
            int dst_size = image.width * image.height * R_bytes_per_pixel(image.format);
            void *dst = R_alloc(allocator, dst_size);

            if (dst) {
                R_bool format_success = R_format_pixels(image.data, R_image_size(image),
                                                        image.format, dst, dst_size, new_format);
                R_assert(format_success);

                result = (R_image){
                        .data = dst,
                        .width = image.width,
                        .height = image.height,
                        .format = new_format,
                        .valid = 1,
                };
            } else
                R_log_error(R_bad_alloc, "Allocation of size %d failed.", dst_size);
        } else
            R_log_error(
                    R_bad_argument,
                    "Cannot format compressed pixel formats. `image.format`: %d, `dst_format`: %d",
                    image.format, new_format);
    } else
        R_log_error(R_bad_argument, "Image is invalid.");

    return result;
}

// Apply alpha mask to image. Note 1: Returned image is GRAY_ALPHA (16bit) or RGBA (32bit). Note 2: alphaMask should be same size as image
R_public R_image R_image_alpha_mask_to_buffer(R_image image, R_image alpha_mask, void *dst,
                                              R_int dst_size) {
    R_image result = {0};

    if (image.valid && alpha_mask.valid) {
        if (image.width == alpha_mask.width && image.height == alpha_mask.height) {
            if (R_is_compressed_format(image.format) &&
                alpha_mask.format == R_pixel_format_grayscale) {
                if (dst_size >= R_image_size_in_format(image, R_pixel_format_gray_alpha)) {
                    // Apply alpha mask to alpha channel
                    for (R_int i = 0; i < image.width * image.height; i++) {
                        unsigned char mask_pixel = 0;
                        R_format_one_pixel(((unsigned char *) alpha_mask.data) + i,
                                           alpha_mask.format, &mask_pixel,
                                           R_pixel_format_grayscale);

                        // Todo: Finish implementing this function
                        //((unsigned char*)dst)[k] = mask_pixel;
                    }

                    result.data = dst;
                    result.width = image.width;
                    result.height = image.height;
                    result.format = R_pixel_format_gray_alpha;
                    result.valid = 1;
                }
            } else
                R_log_error(R_bad_argument,
                            "Expected compressed pixel formats. `image.format`: %d, "
                            "`alpha_mask.format`: %d",
                            image.format, alpha_mask.format);
        } else
            R_log_error(R_bad_argument,
                        "Alpha mask must be same size as image but was w: %d, h: %d",
                        alpha_mask.width, alpha_mask.height);
    } else
        R_log_error(R_bad_argument,
                    "One image was invalid. `image.valid`: %d, `alpha_mask.valid`: %d", image.valid,
                    alpha_mask.valid);

    return result;
}

// Clear alpha channel to desired color. Note: Threshold defines the alpha limit, 0.0f to 1.0f
R_public R_image R_image_alpha_clear(R_image image, R_color color, float threshold,
                                     R_allocator allocator, R_allocator temp_allocator) {
    R_image result = {0};

    if (image.valid) {
        R_color *pixels = R_image_pixels_to_rgba32(image, temp_allocator);

        if (pixels) {
            for (R_int i = 0; i < image.width * image.height; i++) {
                if (pixels[i].a <= (unsigned char) (threshold * 255.0f)) { pixels[i] = color; }
            }

            R_image temp_image = {.data = pixels,
                                  .width = image.width,
                                  .height = image.height,
                                  .format = R_pixel_format_r8g8b8a8,
                                  .valid = 1};

            result = R_image_format(temp_image, image.format, allocator);
        }

        R_free(temp_allocator, pixels);
    } else
        R_log_error(R_bad_argument, "Image is invalid.");

    return result;
}

// Premultiply alpha channel
R_public R_image R_image_alpha_premultiply(R_image image, R_allocator allocator,
                                           R_allocator temp_allocator) {
    R_image result = {0};

    if (image.valid) {
        float alpha = 0.0f;
        R_color *pixels = R_image_pixels_to_rgba32(image, temp_allocator);

        if (pixels) {
            for (R_int i = 0; i < image.width * image.height; i++) {
                alpha = (float) pixels[i].a / 255.0f;
                pixels[i].r = (unsigned char) ((float) pixels[i].r * alpha);
                pixels[i].g = (unsigned char) ((float) pixels[i].g * alpha);
                pixels[i].b = (unsigned char) ((float) pixels[i].b * alpha);
            }

            R_image temp_image = {
                    .data = pixels,
                    .width = image.width,
                    .height = image.height,
                    .format = R_pixel_format_r8g8b8a8,
                    .valid = 1,
            };

            result = R_image_format(temp_image, image.format, allocator);
        }

        R_free(temp_allocator, pixels);
    } else
        R_log_error(R_bad_argument, "Image is invalid.");

    return result;
}

R_public R_rec R_image_alpha_crop_rec(R_image image, float threshold) {
    if (!image.valid) return (R_rec){0};

    int bpp = R_bytes_per_pixel(image.format);

    int x_min = INT_MAX;
    int x_max = 0;
    int y_min = INT_MAX;
    int y_max = 0;

    char *src = image.data;

    for (R_int y = 0; y < image.height; y++) {
        for (R_int x = 0; x < image.width; x++) {
            int pixel = (y * image.width + x) * bpp;
            R_color pixel_rgba32 = R_format_one_pixel_to_rgba32(&src[pixel], image.format);

            if (pixel_rgba32.a > (unsigned char) (threshold * 255.0f)) {
                if (x < x_min) x_min = x;
                if (x > x_max) x_max = x;
                if (y < y_min) y_min = y;
                if (y > y_max) y_max = y;
            }
        }
    }

    return (R_rec){x_min, y_min, (x_max + 1) - x_min, (y_max + 1) - y_min};
}

// Crop image depending on alpha value
R_public R_image R_image_alpha_crop(R_image image, float threshold, R_allocator allocator) {
    R_rec crop = R_image_alpha_crop_rec(image, threshold);

    return R_image_crop(image, crop, allocator);
}

// Dither image data to 16bpp or lower (Floyd-Steinberg dithering) Note: In case selected bpp do not represent an known 16bit format, dithered data is stored in the LSB part of the unsigned short
R_public R_image R_image_dither(const R_image image, int r_bpp, int g_bpp, int b_bpp, int a_bpp,
                                R_allocator allocator, R_allocator temp_allocator) {
    R_image result = {0};

    if (image.valid) {
        if (image.format == R_pixel_format_r8g8b8) {
            if (image.format == R_pixel_format_r8g8b8 && (r_bpp + g_bpp + b_bpp + a_bpp) < 16) {
                R_color *pixels = R_image_pixels_to_rgba32(image, temp_allocator);

                if ((image.format != R_pixel_format_r8g8b8) &&
                    (image.format != R_pixel_format_r8g8b8a8)) {
                    R_log(R_log_type_warning, "R_image format is already 16bpp or lower, "
                                              "dithering could have no effect");
                }

                // Todo: Finish implementing this function
                // // Define new image format, check if desired bpp match internal known format
                // if ((r_bpp == 5) && (g_bpp == 6) && (b_bpp == 5) && (a_bpp == 0)) image.format = RF_UNCOMPRESSED_R5G6B5;
                // else if ((r_bpp == 5) && (g_bpp == 5) && (b_bpp == 5) && (a_bpp == 1)) image.format = RF_UNCOMPRESSED_R5G5B5A1;
                // else if ((r_bpp == 4) && (g_bpp == 4) && (b_bpp == 4) && (a_bpp == 4)) image.format = RF_UNCOMPRESSED_R4G4B4A4;
                // else
                // {
                //     image.format = 0;
                //     RF_LOG(RF_LOG_TYPE_WARNING, "Unsupported dithered OpenGL internal format: %ibpp (R%i_g%i_b%i_a%i)", (r_bpp + g_bpp + b_bpp + a_bpp), r_bpp, g_bpp, b_bpp, a_bpp);
                // }
                //
                // // NOTE: We will store the dithered data as unsigned short (16bpp)
                // image.data = (unsigned short*) RF_ALLOC(image.allocator, image.width * image.height * sizeof(unsigned short));

                R_color old_pixel = R_white;
                R_color new_pixel = R_white;

                int r_error, g_error, b_error;
                unsigned short r_pixel, g_pixel, b_pixel,
                        a_pixel;// Used for 16bit pixel composition

                for (R_int y = 0; y < image.height; y++) {
                    for (R_int x = 0; x < image.width; x++) {
                        old_pixel = pixels[y * image.width + x];

                        // NOTE: New pixel obtained by bits truncate, it would be better to round values (check R_image_format())
                        new_pixel.r = old_pixel.r >> (8 - r_bpp);// R bits
                        new_pixel.g = old_pixel.g >> (8 - g_bpp);// G bits
                        new_pixel.b = old_pixel.b >> (8 - b_bpp);// B bits
                        new_pixel.a = old_pixel.a >> (8 - a_bpp);// A bits (not used on dithering)

                        // NOTE: Error must be computed between new and old pixel but using same number of bits!
                        // We want to know how much color precision we have lost...
                        r_error = (int) old_pixel.r - (int) (new_pixel.r << (8 - r_bpp));
                        g_error = (int) old_pixel.g - (int) (new_pixel.g << (8 - g_bpp));
                        b_error = (int) old_pixel.b - (int) (new_pixel.b << (8 - b_bpp));

                        pixels[y * image.width + x] = new_pixel;

                        // NOTE: Some cases are out of the array and should be ignored
                        if (x < (image.width - 1)) {
                            pixels[y * image.width + x + 1].r =
                                    R_min_i((int) pixels[y * image.width + x + 1].r +
                                                    (int) ((float) r_error * 7.0f / 16),
                                            0xff);
                            pixels[y * image.width + x + 1].g =
                                    R_min_i((int) pixels[y * image.width + x + 1].g +
                                                    (int) ((float) g_error * 7.0f / 16),
                                            0xff);
                            pixels[y * image.width + x + 1].b =
                                    R_min_i((int) pixels[y * image.width + x + 1].b +
                                                    (int) ((float) b_error * 7.0f / 16),
                                            0xff);
                        }

                        if ((x > 0) && (y < (image.height - 1))) {
                            pixels[(y + 1) * image.width + x - 1].r =
                                    R_min_i((int) pixels[(y + 1) * image.width + x - 1].r +
                                                    (int) ((float) r_error * 3.0f / 16),
                                            0xff);
                            pixels[(y + 1) * image.width + x - 1].g =
                                    R_min_i((int) pixels[(y + 1) * image.width + x - 1].g +
                                                    (int) ((float) g_error * 3.0f / 16),
                                            0xff);
                            pixels[(y + 1) * image.width + x - 1].b =
                                    R_min_i((int) pixels[(y + 1) * image.width + x - 1].b +
                                                    (int) ((float) b_error * 3.0f / 16),
                                            0xff);
                        }

                        if (y < (image.height - 1)) {
                            pixels[(y + 1) * image.width + x].r =
                                    R_min_i((int) pixels[(y + 1) * image.width + x].r +
                                                    (int) ((float) r_error * 5.0f / 16),
                                            0xff);
                            pixels[(y + 1) * image.width + x].g =
                                    R_min_i((int) pixels[(y + 1) * image.width + x].g +
                                                    (int) ((float) g_error * 5.0f / 16),
                                            0xff);
                            pixels[(y + 1) * image.width + x].b =
                                    R_min_i((int) pixels[(y + 1) * image.width + x].b +
                                                    (int) ((float) b_error * 5.0f / 16),
                                            0xff);
                        }

                        if ((x < (image.width - 1)) && (y < (image.height - 1))) {
                            pixels[(y + 1) * image.width + x + 1].r =
                                    R_min_i((int) pixels[(y + 1) * image.width + x + 1].r +
                                                    (int) ((float) r_error * 1.0f / 16),
                                            0xff);
                            pixels[(y + 1) * image.width + x + 1].g =
                                    R_min_i((int) pixels[(y + 1) * image.width + x + 1].g +
                                                    (int) ((float) g_error * 1.0f / 16),
                                            0xff);
                            pixels[(y + 1) * image.width + x + 1].b =
                                    R_min_i((int) pixels[(y + 1) * image.width + x + 1].b +
                                                    (int) ((float) b_error * 1.0f / 16),
                                            0xff);
                        }

                        r_pixel = (unsigned short) new_pixel.r;
                        g_pixel = (unsigned short) new_pixel.g;
                        b_pixel = (unsigned short) new_pixel.b;
                        a_pixel = (unsigned short) new_pixel.a;

                        ((unsigned short *) image.data)[y * image.width + x] =
                                (r_pixel << (g_bpp + b_bpp + a_bpp)) |
                                (g_pixel << (b_bpp + a_bpp)) | (b_pixel << a_bpp) | a_pixel;
                    }
                }

                R_free(temp_allocator, pixels);
            } else
                R_log_error(
                        R_bad_argument,
                        "Unsupported dithering bpps (%ibpp), only 16bpp or lower modes supported",
                        (r_bpp + g_bpp + b_bpp + a_bpp));
        }
    }

    return result;
}

// Flip image vertically
R_public R_image R_image_flip_vertical_to_buffer(R_image image, void *dst, R_int dst_size) {
    R_image result = {0};

    if (image.valid) {
        if (dst_size >= R_image_size(image)) {
            int bpp = R_bytes_per_pixel(image.format);

            for (R_int y = 0; y < image.height; y++) {
                for (R_int x = 0; x < image.width; x++) {
                    void *dst_pixel = ((unsigned char *) dst) + (y * image.width + x) * bpp;
                    void *src_pixel = ((unsigned char *) image.data) +
                                      ((image.height - 1 - y) * image.width + x) * bpp;

                    memcpy(dst_pixel, src_pixel, bpp);
                }
            }

            result = image;
            result.data = dst;
        }
    }

    return result;
}

R_public R_image R_image_flip_vertical(R_image image, R_allocator allocator) {
    if (!image.valid) return (R_image){0};

    int size = R_image_size(image);
    void *dst = R_alloc(allocator, size);

    R_image result = R_image_flip_vertical_to_buffer(image, dst, size);
    if (!result.valid) R_free(allocator, dst);

    return result;
}

// Flip image horizontally
R_public R_image R_image_flip_horizontal_to_buffer(R_image image, void *dst, R_int dst_size) {
    R_image result = {0};

    if (image.valid) {
        if (dst_size >= R_image_size(image)) {
            int bpp = R_bytes_per_pixel(image.format);

            for (R_int y = 0; y < image.height; y++) {
                for (R_int x = 0; x < image.width; x++) {
                    void *dst_pixel = ((unsigned char *) dst) + (y * image.width + x) * bpp;
                    void *src_pixel = ((unsigned char *) image.data) +
                                      (y * image.width + (image.width - 1 - x)) * bpp;

                    memcpy(dst_pixel, src_pixel, bpp);
                }
            }

            result = image;
            result.data = dst;
        }
    }

    return result;
}

R_public R_image R_image_flip_horizontal(R_image image, R_allocator allocator) {
    if (!image.valid) return (R_image){0};

    int size = R_image_size(image);
    void *dst = R_alloc(allocator, size);

    R_image result = R_image_flip_horizontal_to_buffer(image, dst, size);
    if (!result.valid) R_free(allocator, dst);

    return result;
}

// Rotate image clockwise 90deg
R_public R_image R_image_rotate_cw_to_buffer(R_image image, void *dst, R_int dst_size) {
    R_image result = {0};

    if (image.valid) {
        if (dst_size >= R_image_size(image)) {
            int bpp = R_bytes_per_pixel(image.format);

            for (R_int y = 0; y < image.height; y++) {
                for (R_int x = 0; x < image.width; x++) {
                    void *dst_pixel = ((unsigned char *) dst) +
                                      (x * image.height + (image.height - y - 1)) * bpp;
                    void *src_pixel = ((unsigned char *) image.data) + (y * image.width + x) * bpp;

                    memcpy(dst_pixel, src_pixel, bpp);
                }
            }

            result = image;
            result.data = dst;
        }
    }

    return result;
}

R_public R_image R_image_rotate_cw(R_image image) {
    return R_image_rotate_cw_to_buffer(image, image.data, R_image_size(image));
}

// Rotate image counter-clockwise 90deg
R_public R_image R_image_rotate_ccw_to_buffer(R_image image, void *dst, R_int dst_size) {
    R_image result = {0};

    if (image.valid) {
        if (dst_size >= R_image_size(image)) {
            int bpp = R_bytes_per_pixel(image.format);

            for (R_int y = 0; y < image.height; y++) {
                for (R_int x = 0; x < image.width; x++) {
                    void *dst_pixel = ((unsigned char *) dst) + (x * image.height + y) * bpp;
                    void *src_pixel = ((unsigned char *) image.data) +
                                      (y * image.width + (image.width - x - 1)) * bpp;

                    memcpy(dst_pixel, src_pixel, bpp);
                }
            }

            result = image;
            result.data = dst;
        }
    }

    return result;
}

R_public R_image R_image_rotate_ccw(R_image image) {
    return R_image_rotate_ccw_to_buffer(image, image.data, R_image_size(image));
}

// Modify image color: tint
R_public R_image R_image_color_tint_to_buffer(R_image image, R_color color, void *dst,
                                              R_int dst_size) {
    R_image result = {0};

    if (image.valid) {
        if (dst_size >= R_image_size(image)) {
            int bpp = R_bytes_per_pixel(image.format);

            float c_r = ((float) color.r) / 255.0f;
            float c_g = ((float) color.g) / 255.0f;
            float c_b = ((float) color.b) / 255.0f;
            float c_a = ((float) color.a) / 255.0f;

            for (R_int y = 0; y < image.height; y++) {
                for (R_int x = 0; x < image.width; x++) {
                    int index = y * image.width + x;
                    void *src_pixel = ((unsigned char *) image.data) + index * bpp;
                    void *dst_pixel = ((unsigned char *) image.data) + index * bpp;

                    R_color pixel_rgba32 = R_format_one_pixel_to_rgba32(src_pixel, image.format);

                    pixel_rgba32.r =
                            (unsigned char) (255.f * (((float) pixel_rgba32.r) / 255.f * c_r));
                    pixel_rgba32.g =
                            (unsigned char) (255.f * (((float) pixel_rgba32.g) / 255.f * c_g));
                    pixel_rgba32.b =
                            (unsigned char) (255.f * (((float) pixel_rgba32.b) / 255.f * c_b));
                    pixel_rgba32.a =
                            (unsigned char) (255.f * (((float) pixel_rgba32.a) / 255.f * c_a));

                    R_format_one_pixel(&pixel_rgba32, R_pixel_format_r8g8b8a8, dst_pixel,
                                       image.format);
                }
            }

            result = image;
            result.data = dst;
        }
    }

    return result;
}

R_public R_image R_image_color_tint(R_image image, R_color color) {
    return R_image_color_tint_to_buffer(image, color, image.data, R_image_size(image));
}

// Modify image color: invert
R_public R_image R_image_color_invert_to_buffer(R_image image, void *dst, R_int dst_size) {
    R_image result = {0};

    if (image.valid) {
        if (dst_size >= R_image_size(image)) {
            int bpp = R_bytes_per_pixel(image.format);

            for (R_int y = 0; y < image.height; y++) {
                for (R_int x = 0; x < image.width; x++) {
                    int index = y * image.width + x;
                    void *src_pixel = ((unsigned char *) image.data) + index * bpp;
                    void *dst_pixel = ((unsigned char *) dst) + index * bpp;

                    R_color pixel_rgba32 = R_format_one_pixel_to_rgba32(src_pixel, image.format);
                    pixel_rgba32.r = 255 - pixel_rgba32.r;
                    pixel_rgba32.g = 255 - pixel_rgba32.g;
                    pixel_rgba32.b = 255 - pixel_rgba32.b;

                    R_format_one_pixel(&pixel_rgba32, R_pixel_format_r8g8b8a8, dst_pixel,
                                       image.format);
                }
            }

            result = image;
            result.data = dst;
        }
    }

    return result;
}

R_public R_image R_image_color_invert(R_image image) {
    return R_image_color_invert_to_buffer(image, image.data, R_image_size(image));
}

// Modify image color: grayscale
R_public R_image R_image_color_grayscale_to_buffer(R_image image, void *dst, R_int dst_size) {
    return R_image_format_to_buffer(image, R_pixel_format_grayscale, dst, dst_size);
}

R_public R_image R_image_color_grayscale(R_image image) {
    R_image result = {0};

    if (image.valid) {
        result = R_image_color_grayscale_to_buffer(image, image.data, R_image_size(image));
    }

    return result;
}

// Modify image color: contrast
// NOTE: Contrast values between -100 and 100
R_public R_image R_image_color_contrast_to_buffer(R_image image, float contrast, void *dst,
                                                  R_int dst_size) {
    R_image result = {0};

    if (image.valid) {
        if (dst_size >= R_image_size(image)) {
            if (contrast < -100) contrast = -100;
            if (contrast > +100) contrast = +100;

            contrast = (100.0f + contrast) / 100.0f;
            contrast *= contrast;

            int bpp = R_bytes_per_pixel(image.format);

            for (R_int y = 0; y < image.height; y++) {
                for (R_int x = 0; x < image.width; x++) {
                    int index = y * image.width + x;
                    void *src_pixel = ((unsigned char *) image.data) + index * bpp;
                    void *dst_pixel = ((unsigned char *) dst) + index * bpp;

                    R_color src_pixel_rgba32 =
                            R_format_one_pixel_to_rgba32(src_pixel, image.format);

                    float p_r = ((float) src_pixel_rgba32.r) / 255.0f;
                    p_r -= 0.5;
                    p_r *= contrast;
                    p_r += 0.5;
                    p_r *= 255;
                    if (p_r < 0) p_r = 0;
                    if (p_r > 255) p_r = 255;

                    float p_g = ((float) src_pixel_rgba32.g) / 255.0f;
                    p_g -= 0.5;
                    p_g *= contrast;
                    p_g += 0.5;
                    p_g *= 255;
                    if (p_g < 0) p_g = 0;
                    if (p_g > 255) p_g = 255;

                    float p_b = ((float) src_pixel_rgba32.b) / 255.0f;
                    p_b -= 0.5;
                    p_b *= contrast;
                    p_b += 0.5;
                    p_b *= 255;
                    if (p_b < 0) p_b = 0;
                    if (p_b > 255) p_b = 255;

                    src_pixel_rgba32.r = (unsigned char) p_r;
                    src_pixel_rgba32.g = (unsigned char) p_g;
                    src_pixel_rgba32.b = (unsigned char) p_b;

                    R_format_one_pixel(&src_pixel_rgba32, R_pixel_format_r8g8b8a8, dst_pixel,
                                       image.format);
                }
            }

            result = image;
            result.data = dst;
        }
    }

    return result;
}

R_public R_image R_image_color_contrast(R_image image, int contrast) {
    return R_image_color_contrast_to_buffer(image, contrast, image.data, R_image_size(image));
}

// Modify image color: brightness
// NOTE: Brightness values between -255 and 255
R_public R_image R_image_color_brightness_to_buffer(R_image image, int brightness, void *dst,
                                                    R_int dst_size) {
    R_image result = {0};

    if (image.valid) {
        if (dst_size >= R_image_size(image)) {
            if (brightness < -255) brightness = -255;
            if (brightness > +255) brightness = +255;

            int bpp = R_bytes_per_pixel(image.format);

            for (R_int y = 0; y < image.height; y++) {
                for (R_int x = 0; x < image.width; x++) {
                    int index = y * image.width + x;

                    void *src_pixel = ((unsigned char *) image.data) + index * bpp;
                    void *dst_pixel = ((unsigned char *) dst) + index * bpp;

                    R_color pixel_rgba32 = R_format_one_pixel_to_rgba32(src_pixel, image.format);

                    int c_r = pixel_rgba32.r + brightness;
                    int c_g = pixel_rgba32.g + brightness;
                    int c_b = pixel_rgba32.b + brightness;

                    if (c_r < 0) c_r = 1;
                    if (c_r > 255) c_r = 255;

                    if (c_g < 0) c_g = 1;
                    if (c_g > 255) c_g = 255;

                    if (c_b < 0) c_b = 1;
                    if (c_b > 255) c_b = 255;

                    pixel_rgba32.r = (unsigned char) c_r;
                    pixel_rgba32.g = (unsigned char) c_g;
                    pixel_rgba32.b = (unsigned char) c_b;

                    R_format_one_pixel(&pixel_rgba32, R_pixel_format_r8g8b8a8, dst_pixel,
                                       image.format);
                }
            }

            result = image;
            result.data = dst;
        }
    }

    return result;
}

R_public R_image R_image_color_brightness(R_image image, int brightness) {
    return R_image_color_brightness_to_buffer(image, brightness, image.data, R_image_size(image));
}

// Modify image color: replace color
R_public R_image R_image_color_replace_to_buffer(R_image image, R_color color, R_color replace,
                                                 void *dst, R_int dst_size) {
    if (image.valid && dst_size >= R_image_size(image)) return (R_image){0};

    R_image result = {0};

    int bpp = R_bytes_per_pixel(image.format);

    for (R_int y = 0; y < image.height; y++) {
        for (R_int x = 0; x < image.width; x++) {
            int index = y * image.width + x;

            void *src_pixel = ((unsigned char *) image.data) + index * bpp;
            void *dst_pixel = ((unsigned char *) dst) + index * bpp;

            R_color pixel_rgba32 = R_format_one_pixel_to_rgba32(src_pixel, image.format);

            if (R_color_match(pixel_rgba32, color)) {
                R_format_one_pixel(&replace, R_pixel_format_r8g8b8a8, dst_pixel, image.format);
            }
        }
    }

    result = image;
    result.data = dst;

    return result;
}

R_public R_image R_image_color_replace(R_image image, R_color color, R_color replace) {
    return R_image_color_replace_to_buffer(image, color, replace, image.data, R_image_size(image));
}

// Generate image: plain color
R_public R_image R_gen_image_color_to_buffer(int width, int height, R_color color, R_color *dst,
                                             R_int dst_size) {
    R_image result = {0};

    for (R_int i = 0; i < dst_size; i++) { dst[i] = color; }

    return (R_image){
            .data = dst,
            .width = width,
            .height = height,
            .format = R_pixel_format_r8g8b8a8,
            .valid = 1,
    };
}

R_public R_image R_gen_image_color(int width, int height, R_color color, R_allocator allocator) {
    R_image result = {0};

    int dst_size = width * height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8);
    void *dst = R_alloc(allocator, dst_size);

    if (dst) { result = R_gen_image_color_to_buffer(width, height, color, dst, width * height); }

    return result;
}

// Generate image: vertical gradient
R_public R_image R_gen_image_gradient_v_to_buffer(int width, int height, R_color top,
                                                  R_color bottom, R_color *dst, R_int dst_size) {
    R_image result = {0};

    if (dst_size >= width * height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8)) {
        for (R_int j = 0; j < height; j++) {
            float factor = ((float) j) / ((float) height);

            for (R_int i = 0; i < width; i++) {
                ((R_color *) dst)[j * width + i].r =
                        (int) ((float) bottom.r * factor + (float) top.r * (1.f - factor));
                ((R_color *) dst)[j * width + i].g =
                        (int) ((float) bottom.g * factor + (float) top.g * (1.f - factor));
                ((R_color *) dst)[j * width + i].b =
                        (int) ((float) bottom.b * factor + (float) top.b * (1.f - factor));
                ((R_color *) dst)[j * width + i].a =
                        (int) ((float) bottom.a * factor + (float) top.a * (1.f - factor));
            }
        }

        result = (R_image){.data = dst,
                           .width = width,
                           .height = height,
                           .format = R_pixel_format_r8g8b8a8,
                           .valid = 1};
    }

    return result;
}

R_public R_image R_gen_image_gradient_v(int width, int height, R_color top, R_color bottom,
                                        R_allocator allocator) {
    R_image result = {0};

    int dst_size = width * height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8);
    R_color *dst = R_alloc(allocator, dst_size);

    if (dst) {
        result = R_gen_image_gradient_v_to_buffer(width, height, top, bottom, dst, dst_size);
    }

    return result;
}

// Generate image: horizontal gradient
R_public R_image R_gen_image_gradient_h_to_buffer(int width, int height, R_color left,
                                                  R_color right, R_color *dst, R_int dst_size) {
    R_image result = {0};

    if (dst_size >= width * height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8)) {
        for (R_int i = 0; i < width; i++) {
            float factor = ((float) i) / ((float) width);

            for (R_int j = 0; j < height; j++) {
                ((R_color *) dst)[j * width + i].r =
                        (int) ((float) right.r * factor + (float) left.r * (1.f - factor));
                ((R_color *) dst)[j * width + i].g =
                        (int) ((float) right.g * factor + (float) left.g * (1.f - factor));
                ((R_color *) dst)[j * width + i].b =
                        (int) ((float) right.b * factor + (float) left.b * (1.f - factor));
                ((R_color *) dst)[j * width + i].a =
                        (int) ((float) right.a * factor + (float) left.a * (1.f - factor));
            }
        }

        result = (R_image){.data = dst,
                           .width = width,
                           .height = height,
                           .format = R_pixel_format_r8g8b8a8,
                           .valid = 1};
    }

    return result;
}

R_public R_image R_gen_image_gradient_h(int width, int height, R_color left, R_color right,
                                        R_allocator allocator) {
    R_image result = {0};

    int dst_size = width * height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8);
    R_color *dst = R_alloc(allocator, dst_size);

    if (dst) {
        result = R_gen_image_gradient_h_to_buffer(width, height, left, right, dst, dst_size);
    }

    return result;
}

// Generate image: radial gradient
R_public R_image R_gen_image_gradient_radial_to_buffer(int width, int height, float density,
                                                       R_color inner, R_color outer, R_color *dst,
                                                       R_int dst_size) {
    R_image result = {0};

    if (dst_size >= width * height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8)) {
        float radius = (width < height) ? ((float) width) / 2.0f : ((float) height) / 2.0f;

        float center_x = ((float) width) / 2.0f;
        float center_y = ((float) height) / 2.0f;

        for (R_int y = 0; y < height; y++) {
            for (R_int x = 0; x < width; x++) {
                float dist = hypotf((float) x - center_x, (float) y - center_y);
                float factor = (dist - radius * density) / (radius * (1.0f - density));

                factor = (float) fmax(factor, 0.f);
                factor = (float) fmin(factor,
                                      1.f);// dist can be bigger than radius so we have to check

                dst[y * width + x].r =
                        (int) ((float) outer.r * factor + (float) inner.r * (1.0f - factor));
                dst[y * width + x].g =
                        (int) ((float) outer.g * factor + (float) inner.g * (1.0f - factor));
                dst[y * width + x].b =
                        (int) ((float) outer.b * factor + (float) inner.b * (1.0f - factor));
                dst[y * width + x].a =
                        (int) ((float) outer.a * factor + (float) inner.a * (1.0f - factor));
            }
        }

        result = (R_image){
                .data = dst,
                .width = width,
                .height = height,
                .format = R_pixel_format_r8g8b8a8,
                .valid = 1,
        };
    }

    return result;
}

R_public R_image R_gen_image_gradient_radial(int width, int height, float density, R_color inner,
                                             R_color outer, R_allocator allocator) {
    R_image result = {0};

    int dst_size = width * height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8);
    R_color *dst = R_alloc(allocator, dst_size);

    if (dst) {
        result = R_gen_image_gradient_radial_to_buffer(width, height, density, inner, outer, dst,
                                                       dst_size);
    }

    return result;
}

// Generate image: checked
R_public R_image R_gen_image_checked_to_buffer(int width, int height, int checks_x, int checks_y,
                                               R_color col1, R_color col2, R_color *dst,
                                               R_int dst_size) {
    R_image result = {0};

    if (dst_size >= width * height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8)) {
        float radius = (width < height) ? ((float) width) / 2.0f : ((float) height) / 2.0f;

        float center_x = ((float) width) / 2.0f;
        float center_y = ((float) height) / 2.0f;

        for (R_int y = 0; y < height; y++) {
            for (R_int x = 0; x < width; x++) {
                if ((x / checks_x + y / checks_y) % 2 == 0) dst[y * width + x] = col1;
                else
                    dst[y * width + x] = col2;
            }
        }

        result = (R_image){
                .data = dst,
                .width = width,
                .height = height,
                .format = R_pixel_format_r8g8b8a8,
                .valid = 1,
        };
    }

    return result;
}

R_public R_image R_gen_image_checked(int width, int height, int checks_x, int checks_y,
                                     R_color col1, R_color col2, R_allocator allocator) {
    R_image result = {0};

    int dst_size = width * height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8);
    R_color *dst = R_alloc(allocator, dst_size);

    if (dst) {
        result = R_gen_image_checked_to_buffer(width, height, checks_x, checks_y, col1, col2, dst,
                                               dst_size);
    }

    return result;
}

// Generate image: white noise
R_public R_image R_gen_image_white_noise_to_buffer(int width, int height, float factor,
                                                   R_rand_proc rand, R_color *dst, R_int dst_size) {
    int result_image_size = width * height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8);
    R_image result = {0};

    if (dst_size < result_image_size || !rand || result_image_size <= 0) return result;

    for (R_int i = 0; i < width * height; i++) {
        if (rand(0, 99) < (int) (factor * 100.0f)) {
            dst[i] = R_white;
        } else {
            dst[i] = R_black;
        }
    }

    result = (R_image){
            .data = dst,
            .width = width,
            .height = height,
            .format = R_pixel_format_r8g8b8a8,
            .valid = 1,
    };

    return result;
}

R_public R_image R_gen_image_white_noise(int width, int height, float factor, R_rand_proc rand,
                                         R_allocator allocator) {
    R_image result = {0};

    int dst_size = width * height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8);

    if (!rand || dst_size <= 0) return result;

    R_color *dst = R_alloc(allocator, dst_size);
    result = R_gen_image_white_noise_to_buffer(width, height, factor, rand, dst, dst_size);

    return result;
}

// Generate image: perlin noise
R_public R_image R_gen_image_perlin_noise_to_buffer(int width, int height, int offset_x,
                                                    int offset_y, float scale, R_color *dst,
                                                    R_int dst_size) {
    R_image result = {0};

    if (dst_size >= width * height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8)) {
        for (R_int y = 0; y < height; y++) {
            for (R_int x = 0; x < width; x++) {
                float nx = (float) (x + offset_x) * scale / (float) width;
                float ny = (float) (y + offset_y) * scale / (float) height;

                // Typical values to start playing with:
                //   lacunarity = ~2.0   -- spacing between successive octaves (use exactly 2.0 for wrapping output)
                //   gain       =  0.5   -- relative weighting applied to each successive octave
                //   octaves    =  6     -- number of "octaves" of noise3() to sum

                // NOTE: We need to translate the data from [-1..1] to [0..1]
                float p = (stb_perlin_fbm_noise3(nx, ny, 1.0f, 2.0f, 0.5f, 6) + 1.0f) / 2.0f;

                int intensity = (int) (p * 255.0f);
                dst[y * width + x] = (R_color){intensity, intensity, intensity, 255};
            }
        }

        result = (R_image){
                .data = dst,
                .width = width,
                .height = height,
                .format = R_pixel_format_r8g8b8a8,
                .valid = 1,
        };
    }

    return result;
}

R_public R_image R_gen_image_perlin_noise(int width, int height, int offset_x, int offset_y,
                                          float scale, R_allocator allocator) {
    R_image result = {0};

    int dst_size = width * height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8);
    R_color *dst = R_alloc(allocator, dst_size);

    if (dst) {
        result = R_gen_image_perlin_noise_to_buffer(width, height, offset_x, offset_y, scale, dst,
                                                    dst_size);
    }

    return result;
}

R_public R_vec2 R_get_seed_for_cellular_image(int seeds_per_row, int tile_size, int i,
                                              R_rand_proc rand) {
    R_vec2 result = {0};

    int y = (i / seeds_per_row) * tile_size + rand(0, tile_size - 1);
    int x = (i % seeds_per_row) * tile_size + rand(0, tile_size - 1);
    result = (R_vec2){(float) x, (float) y};

    return result;
}

// Generate image: cellular algorithm. Bigger tileSize means bigger cells
R_public R_image R_gen_image_cellular_to_buffer(int width, int height, int tile_size,
                                                R_rand_proc rand, R_color *dst, R_int dst_size) {
    R_image result = {0};

    int seeds_per_row = width / tile_size;
    int seeds_per_col = height / tile_size;
    int seeds_count = seeds_per_row * seeds_per_col;

    if (dst_size >= width * height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8)) {
        for (R_int y = 0; y < height; y++) {
            int tile_y = y / tile_size;

            for (R_int x = 0; x < width; x++) {
                int tile_x = x / tile_size;

                float min_distance = INFINITY;

                // Check all adjacent tiles
                for (R_int i = -1; i < 2; i++) {
                    if ((tile_x + i < 0) || (tile_x + i >= seeds_per_row)) continue;

                    for (R_int j = -1; j < 2; j++) {
                        if ((tile_y + j < 0) || (tile_y + j >= seeds_per_col)) continue;

                        R_vec2 neighbor_seed = R_get_seed_for_cellular_image(
                                seeds_per_row, tile_size, (tile_y + j) * seeds_per_row + tile_x + i,
                                rand);

                        float dist =
                                (float) hypot(x - (int) neighbor_seed.x, y - (int) neighbor_seed.y);
                        min_distance = (float) fmin(min_distance, dist);
                    }
                }

                // I made this up but it seems to give good results at all tile sizes
                int intensity = (int) (min_distance * 256.0f / tile_size);
                if (intensity > 255) intensity = 255;

                dst[y * width + x] = (R_color){intensity, intensity, intensity, 255};
            }
        }

        result = (R_image){
                .data = dst,
                .width = width,
                .height = height,
                .format = R_pixel_format_r8g8b8a8,
                .valid = 1,
        };
    }

    return result;
}

R_public R_image R_gen_image_cellular(int width, int height, int tile_size, R_rand_proc rand,
                                      R_allocator allocator) {
    R_image result = {0};

    int dst_size = width * height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8);

    R_color *dst = R_alloc(allocator, dst_size);

    if (dst) {
        result = R_gen_image_cellular_to_buffer(width, height, tile_size, rand, dst, dst_size);
    }

    return result;
}

// Draw an image (source) within an image (destination)
// NOTE: R_color tint is applied to source image
R_public void R_image_draw(R_image *dst, R_image src, R_rec src_rec, R_rec dst_rec, R_color tint,
                           R_allocator temp_allocator) {
    if (src.valid && dst->valid) {
        dst->valid = 0;

        if (src_rec.x < 0) src_rec.x = 0;
        if (src_rec.y < 0) src_rec.y = 0;

        if ((src_rec.x + src_rec.width) > src.width) {
            src_rec.width = src.width - src_rec.x;
            R_log(R_log_type_warning, "Source rectangle width out of bounds, rescaled width: %i",
                  src_rec.width);
        }

        if ((src_rec.y + src_rec.height) > src.height) {
            src_rec.height = src.height - src_rec.y;
            R_log(R_log_type_warning, "Source rectangle height out of bounds, rescaled height: %i",
                  src_rec.height);
        }

        R_image src_copy =
                R_image_copy(src, temp_allocator);// Make a copy of source src to work with it

        // Crop source image to desired source rectangle (if required)
        if ((src.width != (int) src_rec.width) && (src.height != (int) src_rec.height)) {
            R_image old_src_copy = src_copy;
            src_copy = R_image_crop(src_copy, src_rec, temp_allocator);
            R_unload_image(old_src_copy, temp_allocator);
        }

        // Scale source image in case destination rec size is different than source rec size
        if (((int) dst_rec.width != (int) src_rec.width) ||
            ((int) dst_rec.height != (int) src_rec.height)) {
            R_image old_src_copy = src_copy;
            src_copy = R_image_resize(src_copy, (int) dst_rec.width, (int) dst_rec.height,
                                      temp_allocator, temp_allocator);
            R_unload_image(old_src_copy, temp_allocator);
        }

        // Check that dstRec is inside dst image
        // Allow negative position within destination with cropping
        if (dst_rec.x < 0) {
            R_image old_src_copy = src_copy;
            src_copy = R_image_crop(
                    src_copy, (R_rec){-dst_rec.x, 0, dst_rec.width + dst_rec.x, dst_rec.height},
                    temp_allocator);
            dst_rec.width = dst_rec.width + dst_rec.x;
            dst_rec.x = 0;
            R_unload_image(old_src_copy, temp_allocator);
        }

        if ((dst_rec.x + dst_rec.width) > dst->width) {
            R_image old_src_copy = src_copy;
            src_copy = R_image_crop(src_copy, (R_rec){0, 0, dst->width - dst_rec.x, dst_rec.height},
                                    temp_allocator);
            dst_rec.width = dst->width - dst_rec.x;
            R_unload_image(old_src_copy, temp_allocator);
        }

        if (dst_rec.y < 0) {
            R_image old_src_copy = src_copy;
            src_copy = R_image_crop(
                    src_copy, (R_rec){0, -dst_rec.y, dst_rec.width, dst_rec.height + dst_rec.y},
                    temp_allocator);
            dst_rec.height = dst_rec.height + dst_rec.y;
            dst_rec.y = 0;
            R_unload_image(old_src_copy, temp_allocator);
        }

        if ((dst_rec.y + dst_rec.height) > dst->height) {
            R_image old_src_copy = src_copy;
            src_copy = R_image_crop(src_copy, (R_rec){0, 0, dst_rec.width, dst->height - dst_rec.y},
                                    temp_allocator);
            dst_rec.height = dst->height - dst_rec.y;
            R_unload_image(old_src_copy, temp_allocator);
        }

        if (src_copy.valid) {
            // Get image data as R_color pixels array to work with it
            R_color *dst_pixels = R_image_pixels_to_rgba32(*dst, temp_allocator);
            R_color *src_pixels = R_image_pixels_to_rgba32(src_copy, temp_allocator);

            R_unload_image(src_copy, temp_allocator);// Source copy not required any more

            R_vec4 fsrc, fdst, fout;               // Normalized pixel data (ready for operation)
            R_vec4 ftint = R_color_normalize(tint);// Normalized color tint

            // Blit pixels, copy source image into destination
            // TODO: Maybe out-of-bounds blitting could be considered here instead of so much cropping
            for (R_int j = (int) dst_rec.y; j < (int) (dst_rec.y + dst_rec.height); j++) {
                for (R_int i = (int) dst_rec.x; i < (int) (dst_rec.x + dst_rec.width); i++) {
                    // Alpha blending (https://en.wikipedia.org/wiki/Alpha_compositing)

                    fdst = R_color_normalize(dst_pixels[j * (int) dst->width + i]);
                    fsrc = R_color_normalize(
                            src_pixels[(j - (int) dst_rec.y) * (int) dst_rec.width +
                                       (i - (int) dst_rec.x)]);

                    // Apply color tint to source image
                    fsrc.x *= ftint.x;
                    fsrc.y *= ftint.y;
                    fsrc.z *= ftint.z;
                    fsrc.w *= ftint.w;

                    fout.w = fsrc.w + fdst.w * (1.0f - fsrc.w);

                    if (fout.w <= 0.0f) {
                        fout.x = 0.0f;
                        fout.y = 0.0f;
                        fout.z = 0.0f;
                    } else {
                        fout.x = (fsrc.x * fsrc.w + fdst.x * fdst.w * (1 - fsrc.w)) / fout.w;
                        fout.y = (fsrc.y * fsrc.w + fdst.y * fdst.w * (1 - fsrc.w)) / fout.w;
                        fout.z = (fsrc.z * fsrc.w + fdst.z * fdst.w * (1 - fsrc.w)) / fout.w;
                    }

                    dst_pixels[j * (int) dst->width + i] = (R_color){
                            (unsigned char) (fout.x * 255.0f), (unsigned char) (fout.y * 255.0f),
                            (unsigned char) (fout.z * 255.0f), (unsigned char) (fout.w * 255.0f)};

                    // TODO: Support other blending options
                }
            }

            R_bool format_success = R_format_pixels(
                    dst_pixels, R_image_size_in_format(*dst, R_pixel_format_r8g8b8a8),
                    R_pixel_format_r8g8b8a8, dst->data, R_image_size(*dst), dst->format);
            R_assert(format_success);

            R_free(temp_allocator, src_pixels);
            R_free(temp_allocator, dst_pixels);

            dst->valid = 1;
        }
    }
}

// Draw rectangle within an image
R_public void R_image_draw_rectangle(R_image *dst, R_rec rec, R_color color,
                                     R_allocator temp_allocator) {
    if (dst->valid) {
        dst->valid = 0;

        R_image src = R_gen_image_color((int) rec.width, (int) rec.height, color, temp_allocator);

        if (src.valid) {
            R_rec src_rec = (R_rec){0, 0, rec.width, rec.height};

            R_image_draw(dst, src, src_rec, rec, R_white, temp_allocator);

            R_unload_image(src, temp_allocator);
        }
    }
}

// Draw rectangle lines within an image
R_public void R_image_draw_rectangle_lines(R_image *dst, R_rec rec, int thick, R_color color,
                                           R_allocator temp_allocator) {
    R_image_draw_rectangle(dst, (R_rec){rec.x, rec.y, rec.width, thick}, color, temp_allocator);
    R_image_draw_rectangle(dst, (R_rec){rec.x, rec.y + thick, thick, rec.height - thick * 2}, color,
                           temp_allocator);
    R_image_draw_rectangle(
            dst, (R_rec){rec.x + rec.width - thick, rec.y + thick, thick, rec.height - thick * 2},
            color, temp_allocator);
    R_image_draw_rectangle(dst, (R_rec){rec.x, rec.y + rec.height - thick, rec.width, thick}, color,
                           temp_allocator);
}

#pragma endregion

#pragma region mipmaps

R_public int R_mipmaps_image_size(R_mipmaps_image image) {
    int size = 0;
    int width = image.width;
    int height = image.height;

    for (R_int i = 0; i < image.mipmaps; i++) {
        size += width * height * R_bytes_per_pixel(image.format);

        width /= 2;
        height /= 2;

        // Security check for NPOT textures
        if (width < 1) width = 1;
        if (height < 1) height = 1;
    }

    return size;
}

R_public R_mipmaps_stats R_compute_mipmaps_stats(R_image image, int desired_mipmaps_count) {
    if (!image.valid) return (R_mipmaps_stats){0};

    int possible_mip_count = 1;
    int mipmaps_size = R_image_size(image);

    int mip_width = image.width;
    int mip_height = image.height;

    while (mip_width != 1 || mip_height != 1 || possible_mip_count == desired_mipmaps_count) {
        if (mip_width != 1) mip_width /= 2;
        if (mip_height != 1) mip_height /= 2;

        // Safety check for NPOT textures
        if (mip_width < 1) mip_width = 1;
        if (mip_height < 1) mip_height = 1;

        mipmaps_size += mip_width * mip_height * R_bytes_per_pixel(image.format);

        possible_mip_count++;
    }

    return (R_mipmaps_stats){possible_mip_count, mipmaps_size};
}

// Generate all mipmap levels for a provided image. image.data is scaled to include mipmap levels. Mipmaps format is the same as base image
R_public R_mipmaps_image R_image_gen_mipmaps_to_buffer(R_image image, int gen_mipmaps_count,
                                                       void *dst, R_int dst_size,
                                                       R_allocator temp_allocator) {
    if (image.valid) return (R_mipmaps_image){0};

    R_mipmaps_image result = {0};
    R_mipmaps_stats mipmap_stats = R_compute_mipmaps_stats(image, gen_mipmaps_count);

    if (mipmap_stats.possible_mip_counts <= gen_mipmaps_count) {
        if (dst_size == mipmap_stats.mipmaps_buffer_size) {
            // Pointer to current mip location in the dst buffer
            unsigned char *dst_iter = dst;

            // Copy the image to the dst as the first mipmap level
            memcpy(dst_iter, image.data, R_image_size(image));
            dst_iter += R_image_size(image);

            // Create a rgba32 buffer for the mipmap result, half the image size is enough for any mipmap level
            int temp_mipmap_buffer_size = (image.width / 2) * (image.height / 2) *
                                          R_bytes_per_pixel(R_pixel_format_r8g8b8a8);
            void *temp_mipmap_buffer = R_alloc(temp_allocator, temp_mipmap_buffer_size);

            if (temp_mipmap_buffer) {
                int mip_width = image.width / 2;
                int mip_height = image.height / 2;
                int mip_count = 1;
                for (; mip_count < gen_mipmaps_count; mip_count++) {
                    R_image mipmap = R_image_resize_to_buffer(
                            image, mip_width, mip_height, temp_mipmap_buffer,
                            temp_mipmap_buffer_size, temp_allocator);

                    if (mipmap.valid) {
                        int dst_iter_size =
                                dst_size - ((int) (dst_iter - ((unsigned char *) (dst))));

                        R_bool success =
                                R_format_pixels(mipmap.data, R_image_size(mipmap), mipmap.format,
                                                dst_iter, dst_iter_size, image.format);
                        R_assert(success);
                    } else
                        break;

                    mip_width /= 2;
                    mip_height /= 2;

                    // Security check for NPOT textures
                    if (mip_width < 1) mip_width = 1;
                    if (mip_height < 1) mip_height = 1;

                    // Compute next mipmap location in the dst buffer
                    dst_iter += mip_width * mip_height * R_bytes_per_pixel(image.format);
                }

                if (mip_count == gen_mipmaps_count) {
                    result = (R_mipmaps_image){.data = dst,
                                               .width = image.width,
                                               .height = image.height,
                                               .mipmaps = gen_mipmaps_count,
                                               .format = image.format,
                                               .valid = 1};
                }
            }

            R_free(temp_allocator, temp_mipmap_buffer);
        }
    } else
        R_log(R_log_type_warning, "R_image mipmaps already available");

    return result;
}

R_public R_mipmaps_image R_image_gen_mipmaps(R_image image, int desired_mipmaps_count,
                                             R_allocator allocator, R_allocator temp_allocator) {
    if (!image.valid) return (R_mipmaps_image){0};

    R_mipmaps_image result = {0};
    R_mipmaps_stats mipmap_stats = R_compute_mipmaps_stats(image, desired_mipmaps_count);

    if (mipmap_stats.possible_mip_counts <= desired_mipmaps_count) {
        void *dst = R_alloc(allocator, mipmap_stats.mipmaps_buffer_size);

        if (dst) {
            result =
                    R_image_gen_mipmaps_to_buffer(image, desired_mipmaps_count, dst,
                                                  mipmap_stats.mipmaps_buffer_size, temp_allocator);
            if (!result.valid) { R_free(allocator, dst); }
        }
    }

    return result;
}

R_public void R_unload_mipmaps_image(R_mipmaps_image image, R_allocator allocator) {
    R_free(allocator, image.data);
}

#pragma endregion

#pragma region dds

/*
 Required extension:
 GL_EXT_texture_compression_s3tc

 Supported tokens (defined by extensions)
 GL_COMPRESSED_RGB_S3TC_DXT1_EXT      0x83F0
 GL_COMPRESSED_RGBA_S3TC_DXT1_EXT     0x83F1
 GL_COMPRESSED_RGBA_S3TC_DXT3_EXT     0x83F2
 GL_COMPRESSED_RGBA_S3TC_DXT5_EXT     0x83F3
*/

#define RF_FOURCC_DXT1 (0x31545844)// Equivalent to "DXT1" in ASCII
#define RF_FOURCC_DXT3 (0x33545844)// Equivalent to "DXT3" in ASCII
#define RF_FOURCC_DXT5 (0x35545844)// Equivalent to "DXT5" in ASCII

typedef struct R_dds_pixel_format R_dds_pixel_format;
struct R_dds_pixel_format
{
    unsigned int size;
    unsigned int flags;
    unsigned int four_cc;
    unsigned int rgb_bit_count;
    unsigned int r_bit_mask;
    unsigned int g_bit_mask;
    unsigned int b_bit_mask;
    unsigned int a_bit_mask;
};

// DDS Header (must be 124 bytes)
typedef struct R_dds_header R_dds_header;
struct R_dds_header
{
    char id[4];
    unsigned int size;
    unsigned int flags;
    unsigned int height;
    unsigned int width;
    unsigned int pitch_or_linear_size;
    unsigned int depth;
    unsigned int mipmap_count;
    unsigned int reserved_1[11];
    R_dds_pixel_format ddspf;
    unsigned int caps;
    unsigned int caps_2;
    unsigned int caps_3;
    unsigned int caps_4;
    unsigned int reserved_2;
};

R_public R_int R_get_dds_image_size(const void *src, R_int src_size) {
    int result = 0;

    if (src && src_size >= sizeof(R_dds_header)) {
        R_dds_header header = *(R_dds_header *) src;

        // Verify the type of file
        if (R_match_str_cstr(header.id, sizeof(header.id), "DDS ")) {
            if (header.ddspf.rgb_bit_count == 16)// 16bit mode, no compressed
            {
                if (header.ddspf.flags == 0x40)// no alpha channel
                {
                    result = header.width * header.height * sizeof(unsigned short);
                } else if (header.ddspf.flags == 0x41)// with alpha channel
                {
                    if (header.ddspf.a_bit_mask == 0x8000)// 1bit alpha
                    {
                        result = header.width * header.height * sizeof(unsigned short);
                    } else if (header.ddspf.a_bit_mask == 0xf000)// 4bit alpha
                    {
                        result = header.width * header.height * sizeof(unsigned short);
                    }
                }
            } else if (header.ddspf.flags == 0x40 &&
                       header.ddspf.rgb_bit_count == 24)// DDS_RGB, no compressed
            {
                // Not sure if this case exists...
                result = header.width * header.height * 3 * sizeof(unsigned char);
            } else if (header.ddspf.flags == 0x41 &&
                       header.ddspf.rgb_bit_count == 32)// DDS_RGBA, no compressed
            {
                result = header.width * header.height * 4 * sizeof(unsigned char);
            } else if (((header.ddspf.flags == 0x04) || (header.ddspf.flags == 0x05)) &&
                       (header.ddspf.four_cc > 0))// Compressed
            {
                int size;// DDS result data size

                // Calculate data size, including all mipmaps
                if (header.mipmap_count > 1) size = header.pitch_or_linear_size * 2;
                else
                    size = header.pitch_or_linear_size;

                result = size * sizeof(unsigned char);
            }
        }
    }

    return result;
}

R_public R_mipmaps_image R_load_dds_image_to_buffer(const void *src, R_int src_size, void *dst,
                                                    R_int dst_size) {
    R_mipmaps_image result = {0};

    if (src && dst && dst_size > 0 && src_size >= sizeof(R_dds_header)) {
        R_dds_header header = *(R_dds_header *) src;

        src = ((char *) src) + sizeof(R_dds_header);
        src_size -= sizeof(R_dds_header);

        // Verify the type of file
        if (R_match_str_cstr(header.id, sizeof(header.id), "DDS ")) {
            result.width = header.width;
            result.height = header.height;
            result.mipmaps = (header.mipmap_count == 0) ? 1 : header.mipmap_count;

            if (header.ddspf.rgb_bit_count == 16)// 16bit mode, no compressed
            {
                if (header.ddspf.flags == 0x40)// no alpha channel
                {
                    int dds_result_size = header.width * header.height * sizeof(unsigned short);

                    if (src_size >= dds_result_size && dst_size >= dds_result_size) {
                        memcpy(dst, src, dds_result_size);
                        result.format = R_pixel_format_r5g6b5;
                        result.data = dst;
                        result.valid = 1;
                    }
                } else if (header.ddspf.flags == 0x41)// with alpha channel
                {
                    if (header.ddspf.a_bit_mask == 0x8000)// 1bit alpha
                    {
                        int dds_result_size = header.width * header.height * sizeof(unsigned short);

                        if (src_size >= dds_result_size && dst_size >= dds_result_size) {
                            memcpy(dst, src, dds_result_size);

                            unsigned char alpha = 0;

                            // Data comes as A1R5G5B5, it must be reordered to R5G5B5A1
                            for (R_int i = 0; i < result.width * result.height; i++) {
                                alpha = ((unsigned short *) result.data)[i] >> 15;
                                ((unsigned short *) result.data)[i] =
                                        ((unsigned short *) result.data)[i] << 1;
                                ((unsigned short *) result.data)[i] += alpha;
                            }

                            result.format = R_pixel_format_r5g5b5a1;
                            result.data = dst;
                            result.valid = 1;
                        }
                    } else if (header.ddspf.a_bit_mask == 0xf000)// 4bit alpha
                    {
                        int dds_result_size = header.width * header.height * sizeof(unsigned short);

                        if (src_size >= dds_result_size && dst_size >= dds_result_size) {
                            memcpy(dst, src, dds_result_size);

                            unsigned char alpha = 0;

                            // Data comes as A4R4G4B4, it must be reordered R4G4B4A4
                            for (R_int i = 0; i < result.width * result.height; i++) {
                                alpha = ((unsigned short *) result.data)[i] >> 12;
                                ((unsigned short *) result.data)[i] =
                                        ((unsigned short *) result.data)[i] << 4;
                                ((unsigned short *) result.data)[i] += alpha;
                            }

                            result.format = R_pixel_format_r4g4b4a4;
                            result.data = dst;
                            result.valid = 1;
                        }
                    }
                }
            } else if (header.ddspf.flags == 0x40 &&
                       header.ddspf.rgb_bit_count ==
                               24)// DDS_RGB, no compressed, not sure if this case exists...
            {
                int dds_result_size = header.width * header.height * 3;

                if (src_size >= dds_result_size && dst_size >= dds_result_size) {
                    memcpy(dst, src, dds_result_size);
                    result.format = R_pixel_format_r8g8b8;
                    result.data = dst;
                    result.valid = 1;
                }
            } else if (header.ddspf.flags == 0x41 &&
                       header.ddspf.rgb_bit_count == 32)// DDS_RGBA, no compressed
            {
                int dds_result_size = header.width * header.height * 4;

                if (src_size >= dds_result_size && dst_size >= dds_result_size) {
                    memcpy(dst, src, dds_result_size);

                    unsigned char blue = 0;

                    // Data comes as A8R8G8B8, it must be reordered R8G8B8A8 (view next comment)
                    // DirecX understand ARGB as a 32bit DWORD but the actual memory byte alignment is BGRA
                    // So, we must realign B8G8R8A8 to R8G8B8A8
                    for (R_int i = 0; i < header.width * header.height * 4; i += 4) {
                        blue = ((unsigned char *) dst)[i];
                        ((unsigned char *) dst)[i + 0] = ((unsigned char *) dst)[i + 2];
                        ((unsigned char *) dst)[i + 2] = blue;
                    }

                    result.format = R_pixel_format_r8g8b8a8;
                    result.data = dst;
                    result.valid = 1;
                }
            } else if (((header.ddspf.flags == 0x04) || (header.ddspf.flags == 0x05)) &&
                       (header.ddspf.four_cc > 0))// Compressed
            {
                int dds_result_size = (header.mipmap_count > 1) ? (header.pitch_or_linear_size * 2)
                                                                : header.pitch_or_linear_size;

                if (src_size >= dds_result_size && dst_size >= dds_result_size) {
                    memcpy(dst, src, dds_result_size);

                    switch (header.ddspf.four_cc) {
                        case RF_FOURCC_DXT1:
                            result.format = header.ddspf.flags == 0x04 ? R_pixel_format_dxt1_rgb
                                                                       : R_pixel_format_dxt1_rgba;
                            break;
                        case RF_FOURCC_DXT3:
                            result.format = R_pixel_format_dxt3_rgba;
                            break;
                        case RF_FOURCC_DXT5:
                            result.format = R_pixel_format_dxt5_rgba;
                            break;
                        default:
                            break;
                    }

                    result.data = dst;
                    result.valid = 1;
                }
            }
        } else
            R_log_error(R_bad_format, "DDS file does not seem to be a valid result");
    }

    return result;
}

R_public R_mipmaps_image R_load_dds_image(const void *src, R_int src_size, R_allocator allocator) {
    R_mipmaps_image result = {0};

    int dst_size = R_get_dds_image_size(src, src_size);
    void *dst = R_alloc(allocator, dst_size);

    result = R_load_dds_image_to_buffer(src, src_size, dst, dst_size);

    return result;
}

R_public R_mipmaps_image R_load_dds_image_from_file(const char *file, R_allocator allocator,
                                                    R_allocator temp_allocator, R_io_callbacks io) {
    R_mipmaps_image result = {0};

    int src_size = R_file_size(io, file);
    void *src = R_alloc(temp_allocator, src_size);

    if (R_read_file(io, file, src, src_size)) {
        result = R_load_dds_image(src, src_size, allocator);
    }

    R_free(temp_allocator, src);

    return result;
}
#pragma endregion

#pragma region pkm

/*
 Required extensions:
 GL_OES_compressed_ETC1_RGB8_texture  (ETC1) (OpenGL ES 2.0)
 GL_ARB_ES3_compatibility  (ETC2/EAC) (OpenGL ES 3.0)

 Supported tokens (defined by extensions)
 GL_ETC1_RGB8_OES                 0x8D64
 GL_COMPRESSED_RGB8_ETC2          0x9274
 GL_COMPRESSED_RGBA8_ETC2_EAC     0x9278

 Formats list
 version 10: format: 0=ETC1_RGB, [1=ETC1_RGBA, 2=ETC1_RGB_MIP, 3=ETC1_RGBA_MIP] (not used)
 version 20: format: 0=ETC1_RGB, 1=ETC2_RGB, 2=ETC2_RGBA_OLD, 3=ETC2_RGBA, 4=ETC2_RGBA1, 5=ETC2_R, 6=ETC2_RG, 7=ETC2_SIGNED_R, 8=ETC2_SIGNED_R

 NOTE: The extended width and height are the widths rounded up to a multiple of 4.
 NOTE: ETC is always 4bit per pixel (64 bit for each 4x4 block of pixels)
*/

// PKM file (ETC1) Header (16 bytes)
typedef struct R_pkm_header R_pkm_header;
struct R_pkm_header
{
    char id[4];                // "PKM "
    char version[2];           // "10" or "20"
    unsigned short format;     // Data format (big-endian) (Check list below)
    unsigned short width;      // Texture width (big-endian) (origWidth rounded to multiple of 4)
    unsigned short height;     // Texture height (big-endian) (origHeight rounded to multiple of 4)
    unsigned short orig_width; // Original width (big-endian)
    unsigned short orig_height;// Original height (big-endian)
};

R_public R_int R_get_pkm_image_size(const void *src, R_int src_size) {
    int result = 0;

    if (src && src_size > sizeof(R_pkm_header)) {
        R_pkm_header header = *(R_pkm_header *) src;

        // Verify the type of file
        if (R_match_str_cstr(header.id, sizeof(header.id), "PKM ")) {
            // Note: format, width and height come as big-endian, data must be swapped to little-endian
            header.format = ((header.format & 0x00FF) << 8) | ((header.format & 0xFF00) >> 8);
            header.width = ((header.width & 0x00FF) << 8) | ((header.width & 0xFF00) >> 8);
            header.height = ((header.height & 0x00FF) << 8) | ((header.height & 0xFF00) >> 8);

            int bpp = 4;
            if (header.format == 3) bpp = 8;

            result = header.width * header.height * bpp / 8;
        } else
            R_log_error(R_bad_format, "PKM file does not seem to be a valid image");
    }

    return result;
}

// Load image from .pkm
R_public R_image R_load_pkm_image_to_buffer(const void *src, R_int src_size, void *dst,
                                            R_int dst_size) {
    R_image result = {0};

    if (src && src_size >= sizeof(R_pkm_header)) {
        R_pkm_header header = *(R_pkm_header *) src;

        src = (char *) src + sizeof(R_pkm_header);
        src_size -= sizeof(R_pkm_header);

        // Verify the type of file
        if (R_match_str_cstr(header.id, sizeof(header.id), "PKM ")) {
            // Note: format, width and height come as big-endian, data must be swapped to little-endian
            result.format = ((header.format & 0x00FF) << 8) | ((header.format & 0xFF00) >> 8);
            result.width = ((header.width & 0x00FF) << 8) | ((header.width & 0xFF00) >> 8);
            result.height = ((header.height & 0x00FF) << 8) | ((header.height & 0xFF00) >> 8);

            int bpp = (result.format == 3) ? 8 : 4;
            int size = result.width * result.height * bpp / 8;// Total data size in bytes

            if (dst_size >= size && src_size >= size) {
                memcpy(dst, src, size);

                if (header.format == 0) result.format = R_pixel_format_etc1_rgb;
                else if (header.format == 1)
                    result.format = R_pixel_format_etc2_rgb;
                else if (header.format == 3)
                    result.format = R_pixel_format_etc2_eac_rgba;

                result.valid = 1;
            }
        } else
            R_log_error(R_bad_format, "PKM file does not seem to be a valid image");
    }

    return result;
}

R_public R_image R_load_pkm_image(const void *src, R_int src_size, R_allocator allocator) {
    R_image result = {0};

    if (src && src_size > 0) {
        int dst_size = R_get_pkm_image_size(src, src_size);
        void *dst = R_alloc(allocator, dst_size);

        result = R_load_pkm_image_to_buffer(src, src_size, dst, dst_size);
    }

    return result;
}

R_public R_image R_load_pkm_image_from_file(const char *file, R_allocator allocator,
                                            R_allocator temp_allocator, R_io_callbacks io) {
    R_image result = {0};

    int src_size = R_file_size(io, file);
    void *src = R_alloc(temp_allocator, src_size);

    if (R_read_file(io, file, src, src_size)) {
        result = R_load_pkm_image(src, src_size, allocator);
    }

    R_free(temp_allocator, src);

    return result;
}

#pragma endregion

#pragma region ktx

/*
 Required extensions:
 GL_OES_compressed_ETC1_RGB8_texture  (ETC1)
 GL_ARB_ES3_compatibility  (ETC2/EAC)

 Supported tokens (defined by extensions)
 GL_ETC1_RGB8_OES                 0x8D64
 GL_COMPRESSED_RGB8_ETC2          0x9274
 GL_COMPRESSED_RGBA8_ETC2_EAC     0x9278

 KTX file Header (64 bytes)
 v1.1 - https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/
 v2.0 - http://github.khronos.org/KTX-Specification/

 NOTE: Before start of every mipmap data block, we have: unsigned int dataSize
 TODO: Support KTX 2.2 specs!
*/

typedef struct R_ktx_header R_ktx_header;
struct R_ktx_header
{
    char id[12];                         // Identifier: "«KTX 11»\r\n\x1A\n"
    unsigned int endianness;             // Little endian: 0x01 0x02 0x03 0x04
    unsigned int gl_type;                // For compressed textures, gl_type must equal 0
    unsigned int gl_type_size;           // For compressed texture data, usually 1
    unsigned int gl_format;              // For compressed textures is 0
    unsigned int gl_internal_format;     // Compressed internal format
    unsigned int gl_base_internal_format;// Same as gl_format (RGB, RGBA, ALPHA...)
    unsigned int width;                  // Texture image width in pixels
    unsigned int height;                 // Texture image height in pixels
    unsigned int depth;                  // For 2D textures is 0
    unsigned int elements;               // Number of array elements, usually 0
    unsigned int faces;                  // Cubemap faces, for no-cubemap = 1
    unsigned int mipmap_levels;          // Non-mipmapped textures = 1
    unsigned int key_value_data_size;    // Used to encode any arbitrary data...
};

R_public R_int R_get_ktx_image_size(const void *src, R_int src_size) {
    int result = 0;

    if (src && src_size >= sizeof(R_ktx_header)) {
        R_ktx_header header = *(R_ktx_header *) src;
        src = (char *) src + sizeof(R_ktx_header) + header.key_value_data_size;
        src_size -= sizeof(R_ktx_header) + header.key_value_data_size;

        if (R_match_str_cstr(header.id + 1, 6, "KTX 11")) {
            if (src_size > sizeof(unsigned int)) { memcpy(&result, src, sizeof(unsigned int)); }
        }
    }

    return result;
}

R_public R_mipmaps_image R_load_ktx_image_to_buffer(const void *src, R_int src_size, void *dst,
                                                    R_int dst_size) {
    R_mipmaps_image result = {0};

    if (src && src_size > sizeof(R_ktx_header)) {
        R_ktx_header header = *(R_ktx_header *) src;
        src = (char *) src + sizeof(R_ktx_header) + header.key_value_data_size;
        src_size -= sizeof(R_ktx_header) + header.key_value_data_size;

        if (R_match_str_cstr(header.id + 1, 6, "KTX 11")) {
            result.width = header.width;
            result.height = header.height;
            result.mipmaps = header.mipmap_levels;

            int image_size = 0;
            if (src_size > sizeof(unsigned int)) {
                memcpy(&image_size, src, sizeof(unsigned int));
                src = (char *) src + sizeof(unsigned int);
                src_size -= sizeof(unsigned int);

                if (image_size >= src_size && dst_size >= image_size) {
                    memcpy(dst, src, image_size);
                    result.data = dst;

                    switch (header.gl_internal_format) {
                        case 0x8D64:
                            result.format = R_pixel_format_etc1_rgb;
                            break;
                        case 0x9274:
                            result.format = R_pixel_format_etc2_rgb;
                            break;
                        case 0x9278:
                            result.format = R_pixel_format_etc2_eac_rgba;
                            break;
                        default:
                            return result;
                    }

                    result.valid = 1;
                }
            }
        }
    }

    return result;
}

R_public R_mipmaps_image R_load_ktx_image(const void *src, R_int src_size, R_allocator allocator) {
    R_mipmaps_image result = {0};

    if (src && src_size > 0) {
        int dst_size = R_get_ktx_image_size(src, src_size);
        void *dst = R_alloc(allocator, dst_size);

        result = R_load_ktx_image_to_buffer(src, src_size, dst, dst_size);
    }

    return result;
}

R_public R_mipmaps_image R_load_ktx_image_from_file(const char *file, R_allocator allocator,
                                                    R_allocator temp_allocator, R_io_callbacks io) {
    R_mipmaps_image result = {0};

    int src_size = R_file_size(io, file);
    void *src = R_alloc(temp_allocator, src_size);

    if (R_read_file(io, file, src, src_size)) {
        result = R_load_ktx_image(src, src_size, allocator);
    }

    R_free(temp_allocator, src);

    return result;
}

#pragma endregion

#pragma region gif

// Load animated GIF data
//  - R_image.data buffer includes all frames: [image#0][image#1][image#2][...]
//  - Number of frames is returned through 'frames' parameter
//  - Frames delay is returned through 'delays' parameter (int array)
//  - All frames are returned in RGBA format
R_public R_gif R_load_animated_gif(const void *data, R_int data_size, R_allocator allocator,
                                   R_allocator temp_allocator) {
    R_gif gif = {0};

    R_set_global_dependencies_allocator(temp_allocator);
    {
        int component_count = 0;
        void *loaded_gif =
                stbi_load_gif_from_memory(data, data_size, &gif.frame_delays, &gif.width,
                                          &gif.height, &gif.frames_count, &component_count, 4);

        if (loaded_gif && component_count == 4) {
            int loaded_gif_size =
                    gif.width * gif.height * R_bytes_per_pixel(R_pixel_format_r8g8b8a8);
            void *dst = R_alloc(allocator, loaded_gif_size);

            if (dst) {
                memcpy(dst, loaded_gif, loaded_gif_size);

                gif.data = dst;
                gif.format = R_pixel_format_r8g8b8a8;
                gif.valid = 1;
            }
        }

        R_free(temp_allocator, loaded_gif);
    }
    R_set_global_dependencies_allocator((R_allocator){0});

    return gif;
}

R_public R_gif R_load_animated_gif_file(const char *filename, R_allocator allocator,
                                        R_allocator temp_allocator, R_io_callbacks io) {
    R_gif result = (R_gif){0};

    int file_size = R_file_size(io, filename);
    unsigned char *buffer = R_alloc(temp_allocator, file_size);

    if (R_read_file(io, filename, buffer, file_size)) {
        result = R_load_animated_gif(buffer, file_size, allocator, temp_allocator);
    }

    R_free(temp_allocator, buffer);

    return result;
}

R_public R_sizei R_gif_frame_size(R_gif gif) {
    R_sizei result = {0};

    if (gif.valid) {
        result = (R_sizei){gif.width / gif.frames_count, gif.height / gif.frames_count};
    }

    return result;
}

// Returns an image pointing to the frame in the gif
R_public R_image R_get_frame_from_gif(R_gif gif, int frame) {
    R_image result = {0};

    if (gif.valid) {
        R_sizei size = R_gif_frame_size(gif);

        result = (R_image){
                .data = ((unsigned char *) gif.data) +
                        (size.width * size.height * R_bytes_per_pixel(gif.format)) * frame,
                .width = size.width,
                .height = size.height,
                .format = gif.format,
                .valid = 1,
        };
    }

    return result;
}

R_public void R_unload_gif(R_gif gif, R_allocator allocator) {
    if (gif.valid) {
        R_free(allocator, gif.frame_delays);
        R_unload_image(gif.image, allocator);
    }
}

#pragma endregion

#pragma region ez
#ifdef METADOT_OLD

#pragma region extract image data functions
R_public R_color *R_image_pixels_to_rgba32_ez(R_image image) {
    return R_image_pixels_to_rgba32(image, R_default_allocator);
}
R_public R_vec4 *R_image_compute_pixels_to_normalized_ez(R_image image) {
    return R_image_compute_pixels_to_normalized(image, R_default_allocator);
}
R_public R_palette R_image_extract_palette_ez(R_image image, int palette_size) {
    return R_image_extract_palette(image, palette_size, R_default_allocator);
}
#pragma endregion

#pragma region loading &unloading functions
R_public R_image R_load_image_from_file_data_ez(const void *src, int src_size) {
    return R_load_image_from_file_data(src, src_size, R_default_allocator, R_default_allocator);
}
R_public R_image R_load_image_from_hdr_file_data_ez(const void *src, int src_size) {
    return R_load_image_from_hdr_file_data(src, src_size, R_default_allocator, R_default_allocator);
}
R_public R_image R_load_image_from_file_ez(const char *filename) {
    return R_load_image_from_file(filename, R_default_allocator, R_default_allocator, R_default_io);
}
R_public void R_unload_image_ez(R_image image) { R_unload_image(image, R_default_allocator); }
#pragma endregion

#pragma region image manipulation
R_public R_image R_image_copy_ez(R_image image) { return R_image_copy(image, R_default_allocator); }

R_public R_image R_image_crop_ez(R_image image, R_rec crop) {
    return R_image_crop(image, crop, R_default_allocator);
}

R_public R_image R_image_resize_ez(R_image image, int new_width, int new_height) {
    return R_image_resize(image, new_width, new_height, R_default_allocator, R_default_allocator);
}
R_public R_image R_image_resize_nn_ez(R_image image, int new_width, int new_height) {
    return R_image_resize_nn(image, new_width, new_height, R_default_allocator);
}

R_public R_image R_image_format_ez(R_image image, R_uncompressed_pixel_format new_format) {
    return R_image_format(image, new_format, R_default_allocator);
}

R_public R_image R_image_alpha_clear_ez(R_image image, R_color color, float threshold) {
    return R_image_alpha_clear(image, color, threshold, R_default_allocator, R_default_allocator);
}
R_public R_image R_image_alpha_premultiply_ez(R_image image) {
    return R_image_alpha_premultiply(image, R_default_allocator, R_default_allocator);
}
R_public R_image R_image_alpha_crop_ez(R_image image, float threshold) {
    return R_image_alpha_crop(image, threshold, R_default_allocator);
}
R_public R_image R_image_dither_ez(R_image image, int r_bpp, int g_bpp, int b_bpp, int a_bpp) {
    return R_image_dither(image, r_bpp, g_bpp, b_bpp, a_bpp, R_default_allocator,
                          R_default_allocator);
}

R_public R_image R_image_flip_vertical_ez(R_image image) {
    return R_image_flip_vertical(image, R_default_allocator);
}
R_public R_image R_image_flip_horizontal_ez(R_image image) {
    return R_image_flip_horizontal(image, R_default_allocator);
}

R_public R_vec2 R_get_seed_for_cellular_image_ez(int seeds_per_row, int tile_size, int i) {
    return R_get_seed_for_cellular_image(seeds_per_row, tile_size, i, R_default_rand_proc);
}

R_public R_image R_gen_image_color_ez(int width, int height, R_color color) {
    return R_gen_image_color(width, height, color, R_default_allocator);
}
R_public R_image R_gen_image_gradient_v_ez(int width, int height, R_color top, R_color bottom) {
    return R_gen_image_gradient_v(width, height, top, bottom, R_default_allocator);
}
R_public R_image R_gen_image_gradient_h_ez(int width, int height, R_color left, R_color right) {
    return R_gen_image_gradient_h(width, height, left, right, R_default_allocator);
}
R_public R_image R_gen_image_gradient_radial_ez(int width, int height, float density, R_color inner,
                                                R_color outer) {
    return R_gen_image_gradient_radial(width, height, density, inner, outer, R_default_allocator);
}
R_public R_image R_gen_image_checked_ez(int width, int height, int checks_x, int checks_y,
                                        R_color col1, R_color col2) {
    return R_gen_image_checked(width, height, checks_x, checks_y, col1, col2, R_default_allocator);
}
R_public R_image R_gen_image_white_noise_ez(int width, int height, float factor) {
    return R_gen_image_white_noise(width, height, factor, R_default_rand_proc, R_default_allocator);
}
R_public R_image R_gen_image_perlin_noise_ez(int width, int height, int offset_x, int offset_y,
                                             float scale) {
    return R_gen_image_perlin_noise(width, height, offset_x, offset_y, scale, R_default_allocator);
}
R_public R_image R_gen_image_cellular_ez(int width, int height, int tile_size) {
    return R_gen_image_cellular(width, height, tile_size, R_default_rand_proc, R_default_allocator);
}
#pragma endregion

#pragma region mipmaps
R_public R_mipmaps_image R_image_gen_mipmaps_ez(R_image image, int gen_mipmaps_count) {
    return R_image_gen_mipmaps(image, gen_mipmaps_count, R_default_allocator, R_default_allocator);
}
R_public void R_unload_mipmaps_image_ez(R_mipmaps_image image) {
    R_unload_mipmaps_image(image, R_default_allocator);
}
#pragma endregion

#pragma region dds
R_public R_mipmaps_image R_load_dds_image_ez(const void *src, int src_size) {
    return R_load_dds_image(src, src_size, R_default_allocator);
}
R_public R_mipmaps_image R_load_dds_image_from_file_ez(const char *file) {
    return R_load_dds_image_from_file(file, R_default_allocator, R_default_allocator, R_default_io);
}
#pragma endregion

#pragma region pkm
R_public R_image R_load_pkm_image_ez(const void *src, int src_size) {
    return R_load_pkm_image(src, src_size, R_default_allocator);
}
R_public R_image R_load_pkm_image_from_file_ez(const char *file) {
    return R_load_pkm_image_from_file(file, R_default_allocator, R_default_allocator, R_default_io);
}
#pragma endregion

#pragma region ktx
R_public R_mipmaps_image R_load_ktx_image_ez(const void *src, int src_size) {
    return R_load_ktx_image(src, src_size, R_default_allocator);
}
R_public R_mipmaps_image R_load_ktx_image_from_file_ez(const char *file) {
    return R_load_ktx_image_from_file(file, R_default_allocator, R_default_allocator, R_default_io);
}
#pragma endregion

#endif// METADOT_OLD
#pragma endregion

// Load texture from file into GPU memory (VRAM)
R_public R_texture2d R_load_texture_from_file(const char *filename, R_allocator temp_allocator,
                                              R_io_callbacks io) {
    R_texture2d result = {0};

    R_image img = R_load_image_from_file(filename, temp_allocator, temp_allocator, io);

    result = R_load_texture_from_image(img);

    R_unload_image(img, temp_allocator);

    return result;
}

// Load texture from an image file data
R_public R_texture2d R_load_texture_from_file_data(const void *data, R_int dst_size,
                                                   R_allocator temp_allocator) {
    R_image img = R_load_image_from_file_data(data, dst_size, RF_ANY_CHANNELS, temp_allocator,
                                              temp_allocator);

    R_texture2d texture = R_load_texture_from_image(img);

    R_unload_image(img, temp_allocator);

    return texture;
}

// Load texture from image data
R_public R_texture2d R_load_texture_from_image(R_image image) {
    return R_load_texture_from_image_with_mipmaps((R_mipmaps_image){.image = image, .mipmaps = 1});
}

R_public R_texture2d R_load_texture_from_image_with_mipmaps(R_mipmaps_image image) {
    R_texture2d result = {0};

    if (image.valid) {
        result.id = R_gfx_load_texture(image.data, image.width, image.height, image.format,
                                       image.mipmaps);

        if (result.id != 0) {
            result.width = image.width;
            result.height = image.height;
            result.format = image.format;
            result.valid = 1;
        }
    } else
        R_log(R_log_type_warning, "R_texture could not be loaded from R_image");

    return result;
}

// Load cubemap from image, multiple image cubemap layouts supported
R_public R_texture_cubemap R_load_texture_cubemap_from_image(R_image image,
                                                             R_cubemap_layout_type layout_type,
                                                             R_allocator temp_allocator) {
    R_texture_cubemap cubemap = {0};

    if (layout_type == RF_CUBEMAP_AUTO_DETECT)// Try to automatically guess layout type
    {
        // Check image width/height to determine the type of cubemap provided
        if (image.width > image.height) {
            if ((image.width / 6) == image.height) {
                layout_type = RF_CUBEMAP_LINE_HORIZONTAL;
                cubemap.width = image.width / 6;
            } else if ((image.width / 4) == (image.height / 3)) {
                layout_type = RF_CUBEMAP_CROSS_FOUR_BY_TREE;
                cubemap.width = image.width / 4;
            } else if (image.width >= (int) ((float) image.height * 1.85f)) {
                layout_type = RF_CUBEMAP_PANORAMA;
                cubemap.width = image.width / 4;
            }
        } else if (image.height > image.width) {
            if ((image.height / 6) == image.width) {
                layout_type = RF_CUBEMAP_LINE_VERTICAL;
                cubemap.width = image.height / 6;
            } else if ((image.width / 3) == (image.height / 4)) {
                layout_type = RF_CUBEMAP_CROSS_THREE_BY_FOUR;
                cubemap.width = image.width / 3;
            }
        }

        cubemap.height = cubemap.width;
    }

    if (layout_type != RF_CUBEMAP_AUTO_DETECT) {
        int size = cubemap.width;

        R_image faces = {0};     // Vertical column image
        R_rec face_recs[6] = {0};// Face source rectangles
        for (R_int i = 0; i < 6; i++) face_recs[i] = (R_rec){0, 0, size, size};

        if (layout_type == RF_CUBEMAP_LINE_VERTICAL) {
            faces = image;
            for (R_int i = 0; i < 6; i++) face_recs[i].y = size * i;
        } else if (layout_type == RF_CUBEMAP_PANORAMA) {
            // TODO: Convert panorama image to square faces...
            // Ref: https://github.com/denivip/panorama/blob/master/panorama.cpp
        } else {
            if (layout_type == RF_CUBEMAP_LINE_HORIZONTAL) {
                for (R_int i = 0; i < 6; i++) { face_recs[i].x = size * i; }
            } else if (layout_type == RF_CUBEMAP_CROSS_THREE_BY_FOUR) {
                face_recs[0].x = size;
                face_recs[0].y = size;
                face_recs[1].x = size;
                face_recs[1].y = 3 * size;
                face_recs[2].x = size;
                face_recs[2].y = 0;
                face_recs[3].x = size;
                face_recs[3].y = 2 * size;
                face_recs[4].x = 0;
                face_recs[4].y = size;
                face_recs[5].x = 2 * size;
                face_recs[5].y = size;
            } else if (layout_type == RF_CUBEMAP_CROSS_FOUR_BY_TREE) {
                face_recs[0].x = 2 * size;
                face_recs[0].y = size;
                face_recs[1].x = 0;
                face_recs[1].y = size;
                face_recs[2].x = size;
                face_recs[2].y = 0;
                face_recs[3].x = size;
                face_recs[3].y = 2 * size;
                face_recs[4].x = size;
                face_recs[4].y = size;
                face_recs[5].x = 3 * size;
                face_recs[5].y = size;
            }

            // Convert image data to 6 faces in a vertical column, that's the optimum layout for loading
            R_image faces_colors = R_gen_image_color(size, size * 6, R_magenta, temp_allocator);
            faces = R_image_format(faces_colors, image.format, temp_allocator);
            R_unload_image(faces_colors, temp_allocator);

            // TODO: R_image formating does not work with compressed textures!
        }

        for (R_int i = 0; i < 6; i++) {
            R_image_draw(&faces, image, face_recs[i], (R_rec){0, size * i, size, size}, R_white,
                         temp_allocator);
        }

        cubemap.id = R_gfx_load_texture_cubemap(faces.data, size, faces.format);

        if (cubemap.id == 0) { R_log(R_log_type_warning, "Cubemap image could not be loaded."); }

        R_unload_image(faces, temp_allocator);
    } else
        R_log(R_log_type_warning, "Cubemap image layout can not be detected.");

    return cubemap;
}

// Load texture for rendering (framebuffer)
R_public R_render_texture2d R_load_render_texture(int width, int height) {
    R_render_texture2d target =
            R_gfx_load_render_texture(width, height, R_pixel_format_r8g8b8a8, 24, 0);

    return target;
}

// Update GPU texture with new data. Pixels data must match texture.format
R_public void R_update_texture(R_texture2d texture, const void *pixels, R_int pixels_size) {
    R_gfx_update_texture(texture.id, texture.width, texture.height, texture.format, pixels,
                         pixels_size);
}

// Generate GPU mipmaps for a texture
R_public void R_gen_texture_mipmaps(R_texture2d *texture) {
    // NOTE: NPOT textures support check inside function
    // On WebGL (OpenGL ES 2.0) NPOT textures support is limited
    R_gfx_generate_mipmaps(texture);
}

// Set texture wrapping mode
R_public void R_set_texture_wrap(R_texture2d texture, R_texture_wrap_mode wrap_mode) {
    R_gfx_set_texture_wrap(texture, wrap_mode);
}

// Set texture scaling filter mode
R_public void R_set_texture_filter(R_texture2d texture, R_texture_filter_mode filter_mode) {
    R_gfx_set_texture_filter(texture, filter_mode);
}

// Unload texture from GPU memory (VRAM)
R_public void R_unload_texture(R_texture2d texture) {
    if (texture.id > 0) {
        R_gfx_delete_textures(texture.id);

        R_log(R_log_type_info, "[TEX ID %i] Unloaded texture data from VRAM (GPU)", texture.id);
    }
}

// Unload render texture from GPU memory (VRAM)
R_public void R_unload_render_texture(R_render_texture2d target) {
    if (target.id > 0) {
        R_gfx_delete_render_textures(target);

        R_log(R_log_type_info, "[TEX ID %i] Unloaded render texture data from VRAM (GPU)",
              target.id);
    }
}

#pragma region dependencies

#pragma region stb_truetype
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(sz, u) R_alloc(R__global_allocator_for_dependencies, sz)
#define STBTT_free(p, u) R_free(R__global_allocator_for_dependencies, p)
#define STBTT_assert(it) R_assert(it)
#define STBTT_STATIC
#include "Libs/external/stb_truetype.h"
#pragma endregion

#pragma endregion

#pragma region ttf font

R_public R_ttf_font_info R_parse_ttf_font(const void *ttf_data, R_int font_size) {
    R_ttf_font_info result = {0};

    if (ttf_data && font_size > 0) {
        stbtt_fontinfo font_info = {0};
        if (stbtt_InitFont(&font_info, ttf_data, 0)) {
            // Calculate font scale factor
            float scale_factor = stbtt_ScaleForPixelHeight(&font_info, (float) font_size);

            // Calculate font basic metrics
            // NOTE: ascent is equivalent to font baseline
            int ascent, descent, line_gap;
            stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);

            result = (R_ttf_font_info){
                    .ttf_data = ttf_data,
                    .font_size = font_size,
                    .scale_factor = scale_factor,
                    .ascent = ascent,
                    .descent = descent,
                    .line_gap = line_gap,
                    .valid = 1,
            };

            R_assert(sizeof(stbtt_fontinfo) == sizeof(result.internal_stb_font_info));
            memcpy(&result.internal_stb_font_info, &font_info, sizeof(stbtt_fontinfo));
        } else
            R_log_error(R_stbtt_failed, "STB failed to parse ttf font.");
    }

    return result;
}

R_public void R_compute_ttf_font_glyph_metrics(R_ttf_font_info *font_info, const int *codepoints,
                                               R_int codepoints_count, R_glyph_info *dst,
                                               R_int dst_count) {
    if (font_info && font_info->valid) {
        if (dst && dst_count >= codepoints_count) {
            // The stbtt functions called here should not require any allocations
            R_set_global_dependencies_allocator((R_allocator){0});

            font_info->largest_glyph_size = 0;

            float required_area = 0;

            // NOTE: Using simple packaging, one char after another
            for (R_int i = 0; i < codepoints_count; i++) {
                stbtt_fontinfo *stbtt_ctx = (stbtt_fontinfo *) &font_info->internal_stb_font_info;

                dst[i].codepoint = codepoints[i];

                int begin_x = 0;
                int begin_y = 0;
                int end_x = 0;
                int end_y = 0;
                stbtt_GetCodepointBitmapBox(stbtt_ctx, dst[i].codepoint, font_info->scale_factor,
                                            font_info->scale_factor, &begin_x, &begin_y, &end_x,
                                            &end_y);

                dst[i].rec.width = end_x - begin_x;
                dst[i].rec.height = end_y - begin_y;
                dst[i].offset_x = begin_x;
                dst[i].offset_y = begin_y;
                dst[i].offset_y += (int) ((float) font_info->ascent * font_info->scale_factor);

                stbtt_GetCodepointHMetrics(stbtt_ctx, dst[i].codepoint, &dst[i].advance_x, NULL);
                dst[i].advance_x *= font_info->scale_factor;

                const int char_size = dst[i].rec.width * dst[i].rec.height;
                if (char_size > font_info->largest_glyph_size)
                    font_info->largest_glyph_size = char_size;
            }
        }
    }
}

// Note: the atlas is square and this value is the width of the atlas
R_public int R_compute_ttf_font_atlas_width(int padding, R_glyph_info *glyph_metrics,
                                            R_int glyphs_count) {
    int result = 0;

    for (R_int i = 0; i < glyphs_count; i++) {
        // Calculate the area of all glyphs + padding
        // The padding is applied both on the (left and right) and (top and bottom) of the glyph, which is why we multiply by 2
        result += ((glyph_metrics[i].rec.width + 2 * padding) *
                   (glyph_metrics[i].rec.height + 2 * padding));
    }

    // Calculate the width required for a square atlas containing all glyphs
    result = R_next_pot(sqrtf(result) * 1.3f);

    return result;
}

R_public R_image R_generate_ttf_font_atlas(R_ttf_font_info *font_info, int atlas_width, int padding,
                                           R_glyph_info *glyphs, R_int glyphs_count,
                                           R_font_antialias antialias, unsigned short *dst,
                                           R_int dst_count, R_allocator temp_allocator) {
    R_image result = {0};

    if (font_info && font_info->valid) {
        int atlas_pixel_count = atlas_width * atlas_width;

        if (dst && dst_count >= atlas_pixel_count) {
            memset(dst, 0, atlas_pixel_count * 2);

            int largest_glyph_size = 0;
            for (R_int i = 0; i < glyphs_count; i++) {
                int area = glyphs[i].rec.width * glyphs[i].rec.height;
                if (area > largest_glyph_size) { largest_glyph_size = area; }
            }

            // Allocate a grayscale buffer large enough to store the largest glyph
            unsigned char *glyph_buffer = R_alloc(temp_allocator, largest_glyph_size);

            // Use basic packing algorithm to generate the atlas
            if (glyph_buffer) {
                // We update these for every pixel in the loop
                int offset_x = RF_BUILTIN_FONT_PADDING;
                int offset_y = RF_BUILTIN_FONT_PADDING;

                // Set the allocator for stbtt
                R_set_global_dependencies_allocator(temp_allocator);
                {
                    // Using simple packaging, one char after another
                    for (R_int i = 0; i < glyphs_count; i++) {
                        // Extract these variables to shorter names
                        stbtt_fontinfo *stbtt_ctx =
                                (stbtt_fontinfo *) &font_info->internal_stb_font_info;

                        // Get glyph bitmap
                        stbtt_MakeCodepointBitmap(stbtt_ctx, glyph_buffer, glyphs[i].rec.width,
                                                  glyphs[i].rec.height, glyphs[i].rec.width,
                                                  font_info->scale_factor, font_info->scale_factor,
                                                  glyphs[i].codepoint);

                        // Copy pixel data from fc.data to atlas
                        for (R_int y = 0; y < glyphs[i].rec.height; y++) {
                            for (R_int x = 0; x < glyphs[i].rec.width; x++) {
                                unsigned char glyph_pixel =
                                        glyph_buffer[y * ((int) glyphs[i].rec.width) + x];
                                if (antialias == RF_FONT_NO_ANTIALIAS &&
                                    glyph_pixel > RF_BITMAP_ALPHA_THRESHOLD)
                                    glyph_pixel = 0;

                                int dst_index = (offset_y + y) * atlas_width + (offset_x + x);
                                // dst is in RF_UNCOMPRESSED_GRAY_ALPHA which is 2 bytes
                                // for fonts we write the glyph_pixel in the alpha channel which is byte 2
                                ((unsigned char *) (&dst[dst_index]))[0] = 255;
                                ((unsigned char *) (&dst[dst_index]))[1] = glyph_pixel;
                            }
                        }

                        // Fill chars rectangles in atlas info
                        glyphs[i].rec.x = (float) offset_x;
                        glyphs[i].rec.y = (float) offset_y;

                        // Move atlas position X for next character drawing
                        offset_x += (glyphs[i].rec.width + 2 * padding);
                        if (offset_x >= (atlas_width - glyphs[i].rec.width - 2 * padding)) {
                            offset_x = padding;
                            offset_y += font_info->font_size + padding;

                            if (offset_y > (atlas_width - font_info->font_size - padding)) break;
                            //else error dst buffer is not big enough
                        }
                    }
                }
                R_set_global_dependencies_allocator((R_allocator){0});

                result.data = dst;
                result.width = atlas_width;
                result.height = atlas_width;
                result.format = R_pixel_format_gray_alpha;
                result.valid = 1;
            }

            R_free(temp_allocator, glyph_buffer);
        }
    }

    return result;
}

R_public R_font R_ttf_font_from_atlas(int font_size, R_image atlas, R_glyph_info *glyph_metrics,
                                      R_int glyphs_count) {
    R_font result = {0};

    R_texture2d texture = R_load_texture_from_image(atlas);

    if (texture.valid) {
        result = (R_font){
                .glyphs = glyph_metrics,
                .glyphs_count = glyphs_count,
                .texture = texture,
                .base_size = font_size,
                .valid = 1,
        };
    }

    return result;
}

// Load R_font from TTF font file with generation parameters
// NOTE: You can pass an array with desired characters, those characters should be available in the font
// if array is NULL, default char set is selected 32..126
R_public R_font R_load_ttf_font_from_data(const void *font_file_data, int font_size,
                                          R_font_antialias antialias, const int *chars,
                                          R_int char_count, R_allocator allocator,
                                          R_allocator temp_allocator) {
    R_font result = {0};

    R_ttf_font_info font_info = R_parse_ttf_font(font_file_data, font_size);

    // Get the glyph metrics
    R_glyph_info *glyph_metrics = R_alloc(allocator, char_count * sizeof(R_glyph_info));
    R_compute_ttf_font_glyph_metrics(&font_info, chars, char_count, glyph_metrics, char_count);

    // Build the atlas and font
    int atlas_size =
            R_compute_ttf_font_atlas_width(RF_BUILTIN_FONT_PADDING, glyph_metrics, char_count);
    int atlas_pixel_count = atlas_size * atlas_size;
    unsigned short *atlas_buffer =
            R_alloc(temp_allocator, atlas_pixel_count * sizeof(unsigned short));
    R_image atlas = R_generate_ttf_font_atlas(&font_info, atlas_size, RF_BUILTIN_FONT_PADDING,
                                              glyph_metrics, char_count, antialias, atlas_buffer,
                                              atlas_pixel_count, temp_allocator);

    // Get the font
    result = R_ttf_font_from_atlas(font_size, atlas, glyph_metrics, char_count);

    // Free the atlas bitmap
    R_free(temp_allocator, atlas_buffer);

    return result;
}

// Load R_font from file into GPU memory (VRAM)
R_public R_font R_load_ttf_font_from_file(const char *filename, int font_size,
                                          R_font_antialias antialias, R_allocator allocator,
                                          R_allocator temp_allocator, R_io_callbacks io) {
    R_font font = {0};

    if (R_is_file_extension(filename, ".ttf") || R_is_file_extension(filename, ".otf")) {
        int file_size = R_file_size(io, filename);
        void *data = R_alloc(temp_allocator, file_size);

        if (R_read_file(io, filename, data, file_size)) {
            font = R_load_ttf_font_from_data(
                    data, RF_DEFAULT_FONT_SIZE, antialias, (int[]) RF_BUILTIN_FONT_CHARS,
                    RF_BUILTIN_CODEPOINTS_COUNT, allocator, temp_allocator);

            // By default we set point filter (best performance)
            R_set_texture_filter(font.texture, RF_FILTER_POINT);
        }

        R_free(temp_allocator, data);
    }

    return font;
}

#pragma endregion

#pragma region image font

R_public R_bool R_compute_glyph_metrics_from_image(R_image image, R_color key,
                                                   const int *codepoints, R_glyph_info *dst,
                                                   R_int codepoints_and_dst_count) {
    R_bool result = 0;

    if (image.valid && codepoints && dst && codepoints_and_dst_count > 0) {
        const int bpp = R_bytes_per_pixel(image.format);
        const int image_data_size = R_image_size(image);

// This macro uses `bpp` and returns the pixel from the image at the index provided by calling R_format_one_pixel_to_rgba32.
#define RF_GET_PIXEL(index)                                                                        \
    R_format_one_pixel_to_rgba32((char *) image.data + ((index) *bpp), image.format)

        // Parse image data to get char_spacing and line_spacing
        int char_spacing = 0;
        int line_spacing = 0;
        {
            int x = 0;
            int y = 0;
            for (y = 0; y < image.height; y++) {
                R_color pixel = {0};
                for (x = 0; x < image.width; x++) {
                    pixel = RF_GET_PIXEL(y * image.width + x);
                    if (!R_color_match(pixel, key)) break;
                }

                if (!R_color_match(pixel, key)) break;
            }
            char_spacing = x;
            line_spacing = y;
        }

        // Compute char height
        int char_height = 0;
        {
            while (!R_color_match(
                    RF_GET_PIXEL((line_spacing + char_height) * image.width + char_spacing), key)) {
                char_height++;
            }
        }

        // Check array values to get characters: value, x, y, w, h
        int index = 0;
        int line_to_read = 0;
        int x_pos_to_read = char_spacing;

        // Parse image data to get rectangle sizes
        while ((line_spacing + line_to_read * (char_height + line_spacing)) < image.height &&
               index < codepoints_and_dst_count) {
            while (x_pos_to_read < image.width &&
                   !R_color_match(RF_GET_PIXEL((line_spacing +
                                                (char_height + line_spacing) * line_to_read) *
                                                       image.width +
                                               x_pos_to_read),
                                  key)) {
                int char_width = 0;
                while (!R_color_match(
                        RF_GET_PIXEL(((line_spacing + (char_height + line_spacing) * line_to_read) *
                                              image.width +
                                      x_pos_to_read + char_width)),
                        key)) {
                    char_width++;
                }

                dst[index].codepoint = codepoints[index];
                dst[index].rec.x = (float) x_pos_to_read;
                dst[index].rec.y =
                        (float) (line_spacing + line_to_read * (char_height + line_spacing));
                dst[index].rec.width = (float) char_width;
                dst[index].rec.height = (float) char_height;

                // On image based fonts (XNA style), character offsets and x_advance are not required (set to 0)
                dst[index].offset_x = 0;
                dst[index].offset_y = 0;
                dst[index].advance_x = 0;

                index++;

                x_pos_to_read += (char_width + char_spacing);
            }

            line_to_read++;
            x_pos_to_read = char_spacing;
        }

        result = 1;

#undef RF_GET_PIXEL
    }

    return result;
}

R_public R_font R_load_image_font_from_data(R_image image, R_glyph_info *glyphs,
                                            R_int glyphs_count) {
    R_font result = {
            .texture = R_load_texture_from_image(image),
            .glyphs = glyphs,
            .glyphs_count = glyphs_count,
    };

    if (image.valid && glyphs && glyphs_count > 0) {
        result.base_size = glyphs[0].rec.height;
        result.valid = 1;
    }

    return result;
}

R_public R_font R_load_image_font(R_image image, R_color key, R_allocator allocator) {
    R_font result = {0};

    if (image.valid) {
        const int codepoints[] = RF_BUILTIN_FONT_CHARS;
        const int codepoints_count = RF_BUILTIN_CODEPOINTS_COUNT;

        R_glyph_info *glyphs = R_alloc(allocator, codepoints_count * sizeof(R_glyph_info));

        R_compute_glyph_metrics_from_image(image, key, codepoints, glyphs, codepoints_count);

        result = R_load_image_font_from_data(image, glyphs, codepoints_count);
    }

    return result;
}

R_public R_font R_load_image_font_from_file(const char *path, R_color key, R_allocator allocator,
                                            R_allocator temp_allocator, R_io_callbacks io) {
    R_font result = {0};

    R_image image = R_load_image_from_file(path, temp_allocator, temp_allocator, io);

    result = R_load_image_font(image, key, allocator);

    R_unload_image(image, temp_allocator);

    return result;
}

#pragma endregion

// Unload R_font from GPU memory (VRAM)
R_public void R_unload_font(R_font font, R_allocator allocator) {
    if (font.valid) {
        R_unload_texture(font.texture);
        R_free(allocator, font.glyphs);
    }
}

// Returns index position for a unicode character on spritefont
R_public R_glyph_index R_get_glyph_index(R_font font, int character) {
    R_glyph_index result = RF_GLYPH_NOT_FOUND;

    for (R_int i = 0; i < font.glyphs_count; i++) {
        if (font.glyphs[i].codepoint == character) {
            result = i;
            break;
        }
    }

    return result;
}

R_public int R_font_height(R_font font, float font_size) {
    float scale_factor = font_size / font.base_size;
    return (font.base_size + font.base_size / 2) * scale_factor;
}

R_public R_sizef R_measure_text(R_font font, const char *text, float font_size,
                                float extra_spacing) {
    R_sizef result = R_measure_string(font, text, strlen(text), font_size, extra_spacing);
    return result;
}

R_public R_sizef R_measure_text_rec(R_font font, const char *text, R_rec rec, float font_size,
                                    float extra_spacing, R_bool wrap) {
    R_sizef result =
            R_measure_string_rec(font, text, strlen(text), rec, font_size, extra_spacing, wrap);
    return result;
}

R_public R_sizef R_measure_string(R_font font, const char *text, int len, float font_size,
                                  float extra_spacing) {
    R_sizef result = {0};

    if (font.valid) {
        int temp_len = 0;// Used to count longer text line num chars
        int len_counter = 0;

        float text_width = 0.0f;
        float temp_text_width = 0.0f;// Used to count longer text line width

        float text_height = (float) font.base_size;
        float scale_factor = font_size / (float) font.base_size;

        int letter = 0;// Current character
        int index = 0; // Index position in sprite font

        for (R_int i = 0; i < len; i++) {
            len_counter++;

            R_decoded_rune decoded_rune = R_decode_utf8_char(&text[i], len - i);
            index = R_get_glyph_index(font, decoded_rune.codepoint);

            // NOTE: normally we exit the decoding sequence as soon as a bad unsigned char is found (and return 0x3f)
            // but we need to draw all of the bad bytes using the '?' symbol so to not skip any we set next = 1
            if (letter == 0x3f) { decoded_rune.bytes_processed = 1; }
            i += decoded_rune.bytes_processed - 1;

            if (letter != '\n') {
                if (font.glyphs[index].advance_x != 0) {
                    text_width += font.glyphs[index].advance_x;
                } else {
                    text_width += (font.glyphs[index].rec.width + font.glyphs[index].offset_x);
                }
            } else {
                if (temp_text_width < text_width) { temp_text_width = text_width; }

                len_counter = 0;
                text_width = 0;
                text_height +=
                        ((float) font.base_size * 1.5f);// NOTE: Fixed line spacing of 1.5 lines
            }

            if (temp_len < len_counter) { temp_len = len_counter; }
        }

        if (temp_text_width < text_width) temp_text_width = text_width;

        result.width = temp_text_width * scale_factor +
                       (float) ((temp_len - 1) * extra_spacing);// Adds chars spacing to measure
        result.height = text_height * scale_factor;
    }

    return result;
}

R_public R_sizef R_measure_string_rec(R_font font, const char *text, int text_len, R_rec rec,
                                      float font_size, float extra_spacing, R_bool wrap) {
    R_sizef result = {0};

    if (font.valid) {
        int text_offset_x = 0;// Offset between characters
        int text_offset_y = 0;// Required for line break!
        float scale_factor = 0.0f;

        int letter = 0;// Current character
        int index = 0; // Index position in sprite font

        scale_factor = font_size / font.base_size;

        enum {
            MEASURE_WRAP_STATE = 0,
            MEASURE_REGULAR_STATE = 1
        };

        int state = wrap ? MEASURE_WRAP_STATE : MEASURE_REGULAR_STATE;
        int start_line = -1;// Index where to begin drawing (where a line begins)
        int end_line = -1;  // Index where to stop drawing (where a line ends)
        int lastk = -1;     // Holds last value of the character position

        int max_y = 0;
        int first_y = 0;
        R_bool first_y_set = 0;

        for (R_int i = 0, k = 0; i < text_len; i++, k++) {
            int glyph_width = 0;

            R_decoded_rune decoded_rune = R_decode_utf8_char(&text[i], text_len - i);
            letter = decoded_rune.codepoint;
            index = R_get_glyph_index(font, letter);

            // NOTE: normally we exit the decoding sequence as soon as a bad unsigned char is found (and return 0x3f)
            // but we need to draw all of the bad bytes using the '?' symbol so to not skip any we set next = 1
            if (letter == 0x3f) decoded_rune.bytes_processed = 1;
            i += decoded_rune.bytes_processed - 1;

            if (letter != '\n') {
                glyph_width = (font.glyphs[index].advance_x == 0)
                                      ? (int) (font.glyphs[index].rec.width * scale_factor +
                                               extra_spacing)
                                      : (int) (font.glyphs[index].advance_x * scale_factor +
                                               extra_spacing);
            }

            // NOTE: When word_wrap is ON we first measure how much of the text we can draw before going outside of the rec container
            // We store this info in start_line and end_line, then we change states, draw the text between those two variables
            // and change states again and again recursively until the end of the text (or until we get outside of the container).
            // When word_wrap is OFF we don't need the measure state so we go to the drawing state immediately
            // and begin drawing on the next line before we can get outside the container.
            if (state == MEASURE_WRAP_STATE) {
                // TODO: there are multiple types of spaces in UNICODE, maybe it's a good idea to add support for more
                // See: http://jkorpela.fi/chars/spaces.html
                if ((letter == ' ') || (letter == '\t') || (letter == '\n')) { end_line = i; }

                if ((text_offset_x + glyph_width + 1) >= rec.width) {
                    end_line = (end_line < 1) ? i : end_line;
                    if (i == end_line) { end_line -= decoded_rune.bytes_processed; }
                    if ((start_line + decoded_rune.bytes_processed) == end_line) {
                        end_line = i - decoded_rune.bytes_processed;
                    }
                    state = !state;
                } else if ((i + 1) == text_len) {
                    end_line = i;
                    state = !state;
                } else if (letter == '\n') {
                    state = !state;
                }

                if (state == MEASURE_REGULAR_STATE) {
                    text_offset_x = 0;
                    i = start_line;
                    glyph_width = 0;

                    // Save character position when we switch states
                    int tmp = lastk;
                    lastk = k - 1;
                    k = tmp;
                }
            } else {
                if (letter == '\n') {
                    if (!wrap) {
                        text_offset_y +=
                                (int) ((font.base_size + font.base_size / 2) * scale_factor);
                        text_offset_x = 0;
                    }
                } else {
                    if (!wrap && (text_offset_x + glyph_width + 1) >= rec.width) {
                        text_offset_y +=
                                (int) ((font.base_size + font.base_size / 2) * scale_factor);
                        text_offset_x = 0;
                    }

                    if ((text_offset_y + (int) (font.base_size * scale_factor)) > rec.height) break;

                    // The right side expression is the offset of the latest character plus its width (so the end of the line)
                    // We want the highest value of that expression by the end of the function
                    result.width = R_max_f(result.width, rec.x + text_offset_x - 1 + glyph_width);

                    if (!first_y_set) {
                        first_y = rec.y + text_offset_y;
                        first_y_set = 1;
                    }

                    max_y = R_max_i(max_y, rec.y + text_offset_y + font.base_size * scale_factor);
                }

                if (wrap && i == end_line) {
                    text_offset_y += (int) ((font.base_size + font.base_size / 2) * scale_factor);
                    text_offset_x = 0;
                    start_line = end_line;
                    end_line = -1;
                    glyph_width = 0;
                    k = lastk;
                    state = !state;
                }
            }

            text_offset_x += glyph_width;
        }

        result.height = max_y - first_y;
    }

    return result;
}

#pragma region dependencies

#pragma region par shapes
#define PAR_SHAPES_IMPLEMENTATION
#define PAR_MALLOC(T, N) ((T *) R_alloc(R__global_allocator_for_dependencies, N * sizeof(T)))
#define PAR_CALLOC(T, N)                                                                           \
    ((T *) R_calloc_wrapper(R__global_allocator_for_dependencies, N, sizeof(T)))
#define PAR_FREE(BUF) (R_free(R__global_allocator_for_dependencies, BUF))
#define PAR_REALLOC(T, BUF, N, OLD_SZ)                                                             \
    ((T *) R_default_realloc(R__global_allocator_for_dependencies, BUF, sizeof(T) * (N), (OLD_SZ)))
#define PARDEF R_internal
#include "Libs/external/par_shapes.h"
#pragma endregion

#pragma region tinyobj loader
R_internal R_thread_local R_io_callbacks R__tinyobj_io;
#define RF_SET_TINYOBJ_ALLOCATOR(allocator) R__tinyobj_allocator = allocator
#define R_set_tinyobj_io_callbacks(io) R__tinyobj_io = io;

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#define TINYOBJ_MALLOC(size) (R_alloc(R__global_allocator_for_dependencies, (size)))
#define TINYOBJ_REALLOC(p, oldsz, newsz)                                                           \
    (R_default_realloc(R__global_allocator_for_dependencies, (p), (oldsz), (newsz)))
#define TINYOBJ_CALLOC(amount, size)                                                               \
    (R_calloc_wrapper(R__global_allocator_for_dependencies, (amount), (size)))
#define TINYOBJ_FREE(p) (R_free(R__global_allocator_for_dependencies, (p)))
#define TINYOBJDEF R_internal
#include "Libs/external/tinyobjloader.h"

R_internal void R_tinyobj_file_reader_callback(const char *filename, char **buf, size_t *len) {
    if (!filename || !buf || !len) return;

    *len = R_file_size(R__tinyobj_io, filename);

    if (*len) {
        if (!R_read_file(R__tinyobj_io, filename, *buf, *len)) {
            // On error we set the size of output buffer to 0
            *len = 0;
        }
    }
}
#pragma endregion

#pragma region cgltf
#define CGLTF_IMPLEMENTATION
#define CGLTF_MALLOC(size) R_alloc(R__global_allocator_for_dependencies, size)
#define CGLTF_FREE(ptr) R_free(R__global_allocator_for_dependencies, ptr)
#include "Libs/external/cgltf.h"

R_internal cgltf_result R_cgltf_io_read(const struct cgltf_memory_options *memory_options,
                                        const struct cgltf_file_options *file_options,
                                        const char *path, cgltf_size *size, void **data) {
    ((void) memory_options);
    ((void) file_options);

    cgltf_result result = cgltf_result_file_not_found;
    R_io_callbacks *io = (R_io_callbacks *) file_options->user_data;

    int file_size = R_file_size(*io, path);

    if (file_size > 0) {
        void *dst = CGLTF_MALLOC(file_size);

        if (dst == NULL) {
            if (R_read_file(*io, path, data, file_size) && data && size) {
                *data = dst;
                *size = file_size;
                result = cgltf_result_success;
            } else {
                CGLTF_FREE(dst);
                result = cgltf_result_io_error;
            }
        } else {
            result = cgltf_result_out_of_memory;
        }
    }

    return result;
}

R_internal void R_cgltf_io_release(const struct cgltf_memory_options *memory_options,
                                   const struct cgltf_file_options *file_options, void *data) {
    ((void) memory_options);
    ((void) file_options);

    CGLTF_FREE(data);
}
#pragma endregion

#pragma endregion

R_internal R_model R_load_meshes_and_materials_for_model(R_model model, R_allocator allocator,
                                                         R_allocator temp_allocator) {
    // Make sure model transform is set to identity matrix!
    model.transform = R_mat_identity();

    if (model.mesh_count == 0) {
        R_log(R_log_type_warning, "No meshes can be loaded, default to cube mesh.");

        model.mesh_count = 1;
        model.meshes = (R_mesh *) R_alloc(allocator, sizeof(R_mesh));
        memset(model.meshes, 0, sizeof(R_mesh));
        model.meshes[0] = R_gen_mesh_cube(1.0f, 1.0f, 1.0f, allocator, temp_allocator);
    } else {
        // Upload vertex data to GPU (static mesh)
        for (R_int i = 0; i < model.mesh_count; i++) R_gfx_load_mesh(&model.meshes[i], false);
    }

    if (model.material_count == 0) {
        R_log(R_log_type_warning, "No materials can be loaded, default to white material.");

        model.material_count = 1;
        model.materials = (R_material *) R_alloc(allocator, sizeof(R_material));
        memset(model.materials, 0, sizeof(R_material));
        model.materials[0] = R_load_default_material(allocator);

        if (model.mesh_material == NULL) {
            model.mesh_material = (int *) R_alloc(allocator, model.mesh_count * sizeof(int));
            memset(model.mesh_material, 0, model.mesh_count * sizeof(int));
        }
    }

    return model;
}

// Compute mesh bounding box limits. Note: min_vertex and max_vertex should be transformed by model transform matrix
R_public R_bounding_box R_mesh_bounding_box(R_mesh mesh) {
    // Get min and max vertex to construct bounds (AABB)
    R_vec3 min_vertex = {0};
    R_vec3 max_vertex = {0};

    if (mesh.vertices != NULL) {
        min_vertex = (R_vec3){mesh.vertices[0], mesh.vertices[1], mesh.vertices[2]};
        max_vertex = (R_vec3){mesh.vertices[0], mesh.vertices[1], mesh.vertices[2]};

        for (R_int i = 1; i < mesh.vertex_count; i++) {
            min_vertex =
                    R_vec3_min(min_vertex, (R_vec3){mesh.vertices[i * 3], mesh.vertices[i * 3 + 1],
                                                    mesh.vertices[i * 3 + 2]});
            max_vertex =
                    R_vec3_max(max_vertex, (R_vec3){mesh.vertices[i * 3], mesh.vertices[i * 3 + 1],
                                                    mesh.vertices[i * 3 + 2]});
        }
    }

    // Create the bounding box
    R_bounding_box box = {0};
    box.min = min_vertex;
    box.max = max_vertex;

    return box;
}

// Compute mesh tangents
// NOTE: To calculate mesh tangents and binormals we need mesh vertex positions and texture coordinates
// Implementation base don: https://answers.unity.com/questions/7789/calculating-tangents-vector4.html
R_public void R_mesh_compute_tangents(R_mesh *mesh, R_allocator allocator,
                                      R_allocator temp_allocator) {
    if (mesh->tangents == NULL)
        mesh->tangents = (float *) R_alloc(allocator, mesh->vertex_count * 4 * sizeof(float));
    else
        R_log(R_log_type_warning, "R_mesh tangents already exist");

    R_vec3 *tan1 = (R_vec3 *) R_alloc(temp_allocator, mesh->vertex_count * sizeof(R_vec3));
    R_vec3 *tan2 = (R_vec3 *) R_alloc(temp_allocator, mesh->vertex_count * sizeof(R_vec3));

    for (R_int i = 0; i < mesh->vertex_count; i += 3) {
        // Get triangle vertices
        R_vec3 v1 = {mesh->vertices[(i + 0) * 3 + 0], mesh->vertices[(i + 0) * 3 + 1],
                     mesh->vertices[(i + 0) * 3 + 2]};
        R_vec3 v2 = {mesh->vertices[(i + 1) * 3 + 0], mesh->vertices[(i + 1) * 3 + 1],
                     mesh->vertices[(i + 1) * 3 + 2]};
        R_vec3 v3 = {mesh->vertices[(i + 2) * 3 + 0], mesh->vertices[(i + 2) * 3 + 1],
                     mesh->vertices[(i + 2) * 3 + 2]};

        // Get triangle texcoords
        R_vec2 uv1 = {mesh->texcoords[(i + 0) * 2 + 0], mesh->texcoords[(i + 0) * 2 + 1]};
        R_vec2 uv2 = {mesh->texcoords[(i + 1) * 2 + 0], mesh->texcoords[(i + 1) * 2 + 1]};
        R_vec2 uv3 = {mesh->texcoords[(i + 2) * 2 + 0], mesh->texcoords[(i + 2) * 2 + 1]};

        float x1 = v2.x - v1.x;
        float y1 = v2.y - v1.y;
        float z1 = v2.z - v1.z;
        float x2 = v3.x - v1.x;
        float y2 = v3.y - v1.y;
        float z2 = v3.z - v1.z;

        float s1 = uv2.x - uv1.x;
        float t1 = uv2.y - uv1.y;
        float s2 = uv3.x - uv1.x;
        float t2 = uv3.y - uv1.y;

        float div = s1 * t2 - s2 * t1;
        float r = (div == 0.0f) ? (0.0f) : (1.0f / div);

        R_vec3 sdir = {(t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r};
        R_vec3 tdir = {(s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r};

        tan1[i + 0] = sdir;
        tan1[i + 1] = sdir;
        tan1[i + 2] = sdir;

        tan2[i + 0] = tdir;
        tan2[i + 1] = tdir;
        tan2[i + 2] = tdir;
    }

    // Compute tangents considering normals
    for (R_int i = 0; i < mesh->vertex_count; ++i) {
        R_vec3 normal = {mesh->normals[i * 3 + 0], mesh->normals[i * 3 + 1],
                         mesh->normals[i * 3 + 2]};
        R_vec3 tangent = tan1[i];

        // TODO: Review, not sure if tangent computation is right, just used reference proposed maths...
        R_vec3_ortho_normalize(&normal, &tangent);
        mesh->tangents[i * 4 + 0] = tangent.x;
        mesh->tangents[i * 4 + 1] = tangent.y;
        mesh->tangents[i * 4 + 2] = tangent.z;
        mesh->tangents[i * 4 + 3] =
                (R_vec3_dot_product(R_vec3_cross_product(normal, tangent), tan2[i]) < 0.0f) ? -1.0f
                                                                                            : 1.0f;
    }

    R_free(temp_allocator, tan1);
    R_free(temp_allocator, tan2);

    // Load a new tangent attributes buffer
    mesh->vbo_id[RF_LOC_VERTEX_TANGENT] =
            R_gfx_load_attrib_buffer(mesh->vao_id, RF_LOC_VERTEX_TANGENT, mesh->tangents,
                                     mesh->vertex_count * 4 * sizeof(float), false);

    R_log(R_log_type_info, "Tangents computed for mesh");
}

// Compute mesh binormals (aka bitangent)
R_public void R_mesh_compute_binormals(R_mesh *mesh) {
    for (R_int i = 0; i < mesh->vertex_count; i++) {
        R_vec3 normal = {mesh->normals[i * 3 + 0], mesh->normals[i * 3 + 1],
                         mesh->normals[i * 3 + 2]};
        R_vec3 tangent = {mesh->tangents[i * 4 + 0], mesh->tangents[i * 4 + 1],
                          mesh->tangents[i * 4 + 2]};
        float tangent_w = mesh->tangents[i * 4 + 3];

        // TODO: Register computed binormal in mesh->binormal?
        // R_vec3 binormal = R_vec3_mul(R_vec3_cross_product(normal, tangent), tangent_w);
    }
}

// Unload mesh from memory (RAM and/or VRAM)
R_public void R_unload_mesh(R_mesh mesh, R_allocator allocator) {
    R_gfx_unload_mesh(mesh);

    R_free(allocator, mesh.vertices);
    R_free(allocator, mesh.texcoords);
    R_free(allocator, mesh.normals);
    R_free(allocator, mesh.colors);
    R_free(allocator, mesh.tangents);
    R_free(allocator, mesh.texcoords2);
    R_free(allocator, mesh.indices);

    R_free(allocator, mesh.anim_vertices);
    R_free(allocator, mesh.anim_normals);
    R_free(allocator, mesh.bone_weights);
    R_free(allocator, mesh.bone_ids);
    R_free(allocator, mesh.vbo_id);
}

R_public R_model R_load_model(const char *filename, R_allocator allocator,
                              R_allocator temp_allocator, R_io_callbacks io) {
    R_model model = {0};

    if (R_is_file_extension(filename, ".obj")) {
        model = R_load_model_from_obj(filename, allocator, temp_allocator, io);
    }

    if (R_is_file_extension(filename, ".iqm")) {
        model = R_load_model_from_iqm(filename, allocator, temp_allocator, io);
    }

    if (R_is_file_extension(filename, ".gltf") || R_is_file_extension(filename, ".glb")) {
        model = R_load_model_from_gltf(filename, allocator, temp_allocator, io);
    }

    // Make sure model transform is set to identity matrix!
    model.transform = R_mat_identity();
    allocator = allocator;

    if (model.mesh_count == 0) {
        R_log(R_log_type_warning, "No meshes can be loaded, default to cube mesh. Filename: %s",
              filename);

        model.mesh_count = 1;
        model.meshes = (R_mesh *) R_alloc(allocator, model.mesh_count * sizeof(R_mesh));
        memset(model.meshes, 0, model.mesh_count * sizeof(R_mesh));
        model.meshes[0] = R_gen_mesh_cube(1.0f, 1.0f, 1.0f, allocator, temp_allocator);
    } else {
        // Upload vertex data to GPU (static mesh)
        for (R_int i = 0; i < model.mesh_count; i++) { R_gfx_load_mesh(&model.meshes[i], false); }
    }

    if (model.material_count == 0) {
        R_log(R_log_type_warning,
              "No materials can be loaded, default to white material. Filename: %s", filename);

        model.material_count = 1;
        model.materials =
                (R_material *) R_alloc(allocator, model.material_count * sizeof(R_material));
        memset(model.materials, 0, model.material_count * sizeof(R_material));
        model.materials[0] = R_load_default_material(allocator);

        if (model.mesh_material == NULL) {
            model.mesh_material = (int *) R_alloc(allocator, model.mesh_count * sizeof(int));
        }
    }

    return model;
}

// Load OBJ mesh data. Note: This calls into a library to do io, so we need to ask the user for IO callbacks
R_public R_model R_load_model_from_obj(const char *filename, R_allocator allocator,
                                       R_allocator temp_allocator, R_io_callbacks io) {
    R_model model = {0};
    allocator = allocator;

    tinyobj_attrib_t attrib = {0};
    tinyobj_shape_t *meshes = NULL;
    size_t mesh_count = 0;

    tinyobj_material_t *materials = NULL;
    size_t material_count = 0;

    R_set_global_dependencies_allocator(temp_allocator);// Set to NULL at the end of the function
    R_set_tinyobj_io_callbacks(io);
    {
        unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
        int ret =
                tinyobj_parse_obj(&attrib, &meshes, (size_t *) &mesh_count, &materials,
                                  &material_count, filename, R_tinyobj_file_reader_callback, flags);

        if (ret != TINYOBJ_SUCCESS) {
            R_log(R_log_type_warning, "Model data could not be loaded. Filename %s", filename);
        } else {
            R_log(R_log_type_info,
                  "Model data loaded successfully: %i meshes / %i materials, filename: %s",
                  mesh_count, material_count, filename);
        }

        // Init model meshes array
        {
            // TODO: Support multiple meshes... in the meantime, only one mesh is returned
            //model.mesh_count = mesh_count;
            model.mesh_count = 1;
            model.meshes = (R_mesh *) R_alloc(allocator, model.mesh_count * sizeof(R_mesh));
            memset(model.meshes, 0, model.mesh_count * sizeof(R_mesh));
        }

        // Init model materials array
        if (material_count > 0) {
            model.material_count = material_count;
            model.materials =
                    (R_material *) R_alloc(allocator, model.material_count * sizeof(R_material));
            memset(model.materials, 0, model.material_count * sizeof(R_material));
        }

        model.mesh_material = (int *) R_alloc(allocator, model.mesh_count * sizeof(int));
        memset(model.mesh_material, 0, model.mesh_count * sizeof(int));

        // Init model meshes
        for (R_int m = 0; m < 1; m++) {
            R_mesh mesh = (R_mesh){
                    .vertex_count = attrib.num_faces * 3,
                    .triangle_count = attrib.num_faces,

                    .vertices = (float *) R_alloc(allocator,
                                                  (attrib.num_faces * 3) * 3 * sizeof(float)),
                    .texcoords = (float *) R_alloc(allocator,
                                                   (attrib.num_faces * 3) * 2 * sizeof(float)),
                    .normals = (float *) R_alloc(allocator,
                                                 (attrib.num_faces * 3) * 3 * sizeof(float)),
                    .vbo_id = (unsigned int *) R_alloc(allocator,
                                                       RF_MAX_MESH_VBO * sizeof(unsigned int)),
            };

            memset(mesh.vertices, 0, mesh.vertex_count * 3 * sizeof(float));
            memset(mesh.texcoords, 0, mesh.vertex_count * 2 * sizeof(float));
            memset(mesh.normals, 0, mesh.vertex_count * 3 * sizeof(float));
            memset(mesh.vbo_id, 0, RF_MAX_MESH_VBO * sizeof(unsigned int));

            int vCount = 0;
            int vtCount = 0;
            int vnCount = 0;

            for (R_int f = 0; f < attrib.num_faces; f++) {
                // Get indices for the face
                tinyobj_vertex_index_t idx0 = attrib.faces[3 * f + 0];
                tinyobj_vertex_index_t idx1 = attrib.faces[3 * f + 1];
                tinyobj_vertex_index_t idx2 = attrib.faces[3 * f + 2];

                // RF_LOG(RF_LOG_TYPE_DEBUG, "Face %i index: v %i/%i/%i . vt %i/%i/%i . vn %i/%i/%i\n", f, idx0.v_idx, idx1.v_idx, idx2.v_idx, idx0.vt_idx, idx1.vt_idx, idx2.vt_idx, idx0.vn_idx, idx1.vn_idx, idx2.vn_idx);

                // Fill vertices buffer (float) using vertex index of the face
                for (R_int v = 0; v < 3; v++) {
                    mesh.vertices[vCount + v] = attrib.vertices[idx0.v_idx * 3 + v];
                }
                vCount += 3;

                for (R_int v = 0; v < 3; v++) {
                    mesh.vertices[vCount + v] = attrib.vertices[idx1.v_idx * 3 + v];
                }
                vCount += 3;

                for (R_int v = 0; v < 3; v++) {
                    mesh.vertices[vCount + v] = attrib.vertices[idx2.v_idx * 3 + v];
                }
                vCount += 3;

                // Fill texcoords buffer (float) using vertex index of the face
                // NOTE: Y-coordinate must be flipped upside-down
                mesh.texcoords[vtCount + 0] = attrib.texcoords[idx0.vt_idx * 2 + 0];
                mesh.texcoords[vtCount + 1] = 1.0f - attrib.texcoords[idx0.vt_idx * 2 + 1];
                vtCount += 2;
                mesh.texcoords[vtCount + 0] = attrib.texcoords[idx1.vt_idx * 2 + 0];
                mesh.texcoords[vtCount + 1] = 1.0f - attrib.texcoords[idx1.vt_idx * 2 + 1];
                vtCount += 2;
                mesh.texcoords[vtCount + 0] = attrib.texcoords[idx2.vt_idx * 2 + 0];
                mesh.texcoords[vtCount + 1] = 1.0f - attrib.texcoords[idx2.vt_idx * 2 + 1];
                vtCount += 2;

                // Fill normals buffer (float) using vertex index of the face
                for (R_int v = 0; v < 3; v++) {
                    mesh.normals[vnCount + v] = attrib.normals[idx0.vn_idx * 3 + v];
                }
                vnCount += 3;

                for (R_int v = 0; v < 3; v++) {
                    mesh.normals[vnCount + v] = attrib.normals[idx1.vn_idx * 3 + v];
                }
                vnCount += 3;

                for (R_int v = 0; v < 3; v++) {
                    mesh.normals[vnCount + v] = attrib.normals[idx2.vn_idx * 3 + v];
                }
                vnCount += 3;
            }

            model.meshes[m] = mesh;// Assign mesh data to model

            // Assign mesh material for current mesh
            model.mesh_material[m] = attrib.material_ids[m];

            // Set unfound materials to default
            if (model.mesh_material[m] == -1) { model.mesh_material[m] = 0; }
        }

        // Init model materials
        for (R_int m = 0; m < material_count; m++) {
            // Init material to default
            // NOTE: Uses default shader, only RF_MAP_DIFFUSE supported
            model.materials[m] = R_load_default_material(allocator);
            model.materials[m].maps[RF_MAP_DIFFUSE].texture =
                    R_get_default_texture();// Get default texture, in case no texture is defined

            if (materials[m].diffuse_texname != NULL) {
                model.materials[m].maps[RF_MAP_DIFFUSE].texture =
                        R_load_texture_from_file(materials[m].diffuse_texname, temp_allocator,
                                                 io);//char* diffuse_texname; // map_Kd
            }

            model.materials[m].maps[RF_MAP_DIFFUSE].color =
                    (R_color){(float) (materials[m].diffuse[0] * 255.0f),
                              (float) (materials[m].diffuse[1] * 255.0f),
                              (float) (materials[m].diffuse[2] * 255.0f), 255};

            model.materials[m].maps[RF_MAP_DIFFUSE].value = 0.0f;

            if (materials[m].specular_texname != NULL) {
                model.materials[m].maps[RF_MAP_SPECULAR].texture =
                        R_load_texture_from_file(materials[m].specular_texname, temp_allocator,
                                                 io);//char* specular_texname; // map_Ks
            }

            model.materials[m].maps[RF_MAP_SPECULAR].color =
                    (R_color){(float) (materials[m].specular[0] * 255.0f),
                              (float) (materials[m].specular[1] * 255.0f),
                              (float) (materials[m].specular[2] * 255.0f), 255};

            model.materials[m].maps[RF_MAP_SPECULAR].value = 0.0f;

            if (materials[m].bump_texname != NULL) {
                model.materials[m].maps[RF_MAP_NORMAL].texture =
                        R_load_texture_from_file(materials[m].bump_texname, temp_allocator,
                                                 io);//char* bump_texname; // map_bump, bump
            }

            model.materials[m].maps[RF_MAP_NORMAL].color = R_white;
            model.materials[m].maps[RF_MAP_NORMAL].value = materials[m].shininess;

            model.materials[m].maps[RF_MAP_EMISSION].color =
                    (R_color){(float) (materials[m].emission[0] * 255.0f),
                              (float) (materials[m].emission[1] * 255.0f),
                              (float) (materials[m].emission[2] * 255.0f), 255};

            if (materials[m].displacement_texname != NULL) {
                model.materials[m].maps[RF_MAP_HEIGHT].texture =
                        R_load_texture_from_file(materials[m].displacement_texname, temp_allocator,
                                                 io);//char* displacement_texname; // disp
            }
        }

        tinyobj_attrib_free(&attrib);
        tinyobj_shapes_free(meshes, mesh_count);
        tinyobj_materials_free(materials, material_count);
    }
    R_set_global_dependencies_allocator((R_allocator){0});
    R_set_tinyobj_io_callbacks((R_io_callbacks){0});

    // NOTE: At this point we have all model data loaded
    R_log(R_log_type_info, "Model loaded successfully in RAM. Filename: %s", filename);

    return R_load_meshes_and_materials_for_model(model, allocator, temp_allocator);
}

// Load IQM mesh data
R_public R_model R_load_model_from_iqm(const char *filename, R_allocator allocator,
                                       R_allocator temp_allocator, R_io_callbacks io) {
#pragma region constants
#define RF_IQM_MAGIC "INTERQUAKEMODEL"// IQM file magic number
#define RF_IQM_VERSION 2              // only IQM version 2 supported

#define RF_BONE_NAME_LENGTH 32// R_bone_info name string length
#define RF_MESH_NAME_LENGTH 32// R_mesh name string length
#pragma endregion

#pragma region IQM file structs
    typedef struct R_iqm_header R_iqm_header;
    struct R_iqm_header
    {
        char magic[16];
        unsigned int version;
        unsigned int filesize;
        unsigned int flags;
        unsigned int num_text, ofs_text;
        unsigned int num_meshes, ofs_meshes;
        unsigned int num_vertexarrays, num_vertexes, ofs_vertexarrays;
        unsigned int num_triangles, ofs_triangles, ofs_adjacency;
        unsigned int num_joints, ofs_joints;
        unsigned int num_poses, ofs_poses;
        unsigned int num_anims, ofs_anims;
        unsigned int num_frames, num_framechannels, ofs_frames, ofs_bounds;
        unsigned int num_comment, ofs_comment;
        unsigned int num_extensions, ofs_extensions;
    };

    typedef struct R_iqm_mesh R_iqm_mesh;
    struct R_iqm_mesh
    {
        unsigned int name;
        unsigned int material;
        unsigned int first_vertex, num_vertexes;
        unsigned int first_triangle, num_triangles;
    };

    typedef struct R_iqm_triangle R_iqm_triangle;
    struct R_iqm_triangle
    {
        unsigned int vertex[3];
    };

    typedef struct R_iqm_joint R_iqm_joint;
    struct R_iqm_joint
    {
        unsigned int name;
        int parent;
        float translate[3], rotate[4], scale[3];
    };

    typedef struct R_iqm_vertex_array R_iqm_vertex_array;
    struct R_iqm_vertex_array
    {
        unsigned int type;
        unsigned int flags;
        unsigned int format;
        unsigned int size;
        unsigned int offset;
    };

    // IQM vertex data types
    typedef enum R_iqm_vertex_type {
        RF_IQM_POSITION = 0,
        RF_IQM_TEXCOORD = 1,
        RF_IQM_NORMAL = 2,
        RF_IQM_TANGENT = 3,// Note: Tangents unused by default
        RF_IQM_BLENDINDEXES = 4,
        RF_IQM_BLENDWEIGHTS = 5,
        RF_IQM_COLOR = 6,   // Note: Vertex colors unused by default
        RF_IQM_CUSTOM = 0x10// Note: Custom vertex values unused by default
    } R_iqm_vertex_type;
#pragma endregion

    R_model model = {0};

    size_t data_size = R_file_size(io, filename);
    unsigned char *data = (unsigned char *) R_alloc(temp_allocator, data_size);

    if (R_read_file(io, filename, data, data_size)) { R_free(temp_allocator, data); }

    R_iqm_header iqm = *((R_iqm_header *) data);

    R_iqm_mesh *imesh;
    R_iqm_triangle *tri;
    R_iqm_vertex_array *va;
    R_iqm_joint *ijoint;

    float *vertex = NULL;
    float *normal = NULL;
    float *text = NULL;
    char *blendi = NULL;
    unsigned char *blendw = NULL;

    if (strncmp(iqm.magic, RF_IQM_MAGIC, sizeof(RF_IQM_MAGIC))) {
        R_log(R_log_type_warning, "[%s] IQM file does not seem to be valid", filename);
        return model;
    }

    if (iqm.version != RF_IQM_VERSION) {
        R_log(R_log_type_warning, "[%s] IQM file version is not supported (%i).", filename,
              iqm.version);
        return model;
    }

    // Meshes data processing
    imesh = (R_iqm_mesh *) R_alloc(temp_allocator, sizeof(R_iqm_mesh) * iqm.num_meshes);
    memcpy(imesh, data + iqm.ofs_meshes, sizeof(R_iqm_mesh) * iqm.num_meshes);

    model.mesh_count = iqm.num_meshes;
    model.meshes = (R_mesh *) R_alloc(allocator, model.mesh_count * sizeof(R_mesh));

    char name[RF_MESH_NAME_LENGTH] = {0};
    for (R_int i = 0; i < model.mesh_count; i++) {
        memcpy(name, data + (iqm.ofs_text + imesh[i].name), RF_MESH_NAME_LENGTH);

        model.meshes[i] = (R_mesh){.vertex_count = imesh[i].num_vertexes};

        model.meshes[i].vertices =
                (float *) R_alloc(allocator, model.meshes[i].vertex_count * 3 *
                                                     sizeof(float));// Default vertex positions
        memset(model.meshes[i].vertices, 0, model.meshes[i].vertex_count * 3 * sizeof(float));

        model.meshes[i].normals =
                (float *) R_alloc(allocator, model.meshes[i].vertex_count * 3 *
                                                     sizeof(float));// Default vertex normals
        memset(model.meshes[i].normals, 0, model.meshes[i].vertex_count * 3 * sizeof(float));

        model.meshes[i].texcoords =
                (float *) R_alloc(allocator, model.meshes[i].vertex_count * 2 *
                                                     sizeof(float));// Default vertex texcoords
        memset(model.meshes[i].texcoords, 0, model.meshes[i].vertex_count * 2 * sizeof(float));

        model.meshes[i].bone_ids =
                (int *) R_alloc(allocator, model.meshes[i].vertex_count * 4 *
                                                   sizeof(float));// Up-to 4 bones supported!
        memset(model.meshes[i].bone_ids, 0, model.meshes[i].vertex_count * 4 * sizeof(float));

        model.meshes[i].bone_weights =
                (float *) R_alloc(allocator, model.meshes[i].vertex_count * 4 *
                                                     sizeof(float));// Up-to 4 bones supported!
        memset(model.meshes[i].bone_weights, 0, model.meshes[i].vertex_count * 4 * sizeof(float));

        model.meshes[i].triangle_count = imesh[i].num_triangles;

        model.meshes[i].indices = (unsigned short *) R_alloc(
                allocator, model.meshes[i].triangle_count * 3 * sizeof(unsigned short));
        memset(model.meshes[i].indices, 0,
               model.meshes[i].triangle_count * 3 * sizeof(unsigned short));

        // Animated verted data, what we actually process for rendering
        // NOTE: Animated vertex should be re-uploaded to GPU (if not using GPU skinning)
        model.meshes[i].anim_vertices =
                (float *) R_alloc(allocator, model.meshes[i].vertex_count * 3 * sizeof(float));
        memset(model.meshes[i].anim_vertices, 0, model.meshes[i].vertex_count * 3 * sizeof(float));

        model.meshes[i].anim_normals =
                (float *) R_alloc(allocator, model.meshes[i].vertex_count * 3 * sizeof(float));
        memset(model.meshes[i].anim_normals, 0, model.meshes[i].vertex_count * 3 * sizeof(float));

        model.meshes[i].vbo_id =
                (unsigned int *) R_alloc(allocator, RF_MAX_MESH_VBO * sizeof(unsigned int));
        memset(model.meshes[i].vbo_id, 0, RF_MAX_MESH_VBO * sizeof(unsigned int));
    }

    // Triangles data processing
    tri = (R_iqm_triangle *) R_alloc(temp_allocator, iqm.num_triangles * sizeof(R_iqm_triangle));
    memcpy(tri, data + iqm.ofs_triangles, iqm.num_triangles * sizeof(R_iqm_triangle));

    for (R_int m = 0; m < model.mesh_count; m++) {
        int tcounter = 0;

        for (R_int i = imesh[m].first_triangle;
             i < (imesh[m].first_triangle + imesh[m].num_triangles); i++) {
            // IQM triangles are stored counter clockwise, but raylib sets opengl to clockwise drawing, so we swap them around
            model.meshes[m].indices[tcounter + 2] = tri[i].vertex[0] - imesh[m].first_vertex;
            model.meshes[m].indices[tcounter + 1] = tri[i].vertex[1] - imesh[m].first_vertex;
            model.meshes[m].indices[tcounter] = tri[i].vertex[2] - imesh[m].first_vertex;
            tcounter += 3;
        }
    }

    // Vertex arrays data processing
    va = (R_iqm_vertex_array *) R_alloc(temp_allocator,
                                        iqm.num_vertexarrays * sizeof(R_iqm_vertex_array));
    memcpy(va, data + iqm.ofs_vertexarrays, iqm.num_vertexarrays * sizeof(R_iqm_vertex_array));

    for (R_int i = 0; i < iqm.num_vertexarrays; i++) {
        switch (va[i].type) {
            case RF_IQM_POSITION: {
                vertex = (float *) R_alloc(temp_allocator, iqm.num_vertexes * 3 * sizeof(float));
                memcpy(vertex, data + va[i].offset, iqm.num_vertexes * 3 * sizeof(float));

                for (R_int m = 0; m < iqm.num_meshes; m++) {
                    int vertex_pos_counter = 0;
                    for (R_int ii = imesh[m].first_vertex * 3;
                         ii < (imesh[m].first_vertex + imesh[m].num_vertexes) * 3; ii++) {
                        model.meshes[m].vertices[vertex_pos_counter] = vertex[ii];
                        model.meshes[m].anim_vertices[vertex_pos_counter] = vertex[ii];
                        vertex_pos_counter++;
                    }
                }
            } break;

            case RF_IQM_NORMAL: {
                normal = (float *) R_alloc(temp_allocator, iqm.num_vertexes * 3 * sizeof(float));
                memcpy(normal, data + va[i].offset, iqm.num_vertexes * 3 * sizeof(float));

                for (R_int m = 0; m < iqm.num_meshes; m++) {
                    int vertex_pos_counter = 0;
                    for (R_int ii = imesh[m].first_vertex * 3;
                         ii < (imesh[m].first_vertex + imesh[m].num_vertexes) * 3; ii++) {
                        model.meshes[m].normals[vertex_pos_counter] = normal[ii];
                        model.meshes[m].anim_normals[vertex_pos_counter] = normal[ii];
                        vertex_pos_counter++;
                    }
                }
            } break;

            case RF_IQM_TEXCOORD: {
                text = (float *) R_alloc(temp_allocator, iqm.num_vertexes * 2 * sizeof(float));
                memcpy(text, data + va[i].offset, iqm.num_vertexes * 2 * sizeof(float));

                for (R_int m = 0; m < iqm.num_meshes; m++) {
                    int vertex_pos_counter = 0;
                    for (R_int ii = imesh[m].first_vertex * 2;
                         ii < (imesh[m].first_vertex + imesh[m].num_vertexes) * 2; ii++) {
                        model.meshes[m].texcoords[vertex_pos_counter] = text[ii];
                        vertex_pos_counter++;
                    }
                }
            } break;

            case RF_IQM_BLENDINDEXES: {
                blendi = (char *) R_alloc(temp_allocator, iqm.num_vertexes * 4 * sizeof(char));
                memcpy(blendi, data + va[i].offset, iqm.num_vertexes * 4 * sizeof(char));

                for (R_int m = 0; m < iqm.num_meshes; m++) {
                    int bone_counter = 0;
                    for (R_int ii = imesh[m].first_vertex * 4;
                         ii < (imesh[m].first_vertex + imesh[m].num_vertexes) * 4; ii++) {
                        model.meshes[m].bone_ids[bone_counter] = blendi[ii];
                        bone_counter++;
                    }
                }
            } break;

            case RF_IQM_BLENDWEIGHTS: {
                blendw = (unsigned char *) R_alloc(temp_allocator,
                                                   iqm.num_vertexes * 4 * sizeof(unsigned char));
                memcpy(blendw, data + va[i].offset, iqm.num_vertexes * 4 * sizeof(unsigned char));

                for (R_int m = 0; m < iqm.num_meshes; m++) {
                    int bone_counter = 0;
                    for (R_int ii = imesh[m].first_vertex * 4;
                         ii < (imesh[m].first_vertex + imesh[m].num_vertexes) * 4; ii++) {
                        model.meshes[m].bone_weights[bone_counter] = blendw[ii] / 255.0f;
                        bone_counter++;
                    }
                }
            } break;
        }
    }

    // Bones (joints) data processing
    ijoint = (R_iqm_joint *) R_alloc(temp_allocator, iqm.num_joints * sizeof(R_iqm_joint));
    memcpy(ijoint, data + iqm.ofs_joints, iqm.num_joints * sizeof(R_iqm_joint));

    model.bone_count = iqm.num_joints;
    model.bones = (R_bone_info *) R_alloc(allocator, iqm.num_joints * sizeof(R_bone_info));
    model.bind_pose = (R_transform *) R_alloc(allocator, iqm.num_joints * sizeof(R_transform));

    for (R_int i = 0; i < iqm.num_joints; i++) {
        // Bones
        model.bones[i].parent = ijoint[i].parent;
        memcpy(model.bones[i].name, data + iqm.ofs_text + ijoint[i].name,
               RF_BONE_NAME_LENGTH * sizeof(char));

        // Bind pose (base pose)
        model.bind_pose[i].translation.x = ijoint[i].translate[0];
        model.bind_pose[i].translation.y = ijoint[i].translate[1];
        model.bind_pose[i].translation.z = ijoint[i].translate[2];

        model.bind_pose[i].rotation.x = ijoint[i].rotate[0];
        model.bind_pose[i].rotation.y = ijoint[i].rotate[1];
        model.bind_pose[i].rotation.z = ijoint[i].rotate[2];
        model.bind_pose[i].rotation.w = ijoint[i].rotate[3];

        model.bind_pose[i].scale.x = ijoint[i].scale[0];
        model.bind_pose[i].scale.y = ijoint[i].scale[1];
        model.bind_pose[i].scale.z = ijoint[i].scale[2];
    }

    // Build bind pose from parent joints
    for (R_int i = 0; i < model.bone_count; i++) {
        if (model.bones[i].parent >= 0) {
            model.bind_pose[i].rotation = R_quaternion_mul(
                    model.bind_pose[model.bones[i].parent].rotation, model.bind_pose[i].rotation);
            model.bind_pose[i].translation =
                    R_vec3_rotate_by_quaternion(model.bind_pose[i].translation,
                                                model.bind_pose[model.bones[i].parent].rotation);
            model.bind_pose[i].translation =
                    R_vec3_add(model.bind_pose[i].translation,
                               model.bind_pose[model.bones[i].parent].translation);
            model.bind_pose[i].scale = R_vec3_mul_v(model.bind_pose[i].scale,
                                                    model.bind_pose[model.bones[i].parent].scale);
        }
    }

    R_free(temp_allocator, imesh);
    R_free(temp_allocator, tri);
    R_free(temp_allocator, va);
    R_free(temp_allocator, vertex);
    R_free(temp_allocator, normal);
    R_free(temp_allocator, text);
    R_free(temp_allocator, blendi);
    R_free(temp_allocator, blendw);
    R_free(temp_allocator, ijoint);

    return R_load_meshes_and_materials_for_model(model, allocator, temp_allocator);
}

/***********************************************************************************
    Function based on work by Wilhem Barbier (@wbrbr)

    Features:
      - Supports .gltf and .glb files
      - Supports embedded (base64) or external textures
      - Loads the albedo/diffuse texture (other maps could be added)
      - Supports multiple mesh per model and multiple primitives per model

    Some restrictions (not exhaustive):
      - Triangle-only meshes
      - Not supported node hierarchies or transforms
      - Only loads the diffuse texture... but not too hard to support other maps (normal, roughness/metalness...)
      - Only supports unsigned short indices (no unsigned char/unsigned int)
      - Only supports float for texture coordinates (no unsigned char/unsigned short)
*************************************************************************************/
// Load texture from cgltf_image
R_internal R_texture2d R_load_texture_from_cgltf_image(cgltf_image *image, const char *tex_path,
                                                       R_color tint, R_allocator temp_allocator,
                                                       R_io_callbacks io) {
    R_texture2d texture = {0};

    if (image->uri) {
        if ((strlen(image->uri) > 5) && (image->uri[0] == 'd') && (image->uri[1] == 'a') &&
            (image->uri[2] == 't') && (image->uri[3] == 'a') && (image->uri[4] == ':')) {
            // Data URI
            // Format: data:<mediatype>;base64,<data>

            // Find the comma
            int i = 0;
            while ((image->uri[i] != ',') && (image->uri[i] != 0)) i++;

            if (image->uri[i] == 0) {
                R_log(R_log_type_warning, "CGLTF R_image: Invalid data URI");
            } else {
                R_base64_output data =
                        R_decode_base64((const unsigned char *) image->uri + i + 1, temp_allocator);

                R_image rimage = R_load_image_from_file_data(data.buffer, data.size, 4,
                                                             temp_allocator, temp_allocator);

                // TODO: Tint shouldn't be applied here!
                R_image_color_tint(rimage, tint);

                texture = R_load_texture_from_image(rimage);

                R_unload_image(rimage, temp_allocator);
                R_free(temp_allocator, data.buffer);
            }
        } else {
            char buff[1024] = {0};
            snprintf(buff, 1024, "%s/%s", tex_path, image->uri);
            R_image rimage = R_load_image_from_file(buff, temp_allocator, temp_allocator, io);

            // TODO: Tint shouldn't be applied here!
            R_image_color_tint(rimage, tint);

            texture = R_load_texture_from_image(rimage);

            R_unload_image(rimage, temp_allocator);
        }
    } else if (image->buffer_view) {
        unsigned char *data = (unsigned char *) R_alloc(temp_allocator, image->buffer_view->size);
        int n = image->buffer_view->offset;
        int stride = image->buffer_view->stride ? image->buffer_view->stride : 1;

        for (R_int i = 0; i < image->buffer_view->size; i++) {
            data[i] = ((unsigned char *) image->buffer_view->buffer->data)[n];
            n += stride;
        }

        R_image rimage = R_load_image_from_file_data(data, image->buffer_view->size, 4,
                                                     temp_allocator, temp_allocator);

        // TODO: Tint shouldn't be applied here!
        R_image_color_tint(rimage, tint);

        texture = R_load_texture_from_image(rimage);

        R_unload_image(rimage, temp_allocator);
        R_free(temp_allocator, data);
    } else {
        texture = R_load_texture_from_image((R_image){.data = &tint,
                                                      .width = 1,
                                                      .height = 1,
                                                      .format = R_pixel_format_r8g8b8a8,
                                                      .valid = true});
    }

    return texture;
}

// Load model from files (meshes and materials)
R_public R_model R_load_model_from_gltf(const char *filename, R_allocator allocator,
                                        R_allocator temp_allocator, R_io_callbacks io) {
#define R_load_accessor(type, nbcomp, acc, dst)                                                    \
    {                                                                                              \
        int n = 0;                                                                                 \
        type *buf = (type *) acc->buffer_view->buffer->data +                                      \
                    acc->buffer_view->offset / sizeof(type) + acc->offset / sizeof(type);          \
        for (R_int k = 0; k < acc->count; k++) {                                                   \
            for (R_int l = 0; l < nbcomp; l++) { dst[nbcomp * k + l] = buf[n + l]; }               \
            n += acc->stride / sizeof(type);                                                       \
        }                                                                                          \
    }

    R_set_global_dependencies_allocator(temp_allocator);
    R_model model = {0};

    cgltf_options options = {
            cgltf_file_type_invalid,
            .file = {.read = &R_cgltf_io_read, .release = &R_cgltf_io_release, .user_data = &io}};

    int data_size = R_file_size(io, filename);
    void *data = R_alloc(temp_allocator, data_size);
    if (!R_read_file(io, filename, data, data_size)) {
        R_free(temp_allocator, data);
        R_set_global_dependencies_allocator(temp_allocator);
        return model;
    }

    cgltf_data *cgltf_data = NULL;
    cgltf_result result = cgltf_parse(&options, data, data_size, &cgltf_data);

    if (result == cgltf_result_success) {
        R_log(R_log_type_info, "[%s][%s] R_model meshes/materials: %i/%i", filename,
              (cgltf_data->file_type == 2) ? "glb" : "gltf", cgltf_data->meshes_count,
              cgltf_data->materials_count);

        // Read cgltf_data buffers
        result = cgltf_load_buffers(&options, cgltf_data, filename);
        if (result != cgltf_result_success) {
            R_log(R_log_type_info, "[%s][%s] Error loading mesh/material buffers", filename,
                  (cgltf_data->file_type == 2) ? "glb" : "gltf");
        }

        int primitivesCount = 0;

        for (R_int i = 0; i < cgltf_data->meshes_count; i++) {
            primitivesCount += (int) cgltf_data->meshes[i].primitives_count;
        }

        // Process glTF cgltf_data and map to model
        allocator = allocator;
        model.mesh_count = primitivesCount;
        model.material_count = cgltf_data->materials_count + 1;
        model.meshes = (R_mesh *) R_alloc(allocator, model.mesh_count * sizeof(R_mesh));
        model.materials =
                (R_material *) R_alloc(allocator, model.material_count * sizeof(R_material));
        model.mesh_material = (int *) R_alloc(allocator, model.mesh_count * sizeof(int));

        memset(model.meshes, 0, model.mesh_count * sizeof(R_mesh));

        for (R_int i = 0; i < model.mesh_count; i++) {
            model.meshes[i].vbo_id =
                    (unsigned int *) R_alloc(allocator, RF_MAX_MESH_VBO * sizeof(unsigned int));
            memset(model.meshes[i].vbo_id, 0, RF_MAX_MESH_VBO * sizeof(unsigned int));
        }

        //For each material
        for (R_int i = 0; i < model.material_count - 1; i++) {
            model.materials[i] = R_load_default_material(allocator);
            R_color tint = (R_color){255, 255, 255, 255};
            const char *tex_path = R_get_directory_path_from_file_path(filename);

            //Ensure material follows raylib support for PBR (metallic/roughness flow)
            if (cgltf_data->materials[i].has_pbr_metallic_roughness) {
                float roughness = cgltf_data->materials[i].pbr_metallic_roughness.roughness_factor;
                float metallic = cgltf_data->materials[i].pbr_metallic_roughness.metallic_factor;

                // NOTE: R_material name not used for the moment
                //if (model.materials[i].name && cgltf_data->materials[i].name) strcpy(model.materials[i].name, cgltf_data->materials[i].name);

                // TODO: REview: shouldn't these be *255 ???
                tint.r = (unsigned char) (cgltf_data->materials[i]
                                                  .pbr_metallic_roughness.base_color_factor[0] *
                                          255);
                tint.g = (unsigned char) (cgltf_data->materials[i]
                                                  .pbr_metallic_roughness.base_color_factor[1] *
                                          255);
                tint.b = (unsigned char) (cgltf_data->materials[i]
                                                  .pbr_metallic_roughness.base_color_factor[2] *
                                          255);
                tint.a = (unsigned char) (cgltf_data->materials[i]
                                                  .pbr_metallic_roughness.base_color_factor[3] *
                                          255);

                model.materials[i].maps[RF_MAP_ROUGHNESS].color = tint;

                if (cgltf_data->materials[i].pbr_metallic_roughness.base_color_texture.texture) {
                    model.materials[i].maps[RF_MAP_ALBEDO].texture =
                            R_load_texture_from_cgltf_image(
                                    cgltf_data->materials[i]
                                            .pbr_metallic_roughness.base_color_texture.texture
                                            ->image,
                                    tex_path, tint, temp_allocator, io);
                }

                // NOTE: Tint isn't need for other textures.. pass null or clear?
                // Just set as white, multiplying by white has no effect
                tint = R_white;

                if (cgltf_data->materials[i]
                            .pbr_metallic_roughness.metallic_roughness_texture.texture) {
                    model.materials[i].maps[RF_MAP_ROUGHNESS].texture =
                            R_load_texture_from_cgltf_image(
                                    cgltf_data->materials[i]
                                            .pbr_metallic_roughness.metallic_roughness_texture
                                            .texture->image,
                                    tex_path, tint, temp_allocator, io);
                }
                model.materials[i].maps[RF_MAP_ROUGHNESS].value = roughness;
                model.materials[i].maps[RF_MAP_METALNESS].value = metallic;

                if (cgltf_data->materials[i].normal_texture.texture) {
                    model.materials[i].maps[RF_MAP_NORMAL].texture =
                            R_load_texture_from_cgltf_image(
                                    cgltf_data->materials[i].normal_texture.texture->image,
                                    tex_path, tint, temp_allocator, io);
                }

                if (cgltf_data->materials[i].occlusion_texture.texture) {
                    model.materials[i].maps[RF_MAP_OCCLUSION].texture =
                            R_load_texture_from_cgltf_image(
                                    cgltf_data->materials[i].occlusion_texture.texture->image,
                                    tex_path, tint, temp_allocator, io);
                }
            }
        }

        model.materials[model.material_count - 1] = R_load_default_material(allocator);

        int primitiveIndex = 0;

        for (R_int i = 0; i < cgltf_data->meshes_count; i++) {
            for (R_int p = 0; p < cgltf_data->meshes[i].primitives_count; p++) {
                for (R_int j = 0; j < cgltf_data->meshes[i].primitives[p].attributes_count; j++) {
                    if (cgltf_data->meshes[i].primitives[p].attributes[j].type ==
                        cgltf_attribute_type_position) {
                        cgltf_accessor *acc =
                                cgltf_data->meshes[i].primitives[p].attributes[j].data;
                        model.meshes[primitiveIndex].vertex_count = acc->count;
                        model.meshes[primitiveIndex].vertices = (float *) R_alloc(
                                allocator,
                                sizeof(float) * model.meshes[primitiveIndex].vertex_count * 3);

                        R_load_accessor(float, 3, acc, model.meshes[primitiveIndex].vertices)
                    } else if (cgltf_data->meshes[i].primitives[p].attributes[j].type ==
                               cgltf_attribute_type_normal) {
                        cgltf_accessor *acc =
                                cgltf_data->meshes[i].primitives[p].attributes[j].data;
                        model.meshes[primitiveIndex].normals =
                                (float *) R_alloc(allocator, sizeof(float) * acc->count * 3);

                        R_load_accessor(float, 3, acc, model.meshes[primitiveIndex].normals)
                    } else if (cgltf_data->meshes[i].primitives[p].attributes[j].type ==
                               cgltf_attribute_type_texcoord) {
                        cgltf_accessor *acc =
                                cgltf_data->meshes[i].primitives[p].attributes[j].data;

                        if (acc->component_type == cgltf_component_type_r_32f) {
                            model.meshes[primitiveIndex].texcoords =
                                    (float *) R_alloc(allocator, sizeof(float) * acc->count * 2);
                            R_load_accessor(float, 2, acc, model.meshes[primitiveIndex].texcoords)
                        } else {
                            // TODO: Support normalized unsigned unsigned char/unsigned short texture coordinates
                            R_log(R_log_type_warning, "[%s] R_texture coordinates must be float",
                                  filename);
                        }
                    }
                }

                cgltf_accessor *acc = cgltf_data->meshes[i].primitives[p].indices;

                if (acc) {
                    if (acc->component_type == cgltf_component_type_r_16u) {
                        model.meshes[primitiveIndex].triangle_count = acc->count / 3;
                        model.meshes[primitiveIndex].indices = (unsigned short *) R_alloc(
                                allocator, sizeof(unsigned short) *
                                                   model.meshes[primitiveIndex].triangle_count * 3);
                        R_load_accessor(unsigned short, 1, acc,
                                        model.meshes[primitiveIndex].indices)
                    } else {
                        // TODO: Support unsigned unsigned char/unsigned int
                        R_log(R_log_type_warning, "[%s] Indices must be unsigned short", filename);
                    }
                } else {
                    // Unindexed mesh
                    model.meshes[primitiveIndex].triangle_count =
                            model.meshes[primitiveIndex].vertex_count / 3;
                }

                if (cgltf_data->meshes[i].primitives[p].material) {
                    // Compute the offset
                    model.mesh_material[primitiveIndex] =
                            cgltf_data->meshes[i].primitives[p].material - cgltf_data->materials;
                } else {
                    model.mesh_material[primitiveIndex] = model.material_count - 1;
                }

                primitiveIndex++;
            }
        }

        cgltf_free(cgltf_data);
    } else {
        R_log(R_log_type_warning, "[%s] glTF cgltf_data could not be loaded", filename);
    }

    R_set_global_dependencies_allocator((R_allocator){0});

    return model;

#undef R_load_accessor
}

// Load model from generated mesh. Note: The function takes ownership of the mesh in model.meshes[0]
R_public R_model R_load_model_from_mesh(R_mesh mesh, R_allocator allocator) {
    R_model model = {0};

    model.transform = R_mat_identity();

    model.mesh_count = 1;
    model.meshes = (R_mesh *) R_alloc(allocator, model.mesh_count * sizeof(R_mesh));
    memset(model.meshes, 0, model.mesh_count * sizeof(R_mesh));
    model.meshes[0] = mesh;

    model.material_count = 1;
    model.materials = (R_material *) R_alloc(allocator, model.material_count * sizeof(R_material));
    memset(model.materials, 0, model.material_count * sizeof(R_material));
    model.materials[0] = R_load_default_material(allocator);

    model.mesh_material = (int *) R_alloc(allocator, model.mesh_count * sizeof(int));
    memset(model.mesh_material, 0, model.mesh_count * sizeof(int));
    model.mesh_material[0] = 0;// First material index

    return model;
}

// Get collision info between ray and model
R_public R_ray_hit_info R_collision_ray_model(R_ray ray, R_model model) {
    R_ray_hit_info result = {0};

    for (R_int m = 0; m < model.mesh_count; m++) {
        // Check if mesh has vertex data on CPU for testing
        if (model.meshes[m].vertices != NULL) {
            // model->mesh.triangle_count may not be set, vertex_count is more reliable
            int triangle_count = model.meshes[m].vertex_count / 3;

            // Test against all triangles in mesh
            for (R_int i = 0; i < triangle_count; i++) {
                R_vec3 a, b, c;
                R_vec3 *vertdata = (R_vec3 *) model.meshes[m].vertices;

                if (model.meshes[m].indices) {
                    a = vertdata[model.meshes[m].indices[i * 3 + 0]];
                    b = vertdata[model.meshes[m].indices[i * 3 + 1]];
                    c = vertdata[model.meshes[m].indices[i * 3 + 2]];
                } else {
                    a = vertdata[i * 3 + 0];
                    b = vertdata[i * 3 + 1];
                    c = vertdata[i * 3 + 2];
                }

                a = R_vec3_transform(a, model.transform);
                b = R_vec3_transform(b, model.transform);
                c = R_vec3_transform(c, model.transform);

                R_ray_hit_info tri_hit_info = R_collision_ray_triangle(ray, a, b, c);

                if (tri_hit_info.hit) {
                    // Save the closest hit triangle
                    if ((!result.hit) || (result.distance > tri_hit_info.distance))
                        result = tri_hit_info;
                }
            }
        }
    }

    return result;
}

// Unload model from memory (RAM and/or VRAM)
R_public void R_unload_model(R_model model, R_allocator allocator) {
    for (R_int i = 0; i < model.mesh_count; i++) R_unload_mesh(model.meshes[i], allocator);

    // As the user could be sharing shaders and textures between models,
    // we don't unload the material but just free it's maps, the user
    // is responsible for freeing models shaders and textures
    for (R_int i = 0; i < model.material_count; i++) R_free(allocator, model.materials[i].maps);

    R_free(allocator, model.meshes);
    R_free(allocator, model.materials);
    R_free(allocator, model.mesh_material);

    // Unload animation data
    R_free(allocator, model.bones);
    R_free(allocator, model.bind_pose);

    R_log(R_log_type_info, "Unloaded model data from RAM and VRAM");
}

#pragma region materials

// Load default material (Supports: DIFFUSE, SPECULAR, NORMAL maps)
R_public R_material R_load_default_material(R_allocator allocator) {
    R_material material = {0};
    material.maps =
            (R_material_map *) R_alloc(allocator, RF_MAX_MATERIAL_MAPS * sizeof(R_material_map));
    memset(material.maps, 0, RF_MAX_MATERIAL_MAPS * sizeof(R_material_map));

    material.shader = R_get_default_shader();
    material.maps[RF_MAP_DIFFUSE].texture = R_get_default_texture();// White texture (1x1 pixel)
    //material.maps[RF_MAP_NORMAL].texture;         // NOTE: By default, not set
    //material.maps[RF_MAP_SPECULAR].texture;       // NOTE: By default, not set

    material.maps[RF_MAP_DIFFUSE].color = R_white; // Diffuse color
    material.maps[RF_MAP_SPECULAR].color = R_white;// Specular color

    return material;
}

// TODO: Support IQM and GLTF for materials parsing
// TODO: Process materials to return
// Load materials from model file
R_public R_materials_array R_load_materials_from_mtl(const char *filename, R_allocator allocator,
                                                     R_io_callbacks io) {
    if (!filename) return (R_materials_array){0};

    R_materials_array result = {0};

    R_set_global_dependencies_allocator(allocator);
    R_set_tinyobj_io_callbacks(io);
    {
        size_t size = 0;
        tinyobj_material_t *mats = 0;
        if (tinyobj_parse_mtl_file(&mats, (size_t *) &size, filename,
                                   R_tinyobj_file_reader_callback) != TINYOBJ_SUCCESS) {
            // Log Error
        }
        tinyobj_materials_free(mats, result.size);

        result.size = size;
    }
    R_set_tinyobj_io_callbacks((R_io_callbacks){0});
    R_set_global_dependencies_allocator((R_allocator){0});

    // Set materials shader to default (DIFFUSE, SPECULAR, NORMAL)
    for (R_int i = 0; i < result.size; i++) { result.materials[i].shader = R_get_default_shader(); }

    return result;
}

R_public void R_unload_material(R_material material, R_allocator allocator) {
    // Unload material shader (avoid unloading default shader, managed by raylib)
    if (material.shader.id != R_get_default_shader().id) { R_gfx_unload_shader(material.shader); }

    // Unload loaded texture maps (avoid unloading default texture, managed by raylib)
    for (R_int i = 0; i < RF_MAX_MATERIAL_MAPS; i++) {
        if (material.maps[i].texture.id != R_get_default_texture().id) {
            R_gfx_delete_textures(material.maps[i].texture.id);
        }
    }

    R_free(allocator, material.maps);
}

R_public void R_set_material_texture(
        R_material *material, R_material_map_type map_type,
        R_texture2d
                texture);// Set texture for a material map type (R_map_diffuse, R_map_specular...)

R_public void R_set_model_mesh_material(R_model *model, int mesh_id,
                                        int material_id);// Set material for a mesh

#pragma endregion

#pragma region model animations
R_public R_model_animation_array R_load_model_animations_from_iqm_file(const char *filename,
                                                                       R_allocator allocator,
                                                                       R_allocator temp_allocator,
                                                                       R_io_callbacks io) {
    int size = R_file_size(io, filename);
    void *data = R_alloc(temp_allocator, size);

    R_model_animation_array result =
            R_load_model_animations_from_iqm(data, size, allocator, temp_allocator);

    R_free(temp_allocator, data);

    return result;
}

R_public R_model_animation_array R_load_model_animations_from_iqm(const unsigned char *data,
                                                                  int data_size,
                                                                  R_allocator allocator,
                                                                  R_allocator temp_allocator) {
    if (!data || !data_size) return (R_model_animation_array){0};

#define RF_IQM_MAGIC "INTERQUAKEMODEL"// IQM file magic number
#define RF_IQM_VERSION 2              // only IQM version 2 supported

    typedef struct R_iqm_header R_iqm_header;
    struct R_iqm_header
    {
        char magic[16];
        unsigned int version;
        unsigned int filesize;
        unsigned int flags;
        unsigned int num_text, ofs_text;
        unsigned int num_meshes, ofs_meshes;
        unsigned int num_vertexarrays, num_vertexes, ofs_vertexarrays;
        unsigned int num_triangles, ofs_triangles, ofs_adjacency;
        unsigned int num_joints, ofs_joints;
        unsigned int num_poses, ofs_poses;
        unsigned int num_anims, ofs_anims;
        unsigned int num_frames, num_framechannels, ofs_frames, ofs_bounds;
        unsigned int num_comment, ofs_comment;
        unsigned int num_extensions, ofs_extensions;
    };

    typedef struct R_iqm_pose R_iqm_pose;
    struct R_iqm_pose
    {
        int parent;
        unsigned int mask;
        float channeloffset[10];
        float channelscale[10];
    };

    typedef struct R_iqm_anim R_iqm_anim;
    struct R_iqm_anim
    {
        unsigned int name;
        unsigned int first_frame, num_frames;
        float framerate;
        unsigned int flags;
    };

    R_iqm_header iqm;

    // Read IQM header
    memcpy(&iqm, data, sizeof(R_iqm_header));

    if (strncmp(iqm.magic, RF_IQM_MAGIC, sizeof(RF_IQM_MAGIC))) {
        char temp_str[sizeof(RF_IQM_MAGIC) + 1] = {0};
        memcpy(temp_str, iqm.magic, sizeof(RF_IQM_MAGIC));
        R_log_error(R_bad_format, "Magic Number \"%s\"does not match.", temp_str);

        return (R_model_animation_array){0};
    }

    if (iqm.version != RF_IQM_VERSION) {
        R_log_error(R_bad_format, "IQM version %i is incorrect.", iqm.version);

        return (R_model_animation_array){0};
    }

    R_model_animation_array result = {
            .size = iqm.num_anims,
    };

    // Get bones data
    R_iqm_pose *poses = (R_iqm_pose *) R_alloc(temp_allocator, iqm.num_poses * sizeof(R_iqm_pose));
    memcpy(poses, data + iqm.ofs_poses, iqm.num_poses * sizeof(R_iqm_pose));

    // Get animations data
    R_iqm_anim *anim = (R_iqm_anim *) R_alloc(temp_allocator, iqm.num_anims * sizeof(R_iqm_anim));
    memcpy(anim, data + iqm.ofs_anims, iqm.num_anims * sizeof(R_iqm_anim));

    R_model_animation *animations =
            (R_model_animation *) R_alloc(allocator, iqm.num_anims * sizeof(R_model_animation));

    result.anims = animations;
    result.size = iqm.num_anims;

    // frameposes
    unsigned short *framedata = (unsigned short *) R_alloc(
            temp_allocator, iqm.num_frames * iqm.num_framechannels * sizeof(unsigned short));
    memcpy(framedata, data + iqm.ofs_frames,
           iqm.num_frames * iqm.num_framechannels * sizeof(unsigned short));

    for (R_int a = 0; a < iqm.num_anims; a++) {
        animations[a].frame_count = anim[a].num_frames;
        animations[a].bone_count = iqm.num_poses;
        animations[a].bones =
                (R_bone_info *) R_alloc(allocator, iqm.num_poses * sizeof(R_bone_info));
        animations[a].frame_poses =
                (R_transform **) R_alloc(allocator, anim[a].num_frames * sizeof(R_transform *));
        //animations[a].framerate = anim.framerate;     // TODO: Use framerate?

        for (R_int j = 0; j < iqm.num_poses; j++) {
            strcpy(animations[a].bones[j].name, "ANIMJOINTNAME");
            animations[a].bones[j].parent = poses[j].parent;
        }

        for (R_int j = 0; j < anim[a].num_frames; j++) {
            animations[a].frame_poses[j] =
                    (R_transform *) R_alloc(allocator, iqm.num_poses * sizeof(R_transform));
        }

        int dcounter = anim[a].first_frame * iqm.num_framechannels;

        for (R_int frame = 0; frame < anim[a].num_frames; frame++) {
            for (R_int i = 0; i < iqm.num_poses; i++) {
                animations[a].frame_poses[frame][i].translation.x = poses[i].channeloffset[0];

                if (poses[i].mask & 0x01) {
                    animations[a].frame_poses[frame][i].translation.x +=
                            framedata[dcounter] * poses[i].channelscale[0];
                    dcounter++;
                }

                animations[a].frame_poses[frame][i].translation.y = poses[i].channeloffset[1];

                if (poses[i].mask & 0x02) {
                    animations[a].frame_poses[frame][i].translation.y +=
                            framedata[dcounter] * poses[i].channelscale[1];
                    dcounter++;
                }

                animations[a].frame_poses[frame][i].translation.z = poses[i].channeloffset[2];

                if (poses[i].mask & 0x04) {
                    animations[a].frame_poses[frame][i].translation.z +=
                            framedata[dcounter] * poses[i].channelscale[2];
                    dcounter++;
                }

                animations[a].frame_poses[frame][i].rotation.x = poses[i].channeloffset[3];

                if (poses[i].mask & 0x08) {
                    animations[a].frame_poses[frame][i].rotation.x +=
                            framedata[dcounter] * poses[i].channelscale[3];
                    dcounter++;
                }

                animations[a].frame_poses[frame][i].rotation.y = poses[i].channeloffset[4];

                if (poses[i].mask & 0x10) {
                    animations[a].frame_poses[frame][i].rotation.y +=
                            framedata[dcounter] * poses[i].channelscale[4];
                    dcounter++;
                }

                animations[a].frame_poses[frame][i].rotation.z = poses[i].channeloffset[5];

                if (poses[i].mask & 0x20) {
                    animations[a].frame_poses[frame][i].rotation.z +=
                            framedata[dcounter] * poses[i].channelscale[5];
                    dcounter++;
                }

                animations[a].frame_poses[frame][i].rotation.w = poses[i].channeloffset[6];

                if (poses[i].mask & 0x40) {
                    animations[a].frame_poses[frame][i].rotation.w +=
                            framedata[dcounter] * poses[i].channelscale[6];
                    dcounter++;
                }

                animations[a].frame_poses[frame][i].scale.x = poses[i].channeloffset[7];

                if (poses[i].mask & 0x80) {
                    animations[a].frame_poses[frame][i].scale.x +=
                            framedata[dcounter] * poses[i].channelscale[7];
                    dcounter++;
                }

                animations[a].frame_poses[frame][i].scale.y = poses[i].channeloffset[8];

                if (poses[i].mask & 0x100) {
                    animations[a].frame_poses[frame][i].scale.y +=
                            framedata[dcounter] * poses[i].channelscale[8];
                    dcounter++;
                }

                animations[a].frame_poses[frame][i].scale.z = poses[i].channeloffset[9];

                if (poses[i].mask & 0x200) {
                    animations[a].frame_poses[frame][i].scale.z +=
                            framedata[dcounter] * poses[i].channelscale[9];
                    dcounter++;
                }

                animations[a].frame_poses[frame][i].rotation =
                        R_quaternion_normalize(animations[a].frame_poses[frame][i].rotation);
            }
        }

        // Build frameposes
        for (R_int frame = 0; frame < anim[a].num_frames; frame++) {
            for (R_int i = 0; i < animations[a].bone_count; i++) {
                if (animations[a].bones[i].parent >= 0) {
                    animations[a].frame_poses[frame][i].rotation = R_quaternion_mul(
                            animations[a]
                                    .frame_poses[frame][animations[a].bones[i].parent]
                                    .rotation,
                            animations[a].frame_poses[frame][i].rotation);
                    animations[a].frame_poses[frame][i].translation = R_vec3_rotate_by_quaternion(
                            animations[a].frame_poses[frame][i].translation,
                            animations[a]
                                    .frame_poses[frame][animations[a].bones[i].parent]
                                    .rotation);
                    animations[a].frame_poses[frame][i].translation =
                            R_vec3_add(animations[a].frame_poses[frame][i].translation,
                                       animations[a]
                                               .frame_poses[frame][animations[a].bones[i].parent]
                                               .translation);
                    animations[a].frame_poses[frame][i].scale = R_vec3_mul_v(
                            animations[a].frame_poses[frame][i].scale,
                            animations[a].frame_poses[frame][animations[a].bones[i].parent].scale);
                }
            }
        }
    }

    R_free(temp_allocator, framedata);
    R_free(temp_allocator, poses);
    R_free(temp_allocator, anim);

    return result;
}

// Update model animated vertex data (positions and normals) for a given frame
R_public void R_update_model_animation(R_model model, R_model_animation anim, int frame) {
    if ((anim.frame_count > 0) && (anim.bones != NULL) && (anim.frame_poses != NULL)) { return; }

    if (frame >= anim.frame_count) { frame = frame % anim.frame_count; }

    for (R_int m = 0; m < model.mesh_count; m++) {
        R_vec3 anim_vertex = {0};
        R_vec3 anim_normal = {0};

        R_vec3 in_translation = {0};
        R_quaternion in_rotation = {0};
        R_vec3 in_scale = {0};

        R_vec3 out_translation = {0};
        R_quaternion out_rotation = {0};
        R_vec3 out_scale = {0};

        int vertex_pos_counter = 0;
        int bone_counter = 0;
        int bone_id = 0;

        for (R_int i = 0; i < model.meshes[m].vertex_count; i++) {
            bone_id = model.meshes[m].bone_ids[bone_counter];
            in_translation = model.bind_pose[bone_id].translation;
            in_rotation = model.bind_pose[bone_id].rotation;
            in_scale = model.bind_pose[bone_id].scale;
            out_translation = anim.frame_poses[frame][bone_id].translation;
            out_rotation = anim.frame_poses[frame][bone_id].rotation;
            out_scale = anim.frame_poses[frame][bone_id].scale;

            // Vertices processing
            // NOTE: We use meshes.vertices (default vertex position) to calculate meshes.anim_vertices (animated vertex position)
            anim_vertex = (R_vec3){model.meshes[m].vertices[vertex_pos_counter],
                                   model.meshes[m].vertices[vertex_pos_counter + 1],
                                   model.meshes[m].vertices[vertex_pos_counter + 2]};
            anim_vertex = R_vec3_mul_v(anim_vertex, out_scale);
            anim_vertex = R_vec3_sub(anim_vertex, in_translation);
            anim_vertex = R_vec3_rotate_by_quaternion(
                    anim_vertex, R_quaternion_mul(out_rotation, R_quaternion_invert(in_rotation)));
            anim_vertex = R_vec3_add(anim_vertex, out_translation);
            model.meshes[m].anim_vertices[vertex_pos_counter] = anim_vertex.x;
            model.meshes[m].anim_vertices[vertex_pos_counter + 1] = anim_vertex.y;
            model.meshes[m].anim_vertices[vertex_pos_counter + 2] = anim_vertex.z;

            // Normals processing
            // NOTE: We use meshes.baseNormals (default normal) to calculate meshes.normals (animated normals)
            anim_normal = (R_vec3){model.meshes[m].normals[vertex_pos_counter],
                                   model.meshes[m].normals[vertex_pos_counter + 1],
                                   model.meshes[m].normals[vertex_pos_counter + 2]};
            anim_normal = R_vec3_rotate_by_quaternion(
                    anim_normal, R_quaternion_mul(out_rotation, R_quaternion_invert(in_rotation)));
            model.meshes[m].anim_normals[vertex_pos_counter] = anim_normal.x;
            model.meshes[m].anim_normals[vertex_pos_counter + 1] = anim_normal.y;
            model.meshes[m].anim_normals[vertex_pos_counter + 2] = anim_normal.z;
            vertex_pos_counter += 3;

            bone_counter += 4;
        }

        // Upload new vertex data to GPU for model drawing
        R_gfx_update_buffer(model.meshes[m].vbo_id[0], model.meshes[m].anim_vertices,
                            model.meshes[m].vertex_count * 3 *
                                    sizeof(float));// Update vertex position
        R_gfx_update_buffer(model.meshes[m].vbo_id[2], model.meshes[m].anim_vertices,
                            model.meshes[m].vertex_count * 3 *
                                    sizeof(float));// Update vertex normals
    }
}

// Check model animation skeleton match. Only number of bones and parent connections are checked
R_public R_bool R_is_model_animation_valid(R_model model, R_model_animation anim) {
    int result = 1;

    if (model.bone_count != anim.bone_count) result = 0;
    else {
        for (R_int i = 0; i < model.bone_count; i++) {
            if (model.bones[i].parent != anim.bones[i].parent) {
                result = 0;
                break;
            }
        }
    }

    return result;
}

// Unload animation data
R_public void R_unload_model_animation(R_model_animation anim, R_allocator allocator) {
    for (R_int i = 0; i < anim.frame_count; i++) R_free(allocator, anim.frame_poses[i]);

    R_free(allocator, anim.bones);
    R_free(allocator, anim.frame_poses);
}
#pragma endregion

#pragma region mesh generation

R_public R_mesh R_gen_mesh_cube(float width, float height, float length, R_allocator allocator,
                                R_allocator temp_allocator) {
    R_mesh mesh = {0};
    mesh.vbo_id = (unsigned int *) R_alloc(allocator, RF_MAX_MESH_VBO * sizeof(unsigned int));
    memset(mesh.vbo_id, 0, RF_MAX_MESH_VBO * sizeof(unsigned int));

#define R_custom_mesh_gen_cube//Todo: Investigate this macro
    /*
    Platonic solids:
    par_shapes_mesh* par_shapes_create_tetrahedron();       // 4 sides polyhedron (pyramid)
    par_shapes_mesh* par_shapes_create_cube();              // 6 sides polyhedron (cube)
    par_shapes_mesh* par_shapes_create_octahedron();        // 8 sides polyhedron (dyamond)
    par_shapes_mesh* par_shapes_create_dodecahedron();      // 12 sides polyhedron
    par_shapes_mesh* par_shapes_create_icosahedron();       // 20 sides polyhedron
    */

    // Platonic solid generation: cube (6 sides)
    // NOTE: No normals/texcoords generated by default
    //RF_SET_PARSHAPES_ALLOCATOR(temp_allocator);
    {
        par_shapes_mesh *cube = par_shapes_create_cube();
        cube->tcoords = PAR_MALLOC(float, 2 * cube->npoints);

        for (R_int i = 0; i < 2 * cube->npoints; i++) { cube->tcoords[i] = 0.0f; }

        par_shapes_scale(cube, width, height, length);
        par_shapes_translate(cube, -width / 2, 0.0f, -length / 2);
        par_shapes_compute_normals(cube);

        mesh.vertices = (float *) R_alloc(allocator, cube->ntriangles * 3 * 3 * sizeof(float));
        mesh.texcoords = (float *) R_alloc(allocator, cube->ntriangles * 3 * 2 * sizeof(float));
        mesh.normals = (float *) R_alloc(allocator, cube->ntriangles * 3 * 3 * sizeof(float));

        mesh.vertex_count = cube->ntriangles * 3;
        mesh.triangle_count = cube->ntriangles;

        for (R_int k = 0; k < mesh.vertex_count; k++) {
            mesh.vertices[k * 3] = cube->points[cube->triangles[k] * 3];
            mesh.vertices[k * 3 + 1] = cube->points[cube->triangles[k] * 3 + 1];
            mesh.vertices[k * 3 + 2] = cube->points[cube->triangles[k] * 3 + 2];

            mesh.normals[k * 3] = cube->normals[cube->triangles[k] * 3];
            mesh.normals[k * 3 + 1] = cube->normals[cube->triangles[k] * 3 + 1];
            mesh.normals[k * 3 + 2] = cube->normals[cube->triangles[k] * 3 + 2];

            mesh.texcoords[k * 2] = cube->tcoords[cube->triangles[k] * 2];
            mesh.texcoords[k * 2 + 1] = cube->tcoords[cube->triangles[k] * 2 + 1];
        }

        par_shapes_free_mesh(cube);
    }
    //RF_SET_PARSHAPES_ALLOCATOR((R_allocator) {0});

    // Upload vertex data to GPU (static mesh)
    R_gfx_load_mesh(&mesh, false);

    return mesh;
}

// Generate polygonal mesh
R_public R_mesh R_gen_mesh_poly(int sides, float radius, R_allocator allocator,
                                R_allocator temp_allocator) {
    R_mesh mesh = {0};
    mesh.vbo_id = (unsigned int *) R_alloc(allocator, RF_MAX_MESH_VBO * sizeof(unsigned int));
    memset(mesh.vbo_id, 0, RF_MAX_MESH_VBO * sizeof(unsigned int));
    int vertex_count = sides * 3;

    // Vertices definition
    R_vec3 *vertices = (R_vec3 *) R_alloc(temp_allocator, vertex_count * sizeof(R_vec3));
    for (R_int i = 0, v = 0; i < 360; i += 360 / sides, v += 3) {
        vertices[v] = (R_vec3){0.0f, 0.0f, 0.0f};
        vertices[v + 1] =
                (R_vec3){sinf(R_deg2rad * i) * radius, 0.0f, cosf(R_deg2rad * i) * radius};
        vertices[v + 2] = (R_vec3){sinf(R_deg2rad * (i + 360 / sides)) * radius, 0.0f,
                                   cosf(R_deg2rad * (i + 360 / sides)) * radius};
    }

    // Normals definition
    R_vec3 *normals = (R_vec3 *) R_alloc(temp_allocator, vertex_count * sizeof(R_vec3));
    for (R_int n = 0; n < vertex_count; n++) normals[n] = (R_vec3){0.0f, 1.0f, 0.0f};// R_vec3.up;

    // TexCoords definition
    R_vec2 *texcoords = (R_vec2 *) R_alloc(temp_allocator, vertex_count * sizeof(R_vec2));
    for (R_int n = 0; n < vertex_count; n++) texcoords[n] = (R_vec2){0.0f, 0.0f};

    mesh.vertex_count = vertex_count;
    mesh.triangle_count = sides;
    mesh.vertices = (float *) R_alloc(allocator, mesh.vertex_count * 3 * sizeof(float));
    mesh.texcoords = (float *) R_alloc(allocator, mesh.vertex_count * 2 * sizeof(float));
    mesh.normals = (float *) R_alloc(allocator, mesh.vertex_count * 3 * sizeof(float));

    // R_mesh vertices position array
    for (R_int i = 0; i < mesh.vertex_count; i++) {
        mesh.vertices[3 * i] = vertices[i].x;
        mesh.vertices[3 * i + 1] = vertices[i].y;
        mesh.vertices[3 * i + 2] = vertices[i].z;
    }

    // R_mesh texcoords array
    for (R_int i = 0; i < mesh.vertex_count; i++) {
        mesh.texcoords[2 * i] = texcoords[i].x;
        mesh.texcoords[2 * i + 1] = texcoords[i].y;
    }

    // R_mesh normals array
    for (R_int i = 0; i < mesh.vertex_count; i++) {
        mesh.normals[3 * i] = normals[i].x;
        mesh.normals[3 * i + 1] = normals[i].y;
        mesh.normals[3 * i + 2] = normals[i].z;
    }

    R_free(temp_allocator, vertices);
    R_free(temp_allocator, normals);
    R_free(temp_allocator, texcoords);

    // Upload vertex data to GPU (static mesh)
    R_gfx_load_mesh(&mesh, false);

    return mesh;
}

// Generate plane mesh (with subdivisions)
R_public R_mesh R_gen_mesh_plane(float width, float length, int res_x, int res_z,
                                 R_allocator allocator, R_allocator temp_allocator) {
    R_mesh mesh = {0};
    mesh.vbo_id = (unsigned int *) R_alloc(allocator, RF_MAX_MESH_VBO * sizeof(unsigned int));
    memset(mesh.vbo_id, 0, RF_MAX_MESH_VBO * sizeof(unsigned int));

#define R_custom_mesh_gen_plane//Todo: Investigate this macro

    R_set_global_dependencies_allocator(temp_allocator);
    {
        par_shapes_mesh *plane =
                par_shapes_create_plane(res_x, res_z);// No normals/texcoords generated!!!
        par_shapes_scale(plane, width, length, 1.0f);

        float axis[] = {1, 0, 0};
        par_shapes_rotate(plane, -R_pi / 2.0f, axis);
        par_shapes_translate(plane, -width / 2, 0.0f, length / 2);

        mesh.vertices = (float *) R_alloc(allocator, plane->ntriangles * 3 * 3 * sizeof(float));
        mesh.texcoords = (float *) R_alloc(allocator, plane->ntriangles * 3 * 2 * sizeof(float));
        mesh.normals = (float *) R_alloc(allocator, plane->ntriangles * 3 * 3 * sizeof(float));
        mesh.vbo_id = (unsigned int *) R_alloc(allocator, RF_MAX_MESH_VBO * sizeof(unsigned int));
        memset(mesh.vbo_id, 0, RF_MAX_MESH_VBO * sizeof(unsigned int));

        mesh.vertex_count = plane->ntriangles * 3;
        mesh.triangle_count = plane->ntriangles;

        for (R_int k = 0; k < mesh.vertex_count; k++) {
            mesh.vertices[k * 3] = plane->points[plane->triangles[k] * 3];
            mesh.vertices[k * 3 + 1] = plane->points[plane->triangles[k] * 3 + 1];
            mesh.vertices[k * 3 + 2] = plane->points[plane->triangles[k] * 3 + 2];

            mesh.normals[k * 3] = plane->normals[plane->triangles[k] * 3];
            mesh.normals[k * 3 + 1] = plane->normals[plane->triangles[k] * 3 + 1];
            mesh.normals[k * 3 + 2] = plane->normals[plane->triangles[k] * 3 + 2];

            mesh.texcoords[k * 2] = plane->tcoords[plane->triangles[k] * 2];
            mesh.texcoords[k * 2 + 1] = plane->tcoords[plane->triangles[k] * 2 + 1];
        }

        par_shapes_free_mesh(plane);
    }
    R_set_global_dependencies_allocator((R_allocator){0});

    // Upload vertex data to GPU (static mesh)
    R_gfx_load_mesh(&mesh, false);

    return mesh;
}

// Generate sphere mesh (standard sphere)
R_public R_mesh R_gen_mesh_sphere(float radius, int rings, int slices, R_allocator allocator,
                                  R_allocator temp_allocator) {
    R_mesh mesh = {0};
    mesh.vbo_id = (unsigned int *) R_alloc(allocator, RF_MAX_MESH_VBO * sizeof(unsigned int));
    memset(mesh.vbo_id, 0, RF_MAX_MESH_VBO * sizeof(unsigned int));

    R_set_global_dependencies_allocator(temp_allocator);
    {
        par_shapes_mesh *sphere = par_shapes_create_parametric_sphere(slices, rings);
        par_shapes_scale(sphere, radius, radius, radius);
        // NOTE: Soft normals are computed internally

        mesh.vertices = (float *) R_alloc(allocator, sphere->ntriangles * 3 * 3 * sizeof(float));
        mesh.texcoords = (float *) R_alloc(allocator, sphere->ntriangles * 3 * 2 * sizeof(float));
        mesh.normals = (float *) R_alloc(allocator, sphere->ntriangles * 3 * 3 * sizeof(float));

        mesh.vertex_count = sphere->ntriangles * 3;
        mesh.triangle_count = sphere->ntriangles;

        for (R_int k = 0; k < mesh.vertex_count; k++) {
            mesh.vertices[k * 3] = sphere->points[sphere->triangles[k] * 3];
            mesh.vertices[k * 3 + 1] = sphere->points[sphere->triangles[k] * 3 + 1];
            mesh.vertices[k * 3 + 2] = sphere->points[sphere->triangles[k] * 3 + 2];

            mesh.normals[k * 3] = sphere->normals[sphere->triangles[k] * 3];
            mesh.normals[k * 3 + 1] = sphere->normals[sphere->triangles[k] * 3 + 1];
            mesh.normals[k * 3 + 2] = sphere->normals[sphere->triangles[k] * 3 + 2];

            mesh.texcoords[k * 2] = sphere->tcoords[sphere->triangles[k] * 2];
            mesh.texcoords[k * 2 + 1] = sphere->tcoords[sphere->triangles[k] * 2 + 1];
        }

        par_shapes_free_mesh(sphere);
    }
    R_set_global_dependencies_allocator((R_allocator){0});

    // Upload vertex data to GPU (static mesh)
    R_gfx_load_mesh(&mesh, false);

    return mesh;
}

// Generate hemi-sphere mesh (half sphere, no bottom cap)
R_public R_mesh R_gen_mesh_hemi_sphere(float radius, int rings, int slices, R_allocator allocator,
                                       R_allocator temp_allocator) {
    R_mesh mesh = {0};
    mesh.vbo_id = (unsigned int *) R_alloc(allocator, RF_MAX_MESH_VBO * sizeof(unsigned int));
    memset(mesh.vbo_id, 0, RF_MAX_MESH_VBO * sizeof(unsigned int));

    R_set_global_dependencies_allocator(temp_allocator);
    {
        par_shapes_mesh *sphere = par_shapes_create_hemisphere(slices, rings);
        par_shapes_scale(sphere, radius, radius, radius);
        // NOTE: Soft normals are computed internally

        mesh.vertices = (float *) R_alloc(allocator, sphere->ntriangles * 3 * 3 * sizeof(float));
        mesh.texcoords = (float *) R_alloc(allocator, sphere->ntriangles * 3 * 2 * sizeof(float));
        mesh.normals = (float *) R_alloc(allocator, sphere->ntriangles * 3 * 3 * sizeof(float));

        mesh.vertex_count = sphere->ntriangles * 3;
        mesh.triangle_count = sphere->ntriangles;

        for (R_int k = 0; k < mesh.vertex_count; k++) {
            mesh.vertices[k * 3] = sphere->points[sphere->triangles[k] * 3];
            mesh.vertices[k * 3 + 1] = sphere->points[sphere->triangles[k] * 3 + 1];
            mesh.vertices[k * 3 + 2] = sphere->points[sphere->triangles[k] * 3 + 2];

            mesh.normals[k * 3] = sphere->normals[sphere->triangles[k] * 3];
            mesh.normals[k * 3 + 1] = sphere->normals[sphere->triangles[k] * 3 + 1];
            mesh.normals[k * 3 + 2] = sphere->normals[sphere->triangles[k] * 3 + 2];

            mesh.texcoords[k * 2] = sphere->tcoords[sphere->triangles[k] * 2];
            mesh.texcoords[k * 2 + 1] = sphere->tcoords[sphere->triangles[k] * 2 + 1];
        }

        par_shapes_free_mesh(sphere);
    }
    R_set_global_dependencies_allocator((R_allocator){0});

    // Upload vertex data to GPU (static mesh)
    R_gfx_load_mesh(&mesh, false);

    return mesh;
}

// Generate cylinder mesh
R_public R_mesh R_gen_mesh_cylinder(float radius, float height, int slices, R_allocator allocator,
                                    R_allocator temp_allocator) {
    R_mesh mesh = {0};
    mesh.vbo_id = (unsigned int *) R_alloc(allocator, RF_MAX_MESH_VBO * sizeof(unsigned int));
    memset(mesh.vbo_id, 0, RF_MAX_MESH_VBO * sizeof(unsigned int));

    R_set_global_dependencies_allocator(temp_allocator);
    {
        // Instance a cylinder that sits on the Z=0 plane using the given tessellation
        // levels across the UV domain.  Think of "slices" like a number of pizza
        // slices, and "stacks" like a number of stacked rings.
        // Height and radius are both 1.0, but they can easily be changed with par_shapes_scale
        par_shapes_mesh *cylinder = par_shapes_create_cylinder(slices, 8);
        par_shapes_scale(cylinder, radius, radius, height);
        float axis[] = {1, 0, 0};
        par_shapes_rotate(cylinder, -R_pi / 2.0f, axis);

        // Generate an orientable disk shape (top cap)
        float center[] = {0, 0, 0};
        float normal[] = {0, 0, 1};
        float normal_minus_1[] = {0, 0, -1};
        par_shapes_mesh *cap_top = par_shapes_create_disk(radius, slices, center, normal);
        cap_top->tcoords = PAR_MALLOC(float, 2 * cap_top->npoints);
        for (R_int i = 0; i < 2 * cap_top->npoints; i++) { cap_top->tcoords[i] = 0.0f; }

        par_shapes_rotate(cap_top, -R_pi / 2.0f, axis);
        par_shapes_translate(cap_top, 0, height, 0);

        // Generate an orientable disk shape (bottom cap)
        par_shapes_mesh *cap_bottom =
                par_shapes_create_disk(radius, slices, center, normal_minus_1);
        cap_bottom->tcoords = PAR_MALLOC(float, 2 * cap_bottom->npoints);
        for (R_int i = 0; i < 2 * cap_bottom->npoints; i++) cap_bottom->tcoords[i] = 0.95f;
        par_shapes_rotate(cap_bottom, R_pi / 2.0f, axis);

        par_shapes_merge_and_free(cylinder, cap_top);
        par_shapes_merge_and_free(cylinder, cap_bottom);

        mesh.vertices = (float *) R_alloc(allocator, cylinder->ntriangles * 3 * 3 * sizeof(float));
        mesh.texcoords = (float *) R_alloc(allocator, cylinder->ntriangles * 3 * 2 * sizeof(float));
        mesh.normals = (float *) R_alloc(allocator, cylinder->ntriangles * 3 * 3 * sizeof(float));

        mesh.vertex_count = cylinder->ntriangles * 3;
        mesh.triangle_count = cylinder->ntriangles;

        for (R_int k = 0; k < mesh.vertex_count; k++) {
            mesh.vertices[k * 3] = cylinder->points[cylinder->triangles[k] * 3];
            mesh.vertices[k * 3 + 1] = cylinder->points[cylinder->triangles[k] * 3 + 1];
            mesh.vertices[k * 3 + 2] = cylinder->points[cylinder->triangles[k] * 3 + 2];

            mesh.normals[k * 3] = cylinder->normals[cylinder->triangles[k] * 3];
            mesh.normals[k * 3 + 1] = cylinder->normals[cylinder->triangles[k] * 3 + 1];
            mesh.normals[k * 3 + 2] = cylinder->normals[cylinder->triangles[k] * 3 + 2];

            mesh.texcoords[k * 2] = cylinder->tcoords[cylinder->triangles[k] * 2];
            mesh.texcoords[k * 2 + 1] = cylinder->tcoords[cylinder->triangles[k] * 2 + 1];
        }

        par_shapes_free_mesh(cylinder);
    }
    R_set_global_dependencies_allocator((R_allocator){0});

    // Upload vertex data to GPU (static mesh)
    R_gfx_load_mesh(&mesh, false);

    return mesh;
}

// Generate torus mesh
R_public R_mesh R_gen_mesh_torus(float radius, float size, int rad_seg, int sides,
                                 R_allocator allocator, R_allocator temp_allocator) {
    R_mesh mesh = {0};
    mesh.vbo_id = (unsigned int *) R_alloc(allocator, RF_MAX_MESH_VBO * sizeof(unsigned int));
    memset(mesh.vbo_id, 0, RF_MAX_MESH_VBO * sizeof(unsigned int));

    if (radius > 1.0f) radius = 1.0f;
    else if (radius < 0.1f)
        radius = 0.1f;

    R_set_global_dependencies_allocator(temp_allocator);
    {
        // Create a donut that sits on the Z=0 plane with the specified inner radius
        // The outer radius can be controlled with par_shapes_scale
        par_shapes_mesh *torus = par_shapes_create_torus(rad_seg, sides, radius);
        par_shapes_scale(torus, size / 2, size / 2, size / 2);

        mesh.vertices = (float *) R_alloc(allocator, torus->ntriangles * 3 * 3 * sizeof(float));
        mesh.texcoords = (float *) R_alloc(allocator, torus->ntriangles * 3 * 2 * sizeof(float));
        mesh.normals = (float *) R_alloc(allocator, torus->ntriangles * 3 * 3 * sizeof(float));

        mesh.vertex_count = torus->ntriangles * 3;
        mesh.triangle_count = torus->ntriangles;

        for (R_int k = 0; k < mesh.vertex_count; k++) {
            mesh.vertices[k * 3] = torus->points[torus->triangles[k] * 3];
            mesh.vertices[k * 3 + 1] = torus->points[torus->triangles[k] * 3 + 1];
            mesh.vertices[k * 3 + 2] = torus->points[torus->triangles[k] * 3 + 2];

            mesh.normals[k * 3] = torus->normals[torus->triangles[k] * 3];
            mesh.normals[k * 3 + 1] = torus->normals[torus->triangles[k] * 3 + 1];
            mesh.normals[k * 3 + 2] = torus->normals[torus->triangles[k] * 3 + 2];

            mesh.texcoords[k * 2] = torus->tcoords[torus->triangles[k] * 2];
            mesh.texcoords[k * 2 + 1] = torus->tcoords[torus->triangles[k] * 2 + 1];
        }

        par_shapes_free_mesh(torus);
    }
    R_set_global_dependencies_allocator((R_allocator){0});

    // Upload vertex data to GPU (static mesh)
    R_gfx_load_mesh(&mesh, false);

    return mesh;
}

// Generate trefoil knot mesh
R_public R_mesh R_gen_mesh_knot(float radius, float size, int rad_seg, int sides,
                                R_allocator allocator, R_allocator temp_allocator) {
    R_mesh mesh = {0};
    mesh.vbo_id = (unsigned int *) R_alloc(allocator, RF_MAX_MESH_VBO * sizeof(unsigned int));
    memset(mesh.vbo_id, 0, RF_MAX_MESH_VBO * sizeof(unsigned int));

    if (radius > 3.0f) radius = 3.0f;
    else if (radius < 0.5f)
        radius = 0.5f;

    R_set_global_dependencies_allocator(temp_allocator);
    {
        par_shapes_mesh *knot = par_shapes_create_trefoil_knot(rad_seg, sides, radius);
        par_shapes_scale(knot, size, size, size);

        mesh.vertices = (float *) R_alloc(allocator, knot->ntriangles * 3 * 3 * sizeof(float));
        mesh.texcoords = (float *) R_alloc(allocator, knot->ntriangles * 3 * 2 * sizeof(float));
        mesh.normals = (float *) R_alloc(allocator, knot->ntriangles * 3 * 3 * sizeof(float));

        mesh.vertex_count = knot->ntriangles * 3;
        mesh.triangle_count = knot->ntriangles;

        for (R_int k = 0; k < mesh.vertex_count; k++) {
            mesh.vertices[k * 3] = knot->points[knot->triangles[k] * 3];
            mesh.vertices[k * 3 + 1] = knot->points[knot->triangles[k] * 3 + 1];
            mesh.vertices[k * 3 + 2] = knot->points[knot->triangles[k] * 3 + 2];

            mesh.normals[k * 3] = knot->normals[knot->triangles[k] * 3];
            mesh.normals[k * 3 + 1] = knot->normals[knot->triangles[k] * 3 + 1];
            mesh.normals[k * 3 + 2] = knot->normals[knot->triangles[k] * 3 + 2];

            mesh.texcoords[k * 2] = knot->tcoords[knot->triangles[k] * 2];
            mesh.texcoords[k * 2 + 1] = knot->tcoords[knot->triangles[k] * 2 + 1];
        }

        par_shapes_free_mesh(knot);
    }
    R_set_global_dependencies_allocator((R_allocator){0});

    // Upload vertex data to GPU (static mesh)
    R_gfx_load_mesh(&mesh, false);

    return mesh;
}

// Generate a mesh from heightmap
// NOTE: Vertex data is uploaded to GPU
R_public R_mesh R_gen_mesh_heightmap(R_image heightmap, R_vec3 size, R_allocator allocator,
                                     R_allocator temp_allocator) {
#define RF_GRAY_VALUE(c) ((c.r + c.g + c.b) / 3)

    R_mesh mesh = {0};
    mesh.vbo_id = (unsigned int *) R_alloc(allocator, RF_MAX_MESH_VBO * sizeof(unsigned int));
    memset(mesh.vbo_id, 0, RF_MAX_MESH_VBO * sizeof(unsigned int));

    int map_x = heightmap.width;
    int map_z = heightmap.height;

    R_color *pixels = R_image_pixels_to_rgba32(heightmap, temp_allocator);

    // NOTE: One vertex per pixel
    mesh.triangle_count = (map_x - 1) * (map_z - 1) * 2;// One quad every four pixels

    mesh.vertex_count = mesh.triangle_count * 3;

    mesh.vertices = (float *) R_alloc(allocator, mesh.vertex_count * 3 * sizeof(float));
    mesh.normals = (float *) R_alloc(allocator, mesh.vertex_count * 3 * sizeof(float));
    mesh.texcoords = (float *) R_alloc(allocator, mesh.vertex_count * 2 * sizeof(float));
    mesh.colors = NULL;

    int vertex_pos_counter = 0;     // Used to count vertices float by float
    int vertex_texcoord_counter = 0;// Used to count texcoords float by float
    int n_counter = 0;              // Used to count normals float by float
    int tris_counter = 0;

    R_vec3 scale_factor = {size.x / map_x, size.y / 255.0f, size.z / map_z};

    for (R_int z = 0; z < map_z - 1; z++) {
        for (R_int x = 0; x < map_x - 1; x++) {
            // Fill vertices array with data
            //----------------------------------------------------------

            // one triangle - 3 vertex
            mesh.vertices[vertex_pos_counter] = (float) x * scale_factor.x;
            mesh.vertices[vertex_pos_counter + 1] =
                    (float) RF_GRAY_VALUE(pixels[x + z * map_x]) * scale_factor.y;
            mesh.vertices[vertex_pos_counter + 2] = (float) z * scale_factor.z;

            mesh.vertices[vertex_pos_counter + 3] = (float) x * scale_factor.x;
            mesh.vertices[vertex_pos_counter + 4] =
                    (float) RF_GRAY_VALUE(pixels[x + (z + 1) * map_x]) * scale_factor.y;
            mesh.vertices[vertex_pos_counter + 5] = (float) (z + 1) * scale_factor.z;

            mesh.vertices[vertex_pos_counter + 6] = (float) (x + 1) * scale_factor.x;
            mesh.vertices[vertex_pos_counter + 7] =
                    (float) RF_GRAY_VALUE(pixels[(x + 1) + z * map_x]) * scale_factor.y;
            mesh.vertices[vertex_pos_counter + 8] = (float) z * scale_factor.z;

            // another triangle - 3 vertex
            mesh.vertices[vertex_pos_counter + 9] = mesh.vertices[vertex_pos_counter + 6];
            mesh.vertices[vertex_pos_counter + 10] = mesh.vertices[vertex_pos_counter + 7];
            mesh.vertices[vertex_pos_counter + 11] = mesh.vertices[vertex_pos_counter + 8];

            mesh.vertices[vertex_pos_counter + 12] = mesh.vertices[vertex_pos_counter + 3];
            mesh.vertices[vertex_pos_counter + 13] = mesh.vertices[vertex_pos_counter + 4];
            mesh.vertices[vertex_pos_counter + 14] = mesh.vertices[vertex_pos_counter + 5];

            mesh.vertices[vertex_pos_counter + 15] = (float) (x + 1) * scale_factor.x;
            mesh.vertices[vertex_pos_counter + 16] =
                    (float) RF_GRAY_VALUE(pixels[(x + 1) + (z + 1) * map_x]) * scale_factor.y;
            mesh.vertices[vertex_pos_counter + 17] = (float) (z + 1) * scale_factor.z;
            vertex_pos_counter += 18;// 6 vertex, 18 floats

            // Fill texcoords array with data
            //--------------------------------------------------------------
            mesh.texcoords[vertex_texcoord_counter] = (float) x / (map_x - 1);
            mesh.texcoords[vertex_texcoord_counter + 1] = (float) z / (map_z - 1);

            mesh.texcoords[vertex_texcoord_counter + 2] = (float) x / (map_x - 1);
            mesh.texcoords[vertex_texcoord_counter + 3] = (float) (z + 1) / (map_z - 1);

            mesh.texcoords[vertex_texcoord_counter + 4] = (float) (x + 1) / (map_x - 1);
            mesh.texcoords[vertex_texcoord_counter + 5] = (float) z / (map_z - 1);

            mesh.texcoords[vertex_texcoord_counter + 6] =
                    mesh.texcoords[vertex_texcoord_counter + 4];
            mesh.texcoords[vertex_texcoord_counter + 7] =
                    mesh.texcoords[vertex_texcoord_counter + 5];

            mesh.texcoords[vertex_texcoord_counter + 8] =
                    mesh.texcoords[vertex_texcoord_counter + 2];
            mesh.texcoords[vertex_texcoord_counter + 9] =
                    mesh.texcoords[vertex_texcoord_counter + 3];

            mesh.texcoords[vertex_texcoord_counter + 10] = (float) (x + 1) / (map_x - 1);
            mesh.texcoords[vertex_texcoord_counter + 11] = (float) (z + 1) / (map_z - 1);

            vertex_texcoord_counter += 12;// 6 texcoords, 12 floats

            // Fill normals array with data
            //--------------------------------------------------------------
            for (R_int i = 0; i < 18; i += 3) {
                mesh.normals[n_counter + i] = 0.0f;
                mesh.normals[n_counter + i + 1] = 1.0f;
                mesh.normals[n_counter + i + 2] = 0.0f;
            }

            // TODO: Calculate normals in an efficient way

            n_counter += 18;// 6 vertex, 18 floats
            tris_counter += 2;
        }
    }

    R_free(temp_allocator, pixels);

    // Upload vertex data to GPU (static mesh)
    R_gfx_load_mesh(&mesh, false);

    return mesh;
}

// Generate a cubes mesh from pixel data
// NOTE: Vertex data is uploaded to GPU
R_public R_mesh R_gen_mesh_cubicmap(R_image cubicmap, R_vec3 cube_size, R_allocator allocator,
                                    R_allocator temp_allocator) {
    R_mesh mesh = {0};
    mesh.vbo_id = (unsigned int *) R_alloc(allocator, RF_MAX_MESH_VBO * sizeof(unsigned int));
    memset(mesh.vbo_id, 0, RF_MAX_MESH_VBO * sizeof(unsigned int));

    R_color *cubicmap_pixels = R_image_pixels_to_rgba32(cubicmap, temp_allocator);

    int map_width = cubicmap.width;
    int map_height = cubicmap.height;

    // NOTE: Max possible number of triangles numCubes*(12 triangles by cube)
    int maxTriangles = cubicmap.width * cubicmap.height * 12;

    int vertex_pos_counter = 0;     // Used to count vertices
    int vertex_texcoord_counter = 0;// Used to count texcoords
    int n_counter = 0;              // Used to count normals

    float w = cube_size.x;
    float h = cube_size.z;
    float h2 = cube_size.y;

    R_vec3 *map_vertices = (R_vec3 *) R_alloc(temp_allocator, maxTriangles * 3 * sizeof(R_vec3));
    R_vec2 *map_texcoords = (R_vec2 *) R_alloc(temp_allocator, maxTriangles * 3 * sizeof(R_vec2));
    R_vec3 *map_normals = (R_vec3 *) R_alloc(temp_allocator, maxTriangles * 3 * sizeof(R_vec3));

    // Define the 6 normals of the cube, we will combine them accordingly later...
    R_vec3 n1 = {1.0f, 0.0f, 0.0f};
    R_vec3 n2 = {-1.0f, 0.0f, 0.0f};
    R_vec3 n3 = {0.0f, 1.0f, 0.0f};
    R_vec3 n4 = {0.0f, -1.0f, 0.0f};
    R_vec3 n5 = {0.0f, 0.0f, 1.0f};
    R_vec3 n6 = {0.0f, 0.0f, -1.0f};

    // NOTE: We use texture rectangles to define different textures for top-bottom-front-back-right-left (6)
    typedef struct R_recf R_recf;
    struct R_recf
    {
        float x;
        float y;
        float width;
        float height;
    };

    R_recf right_tex_uv = {0.0f, 0.0f, 0.5f, 0.5f};
    R_recf left_tex_uv = {0.5f, 0.0f, 0.5f, 0.5f};
    R_recf front_tex_uv = {0.0f, 0.0f, 0.5f, 0.5f};
    R_recf back_tex_uv = {0.5f, 0.0f, 0.5f, 0.5f};
    R_recf top_tex_uv = {0.0f, 0.5f, 0.5f, 0.5f};
    R_recf bottom_tex_uv = {0.5f, 0.5f, 0.5f, 0.5f};

    for (R_int z = 0; z < map_height; ++z) {
        for (R_int x = 0; x < map_width; ++x) {
            // Define the 8 vertex of the cube, we will combine them accordingly later...
            R_vec3 v1 = {w * (x - 0.5f), h2, h * (z - 0.5f)};
            R_vec3 v2 = {w * (x - 0.5f), h2, h * (z + 0.5f)};
            R_vec3 v3 = {w * (x + 0.5f), h2, h * (z + 0.5f)};
            R_vec3 v4 = {w * (x + 0.5f), h2, h * (z - 0.5f)};
            R_vec3 v5 = {w * (x + 0.5f), 0, h * (z - 0.5f)};
            R_vec3 v6 = {w * (x - 0.5f), 0, h * (z - 0.5f)};
            R_vec3 v7 = {w * (x - 0.5f), 0, h * (z + 0.5f)};
            R_vec3 v8 = {w * (x + 0.5f), 0, h * (z + 0.5f)};

            // We check pixel color to be RF_WHITE, we will full cubes
            if ((cubicmap_pixels[z * cubicmap.width + x].r == 255) &&
                (cubicmap_pixels[z * cubicmap.width + x].g == 255) &&
                (cubicmap_pixels[z * cubicmap.width + x].b == 255)) {
                // Define triangles (Checking Collateral Cubes!)
                //----------------------------------------------

                // Define top triangles (2 tris, 6 vertex --> v1-v2-v3, v1-v3-v4)
                map_vertices[vertex_pos_counter] = v1;
                map_vertices[vertex_pos_counter + 1] = v2;
                map_vertices[vertex_pos_counter + 2] = v3;
                map_vertices[vertex_pos_counter + 3] = v1;
                map_vertices[vertex_pos_counter + 4] = v3;
                map_vertices[vertex_pos_counter + 5] = v4;
                vertex_pos_counter += 6;

                map_normals[n_counter] = n3;
                map_normals[n_counter + 1] = n3;
                map_normals[n_counter + 2] = n3;
                map_normals[n_counter + 3] = n3;
                map_normals[n_counter + 4] = n3;
                map_normals[n_counter + 5] = n3;
                n_counter += 6;

                map_texcoords[vertex_texcoord_counter] = (R_vec2){top_tex_uv.x, top_tex_uv.y};
                map_texcoords[vertex_texcoord_counter + 1] =
                        (R_vec2){top_tex_uv.x, top_tex_uv.y + top_tex_uv.height};
                map_texcoords[vertex_texcoord_counter + 2] =
                        (R_vec2){top_tex_uv.x + top_tex_uv.width, top_tex_uv.y + top_tex_uv.height};
                map_texcoords[vertex_texcoord_counter + 3] = (R_vec2){top_tex_uv.x, top_tex_uv.y};
                map_texcoords[vertex_texcoord_counter + 4] =
                        (R_vec2){top_tex_uv.x + top_tex_uv.width, top_tex_uv.y + top_tex_uv.height};
                map_texcoords[vertex_texcoord_counter + 5] =
                        (R_vec2){top_tex_uv.x + top_tex_uv.width, top_tex_uv.y};
                vertex_texcoord_counter += 6;

                // Define bottom triangles (2 tris, 6 vertex --> v6-v8-v7, v6-v5-v8)
                map_vertices[vertex_pos_counter] = v6;
                map_vertices[vertex_pos_counter + 1] = v8;
                map_vertices[vertex_pos_counter + 2] = v7;
                map_vertices[vertex_pos_counter + 3] = v6;
                map_vertices[vertex_pos_counter + 4] = v5;
                map_vertices[vertex_pos_counter + 5] = v8;
                vertex_pos_counter += 6;

                map_normals[n_counter] = n4;
                map_normals[n_counter + 1] = n4;
                map_normals[n_counter + 2] = n4;
                map_normals[n_counter + 3] = n4;
                map_normals[n_counter + 4] = n4;
                map_normals[n_counter + 5] = n4;
                n_counter += 6;

                map_texcoords[vertex_texcoord_counter] =
                        (R_vec2){bottom_tex_uv.x + bottom_tex_uv.width, bottom_tex_uv.y};
                map_texcoords[vertex_texcoord_counter + 1] =
                        (R_vec2){bottom_tex_uv.x, bottom_tex_uv.y + bottom_tex_uv.height};
                map_texcoords[vertex_texcoord_counter + 2] =
                        (R_vec2){bottom_tex_uv.x + bottom_tex_uv.width,
                                 bottom_tex_uv.y + bottom_tex_uv.height};
                map_texcoords[vertex_texcoord_counter + 3] =
                        (R_vec2){bottom_tex_uv.x + bottom_tex_uv.width, bottom_tex_uv.y};
                map_texcoords[vertex_texcoord_counter + 4] =
                        (R_vec2){bottom_tex_uv.x, bottom_tex_uv.y};
                map_texcoords[vertex_texcoord_counter + 5] =
                        (R_vec2){bottom_tex_uv.x, bottom_tex_uv.y + bottom_tex_uv.height};
                vertex_texcoord_counter += 6;

                if (((z < cubicmap.height - 1) &&
                     (cubicmap_pixels[(z + 1) * cubicmap.width + x].r == 0) &&
                     (cubicmap_pixels[(z + 1) * cubicmap.width + x].g == 0) &&
                     (cubicmap_pixels[(z + 1) * cubicmap.width + x].b == 0)) ||
                    (z == cubicmap.height - 1)) {
                    // Define front triangles (2 tris, 6 vertex) --> v2 v7 v3, v3 v7 v8
                    // NOTE: Collateral occluded faces are not generated
                    map_vertices[vertex_pos_counter] = v2;
                    map_vertices[vertex_pos_counter + 1] = v7;
                    map_vertices[vertex_pos_counter + 2] = v3;
                    map_vertices[vertex_pos_counter + 3] = v3;
                    map_vertices[vertex_pos_counter + 4] = v7;
                    map_vertices[vertex_pos_counter + 5] = v8;
                    vertex_pos_counter += 6;

                    map_normals[n_counter] = n6;
                    map_normals[n_counter + 1] = n6;
                    map_normals[n_counter + 2] = n6;
                    map_normals[n_counter + 3] = n6;
                    map_normals[n_counter + 4] = n6;
                    map_normals[n_counter + 5] = n6;
                    n_counter += 6;

                    map_texcoords[vertex_texcoord_counter] =
                            (R_vec2){front_tex_uv.x, front_tex_uv.y};
                    map_texcoords[vertex_texcoord_counter + 1] =
                            (R_vec2){front_tex_uv.x, front_tex_uv.y + front_tex_uv.height};
                    map_texcoords[vertex_texcoord_counter + 2] =
                            (R_vec2){front_tex_uv.x + front_tex_uv.width, front_tex_uv.y};
                    map_texcoords[vertex_texcoord_counter + 3] =
                            (R_vec2){front_tex_uv.x + front_tex_uv.width, front_tex_uv.y};
                    map_texcoords[vertex_texcoord_counter + 4] =
                            (R_vec2){front_tex_uv.x, front_tex_uv.y + front_tex_uv.height};
                    map_texcoords[vertex_texcoord_counter + 5] =
                            (R_vec2){front_tex_uv.x + front_tex_uv.width,
                                     front_tex_uv.y + front_tex_uv.height};
                    vertex_texcoord_counter += 6;
                }

                if (((z > 0) && (cubicmap_pixels[(z - 1) * cubicmap.width + x].r == 0) &&
                     (cubicmap_pixels[(z - 1) * cubicmap.width + x].g == 0) &&
                     (cubicmap_pixels[(z - 1) * cubicmap.width + x].b == 0)) ||
                    (z == 0)) {
                    // Define back triangles (2 tris, 6 vertex) --> v1 v5 v6, v1 v4 v5
                    // NOTE: Collateral occluded faces are not generated
                    map_vertices[vertex_pos_counter] = v1;
                    map_vertices[vertex_pos_counter + 1] = v5;
                    map_vertices[vertex_pos_counter + 2] = v6;
                    map_vertices[vertex_pos_counter + 3] = v1;
                    map_vertices[vertex_pos_counter + 4] = v4;
                    map_vertices[vertex_pos_counter + 5] = v5;
                    vertex_pos_counter += 6;

                    map_normals[n_counter] = n5;
                    map_normals[n_counter + 1] = n5;
                    map_normals[n_counter + 2] = n5;
                    map_normals[n_counter + 3] = n5;
                    map_normals[n_counter + 4] = n5;
                    map_normals[n_counter + 5] = n5;
                    n_counter += 6;

                    map_texcoords[vertex_texcoord_counter] =
                            (R_vec2){back_tex_uv.x + back_tex_uv.width, back_tex_uv.y};
                    map_texcoords[vertex_texcoord_counter + 1] =
                            (R_vec2){back_tex_uv.x, back_tex_uv.y + back_tex_uv.height};
                    map_texcoords[vertex_texcoord_counter + 2] = (R_vec2){
                            back_tex_uv.x + back_tex_uv.width, back_tex_uv.y + back_tex_uv.height};
                    map_texcoords[vertex_texcoord_counter + 3] =
                            (R_vec2){back_tex_uv.x + back_tex_uv.width, back_tex_uv.y};
                    map_texcoords[vertex_texcoord_counter + 4] =
                            (R_vec2){back_tex_uv.x, back_tex_uv.y};
                    map_texcoords[vertex_texcoord_counter + 5] =
                            (R_vec2){back_tex_uv.x, back_tex_uv.y + back_tex_uv.height};
                    vertex_texcoord_counter += 6;
                }

                if (((x < cubicmap.width - 1) &&
                     (cubicmap_pixels[z * cubicmap.width + (x + 1)].r == 0) &&
                     (cubicmap_pixels[z * cubicmap.width + (x + 1)].g == 0) &&
                     (cubicmap_pixels[z * cubicmap.width + (x + 1)].b == 0)) ||
                    (x == cubicmap.width - 1)) {
                    // Define right triangles (2 tris, 6 vertex) --> v3 v8 v4, v4 v8 v5
                    // NOTE: Collateral occluded faces are not generated
                    map_vertices[vertex_pos_counter] = v3;
                    map_vertices[vertex_pos_counter + 1] = v8;
                    map_vertices[vertex_pos_counter + 2] = v4;
                    map_vertices[vertex_pos_counter + 3] = v4;
                    map_vertices[vertex_pos_counter + 4] = v8;
                    map_vertices[vertex_pos_counter + 5] = v5;
                    vertex_pos_counter += 6;

                    map_normals[n_counter] = n1;
                    map_normals[n_counter + 1] = n1;
                    map_normals[n_counter + 2] = n1;
                    map_normals[n_counter + 3] = n1;
                    map_normals[n_counter + 4] = n1;
                    map_normals[n_counter + 5] = n1;
                    n_counter += 6;

                    map_texcoords[vertex_texcoord_counter] =
                            (R_vec2){right_tex_uv.x, right_tex_uv.y};
                    map_texcoords[vertex_texcoord_counter + 1] =
                            (R_vec2){right_tex_uv.x, right_tex_uv.y + right_tex_uv.height};
                    map_texcoords[vertex_texcoord_counter + 2] =
                            (R_vec2){right_tex_uv.x + right_tex_uv.width, right_tex_uv.y};
                    map_texcoords[vertex_texcoord_counter + 3] =
                            (R_vec2){right_tex_uv.x + right_tex_uv.width, right_tex_uv.y};
                    map_texcoords[vertex_texcoord_counter + 4] =
                            (R_vec2){right_tex_uv.x, right_tex_uv.y + right_tex_uv.height};
                    map_texcoords[vertex_texcoord_counter + 5] =
                            (R_vec2){right_tex_uv.x + right_tex_uv.width,
                                     right_tex_uv.y + right_tex_uv.height};
                    vertex_texcoord_counter += 6;
                }

                if (((x > 0) && (cubicmap_pixels[z * cubicmap.width + (x - 1)].r == 0) &&
                     (cubicmap_pixels[z * cubicmap.width + (x - 1)].g == 0) &&
                     (cubicmap_pixels[z * cubicmap.width + (x - 1)].b == 0)) ||
                    (x == 0)) {
                    // Define left triangles (2 tris, 6 vertex) --> v1 v7 v2, v1 v6 v7
                    // NOTE: Collateral occluded faces are not generated
                    map_vertices[vertex_pos_counter] = v1;
                    map_vertices[vertex_pos_counter + 1] = v7;
                    map_vertices[vertex_pos_counter + 2] = v2;
                    map_vertices[vertex_pos_counter + 3] = v1;
                    map_vertices[vertex_pos_counter + 4] = v6;
                    map_vertices[vertex_pos_counter + 5] = v7;
                    vertex_pos_counter += 6;

                    map_normals[n_counter] = n2;
                    map_normals[n_counter + 1] = n2;
                    map_normals[n_counter + 2] = n2;
                    map_normals[n_counter + 3] = n2;
                    map_normals[n_counter + 4] = n2;
                    map_normals[n_counter + 5] = n2;
                    n_counter += 6;

                    map_texcoords[vertex_texcoord_counter] = (R_vec2){left_tex_uv.x, left_tex_uv.y};
                    map_texcoords[vertex_texcoord_counter + 1] = (R_vec2){
                            left_tex_uv.x + left_tex_uv.width, left_tex_uv.y + left_tex_uv.height};
                    map_texcoords[vertex_texcoord_counter + 2] =
                            (R_vec2){left_tex_uv.x + left_tex_uv.width, left_tex_uv.y};
                    map_texcoords[vertex_texcoord_counter + 3] =
                            (R_vec2){left_tex_uv.x, left_tex_uv.y};
                    map_texcoords[vertex_texcoord_counter + 4] =
                            (R_vec2){left_tex_uv.x, left_tex_uv.y + left_tex_uv.height};
                    map_texcoords[vertex_texcoord_counter + 5] = (R_vec2){
                            left_tex_uv.x + left_tex_uv.width, left_tex_uv.y + left_tex_uv.height};
                    vertex_texcoord_counter += 6;
                }
            }
            // We check pixel color to be RF_BLACK, we will only draw floor and roof
            else if ((cubicmap_pixels[z * cubicmap.width + x].r == 0) &&
                     (cubicmap_pixels[z * cubicmap.width + x].g == 0) &&
                     (cubicmap_pixels[z * cubicmap.width + x].b == 0)) {
                // Define top triangles (2 tris, 6 vertex --> v1-v2-v3, v1-v3-v4)
                map_vertices[vertex_pos_counter] = v1;
                map_vertices[vertex_pos_counter + 1] = v3;
                map_vertices[vertex_pos_counter + 2] = v2;
                map_vertices[vertex_pos_counter + 3] = v1;
                map_vertices[vertex_pos_counter + 4] = v4;
                map_vertices[vertex_pos_counter + 5] = v3;
                vertex_pos_counter += 6;

                map_normals[n_counter] = n4;
                map_normals[n_counter + 1] = n4;
                map_normals[n_counter + 2] = n4;
                map_normals[n_counter + 3] = n4;
                map_normals[n_counter + 4] = n4;
                map_normals[n_counter + 5] = n4;
                n_counter += 6;

                map_texcoords[vertex_texcoord_counter] = (R_vec2){top_tex_uv.x, top_tex_uv.y};
                map_texcoords[vertex_texcoord_counter + 1] =
                        (R_vec2){top_tex_uv.x + top_tex_uv.width, top_tex_uv.y + top_tex_uv.height};
                map_texcoords[vertex_texcoord_counter + 2] =
                        (R_vec2){top_tex_uv.x, top_tex_uv.y + top_tex_uv.height};
                map_texcoords[vertex_texcoord_counter + 3] = (R_vec2){top_tex_uv.x, top_tex_uv.y};
                map_texcoords[vertex_texcoord_counter + 4] =
                        (R_vec2){top_tex_uv.x + top_tex_uv.width, top_tex_uv.y};
                map_texcoords[vertex_texcoord_counter + 5] =
                        (R_vec2){top_tex_uv.x + top_tex_uv.width, top_tex_uv.y + top_tex_uv.height};
                vertex_texcoord_counter += 6;

                // Define bottom triangles (2 tris, 6 vertex --> v6-v8-v7, v6-v5-v8)
                map_vertices[vertex_pos_counter] = v6;
                map_vertices[vertex_pos_counter + 1] = v7;
                map_vertices[vertex_pos_counter + 2] = v8;
                map_vertices[vertex_pos_counter + 3] = v6;
                map_vertices[vertex_pos_counter + 4] = v8;
                map_vertices[vertex_pos_counter + 5] = v5;
                vertex_pos_counter += 6;

                map_normals[n_counter] = n3;
                map_normals[n_counter + 1] = n3;
                map_normals[n_counter + 2] = n3;
                map_normals[n_counter + 3] = n3;
                map_normals[n_counter + 4] = n3;
                map_normals[n_counter + 5] = n3;
                n_counter += 6;

                map_texcoords[vertex_texcoord_counter] =
                        (R_vec2){bottom_tex_uv.x + bottom_tex_uv.width, bottom_tex_uv.y};
                map_texcoords[vertex_texcoord_counter + 1] =
                        (R_vec2){bottom_tex_uv.x + bottom_tex_uv.width,
                                 bottom_tex_uv.y + bottom_tex_uv.height};
                map_texcoords[vertex_texcoord_counter + 2] =
                        (R_vec2){bottom_tex_uv.x, bottom_tex_uv.y + bottom_tex_uv.height};
                map_texcoords[vertex_texcoord_counter + 3] =
                        (R_vec2){bottom_tex_uv.x + bottom_tex_uv.width, bottom_tex_uv.y};
                map_texcoords[vertex_texcoord_counter + 4] =
                        (R_vec2){bottom_tex_uv.x, bottom_tex_uv.y + bottom_tex_uv.height};
                map_texcoords[vertex_texcoord_counter + 5] =
                        (R_vec2){bottom_tex_uv.x, bottom_tex_uv.y};
                vertex_texcoord_counter += 6;
            }
        }
    }

    // Move data from map_vertices temp arays to vertices float array
    mesh.vertex_count = vertex_pos_counter;
    mesh.triangle_count = vertex_pos_counter / 3;

    mesh.vertices = (float *) R_alloc(allocator, mesh.vertex_count * 3 * sizeof(float));
    mesh.normals = (float *) R_alloc(allocator, mesh.vertex_count * 3 * sizeof(float));
    mesh.texcoords = (float *) R_alloc(allocator, mesh.vertex_count * 2 * sizeof(float));
    mesh.colors = NULL;

    int f_counter = 0;

    // Move vertices data
    for (R_int i = 0; i < vertex_pos_counter; i++) {
        mesh.vertices[f_counter] = map_vertices[i].x;
        mesh.vertices[f_counter + 1] = map_vertices[i].y;
        mesh.vertices[f_counter + 2] = map_vertices[i].z;
        f_counter += 3;
    }

    f_counter = 0;

    // Move normals data
    for (R_int i = 0; i < n_counter; i++) {
        mesh.normals[f_counter] = map_normals[i].x;
        mesh.normals[f_counter + 1] = map_normals[i].y;
        mesh.normals[f_counter + 2] = map_normals[i].z;
        f_counter += 3;
    }

    f_counter = 0;

    // Move texcoords data
    for (R_int i = 0; i < vertex_texcoord_counter; i++) {
        mesh.texcoords[f_counter] = map_texcoords[i].x;
        mesh.texcoords[f_counter + 1] = map_texcoords[i].y;
        f_counter += 2;
    }

    R_free(temp_allocator, map_vertices);
    R_free(temp_allocator, map_normals);
    R_free(temp_allocator, map_texcoords);

    R_free(temp_allocator, cubicmap_pixels);// Free image pixel data

    // Upload vertex data to GPU (static mesh)
    R_gfx_load_mesh(&mesh, false);

    return mesh;
}

#pragma endregion

#pragma region internal functions

// Get texture to draw shapes, the user can customize this using R_set_shapes_texture
R_internal R_texture2d R_get_shapes_texture() {
    if (R_ctx.tex_shapes.id == 0) {
        R_ctx.tex_shapes = R_get_default_texture();
        R_ctx.rec_tex_shapes = (R_rec){0.0f, 0.0f, 1.0f, 1.0f};
    }

    return R_ctx.tex_shapes;
}

// Cubic easing in-out. Note: Required for R_draw_line_bezier()
R_internal float R_shapes_ease_cubic_in_out(float t, float b, float c, float d) {
    if ((t /= 0.5f * d) < 1) return 0.5f * c * t * t * t + b;

    t -= 2;

    return 0.5f * c * (t * t * t + 2.0f) + b;
}

#pragma endregion

R_public R_vec2 R_center_to_screen(float w, float h) {
    R_vec2 result = {R_ctx.render_width / 2 - w / 2, R_ctx.render_height / 2 - h / 2};
    return result;
}

// Set background color (framebuffer clear color)
R_public void R_clear(R_color color) {
    R_gfx_clear_color(color.r, color.g, color.b, color.a);// Set clear color
    R_gfx_clear_screen_buffers();                         // Clear current framebuffers
}

// Setup canvas (framebuffer) to start drawing
R_public void R_begin() {
    R_gfx_load_identity();                                       // Reset current matrix (MODELVIEW)
    R_gfx_mult_matrixf(R_mat_to_float16(R_ctx.screen_scaling).v);// Apply screen scaling

    //R_gfx_translatef(0.375, 0.375, 0);    // HACK to have 2D pixel-perfect drawing on OpenGL 1.1, not required with OpenGL 3.3+
}

// End canvas drawing and swap buffers (double buffering)
R_public void R_end() { R_gfx_draw(); }

// Initialize 2D mode with custom camera (2D)
R_public void R_begin_2d(R_camera2d camera) {
    R_gfx_draw();

    R_gfx_load_identity();// Reset current matrix (MODELVIEW)

    // Apply screen scaling if required
    R_gfx_mult_matrixf(R_mat_to_float16(R_ctx.screen_scaling).v);

    // Apply 2d camera transformation to R_gfxobal_model_view
    R_gfx_mult_matrixf(R_mat_to_float16(R_get_camera_matrix2d(camera)).v);
}

// Ends 2D mode with custom camera
R_public void R_end_2d() {
    R_gfx_draw();

    R_gfx_load_identity();                                       // Reset current matrix (MODELVIEW)
    R_gfx_mult_matrixf(R_mat_to_float16(R_ctx.screen_scaling).v);// Apply screen scaling if required
}

// Initializes 3D mode with custom camera (3D)
R_public void R_begin_3d(R_camera3d camera) {
    R_gfx_draw();

    R_gfx_matrix_mode(RF_PROJECTION);// Switch to GL_PROJECTION matrix
    R_gfx_push_matrix();// Save previous matrix, which contains the settings for the 2d ortho GL_PROJECTION
    R_gfx_load_identity();// Reset current matrix (PROJECTION)

    float aspect = (float) R_ctx.current_width / (float) R_ctx.current_height;

    if (camera.type == RF_CAMERA_PERSPECTIVE) {
        // Setup perspective GL_PROJECTION
        double top = 0.01 * tan(camera.fovy * 0.5 * R_deg2rad);
        double right = top * aspect;

        R_gfx_frustum(-right, right, -top, top, 0.01, 1000.0);
    } else if (camera.type == RF_CAMERA_ORTHOGRAPHIC) {
        // Setup orthographic GL_PROJECTION
        double top = camera.fovy / 2.0;
        double right = top * aspect;

        R_gfx_ortho(-right, right, -top, top, 0.01, 1000.0);
    }

    // NOTE: zNear and zFar values are important when computing depth buffer values

    R_gfx_matrix_mode(RF_MODELVIEW);// Switch back to R_gfxobal_model_view matrix
    R_gfx_load_identity();          // Reset current matrix (MODELVIEW)

    // Setup R_camera3d view
    R_mat mat_view = R_mat_look_at(camera.position, camera.target, camera.up);
    R_gfx_mult_matrixf(
            R_mat_to_float16(mat_view).v);// Multiply MODELVIEW matrix by view matrix (camera)

    R_gfx_enable_depth_test();// Enable DEPTH_TEST for 3D
}

// Ends 3D mode and returns to default 2D orthographic mode
R_public void R_end_3d() {
    R_gfx_draw();// Process internal buffers (update + draw)

    R_gfx_matrix_mode(RF_PROJECTION);// Switch to GL_PROJECTION matrix
    R_gfx_pop_matrix();// Restore previous matrix (PROJECTION) from matrix R_gfxobal_gl_stack

    R_gfx_matrix_mode(RF_MODELVIEW);// Get back to R_gfxobal_model_view matrix
    R_gfx_load_identity();          // Reset current matrix (MODELVIEW)

    R_gfx_mult_matrixf(R_mat_to_float16(R_ctx.screen_scaling).v);// Apply screen scaling if required

    R_gfx_disable_depth_test();// Disable DEPTH_TEST for 2D
}

// Initializes render texture for drawing
R_public void R_begin_render_to_texture(R_render_texture2d target) {
    R_gfx_draw();

    R_gfx_enable_render_texture(target.id);// Enable render target

    // Set viewport to framebuffer size
    R_gfx_viewport(0, 0, target.texture.width, target.texture.height);

    R_gfx_matrix_mode(RF_PROJECTION);// Switch to PROJECTION matrix
    R_gfx_load_identity();           // Reset current matrix (PROJECTION)

    // Set orthographic GL_PROJECTION to current framebuffer size
    // NOTE: Configured top-left corner as (0, 0)
    R_gfx_ortho(0, target.texture.width, target.texture.height, 0, 0.0f, 1.0f);

    R_gfx_matrix_mode(RF_MODELVIEW);// Switch back to MODELVIEW matrix
    R_gfx_load_identity();          // Reset current matrix (MODELVIEW)

    //R_gfx_scalef(0.0f, -1.0f, 0.0f);      // Flip Y-drawing (?)

    // Setup current width/height for proper aspect ratio
    // calculation when using R_begin_mode3d()
    R_ctx.current_width = target.texture.width;
    R_ctx.current_height = target.texture.height;
}

// Ends drawing to render texture
R_public void R_end_render_to_texture() {
    R_gfx_draw();

    R_gfx_disable_render_texture();// Disable render target

    // Set viewport to default framebuffer size
    R_set_viewport(R_ctx.render_width, R_ctx.render_height);

    // Reset current screen size
    R_ctx.current_width = R_ctx.render_width;
    R_ctx.current_height = R_ctx.render_height;
}

// Begin scissor mode (define screen area for following drawing)
// NOTE: Scissor rec refers to bottom-left corner, we change it to upper-left
R_public void R_begin_scissor_mode(int x, int y, int width, int height) {
    R_gfx_draw();// Force drawing elements

    R_gfx_enable_scissor_test();
    R_gfx_scissor(x, R_ctx.render_width - (y + height), width, height);
}

// End scissor mode
R_public void R_end_scissor_mode() {
    R_gfx_draw();// Force drawing elements
    R_gfx_disable_scissor_test();
}

// Begin custom shader mode
R_public void R_begin_shader(R_shader shader) {
    if (R_ctx.current_shader.id != shader.id) {
        R_gfx_draw();
        R_ctx.current_shader = shader;
    }
}

// End custom shader mode (returns to default shader)
R_public void R_end_shader() { R_begin_shader(R_ctx.default_shader); }

// Begin blending mode (alpha, additive, multiplied). Default blend mode is alpha
R_public void R_begin_blend_mode(R_blend_mode mode) { R_gfx_blend_mode(mode); }

// End blending mode (reset to default: alpha blending)
R_public void R_end_blend_mode() { R_gfx_blend_mode(RF_BLEND_ALPHA); }

// Draw a pixel
R_public void R_draw_pixel(int pos_x, int pos_y, R_color color) {
    R_gfx_begin(RF_LINES);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);
    R_gfx_vertex2i(pos_x, pos_y);
    R_gfx_vertex2i(pos_x + 1, pos_y + 1);
    R_gfx_end();
}

// Draw a pixel (Vector version)
R_public void R_draw_pixel_v(R_vec2 position, R_color color) {
    R_draw_pixel(position.x, position.y, color);
}

// Draw a line
R_public void R_draw_line(int startPosX, int startPosY, int endPosX, int endPosY, R_color color) {
    R_gfx_begin(RF_LINES);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);
    R_gfx_vertex2i(startPosX, startPosY);
    R_gfx_vertex2i(endPosX, endPosY);
    R_gfx_end();
}

// Draw a line (Vector version)
R_public void R_draw_line_v(R_vec2 startPos, R_vec2 endPos, R_color color) {
    R_draw_line(startPos.x, startPos.y, endPos.x, endPos.y, color);
}

// Draw a line defining thickness
R_public void R_draw_line_ex(R_vec2 start_pos, R_vec2 end_pos, float thick, R_color color) {
    if (start_pos.x > end_pos.x) {
        R_vec2 temp_pos = start_pos;
        start_pos = end_pos;
        end_pos = temp_pos;
    }

    float dx = end_pos.x - start_pos.x;
    float dy = end_pos.y - start_pos.y;

    float d = sqrtf(dx * dx + dy * dy);
    float angle = asinf(dy / d);

    R_gfx_enable_texture(R_get_shapes_texture().id);

    R_gfx_push_matrix();
    R_gfx_translatef((float) start_pos.x, (float) start_pos.y, 0.0f);
    R_gfx_rotatef(R_rad2deg * angle, 0.0f, 0.0f, 1.0f);
    R_gfx_translatef(0, (thick > 1.0f) ? -thick / 2.0f : -1.0f, 0.0f);

    R_gfx_begin(RF_QUADS);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);
    R_gfx_normal3f(0.0f, 0.0f, 1.0f);

    R_gfx_tex_coord2f(R_ctx.rec_tex_shapes.x / R_ctx.tex_shapes.width,
                      R_ctx.rec_tex_shapes.y / R_ctx.tex_shapes.height);
    R_gfx_vertex2f(0.0f, 0.0f);

    R_gfx_tex_coord2f(R_ctx.rec_tex_shapes.x / R_ctx.tex_shapes.width,
                      (R_ctx.rec_tex_shapes.y + R_ctx.rec_tex_shapes.height) /
                              R_ctx.tex_shapes.height);
    R_gfx_vertex2f(0.0f, thick);

    R_gfx_tex_coord2f(
            (R_ctx.rec_tex_shapes.x + R_ctx.rec_tex_shapes.width) / R_ctx.tex_shapes.width,
            (R_ctx.rec_tex_shapes.y + R_ctx.rec_tex_shapes.height) / R_ctx.tex_shapes.height);
    R_gfx_vertex2f(d, thick);

    R_gfx_tex_coord2f((R_ctx.rec_tex_shapes.x + R_ctx.rec_tex_shapes.width) /
                              R_ctx.tex_shapes.width,
                      R_ctx.rec_tex_shapes.y / R_ctx.tex_shapes.height);
    R_gfx_vertex2f(d, 0.0f);
    R_gfx_end();
    R_gfx_pop_matrix();

    R_gfx_disable_texture();
}

// Draw line using cubic-bezier curves in-out
R_public void R_draw_line_bezier(R_vec2 start_pos, R_vec2 end_pos, float thick, R_color color) {
#define RF_LINE_DIVISIONS 24// Bezier line divisions

    R_vec2 previous = start_pos;
    R_vec2 current;

    for (R_int i = 1; i <= RF_LINE_DIVISIONS; i++) {
        // Cubic easing in-out
        // NOTE: Easing is calculated only for y position value
        current.y = R_shapes_ease_cubic_in_out((float) i, start_pos.y, end_pos.y - start_pos.y,
                                               (float) RF_LINE_DIVISIONS);
        current.x = previous.x + (end_pos.x - start_pos.x) / (float) RF_LINE_DIVISIONS;

        R_draw_line_ex(previous, current, thick, color);

        previous = current;
    }

#undef RF_LINE_DIVISIONS
}

// Draw lines sequence
R_public void R_draw_line_strip(R_vec2 *points, int points_count, R_color color) {
    if (points_count >= 2) {
        if (R_gfx_check_buffer_limit(points_count)) R_gfx_draw();

        R_gfx_begin(RF_LINES);
        R_gfx_color4ub(color.r, color.g, color.b, color.a);

        for (R_int i = 0; i < points_count - 1; i++) {
            R_gfx_vertex2f(points[i].x, points[i].y);
            R_gfx_vertex2f(points[i + 1].x, points[i + 1].y);
        }
        R_gfx_end();
    }
}

// Draw a color-filled circle
R_public void R_draw_circle(int center_x, int center_y, float radius, R_color color) {
    R_draw_circle_sector((R_vec2){center_x, center_y}, radius, 0, 360, 36, color);
}

// Draw a color-filled circle (Vector version)
R_public void R_draw_circle_v(R_vec2 center, float radius, R_color color) {
    R_draw_circle(center.x, center.y, radius, color);
}

// Draw a piece of a circle
R_public void R_draw_circle_sector(R_vec2 center, float radius, int start_angle, int end_angle,
                                   int segments, R_color color) {
    if (radius <= 0.0f) radius = 0.1f;// Avoid div by zero

    // Function expects (endAngle > start_angle)
    if (end_angle < start_angle) {
        // Swap values
        int tmp = start_angle;
        start_angle = end_angle;
        end_angle = tmp;
    }

    if (segments < 4) {
        // Calculate how many segments we need to draw a smooth circle, taken from https://stackoverflow.com/a/2244088
        float CIRCLE_ERROR_RATE = 0.5f;

        // Calculate the maximum angle between segments based on the error rate.
        float th = acosf(2 * powf(1 - CIRCLE_ERROR_RATE / radius, 2) - 1);
        segments = (end_angle - start_angle) * ceilf(2 * R_pi / th) / 360;

        if (segments <= 0) segments = 4;
    }

    float step_length = (float) (end_angle - start_angle) / (float) segments;
    float angle = start_angle;
    if (R_gfx_check_buffer_limit(3 * segments)) R_gfx_draw();

    R_gfx_begin(RF_TRIANGLES);
    for (R_int i = 0; i < segments; i++) {
        R_gfx_color4ub(color.r, color.g, color.b, color.a);

        R_gfx_vertex2f(center.x, center.y);
        R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * radius,
                       center.y + cosf(R_deg2rad * angle) * radius);
        R_gfx_vertex2f(center.x + sinf(R_deg2rad * (angle + step_length)) * radius,
                       center.y + cosf(R_deg2rad * (angle + step_length)) * radius);

        angle += step_length;
    }
    R_gfx_end();
}

R_public void R_draw_circle_sector_lines(R_vec2 center, float radius, int start_angle,
                                         int end_angle, int segments, R_color color) {
    if (radius <= 0.0f) radius = 0.1f;// Avoid div by zero issue

    // Function expects (endAngle > start_angle)
    if (end_angle < start_angle) {
        // Swap values
        int tmp = start_angle;
        start_angle = end_angle;
        end_angle = tmp;
    }

    if (segments < 4) {
        // Calculate how many segments we need to draw a smooth circle, taken from https://stackoverflow.com/a/2244088

        float CIRCLE_ERROR_RATE = 0.5f;

        // Calculate the maximum angle between segments based on the error rate.
        float th = acosf(2 * powf(1 - CIRCLE_ERROR_RATE / radius, 2) - 1);
        segments = (end_angle - start_angle) * ceilf(2 * R_pi / th) / 360;

        if (segments <= 0) segments = 4;
    }

    float step_length = (float) (end_angle - start_angle) / (float) segments;
    float angle = start_angle;

    // Hide the cap lines when the circle is full
    R_bool show_cap_lines = 1;
    int limit = 2 * (segments + 2);
    if ((end_angle - start_angle) % 360 == 0) {
        limit = 2 * segments;
        show_cap_lines = 0;
    }

    if (R_gfx_check_buffer_limit(limit)) R_gfx_draw();

    R_gfx_begin(RF_LINES);
    if (show_cap_lines) {
        R_gfx_color4ub(color.r, color.g, color.b, color.a);
        R_gfx_vertex2f(center.x, center.y);
        R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * radius,
                       center.y + cosf(R_deg2rad * angle) * radius);
    }

    for (R_int i = 0; i < segments; i++) {
        R_gfx_color4ub(color.r, color.g, color.b, color.a);

        R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * radius,
                       center.y + cosf(R_deg2rad * angle) * radius);
        R_gfx_vertex2f(center.x + sinf(R_deg2rad * (angle + step_length)) * radius,
                       center.y + cosf(R_deg2rad * (angle + step_length)) * radius);

        angle += step_length;
    }

    if (show_cap_lines) {
        R_gfx_color4ub(color.r, color.g, color.b, color.a);
        R_gfx_vertex2f(center.x, center.y);
        R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * radius,
                       center.y + cosf(R_deg2rad * angle) * radius);
    }
    R_gfx_end();
}

// Draw a gradient-filled circle
// NOTE: Gradient goes from center (color1) to border (color2)
R_public void R_draw_circle_gradient(int center_x, int center_y, float radius, R_color color1,
                                     R_color color2) {
    if (R_gfx_check_buffer_limit(3 * 36)) R_gfx_draw();

    R_gfx_begin(RF_TRIANGLES);
    for (R_int i = 0; i < 360; i += 10) {
        R_gfx_color4ub(color1.r, color1.g, color1.b, color1.a);
        R_gfx_vertex2f(center_x, center_y);
        R_gfx_color4ub(color2.r, color2.g, color2.b, color2.a);
        R_gfx_vertex2f(center_x + sinf(R_deg2rad * i) * radius,
                       center_y + cosf(R_deg2rad * i) * radius);
        R_gfx_color4ub(color2.r, color2.g, color2.b, color2.a);
        R_gfx_vertex2f(center_x + sinf(R_deg2rad * (i + 10)) * radius,
                       center_y + cosf(R_deg2rad * (i + 10)) * radius);
    }
    R_gfx_end();
}

// Draw circle outline
R_public void R_draw_circle_lines(int center_x, int center_y, float radius, R_color color) {
    if (R_gfx_check_buffer_limit(2 * 36)) R_gfx_draw();

    R_gfx_begin(RF_LINES);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);

    // NOTE: Circle outline is drawn pixel by pixel every degree (0 to 360)
    for (R_int i = 0; i < 360; i += 10) {
        R_gfx_vertex2f(center_x + sinf(R_deg2rad * i) * radius,
                       center_y + cosf(R_deg2rad * i) * radius);
        R_gfx_vertex2f(center_x + sinf(R_deg2rad * (i + 10)) * radius,
                       center_y + cosf(R_deg2rad * (i + 10)) * radius);
    }
    R_gfx_end();
}

R_public void R_draw_ring(R_vec2 center, float inner_radius, float outer_radius, int start_angle,
                          int end_angle, int segments, R_color color) {
    if (start_angle == end_angle) return;

    // Function expects (outerRadius > innerRadius)
    if (outer_radius < inner_radius) {
        float tmp = outer_radius;
        outer_radius = inner_radius;
        inner_radius = tmp;

        if (outer_radius <= 0.0f) outer_radius = 0.1f;
    }

    // Function expects (endAngle > start_angle)
    if (end_angle < start_angle) {
        // Swap values
        int tmp = start_angle;
        start_angle = end_angle;
        end_angle = tmp;
    }

    if (segments < 4) {
        // Calculate how many segments we need to draw a smooth circle, taken from https://stackoverflow.com/a/2244088

        float CIRCLE_ERROR_RATE = 0.5f;

        // Calculate the maximum angle between segments based on the error rate.
        float th = acosf(2 * powf(1 - CIRCLE_ERROR_RATE / outer_radius, 2) - 1);
        segments = (end_angle - start_angle) * ceilf(2 * R_pi / th) / 360;

        if (segments <= 0) segments = 4;
    }

    // Not a ring
    if (inner_radius <= 0.0f) {
        R_draw_circle_sector(center, outer_radius, start_angle, end_angle, segments, color);
        return;
    }

    float step_length = (float) (end_angle - start_angle) / (float) segments;
    float angle = start_angle;
    if (R_gfx_check_buffer_limit(6 * segments)) R_gfx_draw();

    R_gfx_begin(RF_TRIANGLES);
    for (R_int i = 0; i < segments; i++) {
        R_gfx_color4ub(color.r, color.g, color.b, color.a);

        R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * inner_radius,
                       center.y + cosf(R_deg2rad * angle) * inner_radius);
        R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * outer_radius,
                       center.y + cosf(R_deg2rad * angle) * outer_radius);
        R_gfx_vertex2f(center.x + sinf(R_deg2rad * (angle + step_length)) * inner_radius,
                       center.y + cosf(R_deg2rad * (angle + step_length)) * inner_radius);

        R_gfx_vertex2f(center.x + sinf(R_deg2rad * (angle + step_length)) * inner_radius,
                       center.y + cosf(R_deg2rad * (angle + step_length)) * inner_radius);
        R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * outer_radius,
                       center.y + cosf(R_deg2rad * angle) * outer_radius);
        R_gfx_vertex2f(center.x + sinf(R_deg2rad * (angle + step_length)) * outer_radius,
                       center.y + cosf(R_deg2rad * (angle + step_length)) * outer_radius);

        angle += step_length;
    }
    R_gfx_end();
}

R_public void R_draw_ring_lines(R_vec2 center, float inner_radius, float outer_radius,
                                int start_angle, int end_angle, int segments, R_color color) {
    if (start_angle == end_angle) return;

    // Function expects (outerRadius > innerRadius)
    if (outer_radius < inner_radius) {
        float tmp = outer_radius;
        outer_radius = inner_radius;
        inner_radius = tmp;

        if (outer_radius <= 0.0f) outer_radius = 0.1f;
    }

    // Function expects (endAngle > start_angle)
    if (end_angle < start_angle) {
        // Swap values
        int tmp = start_angle;
        start_angle = end_angle;
        end_angle = tmp;
    }

    if (segments < 4) {
        // Calculate how many segments we need to draw a smooth circle, taken from https://stackoverflow.com/a/2244088

        float CIRCLE_ERROR_RATE = 0.5f;

        // Calculate the maximum angle between segments based on the error rate.
        float th = acosf(2 * powf(1 - CIRCLE_ERROR_RATE / outer_radius, 2) - 1);
        segments = (end_angle - start_angle) * ceilf(2 * R_pi / th) / 360;

        if (segments <= 0) segments = 4;
    }

    if (inner_radius <= 0.0f) {
        R_draw_circle_sector_lines(center, outer_radius, start_angle, end_angle, segments, color);
        return;
    }

    float step_length = (float) (end_angle - start_angle) / (float) segments;
    float angle = start_angle;

    R_bool show_cap_lines = 1;
    int limit = 4 * (segments + 1);
    if ((end_angle - start_angle) % 360 == 0) {
        limit = 4 * segments;
        show_cap_lines = 0;
    }

    if (R_gfx_check_buffer_limit(limit)) R_gfx_draw();

    R_gfx_begin(RF_LINES);
    if (show_cap_lines) {
        R_gfx_color4ub(color.r, color.g, color.b, color.a);
        R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * outer_radius,
                       center.y + cosf(R_deg2rad * angle) * outer_radius);
        R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * inner_radius,
                       center.y + cosf(R_deg2rad * angle) * inner_radius);
    }

    for (R_int i = 0; i < segments; i++) {
        R_gfx_color4ub(color.r, color.g, color.b, color.a);

        R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * outer_radius,
                       center.y + cosf(R_deg2rad * angle) * outer_radius);
        R_gfx_vertex2f(center.x + sinf(R_deg2rad * (angle + step_length)) * outer_radius,
                       center.y + cosf(R_deg2rad * (angle + step_length)) * outer_radius);

        R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * inner_radius,
                       center.y + cosf(R_deg2rad * angle) * inner_radius);
        R_gfx_vertex2f(center.x + sinf(R_deg2rad * (angle + step_length)) * inner_radius,
                       center.y + cosf(R_deg2rad * (angle + step_length)) * inner_radius);

        angle += step_length;
    }

    if (show_cap_lines) {
        R_gfx_color4ub(color.r, color.g, color.b, color.a);
        R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * outer_radius,
                       center.y + cosf(R_deg2rad * angle) * outer_radius);
        R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * inner_radius,
                       center.y + cosf(R_deg2rad * angle) * inner_radius);
    }
    R_gfx_end();
}

// Draw a color-filled rectangle
R_public void R_draw_rectangle(int posX, int posY, int width, int height, R_color color) {
    R_draw_rectangle_v((R_vec2){(float) posX, (float) posY},
                       (R_vec2){(float) width, (float) height}, color);
}

// Draw a color-filled rectangle (Vector version)
R_public void R_draw_rectangle_v(R_vec2 position, R_vec2 size, R_color color) {
    R_draw_rectangle_pro((R_rec){position.x, position.y, size.x, size.y}, (R_vec2){0.0f, 0.0f},
                         0.0f, color);
}

// Draw a color-filled rectangle
R_public void R_draw_rectangle_rec(R_rec rec, R_color color) {
    R_draw_rectangle_pro(rec, (R_vec2){0.0f, 0.0f}, 0.0f, color);
}

// Draw a color-filled rectangle with pro parameters
R_public void R_draw_rectangle_pro(R_rec rec, R_vec2 origin, float rotation, R_color color) {
    R_gfx_enable_texture(R_get_shapes_texture().id);

    R_gfx_push_matrix();
    R_gfx_translatef(rec.x, rec.y, 0.0f);
    R_gfx_rotatef(rotation, 0.0f, 0.0f, 1.0f);
    R_gfx_translatef(-origin.x, -origin.y, 0.0f);

    R_gfx_begin(RF_QUADS);
    R_gfx_normal3f(0.0f, 0.0f, 1.0f);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);

    R_gfx_tex_coord2f(R_ctx.rec_tex_shapes.x / R_ctx.tex_shapes.width,
                      R_ctx.rec_tex_shapes.y / R_ctx.tex_shapes.height);
    R_gfx_vertex2f(0.0f, 0.0f);

    R_gfx_tex_coord2f(R_ctx.rec_tex_shapes.x / R_ctx.tex_shapes.width,
                      (R_ctx.rec_tex_shapes.y + R_ctx.rec_tex_shapes.height) /
                              R_ctx.tex_shapes.height);
    R_gfx_vertex2f(0.0f, rec.height);

    R_gfx_tex_coord2f(
            (R_ctx.rec_tex_shapes.x + R_ctx.rec_tex_shapes.width) / R_ctx.tex_shapes.width,
            (R_ctx.rec_tex_shapes.y + R_ctx.rec_tex_shapes.height) / R_ctx.tex_shapes.height);
    R_gfx_vertex2f(rec.width, rec.height);

    R_gfx_tex_coord2f((R_ctx.rec_tex_shapes.x + R_ctx.rec_tex_shapes.width) /
                              R_ctx.tex_shapes.width,
                      R_ctx.rec_tex_shapes.y / R_ctx.tex_shapes.height);
    R_gfx_vertex2f(rec.width, 0.0f);
    R_gfx_end();
    R_gfx_pop_matrix();

    R_gfx_disable_texture();
}

// Draw a vertical-gradient-filled rectangle
// NOTE: Gradient goes from bottom (color1) to top (color2)
R_public void R_draw_rectangle_gradient_v(int pos_x, int pos_y, int width, int height,
                                          R_color color1, R_color color2) {
    R_draw_rectangle_gradient((R_rec){(float) pos_x, (float) pos_y, (float) width, (float) height},
                              color1, color2, color2, color1);
}

// Draw a horizontal-gradient-filled rectangle
// NOTE: Gradient goes from bottom (color1) to top (color2)
R_public void R_draw_rectangle_gradient_h(int pos_x, int pos_y, int width, int height,
                                          R_color color1, R_color color2) {
    R_draw_rectangle_gradient((R_rec){(float) pos_x, (float) pos_y, (float) width, (float) height},
                              color1, color1, color2, color2);
}

// Draw a gradient-filled rectangle
// NOTE: Colors refer to corners, starting at top-lef corner and counter-clockwise
R_public void R_draw_rectangle_gradient(R_rec rec, R_color col1, R_color col2, R_color col3,
                                        R_color col4) {
    R_gfx_enable_texture(R_get_shapes_texture().id);

    R_gfx_push_matrix();
    R_gfx_begin(RF_QUADS);
    R_gfx_normal3f(0.0f, 0.0f, 1.0f);

    // NOTE: Default raylib font character 95 is a white square
    R_gfx_color4ub(col1.r, col1.g, col1.b, col1.a);
    R_gfx_tex_coord2f(R_ctx.rec_tex_shapes.x / R_ctx.tex_shapes.width,
                      R_ctx.rec_tex_shapes.y / R_ctx.tex_shapes.height);
    R_gfx_vertex2f(rec.x, rec.y);

    R_gfx_color4ub(col2.r, col2.g, col2.b, col2.a);
    R_gfx_tex_coord2f(R_ctx.rec_tex_shapes.x / R_ctx.tex_shapes.width,
                      (R_ctx.rec_tex_shapes.y + R_ctx.rec_tex_shapes.height) /
                              R_ctx.tex_shapes.height);
    R_gfx_vertex2f(rec.x, rec.y + rec.height);

    R_gfx_color4ub(col3.r, col3.g, col3.b, col3.a);
    R_gfx_tex_coord2f(
            (R_ctx.rec_tex_shapes.x + R_ctx.rec_tex_shapes.width) / R_ctx.tex_shapes.width,
            (R_ctx.rec_tex_shapes.y + R_ctx.rec_tex_shapes.height) / R_ctx.tex_shapes.height);
    R_gfx_vertex2f(rec.x + rec.width, rec.y + rec.height);

    R_gfx_color4ub(col4.r, col4.g, col4.b, col4.a);
    R_gfx_tex_coord2f((R_ctx.rec_tex_shapes.x + R_ctx.rec_tex_shapes.width) /
                              R_ctx.tex_shapes.width,
                      R_ctx.rec_tex_shapes.y / R_ctx.tex_shapes.height);
    R_gfx_vertex2f(rec.x + rec.width, rec.y);
    R_gfx_end();
    R_gfx_pop_matrix();

    R_gfx_disable_texture();
}

// Draw rectangle outline with extended parameters
R_public void R_draw_rectangle_outline(R_rec rec, int line_thick, R_color color) {
    if (line_thick > rec.width || line_thick > rec.height) {
        if (rec.width > rec.height) line_thick = (int) rec.height / 2;
        else if (rec.width < rec.height)
            line_thick = (int) rec.width / 2;
    }

    R_draw_rectangle_pro((R_rec){(int) rec.x, (int) rec.y, (int) rec.width, line_thick},
                         (R_vec2){0.0f, 0.0f}, 0.0f, color);
    R_draw_rectangle_pro((R_rec){(int) (rec.x - line_thick + rec.width), (int) (rec.y + line_thick),
                                 line_thick, (int) (rec.height - line_thick * 2.0f)},
                         (R_vec2){0.0f, 0.0f}, 0.0f, color);
    R_draw_rectangle_pro((R_rec){(int) rec.x, (int) (rec.y + rec.height - line_thick),
                                 (int) rec.width, line_thick},
                         (R_vec2){0.0f, 0.0f}, 0.0f, color);
    R_draw_rectangle_pro((R_rec){(int) rec.x, (int) (rec.y + line_thick), line_thick,
                                 (int) (rec.height - line_thick * 2)},
                         (R_vec2){0.0f, 0.0f}, 0.0f, color);
}

// Draw rectangle with rounded edges
R_public void R_draw_rectangle_rounded(R_rec rec, float roundness, int segments, R_color color) {
    // Not a rounded rectangle
    if ((roundness <= 0.0f) || (rec.width < 1) || (rec.height < 1)) {
        R_draw_rectangle_pro(rec, (R_vec2){0.0f, 0.0f}, 0.0f, color);
        return;
    }

    if (roundness >= 1.0f) roundness = 1.0f;

    // Calculate corner radius
    float radius =
            (rec.width > rec.height) ? (rec.height * roundness) / 2 : (rec.width * roundness) / 2;
    if (radius <= 0.0f) return;

    // Calculate number of segments to use for the corners
    if (segments < 4) {
        // Calculate how many segments we need to draw a smooth circle, taken from https://stackoverflow.com/a/2244088

        float CIRCLE_ERROR_RATE = 0.5f;

        // Calculate the maximum angle between segments based on the error rate.
        float th = acosf(2 * powf(1 - CIRCLE_ERROR_RATE / radius, 2) - 1);
        segments = ceilf(2 * R_pi / th) / 4;
        if (segments <= 0) segments = 4;
    }

    float step_length = 90.0f / (float) segments;

    /*  Quick sketch to make sense of all of this (there are 9 parts to draw, also mark the 12 points we'll use below)
     *  Not my best attempt at ASCII art, just preted it's rounded rectangle :)
     *     P0                    P1
     *       ____________________
     *     /|                    |\
     *    /1|          2         |3\
     *P7 /__|____________________|__\ P2
     *  /   |                    |  _\ P2
     *  |   |P8                P9|   |
     *  | 8 |          9         | 4 |
     *  | __|____________________|__ |
     *P6 \  |P11              P10|  / P3
     *    \7|          6         |5/
     *     \|____________________|/
     *     P5                    P4
     */

    const R_vec2 point[12] = {
            // coordinates of the 12 points that define the rounded rect (the idea here is to make things easier)
            {(float) rec.x + radius, rec.y},
            {(float) (rec.x + rec.width) - radius, rec.y},
            {rec.x + rec.width, (float) rec.y + radius},// PO, P1, P2
            {rec.x + rec.width, (float) (rec.y + rec.height) - radius},
            {(float) (rec.x + rec.width) - radius, rec.y + rec.height},// P3, P4
            {(float) rec.x + radius, rec.y + rec.height},
            {rec.x, (float) (rec.y + rec.height) - radius},
            {rec.x, (float) rec.y + radius},// P5, P6, P7
            {(float) rec.x + radius, (float) rec.y + radius},
            {(float) (rec.x + rec.width) - radius, (float) rec.y + radius},// P8, P9
            {(float) (rec.x + rec.width) - radius, (float) (rec.y + rec.height) - radius},
            {(float) rec.x + radius, (float) (rec.y + rec.height) - radius}// P10, P11
    };

    const R_vec2 centers[4] = {point[8], point[9], point[10], point[11]};
    const float angles[4] = {180.0f, 90.0f, 0.0f, 270.0f};
    if (R_gfx_check_buffer_limit(12 * segments + 5 * 6))
        R_gfx_draw();// 4 corners with 3 vertices per segment + 5 rectangles with 6 vertices each

    R_gfx_begin(RF_TRIANGLES);
    // Draw all of the 4 corners: [1] Upper Left Corner, [3] Upper Right Corner, [5] Lower Right Corner, [7] Lower Left Corner
    for (R_int k = 0; k < 4; ++k)// Hope the compiler is smart enough to unroll this loop
    {
        float angle = angles[k];
        const R_vec2 center = centers[k];
        for (R_int i = 0; i < segments; i++) {
            R_gfx_color4ub(color.r, color.g, color.b, color.a);
            R_gfx_vertex2f(center.x, center.y);
            R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * radius,
                           center.y + cosf(R_deg2rad * angle) * radius);
            R_gfx_vertex2f(center.x + sinf(R_deg2rad * (angle + step_length)) * radius,
                           center.y + cosf(R_deg2rad * (angle + step_length)) * radius);
            angle += step_length;
        }
    }

    // [2] Upper R_rec
    R_gfx_color4ub(color.r, color.g, color.b, color.a);
    R_gfx_vertex2f(point[0].x, point[0].y);
    R_gfx_vertex2f(point[8].x, point[8].y);
    R_gfx_vertex2f(point[9].x, point[9].y);
    R_gfx_vertex2f(point[1].x, point[1].y);
    R_gfx_vertex2f(point[0].x, point[0].y);
    R_gfx_vertex2f(point[9].x, point[9].y);

    // [4] Right R_rec
    R_gfx_color4ub(color.r, color.g, color.b, color.a);
    R_gfx_vertex2f(point[9].x, point[9].y);
    R_gfx_vertex2f(point[10].x, point[10].y);
    R_gfx_vertex2f(point[3].x, point[3].y);
    R_gfx_vertex2f(point[2].x, point[2].y);
    R_gfx_vertex2f(point[9].x, point[9].y);
    R_gfx_vertex2f(point[3].x, point[3].y);

    // [6] Bottom R_rec
    R_gfx_color4ub(color.r, color.g, color.b, color.a);
    R_gfx_vertex2f(point[11].x, point[11].y);
    R_gfx_vertex2f(point[5].x, point[5].y);
    R_gfx_vertex2f(point[4].x, point[4].y);
    R_gfx_vertex2f(point[10].x, point[10].y);
    R_gfx_vertex2f(point[11].x, point[11].y);
    R_gfx_vertex2f(point[4].x, point[4].y);

    // [8] Left R_rec
    R_gfx_color4ub(color.r, color.g, color.b, color.a);
    R_gfx_vertex2f(point[7].x, point[7].y);
    R_gfx_vertex2f(point[6].x, point[6].y);
    R_gfx_vertex2f(point[11].x, point[11].y);
    R_gfx_vertex2f(point[8].x, point[8].y);
    R_gfx_vertex2f(point[7].x, point[7].y);
    R_gfx_vertex2f(point[11].x, point[11].y);

    // [9] Middle R_rec
    R_gfx_color4ub(color.r, color.g, color.b, color.a);
    R_gfx_vertex2f(point[8].x, point[8].y);
    R_gfx_vertex2f(point[11].x, point[11].y);
    R_gfx_vertex2f(point[10].x, point[10].y);
    R_gfx_vertex2f(point[9].x, point[9].y);
    R_gfx_vertex2f(point[8].x, point[8].y);
    R_gfx_vertex2f(point[10].x, point[10].y);
    R_gfx_end();
}

// Draw rectangle with rounded edges outline
R_public void R_draw_rectangle_rounded_lines(R_rec rec, float roundness, int segments,
                                             int line_thick, R_color color) {
    if (line_thick < 0) line_thick = 0;

    // Not a rounded rectangle
    if (roundness <= 0.0f) {
        R_draw_rectangle_outline((R_rec){rec.x - line_thick, rec.y - line_thick,
                                         rec.width + 2 * line_thick, rec.height + 2 * line_thick},
                                 line_thick, color);
        return;
    }

    if (roundness >= 1.0f) roundness = 1.0f;

    // Calculate corner radius
    float radius =
            (rec.width > rec.height) ? (rec.height * roundness) / 2 : (rec.width * roundness) / 2;
    if (radius <= 0.0f) return;

    // Calculate number of segments to use for the corners
    if (segments < 4) {
        // Calculate how many segments we need to draw a smooth circle, taken from https://stackoverflow.com/a/2244088

        float CIRCLE_ERROR_RATE = 0.5f;

        // Calculate the maximum angle between segments based on the error rate.
        float th = acosf(2 * powf(1 - CIRCLE_ERROR_RATE / radius, 2) - 1);
        segments = ceilf(2 * R_pi / th) / 2;
        if (segments <= 0) segments = 4;
    }

    float step_length = 90.0f / (float) segments;
    const float outer_radius = radius + (float) line_thick, inner_radius = radius;

    /*  Quick sketch to make sense of all of this (mark the 16 + 4(corner centers P16-19) points we'll use below)
     *  Not my best attempt at ASCII art, just preted it's rounded rectangle :)
     *     P0                     P1
     *        ====================
     *     // P8                P9 \
     *    //                        \
     *P7 // P15                  P10 \\ P2
        \\ P2
     *  ||   *P16             P17*    ||
     *  ||                            ||
     *  || P14                   P11  ||
     *P6 \\  *P19             P18*   // P3
     *    \\                        //
     *     \\ P13              P12 //
     *        ====================
     *     P5                     P4

     */
    const R_vec2 point[16] = {
            {(float) rec.x + inner_radius, rec.y - line_thick},
            {(float) (rec.x + rec.width) - inner_radius, rec.y - line_thick},
            {rec.x + rec.width + line_thick, (float) rec.y + inner_radius},// PO, P1, P2
            {rec.x + rec.width + line_thick, (float) (rec.y + rec.height) - inner_radius},
            {(float) (rec.x + rec.width) - inner_radius, rec.y + rec.height + line_thick},// P3, P4
            {(float) rec.x + inner_radius, rec.y + rec.height + line_thick},
            {rec.x - line_thick, (float) (rec.y + rec.height) - inner_radius},
            {rec.x - line_thick, (float) rec.y + inner_radius},// P5, P6, P7
            {(float) rec.x + inner_radius, rec.y},
            {(float) (rec.x + rec.width) - inner_radius, rec.y},// P8, P9
            {rec.x + rec.width, (float) rec.y + inner_radius},
            {rec.x + rec.width, (float) (rec.y + rec.height) - inner_radius},// P10, P11
            {(float) (rec.x + rec.width) - inner_radius, rec.y + rec.height},
            {(float) rec.x + inner_radius, rec.y + rec.height},// P12, P13
            {rec.x, (float) (rec.y + rec.height) - inner_radius},
            {rec.x, (float) rec.y + inner_radius}// P14, P15
    };

    const R_vec2 centers[4] = {
            {(float) rec.x + inner_radius, (float) rec.y + inner_radius},
            {(float) (rec.x + rec.width) - inner_radius, (float) rec.y + inner_radius},// P16, P17
            {(float) (rec.x + rec.width) - inner_radius,
             (float) (rec.y + rec.height) - inner_radius},
            {(float) rec.x + inner_radius, (float) (rec.y + rec.height) - inner_radius}// P18, P19
    };

    const float angles[4] = {180.0f, 90.0f, 0.0f, 270.0f};

    if (line_thick > 1) {
        if (R_gfx_check_buffer_limit(4 * 6 * segments + 4 * 6))
            R_gfx_draw();// 4 corners with 6(2 * 3) vertices for each segment + 4 rectangles with 6 vertices each

        R_gfx_begin(RF_TRIANGLES);

        // Draw all of the 4 corners first: Upper Left Corner, Upper Right Corner, Lower Right Corner, Lower Left Corner
        for (R_int k = 0; k < 4; ++k)// Hope the compiler is smart enough to unroll this loop
        {
            float angle = angles[k];
            const R_vec2 center = centers[k];

            for (R_int i = 0; i < segments; i++) {
                R_gfx_color4ub(color.r, color.g, color.b, color.a);

                R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * inner_radius,
                               center.y + cosf(R_deg2rad * angle) * inner_radius);
                R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * outer_radius,
                               center.y + cosf(R_deg2rad * angle) * outer_radius);
                R_gfx_vertex2f(center.x + sinf(R_deg2rad * (angle + step_length)) * inner_radius,
                               center.y + cosf(R_deg2rad * (angle + step_length)) * inner_radius);

                R_gfx_vertex2f(center.x + sinf(R_deg2rad * (angle + step_length)) * inner_radius,
                               center.y + cosf(R_deg2rad * (angle + step_length)) * inner_radius);
                R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * outer_radius,
                               center.y + cosf(R_deg2rad * angle) * outer_radius);
                R_gfx_vertex2f(center.x + sinf(R_deg2rad * (angle + step_length)) * outer_radius,
                               center.y + cosf(R_deg2rad * (angle + step_length)) * outer_radius);

                angle += step_length;
            }
        }

        // Upper rectangle
        R_gfx_color4ub(color.r, color.g, color.b, color.a);
        R_gfx_vertex2f(point[0].x, point[0].y);
        R_gfx_vertex2f(point[8].x, point[8].y);
        R_gfx_vertex2f(point[9].x, point[9].y);
        R_gfx_vertex2f(point[1].x, point[1].y);
        R_gfx_vertex2f(point[0].x, point[0].y);
        R_gfx_vertex2f(point[9].x, point[9].y);

        // Right rectangle
        R_gfx_color4ub(color.r, color.g, color.b, color.a);
        R_gfx_vertex2f(point[10].x, point[10].y);
        R_gfx_vertex2f(point[11].x, point[11].y);
        R_gfx_vertex2f(point[3].x, point[3].y);
        R_gfx_vertex2f(point[2].x, point[2].y);
        R_gfx_vertex2f(point[10].x, point[10].y);
        R_gfx_vertex2f(point[3].x, point[3].y);

        // Lower rectangle
        R_gfx_color4ub(color.r, color.g, color.b, color.a);
        R_gfx_vertex2f(point[13].x, point[13].y);
        R_gfx_vertex2f(point[5].x, point[5].y);
        R_gfx_vertex2f(point[4].x, point[4].y);
        R_gfx_vertex2f(point[12].x, point[12].y);
        R_gfx_vertex2f(point[13].x, point[13].y);
        R_gfx_vertex2f(point[4].x, point[4].y);

        // Left rectangle
        R_gfx_color4ub(color.r, color.g, color.b, color.a);
        R_gfx_vertex2f(point[7].x, point[7].y);
        R_gfx_vertex2f(point[6].x, point[6].y);
        R_gfx_vertex2f(point[14].x, point[14].y);
        R_gfx_vertex2f(point[15].x, point[15].y);
        R_gfx_vertex2f(point[7].x, point[7].y);
        R_gfx_vertex2f(point[14].x, point[14].y);
        R_gfx_end();

    } else {
        // Use LINES to draw the outline
        if (R_gfx_check_buffer_limit(8 * segments + 4 * 2))
            R_gfx_draw();// 4 corners with 2 vertices for each segment + 4 rectangles with 2 vertices each

        R_gfx_begin(RF_LINES);

        // Draw all of the 4 corners first: Upper Left Corner, Upper Right Corner, Lower Right Corner, Lower Left Corner
        for (R_int k = 0; k < 4; ++k)// Hope the compiler is smart enough to unroll this loop
        {
            float angle = angles[k];
            const R_vec2 center = centers[k];

            for (R_int i = 0; i < segments; i++) {
                R_gfx_color4ub(color.r, color.g, color.b, color.a);
                R_gfx_vertex2f(center.x + sinf(R_deg2rad * angle) * outer_radius,
                               center.y + cosf(R_deg2rad * angle) * outer_radius);
                R_gfx_vertex2f(center.x + sinf(R_deg2rad * (angle + step_length)) * outer_radius,
                               center.y + cosf(R_deg2rad * (angle + step_length)) * outer_radius);
                angle += step_length;
            }
        }
        // And now the remaining 4 lines
        for (int i = 0; i < 8; i += 2) {
            R_gfx_color4ub(color.r, color.g, color.b, color.a);
            R_gfx_vertex2f(point[i].x, point[i].y);
            R_gfx_vertex2f(point[i + 1].x, point[i + 1].y);
        }
        R_gfx_end();
    }
}

// Draw a triangle
// NOTE: Vertex must be provided in counter-clockwise order
R_public void R_draw_triangle(R_vec2 v1, R_vec2 v2, R_vec2 v3, R_color color) {
    if (R_gfx_check_buffer_limit(4)) R_gfx_draw();
    R_gfx_begin(RF_TRIANGLES);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);
    R_gfx_vertex2f(v1.x, v1.y);
    R_gfx_vertex2f(v2.x, v2.y);
    R_gfx_vertex2f(v3.x, v3.y);
    R_gfx_end();
}

// Draw a triangle using lines
// NOTE: Vertex must be provided in counter-clockwise order
R_public void R_draw_triangle_lines(R_vec2 v1, R_vec2 v2, R_vec2 v3, R_color color) {
    if (R_gfx_check_buffer_limit(6)) R_gfx_draw();

    R_gfx_begin(RF_LINES);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);
    R_gfx_vertex2f(v1.x, v1.y);
    R_gfx_vertex2f(v2.x, v2.y);

    R_gfx_vertex2f(v2.x, v2.y);
    R_gfx_vertex2f(v3.x, v3.y);

    R_gfx_vertex2f(v3.x, v3.y);
    R_gfx_vertex2f(v1.x, v1.y);
    R_gfx_end();
}

// Draw a triangle fan defined by points
// NOTE: First vertex provided is the center, shared by all triangles
R_public void R_draw_triangle_fan(R_vec2 *points, int points_count, R_color color) {
    if (points_count >= 3) {
        if (R_gfx_check_buffer_limit((points_count - 2) * 4)) R_gfx_draw();

        R_gfx_enable_texture(R_get_shapes_texture().id);
        R_gfx_begin(RF_QUADS);
        R_gfx_color4ub(color.r, color.g, color.b, color.a);

        for (R_int i = 1; i < points_count - 1; i++) {
            R_gfx_tex_coord2f(R_ctx.rec_tex_shapes.x / R_ctx.tex_shapes.width,
                              R_ctx.rec_tex_shapes.y / R_ctx.tex_shapes.height);
            R_gfx_vertex2f(points[0].x, points[0].y);

            R_gfx_tex_coord2f(R_ctx.rec_tex_shapes.x / R_ctx.tex_shapes.width,
                              (R_ctx.rec_tex_shapes.y + R_ctx.rec_tex_shapes.height) /
                                      R_ctx.tex_shapes.height);
            R_gfx_vertex2f(points[i].x, points[i].y);

            R_gfx_tex_coord2f((R_ctx.rec_tex_shapes.x + R_ctx.rec_tex_shapes.width) /
                                      R_ctx.tex_shapes.width,
                              (R_ctx.rec_tex_shapes.y + R_ctx.rec_tex_shapes.height) /
                                      R_ctx.tex_shapes.height);
            R_gfx_vertex2f(points[i + 1].x, points[i + 1].y);

            R_gfx_tex_coord2f((R_ctx.rec_tex_shapes.x + R_ctx.rec_tex_shapes.width) /
                                      R_ctx.tex_shapes.width,
                              R_ctx.rec_tex_shapes.y / R_ctx.tex_shapes.height);
            R_gfx_vertex2f(points[i + 1].x, points[i + 1].y);
        }
        R_gfx_end();
        R_gfx_disable_texture();
    }
}

// Draw a triangle strip defined by points
// NOTE: Every new vertex connects with previous two
R_public void R_draw_triangle_strip(R_vec2 *points, int points_count, R_color color) {
    if (points_count >= 3) {
        if (R_gfx_check_buffer_limit(points_count)) R_gfx_draw();

        R_gfx_begin(RF_TRIANGLES);
        R_gfx_color4ub(color.r, color.g, color.b, color.a);

        for (R_int i = 2; i < points_count; i++) {
            if ((i % 2) == 0) {
                R_gfx_vertex2f(points[i].x, points[i].y);
                R_gfx_vertex2f(points[i - 2].x, points[i - 2].y);
                R_gfx_vertex2f(points[i - 1].x, points[i - 1].y);
            } else {
                R_gfx_vertex2f(points[i].x, points[i].y);
                R_gfx_vertex2f(points[i - 1].x, points[i - 1].y);
                R_gfx_vertex2f(points[i - 2].x, points[i - 2].y);
            }
        }
        R_gfx_end();
    }
}

// Draw a regular polygon of n sides (Vector version)
R_public void R_draw_poly(R_vec2 center, int sides, float radius, float rotation, R_color color) {
    if (sides < 3) sides = 3;
    float centralAngle = 0.0f;

    if (R_gfx_check_buffer_limit(4 * (360 / sides))) R_gfx_draw();

    R_gfx_push_matrix();
    R_gfx_translatef(center.x, center.y, 0.0f);
    R_gfx_rotatef(rotation, 0.0f, 0.0f, 1.0f);
    R_gfx_begin(RF_TRIANGLES);
    for (R_int i = 0; i < sides; i++) {
        R_gfx_color4ub(color.r, color.g, color.b, color.a);

        R_gfx_vertex2f(0, 0);
        R_gfx_vertex2f(sinf(R_deg2rad * centralAngle) * radius,
                       cosf(R_deg2rad * centralAngle) * radius);

        centralAngle += 360.0f / (float) sides;
        R_gfx_vertex2f(sinf(R_deg2rad * centralAngle) * radius,
                       cosf(R_deg2rad * centralAngle) * radius);
    }
    R_gfx_end();

    R_gfx_pop_matrix();
}

// Draw a R_texture2d with extended parameters
R_public void R_draw_texture(R_texture2d texture, int x, int y, R_color tint) {
    R_draw_texture_ex(texture, x, y, texture.width, texture.height, 0, tint);
}

// Draw a R_texture2d with extended parameters
R_public void R_draw_texture_ex(R_texture2d texture, int x, int y, int w, int h, float rotation,
                                R_color tint) {
    R_rec source_rec = {0.0f, 0.0f, (float) texture.width, (float) texture.height};
    R_rec dest_rec = {x, y, w, h};
    R_vec2 origin = {0.0f, 0.0f};

    R_draw_texture_region(texture, source_rec, dest_rec, origin, rotation, tint);
}

// Draw a part of a texture (defined by a rectangle) with 'pro' parameters. Note: origin is relative to destination rectangle size
R_public void R_draw_texture_region(R_texture2d texture, R_rec source_rec, R_rec dest_rec,
                                    R_vec2 origin, float rotation, R_color tint) {
    // Check if texture is valid
    if (texture.id > 0) {
        float width = (float) texture.width;
        float height = (float) texture.height;

        R_bool flip_x = 0;

        if (source_rec.width < 0) {
            flip_x = 1;
            source_rec.width *= -1;
        }
        if (source_rec.height < 0) source_rec.y -= source_rec.height;

        R_gfx_enable_texture(texture.id);

        R_gfx_push_matrix();
        R_gfx_translatef(dest_rec.x, dest_rec.y, 0.0f);
        R_gfx_rotatef(rotation, 0.0f, 0.0f, 1.0f);
        R_gfx_translatef(-origin.x, -origin.y, 0.0f);

        R_gfx_begin(RF_QUADS);
        R_gfx_color4ub(tint.r, tint.g, tint.b, tint.a);
        R_gfx_normal3f(0.0f, 0.0f, 1.0f);// Normal vector pointing towards viewer

        // Bottom-left corner for texture and quad
        if (flip_x)
            R_gfx_tex_coord2f((source_rec.x + source_rec.width) / width, source_rec.y / height);
        else
            R_gfx_tex_coord2f(source_rec.x / width, source_rec.y / height);
        R_gfx_vertex2f(0.0f, 0.0f);

        // Bottom-right corner for texture and quad
        if (flip_x)
            R_gfx_tex_coord2f((source_rec.x + source_rec.width) / width,
                              (source_rec.y + source_rec.height) / height);
        else
            R_gfx_tex_coord2f(source_rec.x / width, (source_rec.y + source_rec.height) / height);
        R_gfx_vertex2f(0.0f, dest_rec.height);

        // Top-right corner for texture and quad
        if (flip_x)
            R_gfx_tex_coord2f(source_rec.x / width, (source_rec.y + source_rec.height) / height);
        else
            R_gfx_tex_coord2f((source_rec.x + source_rec.width) / width,
                              (source_rec.y + source_rec.height) / height);
        R_gfx_vertex2f(dest_rec.width, dest_rec.height);

        // Top-left corner for texture and quad
        if (flip_x) R_gfx_tex_coord2f(source_rec.x / width, source_rec.y / height);
        else
            R_gfx_tex_coord2f((source_rec.x + source_rec.width) / width, source_rec.y / height);
        R_gfx_vertex2f(dest_rec.width, 0.0f);
        R_gfx_end();
        R_gfx_pop_matrix();

        R_gfx_disable_texture();
    }
}

// Draws a texture (or part of it) that stretches or shrinks nicely using n-patch info
R_public void R_draw_texture_npatch(R_texture2d texture, R_npatch_info n_patch_info, R_rec dest_rec,
                                    R_vec2 origin, float rotation, R_color tint) {
    if (texture.id > 0) {
        float width = (float) texture.width;
        float height = (float) texture.height;

        float patch_width = (dest_rec.width <= 0.0f) ? 0.0f : dest_rec.width;
        float patch_height = (dest_rec.height <= 0.0f) ? 0.0f : dest_rec.height;

        if (n_patch_info.source_rec.width < 0)
            n_patch_info.source_rec.x -= n_patch_info.source_rec.width;
        if (n_patch_info.source_rec.height < 0)
            n_patch_info.source_rec.y -= n_patch_info.source_rec.height;
        if (n_patch_info.type == RF_NPT_3PATCH_HORIZONTAL)
            patch_height = n_patch_info.source_rec.height;
        if (n_patch_info.type == RF_NPT_3PATCH_VERTICAL)
            patch_width = n_patch_info.source_rec.width;

        R_bool draw_center = 1;
        R_bool draw_middle = 1;
        float left_border = (float) n_patch_info.left;
        float top_border = (float) n_patch_info.top;
        float right_border = (float) n_patch_info.right;
        float bottom_border = (float) n_patch_info.bottom;

        // adjust the lateral (left and right) border widths in case patch_width < texture.width
        if (patch_width <= (left_border + right_border) &&
            n_patch_info.type != RF_NPT_3PATCH_VERTICAL) {
            draw_center = 0;
            left_border = (left_border / (left_border + right_border)) * patch_width;
            right_border = patch_width - left_border;
        }
        // adjust the lateral (top and bottom) border heights in case patch_height < texture.height
        if (patch_height <= (top_border + bottom_border) &&
            n_patch_info.type != RF_NPT_3PATCH_HORIZONTAL) {
            draw_middle = 0;
            top_border = (top_border / (top_border + bottom_border)) * patch_height;
            bottom_border = patch_height - top_border;
        }

        R_vec2 vert_a, vert_b, vert_c, vert_d;
        vert_a.x = 0.0f;                        // outer left
        vert_a.y = 0.0f;                        // outer top
        vert_b.x = left_border;                 // inner left
        vert_b.y = top_border;                  // inner top
        vert_c.x = patch_width - right_border;  // inner right
        vert_c.y = patch_height - bottom_border;// inner bottom
        vert_d.x = patch_width;                 // outer right
        vert_d.y = patch_height;                // outer bottom

        R_vec2 coord_a, coord_b, coord_c, coord_d;
        coord_a.x = n_patch_info.source_rec.x / width;
        coord_a.y = n_patch_info.source_rec.y / height;
        coord_b.x = (n_patch_info.source_rec.x + left_border) / width;
        coord_b.y = (n_patch_info.source_rec.y + top_border) / height;
        coord_c.x =
                (n_patch_info.source_rec.x + n_patch_info.source_rec.width - right_border) / width;
        coord_c.y = (n_patch_info.source_rec.y + n_patch_info.source_rec.height - bottom_border) /
                    height;
        coord_d.x = (n_patch_info.source_rec.x + n_patch_info.source_rec.width) / width;
        coord_d.y = (n_patch_info.source_rec.y + n_patch_info.source_rec.height) / height;

        R_gfx_enable_texture(texture.id);

        R_gfx_push_matrix();
        R_gfx_translatef(dest_rec.x, dest_rec.y, 0.0f);
        R_gfx_rotatef(rotation, 0.0f, 0.0f, 1.0f);
        R_gfx_translatef(-origin.x, -origin.y, 0.0f);

        R_gfx_begin(RF_QUADS);
        R_gfx_color4ub(tint.r, tint.g, tint.b, tint.a);
        R_gfx_normal3f(0.0f, 0.0f, 1.0f);// Normal vector pointing towards viewer

        if (n_patch_info.type == RF_NPT_9PATCH) {
            // ------------------------------------------------------------
            // TOP-LEFT QUAD
            R_gfx_tex_coord2f(coord_a.x, coord_b.y);
            R_gfx_vertex2f(vert_a.x, vert_b.y);// Bottom-left corner for texture and quad
            R_gfx_tex_coord2f(coord_b.x, coord_b.y);
            R_gfx_vertex2f(vert_b.x, vert_b.y);// Bottom-right corner for texture and quad
            R_gfx_tex_coord2f(coord_b.x, coord_a.y);
            R_gfx_vertex2f(vert_b.x, vert_a.y);// Top-right corner for texture and quad
            R_gfx_tex_coord2f(coord_a.x, coord_a.y);
            R_gfx_vertex2f(vert_a.x, vert_a.y);// Top-left corner for texture and quad
            if (draw_center) {
                // TOP-CENTER QUAD
                R_gfx_tex_coord2f(coord_b.x, coord_b.y);
                R_gfx_vertex2f(vert_b.x, vert_b.y);// Bottom-left corner for texture and quad
                R_gfx_tex_coord2f(coord_c.x, coord_b.y);
                R_gfx_vertex2f(vert_c.x, vert_b.y);// Bottom-right corner for texture and quad
                R_gfx_tex_coord2f(coord_c.x, coord_a.y);
                R_gfx_vertex2f(vert_c.x, vert_a.y);// Top-right corner for texture and quad
                R_gfx_tex_coord2f(coord_b.x, coord_a.y);
                R_gfx_vertex2f(vert_b.x, vert_a.y);// Top-left corner for texture and quad
            }
            // TOP-RIGHT QUAD
            R_gfx_tex_coord2f(coord_c.x, coord_b.y);
            R_gfx_vertex2f(vert_c.x, vert_b.y);// Bottom-left corner for texture and quad
            R_gfx_tex_coord2f(coord_d.x, coord_b.y);
            R_gfx_vertex2f(vert_d.x, vert_b.y);// Bottom-right corner for texture and quad
            R_gfx_tex_coord2f(coord_d.x, coord_a.y);
            R_gfx_vertex2f(vert_d.x, vert_a.y);// Top-right corner for texture and quad
            R_gfx_tex_coord2f(coord_c.x, coord_a.y);
            R_gfx_vertex2f(vert_c.x, vert_a.y);// Top-left corner for texture and quad
            if (draw_middle) {
                // ------------------------------------------------------------
                // MIDDLE-LEFT QUAD
                R_gfx_tex_coord2f(coord_a.x, coord_c.y);
                R_gfx_vertex2f(vert_a.x, vert_c.y);// Bottom-left corner for texture and quad
                R_gfx_tex_coord2f(coord_b.x, coord_c.y);
                R_gfx_vertex2f(vert_b.x, vert_c.y);// Bottom-right corner for texture and quad
                R_gfx_tex_coord2f(coord_b.x, coord_b.y);
                R_gfx_vertex2f(vert_b.x, vert_b.y);// Top-right corner for texture and quad
                R_gfx_tex_coord2f(coord_a.x, coord_b.y);
                R_gfx_vertex2f(vert_a.x, vert_b.y);// Top-left corner for texture and quad
                if (draw_center) {
                    // MIDDLE-CENTER QUAD
                    R_gfx_tex_coord2f(coord_b.x, coord_c.y);
                    R_gfx_vertex2f(vert_b.x, vert_c.y);// Bottom-left corner for texture and quad
                    R_gfx_tex_coord2f(coord_c.x, coord_c.y);
                    R_gfx_vertex2f(vert_c.x, vert_c.y);// Bottom-right corner for texture and quad
                    R_gfx_tex_coord2f(coord_c.x, coord_b.y);
                    R_gfx_vertex2f(vert_c.x, vert_b.y);// Top-right corner for texture and quad
                    R_gfx_tex_coord2f(coord_b.x, coord_b.y);
                    R_gfx_vertex2f(vert_b.x, vert_b.y);// Top-left corner for texture and quad
                }

                // MIDDLE-RIGHT QUAD
                R_gfx_tex_coord2f(coord_c.x, coord_c.y);
                R_gfx_vertex2f(vert_c.x, vert_c.y);// Bottom-left corner for texture and quad
                R_gfx_tex_coord2f(coord_d.x, coord_c.y);
                R_gfx_vertex2f(vert_d.x, vert_c.y);// Bottom-right corner for texture and quad
                R_gfx_tex_coord2f(coord_d.x, coord_b.y);
                R_gfx_vertex2f(vert_d.x, vert_b.y);// Top-right corner for texture and quad
                R_gfx_tex_coord2f(coord_c.x, coord_b.y);
                R_gfx_vertex2f(vert_c.x, vert_b.y);// Top-left corner for texture and quad
            }

            // ------------------------------------------------------------
            // BOTTOM-LEFT QUAD
            R_gfx_tex_coord2f(coord_a.x, coord_d.y);
            R_gfx_vertex2f(vert_a.x, vert_d.y);// Bottom-left corner for texture and quad
            R_gfx_tex_coord2f(coord_b.x, coord_d.y);
            R_gfx_vertex2f(vert_b.x, vert_d.y);// Bottom-right corner for texture and quad
            R_gfx_tex_coord2f(coord_b.x, coord_c.y);
            R_gfx_vertex2f(vert_b.x, vert_c.y);// Top-right corner for texture and quad
            R_gfx_tex_coord2f(coord_a.x, coord_c.y);
            R_gfx_vertex2f(vert_a.x, vert_c.y);// Top-left corner for texture and quad
            if (draw_center) {
                // BOTTOM-CENTER QUAD
                R_gfx_tex_coord2f(coord_b.x, coord_d.y);
                R_gfx_vertex2f(vert_b.x, vert_d.y);// Bottom-left corner for texture and quad
                R_gfx_tex_coord2f(coord_c.x, coord_d.y);
                R_gfx_vertex2f(vert_c.x, vert_d.y);// Bottom-right corner for texture and quad
                R_gfx_tex_coord2f(coord_c.x, coord_c.y);
                R_gfx_vertex2f(vert_c.x, vert_c.y);// Top-right corner for texture and quad
                R_gfx_tex_coord2f(coord_b.x, coord_c.y);
                R_gfx_vertex2f(vert_b.x, vert_c.y);// Top-left corner for texture and quad
            }

            // BOTTOM-RIGHT QUAD
            R_gfx_tex_coord2f(coord_c.x, coord_d.y);
            R_gfx_vertex2f(vert_c.x, vert_d.y);// Bottom-left corner for texture and quad
            R_gfx_tex_coord2f(coord_d.x, coord_d.y);
            R_gfx_vertex2f(vert_d.x, vert_d.y);// Bottom-right corner for texture and quad
            R_gfx_tex_coord2f(coord_d.x, coord_c.y);
            R_gfx_vertex2f(vert_d.x, vert_c.y);// Top-right corner for texture and quad
            R_gfx_tex_coord2f(coord_c.x, coord_c.y);
            R_gfx_vertex2f(vert_c.x, vert_c.y);// Top-left corner for texture and quad
        } else if (n_patch_info.type == RF_NPT_3PATCH_VERTICAL) {
            // TOP QUAD
            // -----------------------------------------------------------
            // R_texture coords                 Vertices
            R_gfx_tex_coord2f(coord_a.x, coord_b.y);
            R_gfx_vertex2f(vert_a.x, vert_b.y);// Bottom-left corner for texture and quad
            R_gfx_tex_coord2f(coord_d.x, coord_b.y);
            R_gfx_vertex2f(vert_d.x, vert_b.y);// Bottom-right corner for texture and quad
            R_gfx_tex_coord2f(coord_d.x, coord_a.y);
            R_gfx_vertex2f(vert_d.x, vert_a.y);// Top-right corner for texture and quad
            R_gfx_tex_coord2f(coord_a.x, coord_a.y);
            R_gfx_vertex2f(vert_a.x, vert_a.y);// Top-left corner for texture and quad
            if (draw_center) {
                // MIDDLE QUAD
                // -----------------------------------------------------------
                // R_texture coords                 Vertices
                R_gfx_tex_coord2f(coord_a.x, coord_c.y);
                R_gfx_vertex2f(vert_a.x, vert_c.y);// Bottom-left corner for texture and quad
                R_gfx_tex_coord2f(coord_d.x, coord_c.y);
                R_gfx_vertex2f(vert_d.x, vert_c.y);// Bottom-right corner for texture and quad
                R_gfx_tex_coord2f(coord_d.x, coord_b.y);
                R_gfx_vertex2f(vert_d.x, vert_b.y);// Top-right corner for texture and quad
                R_gfx_tex_coord2f(coord_a.x, coord_b.y);
                R_gfx_vertex2f(vert_a.x, vert_b.y);// Top-left corner for texture and quad
            }
            // BOTTOM QUAD
            // -----------------------------------------------------------
            // R_texture coords                 Vertices
            R_gfx_tex_coord2f(coord_a.x, coord_d.y);
            R_gfx_vertex2f(vert_a.x, vert_d.y);// Bottom-left corner for texture and quad
            R_gfx_tex_coord2f(coord_d.x, coord_d.y);
            R_gfx_vertex2f(vert_d.x, vert_d.y);// Bottom-right corner for texture and quad
            R_gfx_tex_coord2f(coord_d.x, coord_c.y);
            R_gfx_vertex2f(vert_d.x, vert_c.y);// Top-right corner for texture and quad
            R_gfx_tex_coord2f(coord_a.x, coord_c.y);
            R_gfx_vertex2f(vert_a.x, vert_c.y);// Top-left corner for texture and quad
        } else if (n_patch_info.type == RF_NPT_3PATCH_HORIZONTAL) {
            // LEFT QUAD
            // -----------------------------------------------------------
            // R_texture coords                 Vertices
            R_gfx_tex_coord2f(coord_a.x, coord_d.y);
            R_gfx_vertex2f(vert_a.x, vert_d.y);// Bottom-left corner for texture and quad
            R_gfx_tex_coord2f(coord_b.x, coord_d.y);
            R_gfx_vertex2f(vert_b.x, vert_d.y);// Bottom-right corner for texture and quad
            R_gfx_tex_coord2f(coord_b.x, coord_a.y);
            R_gfx_vertex2f(vert_b.x, vert_a.y);// Top-right corner for texture and quad
            R_gfx_tex_coord2f(coord_a.x, coord_a.y);
            R_gfx_vertex2f(vert_a.x, vert_a.y);// Top-left corner for texture and quad
            if (draw_center) {
                // CENTER QUAD
                // -----------------------------------------------------------
                // R_texture coords                 Vertices
                R_gfx_tex_coord2f(coord_b.x, coord_d.y);
                R_gfx_vertex2f(vert_b.x, vert_d.y);// Bottom-left corner for texture and quad
                R_gfx_tex_coord2f(coord_c.x, coord_d.y);
                R_gfx_vertex2f(vert_c.x, vert_d.y);// Bottom-right corner for texture and quad
                R_gfx_tex_coord2f(coord_c.x, coord_a.y);
                R_gfx_vertex2f(vert_c.x, vert_a.y);// Top-right corner for texture and quad
                R_gfx_tex_coord2f(coord_b.x, coord_a.y);
                R_gfx_vertex2f(vert_b.x, vert_a.y);// Top-left corner for texture and quad
            }
            // RIGHT QUAD
            // -----------------------------------------------------------
            // R_texture coords                 Vertices
            R_gfx_tex_coord2f(coord_c.x, coord_d.y);
            R_gfx_vertex2f(vert_c.x, vert_d.y);// Bottom-left corner for texture and quad
            R_gfx_tex_coord2f(coord_d.x, coord_d.y);
            R_gfx_vertex2f(vert_d.x, vert_d.y);// Bottom-right corner for texture and quad
            R_gfx_tex_coord2f(coord_d.x, coord_a.y);
            R_gfx_vertex2f(vert_d.x, vert_a.y);// Top-right corner for texture and quad
            R_gfx_tex_coord2f(coord_c.x, coord_a.y);
            R_gfx_vertex2f(vert_c.x, vert_a.y);// Top-left corner for texture and quad
        }
        R_gfx_end();
        R_gfx_pop_matrix();

        R_gfx_disable_texture();
    }
}

// Draw text (using default font)
R_public void R_draw_string(const char *text, int text_len, int posX, int posY, int fontSize,
                            R_color color) {
    // Check if default font has been loaded
    if (R_get_default_font().texture.id == 0 || text_len == 0) return;

    R_vec2 position = {(float) posX, (float) posY};

    int defaultFontSize = 10;// Default Font chars height in pixel
    if (fontSize < defaultFontSize) fontSize = defaultFontSize;
    int spacing = fontSize / defaultFontSize;

    R_draw_string_ex(R_get_default_font(), text, text_len, position, (float) fontSize,
                     (float) spacing, color);
}

// Draw text with custom font
R_public void R_draw_string_ex(R_font font, const char *text, int text_len, R_vec2 position,
                               float font_size, float spacing, R_color tint) {
    int text_offset_y = 0;     // Required for line break!
    float text_offset_x = 0.0f;// Offset between characters
    float scale_factor = 0.0f;

    int letter = 0;// Current character
    int index = 0; // Index position in sprite font

    scale_factor = font_size / font.base_size;

    for (R_int i = 0; i < text_len; i++) {
        R_decoded_rune decoded_rune = R_decode_utf8_char(&text[i], text_len - i);
        letter = decoded_rune.codepoint;
        index = R_get_glyph_index(font, letter);

        // NOTE: Normally we exit the decoding sequence as soon as a bad unsigned char is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol so to not skip any we set 'next = 1'
        if (letter == 0x3f) decoded_rune.bytes_processed = 1;
        i += (decoded_rune.bytes_processed - 1);

        if (letter == '\n') {
            // NOTE: Fixed line spacing of 1.5 lines
            text_offset_y += (int) ((font.base_size + font.base_size / 2) * scale_factor);
            text_offset_x = 0.0f;
        } else {
            if (letter != ' ') {
                R_rec src_rec = font.glyphs[index].rec;
                R_rec dst_rec = {
                        position.x + text_offset_x + font.glyphs[index].offset_x * scale_factor,
                        position.y + text_offset_y + font.glyphs[index].offset_y * scale_factor,
                        font.glyphs[index].rec.width * scale_factor,
                        font.glyphs[index].rec.height * scale_factor};
                R_draw_texture_region(font.texture, src_rec, dst_rec, (R_vec2){0}, 0.0f, tint);
            }

            if (font.glyphs[index].advance_x == 0)
                text_offset_x += ((float) font.glyphs[index].rec.width * scale_factor + spacing);
            else
                text_offset_x += ((float) font.glyphs[index].advance_x * scale_factor + spacing);
        }
    }
}

// Draw text wrapped
R_public void R_draw_string_wrap(R_font font, const char *text, int text_len, R_vec2 position,
                                 float font_size, float spacing, R_color tint, float wrap_width,
                                 R_text_wrap_mode mode) {
    R_rec rec = {0, 0, wrap_width, FLT_MAX};
    R_draw_string_rec(font, text, text_len, rec, font_size, spacing, mode, tint);
}

// Draw text using font inside rectangle limits
R_public void R_draw_string_rec(R_font font, const char *text, int text_len, R_rec rec,
                                float font_size, float spacing, R_text_wrap_mode wrap,
                                R_color tint) {
    int text_offset_x = 0;// Offset between characters
    int text_offset_y = 0;// Required for line break!
    float scale_factor = 0.0f;

    int letter = 0;// Current character
    int index = 0; // Index position in sprite font

    scale_factor = font_size / font.base_size;

    enum {
        MEASURE_WORD_WRAP_STATE = 0,
        MEASURE_RESULT_STATE = 1
    };

    int state = wrap == RF_WORD_WRAP ? MEASURE_WORD_WRAP_STATE : MEASURE_RESULT_STATE;
    int start_line = -1;// Index where to begin drawing (where a line begins)
    int end_line = -1;  // Index where to stop drawing (where a line ends)
    int lastk = -1;     // Holds last value of the character position

    for (R_int i = 0, k = 0; i < text_len; i++, k++) {
        int glyph_width = 0;

        R_decoded_rune decoded_rune = R_decode_utf8_char(&text[i], text_len - i);
        letter = decoded_rune.codepoint;
        index = R_get_glyph_index(font, letter);

        // NOTE: normally we exit the decoding sequence as soon as a bad unsigned char is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol so to not skip any we set next = 1
        if (letter == 0x3f) decoded_rune.bytes_processed = 1;
        i += decoded_rune.bytes_processed - 1;

        if (letter != '\n') {
            glyph_width = (font.glyphs[index].advance_x == 0)
                                  ? (int) (font.glyphs[index].rec.width * scale_factor + spacing)
                                  : (int) (font.glyphs[index].advance_x * scale_factor + spacing);
        }

        // NOTE: When wrap is ON we first measure how much of the text we can draw before going outside of the rec container
        // We store this info in start_line and end_line, then we change states, draw the text between those two variables
        // and change states again and again recursively until the end of the text (or until we get outside of the container).
        // When wrap is OFF we don't need the measure state so we go to the drawing state immediately
        // and begin drawing on the next line before we can get outside the container.
        if (state == MEASURE_WORD_WRAP_STATE) {
            // TODO: there are multiple types of spaces in UNICODE, maybe it's a good idea to add support for more
            // See: http://jkorpela.fi/chars/spaces.html
            if ((letter == ' ') || (letter == '\t') || (letter == '\n')) end_line = i;

            if ((text_offset_x + glyph_width + 1) >= rec.width) {
                end_line = (end_line < 1) ? i : end_line;
                if (i == end_line) end_line -= decoded_rune.bytes_processed;
                if ((start_line + decoded_rune.bytes_processed) == end_line)
                    end_line = i - decoded_rune.bytes_processed;
                state = !state;
            } else if ((i + 1) == text_len) {
                end_line = i;
                state = !state;
            } else if (letter == '\n') {
                state = !state;
            }

            if (state == MEASURE_RESULT_STATE) {
                text_offset_x = 0;
                i = start_line;
                glyph_width = 0;

                // Save character position when we switch states
                int tmp = lastk;
                lastk = k - 1;
                k = tmp;
            }
        } else {
            if (letter == '\n') {
                if (!wrap) {
                    text_offset_y += (int) ((font.base_size + font.base_size / 2) * scale_factor);
                    text_offset_x = 0;
                }
            } else {
                if (!wrap && ((text_offset_x + glyph_width + 1) >= rec.width)) {
                    text_offset_y += (int) ((font.base_size + font.base_size / 2) * scale_factor);
                    text_offset_x = 0;
                }

                if ((text_offset_y + (int) (font.base_size * scale_factor)) > rec.height) break;

                // Draw glyph
                if ((letter != ' ') && (letter != '\t')) {
                    R_draw_texture_region(
                            font.texture, font.glyphs[index].rec,
                            (R_rec){rec.x + text_offset_x +
                                            font.glyphs[index].offset_x * scale_factor,
                                    rec.y + text_offset_y +
                                            font.glyphs[index].offset_y * scale_factor,
                                    font.glyphs[index].rec.width * scale_factor,
                                    font.glyphs[index].rec.height * scale_factor},
                            (R_vec2){0, 0}, 0.0f, tint);
                }
            }

            if (wrap && (i == end_line)) {
                text_offset_y += (int) ((font.base_size + font.base_size / 2) * scale_factor);
                text_offset_x = 0;
                start_line = end_line;
                end_line = -1;
                glyph_width = 0;
                k = lastk;
                state = !state;
            }
        }

        text_offset_x += glyph_width;
    }
}

R_public void R_draw_text(const char *text, int posX, int posY, int font_size, R_color color) {
    R_draw_string(text, strlen(text), posX, posY, font_size, color);
}

R_public void R_draw_text_ex(R_font font, const char *text, R_vec2 position, float fontSize,
                             float spacing, R_color tint) {
    R_draw_string_ex(font, text, strlen(text), position, fontSize, spacing, tint);
}

R_public void R_draw_text_wrap(R_font font, const char *text, R_vec2 position, float font_size,
                               float spacing, R_color tint, float wrap_width,
                               R_text_wrap_mode mode) {
    R_draw_string_wrap(font, text, strlen(text), position, font_size, spacing, tint, wrap_width,
                       mode);
}

R_public void R_draw_text_rec(R_font font, const char *text, R_rec rec, float font_size,
                              float spacing, R_text_wrap_mode wrap, R_color tint) {
    R_draw_string_rec(font, text, strlen(text), rec, font_size, spacing, wrap, tint);
}

R_public void R_draw_line3d(R_vec3 start_pos, R_vec3 end_pos, R_color color) {
    R_gfx_begin(RF_LINES);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);
    R_gfx_vertex3f(start_pos.x, start_pos.y, start_pos.z);
    R_gfx_vertex3f(end_pos.x, end_pos.y, end_pos.z);
    R_gfx_end();
}

// Draw a circle in 3D world space
R_public void R_draw_circle3d(R_vec3 center, float radius, R_vec3 rotation_axis,
                              float rotationAngle, R_color color) {
    if (R_gfx_check_buffer_limit(2 * 36)) R_gfx_draw();

    R_gfx_push_matrix();
    R_gfx_translatef(center.x, center.y, center.z);
    R_gfx_rotatef(rotationAngle, rotation_axis.x, rotation_axis.y, rotation_axis.z);

    R_gfx_begin(RF_LINES);
    for (R_int i = 0; i < 360; i += 10) {
        R_gfx_color4ub(color.r, color.g, color.b, color.a);

        R_gfx_vertex3f(sinf(R_deg2rad * i) * radius, cosf(R_deg2rad * i) * radius, 0.0f);
        R_gfx_vertex3f(sinf(R_deg2rad * (i + 10)) * radius, cosf(R_deg2rad * (i + 10)) * radius,
                       0.0f);
    }
    R_gfx_end();
    R_gfx_pop_matrix();
}

// Draw cube
// NOTE: Cube position is the center position
R_public void R_draw_cube(R_vec3 position, float width, float height, float length, R_color color) {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    if (R_gfx_check_buffer_limit(36)) R_gfx_draw();

    R_gfx_push_matrix();
    // NOTE: Transformation is applied in inverse order (scale -> rotate -> translate)
    R_gfx_translatef(position.x, position.y, position.z);
    //R_gfx_rotatef(45, 0, 1, 0);
    //R_gfx_scalef(1.0f, 1.0f, 1.0f);   // NOTE: Vertices are directly scaled on definition

    R_gfx_begin(RF_TRIANGLES);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);

    // Front face
    R_gfx_vertex3f(x - width / 2, y - height / 2, z + length / 2);// Bottom Left
    R_gfx_vertex3f(x + width / 2, y - height / 2, z + length / 2);// Bottom Right
    R_gfx_vertex3f(x - width / 2, y + height / 2, z + length / 2);// Top Left

    R_gfx_vertex3f(x + width / 2, y + height / 2, z + length / 2);// Top Right
    R_gfx_vertex3f(x - width / 2, y + height / 2, z + length / 2);// Top Left
    R_gfx_vertex3f(x + width / 2, y - height / 2, z + length / 2);// Bottom Right

    // Back face
    R_gfx_vertex3f(x - width / 2, y - height / 2, z - length / 2);// Bottom Left
    R_gfx_vertex3f(x - width / 2, y + height / 2, z - length / 2);// Top Left
    R_gfx_vertex3f(x + width / 2, y - height / 2, z - length / 2);// Bottom Right

    R_gfx_vertex3f(x + width / 2, y + height / 2, z - length / 2);// Top Right
    R_gfx_vertex3f(x + width / 2, y - height / 2, z - length / 2);// Bottom Right
    R_gfx_vertex3f(x - width / 2, y + height / 2, z - length / 2);// Top Left

    // Top face
    R_gfx_vertex3f(x - width / 2, y + height / 2, z - length / 2);// Top Left
    R_gfx_vertex3f(x - width / 2, y + height / 2, z + length / 2);// Bottom Left
    R_gfx_vertex3f(x + width / 2, y + height / 2, z + length / 2);// Bottom Right

    R_gfx_vertex3f(x + width / 2, y + height / 2, z - length / 2);// Top Right
    R_gfx_vertex3f(x - width / 2, y + height / 2, z - length / 2);// Top Left
    R_gfx_vertex3f(x + width / 2, y + height / 2, z + length / 2);// Bottom Right

    // Bottom face
    R_gfx_vertex3f(x - width / 2, y - height / 2, z - length / 2);// Top Left
    R_gfx_vertex3f(x + width / 2, y - height / 2, z + length / 2);// Bottom Right
    R_gfx_vertex3f(x - width / 2, y - height / 2, z + length / 2);// Bottom Left

    R_gfx_vertex3f(x + width / 2, y - height / 2, z - length / 2);// Top Right
    R_gfx_vertex3f(x + width / 2, y - height / 2, z + length / 2);// Bottom Right
    R_gfx_vertex3f(x - width / 2, y - height / 2, z - length / 2);// Top Left

    // Right face
    R_gfx_vertex3f(x + width / 2, y - height / 2, z - length / 2);// Bottom Right
    R_gfx_vertex3f(x + width / 2, y + height / 2, z - length / 2);// Top Right
    R_gfx_vertex3f(x + width / 2, y + height / 2, z + length / 2);// Top Left

    R_gfx_vertex3f(x + width / 2, y - height / 2, z + length / 2);// Bottom Left
    R_gfx_vertex3f(x + width / 2, y - height / 2, z - length / 2);// Bottom Right
    R_gfx_vertex3f(x + width / 2, y + height / 2, z + length / 2);// Top Left

    // Left face
    R_gfx_vertex3f(x - width / 2, y - height / 2, z - length / 2);// Bottom Right
    R_gfx_vertex3f(x - width / 2, y + height / 2, z + length / 2);// Top Left
    R_gfx_vertex3f(x - width / 2, y + height / 2, z - length / 2);// Top Right

    R_gfx_vertex3f(x - width / 2, y - height / 2, z + length / 2);// Bottom Left
    R_gfx_vertex3f(x - width / 2, y + height / 2, z + length / 2);// Top Left
    R_gfx_vertex3f(x - width / 2, y - height / 2, z - length / 2);// Bottom Right
    R_gfx_end();
    R_gfx_pop_matrix();
}

// Draw cube wires
R_public void R_draw_cube_wires(R_vec3 position, float width, float height, float length,
                                R_color color) {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    if (R_gfx_check_buffer_limit(36)) R_gfx_draw();

    R_gfx_push_matrix();
    R_gfx_translatef(position.x, position.y, position.z);

    R_gfx_begin(RF_LINES);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);

    // Front Face -----------------------------------------------------
    // Bottom Line
    R_gfx_vertex3f(x - width / 2, y - height / 2, z + length / 2);// Bottom Left
    R_gfx_vertex3f(x + width / 2, y - height / 2, z + length / 2);// Bottom Right

    // Left Line
    R_gfx_vertex3f(x + width / 2, y - height / 2, z + length / 2);// Bottom Right
    R_gfx_vertex3f(x + width / 2, y + height / 2, z + length / 2);// Top Right

    // Top Line
    R_gfx_vertex3f(x + width / 2, y + height / 2, z + length / 2);// Top Right
    R_gfx_vertex3f(x - width / 2, y + height / 2, z + length / 2);// Top Left

    // Right Line
    R_gfx_vertex3f(x - width / 2, y + height / 2, z + length / 2);// Top Left
    R_gfx_vertex3f(x - width / 2, y - height / 2, z + length / 2);// Bottom Left

    // Back Face ------------------------------------------------------
    // Bottom Line
    R_gfx_vertex3f(x - width / 2, y - height / 2, z - length / 2);// Bottom Left
    R_gfx_vertex3f(x + width / 2, y - height / 2, z - length / 2);// Bottom Right

    // Left Line
    R_gfx_vertex3f(x + width / 2, y - height / 2, z - length / 2);// Bottom Right
    R_gfx_vertex3f(x + width / 2, y + height / 2, z - length / 2);// Top Right

    // Top Line
    R_gfx_vertex3f(x + width / 2, y + height / 2, z - length / 2);// Top Right
    R_gfx_vertex3f(x - width / 2, y + height / 2, z - length / 2);// Top Left

    // Right Line
    R_gfx_vertex3f(x - width / 2, y + height / 2, z - length / 2);// Top Left
    R_gfx_vertex3f(x - width / 2, y - height / 2, z - length / 2);// Bottom Left

    // Top Face -------------------------------------------------------
    // Left Line
    R_gfx_vertex3f(x - width / 2, y + height / 2, z + length / 2);// Top Left Front
    R_gfx_vertex3f(x - width / 2, y + height / 2, z - length / 2);// Top Left Back

    // Right Line
    R_gfx_vertex3f(x + width / 2, y + height / 2, z + length / 2);// Top Right Front
    R_gfx_vertex3f(x + width / 2, y + height / 2, z - length / 2);// Top Right Back

    // Bottom Face  ---------------------------------------------------
    // Left Line
    R_gfx_vertex3f(x - width / 2, y - height / 2, z + length / 2);// Top Left Front
    R_gfx_vertex3f(x - width / 2, y - height / 2, z - length / 2);// Top Left Back

    // Right Line
    R_gfx_vertex3f(x + width / 2, y - height / 2, z + length / 2);// Top Right Front
    R_gfx_vertex3f(x + width / 2, y - height / 2, z - length / 2);// Top Right Back
    R_gfx_end();
    R_gfx_pop_matrix();
}

// Draw cube
// NOTE: Cube position is the center position
R_public void R_draw_cube_texture(R_texture2d texture, R_vec3 position, float width, float height,
                                  float length, R_color color) {
    float x = position.x;
    float y = position.y;
    float z = position.z;

    if (R_gfx_check_buffer_limit(36)) R_gfx_draw();

    R_gfx_enable_texture(texture.id);

    //R_gfx_push_matrix();
    // NOTE: Transformation is applied in inverse order (scale -> rotate -> translate)
    //R_gfx_translatef(2.0f, 0.0f, 0.0f);
    //R_gfx_rotatef(45, 0, 1, 0);
    //R_gfx_scalef(2.0f, 2.0f, 2.0f);

    R_gfx_begin(RF_QUADS);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);
    // Front Face
    R_gfx_normal3f(0.0f, 0.0f, 1.0f);// Normal Pointing Towards Viewer
    R_gfx_tex_coord2f(0.0f, 0.0f);
    R_gfx_vertex3f(x - width / 2, y - height / 2,
                   z + length / 2);// Bottom Left Of The R_texture and Quad
    R_gfx_tex_coord2f(1.0f, 0.0f);
    R_gfx_vertex3f(x + width / 2, y - height / 2,
                   z + length / 2);// Bottom Right Of The R_texture and Quad
    R_gfx_tex_coord2f(1.0f, 1.0f);
    R_gfx_vertex3f(x + width / 2, y + height / 2,
                   z + length / 2);// Top Right Of The R_texture and Quad
    R_gfx_tex_coord2f(0.0f, 1.0f);
    R_gfx_vertex3f(x - width / 2, y + height / 2,
                   z + length / 2);// Top Left Of The R_texture and Quad
    // Back Face
    R_gfx_normal3f(0.0f, 0.0f, -1.0f);// Normal Pointing Away From Viewer
    R_gfx_tex_coord2f(1.0f, 0.0f);
    R_gfx_vertex3f(x - width / 2, y - height / 2,
                   z - length / 2);// Bottom Right Of The R_texture and Quad
    R_gfx_tex_coord2f(1.0f, 1.0f);
    R_gfx_vertex3f(x - width / 2, y + height / 2,
                   z - length / 2);// Top Right Of The R_texture and Quad
    R_gfx_tex_coord2f(0.0f, 1.0f);
    R_gfx_vertex3f(x + width / 2, y + height / 2,
                   z - length / 2);// Top Left Of The R_texture and Quad
    R_gfx_tex_coord2f(0.0f, 0.0f);
    R_gfx_vertex3f(x + width / 2, y - height / 2,
                   z - length / 2);// Bottom Left Of The R_texture and Quad
    // Top Face
    R_gfx_normal3f(0.0f, 1.0f, 0.0f);// Normal Pointing Up
    R_gfx_tex_coord2f(0.0f, 1.0f);
    R_gfx_vertex3f(x - width / 2, y + height / 2,
                   z - length / 2);// Top Left Of The R_texture and Quad
    R_gfx_tex_coord2f(0.0f, 0.0f);
    R_gfx_vertex3f(x - width / 2, y + height / 2,
                   z + length / 2);// Bottom Left Of The R_texture and Quad
    R_gfx_tex_coord2f(1.0f, 0.0f);
    R_gfx_vertex3f(x + width / 2, y + height / 2,
                   z + length / 2);// Bottom Right Of The R_texture and Quad
    R_gfx_tex_coord2f(1.0f, 1.0f);
    R_gfx_vertex3f(x + width / 2, y + height / 2,
                   z - length / 2);// Top Right Of The R_texture and Quad
    // Bottom Face
    R_gfx_normal3f(0.0f, -1.0f, 0.0f);// Normal Pointing Down
    R_gfx_tex_coord2f(1.0f, 1.0f);
    R_gfx_vertex3f(x - width / 2, y - height / 2,
                   z - length / 2);// Top Right Of The R_texture and Quad
    R_gfx_tex_coord2f(0.0f, 1.0f);
    R_gfx_vertex3f(x + width / 2, y - height / 2,
                   z - length / 2);// Top Left Of The R_texture and Quad
    R_gfx_tex_coord2f(0.0f, 0.0f);
    R_gfx_vertex3f(x + width / 2, y - height / 2,
                   z + length / 2);// Bottom Left Of The R_texture and Quad
    R_gfx_tex_coord2f(1.0f, 0.0f);
    R_gfx_vertex3f(x - width / 2, y - height / 2,
                   z + length / 2);// Bottom Right Of The R_texture and Quad
    // Right face
    R_gfx_normal3f(1.0f, 0.0f, 0.0f);// Normal Pointing Right
    R_gfx_tex_coord2f(1.0f, 0.0f);
    R_gfx_vertex3f(x + width / 2, y - height / 2,
                   z - length / 2);// Bottom Right Of The R_texture and Quad
    R_gfx_tex_coord2f(1.0f, 1.0f);
    R_gfx_vertex3f(x + width / 2, y + height / 2,
                   z - length / 2);// Top Right Of The R_texture and Quad
    R_gfx_tex_coord2f(0.0f, 1.0f);
    R_gfx_vertex3f(x + width / 2, y + height / 2,
                   z + length / 2);// Top Left Of The R_texture and Quad
    R_gfx_tex_coord2f(0.0f, 0.0f);
    R_gfx_vertex3f(x + width / 2, y - height / 2,
                   z + length / 2);// Bottom Left Of The R_texture and Quad
    // Left Face
    R_gfx_normal3f(-1.0f, 0.0f, 0.0f);// Normal Pointing Left
    R_gfx_tex_coord2f(0.0f, 0.0f);
    R_gfx_vertex3f(x - width / 2, y - height / 2,
                   z - length / 2);// Bottom Left Of The R_texture and Quad
    R_gfx_tex_coord2f(1.0f, 0.0f);
    R_gfx_vertex3f(x - width / 2, y - height / 2,
                   z + length / 2);// Bottom Right Of The R_texture and Quad
    R_gfx_tex_coord2f(1.0f, 1.0f);
    R_gfx_vertex3f(x - width / 2, y + height / 2,
                   z + length / 2);// Top Right Of The R_texture and Quad
    R_gfx_tex_coord2f(0.0f, 1.0f);
    R_gfx_vertex3f(x - width / 2, y + height / 2,
                   z - length / 2);// Top Left Of The R_texture and Quad
    R_gfx_end();
    //R_gfx_pop_matrix();

    R_gfx_disable_texture();
}

// Draw sphere
R_public void R_draw_sphere(R_vec3 center_pos, float radius, R_color color) {
    R_draw_sphere_ex(center_pos, radius, 16, 16, color);
}

// Draw sphere with extended parameters
R_public void R_draw_sphere_ex(R_vec3 center_pos, float radius, int rings, int slices,
                               R_color color) {
    int num_vertex = (rings + 2) * slices * 6;
    if (R_gfx_check_buffer_limit(num_vertex)) R_gfx_draw();

    R_gfx_push_matrix();
    // NOTE: Transformation is applied in inverse order (scale -> translate)
    R_gfx_translatef(center_pos.x, center_pos.y, center_pos.z);
    R_gfx_scalef(radius, radius, radius);

    R_gfx_begin(RF_TRIANGLES);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);

    for (R_int i = 0; i < (rings + 2); i++) {
        for (R_int j = 0; j < slices; j++) {
            R_gfx_vertex3f(cosf(R_deg2rad * (270 + (180 / (rings + 1)) * i)) *
                                   sinf(R_deg2rad * (j * 360 / slices)),
                           sinf(R_deg2rad * (270 + (180 / (rings + 1)) * i)),
                           cosf(R_deg2rad * (270 + (180 / (rings + 1)) * i)) *
                                   cosf(R_deg2rad * (j * 360 / slices)));
            R_gfx_vertex3f(cosf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))) *
                                   sinf(R_deg2rad * ((j + 1) * 360 / slices)),
                           sinf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))),
                           cosf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))) *
                                   cosf(R_deg2rad * ((j + 1) * 360 / slices)));
            R_gfx_vertex3f(cosf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))) *
                                   sinf(R_deg2rad * (j * 360 / slices)),
                           sinf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))),
                           cosf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))) *
                                   cosf(R_deg2rad * (j * 360 / slices)));

            R_gfx_vertex3f(cosf(R_deg2rad * (270 + (180 / (rings + 1)) * i)) *
                                   sinf(R_deg2rad * (j * 360 / slices)),
                           sinf(R_deg2rad * (270 + (180 / (rings + 1)) * i)),
                           cosf(R_deg2rad * (270 + (180 / (rings + 1)) * i)) *
                                   cosf(R_deg2rad * (j * 360 / slices)));
            R_gfx_vertex3f(cosf(R_deg2rad * (270 + (180 / (rings + 1)) * (i))) *
                                   sinf(R_deg2rad * ((j + 1) * 360 / slices)),
                           sinf(R_deg2rad * (270 + (180 / (rings + 1)) * (i))),
                           cosf(R_deg2rad * (270 + (180 / (rings + 1)) * (i))) *
                                   cosf(R_deg2rad * ((j + 1) * 360 / slices)));
            R_gfx_vertex3f(cosf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))) *
                                   sinf(R_deg2rad * ((j + 1) * 360 / slices)),
                           sinf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))),
                           cosf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))) *
                                   cosf(R_deg2rad * ((j + 1) * 360 / slices)));
        }
    }
    R_gfx_end();
    R_gfx_pop_matrix();
}

// Draw sphere wires
R_public void R_draw_sphere_wires(R_vec3 center_pos, float radius, int rings, int slices,
                                  R_color color) {
    int num_vertex = (rings + 2) * slices * 6;
    if (R_gfx_check_buffer_limit(num_vertex)) R_gfx_draw();

    R_gfx_push_matrix();
    // NOTE: Transformation is applied in inverse order (scale -> translate)
    R_gfx_translatef(center_pos.x, center_pos.y, center_pos.z);
    R_gfx_scalef(radius, radius, radius);

    R_gfx_begin(RF_LINES);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);

    for (R_int i = 0; i < (rings + 2); i++) {
        for (R_int j = 0; j < slices; j++) {
            R_gfx_vertex3f(cosf(R_deg2rad * (270 + (180 / (rings + 1)) * i)) *
                                   sinf(R_deg2rad * (j * 360 / slices)),
                           sinf(R_deg2rad * (270 + (180 / (rings + 1)) * i)),
                           cosf(R_deg2rad * (270 + (180 / (rings + 1)) * i)) *
                                   cosf(R_deg2rad * (j * 360 / slices)));
            R_gfx_vertex3f(cosf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))) *
                                   sinf(R_deg2rad * ((j + 1) * 360 / slices)),
                           sinf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))),
                           cosf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))) *
                                   cosf(R_deg2rad * ((j + 1) * 360 / slices)));

            R_gfx_vertex3f(cosf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))) *
                                   sinf(R_deg2rad * ((j + 1) * 360 / slices)),
                           sinf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))),
                           cosf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))) *
                                   cosf(R_deg2rad * ((j + 1) * 360 / slices)));
            R_gfx_vertex3f(cosf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))) *
                                   sinf(R_deg2rad * (j * 360 / slices)),
                           sinf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))),
                           cosf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))) *
                                   cosf(R_deg2rad * (j * 360 / slices)));

            R_gfx_vertex3f(cosf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))) *
                                   sinf(R_deg2rad * (j * 360 / slices)),
                           sinf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))),
                           cosf(R_deg2rad * (270 + (180 / (rings + 1)) * (i + 1))) *
                                   cosf(R_deg2rad * (j * 360 / slices)));

            R_gfx_vertex3f(cosf(R_deg2rad * (270 + (180 / (rings + 1)) * i)) *
                                   sinf(R_deg2rad * (j * 360 / slices)),
                           sinf(R_deg2rad * (270 + (180 / (rings + 1)) * i)),
                           cosf(R_deg2rad * (270 + (180 / (rings + 1)) * i)) *
                                   cosf(R_deg2rad * (j * 360 / slices)));
        }
    }

    R_gfx_end();
    R_gfx_pop_matrix();
}

// Draw a cylinder
// NOTE: It could be also used for pyramid and cone
R_public void R_draw_cylinder(R_vec3 position, float radius_top, float radius_bottom, float height,
                              int sides, R_color color) {
    if (sides < 3) sides = 3;

    int num_vertex = sides * 6;
    if (R_gfx_check_buffer_limit(num_vertex)) R_gfx_draw();

    R_gfx_push_matrix();
    R_gfx_translatef(position.x, position.y, position.z);

    R_gfx_begin(RF_TRIANGLES);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);

    if (radius_top > 0) {
        // Draw Body -------------------------------------------------------------------------------------
        for (R_int i = 0; i < 360; i += 360 / sides) {
            R_gfx_vertex3f(sinf(R_deg2rad * i) * radius_bottom, 0,
                           cosf(R_deg2rad * i) * radius_bottom);// Bottom Left
            R_gfx_vertex3f(sinf(R_deg2rad * (i + 360 / sides)) * radius_bottom, 0,
                           cosf(R_deg2rad * (i + 360 / sides)) * radius_bottom);// Bottom Right
            R_gfx_vertex3f(sinf(R_deg2rad * (i + 360 / sides)) * radius_top, height,
                           cosf(R_deg2rad * (i + 360 / sides)) * radius_top);// Top Right

            R_gfx_vertex3f(sinf(R_deg2rad * i) * radius_top, height,
                           cosf(R_deg2rad * i) * radius_top);// Top Left
            R_gfx_vertex3f(sinf(R_deg2rad * i) * radius_bottom, 0,
                           cosf(R_deg2rad * i) * radius_bottom);// Bottom Left
            R_gfx_vertex3f(sinf(R_deg2rad * (i + 360 / sides)) * radius_top, height,
                           cosf(R_deg2rad * (i + 360 / sides)) * radius_top);// Top Right
        }

        // Draw Cap --------------------------------------------------------------------------------------
        for (R_int i = 0; i < 360; i += 360 / sides) {
            R_gfx_vertex3f(0, height, 0);
            R_gfx_vertex3f(sinf(R_deg2rad * i) * radius_top, height,
                           cosf(R_deg2rad * i) * radius_top);
            R_gfx_vertex3f(sinf(R_deg2rad * (i + 360 / sides)) * radius_top, height,
                           cosf(R_deg2rad * (i + 360 / sides)) * radius_top);
        }
    } else {
        // Draw Cone -------------------------------------------------------------------------------------
        for (R_int i = 0; i < 360; i += 360 / sides) {
            R_gfx_vertex3f(0, height, 0);
            R_gfx_vertex3f(sinf(R_deg2rad * i) * radius_bottom, 0,
                           cosf(R_deg2rad * i) * radius_bottom);
            R_gfx_vertex3f(sinf(R_deg2rad * (i + 360 / sides)) * radius_bottom, 0,
                           cosf(R_deg2rad * (i + 360 / sides)) * radius_bottom);
        }
    }

    // Draw Base -----------------------------------------------------------------------------------------
    for (R_int i = 0; i < 360; i += 360 / sides) {
        R_gfx_vertex3f(0, 0, 0);
        R_gfx_vertex3f(sinf(R_deg2rad * (i + 360 / sides)) * radius_bottom, 0,
                       cosf(R_deg2rad * (i + 360 / sides)) * radius_bottom);
        R_gfx_vertex3f(sinf(R_deg2rad * i) * radius_bottom, 0, cosf(R_deg2rad * i) * radius_bottom);
    }

    R_gfx_end();
    R_gfx_pop_matrix();
}

// Draw a wired cylinder
// NOTE: It could be also used for pyramid and cone
R_public void R_draw_cylinder_wires(R_vec3 position, float radius_top, float radius_bottom,
                                    float height, int sides, R_color color) {
    if (sides < 3) sides = 3;

    int num_vertex = sides * 8;
    if (R_gfx_check_buffer_limit(num_vertex)) R_gfx_draw();

    R_gfx_push_matrix();
    R_gfx_translatef(position.x, position.y, position.z);

    R_gfx_begin(RF_LINES);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);

    for (R_int i = 0; i < 360; i += 360 / sides) {
        R_gfx_vertex3f(sinf(R_deg2rad * i) * radius_bottom, 0, cosf(R_deg2rad * i) * radius_bottom);
        R_gfx_vertex3f(sinf(R_deg2rad * (i + 360 / sides)) * radius_bottom, 0,
                       cosf(R_deg2rad * (i + 360 / sides)) * radius_bottom);

        R_gfx_vertex3f(sinf(R_deg2rad * (i + 360 / sides)) * radius_bottom, 0,
                       cosf(R_deg2rad * (i + 360 / sides)) * radius_bottom);
        R_gfx_vertex3f(sinf(R_deg2rad * (i + 360 / sides)) * radius_top, height,
                       cosf(R_deg2rad * (i + 360 / sides)) * radius_top);

        R_gfx_vertex3f(sinf(R_deg2rad * (i + 360 / sides)) * radius_top, height,
                       cosf(R_deg2rad * (i + 360 / sides)) * radius_top);
        R_gfx_vertex3f(sinf(R_deg2rad * i) * radius_top, height, cosf(R_deg2rad * i) * radius_top);

        R_gfx_vertex3f(sinf(R_deg2rad * i) * radius_top, height, cosf(R_deg2rad * i) * radius_top);
        R_gfx_vertex3f(sinf(R_deg2rad * i) * radius_bottom, 0, cosf(R_deg2rad * i) * radius_bottom);
    }
    R_gfx_end();
    R_gfx_pop_matrix();
}

// Draw a plane
R_public void R_draw_plane(R_vec3 center_pos, R_vec2 size, R_color color) {
    if (R_gfx_check_buffer_limit(4)) R_gfx_draw();

    // NOTE: Plane is always created on XZ ground
    R_gfx_push_matrix();
    R_gfx_translatef(center_pos.x, center_pos.y, center_pos.z);
    R_gfx_scalef(size.x, 1.0f, size.y);

    R_gfx_begin(RF_QUADS);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);
    R_gfx_normal3f(0.0f, 1.0f, 0.0f);

    R_gfx_vertex3f(-0.5f, 0.0f, -0.5f);
    R_gfx_vertex3f(-0.5f, 0.0f, 0.5f);
    R_gfx_vertex3f(0.5f, 0.0f, 0.5f);
    R_gfx_vertex3f(0.5f, 0.0f, -0.5f);
    R_gfx_end();
    R_gfx_pop_matrix();
}

// Draw a ray line
R_public void R_draw_ray(R_ray ray, R_color color) {
    float scale = 10000;

    R_gfx_begin(RF_LINES);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);
    R_gfx_color4ub(color.r, color.g, color.b, color.a);

    R_gfx_vertex3f(ray.position.x, ray.position.y, ray.position.z);
    R_gfx_vertex3f(ray.position.x + ray.direction.x * scale,
                   ray.position.y + ray.direction.y * scale,
                   ray.position.z + ray.direction.z * scale);
    R_gfx_end();
}

// Draw a grid centered at (0, 0, 0)
R_public void R_draw_grid(int slices, float spacing) {
    int half_slices = slices / 2;

    if (R_gfx_check_buffer_limit(slices * 4)) R_gfx_draw();

    R_gfx_begin(RF_LINES);
    for (R_int i = -half_slices; i <= half_slices; i++) {
        if (i == 0) {
            R_gfx_color3f(0.5f, 0.5f, 0.5f);
            R_gfx_color3f(0.5f, 0.5f, 0.5f);
            R_gfx_color3f(0.5f, 0.5f, 0.5f);
            R_gfx_color3f(0.5f, 0.5f, 0.5f);
        } else {
            R_gfx_color3f(0.75f, 0.75f, 0.75f);
            R_gfx_color3f(0.75f, 0.75f, 0.75f);
            R_gfx_color3f(0.75f, 0.75f, 0.75f);
            R_gfx_color3f(0.75f, 0.75f, 0.75f);
        }

        R_gfx_vertex3f((float) i * spacing, 0.0f, (float) -half_slices * spacing);
        R_gfx_vertex3f((float) i * spacing, 0.0f, (float) half_slices * spacing);

        R_gfx_vertex3f((float) -half_slices * spacing, 0.0f, (float) i * spacing);
        R_gfx_vertex3f((float) half_slices * spacing, 0.0f, (float) i * spacing);
    }
    R_gfx_end();
}

// Draw gizmo
R_public void R_draw_gizmo(R_vec3 position) {
    // NOTE: RGB = XYZ
    float length = 1.0f;

    R_gfx_push_matrix();
    R_gfx_translatef(position.x, position.y, position.z);
    R_gfx_scalef(length, length, length);

    R_gfx_begin(RF_LINES);
    R_gfx_color3f(1.0f, 0.0f, 0.0f);
    R_gfx_vertex3f(0.0f, 0.0f, 0.0f);
    R_gfx_color3f(1.0f, 0.0f, 0.0f);
    R_gfx_vertex3f(1.0f, 0.0f, 0.0f);

    R_gfx_color3f(0.0f, 1.0f, 0.0f);
    R_gfx_vertex3f(0.0f, 0.0f, 0.0f);
    R_gfx_color3f(0.0f, 1.0f, 0.0f);
    R_gfx_vertex3f(0.0f, 1.0f, 0.0f);

    R_gfx_color3f(0.0f, 0.0f, 1.0f);
    R_gfx_vertex3f(0.0f, 0.0f, 0.0f);
    R_gfx_color3f(0.0f, 0.0f, 1.0f);
    R_gfx_vertex3f(0.0f, 0.0f, 1.0f);
    R_gfx_end();
    R_gfx_pop_matrix();
}

// Draw a model (with texture if set)
R_public void R_draw_model(R_model model, R_vec3 position, float scale, R_color tint) {
    R_vec3 vScale = {scale, scale, scale};
    R_vec3 rotationAxis = {0.0f, 1.0f, 0.0f};

    R_draw_model_ex(model, position, rotationAxis, 0.0f, vScale, tint);
}

// Draw a model with extended parameters
R_public void R_draw_model_ex(R_model model, R_vec3 position, R_vec3 rotation_axis,
                              float rotationAngle, R_vec3 scale, R_color tint) {
    // Calculate transformation matrix from function parameters
    // Get transform matrix (rotation -> scale -> translation)
    R_mat mat_scale = R_mat_scale(scale.x, scale.y, scale.z);
    R_mat mat_rotation = R_mat_rotate(rotation_axis, rotationAngle * R_deg2rad);
    R_mat mat_translation = R_mat_translate(position.x, position.y, position.z);

    R_mat mat_transform = R_mat_mul(R_mat_mul(mat_scale, mat_rotation), mat_translation);

    // Combine model transformation matrix (model.transform) with matrix generated by function parameters (mat_transform)
    model.transform = R_mat_mul(model.transform, mat_transform);

    for (R_int i = 0; i < model.mesh_count; i++) {
        // TODO: Review color + tint premultiplication mechanism
        R_color color = model.materials[model.mesh_material[i]].maps[RF_MAP_DIFFUSE].color;

        R_color color_tint = R_white;
        color_tint.r = (((float) color.r / 255.0) * ((float) tint.r / 255.0)) * 255;
        color_tint.g = (((float) color.g / 255.0) * ((float) tint.g / 255.0)) * 255;
        color_tint.b = (((float) color.b / 255.0) * ((float) tint.b / 255.0)) * 255;
        color_tint.a = (((float) color.a / 255.0) * ((float) tint.a / 255.0)) * 255;

        model.materials[model.mesh_material[i]].maps[RF_MAP_DIFFUSE].color = color_tint;
        R_gfx_draw_mesh(model.meshes[i], model.materials[model.mesh_material[i]], model.transform);
        model.materials[model.mesh_material[i]].maps[RF_MAP_DIFFUSE].color = color;
    }
}

// Draw a model wires (with texture if set) with extended parameters
R_public void R_draw_model_wires(R_model model, R_vec3 position, R_vec3 rotation_axis,
                                 float rotationAngle, R_vec3 scale, R_color tint) {
    R_gfx_enable_wire_mode();

    R_draw_model_ex(model, position, rotation_axis, rotationAngle, scale, tint);

    R_gfx_disable_wire_mode();
}

// Draw a bounding box with wires
R_public void R_draw_bounding_box(R_bounding_box box, R_color color) {
    R_vec3 size;

    size.x = (float) fabs(box.max.x - box.min.x);
    size.y = (float) fabs(box.max.y - box.min.y);
    size.z = (float) fabs(box.max.z - box.min.z);

    R_vec3 center = {box.min.x + size.x / 2.0f, box.min.y + size.y / 2.0f,
                     box.min.z + size.z / 2.0f};

    R_draw_cube_wires(center, size.x, size.y, size.z, color);
}

// Draw a billboard
R_public void R_draw_billboard(R_camera3d camera, R_texture2d texture, R_vec3 center, float size,
                               R_color tint) {
    R_rec source_rec = {0.0f, 0.0f, (float) texture.width, (float) texture.height};

    R_draw_billboard_rec(camera, texture, source_rec, center, size, tint);
}

// Draw a billboard (part of a texture defined by a rectangle)
R_public void R_draw_billboard_rec(R_camera3d camera, R_texture2d texture, R_rec source_rec,
                                   R_vec3 center, float size, R_color tint) {
    // NOTE: Billboard size will maintain source_rec aspect ratio, size will represent billboard width
    R_vec2 size_ratio = {size, size * (float) source_rec.height / source_rec.width};

    R_mat mat_view = R_mat_look_at(camera.position, camera.target, camera.up);

    R_vec3 right = {mat_view.m0, mat_view.m4, mat_view.m8};
    //R_vec3 up = { mat_view.m1, mat_view.m5, mat_view.m9 };

    // NOTE: Billboard locked on axis-Y
    R_vec3 up = {0.0f, 1.0f, 0.0f};
    /*
        a-------b
        |       |
        |   *   |
        |       |
        d-------c
    */
    right = R_vec3_scale(right, size_ratio.x / 2);
    up = R_vec3_scale(up, size_ratio.y / 2);

    R_vec3 p1 = R_vec3_add(right, up);
    R_vec3 p2 = R_vec3_sub(right, up);

    R_vec3 a = R_vec3_sub(center, p2);
    R_vec3 b = R_vec3_add(center, p1);
    R_vec3 c = R_vec3_add(center, p2);
    R_vec3 d = R_vec3_sub(center, p1);

    if (R_gfx_check_buffer_limit(4)) R_gfx_draw();

    R_gfx_enable_texture(texture.id);

    R_gfx_begin(RF_QUADS);
    R_gfx_color4ub(tint.r, tint.g, tint.b, tint.a);

    // Bottom-left corner for texture and quad
    R_gfx_tex_coord2f((float) source_rec.x / texture.width, (float) source_rec.y / texture.height);
    R_gfx_vertex3f(a.x, a.y, a.z);

    // Top-left corner for texture and quad
    R_gfx_tex_coord2f((float) source_rec.x / texture.width,
                      (float) (source_rec.y + source_rec.height) / texture.height);
    R_gfx_vertex3f(d.x, d.y, d.z);

    // Top-right corner for texture and quad
    R_gfx_tex_coord2f((float) (source_rec.x + source_rec.width) / texture.width,
                      (float) (source_rec.y + source_rec.height) / texture.height);
    R_gfx_vertex3f(c.x, c.y, c.z);

    // Bottom-right corner for texture and quad
    R_gfx_tex_coord2f((float) (source_rec.x + source_rec.width) / texture.width,
                      (float) source_rec.y / texture.height);
    R_gfx_vertex3f(b.x, b.y, b.z);
    R_gfx_end();

    R_gfx_disable_texture();
}

R_public R_render_batch R_create_custom_render_batch_from_buffers(R_vertex_buffer *vertex_buffers,
                                                                  R_int vertex_buffers_count,
                                                                  R_draw_call *draw_calls,
                                                                  R_int draw_calls_count) {
    if (!vertex_buffers || !draw_calls || vertex_buffers_count < 0 || draw_calls_count < 0) {
        return (R_render_batch){0};
    }

    R_render_batch batch = {0};
    batch.vertex_buffers = vertex_buffers;
    batch.vertex_buffers_count = vertex_buffers_count;
    batch.draw_calls = draw_calls;
    batch.draw_calls_size = draw_calls_count;

    for (R_int i = 0; i < vertex_buffers_count; i++) {
        memset(vertex_buffers[i].vertices, 0,
               RF_GFX_VERTEX_COMPONENT_COUNT * vertex_buffers[i].elements_count);
        memset(vertex_buffers[i].texcoords, 0,
               RF_GFX_TEXCOORD_COMPONENT_COUNT * vertex_buffers[i].elements_count);
        memset(vertex_buffers[i].colors, 0,
               RF_GFX_COLOR_COMPONENT_COUNT * vertex_buffers[i].elements_count);

        int k = 0;

        // Indices can be initialized right now
        for (R_int j = 0;
             j < (RF_GFX_VERTEX_INDEX_COMPONENT_COUNT * vertex_buffers[i].elements_count); j += 6) {
            vertex_buffers[i].indices[j + 0] = 4 * k + 0;
            vertex_buffers[i].indices[j + 1] = 4 * k + 1;
            vertex_buffers[i].indices[j + 2] = 4 * k + 2;
            vertex_buffers[i].indices[j + 3] = 4 * k + 0;
            vertex_buffers[i].indices[j + 4] = 4 * k + 2;
            vertex_buffers[i].indices[j + 5] = 4 * k + 3;

            k++;
        }

        vertex_buffers[i].v_counter = 0;
        vertex_buffers[i].tc_counter = 0;
        vertex_buffers[i].c_counter = 0;

        R_gfx_init_vertex_buffer(&vertex_buffers[i]);
    }

    for (R_int i = 0; i < RF_DEFAULT_BATCH_DRAW_CALLS_COUNT; i++) {
        batch.draw_calls[i] = (R_draw_call){
                .mode = RF_QUADS,
                .texture_id = R_ctx.default_texture_id,
        };
    }

    batch.draw_calls_counter = 1;// Reset draws counter
    batch.current_depth = -1.0f; // Reset depth value
    batch.valid = 1;

    return batch;
}

// TODO: Not working yet
R_public R_render_batch R_create_custom_render_batch(R_int vertex_buffers_count,
                                                     R_int draw_calls_count,
                                                     R_int vertex_buffer_elements_count,
                                                     R_allocator allocator) {
    if (vertex_buffers_count < 0 || draw_calls_count < 0 || vertex_buffer_elements_count < 0) {
        return (R_render_batch){0};
    }

    R_render_batch result = {0};

    R_int vertex_buffer_array_size = sizeof(R_vertex_buffer) * vertex_buffers_count;
    R_int vertex_buffers_memory_size =
            (sizeof(R_one_element_vertex_buffer) * vertex_buffer_elements_count) *
            vertex_buffers_count;
    R_int draw_calls_array_size = sizeof(R_draw_call) * draw_calls_count;
    R_int allocation_size =
            vertex_buffer_array_size + draw_calls_array_size + vertex_buffers_memory_size;

    char *memory = R_alloc(allocator, allocation_size);

    if (memory) {
        R_vertex_buffer *buffers = (R_vertex_buffer *) memory;
        R_draw_call *draw_calls = (R_draw_call *) (memory + vertex_buffer_array_size);
        char *buffers_memory = memory + vertex_buffer_array_size + draw_calls_array_size;

        R_assert(((char *) draw_calls - memory) ==
                 draw_calls_array_size + vertex_buffers_memory_size);
        R_assert((buffers_memory - memory) == vertex_buffers_memory_size);
        R_assert((buffers_memory - memory) == sizeof(R_one_element_vertex_buffer) *
                                                      vertex_buffer_elements_count *
                                                      vertex_buffers_count);

        for (R_int i = 0; i < vertex_buffers_count; i++) {
            R_int one_vertex_buffer_memory_size =
                    sizeof(R_one_element_vertex_buffer) * vertex_buffer_elements_count;
            R_int vertices_size = sizeof(R_gfx_vertex_data_type) * vertex_buffer_elements_count;
            R_int texcoords_size = sizeof(R_gfx_texcoord_data_type) * vertex_buffer_elements_count;
            R_int colors_size = sizeof(R_gfx_color_data_type) * vertex_buffer_elements_count;
            R_int indices_size =
                    sizeof(R_gfx_vertex_index_data_type) * vertex_buffer_elements_count;

            char *this_buffer_memory = buffers_memory + one_vertex_buffer_memory_size * i;

            buffers[i].elements_count = vertex_buffer_elements_count;
            buffers[i].vertices = (R_gfx_vertex_data_type *) this_buffer_memory;
            buffers[i].texcoords = (R_gfx_texcoord_data_type *) this_buffer_memory + vertices_size;
            buffers[i].colors =
                    (R_gfx_color_data_type *) this_buffer_memory + vertices_size + texcoords_size;
            buffers[i].indices = (R_gfx_vertex_index_data_type *) this_buffer_memory +
                                 vertices_size + texcoords_size + colors_size;
        }

        result = R_create_custom_render_batch_from_buffers(buffers, vertex_buffers_count,
                                                           draw_calls, draw_calls_count);
    }

    return result;
}

R_public R_render_batch R_create_default_render_batch_from_memory(R_default_render_batch *memory) {
    if (!memory) { return (R_render_batch){0}; }

    for (R_int i = 0; i < RF_DEFAULT_BATCH_VERTEX_BUFFERS_COUNT; i++) {
        memory->vertex_buffers[i].elements_count = RF_DEFAULT_BATCH_ELEMENTS_COUNT;
        memory->vertex_buffers[i].vertices = memory->vertex_buffers_memory[i].vertices;
        memory->vertex_buffers[i].texcoords = memory->vertex_buffers_memory[i].texcoords;
        memory->vertex_buffers[i].colors = memory->vertex_buffers_memory[i].colors;
        memory->vertex_buffers[i].indices = memory->vertex_buffers_memory[i].indices;
    }

    return R_create_custom_render_batch_from_buffers(
            memory->vertex_buffers, RF_DEFAULT_BATCH_VERTEX_BUFFERS_COUNT, memory->draw_calls,
            RF_DEFAULT_BATCH_DRAW_CALLS_COUNT);
}

R_public R_render_batch R_create_default_render_batch(R_allocator allocator) {
    R_default_render_batch *memory = R_alloc(allocator, sizeof(R_default_render_batch));
    return R_create_default_render_batch_from_memory(memory);
}

R_public void R_set_active_render_batch(R_render_batch *batch) { R_ctx.current_batch = batch; }

R_public void R_unload_render_batch(R_render_batch batch, R_allocator allocator) {
    R_free(allocator, batch.vertex_buffers);
}

R_internal void R__gfx_backend_internal_init(R_gfx_backend_data *gfx_data);

R_public void R_gfx_init(R_gfx_context *ctx, int screen_width, int screen_height,
                         R_gfx_backend_data *gfx_data) {
    *ctx = (R_gfx_context){0};
    R_set_global_gfx_context_pointer(ctx);

    R_ctx.current_matrix_mode = -1;
    R_ctx.screen_scaling = R_mat_identity();

    R_ctx.framebuffer_width = screen_width;
    R_ctx.framebuffer_height = screen_height;
    R_ctx.render_width = screen_width;
    R_ctx.render_height = screen_height;
    R_ctx.current_width = screen_width;
    R_ctx.current_height = screen_height;

    R__gfx_backend_internal_init(gfx_data);

    // Initialize default shaders and default textures
    {
        // Init default white texture
        unsigned char pixels[4] = {255, 255, 255, 255};// 1 pixel RGBA (4 bytes)
        R_ctx.default_texture_id = R_gfx_load_texture(pixels, 1, 1, R_pixel_format_r8g8b8a8, 1);

        if (R_ctx.default_texture_id != 0) {
            R_log(R_log_type_info, "Base white texture loaded successfully. [ Texture ID: %d ]",
                  R_ctx.default_texture_id);
        } else {
            R_log(R_log_type_warning, "Base white texture could not be loaded");
        }

        // Init default shader (customized for GL 3.3 and ES2)
        R_ctx.default_shader = R_load_default_shader();
        R_ctx.current_shader = R_ctx.default_shader;

        // Init transformations matrix accumulator
        R_ctx.transform = R_mat_identity();

        // Init internal matrix stack (emulating OpenGL 1)
        for (R_int i = 0; i < RF_MAX_MATRIX_STACK_SIZE; i++) { R_ctx.stack[i] = R_mat_identity(); }

        // Init internal matrices
        R_ctx.projection = R_mat_identity();
        R_ctx.modelview = R_mat_identity();
        R_ctx.current_matrix = &R_ctx.modelview;
    }

    // Setup default viewport
    R_set_viewport(screen_width, screen_height);

// Load default font
#if !defined(METADOT_NO_DEFAULT_FONT)
    {
        // NOTE: Using UTF8 encoding table for Unicode U+0000..U+00FF Basic Latin + Latin-1 Supplement
        // http://www.utf8-chartable.de/unicode-utf8-table.pl

        R_ctx.default_font.glyphs_count = 224;// Number of chars included in our default font

        // Default font is directly defined here (data generated from a sprite font image)
        // This way, we reconstruct R_font without creating large global variables
        // This data is automatically allocated to Stack and automatically deallocated at the end of this function
        static unsigned int default_font_data[512] = {
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00200020, 0x0001b000, 0x00000000,
                0x00000000, 0x8ef92520, 0x00020a00, 0x7dbe8000, 0x1f7df45f, 0x4a2bf2a0, 0x0852091e,
                0x41224000, 0x10041450, 0x2e292020, 0x08220812, 0x41222000, 0x10041450, 0x10f92020,
                0x3efa084c, 0x7d22103c, 0x107df7de, 0xe8a12020, 0x08220832, 0x05220800, 0x10450410,
                0xa4a3f000, 0x08520832, 0x05220400, 0x10450410, 0xe2f92020, 0x0002085e, 0x7d3e0281,
                0x107df41f, 0x00200000, 0x8001b000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0xc0000fbe, 0xfbf7e00f, 0x5fbf7e7d, 0x0050bee8,
                0x440808a2, 0x0a142fe8, 0x50810285, 0x0050a048, 0x49e428a2, 0x0a142828, 0x40810284,
                0x0048a048, 0x10020fbe, 0x09f7ebaf, 0xd89f3e84, 0x0047a04f, 0x09e48822, 0x0a142aa1,
                0x50810284, 0x0048a048, 0x04082822, 0x0a142fa0, 0x50810285, 0x0050a248, 0x00008fbe,
                0xfbf42021, 0x5f817e7d, 0x07d09ce8, 0x00008000, 0x00000fe0, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000c0180, 0xdfbf4282, 0x0bfbf7ef,
                0x42850505, 0x004804bf, 0x50a142c6, 0x08401428, 0x42852505, 0x00a808a0, 0x50a146aa,
                0x08401428, 0x42852505, 0x00081090, 0x5fa14a92, 0x0843f7e8, 0x7e792505, 0x00082088,
                0x40a15282, 0x08420128, 0x40852489, 0x00084084, 0x40a16282, 0x0842022a, 0x40852451,
                0x00088082, 0xc0bf4282, 0xf843f42f, 0x7e85fc21, 0x3e0900bf, 0x00000000, 0x00000004,
                0x00000000, 0x000c0180, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x04000402, 0x41482000, 0x00000000, 0x00000800, 0x04000404, 0x4100203c, 0x00000000,
                0x00000800, 0xf7df7df0, 0x514bef85, 0xbefbefbe, 0x04513bef, 0x14414500, 0x494a2885,
                0xa28a28aa, 0x04510820, 0xf44145f0, 0x474a289d, 0xa28a28aa, 0x04510be0, 0x14414510,
                0x494a2884, 0xa28a28aa, 0x02910a00, 0xf7df7df0, 0xd14a2f85, 0xbefbe8aa, 0x011f7be0,
                0x00000000, 0x00400804, 0x20080000, 0x00000000, 0x00000000, 0x00600f84, 0x20080000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xac000000, 0x00000f01,
                0x00000000, 0x00000000, 0x24000000, 0x00000f01, 0x00000000, 0x06000000, 0x24000000,
                0x00000f01, 0x00000000, 0x09108000, 0x24fa28a2, 0x00000f01, 0x00000000, 0x013e0000,
                0x2242252a, 0x00000f52, 0x00000000, 0x038a8000, 0x2422222a, 0x00000f29, 0x00000000,
                0x010a8000, 0x2412252a, 0x00000f01, 0x00000000, 0x010a8000, 0x24fbe8be, 0x00000f01,
                0x00000000, 0x0ebe8000, 0xac020000, 0x00000f01, 0x00000000, 0x00048000, 0x0003e000,
                0x00000f00, 0x00000000, 0x00008000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00000038, 0x8443b80e, 0x00203a03, 0x02bea080, 0xf0000020, 0xc452208a,
                0x04202b02, 0xf8029122, 0x07f0003b, 0xe44b388e, 0x02203a02, 0x081e8a1c, 0x0411e92a,
                0xf4420be0, 0x01248202, 0xe8140414, 0x05d104ba, 0xe7c3b880, 0x00893a0a, 0x283c0e1c,
                0x04500902, 0xc4400080, 0x00448002, 0xe8208422, 0x04500002, 0x80400000, 0x05200002,
                0x083e8e00, 0x04100002, 0x804003e0, 0x07000042, 0xf8008400, 0x07f00003, 0x80400000,
                0x04000022, 0x00000000, 0x00000000, 0x80400000, 0x04000002, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00800702, 0x1848a0c2, 0x84010000, 0x02920921, 0x01042642,
                0x00005121, 0x42023f7f, 0x00291002, 0xefc01422, 0x7efdfbf7, 0xefdfa109, 0x03bbbbf7,
                0x28440f12, 0x42850a14, 0x20408109, 0x01111010, 0x28440408, 0x42850a14, 0x2040817f,
                0x01111010, 0xefc78204, 0x7efdfbf7, 0xe7cf8109, 0x011111f3, 0x2850a932, 0x42850a14,
                0x2040a109, 0x01111010, 0x2850b840, 0x42850a14, 0xefdfbf79, 0x03bbbbf7, 0x001fa020,
                0x00000000, 0x00001000, 0x00000000, 0x00002070, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x08022800, 0x00012283, 0x02430802,
                0x01010001, 0x8404147c, 0x20000144, 0x80048404, 0x00823f08, 0xdfbf4284, 0x7e03f7ef,
                0x142850a1, 0x0000210a, 0x50a14684, 0x528a1428, 0x142850a1, 0x03efa17a, 0x50a14a9e,
                0x52521428, 0x142850a1, 0x02081f4a, 0x50a15284, 0x4a221428, 0xf42850a1, 0x03efa14b,
                0x50a16284, 0x4a521428, 0x042850a1, 0x0228a17a, 0xdfbf427c, 0x7e8bf7ef, 0xf7efdfbf,
                0x03efbd0b, 0x00000000, 0x04000000, 0x00000000, 0x00000008, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00200508,
                0x00840400, 0x11458122, 0x00014210, 0x00514294, 0x51420800, 0x20a22a94, 0x0050a508,
                0x00200000, 0x00000000, 0x00050000, 0x08000000, 0xfefbefbe, 0xfbefbefb, 0xfbeb9114,
                0x00fbefbe, 0x20820820, 0x8a28a20a, 0x8a289114, 0x3e8a28a2, 0xfefbefbe, 0xfbefbe0b,
                0x8a289114, 0x008a28a2, 0x228a28a2, 0x08208208, 0x8a289114, 0x088a28a2, 0xfefbefbe,
                0xfbefbefb, 0xfa2f9114, 0x00fbefbe, 0x00000000, 0x00000040, 0x00000000, 0x00000000,
                0x00000000, 0x00000020, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00210100, 0x00000004, 0x00000000, 0x00000000, 0x14508200, 0x00001402,
                0x00000000, 0x00000000, 0x00000010, 0x00000020, 0x00000000, 0x00000000, 0xa28a28be,
                0x00002228, 0x00000000, 0x00000000, 0xa28a28aa, 0x000022e8, 0x00000000, 0x00000000,
                0xa28a28aa, 0x000022a8, 0x00000000, 0x00000000, 0xa28a28aa, 0x000022e8, 0x00000000,
                0x00000000, 0xbefbefbe, 0x00003e2f, 0x00000000, 0x00000000, 0x00000004, 0x00002028,
                0x00000000, 0x00000000, 0x80000000, 0x00003e0f, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
                0x00000000};

        int chars_height = 10;
        int chars_divisor =
                1;// Every char is separated from the consecutive by a 1 pixel divisor, horizontally and vertically

        int chars_width[224] = {
                3, 1, 4, 6, 5, 7, 6, 2, 3, 3, 5, 5, 2, 4, 1, 7, 5, 2, 5, 5, 5, 5, 5, 5, 5, 5, 1, 1,
                3, 4, 3, 6, 7, 6, 6, 6, 6, 6, 6, 6, 6, 3, 5, 6, 5, 7, 6, 6, 6, 6, 6, 6, 7, 6, 7, 7,
                6, 6, 6, 2, 7, 2, 3, 5, 2, 5, 5, 5, 5, 5, 4, 5, 5, 1, 2, 5, 2, 5, 5, 5, 5, 5, 5, 5,
                4, 5, 5, 5, 5, 5, 5, 3, 1, 3, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 5, 5, 7, 1, 5, 3, 7, 3, 5,
                4, 1, 7, 4, 3, 5, 3, 3, 2, 5, 6, 1, 2, 2, 3, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 6,
                6, 6, 6, 6, 3, 3, 3, 3, 7, 6, 6, 6, 6, 6, 6, 5, 6, 6, 6, 6, 6, 6, 4, 6, 5, 5, 5, 5,
                5, 5, 9, 5, 5, 5, 5, 5, 2, 2, 3, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 3, 5};

        // Re-construct image from R_ctx->default_font_data and generate a texture
        //----------------------------------------------------------------------
        {
            R_color font_pixels[128 * 128] = {0};

            int counter = 0;// R_font data elements counter

            // Fill with default_font_data (convert from bit to pixel!)
            for (R_int i = 0; i < 128 * 128; i += 32) {
                for (R_int j = 31; j >= 0; j--) {
                    const int bit_check = (default_font_data[counter]) & (1u << j);
                    if (bit_check) font_pixels[i + j] = R_white;
                }

                counter++;

                if (counter > 512) counter = 0;// Security check...
            }

            R_bool format_success = R_format_pixels(
                    font_pixels, 128 * 128 * sizeof(R_color), R_pixel_format_r8g8b8a8,
                    R_ctx.default_font_buffers.pixels, 128 * 128 * 2, R_pixel_format_gray_alpha);

            R_assert(format_success);
        }

        R_image font_image = {
                .data = R_ctx.default_font_buffers.pixels,
                .format = R_pixel_format_gray_alpha,
                .width = 128,
                .height = 128,
                .valid = 1,
        };

        R_ctx.default_font.texture = R_load_texture_from_image(font_image);

        // Allocate space for our characters info data
        R_ctx.default_font.glyphs = R_ctx.default_font_buffers.chars;

        int current_line = 0;
        int current_pos_x = chars_divisor;
        int test_pos_x = chars_divisor;
        int char_pixels_iter = 0;

        for (R_int i = 0; i < R_ctx.default_font.glyphs_count; i++) {
            R_ctx.default_font.glyphs[i].codepoint = 32 + i;// First char is 32

            R_ctx.default_font.glyphs[i].rec.x = (float) current_pos_x;
            R_ctx.default_font.glyphs[i].rec.y =
                    (float) (chars_divisor + current_line * (chars_height + chars_divisor));
            R_ctx.default_font.glyphs[i].rec.width = (float) chars_width[i];
            R_ctx.default_font.glyphs[i].rec.height = (float) chars_height;

            test_pos_x += (int) (R_ctx.default_font.glyphs[i].rec.width + (float) chars_divisor);

            if (test_pos_x >= R_ctx.default_font.texture.width) {
                current_line++;
                current_pos_x = 2 * chars_divisor + chars_width[i];
                test_pos_x = current_pos_x;

                R_ctx.default_font.glyphs[i].rec.x = (float) (chars_divisor);
                R_ctx.default_font.glyphs[i].rec.y =
                        (float) (chars_divisor + current_line * (chars_height + chars_divisor));
            } else
                current_pos_x = test_pos_x;

            // NOTE: On default font character offsets and xAdvance are not required
            R_ctx.default_font.glyphs[i].offset_x = 0;
            R_ctx.default_font.glyphs[i].offset_y = 0;
            R_ctx.default_font.glyphs[i].advance_x = 0;
        }

        R_ctx.default_font.base_size = (int) R_ctx.default_font.glyphs[0].rec.height;
        R_ctx.default_font.valid = 1;

        R_log(R_log_type_info, "[TEX ID %i] Default font loaded successfully",
              R_ctx.default_font.texture.id);
    }
#endif
}

#pragma region getters

// Get the default font, useful to be used with extended parameters
R_public R_font R_get_default_font() { return R_ctx.default_font; }

// Get default shader
R_public R_shader R_get_default_shader() { return R_ctx.default_shader; }

// Get default internal texture (white texture)
R_public R_texture2d R_get_default_texture() {
    R_texture2d texture = {0};
    texture.id = R_ctx.default_texture_id;
    texture.width = 1;
    texture.height = 1;
    texture.mipmaps = 1;
    texture.format = R_pixel_format_r8g8b8a8;

    return texture;
}

//Get the context pointer
R_public R_gfx_context *R_get_gfx_context() { return &R_ctx; }

// Get pixel data from GPU frontbuffer and return an R_image (screenshot)
R_public R_image R_get_screen_data(R_color *dst, R_int dst_size) {
    R_image image = {0};

    if (dst && dst_size == R_ctx.render_width * R_ctx.render_height) {
        R_gfx_read_screen_pixels(dst, R_ctx.render_width, R_ctx.render_height);

        image.data = dst;
        image.width = R_ctx.render_width;
        image.height = R_ctx.render_height;
        image.format = R_pixel_format_r8g8b8a8;
        image.valid = 1;
    }

    return image;
}

R_public R_log_type R_get_current_log_filter() { return R_ctx.logger_filter; }

#pragma endregion

#pragma region setters

// Define default texture used to draw shapes
R_public void R_set_shapes_texture(R_texture2d texture, R_rec source) {
    R_ctx.tex_shapes = texture;
    R_ctx.rec_tex_shapes = source;
}

// Set the global context pointer
R_public void R_set_global_gfx_context_pointer(R_gfx_context *ctx) {
    R__global_gfx_context_ptr = ctx;
}

// Set viewport for a provided width and height
R_public void R_set_viewport(int width, int height) {
    R_ctx.render_width = width;
    R_ctx.render_height = height;

    // Set viewport width and height
    R_gfx_viewport(0, 0, R_ctx.render_width, R_ctx.render_height);

    R_gfx_matrix_mode(RF_PROJECTION);// Switch to PROJECTION matrix
    R_gfx_load_identity();           // Reset current matrix (PROJECTION)

    // Set orthographic GL_PROJECTION to current framebuffer size, top-left corner is (0, 0)
    R_gfx_ortho(0, R_ctx.render_width, R_ctx.render_height, 0, 0.0f, 1.0f);

    R_gfx_matrix_mode(RF_MODELVIEW);// Switch back to MODELVIEW matrix
    R_gfx_load_identity();          // Reset current matrix (MODELVIEW)
}

R_public inline R_int R_libc_rand_wrapper(R_int min, R_int max) {
    return rand() % (max + 1 - min) + min;
}

#pragma endregion
