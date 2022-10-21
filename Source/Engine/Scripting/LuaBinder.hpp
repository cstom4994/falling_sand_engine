#pragma once

#include "Libs/lua/sol/sol.hpp"

#include "Engine/Reflecting/Refl.hpp"

namespace MetaEngine::LuaBinder {
    void bindEverything(sol::state& state);
}
