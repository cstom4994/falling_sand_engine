#pragma once

#include "Libs/lua/sol/sol.hpp"

#include "Engine/Refl.hpp"

namespace MetaEngine::LuaBinder {
    void bindEverything(sol::state& state);
}
