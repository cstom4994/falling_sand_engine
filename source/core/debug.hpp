// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_DEBUGIMPL_HPP
#define ME_DEBUGIMPL_HPP

#include "core/core.hpp"
#include "core/macros.hpp"
#include "ui/imgui/imgui_impl.hpp"

struct DebugInfo {
    std::string platform;
    std::string compiler;
    std::string compiler_version;
    std::string cpp;
};

int metadot_buildnum(void);
DebugInfo metadot_metadata(void);

#endif