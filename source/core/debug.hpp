// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_DEBUGIMPL_HPP_
#define _METADOT_DEBUGIMPL_HPP_

#include "core/core.hpp"
#include "core/macros.h"
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