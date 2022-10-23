// Copyright(c) 2022, KaoruXun All rights reserved.

#include "embed.hpp"
#include <iostream>

namespace {

    auto ParseCommandLine(tcb::span<char *> arguments) {
        auto dst = std::pair(std::string(), std::vector<std::string>());
        auto find_output_flag = [](auto &val) {
            return std::string(val) == "-o";
        };

        auto output_location = std::find_if(arguments.begin(), arguments.end(), find_output_flag);

        if (output_location == arguments.end()) {
            throw std::runtime_error("");
        }
        try {
            if (std::distance(arguments.begin(), output_location) > static_cast<std::int32_t>(arguments.size()) - 2)
                throw std::runtime_error("Invalid output options");

            if (std::count_if(arguments.begin(), arguments.end(), find_output_flag) != 1)
                throw std::runtime_error("Output path option collision");

            dst.second.assign(arguments.begin(), output_location);
            dst.second.insert(dst.second.end(), output_location + 2, arguments.end());
            dst.first = *(output_location + 1);
        } catch (const std::exception &e) {
            std::cout << e.what() << std::endl
                      << std::endl;
            std::cout << "Given: embed.exe ";
            for (auto &el: arguments)
                std::cout << el << " ";
            std::cout << std::endl
                      << std::endl;
        }

        return dst;
    }
}// namespace

int main(int argc, char *argv[]) {
    try {
        auto [root, entries] = ParseCommandLine(tcb::span(argv + 1, argc - 1));

        Embed e(root);

        e.SaveAll(entries);
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}