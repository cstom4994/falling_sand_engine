#include "meo_opt.h"

#if MEO_OPT_META

#include <string.h>

static const char* meta_source = R"meo(
class Meta {
  static getModuleVariables(module) {
    if (!(module is String)) Fiber.abort("Module name must be a string.")
    var result = getModuleVariables_(module)
    if (result != null) return result

    Fiber.abort("Could not find a module named '%(module)'.")
  }

  static eval(source) {
    if (!(source is String)) Fiber.abort("Source code must be a string.")

    var closure = compile_(source, false, false)
    // TODO: Include compile errors.
    if (closure == null) Fiber.abort("Could not compile source code.")

    closure.call()
  }

  static compileExpression(source) {
    if (!(source is String)) Fiber.abort("Source code must be a string.")
    return compile_(source, true, true)
  }

  static compile(source) {
    if (!(source is String)) Fiber.abort("Source code must be a string.")
    return compile_(source, false, true)
  }

  foreign static compile_(source, isExpression, printErrors)
  foreign static getModuleVariables_(module)
}

)meo";

#include "meo_vm.h"

void metaCompile(MeoVM* vm) {
    const char* source = meoGetSlotString(vm, 1);
    bool isExpression = meoGetSlotBool(vm, 2);
    bool printErrors = meoGetSlotBool(vm, 3);

    // TODO: Allow passing in module?
    // Look up the module surrounding the callsite. This is brittle. The -2 walks
    // up the callstack assuming that the meta module has one level of
    // indirection before hitting the user's code. Any change to meta may require
    // this constant to be tweaked.
    ObjFiber* currentFiber = vm->fiber;
    ObjFn* fn = currentFiber->frames[currentFiber->numFrames - 2].closure->fn;
    ObjString* module = fn->module->name;

    ObjClosure* closure = meoCompileSource(vm, module->value, source, isExpression, printErrors);

    // Return the result. We can't use the public API for this since we have a
    // bare ObjClosure*.
    if (closure == NULL) {
        vm->apiStack[0] = NULL_VAL;
    } else {
        vm->apiStack[0] = OBJ_VAL(closure);
    }
}

void metaGetModuleVariables(MeoVM* vm) {
    meoEnsureSlots(vm, 3);

    Value moduleValue = meoMapGet(vm->modules, vm->apiStack[1]);
    if (IS_UNDEFINED(moduleValue)) {
        vm->apiStack[0] = NULL_VAL;
        return;
    }

    ObjModule* module = AS_MODULE(moduleValue);
    ObjList* names = meoNewList(vm, module->variableNames.count);
    vm->apiStack[0] = OBJ_VAL(names);

    // Initialize the elements to null in case a collection happens when we
    // allocate the strings below.
    for (int i = 0; i < names->elements.count; i++) {
        names->elements.data[i] = NULL_VAL;
    }

    for (int i = 0; i < names->elements.count; i++) {
        names->elements.data[i] = OBJ_VAL(module->variableNames.data[i]);
    }
}

const char* meoMetaSource() { return meta_source; }

MeoForeignMethodFn meoMetaBindForeignMethod(MeoVM* vm, const char* className, bool isStatic, const char* signature) {
    // There is only one foreign method in the meta module.
    ASSERT(strcmp(className, "Meta") == 0, "Should be in Meta class.");
    ASSERT(isStatic, "Should be static.");

    if (strcmp(signature, "compile_(_,_,_)") == 0) {
        return metaCompile;
    }

    if (strcmp(signature, "getModuleVariables_(_)") == 0) {
        return metaGetModuleVariables;
    }

    ASSERT(false, "Unknown method.");
    return NULL;
}

#endif

#if MEO_OPT_RANDOM

#include <string.h>
#include <time.h>

#include "meo.h"

static const char* random_source = R"meo(
foreign class Random {
  construct new() {
    seed_()
  }

  construct new(seed) {
    if (seed is Num) {
      seed_(seed)
    } else if (seed is Sequence) {
      if (seed.isEmpty) Fiber.abort("Sequence cannot be empty.")

      // TODO: Empty sequence.
      var seeds = []
      for (element in seed) {
        if (!(element is Num)) Fiber.abort("Sequence elements must all be numbers.")

        seeds.add(element)
        if (seeds.count == 16) break
      }

      // Cycle the values to fill in any missing slots.
      var i = 0
      while (seeds.count < 16) {
        seeds.add(seeds[i])
        i = i + 1
      }

      seed_(
          seeds[0], seeds[1], seeds[2], seeds[3],
          seeds[4], seeds[5], seeds[6], seeds[7],
          seeds[8], seeds[9], seeds[10], seeds[11],
          seeds[12], seeds[13], seeds[14], seeds[15])
    } else {
      Fiber.abort("Seed must be a number or a sequence of numbers.")
    }
  }

  foreign seed_()
  foreign seed_(seed)
  foreign seed_(n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12, n13, n14, n15, n16)

  foreign float()
  float(end) { float() * end }
  float(start, end) { float() * (end - start) + start }

  foreign int()
  int(end) { (float() * end).floor }
  int(start, end) { (float() * (end - start)).floor + start }

  sample(list) {
    if (list.count == 0) Fiber.abort("Not enough elements to sample.")
    return list[int(list.count)]
  }
  sample(list, count) {
    if (count > list.count) Fiber.abort("Not enough elements to sample.")

    var result = []

    // The algorithm described in "Programming pearls: a sample of brilliance".
    // Use a hash map for sample sizes less than 1/4 of the population size and
    // an array of booleans for larger samples. This simple heuristic improves
    // performance for large sample sizes as well as reduces memory usage.
    if (count * 4 < list.count) {
      var picked = {}
      for (i in list.count - count...list.count) {
        var index = int(i + 1)
        if (picked.containsKey(index)) index = i
        picked[index] = true
        result.add(list[index])
      }
    } else {
      var picked = List.filled(list.count, false)
      for (i in list.count - count...list.count) {
        var index = int(i + 1)
        if (picked[index]) index = i
        picked[index] = true
        result.add(list[index])
      }
    }

    return result
  }

  shuffle(list) {
    if (list.isEmpty) return

    // Fisher-Yates shuffle.
    for (i in 0...list.count - 1) {
      var from = int(i, list.count)
      var temp = list[from]
      list[from] = list[i]
      list[i] = temp
    }
  }
}

)meo";

#include "meo_vm.h"

// Implements the well equidistributed long-period linear PRNG (WELL512a).
//
// https://en.wikipedia.org/wiki/Well_equidistributed_long-period_linear
typedef struct {
    uint32_t state[16];
    uint32_t index;
} Well512;

// Code from: http://www.lomont.org/Math/Papers/2008/Lomont_PRNG_2008.pdf
static uint32_t advanceState(Well512* well) {
    uint32_t a, b, c, d;
    a = well->state[well->index];
    c = well->state[(well->index + 13) & 15];
    b = a ^ c ^ (a << 16) ^ (c << 15);
    c = well->state[(well->index + 9) & 15];
    c ^= (c >> 11);
    a = well->state[well->index] = b ^ c;
    d = a ^ ((a << 5) & 0xda442d24U);

    well->index = (well->index + 15) & 15;
    a = well->state[well->index];
    well->state[well->index] = a ^ b ^ d ^ (a << 2) ^ (b << 18) ^ (c << 28);
    return well->state[well->index];
}

static void randomAllocate(MeoVM* vm) {
    Well512* well = (Well512*)meoSetSlotNewForeign(vm, 0, 0, sizeof(Well512));
    well->index = 0;
}

static void randomSeed0(MeoVM* vm) {
    Well512* well = (Well512*)meoGetSlotForeign(vm, 0);

    srand((uint32_t)time(NULL));
    for (int i = 0; i < 16; i++) {
        well->state[i] = rand();
    }
}

static void randomSeed1(MeoVM* vm) {
    Well512* well = (Well512*)meoGetSlotForeign(vm, 0);

    srand((uint32_t)meoGetSlotDouble(vm, 1));
    for (int i = 0; i < 16; i++) {
        well->state[i] = rand();
    }
}

static void randomSeed16(MeoVM* vm) {
    Well512* well = (Well512*)meoGetSlotForeign(vm, 0);

    for (int i = 0; i < 16; i++) {
        well->state[i] = (uint32_t)meoGetSlotDouble(vm, i + 1);
    }
}

static void randomFloat(MeoVM* vm) {
    Well512* well = (Well512*)meoGetSlotForeign(vm, 0);

    // A double has 53 bits of precision in its mantissa, and we'd like to take
    // full advantage of that, so we need 53 bits of random source data.

    // First, start with 32 random bits, shifted to the left 21 bits.
    double result = (double)advanceState(well) * (1 << 21);

    // Then add another 21 random bits.
    result += (double)(advanceState(well) & ((1 << 21) - 1));

    // Now we have a number from 0 - (2^53). Divide be the range to get a double
    // from 0 to 1.0 (half-inclusive).
    result /= 9007199254740992.0;

    meoSetSlotDouble(vm, 0, result);
}

static void randomInt0(MeoVM* vm) {
    Well512* well = (Well512*)meoGetSlotForeign(vm, 0);

    meoSetSlotDouble(vm, 0, (double)advanceState(well));
}

const char* meoRandomSource() { return random_source; }

MeoForeignClassMethods meoRandomBindForeignClass(MeoVM* vm, const char* module, const char* className) {
    ASSERT(strcmp(className, "Random") == 0, "Should be in Random class.");
    MeoForeignClassMethods methods;
    methods.allocate = randomAllocate;
    methods.finalize = NULL;
    return methods;
}

MeoForeignMethodFn meoRandomBindForeignMethod(MeoVM* vm, const char* className, bool isStatic, const char* signature) {
    ASSERT(strcmp(className, "Random") == 0, "Should be in Random class.");

    if (strcmp(signature, "<allocate>") == 0) return randomAllocate;
    if (strcmp(signature, "seed_()") == 0) return randomSeed0;
    if (strcmp(signature, "seed_(_)") == 0) return randomSeed1;

    if (strcmp(signature, "seed_(_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_)") == 0) {
        return randomSeed16;
    }

    if (strcmp(signature, "float()") == 0) return randomFloat;
    if (strcmp(signature, "int()") == 0) return randomInt0;

    ASSERT(false, "Unknown method.");
    return NULL;
}

#endif
