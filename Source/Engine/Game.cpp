// Copyright(c) 2022, KaoruXun All rights reserved.


#include "Game.hpp"
#include <iterator>
#include <regex>

#include "DebugImpl.hpp"
#include "DefaultGenerator.cpp"
#include "GCManager.hpp"
#include "MaterialTestGenerator.cpp"

#include "Settings.hpp"
#include "Utils.hpp"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl.h"
#include "imgui.h"

#include "CoreCLREmbed/CoreCLREmbed.hpp"
#include "Engine/Core.hpp"
#include "Engine/GCManager.hpp"
#include "Engine/ImGuiBase.h"
#include "Engine/Macros.hpp"
#include "Engine/ModuleStack.h"
#include "Engine/Scripting/LuaLayer.hpp"
#include "Engine/Scripting/Scripting.hpp"


#include "Engine/FileSystem.hpp"
#include "Engine/lib/final_dynamic_opengl.h"
#include "Engine/lib/structopt.hpp"

#include <imgui/IconsFontAwesome5.h>
#include <string.h>

#include "InEngine.h"

#define CR_HOST CR_UNSAFE
#include "Engine/lib/cr.h"

#ifdef _WIN32
#include <SDL_syswm.h>
#include <shobjidl.h>
#endif

const char *logo = R"(
      __  __      _        _____        _   
     |  \/  |    | |      |  __ \      | |  
     | \  / | ___| |_ __ _| |  | | ___ | |_ 
     | |\/| |/ _ \ __/ _` | |  | |/ _ \| __|
     | |  | |  __/ || (_| | |__| | (_) | |_ 
     |_|  |_|\___|\__\__,_|_____/ \___/ \__|
                                             
)";

const std::string win_title_client = U8("MetaDot 少女祈祷中");
const std::string win_title_server = U8("MetaDot Server");

const char *plugin = CR_PLUGIN("CppSource");

extern void fuckme();
extern void testDukpp();

namespace MetaEngine {
    cr_plugin ctx;
}

struct Options
{
    std::optional<std::string> test;
    std::vector<std::string> files;
};
STRUCTOPT(Options, test, files);

HostData Game::data;

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


void Game::updateMaterialSounds() {
    uint16_t waterCt = std::min(movingTiles[Materials::WATER.id], (uint16_t) 5000);
    float water = (float) waterCt / 3000;
    //METADOT_BUG("{} / {} = {}", waterCt, 3000, water);
    audioEngine.SetEventParameter("event:/World/WaterFlow", "FlowIntensity", water);
}

Game::Game() {
    METADOT_GC_INIT();
}

Game::~Game() {
    METADOT_GC_EXIT();
}

int Game::init(int argc, char *argv[]) {

    try {
        auto options = structopt::app("my_app").parse<Options>(argc, argv);

        if (!options.test->empty()) {
            if (options.test == "test_clr") {
                testclr();
                return 0;
            }
            if (options.test == "test_mu") {
                return interp.evaluateFile(std::string(options.files));
            }
        }

    } catch (structopt::exception &e) {
        std::cout << e.what() << "\n";
        std::cout << e.help();
    }


    //networkMode = clArgs->getBool("server") ? NetworkMode::SERVER : NetworkMode::HOST;
    networkMode = NetworkMode::HOST;

    // init console & print title

    std::cout << logo << std::endl;

    loguru::init(argc, argv);

    terminal_log = new ImTerm::terminal<terminal_commands>(cmd_struct);

    MetaEngine::ResourceMan::init();//init location of /res

    InitPhysFS();

    METADOT_INFO("Starting game...");

    bool openDebugUIs = false;
    DebugCheatsUI::visible = openDebugUIs;
    DebugDrawUI::visible = openDebugUIs;
    Settings::draw_frame_graph = openDebugUIs;
    if (!openDebugUIs) {
        Settings::draw_background = true;
        Settings::draw_background_grid = false;
        Settings::draw_load_zones = false;
        Settings::draw_physics_debug = false;
        Settings::draw_chunk_state = false;
        Settings::draw_debug_stats = false;
        Settings::draw_detailed_material_info = false;
        Settings::draw_temperature_map = false;
    }

    this->gameDir = MetaEngine::GameDir(METADOT_RESLOC("gamedir/"));

    Networking::init();
    if (networkMode == NetworkMode::SERVER) {
        int port = Settings::server_port;
        if (argc >= 3) {
            port = atoi(argv[2]);
        }
        server = Server::start(port);
        SDL_SetWindowTitle(window, win_title_server.c_str());

        /*while (true) {
            METADOT_BUG("[SERVER] tick {0:d}", server->server->connectedPeers);
            server->tick();
            Sleep(500);
        }
        return 0;*/

    } else {
        client = Client::start();
        //client->connect(Settings::server_ip.c_str(), Settings::server_port);

        //while (true) {
        //	METADOT_BUG("[CLIENT] tick");

        //	/* Create a reliable packet of size 7 containing "packet\0" */
        //	ENetPacket * packet = enet_packet_create("keepalive",
        //		strlen("keepalive") + 1, 0);
        //	/* Send the packet to the peer over channel id 0. */
        //	/* One could also broadcast the packet by         */
        //	/* enet_host_broadcast (host, 0, packet);         */
        //	enet_peer_send(client->peer, 0, packet);

        //	for (int i = 0; i < 1000; i++) {
        //		client->tick();
        //		Sleep(1);
        //	}
        //}
        //return 0;

        SDL_SetWindowTitle(window, win_title_client.c_str());
    }

    ctpl::thread_pool *initThreadPool = new ctpl::thread_pool(3);
    std::future<void> initThread;
    ctpl::thread_pool *worldInitThreadPool = new ctpl::thread_pool(3);
    std::future<void> worldInitThread;
    if (networkMode != NetworkMode::SERVER) {

        // init fmod


        initThread = initThreadPool->push([&](int id) {
            METADOT_INFO("Initializing audio engine...");

            audioEngine.Init();

            std::vector<std::future<void>> results = {};
            auto updateDirtyPool = new ctpl::thread_pool(1);

            results.push_back(updateDirtyPool->push([](int id) {
                MountPhysFS(METADOT_RESLOC_STR("data/fuckme.zip"), "fuckme");
            }));

            for (auto &v: results) {
                v.get();
            }

            auto fu = LoadFileTextFromPhysFS("fuckme/fucker.txt");

            METADOT_INFO("i'm {}", fu);

            audioEngine.LoadEvent("event:/Music/Title");

            audioEngine.LoadEvent("event:/Player/Jump");
            audioEngine.LoadEvent("event:/Player/Fly");
            audioEngine.LoadEvent("event:/Player/Wind");
            audioEngine.LoadEvent("event:/Player/Impact");

            audioEngine.LoadEvent("event:/World/Sand");
            audioEngine.LoadEvent("event:/World/WaterFlow");

            audioEngine.LoadEvent("event:/GUI/GUI_Hover");
            audioEngine.LoadEvent("event:/GUI/GUI_Slider");

            audioEngine.PlayEvent("event:/Player/Fly");
            audioEngine.PlayEvent("event:/Player/Wind");
            audioEngine.PlayEvent("event:/World/Sand");
            audioEngine.PlayEvent("event:/World/WaterFlow");
        });
    }

    // init sdl


    METADOT_INFO("Initializing SDL...");
    uint32 sdl_init_flags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
    if (SDL_Init(sdl_init_flags) < 0) {
        METADOT_ERROR("SDL_Init failed: {}", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "opengl", SDL_HINT_OVERRIDE);

    if (networkMode != NetworkMode::SERVER) {
        // create the window
        METADOT_INFO("Creating game window...");

        auto title = MetaEngine::Utils::Format("{0} Build {1} - {2}", win_title_client, __DATE__, __TIME__);

        window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

        if (window == nullptr) {
            METADOT_ERROR("Could not create SDL_Window: {}", SDL_GetError());
            return EXIT_FAILURE;
        }

        SDL_SetWindowIcon(window, Textures::loadTexture("data/assets/Icon_32x.png"));

        // create gpu target
        METADOT_INFO("Creating gpu target...");

        METAENGINE_Render_SetDebugLevel(METAENGINE_Render_DEBUG_LEVEL_MAX);
        METAENGINE_Render_SetPreInitFlags(METAENGINE_Render_INIT_DISABLE_VSYNC);


        METAENGINE_Render_SetInitWindow(SDL_GetWindowID(window));


        target = METAENGINE_Render_Init(WIDTH, HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI);


        if (target == NULL) {
            METADOT_ERROR("Could not create METAENGINE_Render_Target: {}", SDL_GetError());
            return EXIT_FAILURE;
        }
        realTarget = target;

        SDL_GLContext &gl_context = target->context->context;

        SDL_GL_MakeCurrent(window, gl_context);

        if (!fglLoadOpenGL(true)) {
            std::cout << "Failed to initialize OpenGL loader!" << std::endl;
            return EXIT_FAILURE;
        }


        METADOT_INFO("Initializing InitFont...");
        if (!Drawing::InitFont(&gl_context)) {
            METADOT_ERROR("InitFont failed");
            return EXIT_FAILURE;
        }


        if (networkMode != NetworkMode::SERVER) {

            font64 = Drawing::LoadFont("data/assets/fonts/pixel_operator/PixelOperator.ttf", 64);
            font16 = Drawing::LoadFont("data/assets/fonts/pixel_operator/PixelOperator.ttf", 16);
            font14 = Drawing::LoadFont("data/assets/fonts/pixel_operator/PixelOperator.ttf", 14);
        }

        m_ImGuiLayer = new MetaEngine::ImGuiLayer();

        m_ImGuiLayer->Init(window, gl_context);

        // load splash screen


        METADOT_INFO("Loading splash screen...");

        METAENGINE_Render_Clear(target);

        METAENGINE_Render_Flip(target);


        SDL_Surface *splashSurf = Textures::loadTexture("data/assets/title/splash.png");

        METAENGINE_Render_Image *splashImg = METAENGINE_Render_CopyImageFromSurface(splashSurf);

        METAENGINE_Render_SetImageFilter(splashImg, METAENGINE_Render_FILTER_NEAREST);

        METAENGINE_Render_BlitRect(splashImg, NULL, target, NULL);

        METAENGINE_Render_FreeImage(splashImg);

        SDL_FreeSurface(splashSurf);

        METAENGINE_Render_Flip(target);

        m_ModuleStack = new MetaEngine::ModuleStack();

        auto m_LuaLayer = new MetaEngine::LuaLayer();
        m_ModuleStack->pushLayer(m_LuaLayer);

        METADOT_MODULE_GET("LuaLayer", MetaEngine::LuaLayer, mm_LuaLayer);

        mm_LuaLayer->getSolState()->script("print(\'haha\')");


#ifdef _WIN32
        SDL_SysWMinfo info{};
        SDL_VERSION(&info.version);
        if (SDL_GetWindowWMInfo(window, &info)) {
            METADOT_ASSERT_E(IsWindow(info.info.win.window));
            this->data.wndh = info.info.win.window;
        } else {
            this->data.wndh = NULL;
        }
#else
        this->data.wndh = 0;
#endif
        this->data.window = window;
        this->data.imgui_context = m_ImGuiLayer->getImGuiCtx();

        MetaEngine::any_function func1{&IamAfuckingNamespace::func1};
        MetaEngine::any_function func2{&IamAfuckingNamespace::func2};

        this->data.Functions.insert(std::make_pair("func1", func1));
        this->data.Functions.insert(std::make_pair("func2", func2));

        RegisterFunctions(func_log_info, IamAfuckingNamespace::func_log_info);


        MetaEngine::ctx.userdata = &this->data;
        cr_plugin_open(MetaEngine::ctx, plugin);

        initThread.get();

        //SoLoud::Soloud soloud;
        //int mp3Handle, oggHandle;
        //SoLoud::WavStream ogg, mp3;
        //soloud.init();

        //mp3.load(METADOT_RESLOC_STR("data/assets/music/ee.mp3"));
        //ogg.load(METADOT_RESLOC_STR("data/assets/music/background.ogg"));

        //mp3.setLooping(1);
        //ogg.setLooping(1);

        //mp3Handle = soloud.play(mp3, 1, 0, 1);
        //oggHandle = soloud.play(ogg, 0, 0, 1);

        //SoLoud::handle groupHandle = soloud.createVoiceGroup();
        //soloud.addVoiceToGroup(groupHandle, mp3Handle);
        //soloud.addVoiceToGroup(groupHandle, oggHandle);

        //soloud.setProtectVoice(groupHandle, 1);
        //soloud.setPause(groupHandle, 0);

        audioEngine.PlayEvent("event:/Music/Title");
        audioEngine.Update();
    }

    // init the world

    worldInitThread = worldInitThreadPool->push([&](int id) {
        fuckme();
        testDukpp();
    });

    if (networkMode != NetworkMode::SERVER) {
        // init steam and discord

        initThread = initThreadPool->push([&](int id) {

        });

        // init the background


        METADOT_INFO("Loading backgrounds...");

        std::vector<BackgroundLayer> testOverworldLayers = {
                BackgroundLayer(Textures::loadTexture("data/assets/backgrounds/TestOverworld/layer2.png", SDL_PIXELFORMAT_ARGB8888), 0.125, 0.125, 1, 0),
                BackgroundLayer(Textures::loadTexture("data/assets/backgrounds/TestOverworld/layer3.png", SDL_PIXELFORMAT_ARGB8888), 0.25, 0.25, 0, 0),
                BackgroundLayer(Textures::loadTexture("data/assets/backgrounds/TestOverworld/layer4.png", SDL_PIXELFORMAT_ARGB8888), 0.375, 0.375, 4, 0),
                BackgroundLayer(Textures::loadTexture("data/assets/backgrounds/TestOverworld/layer5.png", SDL_PIXELFORMAT_ARGB8888), 0.5, 0.5, 0, 0)};

        backgrounds = new Backgrounds();
        Background *bg = new Background(0x7EAFCB, testOverworldLayers);
        backgrounds->Push("TEST_OVERWORLD", bg);
        backgrounds->Get("TEST_OVERWORLD")->init();
    }

    // init the rng

    METADOT_INFO("Seeding RNG...");
    unsigned int seed = (unsigned int) UTime::millis();
    srand(seed);

    // register & set up materials

    METADOT_INFO("Setting up materials...");
    Materials::init();


    movingTiles = new uint16_t[Materials::nMaterials];

    b2DebugDraw = new b2DebugDraw_impl(target);


    worldInitThread.get();


    METADOT_INFO("Initializing world...");
    world = new World();
    world->noSaveLoad = true;
    world->init(gameDir.getWorldPath("mainMenu"), (int) ceil(MAX_WIDTH / 3 / (double) CHUNK_W) * CHUNK_W + CHUNK_W * 3, (int) ceil(MAX_HEIGHT / 3 / (double) CHUNK_H) * CHUNK_H + CHUNK_H * 3, target, &audioEngine, networkMode);


    if (networkMode != NetworkMode::SERVER) {
        // set up main menu ui


        METADOT_INFO("Setting up main menu...");
        STBTTF_Font *labelFont = Drawing::LoadFont("data/assets/fonts/pixel_operator/PixelOperator.ttf", 32);
        STBTTF_Font *uiFont = Drawing::LoadFont("data/assets/fonts/pixel_operator/PixelOperator.ttf", 16);


        std::string displayMode = "windowed";

        if (displayMode == "windowed") {
            setDisplayMode(DisplayMode::WINDOWED);
        } else if (displayMode == "borderless") {
            setDisplayMode(DisplayMode::BORDERLESS);
        } else if (displayMode == "fullscreen") {
            setDisplayMode(DisplayMode::FULLSCREEN);
        }

        setVSync(true);

        // TODO: load settings from settings file
        // also note OptionsUI::minimizeOnFocus exists
        setMinimizeOnLostFocus(false);
    }


    // init threadpools


    updateDirtyPool = new ctpl::thread_pool(6);
    rotateVectorsPool = new ctpl::thread_pool(3);


    if (networkMode != NetworkMode::SERVER) {
        // load shaders
        loadShaders();


        initThread.get();// steam & discord
    }


    return this->run(argc, argv);
}

void Game::loadShaders() {


    if (waterShader) delete waterShader;
    if (waterFlowPassShader) delete waterFlowPassShader;
    if (newLightingShader) delete newLightingShader;
    if (fireShader) delete fireShader;
    if (fire2Shader) delete fire2Shader;

    waterShader = new WaterShader();
    waterFlowPassShader = new WaterFlowPassShader();
    newLightingShader = new NewLightingShader();
    fireShader = new FireShader();
    fire2Shader = new Fire2Shader();
}

void Game::handleWindowSizeChange(int newWidth, int newHeight) {


    int prevWidth = WIDTH;
    int prevHeight = HEIGHT;

    WIDTH = newWidth;
    HEIGHT = newHeight;

    if (loadingTexture) {
        METAENGINE_Render_FreeImage(loadingTexture);
        METAENGINE_Render_FreeImage(texture);
        METAENGINE_Render_FreeImage(worldTexture);
        METAENGINE_Render_FreeImage(lightingTexture);
        METAENGINE_Render_FreeImage(emissionTexture);
        METAENGINE_Render_FreeImage(textureFire);
        METAENGINE_Render_FreeImage(textureFlow);
        METAENGINE_Render_FreeImage(texture2Fire);
        METAENGINE_Render_FreeImage(textureLayer2);
        METAENGINE_Render_FreeImage(textureBackground);
        METAENGINE_Render_FreeImage(textureObjects);
        METAENGINE_Render_FreeImage(textureObjectsLQ);
        METAENGINE_Render_FreeImage(textureObjectsBack);
        METAENGINE_Render_FreeImage(textureParticles);
        METAENGINE_Render_FreeImage(textureEntities);
        METAENGINE_Render_FreeImage(textureEntitiesLQ);
        METAENGINE_Render_FreeImage(temperatureMap);
        METAENGINE_Render_FreeImage(backgroundImage);
    }

    // create textures


    METADOT_INFO("Creating world textures...");
    loadingOnColor = 0xFFFFFFFF;
    loadingOffColor = 0x000000FF;

    loadingTexture = METAENGINE_Render_CreateImage(
            loadingScreenW = (WIDTH / 20), loadingScreenH = (HEIGHT / 20),
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);


    METAENGINE_Render_SetImageFilter(loadingTexture, METAENGINE_Render_FILTER_NEAREST);


    texture = METAENGINE_Render_CreateImage(
            world->width, world->height,
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);


    METAENGINE_Render_SetImageFilter(texture, METAENGINE_Render_FILTER_NEAREST);


    worldTexture = METAENGINE_Render_CreateImage(
            world->width * Settings::hd_objects_size, world->height * Settings::hd_objects_size,
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);


    METAENGINE_Render_SetImageFilter(worldTexture, METAENGINE_Render_FILTER_NEAREST);


    METAENGINE_Render_LoadTarget(worldTexture);


    lightingTexture = METAENGINE_Render_CreateImage(
            world->width, world->height,
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
    METAENGINE_Render_SetImageFilter(lightingTexture, METAENGINE_Render_FILTER_NEAREST);
    METAENGINE_Render_LoadTarget(lightingTexture);


    emissionTexture = METAENGINE_Render_CreateImage(
            world->width, world->height,
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
    METAENGINE_Render_SetImageFilter(emissionTexture, METAENGINE_Render_FILTER_NEAREST);


    textureFlow = METAENGINE_Render_CreateImage(
            world->width, world->height,
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
    METAENGINE_Render_SetImageFilter(textureFlow, METAENGINE_Render_FILTER_NEAREST);


    textureFlowSpead = METAENGINE_Render_CreateImage(
            world->width, world->height,
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
    METAENGINE_Render_SetImageFilter(textureFlowSpead, METAENGINE_Render_FILTER_NEAREST);
    METAENGINE_Render_LoadTarget(textureFlowSpead);

    textureFire = METAENGINE_Render_CreateImage(
            world->width, world->height,
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
    METAENGINE_Render_SetImageFilter(textureFire, METAENGINE_Render_FILTER_NEAREST);

    texture2Fire = METAENGINE_Render_CreateImage(
            world->width, world->height,
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
    METAENGINE_Render_SetImageFilter(texture2Fire, METAENGINE_Render_FILTER_NEAREST);
    METAENGINE_Render_LoadTarget(texture2Fire);

    textureLayer2 = METAENGINE_Render_CreateImage(
            world->width, world->height,
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
    METAENGINE_Render_SetImageFilter(textureLayer2, METAENGINE_Render_FILTER_NEAREST);


    textureBackground = METAENGINE_Render_CreateImage(
            world->width, world->height,
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
    METAENGINE_Render_SetImageFilter(textureBackground, METAENGINE_Render_FILTER_NEAREST);

    textureObjects = METAENGINE_Render_CreateImage(
            world->width * (Settings::hd_objects ? Settings::hd_objects_size : 1), world->height * (Settings::hd_objects ? Settings::hd_objects_size : 1),
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
    METAENGINE_Render_SetImageFilter(textureObjects, METAENGINE_Render_FILTER_NEAREST);
    METAENGINE_Render_LoadTarget(textureObjects);


    textureObjectsLQ = METAENGINE_Render_CreateImage(
            world->width, world->height,
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
    METAENGINE_Render_SetImageFilter(textureObjectsLQ, METAENGINE_Render_FILTER_NEAREST);
    METAENGINE_Render_LoadTarget(textureObjectsLQ);


    textureObjectsBack = METAENGINE_Render_CreateImage(
            world->width * (Settings::hd_objects ? Settings::hd_objects_size : 1), world->height * (Settings::hd_objects ? Settings::hd_objects_size : 1),
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
    METAENGINE_Render_SetImageFilter(textureObjectsBack, METAENGINE_Render_FILTER_NEAREST);
    METAENGINE_Render_LoadTarget(textureObjectsBack);


    textureParticles = METAENGINE_Render_CreateImage(
            world->width, world->height,
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);


    METAENGINE_Render_SetImageFilter(textureParticles, METAENGINE_Render_FILTER_NEAREST);


    textureEntities = METAENGINE_Render_CreateImage(
            world->width * (Settings::hd_objects ? Settings::hd_objects_size : 1), world->height * (Settings::hd_objects ? Settings::hd_objects_size : 1),
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);


    METAENGINE_Render_LoadTarget(textureEntities);


    METAENGINE_Render_SetImageFilter(textureEntities, METAENGINE_Render_FILTER_NEAREST);


    textureEntitiesLQ = METAENGINE_Render_CreateImage(
            world->width, world->height,
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);


    METAENGINE_Render_LoadTarget(textureEntitiesLQ);


    METAENGINE_Render_SetImageFilter(textureEntitiesLQ, METAENGINE_Render_FILTER_NEAREST);


    temperatureMap = METAENGINE_Render_CreateImage(
            world->width, world->height,
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);


    METAENGINE_Render_SetImageFilter(temperatureMap, METAENGINE_Render_FILTER_NEAREST);


    backgroundImage = METAENGINE_Render_CreateImage(
            WIDTH, HEIGHT,
            METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);


    METAENGINE_Render_SetImageFilter(backgroundImage, METAENGINE_Render_FILTER_NEAREST);


    METAENGINE_Render_LoadTarget(backgroundImage);


    // create texture pixel buffers

    pixels = std::vector<unsigned char>(world->width * world->height * 4, 0);
    pixels_ar = &pixels[0];
    pixelsLayer2 = std::vector<unsigned char>(world->width * world->height * 4, 0);
    pixelsLayer2_ar = &pixelsLayer2[0];
    pixelsBackground = std::vector<unsigned char>(world->width * world->height * 4, 0);
    pixelsBackground_ar = &pixelsBackground[0];
    pixelsObjects = std::vector<unsigned char>(world->width * world->height * 4, SDL_ALPHA_TRANSPARENT);
    pixelsObjects_ar = &pixelsObjects[0];
    pixelsTemp = std::vector<unsigned char>(world->width * world->height * 4, SDL_ALPHA_TRANSPARENT);
    pixelsTemp_ar = &pixelsTemp[0];
    pixelsParticles = std::vector<unsigned char>(world->width * world->height * 4, SDL_ALPHA_TRANSPARENT);
    pixelsParticles_ar = &pixelsParticles[0];
    pixelsLoading = std::vector<unsigned char>(loadingTexture->w * loadingTexture->h * 4, SDL_ALPHA_TRANSPARENT);
    pixelsLoading_ar = &pixelsLoading[0];
    pixelsFire = std::vector<unsigned char>(world->width * world->height * 4, 0);
    pixelsFire_ar = &pixelsFire[0];
    pixelsFlow = std::vector<unsigned char>(world->width * world->height * 4, 0);
    pixelsFlow_ar = &pixelsFlow[0];
    pixelsEmission = std::vector<unsigned char>(world->width * world->height * 4, 0);
    pixelsEmission_ar = &pixelsEmission[0];


    accLoadX -= (newWidth - prevWidth) / 2.0f / scale;
    accLoadY -= (newHeight - prevHeight) / 2.0f / scale;

    tickChunkLoading();

    for (int x = 0; x < world->width; x++) {
        for (int y = 0; y < world->height; y++) {
            world->dirty[x + y * world->width] = true;
            world->layer2Dirty[x + y * world->width] = true;
            world->backgroundDirty[x + y * world->width] = true;
        }
    }
}

void Game::setWindowFlash(WindowFlashAction action, int count, int period) {
    // TODO: look into alternatives for linux/crossplatform
#ifdef _WIN32

    FLASHWINFO flash;
    flash.cbSize = sizeof(FLASHWINFO);
    //flash.hwnd = hwnd;
    flash.uCount = count;
    flash.dwTimeout = period;

    // pretty sure these flags are supposed to work but they all seem to do the same thing on my machine so idk
    switch (action) {
        case WindowFlashAction::START:
            flash.dwFlags = FLASHW_ALL;
            break;
        case WindowFlashAction::START_COUNT:
            flash.dwFlags = FLASHW_ALL | FLASHW_TIMER;
            break;
        case WindowFlashAction::START_UNTIL_FG:
            flash.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
            break;
        case WindowFlashAction::STOP:
            flash.dwFlags = FLASHW_STOP;
            break;
    }

    FlashWindowEx(&flash);

#endif
}

void Game::setDisplayMode(DisplayMode mode) {
    switch (mode) {
        case DisplayMode::WINDOWED:
            SDL_SetWindowDisplayMode(window, NULL);
            SDL_SetWindowFullscreen(window, 0);
            OptionsUI::item_current_idx = 0;
            break;
        case DisplayMode::BORDERLESS:
            SDL_SetWindowDisplayMode(window, NULL);
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            OptionsUI::item_current_idx = 1;
            break;
        case DisplayMode::FULLSCREEN:
            SDL_MaximizeWindow(window);

            int w;
            int h;
            SDL_GetWindowSize(window, &w, &h);

            SDL_DisplayMode disp;
            SDL_GetWindowDisplayMode(window, &disp);

            disp.w = w;
            disp.h = h;

            SDL_SetWindowDisplayMode(window, &disp);
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
            OptionsUI::item_current_idx = 2;
            break;
    }

    int w;
    int h;
    SDL_GetWindowSize(window, &w, &h);

    METAENGINE_Render_SetWindowResolution(w, h);
    METAENGINE_Render_ResetProjection(realTarget);

    handleWindowSizeChange(w, h);
}

void Game::setVSync(bool vsync) {
    SDL_GL_SetSwapInterval(vsync ? 1 : 0);
    OptionsUI::vsync = vsync;
}

void Game::setMinimizeOnLostFocus(bool minimize) {
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, minimize ? "1" : "0");
    OptionsUI::minimizeOnFocus = minimize;
}

int Game::run(int argc, char *argv[]) {
    startTime = UTime::millis();

    // start loading chunks


    METADOT_INFO("Queueing chunk loading...");
    for (int x = -CHUNK_W * 4; x < world->width + CHUNK_W * 4; x += CHUNK_W) {
        for (int y = -CHUNK_H * 3; y < world->height + CHUNK_H * 8; y += CHUNK_H) {
            world->queueLoadChunk(x / CHUNK_W, y / CHUNK_H, true, true);
        }
    }


    // start game loop


    METADOT_INFO("Starting game loop...");

    freeCamX = world->width / 2.0f - CHUNK_W / 2;
    freeCamY = world->height / 2.0f - (int) (CHUNK_H * 0.75);
    if (world->player) {
        plPosX = world->player->x;
        plPosY = world->player->y;
    } else {
        plPosX = freeCamX;
        plPosY = freeCamY;
    }

    SDL_Event windowEvent;

    long long lastFPS = UTime::millis();
    int frames = 0;
    fps = 0;

    lastTime = UTime::millis();
    lastTick = lastTime;
    long long lastTickPhysics = lastTime;

    mspt = 33;
    long msptPhysics = 16;

    scale = 3;
    ofsX = (int) (-CHUNK_W * 4);
    ofsY = (int) (-CHUNK_H * 2.5);

    ofsX = (ofsX - WIDTH / 2) / 2 * 3 + WIDTH / 2;
    ofsY = (ofsY - HEIGHT / 2) / 2 * 3 + HEIGHT / 2;

    for (int i = 0; i < frameTimeNum; i++) {
        frameTime[i] = 0;
    }
    objectDelete = new bool[world->width * world->height];


    fadeInStart = UTime::millis();
    fadeInLength = 250;
    fadeInWaitFrames = 5;

    // game loop

    while (this->running) {

        now = UTime::millis();
        deltaTime = now - lastTime;

        if (networkMode != NetworkMode::SERVER) {

            // handle window events


            while (SDL_PollEvent(&windowEvent)) {

                if (windowEvent.type == SDL_WINDOWEVENT) {
                    if (windowEvent.window.event == SDL_WINDOWEVENT_RESIZED) {
                        //METADOT_INFO("Resizing window...");
                        METAENGINE_Render_SetWindowResolution(windowEvent.window.data1, windowEvent.window.data2);
                        METAENGINE_Render_ResetProjection(realTarget);
                        handleWindowSizeChange(windowEvent.window.data1, windowEvent.window.data2);
                    }
                }


                ImGui_ImplSDL2_ProcessEvent(&windowEvent);


                if (windowEvent.type == SDL_QUIT) {
                    goto exit;
                }

                if (ImGui::GetIO().WantCaptureMouse) {
                    if (windowEvent.type == SDL_MOUSEBUTTONDOWN || windowEvent.type == SDL_MOUSEBUTTONUP || windowEvent.type == SDL_MOUSEMOTION || windowEvent.type == SDL_MOUSEWHEEL) {
                        continue;
                    }
                }

                if (ImGui::GetIO().WantCaptureKeyboard) {
                    if (windowEvent.type == SDL_KEYDOWN || windowEvent.type == SDL_KEYUP) {
                        continue;
                    }
                }

                if (windowEvent.type == SDL_MOUSEWHEEL) {
                    // zoom in/out

                    int deltaScale = windowEvent.wheel.y;
                    int oldScale = scale;
                    scale += deltaScale;
                    if (scale < 1) scale = 1;

                    ofsX = (ofsX - WIDTH / 2) / oldScale * scale + WIDTH / 2;
                    ofsY = (ofsY - HEIGHT / 2) / oldScale * scale + HEIGHT / 2;

                } else if (windowEvent.type == SDL_MOUSEMOTION) {
                    if (Controls::DEBUG_DRAW->get()) {
                        // draw material

                        int x = (int) ((windowEvent.motion.x - ofsX - camX) / scale);
                        int y = (int) ((windowEvent.motion.y - ofsY - camY) / scale);

                        if (lastDrawMX == 0 && lastDrawMY == 0) {
                            lastDrawMX = x;
                            lastDrawMY = y;
                        }

                        world->forLine(lastDrawMX, lastDrawMY, x, y, [&](int index) {
                            int lineX = index % world->width;
                            int lineY = index / world->width;

                            for (int xx = -DebugDrawUI::brushSize / 2; xx < (int) (ceil(DebugDrawUI::brushSize / 2.0)); xx++) {
                                for (int yy = -DebugDrawUI::brushSize / 2; yy < (int) (ceil(DebugDrawUI::brushSize / 2.0)); yy++) {
                                    if (lineX + xx < 0 || lineY + yy < 0 || lineX + xx >= world->width || lineY + yy >= world->height) continue;
                                    MaterialInstance tp = Tiles::create(DebugDrawUI::selectedMaterial, lineX + xx, lineY + yy);
                                    world->tiles[(lineX + xx) + (lineY + yy) * world->width] = tp;
                                    world->dirty[(lineX + xx) + (lineY + yy) * world->width] = true;
                                }
                            }

                            return false;
                        });

                        lastDrawMX = x;
                        lastDrawMY = y;

                    } else {
                        lastDrawMX = 0;
                        lastDrawMY = 0;
                    }

                    if (Controls::mmouse) {
                        // erase material

                        // erase from world
                        int x = (int) ((windowEvent.motion.x - ofsX - camX) / scale);
                        int y = (int) ((windowEvent.motion.y - ofsY - camY) / scale);

                        if (lastEraseMX == 0 && lastEraseMY == 0) {
                            lastEraseMX = x;
                            lastEraseMY = y;
                        }

                        world->forLine(lastEraseMX, lastEraseMY, x, y, [&](int index) {
                            int lineX = index % world->width;
                            int lineY = index / world->width;

                            for (int xx = -DebugDrawUI::brushSize / 2; xx < (int) (ceil(DebugDrawUI::brushSize / 2.0)); xx++) {
                                for (int yy = -DebugDrawUI::brushSize / 2; yy < (int) (ceil(DebugDrawUI::brushSize / 2.0)); yy++) {

                                    if (abs(xx) + abs(yy) == DebugDrawUI::brushSize) continue;
                                    if (world->getTile(lineX + xx, lineY + yy).mat->physicsType != PhysicsType::AIR) {
                                        world->setTile(lineX + xx, lineY + yy, Tiles::NOTHING);
                                        world->lastMeshZone.x--;
                                    }
                                    if (world->getTileLayer2(lineX + xx, lineY + yy).mat->physicsType != PhysicsType::AIR) {
                                        world->setTileLayer2(lineX + xx, lineY + yy, Tiles::NOTHING);
                                    }
                                }
                            }
                            return false;
                        });

                        lastEraseMX = x;
                        lastEraseMY = y;

                        // erase from rigidbodies
                        // this copies the vector
                        std::vector<RigidBody *> rbs = world->rigidBodies;

                        for (size_t i = 0; i < rbs.size(); i++) {
                            RigidBody *cur = rbs[i];
                            if (cur->body->IsEnabled()) {
                                float s = sin(-cur->body->GetAngle());
                                float c = cos(-cur->body->GetAngle());
                                bool upd = false;
                                for (float xx = -3; xx <= 3; xx += 0.5) {
                                    for (float yy = -3; yy <= 3; yy += 0.5) {
                                        if (abs(xx) + abs(yy) == 6) continue;
                                        // rotate point

                                        float tx = x + xx - cur->body->GetPosition().x;
                                        float ty = y + yy - cur->body->GetPosition().y;

                                        int ntx = (int) (tx * c - ty * s);
                                        int nty = (int) (tx * s + ty * c);

                                        if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w && nty < cur->surface->h) {
                                            Uint32 pixel = METADOT_GET_PIXEL(cur->surface, ntx, nty);
                                            if (((pixel >> 24) & 0xff) != 0x00) {
                                                METADOT_GET_PIXEL(cur->surface, ntx, nty) = 0x00000000;
                                                upd = true;
                                            }
                                        }
                                    }
                                }

                                if (upd) {
                                    METAENGINE_Render_FreeImage(cur->texture);
                                    cur->texture = METAENGINE_Render_CopyImageFromSurface(cur->surface);
                                    METAENGINE_Render_SetImageFilter(cur->texture, METAENGINE_Render_FILTER_NEAREST);
                                    //world->updateRigidBodyHitbox(cur);
                                    cur->needsUpdate = true;
                                }
                            }
                        }


                    } else {
                        lastEraseMX = 0;
                        lastEraseMY = 0;
                    }
                } else if (windowEvent.type == SDL_KEYDOWN) {
                    Controls::keyEvent(windowEvent.key);
                } else if (windowEvent.type == SDL_KEYUP) {
                    Controls::keyEvent(windowEvent.key);
                }

                if (windowEvent.type == SDL_MOUSEBUTTONDOWN) {
                    if (windowEvent.button.button == SDL_BUTTON_LEFT) {
                        Controls::lmouse = true;

                        if (world->player && world->player->heldItem != NULL) {
                            if (world->player->heldItem->getFlag(ItemFlags::VACUUM)) {
                                world->player->holdVacuum = true;
                            } else if (world->player->heldItem->getFlag(ItemFlags::HAMMER)) {
                                //#define HAMMER_DEBUG_PHYSICS
#ifdef HAMMER_DEBUG_PHYSICS
                                int x = (int) ((windowEvent.button.x - ofsX - camX) / scale);
                                int y = (int) ((windowEvent.button.y - ofsY - camY) / scale);

                                world->physicsCheck(x, y);
#else
                                mx = windowEvent.button.x;
                                my = windowEvent.button.y;
                                int startInd = getAimSolidSurface(64);

                                if (startInd != -1) {
                                    //world->player->hammerX = x;
                                    //world->player->hammerY = y;
                                    world->player->hammerX = startInd % world->width;
                                    world->player->hammerY = startInd / world->width;
                                    world->player->holdHammer = true;
                                    //METADOT_BUG("hammer down: {0:d} {0:d} {0:d} {0:d} {0:d}", x, y, startInd, startInd % world->width, startInd / world->width);
                                    //world->setTile(world->player->hammerX, world->player->hammerY, MaterialInstance(&Materials::GENERIC_SOLID, 0x00ff00ff));
                                }
#endif
#undef HAMMER_DEBUG_PHYSICS
                            } else if (world->player->heldItem->getFlag(ItemFlags::CHISEL)) {
                                // if hovering rigidbody, open in chisel

                                int x = (int) ((mx - ofsX - camX) / scale);
                                int y = (int) ((my - ofsY - camY) / scale);

                                std::vector<RigidBody *> rbs = world->rigidBodies;// copy
                                for (size_t i = 0; i < rbs.size(); i++) {
                                    RigidBody *cur = rbs[i];

                                    bool connect = false;
                                    if (cur->body->IsEnabled()) {
                                        float s = sin(-cur->body->GetAngle());
                                        float c = cos(-cur->body->GetAngle());
                                        bool upd = false;
                                        for (float xx = -3; xx <= 3; xx += 0.5) {
                                            for (float yy = -3; yy <= 3; yy += 0.5) {
                                                if (abs(xx) + abs(yy) == 6) continue;
                                                // rotate point

                                                float tx = x + xx - cur->body->GetPosition().x;
                                                float ty = y + yy - cur->body->GetPosition().y;

                                                int ntx = (int) (tx * c - ty * s);
                                                int nty = (int) (tx * s + ty * c);

                                                if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w && nty < cur->surface->h) {
                                                    Uint32 pixel = METADOT_GET_PIXEL(cur->surface, ntx, nty);
                                                    if (((pixel >> 24) & 0xff) != 0x00) {
                                                        connect = true;
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    if (connect) {

                                        // previously: open chisel ui

                                        break;
                                    }
                                }

                            } else if (world->player->heldItem->getFlag(ItemFlags::TOOL)) {
                                // break with pickaxe

                                float breakSize = world->player->heldItem->breakSize;

                                int x = (int) (world->player->x + world->player->hw / 2.0f + world->loadZone.x + 10 * (float) cos((world->player->holdAngle + 180) * 3.1415f / 180.0f) - breakSize / 2);
                                int y = (int) (world->player->y + world->player->hh / 2.0f + world->loadZone.y + 10 * (float) sin((world->player->holdAngle + 180) * 3.1415f / 180.0f) - breakSize / 2);

                                SDL_Surface *tex = SDL_CreateRGBSurfaceWithFormat(0, (int) breakSize, (int) breakSize, 32, SDL_PIXELFORMAT_ARGB8888);

                                int n = 0;
                                for (int xx = 0; xx < breakSize; xx++) {
                                    for (int yy = 0; yy < breakSize; yy++) {
                                        float cx = (float) ((xx / breakSize) - 0.5);
                                        float cy = (float) ((yy / breakSize) - 0.5);

                                        if (cx * cx + cy * cy > 0.25f) continue;

                                        if (world->tiles[(x + xx) + (y + yy) * world->width].mat->physicsType == PhysicsType::SOLID) {
                                            METADOT_GET_PIXEL(tex, xx, yy) = world->tiles[(x + xx) + (y + yy) * world->width].color;
                                            world->tiles[(x + xx) + (y + yy) * world->width] = Tiles::NOTHING;
                                            world->dirty[(x + xx) + (y + yy) * world->width] = true;

                                            n++;
                                        }
                                    }
                                }

                                if (n > 0) {
                                    audioEngine.PlayEvent("event:/Player/Impact");
                                    b2PolygonShape s;
                                    s.SetAsBox(1, 1);
                                    RigidBody *rb = world->makeRigidBody(b2_dynamicBody, (float) x, (float) y, 0, s, 1, (float) 0.3, tex);

                                    b2Filter bf = {};
                                    bf.categoryBits = 0x0001;
                                    bf.maskBits = 0xffff;
                                    rb->body->GetFixtureList()[0].SetFilterData(bf);

                                    rb->body->SetLinearVelocity({(float) ((rand() % 100) / 100.0 - 0.5), (float) ((rand() % 100) / 100.0 - 0.5)});

                                    world->rigidBodies.push_back(rb);
                                    world->updateRigidBodyHitbox(rb);

                                    world->lastMeshLoadZone.x--;
                                    world->updateWorldMesh();
                                }
                            }
                        }

                    } else if (windowEvent.button.button == SDL_BUTTON_RIGHT) {
                        Controls::rmouse = true;
                        if (world->player) world->player->startThrow = UTime::millis();
                    } else if (windowEvent.button.button == SDL_BUTTON_MIDDLE) {
                        Controls::mmouse = true;
                    }
                } else if (windowEvent.type == SDL_MOUSEBUTTONUP) {
                    if (windowEvent.button.button == SDL_BUTTON_LEFT) {
                        Controls::lmouse = false;

                        if (world->player) {
                            if (world->player->heldItem) {
                                if (world->player->heldItem->getFlag(ItemFlags::VACUUM)) {
                                    if (world->player->holdVacuum) {
                                        world->player->holdVacuum = false;
                                    }
                                } else if (world->player->heldItem->getFlag(ItemFlags::HAMMER)) {
                                    if (world->player->holdHammer) {
                                        int x = (int) ((windowEvent.button.x - ofsX - camX) / scale);
                                        int y = (int) ((windowEvent.button.y - ofsY - camY) / scale);

                                        int dx = world->player->hammerX - x;
                                        int dy = world->player->hammerY - y;
                                        float len = sqrtf(dx * dx + dy * dy);
                                        float udx = dx / len;
                                        float udy = dy / len;

                                        int ex = world->player->hammerX + dx;
                                        int ey = world->player->hammerY + dy;
                                        METADOT_BUG("hammer up: {0:d} {0:d} {0:d} {0:d}", ex, ey, dx, dy);
                                        int endInd = -1;

                                        int nSegments = 1 + len / 10;
                                        std::vector<std::tuple<int, int>> points = {};
                                        for (int i = 0; i < nSegments; i++) {
                                            int sx = world->player->hammerX + (int) ((float) (dx / nSegments) * (i + 1));
                                            int sy = world->player->hammerY + (int) ((float) (dy / nSegments) * (i + 1));
                                            sx += rand() % 3 - 1;
                                            sy += rand() % 3 - 1;
                                            points.push_back(std::tuple<int, int>(sx, sy));
                                        }

                                        int nTilesChanged = 0;
                                        for (size_t i = 0; i < points.size(); i++) {
                                            int segSx = i == 0 ? world->player->hammerX : std::get<0>(points[i - 1]);
                                            int segSy = i == 0 ? world->player->hammerY : std::get<1>(points[i - 1]);
                                            int segEx = std::get<0>(points[i]);
                                            int segEy = std::get<1>(points[i]);

                                            bool hitSolidYet = false;
                                            bool broke = false;
                                            world->forLineCornered(segSx, segSy, segEx, segEy, [&](int index) {
                                                if (world->tiles[index].mat->physicsType != PhysicsType::SOLID) {
                                                    if (hitSolidYet && (abs((index % world->width) - segSx) + (abs((index / world->width) - segSy)) > 1)) {
                                                        broke = true;
                                                        return true;
                                                    }
                                                    return false;
                                                }
                                                hitSolidYet = true;
                                                world->tiles[index] = MaterialInstance(&Materials::GENERIC_SAND, Drawing::darkenColor(world->tiles[index].color, 0.5f));
                                                world->dirty[index] = true;
                                                endInd = index;
                                                nTilesChanged++;
                                                return false;
                                            });

                                            //world->setTile(segSx, segSy, MaterialInstance(&Materials::GENERIC_SOLID, 0x00ff00ff));
                                            if (broke) break;
                                        }

                                        //world->setTile(ex, ey, MaterialInstance(&Materials::GENERIC_SOLID, 0xff0000ff));

                                        int hx = (world->player->hammerX + (endInd % world->width)) / 2;
                                        int hy = (world->player->hammerY + (endInd / world->width)) / 2;

                                        if (world->getTile((int) (hx + udy * 2), (int) (hy - udx * 2)).mat->physicsType == PhysicsType::SOLID) {
                                            world->physicsCheck((int) (hx + udy * 2), (int) (hy - udx * 2));
                                        }

                                        if (world->getTile((int) (hx - udy * 2), (int) (hy + udx * 2)).mat->physicsType == PhysicsType::SOLID) {
                                            world->physicsCheck((int) (hx - udy * 2), (int) (hy + udx * 2));
                                        }

                                        if (nTilesChanged > 0) {
                                            audioEngine.PlayEvent("event:/Player/Impact");
                                        }

                                        //world->setTile((int)(hx), (int)(hy), MaterialInstance(&Materials::GENERIC_SOLID, 0xffffffff));
                                        //world->setTile((int)(hx + udy * 6), (int)(hy - udx * 6), MaterialInstance(&Materials::GENERIC_SOLID, 0xffff00ff));
                                        //world->setTile((int)(hx - udy * 6), (int)(hy + udx * 6), MaterialInstance(&Materials::GENERIC_SOLID, 0x00ffffff));
                                    }
                                    world->player->holdHammer = false;
                                }
                            }
                        }
                    } else if (windowEvent.button.button == SDL_BUTTON_RIGHT) {
                        Controls::rmouse = false;
                        // pick up / throw item

                        int x = (int) ((mx - ofsX - camX) / scale);
                        int y = (int) ((my - ofsY - camY) / scale);

                        bool swapped = false;
                        std::vector<RigidBody *> rbs = world->rigidBodies;// copy;
                        for (size_t i = 0; i < rbs.size(); i++) {
                            RigidBody *cur = rbs[i];

                            bool connect = false;
                            if (cur->body->IsEnabled()) {
                                float s = sin(-cur->body->GetAngle());
                                float c = cos(-cur->body->GetAngle());
                                bool upd = false;
                                for (float xx = -3; xx <= 3; xx += 0.5) {
                                    for (float yy = -3; yy <= 3; yy += 0.5) {
                                        if (abs(xx) + abs(yy) == 6) continue;
                                        // rotate point

                                        float tx = x + xx - cur->body->GetPosition().x;
                                        float ty = y + yy - cur->body->GetPosition().y;

                                        int ntx = (int) (tx * c - ty * s);
                                        int nty = (int) (tx * s + ty * c);

                                        if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w && nty < cur->surface->h) {
                                            if (((METADOT_GET_PIXEL(cur->surface, ntx, nty) >> 24) & 0xff) != 0x00) {
                                                connect = true;
                                            }
                                        }
                                    }
                                }
                            }

                            if (connect) {
                                if (world->player) {
                                    world->player->setItemInHand(Item::makeItem(ItemFlags::RIGIDBODY, cur), world);

                                    world->b2world->DestroyBody(cur->body);
                                    world->rigidBodies.erase(std::remove(world->rigidBodies.begin(), world->rigidBodies.end(), cur), world->rigidBodies.end());

                                    swapped = true;
                                }
                                break;
                            }
                        }

                        if (!swapped) {
                            if (world->player) world->player->setItemInHand(NULL, world);
                        }

                    } else if (windowEvent.button.button == SDL_BUTTON_MIDDLE) {
                        Controls::mmouse = false;
                    }
                }

                if (windowEvent.type == SDL_MOUSEMOTION) {
                    mx = windowEvent.motion.x;
                    my = windowEvent.motion.y;
                }
            }
        }


        if (networkMode == NetworkMode::SERVER) {
            server->tick();
        } else if (networkMode == NetworkMode::CLIENT) {
            client->tick();
        }

        if (networkMode != NetworkMode::SERVER) {
            //if(Settings::tick_world)
            updateFrameEarly();
            for (MetaEngine::Module *l: *m_ModuleStack) {
                l->onUpdate();
            }
        }

        while (now - lastTick > mspt) {
            if (Settings::tick_world && networkMode != NetworkMode::CLIENT)
                tick();
            target = realTarget;
            lastTick = now;
            tickTime++;
        }

        if (networkMode != NetworkMode::SERVER) {
            if (Settings::tick_world)
                updateFrameLate();
        }


        if (networkMode != NetworkMode::SERVER) {
            // render


            target = realTarget;
            METAENGINE_Render_Clear(target);


            renderEarly();
            target = realTarget;

            renderLate();
            target = realTarget;

            // render ImGui


            METAENGINE_Render_ActivateShaderProgram(0, NULL);
            METAENGINE_Render_FlushBlitBuffer();

            m_ImGuiLayer->begin();

            m_ImGuiLayer->Render(this);


            for (MetaEngine::Module *l: *m_ModuleStack) {
                l->onRender();
            }

            cr_plugin_update(MetaEngine::ctx);


            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu(ICON_FA_SYNC " 系统")) {
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu(ICON_FA_ARCHIVE " 工具")) {
                    if (ImGui::MenuItem("八个雅鹿", "CTRL+A")) {}
                    ImGui::Separator();
                    ImGui::Checkbox("调整", &Settings::ui_tweak);
                    ImGui::Checkbox("脚本编辑器", &Settings::ui_code_editor);
                    ImGui::Checkbox("Inspector", &Settings::ui_inspector);
                    ImGui::Checkbox("内存监测", &Settings::ui_gcmanager);
                    ImGui::Checkbox("控制台", &Settings::ui_console);
                    ImGui::EndMenu();
                }

                ImGui::SameLine(ImGui::GetWindowWidth() - 310);

                ImGui::Separator();
                ImGui::Text("%.3f ms/frame (%.1f(%d) FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate, feelsLikeFps);

                ImGui::EndMainMenuBar();
            }


            if (DebugDrawUI::visible) {
                ImGui::Begin("Debug Info");
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

                METAENGINE_Render_Renderer *renderer = METAENGINE_Render_GetCurrentRenderer();
                METAENGINE_Render_RendererID id = renderer->id;

                ImGui::Text("Using renderer: %s (%d.%d)\n", id.name, id.major_version, id.minor_version);
                ImGui::Text("  Shader versions supported: %d to %d\n\n", renderer->min_shader_version, renderer->max_shader_version);

                ImGui::End();
            }

            if (Settings::ui_console) {
                terminal_log->show();
            }

            if (Settings::draw_material_info && !ImGui::GetIO().WantCaptureMouse) {

                int msx = (int) ((mx - ofsX - camX) / scale);
                int msy = (int) ((my - ofsY - camY) / scale);

                MaterialInstance tile;

                if (msx >= 0 && msy >= 0 && msx < world->width && msy < world->height) {
                    tile = world->tiles[msx + msy * world->width];
                    //Drawing::drawText(target, tile.mat->name.c_str(), font16, mx + 14, my, 0xff, 0xff, 0xff, ALIGN_LEFT);

                    if (tile.mat->id == Materials::GENERIC_AIR.id) {
                        std::vector<RigidBody *> rbs = world->rigidBodies;

                        for (size_t i = 0; i < rbs.size(); i++) {
                            RigidBody *cur = rbs[i];
                            if (cur->body->IsEnabled()) {
                                float s = sin(-cur->body->GetAngle());
                                float c = cos(-cur->body->GetAngle());
                                bool upd = false;

                                float tx = msx - cur->body->GetPosition().x;
                                float ty = msy - cur->body->GetPosition().y;

                                int ntx = (int) (tx * c - ty * s);
                                int nty = (int) (tx * s + ty * c);

                                if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w && nty < cur->surface->h) {
                                    tile = cur->tiles[ntx + nty * cur->matWidth];
                                }
                            }
                        }
                    }

                    if (tile.mat->id != Materials::GENERIC_AIR.id) {

                        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.11f, 0.11f, 0.11f, 0.4f));
                        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.00f, 1.00f, 1.00f, 0.2f));
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", tile.mat->name.c_str());

                        if (Settings::draw_detailed_material_info) {

                            if (tile.mat->physicsType == PhysicsType::SOUP) {
                                ImGui::Text("fluidAmount = %f", tile.fluidAmount);
                            }

                            int ln = 0;
                            if (tile.mat->interact) {
                                for (size_t i = 0; i < Materials::MATERIALS.size(); i++) {
                                    if (tile.mat->nInteractions[i] > 0) {
                                        char buff2[40];
                                        snprintf(buff2, sizeof(buff2), "    %s", Materials::MATERIALS[i]->name.c_str());
                                        //Drawing::drawText(target, buff2, font16, mx + 14, my + 14 * ++ln, 0xff, 0xff, 0xff, ALIGN_LEFT);
                                        ImGui::Text("%s", buff2);

                                        for (int j = 0; j < tile.mat->nInteractions[i]; j++) {
                                            MaterialInteraction inter = tile.mat->interactions[i][j];
                                            char buff1[40];
                                            if (inter.type == INTERACT_TRANSFORM_MATERIAL) {
                                                snprintf(buff1, sizeof(buff1), "        %s %s r=%d x=%d y=%d", "TRANSFORM", Materials::MATERIALS[inter.data1]->name.c_str(), inter.data2, inter.ofsX, inter.ofsY);
                                            } else if (inter.type == INTERACT_SPAWN_MATERIAL) {
                                                snprintf(buff1, sizeof(buff1), "        %s %s r=%d x=%d y=%d", "SPAWN", Materials::MATERIALS[inter.data1]->name.c_str(), inter.data2, inter.ofsX, inter.ofsY);
                                            }
                                            //Drawing::drawText(target, buff1, font16, mx + 14, my + 14 * ++ln, 0xff, 0xff, 0xff, ALIGN_LEFT);
                                            ImGui::Text("%s", buff1);
                                        }
                                    }
                                }
                            }
                        }

                        ImGui::EndTooltip();
                        ImGui::PopStyleColor();
                        ImGui::PopStyleColor();
                    }
                }
            }

            m_ImGuiLayer->end();


            // render fade in/out
            if (fadeInWaitFrames > 0) {
                fadeInWaitFrames--;
                fadeInStart = now;
                METAENGINE_Render_RectangleFilled(target, 0, 0, WIDTH, HEIGHT, {0, 0, 0, 255});
            } else if (fadeInStart > 0 && fadeInLength > 0) {

                float thru = 1 - (float) (now - fadeInStart) / fadeInLength;

                if (thru >= 0 && thru <= 1) {
                    METAENGINE_Render_RectangleFilled(target, 0, 0, WIDTH, HEIGHT, {0, 0, 0, (uint8) (thru * 255)});
                } else {
                    fadeInStart = 0;
                    fadeInLength = 0;
                }
            }


            if (fadeOutWaitFrames > 0) {
                fadeOutWaitFrames--;
                fadeOutStart = now;
            } else if (fadeOutStart > 0 && fadeOutLength > 0) {

                float thru = (float) (now - fadeOutStart) / fadeOutLength;

                if (thru >= 0 && thru <= 1) {
                    METAENGINE_Render_RectangleFilled(target, 0, 0, WIDTH, HEIGHT, {0, 0, 0, (uint8) (thru * 255)});
                } else {
                    METAENGINE_Render_RectangleFilled(target, 0, 0, WIDTH, HEIGHT, {0, 0, 0, 255});
                    fadeOutStart = 0;
                    fadeOutLength = 0;
                    fadeOutCallback();
                }
            }


            METAENGINE_Render_Flip(target);
        }

        //        if (Time::millis() - now < 2) {
        //#ifdef _WIN32
        //            Sleep(2 - (Time::millis() - now));
        //#else
        //            sleep((2 - (Time::millis() - now)) / 1000.0f);
        //#endif
        //        }


        frames++;
        if (now - lastFPS >= 1000) {
            lastFPS = now;
            //METADOT_INFO("{0:d} FPS", frames);
            if (networkMode == NetworkMode::SERVER) {
                METADOT_BUG("{0:d} peers connected.", server->server->connectedPeers);
            }
            fps = frames;
            dt_fps.w = -1;
            frames = 0;

            // calculate "feels like" fps

            float sum = 0;
            float num = 0.01;

            for (int i = 0; i < frameTimeNum; i++) {
                float weight = frameTime[i];
                sum += weight * frameTime[i];
                num += weight;
            }

            feelsLikeFps = 1000 / (sum / num);

            dt_feelsLikeFps.w = -1;
        }

        for (int i = 1; i < frameTimeNum; i++) {
            frameTime[i - 1] = frameTime[i];
        }
        frameTime[frameTimeNum - 1] = (uint16_t) (UTime::millis() - now);


        lastTime = now;
    }

exit:


    METADOT_INFO("Shutting down...");

    std::vector<std::future<void>> results = {};

    for (auto &p: world->chunkCache) {
        if (p.first == INT_MIN) continue;
        for (auto &p2: p.second) {
            if (p2.first == INT_MIN) continue;

            results.push_back(updateDirtyPool->push([&](int id) {
                Chunk *m = p2.second;
                world->unloadChunk(m);
            }));
        }
    }


    for (int i = 0; i < results.size(); i++) {
        results[i].get();
    }


    audioEngine.Shutdown();

    m_ImGuiLayer->onDetach();


    //soloud.destroyVoiceGroup(groupHandle);

    //soloud.deinit();

    for (MetaEngine::Module *l: *m_ModuleStack)
        l->onDetach();

    cr_plugin_close(MetaEngine::ctx);


    ClosePhysFS();

    // release resources & shutdown

    delete m_ImGuiLayer;
    delete objectDelete;
    delete backgrounds;
    delete terminal_log;

    running = false;

    fglUnloadOpenGL();

    if (networkMode != NetworkMode::SERVER) {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    METADOT_INFO("EXIT_SUCCESS");

    return EXIT_SUCCESS;
}

void Game::updateFrameEarly() {


    // handle controls


    if (Controls::DEBUG_UI->get()) {
        DebugDrawUI::visible ^= true;
        DebugCheatsUI::visible ^= true;
    }

    if (Settings::draw_frame_graph) {
        if (Controls::STATS_DISPLAY->get()) {
            Settings::draw_frame_graph = false;
            Settings::draw_debug_stats = false;
            Settings::draw_chunk_state = false;
            Settings::draw_detailed_material_info = false;
        }
    } else {
        if (Controls::STATS_DISPLAY->get()) {
            Settings::draw_frame_graph = true;
            Settings::draw_debug_stats = true;

            if (Controls::STATS_DISPLAY_DETAILED->get()) {
                Settings::draw_chunk_state = true;
                Settings::draw_detailed_material_info = true;
            }
        }
    }


    if (Controls::DEBUG_REFRESH->get()) {
        for (int x = 0; x < world->width; x++) {
            for (int y = 0; y < world->height; y++) {
                world->dirty[x + y * world->width] = true;
                world->layer2Dirty[x + y * world->width] = true;
                world->backgroundDirty[x + y * world->width] = true;
            }
        }
    }

    if (Controls::DEBUG_RIGID->get()) {
        for (auto &cur: world->rigidBodies) {
            if (cur->body->IsEnabled()) {
                float s = sin(cur->body->GetAngle());
                float c = cos(cur->body->GetAngle());
                bool upd = false;

                for (int xx = 0; xx < cur->matWidth; xx++) {
                    for (int yy = 0; yy < cur->matHeight; yy++) {
                        int tx = xx * c - yy * s + cur->body->GetPosition().x;
                        int ty = xx * s + yy * c + cur->body->GetPosition().y;

                        MaterialInstance tt = cur->tiles[xx + yy * cur->matWidth];
                        if (tt.mat->id != Materials::GENERIC_AIR.id) {
                            if (world->tiles[tx + ty * world->width].mat->id == Materials::GENERIC_AIR.id) {
                                world->tiles[tx + ty * world->width] = tt;
                                world->dirty[tx + ty * world->width] = true;
                            } else if (world->tiles[(tx + 1) + ty * world->width].mat->id == Materials::GENERIC_AIR.id) {
                                world->tiles[(tx + 1) + ty * world->width] = tt;
                                world->dirty[(tx + 1) + ty * world->width] = true;
                            } else if (world->tiles[(tx - 1) + ty * world->width].mat->id == Materials::GENERIC_AIR.id) {
                                world->tiles[(tx - 1) + ty * world->width] = tt;
                                world->dirty[(tx - 1) + ty * world->width] = true;
                            } else if (world->tiles[tx + (ty + 1) * world->width].mat->id == Materials::GENERIC_AIR.id) {
                                world->tiles[tx + (ty + 1) * world->width] = tt;
                                world->dirty[tx + (ty + 1) * world->width] = true;
                            } else if (world->tiles[tx + (ty - 1) * world->width].mat->id == Materials::GENERIC_AIR.id) {
                                world->tiles[tx + (ty - 1) * world->width] = tt;
                                world->dirty[tx + (ty - 1) * world->width] = true;
                            } else {
                                world->tiles[tx + ty * world->width] = Tiles::createObsidian(tx, ty);
                                world->dirty[tx + ty * world->width] = true;
                            }
                        }
                    }
                }

                if (upd) {
                    METAENGINE_Render_FreeImage(cur->texture);
                    cur->texture = METAENGINE_Render_CopyImageFromSurface(cur->surface);
                    METAENGINE_Render_SetImageFilter(cur->texture, METAENGINE_Render_FILTER_NEAREST);
                    //world->updateRigidBodyHitbox(cur);
                    cur->needsUpdate = true;
                }
            }

            world->b2world->DestroyBody(cur->body);
        }
        world->rigidBodies.clear();
    }

    if (Controls::DEBUG_UPDATE_WORLD_MESH->get()) {
        world->updateWorldMesh();
    }

    if (Controls::DEBUG_EXPLODE->get()) {
        int x = (int) ((mx - ofsX - camX) / scale);
        int y = (int) ((my - ofsY - camY) / scale);
        world->explosion(x, y, 30);
    }

    if (Controls::DEBUG_CARVE->get()) {
        // carve square

        int x = (int) ((mx - ofsX - camX) / scale - 16);
        int y = (int) ((my - ofsY - camY) / scale - 16);

        SDL_Surface *tex = SDL_CreateRGBSurfaceWithFormat(0, 32, 32, 32, SDL_PIXELFORMAT_ARGB8888);

        int n = 0;
        for (int xx = 0; xx < 32; xx++) {
            for (int yy = 0; yy < 32; yy++) {

                if (world->tiles[(x + xx) + (y + yy) * world->width].mat->physicsType == PhysicsType::SOLID) {
                    METADOT_GET_PIXEL(tex, xx, yy) = world->tiles[(x + xx) + (y + yy) * world->width].color;
                    world->tiles[(x + xx) + (y + yy) * world->width] = Tiles::NOTHING;
                    world->dirty[(x + xx) + (y + yy) * world->width] = true;
                    n++;
                }
            }
        }
        if (n > 0) {
            b2PolygonShape s;
            s.SetAsBox(1, 1);
            RigidBody *rb = world->makeRigidBody(b2_dynamicBody, (float) x, (float) y, 0, s, 1, (float) 0.3, tex);
            for (int tx = 0; tx < tex->w; tx++) {
                b2Filter bf = {};
                bf.categoryBits = 0x0002;
                bf.maskBits = 0x0001;
                rb->body->GetFixtureList()[0].SetFilterData(bf);
            }
            world->rigidBodies.push_back(rb);
            world->updateRigidBodyHitbox(rb);

            world->updateWorldMesh();
        }
    }

    if (Controls::DEBUG_BRUSHSIZE_INC->get()) {
        DebugDrawUI::brushSize = DebugDrawUI::brushSize < 50 ? DebugDrawUI::brushSize + 1 : DebugDrawUI::brushSize;
    }

    if (Controls::DEBUG_BRUSHSIZE_DEC->get()) {
        DebugDrawUI::brushSize = DebugDrawUI::brushSize > 1 ? DebugDrawUI::brushSize - 1 : DebugDrawUI::brushSize;
    }

    if (Controls::DEBUG_TOGGLE_PLAYER->get()) {
        if (world->player) {
            freeCamX = world->player->x + world->player->hw / 2.0f;
            freeCamY = world->player->y - world->player->hh / 2.0f;
            world->entities.erase(std::remove(world->entities.begin(), world->entities.end(), world->player), world->entities.end());
            world->b2world->DestroyBody(world->player->rb->body);
            delete world->player;
            world->player = nullptr;
        } else {
            Player *e = new Player();
            e->x = -world->loadZone.x + world->tickZone.x + world->tickZone.w / 2.0f;
            e->y = -world->loadZone.y + world->tickZone.y + world->tickZone.h / 2.0f;
            e->vx = 0;
            e->vy = 0;
            e->hw = 10;
            e->hh = 20;
            b2PolygonShape sh;
            sh.SetAsBox(e->hw / 2.0f + 1, e->hh / 2.0f);
            e->rb = world->makeRigidBody(b2BodyType::b2_kinematicBody, e->x + e->hw / 2.0f - 0.5, e->y + e->hh / 2.0f - 0.5, 0, sh, 1, 1, NULL);
            e->rb->body->SetGravityScale(0);
            e->rb->body->SetLinearDamping(0);
            e->rb->body->SetAngularDamping(0);

            Item *i3 = new Item();
            i3->setFlag(ItemFlags::VACUUM);
            i3->vacuumParticles = {};
            i3->surface = Textures::loadTexture("data/assets/objects/testVacuum.png");
            i3->texture = METAENGINE_Render_CopyImageFromSurface(i3->surface);
            METAENGINE_Render_SetImageFilter(i3->texture, METAENGINE_Render_FILTER_NEAREST);
            i3->pivotX = 6;
            e->setItemInHand(i3, world);

            b2Filter bf = {};
            bf.categoryBits = 0x0001;
            //bf.maskBits = 0x0000;
            e->rb->body->GetFixtureList()[0].SetFilterData(bf);

            world->entities.push_back(e);
            world->player = e;

            /*accLoadX = 0;
            accLoadY = 0;*/
        }
    }

    if (Controls::PAUSE->get()) {
        if (this->state == GameState::INGAME) {
            IngameUI::visible = !IngameUI::visible;
        }
    }


    audioEngine.Update();


    if (state == LOADING) {

    } else {
        audioEngine.SetEventParameter("event:/World/Sand", "Sand", 0);
        if (world->player && world->player->heldItem != NULL && world->player->heldItem->getFlag(ItemFlags::FLUID_CONTAINER)) {
            if (Controls::lmouse && world->player->heldItem->carry.size() > 0) {
                // shoot fluid from container

                int x = (int) (world->player->x + world->player->hw / 2.0f + world->loadZone.x + 10 * (float) cos((world->player->holdAngle + 180) * 3.1415f / 180.0f));
                int y = (int) (world->player->y + world->player->hh / 2.0f + world->loadZone.y + 10 * (float) sin((world->player->holdAngle + 180) * 3.1415f / 180.0f));

                MaterialInstance mat = world->player->heldItem->carry[world->player->heldItem->carry.size() - 1];
                world->player->heldItem->carry.pop_back();
                world->addParticle(new Particle(mat, (float) x, (float) y, (float) (world->player->vx / 2 + (rand() % 10 - 5) / 10.0f + 1.5f * (float) cos((world->player->holdAngle + 180) * 3.1415f / 180.0f)), (float) (world->player->vy / 2 + -(rand() % 5 + 5) / 10.0f + 1.5f * (float) sin((world->player->holdAngle + 180) * 3.1415f / 180.0f)), 0, (float) 0.1));

                int i = (int) world->player->heldItem->carry.size();
                i = (int) ((i / (float) world->player->heldItem->capacity) * world->player->heldItem->fill.size());
                UInt16Point pt = world->player->heldItem->fill[i];
                METADOT_GET_PIXEL(world->player->heldItem->surface, pt.x, pt.y) = 0x00;

                world->player->heldItem->texture = METAENGINE_Render_CopyImageFromSurface(world->player->heldItem->surface);
                METAENGINE_Render_SetImageFilter(world->player->heldItem->texture, METAENGINE_Render_FILTER_NEAREST);

                audioEngine.SetEventParameter("event:/World/Sand", "Sand", 1);

            } else {
                // pick up fluid into container

                float breakSize = world->player->heldItem->breakSize;

                int x = (int) (world->player->x + world->player->hw / 2.0f + world->loadZone.x + 10 * (float) cos((world->player->holdAngle + 180) * 3.1415f / 180.0f) - breakSize / 2);
                int y = (int) (world->player->y + world->player->hh / 2.0f + world->loadZone.y + 10 * (float) sin((world->player->holdAngle + 180) * 3.1415f / 180.0f) - breakSize / 2);

                int n = 0;
                for (int xx = 0; xx < breakSize; xx++) {
                    for (int yy = 0; yy < breakSize; yy++) {
                        if (world->player->heldItem->capacity == 0 || (world->player->heldItem->carry.size() < world->player->heldItem->capacity)) {
                            float cx = (float) ((xx / breakSize) - 0.5);
                            float cy = (float) ((yy / breakSize) - 0.5);

                            if (cx * cx + cy * cy > 0.25f) continue;

                            if (world->tiles[(x + xx) + (y + yy) * world->width].mat->physicsType == PhysicsType::SAND || world->tiles[(x + xx) + (y + yy) * world->width].mat->physicsType == PhysicsType::SOUP) {
                                world->player->heldItem->carry.push_back(world->tiles[(x + xx) + (y + yy) * world->width]);

                                int i = (int) world->player->heldItem->carry.size() - 1;
                                i = (int) ((i / (float) world->player->heldItem->capacity) * world->player->heldItem->fill.size());
                                UInt16Point pt = world->player->heldItem->fill[i];
                                Uint32 c = world->tiles[(x + xx) + (y + yy) * world->width].color;
                                METADOT_GET_PIXEL(world->player->heldItem->surface, pt.x, pt.y) = (world->tiles[(x + xx) + (y + yy) * world->width].mat->alpha << 24) + c;

                                world->player->heldItem->texture = METAENGINE_Render_CopyImageFromSurface(world->player->heldItem->surface);
                                METAENGINE_Render_SetImageFilter(world->player->heldItem->texture, METAENGINE_Render_FILTER_NEAREST);

                                world->tiles[(x + xx) + (y + yy) * world->width] = Tiles::NOTHING;
                                world->dirty[(x + xx) + (y + yy) * world->width] = true;
                                n++;
                            }
                        }
                    }
                }

                if (n > 0) {
                    audioEngine.PlayEvent("event:/Player/Impact");
                }
            }
        }

        // rigidbody hover


        int x = (int) ((mx - ofsX - camX) / scale);
        int y = (int) ((my - ofsY - camY) / scale);

        bool swapped = false;
        float hoverDelta = 10.0 * deltaTime / 1000.0;

        // this copies the vector
        std::vector<RigidBody *> rbs = world->rigidBodies;
        for (size_t i = 0; i < rbs.size(); i++) {
            RigidBody *cur = rbs[i];

            if (swapped) {
                cur->hover = (float) std::fmax(0, cur->hover - hoverDelta);
                continue;
            }

            bool connect = false;
            if (cur->body->IsEnabled()) {
                float s = sin(-cur->body->GetAngle());
                float c = cos(-cur->body->GetAngle());
                bool upd = false;
                for (float xx = -3; xx <= 3; xx += 0.5) {
                    for (float yy = -3; yy <= 3; yy += 0.5) {
                        if (abs(xx) + abs(yy) == 6) continue;
                        // rotate point

                        float tx = x + xx - cur->body->GetPosition().x;
                        float ty = y + yy - cur->body->GetPosition().y;

                        int ntx = (int) (tx * c - ty * s);
                        int nty = (int) (tx * s + ty * c);

                        if (cur->surface != nullptr) {
                            if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w && nty < cur->surface->h) {
                                if (((METADOT_GET_PIXEL(cur->surface, ntx, nty) >> 24) & 0xff) != 0x00) {
                                    connect = true;
                                }
                            }
                        } else {
                            METADOT_ERROR("cur->surface = nullptr");
                        }
                    }
                }
            }

            if (connect) {
                swapped = true;
                cur->hover = (float) std::fmin(1, cur->hover + hoverDelta);
            } else {
                cur->hover = (float) std::fmax(0, cur->hover - hoverDelta);
            }
        }


        // update world->tickZone

        world->tickZone = {CHUNK_W, CHUNK_H, world->width - CHUNK_W * 2, world->height - CHUNK_H * 2};
        if (world->tickZone.x + world->tickZone.w > world->width) {
            world->tickZone.x = world->width - world->tickZone.w;
        }

        if (world->tickZone.y + world->tickZone.h > world->height) {
            world->tickZone.y = world->height - world->tickZone.h;
        }
    }
}

void Game::tick() {


    //METADOT_BUG("{0:d} {0:d}", accLoadX, accLoadY);
    if (state == LOADING) {
        if (world) {
            // tick chunkloading
            world->frame();
            if (world->readyToMerge.size() == 0 && fadeOutStart == 0) {
                fadeOutStart = now;
                fadeOutLength = 250;
                fadeOutCallback = [&]() {
                    fadeInStart = now;
                    fadeInLength = 500;
                    fadeInWaitFrames = 4;
                    state = stateAfterLoad;
                };

                setWindowFlash(WindowFlashAction::START_COUNT, 1, 333);
            }
        }
    } else {

        int lastReadyToMergeSize = (int) world->readyToMerge.size();

        // check chunk loading
        tickChunkLoading();

        if (world->needToTickGeneration) world->tickChunkGeneration();

        // clear objects
        METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_Clear(textureObjects->target);

        METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_Clear(textureObjectsLQ->target);

        METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_Clear(textureObjectsBack->target);

        if (Settings::tick_world && world->readyToMerge.size() == 0) {
            world->tickChunks();
        }

        // render objects


        memset(objectDelete, false, (size_t) world->width * world->height * sizeof(bool));


        METAENGINE_Render_SetBlendMode(textureObjects, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_SetBlendMode(textureObjectsLQ, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_SetBlendMode(textureObjectsBack, METAENGINE_Render_BLEND_NORMAL);

        for (size_t i = 0; i < world->rigidBodies.size(); i++) {
            RigidBody *cur = world->rigidBodies[i];
            if (cur == nullptr) continue;
            if (cur->surface == nullptr) continue;
            if (!cur->body->IsEnabled()) continue;

            float x = cur->body->GetPosition().x;
            float y = cur->body->GetPosition().y;

            // draw

            METAENGINE_Render_Target *tgt = cur->back ? textureObjectsBack->target : textureObjects->target;
            METAENGINE_Render_Target *tgtLQ = cur->back ? textureObjectsBack->target : textureObjectsLQ->target;
            int scaleObjTex = Settings::hd_objects ? Settings::hd_objects_size : 1;

            METAENGINE_Render_Rect r = {x * scaleObjTex, y * scaleObjTex, (float) cur->surface->w * scaleObjTex, (float) cur->surface->h * scaleObjTex};

            if (cur->texNeedsUpdate) {
                if (cur->texture != nullptr) {
                    METAENGINE_Render_FreeImage(cur->texture);
                }
                cur->texture = METAENGINE_Render_CopyImageFromSurface(cur->surface);
                METAENGINE_Render_SetImageFilter(cur->texture, METAENGINE_Render_FILTER_NEAREST);
                cur->texNeedsUpdate = false;
            }

            METAENGINE_Render_BlitRectX(cur->texture, NULL, tgt, &r, cur->body->GetAngle() * 180 / (float) M_PI, 0, 0, METAENGINE_Render_FLIP_NONE);


            // draw outline

            Uint8 outlineAlpha = (Uint8) (cur->hover * 255);
            if (outlineAlpha > 0) {
                SDL_Color col = {0xff, 0xff, 0x80, outlineAlpha};
                METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_NORMAL_FACTOR_ALPHA);// SDL_BLENDMODE_BLEND
                for (auto &l: cur->outline) {
                    b2Vec2 *vec = new b2Vec2[l.GetNumPoints()];
                    for (int j = 0; j < l.GetNumPoints(); j++) {
                        vec[j] = {(float) l.GetPoint(j).x / scale, (float) l.GetPoint(j).y / scale};
                    }
                    Drawing::drawPolygon(tgtLQ, col, vec, (int) x, (int) y, scale, l.GetNumPoints(), cur->body->GetAngle(), 0, 0);
                    delete[] vec;
                }
                METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_NORMAL);// SDL_BLENDMODE_NONE
            }


            // displace fluids

            float s = sin(cur->body->GetAngle());
            float c = cos(cur->body->GetAngle());

            std::vector<std::pair<int, int>> checkDirs = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};

            for (int tx = 0; tx < cur->matWidth; tx++) {
                for (int ty = 0; ty < cur->matHeight; ty++) {
                    MaterialInstance rmat = cur->tiles[tx + ty * cur->matWidth];
                    if (rmat.mat->id == Materials::GENERIC_AIR.id) continue;

                    // rotate point
                    int wx = (int) (tx * c - (ty + 1) * s + x);
                    int wy = (int) (tx * s + (ty + 1) * c + y);

                    for (auto &dir: checkDirs) {
                        int wxd = wx + dir.first;
                        int wyd = wy + dir.second;

                        if (wxd < 0 || wyd < 0 || wxd >= world->width || wyd >= world->height) continue;
                        if (world->tiles[wxd + wyd * world->width].mat->physicsType == PhysicsType::AIR) {
                            world->tiles[wxd + wyd * world->width] = rmat;
                            world->dirty[wxd + wyd * world->width] = true;
                            //objectDelete[wxd + wyd * world->width] = true;
                            break;
                        } else if (world->tiles[wxd + wyd * world->width].mat->physicsType == PhysicsType::SAND) {
                            world->addParticle(new Particle(world->tiles[wxd + wyd * world->width], (float) wxd, (float) (wyd - 3), (float) ((rand() % 10 - 5) / 10.0f), (float) (-(rand() % 5 + 5) / 10.0f), 0, (float) 0.1));
                            world->tiles[wxd + wyd * world->width] = rmat;
                            //objectDelete[wxd + wyd * world->width] = true;
                            world->dirty[wxd + wyd * world->width] = true;
                            cur->body->SetLinearVelocity({cur->body->GetLinearVelocity().x * (float) 0.99, cur->body->GetLinearVelocity().y * (float) 0.99});
                            cur->body->SetAngularVelocity(cur->body->GetAngularVelocity() * (float) 0.98);
                            break;
                        } else if (world->tiles[wxd + wyd * world->width].mat->physicsType == PhysicsType::SOUP) {
                            world->addParticle(new Particle(world->tiles[wxd + wyd * world->width], (float) wxd, (float) (wyd - 3), (float) ((rand() % 10 - 5) / 10.0f), (float) (-(rand() % 5 + 5) / 10.0f), 0, (float) 0.1));
                            world->tiles[wxd + wyd * world->width] = rmat;
                            //objectDelete[wxd + wyd * world->width] = true;
                            world->dirty[wxd + wyd * world->width] = true;
                            cur->body->SetLinearVelocity({cur->body->GetLinearVelocity().x * (float) 0.998, cur->body->GetLinearVelocity().y * (float) 0.998});
                            cur->body->SetAngularVelocity(cur->body->GetAngularVelocity() * (float) 0.99);
                            break;
                        }
                    }
                }
            }
        }


        // render entities


        if (lastReadyToMergeSize == 0) {
            world->tickEntities(textureEntities->target);

            if (world->player) {
                if (world->player->holdHammer) {
                    int x = (int) ((mx - ofsX - camX) / scale);
                    int y = (int) ((my - ofsY - camY) / scale);
                    METAENGINE_Render_Line(textureEntitiesLQ->target, x, y, world->player->hammerX, world->player->hammerY, {0xff, 0xff, 0x00, 0xff});
                }
            }
        }
        METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_NORMAL);// SDL_BLENDMODE_NONE


        // entity fluid displacement & make solid

        for (size_t i = 0; i < world->entities.size(); i++) {
            Entity cur = *world->entities[i];

            for (int tx = 0; tx < cur.hw; tx++) {
                for (int ty = 0; ty < cur.hh; ty++) {

                    int wx = (int) (tx + cur.x + world->loadZone.x);
                    int wy = (int) (ty + cur.y + world->loadZone.y);
                    if (wx < 0 || wy < 0 || wx >= world->width || wy >= world->height) continue;
                    if (world->tiles[wx + wy * world->width].mat->physicsType == PhysicsType::AIR) {
                        world->tiles[wx + wy * world->width] = Tiles::OBJECT;
                        objectDelete[wx + wy * world->width] = true;
                    } else if (world->tiles[wx + wy * world->width].mat->physicsType == PhysicsType::SAND || world->tiles[wx + wy * world->width].mat->physicsType == PhysicsType::SOUP) {
                        world->addParticle(new Particle(world->tiles[wx + wy * world->width], (float) (wx + rand() % 3 - 1 - cur.vx), (float) (wy - abs(cur.vy)), (float) (-cur.vx / 4 + (rand() % 10 - 5) / 5.0f), (float) (-cur.vy / 4 + -(rand() % 5 + 5) / 5.0f), 0, (float) 0.1));
                        world->tiles[wx + wy * world->width] = Tiles::OBJECT;
                        objectDelete[wx + wy * world->width] = true;
                        world->dirty[wx + wy * world->width] = true;
                    }
                }
            }
        }


        if (Settings::tick_world && world->readyToMerge.size() == 0) {
            world->tick();
        }

        if (Controls::DEBUG_TICK->get()) {
            world->tick();
        }

        // player movement
        tickPlayer();

        // update particles, tickObjects, update dirty
        // TODO: this is not entirely thread safe since tickParticles changes World::tiles and World::dirty


        bool hadDirty = false;
        bool hadLayer2Dirty = false;
        bool hadBackgroundDirty = false;
        bool hadFire = false;
        bool hadFlow = false;

        //int pitch;
        //void* vdpixels_ar = texture->data;
        //unsigned char* dpixels_ar = (unsigned char*)vdpixels_ar;
        unsigned char *dpixels_ar = pixels_ar;
        unsigned char *dpixelsFire_ar = pixelsFire_ar;
        unsigned char *dpixelsFlow_ar = pixelsFlow_ar;
        unsigned char *dpixelsEmission_ar = pixelsEmission_ar;

        std::vector<std::future<void>> results = {};

        results.push_back(updateDirtyPool->push([&](int id) {
            //SDL_SetRenderTarget(renderer, textureParticles);
            void *particlePixels = pixelsParticles_ar;

            memset(particlePixels, 0, (size_t) world->width * world->height * 4);

            world->renderParticles((unsigned char **) &particlePixels);
            world->tickParticles();

            //SDL_SetRenderTarget(renderer, NULL);
        }));

        if (world->readyToMerge.size() == 0) {
            results.push_back(updateDirtyPool->push([&](int id) {
                world->tickObjectBounds();
            }));
        }


        for (int i = 0; i < results.size(); i++) {
            results[i].get();
        }


        for (size_t i = 0; i < world->rigidBodies.size(); i++) {
            RigidBody *cur = world->rigidBodies[i];
            if (cur == nullptr) continue;
            if (cur->surface == nullptr) continue;
            if (!cur->body->IsEnabled()) continue;

            float x = cur->body->GetPosition().x;
            float y = cur->body->GetPosition().y;

            float s = sin(cur->body->GetAngle());
            float c = cos(cur->body->GetAngle());

            std::vector<std::pair<int, int>> checkDirs = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (int tx = 0; tx < cur->matWidth; tx++) {
                for (int ty = 0; ty < cur->matHeight; ty++) {
                    MaterialInstance rmat = cur->tiles[tx + ty * cur->matWidth];
                    if (rmat.mat->id == Materials::GENERIC_AIR.id) continue;

                    // rotate point
                    int wx = (int) (tx * c - (ty + 1) * s + x);
                    int wy = (int) (tx * s + (ty + 1) * c + y);

                    bool found = false;
                    for (auto &dir: checkDirs) {
                        int wxd = wx + dir.first;
                        int wyd = wy + dir.second;

                        if (wxd < 0 || wyd < 0 || wxd >= world->width || wyd >= world->height) continue;
                        if (world->tiles[wxd + wyd * world->width] == rmat) {
                            cur->tiles[tx + ty * cur->matWidth] = world->tiles[wxd + wyd * world->width];
                            world->tiles[wxd + wyd * world->width] = Tiles::NOTHING;
                            world->dirty[wxd + wyd * world->width] = true;
                            found = true;

                            //for(int dxx = -1; dxx <= 1; dxx++) {
                            //    for(int dyy = -1; dyy <= 1; dyy++) {
                            //        if(world->tiles[(wxd + dxx) + (wyd + dyy) * world->width].mat->physicsType == PhysicsType::SAND || world->tiles[(wxd + dxx) + (wyd + dyy) * world->width].mat->physicsType == PhysicsType::SOUP) {
                            //            uint32_t color = world->tiles[(wxd + dxx) + (wyd + dyy) * world->width].color;

                            //            unsigned int offset = ((wxd + dxx) + (wyd + dyy) * world->width) * 4;

                            //            dpixels_ar[offset + 2] = ((color >> 0) & 0xff);        // b
                            //            dpixels_ar[offset + 1] = ((color >> 8) & 0xff);        // g
                            //            dpixels_ar[offset + 0] = ((color >> 16) & 0xff);        // r
                            //            dpixels_ar[offset + 3] = world->tiles[(wxd + dxx) + (wyd + dyy) * world->width].mat->alpha;    // a
                            //        }
                            //    }
                            //}

                            //if(!Settings::draw_load_zones) {
                            //    unsigned int offset = (wxd + wyd * world->width) * 4;
                            //    dpixels_ar[offset + 2] = 0;        // b
                            //    dpixels_ar[offset + 1] = 0;        // g
                            //    dpixels_ar[offset + 0] = 0xff;        // r
                            //    dpixels_ar[offset + 3] = 0xff;    // a
                            //}


                            break;
                        }
                    }

                    if (!found) {
                        if (world->tiles[wx + wy * world->width].mat->id == Materials::GENERIC_AIR.id) {
                            cur->tiles[tx + ty * cur->matWidth] = Tiles::NOTHING;
                        }
                    }
                }
            }

            for (int tx = 0; tx < cur->surface->w; tx++) {
                for (int ty = 0; ty < cur->surface->h; ty++) {
                    MaterialInstance mat = cur->tiles[tx + ty * cur->surface->w];
                    if (mat.mat->id == Materials::GENERIC_AIR.id) {
                        METADOT_GET_PIXEL(cur->surface, tx, ty) = 0x00000000;
                    } else {
                        METADOT_GET_PIXEL(cur->surface, tx, ty) = (mat.mat->alpha << 24) + (mat.color & 0x00ffffff);
                    }
                }
            }

            METAENGINE_Render_FreeImage(cur->texture);
            cur->texture = METAENGINE_Render_CopyImageFromSurface(cur->surface);
            METAENGINE_Render_SetImageFilter(cur->texture, METAENGINE_Render_FILTER_NEAREST);

            cur->needsUpdate = true;
        }

        if (world->readyToMerge.size() == 0) {
            if (Settings::tick_box2d) world->tickObjects();
        }

        if (tickTime % 10 == 0) world->tickObjectsMesh();

        for (int i = 0; i < Materials::nMaterials; i++) movingTiles[i] = 0;

        results.clear();
        results.push_back(updateDirtyPool->push([&](int id) {
            for (int i = 0; i < world->width * world->height; i++) {
                const unsigned int offset = i * 4;

                if (world->dirty[i]) {
                    hadDirty = true;
                    movingTiles[world->tiles[i].mat->id]++;
                    if (world->tiles[i].mat->physicsType == PhysicsType::AIR) {
                        dpixels_ar[offset + 0] = 0;                    // b
                        dpixels_ar[offset + 1] = 0;                    // g
                        dpixels_ar[offset + 2] = 0;                    // r
                        dpixels_ar[offset + 3] = SDL_ALPHA_TRANSPARENT;// a

                        dpixelsFire_ar[offset + 0] = 0;                    // b
                        dpixelsFire_ar[offset + 1] = 0;                    // g
                        dpixelsFire_ar[offset + 2] = 0;                    // r
                        dpixelsFire_ar[offset + 3] = SDL_ALPHA_TRANSPARENT;// a

                        dpixelsEmission_ar[offset + 0] = 0;                    // b
                        dpixelsEmission_ar[offset + 1] = 0;                    // g
                        dpixelsEmission_ar[offset + 2] = 0;                    // r
                        dpixelsEmission_ar[offset + 3] = SDL_ALPHA_TRANSPARENT;// a

                        world->flowY[i] = 0;
                        world->flowX[i] = 0;
                    } else {
                        Uint32 color = world->tiles[i].color;
                        Uint32 emit = world->tiles[i].mat->emitColor;
                        //float br = world->light[i];
                        dpixels_ar[offset + 2] = ((color >> 0) & 0xff);     // b
                        dpixels_ar[offset + 1] = ((color >> 8) & 0xff);     // g
                        dpixels_ar[offset + 0] = ((color >> 16) & 0xff);    // r
                        dpixels_ar[offset + 3] = world->tiles[i].mat->alpha;// a

                        dpixelsEmission_ar[offset + 2] = ((emit >> 0) & 0xff); // b
                        dpixelsEmission_ar[offset + 1] = ((emit >> 8) & 0xff); // g
                        dpixelsEmission_ar[offset + 0] = ((emit >> 16) & 0xff);// r
                        dpixelsEmission_ar[offset + 3] = ((emit >> 24) & 0xff);// a

                        if (world->tiles[i].mat->id == Materials::FIRE.id) {
                            dpixelsFire_ar[offset + 2] = ((color >> 0) & 0xff);     // b
                            dpixelsFire_ar[offset + 1] = ((color >> 8) & 0xff);     // g
                            dpixelsFire_ar[offset + 0] = ((color >> 16) & 0xff);    // r
                            dpixelsFire_ar[offset + 3] = world->tiles[i].mat->alpha;// a
                            hadFire = true;
                        }
                        if (world->tiles[i].mat->physicsType == PhysicsType::SOUP) {

                            float newFlowX = world->prevFlowX[i] + (world->flowX[i] - world->prevFlowX[i]) * 0.25;
                            float newFlowY = world->prevFlowY[i] + (world->flowY[i] - world->prevFlowY[i]) * 0.25;
                            if (newFlowY < 0) newFlowY *= 0.5;

                            dpixelsFlow_ar[offset + 2] = 0;                                                                                                       // b
                            dpixelsFlow_ar[offset + 1] = std::min(std::max(newFlowY * (3.0 / world->tiles[i].mat->iterations + 0.5) / 4.0 + 0.5, 0.0), 1.0) * 255;// g
                            dpixelsFlow_ar[offset + 0] = std::min(std::max(newFlowX * (3.0 / world->tiles[i].mat->iterations + 0.5) / 4.0 + 0.5, 0.0), 1.0) * 255;// r
                            dpixelsFlow_ar[offset + 3] = 0xff;                                                                                                    // a
                            hadFlow = true;
                            world->prevFlowX[i] = newFlowX;
                            world->prevFlowY[i] = newFlowY;
                            world->flowY[i] = 0;
                            world->flowX[i] = 0;
                        } else {
                            world->flowY[i] = 0;
                            world->flowX[i] = 0;
                        }
                    }
                }
            }
        }));

        //void* vdpixelsLayer2_ar = textureLayer2->data;
        //unsigned char* dpixelsLayer2_ar = (unsigned char*)vdpixelsLayer2_ar;
        unsigned char *dpixelsLayer2_ar = pixelsLayer2_ar;
        results.push_back(updateDirtyPool->push([&](int id) {
            for (int i = 0; i < world->width * world->height; i++) {
                /*for (int x = 0; x < world->width; x++) {
                    for (int y = 0; y < world->height; y++) {*/
                //const unsigned int i = x + y * world->width;
                const unsigned int offset = i * 4;
                if (world->layer2Dirty[i]) {
                    hadLayer2Dirty = true;
                    if (world->layer2[i].mat->physicsType == PhysicsType::AIR) {
                        if (Settings::draw_background_grid) {
                            Uint32 color = ((i) % 2) == 0 ? 0x888888 : 0x444444;
                            dpixelsLayer2_ar[offset + 2] = (color >> 0) & 0xff; // b
                            dpixelsLayer2_ar[offset + 1] = (color >> 8) & 0xff; // g
                            dpixelsLayer2_ar[offset + 0] = (color >> 16) & 0xff;// r
                            dpixelsLayer2_ar[offset + 3] = SDL_ALPHA_OPAQUE;    // a
                            continue;
                        } else {
                            dpixelsLayer2_ar[offset + 0] = 0;                    // b
                            dpixelsLayer2_ar[offset + 1] = 0;                    // g
                            dpixelsLayer2_ar[offset + 2] = 0;                    // r
                            dpixelsLayer2_ar[offset + 3] = SDL_ALPHA_TRANSPARENT;// a
                            continue;
                        }
                    }
                    Uint32 color = world->layer2[i].color;
                    dpixelsLayer2_ar[offset + 2] = (color >> 0) & 0xff;        // b
                    dpixelsLayer2_ar[offset + 1] = (color >> 8) & 0xff;        // g
                    dpixelsLayer2_ar[offset + 0] = (color >> 16) & 0xff;       // r
                    dpixelsLayer2_ar[offset + 3] = world->layer2[i].mat->alpha;// a
                }
            }
        }));

        //void* vdpixelsBackground_ar = textureBackground->data;
        //unsigned char* dpixelsBackground_ar = (unsigned char*)vdpixelsBackground_ar;
        unsigned char *dpixelsBackground_ar = pixelsBackground_ar;
        results.push_back(updateDirtyPool->push([&](int id) {
            for (int i = 0; i < world->width * world->height; i++) {
                /*for (int x = 0; x < world->width; x++) {
                    for (int y = 0; y < world->height; y++) {*/
                //const unsigned int i = x + y * world->width;
                const unsigned int offset = i * 4;

                if (world->backgroundDirty[i]) {
                    hadBackgroundDirty = true;
                    Uint32 color = world->background[i];
                    dpixelsBackground_ar[offset + 2] = (color >> 0) & 0xff; // b
                    dpixelsBackground_ar[offset + 1] = (color >> 8) & 0xff; // g
                    dpixelsBackground_ar[offset + 0] = (color >> 16) & 0xff;// r
                    dpixelsBackground_ar[offset + 3] = (color >> 24) & 0xff;// a
                }

                //}
            }
        }));


        for (int i = 0; i < world->width * world->height; i++) {
            /*for (int x = 0; x < world->width; x++) {
                for (int y = 0; y < world->height; y++) {*/
            //const unsigned int i = x + y * world->width;
            const unsigned int offset = i * 4;

            if (objectDelete[i]) {
                world->tiles[i] = Tiles::NOTHING;
            }
        }


        //results.push_back(updateDirtyPool->push([&](int id) {

        //}));

        for (int i = 0; i < results.size(); i++) {
            results[i].get();
        }


        updateMaterialSounds();


        METAENGINE_Render_UpdateImageBytes(
                textureParticles,
                NULL,
                &pixelsParticles_ar[0],
                world->width * 4);


        if (hadDirty) memset(world->dirty, false, (size_t) world->width * world->height);
        if (hadLayer2Dirty) memset(world->layer2Dirty, false, (size_t) world->width * world->height);
        if (hadBackgroundDirty) memset(world->backgroundDirty, false, (size_t) world->width * world->height);


        if (Settings::tick_temperature && tickTime % 4 == 2) {
            world->tickTemperature();
        }
        if (Settings::draw_temperature_map && tickTime % 4 == 0) {
            renderTemperatureMap(world);
        }


        if (hadDirty) {
            METAENGINE_Render_UpdateImageBytes(
                    texture,
                    NULL,
                    &pixels[0],
                    world->width * 4);

            METAENGINE_Render_UpdateImageBytes(
                    emissionTexture,
                    NULL,
                    &pixelsEmission[0],
                    world->width * 4);
        }

        if (hadLayer2Dirty) {
            METAENGINE_Render_UpdateImageBytes(
                    textureLayer2,
                    NULL,
                    &pixelsLayer2[0],
                    world->width * 4);
        }

        if (hadBackgroundDirty) {
            METAENGINE_Render_UpdateImageBytes(
                    textureBackground,
                    NULL,
                    &pixelsBackground[0],
                    world->width * 4);
        }

        if (hadFlow) {
            METAENGINE_Render_UpdateImageBytes(
                    textureFlow,
                    NULL,
                    &pixelsFlow[0],
                    world->width * 4);

            waterFlowPassShader->dirty = true;
        }

        if (hadFire) {
            METAENGINE_Render_UpdateImageBytes(
                    textureFire,
                    NULL,
                    &pixelsFire[0],
                    world->width * 4);
        }

        if (Settings::draw_temperature_map) {
            METAENGINE_Render_UpdateImageBytes(
                    temperatureMap,
                    NULL,
                    &pixelsTemp[0],
                    world->width * 4);
        }

        /*METAENGINE_Render_UpdateImageBytes(
            textureObjects,
            NULL,
            &pixelsObjects[0],
            world->width * 4
        );*/


        if (Settings::tick_box2d && tickTime % 4 == 0) world->updateWorldMesh();
    }
}

void Game::tickChunkLoading() {


    // if need to load chunks
    if ((abs(accLoadX) > CHUNK_W / 2 || abs(accLoadY) > CHUNK_H / 2)) {
        while (world->toLoad.size() > 0) {
            // tick chunkloading
            world->frame();
        }

        //iterate


        for (int i = 0; i < world->width * world->height; i++) {
            const unsigned int offset = i * 4;

#define UCH_SET_PIXEL(pix_ar, ofs, c_r, c_g, c_b, c_a) \
    pix_ar[ofs + 0] = c_b;                             \
    pix_ar[ofs + 1] = c_g;                             \
    pix_ar[ofs + 2] = c_r;                             \
    pix_ar[ofs + 3] = c_a;

            if (world->dirty[i]) {
                if (world->tiles[i].mat->physicsType == PhysicsType::AIR) {
                    UCH_SET_PIXEL(pixels_ar, offset, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
                } else {
                    Uint32 color = world->tiles[i].color;
                    Uint32 emit = world->tiles[i].mat->emitColor;
                    UCH_SET_PIXEL(pixels_ar, offset, (color >> 0) & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff, world->tiles[i].mat->alpha);
                    UCH_SET_PIXEL(pixelsEmission_ar, offset, (emit >> 0) & 0xff, (emit >> 8) & 0xff, (emit >> 16) & 0xff, (emit >> 24) & 0xff);
                }
            }

            if (world->layer2Dirty[i]) {
                if (world->layer2[i].mat->physicsType == PhysicsType::AIR) {
                    if (Settings::draw_background_grid) {
                        Uint32 color = ((i) % 2) == 0 ? 0x888888 : 0x444444;
                        UCH_SET_PIXEL(pixelsLayer2_ar, offset, (color >> 0) & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff, SDL_ALPHA_OPAQUE);
                    } else {
                        UCH_SET_PIXEL(pixelsLayer2_ar, offset, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
                    }
                    continue;
                }
                Uint32 color = world->layer2[i].color;
                UCH_SET_PIXEL(pixelsLayer2_ar, offset, (color >> 0) & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff, world->layer2[i].mat->alpha);
            }

            if (world->backgroundDirty[i]) {
                Uint32 color = world->background[i];
                UCH_SET_PIXEL(pixelsBackground_ar, offset, (color >> 0) & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff, (color >> 24) & 0xff);
            }
#undef UCH_SET_PIXEL
        }


        memset(world->dirty, false, (size_t) world->width * world->height);
        memset(world->layer2Dirty, false, (size_t) world->width * world->height);
        memset(world->backgroundDirty, false, (size_t) world->width * world->height);


        while ((abs(accLoadX) > CHUNK_W / 2 || abs(accLoadY) > CHUNK_H / 2)) {
            int subX = std::fmax(std::fmin(accLoadX, CHUNK_W / 2), -CHUNK_W / 2);
            if (abs(subX) < CHUNK_W / 2) subX = 0;
            int subY = std::fmax(std::fmin(accLoadY, CHUNK_H / 2), -CHUNK_H / 2);
            if (abs(subY) < CHUNK_H / 2) subY = 0;

            world->loadZone.x += subX;
            world->loadZone.y += subY;

            int delta = 4 * (subX + subY * world->width);

            std::vector<std::future<void>> results = {};
            if (delta > 0) {
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(pixels_ar[0]), &(pixels_ar[world->width * world->height * 4]) - delta, &(pixels_ar[world->width * world->height * 4]));
                    //rotate(pixels.begin(), pixels.end() - delta, pixels.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(pixelsLayer2_ar[0]), &(pixelsLayer2_ar[world->width * world->height * 4]) - delta, &(pixelsLayer2_ar[world->width * world->height * 4]));
                    //rotate(pixelsLayer2.begin(), pixelsLayer2.end() - delta, pixelsLayer2.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(pixelsBackground_ar[0]), &(pixelsBackground_ar[world->width * world->height * 4]) - delta, &(pixelsBackground_ar[world->width * world->height * 4]));
                    //rotate(pixelsBackground.begin(), pixelsBackground.end() - delta, pixelsBackground.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(pixelsFire_ar[0]), &(pixelsFire_ar[world->width * world->height * 4]) - delta, &(pixelsFire_ar[world->width * world->height * 4]));
                    //rotate(pixelsFire_ar.begin(), pixelsFire_ar.end() - delta, pixelsFire_ar.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(pixelsFlow_ar[0]), &(pixelsFlow_ar[world->width * world->height * 4]) - delta, &(pixelsFlow_ar[world->width * world->height * 4]));
                    //rotate(pixelsFlow_ar.begin(), pixelsFlow_ar.end() - delta, pixelsFlow_ar.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(pixelsEmission_ar[0]), &(pixelsEmission_ar[world->width * world->height * 4]) - delta, &(pixelsEmission_ar[world->width * world->height * 4]));
                    //rotate(pixelsEmission_ar.begin(), pixelsEmission_ar.end() - delta, pixelsEmission_ar.end());
                }));
            } else if (delta < 0) {
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(pixels_ar[0]), &(pixels_ar[0]) - delta, &(pixels_ar[world->width * world->height * 4]));
                    //rotate(pixels.begin(), pixels.begin() - delta, pixels.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(pixelsLayer2_ar[0]), &(pixelsLayer2_ar[0]) - delta, &(pixelsLayer2_ar[world->width * world->height * 4]));
                    //rotate(pixelsLayer2.begin(), pixelsLayer2.begin() - delta, pixelsLayer2.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(pixelsBackground_ar[0]), &(pixelsBackground_ar[0]) - delta, &(pixelsBackground_ar[world->width * world->height * 4]));
                    //rotate(pixelsBackground.begin(), pixelsBackground.begin() - delta, pixelsBackground.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(pixelsFire_ar[0]), &(pixelsFire_ar[0]) - delta, &(pixelsFire_ar[world->width * world->height * 4]));
                    //rotate(pixelsFire_ar.begin(), pixelsFire_ar.begin() - delta, pixelsFire_ar.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(pixelsFlow_ar[0]), &(pixelsFlow_ar[0]) - delta, &(pixelsFlow_ar[world->width * world->height * 4]));
                    //rotate(pixelsFlow_ar.begin(), pixelsFlow_ar.begin() - delta, pixelsFlow_ar.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(pixelsEmission_ar[0]), &(pixelsEmission_ar[0]) - delta, &(pixelsEmission_ar[world->width * world->height * 4]));
                    //rotate(pixelsEmission_ar.begin(), pixelsEmission_ar.begin() - delta, pixelsEmission_ar.end());
                }));
            }

            for (auto &v: results) {
                v.get();
            }


#define CLEARPIXEL(pixels, ofs)                                 \
    pixels[ofs + 0] = pixels[ofs + 1] = pixels[ofs + 2] = 0xff; \
    pixels[ofs + 3] = SDL_ALPHA_TRANSPARENT;


            for (int x = 0; x < abs(subX); x++) {
                for (int y = 0; y < world->height; y++) {
                    const unsigned int offset = (world->width * 4 * y) + x * 4;
                    if (offset < world->width * world->height * 4) {
                        CLEARPIXEL(pixels_ar, offset);
                        CLEARPIXEL(pixelsLayer2_ar, offset);
                        CLEARPIXEL(pixelsObjects_ar, offset);
                        CLEARPIXEL(pixelsBackground_ar, offset);
                        CLEARPIXEL(pixelsFire_ar, offset);
                        CLEARPIXEL(pixelsFlow_ar, offset);
                        CLEARPIXEL(pixelsEmission_ar, offset);
                    }
                }
            }


            for (int y = 0; y < abs(subY); y++) {
                for (int x = 0; x < world->width; x++) {
                    const unsigned int offset = (world->width * 4 * y) + x * 4;
                    if (offset < world->width * world->height * 4) {
                        CLEARPIXEL(pixels_ar, offset);
                        CLEARPIXEL(pixelsLayer2_ar, offset);
                        CLEARPIXEL(pixelsObjects_ar, offset);
                        CLEARPIXEL(pixelsBackground_ar, offset);
                        CLEARPIXEL(pixelsFire_ar, offset);
                        CLEARPIXEL(pixelsFlow_ar, offset);
                        CLEARPIXEL(pixelsEmission_ar, offset);
                    }
                }
            }


#undef CLEARPIXEL

            accLoadX -= subX;
            accLoadY -= subY;

            ofsX -= subX * scale;
            ofsY -= subY * scale;
        }


        world->tickChunks();
        world->updateWorldMesh();
        world->dirty[0] = true;
        world->layer2Dirty[0] = true;
        world->backgroundDirty[0] = true;

    } else {
        world->frame();
    }
}

void Game::tickPlayer() {


    if (world->player) {
        if (Controls::PLAYER_UP->get() && !Controls::DEBUG_DRAW->get()) {
            if (world->player->ground) {
                world->player->vy = -4;
                audioEngine.PlayEvent("event:/Player/Jump");
            }
        }

        world->player->vy += (float) (((Controls::PLAYER_UP->get() && !Controls::DEBUG_DRAW->get()) ? (world->player->vy > -1 ? -0.8 : -0.35) : 0) + (Controls::PLAYER_DOWN->get() ? 0.1 : 0));
        if (Controls::PLAYER_UP->get() && !Controls::DEBUG_DRAW->get()) {
            audioEngine.SetEventParameter("event:/Player/Fly", "Intensity", 1);
            for (int i = 0; i < 4; i++) {
                Particle *p = new Particle(Tiles::createLava(), (float) (world->player->x + world->loadZone.x + world->player->hw / 2 + rand() % 5 - 2 + world->player->vx), (float) (world->player->y + world->loadZone.y + world->player->hh + world->player->vy), (float) ((rand() % 10 - 5) / 10.0f + world->player->vx / 2.0f), (float) ((rand() % 10) / 10.0f + 1 + world->player->vy / 2.0f), 0, (float) 0.025);
                p->temporary = true;
                p->lifetime = 120;
                world->addParticle(p);
            }
        } else {
            audioEngine.SetEventParameter("event:/Player/Fly", "Intensity", 0);
        }

        if (world->player->vy > 0) {
            audioEngine.SetEventParameter("event:/Player/Wind", "Wind", (float) (world->player->vy / 12.0));
        } else {
            audioEngine.SetEventParameter("event:/Player/Wind", "Wind", 0);
        }

        world->player->vx += (float) ((Controls::PLAYER_LEFT->get() ? (world->player->vx > 0 ? -0.4 : -0.2) : 0) + (Controls::PLAYER_RIGHT->get() ? (world->player->vx < 0 ? 0.4 : 0.2) : 0));
        if (!Controls::PLAYER_LEFT->get() && !Controls::PLAYER_RIGHT->get()) world->player->vx *= (float) (world->player->ground ? 0.85 : 0.96);
        if (world->player->vx > 4.5) world->player->vx = 4.5;
        if (world->player->vx < -4.5) world->player->vx = -4.5;
    } else {
        if (state == INGAME) {
            freeCamX += (float) ((Controls::PLAYER_LEFT->get() ? -5 : 0) + (Controls::PLAYER_RIGHT->get() ? 5 : 0));
            freeCamY += (float) ((Controls::PLAYER_UP->get() ? -5 : 0) + (Controls::PLAYER_DOWN->get() ? 5 : 0));
        } else {
        }
    }


    if (world->player) {
        desCamX = (float) (-(mx - (WIDTH / 2)) / 4);
        desCamY = (float) (-(my - (HEIGHT / 2)) / 4);

        world->player->holdAngle = (float) (atan2(desCamY, desCamX) * 180 / (float) M_PI);

        desCamX = 0;
        desCamY = 0;
    } else {
        desCamX = 0;
        desCamY = 0;
    }


    if (world->player) {
        if (world->player->heldItem) {
            if (world->player->heldItem->getFlag(ItemFlags::VACUUM)) {
                if (world->player->holdVacuum) {

                    int wcx = (int) ((WIDTH / 2.0f - ofsX - camX) / scale);
                    int wcy = (int) ((HEIGHT / 2.0f - ofsY - camY) / scale);

                    int wmx = (int) ((mx - ofsX - camX) / scale);
                    int wmy = (int) ((my - ofsY - camY) / scale);

                    int mdx = wmx - wcx;
                    int mdy = wmy - wcy;

                    int distSq = mdx * mdx + mdy * mdy;
                    if (distSq <= 256 * 256) {

                        int sind = -1;
                        bool inObject = true;
                        world->forLine(wcx, wcy, wmx, wmy, [&](int ind) {
                            if (world->tiles[ind].mat->physicsType == PhysicsType::OBJECT) {
                                if (!inObject) {
                                    sind = ind;
                                    return true;
                                }
                            } else {
                                inObject = false;
                            }

                            if (world->tiles[ind].mat->physicsType == PhysicsType::SOLID || world->tiles[ind].mat->physicsType == PhysicsType::SAND || world->tiles[ind].mat->physicsType == PhysicsType::SOUP) {
                                sind = ind;
                                return true;
                            }
                            return false;
                        });

                        int x = sind == -1 ? wmx : sind % world->width;
                        int y = sind == -1 ? wmy : sind / world->width;

                        std::function<void(MaterialInstance, int, int)> makeParticle = [&](MaterialInstance tile, int xPos, int yPos) {
                            Particle *par = new Particle(tile, xPos, yPos, 0, 0, 0, (float) 0.01f);
                            par->vx = (rand() % 10 - 5) / 5.0f * 1.0f;
                            par->vy = (rand() % 10 - 5) / 5.0f * 1.0f;
                            par->ax = -par->vx / 10.0f;
                            par->ay = -par->vy / 10.0f;
                            if (par->ay == 0 && par->ax == 0) par->ay = 0.01f;

                            //par->targetX = world->player->x + world->player->hw / 2 + world->loadZone.x;
                            //par->targetY = world->player->y + world->player->hh / 2 + world->loadZone.y;
                            //par->targetForce = 0.35f;

                            par->lifetime = 6;

                            par->phase = true;

                            world->player->heldItem->vacuumParticles.push_back(par);

                            par->killCallback = [&]() {
                                auto &v = world->player->heldItem->vacuumParticles;
                                v.erase(std::remove(v.begin(), v.end(), par), v.end());
                            };

                            world->addParticle(par);
                        };

                        int rad = 5;
                        int clipRadSq = rad * rad;
                        clipRadSq += rand() % clipRadSq / 4;
                        for (int xx = -rad; xx <= rad; xx++) {
                            for (int yy = -rad; yy <= rad; yy++) {
                                if (xx * xx + yy * yy > clipRadSq) continue;
                                if ((yy == -rad || yy == rad) && (xx == -rad || xx == rad)) {
                                    continue;
                                }

                                MaterialInstance tile = world->tiles[(x + xx) + (y + yy) * world->width];
                                if (tile.mat->physicsType == PhysicsType::SOLID || tile.mat->physicsType == PhysicsType::SAND || tile.mat->physicsType == PhysicsType::SOUP) {
                                    makeParticle(tile, x + xx, y + yy);
                                    world->tiles[(x + xx) + (y + yy) * world->width] = Tiles::NOTHING;
                                    //world->tiles[(x + xx) + (y + yy) * world->width] = Tiles::createFire();
                                    world->dirty[(x + xx) + (y + yy) * world->width] = true;
                                }
                            }
                        }

                        world->particles.erase(std::remove_if(world->particles.begin(), world->particles.end(), [&](Particle *cur) {
                                                   if (cur->targetForce == 0 && !cur->phase) {
                                                       int rad = 5;
                                                       for (int xx = -rad; xx <= rad; xx++) {
                                                           for (int yy = -rad; yy <= rad; yy++) {
                                                               if ((yy == -rad || yy == rad) && (xx == -rad || x == rad)) continue;

                                                               if (((int) (cur->x) == (x + xx)) && ((int) (cur->y) == (y + yy))) {

                                                                   cur->vx = (rand() % 10 - 5) / 5.0f * 1.0f;
                                                                   cur->vy = (rand() % 10 - 5) / 5.0f * 1.0f;
                                                                   cur->ax = -cur->vx / 10.0f;
                                                                   cur->ay = -cur->vy / 10.0f;
                                                                   if (cur->ay == 0 && cur->ax == 0) cur->ay = 0.01f;

                                                                   //par->targetX = world->player->x + world->player->hw / 2 + world->loadZone.x;
                                                                   //par->targetY = world->player->y + world->player->hh / 2 + world->loadZone.y;
                                                                   //par->targetForce = 0.35f;

                                                                   cur->lifetime = 6;

                                                                   cur->phase = true;

                                                                   world->player->heldItem->vacuumParticles.push_back(cur);

                                                                   cur->killCallback = [&]() {
                                                                       auto &v = world->player->heldItem->vacuumParticles;
                                                                       v.erase(std::remove(v.begin(), v.end(), cur), v.end());
                                                                   };

                                                                   return false;
                                                               }
                                                           }
                                                       }
                                                   }

                                                   return false;
                                               }),
                                               world->particles.end());

                        std::vector<RigidBody *> rbs = world->rigidBodies;


                        for (size_t i = 0; i < rbs.size(); i++) {
                            RigidBody *cur = rbs[i];
                            if (cur->surface == nullptr) {
                                METADOT_ERROR("cur->surface == nullptr");
                                continue;
                            }
                            if (cur->body->IsEnabled()) {
                                float s = sin(-cur->body->GetAngle());
                                float c = cos(-cur->body->GetAngle());
                                bool upd = false;
                                for (int xx = -rad; xx <= rad; xx++) {
                                    for (int yy = -rad; yy <= rad; yy++) {
                                        if ((yy == -rad || yy == rad) && (xx == -rad || x == rad)) continue;
                                        // rotate point

                                        float tx = x + xx - cur->body->GetPosition().x;
                                        float ty = y + yy - cur->body->GetPosition().y;

                                        int ntx = (int) (tx * c - ty * s);
                                        int nty = (int) (tx * s + ty * c);

                                        if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w && nty < cur->surface->h) {
                                            Uint32 pixel = METADOT_GET_PIXEL(cur->surface, ntx, nty);
                                            if (((pixel >> 24) & 0xff) != 0x00) {
                                                METADOT_GET_PIXEL(cur->surface, ntx, nty) = 0x00000000;
                                                upd = true;

                                                makeParticle(MaterialInstance(&Materials::GENERIC_SOLID, pixel), (x + xx), (y + yy));
                                            }
                                        }
                                    }
                                }

                                if (upd) {
                                    METAENGINE_Render_FreeImage(cur->texture);
                                    cur->texture = METAENGINE_Render_CopyImageFromSurface(cur->surface);
                                    METAENGINE_Render_SetImageFilter(cur->texture, METAENGINE_Render_FILTER_NEAREST);
                                    //world->updateRigidBodyHitbox(cur);
                                    cur->needsUpdate = true;
                                }
                            }
                        }
                    }
                }

                if (world->player->heldItem->vacuumParticles.size() > 0) {
                    world->player->heldItem->vacuumParticles.erase(std::remove_if(world->player->heldItem->vacuumParticles.begin(), world->player->heldItem->vacuumParticles.end(), [&](Particle *cur) {
                                                                       if (cur->lifetime <= 0) {
                                                                           cur->targetForce = 0.45f;
                                                                           cur->targetX = world->player->x + world->player->hw / 2.0f + world->loadZone.x;
                                                                           cur->targetY = world->player->y + world->player->hh / 2.0f + world->loadZone.y;
                                                                           cur->ax = 0;
                                                                           cur->ay = 0.01f;
                                                                       }

                                                                       float tdx = cur->targetX - cur->x;
                                                                       float tdy = cur->targetY - cur->y;

                                                                       if (tdx * tdx + tdy * tdy < 10 * 10) {
                                                                           cur->temporary = true;
                                                                           cur->lifetime = 0;
                                                                           //METADOT_BUG("vacuum {}", cur->tile.mat->name.c_str());
                                                                           return true;
                                                                       }

                                                                       return false;
                                                                   }),
                                                                   world->player->heldItem->vacuumParticles.end());
                }
            }
        }
    }
}

void Game::updateFrameLate() {

    if (state == LOADING) {

    } else {

        // update camera

        int nofsX;
        int nofsY;

        if (world->player) {

            if (now - lastTick <= mspt) {
                float thruTick = (float) ((now - lastTick) / (double) mspt);

                plPosX = world->player->x + (int) (world->player->vx * thruTick);
                plPosY = world->player->y + (int) (world->player->vy * thruTick);
            } else {
                //plPosX = world->player->x;
                //plPosY = world->player->y;
            }

            //plPosX = (float)(plPosX + (world->player->x - plPosX) / 25.0);
            //plPosY = (float)(plPosY + (world->player->y - plPosY) / 25.0);

            nofsX = (int) (-((int) plPosX + world->player->hw / 2 + world->loadZone.x) * scale + WIDTH / 2);
            nofsY = (int) (-((int) plPosY + world->player->hh / 2 + world->loadZone.y) * scale + HEIGHT / 2);
        } else {
            plPosX = (float) (plPosX + (freeCamX - plPosX) / 50.0f);
            plPosY = (float) (plPosY + (freeCamY - plPosY) / 50.0f);

            nofsX = (int) (-(plPosX + 0 + world->loadZone.x) * scale + WIDTH / 2.0f);
            nofsY = (int) (-(plPosY + 0 + world->loadZone.y) * scale + HEIGHT / 2.0f);
        }

        accLoadX += (nofsX - ofsX) / (float) scale;
        accLoadY += (nofsY - ofsY) / (float) scale;
        //METADOT_BUG("{0:f} {0:f}", plPosX, plPosY);
        //METADOT_BUG("a {0:d} {0:d}", nofsX, nofsY);
        //METADOT_BUG("{0:d} {0:d}", nofsX - ofsX, nofsY - ofsY);
        ofsX += (nofsX - ofsX);
        ofsY += (nofsY - ofsY);


        camX = (float) (camX + (desCamX - camX) * (now - lastTime) / 250.0f);
        camY = (float) (camY + (desCamY - camY) * (now - lastTime) / 250.0f);
    }
}

void Game::renderEarly() {

    if (state == LOADING) {
        if (now - lastLoadingTick > 20) {
            // render loading screen


            unsigned int *ldPixels = (unsigned int *) pixelsLoading_ar;
            bool anyFalse = false;
            //int drop  = (sin(now / 250.0) + 1) / 2 * loadingScreenW;
            //int drop2 = (-sin(now / 250.0) + 1) / 2 * loadingScreenW;
            for (int x = 0; x < loadingScreenW; x++) {
                for (int y = loadingScreenH - 1; y >= 0; y--) {
                    int i = (x + y * loadingScreenW);
                    bool state = ldPixels[i] == loadingOnColor;

                    if (!state) anyFalse = true;
                    bool newState = state;
                    //newState = rand() % 2;

                    if (!state && y == 0) {
                        if (rand() % 6 == 0) {
                            newState = true;
                        }
                        /*if (x >= drop - 1 && x <= drop + 1) {
                            newState = true;
                        }else if (x >= drop2 - 1 && x <= drop2 + 1) {
                            newState = true;
                        }*/
                    }

                    if (state && y < loadingScreenH - 1) {
                        if (ldPixels[(x + (y + 1) * loadingScreenW)] == loadingOffColor) {
                            ldPixels[(x + (y + 1) * loadingScreenW)] = loadingOnColor;
                            newState = false;
                        } else {
                            bool canLeft = x > 0 && ldPixels[((x - 1) + (y + 1) * loadingScreenW)] == loadingOffColor;
                            bool canRight = x < loadingScreenW - 1 && ldPixels[((x + 1) + (y + 1) * loadingScreenW)] == loadingOffColor;
                            if (canLeft && !(canRight && (rand() % 2 == 0))) {
                                ldPixels[((x - 1) + (y + 1) * loadingScreenW)] = loadingOnColor;
                                newState = false;
                            } else if (canRight) {
                                ldPixels[((x + 1) + (y + 1) * loadingScreenW)] = loadingOnColor;
                                newState = false;
                            }
                        }
                    }

                    ldPixels[(x + y * loadingScreenW)] = (newState ? loadingOnColor : loadingOffColor);
                    int sx = WIDTH / loadingScreenW;
                    int sy = HEIGHT / loadingScreenH;
                    //METAENGINE_Render_RectangleFilled(target, x * sx, y * sy, x * sx + sx, y * sy + sy, state ? SDL_Color{ 0xff, 0, 0, 0xff } : SDL_Color{ 0, 0xff, 0, 0xff });
                }
            }
            if (!anyFalse) {
                uint32 tmp = loadingOnColor;
                loadingOnColor = loadingOffColor;
                loadingOffColor = tmp;
            }

            METAENGINE_Render_UpdateImageBytes(
                    loadingTexture,
                    NULL,
                    &pixelsLoading_ar[0],
                    loadingScreenW * 4);


            lastLoadingTick = now;
        } else {
            //#ifdef _WIN32
            //            Sleep(5);
            //#else
            //            sleep(5 / 1000.0f);
            //#endif
        }
        METAENGINE_Render_ActivateShaderProgram(0, NULL);
        METAENGINE_Render_BlitRect(loadingTexture, NULL, target, NULL);
        if (dt_loading.w == -1) {
            dt_loading = Drawing::drawTextParams(target, "Loading...", font64, WIDTH / 2, HEIGHT / 2 - 32, 255, 255, 255, ALIGN_CENTER);
        }
        Drawing::drawText(target, dt_loading, WIDTH / 2, HEIGHT / 2 - 32, ALIGN_CENTER);
    } else {
        // render entities with LERP

        if (now - lastTick <= mspt) {
            METAENGINE_Render_Clear(textureEntities->target);
            METAENGINE_Render_Clear(textureEntitiesLQ->target);
            if (world->player) {
                float thruTick = (float) ((now - lastTick) / (double) mspt);

                METAENGINE_Render_SetBlendMode(textureEntities, METAENGINE_Render_BLEND_ADD);
                METAENGINE_Render_SetBlendMode(textureEntitiesLQ, METAENGINE_Render_BLEND_ADD);
                int scaleEnt = Settings::hd_objects ? Settings::hd_objects_size : 1;

                for (auto &v: world->entities) {
                    v->renderLQ(textureEntitiesLQ->target, world->loadZone.x + (int) (v->vx * thruTick), world->loadZone.y + (int) (v->vy * thruTick));
                    v->render(textureEntities->target, world->loadZone.x + (int) (v->vx * thruTick), world->loadZone.y + (int) (v->vy * thruTick));
                }

                if (world->player && world->player->heldItem != NULL) {
                    if (world->player->heldItem->getFlag(ItemFlags::HAMMER)) {
                        if (world->player->holdHammer) {
                            int x = (int) ((mx - ofsX - camX) / scale);
                            int y = (int) ((my - ofsY - camY) / scale);

                            int dx = x - world->player->hammerX;
                            int dy = y - world->player->hammerY;
                            float len = sqrt(dx * dx + dy * dy);
                            if (len > 40) {
                                dx = dx / len * 40;
                                dy = dy / len * 40;
                            }

                            METAENGINE_Render_Line(textureEntitiesLQ->target, world->player->hammerX + dx, world->player->hammerY + dy, world->player->hammerX, world->player->hammerY, {0xff, 0xff, 0x00, 0xff});
                        } else {
                            int startInd = getAimSolidSurface(64);

                            if (startInd != -1) {
                                int x = startInd % world->width;
                                int y = startInd / world->width;
                                METAENGINE_Render_Rectangle(textureEntitiesLQ->target, x - 1, y - 1, x + 1, y + 1, {0xff, 0xff, 0x00, 0xE0});
                            }
                        }
                    }
                }
                METAENGINE_Render_SetBlendMode(textureEntities, METAENGINE_Render_BLEND_NORMAL);
                METAENGINE_Render_SetBlendMode(textureEntitiesLQ, METAENGINE_Render_BLEND_NORMAL);
            }
        }


        if (Controls::mmouse) {
            int x = (int) ((mx - ofsX - camX) / scale);
            int y = (int) ((my - ofsY - camY) / scale);
            METAENGINE_Render_RectangleFilled(textureEntitiesLQ->target, x - DebugDrawUI::brushSize / 2.0f, y - DebugDrawUI::brushSize / 2.0f, x + (int) (ceil(DebugDrawUI::brushSize / 2.0)), y + (int) (ceil(DebugDrawUI::brushSize / 2.0)), {0xff, 0x40, 0x40, 0x90});
            METAENGINE_Render_Rectangle(textureEntitiesLQ->target, x - DebugDrawUI::brushSize / 2.0f, y - DebugDrawUI::brushSize / 2.0f, x + (int) (ceil(DebugDrawUI::brushSize / 2.0)) + 1, y + (int) (ceil(DebugDrawUI::brushSize / 2.0)) + 1, {0xff, 0x40, 0x40, 0xE0});
        } else if (Controls::DEBUG_DRAW->get()) {
            int x = (int) ((mx - ofsX - camX) / scale);
            int y = (int) ((my - ofsY - camY) / scale);
            METAENGINE_Render_RectangleFilled(textureEntitiesLQ->target, x - DebugDrawUI::brushSize / 2.0f, y - DebugDrawUI::brushSize / 2.0f, x + (int) (ceil(DebugDrawUI::brushSize / 2.0)), y + (int) (ceil(DebugDrawUI::brushSize / 2.0)), {0x00, 0xff, 0xB0, 0x80});
            METAENGINE_Render_Rectangle(textureEntitiesLQ->target, x - DebugDrawUI::brushSize / 2.0f, y - DebugDrawUI::brushSize / 2.0f, x + (int) (ceil(DebugDrawUI::brushSize / 2.0)) + 1, y + (int) (ceil(DebugDrawUI::brushSize / 2.0)) + 1, {0x00, 0xff, 0xB0, 0xE0});
        }
    }
}

void Game::renderLate() {


    target = backgroundImage->target;
    METAENGINE_Render_Clear(target);

    if (state == LOADING) {

    } else {
        // draw backgrounds


        Background *bg = backgrounds->Get("TEST_OVERWORLD");
        if (Settings::draw_background && scale <= bg->layers[0].surface.size() && world->loadZone.y > -5 * CHUNK_H) {
            METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_SET);
            SDL_Color col = {static_cast<Uint8>((bg->solid >> 16) & 0xff), static_cast<Uint8>((bg->solid >> 8) & 0xff), static_cast<Uint8>((bg->solid >> 0) & 0xff), 0xff};
            METAENGINE_Render_ClearColor(target, col);

            METAENGINE_Render_Rect *dst = new METAENGINE_Render_Rect();
            METAENGINE_Render_Rect *src = new METAENGINE_Render_Rect();

            float arX = (float) WIDTH / (bg->layers[0].surface[0]->w);
            float arY = (float) HEIGHT / (bg->layers[0].surface[0]->h);

            double time = UTime::millis() / 1000.0;

            METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_NORMAL);

            for (size_t i = 0; i < bg->layers.size(); i++) {
                BackgroundLayer cur = bg->layers[i];

                SDL_Surface *texture = cur.surface[(size_t) scale - 1];

                METAENGINE_Render_Image *tex = cur.texture[(size_t) scale - 1];
                METAENGINE_Render_SetBlendMode(tex, METAENGINE_Render_BLEND_NORMAL);

                int tw = texture->w;
                int th = texture->h;

                int iter = (int) ceil((float) WIDTH / (tw)) + 1;
                for (int n = 0; n < iter; n++) {

                    src->x = 0;
                    src->y = 0;
                    src->w = tw;
                    src->h = th;

                    dst->x = (((ofsX + camX) + world->loadZone.x * scale) + n * tw / cur.parralaxX) * cur.parralaxX + world->width / 2.0f * scale - tw / 2.0f;
                    dst->y = ((ofsY + camY) + world->loadZone.y * scale) * cur.parralaxY + world->height / 2.0f * scale - th / 2.0f - HEIGHT / 3.0f * (scale - 1);
                    dst->w = (float) tw;
                    dst->h = (float) th;

                    dst->x += (float) (scale * fmod(cur.moveX * time, tw));

                    //TODO: optimize
                    while (dst->x >= WIDTH - 10) dst->x -= (iter * tw);
                    while (dst->x + dst->w < 0) dst->x += (iter * tw - 1);

                    //TODO: optimize
                    if (dst->x < 0) {
                        dst->w += dst->x;
                        src->x -= (int) dst->x;
                        src->w += (int) dst->x;
                        dst->x = 0;
                    }

                    if (dst->y < 0) {
                        dst->h += dst->y;
                        src->y -= (int) dst->y;
                        src->h += (int) dst->y;
                        dst->y = 0;
                    }

                    if (dst->x + dst->w >= WIDTH) {
                        src->w -= (int) ((dst->x + dst->w) - WIDTH);
                        dst->w += WIDTH - (dst->x + dst->w);
                    }

                    if (dst->y + dst->h >= HEIGHT) {
                        src->h -= (int) ((dst->y + dst->h) - HEIGHT);
                        dst->h += HEIGHT - (dst->y + dst->h);
                    }

                    METAENGINE_Render_BlitRect(tex, src, target, dst);
                }
            }

            delete dst;
            delete src;
        }


        METAENGINE_Render_Rect r1 = METAENGINE_Render_Rect{(float) (ofsX + camX), (float) (ofsY + camY), (float) (world->width * scale), (float) (world->height * scale)};
        METAENGINE_Render_SetBlendMode(textureBackground, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(textureBackground, NULL, target, &r1);

        METAENGINE_Render_SetBlendMode(textureLayer2, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(textureLayer2, NULL, target, &r1);

        METAENGINE_Render_SetBlendMode(textureObjectsBack, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(textureObjectsBack, NULL, target, &r1);

        // shader

        if (Settings::draw_shaders) {

            if (waterFlowPassShader->dirty && Settings::water_showFlow) {

                waterFlowPassShader->activate();
                waterFlowPassShader->update(world->width, world->height);
                METAENGINE_Render_SetBlendMode(textureFlow, METAENGINE_Render_BLEND_SET);
                METAENGINE_Render_BlitRect(textureFlow, NULL, textureFlowSpead->target, NULL);


                waterFlowPassShader->dirty = false;
            }

            waterShader->activate();
            float t = (now - startTime) / 1000.0;
            waterShader->update(t, target->w * scale, target->h * scale, texture, r1.x, r1.y, r1.w, r1.h, scale, textureFlowSpead, Settings::water_overlay, Settings::water_showFlow, Settings::water_pixelated);
        }

        target = realTarget;

        METAENGINE_Render_BlitRect(backgroundImage, NULL, target, NULL);

        METAENGINE_Render_SetBlendMode(texture, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_ActivateShaderProgram(0, NULL);


        // done shader


        int lmsx = (int) ((mx - ofsX - camX) / scale);
        int lmsy = (int) ((my - ofsY - camY) / scale);

        METAENGINE_Render_Clear(worldTexture->target);

        METAENGINE_Render_BlitRect(texture, NULL, worldTexture->target, NULL);

        METAENGINE_Render_SetBlendMode(textureObjects, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(textureObjects, NULL, worldTexture->target, NULL);
        METAENGINE_Render_SetBlendMode(textureObjectsLQ, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(textureObjectsLQ, NULL, worldTexture->target, NULL);

        METAENGINE_Render_SetBlendMode(textureParticles, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(textureParticles, NULL, worldTexture->target, NULL);

        METAENGINE_Render_SetBlendMode(textureEntitiesLQ, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(textureEntitiesLQ, NULL, worldTexture->target, NULL);
        METAENGINE_Render_SetBlendMode(textureEntities, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(textureEntities, NULL, worldTexture->target, NULL);


        if (Settings::draw_shaders) newLightingShader->activate();

        // I use this to only rerender the lighting when a parameter changes or N times per second anyway
        // Doing this massively reduces the GPU load of the shader
        bool needToRerenderLighting = false;

        static long long lastLightingForceRefresh = 0;
        long long now = UTime::millis();
        if (now - lastLightingForceRefresh > 100) {
            lastLightingForceRefresh = now;
            needToRerenderLighting = true;
        }

        if (Settings::draw_shaders && world) {
            float lightTx;
            float lightTy;

            if (world->player) {
                lightTx = (world->loadZone.x + world->player->x + world->player->hw / 2.0f) / (float) world->width;
                lightTy = (world->loadZone.y + world->player->y + world->player->hh / 2.0f) / (float) world->height;
            } else {
                lightTx = lmsx / (float) world->width;
                lightTy = lmsy / (float) world->height;
            }

            if (newLightingShader->lastLx != lightTx || newLightingShader->lastLy != lightTy) needToRerenderLighting = true;
            newLightingShader->update(worldTexture, emissionTexture, lightTx, lightTy);
            if (newLightingShader->lastQuality != Settings::lightingQuality) {
                needToRerenderLighting = true;
            }
            newLightingShader->setQuality(Settings::lightingQuality);

            int nBg = 0;
            int range = 64;
            for (int xx = std::max(0, (int) (lightTx * world->width) - range); xx <= std::min((int) (lightTx * world->width) + range, world->width - 1); xx++) {
                for (int yy = std::max(0, (int) (lightTy * world->height) - range); yy <= std::min((int) (lightTy * world->height) + range, world->height - 1); yy++) {
                    if (world->background[xx + yy * world->width] != 0x00) {
                        nBg++;
                    }
                }
            }

            newLightingShader_insideDes = std::min(std::max(0.0f, (float) nBg / ((range * 2) * (range * 2))), 1.0f);
            newLightingShader_insideCur += (newLightingShader_insideDes - newLightingShader_insideCur) / 2.0f * (deltaTime / 1000.0f);

            float ins = newLightingShader_insideCur < 0.05 ? 0.0 : newLightingShader_insideCur;
            if (newLightingShader->lastInside != ins) needToRerenderLighting = true;
            newLightingShader->setInside(ins);
            newLightingShader->setBounds(world->tickZone.x * Settings::hd_objects_size, world->tickZone.y * Settings::hd_objects_size, (world->tickZone.x + world->tickZone.w) * Settings::hd_objects_size, (world->tickZone.y + world->tickZone.h) * Settings::hd_objects_size);

            if (newLightingShader->lastSimpleMode != Settings::simpleLighting) needToRerenderLighting = true;
            newLightingShader->setSimpleMode(Settings::simpleLighting);

            if (newLightingShader->lastEmissionEnabled != Settings::lightingEmission) needToRerenderLighting = true;
            newLightingShader->setEmissionEnabled(Settings::lightingEmission);

            if (newLightingShader->lastDitheringEnabled != Settings::lightingDithering) needToRerenderLighting = true;
            newLightingShader->setDitheringEnabled(Settings::lightingDithering);
        }

        if (Settings::draw_shaders && needToRerenderLighting) {
            METAENGINE_Render_Clear(lightingTexture->target);
            METAENGINE_Render_BlitRect(worldTexture, NULL, lightingTexture->target, NULL);
        }
        if (Settings::draw_shaders) METAENGINE_Render_ActivateShaderProgram(0, NULL);


        METAENGINE_Render_BlitRect(worldTexture, NULL, target, &r1);

        if (Settings::draw_shaders) {
            METAENGINE_Render_SetBlendMode(lightingTexture, Settings::draw_light_overlay ? METAENGINE_Render_BLEND_NORMAL : METAENGINE_Render_BLEND_MULTIPLY);
            METAENGINE_Render_BlitRect(lightingTexture, NULL, target, &r1);
        }


        if (Settings::draw_shaders) {
            METAENGINE_Render_Clear(texture2Fire->target);

            fireShader->activate();
            fireShader->update(textureFire);
            METAENGINE_Render_BlitRect(textureFire, NULL, texture2Fire->target, NULL);
            METAENGINE_Render_ActivateShaderProgram(0, NULL);


            fire2Shader->activate();
            fire2Shader->update(texture2Fire);
            METAENGINE_Render_BlitRect(texture2Fire, NULL, target, &r1);
            METAENGINE_Render_ActivateShaderProgram(0, NULL);
        }

        // done light

        renderOverlays();
    }
}

void Game::renderOverlays() {


    METAENGINE_Render_Rect r1 = METAENGINE_Render_Rect{(float) (ofsX + camX), (float) (ofsY + camY), (float) (world->width * scale), (float) (world->height * scale)};
    METAENGINE_Render_Rect r2 = METAENGINE_Render_Rect{(float) (ofsX + camX + world->tickZone.x * scale), (float) (ofsY + camY + world->tickZone.y * scale), (float) (world->tickZone.w * scale), (float) (world->tickZone.h * scale)};

    if (Settings::draw_temperature_map) {
        METAENGINE_Render_SetBlendMode(temperatureMap, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(temperatureMap, NULL, target, &r1);
    }


    if (Settings::draw_load_zones) {
        METAENGINE_Render_Rect r2m = METAENGINE_Render_Rect{(float) (ofsX + camX + world->meshZone.x * scale),
                                                            (float) (ofsY + camY + world->meshZone.y * scale),
                                                            (float) (world->meshZone.w * scale),
                                                            (float) (world->meshZone.h * scale)};

        METAENGINE_Render_Rectangle2(target, r2m, {0x00, 0xff, 0xff, 0xff});
        METAENGINE_Render_Rectangle2(target, r2, {0xff, 0x00, 0x00, 0xff});
    }

    if (Settings::draw_load_zones) {

        SDL_Color col = {0xff, 0x00, 0x00, 0x20};
        METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_NORMAL);

        METAENGINE_Render_Rect r3 = METAENGINE_Render_Rect{(float) (0), (float) (0), (float) ((ofsX + camX + world->tickZone.x * scale)), (float) (HEIGHT)};
        METAENGINE_Render_Rectangle2(target, r3, col);

        METAENGINE_Render_Rect r4 = METAENGINE_Render_Rect{(float) (ofsX + camX + world->tickZone.x * scale + world->tickZone.w * scale), (float) (0), (float) ((WIDTH) - (ofsX + camX + world->tickZone.x * scale + world->tickZone.w * scale)), (float) (HEIGHT)};
        METAENGINE_Render_Rectangle2(target, r3, col);

        METAENGINE_Render_Rect r5 = METAENGINE_Render_Rect{(float) (ofsX + camX + world->tickZone.x * scale), (float) (0), (float) (world->tickZone.w * scale), (float) (ofsY + camY + world->tickZone.y * scale)};
        METAENGINE_Render_Rectangle2(target, r3, col);

        METAENGINE_Render_Rect r6 = METAENGINE_Render_Rect{(float) (ofsX + camX + world->tickZone.x * scale), (float) (ofsY + camY + world->tickZone.y * scale + world->tickZone.h * scale), (float) (world->tickZone.w * scale), (float) (HEIGHT - (ofsY + camY + world->tickZone.y * scale + world->tickZone.h * scale))};
        METAENGINE_Render_Rectangle2(target, r6, col);

        col = {0x00, 0xff, 0x00, 0xff};
        METAENGINE_Render_Rect r7 = METAENGINE_Render_Rect{(float) (ofsX + camX + world->width / 2 * scale - (WIDTH / 3 * scale / 2)), (float) (ofsY + camY + world->height / 2 * scale - (HEIGHT / 3 * scale / 2)), (float) (WIDTH / 3 * scale), (float) (HEIGHT / 3 * scale)};
        METAENGINE_Render_Rectangle2(target, r7, col);
    }

    if (Settings::draw_physics_debug) {
        //
        //for(size_t i = 0; i < world->rigidBodies.size(); i++) {
        //    RigidBody cur = *world->rigidBodies[i];

        //    float x = cur.body->GetPosition().x;
        //    float y = cur.body->GetPosition().y;
        //    x = ((x)*scale + ofsX + camX);
        //    y = ((y)*scale + ofsY + camY);

        //    /*SDL_Rect* r = new SDL_Rect{ (int)x, (int)y, cur.surface->w * scale, cur.surface->h * scale };
        //    SDL_RenderCopyEx(renderer, cur.texture, NULL, r, cur.body->GetAngle() * 180 / M_PI, new SDL_Point{ 0, 0 }, SDL_RendererFlip::SDL_FLIP_NONE);
        //    delete r;*/

        //    Uint32 color = 0x0000ff;

        //    SDL_Color col = {(color >> 16) & 0xff, (color >> 8) & 0xff, (color >> 0) & 0xff, 0xff};

        //    b2Fixture* fix = cur.body->GetFixtureList();
        //    while(fix) {
        //        b2Shape* shape = fix->GetShape();

        //        switch(shape->GetType()) {
        //        case b2Shape::Type::e_polygon:
        //            b2PolygonShape* poly = (b2PolygonShape*)shape;
        //            b2Vec2* verts = poly->m_vertices;

        //            Drawing::drawPolygon(target, col, verts, (int)x, (int)y, scale, poly->m_count, cur.body->GetAngle()/* + fmod((Time::millis() / 1000.0), 360)*/, 0, 0);

        //            break;
        //        }

        //        fix = fix->GetNext();
        //    }
        //}

        //if(world->player) {
        //    RigidBody cur = *world->player->rb;

        //    float x = cur.body->GetPosition().x;
        //    float y = cur.body->GetPosition().y;
        //    x = ((x)*scale + ofsX + camX);
        //    y = ((y)*scale + ofsY + camY);

        //    Uint32 color = 0x0000ff;

        //    SDL_Color col = {(color >> 16) & 0xff, (color >> 8) & 0xff, (color >> 0) & 0xff, 0xff};

        //    b2Fixture* fix = cur.body->GetFixtureList();
        //    while(fix) {
        //        b2Shape* shape = fix->GetShape();
        //        switch(shape->GetType()) {
        //        case b2Shape::Type::e_polygon:
        //            b2PolygonShape* poly = (b2PolygonShape*)shape;
        //            b2Vec2* verts = poly->m_vertices;

        //            Drawing::drawPolygon(target, col, verts, (int)x, (int)y, scale, poly->m_count, cur.body->GetAngle()/* + fmod((Time::millis() / 1000.0), 360)*/, 0, 0);

        //            break;
        //        }

        //        fix = fix->GetNext();
        //    }
        //}

        //for(size_t i = 0; i < world->worldRigidBodies.size(); i++) {
        //    RigidBody cur = *world->worldRigidBodies[i];

        //    float x = cur.body->GetPosition().x;
        //    float y = cur.body->GetPosition().y;
        //    x = ((x)*scale + ofsX + camX);
        //    y = ((y)*scale + ofsY + camY);

        //    Uint32 color = 0x00ff00;

        //    SDL_Color col = {(color >> 16) & 0xff, (color >> 8) & 0xff, (color >> 0) & 0xff, 0xff};

        //    b2Fixture* fix = cur.body->GetFixtureList();
        //    while(fix) {
        //        b2Shape* shape = fix->GetShape();
        //        switch(shape->GetType()) {
        //        case b2Shape::Type::e_polygon:
        //            b2PolygonShape* poly = (b2PolygonShape*)shape;
        //            b2Vec2* verts = poly->m_vertices;

        //            Drawing::drawPolygon(target, col, verts, (int)x, (int)y, scale, poly->m_count, cur.body->GetAngle()/* + fmod((Time::millis() / 1000.0), 360)*/, 0, 0);

        //            break;
        //        }

        //        fix = fix->GetNext();
        //    }

        //    /*color = 0xff0000;
        //    SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xff, (color >> 8) & 0xff, (color >> 0) & 0xff, 0xff);
        //    SDL_RenderDrawPoint(renderer, x, y);
        //    color = 0x00ff00;
        //    SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xff, (color >> 8) & 0xff, (color >> 0) & 0xff, 0xff);
        //    SDL_RenderDrawPoint(renderer, cur.body->GetLocalCenter().x * scale + ofsX, cur.body->GetLocalCenter().y * scale + ofsY);*/
        //}

        int minChX = (int) floor((world->meshZone.x - world->loadZone.x) / CHUNK_W);
        int minChY = (int) floor((world->meshZone.y - world->loadZone.y) / CHUNK_H);
        int maxChX = (int) ceil((world->meshZone.x + world->meshZone.w - world->loadZone.x) / CHUNK_W);
        int maxChY = (int) ceil((world->meshZone.y + world->meshZone.h - world->loadZone.y) / CHUNK_H);

        for (int cx = minChX; cx <= maxChX; cx++) {
            for (int cy = minChY; cy <= maxChY; cy++) {
                Chunk *ch = world->getChunk(cx, cy);
                SDL_Color col = {255, 0, 0, 255};

                float x = ((ch->x * CHUNK_W + world->loadZone.x) * scale + ofsX + camX);
                float y = ((ch->y * CHUNK_H + world->loadZone.y) * scale + ofsY + camY);

                METAENGINE_Render_Rectangle(target, x, y, x + CHUNK_W * scale, y + CHUNK_H * scale, {50, 50, 0, 255});

                //for(int i = 0; i < ch->polys.size(); i++) {
                //    Drawing::drawPolygon(target, col, ch->polys[i].m_vertices, (int)x, (int)y, scale, ch->polys[i].m_count, 0/* + fmod((Time::millis() / 1000.0), 360)*/, 0, 0);
                //}
            }
        }

        //

        world->b2world->SetDebugDraw(b2DebugDraw);
        b2DebugDraw->scale = scale;
        b2DebugDraw->xOfs = ofsX + camX;
        b2DebugDraw->yOfs = ofsY + camY;
        b2DebugDraw->SetFlags(0);
        if (Settings::draw_b2d_shape) b2DebugDraw->AppendFlags(b2Draw::e_shapeBit);
        if (Settings::draw_b2d_joint) b2DebugDraw->AppendFlags(b2Draw::e_jointBit);
        if (Settings::draw_b2d_aabb) b2DebugDraw->AppendFlags(b2Draw::e_aabbBit);
        if (Settings::draw_b2d_pair) b2DebugDraw->AppendFlags(b2Draw::e_pairBit);
        if (Settings::draw_b2d_centerMass) b2DebugDraw->AppendFlags(b2Draw::e_centerOfMassBit);
        world->b2world->DebugDraw();
    }


    if (dt_fps.w == -1) {
        char buffFps[20];
        snprintf(buffFps, sizeof(buffFps), "%d FPS", fps);
        //if (dt_fps.t1 != nullptr) METAENGINE_Render_FreeImage(dt_fps.t1);
        //if (dt_fps.t2 != nullptr) METAENGINE_Render_FreeImage(dt_fps.t2);
        dt_fps = Drawing::drawTextParams(target, buffFps, font16, WIDTH - 4, 2, 0xff, 0xff, 0xff, ALIGN_RIGHT);
    }

    Drawing::drawText(target, dt_fps, WIDTH - 4, 2, ALIGN_RIGHT);


    if (dt_feelsLikeFps.w == -1) {
        char buffFps[22];
        snprintf(buffFps, sizeof(buffFps), "Feels Like: %d FPS", feelsLikeFps);
        //if (dt_feelsLikeFps.t1 != nullptr) METAENGINE_Render_FreeImage(dt_feelsLikeFps.t1);
        //if (dt_feelsLikeFps.t2 != nullptr) METAENGINE_Render_FreeImage(dt_feelsLikeFps.t2);
        dt_feelsLikeFps = Drawing::drawTextParams(target, buffFps, font16, WIDTH - 4, 2, 0xff, 0xff, 0xff, ALIGN_RIGHT);
    }

    Drawing::drawText(target, dt_feelsLikeFps, WIDTH - 4, 2 + 14, ALIGN_RIGHT);


    if (Settings::draw_chunk_state) {

        int chSize = 10;

        int centerX = WIDTH / 2;
        int centerY = CHUNK_UNLOAD_DIST * chSize + 10;


        int pposX = plPosX;
        int pposY = plPosY;
        int pchx = (int) ((pposX / CHUNK_W) * chSize);
        int pchy = (int) ((pposY / CHUNK_H) * chSize);
        int pchxf = (int) (((float) pposX / CHUNK_W) * chSize);
        int pchyf = (int) (((float) pposY / CHUNK_H) * chSize);

        METAENGINE_Render_Rectangle(target, centerX - chSize * CHUNK_UNLOAD_DIST + chSize, centerY - chSize * CHUNK_UNLOAD_DIST + chSize, centerX + chSize * CHUNK_UNLOAD_DIST + chSize, centerY + chSize * CHUNK_UNLOAD_DIST + chSize, {0xcc, 0xcc, 0xcc, 0xff});

        METAENGINE_Render_Rect r = {0, 0, (float) chSize, (float) chSize};
        for (auto &p: world->chunkCache) {
            if (p.first == INT_MIN) continue;
            int cx = p.first;
            for (auto &p2: p.second) {
                if (p2.first == INT_MIN) continue;
                int cy = p2.first;
                Chunk *m = p2.second;
                r.x = centerX + m->x * chSize - pchx;
                r.y = centerY + m->y * chSize - pchy;
                SDL_Color col;
                if (m->generationPhase == -1) {
                    col = {0x60, 0x60, 0x60, 0xff};
                } else if (m->generationPhase == 0) {
                    col = {0xff, 0x00, 0x00, 0xff};
                } else if (m->generationPhase == 1) {
                    col = {0x00, 0xff, 0x00, 0xff};
                } else if (m->generationPhase == 2) {
                    col = {0x00, 0x00, 0xff, 0xff};
                } else if (m->generationPhase == 3) {
                    col = {0xff, 0xff, 0x00, 0xff};
                } else if (m->generationPhase == 4) {
                    col = {0xff, 0x00, 0xff, 0xff};
                } else if (m->generationPhase == 5) {
                    col = {0x00, 0xff, 0xff, 0xff};
                } else {
                }
                METAENGINE_Render_Rectangle2(target, r, col);
            }
        }

        int loadx = (int) (((float) -world->loadZone.x / CHUNK_W) * chSize);
        int loady = (int) (((float) -world->loadZone.y / CHUNK_H) * chSize);

        int loadx2 = (int) (((float) (-world->loadZone.x + world->loadZone.w) / CHUNK_W) * chSize);
        int loady2 = (int) (((float) (-world->loadZone.y + world->loadZone.h) / CHUNK_H) * chSize);
        METAENGINE_Render_Rectangle(target, centerX - pchx + loadx, centerY - pchy + loady, centerX - pchx + loadx2, centerY - pchy + loady2, {0x00, 0xff, 0xff, 0xff});

        METAENGINE_Render_Rectangle(target, centerX - pchx + pchxf, centerY - pchy + pchyf, centerX + 1 - pchx + pchxf, centerY + 1 - pchy + pchyf, {0x00, 0xff, 0x00, 0xff});
    }

    if (Settings::draw_debug_stats) {

        int dbgIndex = 1;

        int lineHeight = 16;

        char buff1[32];
        std::string buffAsStdStr1;

        snprintf(buff1, sizeof(buff1), "XY: %.2f / %.2f", plPosX, plPosY);
        buffAsStdStr1 = buff1;
        Drawing::drawTextBG(target, buffAsStdStr1.c_str(), font16, 4, 2 + (lineHeight * dbgIndex++), 0xff, 0xff, 0xff, {0x00, 0x00, 0x00, 0x40}, ALIGN_LEFT);

        snprintf(buff1, sizeof(buff1), "V: %.3f / %.3f", world->player ? world->player->vx : 0, world->player ? world->player->vy : 0);
        buffAsStdStr1 = buff1;
        Drawing::drawTextBG(target, buffAsStdStr1.c_str(), font16, 4, 2 + (lineHeight * dbgIndex++), 0xff, 0xff, 0xff, {0x00, 0x00, 0x00, 0x40}, ALIGN_LEFT);

        snprintf(buff1, sizeof(buff1), "Particles: %d", (int) world->particles.size());
        buffAsStdStr1 = buff1;
        Drawing::drawTextBG(target, buffAsStdStr1.c_str(), font16, 4, 2 + (lineHeight * dbgIndex++), 0xff, 0xff, 0xff, {0x00, 0x00, 0x00, 0x40}, ALIGN_LEFT);

        snprintf(buff1, sizeof(buff1), "Entities: %d", (int) world->entities.size());
        buffAsStdStr1 = buff1;
        Drawing::drawTextBG(target, buffAsStdStr1.c_str(), font16, 4, 2 + (lineHeight * dbgIndex++), 0xff, 0xff, 0xff, {0x00, 0x00, 0x00, 0x40}, ALIGN_LEFT);

        int rbCt = 0;
        for (auto &r: world->rigidBodies) {
            if (r->body->IsEnabled()) rbCt++;
        }

        snprintf(buff1, sizeof(buff1), "RigidBodies: %d/%d O, %d W", rbCt, (int) world->rigidBodies.size(), (int) world->worldRigidBodies.size());
        buffAsStdStr1 = buff1;
        Drawing::drawTextBG(target, buffAsStdStr1.c_str(), font16, 4, 2 + (lineHeight * dbgIndex++), 0xff, 0xff, 0xff, {0x00, 0x00, 0x00, 0x40}, ALIGN_LEFT);

        int rbTriACt = 0;
        int rbTriCt = 0;
        for (size_t i = 0; i < world->rigidBodies.size(); i++) {
            RigidBody cur = *world->rigidBodies[i];

            b2Fixture *fix = cur.body->GetFixtureList();
            while (fix) {
                b2Shape *shape = fix->GetShape();

                switch (shape->GetType()) {
                    case b2Shape::Type::e_polygon:
                        rbTriCt++;
                        if (cur.body->IsEnabled()) rbTriACt++;
                        break;
                }

                fix = fix->GetNext();
            }
        }

        int rbTriWCt = 0;

        int minChX = (int) floor((world->meshZone.x - world->loadZone.x) / CHUNK_W);
        int minChY = (int) floor((world->meshZone.y - world->loadZone.y) / CHUNK_H);
        int maxChX = (int) ceil((world->meshZone.x + world->meshZone.w - world->loadZone.x) / CHUNK_W);
        int maxChY = (int) ceil((world->meshZone.y + world->meshZone.h - world->loadZone.y) / CHUNK_H);

        for (int cx = minChX; cx <= maxChX; cx++) {
            for (int cy = minChY; cy <= maxChY; cy++) {
                Chunk *ch = world->getChunk(cx, cy);
                for (int i = 0; i < ch->polys.size(); i++) {
                    rbTriWCt++;
                }
            }
        }

        snprintf(buff1, sizeof(buff1), "Tris: %d/%d O, %d W", rbTriACt, rbTriCt, rbTriWCt);
        buffAsStdStr1 = buff1;
        Drawing::drawTextBG(target, buffAsStdStr1.c_str(), font16, 4, 2 + (lineHeight * dbgIndex++), 0xff, 0xff, 0xff, {0x00, 0x00, 0x00, 0x40}, ALIGN_LEFT);

        int chCt = 0;
        for (auto &p: world->chunkCache) {
            if (p.first == INT_MIN) continue;
            int cx = p.first;
            for (auto &p2: p.second) {
                if (p2.first == INT_MIN) continue;
                chCt++;
            }
        }

        snprintf(buff1, sizeof(buff1), "Cached Chunks: %d", chCt);
        buffAsStdStr1 = buff1;
        Drawing::drawTextBG(target, buffAsStdStr1.c_str(), font16, 4, 2 + (lineHeight * dbgIndex++), 0xff, 0xff, 0xff, {0x00, 0x00, 0x00, 0x40}, ALIGN_LEFT);

        snprintf(buff1, sizeof(buff1), "world->readyToReadyToMerge (%d)", (int) world->readyToReadyToMerge.size());
        buffAsStdStr1 = buff1;
        Drawing::drawTextBG(target, buffAsStdStr1.c_str(), font16, 4, 2 + (lineHeight * dbgIndex++), 0xff, 0xff, 0xff, {0x00, 0x00, 0x00, 0x40}, ALIGN_LEFT);
        for (size_t i = 0; i < world->readyToReadyToMerge.size(); i++) {
            char buff[10];
            snprintf(buff, sizeof(buff), "    #%d", (int) i);
            std::string buffAsStdStr = buff;
            Drawing::drawTextBG(target, buffAsStdStr.c_str(), font16, 4, 2 + (lineHeight * dbgIndex++), 0xff, 0xff, 0xff, {0x00, 0x00, 0x00, 0x40}, ALIGN_LEFT);
        }
        char buff2[30];
        snprintf(buff2, sizeof(buff2), "world->readyToMerge (%d)", (int) world->readyToMerge.size());
        std::string buffAsStdStr2 = buff2;
        Drawing::drawTextBG(target, buffAsStdStr2.c_str(), font16, 4, 2 + (lineHeight * dbgIndex++), 0xff, 0xff, 0xff, {0x00, 0x00, 0x00, 0x40}, ALIGN_LEFT);
        for (size_t i = 0; i < world->readyToMerge.size(); i++) {
            char buff[20];
            snprintf(buff, sizeof(buff), "    #%d (%d, %d)", (int) i, world->readyToMerge[i]->x, world->readyToMerge[i]->y);
            std::string buffAsStdStr = buff;
            Drawing::drawTextBG(target, buffAsStdStr.c_str(), font16, 4, 2 + (lineHeight * dbgIndex++), 0xff, 0xff, 0xff, {0x00, 0x00, 0x00, 0x40}, ALIGN_LEFT);
        }
    }

    if (Settings::draw_frame_graph) {

        for (int i = 0; i <= 4; i++) {
            if (dt_frameGraph[i].w == -1) {
                char buff[20];
                snprintf(buff, sizeof(buff), "%d", i * 25);
                std::string buffAsStdStr = buff;
                //if (dt_frameGraph[i].t1 != nullptr) METAENGINE_Render_FreeImage(dt_frameGraph[i].t1);
                //if (dt_frameGraph[i].t2 != nullptr) METAENGINE_Render_FreeImage(dt_frameGraph[i].t2);
                dt_frameGraph[i] = Drawing::drawTextParams(target, buffAsStdStr.c_str(), font14, WIDTH - 20, HEIGHT - 15 - (i * 25) - 2, 0xff, 0xff, 0xff, ALIGN_LEFT);
            }

            Drawing::drawText(target, dt_frameGraph[i], WIDTH - 20, HEIGHT - 15 - (i * 25) - 2, ALIGN_LEFT);
            METAENGINE_Render_Line(target, WIDTH - 30 - frameTimeNum - 5, HEIGHT - 10 - (i * 25), WIDTH - 25, HEIGHT - 10 - (i * 25), {0xff, 0xff, 0xff, 0xff});
        }
        /*for (int i = 0; i <= 100; i += 25) {
            char buff[20];
            snprintf(buff, sizeof(buff), "%d", i);
            std::string buffAsStdStr = buff;
            Drawing::drawText(renderer, buffAsStdStr.c_str(), font14, WIDTH - 20, HEIGHT - 15 - i - 2, 0xff, 0xff, 0xff, ALIGN_LEFT);
            SDL_RenderDrawLine(renderer, WIDTH - 30 - frameTimeNum - 5, HEIGHT - 10 - i, WIDTH - 25, HEIGHT - 10 - i);
        }*/

        for (int i = 0; i < frameTimeNum; i++) {
            int h = frameTime[i];

            SDL_Color col;
            if (h <= (int) (1000 / 144.0)) {
                col = {0x00, 0xff, 0x00, 0xff};
            } else if (h <= (int) (1000 / 60.0)) {
                col = {0xa0, 0xe0, 0x00, 0xff};
            } else if (h <= (int) (1000 / 30.0)) {
                col = {0xff, 0xff, 0x00, 0xff};
            } else if (h <= (int) (1000 / 15.0)) {
                col = {0xff, 0x80, 0x00, 0xff};
            } else {
                col = {0xff, 0x00, 0x00, 0xff};
            }

            METAENGINE_Render_Line(target, WIDTH - frameTimeNum - 30 + i, HEIGHT - 10 - h, WIDTH - frameTimeNum - 30 + i, HEIGHT - 10, col);
            //SDL_RenderDrawLine(renderer, WIDTH - frameTimeNum - 30 + i, HEIGHT - 10 - h, WIDTH - frameTimeNum - 30 + i, HEIGHT - 10);
        }

        METAENGINE_Render_Line(target, WIDTH - 30 - frameTimeNum - 5, HEIGHT - 10 - (int) (1000.0 / fps), WIDTH - 25, HEIGHT - 10 - (int) (1000.0 / fps), {0x00, 0xff, 0xff, 0xff});
        METAENGINE_Render_Line(target, WIDTH - 30 - frameTimeNum - 5, HEIGHT - 10 - (int) (1000.0 / feelsLikeFps), WIDTH - 25, HEIGHT - 10 - (int) (1000.0 / feelsLikeFps), {0xff, 0x00, 0xff, 0xff});
    }

    METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_NORMAL);


#ifdef DEVELOPMENT_BUILD
    if (dt_versionInfo1.w == -1) {
        char buffDevBuild[40];
        snprintf(buffDevBuild, sizeof(buffDevBuild), "Development Build");
        //if (dt_versionInfo1.t1 != nullptr) METAENGINE_Render_FreeImage(dt_versionInfo1.t1);
        //if (dt_versionInfo1.t2 != nullptr) METAENGINE_Render_FreeImage(dt_versionInfo1.t2);
        dt_versionInfo1 = Drawing::drawTextParams(target, buffDevBuild, font16, 4, HEIGHT - 32 - 13, 0xff, 0xff, 0xff, ALIGN_LEFT);

        char buffVersion[40];
        snprintf(buffVersion, sizeof(buffVersion), "Version %s - dev", VERSION);
        //if (dt_versionInfo2.t1 != nullptr) METAENGINE_Render_FreeImage(dt_versionInfo2.t1);
        //if (dt_versionInfo2.t2 != nullptr) METAENGINE_Render_FreeImage(dt_versionInfo2.t2);
        dt_versionInfo2 = Drawing::drawTextParams(target, buffVersion, font16, 4, HEIGHT - 32, 0xff, 0xff, 0xff, ALIGN_LEFT);

        char buffBuildDate[40];
        snprintf(buffBuildDate, sizeof(buffBuildDate), "%s : %s", __DATE__, __TIME__);
        //if (dt_versionInfo3.t1 != nullptr) METAENGINE_Render_FreeImage(dt_versionInfo3.t1);
        //if (dt_versionInfo3.t2 != nullptr) METAENGINE_Render_FreeImage(dt_versionInfo3.t2);
        dt_versionInfo3 = Drawing::drawTextParams(target, buffBuildDate, font16, 4, HEIGHT - 32 + 13, 0xff, 0xff, 0xff, ALIGN_LEFT);
    }

    Drawing::drawText(target, dt_versionInfo1, 4, HEIGHT - 32 - 13, ALIGN_LEFT);
    Drawing::drawText(target, dt_versionInfo2, 4, HEIGHT - 32, ALIGN_LEFT);
    Drawing::drawText(target, dt_versionInfo3, 4, HEIGHT - 32 + 13, ALIGN_LEFT);
#elif defined ALPHA_BUILD
    char buffDevBuild[40];
    snprintf(buffDevBuild, sizeof(buffDevBuild), "Alpha Build");
    Drawing::drawText(target, buffDevBuild, font16, 4, HEIGHT - 32, 0xff, 0xff, 0xff, ALIGN_LEFT);

    char buffVersion[40];
    snprintf(buffVersion, sizeof(buffVersion), "Version %s - alpha", VERSION);
    Drawing::drawText(target, buffVersion, font16, 4, HEIGHT - 32 + 13, 0xff, 0xff, 0xff, ALIGN_LEFT);
#else
    char buffVersion[40];
    snprintf(buffVersion, sizeof(buffVersion), "Version %s", VERSION);
    Drawing::drawText(target, buffVersion, font16, 4, HEIGHT - 32 + 13, 0xff, 0xff, 0xff, ALIGN_LEFT);
#endif
}

void Game::renderTemperatureMap(World *world) {


    for (int x = 0; x < world->width; x++) {
        for (int y = 0; y < world->height; y++) {
            auto t = world->tiles[x + y * world->width];
            int32_t temp = t.temperature;
            Uint32 color = (Uint8) ((temp + 1024) / 2048.0f * 255);

            const unsigned int offset = (world->width * 4 * y) + x * 4;
            pixelsTemp_ar[offset + 0] = color;// b
            pixelsTemp_ar[offset + 1] = color;// g
            pixelsTemp_ar[offset + 2] = color;// r
            pixelsTemp_ar[offset + 3] = 0xf0; // a
        }
    }
}

int Game::getAimSurface(int dist) {
    int dcx = this->mx - WIDTH / 2;
    int dcy = this->my - HEIGHT / 2;

    float len = sqrtf(dcx * dcx + dcy * dcy);
    float udx = dcx / len;
    float udy = dcy / len;

    int mmx = WIDTH / 2.0f + udx * dist;
    int mmy = HEIGHT / 2.0f + udy * dist;

    int wcx = (int) ((WIDTH / 2.0f - ofsX - camX) / scale);
    int wcy = (int) ((HEIGHT / 2.0f - ofsY - camY) / scale);

    int wmx = (int) ((mmx - ofsX - camX) / scale);
    int wmy = (int) ((mmy - ofsY - camY) / scale);

    int startInd = -1;
    world->forLine(wcx, wcy, wmx, wmy, [&](int ind) {
        if (world->tiles[ind].mat->physicsType == PhysicsType::SOLID || world->tiles[ind].mat->physicsType == PhysicsType::SAND || world->tiles[ind].mat->physicsType == PhysicsType::SOUP) {
            startInd = ind;
            return true;
        }
        return false;
    });

    return startInd;
}

void Game::quitToMainMenu() {


    if (state == LOADING) return;

    std::string pref = "Saved in: ";

    std::string worldName = "mainMenu";
    char *wn = (char *) worldName.c_str();

    METADOT_INFO("Loading main menu @ {}", gameDir.getWorldPath(wn));
    MainMenuUI::visible = false;
    state = LOADING;
    stateAfterLoad = MAIN_MENU;


    delete world;
    world = nullptr;


    WorldGenerator *generator = new MaterialTestGenerator();

    std::string wpStr = gameDir.getWorldPath(wn);


    world = new World();
    world->noSaveLoad = true;
    world->init(wpStr, (int) ceil(Game::MAX_WIDTH / 3 / (double) CHUNK_W) * CHUNK_W + CHUNK_W * 3, (int) ceil(Game::MAX_HEIGHT / 3 / (double) CHUNK_H) * CHUNK_H + CHUNK_H * 3, target, &audioEngine, networkMode, generator);


    METADOT_INFO("Queueing chunk loading...");
    for (int x = -CHUNK_W * 4; x < world->width + CHUNK_W * 4; x += CHUNK_W) {
        for (int y = -CHUNK_H * 3; y < world->height + CHUNK_H * 8; y += CHUNK_H) {
            world->queueLoadChunk(x / CHUNK_W, y / CHUNK_H, true, true);
        }
    }


    std::fill(pixels.begin(), pixels.end(), 0);
    std::fill(pixelsBackground.begin(), pixelsBackground.end(), 0);
    std::fill(pixelsLayer2.begin(), pixelsLayer2.end(), 0);
    std::fill(pixelsFire.begin(), pixelsFire.end(), 0);
    std::fill(pixelsFlow.begin(), pixelsFlow.end(), 0);
    std::fill(pixelsEmission.begin(), pixelsEmission.end(), 0);
    std::fill(pixelsParticles.begin(), pixelsParticles.end(), 0);

    METAENGINE_Render_UpdateImageBytes(
            texture,
            NULL,
            &pixels[0],
            world->width * 4);

    METAENGINE_Render_UpdateImageBytes(
            textureBackground,
            NULL,
            &pixelsBackground[0],
            world->width * 4);

    METAENGINE_Render_UpdateImageBytes(
            textureLayer2,
            NULL,
            &pixelsLayer2[0],
            world->width * 4);

    METAENGINE_Render_UpdateImageBytes(
            textureFire,
            NULL,
            &pixelsFire[0],
            world->width * 4);

    METAENGINE_Render_UpdateImageBytes(
            textureFlow,
            NULL,
            &pixelsFlow[0],
            world->width * 4);

    METAENGINE_Render_UpdateImageBytes(
            emissionTexture,
            NULL,
            &pixelsEmission[0],
            world->width * 4);

    METAENGINE_Render_UpdateImageBytes(
            textureParticles,
            NULL,
            &pixelsParticles[0],
            world->width * 4);


    MainMenuUI::visible = true;
}

int Game::getAimSolidSurface(int dist) {
    int dcx = this->mx - WIDTH / 2;
    int dcy = this->my - HEIGHT / 2;

    float len = sqrtf(dcx * dcx + dcy * dcy);
    float udx = dcx / len;
    float udy = dcy / len;

    int mmx = WIDTH / 2.0f + udx * dist;
    int mmy = HEIGHT / 2.0f + udy * dist;

    int wcx = (int) ((WIDTH / 2.0f - ofsX - camX) / scale);
    int wcy = (int) ((HEIGHT / 2.0f - ofsY - camY) / scale);

    int wmx = (int) ((mmx - ofsX - camX) / scale);
    int wmy = (int) ((mmy - ofsY - camY) / scale);

    int startInd = -1;
    world->forLine(wcx, wcy, wmx, wmy, [&](int ind) {
        if (world->tiles[ind].mat->physicsType == PhysicsType::SOLID) {
            startInd = ind;
            return true;
        }
        return false;
    });

    return startInd;
}
