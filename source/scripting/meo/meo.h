#ifndef meo_h
#define meo_h

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

// A single virtual machine for executing Meo code.
//
// Meo has no global state, so all state stored by a running interpreter lives
// here.
typedef struct MeoVM MeoVM;

// A handle to a Meo object.
//
// This lets code outside of the VM hold a persistent reference to an object.
// After a handle is acquired, and until it is released, this ensures the
// garbage collector will not reclaim the object it references.
typedef struct MeoHandle MeoHandle;

// A generic allocation function that handles all explicit memory management
// used by Meo. It's used like so:
//
// - To allocate new memory, [memory] is NULL and [newSize] is the desired
//   size. It should return the allocated memory or NULL on failure.
//
// - To attempt to grow an existing allocation, [memory] is the memory, and
//   [newSize] is the desired size. It should return [memory] if it was able to
//   grow it in place, or a new pointer if it had to move it.
//
// - To shrink memory, [memory] and [newSize] are the same as above but it will
//   always return [memory].
//
// - To free memory, [memory] will be the memory to free and [newSize] will be
//   zero. It should return NULL.
typedef void* (*MeoReallocateFn)(void* memory, size_t newSize, void* userData);

// A function callable from Meo code, but implemented in C.
typedef void (*MeoForeignMethodFn)(MeoVM* vm);

// A finalizer function for freeing resources owned by an instance of a foreign
// class. Unlike most foreign methods, finalizers do not have access to the VM
// and should not interact with it since it's in the middle of a garbage
// collection.
typedef void (*MeoFinalizerFn)(void* data);

// Gives the host a chance to canonicalize the imported module name,
// potentially taking into account the (previously resolved) name of the module
// that contains the import. Typically, this is used to implement relative
// imports.
typedef const char* (*MeoResolveModuleFn)(MeoVM* vm, const char* importer, const char* name);

// Forward declare
struct MeoLoadModuleResult;

// Called after loadModuleFn is called for module [name]. The original returned result
// is handed back to you in this callback, so that you can free memory if appropriate.
typedef void (*MeoLoadModuleCompleteFn)(MeoVM* vm, const char* name, struct MeoLoadModuleResult result);

// The result of a loadModuleFn call.
// [source] is the source code for the module, or NULL if the module is not found.
// [onComplete] an optional callback that will be called once Meo is done with the result.
typedef struct MeoLoadModuleResult {
    const char* source;
    MeoLoadModuleCompleteFn onComplete;
    void* userData;
} MeoLoadModuleResult;

// Loads and returns the source code for the module [name].
typedef MeoLoadModuleResult (*MeoLoadModuleFn)(MeoVM* vm, const char* name);

// Returns a pointer to a foreign method on [className] in [module] with
// [signature].
typedef MeoForeignMethodFn (*MeoBindForeignMethodFn)(MeoVM* vm, const char* module, const char* className, bool isStatic, const char* signature);

// Displays a string of text to the user.
typedef void (*MeoWriteFn)(MeoVM* vm, const char* text);

typedef enum {
    // A syntax or resolution error detected at compile time.
    MEO_ERROR_COMPILE,

    // The error message for a runtime error.
    MEO_ERROR_RUNTIME,

    // One entry of a runtime error's stack trace.
    MEO_ERROR_STACK_TRACE
} MeoErrorType;

// Reports an error to the user.
//
// An error detected during compile time is reported by calling this once with
// [type] `MEO_ERROR_COMPILE`, the resolved name of the [module] and [line]
// where the error occurs, and the compiler's error [message].
//
// A runtime error is reported by calling this once with [type]
// `MEO_ERROR_RUNTIME`, no [module] or [line], and the runtime error's
// [message]. After that, a series of [type] `MEO_ERROR_STACK_TRACE` calls are
// made for each line in the stack trace. Each of those has the resolved
// [module] and [line] where the method or function is defined and [message] is
// the name of the method or function.
typedef void (*MeoErrorFn)(MeoVM* vm, MeoErrorType type, const char* module, int line, const char* message);

typedef struct {
    // The callback invoked when the foreign object is created.
    //
    // This must be provided. Inside the body of this, it must call
    // [meoSetSlotNewForeign()] exactly once.
    MeoForeignMethodFn allocate;

    // The callback invoked when the garbage collector is about to collect a
    // foreign object's memory.
    //
    // This may be `NULL` if the foreign class does not need to finalize.
    MeoFinalizerFn finalize;
} MeoForeignClassMethods;

// Returns a pair of pointers to the foreign methods used to allocate and
// finalize the data for instances of [className] in resolved [module].
typedef MeoForeignClassMethods (*MeoBindForeignClassFn)(MeoVM* vm, const char* module, const char* className);

typedef struct {
    // The callback Meo will use to allocate, reallocate, and deallocate memory.
    //
    // If `NULL`, defaults to a built-in function that uses `realloc` and `free`.
    MeoReallocateFn reallocateFn;

    // The callback Meo uses to resolve a module name.
    //
    // Some host applications may wish to support "relative" imports, where the
    // meaning of an import string depends on the module that contains it. To
    // support that without baking any policy into Meo itself, the VM gives the
    // host a chance to resolve an import string.
    //
    // Before an import is loaded, it calls this, passing in the name of the
    // module that contains the import and the import string. The host app can
    // look at both of those and produce a new "canonical" string that uniquely
    // identifies the module. This string is then used as the name of the module
    // going forward. It is what is passed to [loadModuleFn], how duplicate
    // imports of the same module are detected, and how the module is reported in
    // stack traces.
    //
    // If you leave this function NULL, then the original import string is
    // treated as the resolved string.
    //
    // If an import cannot be resolved by the embedder, it should return NULL and
    // Meo will report that as a runtime error.
    //
    // Meo will take ownership of the string you return and free it for you, so
    // it should be allocated using the same allocation function you provide
    // above.
    MeoResolveModuleFn resolveModuleFn;

    // The callback Meo uses to load a module.
    //
    // Since Meo does not talk directly to the file system, it relies on the
    // embedder to physically locate and read the source code for a module. The
    // first time an import appears, Meo will call this and pass in the name of
    // the module being imported. The method will return a result, which contains
    // the source code for that module. Memory for the source is owned by the
    // host application, and can be freed using the onComplete callback.
    //
    // This will only be called once for any given module name. Meo caches the
    // result internally so subsequent imports of the same module will use the
    // previous source and not call this.
    //
    // If a module with the given name could not be found by the embedder, it
    // should return NULL and Meo will report that as a runtime error.
    MeoLoadModuleFn loadModuleFn;

    // The callback Meo uses to find a foreign method and bind it to a class.
    //
    // When a foreign method is declared in a class, this will be called with the
    // foreign method's module, class, and signature when the class body is
    // executed. It should return a pointer to the foreign function that will be
    // bound to that method.
    //
    // If the foreign function could not be found, this should return NULL and
    // Meo will report it as runtime error.
    MeoBindForeignMethodFn bindForeignMethodFn;

    // The callback Meo uses to find a foreign class and get its foreign methods.
    //
    // When a foreign class is declared, this will be called with the class's
    // module and name when the class body is executed. It should return the
    // foreign functions uses to allocate and (optionally) finalize the bytes
    // stored in the foreign object when an instance is created.
    MeoBindForeignClassFn bindForeignClassFn;

    // The callback Meo uses to display text when `System.print()` or the other
    // related functions are called.
    //
    // If this is `NULL`, Meo discards any printed text.
    MeoWriteFn writeFn;

    // The callback Meo uses to report errors.
    //
    // When an error occurs, this will be called with the module name, line
    // number, and an error message. If this is `NULL`, Meo doesn't report any
    // errors.
    MeoErrorFn errorFn;

    // The number of bytes Meo will allocate before triggering the first garbage
    // collection.
    //
    // If zero, defaults to 10MB.
    size_t initialHeapSize;

    // After a collection occurs, the threshold for the next collection is
    // determined based on the number of bytes remaining in use. This allows Meo
    // to shrink its memory usage automatically after reclaiming a large amount
    // of memory.
    //
    // This can be used to ensure that the heap does not get too small, which can
    // in turn lead to a large number of collections afterwards as the heap grows
    // back to a usable size.
    //
    // If zero, defaults to 1MB.
    size_t minHeapSize;

    // Meo will resize the heap automatically as the number of bytes
    // remaining in use after a collection changes. This number determines the
    // amount of additional memory Meo will use after a collection, as a
    // percentage of the current heap size.
    //
    // For example, say that this is 50. After a garbage collection, when there
    // are 400 bytes of memory still in use, the next collection will be triggered
    // after a total of 600 bytes are allocated (including the 400 already in
    // use.)
    //
    // Setting this to a smaller number wastes less memory, but triggers more
    // frequent garbage collections.
    //
    // If zero, defaults to 50.
    int heapGrowthPercent;

    // User-defined data associated with the VM.
    void* userData;

} MeoConfiguration;

typedef enum { MEO_RESULT_SUCCESS, MEO_RESULT_COMPILE_ERROR, MEO_RESULT_RUNTIME_ERROR } MeoInterpretResult;

// The type of an object stored in a slot.
//
// This is not necessarily the object's *class*, but instead its low level
// representation type.
typedef enum {
    MEO_TYPE_BOOL,
    MEO_TYPE_NUM,
    MEO_TYPE_FOREIGN,
    MEO_TYPE_LIST,
    MEO_TYPE_MAP,
    MEO_TYPE_NULL,
    MEO_TYPE_STRING,

    // The object is of a type that isn't accessible by the C API.
    MEO_TYPE_UNKNOWN
} MeoType;

// Get the current meo version number.
//
// Can be used to range checks over versions.
int meoGetVersionNumber();

// Initializes [configuration] with all of its default values.
//
// Call this before setting the particular fields you care about.
void meoInitConfiguration(MeoConfiguration* configuration);

// Creates a new Meo virtual machine using the given [configuration]. Meo
// will copy the configuration data, so the argument passed to this can be
// freed after calling this. If [configuration] is `NULL`, uses a default
// configuration.
MeoVM* meoNewVM(MeoConfiguration* configuration);

// Disposes of all resources is use by [vm], which was previously created by a
// call to [meoNewVM].
void meoFreeVM(MeoVM* vm);

// Immediately run the garbage collector to free unused memory.
void meoCollectGarbage(MeoVM* vm);

// Runs [source], a string of Meo source code in a new fiber in [vm] in the
// context of resolved [module].
MeoInterpretResult meoInterpret(MeoVM* vm, const char* module, const char* source);

// Creates a handle that can be used to invoke a method with [signature] on
// using a receiver and arguments that are set up on the stack.
//
// This handle can be used repeatedly to directly invoke that method from C
// code using [meoCall].
//
// When you are done with this handle, it must be released using
// [meoReleaseHandle].
MeoHandle* meoMakeCallHandle(MeoVM* vm, const char* signature);

// Calls [method], using the receiver and arguments previously set up on the
// stack.
//
// [method] must have been created by a call to [meoMakeCallHandle]. The
// arguments to the method must be already on the stack. The receiver should be
// in slot 0 with the remaining arguments following it, in order. It is an
// error if the number of arguments provided does not match the method's
// signature.
//
// After this returns, you can access the return value from slot 0 on the stack.
MeoInterpretResult meoCall(MeoVM* vm, MeoHandle* method);

// Releases the reference stored in [handle]. After calling this, [handle] can
// no longer be used.
void meoReleaseHandle(MeoVM* vm, MeoHandle* handle);

// The following functions are intended to be called from foreign methods or
// finalizers. The interface Meo provides to a foreign method is like a
// register machine: you are given a numbered array of slots that values can be
// read from and written to. Values always live in a slot (unless explicitly
// captured using meoGetSlotHandle(), which ensures the garbage collector can
// find them.
//
// When your foreign function is called, you are given one slot for the receiver
// and each argument to the method. The receiver is in slot 0 and the arguments
// are in increasingly numbered slots after that. You are free to read and
// write to those slots as you want. If you want more slots to use as scratch
// space, you can call meoEnsureSlots() to add more.
//
// When your function returns, every slot except slot zero is discarded and the
// value in slot zero is used as the return value of the method. If you don't
// store a return value in that slot yourself, it will retain its previous
// value, the receiver.
//
// While Meo is dynamically typed, C is not. This means the C interface has to
// support the various types of primitive values a Meo variable can hold: bool,
// double, string, etc. If we supported this for every operation in the C API,
// there would be a combinatorial explosion of functions, like "get a
// double-valued element from a list", "insert a string key and double value
// into a map", etc.
//
// To avoid that, the only way to convert to and from a raw C value is by going
// into and out of a slot. All other functions work with values already in a
// slot. So, to add an element to a list, you put the list in one slot, and the
// element in another. Then there is a single API function meoInsertInList()
// that takes the element out of that slot and puts it into the list.
//
// The goal of this API is to be easy to use while not compromising performance.
// The latter means it does not do type or bounds checking at runtime except
// using assertions which are generally removed from release builds. C is an
// unsafe language, so it's up to you to be careful to use it correctly. In
// return, you get a very fast FFI.

// Returns the number of slots available to the current foreign method.
int meoGetSlotCount(MeoVM* vm);

// Ensures that the foreign method stack has at least [numSlots] available for
// use, growing the stack if needed.
//
// Does not shrink the stack if it has more than enough slots.
//
// It is an error to call this from a finalizer.
void meoEnsureSlots(MeoVM* vm, int numSlots);

// Gets the type of the object in [slot].
MeoType meoGetSlotType(MeoVM* vm, int slot);

// Reads a boolean value from [slot].
//
// It is an error to call this if the slot does not contain a boolean value.
bool meoGetSlotBool(MeoVM* vm, int slot);

// Reads a byte array from [slot].
//
// The memory for the returned string is owned by Meo. You can inspect it
// while in your foreign method, but cannot keep a pointer to it after the
// function returns, since the garbage collector may reclaim it.
//
// Returns a pointer to the first byte of the array and fill [length] with the
// number of bytes in the array.
//
// It is an error to call this if the slot does not contain a string.
const char* meoGetSlotBytes(MeoVM* vm, int slot, int* length);

// Reads a number from [slot].
//
// It is an error to call this if the slot does not contain a number.
double meoGetSlotDouble(MeoVM* vm, int slot);

// Reads a foreign object from [slot] and returns a pointer to the foreign data
// stored with it.
//
// It is an error to call this if the slot does not contain an instance of a
// foreign class.
void* meoGetSlotForeign(MeoVM* vm, int slot);

// Reads a string from [slot].
//
// The memory for the returned string is owned by Meo. You can inspect it
// while in your foreign method, but cannot keep a pointer to it after the
// function returns, since the garbage collector may reclaim it.
//
// It is an error to call this if the slot does not contain a string.
const char* meoGetSlotString(MeoVM* vm, int slot);

// Creates a handle for the value stored in [slot].
//
// This will prevent the object that is referred to from being garbage collected
// until the handle is released by calling [meoReleaseHandle()].
MeoHandle* meoGetSlotHandle(MeoVM* vm, int slot);

// Stores the boolean [value] in [slot].
void meoSetSlotBool(MeoVM* vm, int slot, bool value);

// Stores the array [length] of [bytes] in [slot].
//
// The bytes are copied to a new string within Meo's heap, so you can free
// memory used by them after this is called.
void meoSetSlotBytes(MeoVM* vm, int slot, const char* bytes, size_t length);

// Stores the numeric [value] in [slot].
void meoSetSlotDouble(MeoVM* vm, int slot, double value);

// Creates a new instance of the foreign class stored in [classSlot] with [size]
// bytes of raw storage and places the resulting object in [slot].
//
// This does not invoke the foreign class's constructor on the new instance. If
// you need that to happen, call the constructor from Meo, which will then
// call the allocator foreign method. In there, call this to create the object
// and then the constructor will be invoked when the allocator returns.
//
// Returns a pointer to the foreign object's data.
void* meoSetSlotNewForeign(MeoVM* vm, int slot, int classSlot, size_t size);

// Stores a new empty list in [slot].
void meoSetSlotNewList(MeoVM* vm, int slot);

// Stores a new empty map in [slot].
void meoSetSlotNewMap(MeoVM* vm, int slot);

// Stores null in [slot].
void meoSetSlotNull(MeoVM* vm, int slot);

// Stores the string [text] in [slot].
//
// The [text] is copied to a new string within Meo's heap, so you can free
// memory used by it after this is called. The length is calculated using
// [strlen()]. If the string may contain any null bytes in the middle, then you
// should use [meoSetSlotBytes()] instead.
void meoSetSlotString(MeoVM* vm, int slot, const char* text);

// Stores the value captured in [handle] in [slot].
//
// This does not release the handle for the value.
void meoSetSlotHandle(MeoVM* vm, int slot, MeoHandle* handle);

// Returns the number of elements in the list stored in [slot].
int meoGetListCount(MeoVM* vm, int slot);

// Reads element [index] from the list in [listSlot] and stores it in
// [elementSlot].
void meoGetListElement(MeoVM* vm, int listSlot, int index, int elementSlot);

// Sets the value stored at [index] in the list at [listSlot],
// to the value from [elementSlot].
void meoSetListElement(MeoVM* vm, int listSlot, int index, int elementSlot);

// Takes the value stored at [elementSlot] and inserts it into the list stored
// at [listSlot] at [index].
//
// As in Meo, negative indexes can be used to insert from the end. To append
// an element, use `-1` for the index.
void meoInsertInList(MeoVM* vm, int listSlot, int index, int elementSlot);

// Returns the number of entries in the map stored in [slot].
int meoGetMapCount(MeoVM* vm, int slot);

// Returns true if the key in [keySlot] is found in the map placed in [mapSlot].
bool meoGetMapContainsKey(MeoVM* vm, int mapSlot, int keySlot);

// Retrieves a value with the key in [keySlot] from the map in [mapSlot] and
// stores it in [valueSlot].
void meoGetMapValue(MeoVM* vm, int mapSlot, int keySlot, int valueSlot);

// Takes the value stored at [valueSlot] and inserts it into the map stored
// at [mapSlot] with key [keySlot].
void meoSetMapValue(MeoVM* vm, int mapSlot, int keySlot, int valueSlot);

// Removes a value from the map in [mapSlot], with the key from [keySlot],
// and place it in [removedValueSlot]. If not found, [removedValueSlot] is
// set to null, the same behaviour as the Meo Map API.
void meoRemoveMapValue(MeoVM* vm, int mapSlot, int keySlot, int removedValueSlot);

// Looks up the top level variable with [name] in resolved [module] and stores
// it in [slot].
void meoGetVariable(MeoVM* vm, const char* module, const char* name, int slot);

// Looks up the top level variable with [name] in resolved [module],
// returns false if not found. The module must be imported at the time,
// use meoHasModule to ensure that before calling.
bool meoHasVariable(MeoVM* vm, const char* module, const char* name);

// Returns true if [module] has been imported/resolved before, false if not.
bool meoHasModule(MeoVM* vm, const char* module);

// Sets the current fiber to be aborted, and uses the value in [slot] as the
// runtime error object.
void meoAbortFiber(MeoVM* vm, int slot);

// Returns the user data associated with the MeoVM.
void* meoGetUserData(MeoVM* vm);

// Sets user data associated with the MeoVM.
void meoSetUserData(MeoVM* vm, void* userData);

#endif
