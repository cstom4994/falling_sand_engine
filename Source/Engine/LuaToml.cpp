
#include "LuaToml.hpp"
#include "Libs/lua/sol/sol.hpp"
#include "Libs/toml.hpp"
#include "LuaToml.hpp"
#include "LuaUtilities.hpp"
#include <cassert>
#include <cstddef>
#include <exception>
#include <iostream>
#include <optional>
#include <string>

void insertNodeInTable(
        sol::table &luaTable, std::variant<std::string, size_t> keyOrIndex, void *value) {
    auto node = reinterpret_cast<toml::node *>(value);

    switch (node->type()) {
        case toml::node_type::string: {
            auto v = std::string(*node->as_string());
            try {
                luaTable[std::get<std::string>(keyOrIndex)] = v;
            } catch (std::bad_variant_access) { luaTable[std::get<size_t>(keyOrIndex)] = v; }
            break;
        }

        case toml::node_type::integer: {
            auto v = int64_t{*node->as_integer()};

            try {
                luaTable[std::get<std::string>(keyOrIndex)] = v;
            } catch (std::bad_variant_access) { luaTable[std::get<size_t>(keyOrIndex)] = v; }

            break;
        }

        case toml::node_type::floating_point: {
            auto v = double{*node->as_floating_point()};

            try {
                luaTable[std::get<std::string>(keyOrIndex)] = v;
            } catch (std::bad_variant_access) { luaTable[std::get<size_t>(keyOrIndex)] = v; }

            break;
        }

        case toml::node_type::boolean: {
            auto v = *node->as_boolean() ? 1 : 0;

            try {
                luaTable[std::get<std::string>(keyOrIndex)] = v;
            } catch (std::bad_variant_access) { luaTable[std::get<size_t>(keyOrIndex)] = v; }

            break;
        }
        case toml::node_type::array: {
            auto newLTable = sol::table(luaTable.lua_state(), sol::create);

            tomlArrayToLuaArray(*node->as_array(), newLTable);

            newLTable.push();

            try {
                luaTable[std::get<std::string>(keyOrIndex)] = newLTable;
            } catch (std::bad_variant_access) {
                luaTable[std::get<size_t>(keyOrIndex)] = newLTable;
            }

            break;
        }
        case toml::node_type::table: {
            auto newLTable = sol::table(luaTable.lua_state(), sol::create);

            tomlToLuaTable(*node->as_table(), newLTable);

            newLTable.push();

            try {
                luaTable[std::get<std::string>(keyOrIndex)] = newLTable;
            } catch (std::bad_variant_access) {
                luaTable[std::get<size_t>(keyOrIndex)] = newLTable;
            }

            break;
        }
        case toml::node_type::date: {
            auto v = TOMLDate(*(*node->as_date()));

            try {
                luaTable[std::get<std::string>(keyOrIndex)] = v;
            } catch (std::bad_variant_access) { luaTable[std::get<size_t>(keyOrIndex)] = v; }

            break;
        }
        case toml::node_type::time: {
            auto v = TOMLTime(*(*node->as_time()));

            try {
                luaTable[std::get<std::string>(keyOrIndex)] = v;
            } catch (std::bad_variant_access) { luaTable[std::get<size_t>(keyOrIndex)] = v; }

            break;
        }
        case toml::node_type::date_time: {
            auto v = *(*node->as_date_time());

            auto dt = TOMLDateTime(TOMLDate(v.date), TOMLTime(v.time));

            if (v.offset.has_value()) { dt.setTimeOffset(TOMLTimeOffset(v.offset.value())); }

            try {
                luaTable[std::get<std::string>(keyOrIndex)] = dt;
            } catch (std::bad_variant_access) { luaTable[std::get<size_t>(keyOrIndex)] = dt; }

            break;
        }

        default:
            break;
    }
}

void tomlArrayToLuaArray(toml::array &tomlArray, sol::table &luaTable) {
    size_t size = tomlArray.size();

    for (size_t i = 0; i < size; i++) {
        auto element = tomlArray.get(i);
        size_t index = i + 1;
        insertNodeInTable(luaTable, index, reinterpret_cast<void *>(element));
    };
}

/// Convert `luaTable` into a `toml::table`.
void tomlToLuaTable(toml::table &table, sol::table &lTable) {
    for (auto &&[key, value]: table) {
        auto k = std::string(key);
        insertNodeInTable(lTable, k, reinterpret_cast<void *>(&value));
    }
}

void *luaValueToTomlNode(sol::object &luaValue) {
    switch (luaValue.get_type()) {
        case sol::type::number: {
            int64_t intVal = luaValue.as<int64_t>();
            double doubleVal = luaValue.as<double>();

            if (doubleVal == intVal) {
                auto node = new toml::value(intVal);
                return reinterpret_cast<void *>(node);
            } else {
                auto node = new toml::value(doubleVal);
                return reinterpret_cast<void *>(node);
            }

            break;
        }
        case sol::type::boolean: {
            auto node = new toml::value(luaValue.as<bool>());
            return reinterpret_cast<void *>(node);
            break;
        }
        case sol::type::string: {
            auto node = new toml::value(luaValue.as<std::string>());
            return reinterpret_cast<void *>(node);
            break;
        }
        case sol::type::userdata: {
            if (luaValue.is<TOMLDate>()) {
                auto node = new toml::value(luaValue.as<TOMLDate>().date);
                return reinterpret_cast<void *>(node);
            } else if (luaValue.is<TOMLTime>()) {
                auto node = new toml::value(luaValue.as<TOMLTime>().time);
                return reinterpret_cast<void *>(node);
            } else if (luaValue.is<TOMLDateTime>()) {
                auto node = new toml::value(luaValue.as<TOMLDateTime>().asDateTime());
                return reinterpret_cast<void *>(node);
            }
            break;
        }
        case sol::type::table: {
            auto table = luaValue.as<sol::table>();

            bool keyIsInt = false;

            for (auto &&[key, _]: table) {
                if (key.is<std::string>()) {
                    keyIsInt = false;
                    break;
                } else if (key.is<int64_t>()) {
                    keyIsInt = true;
                    break;
                } else {
                    // TODO: Error!
                    auto k = keyToString(key);
                    throw sol::error((k->empty() ? "The indexes in a table"
                                                 : std::string("The index") + k.value() +
                                                           " should be a integer or a string, not a " +
                                                           solLuaDataTypeToString(key.get_type()))
                                             .c_str());
                }
            }

            if (keyIsInt) {
                return reinterpret_cast<void *>(
                        new toml::array(tomlArrayFromLuaArray(luaValue.as<sol::table>())));
            } else {
                return reinterpret_cast<void *>(
                        new toml::table(tomlTableFromLuaTable(luaValue.as<sol::table>())));
            }

            break;
        }
        default:
            break;
    }

    return NULL;
}

toml::array tomlArrayFromLuaArray(sol::table luaArray) {
    auto tomlArray = toml::array();

    for (auto &&[key, value]: luaArray) {
        if (!key.is<int64_t>()) {
            auto k = keyToString(key);
            throw sol::error((k->empty() ? "The indexes in an array"
                                         : std::string("The index ") + k.value() +
                                                   " should be a integer, not a " +
                                                   solLuaDataTypeToString(key.get_type()))
                                     .c_str());
        }
        if (auto v = reinterpret_cast<toml::node *>(luaValueToTomlNode(value))) {

            tomlArray.push_back(*v);
        }
    }

    return tomlArray;
}

toml::table tomlTableFromLuaTable(sol::table luaTable) {
    auto table = toml::table();

    for (auto &&[key, value]: luaTable) {
        if (!key.is<std::string>()) {
            auto k = keyToString(key);
            throw sol::error((k->empty() ? "The keys in a table"
                                         : std::string("The key ") + k.value() +
                                                   " should be a string, not " +
                                                   solLuaDataTypeToString(key.get_type()))
                                     .c_str());
        }

        if (auto v = reinterpret_cast<toml::node *>(luaValueToTomlNode(value))) {
            table.insert(key.as<std::string>(), *v);
        }
    }

    return table;
}

std::ostream &operator<<(std::ostream &os, const TOMLDate &date) {
    os << date.date;
    return os;
}

std::ostream &operator<<(std::ostream &os, const TOMLTime &time) {
    os << time.time;
    return os;
}

std::ostream &operator<<(std::ostream &os, const TOMLTimeOffset &timeOffset) {
    os << timeOffset.timeOffset;
    return os;
}

std::ostream &operator<<(std::ostream &os, const TOMLDateTime &dateTime) {
    os << dateTime.asDateTime();
    return os;
}

// ------------


#ifdef __cplusplus
extern "C"
{
#endif

    int encode(lua_State *L) {
        sol::state_view state(L);
        sol::table table;

        sol::stack::check<sol::table>(
                L, 1, [](lua_State *s, int, sol::type, sol::type, const char * = nullptr) {
                    return luaL_argerror(
                            s, 1, "A Lua table with strings as keys should be the first and only argument");
                });
        table = sol::stack::get<sol::table>(L, 1);

        auto flags = tableToFormatFlags(sol::stack::check_get<sol::table>(L, 2));

        toml::table tomlTable;

        try {
            tomlTable = tomlTableFromLuaTable(table);
        } catch (std::exception &e) {
            return luaL_error(
                    L, (std::string("An error occurred during encoding: ") + e.what()).c_str());
        }

        std::stringstream ss;

        ss << toml::toml_formatter(tomlTable, flags);

        return sol::stack::push(L, ss.str());

        return 1;
    }

    int decode(lua_State *L) {
        sol::state_view state(L);
        auto res = getTableFromStringInState(state);

        try {
            try {
                auto tomlTable = std::get<toml::table>(res);

                auto luaTable = state.create_table();

                tomlToLuaTable(tomlTable, luaTable);

                return luaTable.push();
            } catch (std::bad_variant_access) { return std::get<int>(res); }
        } catch (std::exception &e) {
            return luaL_error(
                    L, (std::string("An error occurred during decoding: ") + e.what()).c_str());
        }
    }

    int tomlToJSON(lua_State *L) {
        auto flags = tableToFormatFlags(sol::stack::check_get<sol::table>(L, 2));
        return tomlTo<toml::json_formatter>(sol::state_view(L), flags);
    }

    int tomlToYAML(lua_State *L) {
        auto flags = tableToFormatFlags(sol::stack::check_get<sol::table>(L, 2));
        return tomlTo<toml::yaml_formatter>(sol::state_view(L), flags);
    }

#ifdef __cplusplus
}
#endif

extern "C"
{
    LUALIB_API int luaopen_toml(lua_State *L) {
        sol::state_view state(L);
        sol::table module = state.create_table();

        // Setup functions.
        module["encode"] = &encode;
        module["decode"] = &decode;
        module["tomlToJSON"] = &tomlToJSON;
        module["tomlToYAML"] = &tomlToYAML;

        // Setup UserType - Date
        sol::usertype<TOMLDate> tomlDate = module.new_usertype<TOMLDate>(
                "Date", sol::constructors<TOMLDate(uint16_t, uint8_t, uint8_t)>());

        tomlDate["year"] = sol::property(&TOMLDate::getYear, &TOMLDate::setYear);
        tomlDate["month"] = sol::property(&TOMLDate::getMonth, &TOMLDate::setMonth);
        tomlDate["day"] = sol::property(&TOMLDate::getDay, &TOMLDate::setDay);

        // Setup UserType - Time
        sol::usertype<TOMLTime> tomlTime = module.new_usertype<TOMLTime>(
                "Time", sol::constructors<TOMLTime(uint8_t, uint8_t, uint8_t, uint16_t)>());

        tomlTime["hour"] = sol::property(&TOMLTime::getHour, &TOMLTime::setHour);
        tomlTime["minute"] = sol::property(&TOMLTime::getMinute, &TOMLTime::setMinute);
        tomlTime["second"] = sol::property(&TOMLTime::getSecond, &TOMLTime::setSecond);
        tomlTime["nanoSecond"] = sol::property(&TOMLTime::getNanoSecond, &TOMLTime::setNanoSecond);

        // Setup UserType - TimeOffset
        sol::usertype<TOMLTimeOffset> tomlTimeOffset = module.new_usertype<TOMLTimeOffset>(
                "TimeOffset", sol::constructors<TOMLTimeOffset(int8_t, int8_t)>());

        tomlTimeOffset["minutes"] = sol::property(&TOMLTimeOffset::minutes);

        // Setup UserType - DateTime
        sol::usertype<TOMLDateTime> tomlDateTime = module.new_usertype<TOMLDateTime>(
                "DateTime", sol::constructors<
                                    TOMLDateTime(TOMLDate, TOMLTime),
                                    TOMLDateTime(TOMLDate, TOMLTime, TOMLTimeOffset)>());

        tomlDateTime["date"] = sol::property(&TOMLDateTime::getDate, &TOMLDateTime::setDate);
        tomlDateTime["time"] = sol::property(&TOMLDateTime::getTime, &TOMLDateTime::setTime);
        tomlDateTime["timeOffset"] =
                sol::property(&TOMLDateTime::getTimeOffset, &TOMLDateTime::setTimeOffset);

        return module.push();
    }
}
