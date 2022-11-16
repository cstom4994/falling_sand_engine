// Copyright(c) 2022, KaoruXun All rights reserved.

#pragma once

#include "Macros.hpp"

static const int WINDOWS_MAX_WIDTH = 1920;
static const int WINDOWS_MAX_HEIGHT = 1080;

static const int RENDER_C_TEST = 3;

static const std::string win_title_client = U8("MetaDot 少女祈祷中");
static const std::string win_title_server = U8("MetaDot Server");
static const std::string win_game = U8("MetaDot");

static const int VERSION_MAJOR = METADOT_VERSION_MAJOR;
static const int VERSION_MINOR = METADOT_VERSION_MINOR;
static const int VERSION_REV = METADOT_VERSION_BUILD;
static const char *VERSION = METADOT_VERSION_TEXT;
static const char *VERSION_COMPATIBILITY[] = {VERSION, "0.0.1", 0};