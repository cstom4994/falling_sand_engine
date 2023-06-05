
#ifndef ME_CVAR_HPP
#define ME_CVAR_HPP

#include "engine/core/core.hpp"
#include "engine/core/cpp/type.hpp"
#include "engine/game_utils/jsonwarp.h"
#include "engine/meta/reflection.hpp"

struct GlobalDEF {
    bool draw_frame_graph;
    bool draw_background;
    bool draw_background_grid;
    bool draw_load_zones;
    bool draw_physics_debug;
    bool draw_b2d_shape;
    bool draw_b2d_joint;
    bool draw_b2d_aabb;
    bool draw_b2d_pair;
    bool draw_b2d_centerMass;
    bool draw_chunk_state;
    bool draw_debug_stats;
    bool draw_material_info;
    bool draw_detailed_material_info;
    bool draw_uinode_bounds;
    bool draw_temperature_map;
    bool draw_cursor;

    bool ui_tweak;

    bool draw_shaders;
    int water_overlay;
    bool water_showFlow;
    bool water_pixelated;
    f32 lightingQuality;
    bool draw_light_overlay;
    bool simpleLighting;
    bool lightingEmission;
    bool lightingDithering;

    bool tick_world;
    bool tick_box2d;
    bool tick_temperature;
    bool hd_objects;

    int hd_objects_size;

    bool draw_ui_debug;
    bool draw_imgui_debug;
    bool draw_profiler;
    bool draw_console;
    bool draw_pack_editor;
};
METADOT_STRUCT(GlobalDEF, draw_frame_graph, draw_background, draw_background_grid, draw_load_zones, draw_physics_debug, draw_b2d_shape, draw_b2d_joint, draw_b2d_aabb, draw_b2d_pair,
               draw_b2d_centerMass, draw_chunk_state, draw_debug_stats, draw_material_info, draw_detailed_material_info, draw_uinode_bounds, draw_temperature_map, draw_cursor, ui_tweak, draw_shaders,
               water_overlay, water_showFlow, water_pixelated, lightingQuality, draw_light_overlay, simpleLighting, lightingEmission, lightingDithering, tick_world, tick_box2d, tick_temperature,
               hd_objects, hd_objects_size, draw_ui_debug, draw_imgui_debug, draw_profiler, draw_console, draw_pack_editor);

void InitGlobalDEF(GlobalDEF *_struct, bool openDebugUIs);
void LoadGlobalDEF(std::string globaldef_src);
void SaveGlobalDEF(std::string globaldef_src);

namespace ME::cvar {

template <typename T>
T Cast(std::string) {
    throw no_cast_available(0, "");
}

#define CVAR_CAST_DEF(_type, _func)              \
    template <>                                  \
    _type ME::cvar::Cast<_type>(std::string s) { \
        return _func(s);                         \
    }

#define CVAR_CAST_DECL(_type) \
    template <>               \
    _type Cast<_type>(std::string s)

CVAR_CAST_DECL(char);
CVAR_CAST_DECL(short);
CVAR_CAST_DECL(int);
CVAR_CAST_DECL(long);
CVAR_CAST_DECL(float);
CVAR_CAST_DECL(double);
CVAR_CAST_DECL(long double);
CVAR_CAST_DECL(std::string);
CVAR_CAST_DECL(const char *);

template <typename T>
std::string NameOFType() {
    std::string_view name_of_type = MetaEngine::type_name<T>().View();
    std::string name_of_type_str = std::string(name_of_type.data(), name_of_type.size());
    return name_of_type_str;
}
}  // namespace ME::cvar

#endif