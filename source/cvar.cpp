
#include "cvar.hpp"

#include "scripting/scripting.hpp"
#include "game_ui.hpp"

void InitGlobalDEF(GlobalDEF *_struct, bool openDebugUIs) {

    auto GlobalDEF = Scripting::GetSingletonPtr()->Lua->s_lua["global_def"];

    if (!GlobalDEF.isNilref()) {
        LoadLuaConfig(_struct, GlobalDEF, draw_frame_graph);
        LoadLuaConfig(_struct, GlobalDEF, draw_background);
        LoadLuaConfig(_struct, GlobalDEF, draw_background_grid);
        LoadLuaConfig(_struct, GlobalDEF, draw_load_zones);
        LoadLuaConfig(_struct, GlobalDEF, draw_physics_debug);
        LoadLuaConfig(_struct, GlobalDEF, draw_b2d_shape);
        LoadLuaConfig(_struct, GlobalDEF, draw_b2d_joint);
        LoadLuaConfig(_struct, GlobalDEF, draw_b2d_aabb);
        LoadLuaConfig(_struct, GlobalDEF, draw_b2d_pair);
        LoadLuaConfig(_struct, GlobalDEF, draw_b2d_centerMass);
        LoadLuaConfig(_struct, GlobalDEF, draw_chunk_state);
        LoadLuaConfig(_struct, GlobalDEF, draw_debug_stats);
        LoadLuaConfig(_struct, GlobalDEF, draw_material_info);
        LoadLuaConfig(_struct, GlobalDEF, draw_detailed_material_info);
        LoadLuaConfig(_struct, GlobalDEF, draw_uinode_bounds);
        LoadLuaConfig(_struct, GlobalDEF, draw_temperature_map);
        LoadLuaConfig(_struct, GlobalDEF, draw_cursor);
        LoadLuaConfig(_struct, GlobalDEF, ui_tweak);
        LoadLuaConfig(_struct, GlobalDEF, draw_shaders);
        LoadLuaConfig(_struct, GlobalDEF, water_overlay);
        LoadLuaConfig(_struct, GlobalDEF, water_showFlow);
        LoadLuaConfig(_struct, GlobalDEF, water_pixelated);
        LoadLuaConfig(_struct, GlobalDEF, lightingQuality);
        LoadLuaConfig(_struct, GlobalDEF, draw_light_overlay);
        LoadLuaConfig(_struct, GlobalDEF, simpleLighting);
        LoadLuaConfig(_struct, GlobalDEF, lightingEmission);
        LoadLuaConfig(_struct, GlobalDEF, lightingDithering);
        LoadLuaConfig(_struct, GlobalDEF, tick_world);
        LoadLuaConfig(_struct, GlobalDEF, tick_box2d);
        LoadLuaConfig(_struct, GlobalDEF, tick_temperature);
        LoadLuaConfig(_struct, GlobalDEF, hd_objects);
        LoadLuaConfig(_struct, GlobalDEF, hd_objects_size);
        LoadLuaConfig(_struct, GlobalDEF, draw_ui_debug);
        LoadLuaConfig(_struct, GlobalDEF, draw_imgui_debug);
        LoadLuaConfig(_struct, GlobalDEF, draw_profiler);

    } else {
        METADOT_ERROR("GlobalDEF WAS NULL");
    }

    gameUI.visible_debugdraw = openDebugUIs;
    _struct->draw_frame_graph = openDebugUIs;
    if (!openDebugUIs) {
        _struct->draw_background = true;
        _struct->draw_background_grid = false;
        _struct->draw_load_zones = false;
        _struct->draw_physics_debug = false;
        _struct->draw_chunk_state = false;
        _struct->draw_debug_stats = false;
        _struct->draw_detailed_material_info = false;
        _struct->draw_temperature_map = false;
    }

    METADOT_INFO("SettingsData loaded");
}

void LoadGlobalDEF(std::string globaldef_src) {}

void SaveGlobalDEF(std::string globaldef_src) {
    // std::string settings_data = "GetSettingsData = function()\nsettings_data = {}\n";
    // SaveLuaConfig(*_struct, "settings_data", settings_data);
    // settings_data += "return settings_data\nend";
    // std::ofstream o(globaldef_src);
    // o << settings_data;
}