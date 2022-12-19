// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_PLATFORMDETAIL_H_
#define _METADOT_PLATFORMDETAIL_H_

#include "core/macros.h"

#if defined(METADOT_PLATFORM_WINDOWS)
#include <Windows.h>
#include <io.h>
#include <shobjidl.h>

#if defined(_MSC_VER)
#define __func__ __FUNCTION__
#endif

#undef min
#undef max

#define PATH_SEP '\\'
#ifndef PATH_MAX
#define PATH_MAX 260
#endif

#elif defined(METADOT_PLATFORM_LINUX)
#include <bits/types/struct_tm.h>
#include <bits/types/time_t.h>
#include <dirent.h>
#include <limits.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <sys/types.h>
#define PATH_SEP '/'
#elif defined(METADOT_PLATFORM_APPLE)
#include <TargetConditionals.h>
#include <mach-o/dyld.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#endif

#ifdef METADOT_PLATFORM_WINDOWS
#define getFullPath(a, b) GetFullPathName(a, MAX_PATH, b, NULL)
#define rmdir(a) _rmdir(a)
#define PATH_SEPARATOR '\\'
#else
#define getFullPath(a, b) realpath(a, b)
#define PATH_SEPARATOR '/'
#endif

#ifndef MAX_PATH
#include <limits.h>
#ifdef PATH_MAX
#define MAX_PATH PATH_MAX
#else
#define MAX_PATH 256
#endif
#endif

#if MAX_PATH > 1024
#undef MAX_PATH
#define MAX_PATH 1024
#endif

#define FS_LINE_INCR 256

#endif  // !_METADOT_PLATFORMDETAIL_H_
