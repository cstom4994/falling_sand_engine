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
defines {"_DEBUG", "DEBUG"}
symbols "On"

filter "configurations:Release"
defines {"_NDEBUG", "NDEBUG"}
optimize "On"
flags {"NoRuntimeChecks"}

MONO_STATIC = 0

local mono_libs = {}
if (MONO_STATIC == 0) then
mono_libs = {"coreclr.import"}
else
mono_libs = {"monosgen-2.0", "mono-component-debugger-static", "mono-component-diagnostics_tracing-static",
             "mono-component-hot_reload-static"}
end

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
libdirs {"dependencies/SDL2/lib/x64", "dependencies/libffi/lib", "dependencies/mono/lib"}

objdir "vsbuild/obj/%{cfg.platform}_%{cfg.buildcfg}"

includedirs {"source"}

includedirs {"dependencies/SDL2/include", "dependencies/libffi/include", "dependencies/mono/include"}

----------------------------------------------------------------------------
-- projects
----------------------------------------------------------------------------

project "MetaDotLibs"
do
    kind "StaticLib"
    language "C++"

    files {"source/libs/*.cpp", "source/libs/*.c", "source/libs/ImGui/**.cpp", "source/libs/ImGui/**.c",
           "source/libs/glad/**.c", "source/libs/lz4/**.c", "source/libs/lua/**.c", "source/libs/external/*.c"}

    files {"source/libs/**.h", "source/libs/**.hpp"}

end

project "MetaDot"
do
    kind "ConsoleApp"
    language "C++"
    targetdir "output"
    debugdir "output/../"

    files {"source/engine/**.cpp", "source/engine/**.c", "source/engine/**.h", "source/engine/**.hpp"}

    links {"MetaDotLibs", "SDL2", "ffi", mono_libs, win32_libs}
end

project "ManagedCore"
do
    kind "SharedLib"
    language "C#"
    targetdir "output"
    debugdir "output/../"

    clr "Unsafe"

    dotnetframework "4.8"

    files {"source/managed/core/**.cs"}

    warnings "off"
end

project "Managed"
do
    kind "SharedLib"
    language "C#"
    targetdir "output"
    debugdir "output/../"

    clr "Unsafe"

    links {"ManagedCore"}

    dotnetframework "4.8"

    files {"source/managed/runtime/**.cs"}

    warnings "off"
end
