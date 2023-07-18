// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "cvar.hpp"

#include "engine/scripting/scripting.hpp"
#include "game_ui.hpp"

void InitGlobalDEF(GlobalDEF* s, bool openDebugUIs) {

    auto GlobalDEF = Scripting::get_singleton_ptr()->Lua->s_lua["global_def"];

    if (!GlobalDEF.is_nil_ref()) {
        s->draw_frame_graph = GlobalDEF["draw_frame_graph"].get<decltype(s->draw_frame_graph)>();
        s->draw_background = GlobalDEF["draw_background"].get<decltype(s->draw_background)>();
        s->draw_background_grid = GlobalDEF["draw_background_grid"].get<decltype(s->draw_background_grid)>();
        s->draw_load_zones = GlobalDEF["draw_load_zones"].get<decltype(s->draw_load_zones)>();
        s->draw_physics_debug = GlobalDEF["draw_physics_debug"].get<decltype(s->draw_physics_debug)>();
        s->draw_b2d_shape = GlobalDEF["draw_b2d_shape"].get<decltype(s->draw_b2d_shape)>();
        s->draw_b2d_joint = GlobalDEF["draw_b2d_joint"].get<decltype(s->draw_b2d_joint)>();
        s->draw_b2d_aabb = GlobalDEF["draw_b2d_aabb"].get<decltype(s->draw_b2d_aabb)>();
        s->draw_b2d_pair = GlobalDEF["draw_b2d_pair"].get<decltype(s->draw_b2d_pair)>();
        s->draw_b2d_centerMass = GlobalDEF["draw_b2d_centerMass"].get<decltype(s->draw_b2d_centerMass)>();
        s->draw_chunk_state = GlobalDEF["draw_chunk_state"].get<decltype(s->draw_chunk_state)>();
        s->draw_debug_stats = GlobalDEF["draw_debug_stats"].get<decltype(s->draw_debug_stats)>();
        s->draw_material_info = GlobalDEF["draw_material_info"].get<decltype(s->draw_material_info)>();
        s->draw_detailed_material_info = GlobalDEF["draw_detailed_material_info"].get<decltype(s->draw_detailed_material_info)>();
        s->draw_uinode_bounds = GlobalDEF["draw_uinode_bounds"].get<decltype(s->draw_uinode_bounds)>();
        s->draw_temperature_map = GlobalDEF["draw_temperature_map"].get<decltype(s->draw_temperature_map)>();
        s->draw_cursor = GlobalDEF["draw_cursor"].get<decltype(s->draw_cursor)>();
        s->ui_tweak = GlobalDEF["ui_tweak"].get<decltype(s->ui_tweak)>();
        s->draw_shaders = GlobalDEF["draw_shaders"].get<decltype(s->draw_shaders)>();
        s->water_overlay = GlobalDEF["water_overlay"].get<decltype(s->water_overlay)>();
        s->water_showFlow = GlobalDEF["water_showFlow"].get<decltype(s->water_showFlow)>();
        s->water_pixelated = GlobalDEF["water_pixelated"].get<decltype(s->water_pixelated)>();
        s->lightingQuality = GlobalDEF["lightingQuality"].get<decltype(s->lightingQuality)>();
        s->draw_light_overlay = GlobalDEF["draw_light_overlay"].get<decltype(s->draw_light_overlay)>();
        s->simpleLighting = GlobalDEF["simpleLighting"].get<decltype(s->simpleLighting)>();
        s->lightingEmission = GlobalDEF["lightingEmission"].get<decltype(s->lightingEmission)>();
        s->lightingDithering = GlobalDEF["lightingDithering"].get<decltype(s->lightingDithering)>();
        s->tick_world = GlobalDEF["tick_world"].get<decltype(s->tick_world)>();
        s->tick_box2d = GlobalDEF["tick_box2d"].get<decltype(s->tick_box2d)>();
        s->tick_temperature = GlobalDEF["tick_temperature"].get<decltype(s->tick_temperature)>();
        s->hd_objects = GlobalDEF["hd_objects"].get<decltype(s->hd_objects)>();
        s->hd_objects_size = GlobalDEF["hd_objects_size"].get<decltype(s->hd_objects_size)>();
        s->draw_ui_debug = GlobalDEF["draw_ui_debug"].get<decltype(s->draw_ui_debug)>();
        s->draw_imgui_debug = GlobalDEF["draw_imgui_debug"].get<decltype(s->draw_imgui_debug)>();
        s->draw_profiler = GlobalDEF["draw_profiler"].get<decltype(s->draw_profiler)>();
        s->draw_console = GlobalDEF["draw_console"].get<decltype(s->draw_console)>();
        s->draw_pack_editor = GlobalDEF["draw_pack_editor"].get<decltype(s->draw_pack_editor)>();

        s->cell_iter = GlobalDEF["cell_iter"].get<int>();

    } else {
        METADOT_ERROR("Load GlobalDEF failed");
    }

    gameUI.visible_debugdraw = openDebugUIs;
    s->draw_frame_graph = openDebugUIs;
    if (!openDebugUIs) {
        s->draw_background = true;
        s->draw_background_grid = false;
        s->draw_load_zones = false;
        s->draw_physics_debug = false;
        s->draw_chunk_state = false;
        s->draw_debug_stats = false;
        s->draw_detailed_material_info = false;
        s->draw_temperature_map = false;
    }

    METADOT_INFO("GlobalDEF loaded");
}

void LoadGlobalDEF(std::string globaldef_src) {}

void SaveGlobalDEF(std::string globaldef_src) {
    // std::string settings_data = "GetSettingsData = function()\nsettings_data = {}\n";
    // SaveLuaConfig(*s, "settings_data", settings_data);
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
