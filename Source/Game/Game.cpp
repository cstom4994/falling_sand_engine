// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Game/Game.hpp"

Game::Game() {}
Game::~Game() {}

#include "Engine/Render/MRender.hpp"
#include "Libs/libxlsxwriter/xlsxwriter.h"
#include "Libs/structopt.hpp"

#include <cmath>
#include <cstdlib>

const char *logo = R"(
      __  __      _        _____        _   
     |  \/  |    | |      |  __ \      | |  
     | \  / | ___| |_ __ _| |  | | ___ | |_ 
     | |\/| |/ _ \ __/ _` | |  | |/ _ \| __|
     | |  | |  __/ || (_| | |__| | (_) | |_ 
     |_|  |_|\___|\__\__,_|_____/ \___/ \__|
                                             
)";


struct Options
{
    std::optional<std::string> test;
    std::vector<std::string> files;
};
STRUCTOPT(Options, test, files);

#include "Scripting/MuScript/Library/MuScript.hpp"

MuScript::MuScriptInterpreter interp(MuScript::ModulePrivilege::allPrivilege);

MuScript::Int integrationExample(MuScript::Int a, MuScript::Int b) {
    return (a * b) + b;
}

void integrationExample() {


#define VAR_HERE() int, float, std::string
    using Type_Tuple = std::tuple<VAR_HERE()>;

    Type_Tuple tpt;
    std::apply([&](auto &&...args) {
        (printf("this type %s\n", typeid(args).name()), ...);
    },
               tpt);


    // Demo c++ integration
    // Step 1: Create a function wrapper
    auto newfunc = interp.newFunction("integrationExample", [](const MuScript::List &args) {
        // MuScript doesn't enforce argument counts, so make sure you have enough
        if (args.size() < 2) {
            return std::make_shared<MuScript::Value>();
        }
        // Dereference arguments
        auto a = *args[0];
        auto b = *args[1];
        // Coerce types
        a.hardconvert(MuScript::Type::Int);
        b.hardconvert(MuScript::Type::Int);
        // Call c++ code
        auto result = integrationExample(a.getInt(), b.getInt());
        // Wrap and return
        return std::make_shared<MuScript::Value>(result);
    });

    // Step 2: Call into MuScript
    // send command into script interperereter
    interp.readLine("i = integrationExample(4, 3);");

    // get a value from the interpereter
    auto varRef = interp.resolveVariable("i");

    // or just call a function directly
    varRef = interp.callFunctionWithArgs(newfunc, MuScript::Int(4), MuScript::Int(3));

    // Setp 3: Unwrap your result
    // if the type is known
    int64_t i = varRef->getInt();
    std::cout << i << "\n";

    // visit style
    std::visit([](auto &&arg) { std::cout << arg << "\n"; }, varRef->value);

    // switch style
    switch (varRef->getType()) {
        case MuScript::Type::Int:
            std::cout << varRef->getInt() << "\n";
            break;
        case MuScript::Type::Float:
            std::cout << varRef->getFloat() << "\n";
            break;
        case MuScript::Type::String:
            std::cout << varRef->getString() << "\n";
            break;
        default:
            break;
    }

    // create a MuScript class from C++:
    interp.newClass("beansClass", {{"color", std::make_shared<MuScript::Value>("white")}},
                    // constructor is required
                    [](MuScript::Class *classs, MuScript::ScopeRef scope, const MuScript::List &vars) {
                        if (vars.size() > 0) {
                            interp.resolveVariable("color", classs, scope) = vars[0];
                        }
                        return std::make_shared<MuScript::Value>();
                    },
                    // add as many functions as you want
                    {
                            {"changeColor", [](MuScript::Class *classs, MuScript::ScopeRef scope, const MuScript::List &vars) {
                                 if (vars.size() > 0) {
                                     interp.resolveVariable("color", classs, scope) = vars[0];
                                 }
                                 return std::make_shared<MuScript::Value>();
                             }},
                            {"isRipe", [](MuScript::Class *classs, MuScript::ScopeRef scope, const MuScript::List &) {
                                 auto color = interp.resolveVariable("color", classs, scope);
                                 if (color->getType() == MuScript::Type::String) { return std::make_shared<MuScript::Value>(color->getString() == "brown"); }
                                 return std::make_shared<MuScript::Value>(false);
                             }},
                    });

    // use the class
    interp.readLine("bean = beansClass(\"grey\");");
    interp.readLine("ripe = bean.isRipe();");

    // get values from the interpereter
    auto beanRef = interp.resolveVariable("bean");
    auto ripeRef = interp.resolveVariable("ripe");

    // read the values!
    if (beanRef->getType() == MuScript::Type::Class && ripeRef->getType() == MuScript::Type::Int) {
        auto colorRef = beanRef->getClass()->variables["color"];
        if (colorRef->getType() == MuScript::Type::String) {
            std::cout << "My bean is " << beanRef->getClass()->variables["color"]->getString() << " and it is " << (ripeRef->getBool() ? "ripe" : "unripe") << "\n";
        }
    }
}

// Called on every frame of the application.
static void frame(void) {
    // Get current window size.
    double frame_time = sapp_frame_duration() * 1000.0;
    int width = sapp_width(), height = sapp_height();
    float center_width = width / 2;
    float center_height = height / 2;

    // Begin recording draw commands for a frame buffer of size (width, height).
    sgp_begin(width, height);
    // Set frame buffer drawing region to (0,0,width,height).
    sgp_viewport(0, 0, width, height);
    // Set drawing coordinate space to (left=-ratio, right=ratio, top=1, bottom=-1).

    // Clear the frame buffer.
    sgp_set_color(0.1f, 0.1f, 0.1f, 1.0f);
    sgp_clear();

    // Draw an animated rectangle that rotates and changes its colors.
    float time = sapp_frame_count() * sapp_frame_duration();
    float r = sinf(time) * 0.5 + 0.5, g = cosf(time) * 0.5 + 0.5;

    sgp_set_color(r, g, 0.3f, 1.0f);
    sgp_push_transform();
    sgp_rotate_at(time, center_width, center_height);
    sgp_draw_filled_rect(center_width - 100.0f, center_height - 100.0f, 200.0f, 200.0f);
    sgp_pop_transform();

    // Begin a render pass.
    sg_pass_action pass_action = {0};
    sg_begin_default_pass(&pass_action, width, height);
    // Dispatch all draw commands to Sokol GFX.
    sgp_flush();
    // Finish a draw command queue, clearing it.
    sgp_end();

    simgui_frame_desc_t frame_t = {
            .width = sapp_width(),
            .height = sapp_height(),
            .delta_time = sapp_frame_duration(),
            .dpi_scale = sapp_dpi_scale(),
    };

    simgui_new_frame(&frame_t);

    /*=== UI CODE STARTS HERE ===*/
    igSetNextWindowPos((ImVec2){10, 10}, ImGuiCond_Once, (ImVec2){0, 0});
    igSetNextWindowSize((ImVec2){400, 100}, ImGuiCond_Once);
    igBegin("Hello Dear ImGui!", 0, ImGuiWindowFlags_None);
    igText("Frame time: %.3f ms\n", frame_time);
    igEnd();
    /*=== UI CODE ENDS HERE ===*/

    simgui_render();
    // End render pass.
    sg_end_pass();
    // Commit Sokol render.
    sg_commit();
}

void event(const sapp_event *e) {
    simgui_handle_event(e);
}

// Called when the application is initializing.
static void init(void) {


    // Initialize Sokol GFX.
    sg_desc sgdesc = {.context = sapp_sgcontext()};
    sg_setup(&sgdesc);
    if (!sg_isvalid()) {
        fprintf(stderr, "Failed to create Sokol GFX context!\n");
        exit(-1);
    }

    // Initialize Sokol GP, adjust the size of command buffers for your own use.
    sgp_desc sgpdesc = {0};
    sgp_setup(&sgpdesc);
    if (!sgp_is_valid()) {
        fprintf(stderr, "Failed to create Sokol GP context: %s\n", sgp_get_error_message(sgp_get_last_error()));
        exit(-1);
    }

    simgui_desc_t imgui_t = {
            .sample_count = 4,
    };

    simgui_setup(&imgui_t);
}

// Called when the application is shutting down.
static void cleanup(void) {
    // Cleanup Sokol GP, Sokol Imgui and Sokol GFX resources.
    sgp_shutdown();
    simgui_shutdown();
    sg_shutdown();
}

sapp_desc sapp_metadot_main = {
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .window_title = "MetaDot Sokol renderer test",
        .sample_count = 4,// Enable anti aliasing.
};

sapp_desc sapp_metadot_void = {

};

// Implement application main through Sokol APP.
sapp_desc sokol_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    try {
        auto options = structopt::app("my_app").parse<Options>(argc, argv);

        if (!options.test.value_or("").empty()) {
            if (options.test == "test_mu") {
                for (auto t: options.files) {
                    //METADOT_UNIT(!interp.evaluateFile(std::string(t)));
                }

                std::string test = R"(

class test {
	var x = 1;
	var y = "MuScript 宽字符测试";
	func test() {
		x = 2;
	}
}

a = test();

if (a.x == 2) {
	print(a.y);
}

b = inspect(a.x);

print(b);
                )";

                interp.evaluate(test);

                return sapp_metadot_void;
            }
            if (options.test == "test_xlsx") {
                lxw_workbook *workbook = workbook_new("./data/test.xlsx");
                lxw_worksheet *worksheet = workbook_add_worksheet(workbook, NULL);

                worksheet_write_string(worksheet, 0, 0, "Hello", NULL);
                worksheet_write_number(worksheet, 1, 0, 123, NULL);

                workbook_close(workbook);
                return sapp_metadot_void;
            }
        }

    } catch (structopt::exception &e) {
        std::cout << e.what() << "\n";
        std::cout << e.help();
    }

    // init console & print title
    std::cout << logo << std::endl;

    loguru::init(argc, argv);
    METADOT_INFO("Starting game...");

    return sapp_metadot_main;
}


int Game::init(int argc, char *argv[]) {
    return 0;
}
int Game::run(int argc, char *argv[]) {
    return 0;
}