
#ifndef ME_IMGUI_LUA_HPP
#define ME_IMGUI_LUA_HPP

#include <array>
#include <functional>
#include <list>
#include <string>

#include "engine/scripting/lua_wrapper.hpp"
#include "engine/ui/imgui_utils.hpp"

using namespace ME;

namespace lib {
class element {
public:
    virtual void render() = 0;

    bool operator==(const element& other) { return this == &other; }
};

class window {
private:
    std::string _title;
    ImVec4 _active_color;
    ImVec4 _dropdown_hover_color;
    ImVec4 _dropdown_active_color;

    std::list<element*> elements;

public:
    void render() {

        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, _active_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, _dropdown_active_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, _dropdown_hover_color);

        ImGui::Begin(_title.c_str(), nullptr);

        if (ImGui::IsWindowHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
        } else {
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();

        for (auto element : elements) {
            element->render();
        }

        ImGui::End();
    }

    window(const char* title)
        : _title(title), _active_color(ImVec4(0.16f, 0.29f, 0.48f, 1.00f)), _dropdown_hover_color(ImVec4(0.26f, 0.59f, 0.98f, 1.00f)), _dropdown_active_color(ImVec4(0.06f, 0.53f, 0.98f, 1.00f)) {}

    void add_element(element* element) {
        if (std::find(elements.begin(), elements.end(), element) == elements.end()) {
            elements.push_back(element);
        }
    }

    const std::string get_title() const { return _title; }

    void set_color(float r, float g, float b) {
        _active_color = ImVec4(r, g, b, 1.0f);
        _dropdown_hover_color = ImVec4(r + 0.10f, g + 0.11f, b + 0.11f, 1.0f);
        _dropdown_active_color = ImVec4(r - 0.10f, g - 0.11f, b - 0.11f, 1.0f);
    }
};

class button : public element {
    std::function<void()> _callback;
    std::string _text;
    bool _same_line;
    ImVec4 _color;
    ImVec4 _hover_color;
    ImVec4 _active_color;

public:
    void render() override {
        ImGui::PushStyleColor(ImGuiCol_Button, _color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, _hover_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, _active_color);

        if (_same_line) {
            ImGui::SameLine();
        }

        if (ImGui::Button(_text.c_str()) && _callback) {
            _callback();
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
    }

    button(bool same_line = false)
        : _color(ImVec4(0.26f, 0.59f, 0.98f, 0.40f)), _hover_color(ImVec4(0.26f, 0.59f, 0.98f, 1.00f)), _active_color(ImVec4(0.06f, 0.53f, 0.98f, 1.00f)), _same_line(same_line) {}

    button(const char* text, bool same_line = false)
        : _text(text), _same_line(same_line), _color(ImVec4(0.26f, 0.59f, 0.98f, 0.40f)), _hover_color(ImVec4(0.26f, 0.59f, 0.98f, 1.00f)), _active_color(ImVec4(0.06f, 0.53f, 0.98f, 1.00f)) {}

    button(const char* text, std::function<void()> callback, bool same_line = false)
        : _text(text),
          _callback(callback),
          _same_line(same_line),
          _color(ImVec4(0.26f, 0.59f, 0.98f, 0.40f)),
          _hover_color(ImVec4(0.26f, 0.59f, 0.98f, 1.00f)),
          _active_color(ImVec4(0.06f, 0.53f, 0.98f, 1.00f)) {}

    void set_text(const char* text) { _text = text; }

    void set_color(float r, float g, float b) {
        _color = ImVec4(r, g, b, 0.62f);
        _hover_color = ImVec4(r + 0.05f, g + 0.08f, b + 0.09f, 0.79f);
        _active_color = ImVec4(r + 0.11f, g + 0.14f, b + 0.18f, 1.00f);
    }

    void set_callback(std::function<void()> callback) { _callback = callback; }

    const std::string get_text() const { return _text; }
};

class label : public element {
    std::string _text;
    bool _same_line;

public:
    void render() override {
        if (_same_line) {
            ImGui::SameLine();
        }

        ImGui::Text(_text.c_str());
    }

    label(bool same_line = false) : _same_line(same_line) {}

    label(const char* text, bool same_line = false) : _text(text), _same_line(same_line) {}

    void set_text(const char* text) { _text = text; }

    const std::string get_text() const { return _text; }
};

class check_box : public element {
    std::string _text;
    bool _same_line;
    bool _toggled;
    std::function<void(bool)> _callback;

public:
    void render() override {
        if (_same_line) {
            ImGui::SameLine();
        }

        if (ImGui::Checkbox(_text.c_str(), &_toggled) && _callback) {
            _callback(_toggled);
        }
    }

    check_box(const char* text, bool same_line = false) : _text(text), _toggled(false), _same_line(same_line) {}

    void set_text(const char* text) { _text = text; }

    void set_toggle(bool toggle) { _toggled = toggle; }

    void set_callback(std::function<void(bool)> callback) { _callback = callback; }

    const bool is_toggled() const { return _toggled; }

    const std::string get_text() const { return _text; }
};

class slider : public element {
    int _min, _max, _current;
    std::string _text;
    bool _same_line;

public:
    void render() override { ImGui::SliderInt(_text.c_str(), &_current, _min, _max); }

    slider(const char* text, bool same_line) : _text(text), _same_line(same_line), _current(0), _min(0), _max(10) {}

    void set_text(const char* new_text) { _text = new_text; }

    void set_min(int min) { _min = min; }

    void set_max(int max) { _max = max; }

    const std::string get_text() const { return _text; }

    const int get() const { return _current; }

    const int get_min() const { return _min; }

    const int get_max() const { return _max; }
};

struct color3 {
    float r;
    float g;
    float b;
};

class color_picker : public element {
    std::string _text;
    bool _same_line;
    std::array<float, 4> _colors;

public:
    void render() override {
        if (_same_line) {
            ImGui::SameLine();
        }

        ImGui::ColorPicker4(_text.c_str(), _colors.data(), ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoSidePreview);
    }

    color_picker(bool same_line = false) : _text(std::to_string(std::rand())), _colors({0, 0, 0, 0}), _same_line(same_line) {}

    void set_text(const char* new_text) { _text = new_text; }

    const std::string get_text() const { return _text; }

    const color3 get_colors() { return {_colors[0], _colors[1], _colors[2]}; }
};

class combo_box : public element {
    std::vector<std::string> _members;
    std::function<void(const char*)> _callback;
    std::string _selected;
    std::string _id;
    bool _same_line;

public:
    void render() override {
        if (_same_line) {
            ImGui::SameLine();
        }

        if (ImGui::BeginCombo(_id.c_str(), _selected.c_str())) {
            for (auto& member : _members) {
                auto member_str = member.c_str();
                bool is_selected = &_selected == &member;

                if (ImGui::Selectable(member_str, is_selected)) {
                    _selected = member;

                    if (_callback) {
                        _callback(member_str);
                    }
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }
    }

    combo_box(const char* id, bool same_line = false) : _id(id), _same_line(same_line) {}

    void set_text(const char* id) { _id = id; }

    void set_callback(std::function<void(const char*)> callback) { _callback = callback; }

    void add(const char* member) { _members.emplace_back(member); }

    void remove(const char* member) {
        std::erase_if(_members, [=](auto& str) { return member == str; });
    }

    const std::string get_text() const { return _id; }
};

class tab : public element {
private:
    std::list<element*> elements;
    std::string _title;

public:
    void render() override {
        if (ImGui::BeginTabItem(_title.c_str())) {
            for (auto element : elements) {
                element->render();
            }

            ImGui::EndTabItem();
        }
    }

    tab(const char* title) : _title(title) {}

    void set_title(const char* new_title) { _title = new_title; }

    void add_element(element* element) {
        if (std::find(elements.begin(), elements.end(), element) == elements.end()) {
            elements.push_back(element);
        }
    }

    std::string get_title() const { return _title; }
};

class tab_selector : public element {
    std::list<tab> tabs;
    std::string _id;
    bool _same_line;

public:
    void render() override {
        if (_same_line) {
            ImGui::SameLine();
        }

        if (ImGui::BeginTabBar(_id.c_str())) {
            for (auto& tab : tabs) {
                tab.render();
            }

            ImGui::EndTabBar();
        }
    }

    tab_selector(const char* id, bool same_line = false) : _id(id), _same_line(same_line) {}

    tab* add_tab(const char* title) { return &tabs.emplace_back(title); }
};
}  // namespace lib

template <typename T>
constexpr inline void set_userdata(lua_State* L, T* element) {
    *static_cast<T**>(lua_newuserdata(L, sizeof(T*))) = element;
}

template <typename T>
constexpr inline T* to_class_ptr(lua_State* L, int idx) {
    return *static_cast<T**>(lua_touserdata(L, idx));
}

void set_type(lua_State* L, const char* type_name) {
    lua_pushstring(L, type_name);

    lua_pushvalue(L, -1);
    lua_setfield(L, -3, "__type");

    lua_pushcclosure(
            L,
            [](lua_State* L) {
                lua_pushstring(L, lua_tostring(L, lua_upvalueindex(1)));
                return 1;
            },
            1);
    lua_setfield(L, -2, "__tostring");
}

void assign_to_parent(lua_State* L, lib::element* element) {
    lua_getmetatable(L, 2);
    lua_getfield(L, -1, "__type");

    const char* type = lua_tostring(L, -1);
    lua_pop(L, 2);

    if (strcmp(type, "Window") == 0) {
        auto window = to_class_ptr<lib::window>(L, 2);
        window->add_element(element);
    } else if (strcmp(type, "Tab") == 0) {
        auto tab = to_class_ptr<lib::tab>(L, 2);
        tab->add_element(element);
    }
}

namespace lua_bind {

static std::vector<lib::window> windows;
static lua_State* l_G;

void window(lua_State* L) {

    // ME_BUG("imgui window %s", lua_tostring(L, 2));

    auto window = &windows.emplace_back(lua_tostring(L, 2));
    set_userdata<lib::window>(L, window);

    lua_newtable(L);

    lua_pushvalue(L, -2);
    lua_pushcclosure(
            L,
            [](lua_State* L) {
                const char* idx = lua_tostring(L, 2);
                auto window = to_class_ptr<lib::window>(L, lua_upvalueindex(1));

                lua_pop(L, 2);

                if (strcmp(idx, "Title") == 0) {
                    lua_pushstring(L, window->get_title().c_str());
                }

                return 1;
            },
            1);
    lua_setfield(L, -2, "__index");

    lua_pushstring(L, "Window");
    lua_setfield(L, -2, "__type");

    lua_setmetatable(L, -2);
}

void button(lua_State* L) {
    bool same_line = !lua_isnil(L, 3) && lua_isboolean(L, 3) && lua_toboolean(L, 3);
    auto button = new lib::button(same_line);

    set_userdata<lib::button>(L, button);
    assign_to_parent(L, button);

    lua_newtable(L);  // metatable

    set_type(L, "Button");

    lua_pushlightuserdata(L, button);
    lua_pushcclosure(
            L,
            [](lua_State* L) {
                const char* idx = lua_tostring(L, 2);
                auto button = (lib::button*)lua_touserdata(L, lua_upvalueindex(1));

                lua_pop(L, 2);

                if (strcmp(idx, "Text") == 0) {
                    lua_pushstring(L, button->get_text().c_str());
                }

                return 1;
            },
            1);
    lua_setfield(L, -2, "__index");

    lua_pushlightuserdata(L, button);
    lua_pushcclosure(
            L,
            [](lua_State* L) {
                const char* idx = lua_tostring(L, 2);
                auto button = (lib::button*)lua_touserdata(L, lua_upvalueindex(1));

                if (strcmp(idx, "Text") == 0) {
                    button->set_text(lua_tostring(L, 3));
                } else if (strcmp(idx, "Callback") == 0) {
                    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

                    button->set_callback([=]() {
                        lua_rawgeti(l_G, LUA_REGISTRYINDEX, ref);
                        lua_pcall(L, 0, 0, 0);
                    });
                }

                return 0;
            },
            1);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, -2);
}

void label(lua_State* L) {
    bool same_line = !lua_isnil(L, 3) && lua_isboolean(L, 3) && lua_toboolean(L, 3);
    auto label = new lib::label(same_line);

    set_userdata<lib::label>(L, label);
    assign_to_parent(L, label);

    lua_newtable(L);  // metatable

    set_type(L, "Label");

    lua_pushlightuserdata(L, label);
    lua_pushcclosure(
            L,
            [](lua_State* L) {
                const char* idx = lua_tostring(L, 2);
                auto label = (lib::label*)lua_touserdata(L, lua_upvalueindex(1));

                lua_pop(L, 2);

                if (strcmp(idx, "Text") == 0) {
                    lua_pushstring(L, label->get_text().c_str());
                }

                return 1;
            },
            1);
    lua_setfield(L, -2, "__index");

    lua_pushlightuserdata(L, label);
    lua_pushcclosure(
            L,
            [](lua_State* L) {
                const char* idx = lua_tostring(L, 2);
                auto label = (lib::label*)(lua_touserdata(L, lua_upvalueindex(1)));

                if (strcmp(idx, "Text") == 0) {
                    label->set_text(lua_tostring(L, 3));
                }

                return 0;
            },
            1);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, -2);
}

void check_box(lua_State* L) {
    bool same_line = !lua_isnil(L, 3) && lua_isboolean(L, 3) && lua_toboolean(L, 3);
    auto check_box = new lib::check_box(std::to_string(std::rand()).c_str(), same_line);

    set_userdata<lib::check_box>(L, check_box);
    assign_to_parent(L, check_box);

    lua_newtable(L);

    set_type(L, "Checkbox");

    lua_pushlightuserdata(L, check_box);
    lua_pushcclosure(
            L,
            [](lua_State* L) {
                const char* idx = lua_tostring(L, 2);
                auto check_box = (lib::check_box*)lua_touserdata(L, lua_upvalueindex(1));

                lua_pop(L, 2);

                if (strcmp(idx, "Checked") == 0) {
                    lua_pushboolean(L, check_box->is_toggled());
                } else if (strcmp(idx, "Text") == 0) {
                    lua_pushstring(L, check_box->get_text().c_str());
                }

                return 1;
            },
            1);
    lua_setfield(L, -2, "__index");

    lua_pushlightuserdata(L, check_box);
    lua_pushcclosure(
            L,
            [](lua_State* L) {
                const char* idx = lua_tostring(L, 2);
                auto check_box = (lib::check_box*)lua_touserdata(L, lua_upvalueindex(1));

                if (strcmp(idx, "Checked") == 0) {
                    check_box->set_toggle(lua_toboolean(L, 3));
                } else if (strcmp(idx, "Text") == 0) {
                    check_box->set_text(lua_tostring(L, 3));
                } else if (strcmp(idx, "Callback") == 0) {
                    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

                    check_box->set_callback([=](bool toggled) {
                        lua_rawgeti(l_G, LUA_REGISTRYINDEX, ref);
                        lua_pushboolean(l_G, toggled);
                        lua_pcall(l_G, 1, 0, 0);
                    });
                }

                return 0;
            },
            1);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, -2);
}

void slider(lua_State* L) {
    bool same_line = !lua_isnil(L, 3) && lua_isboolean(L, 3) && lua_toboolean(L, 3);
    auto slider = new lib::slider(std::to_string(std::rand()).c_str(), same_line);

    set_userdata<lib::slider>(L, slider);
    assign_to_parent(L, slider);

    lua_newtable(L);

    set_type(L, "Slider");

    lua_pushlightuserdata(L, slider);
    lua_pushcclosure(
            L,
            [](lua_State* L) {
                const char* idx = lua_tostring(L, 2);
                auto slider = (lib::slider*)lua_touserdata(L, lua_upvalueindex(1));

                if (strcmp(idx, "Value")) {
                    lua_pushnumber(L, slider->get());
                } else if (strcmp(idx, "Min") == 0) {
                    lua_pushnumber(L, slider->get_min());
                } else if (strcmp(idx, "Max") == 0) {
                    lua_pushnumber(L, slider->get_max());
                } else if (strcmp(idx, "Text") == 0) {
                    lua_pushstring(L, slider->get_text().c_str());
                }

                return 1;
            },
            1);
    lua_setfield(L, -2, "__index");

    lua_pushlightuserdata(L, slider);
    lua_pushcclosure(
            L,
            [](lua_State* L) {
                const char* idx = lua_tostring(L, 2);
                auto slider = (lib::slider*)lua_touserdata(L, lua_upvalueindex(1));

                if (strcmp(idx, "Min") == 0) {
                    slider->set_min(lua_tonumber(L, 3));
                } else if (strcmp(idx, "Max") == 0) {
                    slider->set_max(lua_tonumber(L, 3));
                } else if (strcmp(idx, "Text") == 0) {
                    slider->set_text(lua_tostring(L, 3));
                }

                return 0;
            },
            1);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, -2);
}

void color_picker(lua_State* L) {
    bool same_line = !lua_isnil(L, 3) && lua_isboolean(L, 3) && lua_toboolean(L, 3);
    auto color_picker = new lib::color_picker(same_line);

    set_userdata<lib::color_picker>(L, color_picker);
    assign_to_parent(L, color_picker);

    lua_newtable(L);

    set_type(L, "ColorPicker");

    lua_pushlightuserdata(L, color_picker);
    lua_pushcclosure(
            L,
            [](lua_State* L) {
                const char* idx = lua_tostring(L, 2);
                auto color_picker = (lib::color_picker*)lua_touserdata(L, lua_upvalueindex(1));

                lua_pop(L, 2);

                if (strcmp(idx, "PickedColor3") == 0) {
                    lua_pushnil(L);
                } else if (strcmp(idx, "Text") == 0) {
                    lua_pushstring(L, color_picker->get_text().c_str());
                }

                return 1;
            },
            1);
    lua_setfield(L, -2, "__index");

    lua_pushlightuserdata(L, color_picker);
    lua_pushcclosure(
            L,
            [](lua_State* L) {
                auto color_picker = (lib::color_picker*)lua_touserdata(L, lua_upvalueindex(1));

                if (strcmp(lua_tostring(L, 2), "Text") == 0) {
                    color_picker->set_text(lua_tostring(L, 3));
                }

                return 0;
            },
            1);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, -2);
}

void list_box(lua_State* L) {
    bool same_line = !lua_isnil(L, 3) && lua_isboolean(L, 3) && lua_toboolean(L, 3);
    auto list_box = new lib::combo_box(std::to_string(std::rand()).c_str(), same_line);

    set_userdata<lib::combo_box>(L, list_box);
    assign_to_parent(L, list_box);

    lua_newtable(L);  // metatable

    set_type(L, "ListBox");

    lua_pushvalue(L, -2);
    lua_pushcclosure(
            L,
            [](lua_State* L) {
                const char* idx = lua_tostring(L, 2);
                auto list_box = (lib::combo_box*)lua_touserdata(L, lua_upvalueindex(1));

                // lua_pop(L, 2);

                if (strcmp(idx, "Text") == 0) {
                    lua_pushstring(L, list_box->get_text().c_str());
                } else if (strcmp(idx, "Add") == 0) {
                    lua_pushcclosure(
                            L,
                            [](lua_State* L) {
                                auto list_box = to_class_ptr<lib::combo_box>(L, 1);
                                list_box->add(lua_tostring(L, 2));
                                return 0;
                            },
                            0);
                } else if (strcmp(idx, "Remove") == 0) {
                    lua_pushcclosure(
                            L,
                            [](lua_State* L) {
                                auto list_box = to_class_ptr<lib::combo_box>(L, 1);
                                list_box->remove(lua_tostring(L, 2));
                                return 0;
                            },
                            0);
                }

                return 1;
            },
            1);
    lua_setfield(L, -2, "__index");

    lua_pushvalue(L, -2);
    lua_pushcclosure(
            L,
            [](lua_State* L) {
                const char* idx = lua_tostring(L, 2);
                auto list_box = (lib::combo_box*)lua_touserdata(L, lua_upvalueindex(1));

                if (strcmp(idx, "Text") == 0) {
                    list_box->set_text(lua_tostring(L, 3));
                } else if (strcmp(idx, "Callback") == 0) {
                    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

                    list_box->set_callback([=](const char* selected) {
                        lua_rawgeti(l_G, LUA_REGISTRYINDEX, ref);
                        lua_pushstring(L, selected);
                        lua_pcall(L, 1, 0, 0);
                    });
                }

                return 0;
            },
            1);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, -2);
}

int tab(lua_State* L) {
    auto tab_selector = to_class_ptr<lib::tab_selector>(L, 1);
    auto tab = tab_selector->add_tab(lua_tostring(L, 2));

    lua_pop(L, 2);

    set_userdata<lib::tab>(L, tab);

    lua_newtable(L);

    set_type(L, "Tab");

    lua_pushvalue(L, -2);
    lua_pushcclosure(
            L,
            [](lua_State* L) {
                const char* idx = lua_tostring(L, 2);
                auto tab = to_class_ptr<lib::tab>(L, lua_upvalueindex(1));

                lua_pop(L, 2);

                if (strcmp(idx, "Text")) {
                    lua_pushstring(L, tab->get_title().c_str());
                }

                return 1;
            },
            1);
    lua_setfield(L, -2, "__index");

    lua_pushvalue(L, -2);
    lua_pushcclosure(
            L,
            [](lua_State* L) {
                const char* idx = lua_tostring(L, 2);
                auto tab = to_class_ptr<lib::tab>(L, lua_upvalueindex(1));

                if (strcmp(idx, "Text")) {
                    tab->set_title(lua_tostring(L, 3));
                }

                return 0;
            },
            1);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, -2);
    return 1;
}

void tab_selector(lua_State* L) {
    bool same_line = !lua_isnil(L, 3) && lua_isboolean(L, 3) && lua_toboolean(L, 3);
    auto tab_selector = new lib::tab_selector(std::to_string(std::rand()).c_str(), same_line);

    set_userdata<lib::tab_selector>(L, tab_selector);
    assign_to_parent(L, tab_selector);

    lua_newtable(L);

    set_type(L, "TabSelector");

    lua_pushvalue(L, -2);
    lua_pushcclosure(
            L,
            [](lua_State* L) {
                if (strcmp(lua_tostring(L, 2), "AddTab") == 0) {
                    lua_pushvalue(L, lua_upvalueindex(1));
                    lua_pushcclosure(L, tab, 1);
                }
                return 1;
            },
            1);
    lua_setfield(L, -2, "__index");

    lua_setmetatable(L, -2);
}

void imgui_init_lua() {
    lua_newtable(l_G);
    lua_pushcclosure(
            l_G,
            [](lua_State* L) {
                if (lua_isstring(L, 1)) {
                    const char* element = lua_tostring(L, 1);

                    if (ME_str_equals(element, "Window")) {
                        lua_bind::window(L);
                    } else if (ME_str_equals(element, "Button")) {
                        lua_bind::button(L);
                    } else if (ME_str_equals(element, "Label")) {
                        lua_bind::label(L);
                    } else if (ME_str_equals(element, "Checkbox")) {
                        lua_bind::check_box(L);
                    } else if (ME_str_equals(element, "Slider")) {
                        lua_bind::slider(L);
                    } else if (ME_str_equals(element, "ColorPicker")) {
                        lua_bind::color_picker(L);
                    } else if (ME_str_equals(element, "ListBox")) {
                        lua_bind::list_box(L);
                    } else if (ME_str_equals(element, "TabSelector")) {
                        lua_bind::tab_selector(L);
                    }
                }

                return 1;
            },
            0);
    lua_setfield(l_G, -2, "new");
    lua_setglobal(l_G, "ImGui");
}
}  // namespace lua_bind

#endif