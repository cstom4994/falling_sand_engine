// Copyright(c) 2022, KaoruXun All rights reserved.

#include "ImGuiLayer.hpp"
#include "Engine/Memory/Memory.hpp"
#include "Engine/UserInterface/IMGUI/ImGuiBase.hpp"
#include "Game/Const.hpp"
#include "Core/Core.hpp"
#include "Core/Global.hpp"
#include "Game/InEngine.h"
#include "Game/Legacy/Game.hpp"
#include "Game/Legacy/GameUI.hpp"
#include "Game/Legacy/Networking.hpp"
#include "Core/Macros.hpp"
#include "Game/Textures.hpp"
#include "Game/Utils.hpp"
#include "ImGui/imgui.h"
#include "Libs/ImGui/implot.h"
#include "Settings.hpp"

#include "glew.h"

#include <cstddef>
#include <cstdio>
#include <map>

#include <imgui/IconsFontAwesome5.h>


#if defined(_METADOT_IMM32)

#if defined(_WIN32)
#include <CommCtrl.h>
#include <Windows.h>
#include <vcruntime_string.h>
#endif /* defined( _WIN32 ) */

static int common_control_initialize() {
    HMODULE comctl32 = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"Comctl32.dll", &comctl32)) {
        return EXIT_FAILURE;
    }

    assert(comctl32 != nullptr);
    if (comctl32) {
        {
            typename std::add_pointer<decltype(InitCommonControlsEx)>::type lpfnInitCommonControlsEx =
                    reinterpret_cast<typename std::add_pointer<decltype(InitCommonControlsEx)>::type>(GetProcAddress(comctl32, "InitCommonControlsEx"));

            if (lpfnInitCommonControlsEx) {
                const INITCOMMONCONTROLSEX initcommoncontrolsex = {sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES};
                if (!lpfnInitCommonControlsEx(&initcommoncontrolsex)) {
                    assert(!" InitCommonControlsEx(&initcommoncontrolsex) ");
                    return EXIT_FAILURE;
                }
                //OutputDebugStringW(L"initCommonControlsEx Enable\n");
                return 0;
            }
        }
        {
            InitCommonControls();
            //OutputDebugStringW(L"initCommonControls Enable\n");
            return 0;
        }
    }
    return 1;
}

#endif

#if defined(METADOT_EMBEDRES)

static unsigned char font_fz[] = {
#include "FZXIANGSU12.ttf.h"
};

static unsigned char font_silver[] = {
#include "Silver.ttf.h"
};

static unsigned char font_fa[] = {
#include "fa_solid_900.ttf.h"
};

#endif


namespace layout {
    constexpr auto kRenderOptionsPanelWidth = 300;
    constexpr auto kMargin = 5;
    constexpr auto kPanelSpacing = 10;
    constexpr auto kColorWidgetWidth = 250.0F;
}// namespace layout

static bool s_is_animaiting = false;
static const int view_size = 256;


ImVec2 ImGuiLayer::GetNextWindowsPos(ImGuiWindowTags tag, ImVec2 pos) {

    if (tag & UI_MainMenu)
        ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImVec2 windowspos = ImGui::GetPlatformIO().Platform_GetWindowSize(ImGui::GetMainViewport());
#if defined(METADOT_PLATFORM_APPLE)// macOS retina
        windowspos /= 4;
#endif
        pos += windowspos;
    }

    return pos;
}

ImGuiLayer::ImGuiLayer() {
}

class OpenGL3TextureManager {
public:
    ~OpenGL3TextureManager() {
        for (int i = 0; i < mTextures.size(); ++i) {
            GLuint tid = mTextures[i];
            glDeleteTextures(1, &tid);
        }
        mTextures.clear();
    }

    ImTextureID createTexture(void *pixels, int width, int height) {
        // Upload texture to graphics system
        GLuint texture_id = 0;
        GLint last_texture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_UNPACK_ROW_LENGTH
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glBindTexture(GL_TEXTURE_2D, last_texture);
        mTextures.reserve(mTextures.size() + 1);
        mTextures.push_back(texture_id);
        return (ImTextureID) (intptr_t) texture_id;
    }

    void deleteTexture(ImTextureID id) {
        GLuint tex = (GLuint) (intptr_t) id;
        glDeleteTextures(1, &tex);
    }

private:
    typedef ImVector<GLuint> Textures;

    Textures mTextures;
};

static bool firstRun = false;

#if defined(_METADOT_IMM32)
ImGUIIMMCommunication imguiIMMCommunication{};
#endif


// void TextFuck(std::string text)
// {

// 	ImGui::SetNextWindowPos(ImGui::GetMousePos());

// 	ImGuiWindowFlags flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
// 	ImGui::Begin("text", NULL, flags);
// 	ImGui::Text(text.c_str());
// 	ImGui::End();
// }

void ImGuiLayer::Init(C_Window *p_window, void *p_gl_context) {
    window = p_window;
    gl_context = p_gl_context;

    IMGUI_CHECKVERSION();

    //ImGui::SetAllocatorFunctions(myMalloc, myFree);

    m_imgui = ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO &io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    io.IniFilename = "imgui.ini";

    ImFontConfig config;
    config.OversampleH = 1;
    config.OversampleV = 1;
    config.PixelSnapH = 1;

    float scale = 1.0f;

    // Using cstd malloc because imgui will use default delete to destruct fonts' data

#if defined(METADOT_EMBEDRES)
    void *fonts_1 = malloc(sizeof(font_fz));
    void *fonts_2 = malloc(sizeof(font_silver));
    void *fonts_3 = malloc(sizeof(font_fa));

    memcpy(fonts_1, (void *) font_fz, sizeof(font_fz));
    memcpy(fonts_2, (void *) font_silver, sizeof(font_silver));
    memcpy(fonts_3, (void *) font_fa, sizeof(font_fa));

    io.Fonts->AddFontFromMemoryTTF(fonts_1, sizeof(font_fz), 16.0f, &config, io.Fonts->GetGlyphRangesChineseFull());

    config.MergeMode = true;
    config.GlyphMinAdvanceX = 10.0f;

    static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    io.Fonts->AddFontFromMemoryTTF(fonts_3, sizeof(font_fa), 15.0f, &config, icon_ranges);
    io.Fonts->AddFontFromMemoryTTF(fonts_2, sizeof(font_silver), 26.0f, &config);
#else

    io.Fonts->AddFontFromFileTTF(METADOT_RESLOC("data/assets/fonts/FZXIANGSU12.ttf").c_str(), 22.0f, &config, io.Fonts->GetGlyphRangesChineseFull());

    config.MergeMode = true;
    config.GlyphMinAdvanceX = 10.0f;

    static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    io.Fonts->AddFontFromFileTTF(METADOT_RESLOC("data/assets/fonts/fa_solid_900.ttf").c_str(), 18.0f, &config, icon_ranges);
    io.Fonts->AddFontFromFileTTF(METADOT_RESLOC("data/assets/fonts/Silver.ttf").c_str(), 32.0f, &config);

#endif


    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }


    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);

    const char *glsl_version = "#version 330 core";
    ImGui_ImplOpenGL3_Init(glsl_version);


    style.ScaleAllSizes(scale);

    ImGuiHelper::init_style(0.5f, 0.5f);


#if defined(_METADOT_IMM32)
    common_control_initialize();
    VERIFY(imguiIMMCommunication.subclassify(window));
#endif


    //registerWindow("NavBar", &dynamic_cast<FakeWindow*>(APwin())->m_enableNavigationBar);

    //	static const char* fileToEdit = "test.cpp";

    editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());


#if 0

        // set your own known preprocessor symbols...
        static const char* ppnames[] = { "NULL", "PM_REMOVE",
            "ZeroMemory", "DXGI_SWAP_EFFECT_DISCARD", "D3D_FEATURE_LEVEL", "D3D_DRIVER_TYPE_HARDWARE", "WINAPI","D3D11_SDK_VERSION", "assert" };
        // ... and their corresponding values
        static const char* ppvalues[] = {
            "#define NULL ((void*)0)",
            "#define PM_REMOVE (0x0001)",
            "Microsoft's own memory zapper function\n(which is a macro actually)\nvoid ZeroMemory(\n\t[in] PVOID  Destination,\n\t[in] SIZE_T Length\n); ",
            "enum DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_DISCARD = 0",
            "enum D3D_FEATURE_LEVEL",
            "enum D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE  = ( D3D_DRIVER_TYPE_UNKNOWN + 1 )",
            "#define WINAPI __stdcall",
            "#define D3D11_SDK_VERSION (7)",
            " #define assert(expression) (void)(                                                  \n"
            "    (!!(expression)) ||                                                              \n"
            "    (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \n"
            " )"
        };

        for (int i = 0; i < sizeof(ppnames) / sizeof(ppnames[0]); ++i)
        {
            TextEditor::Identifier id;
            id.mDeclaration = ppvalues[i];
            lang.mPreprocIdentifiers.insert(std::make_pair(std::string(ppnames[i]), id));
        }

        // set your own identifiers
        static const char* identifiers[] = {
            "HWND", "HRESULT", "LPRESULT","D3D11_RENDER_TARGET_VIEW_DESC", "DXGI_SWAP_CHAIN_DESC","MSG","LRESULT","WPARAM", "LPARAM","UINT","LPVOID",
            "ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer", "ID3D10Blob", "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
            "ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView", "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
            "IDXGISwapChain", "ID3D11RenderTargetView", "ID3D11Texture2D", "TextEditor" };
        static const char* idecls[] =
        {
            "typedef HWND_* HWND", "typedef long HRESULT", "typedef long* LPRESULT", "struct D3D11_RENDER_TARGET_VIEW_DESC", "struct DXGI_SWAP_CHAIN_DESC",
            "typedef tagMSG MSG\n * Message structure","typedef LONG_PTR LRESULT","WPARAM", "LPARAM","UINT","LPVOID",
            "ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer", "ID3D10Blob", "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
            "ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView", "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
            "IDXGISwapChain", "ID3D11RenderTargetView", "ID3D11Texture2D", "class TextEditor" };
        for (int i = 0; i < sizeof(identifiers) / sizeof(identifiers[0]); ++i)
        {
            TextEditor::Identifier id;
            id.mDeclaration = std::string(idecls[i]);
            lang.mIdentifiers.insert(std::make_pair(std::string(identifiers[i]), id));
        }
        editor.SetLanguageDefinition(lang);
        //editor.SetPalette(TextEditor::GetLightPalette());

        // error markers
        TextEditor::ErrorMarkers markers;
        markers.insert(std::make_pair<int, std::string>(6, "Example error here:\nInclude file not found: \"TextEditor.h\""));
        markers.insert(std::make_pair<int, std::string>(41, "Another example error"));
        editor.SetErrorMarkers(markers);

#endif


    std::ifstream t(fileToEdit);
    if (t.good()) {
        std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        editor.SetText(str);
    }

    firstRun = true;
}

void ImGuiLayer::onDetach() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}

void ImGuiLayer::begin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();
}

void ImGuiLayer::end() {
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui::Render();
    SDL_GL_MakeCurrent(window, gl_context);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        C_Window *backup_current_window = SDL_GL_GetCurrentWindow();
        C_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }
}

auto myCollapsingHeader = [](const char *name) -> bool {
    ImGuiStyle &style = ImGui::GetStyle();
    ImGui::PushStyleColor(ImGuiCol_Header, style.Colors[ImGuiCol_Button]);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, style.Colors[ImGuiCol_ButtonHovered]);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, style.Colors[ImGuiCol_ButtonActive]);
    bool b = ImGui::CollapsingHeader(name);
    ImGui::PopStyleColor(3);
    return b;
};

auto ImGuiAutoTest() -> void {


    if (myCollapsingHeader("About ImGui::Auto()")) {
        ImGui::Auto(R"comment(
ImGui::Auto() is one simple function that can create GUI for almost any data structure using ImGui functions.
a. Your data is presented in tree-like structure that is defined by your type.
    The generated code is highly efficient.
b. Const types are display only, the user cannot modify them.
    Use ImGui::as_cost() to permit the user to modify a non-const type.
The following types are supported (with nesting):
1 Strings. Flavours of std::string and char*. Use std::string for input.
2 Numbers. Integers, Floating points, ImVec{2,4}
3 STL Containers. Standard containers are supported. (std::vector, std::map,...)
The contained type has to be supported. If it is not, see 8.
4 Pointers, arrays. Pointed type must be supported.
5 std::pair, std::tuple. Types used must be supported.
6 structs and simple classes! The struct is converted to a tuple, and displayed as such.
    * Requirements: C++14 compiler (GCC-5.0+, Clang, Visual Studio 2017 with /std:c++17, ...)
    * How? I'm using this libary https://github.com/apolukhin/magic_get (ImGui::Auto() is only VS2017 C++17 tested.)
7 Functions. A void(void) type function becomes simple button.
For other types, you can input the arguments and calculate the result.
Not yet implemented!
8 You can define ImGui::Auto() for your own type! Use the following macros:
    * IMGUI_AUTO_DEFINE_INLINE(template_spec, type_spec, code)	single line definition, teplates shouldn't have commas inside!
    * IMGUI_AUTO_DEFINE_BEGIN(template_spec, type_spec)        start multiple line definition, no commas in arguments!
    * IMGUI_AUTO_DEFINE_BEGIN_P(template_spec, type_spec)      start multiple line definition, can have commas, use parentheses!
    * IMGUI_AUTO_DEFINE_END                                    end multiple line definition with this.
where
    * template_spec   describes how the type is templated. For fully specialized, use "template<>" only
    * type_spec       is the type for witch you define the ImGui::Auto() function.
	* var             will be the generated function argument of type type_spec.
	* name            is the const std::string& given by the user, and/or generated by the caller ImGui::Auto function
Example:           IMGUI_AUTO_DEFINE_INLINE(template<>, bool, ImGui::Checkbox(name.c_str(), &var);)
Tipps: - You may use ImGui::Auto_t<type>::Auto(var, name) functions directly.
        - Check imgui::detail namespace for other helper functions.

The libary uses partial template specialization, so definitions can overlap, the most specialized will be chosen.
However, using large, nested structs can lead to slow compilation.
Container data, large structs and tuples are hidden by default.
)comment");


        MarkdownData md1;
        md1.data = R"markdown(


# Table

Name &nbsp; &nbsp; &nbsp; &nbsp; | Multiline &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;<br>header  | [Link&nbsp;](#link1)
:------|:-------------------|:--
Value-One | Long <br>explanation <br>with \<br\>\'s|1
~~Value-Two~~ | __text auto wrapped__\: long explanation here |25 37 43 56 78 90
**etc** | [~~Some **link**~~](https://github.com/mekhontsev/ImMarkdown)|3

# List

1. First ordered list item
2. 测试中文
   * Unordered sub-list 1.
   * Unordered sub-list 2.
1. Actual numbers don't matter, just that it's a number
   1. **Ordered** sub-list 1
   2. **Ordered** sub-list 2
4. And another item with minuses.
   - __sub-list with underline__
   - sub-list with escapes: \[looks like\]\(link\)
5. ~~Item with pluses and strikethrough~~.
   + sub-list 1
   + sub-list 2
   + [Just a link](https://github.com/mekhontsev/ImMarkdown).
      * Item with [link1](#link1)
      * Item with bold [**link2**](#link1)
6. test12345




)markdown";

        ImGui::Auto(md1);

        ImGui::Text(U8("中文测试"));

        if (ImGui::TreeNode("TODO-s")) {
            ImGui::Auto(R"todos(
	1	Insert items to (non-const) set, map, list, ect
	2	Call any function or function object.
	3	Deduce function arguments as a tuple. User can edit the arguments, call the function (button), and view return value.
	4	All of the above needs self ImGui::Auto() allocated memmory. Current plan is to prioritize low memory usage
		over user experiance. (Beacuse if you want good UI, you code it, not generate it.)
		Plan A:
			a)	Data is stored in a map. Key can be the presneted object's address.
			b)	Data is allocated when not already present in the map
			c)	std::unique_ptr<void, deleter> is used in the map with deleter function manually added (or equivalent something)
			d)	The first call for ImGui::Auto at every few frames should delete
				(some of, or) all data that was not accesed in the last (few) frames. How?
		This means after closing a TreeNode, and opening it, the temporary values might be deleted and recreated.
)todos");
            ImGui::TreePop();
        }
    }
    if (myCollapsingHeader("1. String")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	ImGui::Auto("Hello Imgui::Auto() !"); //This is how this text is written as well.)code");
        ImGui::Auto("Hello Imgui::Auto() !");//This is how this text is written as well.

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::string str = "Hello ImGui::Auto() for strings!";
	ImGui::Auto(str, "asd");)code");
        static std::string str = "Hello ImGui::Auto() for strings!";
        ImGui::Auto(str, "str");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::string str2 = "ImGui::Auto()\n Automatically uses multiline input for strings!\n:)";
	ImGui::Auto(str2, "str2");)code");
        static std::string str2 = "ImGui::Auto()\n Automatically uses multiline input for strings!\n:)";
        ImGui::Auto(str2, "str2");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static const std::string conststr = "Const types are not to be changed!";
	ImGui::Auto(conststr, "conststr");)code");
        static const std::string conststr = "Const types are not to be changed!";
        ImGui::Auto(conststr, "conststr");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	char * buffer = "To edit a string use std::string. Manual buffers are unwelcome here.";
	ImGui::Auto(buffer, "buffer");)code");
        const char *buffer = "To edit a string use std::string. Manual buffers are unwelcome here.";
        ImGui::Auto(buffer, "buffer");

        ImGui::Unindent();
    }
    if (myCollapsingHeader("2. Numbers")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	static int i = 42;
	ImGui::Auto(i, "i");)code");
        static int i = 42;
        ImGui::Auto(i, "i");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static float f = 3.14;
	ImGui::Auto(f, "f");)code");
        static float f = 3.14;
        ImGui::Auto(f, "f");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static ImVec4 f4 = {1.5f,2.1f,3.4f,4.3f};
	ImGui::Auto(f4, "f4");)code");
        static ImVec4 f4 = {1.5f, 2.1f, 3.4f, 4.3f};
        ImGui::Auto(f4, "f4");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static const ImVec2 f2 = {1.f,2.f};
	ImGui::Auto(f2, "f2");)code");
        static const ImVec2 f2 = {1.f, 2.f};
        ImGui::Auto(f2, "f2");

        ImGui::Unindent();
    }
    if (myCollapsingHeader("3. Containers")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	static std::vector<std::string> vec = { "First string","Second str",":)" };
	ImGui::Auto(vec,"vec");)code");
        static std::vector<std::string> vec = {"First string", "Second str", ":)"};
        ImGui::Auto(vec, "vec");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static const std::vector<float> constvec = { 3,1,2.1f,4,3,4,5 };
	ImGui::Auto(constvec,"constvec");	//Cannot change vector, nor values)code");
        static const std::vector<float> constvec = {3, 1, 2.1f, 4, 3, 4, 5};
        ImGui::Auto(constvec, "constvec");//Cannot change vector, nor values

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::vector<bool> bvec = { false, true, false, false };
	ImGui::Auto(bvec,"bvec");)code");
        static std::vector<bool> bvec = {false, true, false, false};
        ImGui::Auto(bvec, "bvec");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static const std::vector<bool> constbvec = { false, true, false, false };
	ImGui::Auto(constbvec,"constbvec");
	)code");
        static const std::vector<bool> constbvec = {false, true, false, false};
        ImGui::Auto(constbvec, "constbvec");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::map<int, float> map = { {3,2},{1,2} };
	ImGui::Auto(map, "map");	// insert and other operations)code");
        static std::map<int, float> map = {{3, 2}, {1, 2}};
        ImGui::Auto(map, "map");// insert and other operations

        if (ImGui::TreeNode("All cases")) {
            ImGui::Auto(R"code(
	static std::deque<bool> deque = { false, true, false, false };
	ImGui::Auto(deque,"deque");)code");
            static std::deque<bool> deque = {false, true, false, false};
            ImGui::Auto(deque, "deque");

            ImGui::NewLine();
            ImGui::Separator();

            ImGui::Auto(R"code(
	static std::set<char*> set = { "set","with","char*" };
	ImGui::Auto(set,"set");)code");
            static std::set<const char *> set = {"set", "with", "char*"};//for some reason, this does not work
            ImGui::Auto(set, "set");                                     // the problem is with the const iterator, but

            ImGui::NewLine();
            ImGui::Separator();

            ImGui::Auto(R"code(
	static std::map<char*, std::string> map = { {"asd","somevalue"},{"bsd","value"} };
	ImGui::Auto(map, "map");	// insert and other operations)code");
            static std::map<const char *, std::string> map = {{"asd", "somevalue"}, {"bsd", "value"}};
            ImGui::Auto(map, "map");// insert and other operations

            ImGui::TreePop();
        }
        ImGui::Unindent();
    }
    if (myCollapsingHeader("4. Pointers and Arrays")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	static float *pf = nullptr;
	ImGui::Auto(pf, "pf");)code");
        static float *pf = nullptr;
        ImGui::Auto(pf, "pf");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static int i=10, *pi=&i;
	ImGui::Auto(pi, "pi");)code");
        static int i = 10, *pi = &i;
        ImGui::Auto(pi, "pi");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static const std::string cs= "I cannot be changed!", * cps=&cs;
	ImGui::Auto(cps, "cps");)code");
        static const std::string cs = "I cannot be changed!", *cps = &cs;
        ImGui::Auto(cps, "cps");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::string str = "I can be changed! (my pointee cannot)";
	static std::string *const strpc = &str;)code");
        static std::string str = "I can be changed! (my pointee cannot)";
        static std::string *const strpc = &str;
        ImGui::Auto(strpc, "strpc");

        ImGui::NewLine();
        ImGui::Separator();
        ImGui::Auto(R"code(
	static std::array<float,5> farray = { 1.2, 3.4, 5.6, 7.8, 9.0 };
	ImGui::Auto(farray, "std::array");)code");
        static std::array<float, 5> farray = {1.2, 3.4, 5.6, 7.8, 9.0};
        ImGui::Auto(farray, "std::array");


        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static float farr[5] = { 1.2, 3.4, 5.6, 7.8, 9.0 };
	ImGui::Auto(farr, "float[5]");)code");
        static float farr[5] = {11.2, 3.4, 5.6, 7.8, 911.0};
        ImGui::Auto(farr, "float[5]");

        ImGui::Unindent();
    }
    if (myCollapsingHeader("5. Pairs and Tuples")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	static std::pair<bool, ImVec2> pair = { true,{2.1f,3.2f} };
	ImGui::Auto(pair, "pair");)code");
        static std::pair<bool, ImVec2> pair = {true, {2.1f, 3.2f}};
        ImGui::Auto(pair, "pair");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::pair<int, std::string> pair2 = { -3,"simple types appear next to each other in a pair" };
	ImGui::Auto(pair2, "pair2");)code");
        static std::pair<int, std::string> pair2 = {-3, "simple types appear next to each other in a pair"};
        ImGui::Auto(pair2, "pair2");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	ImGui::Auto(ImGui::as_const(pair), "as_const(pair)"); //easy way to view as const)code");
        ImGui::Auto(ImGui::as_const(pair), "as_const(pair)");//easy way to view as const

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	std::tuple<const int, std::string, ImVec2> tuple = { 42, "string in tuple", {3.1f,3.2f} };
	ImGui::Auto(tuple, "tuple");)code");
        std::tuple<const int, std::string, ImVec2> tuple = {42, "string in tuple", {3.1f, 3.2f}};
        ImGui::Auto(tuple, "tuple");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	const std::tuple<int, const char*, ImVec2> consttuple = { 42, "smaller tuples are inlined", {3.1f,3.2f} };
	ImGui::Auto(consttuple, "consttuple");)code");
        const std::tuple<int, const char *, ImVec2> consttuple = {42, "Smaller tuples are inlined", {3.1f, 3.2f}};
        ImGui::Auto(consttuple, "consttuple");


        ImGui::Unindent();
    }
    if (myCollapsingHeader("6. Structs!!")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	struct A //Structs are automagically converted to tuples!
	{
		int i = 216;
		bool b = true;
	};
	static A a;
	ImGui::Auto("a", a);)code");
        struct A
        {
            int i = 216;
            bool b = true;
        };
        static A a;
        ImGui::Auto(a, "a");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	ImGui::Auto(ImGui::as_const(a), "as_const(a)");// const structs are possible)code");
        ImGui::Auto(ImGui::as_const(a), "as_const(a)");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	struct B
	{
		std::string str = "Unfortunatelly, cannot deduce const-ness from within a struct";
		const A a = A();
	};
	static B b;
	ImGui::Auto(b, "b");)code");
        struct B
        {
            std::string str = "Unfortunatelly, cannot deduce const-ness from within a struct";
            const A a = A();
        };
        static B b;
        ImGui::Auto(b, "b");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::vector<B> vec = { {"vector of structs!", A()}, B() };
	ImGui::Auto(vec, "vec");)code");
        static std::vector<B> vec = {{"vector of structs!", A()}, B()};
        ImGui::Auto(vec, "vec");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	struct C
	{
		std::list<B> vec;
		A *a;
	};
	static C c = { {{"Container inside a struct!", A() }}, &a };
	ImGui::Auto(c, "c");)code");
        struct C
        {
            std::list<B> vec;
            A *a;
        };
        static C c = {{{"Container inside a struct!", A()}}, &a};
        ImGui::Auto(c, "c");

        ImGui::Unindent();
    }
    if (myCollapsingHeader("Functions")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	void (*func)() = []() { ImGui::TextColored(ImVec4(0.5, 1, 0.5, 1.0), "Button pressed, function called :)"); };
	ImGui::Auto(func, "void(void) function");)code");
        void (*func)() = []() { ImGui::SameLine(); ImGui::TextColored(ImVec4(0.5, 1, 0.5, 1.0), "Button pressed, function called :)"); };
        ImGui::Auto(func, "void(void) function");

        ImGui::Unindent();
    }
}

void ImGuiLayer::Render(Game *game)

{

#if defined(_METADOT_IMM32)
    imguiIMMCommunication();
#endif


    //ImGui::Begin("Progress Indicators");

    //const ImU32 col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
    //const ImU32 bg = ImGui::GetColorU32(ImGuiCol_Button);

    //ImGui::Spinner("##spinner", 15, 6, col);
    //ImGui::BufferingBar("##buffer_bar", 0.7f, ImVec2(400, 6), bg, col);

    //ImGui::End();


#if 0

        {

            ImGuiWindowFlags option_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
            const auto parent_size = ImGui::GetMainViewport()->WorkSize;
            const auto parent_pos = ImGui::GetMainViewport()->WorkPos;

            static auto file_panel_bottom = 0.0F;
            // file panel
            {
                ImGui::SetNextWindowSize({ layout::kRenderOptionsPanelWidth, 0 });
                ImGui::SetNextWindowPos({ parent_pos.x + layout::kMargin, parent_pos.y + layout::kMargin });
                ImGui::SetNextWindowBgAlpha(0.5F);
                ImGui::Begin("FilePanel", nullptr, option_flags | ImGuiWindowFlags_NoScrollbar);

                ImGuiHelper::AlignedText(std::string(ICON_FA_FILE) + " ", ImGuiHelper::Alignment::kVerticalCenter);
                ImGui::SameLine();

                if (ImGui::Button("Load mesh from file...")) {
                    //make_singleton<common::Switch>().OpenFile();
                }
                file_panel_bottom = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;

                ImGui::End();
            }

            // options panel
            static float options_panel_bottom = 0;
            {
                ImGui::SetNextWindowSize({ layout::kRenderOptionsPanelWidth, 0 });
                ImGui::SetNextWindowPos({ parent_pos.x + layout::kMargin, file_panel_bottom + layout::kPanelSpacing });
                ImGui::SetNextWindowBgAlpha(0.5F);
                ImGui::Begin("RenderOptionsWindow", nullptr, option_flags | ImGuiWindowFlags_NoScrollbar);
                // tabs
                static auto tabs = std::vector<std::string>{ U8("Surface"), U8("Line"), U8("Points"), U8("全局") };
                static auto selected_index = 0;
                ImGuiHelper::ButtonTab(tabs, selected_index);

                // detail options
                {
                    ImGui::Dummy({ 0, 10 });
                    using func = std::function<void()>;
                    //static std::array<func, 4> options{ SurfaceRenderOptions::show, LineRenderOptions::show, PointsRenderOptions::show,
                    //                                   GlobRenderOptions::show };
                    //options.at(selected_index)();
                }
                options_panel_bottom = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;
                ImGui::End();
            }

            // debug panel
            static auto debug_panel_bottom = 0.0F;
            {
                ImGui::SetNextWindowSize({ layout::kRenderOptionsPanelWidth, 0 });
                ImGui::SetNextWindowPos({ parent_pos.x + layout::kMargin, options_panel_bottom + layout::kPanelSpacing });
                ImGui::SetNextWindowBgAlpha(0.5F);
                ImGui::Begin("DebugPanel", nullptr, option_flags | ImGuiWindowFlags_NoScrollbar);

                auto static show_metrics = false;
                ImGuiHelper::SwitchButton(ICON_FA_WRENCH, "Window Metrics", show_metrics);
                ImGuiHelper::ListSeparator();
                if (show_metrics) {
                    ImGui::ShowMetricsWindow();
                }

                auto static show_demo = false;
                ImGuiHelper::SwitchButton(ICON_FA_ROCKET, "Demo", show_demo);
                if (show_demo) {
                    ImGui::ShowDemoWindow();

                    //m_ImGuiLayer->Render();
                }

                debug_panel_bottom = ImGui::GetWindowSize().y;
                ImGui::End();
            }
        }

#endif

    if (Settings::ui_code_editor) {

        auto cpos = editor.GetCursorPosition();
        ImGui::Begin("脚本编辑器", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
        ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save")) {
                    auto textToSave = editor.GetText();
                    /// save text....
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                bool ro = editor.IsReadOnly();
                if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
                    editor.SetReadOnly(ro);
                ImGui::Separator();

                if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor.CanUndo()))
                    editor.Undo();
                if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor.CanRedo()))
                    editor.Redo();

                ImGui::Separator();

                if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
                    editor.Copy();
                if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && editor.HasSelection()))
                    editor.Cut();
                if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && editor.HasSelection()))
                    editor.Delete();
                if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
                    editor.Paste();

                ImGui::Separator();

                if (ImGui::MenuItem("Select all", nullptr, nullptr))
                    editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Dark palette"))
                    editor.SetPalette(TextEditor::GetDarkPalette());
                if (ImGui::MenuItem("Light palette"))
                    editor.SetPalette(TextEditor::GetLightPalette());
                if (ImGui::MenuItem("Retro blue palette"))
                    editor.SetPalette(TextEditor::GetRetroBluePalette());
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(),
                    editor.IsOverwrite() ? "Ovr" : "Ins",
                    editor.CanUndo() ? "*" : " ",
                    editor.GetLanguageDefinition().mName.c_str(), fileToEdit);

        editor.Render("TextEditor");
        ImGui::End();
    }

    if (Settings::ui_tweak) {

        ImGui::Begin("MetaEngine Tweaks");

        ImGui::BeginTabBar(U8("协变与逆变"));

        if (ImGui::BeginTabItem(U8("首页"))) {
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(U8("热更新"))) {

            //game->data.Functions["func_drawInTweak"].invoke({});

            //game->data->draw();

            ImGui::EndTabItem();
        }


        if (ImGui::BeginTabItem(U8("测试"))) {
            ImGui::BeginTabBar(U8("测试#haha"));
            if (ImGui::BeginTabItem(U8("自动序列测试"))) {
                ImGuiAutoTest();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(U8("ImPlot测试"))) {
                static bool ui_implot_test = true;
                ImPlot::ShowDemoWindow(&ui_implot_test);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(U8("调整"))) {
            if (myCollapsingHeader(U8("遥测"))) {
                GameUI::DebugUI::Draw(game);
            }
            // Call the function in our RCC++ class
            //if (myCollapsingHeader("RCCpp"))
            //    getSystemTable()->pRCCppMainLoopI->MainLoop();

            //if (myCollapsingHeader("WorldInfo"))
            //    MetaEngine::App::get().getModuleStack().getInstances("WorldLayer")->onImGuiInnerRender();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();

        ImGui::End();
    }


    GameUI::GameUI_Draw(game);

    if (Settings::ui_inspector) {
        //DrawPropertyWindow();
    }

    if (Settings::ui_gcmanager) {
        ImGui::Begin("GC");
#if defined(METADOT_DEBUG)
        for (auto [name, size]: GC::MemoryDebugMap) {
            ImGui::Text(Utils::Format("{0} {1}", name, size).c_str());
        }
#endif
        ImGui::End();
    }
}

void ImGuiLayer::registerWindow(std::string_view windowName, bool *opened) {
    for (auto &m_win: m_wins)
        if (m_win.name == windowName)
            return;
    m_wins.push_back({std::string(windowName), opened});
}
