// Copyright(c) 2022, KaoruXun All rights reserved.

#pragma once

#include "container.h"
#include "metafunctions.h"

#include <cstdint>

namespace MetaEngine::ECS {
    namespace detail {
        using entity_id_t = std::uintmax_t;
    }// namespace detail

    template<typename... Ts>
    struct tag_list
    {
        static_assert(meta::is_typelist_unique_v<meta::typelist<Ts...>>, "tag_list must be unique");
    };

    template<typename... Ts>
    struct component_list
    {
        static_assert(meta::is_typelist_unique_v<meta::typelist<Ts...>>,
                      "component_list must be unique");

        template<typename T>
        using container_type = flat_map<detail::entity_id_t, T>;
        using type = std::tuple<container_type<Ts>...>;
    };
}// namespace MetaEngine::ECS
