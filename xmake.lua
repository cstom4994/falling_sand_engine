
set_project("MetaDot.Runtime")

add_requires("libsdl", {configs = {shared = false}, verify = true})

add_rules("plugin.vsxmake.autoupdate")

set_policy("check.auto_ignore_flags", true)

set_languages("clatest", "c++20")
set_arch("x64")

add_rules("mode.debug", "mode.release")

rule("csharp")
	set_extensions(".csproj")
	on_build_file(function (target, sourcefile)
		os.execv("dotnet", {"build", sourcefile, "-o", target:targetdir()})
	end)
	on_clean(function (target, sourcefile)
		os.execv("dotnet", {"clean", sourcefile, "-o", target:targetdir()})
	end)
	on_link(function () end)
rule_end()

rule("uidsl")
    set_extensions('.lua')
    on_load(function(target)
        local outdir = path.join(target:autogendir(), "uidsl")
        if not os.isdir(outdir) then
            os.mkdir(outdir)
        end
        target:set('policy', 'build.across_targets_in_parallel', false)
        target:add('deps', 'luaexe')
        target:add("includedirs", target:autogendir())
    end)
    before_buildcmd_file(function(target, batchcmds, srcfile, opt)
        import('core.project.project')
        local outdir = path.join(target:autogendir(), "uidsl")
        target:add("includedirs", target:autogendir())

        batchcmds:show_progress(opt.progress, "${color.build.object}processing \"%s\"", srcfile)
        local name=srcfile:match('[\\/]?(%w+)%.%w+$')
        local headerpath=path.join(outdir, name:lower()..'.h')
        local implpath=path.join(outdir, name:lower()..'_imgui_inspector.cpp')

        local args = {'-e', 'package.path="'..path.join(os.scriptdir(), "Source/Engine/IMGUI"):gsub('\\','/')..'/?.lua"',
                        path.join(path.join(os.scriptdir(), "Source/Engine/IMGUI"), 'uidslparser.lua'),
                        '-H', path(headerpath), '-I', path(implpath), '--cpp',
                        os.projectdir()..'/'..path(srcfile)}
        batchcmds:vrunv(project.target('luaexe'):targetfile(), args)

        local objfile=target:objectfile(implpath)
        table.insert(target:objectfiles(), objfile)
        batchcmds:compile(implpath, objfile)

        batchcmds:add_depfiles(srcfile,headerpath,implpath)
        batchcmds:set_depmtime(os.mtime(objfile))
        batchcmds:set_depcache(target:dependfile(objfile))
    end)
rule_end()

if is_mode("debug") then
    add_defines("CET_DEBUG")
    set_optimize("none")
elseif is_mode("release") then
    add_defines("NDEBUG")
    set_optimize("fastest")
end

if (is_os("windows")) then 
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
    "/utf-8", "/Zc:__cplusplus", "/EHa"
    )

    add_cxflags("/bigobj")
elseif (is_os("linux")) then
    error("No more linux for now")
end

include_dir_list = {
    "Source",
    "Source/Engine",
    "Source/Libs/lua/lua",
    "Source/Vendor",
    "Source/Vendor/imgui",
    "Source/Vendor/stb",
    "Source/Vendor/enet",
    "Source/Vendor/box2d/include",
    "Source/Vendor/json/include",
    "Source/Vendor/coreclr",
    "Source/Vendor/fmt/include"
    }

defines_list = {
    "_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING",
    "IMGUI_IMPL_OPENGL_LOADER_GLAD",
    "IMGUI_IMPL_OPENGL_LOADER_CUSTOM",
    "SDL_GPU_DISABLE_GLES",
}

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

target("vendor")
    set_kind("static")
    add_packages("libsdl")
    add_includedirs(include_dir_list)
    add_defines(defines_list)
    add_files("Source/Vendor/**.c")
    add_files("Source/Vendor/**.cc")
    add_files("Source/Vendor/**.cpp")
    remove_files("Source/Vendor/fmt/src/fmt.cc")
	add_headerfiles("Source/Vendor/**.h")
	add_headerfiles("Source/Vendor/**.hpp")
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

target("CoreCLREmbed")
    set_kind("static")
    add_includedirs(include_dir_list)
    add_defines(defines_list)
    add_files("Source/CoreCLREmbed/**.cpp")
	add_headerfiles("Source/CoreCLREmbed/**.h")
	add_headerfiles("Source/CoreCLREmbed/**.hpp")
	add_headerfiles("Source/Vendor/coreclr/**.h")
    set_symbols("debug")

target("Libs")
    set_kind("static")
    add_includedirs(include_dir_list)
    add_defines(defines_list)
    add_files("Source/Libs/**.cpp")
    add_files("Source/Libs/**.cc")
    add_files("Source/Libs/**.c")
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
    add_deps("vendor")
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
	add_headerfiles("Source/Engine/**.inc")
    add_files('Source/Engine/IMGUI/uidslexpr.lua', {rule='utils.bin2c'})
    set_symbols("debug")

target("MetaDot")
    set_kind("binary")
    add_rules("uidsl")
    add_packages("libsdl")
    set_targetdir("./output")
    add_includedirs(include_dir_list)
    add_defines(defines_list)
    add_deps("vendor", "Libs", "lua", "Engine", "CoreCLREmbed")
    add_links("nethost")
    add_links(link_list)
    add_files("Source/Game/**.cpp")
	add_headerfiles("Source/Game/**.h")
	add_headerfiles("Source/Game/**.hpp")
	add_headerfiles("Source/Game/**.inl")
	add_headerfiles("Source/Game/**.inc")
    add_files('Source/Game/uidsl/*.lua')
    add_headerfiles("Source/Shared/**.hpp")
    add_rules("utils.bin2c", {extensions = {".ttf"}})
    add_files("Resources/**.ttf")
	add_headerfiles("Resources/**.h")
    add_linkdirs("Source/Vendor/coreclr")
    set_symbols("debug")

target("ManagedLib")
	set_kind("binary")
	add_rules("csharp")
    set_targetdir("./output")
	add_files("Source/ManagedLib/**.csproj")