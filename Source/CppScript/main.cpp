#include <chrono>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <thread>

#include <SDL.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "Engine/lib/cr.h"

#include "Engine/Utils.hpp"
#include "Shared/Interface.hpp"

#include "Engine/Refl.hpp"

//#define IMGUI_GUEST_ONLY

static uint32_t g_failure = 0;
static HostData *g_data =
        nullptr;// hold user data kept on host and received from host

// Some saved state between reloads
static unsigned int CR_STATE g_version = 0;
#if defined(IMGUI_GUEST_ONLY)
static ImGuiContext CR_STATE *g_imgui_context = nullptr;
static ImFontAtlas CR_STATE *g_default_font_atlas = nullptr;
#endif// #if defined(IMGUI_GUEST_ONLY)

void func_drawInTweak() {
    ImGui::Text("Hello, world!");

    ImGui::Text("热更新版本: %d", g_version);
    if (g_failure)
        ImGui::Text("热更新失败原因: %d", g_failure);
}

bool imui_init() {

    IMGUI_CHECKVERSION();

#if defined(IMGUI_GUEST_ONLY)
    if (!g_imgui_context) {
        g_imgui_context = new ImGuiContext;
        g_default_font_atlas = new ImFontAtlas;
    }
    ImGui::SetCurrentContext(g_imgui_context);
    ImGui::GetIO().Fonts = g_default_font_atlas;
#else
    ImGui::SetCurrentContext(g_data->imgui_context);
#endif

    std::string a = "guckguck!";

    g_data->Functions["func_log_info"].invoke({&a});

    // MetaEngine::any_function func{ &func_drawInTweak };
    // g_data->Functions.insert(std::make_pair("func_drawInTweak", func));
    g_data->draw = func_drawInTweak;

    return true;
}

void imui_shutdown() {}

void test_crash() {
    ImGui::EndFrame();
    int *addr = NULL;
    (void) addr;// warning
    int i = *addr;
    (void) i;
}

void imui_draw() {

    // test_crash(); // uncomment to test crash protection
}

CR_EXPORT int cr_main(cr_plugin *ctx, cr_op operation) {
    assert(ctx);
    g_data = (HostData *) ctx->userdata;
    g_version = ctx->version;
    g_failure = ctx->failure;

    if (operation != CR_STEP) {

        std::string loadmsg = MetaEngine::Utils::Format(
                "InternalCppScript: {0}(version: {1})",
                operation == CR_LOAD ? "load" : "unload", ctx->version);
        g_data->Functions["func_log_info"].invoke({&loadmsg});

        int *addr = NULL;
        (void) addr;// warning
                    // to test crash protection during load
                    // int i = *addr;
    }
    // crash protection may cause the version to decrement. So we can test current
    // version against one tracked between instances with CR_STATE to signal that
    // we're not running the most recent instance.
    if (ctx->version < g_version) {
        // a failure code is acessible in the `failure` variable from the
        // `cr_plugin` context. on windows this is the structured exception error
        // code, for more info:
        //      https://msdn.microsoft.com/en-us/library/windows/desktop/ms679356(v=vs.85).aspx
        fprintf(stdout, "A rollback happened due to failure: %x!\n", ctx->failure);
    }
    g_version = ctx->version;

    // Not this does not carry state between instances (no CR_STATE), this means
    // each time we load an instance this value will be reset to its initial state
    // (true), and then we can print the loaded instance version one time only by
    // instance version.
    static bool print_version = true;
    if (print_version) {
        // fprintf(stdout, "loaded version: %d\n", ctx->version);

        // auto str = std::string{"gaga"};
        // g_data->Functions.func_log_info->invoke({&str});

        // disable further printing for this instance only
        print_version = false;
    }

    switch (operation) {
        case CR_LOAD:
            imui_init();
            return 0;
        case CR_UNLOAD:
            // if needed, save stuff to pass over to next instance
            return 0;
        case CR_CLOSE:
            imui_shutdown();
            return 0;
        case CR_STEP:
            imui_draw();
            return 0;
    }

    return 0;
}
