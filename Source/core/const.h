// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_CONST_H_
#define _METADOT_CONST_H_

#include "core/macros.h"

static const int WINDOWS_MAX_WIDTH = 1920;
static const int WINDOWS_MAX_HEIGHT = 1080;

static const int RENDER_C_TEST = 3;

static const char *logo = "__  __      _        _____        _   "
                          "|  \/  |    | |      |  __ \      | |  "
                          "| \  / | ___| |_ __ _| |  | | ___ | |_ "
                          "| |\/| |/ _ \ __/ _` | |  | |/ _ \| __|"
                          "| |  | |  __/ || (_| | |__| | (_) | |_ "
                          "|_|  |_|\___|\__\__,_|_____/ \___/ \__|";

static const char *win_title_client = CC("MetaDot 少女祈祷中");
static const char *win_title_server = CC("MetaDot Server");
static const char *win_game = CC("MetaDot");

static const int CHUNK_W = 128;
static const int CHUNK_H = 128;
static const int CHUNK_UNLOAD_DIST = 16;

static const float FLUID_MaxValue = 0.5f;
static const float FLUID_MinValue = 0.0005f;

// Extra liquid a cell can store than the cell above it
static const float FLUID_MaxCompression = 0.1f;

// Lowest and highest amount of liquids allowed to flow per iteration
static const float FLUID_MinFlow = 0.05f;
static const float FLUID_MaxFlow = 8.0f;

// Adjusts flow speed (0.0f - 1.0f)
static const float FLUID_FlowSpeed = 1.0f;

static const int FrameTimeNum = 100;

static const int GameTick = 4;

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
static const char *VERSION_COMPATIBILITY[] = {METADOT_VERSION_TEXT, "0.0.1", 0};

#endif
