// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Game/Game.hpp"

Game::Game() {}
Game::~Game() {}

// Includes Sokol GFX, Sokol GP and Sokol APP, doing all implementations.
// #define SOKOL_IMPL
#include "Libs/sokol/sokol_app.h"
#include "Libs/sokol/sokol_gfx.h"
#include "Libs/sokol/sokol_glue.h"
#include "Libs/sokol/sokol_gp.h"
//#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define SOKOL_IMGUI_IMPL
#include "Libs/ImGui/cimgui.h"
#include "Libs/sokol/sokol_imgui.h"

#include <cmath>
#include <cstdlib>

// Called on every frame of the application.
static void frame(void) {
    // Get current window size.
    double frame_time = sapp_frame_duration() * 1000.0;
    int width = sapp_width(), height = sapp_height();
    float center_width = width / 2;
    float center_height = height / 2;

    // Begin recording draw commands for a frame buffer of size (width, height).
    sgp_begin(width, height);
    // Set frame buffer drawing region to (0,0,width,height).
    sgp_viewport(0, 0, width, height);
    // Set drawing coordinate space to (left=-ratio, right=ratio, top=1, bottom=-1).

    // Clear the frame buffer.
    sgp_set_color(0.1f, 0.1f, 0.1f, 1.0f);
    sgp_clear();

    // Draw an animated rectangle that rotates and changes its colors.
    float time = sapp_frame_count() * sapp_frame_duration();
    float r = sinf(time) * 0.5 + 0.5, g = cosf(time) * 0.5 + 0.5;

    sgp_set_color(r, g, 0.3f, 1.0f);
    sgp_push_transform();
    sgp_rotate_at(time, center_width, center_height);
    sgp_draw_filled_rect(center_width - 100.0f, center_height - 100.0f, 200.0f, 200.0f);
    sgp_pop_transform();

    // Begin a render pass.
    sg_pass_action pass_action = {0};
    sg_begin_default_pass(&pass_action, width, height);
    // Dispatch all draw commands to Sokol GFX.
    sgp_flush();
    // Finish a draw command queue, clearing it.
    sgp_end();

    simgui_frame_desc_t frame_t = {
            .width = sapp_width(),
            .height = sapp_height(),
            .delta_time = sapp_frame_duration(),
            .dpi_scale = sapp_dpi_scale(),
    };

    simgui_new_frame(&frame_t);

    /*=== UI CODE STARTS HERE ===*/
    igSetNextWindowPos((ImVec2){10, 10}, ImGuiCond_Once, (ImVec2){0, 0});
    igSetNextWindowSize((ImVec2){400, 100}, ImGuiCond_Once);
    igBegin("Hello Dear ImGui!", 0, ImGuiWindowFlags_None);
    igText("Frame time: %.3f ms\n", frame_time);
    igEnd();
    /*=== UI CODE ENDS HERE ===*/

    simgui_render();
    // End render pass.
    sg_end_pass();
    // Commit Sokol render.
    sg_commit();
}

void event(const sapp_event *e) {
    simgui_handle_event(e);
}

// Called when the application is initializing.
static void init(void) {
    // Initialize Sokol GFX.
    sg_desc sgdesc = {.context = sapp_sgcontext()};
    sg_setup(&sgdesc);
    if (!sg_isvalid()) {
        fprintf(stderr, "Failed to create Sokol GFX context!\n");
        exit(-1);
    }

    // Initialize Sokol GP, adjust the size of command buffers for your own use.
    sgp_desc sgpdesc = {0};
    sgp_setup(&sgpdesc);
    if (!sgp_is_valid()) {
        fprintf(stderr, "Failed to create Sokol GP context: %s\n", sgp_get_error_message(sgp_get_last_error()));
        exit(-1);
    }

    simgui_desc_t imgui_t = {
            .sample_count = 4,
    };

    simgui_setup(&imgui_t);
}

// Called when the application is shutting down.
static void cleanup(void) {
    // Cleanup Sokol GP, Sokol Imgui and Sokol GFX resources.
    sgp_shutdown();
    simgui_shutdown();
    sg_shutdown();
}

// Implement application main through Sokol APP.
sapp_desc sokol_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    return (sapp_desc){
            .init_cb = init,
            .frame_cb = frame,
            .cleanup_cb = cleanup,
            .event_cb = event,
            .window_title = "MetaDot Sokol renderer test",
            .sample_count = 4,// Enable anti aliasing.
    };
}


int Game::init(int argc, char *argv[]) {
    return 0;
}
int Game::run(int argc, char *argv[]) {
    return 0;
}