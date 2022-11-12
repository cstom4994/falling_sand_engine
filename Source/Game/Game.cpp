// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Game/Game.hpp"

Game::Game() {}
Game::~Game() {}

#include "Engine/Render/MRender.hpp"
#include "Libs/raylib/raylib.h"
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

int main(int argc, char *argv[]) {

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

                return 0;
            }
            if (options.test == "test_xlsx") {
                lxw_workbook *workbook = workbook_new("./data/test.xlsx");
                lxw_worksheet *worksheet = workbook_add_worksheet(workbook, NULL);

                worksheet_write_string(worksheet, 0, 0, "Hello", NULL);
                worksheet_write_number(worksheet, 1, 0, 123, NULL);

                workbook_close(workbook);
                return 0;
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

    InitWindow(800, 450, "Test");

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();


    return 0;
}


int Game::init(int argc, char *argv[]) {
    return 0;
}
int Game::run(int argc, char *argv[]) {
    return 0;
}