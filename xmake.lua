set_project("MetaDot")

add_rules("plugin.vsxmake.autoupdate")

set_policy("check.auto_ignore_flags", true)

set_languages("c17", "c++20")

add_rules("mode.debug", "mode.release")

if is_os("windows") then
	add_requires("vcpkg::sdl2")
	add_packages("vcpkg::sdl2")
	-- add_requires("vcpkg::pthreads")
	-- add_packages("vcpkg::pthreads")
else
	add_requires("libsdl")
	add_packages("libsdl")
end

option("build_unity")
set_default(false)
set_description("Toggle to enable unity build")
option_end()

option("build_audio")
set_default(false)
set_description("Toggle to enable audio module")
option_end()

-- rule("metadot.uidsl")
-- set_extensions('.uidsl')
-- on_load(function(target)
--     local outdir = path.join(path.join(os.projectdir(), "Source/Generated"),
--                              "uidsl")
--     if not os.isdir(outdir) then os.mkdir(outdir) end
--     target:set('policy', 'build.across_targets_in_parallel', false)
--     target:add('deps', 'luaexe')
--     -- target:add("includedirs", path.join(os.projectdir(), "Source/Generated"))
-- end)
-- before_buildcmd_file(function(target, batchcmds, srcfile, opt)
--     import('core.project.project')
--     local outdir = path.join(path.join(os.projectdir(), "Source/Generated"),
--                              "uidsl")
--     -- target:add("includedirs", path.join(os.projectdir(), "Source/Generated"))

--     batchcmds:show_progress(opt.progress,
--                             "${color.build.object}Generating UIDSL %s", srcfile)
--     local name = srcfile:match('[\\/]?(%w+)%.%w+$')
--     local headerpath = path.join(outdir, name:lower() .. '.h')
--     local implpath = path.join(outdir, name:lower() .. '_imgui_inspector.cpp')
--     local outfile = os.projectdir() .. '/' .. path(srcfile)

--     local args = {
--         '-e', 'package.path="' ..
--             path.join(os.projectdir(), "Source/engine/UserInterface/IMGUI"):gsub(
--                 '\\', '/') .. '/?.lua"',
--         path.join(
--             path.join(os.projectdir(), "Source/engine/UserInterface/IMGUI"),
--             'uidslparser.lua'), '-H', path(headerpath), '-I', path(implpath),
--         '--cpp', outfile
--     }
--     batchcmds:vrunv(project.target('luaexe'):targetfile(), args)

--     -- local objfile=target:objectfile(implpath)
--     -- table.insert(target:objectfiles(), objfile)
--     -- batchcmds:compile(implpath, objfile)

--     batchcmds:add_depfiles(srcfile)
--     local dependfile = target:dependfile(implpath)
--     batchcmds:set_depmtime(os.mtime(dependfile))
--     batchcmds:set_depcache(dependfile)
-- end)
-- rule_end()

if is_mode("debug") then
	add_defines("DEBUG", "_DEBUG")
	set_optimize("none")
	set_symbols("debug")
elseif is_mode("release") then
	add_defines("NDEBUG")
	set_optimize("faster")
end

set_fpmodels("strict")
set_exceptions("cxx", "objc")

if is_os("windows") then
	set_arch("x64")
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
	add_defines("_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING")

	if is_mode("release") then
		set_runtimes("MD")
	else
		set_runtimes("MDd")
	end

	add_cxxflags(
		"/wd4267",
		"/wd4244",
		"/wd4305",
		"/wd4018",
		"/wd4800",
		"/wd5030",
		"/wd5222",
		"/wd4554",
		"/wd4002",
		"/utf-8",
		"/Zc:__cplusplus",
		"/Za"
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
elseif is_os("linux") then
	set_arch("x86_64")

	set_toolchains("clang")

	add_defines("__linux")
	add_cxflags("-fPIC")
	link_list = {}
elseif is_os("macosx") then
	set_arch("aarch64")

	set_toolchains("clang")

	add_cxflags("-fPIC")

	-- add_mxflags("-fno-objc-arc", {force = true})
	-- add_frameworks("CoreFoundation", "Cocoa", "IOKit", "Metal", "MetalKit", "QuartzCore", "AudioToolBox", {public = true})

	if is_arch("arm.*") then
		add_defines("__METADOT_ARCH_ARM")
	elseif is_arch("x86_64", "i386") then
		add_defines("__METADOT_ARCH_X86")
	end

	link_list = {}
end

if has_config("build_audio") then
	add_defines("METADOT_BUILD_AUDIO")
	if is_os("macosx") then
		add_linkdirs("output")
		add_rpathdirs("./")
		add_rpathdirs("./output")
	end
end

-- add_cxflags("-fstrict-aliasing", "-fomit-frame-pointer", "-Wmicrosoft-cast", "-fpermissive", "-Wunqualified-std-cast-call", "-ffp-contract=on", "-fno-fast-math")

include_dir_list = {
	"Source",

	"Source/libs/libcss/libcss/include",
	"Source/libs/libcss/libcss/src",
	"Source/libs/libcss/libparserutils/include",
	"Source/libs/libcss/libparserutils/src",
	"Source/libs/libcss/libwapcaplet/include",
	"Source/libs/libcss/libwapcaplet/src",
}

defines_list = {}

target("MetaDot")
do
	if has_config("build_unity") then
		add_rules("c.unity_build", { batchsize = 4 })
		add_rules("c++.unity_build", { batchsize = 4 })
	else
		add_rules("c.unity_build", { batchsize = 0 })
		add_rules("c++.unity_build", { batchsize = 0 })
	end
	set_kind("binary")
	set_targetdir("./output")
	add_includedirs(include_dir_list)
	add_defines(defines_list)

	add_links(link_list)

	add_files("Source/libs/*.cpp")
	add_files("Source/libs/*.c")
	add_files("Source/libs/ImGui/**.cpp", "Source/libs/ImGui/**.c", "Source/libs/glad/**.c")
	add_files("Source/libs/physfs/**.c")
	if is_os("macosx") then
		add_files("Source/libs/physfs/**.m")
	end
	add_files("Source/libs/libcss/**.c")
	add_files("Source/libs/lz4/**.c")
	add_files("Source/libs/lua/host/**.c")
	add_files("Source/libs/lua/*.c")

	add_files("Source/*.c")
	add_files("Source/*.cpp")

	add_files("Source/core/**.c")
	add_files("Source/core/**.cpp")
	add_files("Source/ecs/**.c")
	-- add_files("Source/ecs/**.cpp")
	-- add_files("Source/game_utils/**.c")
	add_files("Source/game_utils/**.cpp")
	add_files("Source/internal/**.c")
	add_files("Source/internal/**.cpp")
	-- add_files("Source/meta/**.c")
	add_files("Source/meta/**.cpp")
	add_files("Source/renderer/**.c")
	add_files("Source/renderer/**.cpp")
	add_files("Source/scripting/**.c")
	add_files("Source/scripting/**.cpp")
	add_files("Source/imgui/**.cpp")

	add_headerfiles("Source/**.h")
	add_headerfiles("Source/**.hpp")

	if is_os("macosx") and has_config("build_audio") then
		add_links("fmod", "fmodstudio")
	end
end

-- target("TestFFI")
-- do
--     set_kind("shared")
--     set_targetdir("./output")
--     add_includedirs(include_dir_list)
--     add_defines(defines_list)
--     add_files("Source/tests/test_ffi.c")
--     add_headerfiles("Source/tests/**.h")
-- end

-- target("TestCOBJ")
-- do
--     set_kind("binary")
--     set_targetdir("./output")
--     add_includedirs(include_dir_list)
--     add_defines(defines_list)
--     add_deps("libs")
--     add_files("Source/tests/test_cobj.c")
--     add_files("Source/engine/engine_meta.c")
--     add_headerfiles("Source/tests/**.h")
-- end

-- target("TestPromise")
-- do
--     set_kind("binary")
--     set_targetdir("./output")
--     add_includedirs(include_dir_list)
--     add_defines(defines_list)
--     add_deps("libs")
--     add_files("Source/tests/test_promise.cpp")
--     add_headerfiles("Source/tests/**.h")
-- end

-- target("TestLayout")
-- do
--     set_kind("binary")
--     set_targetdir("./output")
--     add_includedirs(include_dir_list)
--     add_defines(defines_list)
--     add_deps("libs")
--     add_files("Source/tests/test_layout.c")
--     add_files("Source/engine/ui_layout.c")
--     add_headerfiles("Source/tests/**.h")
-- end

-- target("TestTween")
-- do
--     set_kind("binary")
--     set_targetdir("./output")
--     add_includedirs(include_dir_list)
--     add_defines(defines_list)
--     add_deps("libs")
--     add_files("Source/tests/test_tween.cpp")
--     add_headerfiles("Source/tests/**.h")
-- end
