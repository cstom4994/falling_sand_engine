// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_PLAYER_HPP
#define ME_PLAYER_HPP

#include "engine/game_datastruct.hpp"
#include "engine/physics/physics_math.hpp"
#include "game/items.hpp"

#pragma region Rigidbody

class RigidBody {
private:
    C_Surface *surface = nullptr;
    R_Image *texture = nullptr;

public:
    std::string name;

    ME::phy::Body *body = nullptr;

    int matWidth = 0;
    int matHeight = 0;
    MaterialInstance *tiles = nullptr;

    // hitbox needs update
    bool needsUpdate = false;

    // surface needs to be converted to texture
    bool texNeedsUpdate = false;

    int weldX = -1;
    int weldY = -1;
    bool back = false;
    std::list<TPPLPoly> outline;
    std::list<TPPLPoly> outline2;
    f32 hover = 0;

    Item *item = nullptr;

public:
    RigidBody(ME::phy::Body *body, std::string name = "unknown");
    ~RigidBody();

    bool set_surface(C_Surface *sur);
    C_Surface *get_surface();

    bool set_texture(R_Image *tex);
    R_Image *get_texture();

    void clean();
};

template <>
struct ME::meta::static_refl::TypeInfo<RigidBody> : TypeInfoBase<RigidBody> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("matWidth"), &Type::matWidth},
            Field{TSTR("matHeight"), &Type::matHeight},
            Field{TSTR("needsUpdate"), &Type::needsUpdate},
            Field{TSTR("texNeedsUpdate"), &Type::texNeedsUpdate},
            Field{TSTR("weldX"), &Type::weldX},
            Field{TSTR("weldY"), &Type::weldY},
            Field{TSTR("back"), &Type::back},
            Field{TSTR("hover"), &Type::hover},
            // Field{TSTR("item"), &Type::item},
    };
};

ME_GUI_DEFINE_BEGIN(template <>, RigidBody)
ImGui::Text("RigidBody: %s", var.name.c_str());
ImGui::Text("matWidth: %d", var.matWidth);
ImGui::Text("matHeight: %d", var.matHeight);
ImGui::Text("weldX: %d", var.weldX);
ImGui::Text("weldY: %d", var.weldY);
ImGui::Text("needsUpdate: %s", BOOL_STRING(var.needsUpdate));
ImGui::Text("texNeedsUpdate: %s", BOOL_STRING(var.texNeedsUpdate));
ME_GUI_DEFINE_END

using RigidBodyPtr = ME::ref<RigidBody>;

struct RigidBodyBinding : public ME::LuaWrapper::PodBind::Binding<RigidBodyBinding, RigidBody> {
    static constexpr const char *class_name = "RigidBody";

    static luaL_Reg *members() {
        static luaL_Reg members[] = {{nullptr, nullptr}};
        return members;
    }

    static ME::LuaWrapper::PodBind::bind_properties *properties() {
        static ME::LuaWrapper::PodBind::bind_properties properties[] = {{"name", get_name, set_name}, {nullptr, nullptr, nullptr}};
        return properties;
    }

    // Lua constructor
    static int create(lua_State *L) {
        ME::println("Create called");
        ME::LuaWrapper::PodBind::CheckArgCount(L, 2);
        // b2Body *body = (b2Body *)lua_touserdata(L, 1);
        const char *name = luaL_checkstring(L, 2);
        RigidBodyPtr sp = ME::create_ref<RigidBody>(nullptr, name);
        push(L, sp);
        return 1;
    }

    // Method glue functions
    //

    // static int walk(lua_State *L) {
    //     LuaWrapper::PodBind::CheckArgCount(L, 1);
    //     RigidBodyPtr a = fromStack(L, 1);
    //     a->walk();
    //     return 0;
    // }

    // static int setName(lua_State *L) {
    //     LuaWrapper::PodBind::CheckArgCount(L, 2);
    //     RigidBodyPtr a = fromStack(L, 1);
    //     const char *name = lua_tostring(L, 2);
    //     a->setName(name);
    //     return 0;
    // }

    // Propertie getters and setters

    // 1 - class metatable
    // 2 - key
    static int get_name(lua_State *L) {
        ME::LuaWrapper::PodBind::CheckArgCount(L, 2);
        RigidBodyPtr a = fromStack(L, 1);
        lua_pushstring(L, a->name.c_str());
        return 1;
    }

    // 1 - class metatable
    // 2 - key
    // 3 - value
    static int set_name(lua_State *L) {
        ME::LuaWrapper::PodBind::CheckArgCount(L, 3);
        RigidBodyPtr a = fromStack(L, 1);
        a->name = lua_tostring(L, 3);
        return 0;
    }
};

#pragma endregion Rigidbody

#pragma region Player

typedef enum EnumPlayerHoldType {
    None = 0,
    Hammer = 1,
    Vacuum,
} EnumPlayerHoldType;

template <>
struct ME::meta::static_refl::TypeInfo<EnumPlayerHoldType> : TypeInfoBase<EnumPlayerHoldType> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("None"), Type::None},
            Field{TSTR("Hammer"), Type::Hammer},
            Field{TSTR("Vacuum"), Type::Vacuum},
    };
};

ME_GUI_DEFINE_BEGIN(template <>, EnumPlayerHoldType)
ImGui::Text("EnumPlayerHoldType: %s", ME::meta::static_refl::TypeInfo<EnumPlayerHoldType>::fields.NameOfValue(var).data());
ME_GUI_DEFINE_END

struct Controlable {};

class Player {
public:
    Item *heldItem = nullptr;
    f32 holdAngle = 0;
    i64 startThrow = 0;
    EnumPlayerHoldType holdtype = None;
    int hammerX = 0;
    int hammerY = 0;

    void render(WorldEntity *we, R_Target *target, int ofsX, int ofsY);
    void renderLQ(WorldEntity *we, R_Target *target, int ofsX, int ofsY);
    void setItemInHand(WorldEntity *we, Item *item, World *world);

    Player(const Player &p) = default;

    Player();
    ~Player();
};

template <>
struct ME::meta::static_refl::TypeInfo<Player> : TypeInfoBase<Player> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("heldItem"), &Type::heldItem},
            Field{TSTR("holdAngle"), &Type::holdAngle},
            Field{TSTR("startThrow"), &Type::startThrow},
            Field{TSTR("holdtype"), &Type::holdtype},
            Field{TSTR("hammerX"), &Type::hammerX},
            Field{TSTR("hammerY"), &Type::hammerY},

            Field{TSTR("render"), static_cast<void (Type::*)(WorldEntity *we, R_Target *target, int ofsX, int ofsY) /* const */>(&Type::render)},
            Field{TSTR("renderLQ"), static_cast<void (Type::*)(WorldEntity *we, R_Target *target, int ofsX, int ofsY) /* const */>(&Type::renderLQ)},
            Field{TSTR("setItemInHand"), static_cast<void (Type::*)(WorldEntity *we, Item *item, World *world) /* const */>(&Type::setItemInHand)},
    };
};

ME_GUI_DEFINE_BEGIN(template <>, Player)
ME::meta::static_refl::TypeInfo<Player>::ForEachVarOf(var, [&](const auto &field, auto &&value) { ImGui::Auto(value, std::string(field.name)); });
ME_GUI_DEFINE_END

struct move_player_event {
    f32 dt;
    f32 thruTick;
    Game *g;
};

struct entity_update_event {
    Game *g;
};

class ControableSystem : public ME::ECS::system<move_player_event> {
public:
    void process(ME::ECS::registry &world, const move_player_event &evt) override;
};

class WorldEntitySystem : public ME::ECS::system<entity_update_event> {
public:
    void process(ME::ECS::registry &world, const entity_update_event &evt) override;
};

#pragma endregion Player

#endif