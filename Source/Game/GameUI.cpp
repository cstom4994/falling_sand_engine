// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Game/GameUI.hpp"
#include "Core/Global.hpp"
#include "Engine/Memory.hpp"
#include "Game/Game.hpp"
#include "Game/GameDataStruct.hpp"
#include "Game/ImGuiCore.hpp"
#include "Game/Item.hpp"
#include "Game/Player.hpp"

#include "Game/WorldGenerator.cpp"

#include <string>

void GameUI::GameUI_Draw(Game *game) {
    DebugDrawUI::Draw(game);
    DebugCheatsUI::Draw(game);
    MainMenuUI::Draw(game);
    IngameUI::Draw(game);
}

namespace GameUI {

    int IngameUI::state = 0;

    bool IngameUI::visible = false;
    bool IngameUI::setup = false;

    void IngameUI::Setup() {}

    void IngameUI::Draw(Game *game) {

        if (!visible) return;

        if (state == 0) {
            DrawIngame(game);
        } else if (state == 1) {
            DrawOptions(game);
        }
    }

    void IngameUI::DrawIngame(Game *game) {

        if (!setup) { Setup(); }

        ImGui::SetNextWindowSize(ImVec2(400, 300));
        ImGui::SetNextWindowPos(
                global.ImGuiCore->GetNextWindowsPos(
                        ImGuiWindowTags::UI_MainMenu,
                        ImVec2(global.platform.WIDTH / 2 - 200, global.platform.HEIGHT / 2 - 250)),
                ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Pause Menu", NULL,
                          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
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

        ImGui::SetCursorPos(
                ImVec2(200 - mainMenuButtonsWidth / 2, 25 + mainMenuButtonsYOffset * 1));
        selPos = ImGui::GetCursorPos();
        //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        if (ImGui::Button("##continue", ImVec2(mainMenuButtonsWidth, 36))) { visible = false; }
        ImGui::PopStyleVar();
        ImGui::SetCursorPos(ImVec2(
                selPos.x + mainMenuButtonsWidth / 2 - ImGui::CalcTextSize("继续").x / 2, selPos.y));
        ImGui::Text("继续");
        //ImGui::PopFont();

        ImGui::SetCursorPos(
                ImVec2(200 - mainMenuButtonsWidth / 2, 25 + mainMenuButtonsYOffset * 2));
        selPos = ImGui::GetCursorPos();
        //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        if (ImGui::Button("##options", ImVec2(mainMenuButtonsWidth, 36))) { state = 1; }
        ImGui::PopStyleVar();
        ImGui::SetCursorPos(ImVec2(
                selPos.x + mainMenuButtonsWidth / 2 - ImGui::CalcTextSize("选项").x / 2, selPos.y));
        ImGui::Text("选项");
        //ImGui::PopFont();

        ImGui::SetCursorPos(
                ImVec2(200 - mainMenuButtonsWidth / 2, 25 + mainMenuButtonsYOffset * 4));
        selPos = ImGui::GetCursorPos();
        //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        if (ImGui::Button("##quit", ImVec2(mainMenuButtonsWidth, 36))) {
            visible = false;
            game->quitToMainMenu();
        }
        ImGui::PopStyleVar();
        ImGui::SetCursorPos(ImVec2(selPos.x + mainMenuButtonsWidth / 2 -
                                           ImGui::CalcTextSize("离开到主菜单").x / 2,
                                   selPos.y));
        ImGui::Text("离开到主菜单");
        //ImGui::PopFont();

        ImGui::End();
    }

    void IngameUI::DrawOptions(Game *game) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.11f, 0.11f, 0.9f));
        ImGui::SetNextWindowSize(ImVec2(400, 400));
        ImGui::SetNextWindowPos(
                global.ImGuiCore->GetNextWindowsPos(
                        ImGuiWindowTags::UI_MainMenu,
                        ImVec2(global.platform.WIDTH / 2 - 200, global.platform.HEIGHT / 2 - 250)),
                ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Pause Menu", NULL,
                          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
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
            global.audioEngine.PlayEvent("event:/GUI/GUI_Tab");
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
        ImGui::SetCursorPos(
                ImVec2(selPos.x + 150 / 2 - ImGui::CalcTextSize("返回").x / 2, selPos.y));
        ImGui::Text("返回");
        //ImGui::PopFont();
    }

    void OptionsUI::DrawGeneral(Game *game) {
        ImGui::TextColored(ImVec4(1.0, 1.0, 0.8, 1.0), "%s", "游戏内容");
        ImGui::Indent(4);

        ImGui::Checkbox("材质工具提示", &global.game->GameIsolate_.settings.draw_material_info);

        ImGui::Unindent(4);
    }

    void OptionsUI::DrawVideo(Game *game) {
        ImGui::TextColored(ImVec4(1.0, 1.0, 0.8, 1.0), "%s", "窗口");
        ImGui::Indent(4);

        const char *items[] = {"Windowed", "Fullscreen Borderless", "Fullscreen"};
        const char *combo_label = items
                [item_current_idx];// Label to preview before opening the combo (technically it could be anything)
        ImGui::SetNextItemWidth(190);
        if (ImGui::BeginCombo("Display Mode", combo_label, 0)) {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
                const bool is_selected = (item_current_idx == n);
                if (ImGui::Selectable(items[n], is_selected)) {

                    switch (n) {
                        case 0:
                            global.platform.SetDisplayMode(DisplayMode::WINDOWED);
                            break;
                        case 1:
                            global.platform.SetDisplayMode(DisplayMode::BORDERLESS);
                            break;
                        case 2:
                            global.platform.SetDisplayMode(DisplayMode::FULLSCREEN);
                            break;
                    }

                    item_current_idx = n;
                }

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected) { ImGui::SetItemDefaultFocus(); }
            }
            ImGui::EndCombo();
        }

        if (ImGui::Checkbox("VSync", &vsync)) { global.platform.SetVSync(vsync); }

        if (ImGui::Checkbox("失去焦点后最小化", &minimizeOnFocus)) {
            global.platform.SetMinimizeOnLostFocus(minimizeOnFocus);
        }

        ImGui::Unindent(4);
        ImGui::Separator();

        ImGui::TextColored(ImVec4(1.0, 1.0, 0.8, 1.0), "%s", "Rendering");
        ImGui::Indent(4);

        if (ImGui::Checkbox("高清贴图", &global.game->GameIsolate_.settings.hd_objects)) {
            METAENGINE_Render_FreeTarget(game->TexturePack_.textureObjects->target);
            METAENGINE_Render_FreeImage(game->TexturePack_.textureObjects);
            METAENGINE_Render_FreeTarget(game->TexturePack_.textureObjectsBack->target);
            METAENGINE_Render_FreeImage(game->TexturePack_.textureObjectsBack);
            METAENGINE_Render_FreeTarget(game->TexturePack_.textureEntities->target);
            METAENGINE_Render_FreeImage(game->TexturePack_.textureEntities);

            game->TexturePack_.textureObjects = METAENGINE_Render_CreateImage(
                    game->GameIsolate_.world->width *
                            (global.game->GameIsolate_.settings.hd_objects
                                     ? global.game->GameIsolate_.settings.hd_objects_size
                                     : 1),
                    game->GameIsolate_.world->height *
                            (global.game->GameIsolate_.settings.hd_objects
                                     ? global.game->GameIsolate_.settings.hd_objects_size
                                     : 1),
                    METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
            METAENGINE_Render_SetImageFilter(game->TexturePack_.textureObjects,
                                             METAENGINE_Render_FILTER_NEAREST);

            game->TexturePack_.textureObjectsBack = METAENGINE_Render_CreateImage(
                    game->GameIsolate_.world->width *
                            (global.game->GameIsolate_.settings.hd_objects
                                     ? global.game->GameIsolate_.settings.hd_objects_size
                                     : 1),
                    game->GameIsolate_.world->height *
                            (global.game->GameIsolate_.settings.hd_objects
                                     ? global.game->GameIsolate_.settings.hd_objects_size
                                     : 1),
                    METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
            METAENGINE_Render_SetImageFilter(game->TexturePack_.textureObjectsBack,
                                             METAENGINE_Render_FILTER_NEAREST);

            METAENGINE_Render_LoadTarget(game->TexturePack_.textureObjects);
            METAENGINE_Render_LoadTarget(game->TexturePack_.textureObjectsBack);

            game->TexturePack_.textureEntities = METAENGINE_Render_CreateImage(
                    game->GameIsolate_.world->width *
                            (global.game->GameIsolate_.settings.hd_objects
                                     ? global.game->GameIsolate_.settings.hd_objects_size
                                     : 1),
                    game->GameIsolate_.world->height *
                            (global.game->GameIsolate_.settings.hd_objects
                                     ? global.game->GameIsolate_.settings.hd_objects_size
                                     : 1),
                    METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
            METAENGINE_Render_SetImageFilter(game->TexturePack_.textureEntities,
                                             METAENGINE_Render_FILTER_NEAREST);

            METAENGINE_Render_LoadTarget(game->TexturePack_.textureEntities);
        }

        ImGui::SetNextItemWidth(100);
        ImGui::SliderFloat("光照质量", &global.game->GameIsolate_.settings.lightingQuality, 0.0,
                           1.0, "", 0);
        ImGui::Checkbox("简单光照", &global.game->GameIsolate_.settings.simpleLighting);
        ImGui::Checkbox("抖动光照", &global.game->GameIsolate_.settings.lightingDithering);

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

        std::vector<std::vector<std::string>> disp = {{"bus:/Master", "Master"},
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

    void OptionsUI::DrawInput(Game *game) {}

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

        for (auto &p: std::filesystem::directory_iterator(global.GameDir.getPath("worlds/"))) {
            std::string worldName = p.path().filename().generic_string();

            if (worldName == ".DS_Store") continue;

            WorldMeta meta = WorldMeta::loadWorldMeta(
                    (char *) global.GameDir.getWorldPath(worldName).c_str());

            worlds.push_back(std::make_tuple(worldName, meta));
        }

        sort(worlds.begin(), worlds.end(), sortWorlds);
    }

    void MainMenuUI::Setup() {

        C_Surface *logoSfc = Textures::LoadTexture("data/assets/ui/logo.png");
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

        if (!setup) { Setup(); }
        // long long now = Time::millis();
        // if (now - lastRefresh > 30000) {
        //     RefreshWorlds(game);
        //     lastRefresh = now;
        // }

        ImGui::SetNextWindowSize(ImVec2(400, 350));
        ImGui::SetNextWindowPos(
                global.ImGuiCore->GetNextWindowsPos(ImGuiWindowTags::UI_MainMenu,
                                                    ImVec2(global.platform.WIDTH / 2 - 400 / 2,
                                                           global.platform.HEIGHT / 2 - 350 / 2)),
                ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Main Menu", NULL,
                          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            ImGui::End();
            return;
        }
        pos = ImGui::GetWindowPos();

        ImTextureID texId = (ImTextureID) METAENGINE_Render_GetTextureHandle(title);

        ImVec2 uv_min = ImVec2(0.0f, 0.0f);              // Top-left
        ImVec2 uv_max = ImVec2(1.0f, 1.0f);              // Lower-right
        ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);// No tint
        ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);

        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x - title->w / 2) * 0.5f,
                                   ImGui::GetCursorPosY() + 10));
        ImGui::Image(texId, ImVec2(title->w / 2, title->h / 2), uv_min, uv_max, tint_col,
                     border_col);
        ImGui::TextColored(ImVec4(211.0f, 211.0f, 211.0f, 255.0f), U8("大摆钟送快递"));

        int mainMenuButtonsWidth = 250;
        int mainMenuButtonsYOffset = 55;
        ImGui::SetCursorPos(ImVec2(200 - mainMenuButtonsWidth / 2, 60 + mainMenuButtonsYOffset));
        ImVec2 selPos = ImGui::GetCursorPos();
        //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        if (ImGui::Button("##singleplayer", ImVec2(mainMenuButtonsWidth, 36))) { state = 2; }
        ImGui::PopStyleVar();
        ImGui::SetCursorPos(
                ImVec2(selPos.x + mainMenuButtonsWidth / 2 - ImGui::CalcTextSize("单人游戏").x / 2,
                       selPos.y));
        ImGui::Text(U8("单人游戏"));
        //ImGui::PopFont();

        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);

        ImGui::SetCursorPos(
                ImVec2(200 - mainMenuButtonsWidth / 2, 60 + mainMenuButtonsYOffset * 2));
        selPos = ImGui::GetCursorPos();
        //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        if (ImGui::Button("##multiplayer", ImVec2(mainMenuButtonsWidth, 36))) { state = 2; }
        ImGui::PopStyleVar();
        ImGui::SetCursorPos(
                ImVec2(selPos.x + mainMenuButtonsWidth / 2 - ImGui::CalcTextSize("多人游戏").x / 2,
                       selPos.y));
        ImGui::Text("多人游戏");
        //ImGui::PopFont();

        ImGui::PopItemFlag();
        ImGui::PopStyleVar();

        ImGui::SetCursorPos(
                ImVec2(200 - mainMenuButtonsWidth / 2, 60 + mainMenuButtonsYOffset * 3));
        selPos = ImGui::GetCursorPos();
        //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        if (ImGui::Button("##options", ImVec2(mainMenuButtonsWidth, 36))) { state = 4; }
        ImGui::PopStyleVar();
        ImGui::SetCursorPos(ImVec2(
                selPos.x + mainMenuButtonsWidth / 2 - ImGui::CalcTextSize("选项").x / 2, selPos.y));
        ImGui::Text("选项");
        //ImGui::PopFont();

        ImGui::SetCursorPos(
                ImVec2(200 - mainMenuButtonsWidth / 2, 60 + mainMenuButtonsYOffset * 4));
        selPos = ImGui::GetCursorPos();
        //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        if (ImGui::Button("##quit", ImVec2(mainMenuButtonsWidth, 36))) { game->running = false; }
        ImGui::PopStyleVar();
        ImGui::SetCursorPos(ImVec2(
                selPos.x + mainMenuButtonsWidth / 2 - ImGui::CalcTextSize("退出").x / 2, selPos.y));
        ImGui::Text("退出");
        //ImGui::PopFont();

        ImGui::End();
    }

    void MainMenuUI::DrawSingleplayer(Game *game) {
        long long now = Time::millis();
        if (now - lastRefresh > 30000) {
            RefreshWorlds(game);
            lastRefresh = now;
        }
        if (!visible) return;

        ImGui::SetNextWindowSize(ImVec2(400, 425));
        ImGui::SetNextWindowPos(
                global.ImGuiCore->GetNextWindowsPos(
                        ImGuiWindowTags::UI_MainMenu,
                        ImVec2(global.platform.WIDTH / 2 - 200, global.platform.HEIGHT / 2 - 250)),
                ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Main Menu", NULL,
                          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
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
        ImGui::SetCursorPos(
                ImVec2(selPos.x + mainMenuNewButtonWidth / 2 - ImGui::CalcTextSize("新世界").x / 2,
                       selPos.y));
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

            ImGui::SetCursorPos(
                    ImVec2(200 - 8 - mainMenuButtonsWidth / 2, 4 + (nMainMenuButtons++ * 60)));
            selPos = ImGui::GetCursorScreenPos();
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            if (ImGui::Button("", ImVec2(mainMenuButtonsWidth, 50))) {
                METADOT_INFO("Selected world: {}", worldName.c_str());
                visible = false;

                game->fadeOutStart = game->GameIsolate_.game_timestate.now;
                game->fadeOutLength = 250;
                game->fadeOutCallback = [&, game, worldName]() {
                    game->setGameState(LOADING, INGAME);

                    METADOT_DELETE(C, game->GameIsolate_.world, World);
                    game->GameIsolate_.world = nullptr;

                    //std::thread loadWorldThread([&] () {

                    World *w = nullptr;
                    METADOT_NEW(C, w, World);
                    w->init(global.GameDir.getWorldPath(worldName),
                            (int) ceil(WINDOWS_MAX_WIDTH / 3 / (double) CHUNK_W) * CHUNK_W +
                                    CHUNK_W * 3,
                            (int) ceil(WINDOWS_MAX_HEIGHT / 3 / (double) CHUNK_H) * CHUNK_H +
                                    CHUNK_H * 3,
                            game->RenderTarget_.target, &global.audioEngine,
                            global.game->GameIsolate_.settings.networkMode);
                    w->metadata.lastOpenedTime = Time::millis() / 1000;
                    w->metadata.lastOpenedVersion = std::to_string(MetaDot_buildnum());
                    w->metadata.save(w->worldName);

                    METADOT_INFO("Queueing chunk loading...");
                    for (int x = -CHUNK_W * 4; x < w->width + CHUNK_W * 4; x += CHUNK_W) {
                        for (int y = -CHUNK_H * 3; y < w->height + CHUNK_H * 8; y += CHUNK_H) {
                            w->queueLoadChunk(x / CHUNK_W, y / CHUNK_H, true, true);
                        }
                    }

                    game->GameIsolate_.world = w;
                    //});

                    game->fadeInStart = game->GameIsolate_.game_timestate.now;
                    game->fadeInLength = 250;
                    game->fadeInWaitFrames = 4;
                };
            }
            ImGui::PopStyleVar();

            ImVec2 prevPos = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(selPos.x, selPos.y));

            // time_t t_times;
            // tm *tm_utc = gmtime(&t_times);
            // meta.lastOpenedTime = t_times;

            // // convert to local time
            // time_t time_utc = Time::mkgmtime(tm_utc);
            // time_t time_local = mktime(tm_utc);
            // time_local += time_utc - time_local;
            // tm *tm_local = localtime(&time_local);

            // char *formattedTime = new char[100];
            // strftime(formattedTime, 100, "%#m/%#d/%y %#I:%M%p", tm_local);

            char *filenameAndTimestamp = new char[200];
            snprintf(filenameAndTimestamp, 100, "%s (%s)", worldName.c_str(), "formattedTime");

            //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
            ImGui::SetCursorScreenPos(
                    ImVec2(selPos.x + mainMenuButtonsWidth / 2 -
                                   ImGui::CalcTextSize(meta.worldName.c_str()).x / 2,
                           selPos.y));
            ImGui::Text("%s", meta.worldName.c_str());
            //ImGui::PopFont();

            ImGui::SetCursorScreenPos(
                    ImVec2(selPos.x + mainMenuButtonsWidth / 2 -
                                   ImGui::CalcTextSize(filenameAndTimestamp).x / 2,
                           selPos.y + 32));
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
        if (ImGui::Button("##back", ImVec2(150, 36))) { state = 0; }
        ImGui::PopStyleVar();
        ImGui::SetCursorPos(
                ImVec2(selPos.x + 150 / 2 - ImGui::CalcTextSize("返回").x / 2, selPos.y));
        ImGui::Text("返回");
        //ImGui::PopFont();

        ImGui::End();
    }

    void MainMenuUI::DrawMultiplayer(Game *game) {
        ImGui::SetNextWindowSize(ImVec2(400, 500));
        ImGui::SetNextWindowPos(
                global.ImGuiCore->GetNextWindowsPos(
                        ImGuiWindowTags::UI_MainMenu,
                        ImVec2(global.platform.WIDTH / 2 - 200, global.platform.HEIGHT / 2 - 250)),
                ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Main Menu", NULL,
                          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
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
            if (global.client->connect(global.game->GameIsolate_.settings.server_ip.c_str(),
                                       global.game->GameIsolate_.settings.server_port)) {
                global.game->GameIsolate_.settings.networkMode = NetworkMode::CLIENT;
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
        ImGui::SetNextWindowPos(
                global.ImGuiCore->GetNextWindowsPos(
                        ImGuiWindowTags::UI_MainMenu,
                        ImVec2(global.platform.WIDTH / 2 - 200, global.platform.HEIGHT / 2 - 250)),
                ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Main Menu", NULL,
                          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            ImGui::End();
            return;
        }

        CreateWorldUI::Draw(game);

        ImGui::End();
    }

    void MainMenuUI::DrawOptions(Game *game) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.11f, 0.11f, 0.9f));
        ImGui::SetNextWindowSize(ImVec2(400, 400));
        ImGui::SetNextWindowPos(
                global.ImGuiCore->GetNextWindowsPos(
                        ImGuiWindowTags::UI_MainMenu,
                        ImVec2(global.platform.WIDTH / 2 - 200, global.platform.HEIGHT / 2 - 250)),
                ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Main Menu", NULL,
                          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
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

            ImGui::Checkbox(U8("绘制帧率图"), &global.game->GameIsolate_.settings.draw_frame_graph);
            ImGui::Checkbox(U8("绘制调试状态"),
                            &global.game->GameIsolate_.settings.draw_debug_stats);

            ImGui::Checkbox(U8("绘制区域块状态"),
                            &global.game->GameIsolate_.settings.draw_chunk_state);
            ImGui::Checkbox(U8("绘制加载区域"),
                            &global.game->GameIsolate_.settings.draw_load_zones);

            ImGui::Checkbox(U8("材料工具"), &global.game->GameIsolate_.settings.draw_material_info);
            ImGui::Indent(10.0f);
            ImGui::Checkbox(U8("调试材料"),
                            &global.game->GameIsolate_.settings.draw_detailed_material_info);
            ImGui::Unindent(10.0f);

            if (ImGui::TreeNode(U8("物理"))) {
                ImGui::Checkbox(U8("绘制物理调试"),
                                &global.game->GameIsolate_.settings.draw_physics_debug);

                if (ImGui::TreeNode("Box2D")) {
                    ImGui::Checkbox("shape", &global.game->GameIsolate_.settings.draw_b2d_shape);
                    ImGui::Checkbox("joint", &global.game->GameIsolate_.settings.draw_b2d_joint);
                    ImGui::Checkbox("aabb", &global.game->GameIsolate_.settings.draw_b2d_aabb);
                    ImGui::Checkbox("pair", &global.game->GameIsolate_.settings.draw_b2d_pair);
                    ImGui::Checkbox("center of mass",
                                    &global.game->GameIsolate_.settings.draw_b2d_centerMass);

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            if (ImGui::TreeNode(U8("GLSL方法"))) {
                if (ImGui::Button(U8("重新加载GLSL"))) { global.shaderworker.LoadShaders(); }
                ImGui::Checkbox(U8("绘制GLSL"), &global.game->GameIsolate_.settings.draw_shaders);

                if (ImGui::TreeNode(U8("光照"))) {
                    ImGui::SetNextItemWidth(80);
                    ImGui::SliderFloat(U8("质量"),
                                       &global.game->GameIsolate_.settings.lightingQuality, 0.0,
                                       1.0, "", 0);
                    ImGui::Checkbox(U8("覆盖"),
                                    &global.game->GameIsolate_.settings.draw_light_overlay);
                    ImGui::Checkbox(U8("简单采样"),
                                    &global.game->GameIsolate_.settings.simpleLighting);
                    ImGui::Checkbox(U8("放射"),
                                    &global.game->GameIsolate_.settings.lightingEmission);
                    ImGui::Checkbox(U8("抖动"),
                                    &global.game->GameIsolate_.settings.lightingDithering);

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode(U8("水体"))) {
                    const char *items[] = {"off", "flow map", "distortion"};
                    const char *combo_label =
                            items[global.game->GameIsolate_.settings.water_overlay];
                    ImGui::SetNextItemWidth(80 + 24);
                    if (ImGui::BeginCombo("Overlay", combo_label, 0)) {
                        for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
                            const bool is_selected =
                                    (global.game->GameIsolate_.settings.water_overlay == n);
                            if (ImGui::Selectable(items[n], is_selected)) {
                                global.game->GameIsolate_.settings.water_overlay = n;
                            }

                            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                            if (is_selected) { ImGui::SetItemDefaultFocus(); }
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::Checkbox(U8("显示流程"),
                                    &global.game->GameIsolate_.settings.water_showFlow);
                    ImGui::Checkbox(U8("像素化"),
                                    &global.game->GameIsolate_.settings.water_pixelated);

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            if (ImGui::TreeNode(U8("世界"))) {

                ImGui::Checkbox("Draw Temperature Map",
                                &global.game->GameIsolate_.settings.draw_temperature_map);

                if (ImGui::Checkbox("Draw Background",
                                    &global.game->GameIsolate_.settings.draw_background)) {
                    for (int x = 0; x < game->GameIsolate_.world->width; x++) {
                        for (int y = 0; y < game->GameIsolate_.world->height; y++) {
                            game->GameIsolate_.world
                                    ->dirty[x + y * game->GameIsolate_.world->width] = true;
                            game->GameIsolate_.world
                                    ->layer2Dirty[x + y * game->GameIsolate_.world->width] = true;
                        }
                    }
                }

                if (ImGui::Checkbox("Draw Background Grid",
                                    &global.game->GameIsolate_.settings.draw_background_grid)) {
                    for (int x = 0; x < game->GameIsolate_.world->width; x++) {
                        for (int y = 0; y < game->GameIsolate_.world->height; y++) {
                            game->GameIsolate_.world
                                    ->dirty[x + y * game->GameIsolate_.world->width] = true;
                            game->GameIsolate_.world
                                    ->layer2Dirty[x + y * game->GameIsolate_.world->width] = true;
                        }
                    }
                }

                if (ImGui::Checkbox("HD Objects", &global.game->GameIsolate_.settings.hd_objects)) {
                    METAENGINE_Render_FreeTarget(game->TexturePack_.textureObjects->target);
                    METAENGINE_Render_FreeImage(game->TexturePack_.textureObjects);
                    METAENGINE_Render_FreeTarget(game->TexturePack_.textureObjectsBack->target);
                    METAENGINE_Render_FreeImage(game->TexturePack_.textureObjectsBack);
                    METAENGINE_Render_FreeTarget(game->TexturePack_.textureEntities->target);
                    METAENGINE_Render_FreeImage(game->TexturePack_.textureEntities);

                    game->TexturePack_.textureObjects = METAENGINE_Render_CreateImage(
                            game->GameIsolate_.world->width *
                                    (global.game->GameIsolate_.settings.hd_objects
                                             ? global.game->GameIsolate_.settings.hd_objects_size
                                             : 1),
                            game->GameIsolate_.world->height *
                                    (global.game->GameIsolate_.settings.hd_objects
                                             ? global.game->GameIsolate_.settings.hd_objects_size
                                             : 1),
                            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                    METAENGINE_Render_SetImageFilter(game->TexturePack_.textureObjects,
                                                     METAENGINE_Render_FILTER_NEAREST);

                    game->TexturePack_.textureObjectsBack = METAENGINE_Render_CreateImage(
                            game->GameIsolate_.world->width *
                                    (global.game->GameIsolate_.settings.hd_objects
                                             ? global.game->GameIsolate_.settings.hd_objects_size
                                             : 1),
                            game->GameIsolate_.world->height *
                                    (global.game->GameIsolate_.settings.hd_objects
                                             ? global.game->GameIsolate_.settings.hd_objects_size
                                             : 1),
                            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                    METAENGINE_Render_SetImageFilter(game->TexturePack_.textureObjectsBack,
                                                     METAENGINE_Render_FILTER_NEAREST);

                    METAENGINE_Render_LoadTarget(game->TexturePack_.textureObjects);
                    METAENGINE_Render_LoadTarget(game->TexturePack_.textureObjectsBack);

                    game->TexturePack_.textureEntities = METAENGINE_Render_CreateImage(
                            game->GameIsolate_.world->width *
                                    (global.game->GameIsolate_.settings.hd_objects
                                             ? global.game->GameIsolate_.settings.hd_objects_size
                                             : 1),
                            game->GameIsolate_.world->height *
                                    (global.game->GameIsolate_.settings.hd_objects
                                             ? global.game->GameIsolate_.settings.hd_objects_size
                                             : 1),
                            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                    METAENGINE_Render_SetImageFilter(game->TexturePack_.textureEntities,
                                                     METAENGINE_Render_FILTER_NEAREST);

                    METAENGINE_Render_LoadTarget(game->TexturePack_.textureEntities);
                }

                ImGui::TreePop();
            }

            ImGui::TreePop();
        }

        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode(U8("模拟"))) {
            ImGui::Checkbox(U8("处理世界"), &global.game->GameIsolate_.settings.tick_world);
            ImGui::Checkbox(U8("处理Box2D"), &global.game->GameIsolate_.settings.tick_box2d);
            ImGui::Checkbox(U8("处理温度"), &global.game->GameIsolate_.settings.tick_temperature);

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
            C_Surface *surface =
                    SDL_CreateRGBSurfaceWithFormat(0, 16, 16, 32, SDL_PIXELFORMAT_ARGB8888);
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

            if (x > 0) ImGui::SameLine();
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
        C_Surface *sfc = Textures::LoadTexture("data/assets/objects/testPickaxe.png");
        images.push_back(METAENGINE_Render_CopyImageFromSurface(sfc));
        METAENGINE_Render_SetImageFilter(images[0], METAENGINE_Render_FILTER_NEAREST);
        SDL_FreeSurface(sfc);
        sfc = Textures::LoadTexture("data/assets/objects/testHammer.png");
        images.push_back(METAENGINE_Render_CopyImageFromSurface(sfc));
        METAENGINE_Render_SetImageFilter(images[1], METAENGINE_Render_FILTER_NEAREST);
        SDL_FreeSurface(sfc);
        sfc = Textures::LoadTexture("data/assets/objects/testVacuum.png");
        images.push_back(METAENGINE_Render_CopyImageFromSurface(sfc));
        METAENGINE_Render_SetImageFilter(images[2], METAENGINE_Render_FILTER_NEAREST);
        SDL_FreeSurface(sfc);
        sfc = Textures::LoadTexture("data/assets/objects/testBucket.png");
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
            if (game->GameIsolate_.world == nullptr ||
                game->GameIsolate_.world->WorldIsolate_.player == nullptr) {
                ImGui::Text("世界中没有玩家");
            } else {
                int i = 0;
                ImGui::PushID(i);
                int frame_padding = 4;          // -1 == uses default padding (style.FramePadding)
                ImVec2 size = ImVec2(48, 48);   // Size of the image we want to make visible
                ImVec2 uv0 = ImVec2(0.0f, 0.0f);// UV coordinates for lower-left
                ImVec2 uv1 = ImVec2(1.0f, 1.0f);// UV coordinates for (32,32) in our texture
                ImVec4 bg_col = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);  // Black background
                ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);// No tint

                ImTextureID texId = (ImTextureID) METAENGINE_Render_GetTextureHandle(images[i]);
                if (ImGui::ImageButton(texId, size, uv0, uv1, frame_padding, bg_col, tint_col)) {
                    Item *i3 = new Item();
                    i3->setFlag(ItemFlags::TOOL);
                    i3->surface = Textures::LoadTexture("data/assets/objects/testPickaxe.png");
                    i3->texture = METAENGINE_Render_CopyImageFromSurface(i3->surface);
                    METAENGINE_Render_SetImageFilter(i3->texture, METAENGINE_Render_FILTER_NEAREST);
                    i3->pivotX = 2;
                    game->GameIsolate_.world->WorldIsolate_.player->setItemInHand(
                            i3, game->GameIsolate_.world);
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
                    i3->surface = Textures::LoadTexture("data/assets/objects/testHammer.png");
                    i3->texture = METAENGINE_Render_CopyImageFromSurface(i3->surface);
                    METAENGINE_Render_SetImageFilter(i3->texture, METAENGINE_Render_FILTER_NEAREST);
                    i3->pivotX = 2;
                    game->GameIsolate_.world->WorldIsolate_.player->setItemInHand(
                            i3, game->GameIsolate_.world);
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
                    i3->surface = Textures::LoadTexture("data/assets/objects/testVacuum.png");
                    i3->texture = METAENGINE_Render_CopyImageFromSurface(i3->surface);
                    METAENGINE_Render_SetImageFilter(i3->texture, METAENGINE_Render_FILTER_NEAREST);
                    i3->pivotX = 6;
                    game->GameIsolate_.world->WorldIsolate_.player->setItemInHand(
                            i3, game->GameIsolate_.world);
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
                    i3->surface = Textures::LoadTexture("data/assets/objects/testBucket.png");
                    i3->capacity = 100;
                    i3->loadFillTexture(
                            Textures::LoadTexture("data/assets/objects/testBucket_fill.png"));
                    i3->texture = METAENGINE_Render_CopyImageFromSurface(i3->surface);
                    METAENGINE_Render_SetImageFilter(i3->texture, METAENGINE_Render_FILTER_NEAREST);
                    i3->pivotX = 0;
                    game->GameIsolate_.world->WorldIsolate_.player->setItemInHand(
                            i3, game->GameIsolate_.world);
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

        C_Surface *logoMT = Textures::LoadTexture("data/assets/ui/prev_materialtest.png");
        materialTestWorld = METAENGINE_Render_CopyImageFromSurface(logoMT);
        METAENGINE_Render_SetImageFilter(materialTestWorld, METAENGINE_Render_FILTER_NEAREST);
        SDL_FreeSurface(logoMT);

        C_Surface *logoDef = Textures::LoadTexture("data/assets/ui/prev_default.png");
        defaultWorld = METAENGINE_Render_CopyImageFromSurface(logoDef);
        METAENGINE_Render_SetImageFilter(defaultWorld, METAENGINE_Render_FILTER_NEAREST);
        SDL_FreeSurface(logoDef);

        setup = true;
    }

    void CreateWorldUI::Draw(Game *game) {

        if (!setup) Setup();

        int createWorldWidth = 350;

        //ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 -
                             ImGui::CalcTextSize("Create World").x / 2);
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
        if (ImGui::Selectable("", selIndex == 0, 0, ImVec2(111 - 4, 111))) { selIndex = 0; }

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
        if (ImGui::Selectable("", selIndex == 1, 0, ImVec2(111 - 4, 111))) { selIndex = 1; }

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
        if (ImGui::Button("##back", ImVec2(150, 36))) { MainMenuUI::state = 2; }
        ImGui::PopStyleVar();
        ImGui::SetCursorPos(
                ImVec2(selPos.x + 150 / 2 - ImGui::CalcTextSize("返回").x / 2, selPos.y));
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

            METADOT_INFO("Creating world named \"{}\" at \"{}\"", worldTitle,
                         global.GameDir.getWorldPath(wn));
            MainMenuUI::visible = false;
            game->setGameState(LOADING, INGAME);

            METADOT_DELETE(C, game->GameIsolate_.world, World);
            game->GameIsolate_.world = nullptr;

            WorldGenerator *generator;

            if (selIndex == 0) {
                generator = new MaterialTestGenerator();
            } else if (selIndex == 1) {
                generator = new DefaultGenerator();
            } else {
                // create world UI is in invalid state
                generator = new MaterialTestGenerator();
            }

            std::string wpStr = global.GameDir.getWorldPath(wn);

            METADOT_NEW(C, game->GameIsolate_.world, World);
            game->GameIsolate_.world->init(
                    wpStr,
                    (int) ceil(WINDOWS_MAX_WIDTH / 3 / (double) CHUNK_W) * CHUNK_W + CHUNK_W * 3,
                    (int) ceil(WINDOWS_MAX_HEIGHT / 3 / (double) CHUNK_H) * CHUNK_H + CHUNK_H * 3,
                    game->RenderTarget_.target, &global.audioEngine,
                    global.game->GameIsolate_.settings.networkMode, generator);
            game->GameIsolate_.world->metadata.worldName = std::string(worldNameBuf);
            game->GameIsolate_.world->metadata.lastOpenedTime = Time::millis() / 1000;
            game->GameIsolate_.world->metadata.lastOpenedVersion =
                    std::to_string(MetaDot_buildnum());
            game->GameIsolate_.world->metadata.save(wpStr);

            METADOT_INFO("Queueing chunk loading...");
            for (int x = -CHUNK_W * 4; x < game->GameIsolate_.world->width + CHUNK_W * 4;
                 x += CHUNK_W) {
                for (int y = -CHUNK_H * 3; y < game->GameIsolate_.world->height + CHUNK_H * 8;
                     y += CHUNK_H) {
                    game->GameIsolate_.world->queueLoadChunk(x / CHUNK_W, y / CHUNK_H, true, true);
                }
            }
        }
        ImGui::PopStyleVar();
        ImGui::SetCursorPos(
                ImVec2(selPos.x + 150 / 2 - ImGui::CalcTextSize("Create").x / 2, selPos.y));
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
        std::string folder = global.GameDir.getWorldPath(worldFolderName);
        struct stat buffer;
        bool exists = (stat(folder.c_str(), &buffer) == 0);

        std::string newWorldFolderName = worldFolderName;
        int i = 2;
        while (exists) {
            newWorldFolderName = worldFolderName + " (" + std::to_string(i) + ")";
            folder = global.GameDir.getWorldPath(newWorldFolderName);

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

}// namespace GameUI
