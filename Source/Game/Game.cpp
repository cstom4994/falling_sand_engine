// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Game/Game.hpp"

Game::Game() {}
Game::~Game() {}

#include "Engine/Render/MRender.hpp"
#include "Libs/raylib/raylib.h"
#include "Libs/structopt.hpp"

#include "Libs/raylib/raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "Libs/raylib/raygui.h"

#include "Engine/UserInterface/IMGUI/ImGuiImpl.h"
#include "Engine/UserInterface/NuklearImpl.h"
#include "imgui.h"

#include <cmath>
#include <cstdlib>
#include <string>

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



#include "Game/Cell/types.h"

#include "Game/Cell/cell.h"
#include "Game/Cell/lava.h"
#include "Game/Cell/moose.h"
#include "Game/Cell/sand.h"
#include "Game/Cell/spray.h"
#include "Game/Cell/static.h"
#include "Game/Cell/stone.h"
#include "Game/Cell/water.h"

const int screenWidth = mapSize * Cell::SIZE;
const int screenHeight = screenWidth + 60;

const std::vector<std::string> adjectives = {
        "untidy",
        "damaging",
        "disgusting",
        "nonstop",
        "panoramic",
        "fretful",
        "left",
        "ablaze",
        "educated",
        "animated",
        "jittery",
        "serious",
        "elite",
        "actually",
        "soft",
        "neat",
        "tested",
        "aboriginal",
        "splendid",
        "little"};

typedef struct OperationPair
{
    Cell (*Create)();
    void (*Process)(CellularData &, CellMap &);
} OperationPair;

struct OperationPair Operations[] = {
        OperationPair{CreateAir, ProcessAir},      // Air
        OperationPair{CreateSand, ProcessSand},    // Sand
        OperationPair{CreateStatic, ProcessStatic},// Static
        OperationPair{CreateSpray, ProcessSpray},  // Spray
        OperationPair{CreateMoose, ProcessMoose},  // Moose
        OperationPair{CreateWater, ProcessWater},  // Water
        OperationPair{CreateLava, ProcessLava},
        OperationPair{CreateStone, ProcessStone}};

Cell MakeCell(CellType type) {
    if (type >= CellType::END_TYPE) return Operations[(int) CellType::Air].Create();
    return Operations[(int) type].Create();
}

CellType QuerySelectedType(int key) {
    switch (key) {
        case 0:
            break;
        case KEY_ZERO:
            return CellType::Air;
        case KEY_ONE:
            return CellType::Sand;
        case KEY_TWO:
            return CellType::Static;
        case KEY_THREE:
            return CellType::Spray;
        case KEY_FOUR:
            return CellType::Moose;
        case KEY_FIVE:
            return CellType::Water;
    }
    return CellType::Air;
}

void ProcessCell(const Cell &cell, CellularData &data, CellMap &map) {
    CellType type = cell._id;
    if (type >= CellType::END_TYPE) return Operations[(int) CellType::Air].Process(data, map);
    Operations[(int) type].Process(data, map);
}

void ProcessMap(CellMap &map, unsigned worldClock) {
    int mapSize = map.size();
    for (int i = 0; i < mapSize; i++) {
        for (int j = 0; j < mapSize; j++) {
            const Cell &cell = map[i][j];
            CellularData data = {i, j, worldClock};
            // this is stupid
            if (cell.clock < worldClock)
                ProcessCell(cell, data, map);
        }
    }
};

void DrawUI(CellType &selectedType) {
    int height = 40;
    int width = 90;
    for (int i = (int) CellType::Air; i < (int) CellType::END_TYPE; i++) {
        CellType type = static_cast<CellType>(i);
        Rectangle pos{(float) height / 2 + (width) *i, screenHeight - 50, (float) width, (float) height};
        bool clicked = GuiButton(pos, (std::string("Particle\n") + std::string(Cellnames[i])).c_str());
        if (clicked) {
            selectedType = type;// It just works
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
                // lxw_workbook *workbook = workbook_new("./data/test.xlsx");
                // lxw_worksheet *worksheet = workbook_add_worksheet(workbook, NULL);

                // worksheet_write_string(worksheet, 0, 0, "Hello", NULL);
                // worksheet_write_number(worksheet, 1, 0, 123, NULL);

                // workbook_close(workbook);
                // return 0;
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

    CellType selectedType = CellType::Sand;
    int worldClock = 0;

    static_assert(mapSize % 2 == 0);

    CellMap workMap;
    workMap.resize(mapSize, std::vector<Cell>(mapSize, Cell()));

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

    struct nk_colorf bg = ColorToNuklearF(SKYBLUE);
    struct nk_context *ctx = InitNuklear(10);

    // Main game loop
    while (!WindowShouldClose() && !Quit) {
        ImageViewer.Update();
        SceneView.Update();

        worldClock += 1;
        ProcessMap(workMap, worldClock);

        // Update
        UpdateNuklear(ctx);

        // GUI
        if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
                     NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
                             NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE)) {
            enum {
                EASY,
                HARD
            };
            static int op = EASY;
            static int property = 20;
            nk_layout_row_static(ctx, 30, 80, 1);
            if (nk_button_label(ctx, "button"))
                TraceLog(LOG_INFO, "button pressed");

            nk_layout_row_dynamic(ctx, 30, 2);
            if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

            nk_layout_row_dynamic(ctx, 25, 1);
            nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

            nk_layout_row_dynamic(ctx, 20, 1);
            nk_label(ctx, "background:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx), 400))) {
                nk_layout_row_dynamic(ctx, 120, 1);
                bg = nk_color_picker(ctx, bg, NK_RGBA);
                nk_layout_row_dynamic(ctx, 25, 1);
                bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f, 0.005f);
                bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f, 0.005f);
                bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f, 0.005f);
                bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f, 0.005f);
                nk_combo_end(ctx);
            }
        }

        nk_end(ctx);


        BeginDrawing();
        ClearBackground(DARKGRAY);


        Vector2 mousepos = GetMousePosition();

        // TODO: Replace with texture drawing
        for (int i = 0; i < mapSize; i++) {
            for (int j = 0; j < mapSize; j++) {
                Cell &cell = workMap[i][j];
                int x = (i * Cell::SIZE);
                int y = (j * Cell::SIZE);
                Color col = cell._col;
                //if (selected.x == x && selected.y == y)
                //col = GOLD;
                DrawRectangle(x + padding * i, y + padding * j, Cell::SIZE, Cell::SIZE, col);
            }
        }

        std::string debug("Mouse location at: ");
        debug += std::to_string(mousepos.x);
        debug += ' ';
        debug += std::to_string(mousepos.y);
        DrawText(debug.c_str(), 10, 10, 12, BLACK);

        debug = "Rounded to: ";
        debug += std::to_string(Cell::RoundToNearestSize(mousepos).x);
        debug += ' ';
        debug += std::to_string(Cell::RoundToNearestSize(mousepos).y);
        DrawText(debug.c_str(), 10, 30, 12, BLACK);

        DrawUI(selectedType);


        Mtd_ImGuiBegin();
        DoMainMenu();

        if (ImGuiDemoOpen)
            ImGui::ShowDemoWindow(&ImGuiDemoOpen);

        if (ImageViewer.Open)
            ImageViewer.Show();

        if (SceneView.Open)
            SceneView.Show();

        Mtd_ImGuiEnd();

        DrawNuklear(ctx);


        EndDrawing();


        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Vector2 selected = Cell::RoundToNearestSize(mousepos, padding);
            selected.x = fmaxf(0, selected.x);
            selected.y = fmaxf(0, selected.y);
            if (selected.x < mapSize * Cell::SIZE && selected.y < mapSize * Cell::SIZE) {
                Cell &cell = workMap[(int) selected.x / Cell::SIZE][(int) selected.y / Cell::SIZE];
                cell = MakeCell(selectedType);
            }
        }

        int key = GetKeyPressed();
        if (key != 0) {
            selectedType = QuerySelectedType(key);
        }
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
