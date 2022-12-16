// Copyright(c) 2022, KaoruXun All rights reserved.

#include "lua_wrapper.hpp"

#include <cassert>
#include <cstddef>

namespace LuaStruct {

    std::unordered_map<std::string_view, const ITypeInterface *> &KnownTypeInterfaceMap() {
        static std::unordered_map<std::string_view, const ITypeInterface *>
                KnownTypeInterfaceMapInst;
        return KnownTypeInterfaceMapInst;
    }

    int RequireCoreLuaFunc(lua_State *L) {
        lua_getglobal(L, "require");// #1 = require

        lua_pushvalue(L, -1);                  // #2 = require
        lua_pushstring(L, "core/luash_struct");// #3 = path
        lua_call(L, 1, 0);                     // #1 = require

        lua_pushvalue(L, -1);                 // #2 = require
        lua_pushstring(L, "core/luash_class");// #3 = path
        lua_call(L, 1, 0);                    // #1 = require

        lua_pop(L, 1);
        return 0;
    }

    int OpenLibs(lua_State *L) {
        lua_pushcfunction(L, SetupTypeInterfaceTable);
        lua_call(L, 0, 0);
        lua_pushcfunction(L, SetupPtrMetaTable);
        lua_call(L, 0, 0);
        lua_pushcfunction(L, SetupCppNewDelete);
        lua_call(L, 0, 0);
        lua_pushcfunction(L, RequireCoreLuaFunc);
        lua_call(L, 0, 0);
        return 0;
    }

    namespace {
        int CppNew(lua_State *L) {
            const char *name = lua_tostring(L, 1);
            auto &ti_map = KnownTypeInterfaceMap();
            if (auto iter = ti_map.find(name); iter != ti_map.end()) {
                auto ti = iter->second;
                lua_remove(L, 1);
                if (auto size = lua_tointeger(L, 1); size > 0) { return ti->NewVector(L); }
                return ti->NewScalar(L);
            }
            luaL_error(L, "unrecognized type %s", name);
            return 0;
        }

        int CppDelete(lua_State *L) {
            void *value = lua_touserdata(L, 1);
            if (const ITypeInterface *ti = GetTypeInterface(L, value)) {
                if (auto size = lua_tointeger(L, 2); size > 0) { return ti->DeleteVector(L); }
                return ti->DeleteScalar(L);
            }
            luaL_error(L, "unrecognized ref ptr %x", value);
            return 0;
        }
    }// namespace

    namespace detail {
        void TypeInterfaceStringIndexReplace(const ITypeInterface *ti, lua_State *L,
                                             std::string_view PathPrefix, std::string_view name) {
            // #1 = object
            // #2 = key (TO BE REPLACED)
            // ...(optional)
            if (lua_type(L, 2) == LUA_TSTRING) {
                lua_getglobal(L, "require");                             // #N+1 = require
                lua_pushlstring(L, PathPrefix.data(), PathPrefix.size());// #N+2 = "struct/"
                lua_pushlstring(L, name.data(), name.size());            // #N+3 = "typename"
                lua_concat(L, 2);                                        // #N+2 = "struct/typename"
                lua_call(L, 1, 1);                                       // #N+1 = index table
                if (!lua_isnil(L, -1)) {
                    lua_pushvalue(L, 2);// #N+1 = N
                    lua_gettable(L, -2);// #N+2 = new N
                    lua_replace(L, 2);  // #N+1 = index table
                    lua_pop(L, 1);      // #N = N => new N
                }
            }
        }
    }// namespace detail

    int SetupTypeInterfaceTable(lua_State *L) {
        lua_newtable(L);               // #1 = {}
        lua_setglobal(L, "_LUAOBJECT");// #0
        lua_newtable(L);               // #1 = {}
        lua_setglobal(L, "_LUACLASS"); // #0
        return 0;
    }

    template<class... Args>
    struct TypeList
    {
    };
    using BuiltinTypes = TypeList<bool, char, char16_t, char32_t, wchar_t, short, int, long,
                                  long long, float, double, long double>;

    template<class... Args>
    static void SetupBuiltinTypeInterfaceImpl(TypeList<Args...>) {
        (..., (CreateTypeInterface<Args>()));
    }

    int SetupBuiltinTypeInterface(lua_State *L) {
        SetupBuiltinTypeInterfaceImpl(BuiltinTypes());
        return 0;
    }

    int SetupPtrMetaTable(lua_State *L) {
        lua_pushlightuserdata(L, nullptr);// #1 = lightuseradata
        lua_newtable(L);                  // #2 = metatable
        lua_pushcfunction(L, RefValueMetaDispatch<&ITypeInterface::MetaIndex>);   // #3 = func
        lua_setfield(L, -2, "__index");                                           // #2
        lua_pushcfunction(L, RefValueMetaDispatch<&ITypeInterface::MetaNewIndex>);// #3 = func
        lua_setfield(L, -2, "__newindex");                                        // #2
        lua_pushcfunction(L, RefValueMetaDispatch<&ITypeInterface::MetaCall>);    // #3 = func
        lua_setfield(L, -2, "__call");                                            // #2
        lua_pushcfunction(L, RefValueMetaDispatch<&ITypeInterface::MetaToString>);// #3 = func
        lua_setfield(L, -2, "__tostring");                                        // #2
        lua_setmetatable(L, -2);// #1 = lightuserdata
        lua_pop(L, 1);          // #0
        return 0;
    }

    int SetupCppNewDelete(lua_State *L) {
        lua_newtable(L);// #1 = table
        lua_pushcfunction(L, CppNew);
        lua_setfield(L, -2, "new");
        lua_pushcfunction(L, CppDelete);
        lua_setfield(L, -2, "delete");
        lua_setglobal(L, "cpp");
        return 0;
    }

    int CallOnNamedStructCreate(lua_State *L, std::string_view struct_name) {
        // #1 = struct
        lua_pushlstring(L, struct_name.data(), struct_name.size());// #2 = struct_name

        lua_getglobal(L, "OnStructCreate");
        // #3 = OnStructCreate
        lua_insert(L, -3);// #1=OnStructCreate #2=struct #3=struct_name

        if (int errc = lua_pcall(L, 2, 1, 0)) {
            const char *errmsg = lua_tostring(L, -1);
            // TODO: print errmsg
            // #1 = errmsg
            lua_pop(L, 1);
            lua_pushnil(L);
            return 1;
        }
        // #1 = result
        return 1;
    }

    int PushLuaObjectByPtr(lua_State *L, void *Ptr) {
        assert(Ptr != nullptr);
        lua_getglobal(L, "_LUAOBJECT");// #1 = _R._LUAOBJECT
        assert(!lua_isnil(L, -1));
        lua_pushlightuserdata(L, Ptr);// #2 = ptr
        lua_gettable(L, -2);          // #2 = LuaObject
        if (lua_isnil(L, -1)) {
            lua_pop(L, 2);// #0
            return 0;
        } else {
            lua_remove(L, -2);// #1 = LuaObject
            return 1;
        }
    }

    int LinkPtrToLuaObject(lua_State *L, void *Ptr) {
        assert(lua_istable(L, -1));// #1 = LuaObject
        // add this pointer
        lua_pushlightuserdata(L, Ptr);// #2 = ptr
        lua_setfield(L, -1, "this");  // #1 = LuaObject

        // #1 = LuaObject
        lua_getglobal(L, "_LUAOBJECT");// #2 = _R._LUACLASS
        assert(!lua_isnil(L, -1));
        lua_pushlightuserdata(L, Ptr);// #3 = ptr
        lua_pushvalue(L, -3);         //  #4 = LuaObject
        lua_settable(L, -3);          // #2 = _R._LUACLASS
        lua_pop(L, 1);                // #1 = LuaObject
        return 0;
    }

    int RemoveLuaObject(lua_State *L, void *Ptr) {
        lua_getglobal(L, "_LUAOBJECT");// #1 = _R._LUACLASS
        assert(!lua_isnil(L, -1));
        lua_pushlightuserdata(L, Ptr);// #2 = ptr
        lua_pushnil(L);               // #3 = nil
        lua_settable(L, -3);          // #1 = _R._LUACLASS
        lua_pop(L, 1);                // #0
        return 0;
    }

    bool GetPtrByLuaObject(lua_State *L, int N, void *&out) {
        if (lua_isnil(L, N)) return false;
        lua_getfield(L, N, "this");
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return false;
        }
        out = lua_touserdata(L, -1);
        lua_pop(L, 1);
        return true;
    }

    void SetupRefTypeInterface(lua_State *L, void *ptr, const ITypeInterface *ti) {
        lua_pushlightuserdata(L, (void *) ptr);// #1 = ptr
        lua_pushlightuserdata(L, (void *) ti); // #2 = interface
        lua_settable(L, LUA_REGISTRYINDEX);    // #0
    }
    void ReleaseRefTypeInterface(lua_State *L, void *ptr) {
        lua_pushlightuserdata(L, (void *) ptr);// #1 = ptr
        lua_pushnil(L);                        // #2 = nil
        lua_settable(L, LUA_REGISTRYINDEX);    // #0
    }
    const ITypeInterface *GetTypeInterfaceTop(lua_State *L) {
        // #1 = ptr
        lua_gettable(L, LUA_REGISTRYINDEX);// #1 = interface
        const ITypeInterface *ti = reinterpret_cast<const ITypeInterface *>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return ti;
    }
    const ITypeInterface *GetTypeInterface(lua_State *L, void *ptr) {
        lua_pushlightuserdata(L, ptr);
        return GetTypeInterfaceTop(L);
    }
}// namespace LuaStruct

namespace LuaWrapper {

    namespace PodBind {

        // Creates (on demand) the table for storing 'extra' values per class.
        // Table has weak keys as to not prevent garbage collection of the instances.
        // Leaves the new table at the top of the stack.
        int createExtraValueStore(lua_State *L) {
            lua_newtable(L);
            lua_newtable(L);
            lua_pushliteral(L, "__mode");
            lua_pushliteral(L, "k");
            lua_settable(L, -3);
            lua_setmetatable(L, -2);
            lua_pushcfunction(L, createExtraValueStore);// Use function as GUID
            lua_pushvalue(L, -2);
            lua_settable(L, LUA_REGISTRYINDEX);
            return 1;
        }

        // Called when Lua object is indexed: obj[ndx]
        int LuaBindingIndex(lua_State *L) {
            // 1 - class user data
            // 2 - key
            lua_getmetatable(L, 1);
            // 3 - class metatable
            if (lua_isnumber(L, 2)) {// Array access
                lua_getfield(L, 3, "__arrayindex");
                if (lua_type(L, 4) == LUA_TFUNCTION) {
                    lua_pushvalue(L, 1);
                    lua_pushvalue(L, 2);
                    lua_call(L, 2, 1);
                } else {
                    luaL_error(L, "Attempt to index without __arrayindex");
                }
                return 1;
            }
            lua_pushvalue(L, 2);// Key
            lua_gettable(L, 3);
            if (lua_type(L, 4) != LUA_TNIL) {// Found in metatable.
                return 1;
            }
            lua_pop(L, 1);
            lua_getfield(L, 3, "__properties");
            // 4 - class properties table
            lua_pushvalue(L, 2);
            lua_gettable(L, 4);
            // 5 - property table for key ( or nil )
            if (lua_type(L, 5) == LUA_TTABLE) {// Found in properties.
                lua_getfield(L, 5, "get");
                // Call get function.
                if (lua_type(L, 6) == LUA_TFUNCTION) {
                    lua_pushvalue(L, 1);
                    lua_pushvalue(L, 2);
                    lua_call(L, 2, 1);
                }
                return 1;
            }
            lua_pop(L, 2);// __properties
            lua_pushcfunction(L, createExtraValueStore);
            lua_gettable(L, LUA_REGISTRYINDEX);
            if (lua_type(L, 4) == LUA_TTABLE) {
                lua_pushvalue(L, 1);
                lua_gettable(L, 4);
                if (lua_type(L, 5) == LUA_TTABLE) {// Has added instance vars
                    lua_pushvalue(L, 2);
                    lua_gettable(L, 5);
                    return 1;// Return whatever was found, possibly nil.
                }
            }

            lua_pushnil(L);
            return 1;// Not found, return nil.
        }

        // Get the table holding any extra values for the class metatable
        // at index. Returns nil if there has not been any assigned, and
        // no table yet exists.
        int LuaBindGetExtraValuesTable(lua_State *L, int index) {
            index = lua_absindex(L, index);
            lua_pushcfunction(L, createExtraValueStore);
            lua_gettable(L, LUA_REGISTRYINDEX);
            if (lua_type(L, -1) == LUA_TTABLE) {
                lua_pushvalue(L, index);
                lua_gettable(L, -2);
                lua_remove(L, -2);
            }
            return 1;
        }

        // Called whe Lua object index is assigned: obj[ndx] = blah
        int LuaBindingNewIndex(lua_State *L) {
            // 1 - class user data
            // 2 - key
            // 3 - value
            lua_getmetatable(L, 1);
            if (lua_isnumber(L, 2)) {// Array access
                lua_getfield(L, 4, "__arraynewindex");
                if (lua_type(L, 5) == LUA_TFUNCTION) {
                    lua_pushvalue(L, 1);
                    lua_pushvalue(L, 2);
                    lua_pushvalue(L, 3);
                    lua_call(L, 3, 0);
                } else {
                    luaL_error(L, "Attempt to assign to index without __arraynewindex");
                }
                return 0;
            }
            // 4 - class metatable
            // If it's in the metatable, then update it.
            lua_pushvalue(L, 2);
            lua_gettable(L, 4);
            if (lua_type(L, 5) != LUA_TNIL) {// Found in metatable.
                lua_pushvalue(L, 2);
                lua_pushvalue(L, 3);
                lua_settable(L, 4);
                return 0;
            }
            lua_pop(L, 1);

            lua_getfield(L, 4, "__properties");
            // 5 - class properties table
            lua_pushvalue(L, 2);
            lua_gettable(L, 5);
            // 6 - property table for key ( or nil )
            if (lua_type(L, 6) == LUA_TTABLE) {// Found in properties.
                lua_getfield(L, 6, "set");
                if (lua_type(L, 7) == LUA_TFUNCTION) {
                    // Call set function.
                    lua_pushvalue(L, 1);
                    lua_pushvalue(L, 2);
                    lua_pushvalue(L, 3);
                    lua_call(L, 3, 0);
                }
                return 0;
            }
            lua_settop(L, 3);

            // set in per instance table
            lua_pushcfunction(L, createExtraValueStore);
            lua_gettable(L, LUA_REGISTRYINDEX);
            if (lua_type(L, 4) != LUA_TTABLE) {
                lua_pop(L, 1);
                createExtraValueStore(L);
            }
            lua_pushvalue(L, 1);
            lua_gettable(L, 4);
            if (lua_type(L, 5) != LUA_TTABLE) {// No added instance table yet
                lua_pop(L, 1);
                lua_newtable(L);

                lua_pushvalue(L, 1);
                lua_pushvalue(L, 5);
                lua_settable(L, 4);
            }

            // Set the value in the instance table.
            lua_pushvalue(L, 2);
            lua_pushvalue(L, 3);
            lua_settable(L, 5);
            return 0;
        }

        void LuaBindingSetProperties(lua_State *L, bind_properties *properties) {
            // Assumes table at top of the stack for the properties.
            while (properties->name != nullptr) {

                lua_newtable(L);

                lua_pushcfunction(L, properties->getter);
                lua_setfield(L, -2, "get");
                if (properties->setter) {
                    lua_pushcfunction(L, properties->setter);
                    lua_setfield(L, -2, "set");
                }
                lua_setfield(L, -2, properties->name);

                properties++;
            }
        }

        // If the object at 'index' is a userdata with a metatable containing a __upcast
        // function, then replaces the userdata at 'index' in the stack with the result
        // of calling __upcast.
        // Otherwise the object at index is replaced with nil.
        int LuaBindingUpCast(lua_State *L, int index) {
            void *p = lua_touserdata(L, index);
            if (p != nullptr) {
                if (lua_getmetatable(L, index)) {
                    lua_getfield(L, -1, "__upcast");
                    if (lua_type(L, -1) == LUA_TFUNCTION) {
                        // Call upcast
                        lua_pushvalue(L, -3);
                        lua_call(L, 1, 1);
                        lua_replace(L, index);
                        lua_pop(L, 1);// Remove metatable.
                        return 1;
                    }
                }
            }
            lua_pushnil(L);// Cannot be converted.
            lua_replace(L, index);
            return 1;
        }

        // Check the number of arguments are as expected.
        // Throw an error if not.
        void CheckArgCount(lua_State *L, int expected) {
            int n = lua_gettop(L);
            if (n != expected) {
                luaL_error(L, "Got %d arguments, expected %d", n, expected);
                return;
            }
            return;
        }

    };// namespace PodBind

}// namespace LuaWrapper