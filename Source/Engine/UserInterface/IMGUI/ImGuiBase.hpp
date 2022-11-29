// Copyright(c) 2022, KaoruXun All rights reserved.


#ifndef _METADOT_IMGUIBASE_HPP_
#define _METADOT_IMGUIBASE_HPP_

#include "imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#if defined(_WIN32)
#define _METADOT_IMM32
#else
#include <sys/stat.h>
#endif

#include "Core/Core.hpp"

#include "ImGuiImplGL3.h"
#include "ImGuiImplSDL.h"

#include "Engine/UserInterface/IMGUI/ImGuiHelper.hpp"

#endif