// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Platform.hpp"
#include "Game/Const.hpp"
#include "Game/Legacy/Networking.hpp"
#include "Game/Settings.hpp"
#include "Libs/structopt.hpp"


struct Options
{
    std::optional<std::string> test;
    std::optional<std::string> networkmode;
    std::vector<std::string> files;
};
STRUCTOPT(Options, test, files);

int Platform::ParseRunArgs(int argc, char *argv[]) {
    try {
        auto options = structopt::app(METADOT_NAME).parse<Options>(argc, argv);

        if (!options.networkmode.value_or("").empty()) {
            if (options.networkmode == "server") Settings::networkMode = NetworkMode::SERVER;
        } else {
            Settings::networkMode = NetworkMode::HOST;
        }

        if (!options.test.value_or("").empty()) {
            if (options.test == "test_") {
                return 0;
            }
        }

    } catch (structopt::exception &e) {
        std::cout << e.what() << "\n";
        std::cout << e.help();
    }
    return 0;
}
