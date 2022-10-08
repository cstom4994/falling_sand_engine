// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.

#pragma once

#include "Engine/Macros.hpp"
#include "Engine/Core.hpp"
#include "Engine/DebugImpl.hpp"

//streams
#include <filesystem>
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <set>
#include <mutex>

#include "Engine/Utils.hpp"


/*!
Main namespace of MetaEngine framework. All classes are located here or in nested namespaces
*/
namespace MetaEngine { }


#define DEVELOPMENT_BUILD
#define ALPHA_BUILD

#include <future>
#include <string>
#include <iostream>
#include <algorithm> 
#include <unordered_map>
#include <vector>
#include <deque>
#include <iterator>
#include <filesystem>
#include <regex> 
#include <memory>

#include "SDL.h"

#include "Engine/render/renderer_gpu.h"

#include "lib/AudioEngine/AudioEngine.h"
#include "lib/polypartition.h"
#include "lib/FastNoise/FastNoise.h"
#include "lib/sparsehash/dense_hash_map.h"
