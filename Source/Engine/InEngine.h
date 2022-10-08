// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.

#pragma once

#include "Engine/Core.hpp"
#include "Engine/DebugImpl.hpp"
#include "Engine/Macros.hpp"

//streams
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Engine/Utils.hpp"


/*!
Main namespace of MetaEngine framework. All classes are located here or in nested namespaces
*/
namespace MetaEngine {}


#define DEVELOPMENT_BUILD
#define ALPHA_BUILD

#include <algorithm>
#include <deque>
#include <filesystem>
#include <future>
#include <iostream>
#include <iterator>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#include "SDL.h"

#include "Engine/render/renderer_gpu.h"

#include "lib/AudioEngine/AudioEngine.h"
#include "lib/FastNoise/FastNoise.h"
#include "lib/polypartition.h"
#include "lib/sparsehash/dense_hash_map.h"
