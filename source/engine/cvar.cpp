// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "cvar.hpp"

#include "engine/scripting/scripting.hpp"
#include "game_ui.hpp"

void InitGlobalDEF(GlobalDEF* _struct, bool openDebugUIs) {

    auto GlobalDEF = Scripting::get_singleton_ptr()->Lua->s_lua["global_def"];

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
        LoadLuaConfig(_struct, GlobalDEF, draw_console);
        LoadLuaConfig(_struct, GlobalDEF, draw_pack_editor);

    } else {
        METADOT_ERROR("Load GlobalDEF failed");
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

    METADOT_INFO("GlobalDEF loaded");
}

void LoadGlobalDEF(std::string globaldef_src) {}

void SaveGlobalDEF(std::string globaldef_src) {
    // std::string settings_data = "GetSettingsData = function()\nsettings_data = {}\n";
    // SaveLuaConfig(*_struct, "settings_data", settings_data);
    // settings_data += "return settings_data\nend";
    // std::ofstream o(globaldef_src);
    // o << settings_data;
}

template <>
char ME::cvar::Cast<char>(std::string s) {
    if (s.size() > 0)
        return s[0];
    else
        throw std::out_of_range("argument was empty");
}

template <typename T, typename Test>
void check_args_limit(T v) {
    if (v > std::numeric_limits<Test>::max()) throw std::out_of_range("above numeric limit");
    if (v < std::numeric_limits<Test>::min()) throw std::out_of_range("below numeric limit");
};

template <>
short ME::cvar::Cast<short>(std::string s) {
    int v = std::stoi(s);
    check_args_limit<int, short>(v);
    return (short)v;
}

template <>
bool ME::cvar::Cast<bool>(std::string s) {
    int v = std::stoi(s);
    return (bool)v;
}

template <>
const char* ME::cvar::Cast<const char*>(std::string s) {
    return s.c_str();
}

CVAR_CAST_DEF(int, std::stoi);
CVAR_CAST_DEF(long, std::stol);
CVAR_CAST_DEF(float, std::stof);
CVAR_CAST_DEF(double, std::stod);
CVAR_CAST_DEF(long double, std::stold);
CVAR_CAST_DEF(std::string, std::string);

ME::cvar::BaseCommand::BaseCommand(std::string name) : name(name) {}
ME::cvar::BaseCommand::~BaseCommand() {}

void ME::cvar::BaseCommand::AddParameter(const ME::cvar::CommandParameter& p) { params.push_back(p); }
std::string ME::cvar::BaseCommand::GetName() const { return name; }
ME::cvar::BaseCommand::CommandArgs::size_type ME::cvar::BaseCommand::size() const { return params.size(); }

ME::cvar::BaseCommand::iterator ME::cvar::BaseCommand::begin() { return params.begin(); }
ME::cvar::BaseCommand::iterator ME::cvar::BaseCommand::end() { return params.end(); }

ME::cvar::CommandParameter::CommandParameter(std::string type, std::string name) : type(type), name(name) {}

std::string ME::cvar::CommandParameter::GetName() const { return name; }
std::string ME::cvar::CommandParameter::GetType() const { return type; }

ME::cvar::ConVar::~ConVar() {
    for (auto& p : convars) delete p.second;
}

void ME::cvar::ConVar::RemoveCommand(std::string name) {
    auto it = convars.find(name);
    delete it->second;
    if (it == convars.end()) convars.erase(it);
}

std::string ME::cvar::ConVar::Call(std::string name, std::queue<std::string> args) {
    auto it = convars.find(name);
    if (it == convars.end()) throw std::exception(name.c_str());
    return (*it).second->Call(args);
}

ME::cvar::ConVar::const_iterator ME::cvar::ConVar::begin() const { return convars.begin(); }
ME::cvar::ConVar::const_iterator ME::cvar::ConVar::end() const { return convars.end(); }
// ME::cvar::BaseCommand::const_iterator ME::cvar::BaseCommand::cbegin() const { return params.cbegin(); }
// ME::cvar::BaseCommand::const_iterator ME::cvar::BaseCommand::cend() const { return params.cend(); }
