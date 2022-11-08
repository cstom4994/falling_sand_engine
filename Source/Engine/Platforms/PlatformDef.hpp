// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Game/Macros.hpp"

#include <string>

#if __cplusplus <= 201402L
#error "TODO: fix for this compiler! (at least C++14 is required)"
#endif

namespace MetaEngine {
    namespace Platforms {

        const std::string &GetExecutablePath();
    }
}// namespace MetaEngine

