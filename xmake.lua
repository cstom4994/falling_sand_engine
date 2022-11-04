
set_project("MetaDot.Runtime")

add_requires("libsdl", {configs = {shared = false}, verify = true})
add_requires("zlib", {configs = {shared = false}, verify = true})

add_rules("plugin.vsxmake.autoupdate")

set_policy("check.auto_ignore_flags", true)

set_languages("c17", "c++20")
set_arch("x64")

add_rules("mode.debug", "mode.release")

rule("metadot.uidsl")
    set_extensions('.uidsl')
    on_load(function(target)
        local outdir = path.join(path.join(os.projectdir(), "Source/Generated"), "uidsl")
        if not os.isdir(outdir) then
            os.mkdir(outdir)
        end
        target:set('policy', 'build.across_targets_in_parallel', false)
        target:add('deps', 'luaexe')
        --target:add("includedirs", path.join(os.projectdir(), "Source/Generated"))
    end)
    before_buildcmd_file(function(target, batchcmds, srcfile, opt)
        import('core.project.project')
        local outdir = path.join(path.join(os.projectdir(), "Source/Generated"), "uidsl")
        --target:add("includedirs", path.join(os.projectdir(), "Source/Generated"))

        batchcmds:show_progress(opt.progress, "${color.build.object}Generating UIDSL %s", srcfile)
        local name=srcfile:match('[\\/]?(%w+)%.%w+$')
        local headerpath=path.join(outdir, name:lower()..'.h')
        local implpath=path.join(outdir, name:lower()..'_imgui_inspector.cpp')
        local outfile=os.projectdir()..'/'..path(srcfile)

        local args = {'-e', 'package.path="'..path.join(os.projectdir(), "Source/Engine/IMGUI"):gsub('\\','/')..'/?.lua"',
                        path.join(path.join(os.projectdir(), "Source/Engine/IMGUI"), 'uidslparser.lua'),
                        '-H', path(headerpath), '-I', path(implpath), '--cpp', outfile}
        batchcmds:vrunv(project.target('luaexe'):targetfile(), args)

        -- local objfile=target:objectfile(implpath)
        -- table.insert(target:objectfiles(), objfile)
        -- batchcmds:compile(implpath, objfile)

        batchcmds:add_depfiles(srcfile)
        local dependfile = target:dependfile(implpath)
        batchcmds:set_depmtime(os.mtime(dependfile))
        batchcmds:set_depcache(dependfile)
    end)
rule_end()

if is_mode("debug") then
    add_defines("DEBUG", "_DEBUG")
    set_optimize("none")
elseif is_mode("release") then
    add_defines("NDEBUG")
    set_optimize("faster")
end

set_fpmodels("strict")

if (is_os("windows")) then 

    set_toolchains("clang-cl")

    add_defines("_WINDOWS")
    add_defines("UNICODE")
    add_defines("_UNICODE")
    add_defines("NOMINMAX")
    add_defines("_CRT_SECURE_NO_DEPRECATE")
    add_defines("_CRT_SECURE_NO_WARNINGS")
    add_defines("_CRT_NONSTDC_NO_DEPRECATE")
    add_defines("_SCL_SECURE_NO_WARNINGS")
    add_defines("WIN32_LEAN_AND_MEAN")
    if (is_mode("release")) then
        set_runtimes("MD")
    else
        set_runtimes("MDd")
    end

    add_cxxflags(
    "/wd4267", "/wd4244", "/wd4305", "/wd4018", 
    "/wd4800", "/wd5030", "/wd5222", "/wd4554",
    "/wd4002",
    "/utf-8", "/Zc:__cplusplus", "/EHa",
    "/Za", "/Ze",
    "/fp:precise"
    )

    add_cxflags("/bigobj")

    link_list = {
        "DbgHelp",
        "winmm",
        "opengl32",
        "kernel32",
        "user32",
        "gdi32",
        "iphlpapi",
        "Shlwapi",
        "wsock32",
        "ws2_32",
        "shell32",
        "advapi32",
        "imm32",
        "bcrypt",
        "Avrt",
        "dwmapi",
        "Version",
        "Usp10",
        "userenv",
        "psapi",
        "setupapi",
        "ole32",
        "oleaut32",
    }
elseif (is_os("linux")) then

    set_toolchains("clang")

    add_defines("__linux")
    add_cxflags("-fPIC")
    link_list = {}
elseif (is_os("macosx")) then
    set_toolchains("clang")

    add_cxflags("-fPIC")
    link_list = {}
end

add_cxflags("-fstrict-aliasing", "-fomit-frame-pointer", "-Wmicrosoft-cast", "-fpermissive", "-Wunqualified-std-cast-call", "-ffp-contract=on", "-fno-fast-math")

include_dir_list = {
    "Source",
    "Source/Generated",
    "Source/Engine",
    "Source/Libs/lua/lua",
    "Source/Vendor",
    "Source/Vendor/imgui",
    "Source/Vendor/stb",
    "Source/Vendor/enet",
    "Source/Vendor/box2d/include",
    "Source/Vendor/json/include",
    "Source/Vendor/fmt/include"
    }

defines_list = {
    "_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING",
    "IMGUI_IMPL_OPENGL_LOADER_GLAD",
    "IMGUI_IMPL_OPENGL_LOADER_CUSTOM",
    "MINIZ_NO_ZLIB_COMPATIBLE_NAMES"
}

target("vendor")
    set_kind("static")
    if (not is_os("macosx")) then
        add_rules("c.unity_build")
        add_rules("c++.unity_build")    
    end
    add_packages("libsdl", "zlib")
    add_includedirs(include_dir_list)
    add_defines(defines_list)
    add_files("Source/Vendor/**.c")
    add_files("Source/Vendor/**.cc")
    add_files("Source/Vendor/**.cpp")
	add_headerfiles("Source/Vendor/**.h")
	add_headerfiles("Source/Vendor/**.hpp")
    if (is_os("linux") or is_os("macosx")) then
        remove_files("Source/Vendor/minizip/iowin32.c")
    end
    remove_files("Source/Vendor/fmt/src/fmt.cc")
    set_symbols("debug")

target("lua")
    set_kind("static")
    add_rules("c.unity_build")
    add_includedirs(include_dir_list)
    add_defines(defines_list)
    add_files("Source/Libs/lua/lua/*.c|lua.c|luac.c|onelua.c")

target("luaexe")
    set_basename("lua54")
    set_kind("binary")
    add_includedirs(include_dir_list)
    add_defines(defines_list)
    add_files("Source/Libs/lua/lua/lua.c")
    add_deps("lua")

target("Libs")
    set_kind("static")
    add_rules("c.unity_build", {batchsize = 0})
    add_rules("c++.unity_build", {batchsize = 0})
    add_includedirs(include_dir_list)
    add_defines(defines_list)
    add_packages("libsdl", "zlib")
    add_files("Source/Libs/*.cpp", "Source/Libs/*.cc", "Source/Libs/*.c")
    add_files("Source/Libs/FastNoise/**.cpp", "Source/Libs/ImGui/**.cpp", "Source/Libs/lua/**.c", "Source/Libs/lua/**.cpp", {unity_group = "libone"})
    add_files("Source/Libs/libxlsxwriter/**.c", {unity_group = "libxlsxwriter"})
    add_files("Source/Libs/physfs/**.c")
	add_headerfiles("Source/Libs/**.h")
	add_headerfiles("Source/Libs/**.hpp")
    remove_files("Source/Libs/lua/lua/**")
    set_symbols("debug")

target("CppSource")
    set_kind("shared")
    add_packages("libsdl")
    set_targetdir("./output")
    add_includedirs(include_dir_list)
    add_defines(defines_list)
    add_deps("vendor", "Libs")
    add_links(link_list)
    add_files("Source/CppScript/**.cpp")
	add_headerfiles("Source/CppScript/**.h")
	add_headerfiles("Source/CppScript/**.hpp")
	add_headerfiles("Source/Shared/**.hpp")
    set_symbols("debug")

target("Engine")
    set_kind("static")
    add_packages("libsdl")
    add_includedirs(include_dir_list)
    add_defines(defines_list)
    add_files("Source/Engine/**.cpp")
    add_headerfiles("Source/Engine/**.h")
    add_headerfiles("Source/Engine/**.hpp")
    add_headerfiles("Source/Engine/**.inl")
    add_files('Source/Engine/IMGUI/uidslexpr.lua', {rule='utils.bin2c'})
    set_symbols("debug")

target("MetaDot")
    set_kind("binary")
    add_rules("metadot.uidsl")
    add_packages("libsdl")
    set_targetdir("./output")
    add_includedirs(include_dir_list)
    add_defines(defines_list)
    add_deps("vendor", "Libs", "lua", "Engine")
    add_links(link_list)
    add_files("Source/Generated/**.cpp")
    add_files("Source/Game/**.cpp")
    add_files("Source/Game/uidsl/*.uidsl")
	add_headerfiles("Source/Game/**.h")
	add_headerfiles("Source/Game/**.hpp")
	add_headerfiles("Source/Game/**.inl")
    add_headerfiles("Source/Shared/**.hpp")
    set_symbols("debug")