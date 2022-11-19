// Copyright(c) 2022, KaoruXun All rights reserved.


#include "Settings.hpp"

bool SettingsBase::draw_frame_graph = true;
bool SettingsBase::draw_background = true;
bool SettingsBase::draw_background_grid = false;
bool SettingsBase::draw_load_zones = false;
bool SettingsBase::draw_physics_debug = false;
bool SettingsBase::draw_b2d_shape = true;
bool SettingsBase::draw_b2d_joint = false;
bool SettingsBase::draw_b2d_aabb = false;
bool SettingsBase::draw_b2d_pair = false;
bool SettingsBase::draw_b2d_centerMass = true;
bool SettingsBase::draw_chunk_state = false;
bool SettingsBase::draw_debug_stats = false;
bool SettingsBase::draw_material_info = true;
bool SettingsBase::draw_detailed_material_info = true;
bool SettingsBase::draw_temperature_map = false;
bool SettingsBase::draw_cursor = false;

bool SettingsBase::ui_tweak = false;
bool SettingsBase::ui_code_editor = false;
bool SettingsBase::ui_inspector = false;
bool SettingsBase::ui_gcmanager = false;
bool SettingsBase::ui_console = false;

bool SettingsBase::draw_shaders = true;
int SettingsBase::water_overlay = 0;
bool SettingsBase::water_showFlow = true;
bool SettingsBase::water_pixelated = false;
float SettingsBase::lightingQuality = 0.5f;
bool SettingsBase::draw_light_overlay = false;
bool SettingsBase::simpleLighting = false;
bool SettingsBase::lightingEmission = true;
bool SettingsBase::lightingDithering = false;

bool SettingsBase::tick_world = true;
bool SettingsBase::tick_box2d = true;
bool SettingsBase::tick_temperature = true;
bool SettingsBase::hd_objects = false;

int SettingsBase::hd_objects_size = 3;

std::string SettingsBase::server_ip = "127.0.0.1";
int SettingsBase::server_port = 25555;