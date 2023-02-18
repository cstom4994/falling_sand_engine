#ifndef meo_debug_h
#define meo_debug_h

#include "meo_value.h"
#include "meo_vm.h"

// Prints the stack trace for the current fiber.
//
// Used when a fiber throws a runtime error which is not caught.
void meoDebugPrintStackTrace(MeoVM* vm);

// The "dump" functions are used for debugging Meo itself. Normal code paths
// will not call them unless one of the various DEBUG_ flags is enabled.

// Prints a representation of [value] to stdout.
void meoDumpValue(Value value);

// Prints a representation of the bytecode for [fn] at instruction [i].
int meoDumpInstruction(MeoVM* vm, ObjFn* fn, int i);

// Prints the disassembled code for [fn] to stdout.
void meoDumpCode(MeoVM* vm, ObjFn* fn);

// Prints the contents of the current stack for [fiber] to stdout.
void meoDumpStack(ObjFiber* fiber);

#endif
