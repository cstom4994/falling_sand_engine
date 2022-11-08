// Copyright(c) 2022, KaoruXun All rights reserved.

#include "ImGuiLayer.hpp"

#include "Game/ImGuiBase.h"
#include "Libs/ImGui/implot.h"

#include "Libs/final_dynamic_opengl.h"

#include "Engine/IMGUI/ImGuiDSL.hpp"
#include "Game/Core.hpp"
#include "Game/GCManager.hpp"
#include "Game/Macros.hpp"
#include "Game/Const.hpp"
#include "Settings.hpp"
#include "Game/Utils.hpp"

#include "imgui.h"
#include "uidsl/hello.h"

#include "Game/Legacy/Game.hpp"
#include "Game/InEngine.h"
#include "Game/Legacy/Textures.hpp"

#include <cstdio>
#include <map>

#include "Game/Legacy/DefaultGenerator.cpp"
#include "Game/Legacy/MaterialTestGenerator.cpp"

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

void MetaEngine::GameUI_Draw(Game *game) {
    // for (MetaEngine::Module *l: *game->getModuleStack())
    //     l->onImGuiRender();

    DebugDrawUI::Draw(game);
    DebugCheatsUI::Draw(game);
    MainMenuUI::Draw(game);
    IngameUI::Draw(game);
}


int IngameUI::state = 0;

bool IngameUI::visible = false;
bool IngameUI::setup = false;

void IngameUI::Setup() {
}

void IngameUI::Draw(Game *game) {


    if (!visible) return;

    if (state == 0) {
        DrawIngame(game);
    } else if (state == 1) {
        DrawOptions(game);
    }
}

void IngameUI::DrawIngame(Game *game) {


    if (!setup) {
        Setup();
    }

    ImGui::SetNextWindowSize(ImVec2(400, 300));
    ImGui::SetNextWindowPos(game->getImGuiLayer()->GetNextWindowsPos(MetaEngine::ImGuiWindowTags::UI_MainMenu, ImVec2(game->WIDTH / 2 - 200, game->HEIGHT / 2 - 250)), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Pause Menu", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("选项").x / 2);
    ImGui::Text("选项");
    //ImGui::PopFont();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

    int mainMenuButtonsWidth = 250;
    int mainMenuButtonsYOffset = 50;

    ImVec2 selPos;

    ImGui::SetCursorPos(ImVec2(200 - mainMenuButtonsWidth / 2, 25 + mainMenuButtonsYOffset * 1));
    selPos = ImGui::GetCursorPos();
    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    if (ImGui::Button("##continue", ImVec2(mainMenuButtonsWidth, 36))) {
        visible = false;
    }
    ImGui::PopStyleVar();
    ImGui::SetCursorPos(ImVec2(selPos.x + mainMenuButtonsWidth / 2 - ImGui::CalcTextSize("继续").x / 2, selPos.y));
    ImGui::Text("继续");
    //ImGui::PopFont();

    ImGui::SetCursorPos(ImVec2(200 - mainMenuButtonsWidth / 2, 25 + mainMenuButtonsYOffset * 2));
    selPos = ImGui::GetCursorPos();
    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    if (ImGui::Button("##options", ImVec2(mainMenuButtonsWidth, 36))) {
        state = 1;
    }
    ImGui::PopStyleVar();
    ImGui::SetCursorPos(ImVec2(selPos.x + mainMenuButtonsWidth / 2 - ImGui::CalcTextSize("选项").x / 2, selPos.y));
    ImGui::Text("选项");
    //ImGui::PopFont();


    ImGui::SetCursorPos(ImVec2(200 - mainMenuButtonsWidth / 2, 25 + mainMenuButtonsYOffset * 4));
    selPos = ImGui::GetCursorPos();
    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    if (ImGui::Button("##quit", ImVec2(mainMenuButtonsWidth, 36))) {
        visible = false;
        game->quitToMainMenu();
    }
    ImGui::PopStyleVar();
    ImGui::SetCursorPos(ImVec2(selPos.x + mainMenuButtonsWidth / 2 - ImGui::CalcTextSize("离开到主菜单").x / 2, selPos.y));
    ImGui::Text("离开到主菜单");
    //ImGui::PopFont();

    ImGui::End();
}

void IngameUI::DrawOptions(Game *game) {
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.11f, 0.11f, 0.9f));
    ImGui::SetNextWindowSize(ImVec2(400, 400));
    if (!ImGui::Begin("Pause Menu", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        ImGui::PopStyleColor();
        return;
    }

    OptionsUI::Draw(game);

    ImGui::End();
    ImGui::PopStyleColor();
}


// std::map<std::string, FMOD::Studio::Bus *> OptionsUI::busMap = {};
int OptionsUI::item_current_idx = 0;
bool OptionsUI::vsync = false;
bool OptionsUI::minimizeOnFocus = false;

void OptionsUI::Draw(Game *game) {


    int createWorldWidth = 350;

    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("选项").x / 2);
    ImGui::Text("选项");
    //ImGui::PopFont();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

    static int prevTab = 0;
    int tab = 0;
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("OptionsTabs", tab_bar_flags)) {

        if (ImGui::BeginTabItem("General")) {
            tab = 0;
            ImGui::BeginChild("OptionsTabsCh", ImVec2(0, 250), false);

            DrawGeneral(game);

            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("视频")) {
            tab = 1;
            ImGui::BeginChild("OptionsTabsCh", ImVec2(0, 250), false);

            DrawVideo(game);

            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("音频")) {
            tab = 2;
            ImGui::BeginChild("OptionsTabsCh", ImVec2(0, 250), false);

            DrawAudio(game);

            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("输入")) {
            tab = 3;
            ImGui::BeginChild("OptionsTabsCh", ImVec2(0, 250), false);

            DrawInput(game);

            ImGui::EndChild();
            ImGui::EndTabItem();
        }


        ImGui::EndTabBar();
    }

    if (tab != prevTab) {
        game->getCAudioEngine()->PlayEvent("event:/GUI/GUI_Tab");
        prevTab = tab;
    }

    ImGui::Separator();

    ImGui::SetCursorPos(ImVec2(20, ImGui::GetCursorPosY() + 5));
    ImVec2 selPos = ImGui::GetCursorPos();
    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    if (ImGui::Button("  ", ImVec2(150, 36))) {
        MainMenuUI::state = 0;
        IngameUI::state = 0;
    }
    ImGui::PopStyleVar();
    ImGui::SetCursorPos(ImVec2(selPos.x + 150 / 2 - ImGui::CalcTextSize("返回").x / 2, selPos.y));
    ImGui::Text("返回");
    //ImGui::PopFont();
}

void OptionsUI::DrawGeneral(Game *game) {
    ImGui::TextColored(ImVec4(1.0, 1.0, 0.8, 1.0), "%s", "游戏内容");
    ImGui::Indent(4);

    ImGui::Checkbox("材质工具提示", &Settings::draw_material_info);

    ImGui::Unindent(4);
}

void OptionsUI::DrawVideo(Game *game) {
    ImGui::TextColored(ImVec4(1.0, 1.0, 0.8, 1.0), "%s", "窗口");
    ImGui::Indent(4);

    const char *items[] = {"Windowed", "Fullscreen Borderless", "Fullscreen"};
    const char *combo_label = items[item_current_idx];// Label to preview before opening the combo (technically it could be anything)
    ImGui::SetNextItemWidth(190);
    if (ImGui::BeginCombo("Display Mode", combo_label, 0)) {
        for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
            const bool is_selected = (item_current_idx == n);
            if (ImGui::Selectable(items[n], is_selected)) {

                switch (n) {
                    case 0:
                        game->setDisplayMode(DisplayMode::WINDOWED);
                        break;
                    case 1:
                        game->setDisplayMode(DisplayMode::BORDERLESS);
                        break;
                    case 2:
                        game->setDisplayMode(DisplayMode::FULLSCREEN);
                        break;
                }

                item_current_idx = n;
            }


            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::Checkbox("VSync", &vsync)) {
        game->setVSync(vsync);
    }

    if (ImGui::Checkbox("失去焦点后最小化", &minimizeOnFocus)) {
        game->setMinimizeOnLostFocus(minimizeOnFocus);
    }


    ImGui::Unindent(4);
    ImGui::Separator();

    ImGui::TextColored(ImVec4(1.0, 1.0, 0.8, 1.0), "%s", "Rendering");
    ImGui::Indent(4);

    if (ImGui::Checkbox("高清贴图", &Settings::hd_objects)) {
        METAENGINE_Render_FreeTarget(game->textureObjects->target);
        METAENGINE_Render_FreeImage(game->textureObjects);
        METAENGINE_Render_FreeTarget(game->textureObjectsBack->target);
        METAENGINE_Render_FreeImage(game->textureObjectsBack);
        METAENGINE_Render_FreeTarget(game->textureEntities->target);
        METAENGINE_Render_FreeImage(game->textureEntities);

        game->textureObjects = METAENGINE_Render_CreateImage(
                game->getWorld()->width * (Settings::hd_objects ? Settings::hd_objects_size : 1), game->getWorld()->height * (Settings::hd_objects ? Settings::hd_objects_size : 1),
                METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
        METAENGINE_Render_SetImageFilter(game->textureObjects, METAENGINE_Render_FILTER_NEAREST);

        game->textureObjectsBack = METAENGINE_Render_CreateImage(
                game->getWorld()->width * (Settings::hd_objects ? Settings::hd_objects_size : 1), game->getWorld()->height * (Settings::hd_objects ? Settings::hd_objects_size : 1),
                METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
        METAENGINE_Render_SetImageFilter(game->textureObjectsBack, METAENGINE_Render_FILTER_NEAREST);

        METAENGINE_Render_LoadTarget(game->textureObjects);
        METAENGINE_Render_LoadTarget(game->textureObjectsBack);

        game->textureEntities = METAENGINE_Render_CreateImage(
                game->getWorld()->width * (Settings::hd_objects ? Settings::hd_objects_size : 1), game->getWorld()->height * (Settings::hd_objects ? Settings::hd_objects_size : 1),
                METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
        METAENGINE_Render_SetImageFilter(game->textureEntities, METAENGINE_Render_FILTER_NEAREST);

        METAENGINE_Render_LoadTarget(game->textureEntities);
    }

    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("光照质量", &Settings::lightingQuality, 0.0, 1.0, "", 0);
    ImGui::Checkbox("简单光照", &Settings::simpleLighting);
    ImGui::Checkbox("抖动光照", &Settings::lightingDithering);

    ImGui::Unindent(4);
}

void OptionsUI::DrawAudio(Game *game) {
    ImGui::TextColored(ImVec4(1.0, 1.0, 0.8, 1.0), "%s", "Volume");
    ImGui::Indent(4);

    // if (busMap.size() == 0) {
    //     FMOD::Studio::Bus *busses[20];
    //     int busCt = 0;
    //     game->audioEngine.GetBank(METADOT_RESLOC("data/assets/audio/fmod/Build/Desktop/Master.bank"))->getBusList(busses, 20, &busCt);

    //     busMap = {};

    //     for (int i = 0; i < busCt; i++) {
    //         FMOD::Studio::Bus *b = busses[i];
    //         char path[100];
    //         int ctPath = 0;
    //         b->getPath(path, 100, &ctPath);

    //         busMap[std::string(path)] = b;
    //     }
    // }

    std::vector<std::vector<std::string>> disp = {
            {"bus:/Master", "Master"},
            {"bus:/Master/Underwater/Music", "Music"},
            {"bus:/Master/GUI", "GUI"},
            {"bus:/Master/Underwater/Player", "Player"},
            {"bus:/Master/Underwater/World", "World"}};

    // for (auto &v: disp) {
    //     float volume = 0;
    //     busMap[v[0]]->getVolume(&volume);
    //     volume *= 100;
    //     if (ImGui::SliderFloat(v[1].c_str(), &volume, 0.0f, 100.0f, "%0.0f%%")) {
    //         volume = std::max(0.0f, std::min(volume, 100.0f));
    //         busMap[v[0]]->setVolume(volume / 100.0f);
    //     }
    // }

    ImGui::Unindent(4);
}

void OptionsUI::DrawInput(Game *game) {
}


int MainMenuUI::state = 0;

bool MainMenuUI::visible = true;
bool MainMenuUI::setup = false;
METAENGINE_Render_Image *MainMenuUI::title = nullptr;
bool MainMenuUI::connectButtonEnabled = false;
ImVec2 MainMenuUI::pos = ImVec2(0, 0);
std::vector<std::tuple<std::string, WorldMeta>> MainMenuUI::worlds = {};
long long MainMenuUI::lastRefresh = 0;

bool sortWorlds(std::tuple<std::string, WorldMeta> w1, std::tuple<std::string, WorldMeta> w2) {
    int64_t c1 = std::get<1>(w1).lastOpenedTime;
    int64_t c2 = std::get<1>(w2).lastOpenedTime;

    return (c1 > c2);
}

void MainMenuUI::RefreshWorlds(Game *game) {


    worlds = {};

    for (auto &p: std::filesystem::directory_iterator(game->getGameDir()->getPath("worlds/"))) {
        std::string worldName = p.path().filename().generic_string();

        WorldMeta meta = WorldMeta::loadWorldMeta((char *) game->getGameDir()->getWorldPath(worldName).c_str());

        worlds.push_back(std::make_tuple(worldName, meta));
    }

    sort(worlds.begin(), worlds.end(), sortWorlds);
}

void MainMenuUI::Setup() {


    SDL_Surface *logoSfc = Textures::loadTexture("data/assets/ui/logo.png");
    title = METAENGINE_Render_CopyImageFromSurface(logoSfc);
    METAENGINE_Render_SetImageFilter(title, METAENGINE_Render_FILTER_NEAREST);
    SDL_FreeSurface(logoSfc);

    setup = true;
}

void MainMenuUI::Draw(Game *game) {


    if (!visible) return;

    if (state == 0) {
        DrawMainMenu(game);
    } else if (state == 1) {
        DrawCreateWorld(game);
    } else if (state == 2) {
        DrawSingleplayer(game);
    } else if (state == 3) {
        DrawMultiplayer(game);
    } else if (state == 4) {
        DrawOptions(game);
    }
}

void MainMenuUI::DrawMainMenu(Game *game) {


    if (!setup) {
        Setup();
    }
    long long now = UTime::millis();
    if (now - lastRefresh > 3000) {
        RefreshWorlds(game);
        lastRefresh = now;
    }

    ImGui::SetNextWindowSize(ImVec2(400, 350));
    ImGui::SetNextWindowPos(game->getImGuiLayer()->GetNextWindowsPos(MetaEngine::ImGuiWindowTags::UI_MainMenu, ImVec2(game->WIDTH / 2 - 400 / 2, game->HEIGHT / 2 - 350 / 2)), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Main Menu", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }
    pos = ImGui::GetWindowPos();

    ImTextureID texId = (ImTextureID) METAENGINE_Render_GetTextureHandle(title);

    ImVec2 uv_min = ImVec2(0.0f, 0.0f);              // Top-left
    ImVec2 uv_max = ImVec2(1.0f, 1.0f);              // Lower-right
    ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);// No tint
    ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);

    ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - title->w / 2) * 0.5f, ImGui::GetCursorPosY() + 10));
    ImGui::Image(texId, ImVec2(title->w / 2, title->h / 2), uv_min, uv_max, tint_col, border_col);
    ImGui::TextColored(ImVec4(211.0f, 211.0f, 211.0f, 255.0f), U8("大摆钟送快递"));

    int mainMenuButtonsWidth = 250;
    int mainMenuButtonsYOffset = 55;
    ImGui::SetCursorPos(ImVec2(200 - mainMenuButtonsWidth / 2, 60 + mainMenuButtonsYOffset));
    ImVec2 selPos = ImGui::GetCursorPos();
    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    if (ImGui::Button("##singleplayer", ImVec2(mainMenuButtonsWidth, 36))) {
        state = 2;
    }
    ImGui::PopStyleVar();
    ImGui::SetCursorPos(ImVec2(selPos.x + mainMenuButtonsWidth / 2 - ImGui::CalcTextSize("单人游戏").x / 2, selPos.y));
    ImGui::Text(U8("单人游戏"));
    //ImGui::PopFont();

    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);

    ImGui::SetCursorPos(ImVec2(200 - mainMenuButtonsWidth / 2, 60 + mainMenuButtonsYOffset * 2));
    selPos = ImGui::GetCursorPos();
    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    if (ImGui::Button("##multiplayer", ImVec2(mainMenuButtonsWidth, 36))) {
        state = 2;
    }
    ImGui::PopStyleVar();
    ImGui::SetCursorPos(ImVec2(selPos.x + mainMenuButtonsWidth / 2 - ImGui::CalcTextSize("多人游戏").x / 2, selPos.y));
    ImGui::Text("多人游戏");
    //ImGui::PopFont();

    ImGui::PopItemFlag();
    ImGui::PopStyleVar();

    ImGui::SetCursorPos(ImVec2(200 - mainMenuButtonsWidth / 2, 60 + mainMenuButtonsYOffset * 3));
    selPos = ImGui::GetCursorPos();
    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    if (ImGui::Button("##options", ImVec2(mainMenuButtonsWidth, 36))) {
        state = 4;
    }
    ImGui::PopStyleVar();
    ImGui::SetCursorPos(ImVec2(selPos.x + mainMenuButtonsWidth / 2 - ImGui::CalcTextSize("选项").x / 2, selPos.y));
    ImGui::Text("选项");
    //ImGui::PopFont();


    ImGui::SetCursorPos(ImVec2(200 - mainMenuButtonsWidth / 2, 60 + mainMenuButtonsYOffset * 4));
    selPos = ImGui::GetCursorPos();
    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    if (ImGui::Button("##quit", ImVec2(mainMenuButtonsWidth, 36))) {
        game->running = false;
    }
    ImGui::PopStyleVar();
    ImGui::SetCursorPos(ImVec2(selPos.x + mainMenuButtonsWidth / 2 - ImGui::CalcTextSize("退出").x / 2, selPos.y));
    ImGui::Text("退出");
    //ImGui::PopFont();

    ImGui::End();
}

void MainMenuUI::DrawSingleplayer(Game *game) {
    long long now = UTime::millis();
    if (now - lastRefresh > 3000) {
        RefreshWorlds(game);
        lastRefresh = now;
    }
    if (!visible) return;

    ImGui::SetNextWindowSize(ImVec2(400, 425));
    ImGui::SetNextWindowPos(ImVec2(game->WIDTH / 2 - 200, game->HEIGHT / 2 - 250), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Main Menu", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("单人游戏").x / 2);
    ImGui::Text("单人游戏");
    //ImGui::PopFont();

    int mainMenuButtonsWidth = 300;
    int mainMenuButtonsYOffset = 50;
    int mainMenuNewButtonWidth = 150;
    ImGui::SetCursorPos(ImVec2(200 - mainMenuNewButtonWidth / 2, 10 + mainMenuButtonsYOffset));
    ImVec2 selPos = ImGui::GetCursorPos();
    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    if (ImGui::Button("##newworld", ImVec2(mainMenuNewButtonWidth, 36))) {
        state = 1;
        CreateWorldUI::Reset(game);
    }
    ImGui::PopStyleVar();
    ImGui::SetCursorPos(ImVec2(selPos.x + mainMenuNewButtonWidth / 2 - ImGui::CalcTextSize("新世界").x / 2, selPos.y));
    ImGui::Text("新世界");
    //ImGui::PopFont();

    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + 10));

    ImGui::Separator();

    ImGui::BeginChild("WorldList", ImVec2(0, 250), false);

    int nMainMenuButtons = 0;
    for (auto &t: worlds) {
        std::string worldName = std::get<0>(t);

        WorldMeta meta = std::get<1>(t);

        ImGui::PushID(nMainMenuButtons);

        ImGui::SetCursorPos(ImVec2(200 - 8 - mainMenuButtonsWidth / 2, 4 + (nMainMenuButtons++ * 60)));
        selPos = ImGui::GetCursorScreenPos();
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        if (ImGui::Button("", ImVec2(mainMenuButtonsWidth, 50))) {
            METADOT_INFO("Selected world: {}", worldName.c_str());
            visible = false;

            game->fadeOutStart = game->getGameTimeState().now;
            game->fadeOutLength = 250;
            game->fadeOutCallback = [&, game, worldName]() {
                game->setGameState(LOADING, INGAME);

                delete game->getWorld();
                game->setWorld(nullptr);

                //std::thread loadWorldThread([&] () {

                World *w = new World();
                w->init(game->getGameDir()->getWorldPath(worldName), (int) ceil(WINDOWS_MAX_WIDTH / 3 / (double) CHUNK_W) * CHUNK_W + CHUNK_W * 3, (int) ceil(WINDOWS_MAX_HEIGHT / 3 / (double) CHUNK_H) * CHUNK_H + CHUNK_H * 3, game->target, game->getCAudioEngine(), game->getNetworkMode());
                w->metadata.lastOpenedTime = UTime::millis() / 1000;
                w->metadata.lastOpenedVersion = std::string(VERSION);
                w->metadata.save(w->worldName);


                METADOT_INFO("Queueing chunk loading...");
                for (int x = -CHUNK_W * 4; x < w->width + CHUNK_W * 4; x += CHUNK_W) {
                    for (int y = -CHUNK_H * 3; y < w->height + CHUNK_H * 8; y += CHUNK_H) {
                        w->queueLoadChunk(x / CHUNK_W, y / CHUNK_H, true, true);
                    }
                }


                game->setWorld(w);
                //});

                game->fadeInStart = game->getGameTimeState().now;
                game->fadeInLength = 250;
                game->fadeInWaitFrames = 4;
            };
        }
        ImGui::PopStyleVar();

        ImVec2 prevPos = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(selPos.x, selPos.y));


        time_t t_times;
        tm *tm_utc = gmtime(&t_times);
        meta.lastOpenedTime = t_times;

        // convert to local time
        time_t time_utc = UTime::mkgmtime(tm_utc);
        time_t time_local = mktime(tm_utc);
        time_local += time_utc - time_local;
        tm *tm_local = localtime(&time_local);

        char *formattedTime = new char[100];
        strftime(formattedTime, 100, "%#m/%#d/%y %#I:%M%p", tm_local);

        char *filenameAndTimestamp = new char[200];
        snprintf(filenameAndTimestamp, 100, "%s (%s)", worldName.c_str(), formattedTime);

        //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::SetCursorScreenPos(ImVec2(selPos.x + mainMenuButtonsWidth / 2 - ImGui::CalcTextSize(meta.worldName.c_str()).x / 2, selPos.y));
        ImGui::Text("%s", meta.worldName.c_str());
        //ImGui::PopFont();

        ImGui::SetCursorScreenPos(ImVec2(selPos.x + mainMenuButtonsWidth / 2 - ImGui::CalcTextSize(filenameAndTimestamp).x / 2, selPos.y + 32));
        ImGui::Text("%s", filenameAndTimestamp);

        ImGui::SetCursorScreenPos(prevPos);
        ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::Separator();

    ImGui::SetCursorPos(ImVec2(20, ImGui::GetCursorPosY() + 5));
    selPos = ImGui::GetCursorPos();
    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    if (ImGui::Button("##back", ImVec2(150, 36))) {
        state = 0;
    }
    ImGui::PopStyleVar();
    ImGui::SetCursorPos(ImVec2(selPos.x + 150 / 2 - ImGui::CalcTextSize("返回").x / 2, selPos.y));
    ImGui::Text("返回");
    //ImGui::PopFont();

    ImGui::End();
}

void MainMenuUI::DrawMultiplayer(Game *game) {
    ImGui::SetNextWindowSize(ImVec2(400, 500));
    ImGui::SetNextWindowPos(ImVec2(game->WIDTH / 2 - 200, game->HEIGHT / 2 - 250), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Main Menu", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    static char connectBuf[128] = "";
    //200 - connectWidth / 2 - 60 / 2, 60, connectWidth, 20
    ImGui::PushItemWidth(200);
    ImGui::SetCursorPos(ImVec2(200 - 200 / 2 - 60 / 2, 60));
    if (ImGui::InputTextWithHint("", "ip:port", connectBuf, IM_ARRAYSIZE(connectBuf))) {
        std::regex connectInputRegex("([^:]+):(\\d+)");
        std::string str = std::string(connectBuf);
        connectButtonEnabled = regex_match(str, connectInputRegex);
    }
    ImGui::PopItemWidth();
    ImGui::PopStyleVar();
    ImGui::SameLine();

    if (!connectButtonEnabled) {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    if (ImGui::Button("连接")) {
        METADOT_INFO("connectButton select");
        if (game->getClient()->connect(Settings::server_ip.c_str(), Settings::server_port)) {
            game->setNetworkMode(NetworkMode::CLIENT);
            visible = false;
            game->setGameState(LOADING, INGAME);
        }
    }

    if (!connectButtonEnabled) {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }

    ImGui::End();
}

void MainMenuUI::DrawCreateWorld(Game *game) {
    ImGui::SetNextWindowSize(ImVec2(400, 360));
    if (!ImGui::Begin("Main Menu", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    CreateWorldUI::Draw(game);

    ImGui::End();
}

void MainMenuUI::DrawOptions(Game *game) {
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.11f, 0.11f, 0.9f));
    ImGui::SetNextWindowSize(ImVec2(400, 400));
    if (!ImGui::Begin("Main Menu", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        ImGui::PopStyleColor();
        return;
    }

    OptionsUI::Draw(game);

    ImGui::End();
    ImGui::PopStyleColor();
}


void DebugUI::Draw(Game *game) {

    // ImGui::SetNextWindowSize(ImVec2(250, 0));
    // ImGui::SetNextWindowPos(ImVec2(game->WIDTH - 200 - 15, 25), ImGuiCond_FirstUseEver);
    // if (!ImGui::Begin("Debug Settings", NULL, ImGuiWindowFlags_NoResize)) {
    //     ImGui::End();
    //     return;
    // }

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode(U8("渲染"))) {

        ImGui::Checkbox(U8("绘制帧率图"), &Settings::draw_frame_graph);
        ImGui::Checkbox(U8("绘制调试状态"), &Settings::draw_debug_stats);

        ImGui::Checkbox(U8("绘制区域块状态"), &Settings::draw_chunk_state);
        ImGui::Checkbox(U8("绘制加载区域"), &Settings::draw_load_zones);

        ImGui::Checkbox(U8("材料工具"), &Settings::draw_material_info);
        ImGui::Indent(10.0f);
        ImGui::Checkbox(U8("调试材料"), &Settings::draw_detailed_material_info);
        ImGui::Unindent(10.0f);

        if (ImGui::TreeNode(U8("物理"))) {
            ImGui::Checkbox(U8("绘制物理调试"), &Settings::draw_physics_debug);

            if (ImGui::TreeNode("Box2D")) {
                ImGui::Checkbox("shape", &Settings::draw_b2d_shape);
                ImGui::Checkbox("joint", &Settings::draw_b2d_joint);
                ImGui::Checkbox("aabb", &Settings::draw_b2d_aabb);
                ImGui::Checkbox("pair", &Settings::draw_b2d_pair);
                ImGui::Checkbox("center of mass", &Settings::draw_b2d_centerMass);

                ImGui::TreePop();
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode(U8("GLSL方法"))) {
            if (ImGui::Button(U8("重新加载GLSL"))) {
                game->loadShaders();
            }
            ImGui::Checkbox(U8("绘制GLSL"), &Settings::draw_shaders);

            if (ImGui::TreeNode(U8("光照"))) {
                ImGui::SetNextItemWidth(80);
                ImGui::SliderFloat(U8("质量"), &Settings::lightingQuality, 0.0, 1.0, "", 0);
                ImGui::Checkbox(U8("覆盖"), &Settings::draw_light_overlay);
                ImGui::Checkbox(U8("简单采样"), &Settings::simpleLighting);
                ImGui::Checkbox(U8("放射"), &Settings::lightingEmission);
                ImGui::Checkbox(U8("抖动"), &Settings::lightingDithering);

                ImGui::TreePop();
            }

            if (ImGui::TreeNode(U8("水体"))) {
                const char *items[] = {"off", "flow map", "distortion"};
                const char *combo_label = items[Settings::water_overlay];
                ImGui::SetNextItemWidth(80 + 24);
                if (ImGui::BeginCombo("Overlay", combo_label, 0)) {
                    for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
                        const bool is_selected = (Settings::water_overlay == n);
                        if (ImGui::Selectable(items[n], is_selected)) {
                            Settings::water_overlay = n;
                        }

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::Checkbox(U8("显示流程"), &Settings::water_showFlow);
                ImGui::Checkbox(U8("像素化"), &Settings::water_pixelated);

                ImGui::TreePop();
            }


            ImGui::TreePop();
        }

        if (ImGui::TreeNode(U8("世界"))) {

            ImGui::Checkbox("Draw Temperature Map", &Settings::draw_temperature_map);

            if (ImGui::Checkbox("Draw Background", &Settings::draw_background)) {
                for (int x = 0; x < game->getWorld()->width; x++) {
                    for (int y = 0; y < game->getWorld()->height; y++) {
                        game->getWorld()->dirty[x + y * game->getWorld()->width] = true;
                        game->getWorld()->layer2Dirty[x + y * game->getWorld()->width] = true;
                    }
                }
            }

            if (ImGui::Checkbox("Draw Background Grid", &Settings::draw_background_grid)) {
                for (int x = 0; x < game->getWorld()->width; x++) {
                    for (int y = 0; y < game->getWorld()->height; y++) {
                        game->getWorld()->dirty[x + y * game->getWorld()->width] = true;
                        game->getWorld()->layer2Dirty[x + y * game->getWorld()->width] = true;
                    }
                }
            }

            if (ImGui::Checkbox("HD Objects", &Settings::hd_objects)) {
                METAENGINE_Render_FreeTarget(game->textureObjects->target);
                METAENGINE_Render_FreeImage(game->textureObjects);
                METAENGINE_Render_FreeTarget(game->textureObjectsBack->target);
                METAENGINE_Render_FreeImage(game->textureObjectsBack);
                METAENGINE_Render_FreeTarget(game->textureEntities->target);
                METAENGINE_Render_FreeImage(game->textureEntities);

                game->textureObjects = METAENGINE_Render_CreateImage(
                        game->getWorld()->width * (Settings::hd_objects ? Settings::hd_objects_size : 1), game->getWorld()->height * (Settings::hd_objects ? Settings::hd_objects_size : 1),
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                METAENGINE_Render_SetImageFilter(game->textureObjects, METAENGINE_Render_FILTER_NEAREST);

                game->textureObjectsBack = METAENGINE_Render_CreateImage(
                        game->getWorld()->width * (Settings::hd_objects ? Settings::hd_objects_size : 1), game->getWorld()->height * (Settings::hd_objects ? Settings::hd_objects_size : 1),
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                METAENGINE_Render_SetImageFilter(game->textureObjectsBack, METAENGINE_Render_FILTER_NEAREST);

                METAENGINE_Render_LoadTarget(game->textureObjects);
                METAENGINE_Render_LoadTarget(game->textureObjectsBack);

                game->textureEntities = METAENGINE_Render_CreateImage(
                        game->getWorld()->width * (Settings::hd_objects ? Settings::hd_objects_size : 1), game->getWorld()->height * (Settings::hd_objects ? Settings::hd_objects_size : 1),
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                METAENGINE_Render_SetImageFilter(game->textureEntities, METAENGINE_Render_FILTER_NEAREST);

                METAENGINE_Render_LoadTarget(game->textureEntities);
            }

            ImGui::TreePop();
        }

        ImGui::TreePop();
    }

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode(U8("模拟"))) {
        ImGui::Checkbox(U8("处理世界"), &Settings::tick_world);
        ImGui::Checkbox(U8("处理Box2D"), &Settings::tick_box2d);
        ImGui::Checkbox(U8("处理温度"), &Settings::tick_temperature);

        ImGui::TreePop();
    }


    //ImGui::End();
}


bool DebugDrawUI::visible = true;
int DebugDrawUI::selIndex = -1;
std::vector<METAENGINE_Render_Image *> DebugDrawUI::images = {};
uint8 DebugDrawUI::brushSize = 5;
Material *DebugDrawUI::selectedMaterial = &Materials::GENERIC_AIR;

void DebugDrawUI::Setup() {


    images = {};
    for (size_t i = 0; i < Materials::MATERIALS.size(); i++) {
        Material *mat = Materials::MATERIALS[i];
        SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, 16, 16, 32, SDL_PIXELFORMAT_ARGB8888);
        for (int x = 0; x < surface->w; x++) {
            for (int y = 0; y < surface->h; y++) {
                MaterialInstance m = Tiles::create(mat, x, y);
                METADOT_GET_PIXEL(surface, x, y) = m.color + (m.mat->alpha << 24);
            }
        }
        images.push_back(METAENGINE_Render_CopyImageFromSurface(surface));
        METAENGINE_Render_SetImageFilter(images[i], METAENGINE_Render_FILTER_NEAREST);
        SDL_FreeSurface(surface);
    }
}

void DebugDrawUI::Draw(Game *game) {


    if (images.empty()) Setup();

    if (!visible) return;

    int width = 5;

    int nRows = ceil(Materials::MATERIALS.size() / (float) width);

    ImGui::SetNextWindowSize(ImVec2(40 * width + 16 + 20, 70 + 5 * 40));
    ImGui::SetNextWindowPos(ImVec2(15, 25), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("材料放置测试", NULL, ImGuiWindowFlags_NoResize)) {
        ImGui::End();
        return;
    }

    auto a = selIndex == -1 ? "None" : selectedMaterial->name;
    ImGui::Text("选择: %s", a.c_str());
    ImGui::Text("放置大小: %d", brushSize);

    ImGui::Separator();

    ImGui::BeginChild("材料列表", ImVec2(0, 0), false);
    ImGui::Indent(5);
    for (size_t i = 0; i < Materials::MATERIALS.size(); i++) {
        int x = (int) (i % width);
        int y = (int) (i / width);

        if (x > 0)
            ImGui::SameLine();
        ImGui::PushID((int) i);

        ImVec2 selPos = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(selPos.x, selPos.y + (x != 0 ? -1 : 0)));
        if (ImGui::Selectable("", selIndex == i, 0, ImVec2(32, 36))) {
            selIndex = (int) i;
            selectedMaterial = Materials::MATERIALS[i];
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", Materials::MATERIALS[i]->name.c_str());
            ImGui::EndTooltip();
        }

        ImVec2 prevPos = ImGui::GetCursorScreenPos();
        ImGuiStyle &style = ImGui::GetStyle();
        ImGui::SetCursorScreenPos(ImVec2(selPos.x - 1, selPos.y + (x == 0 ? 1 : 0)));

        // imgui_impl_opengl3.cpp implements ImTextureID as GLuint
        ImTextureID texId = (ImTextureID) METAENGINE_Render_GetTextureHandle(images[i]);

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 uv_min = ImVec2(0.0f, 0.0f);              // Top-left
        ImVec2 uv_max = ImVec2(1.0f, 1.0f);              // Lower-right
        ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);// No tint
        ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);

        ImGui::Image(texId, ImVec2(32, 32), uv_min, uv_max, tint_col, border_col);

        ImGui::SetCursorScreenPos(prevPos);


        ImGui::PopID();

        /*mn->hoverCallback = [hoverMaterialLabel](Material* mat) {
            hoverMaterialLabel->text = mat->name;
            hoverMaterialLabel->updateTexture();
        };
        mn->selectCallback = [&](Material* mat) {
            selectMaterialLabel->text = mat->name;
            selectMaterialLabel->updateTexture();
            selectedMaterial = mat;
        };*/
    }

    ImGui::Unindent(5);
    ImGui::EndChild();

    ImGui::End();
}


bool DebugCheatsUI::visible = true;
std::vector<METAENGINE_Render_Image *> DebugCheatsUI::images = {};

void DebugCheatsUI::Setup() {


    images = {};
    SDL_Surface *sfc = Textures::loadTexture("data/assets/objects/testPickaxe.png");
    images.push_back(METAENGINE_Render_CopyImageFromSurface(sfc));
    METAENGINE_Render_SetImageFilter(images[0], METAENGINE_Render_FILTER_NEAREST);
    SDL_FreeSurface(sfc);
    sfc = Textures::loadTexture("data/assets/objects/testHammer.png");
    images.push_back(METAENGINE_Render_CopyImageFromSurface(sfc));
    METAENGINE_Render_SetImageFilter(images[1], METAENGINE_Render_FILTER_NEAREST);
    SDL_FreeSurface(sfc);
    sfc = Textures::loadTexture("data/assets/objects/testVacuum.png");
    images.push_back(METAENGINE_Render_CopyImageFromSurface(sfc));
    METAENGINE_Render_SetImageFilter(images[2], METAENGINE_Render_FILTER_NEAREST);
    SDL_FreeSurface(sfc);
    sfc = Textures::loadTexture("data/assets/objects/testBucket.png");
    images.push_back(METAENGINE_Render_CopyImageFromSurface(sfc));
    METAENGINE_Render_SetImageFilter(images[3], METAENGINE_Render_FILTER_NEAREST);
    SDL_FreeSurface(sfc);
}

void DebugCheatsUI::Draw(Game *game) {


    if (images.empty()) Setup();

    if (!visible) return;

    ImGui::SetNextWindowSize(ImVec2(40 * 5 + 16 - 4, 0));
    ImGui::SetNextWindowPos(ImVec2(15, 450), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("上帝箱", NULL, 0)) {
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("获得物品")) {
        ImGui::Indent();
        if (game->getWorld() == nullptr || game->getWorld()->player == nullptr) {
            ImGui::Text("世界中没有玩家");
        } else {
            int i = 0;
            ImGui::PushID(i);
            int frame_padding = 4;                           // -1 == uses default padding (style.FramePadding)
            ImVec2 size = ImVec2(48, 48);                    // Size of the image we want to make visible
            ImVec2 uv0 = ImVec2(0.0f, 0.0f);                 // UV coordinates for lower-left
            ImVec2 uv1 = ImVec2(1.0f, 1.0f);                 // UV coordinates for (32,32) in our texture
            ImVec4 bg_col = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);  // Black background
            ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);// No tint

            ImTextureID texId = (ImTextureID) METAENGINE_Render_GetTextureHandle(images[i]);
            if (ImGui::ImageButton(texId, size, uv0, uv1, frame_padding, bg_col, tint_col)) {
                Item *i3 = new Item();
                i3->setFlag(ItemFlags::TOOL);
                i3->surface = Textures::loadTexture("data/assets/objects/testPickaxe.png");
                i3->texture = METAENGINE_Render_CopyImageFromSurface(i3->surface);
                METAENGINE_Render_SetImageFilter(i3->texture, METAENGINE_Render_FILTER_NEAREST);
                i3->pivotX = 2;
                game->getWorld()->player->setItemInHand(i3, game->getWorld());
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("%s", "Pickaxe");
                ImGui::EndTooltip();
            }
            ImGui::PopID();
            ImGui::SameLine();

            i++;

            ImGui::PushID(i);
            texId = (ImTextureID) METAENGINE_Render_GetTextureHandle(images[i]);
            if (ImGui::ImageButton(texId, size, uv0, uv1, frame_padding, bg_col, tint_col)) {
                Item *i3 = new Item();
                i3->setFlag(ItemFlags::HAMMER);
                i3->surface = Textures::loadTexture("data/assets/objects/testHammer.png");
                i3->texture = METAENGINE_Render_CopyImageFromSurface(i3->surface);
                METAENGINE_Render_SetImageFilter(i3->texture, METAENGINE_Render_FILTER_NEAREST);
                i3->pivotX = 2;
                game->getWorld()->player->setItemInHand(i3, game->getWorld());
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("%s", "Hammer");
                ImGui::EndTooltip();
            }
            ImGui::PopID();

            i++;

            ImGui::PushID(i);
            texId = (ImTextureID) METAENGINE_Render_GetTextureHandle(images[i]);
            if (ImGui::ImageButton(texId, size, uv0, uv1, frame_padding, bg_col, tint_col)) {
                Item *i3 = new Item();
                i3->setFlag(ItemFlags::VACUUM);
                i3->surface = Textures::loadTexture("data/assets/objects/testVacuum.png");
                i3->texture = METAENGINE_Render_CopyImageFromSurface(i3->surface);
                METAENGINE_Render_SetImageFilter(i3->texture, METAENGINE_Render_FILTER_NEAREST);
                i3->pivotX = 6;
                game->getWorld()->player->setItemInHand(i3, game->getWorld());
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("%s", "Vacuum");
                ImGui::EndTooltip();
            }
            ImGui::PopID();
            ImGui::SameLine();

            i++;

            ImGui::PushID(i);
            texId = (ImTextureID) METAENGINE_Render_GetTextureHandle(images[i]);
            if (ImGui::ImageButton(texId, size, uv0, uv1, frame_padding, bg_col, tint_col)) {
                Item *i3 = new Item();
                i3->setFlag(ItemFlags::FLUID_CONTAINER);
                i3->surface = Textures::loadTexture("data/assets/objects/testBucket.png");
                i3->capacity = 100;
                i3->loadFillTexture(Textures::loadTexture("data/assets/objects/testBucket_fill.png"));
                i3->texture = METAENGINE_Render_CopyImageFromSurface(i3->surface);
                METAENGINE_Render_SetImageFilter(i3->texture, METAENGINE_Render_FILTER_NEAREST);
                i3->pivotX = 0;
                game->getWorld()->player->setItemInHand(i3, game->getWorld());
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("%s", "Bucket");
                ImGui::EndTooltip();
            }
            ImGui::PopID();
            ImGui::SameLine();
        }
    }

    ImGui::End();
}


char CreateWorldUI::worldNameBuf[32] = "";
bool CreateWorldUI::setup = false;
METAENGINE_Render_Image *CreateWorldUI::materialTestWorld = nullptr;
METAENGINE_Render_Image *CreateWorldUI::defaultWorld = nullptr;
bool CreateWorldUI::createWorldButtonEnabled = false;
std::string CreateWorldUI::worldFolderLabel = "";
int CreateWorldUI::selIndex = 0;

void CreateWorldUI::Setup() {


    SDL_Surface *logoMT = Textures::loadTexture("data/assets/ui/prev_materialtest.png");
    materialTestWorld = METAENGINE_Render_CopyImageFromSurface(logoMT);
    METAENGINE_Render_SetImageFilter(materialTestWorld, METAENGINE_Render_FILTER_NEAREST);
    SDL_FreeSurface(logoMT);

    SDL_Surface *logoDef = Textures::loadTexture("data/assets/ui/prev_default.png");
    defaultWorld = METAENGINE_Render_CopyImageFromSurface(logoDef);
    METAENGINE_Render_SetImageFilter(defaultWorld, METAENGINE_Render_FILTER_NEAREST);
    SDL_FreeSurface(logoDef);

    setup = true;
}

void CreateWorldUI::Draw(Game *game) {


    if (!setup) Setup();

    int createWorldWidth = 350;

    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize("Create World").x / 2);
    ImGui::Text("Create World");
    //ImGui::PopFont();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushItemWidth(createWorldWidth);
    ImGui::SetCursorPos(ImVec2(200 - createWorldWidth / 2, 70));
    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    // if(ImGui::InputTextWithHint("", "", worldNameBuf, IM_ARRAYSIZE(worldNameBuf))) {
    //     std::string text = std::string(worldNameBuf);
    //     inputChanged(text, game);
    // }
    //ImGui::PopFont();
    ImGui::PopItemWidth();
    ImGui::PopStyleVar();

    ImGui::SetCursorPos(ImVec2(200 - createWorldWidth / 2, 70 - 16));
    ImGui::Text("World Name");

    ImGui::SetCursorPos(ImVec2(200 - createWorldWidth / 2, 70 + 40 + 2));
    ImGui::Text("%s", worldFolderLabel.c_str());

    // generator selection

    ImGui::PushID(0);

    ImGui::SetCursorPos(ImVec2(400 / 2 - 100 - 24 - 4, 170));
    ImVec2 selPos = ImGui::GetCursorPos();
    if (ImGui::Selectable("", selIndex == 0, 0, ImVec2(111 - 4, 111))) {
        selIndex = 0;
    }

    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Material Test World");
        ImGui::EndTooltip();
    }

    ImVec2 prevPos = ImGui::GetCursorPos();
    ImGuiStyle &style = ImGui::GetStyle();
    ImGui::SetCursorPos(ImVec2(selPos.x - 1 + 4, selPos.y + 4));

    // imgui_impl_opengl3.cpp implements ImTextureID as GLuint
    ImTextureID texId = (ImTextureID) METAENGINE_Render_GetTextureHandle(materialTestWorld);

    ImVec2 pos = ImGui::GetCursorPos();
    ImVec2 uv_min = ImVec2(0.0f, 0.0f);              // Top-left
    ImVec2 uv_max = ImVec2(1.0f, 1.0f);              // Lower-right
    ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);// No tint
    ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);

    ImGui::Image(texId, ImVec2(100, 100), uv_min, uv_max, tint_col, border_col);

    ImGui::SetCursorPos(prevPos);

    ImGui::PopID();


    ImGui::PushID(1);

    ImGui::SetCursorPos(ImVec2(400 / 2 + 24 - 4, 170));
    selPos = ImGui::GetCursorPos();
    if (ImGui::Selectable("", selIndex == 1, 0, ImVec2(111 - 4, 111))) {
        selIndex = 1;
    }

    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Default World (WIP)");
        ImGui::EndTooltip();
    }

    prevPos = ImGui::GetCursorPos();
    style = ImGui::GetStyle();
    ImGui::SetCursorPos(ImVec2(selPos.x - 1 + 4, selPos.y + 4));

    // imgui_impl_opengl3.cpp implements ImTextureID as GLuint
    texId = (ImTextureID) METAENGINE_Render_GetTextureHandle(defaultWorld);

    pos = ImGui::GetCursorPos();
    uv_min = ImVec2(0.0f, 0.0f);              // Top-left
    uv_max = ImVec2(1.0f, 1.0f);              // Lower-right
    tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);// No tint
    border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);

    ImGui::Image(texId, ImVec2(100, 100), uv_min, uv_max, tint_col, border_col);

    ImGui::SetCursorPos(prevPos);

    ImGui::PopID();

    ImGui::SetCursorPos(ImVec2(20, 360 - 52));
    selPos = ImGui::GetCursorPos();
    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    if (ImGui::Button("##back", ImVec2(150, 36))) {
        MainMenuUI::state = 2;
    }
    ImGui::PopStyleVar();
    ImGui::SetCursorPos(ImVec2(selPos.x + 150 / 2 - ImGui::CalcTextSize("返回").x / 2, selPos.y));
    ImGui::Text("返回");
    //ImGui::PopFont();

    if (!createWorldButtonEnabled) {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    ImGui::SetCursorPos(ImVec2(400 - 170, 360 - 52));
    selPos = ImGui::GetCursorPos();
    //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    if (ImGui::Button("##create", ImVec2(150, 36))) {
        std::string pref = "Saved in: ";

        std::string worldName = worldFolderLabel.substr(pref.length());
        char *wn = (char *) worldName.c_str();

        std::string worldTitle = std::string(worldNameBuf);
        std::regex trimWhitespaceRegex("^ *(.+?) *$");
        worldTitle = regex_replace(worldTitle, trimWhitespaceRegex, "$1");

        METADOT_INFO("Creating world named \"{}\" at \"{}\"", worldTitle, game->getGameDir()->getWorldPath(wn));
        MainMenuUI::visible = false;
        game->setGameState(LOADING, INGAME);

        delete game->getWorld();
        game->setWorld(nullptr);

        WorldGenerator *generator;

        if (selIndex == 0) {
            generator = new MaterialTestGenerator();
        } else if (selIndex == 1) {
            generator = new DefaultGenerator();
        } else {
            // create world UI is in invalid state
            generator = new MaterialTestGenerator();
        }

        std::string wpStr = game->getGameDir()->getWorldPath(wn);


        game->setWorld(new World());
        game->getWorld()->init(wpStr, (int) ceil(WINDOWS_MAX_WIDTH / 3 / (double) CHUNK_W) * CHUNK_W + CHUNK_W * 3, (int) ceil(WINDOWS_MAX_HEIGHT / 3 / (double) CHUNK_H) * CHUNK_H + CHUNK_H * 3, game->target, game->getCAudioEngine(), game->getNetworkMode(), generator);
        game->getWorld()->metadata.worldName = std::string(worldNameBuf);
        game->getWorld()->metadata.lastOpenedTime = UTime::millis() / 1000;
        game->getWorld()->metadata.lastOpenedVersion = std::string(VERSION);
        game->getWorld()->metadata.save(wpStr);


        METADOT_INFO("Queueing chunk loading...");
        for (int x = -CHUNK_W * 4; x < game->getWorld()->width + CHUNK_W * 4; x += CHUNK_W) {
            for (int y = -CHUNK_H * 3; y < game->getWorld()->height + CHUNK_H * 8; y += CHUNK_H) {
                game->getWorld()->queueLoadChunk(x / CHUNK_W, y / CHUNK_H, true, true);
            }
        }
    }
    ImGui::PopStyleVar();
    ImGui::SetCursorPos(ImVec2(selPos.x + 150 / 2 - ImGui::CalcTextSize("Create").x / 2, selPos.y));
    ImGui::Text("Create");
    //ImGui::PopFont();

    if (!createWorldButtonEnabled) {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
}

void CreateWorldUI::inputChanged(std::string text, Game *game) {


    std::regex trimWhitespaceRegex("^ *(.+?) *$");
    text = regex_replace(text, trimWhitespaceRegex, "$1");
    if (text.length() == 0 || text == " ") {
        worldFolderLabel = "Saved in: ";
        createWorldButtonEnabled = false;
        return;
    }

    std::regex worldNameInputRegex("^[\\x20-\\x7E]+$");
    createWorldButtonEnabled = regex_match(text, worldNameInputRegex);

    std::regex worldFolderRegex("[\\/\\\\:*?\"<>|.]");

    std::string worldFolderName = regex_replace(text, worldFolderRegex, "_");
    std::string folder = game->getGameDir()->getWorldPath(worldFolderName);
    struct stat buffer;
    bool exists = (stat(folder.c_str(), &buffer) == 0);

    std::string newWorldFolderName = worldFolderName;
    int i = 2;
    while (exists) {
        newWorldFolderName = worldFolderName + " (" + std::to_string(i) + ")";
        folder = game->getGameDir()->getWorldPath(newWorldFolderName);

        exists = (stat(folder.c_str(), &buffer) == 0);

        i++;
    }


    worldFolderLabel = "Saved in: " + newWorldFolderName;
}

void CreateWorldUI::Reset(Game *game) {
#ifdef _WIN32
    strcpy_s(worldNameBuf, "New World");
#else
    strcpy(worldNameBuf, "New World");
#endif
    inputChanged(std::string(worldNameBuf), game);
}


static std::string const testscript = R"(
label 'A Little Test:'
toggle 'x' {label='Enable Group Foo', default=true}
text 'name' {label='Name', default='Foo\nBar\n!@#$%^&*()_+""', multiline=true}
group 'foo' {label='Foo', disablewhen='{x}==false'}
  label 'A Int Value:' {joinnext=true}
  int 'a' {max=1024, min=1}
  float 'b' {default=1024, ui='drag', speed=1}
  color 'color1' {hdr=true, default={1,0,0}}
  color 'yellow' {hdr=true, alpha=true, default={1,1,0,0.5}}
  color 'green' {hdr=true, default={0,1,0,1}, alpha=false, wheel=true}
  color 'color2' {alpha=true, hsv=true, wheel=true, default={0.3,0.1,0.4,1.0}}
  button 'sayhi' {label='Say Hi'}
  int 'c'
  struct 'X'
    float 'a'
    float 'b'
    double 'c'
  endstruct 'X'
endgroup 'foo'
spacer ''
spacer ''
separator ''
toggle 'y'
group 'bar' {closed=true, label='BarBarBar'}
  float2 'pos' {disablewhen='{length:Points}==0'}
  menu 'mode' {
    class='Mode', label='Mode',
    items={'a','b','c'}, default='b',
    itemlabels={'Apple','Banana','Coffe'},
    itemvalues={4,8,16} }
  color 'color3' {default={0.8,0.2,0.2,1.0}, disablewhen='{mode}!={menu:mode::a}'}
endgroup 'bar'
list 'Points' {class='Point'}
  float3 "pos" {label="Position"}
  float3 "N" {label="Normal", default={0,1,0}, min=-1, max=1}
endlist 'Points'
)";

static std::string luaapitest_src = R"(
local UIDSLfSet = require('UIDSLfSet')
parms = UIDSLfSet.new()
parms:loadScript([[
  button 'buttttton'
  int 'iiiii' { default=2 }
  text 'sss' { default = 'hi there' }
  struct 'Foo'
    float 'x' { default=3 }
  endstruct 'Foo'
]])
parms:updateInspector()
assert(parms['iiiii']==2, string.format('iiiii==%q', parms['iiiii']))
for i,v in pairs(parms['']) do print(i,v) end
)";

namespace ImGui {

    Hello hello;
    std::unordered_set<std::string> modified;

    static UIDSLfSet &dynaDSL() {
        static UIDSLfSet ps;
        return ps;
    }

    void ShowDSLDemoWindow(bool *opened) {
        hello.sayhi = []() {
            printf("hello!\n");
            hello.name = testscript;
            hello.x = dynaDSL().loadScript(testscript);
        };
        hello.test_lua = []() {
            lua_State *L = luaL_newstate();
            luaL_openlibs(L);
            UIDSLfSet::exposeToLua(L);
            if (LUA_OK != luaL_loadbufferx(L, luaapitest_src.c_str(), luaapitest_src.size(), "test", "t")) {
                hello.name = "Cannot load API test script";
                return;
            }
            if (LUA_OK != lua_pcall(L, 0, 0, 0)) {
                hello.name = "Cannot execute API test script:\n";
                hello.name += luaL_optlstring(L, -1, "uknown", 0);
                return;
            }
            hello.name = "everything OK!";
            lua_close(L);
        };
        if (ImGui::Begin("测试ImGuiDSL", opened)) {
            if (ImGuiInspect(hello, modified)) {
                ImGui::Text("Modified Items:");
                for (auto const &item: modified)
                    ImGui::Text("%s", item.c_str());
            }
        }
        ImGui::End();

        if (ImGui::Begin("Dynamic UIDSL Test", opened)) {
            if (dynaDSL().loaded()) {
                dynaDSL().updateInspector();
                for (auto entry: dynaDSL().dirtyEntries()) {
                    printf("modified: %s\n", entry.c_str());
                    if (auto parm = dynaDSL().get(entry)) {
                        if (parm->is<std::string>())
                            printf("%s = %s\n", entry.c_str(), parm->as<std::string>().c_str());
                        else if (parm->is<int>())
                            printf("%s = %d\n", entry.c_str(), parm->as<int>());
                        else if (parm->is<float>())
                            printf("%s = %f\n", entry.c_str(), parm->as<float>());
                        else if (parm->is<ImGuiDSL::float2>()) {
                            auto p = parm->as<ImGuiDSL::float2>();
                            printf("%s = %f, %f\n", entry.c_str(), p.x, p.y);
                        } else if (parm->is<ImGuiDSL::float3>()) {
                            auto p = parm->as<ImGuiDSL::float3>();
                            printf("%s = %f, %f, %f\n", entry.c_str(), p.x, p.y, p.z);
                        }
                    }
                }
            }
        }
        ImGui::End();
    }

}// namespace ImGui

namespace MetaEngine {

    namespace layout {
        constexpr auto kRenderOptionsPanelWidth = 300;
        constexpr auto kMargin = 5;
        constexpr auto kPanelSpacing = 10;
        constexpr auto kColorWidgetWidth = 250.0F;
    }// namespace layout

    static bool s_is_animaiting = false;
    static const int view_size = 256;


    ImVec2 ImGuiLayer::GetNextWindowsPos(ImGuiWindowTags tag, ImVec2 pos) {

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImVec2 windowspos = ImGui::GetPlatformIO().Platform_GetWindowSize(ImGui::GetMainViewport());
            pos += windowspos;
        }

        return pos;
    }

    void ImGuiLayer::renderViewWindows() {
        //for (int i = m_views.size() - 1; i >= 0; --i)
        //{
        //    auto& view = m_views[i];
        //    if (!view.refreshed)
        //    {
        //        //remove those which have not been submitted this frame

        //        if (view.owner)
        //            delete view.texture;
        //        m_views.erase(m_views.begin() + i);
        //        continue;
        //    }
        //    view.refreshed = false;

        //    if (!view.opened)
        //        continue;

        //    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        //    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        //    //ImGui::SetNextWindowDockID(dock_left_id, ImGuiCond_Once);
        //    if (s_is_animaiting)
        //    {
        //        auto& box = s_boxes[i];
        //        ImGui::SetNextWindowPos(ImVec2(box.srcPos.x, box.srcPos.y), ImGuiCond_Always);
        //        ImGui::SetNextWindowSize(ImVec2(box.scale * view_size, box.scale * view_size), ImGuiCond_Always);
        //    }
        //    else
        //    {
        //        ImGui::SetNextWindowSize({ (float)view_size, (float)view_size }, ImGuiCond_Once);
        //    }
        //    ImGui::Begin(view.name.c_str(), &view.opened, ImGuiWindowFlags_NoDecoration);
        //    ImGui::PopStyleVar(2);
        //    auto size = ImGui::GetWindowSize();

        //    ImGui::Image((void*)view.texture->getID(), size, { 0, 1 }, { 1, 0 });
        //    ImGui::End();
        //}
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

    void ImGuiLayer::Init(SDL_Window *p_window, void *p_gl_context) {
        window = p_window;
        gl_context = p_gl_context;

        IMGUI_CHECKVERSION();

        //ImGui::SetAllocatorFunctions(myMalloc, myFree);

        m_imgui = ImGui::CreateContext();
        ImPlot::CreateContext();

        ImGuiIO &io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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

        const char *glsl_version = "#version 400";
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
            SDL_Window *backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
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

                game->data->draw();

                ImGui::EndTabItem();
            }


            if (ImGui::BeginTabItem(U8("测试"))) {
                ImGui::BeginTabBar(U8("测试#haha"));
                if (ImGui::BeginTabItem(U8("自动序列测试"))) {
                    ImGuiAutoTest();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem(U8("ImGuiDsl测试"))) {
                    static bool ui_imguidsl_test = true;
                    ImGui::ShowDSLDemoWindow(&ui_imguidsl_test);
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
                    DebugUI::Draw(game);
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


        MetaEngine::GameUI_Draw(game);

        if (Settings::ui_inspector) {
            //DrawPropertyWindow();
        }

        if (Settings::ui_gcmanager) {
            METADOT_GC_DISPLAY(0.3f);
        }

        // auto ct = complex_thing{};

        // auto config = EditorConfig{ "object" };
        // config.filter.pattern.reserve(50);
        // config.filter.show_parents = true;

        // auto cconfig = EditorConfig{ "config instance" };
        // cconfig.filter.pattern.reserve(50);
        // config.filter.show_parents = true;

        // ui::widget::object_editor(ct, config);
        // ui::widget::object_editor(config, cconfig);

        renderViewWindows();
    }

    void ImGuiLayer::onUpdate() {
    }

    void ImGuiLayer::registerWindow(std::string_view windowName, bool *opened) {
        for (auto &m_win: m_wins)
            if (m_win.name == windowName)
                return;
        m_wins.push_back({std::string(windowName), opened});
    }

    static std::string bloatString(const std::string &s, int size) {
        std::string out = s;
        out.reserve(size);
        while (out.size() < size)
            out += " ";
        return out;
    }

    static std::string newName;

    //opens popup on previous item and sets newName possibly
    //return true if renamed
    static bool renameName(const char *name) {
        bool rename = false;
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(1)) {
            ImGui::OpenPopup("my_select_popupo");
        }

        if (ImGui::BeginPopup("my_select_popupo", ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration)) {
            char c[128]{};
            memcpy(c, name, strlen(name));
            //ImGui::IsAnyWindowFocused()
            //if (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
            //todo imdoc
            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow) && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
                ImGui::SetKeyboardFocusHere();
            if (ImGui::InputText("##heheheehj", c, 127, ImGuiInputTextFlags_EnterReturnsTrue)) {
                //ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
                newName = std::string(c);
                ImGui::CloseCurrentPopup();
                rename = true;
            }
            //else
            ImGui::SetItemDefaultFocus();
            ImGui::EndPopup();
        }
        return rename;
    }
}// namespace MetaEngine
