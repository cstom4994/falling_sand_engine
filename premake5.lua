--
-- Sandbox
--
workspace "Sandbox"
configurations {"Debug", "Release"}

buildoptions {"/utf-8", "/Zc:__cplusplus", "/permissive", "/bigobj"}
disablewarnings {"4005", "4267", "4244", "4305", "4018", "4800", "5030", "5222", "4554", "4002", "4099"}

cppdialect "C++20"
cdialect "C17"

-- characterset("MBCS")

location "vsbuild"

flags {"MultiProcessorCompile"}

staticruntime "on"

win32_libs = {"DbgHelp", "winmm", "opengl32", "kernel32", "user32", "gdi32", "iphlpapi", "Shlwapi", "wsock32", "ws2_32",
              "shell32", "advapi32", "imm32", "bcrypt", "Avrt", "dwmapi", "Version", "Usp10", "userenv", "psapi",
              "setupapi", "ole32", "oleaut32"}

defines {"WIN32", "_WIN32", "_WINDOWS", "NOMINMAX", "UNICODE", "_UNICODE", "_CRT_SECURE_NO_DEPRECATE",
         "_CRT_SECURE_NO_WARNINGS", "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS", "WIN32_LEAN_AND_MEAN",
         "_SCL_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE", "_CRT_SILENCE_NONCONFORMING_TGMATH_H"}

platforms {"Win64"}

filter "configurations:Debug"
defines {"_DEBUG", "DEBUG", "_CRTDBG_MAP_ALLOC"}
symbols "On"
libdirs {"dependencies/SDL2/lib/debug"}

filter "configurations:Release"
defines {"_NDEBUG", "NDEBUG"}
optimize "On"
flags {"NoRuntimeChecks"}
libdirs {"dependencies/SDL2/lib/release"}

-- if is_arch("arm.*") then
--        add_defines("__METADOT_ARCH_ARM")
-- elseif is_arch("x86_64", "i386") then
--        add_defines("__METADOT_ARCH_X86")
-- end

-- filter "platforms:Win32"
-- system "Windows"
-- architecture "x86"

filter "platforms:Win64"
system "Windows"
architecture "x86_64"
libdirs {"dependencies/libffi/lib", "dependencies/fmod/lib"}

objdir "vsbuild/obj/%{cfg.platform}_%{cfg.buildcfg}"

includedirs {"source"}

includedirs {"dependencies/SDL2/include", "dependencies/libffi/include", "dependencies/fmod/include"}

----------------------------------------------------------------------------
-- projects
----------------------------------------------------------------------------

project "external"
do
    kind "StaticLib"
    language "C++"

    files {"source/libs/*.cpp", "source/libs/fastnoise/*.cpp", "source/libs/*.c", "source/libs/ImGui/**.cpp",
           "source/libs/ImGui/**.c", "source/libs/glad/**.c", "source/libs/lz4/**.c", "source/libs/lua/**.c",
           "source/libs/external/*.c"}

    files {"source/libs/**.h", "source/libs/**.hpp"}

    files {"dependencies/SDL2/include/**.**", "dependencies/libffi/include/**.**", "dependencies/fmod/include/**.**"}

    vpaths {
        ["libs/*"] = {"source/libs"},
        ["deps/sdl2/*"] = {"dependencies/SDL2/include"},
        ["deps/libffi/*"] = {"dependencies/libffi/include"},
        ["deps/fmod/*"] = {"dependencies/fmod/include"}
    }

end

project "MetaDot"
do
    kind "ConsoleApp"
    language "C++"
    targetdir "output"
    debugdir "output/../"

    files {"source/engine/**.cpp", "source/engine/**.c", "source/engine/**.h", "source/engine/**.hpp"}
    files {"source/game/**.cpp", "source/game/**.c", "source/game/**.h", "source/game/**.hpp"}

    files {"data/scripts/**.lua", "data/shaders/**.**"}

    files {"premake5.lua"}

    vpaths {
        ["engine/*"] = {"source/engine"},
        ["game/*"] = {"source/game"},
        ["scripts/*"] = {"data/scripts"},
        ["shaders/*"] = {"data/shaders"},
        ["*"] = {"premake5.lua"}
    }

    links {"external", "SDL2", "ffi", "fmod_vc", "fmodstudio_vc", win32_libs}
end
