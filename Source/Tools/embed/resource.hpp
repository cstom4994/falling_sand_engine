// Copyright(c) 2022, KaoruXun All rights reserved.

#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <tuple>
#include <span>


class Resource {
private:
    const std::span<const std::uint8_t> arr_view;
    const std::string path;

public:
    using EmbeddedData = std::span<const std::uint8_t>;

    template<std::size_t sz>
    constexpr Resource(const std::array<std::uint8_t, sz> &arr_, const char *path_) : arr_view(arr_), path(path_) {}
    Resource(const std::vector<std::uint8_t> &vec_, std::string path_) noexcept : arr_view(vec_), path(std::move(path_)) {}

    constexpr auto GetArray() const {
        return arr_view;
    }

    constexpr auto &GetPath() const {
        return path;
    }
};
