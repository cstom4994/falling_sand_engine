#ifndef meo_common_h
#define meo_common_h

// This header contains macros and defines used across the entire Meo
// implementation. In particular, it contains "configuration" defines that
// control how Meo works. Some of these are only used while hacking on Meo
// itself.
//
// This header is *not* intended to be included by code outside of Meo itself.

// Meo pervasively uses the C99 integer types (uint16_t, etc.) along with some
// of the associated limit constants (UINT32_MAX, etc.). The constants are not
// part of standard C++, so aren't included by default by C++ compilers when you
// include <stdint> unless __STDC_LIMIT_MACROS is defined.
#define __STDC_LIMIT_MACROS
#include <stdint.h>

// These flags let you control some details of the interpreter's implementation.
// Usually they trade-off a bit of portability for speed. They default to the
// most efficient behavior.

// If true, then Meo uses a NaN-tagged double for its core value
// representation. Otherwise, it uses a larger more conventional struct. The
// former is significantly faster and more compact. The latter is useful for
// debugging and may be more portable.
//
// Defaults to on.
#ifndef MEO_NAN_TAGGING
#define MEO_NAN_TAGGING 1
#endif

// If true, the VM's interpreter loop uses computed gotos. See this for more:
// http://gcc.gnu.org/onlinedocs/gcc-3.1.1/gcc/Labels-as-Values.html
// Enabling this speeds up the main dispatch loop a bit, but requires compiler
// support.
// see https://bullno1.com/blog/switched-goto for alternative
// Defaults to true on supported compilers.
#ifndef MEO_COMPUTED_GOTO
#if defined(_MSC_VER) && !defined(__clang__)
// No computed gotos in Visual Studio.
#define MEO_COMPUTED_GOTO 0
#else
#define MEO_COMPUTED_GOTO 1
#endif
#endif

// The VM includes a number of optional modules. You can choose to include
// these or not. By default, they are all available. To disable one, set the
// corresponding `MEO_OPT_<name>` define to `0`.
#ifndef MEO_OPT_META
#define MEO_OPT_META 1
#endif

#ifndef MEO_OPT_RANDOM
#define MEO_OPT_RANDOM 1
#endif

// These flags are useful for debugging and hacking on Meo itself. They are not
// intended to be used for production code. They default to off.

// Set this to true to stress test the GC. It will perform a collection before
// every allocation. This is useful to ensure that memory is always correctly
// reachable.
#define MEO_DEBUG_GC_STRESS 0

// Set this to true to log memory operations as they occur.
#define MEO_DEBUG_TRACE_MEMORY 0

// Set this to true to log garbage collections as they occur.
#define MEO_DEBUG_TRACE_GC 0

// Set this to true to print out the compiled bytecode of each function.
#define MEO_DEBUG_DUMP_COMPILED_CODE 0

// Set this to trace each instruction as it's executed.
#define MEO_DEBUG_TRACE_INSTRUCTIONS 0

// The maximum number of module-level variables that may be defined at one time.
// This limitation comes from the 16 bits used for the arguments to
// `CODE_LOAD_MODULE_VAR` and `CODE_STORE_MODULE_VAR`.
#define MAX_MODULE_VARS 65536

// The maximum number of arguments that can be passed to a method. Note that
// this limitation is hardcoded in other places in the VM, in particular, the
// `CODE_CALL_XX` instructions assume a certain maximum number.
#define MAX_PARAMETERS 16

// The maximum name of a method, not including the signature. This is an
// arbitrary but enforced maximum just so we know how long the method name
// strings need to be in the parser.
#define MAX_METHOD_NAME 64

// The maximum length of a method signature. Signatures look like:
//
//     foo        // Getter.
//     foo()      // No-argument method.
//     foo(_)     // One-argument method.
//     foo(_,_)   // Two-argument method.
//     init foo() // Constructor initializer.
//
// The maximum signature length takes into account the longest method name, the
// maximum number of parameters with separators between them, "init ", and "()".
#define MAX_METHOD_SIGNATURE (MAX_METHOD_NAME + (MAX_PARAMETERS * 2) + 6)

// The maximum length of an identifier. The only real reason for this limitation
// is so that error messages mentioning variables can be stack allocated.
#define MAX_VARIABLE_NAME 64

// The maximum number of fields a class can have, including inherited fields.
// This is explicit in the bytecode since `CODE_CLASS` and `CODE_SUBCLASS` take
// a single byte for the number of fields. Note that it's 255 and not 256
// because creating a class takes the *number* of fields, not the *highest
// field index*.
#define MAX_FIELDS 255

// Use the VM's allocator to allocate an object of [type].
#define ALLOCATE(vm, type) ((type*)meoReallocate(vm, NULL, 0, sizeof(type)))

// Use the VM's allocator to allocate an object of [mainType] containing a
// flexible array of [count] objects of [arrayType].
#define ALLOCATE_FLEX(vm, mainType, arrayType, count) ((mainType*)meoReallocate(vm, NULL, 0, sizeof(mainType) + sizeof(arrayType) * (count)))

// Use the VM's allocator to allocate an array of [count] elements of [type].
#define ALLOCATE_ARRAY(vm, type, count) ((type*)meoReallocate(vm, NULL, 0, sizeof(type) * (count)))

// Use the VM's allocator to free the previously allocated memory at [pointer].
#define DEALLOCATE(vm, pointer) meoReallocate(vm, pointer, 0, 0)

// The Microsoft compiler does not support the "inline" modifier when compiling
// as plain C.
#if defined(_MSC_VER) && !defined(__cplusplus)
#define inline _inline
#endif

// This is used to clearly mark flexible-sized arrays that appear at the end of
// some dynamically-allocated structs, known as the "struct hack".
#if __STDC_VERSION__ >= 199901L
// In C99, a flexible array member is just "[]".
#define FLEXIBLE_ARRAY
#else
// Elsewhere, use a zero-sized array. It's technically undefined behavior,
// but works reliably in most known compilers.
#define FLEXIBLE_ARRAY 0
#endif

// Assertions are used to validate program invariants. They indicate things the
// program expects to be true about its internal state during execution. If an
// assertion fails, there is a bug in Meo.
//
// Assertions add significant overhead, so are only enabled in debug builds.
#ifdef DEBUG

#include <stdio.h>

#define ASSERT(condition, message)                                                                         \
    do {                                                                                                   \
        if (!(condition)) {                                                                                \
            fprintf(stderr, "[%s:%d] Assert failed in %s(): %s\n", __FILE__, __LINE__, __func__, message); \
            abort();                                                                                       \
        }                                                                                                  \
    } while (false)

// Indicates that we know execution should never reach this point in the
// program. In debug mode, we assert this fact because it's a bug to get here.
//
// In release mode, we use compiler-specific built in functions to tell the
// compiler the code can't be reached. This avoids "missing return" warnings
// in some cases and also lets it perform some optimizations by assuming the
// code is never reached.
#define UNREACHABLE()                                                                                       \
    do {                                                                                                    \
        fprintf(stderr, "[%s:%d] This code should not be reached in %s()\n", __FILE__, __LINE__, __func__); \
        abort();                                                                                            \
    } while (false)

#else

#define ASSERT(condition, message) \
    do {                           \
    } while (false)

// Tell the compiler that this part of the code will never be reached.
#if defined(_MSC_VER)
#define UNREACHABLE() __assume(0)
#elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#define UNREACHABLE() __builtin_unreachable()
#else
#define UNREACHABLE()
#endif

#endif

#include "meo.h"

// Reusable data structures and other utility functions.

// Forward declare this here to break a cycle between meo_utils.h and
// meo_value.h.
typedef struct sObjString ObjString;

// We need buffers of a few different types. To avoid lots of casting between
// void* and back, we'll use the preprocessor as a poor man's generics and let
// it generate a few type-specific ones.
#define DECLARE_BUFFER(name, type)                                                     \
    typedef struct {                                                                   \
        type* data;                                                                    \
        int count;                                                                     \
        int capacity;                                                                  \
    } name##Buffer;                                                                    \
    void meo##name##BufferInit(name##Buffer* buffer);                                  \
    void meo##name##BufferClear(MeoVM* vm, name##Buffer* buffer);                      \
    void meo##name##BufferFill(MeoVM* vm, name##Buffer* buffer, type data, int count); \
    void meo##name##BufferWrite(MeoVM* vm, name##Buffer* buffer, type data)

// This should be used once for each type instantiation, somewhere in a .c file.
#define DEFINE_BUFFER(name, type)                                                                                            \
    void meo##name##BufferInit(name##Buffer* buffer) {                                                                       \
        buffer->data = NULL;                                                                                                 \
        buffer->capacity = 0;                                                                                                \
        buffer->count = 0;                                                                                                   \
    }                                                                                                                        \
                                                                                                                             \
    void meo##name##BufferClear(MeoVM* vm, name##Buffer* buffer) {                                                           \
        meoReallocate(vm, buffer->data, 0, 0);                                                                               \
        meo##name##BufferInit(buffer);                                                                                       \
    }                                                                                                                        \
                                                                                                                             \
    void meo##name##BufferFill(MeoVM* vm, name##Buffer* buffer, type data, int count) {                                      \
        if (buffer->capacity < buffer->count + count) {                                                                      \
            int capacity = meoPowerOf2Ceil(buffer->count + count);                                                           \
            buffer->data = (type*)meoReallocate(vm, buffer->data, buffer->capacity * sizeof(type), capacity * sizeof(type)); \
            buffer->capacity = capacity;                                                                                     \
        }                                                                                                                    \
                                                                                                                             \
        for (int i = 0; i < count; i++) {                                                                                    \
            buffer->data[buffer->count++] = data;                                                                            \
        }                                                                                                                    \
    }                                                                                                                        \
                                                                                                                             \
    void meo##name##BufferWrite(MeoVM* vm, name##Buffer* buffer, type data) { meo##name##BufferFill(vm, buffer, data, 1); }

DECLARE_BUFFER(Byte, uint8_t);
DECLARE_BUFFER(Int, int);
DECLARE_BUFFER(String, ObjString*);

// TODO: Change this to use a map.
typedef StringBuffer SymbolTable;

// Initializes the symbol table.
void meoSymbolTableInit(SymbolTable* symbols);

// Frees all dynamically allocated memory used by the symbol table, but not the
// SymbolTable itself.
void meoSymbolTableClear(MeoVM* vm, SymbolTable* symbols);

// Adds name to the symbol table. Returns the index of it in the table.
int meoSymbolTableAdd(MeoVM* vm, SymbolTable* symbols, const char* name, size_t length);

// Adds name to the symbol table. Returns the index of it in the table. Will
// use an existing symbol if already present.
int meoSymbolTableEnsure(MeoVM* vm, SymbolTable* symbols, const char* name, size_t length);

// Looks up name in the symbol table. Returns its index if found or -1 if not.
int meoSymbolTableFind(const SymbolTable* symbols, const char* name, size_t length);

void meoBlackenSymbolTable(MeoVM* vm, SymbolTable* symbolTable);

// Returns the number of bytes needed to encode [value] in UTF-8.
//
// Returns 0 if [value] is too large to encode.
int meoUtf8EncodeNumBytes(int value);

// Encodes value as a series of bytes in [bytes], which is assumed to be large
// enough to hold the encoded result.
//
// Returns the number of written bytes.
int meoUtf8Encode(int value, uint8_t* bytes);

// Decodes the UTF-8 sequence starting at [bytes] (which has max [length]),
// returning the code point.
//
// Returns -1 if the bytes are not a valid UTF-8 sequence.
int meoUtf8Decode(const uint8_t* bytes, uint32_t length);

// Returns the number of bytes in the UTF-8 sequence starting with [byte].
//
// If the character at that index is not the beginning of a UTF-8 sequence,
// returns 0.
int meoUtf8DecodeNumBytes(uint8_t byte);

// Returns the smallest power of two that is equal to or greater than [n].
int meoPowerOf2Ceil(int n);

// Validates that [value] is within `[0, count)`. Also allows
// negative indices which map backwards from the end. Returns the valid positive
// index value. If invalid, returns `UINT32_MAX`.
uint32_t meoValidateIndex(uint32_t count, int64_t value);

#pragma region math

// A union to let us reinterpret a double as raw bits and back.
typedef union {
    uint64_t bits64;
    uint32_t bits32[2];
    double num;
} MeoDoubleBits;

#define MEO_DOUBLE_QNAN_POS_MIN_BITS (UINT64_C(0x7FF8000000000000))
#define MEO_DOUBLE_QNAN_POS_MAX_BITS (UINT64_C(0x7FFFFFFFFFFFFFFF))

#define MEO_DOUBLE_NAN (meoDoubleFromBits(MEO_DOUBLE_QNAN_POS_MIN_BITS))

static inline double meoDoubleFromBits(uint64_t bits) {
    MeoDoubleBits data;
    data.bits64 = bits;
    return data.num;
}

static inline uint64_t meoDoubleToBits(double num) {
    MeoDoubleBits data;
    data.num = num;
    return data.bits64;
}

#pragma endregion math

#endif
