#ifndef meo_opt_meta_h
#define meo_opt_meta_h

#include "meo.h"
#include "meo_common.h"

// This module defines the Meta class and its associated methods.
#if MEO_OPT_META

const char* meoMetaSource();
MeoForeignMethodFn meoMetaBindForeignMethod(MeoVM* vm, const char* className, bool isStatic, const char* signature);

#endif

#if MEO_OPT_RANDOM

const char* meoRandomSource();
MeoForeignClassMethods meoRandomBindForeignClass(MeoVM* vm, const char* module, const char* className);
MeoForeignMethodFn meoRandomBindForeignMethod(MeoVM* vm, const char* className, bool isStatic, const char* signature);

#endif

#endif
