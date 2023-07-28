// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "cvar.hpp"

#include "engine/scripting/scripting.hpp"
#include "game_ui.hpp"

namespace ME {

using namespace std::literals;

void InitGlobalDEF(GlobalDEF* s, bool open_debugui) {

    meta::r::class_<GlobalDEF>({{"info", "全局变量"s}})
            .member_("draw_frame_graph", &GlobalDEF::draw_frame_graph, {.metadata{{"info", "是否显示帧率图"s}}})
            .member_("draw_background", &GlobalDEF::draw_background, {.metadata{{"info", "是否绘制背景"s}}})
            .member_("draw_background_grid", &GlobalDEF::draw_background_grid, {.metadata{{"info", "是否绘制背景网格"s}}})
            .member_("draw_load_zones", &GlobalDEF::draw_load_zones, {.metadata{{"info", "是否显示加载区域"s}}})
            .member_("draw_physics_debug", &GlobalDEF::draw_physics_debug, {.metadata{{"info", "是否开启物理调试"s}}})
            .member_("draw_b2d_shape", &GlobalDEF::draw_b2d_shape, {.metadata{{"info", ""s}}})
            .member_("draw_b2d_joint", &GlobalDEF::draw_b2d_joint, {.metadata{{"info", ""s}}})
            .member_("draw_b2d_aabb", &GlobalDEF::draw_b2d_aabb, {.metadata{{"info", ""s}}})
            .member_("draw_b2d_pair", &GlobalDEF::draw_b2d_pair, {.metadata{{"info", ""s}}})
            .member_("draw_b2d_centerMass", &GlobalDEF::draw_b2d_centerMass, {.metadata{{"info", ""s}}})
            .member_("draw_chunk_state", &GlobalDEF::draw_chunk_state, {.metadata{{"info", "是否显示区块状态"s}}})
            .member_("draw_debug_stats", &GlobalDEF::draw_debug_stats, {.metadata{{"info", "是否显示调试信息"s}}})
            .member_("draw_material_info", &GlobalDEF::draw_material_info, {.metadata{{"info", "是否显示材质信息"s}}})
            .member_("draw_detailed_material_info", &GlobalDEF::draw_detailed_material_info, {.metadata{{"info", "是否显示材质详细信息"s}}})
            .member_("draw_uinode_bounds", &GlobalDEF::draw_uinode_bounds, {.metadata{{"info", "是否绘制UINODE"s}}})
            .member_("draw_temperature_map", &GlobalDEF::draw_temperature_map, {.metadata{{"info", "是否绘制温度图"s}}})
            .member_("draw_cursor", &GlobalDEF::draw_cursor, {.metadata{{"info", "是否显示鼠标指针"s}}})
            .member_("ui_tweak", &GlobalDEF::ui_tweak, {.metadata{{"info", "是否打开TWEAK界面"s}}})
            .member_("draw_shaders", &GlobalDEF::draw_shaders, {.metadata{{"info", "是否启用光影"s}}})
            .member_("water_overlay", &GlobalDEF::water_overlay, {.metadata{{"info", "水渲染覆盖"s}}})
            .member_("water_showFlow", &GlobalDEF::water_showFlow, {.metadata{{"info", "是否绘制水渲染流程"s}}})
            .member_("water_pixelated", &GlobalDEF::water_pixelated, {.metadata{{"info", "启用水渲染像素化"s}}})
            .member_("lightingQuality", &GlobalDEF::lightingQuality, {.metadata{{"info", "光照质量"s}, {"imgui", "float_range"s}, {"max", 1.0f}, {"min", 0.0f}}})
            .member_("draw_light_overlay", &GlobalDEF::draw_light_overlay, {.metadata{{"info", "是否启用光照覆盖"s}}})
            .member_("simpleLighting", &GlobalDEF::simpleLighting, {.metadata{{"info", "是否启用光照简单采样"s}}})
            .member_("lightingEmission", &GlobalDEF::lightingEmission, {.metadata{{"info", "是否启用光照放射"s}}})
            .member_("lightingDithering", &GlobalDEF::lightingDithering, {.metadata{{"info", "是否启用光照抖动"s}}})
            .member_("tick_world", &GlobalDEF::tick_world, {.metadata{{"info", "是否启用世界更新"s}}})
            .member_("tick_box2d", &GlobalDEF::tick_box2d, {.metadata{{"info", "是否启用刚体物理更新"s}}})
            .member_("tick_temperature", &GlobalDEF::tick_temperature, {.metadata{{"info", "是否启用世界温度更新"s}}})
            .member_("hd_objects", &GlobalDEF::hd_objects, {.metadata{{"info", ""s}}})
            .member_("hd_objects_size", &GlobalDEF::hd_objects_size, {.metadata{{"info", ""s}}})
            .member_("draw_ui_debug", &GlobalDEF::draw_ui_debug, {.metadata{{"info", ""s}}})
            .member_("draw_imgui_debug", &GlobalDEF::draw_imgui_debug, {.metadata{{"info", "是否显示IMGUI示例窗口"s}}})
            .member_("draw_profiler", &GlobalDEF::draw_profiler, {.metadata{{"info", "是否显示帧检查器"s}}})
            .member_("draw_console", &GlobalDEF::draw_console, {.metadata{{"info", "是否显示控制台"s}}})
            .member_("draw_pack_editor", &GlobalDEF::draw_pack_editor, {.metadata{{"info", "是否显示包编辑器"s}}})
            .member_("draw_code_editor", &GlobalDEF::draw_code_editor, {.metadata{{"info", "是否显示脚本编辑器"s}}})
            .member_("cell_iter", &GlobalDEF::cell_iter, {.metadata{{"info", "Cell迭代次数"s}}})
            .member_("brush_size", &GlobalDEF::brush_size, {.metadata{{"info", "编辑器笔刷大小"s}}});

    auto GlobalDEF = the<scripting>().s_lua["global_def"];

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
        s->brush_size = GlobalDEF["brush_size"].get<int>();

    } else {
        METADOT_ERROR("Load GlobalDEF failed");
    }

    gameUI.visible_debugdraw = open_debugui;
    s->draw_frame_graph = open_debugui;
    if (!open_debugui) {
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
char cvar::Cast<char>(std::string s) {
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
short cvar::Cast<short>(std::string s) {
    int v = std::stoi(s);
    check_args_limit<int, short>(v);
    return (short)v;
}

template <>
bool cvar::Cast<bool>(std::string s) {
    int v = std::stoi(s);
    return (bool)v;
}

template <>
const char* cvar::Cast<const char*>(std::string s) {
    return s.c_str();
}

CVAR_CAST_DEF(int, std::stoi);
CVAR_CAST_DEF(long, std::stol);
CVAR_CAST_DEF(float, std::stof);
CVAR_CAST_DEF(double, std::stod);
CVAR_CAST_DEF(long double, std::stold);
CVAR_CAST_DEF(std::string, std::string);

cvar::BaseCommand::BaseCommand(std::string name) : name(name) {}
cvar::BaseCommand::~BaseCommand() {}

void cvar::BaseCommand::AddParameter(const cvar::CommandParameter& p) { params.push_back(p); }
std::string cvar::BaseCommand::GetName() const { return name; }
cvar::BaseCommand::CommandArgs::size_type cvar::BaseCommand::size() const { return params.size(); }

cvar::BaseCommand::iterator cvar::BaseCommand::begin() { return params.begin(); }
cvar::BaseCommand::iterator cvar::BaseCommand::end() { return params.end(); }

cvar::CommandParameter::CommandParameter(std::string type, std::string name) : type(type), name(name) {}

std::string cvar::CommandParameter::GetName() const { return name; }
std::string cvar::CommandParameter::GetType() const { return type; }

cvar::ConVar::~ConVar() {
    for (auto& p : convars) delete p.second;
}

void cvar::ConVar::RemoveCommand(std::string name) {
    auto it = convars.find(name);
    delete it->second;
    if (it == convars.end()) convars.erase(it);
}

std::string cvar::ConVar::Call(std::string name, std::queue<std::string> args) {
    auto it = convars.find(name);
    if (it == convars.end()) throw std::exception(name.c_str());
    return (*it).second->Call(args);
}

cvar::ConVar::const_iterator cvar::ConVar::begin() const { return convars.begin(); }
cvar::ConVar::const_iterator cvar::ConVar::end() const { return convars.end(); }
// cvar::BaseCommand::const_iterator cvar::BaseCommand::cbegin() const { return params.cbegin(); }
// cvar::BaseCommand::const_iterator cvar::BaseCommand::cend() const { return params.cend(); }

}  // namespace ME