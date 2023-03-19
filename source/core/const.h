// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_CONST_H_
#define _METADOT_CONST_H_

#include "core/macros.h"

static const int WINDOWS_MAX_WIDTH = 1920;
static const int WINDOWS_MAX_HEIGHT = 1080;

static const int RENDER_C_TEST = 3;

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

#define TraceTimeNum 60

static const double PI = 3.14159265358979323846;

static const int GameTick = 4;

#define METADOT_NAME "MetaDot"
#define METADOT_VERSION_TEXT "0.0.3"
#define METADOT_VERSION_MAJOR 0
#define METADOT_VERSION_MINOR 0
#define METADOT_VERSION_BUILD 3
#define METADOT_COMPANY "MetaDot"
#define METADOT_COPYRIGHT "Copyright (c) 2022-2023 KaoruXun. All rights reserved."

#define JRPC_VERSION "2.0"

#define RAD_PER_DEG 0.017453293f
#define DEG_PER_RAD 57.2957795f

static const int VERSION_MAJOR = METADOT_VERSION_MAJOR;
static const int VERSION_MINOR = METADOT_VERSION_MINOR;
static const int VERSION_REV = METADOT_VERSION_BUILD;
static const char *VERSION_COMPATIBILITY[] = {METADOT_VERSION_TEXT, "0.0.1", 0};

static const char *engine_funcs_name_gpu = "_metadot_gpu";
static const char *engine_funcs_name_fs = "_metadot_fs";
static const char *engine_funcs_name_fs_internal = "_metadot_fs_m";
static const char *engine_funcs_name_image = "_metadot_image";
static const char *engine_funcs_name_image_internal = "_metadot_image_m";
static const char *engine_funcs_name_lz4 = "_metadot_lz4";
static const char *engine_funcs_name_csc = "_metadot_cstruct_core";
static const char *engine_funcs_name_cst = "_metadot_cstruct_test";
static const char *engine_funcs_name_ecs = "_metadot_cecs";
static const char *engine_funcs_name_uilayout = "_metadot_ui_layout";

#ifdef __cplusplus

namespace UMeta {
static constexpr char nameof_namespace[] = "UMeta";

static constexpr char initializer[] = "UMeta::initializer";
static constexpr char default_functions[] = "UMeta::default_functions";
static constexpr char constructor[] = "UMeta::constructor";
static constexpr char destructor[] = "UMeta::destructor";

static constexpr char nameof_initializer[] = "initializer";
static constexpr char nameof_default_functions[] = "default_functions";
static constexpr char nameof_constructor[] = "constructor";
static constexpr char nameof_destructor[] = "destructor";
}  // namespace UMeta

#endif

#endif
