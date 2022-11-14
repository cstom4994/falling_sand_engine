#pragma once

#include "Libs/lua/sol/sol.hpp"

#include "Engine/Meta/Refl.hpp"

namespace MetaEngine::LuaBinder {
    void bindEverything(sol::state& state);
}
