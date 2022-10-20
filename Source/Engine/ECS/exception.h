// Copyright(c) 2022, KaoruXun All rights reserved.

#pragma once

#include <cassert>

#define METADOT_ECS_CHECK(condition, error) \
    if constexpr (!(condition)) {          \
        static_assert((condition), error); \
    }
#define METADOT_ECS_CHECK_ALSO(condition, error) \
    else if constexpr (!(condition)) {          \
        static_assert((condition), error);      \
    }

#ifndef METADOT_ECS_NO_EXCEPTIONS
#include <stdexcept>
#endif

namespace MetaEngine::ECS {
    enum class error_code {
        BAD_ENTITY,
        INVALID_COMPONENT
    };

#ifndef METADOT_ECS_NO_EXCEPTIONS

    struct bad_entity : std::logic_error
    {
        using std::logic_error::logic_error;
    };

    struct invalid_component : std::logic_error
    {
        using std::logic_error::logic_error;
    };

#endif

}// namespace MetaEngine::ECS
