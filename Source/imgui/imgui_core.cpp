// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "imgui_core.hpp"

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <iterator>
#include <map>
#include <type_traits>

#include "chunk.hpp"
#include "code_reflection.hpp"
#include "core/alloc.h"
#include "core/const.h"
#include "core/core.hpp"
#include "core/cpp/static_relfection.hpp"
#include "core/cpp/utils.hpp"
#include "core/dbgtools.h"
#include "core/global.hpp"
#include "core/macros.h"
#include "core/profiler/profiler.h"
#include "core/profiler/profiler_imgui.hpp"
#include "engine.h"
#include "engine_scripting.hpp"
#include "filesystem.h"
#include "game.hpp"
#include "game_datastruct.hpp"
#include "game_ui.hpp"
#include "imgui/imgui_css.h"
#include "imgui/imgui_generated.h"
#include "imgui/imgui_impl.hpp"
#include "imgui/lua/script.h"
#include "libs/imgui/imgui.h"
#include "libs/imgui/implot.h"
#include "memory.hpp"
#include "reflectionflat.hpp"
#include "renderer/gpu.hpp"
#include "renderer/metadot_gl.h"
#include "renderer/renderer_gpu.h"
#include "scripting/lua_wrapper.hpp"

IMPLENGINE();

#define LANG(_c) global.I18N.Get(_c).c_str()

extern void ShowAutoTestWindow();

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
                // OutputDebugStringW(L"initCommonControlsEx Enable\n");
                return 0;
            }
        }
        {
            InitCommonControls();
            // OutputDebugStringW(L"initCommonControls Enable\n");
            return 0;
        }
    }
    return 1;
}

#endif

namespace layout {
constexpr auto kRenderOptionsPanelWidth = 300;
constexpr auto kMargin = 5;
constexpr auto kPanelSpacing = 10;
constexpr auto kColorWidgetWidth = 250.0F;
}  // namespace layout

static bool s_is_animaiting = false;
static const int view_size = 256;

ImVec2 ImGuiCore::GetNextWindowsPos(ImGuiWindowTags tag, ImVec2 pos) {
    if (tag & UI_MainMenu) ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImVec2 windowspos = ImGui::GetPlatformIO().Platform_GetWindowPos(ImGui::GetMainViewport());
        pos += windowspos;
    }
    return pos;
}

ImGuiCore::ImGuiCore() {}

class OpenGL3TextureManager : public ImGuiCSS::TextureManager {
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
        return (ImTextureID)(intptr_t)texture_id;
    }

    void deleteTexture(ImTextureID id) {
        GLuint tex = (GLuint)(intptr_t)id;
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

static void *ImGuiMalloc(size_t sz, void *user_data) { return gc_malloc(&gc, sz); }
static void ImGuiFree(void *ptr, void *user_data) { gc_free(&gc, ptr); }

void ImGuiCore::Init() {

    IMGUI_CHECKVERSION();

    ImGui::SetAllocatorFunctions(ImGuiMalloc, ImGuiFree);

    m_imgui = ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO &io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    io.IniFilename = "imgui.ini";

    ImFontConfig config;
    config.OversampleH = 1;
    config.OversampleV = 1;
    config.PixelSnapH = 1;

    F32 scale = 1.0f;

    io.Fonts->AddFontFromFileTTF(METADOT_RESLOC("data/assets/fonts/fusion-pixel.ttf"), 12.0f, &config, io.Fonts->GetGlyphRangesChineseFull());

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplSDL2_Init(Core.window, Core.glContext);

    ImGui_ImplOpenGL3_Init();

    style.ScaleAllSizes(scale);

    ImGuiInitStyle(0.5f, 0.5f);

    // LUA state
    ImGuiCSS::registerBindings(global.scripts->LuaCoreCpp->C->L);
    ctx = ImGuiCSS::createContext(ImGuiCSS::createElementFactory(), new ImGuiCSS::LuaScriptState(global.scripts->LuaCoreCpp->C->L), new OpenGL3TextureManager());
    ctx->scale = ImVec2(scale, scale);

    document = new ImGuiCSS::Document(ctx);
    const char *page = "data/assets/ui/imguicss/simple.xml";
    char *data = ctx->fs->load(page);
    document->parse(data);
    ImGui::MemFree(data);

#if defined(_METADOT_IMM32)
    common_control_initialize();
    VERIFY(imguiIMMCommunication.subclassify(window));
#endif

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

    fileDialog.SetPwd(METADOT_RESLOC("data/scripts"));
    fileDialog.SetTitle("选择文件");
    fileDialog.SetTypeFilters({".lua"});

    firstRun = true;
}

void ImGuiCore::End() {

    delete document;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}

void ImGuiCore::NewFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(Core.window);
    ImGui::NewFrame();
}

void ImGuiCore::Draw() {
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    ImGui::Render();
    SDL_GL_MakeCurrent(Core.window, Core.glContext);
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

auto CollapsingHeader = [](const char *name) -> bool {
    ImGuiStyle &style = ImGui::GetStyle();
    ImGui::PushStyleColor(ImGuiCol_Header, style.Colors[ImGuiCol_Button]);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, style.Colors[ImGuiCol_ButtonHovered]);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, style.Colors[ImGuiCol_ButtonActive]);
    bool b = ImGui::CollapsingHeader(name);
    ImGui::PopStyleColor(3);
    return b;
};

void ImGuiCore::Update() {

#if defined(_METADOT_IMM32)
    imguiIMMCommunication();
#endif

    // ImGui::Begin("Progress Indicators");

    // const ImU32 col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
    // const ImU32 bg = ImGui::GetColorU32(ImGuiCol_Button);

    // ImGui::Spinner("##spinner", 15, 6, col);
    // ImGui::BufferingBar("##buffer_bar", 0.7f, ImVec2(400, 6), bg, col);

    // ImGui::End();

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

                ImGuiHelper::AlignedText(std::string("ICON_FA_FILE") + " ", ImGuiHelper::Alignment::kVerticalCenter);
                ImGui::SameLine();

                if (ImGui::Button("Load mesh from file...")) {
                    //make_singleton<common::Switch>().OpenFile();
                }
                file_panel_bottom = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;

                ImGui::End();
            }

            // options panel
            static F32 options_panel_bottom = 0;
            {
                ImGui::SetNextWindowSize({ layout::kRenderOptionsPanelWidth, 0 });
                ImGui::SetNextWindowPos({ parent_pos.x + layout::kMargin, file_panel_bottom + layout::kPanelSpacing });
                ImGui::SetNextWindowBgAlpha(0.5F);
                ImGui::Begin("RenderOptionsWindow", nullptr, option_flags | ImGuiWindowFlags_NoScrollbar);
                // tabs
                static auto tabs = std::vector<std::string>{ CC("Surface"), CC("Line"), CC("Points"), CC("全局") };
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
                ImGuiHelper::SwitchButton("ICON_FA_WRENCH", "Window Metrics", show_metrics);
                ImGuiHelper::ListSeparator();
                if (show_metrics) {
                    ImGui::ShowMetricsWindow();
                }

                auto static show_demo = false;
                ImGuiHelper::SwitchButton("ICON_FA_ROCKET", "Demo", show_demo);
                if (show_demo) {
                    ImGui::ShowDemoWindow();

                    //m_ImGuiCore->Render();
                }

                debug_panel_bottom = ImGui::GetWindowSize().y;
                ImGui::End();
            }
        }

#endif

    document->render();

    MarkdownData md1;
    md1.data = R"markdown(

# Table

Name &nbsp; &nbsp; &nbsp; &nbsp; | Multiline &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;<br>header  | [Link&nbsp;](#link1)
:------|:-------------------|:--
Value-One | Long <br>explanation <br>with \<br\>\'s|1
~~Value-Two~~ | __text auto wrapped__\: long explanation here |25 37 43 56 78 90
**etc** | [~~Some **link**~~](https://github.com/mekhontsev/ImGuiMarkdown)|3

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
   + [Just a link](https://github.com/mekhontsev/ImGuiMarkdown).
      * Item with [link1](#link1)
      * Item with bold [**link2**](#link1)
6. test12345

)markdown";

    if (global.game->GameIsolate_.globaldef.draw_imgui_debug) {
        ImGui::ShowDemoWindow();
        ImPlot::ShowDemoWindow();
    }

    if (global.game->GameIsolate_.globaldef.draw_profiler) {
        static ProfilerFrame data;
        ProfilerGetFrame(&data);

        // // if (g_multi) ProfilerDrawFrameNavigation(g_frameInfos.data(), g_frameInfos.size());

        static char buffer[10 * 1024];
        ProfilerDrawFrame(&data, buffer, 10 * 1024);
        // // ProfilerDrawStats(&data);
    }

    auto cpos = editor.GetCursorPosition();
    if (global.game->GameIsolate_.globaldef.ui_tweak) {

        ImGui::Begin(LANG("ui_tweaks"), NULL, ImGuiWindowFlags_MenuBar);

        ImGui::BeginTabBar("ui_tweaks_tabbar");

        if (ImGui::BeginTabItem(LANG("ui_console"))) {
            global.game->GameIsolate_.console->DrawUI();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(LANG("ui_info"))) {
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            R_Renderer *renderer = R_GetCurrentRenderer();
            R_RendererID id = renderer->id;

            ImGui::Text("Using renderer: %s", glGetString(GL_RENDERER));
            ImGui::Text("OpenGL version supported: %s", glGetString(GL_VERSION));
            ImGui::Text("Engine renderer: %s (%d.%d)\n", id.name, id.major_version, id.minor_version);
            ImGui::Text("Shader versions supported: %d to %d\n\n", renderer->min_shader_version, renderer->max_shader_version);

            ImGui::Separator();

            MarkdownData TickInfoPanel;
            TickInfoPanel.data = R"(
Info &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; | Data &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; | Comments &nbsp;
:-----------|:------------|:--------
TickTime | {0} | Nothing
DeltaTime | {1} | Nothing
TPS | {2} | Nothing
Mspt | {3} | Nothing
Delta | {4} | Nothing
STDTime | {5} | Nothing
CSTDTime | {6} | Nothing
            )";

            time_t rawtime;
            rawtime = time(NULL);
            struct tm *timeinfo = localtime(&rawtime);

            TickInfoPanel.data = MetaEngine::Format(TickInfoPanel.data, Time.tickCount, Time.deltaTime, Time.tps, Time.mspt, Time.now - Time.lastTick, Time::millis(), rawtime);

            ImGui::Auto(TickInfoPanel);

            ImGui::Text("\nnow: %d-%02d-%02d %02d:%02d:%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

            ImGui::Dummy(ImVec2(0.0f, 20.0f));

            const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

            if (ImGui::BeginTable("table_nested1", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable)) {
                ImGui::TableSetupColumn("A0");
                ImGui::TableSetupColumn("A1");
                ImGui::TableHeadersRow();

                ImGui::TableNextColumn();
                ImGui::Text("A0 Row 0");
                {
                    float rows_height = TEXT_BASE_HEIGHT * 2;
                    if (ImGui::BeginTable("table_nested2", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable)) {
                        ImGui::TableSetupColumn("B0");
                        ImGui::TableSetupColumn("B1");
                        ImGui::TableHeadersRow();

                        ImGui::TableNextRow(ImGuiTableRowFlags_None, rows_height);
                        ImGui::TableNextColumn();
                        ImGui::Text("B0 Row 0");
                        ImGui::TableNextColumn();
                        ImGui::Text("B1 Row 0");
                        ImGui::TableNextRow(ImGuiTableRowFlags_None, rows_height);
                        ImGui::TableNextColumn();
                        ImGui::Text("B0 Row 1");
                        ImGui::TableNextColumn();
                        ImGui::Text("B1 Row 1");

                        ImGui::EndTable();
                    }
                }
                ImGui::TableNextColumn();
                ImGui::Text("A1 Row 0");
                ImGui::TableNextColumn();
                ImGui::Text("A0 Row 1");
                ImGui::TableNextColumn();
                ImGui::Text("A1 Row 1");
                ImGui::EndTable();
            }

            ImGui::Separator();

#if defined(METADOT_DEBUG)
            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImGui::Text("GC:\n");
            for (auto [name, size] : GC::MemoryDebugMap) {
                ImGui::Text("%s", MetaEngine::Format("   {0} {1}", name, size).c_str());
            }
            // ImGui::Auto(GC::MemoryDebugMap, "map");
#endif
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(LANG("ui_test"))) {
            ImGui::BeginTabBar(CC("测试#haha"));
            if (ImGui::BeginTabItem(CC("测试"))) {
                if (ImGui::Button("调用回溯")) print_callstack();
                ImGui::SameLine();
                if (ImGui::Button("DBG")) METADOT_DBG(global.game->TexturePack_.pixels);
                ImGui::SameLine();
                if (ImGui::Button("StaticRefl")) {

                    if (global.game->GameIsolate_.world == nullptr || !global.game->GameIsolate_.world->isPlayerInWorld()) {
                    } else {
                        using namespace MetaEngine::StaticRefl;

                        TypeInfo<Player>::DFS_ForEach([](auto t, std::size_t depth) {
                            for (std::size_t i = 0; i < depth; i++) std::cout << "  ";
                            std::cout << t.name << std::endl;
                        });

                        std::cout << "[non DFS]" << std::endl;
                        TypeInfo<Player>::fields.ForEach([&](auto field) {
                            std::cout << field.name << std::endl;
                            if (field.name == "holdtype" && TypeInfo<EnumPlayerHoldType>::fields.NameOfValue(field.value) == "hammer") {
                                std::cout << "   Passed" << std::endl;
                                // if constexpr (std::is_same<decltype(field.value), EnumPlayerHoldType>().value) {
                                //     std::cout << "   " << field.value << std::endl;
                                // }
                            }
                        });

                        std::cout << "[DFS]" << std::endl;
                        TypeInfo<Player>::DFS_ForEach([](auto t, std::size_t) { t.fields.ForEach([](auto field) { std::cout << field.name << std::endl; }); });

                        constexpr std::size_t fieldNum = TypeInfo<Player>::DFS_Acc(0, [](auto acc, auto, auto) { return acc + 1; });
                        std::cout << "field number : " << fieldNum << std::endl;

                        std::cout << "[var]" << std::endl;

                        // std::cout << "[left]" << std::endl;
                        // TypeInfo<Player>::ForEachVarOf(std::move(*global.game->GameIsolate_.world->WorldIsolate_.player), [](auto field, auto &&var) {
                        //     static_assert(std::is_rvalue_reference_v<decltype(var)>);
                        //     std::cout << field.name << " : " << var << std::endl;
                        // });
                        // std::cout << "[right]" << std::endl;
                        // TypeInfo<Player>::ForEachVarOf(*global.game->GameIsolate_.world->WorldIsolate_.player, [](auto field, auto &&var) {
                        //     static_assert(std::is_lvalue_reference_v<decltype(var)>);
                        //     std::cout << field.name << " : " << var << std::endl;
                        // });

                        auto &p = *global.game->GameIsolate_.world->WorldIsolate_.player;

                        // Just for test
                        Item *i3 = new Item();
                        i3->setFlag(ItemFlags::HAMMER);
                        i3->surface = LoadTexture("data/assets/objects/testHammer.png")->surface;
                        i3->texture = R_CopyImageFromSurface(i3->surface);
                        R_SetImageFilter(i3->texture, R_FILTER_NEAREST);
                        i3->pivotX = 2;

                        TypeInfo<Player>::fields.ForEach([&](auto field) {
                            if constexpr (field.is_func) {
                                if (field.name != "setItemInHand") return;
                                if constexpr (field.ValueTypeIsSameWith(static_cast<void (Player::*)(Item * item, World * world) /* const */>(&Player::setItemInHand)))
                                    (p.*(field.value))(i3, global.game->GameIsolate_.world);
                                // else if constexpr (field.ValueTypeIsSameWith(static_cast<void (Player::*)(Item *item, World *world) /* const */>(&Player::setItemInHand)))
                                //     std::cout << (p.*(field.value))(1.f) << std::endl;
                                else
                                    assert(false);
                            }
                        });

                        // virtual

                        std::cout << "[Virtual Bases]" << std::endl;
                        constexpr auto vbs = TypeInfo<Player>::VirtualBases();
                        vbs.ForEach([](auto info) { std::cout << info.name << std::endl; });

                        std::cout << "[Tree]" << std::endl;
                        TypeInfo<Player>::DFS_ForEach([](auto t, std::size_t depth) {
                            for (std::size_t i = 0; i < depth; i++) std::cout << "  ";
                            std::cout << t.name << std::endl;
                        });

                        std::cout << "[field]" << std::endl;
                        TypeInfo<Player>::DFS_ForEach([](auto t, std::size_t) { t.fields.ForEach([](auto field) { std::cout << field.name << std::endl; }); });

                        std::cout << "[var]" << std::endl;

                        std::cout << "[var : left]" << std::endl;
                        TypeInfo<Player>::ForEachVarOf(std::move(p), [](auto field, auto &&var) {
                            static_assert(std::is_rvalue_reference_v<decltype(var)>);
                            std::cout << var << std::endl;
                        });
                        std::cout << "[var : right]" << std::endl;
                        TypeInfo<Player>::ForEachVarOf(p, [](auto field, auto &&var) {
                            static_assert(std::is_lvalue_reference_v<decltype(var)>);
                            std::cout << field.name << " : " << var << std::endl;
                        });
                    }
                }
                ImGui::Checkbox("Profiler", &global.game->GameIsolate_.globaldef.draw_profiler);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(CC("自动序列测试"))) {
                ShowAutoTestWindow();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(LANG("ui_debug"))) {
            if (CollapsingHeader(LANG("ui_chunk"))) {
                static bool check_rigidbody = false;
                ImGui::Checkbox(CC("只查看刚体有效"), &check_rigidbody);

                for (auto &p1 : global.game->GameIsolate_.world->WorldIsolate_.chunkCache)
                    for (auto &p2 : p1.second)
                        if (ImGui::TreeNode(p2.second->pack_filename.c_str())) {
                            MetaEngine::StaticRefl::TypeInfo<Chunk>::ForEachVarOf(*p2.second, [&](const auto &field, auto &&var) {
                                if (field.name == "pack_filename") return;

                                if (check_rigidbody)
                                    if (field.name == "rb" && &var) {
                                        return;
                                    }
                                // constexpr auto tstr_range = TSTR("Meta::Msg");

                                // if constexpr (decltype(field.attrs)::Contains(tstr_range)) {
                                //     auto r = attr_init(tstr_range, field.attrs.Find(tstr_range).value);
                                //     // cout << "[" << tstr_range.View() << "] " << r.minV << ", " << r.maxV << endl;
                                // }
                                ImGui::Auto(var, std::string(field.name));
                            });
                            ImGui::TreePop();
                        }
            }
            if (CollapsingHeader(LANG("ui_telemetry"))) {
                GameUI::DrawDebugUI(global.game);
            }
#define INSPECTSHADER(_c) MetaEngine::IntrospectShader(#_c, global.game->GameIsolate_.shaderworker->_c->shader)
            if (CollapsingHeader(CC("GLSL"))) {
                INSPECTSHADER(newLightingShader);
                INSPECTSHADER(fireShader);
                INSPECTSHADER(fire2Shader);
                INSPECTSHADER(waterShader);
                INSPECTSHADER(waterFlowPassShader);
                INSPECTSHADER(untexturedShader);
                INSPECTSHADER(blurShader);
            }
#undef INSPECTSHADER
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(LANG("ui_scripts_editor"))) {
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu(LANG("ui_file"))) {
                    if (ImGui::MenuItem(LANG("ui_open"))) {
                        fileDialog.Open();
                    }
                    if (ImGui::MenuItem(LANG("ui_save"))) {
                        if (view_editing && view_contents.size()) {
                            auto textToSave = editor.GetText();
                            std::ofstream o(view_editing->file);
                            o << textToSave;
                        }
                    }
                    if (ImGui::MenuItem(LANG("ui_close"))) {
                        for (auto &code : view_contents) {
                            if (code.file == view_editing->file) {
                                view_contents.erase(std::remove(std::begin(view_contents), std::end(view_contents), code), std::end(view_contents));
                            }
                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu(LANG("ui_edit"))) {
                    bool ro = editor.IsReadOnly();
                    if (ImGui::MenuItem(LANG("ui_readonly_mode"), nullptr, &ro)) editor.SetReadOnly(ro);
                    ImGui::Separator();

                    if (ImGui::MenuItem(LANG("ui_undo"), "ALT-Backspace", nullptr, !ro && editor.CanUndo())) editor.Undo();
                    if (ImGui::MenuItem(LANG("ui_redo"), "Ctrl-Y", nullptr, !ro && editor.CanRedo())) editor.Redo();

                    ImGui::Separator();

                    if (ImGui::MenuItem(LANG("ui_copy"), "Ctrl-C", nullptr, editor.HasSelection())) editor.Copy();
                    if (ImGui::MenuItem(LANG("ui_cut"), "Ctrl-X", nullptr, !ro && editor.HasSelection())) editor.Cut();
                    if (ImGui::MenuItem(LANG("ui_delete"), "Del", nullptr, !ro && editor.HasSelection())) editor.Delete();
                    if (ImGui::MenuItem(LANG("ui_paste"), "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr)) editor.Paste();

                    ImGui::Separator();

                    if (ImGui::MenuItem(LANG("ui_selectall"), nullptr, nullptr)) editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu(LANG("ui_view"))) {
                    if (ImGui::MenuItem("Dark palette")) editor.SetPalette(TextEditor::GetDarkPalette());
                    if (ImGui::MenuItem("Light palette")) editor.SetPalette(TextEditor::GetLightPalette());
                    if (ImGui::MenuItem("Retro blue palette")) editor.SetPalette(TextEditor::GetRetroBluePalette());
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            fileDialog.Display();

            if (fileDialog.HasSelected()) {
                bool shouldopen = true;
                auto fileopen = fileDialog.GetSelected().string();
                for (auto code : view_contents)
                    if (code.file == fileopen) shouldopen = false;
                if (shouldopen) {
                    std::ifstream i(fileopen);
                    if (i.good()) {
                        std::string str((std::istreambuf_iterator<char>(i)), std::istreambuf_iterator<char>());
                        view_contents.push_back(EditorView{.tags = EditorTags::Editor_Code, .file = fileopen, .content = str});
                    }
                }
                fileDialog.ClearSelected();
            }

            ImGui::BeginTabBar("ViewContents");

            for (auto &view : view_contents) {
                if (ImGui::BeginTabItem(FUtil_GetFileName(view.file.c_str()))) {
                    view_editing = &view;

                    if (!view_editing->is_edited) {
                        if (view.tags == EditorTags::Editor_Code) editor.SetText(view_editing->content);
                        view_editing->is_edited = true;
                    }

                    ImGui::EndTabItem();
                } else {
                    if (view.is_edited) {
                        view.is_edited = false;
                    }
                }
            }

            if (view_editing && view_contents.size()) {
                switch (view_editing->tags) {
                    case Editor_Code:
                        ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(), editor.IsOverwrite() ? "Ovr" : "Ins",
                                    editor.CanUndo() ? "*" : " ", editor.GetLanguageDefinition().mName.c_str(), FUtil_GetFileName(view_editing->file.c_str()));

                        editor.Render("TextEditor");
                        break;
                    case Editor_Markdown:
                        break;
                    default:
                        break;
                }
            }
            ImGui::EndTabBar();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        ImGui::End();
    }
}