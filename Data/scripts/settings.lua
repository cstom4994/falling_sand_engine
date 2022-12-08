-- Copyright(c) 2022, KaoruXun All rights reserved.
GetSettingsData = function()
    settings_data = {}
    settings_data.draw_frame_graph = true
    settings_data.draw_background = true
    settings_data.draw_background_grid = false
    settings_data.draw_load_zones = false
    settings_data.draw_physics_debug = false
    settings_data.draw_b2d_shape = true
    settings_data.draw_b2d_joint = false
    settings_data.draw_b2d_aabb = false
    settings_data.draw_b2d_pair = false
    settings_data.draw_b2d_centerMass = true
    settings_data.draw_chunk_state = false
    settings_data.draw_debug_stats = false
    settings_data.draw_material_info = true
    settings_data.draw_detailed_material_info = true
    settings_data.draw_temperature_map = false
    settings_data.draw_cursor = true

    settings_data.ui_tweak = false

    settings_data.draw_shaders = true
    settings_data.water_overlay = 0
    settings_data.water_showFlow = true
    settings_data.water_pixelated = false
    settings_data.lightingQuality = 0.5
    settings_data.draw_light_overlay = false
    settings_data.simpleLighting = false
    settings_data.lightingEmission = true
    settings_data.lightingDithering = false

    settings_data.tick_world = true
    settings_data.tick_box2d = true
    settings_data.tick_temperature = true
    settings_data.hd_objects = false

    settings_data.hd_objects_size = 3

    settings_data.networkMode = -1
    settings_data.server_ip = "127.0.0.1"
    settings_data.server_port = 25555

    return settings_data
end
