// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "scripting.hpp"

#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "engine/background.hpp"
#include "engine/core/core.hpp"
#include "engine/core/cpp/utils.hpp"
#include "engine/core/debug.hpp"
#include "engine/core/global.hpp"
#include "engine/core/io/filesystem.h"
#include "engine/core/memory.h"
#include "engine/core/utils/utility.hpp"
#include "engine/game.hpp"
#include "engine/game_datastruct.hpp"
#include "engine/game_resources.hpp"
#include "engine/meta/reflection.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/scripting/csharp_api.h"
#include "engine/scripting/csharp_bind.hpp"
#include "engine/scripting/ffi/ffi.h"
#include "engine/scripting/lua_wrapper.hpp"
#include "engine/scripting/lua_wrapper_ext.hpp"
#include "engine/scripting/wrap/wrap_engine.hpp"
#include "engine/ui/imgui_impl.hpp"

void func1(std::string a) { std::cout << __FUNCTION__ << " :: " << a << std::endl; }
void func2(std::string a) { std::cout << __FUNCTION__ << " :: " << a << std::endl; }

// in warp_surface.cpp
extern int luaopen_surface(lua_State *L);
extern int luaopen_surface_color(lua_State *L);

IMPLENGINE();

struct MyStruct {
    void (*func1)(std::string);
    void (*func2)(std::string);
};

template <class T>
void As(T arg) {
    // auto namespac = state["Test"].get_or_create<sol::table>();
    // namespac.set_function();
}

auto f = [](auto &...args) { (..., As(args)); };

static void bindBasic() {

    MyStruct myStruct;
    ME::meta::struct_apply(myStruct, f);
}

static std::string proxy_print(lua_State *L) {
    auto argNumber = lua_gettop(L);
    std::string s;
    s.reserve(100);
    for (int i = 1; i < argNumber + 1; ++i) {
        s += lua_tostring(L, i);
        s += i != argNumber ? ", " : "";
    }
    return s;
}

static int metadot_bug(lua_State *L) {
    METADOT_BUG("[LUA] ", proxy_print(L).c_str());
    return 0;
}

static int metadot_info(lua_State *L) {
    METADOT_INFO("[LUA] ", proxy_print(L).c_str());
    return 0;
}

static int metadot_trace(lua_State *L) {
    METADOT_TRACE("[LUA] ", proxy_print(L).c_str());
    return 0;
}

static int metadot_error(lua_State *L) {
    METADOT_ERROR("[LUA] ", proxy_print(L).c_str());
    return 0;
}

static int metadot_warn(lua_State *L) {
    METADOT_WARN("[LUA] ", proxy_print(L).c_str());
    return 0;
}

static int catch_panic(lua_State *L) {
    auto message = lua_tostring(L, -1);
    METADOT_ERROR("[LUA] PANIC ERROR: ", message);
    return 0;
}

static int metadot_run_lua_file_script(lua_State *L) {
    std::string string = lua_tostring(L, 1);
    auto &LuaCore = Scripting::get_singleton_ptr()->Lua->s_lua;
    ME_ASSERT_E(&LuaCore);
    if (ME_str_starts_with(string, "Script:")) ME_str_replace_with(string, "Script:", METADOT_RESLOC("data/scripts/"));
    script_runfile(string.c_str());
    return 0;
}

static int metadot_exit(lua_State *L) {
    // s_lua_layer->closeConsole();
    return 0;
}

// returns table with pairs of path and isDirectory
static int ls(lua_State *L) {
    if (!lua_isstring(L, 1)) {
        METADOT_WARN("invalid lua argument");
        return 0;
    }
    auto string = lua_tostring(L, 1);
    if (!std::filesystem::is_directory(string)) {
        METADOT_WARN(std::format("{0} is not directory", string).c_str());
        return 0;
    }

    lua_newtable(L);
    int i = 0;
    for (auto &p : std::filesystem::directory_iterator(string)) {
        lua_pushnumber(L, i + 1);  // parent table index
        lua_newtable(L);
        lua_pushstring(L, "path");
        lua_pushstring(L, p.path().generic_string().c_str());
        lua_settable(L, -3);
        lua_pushstring(L, "isDirectory");
        lua_pushboolean(L, p.is_directory());
        lua_settable(L, -3);
        lua_settable(L, -3);
        i++;
    }
    return 1;
}

static void add_packagepath(const char *p) {
    auto &s_lua = Scripting::get_singleton_ptr()->Lua->s_lua;
    ME_ASSERT_E(&s_lua);
    s_lua.dostring(std::format("package.path = "
                               "'{0}/?.lua;' .. package.path",
                               p),
                   s_lua.globalTable());
}

static int R_GetTextureAttr(R_Image *image, const char *attr) {
    if (strcmp(attr, "w")) return image->w;
    if (strcmp(attr, "h")) return image->h;
    ME_ASSERT_E(0);
    return 0;
}

void print_error(lua_State *state, int result) {
    const char *message = lua_tostring(state, -1);
    METADOT_ERROR("LuaScript ERROR:\n  ", (message ? message : "no message"));

    if (result != 0) {
        switch (result) {
            case LUA_ERRRUN:
                METADOT_ERROR("Lua Runtime error");
                break;
            case LUA_ERRSYNTAX:
                METADOT_ERROR("Lua syntax error");
                break;
            case LUA_ERRMEM:
                METADOT_ERROR("Lua was unable to allocate the required memory");
                break;
            case LUA_ERRFILE:
                METADOT_ERROR("Lua was unable to find boot file");
                break;
            default:
                METADOT_ERROR("Unknown lua error: %d", result);
        }
    }

    lua_pop(state, 1);
}

static char buf[1024];

static std::string readStringFromFile(const char *filePath) {
    FUTIL_ASSERT_EXIST(filePath);

    FILE *f = fopen(filePath, "r");
    if (!f) return "";
    int read = 0;
    std::string out;
    while ((read = fread(buf, 1, 1024, f)) != 0) out += std::string_view(buf, read);
    fclose(f);
    return out;
}

lua_State *LuaCoreCppFunc(void *luacorecpp) { return ((LuaCore *)luacorecpp)->s_lua.state(); }

static void InitLua(LuaCore *lc) {

    lc->L = lc->s_lua.state();

    luaopen_base(lc->L);
    luaL_openlibs(lc->L);

    lua_atpanic(lc->L, catch_panic);
    lua_register(lc->L, "METADOT_TRACE", metadot_trace);
    lua_register(lc->L, "METADOT_INFO", metadot_info);
    lua_register(lc->L, "METADOT_WARN", metadot_warn);
    lua_register(lc->L, "METADOT_BUG", metadot_bug);
    lua_register(lc->L, "METADOT_ERROR", metadot_error);
    lua_register(lc->L, "METADOT_RUN_FILE", metadot_run_lua_file_script);
    lua_register(lc->L, "runf", metadot_run_lua_file_script);
    lua_register(lc->L, "exit", metadot_exit);
    lua_register(lc->L, "ls", ls);

    ME_debug_setup(lc->L, "debugger", "dbg", NULL, NULL);

    metadot_bind_image(lc->L);
    metadot_bind_gpu(lc->L);
    metadot_bind_fs(lc->L);
    metadot_bind_lz4(lc->L);
    metadot_bind_cstructcore(lc->L);
    metadot_bind_cstructtest(lc->L);

    ME_preload_auto(lc->L, luaopen_surface, "_ME_surface");
    ME_preload_auto(lc->L, luaopen_surface_color, "_ME_surface_color");

    ME_preload_auto(lc->L, ffi_module_open, "ffi");
    ME_preload_auto(lc->L, luaopen_lbind, "lbind");

#define REGISTER_LUAFUNC(_f) lc->s_lua[#_f] = LuaWrapper::function(_f)

    lc->s_lua["METADOT_RESLOC"] = LuaWrapper::function([](const char *a) { return METADOT_RESLOC(a); });
    lc->s_lua["GetSurfaceFromTexture"] = LuaWrapper::function([](Texture *tex) { return tex->surface; });
    lc->s_lua["GetWindowH"] = LuaWrapper::function([]() { return Screen.windowHeight; });
    lc->s_lua["GetWindowW"] = LuaWrapper::function([]() { return Screen.windowWidth; });

    REGISTER_LUAFUNC(SDL_FreeSurface);
    REGISTER_LUAFUNC(R_SetImageFilter);
    REGISTER_LUAFUNC(R_CopyImageFromSurface);
    REGISTER_LUAFUNC(R_GetTextureHandle);
    REGISTER_LUAFUNC(R_GetTextureAttr);
    REGISTER_LUAFUNC(LoadTextureData);
    REGISTER_LUAFUNC(DestroyTexture);
    REGISTER_LUAFUNC(CreateTexture);
    REGISTER_LUAFUNC(metadot_buildnum);
    REGISTER_LUAFUNC(metadot_metadata);
    REGISTER_LUAFUNC(add_packagepath);

#undef REGISTER_LUAFUNC

    lc->s_lua.dostring(std::format("package.path = "
                                   "'{1}/?.lua;{0}/?.lua;{0}/libs/?.lua;{0}/libs/?/init.lua;{0}/libs/"
                                   "?/?.lua;' .. package.path",
                                   METADOT_RESLOC("data/scripts"), normalizePath(std::filesystem::current_path().string()).c_str()),
                       lc->s_lua.globalTable());

    lc->s_lua.dostring(std::format("package.cpath = "
                                   "'{1}/?.{2};{0}/?.{2};{0}/libs/?.{2};{0}/libs/?/init.{2};{0}/libs/"
                                   "?/?.{2};' .. package.cpath",
                                   METADOT_RESLOC("data/scripts"), normalizePath(std::filesystem::current_path().string()).c_str(), "dll"),
                       lc->s_lua.globalTable());

    script_runfile("data/scripts/startup.lua");
}

static void EndLua(LuaCore *_struct) {}

void run_script_in_console(LuaCore *_struct, const char *c) {
    luaL_loadstring(_struct->L, c);
    auto result = ME_debug_pcall(_struct->L, 0, LUA_MULTRET, 0);
    if (result != LUA_OK) {
        print_error(_struct->L);
        return;
    }
}

void script_runfile(const char *filePath) {
    FUTIL_ASSERT_EXIST(filePath);

    int result = luaL_loadfile(Scripting::get_singleton_ptr()->Lua->L, METADOT_RESLOC(filePath));
    if (result != LUA_OK) {
        print_error(Scripting::get_singleton_ptr()->Lua->L);
        return;
    }
    result = ME_debug_pcall(Scripting::get_singleton_ptr()->Lua->L, 0, LUA_MULTRET, 0);

    if (result != LUA_OK) {
        print_error(Scripting::get_singleton_ptr()->Lua->L);
    }
}

#ifdef ND_DEBUG
#define ASS_NAME "ManagedCore.dll"
#else
#define ASS_NAME "D:\\Projects\\Sandbox\\Dev\\output\\ManagedCore.dll"
#endif
// #pragma comment(lib, "C:/Program Files/Mono/lib/mono-2.0-sgen.lib")
/*  types for args in internal calls
 *
static MonoClass*
find_system_class(const char* name)
{
    if (!strcmp(name, "void"))
        return mono_defaults.void_class;
    else if (!strcmp(name, "char")) return mono_defaults.char_class;
    else if (!strcmp(name, "bool")) return mono_defaults.boolean_class;
    else if (!strcmp(name, "byte")) return mono_defaults.byte_class;
    else if (!strcmp(name, "sbyte")) return mono_defaults.sbyte_class;
    else if (!strcmp(name, "uint16")) return mono_defaults.uint16_class;
    else if (!strcmp(name, "int16")) return mono_defaults.int16_class;
    else if (!strcmp(name, "uint")) return mono_defaults.uint32_class;
    else if (!strcmp(name, "int")) return mono_defaults.int32_class;
    else if (!strcmp(name, "ulong")) return mono_defaults.uint64_class;
    else if (!strcmp(name, "long")) return mono_defaults.int64_class;
    else if (!strcmp(name, "uintptr")) return mono_defaults.uint_class;
    else if (!strcmp(name, "intptr")) return mono_defaults.int_class;
    else if (!strcmp(name, "single")) return mono_defaults.single_class;
    else if (!strcmp(name, "double")) return mono_defaults.double_class;
    else if (!strcmp(name, "string")) return mono_defaults.string_class;
    else if (!strcmp(name, "object")) return mono_defaults.object_class;
    else
        return NULL;
}*/
static void l_info(MonoString *s) {
    auto c = mono_string_to_utf8(s);
    METADOT_INFO("[CSharp] ", c);
    mono_free(c);
}

static void l_warn(MonoString *s) {
    auto c = mono_string_to_utf8(s);
    METADOT_WARN("[CSharp] ", c);
    mono_free(c);
}

static void l_error(MonoString *s) {
    auto c = mono_string_to_utf8(s);
    METADOT_ERROR("[CSharp] ", c);
    mono_free(c);
}

static void l_trace(MonoString *s) {
    auto c = mono_string_to_utf8(s);
    METADOT_TRACE("[CSharp] ", c);
    mono_free(c);
}

static mono_bool metadot_copy_file(MonoString *from, MonoString *to) {
    auto f = mono_string_to_utf8(from);
    auto t = mono_string_to_utf8(to);
    bool suk = false;
    try {
        suk = std::filesystem::copy_file(f, t, std::filesystem::copy_options::overwrite_existing);
    } catch (std::exception &e) {
        METADOT_ERROR("Cannot copy {} to {}\n{}", f, t, e.what());
    }
    // METADOT_TRACE("Copying file from {} to {} and {}",f,t,suk);
    mono_free(f);
    mono_free(t);

    return suk;
}

static void metadot_profile_begin_session(MonoString *name, MonoString *path) {
    auto n = mono_string_to_utf8(name);
    auto p = mono_string_to_utf8(path);
    // METADOT_PROFILE_BEGIN_SESSION(n, p);
    mono_free(n);
    mono_free(p);
}

static void metadot_profile_end_session() {}

static MonoString *metadot_current_config() { return mono_string_new(mono_domain_get(), "ME_NO_DEBUG"); }

static MonoDomain *domain;
static MonoImage *image;
static MonoAssembly *assembly;
static MonoObject *entryInstance;

static MonoObject *callCSMethod(const char *methodName, void *obj = nullptr, void **params = nullptr, MonoObject **ex = nullptr) {
    MonoMethodDesc *TypeMethodDesc = mono_method_desc_new(methodName, NULL);
    if (!TypeMethodDesc) return nullptr;

    // Search the method in the image
    MonoMethod *method = mono_method_desc_search_in_image(TypeMethodDesc, image);
    if (!method) {
        assert(false, "C# method not found: {}", methodName);
        return nullptr;
    }

    auto b = mono_runtime_invoke(method, obj, params, ex);
    if (ex && *ex) return nullptr;

    return b;
}

static std::string hotSwapLoc = "MetaDotManagedCore.EntryHotSwap:";
static std::string coldLoc = "MetaDotManagedCore.EntryCold:";
static bool happyLoad = false;

MonoObject *MonoLayer::callEntryMethod(const char *methodName, void *obj, void **params, MonoObject **ex) {
    std::string s = hotSwapEnable ? hotSwapLoc : coldLoc;
    return callCSMethod((s + methodName).c_str(), obj, params, ex);
}

static void initInternalCalls() {
    mono_add_internal_call("MetaDotManagedCore.Log::metadot_trace(string)", l_trace);
    mono_add_internal_call("MetaDotManagedCore.Log::metadot_info(string)", l_info);
    mono_add_internal_call("MetaDotManagedCore.Log::metadot_warn(string)", l_warn);
    mono_add_internal_call("MetaDotManagedCore.Log::metadot_error(string)", l_error);
    mono_add_internal_call("MetaDotManagedCore.Log::METADOT_COPY_FILE(string,string)", metadot_copy_file);
    mono_add_internal_call("MetaDotManagedCore.Log::METADOT_PROFILE_BEGIN_SESSION(string,string)", metadot_profile_begin_session);
    mono_add_internal_call("MetaDotManagedCore.Log::METADOT_PROFILE_END_SESSION()", metadot_profile_end_session);
    mono_add_internal_call("MetaDotManagedCore.Log::METADOT_CURRENT_CONFIG()", metadot_current_config);
}

void MonoLayer::onAttach() {
    std::string monoPath;

    auto monoDir = std::filesystem::current_path();
    if (monoDir.empty()) {
        METADOT_ERROR("Cannot find mono installation dir");
        return;
    }
    // mono_set_dirs((monoDir / "output" / "net").string().c_str(), (monoDir / "etc").string().c_str());
    mono_set_dirs("D:\\Projects\\Sandbox\\Dev\\output\\net", "");
    //  Init a domain
    domain = mono_jit_init("MonoScripter");
    if (!domain) {
        METADOT_ERROR("mono_jit_init failed");
        return;
    }

    initInternalCalls();

    // Open a assembly in the domain
    std::string assPath = ASS_NAME;
    // App::get().getSettings().loadSet("ManagedDLL_FileName",assPath,std::string("Managedd.dll"));
    // assPath = FUtil::getAbsolutePath(assPath.c_str());
    assembly = mono_domain_assembly_open(domain, assPath.c_str());
    if (!assembly) {
        METADOT_ERROR("mono_domain_assembly_open failed");
        return;
    }

    // Get a image from the assembly
    image = mono_assembly_get_image(assembly);
    if (!image) {
        METADOT_ERROR("mono_assembly_get_image failed");
        return;
    }
    MonoClass *entryClass = mono_class_from_name(image, "MetaDotManagedCore", "Entry");
    if (!entryClass) {
        METADOT_ERROR("Cannot load Entry.cs");
    }
    MonoBoolean b = hotSwapEnable;
    void *array = {&b};
    entryInstance = callCSMethod("MetaDotManagedCore.Entry:Init", nullptr, &array);
    if (!entryInstance) {
        METADOT_ERROR("callCSMethod entryInstance failed");
        return;
    }

    /*
        {
            //Get the class
            MonoClass* dogclass;
            dogclass = mono_class_from_name(image, "", "Dog");
            if (!dogclass)
            {
                std::cout << "mono_class_from_name failed" << std::endl;
                system("pause");
                return 1;
            }

            //Create a instance of the class
            MonoObject* dogA;
            dogA = mono_object_new(domain, dogclass);
            if (!dogA)
            {
                std::cout << "mono_object_new failed" << std::endl;
                system("pause");
                return 1;
            }

            //Call its default constructor
            mono_runtime_object_init(dogA);


            //Build a method description object
            MonoObject* result;
            MonoMethodDesc* BarkMethodDesc;
            char* BarkMethodDescStr = "Dog:Bark(int)";
            BarkMethodDesc = mono_method_desc_new(BarkMethodDescStr, NULL);
            if (!BarkMethodDesc)
            {
                std::cout << "mono_method_desc_new failed" << std::endl;
                system("pause");
                return 1;
            }

            //Search the method in the image
            MonoMethod* method;
            method = mono_method_desc_search_in_image(BarkMethodDesc, image);
            if (!method)
            {
                std::cout << "mono_method_desc_search_in_image failed" << std::endl;
                system("pause");
                return 1;
            }

            //Set the arguments for the method
            void* args[1];
            int barkTimes = 10;
            args[0] = &barkTimes;



            MonoObject * pException = NULL;
            //Run the method
            std::cout << "Running the method: " << BarkMethodDescStr << std::endl;
            mono_runtime_invoke(method, dogA, args, &pException);
            if(pException)
            {
                system("pause");
            }

        }*/
    callEntryMethod("OnAttach", entryInstance);

    int size = 0;
    try {
        size = *(int *)mono_object_unbox(callCSMethod("MetaDotManagedCore.Entry:GetLayersSize", entryInstance));
    } catch (...) {
        METADOT_ERROR("Cannot load mono GetLayersSize");
        happyLoad = false;
    }
    happyLoad = size || hotSwapEnable;
}

void MonoLayer::onDetach() { callEntryMethod("OnDetach", entryInstance); }

void MonoLayer::onUpdate() { callEntryMethod("OnUpdate", entryInstance); }

void MonoLayer::reloadAssembly() { callEntryMethod("ReloadAssembly", entryInstance); }

bool MonoLayer::isMonoLoaded() { return happyLoad; }

#if 0
{

#define VAR_HERE() int, float, std::string
    using Type_Tuple = std::tuple<VAR_HERE()>;

    Type_Tuple tpt;
    std::apply([&](auto &&...args) {
        (printf("this type %s\n", typeid(args).name()), ...);
    },
               tpt);

}
#endif

void Scripting::Init() {
    Lua = new LuaCore;
    InitLua(Lua);

    Mono.onAttach();
}

void Scripting::End() {
    EndLua(Lua);
    delete Lua;

    Mono.onDetach();
}

void Scripting::Update() {
    ME_ASSERT_E(Lua);
    // luaL_loadstring(_struct->L, s_couroutineFileSrc.c_str());
    // if (metadot_debug_pcall(_struct->L, 0, LUA_MULTRET, 0) != LUA_OK) {
    //     print_error(_struct->L);
    //     return;
    // }
    auto &luawrap = Lua->s_lua;
    auto OnUpdate = luawrap["OnUpdate"];
    OnUpdate();

    Mono.onUpdate();
}

void Scripting::UpdateRender() {
    ME_ASSERT_E(Lua);
    auto &luawrap = Lua->s_lua;
    auto OnRender = luawrap["OnRender"];
    OnRender();
}

void Scripting::UpdateTick() {
    ME_ASSERT_E(Lua);
    auto &luawrap = Lua->s_lua;
    auto OnGameTickUpdate = luawrap["OnGameTickUpdate"];
    OnGameTickUpdate();
}
