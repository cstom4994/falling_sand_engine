// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Game/Game.hpp"

Game::Game() {}
Game::~Game() {}

#include "Engine/Render/MRender.hpp"
#include "Libs/libxlsxwriter/xlsxwriter.h"
#include "Libs/raylib/raylib.h"
#include "Libs/structopt.hpp"

#include "Libs/raylib/raymath.h"

#include "Engine/IMGUI/ImGuiImpl.h"
#include "imgui.h"


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


bool Quit = false;

bool ImGuiDemoOpen = false;

class DocumentWindow {
public:
    bool Open = false;

    RenderTexture ViewTexture;

    virtual void Setup() = 0;
    virtual void Shutdown() = 0;
    virtual void Show() = 0;
    virtual void Update() = 0;

    bool Focused = false;

    Rectangle ContentRect = {0};
};

class ImageViewerWindow : public DocumentWindow {
public:
    void Setup() override {
        Camera.zoom = 1;
        Camera.target.x = 0;
        Camera.target.y = 0;
        Camera.rotation = 0;
        Camera.offset.x = GetScreenWidth() / 2.0f;
        Camera.offset.y = GetScreenHeight() / 2.0f;

        ViewTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
        ImageTexture = LoadTexture("resources/parrots.png");

        UpdateRenderTexture();
    }

    void Show() override {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::SetNextWindowSizeConstraints(ImVec2(400, 400), ImVec2((float) GetScreenWidth(), (float) GetScreenHeight()));

        Focused = false;

        if (ImGui::Begin("Image Viewer", &Open, ImGuiWindowFlags_NoScrollbar)) {
            // save off the screen space content rectangle
            ContentRect = {ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x, ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y, ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y};

            Focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

            ImVec2 size = ImGui::GetContentRegionAvail();

            // center the scratch pad in the view
            Rectangle viewRect = {0};
            viewRect.x = ViewTexture.texture.width / 2 - size.x / 2;
            viewRect.y = ViewTexture.texture.height / 2 - size.y / 2;
            viewRect.width = size.x;
            viewRect.height = -size.y;

            if (ImGui::BeginChild("Toolbar", ImVec2(ImGui::GetContentRegionAvail().x, 25))) {
                ImGui::SetCursorPosX(2);
                ImGui::SetCursorPosY(3);

                if (ImGui::Button("None")) {
                    CurrentToolMode = ToolMode::None;
                }
                ImGui::SameLine();

                if (ImGui::Button("Move")) {
                    CurrentToolMode = ToolMode::Move;
                }

                ImGui::SameLine();
                switch (CurrentToolMode) {
                    case ToolMode::None:
                        ImGui::TextUnformatted("No Tool");
                        break;
                    case ToolMode::Move:
                        ImGui::TextUnformatted("Move Tool");
                        break;
                    default:
                        break;
                }

                ImGui::SameLine();
                ImGui::TextUnformatted(TextFormat("camera target X%f Y%f", Camera.target.x, Camera.target.y));
                ImGui::EndChild();
            }

            Mtd_ImGuiImageRect(&ViewTexture.texture, (int) size.x, (int) size.y, viewRect);

            ImGui::End();
        }
        ImGui::PopStyleVar();
    }

    void Update() override {
        if (!Open)
            return;

        if (IsWindowResized()) {
            UnloadRenderTexture(ViewTexture);
            ViewTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

            Camera.offset.x = GetScreenWidth() / 2.0f;
            Camera.offset.y = GetScreenHeight() / 2.0f;
        }

        Vector2 mousePos = GetMousePosition();

        if (Focused) {
            if (CurrentToolMode == ToolMode::Move) {
                // only do this tool when the mouse is in the content area of the window
                if (IsMouseButtonDown(0) && CheckCollisionPointRec(mousePos, ContentRect)) {
                    if (!Dragging) {
                        LastMousePos = mousePos;
                        LastTarget = Camera.target;
                    }
                    Dragging = true;
                    Vector2 mouseDelta = Vector2Subtract(LastMousePos, mousePos);

                    mouseDelta.x /= Camera.zoom;
                    mouseDelta.y /= Camera.zoom;
                    Camera.target = Vector2Add(LastTarget, mouseDelta);

                    DirtyScene = true;

                } else {
                    Dragging = false;
                }
            }
        } else {
            Dragging = false;
        }

        if (DirtyScene) {
            DirtyScene = false;
            UpdateRenderTexture();
        }
    }

    Texture ImageTexture;
    Camera2D Camera = {0};

    Vector2 LastMousePos = {0};
    Vector2 LastTarget = {0};
    bool Dragging = false;

    bool DirtyScene = false;

    enum class ToolMode {
        None,
        Move,
    };

    ToolMode CurrentToolMode = ToolMode::None;

    void UpdateRenderTexture() {
        BeginTextureMode(ViewTexture);
        ClearBackground(BLUE);
        BeginMode2D(Camera);
        DrawTexture(ImageTexture, ImageTexture.width / -2, ImageTexture.height / -2, WHITE);
        EndMode2D();
        EndTextureMode();
    }

    void Shutdown() override {
        UnloadRenderTexture(ViewTexture);
        UnloadTexture(ImageTexture);
    }
};

class SceneViewWindow : public DocumentWindow {
public:
    Camera3D Camera = {0};

    void Setup() override {
        ViewTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

        Camera.fovy = 45;
        Camera.up.y = 1;
        Camera.position.y = 3;
        Camera.position.z = -25;

        Image img = GenImageChecked(256, 256, 32, 32, DARKGRAY, WHITE);
        GridTexture = LoadTextureFromImage(img);
        UnloadImage(img);
        GenTextureMipmaps(&GridTexture);
        SetTextureFilter(GridTexture, TEXTURE_FILTER_ANISOTROPIC_16X);
        SetTextureWrap(GridTexture, TEXTURE_WRAP_CLAMP);
    }

    void Shutdown() override {
        UnloadRenderTexture(ViewTexture);
        UnloadTexture(GridTexture);
    }

    void Show() override {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::SetNextWindowSizeConstraints(ImVec2(400, 400), ImVec2((float) GetScreenWidth(), (float) GetScreenHeight()));

        if (ImGui::Begin("3D View", &Open, ImGuiWindowFlags_NoScrollbar)) {
            Focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);

            ImVec2 size = ImGui::GetContentRegionAvail();

            Rectangle viewRect = {0};
            viewRect.x = ViewTexture.texture.width / 2 - size.x / 2;
            viewRect.y = ViewTexture.texture.height / 2 - size.y / 2;
            viewRect.width = size.x;
            viewRect.height = -size.y;

            // draw the view
            Mtd_ImGuiImageRect(&ViewTexture.texture, (int) size.x, (int) size.y, viewRect);

            ImGui::End();
        }
        ImGui::PopStyleVar();
    }

    void Update() override {
        if (!Open)
            return;

        if (IsWindowResized()) {
            UnloadRenderTexture(ViewTexture);
            ViewTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
        }

        float period = 10;
        float magnitude = 25;

        Camera.position.x = (float) (sinf((float) GetTime() / period) * magnitude);

        BeginTextureMode(ViewTexture);
        ClearBackground(SKYBLUE);

        BeginMode3D(Camera);

        // grid of cube trees on a plane to make a "world"
        DrawPlane(Vector3{0, 0, 0}, Vector2{50, 50}, BEIGE);// simple world plane
        float spacing = 4;
        int count = 5;

        for (float x = -count * spacing; x <= count * spacing; x += spacing) {
            for (float z = -count * spacing; z <= count * spacing; z += spacing) {
                Vector3 pos = {x, 0.5f, z};

                Vector3 min = {x - 0.5f, 0, z - 0.5f};
                Vector3 max = {x + 0.5f, 1, z + 0.5f};

                DrawCubeTexture(GridTexture, Vector3{x, 1.5f, z}, 1, 1, 1, GREEN);
                DrawCubeTexture(GridTexture, Vector3{x, 0.5f, z}, 0.25f, 1, 0.25f, BROWN);
            }
        }

        EndMode3D();
        EndTextureMode();
    }

    Texture2D GridTexture = {0};
};


ImageViewerWindow ImageViewer;
SceneViewWindow SceneView;

void DoMainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit"))
                Quit = true;

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("ImGui Demo", nullptr, &ImGuiDemoOpen);
            ImGui::MenuItem("Image Viewer", nullptr, &ImageViewer.Open);
            ImGui::MenuItem("3D View", nullptr, &SceneView.Open);

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}


int Game::init(int argc, char *argv[]) {


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

    int screenWidth = 1080;
    int screenHeight = 720;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Test");
    SetTargetFPS(144);
    Mtd_ImGuiSetup(true);
    ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;

    ImageViewer.Setup();
    ImageViewer.Open = true;

    SceneView.Setup();
    SceneView.Open = true;

    // Main game loop
    while (!WindowShouldClose() && !Quit)
    {
        ImageViewer.Update();
        SceneView.Update();

        BeginDrawing();
        ClearBackground(DARKGRAY);

        Mtd_ImGuiBegin();
        DoMainMenu();

        if (ImGuiDemoOpen)
            ImGui::ShowDemoWindow(&ImGuiDemoOpen);

        if (ImageViewer.Open)
            ImageViewer.Show();

        if (SceneView.Open)
            SceneView.Show();

        Mtd_ImGuiEnd();

        EndDrawing();
    }
    Mtd_ImGuiShutdown();

    ImageViewer.Shutdown();
    SceneView.Shutdown();

    CloseWindow();

    return 0;
}
int Game::run(int argc, char *argv[]) {
    return 0;
}
