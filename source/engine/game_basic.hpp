// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_GAMESCRIPTWRAP_HPP
#define ME_GAMESCRIPTWRAP_HPP

#include <cstddef>
#include <string>
#include <utility>

#include "engine/scripting/scripting.hpp"
#include "engine/ui/imgui_utils.hpp"

namespace ME {

struct Biome;

MAKE_ENUM_FLAGS(SystemFlags, int){
        Default = 1 << 0,
        GamePlay = 1 << 1,
        ImGui = 1 << 2,
        Render = 1 << 3,
};

class IGameSystem {
protected:
    std::string name;
    SystemFlags flags = SystemFlags::Default;

public:
    u32 priority;

public:
    IGameSystem(u32 p = -1, SystemFlags f = SystemFlags::Default, std::string n = "unknown system") {
        if (p != -1) {
            priority = p;
        }
        setFlag(f);
        name = n;
    };
    ~IGameSystem(){};

    void setFlag(SystemFlags f) { flags |= f; }
    bool getFlag(SystemFlags f) { return static_cast<bool>(flags & f); }

    const std::string &getName() const { return name; }

    virtual void create() = 0;
    virtual void destory() = 0;
    virtual void reload() = 0;

    // Register Lua always been called before Create()
    virtual void registerLua(lua_wrapper::State &s_lua) = 0;
};

template <>
struct meta::static_refl::TypeInfo<IGameSystem> : TypeInfoBase<IGameSystem> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {};
};

#define REGISTER_SYSTEM(name) name(u32 p, SystemFlags f = SystemFlags::Default) : IGameSystem(p, f, #name){};

class gameplay : public IGameSystem {
public:
    REGISTER_SYSTEM(gameplay)

    void create() override;
    void destory() override;
    void reload() override;
    void registerLua(lua_wrapper::State &s_lua) override;
};

}  // namespace ME

ME_GUI_DEFINE_BEGIN(template <>, ME::IGameSystem)
ImGui::Text("%s %d", var.getName().c_str(), var.priority);
ME_GUI_DEFINE_END

#endif