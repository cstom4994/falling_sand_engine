// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_ITEMS_HPP
#define ME_ITEMS_HPP

#include <string_view>

#include "engine/core/core.hpp"
#include "engine/core/mathlib.hpp"
#include "engine/core/sdl_wrapper.h"
#include "engine/game_datastruct.hpp"
#include "engine/meta/static_relfection.hpp"
#include "engine/scripting/lua_wrapper.hpp"
#include "engine/textures.hpp"
#include "engine/ui/imgui_utils.hpp"
#include "engine/utils/enum.hpp"
#include "engine/utils/type.hpp"

namespace ME {

ENUM_HPP_CLASS_DECL(ItemFlags, u64,
                    (ItemFlags_None = 1 << 0)(ItemFlags_Rigidbody = 1 << 1)(ItemFlags_Fluid_Container = 1 << 2)(ItemFlags_Tool = 1 << 3)(ItemFlags_Chisel = 1 << 4)(ItemFlags_Hammer = 1 << 5)(
                            ItemFlags_Vacuum = 1 << 6));

ENUM_HPP_REGISTER_TRAITS(ItemFlags);

// template <>
// struct meta::static_refl::TypeInfo<ItemFlags> : TypeInfoBase<ItemFlags> {
//     static constexpr AttrList attrs = {};
//     static constexpr FieldList fields = {
//             Field{TSTR("None"), Type::ItemFlags_None},     Field{TSTR("Rigidbody"), Type::ItemFlags_Rigidbody}, Field{TSTR("Fluid_Container"), Type::ItemFlags_Fluid_Container},
//             Field{TSTR("Tool"), Type::ItemFlags_Tool},     Field{TSTR("Chisel"), Type::ItemFlags_Chisel},       Field{TSTR("Hammer"), Type::ItemFlags_Hammer},
//             Field{TSTR("Vacuum"), Type::ItemFlags_Vacuum},
//     };
// };

class Item {
public:
    std::string name;

    cpp::bitflags::bitflags<ItemFlags> flags = ItemFlags::ItemFlags_None;

    void setFlag(ItemFlags f) { flags |= f; }
    bool getFlag(ItemFlags f) { return static_cast<bool>(flags & f); }

    // 默认贴图
    TextureRef texture;

    // 需要有个R_Image用来存储运行时动态的图像
    R_Image *image = nullptr;

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
struct meta::static_refl::TypeInfo<Item> : TypeInfoBase<Item> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {Field{TSTR("name"), &Type::name},     Field{TSTR("flags"), &Type::flags},         Field{TSTR("pivotX"), &Type::pivotX},
                                         Field{TSTR("pivotY"), &Type::pivotY}, Field{TSTR("breakSize"), &Type::breakSize}, Field{TSTR("capacity"), &Type::capacity}};
};

using ItemLuaPtr = ref<Item>;

struct ItemBinding : public lua_wrapper::PodBind::Binding<ItemBinding, Item> {
    static constexpr const char *class_name = "Item";
    static luaL_Reg *members() { static luaL_Reg members[] = {{"setFlag", setFlag}, {"getFlag", getFlag}, {nullptr, nullptr}}; }
    // Lua constructor
    static int create(lua_State *L) {
        lua_wrapper::PodBind::CheckArgCount(L, 2);
        const char *name = luaL_checkstring(L, 1);
        int age = luaL_checkinteger(L, 2);
        ItemLuaPtr sp = create_ref<Item>();
        push(L, sp);
        return 1;
    }
    // Bind functions
    static int setFlag(lua_State *L) {
        lua_wrapper::PodBind::CheckArgCount(L, 2);
        ItemLuaPtr t = fromStack(L, 1);
        t->setFlag((ItemFlags)lua_tointeger(L, 2));
        return 0;
    }
    static int getFlag(lua_State *L) {
        lua_wrapper::PodBind::CheckArgCount(L, 2);
        ItemLuaPtr t = fromStack(L, 1);
        lua_pushboolean(L, t->getFlag((ItemFlags)lua_tointeger(L, 2)));
        return 1;
    }
};

}  // namespace ME

ME_GUI_DEFINE_BEGIN(template <>, ME::cpp::bitflags::bitflags<ME::ItemFlags>)
// ImGui::Text("%s", std::format("ItemFlags: {0}", meta::static_refl::TypeInfo<ItemFlags>::fields.NameOfValue(var)).c_str());

// meta::static_refl::TypeInfo<ItemFlags>::ForEachVarOf(var, [&](const auto &field, auto &&value) {
//     if (static_cast<bool>(var & value)) {
//         ImGui::Auto(value, std::string(field.name));
//     }
//     ImGui::Text("%d %d %s", (int)var, (int)value, std::string(field.name).c_str());
// });

// 获取物品枚举类别标志
auto var_e = var.as_enum();
if (ME::ItemFlags_traits::to_string(var_e).has_value())  // 判断是否为混合类型
{
    std::string flags_name(ME::ItemFlags_traits::to_string(var_e).value());
    ImGui::Text("ItemFlags:%s(%d)", flags_name.c_str(), (int)var.as_enum());
} else {
    std::string flags_name;  // 不为可知单列枚举则为混合类型
    for (int i = 0; i < ME::ItemFlags_traits::size; i++) {
        auto v = ME::ItemFlags_traits::from_index(i).value();
        if (var.has(v)) {
            flags_name.append(ME::cpp::to_string(v).value());
            if (i < ME::ItemFlags_traits::size - 1) flags_name.append(",");
        }
    }
    ImGui::Text("ItemFlags:%s(%d)", flags_name.c_str(), (int)var.as_enum());
}
ME_GUI_DEFINE_END

ME_GUI_DEFINE_BEGIN(template <>, ME::Item)
ME::meta::static_refl::TypeInfo<ME::Item>::ForEachVarOf(var, [&](const auto &field, auto &&value) { ImGui::Auto(std::forward<decltype(value)>(value), std::string(field.name)); });
ME_GUI_DEFINE_END

#endif