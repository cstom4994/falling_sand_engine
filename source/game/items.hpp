// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_ITEMS_HPP
#define ME_ITEMS_HPP

#include "engine/core/core.hpp"
#include "engine/core/mathlib.hpp"
#include "engine/core/sdl_wrapper.h"
#include "engine/game_datastruct.hpp"
#include "engine/meta/static_relfection.hpp"
#include "engine/scripting/lua_wrapper.hpp"
#include "engine/ui/imgui_helper.hpp"
#include "engine/utils/enum.hpp"
#include "engine/utils/type.hpp"

// MAKE_ENUM_FLAGS(ItemFlags, int){
//         ItemFlags_None = 1 << 0,   ItemFlags_Rigidbody = 1 << 1, ItemFlags_Fluid_Container = 1 << 2, ItemFlags_Tool = 1 << 3,
//         ItemFlags_Chisel = 1 << 4, ItemFlags_Hammer = 1 << 5,    ItemFlags_Vacuum = 1 << 6,
// };

enum class ItemFlags : u64 {
    ItemFlags_None = 1 << 0,
    ItemFlags_Rigidbody = 1 << 1,
    ItemFlags_Fluid_Container = 1 << 2,
    ItemFlags_Tool = 1 << 3,
    ItemFlags_Chisel = 1 << 4,
    ItemFlags_Hammer = 1 << 5,
    ItemFlags_Vacuum = 1 << 6,
};

template <>
struct ME::meta::doenum::customize::enum_range<ItemFlags> {
    static constexpr bool is_flags = true;
};

// template <>
// struct ME::meta::static_refl::TypeInfo<ItemFlags> : TypeInfoBase<ItemFlags> {
//     static constexpr AttrList attrs = {};
//     static constexpr FieldList fields = {
//             Field{TSTR("None"), Type::ItemFlags_None},     Field{TSTR("Rigidbody"), Type::ItemFlags_Rigidbody}, Field{TSTR("Fluid_Container"), Type::ItemFlags_Fluid_Container},
//             Field{TSTR("Tool"), Type::ItemFlags_Tool},     Field{TSTR("Chisel"), Type::ItemFlags_Chisel},       Field{TSTR("Hammer"), Type::ItemFlags_Hammer},
//             Field{TSTR("Vacuum"), Type::ItemFlags_Vacuum},
//     };
// };

ME_GUI_DEFINE_BEGIN(template <>, ItemFlags)
// ImGui::Text("%s", std::format("ItemFlags: {0}", ME::meta::static_refl::TypeInfo<ItemFlags>::fields.NameOfValue(var)).c_str());

// ME::meta::static_refl::TypeInfo<ItemFlags>::ForEachVarOf(var, [&](const auto &field, auto &&value) {
//     if (static_cast<bool>(var & value)) {
//         ImGui::Auto(value, std::string(field.name));
//     }
//     ImGui::Text("%d %d %s", (int)var, (int)value, std::string(field.name).c_str());
// });

auto flags_name = ME::meta::doenum::enum_name((ItemFlags)var);

ImGui::Text("%d %s", (int)var, std::string(flags_name).c_str());

ME_GUI_DEFINE_END

using namespace ME::meta::doenum::bitwise_operators;

class Item {
public:
    std::string name;

    ItemFlags flags = ItemFlags::ItemFlags_None;

    void setFlag(ItemFlags f) { flags |= f; }
    bool getFlag(ItemFlags f) { return static_cast<bool>(flags & f); }

    C_Surface *surface = nullptr;
    R_Image *image = nullptr;

    // Texture *texture = nullptr;

    int pivotX = 0;
    int pivotY = 0;
    f32 breakSize = 16;
    std::vector<MaterialInstance> carry;
    std::vector<U16Point> fill;
    u16 capacity = 0;

    std::vector<CellData *> vacuumCells = {};

    Item(const Item &p) = default;

    Item();
    ~Item();

    static Item *makeItem(ItemFlags flags, RigidBody *rb, std::string n = "unknown");
    static void deleteItem(Item *item);

    void loadFillTexture(C_Surface *tex);
};

template <>
struct ME::meta::static_refl::TypeInfo<Item> : TypeInfoBase<Item> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {Field{TSTR("name"), &Type::name},

                                         Field{TSTR("flags"), &Type::flags},         Field{TSTR("pivotX"), &Type::pivotX},    Field{TSTR("pivotY"), &Type::pivotY},
                                         Field{TSTR("breakSize"), &Type::breakSize}, Field{TSTR("capacity"), &Type::capacity}};
};

ME_GUI_DEFINE_BEGIN(template <>, Item)
ME::meta::static_refl::TypeInfo<Item>::ForEachVarOf(var, [&](const auto &field, auto &&value) { ImGui::Auto(value, std::string(field.name)); });
ME_GUI_DEFINE_END

using ItemLuaPtr = ME::ref<Item>;

struct ItemBinding : public ME::LuaWrapper::PodBind::Binding<ItemBinding, Item> {
    static constexpr const char *class_name = "Item";
    static luaL_Reg *members() { static luaL_Reg members[] = {{"setFlag", setFlag}, {"getFlag", getFlag}, {nullptr, nullptr}}; }
    // Lua constructor
    static int create(lua_State *L) {
        ME::LuaWrapper::PodBind::CheckArgCount(L, 2);
        const char *name = luaL_checkstring(L, 1);
        int age = luaL_checkinteger(L, 2);
        ItemLuaPtr sp = ME::create_ref<Item>();
        push(L, sp);
        return 1;
    }
    // Bind functions
    static int setFlag(lua_State *L) {
        ME::LuaWrapper::PodBind::CheckArgCount(L, 2);
        ItemLuaPtr t = fromStack(L, 1);
        t->setFlag((ItemFlags)lua_tointeger(L, 2));
        return 0;
    }
    static int getFlag(lua_State *L) {
        ME::LuaWrapper::PodBind::CheckArgCount(L, 2);
        ItemLuaPtr t = fromStack(L, 1);
        lua_pushboolean(L, t->getFlag((ItemFlags)lua_tointeger(L, 2)));
        return 1;
    }
};

#endif