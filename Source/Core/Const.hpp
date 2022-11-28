// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_CONST_HPP_
#define _METADOT_CONST_HPP_

#include "Core/Macros.hpp"

static const int WINDOWS_MAX_WIDTH = 1920;
static const int WINDOWS_MAX_HEIGHT = 1080;

static const int RENDER_C_TEST = 3;

static const std::string win_title_client = U8("MetaDot 少女祈祷中");
static const std::string win_title_server = U8("MetaDot Server");
static const std::string win_game = U8("MetaDot");

#define CHUNK_W 128
#define CHUNK_H 128
#define CHUNK_UNLOAD_DIST 16

#define FLUID_MaxValue 0.5f
#define FLUID_MinValue 0.0005f

// Extra liquid a cell can store than the cell above it
#define FLUID_MaxCompression 0.1f

// Lowest and highest amount of liquids allowed to flow per iteration
#define FLUID_MinFlow 0.05f
#define FLUID_MaxFlow 8.0f

// Adjusts flow speed (0.0f - 1.0f)
#define FLUID_FlowSpeed 1.0f

#define METADOT_NAME "MetaDot"
#define METADOT_VERSION_TEXT "0.0.3"
#define METADOT_VERSION_MAJOR 0
#define METADOT_VERSION_MINOR 0
#define METADOT_VERSION_BUILD 3
#define METADOT_VERSION Version(METADOT_VERSION_MAJOR, METADOT_VERSION_MINOR, METADOT_VERSION_BUILD)
#define METADOT_COMPANY "MetaDot"
#define METADOT_COPYRIGHT "Copyright (c) 2022 KaoruXun. All rights reserved."

static const int VERSION_MAJOR = METADOT_VERSION_MAJOR;
static const int VERSION_MINOR = METADOT_VERSION_MINOR;
static const int VERSION_REV = METADOT_VERSION_BUILD;
static const char *VERSION = METADOT_VERSION_TEXT;
static const char *VERSION_COMPATIBILITY[] = {VERSION, "0.0.1", 0};

#endif