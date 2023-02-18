#ifndef meo_core_h
#define meo_core_h

#include "meo_vm.h"

// This module defines the built-in classes and their primitives methods that
// are implemented directly in C code. Some languages try to implement as much
// of the core module itself in the primary language instead of in the host
// language.
//
// With Meo, we try to do as much of it in C as possible. Primitive methods
// are always faster than code written in Meo, and it minimizes startup time
// since we don't have to parse, compile, and execute Meo code.
//
// There is one limitation, though. Methods written in C cannot call Meo ones.
// They can only be the top of the callstack, and immediately return. This
// makes it difficult to have primitive methods that rely on polymorphic
// behavior. For example, `System.print` should call `toString` on its argument,
// including user-defined `toString` methods on user-defined classes.

void meoInitializeCore(MeoVM* vm);

#endif
