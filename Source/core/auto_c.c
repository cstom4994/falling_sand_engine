// Copyright(c) 2022, KaoruXun All rights reserved.
// High level modern c programming
// Link to https://libcello.org

#include "auto_c.h"

struct Header *header(var self)
{
    return (struct Header *) ((char *) self - sizeof(struct Header));
}

var header_init(var head, var type, int alloc) {

    struct Header *self = head;

    self->type = type;

#if METADOT_C_ALLOC_CHECK == 1
    self->alloc = (var) (intptr_t) alloc;
#endif

#if METADOT_C_MAGIC_CHECK == 1
    self->magic = (var) METADOT_C_MAGIC_NUM;
#endif

    return ((char *) self) + sizeof(struct Header);
}

static const char *Alloc_Name(void) { return "Alloc"; }

static const char *Alloc_Brief(void) { return "Memory Allocation"; }

static const char *Alloc_Description(void) {
    return "The `Alloc` class can be used to override how memory is allocated for a "
           "given data type. By default memory is allocated using `calloc` along with "
           "the `Size` class to determine the amount of memory to allocate."
           "\n\n"
           "A custom allocator should be careful to also initialise the header for "
           "the allocated memory using the function `header_init`. MetaDotC objects "
           "without a header wont be recognised as such as so will throw errors when "
           "used with MetaDotC functions."
           "\n\n"
           "Allocated memory is automatically registered with the garbage collector "
           "unless the functions `alloc_raw` and `dealloc_raw` are used.";
}

static const char *Alloc_Definition(void) {
    return "struct Alloc {\n"
           "  var (*alloc)(void);\n"
           "  void (*dealloc)(var);\n"
           "};";
}

static struct Example *Alloc_Examples(void) {

    static struct Example examples[] = {
            {
                    "Usage",
                    "/* Allocation deallocated by Garbage Collector */\n"
                    "var x = alloc(Int);\n"
                    "construct(x, $I(10));\n",
            },
            {
                    "Avoid Garbage Collection",
                    "/* Allocation must be manually deallocated */\n"
                    "var x = alloc_raw(Int);\n"
                    "construct(x, $I(10));\n"
                    "destruct(x);\n"
                    "dealloc_raw(x);\n",
            },
            {NULL, NULL}};

    return examples;
}

static struct Method *Alloc_Methods(void) {

    static struct Method methods[] = {
            {"$",
             "#define $(T, ...)\n"
             "#define $I(X)\n"
             "#define $F(X)\n"
             "#define $S(X)\n"
             "#define $R(X)\n"
             "#define $B(X)",
             "Allocate memory for the given type `T` on the stack and copy in the "
             "given arguments `...` as struct members. Shorthand constructors exist "
             "for native types:\n\n* `$I -> Int` `$F -> Float` `$S -> String`\n*"
             " `$R -> Ref` `$B -> Box`\n\n"},
            {"alloc",
             "#define alloc_stack(T)\n"
             "var alloc(var type);\n"
             "var alloc_raw(var type);\n"
             "var alloc_root(var type);",
             "Allocate memory for a given `type`. To avoid the Garbage Collector "
             "completely use `alloc_raw`, to register the allocation as a root use "
             "`alloc_root`. In the case of raw or root allocations the corresponding "
             "`dealloc` function should be used when done. Memory allocated with "
             "`alloc_stack` is not managed by the Garbage Collector."},
            {"dealloc",
             "void dealloc(var self);\n"
             "void dealloc_raw(var self);\n"
             "void dealloc_root(var self);",
             "Deallocate memory for object `self` manually. If registered with the "
             "Garbage Collector then entry will be removed. If the `raw` variation is "
             "used memory will be deallocated without going via the Garbage Collector."},
            {NULL, NULL, NULL}};

    return methods;
}

var Alloc = MetaDotC(Alloc, Instance(Doc, Alloc_Name, Alloc_Brief, Alloc_Description,
                                     Alloc_Definition, Alloc_Examples, Alloc_Methods));

enum {
    ALLOC_STANDARD,
    ALLOC_RAW,
    ALLOC_ROOT
};

static var alloc_by(var type, int method) {

    struct Alloc *a = type_instance(type, Alloc);
    var self;
    if (a and a->alloc) {
        self = a->alloc();
    } else {
        struct Header *head = calloc(1, sizeof(struct Header) + size(type));

#if METADOT_C_MEMORY_CHECK == 1
        if (head is NULL) {
            throw(OutOfMemoryError, "Cannot create new '%s', out of memory!", type);
        }
#endif

        self = header_init(head, type, AllocHeap);
    }

    switch (method) {
        case ALLOC_STANDARD:
#ifndef METADOT_C_NGC
            set(current(AutoC_GC), self, $I(0));
#endif
            break;
        case ALLOC_RAW:
            break;
        case ALLOC_ROOT:
#ifndef METADOT_C_NGC
            set(current(AutoC_GC), self, $I(1));
#endif
            break;
    }

    return self;
}

var alloc(var type) { return alloc_by(type, ALLOC_STANDARD); }
var alloc_raw(var type) { return alloc_by(type, ALLOC_RAW); }
var alloc_root(var type) { return alloc_by(type, ALLOC_ROOT); }

void dealloc(var self) {

    struct Alloc *a = instance(self, Alloc);
    if (a and a->dealloc) {
        a->dealloc(self);
        return;
    }

#if METADOT_C_ALLOC_CHECK == 1
    if (self is NULL) { throw(ResourceError, "Attempt to deallocate NULL!", ""); }

    if (header(self)->alloc is(var) AllocStatic) {
        throw(ResourceError,
              "Attempt to deallocate %$ "
              "which was allocated statically!",
              self);
    }

    if (header(self)->alloc is(var) AllocStack) {
        throw(ResourceError,
              "Attempt to deallocate %$ "
              "which was allocated on the stack!",
              self);
    }

    if (header(self)->alloc is(var) AllocData) {
        throw(ResourceError,
              "Attempt to deallocate %$ "
              "which was allocated inside a data structure!",
              self);
    }
#endif

#if METADOT_C_ALLOC_CHECK == 1
    size_t s = size(type_of(self));
    for (size_t i = 0; i < (sizeof(struct Header) + s) / sizeof(var); i++) {
        ((var *) header(self))[i] = (var) 0xDeadCe110;
    }
#endif

    free(((char *) self) - sizeof(struct Header));
}

void dealloc_raw(var self) { dealloc(self); }
void dealloc_root(var self) { dealloc(self); }

static const char *New_Name(void) { return "New"; }

static const char *New_Brief(void) { return "Construction and Destruction"; }

static const char *New_Description(void) {
    return "The `New` class allows the user to define constructors and destructors "
           "for a type, accessible via `new` and `del`. Objects allocated with `new` "
           "are allocated on the heap and also registered with the Garbage Collector "
           "this means technically it isn't required to call `del` on them as they "
           "will be cleaned up at a later date."
           "\n\n"
           "The `new_root` function can be called to register a variable with the "
           "Garbage Collector but to indicate that it will be manually destructed "
           "with `del_root` by the user. This should be used for variables that wont "
           "be reachable by the Garbage Collector such as those in the data segment "
           "or only accessible via vanilla C structures."
           "\n\n"
           "The `new_raw` and `del_raw` functions can be called to construct and "
           "destruct objects without going via the Garbage Collector."
           "\n\n"
           "It is also possible to simply call the `construct` and `destruct` "
           "functions if you wish to construct an already allocated object."
           "\n\n"
           "Constructors should assume that memory is zero'd for an object but "
           "nothing else.";
}

static const char *New_Definition(void) {
    return "struct New {\n"
           "  void (*construct_with)(var, var);\n"
           "  void (*destruct)(var);\n"
           "};\n";
}

static struct Example *New_Examples(void) {

    static struct Example examples[] = {{"Usage", "var x = new(Int, $I(1));\n"
                                                  "show(x); /* 1 */\n"
                                                  "show(type_of(x)); /* Int */\n"
                                                  "\n"
                                                  "var y = alloc(Float);\n"
                                                  "construct(y, $F(1.0));\n"
                                                  "show(y); /* 1.0 */\n"
                                                  "destruct(y);\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *New_Methods(void) {

    static struct Method methods[] = {
            {"new",
             "#define new(T, ...)\n"
             "#define new_raw(T, ...)\n"
             "#define new_root(T, ...)\n"
             "var new_with(var type, var args);\n"
             "var new_raw_with(var type, var args);\n"
             "var new_root_with(var type, var args);",
             "Construct a new object of a given `type`. Use `new_raw` to avoid the "
             "Garbage Collector completely, and `new_root` to register the allocation "
             "as a Garbage Collection root. In the case of raw and root allocations "
             "they must be destructed with the corresponding deletion functions."},
            {"del",
             "void del(var self);\n"
             "void del_raw(var self);\n"
             "void del_root(var self);",
             "Destruct the object `self` manually. If registered with the "
             "Garbage Collector then entry will be removed. If `del_raw` is used then"
             "the destruction will be done without going via the Garbage Collector."},
            {"construct",
             "#define construct(self, ...)\n"
             "var construct_with(var self, var args);",
             "Call the constructor on object `self` which has already been allocated."},
            {"destruct", "var destruct(var self);",
             "Call the destructor on object `self` without deallocating the memory "
             "for it."},
            {NULL, NULL, NULL}};

    return methods;
}

var New = MetaDotC(New, Instance(Doc, New_Name, New_Brief, New_Description, New_Definition,
                                 New_Examples, New_Methods));

var construct_with(var self, var args) {
    struct New *n = instance(self, New);
    if (n and n->construct_with) {
        n->construct_with(self, args);
    } else if (len(args) == 1) {
        assign(self, get(args, $I(0)));
    }
    return self;
}

var destruct(var self) {
    struct New *n = instance(self, New);
    if (n and n->destruct) { n->destruct(self); }
    return self;
}

var new_with(var type, var args) { return construct_with(alloc(type), args); }

var new_raw_with(var type, var args) { return construct_with(alloc_raw(type), args); }

var new_root_with(var type, var args) { return construct_with(alloc_root(type), args); }

static void del_by(var self, int method) {

    switch (method) {
        case ALLOC_STANDARD:
        case ALLOC_ROOT:
#ifndef METADOT_C_NGC
            rem(current(AutoC_GC), self);
            return;
#endif
            break;
        case ALLOC_RAW:
            break;
    }

    dealloc(destruct(self));
}

void del(var self) { del_by(self, ALLOC_STANDARD); }
void del_raw(var self) { del_by(self, ALLOC_RAW); }
void del_root(var self) { del_by(self, ALLOC_ROOT); }

static const char *Copy_Name(void) { return "Copy"; }

static const char *Copy_Brief(void) { return "Copyable"; }

static const char *Copy_Description(void) {
    return "The `Copy` class can be used to override the behaviour of an object when "
           "a copy is made of it. By default the `Copy` class allocates a new empty "
           "object of the same type and uses the `Assign` class to set the "
           "contents. The copy is then registered with the Garbage Collector as if it "
           "had been constructed with `new`. This means when using manual memory "
           "management a copy must be deleted manually."
           "\n\n"
           "If the `copy` class is overridden then the implementer may manually have "
           "to register the object with the Garbage Collector if they wish for it to "
           "be tracked."
           "\n\n"
           "By convention `copy` follows the semantics of `Assign`, which typically "
           "means a _deep copy_ should be made, and that an object will create a "
           "copy of all of the sub-objects it references or contains - although this "
           "could vary depending on the type's overridden behaviours.";
}

static const char *Copy_Definition(void) {
    return "struct Copy {\n"
           "  var (*copy)(var);\n"
           "};\n";
}

static struct Example *Copy_Examples(void) {

    static struct Example examples[] = {{"Usage", "var x = new(String, $S(\"Hello\"));\n"
                                                  "var y = copy(x);\n"
                                                  "show(x); /* Hello */\n"
                                                  "show(y); /* Hello */\n"
                                                  "show($I(eq(x, y))); /* 1 */\n"
                                                  "show($I(x is y)); /* 0 */\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Copy_Methods(void) {

    static struct Method methods[] = {
            {"copy", "var copy(var self);", "Make a copy of the object `self`."},
            {NULL, NULL, NULL}};

    return methods;
}

var Copy = MetaDotC(Copy, Instance(Doc, Copy_Name, Copy_Brief, Copy_Description, Copy_Definition,
                                   Copy_Examples, Copy_Methods));

var copy(var self) {

    struct Copy *c = instance(self, Copy);
    if (c and c->copy) { return c->copy(self); }

    return assign(alloc(type_of(self)), self);
}

static const char *Array_Name(void) { return "Array"; }

static const char *Array_Brief(void) { return "Sequential Container"; }

static const char *Array_Description(void) {
    return ""
           "The `Array` type is data structure containing a sequence of a single type "
           "of object. It can dynamically grow and shrink in size depending on how "
           "many elements it contains. It allocates storage for the type specified. "
           "It also deallocates and destroys the objects inside upon destruction."
           "\n\n"
           "Elements are copied into an Array using `assign` and will initially have "
           "zero'd memory."
           "\n\n"
           "Elements are ordered linearly. Elements are accessed by their position in "
           "this sequence directly. Addition and removal of elements at the end of "
           "the sequence is fast, with memory movement required for elements in the "
           "middle of the sequence."
           "\n\n"
           "This is largely equivalent to the C++ construct "
           "[std::vector](http://www.cplusplus.com/reference/vector/vector/)";
}

static struct Example *Array_Examples(void) {

    static struct Example examples[] = {
            {
                    "Construction & Deletion",
                    "var x = new(Array, Int);\n"
                    "push(x, $I(32));\n"
                    "push(x, $I(6));\n"
                    "\n"
                    "/* <'Array' At 0x0000000000414603 [32, 6]> */\n"
                    "show(x);\n",
            },
            {
                    "Element Access",
                    "var x = new(Array, Float, $F(0.01), $F(5.12));\n"
                    "\n"
                    "show(get(x, $I(0))); /* 0.01 */\n"
                    "show(get(x, $I(1))); /* 5.12 */\n"
                    "\n"
                    "set(x, $I(0), $F(500.1));\n"
                    "show(get(x, $I(0))); /* 500.1 */\n",
            },
            {
                    "Membership",
                    "var x = new(Array, Int, $I(1), $I(2), $I(3), $I(4));\n"
                    "\n"
                    "show($I(mem(x, $I(1)))); /* 1 */\n"
                    "show($I(len(x)));        /* 4 */\n"
                    "\n"
                    "rem(x, $I(3));\n"
                    "\n"
                    "show($I(mem(x, $I(3)))); /* 0 */\n"
                    "show($I(len(x)));        /* 3 */\n"
                    "show($I(empty(x)));      /* 0 */\n"
                    "\n"
                    "resize(x, 0);\n"
                    "\n"
                    "show($I(empty(x)));      /* 1 */\n",
            },
            {
                    "Iteration",
                    "var greetings = new(Array, String, \n"
                    "  $S(\"Hello\"), $S(\"Bonjour\"), $S(\"Hej\"));\n"
                    "\n"
                    "foreach(greet in greetings) {\n"
                    "  show(greet);\n"
                    "}\n",
            },
            {NULL, NULL}};

    return examples;
}

struct Array
{
    var type;
    var data;
    size_t tsize;
    size_t nitems;
    size_t nslots;
};

static size_t Array_Step(struct Array *a) { return a->tsize + sizeof(struct Header); }

static var Array_Item(struct Array *a, size_t i) {
    return (char *) a->data + Array_Step(a) * i + sizeof(struct Header);
}

static void Array_Alloc(struct Array *a, size_t i) {
    memset((char *) a->data + Array_Step(a) * i, 0, Array_Step(a));
    struct Header *head = (struct Header *) ((char *) a->data + Array_Step(a) * i);
    header_init(head, a->type, AllocData);
}

static size_t Array_Size_Round(size_t s) {
    return ((s + sizeof(var) - 1) / sizeof(var)) * sizeof(var);
}

static void Array_New(var self, var args) {

    struct Array *a = self;
    a->type = cast(get(args, $I(0)), Type);
    a->tsize = Array_Size_Round(size(a->type));
    a->nitems = len(args) - 1;
    a->nslots = a->nitems;

    if (a->nslots is 0) {
        a->data = NULL;
        return;
    }

    a->data = malloc(a->nslots * Array_Step(a));

#if METADOT_C_MEMORY_CHECK == 1
    if (a->data is NULL) { throw(OutOfMemoryError, "Cannot allocate Array, out of memory!", ""); }
#endif

    for (size_t i = 0; i < a->nitems; i++) {
        Array_Alloc(a, i);
        assign(Array_Item(a, i), get(args, $I(i + 1)));
    }
}

static void Array_Del(var self) {

    struct Array *a = self;

    for (size_t i = 0; i < a->nitems; i++) { destruct(Array_Item(a, i)); }

    free(a->data);
}

static void Array_Clear(var self) {
    struct Array *a = self;

    for (size_t i = 0; i < a->nitems; i++) { destruct(Array_Item(a, i)); }

    free(a->data);
    a->data = NULL;
    a->nitems = 0;
    a->nslots = 0;
}

static void Array_Push(var self, var obj);

static void Array_Assign(var self, var obj) {
    struct Array *a = self;

    Array_Clear(self);

    a->type = implements_method(obj, Iter, iter_type) ? iter_type(obj) : Ref;
    a->tsize = Array_Size_Round(size(a->type));
    a->nitems = 0;
    a->nslots = 0;

    if (implements_method(obj, Len, len) and implements_method(obj, Get, get)) {

        a->nitems = len(obj);
        a->nslots = a->nitems;

        if (a->nslots is 0) {
            a->data = NULL;
            return;
        }

        a->data = malloc(a->nslots * Array_Step(a));

#if METADOT_C_MEMORY_CHECK == 1
        if (a->data is NULL) {
            throw(OutOfMemoryError, "Cannot allocate Array, out of memory!", "");
        }
#endif

        for (size_t i = 0; i < a->nitems; i++) {
            Array_Alloc(a, i);
            assign(Array_Item(a, i), get(obj, $I(i)));
        }

    } else {

        foreach (item in obj) { Array_Push(self, item); }
    }
}

static void Array_Reserve_More(struct Array *a) {

    if (a->nitems > a->nslots) {
        a->nslots = a->nitems + a->nitems / 2;
        a->data = realloc(a->data, Array_Step(a) * a->nslots);
#if METADOT_C_MEMORY_CHECK == 1
        if (a->data is NULL) { throw(OutOfMemoryError, "Cannot grow Array, out of memory!", ""); }
#endif
    }
}

static void Array_Concat(var self, var obj) {

    struct Array *a = self;

    size_t i = 0;
    size_t olen = len(obj);

    a->nitems += olen;
    Array_Reserve_More(a);

    foreach (item in obj) {
        Array_Alloc(a, a->nitems - olen + i);
        assign(Array_Item(a, a->nitems - olen + i), item);
        i++;
    }
}

static var Array_Iter_Init(var self);
static var Array_Iter_Next(var self, var curr);

static int Array_Cmp(var self, var obj) {

    var item0 = Array_Iter_Init(self);
    var item1 = iter_init(obj);

    while (true) {
        if (item0 is Terminal and item1 is Terminal) { return 0; }
        if (item0 is Terminal) { return -1; }
        if (item1 is Terminal) { return 1; }
        int c = cmp(item0, item1);
        if (c < 0) { return -1; }
        if (c > 0) { return 1; }
        item0 = Array_Iter_Next(self, item0);
        item1 = iter_next(obj, item1);
    }

    return 0;
}

static uint64_t Array_Hash(var self) {
    struct Array *a = self;
    uint64_t h = 0;

    for (size_t i = 0; i < a->nitems; i++) { h ^= hash(Array_Item(a, i)); }

    return h;
}

static size_t Array_Len(var self) {
    struct Array *a = self;
    return a->nitems;
}

static bool Array_Mem(var self, var obj) {
    struct Array *a = self;
    for (size_t i = 0; i < a->nitems; i++) {
        if (eq(Array_Item(a, i), obj)) { return true; }
    }
    return false;
}

static void Array_Reserve_Less(struct Array *a) {
    if (a->nslots > a->nitems + a->nitems / 2) {
        a->nslots = a->nitems;
        a->data = realloc(a->data, Array_Step(a) * a->nslots);
    }
}

static void Array_Pop_At(var self, var key) {

    struct Array *a = self;
    int64_t i = c_int(key);
    i = i < 0 ? a->nitems + i : i;

#if METADOT_C_BOUND_CHECK == 1
    if (i < 0 or i >= (int64_t) a->nitems) {
        throw(IndexOutOfBoundsError, "Index '%i' out of bounds for Array of size %i.", key,
              $I(a->nitems));
        return;
    }
#endif

    destruct(Array_Item(a, i));

    memmove((char *) a->data + Array_Step(a) * (i + 0), (char *) a->data + Array_Step(a) * (i + 1),
            Array_Step(a) * ((a->nitems - 1) - i));

    a->nitems--;
    Array_Reserve_Less(a);
}

static void Array_Rem(var self, var obj) {
    struct Array *a = self;
    for (size_t i = 0; i < a->nitems; i++) {
        if (eq(Array_Item(a, i), obj)) {
            Array_Pop_At(a, $I(i));
            return;
        }
    }
    throw(ValueError, "Object %$ not in Array!", obj);
}

static void Array_Push(var self, var obj) {
    struct Array *a = self;
    a->nitems++;
    Array_Reserve_More(a);
    Array_Alloc(a, a->nitems - 1);
    assign(Array_Item(a, a->nitems - 1), obj);
}

static void Array_Push_At(var self, var obj, var key) {
    struct Array *a = self;
    a->nitems++;
    Array_Reserve_More(a);

    int64_t i = c_int(key);
    i = i < 0 ? a->nitems + i : i;

#if METADOT_C_BOUND_CHECK == 1
    if (i < 0 or i >= (int64_t) a->nitems) {
        throw(IndexOutOfBoundsError, "Index '%i' out of bounds for Array of size %i.", key,
              $I(a->nitems));
        return;
    }
#endif

    memmove((char *) a->data + Array_Step(a) * (i + 1), (char *) a->data + Array_Step(a) * (i + 0),
            Array_Step(a) * ((a->nitems - 1) - i));

    Array_Alloc(self, i);
    assign(Array_Item(a, i), obj);
}

static void Array_Pop(var self) {

    struct Array *a = self;

#if METADOT_C_BOUND_CHECK == 1
    if (a->nitems is 0) {
        throw(IndexOutOfBoundsError, "Cannot pop. Array is empty!", "");
        return;
    }
#endif

    destruct(Array_Item(a, a->nitems - 1));

    a->nitems--;
    Array_Reserve_Less(a);
}

static var Array_Get(var self, var key) {

    struct Array *a = self;
    int64_t i = c_int(key);
    i = i < 0 ? a->nitems + i : i;

#if METADOT_C_BOUND_CHECK == 1
    if (i < 0 or i >= (int64_t) a->nitems) {
        return throw(IndexOutOfBoundsError, "Index '%i' out of bounds for Array of size %i.", key,
                     $I(a->nitems));
    }
#endif

    return Array_Item(a, i);
}

static void Array_Set(var self, var key, var val) {

    struct Array *a = self;
    int64_t i = c_int(key);
    i = i < 0 ? a->nitems + i : i;

#if METADOT_C_BOUND_CHECK == 1
    if (i < 0 or i >= (int64_t) a->nitems) {
        throw(IndexOutOfBoundsError, "Index '%i' out of bounds for Array of size %i.", key,
              $I(a->nitems));
        return;
    }
#endif

    assign(Array_Item(a, i), val);
}

static var Array_Iter_Init(var self) {
    struct Array *a = self;
    if (a->nitems is 0) { return Terminal; }
    return Array_Item(a, 0);
}

static var Array_Iter_Next(var self, var curr) {
    struct Array *a = self;
    if (curr >= Array_Item(a, a->nitems - 1)) {
        return Terminal;
    } else {
        return (char *) curr + Array_Step(a);
    }
}

static var Array_Iter_Last(var self) {
    struct Array *a = self;
    if (a->nitems is 0) { return Terminal; }
    return Array_Item(a, a->nitems - 1);
}

static var Array_Iter_Prev(var self, var curr) {
    struct Array *a = self;
    if (curr < Array_Item(a, 0)) {
        return Terminal;
    } else {
        return (char *) curr - Array_Step(a);
    }
}

static var Array_Iter_Type(var self) {
    struct Array *a = self;
    return a->type;
}

static size_t Array_Sort_Partition(struct Array *a, int64_t l, int64_t r, bool (*f)(var, var)) {

    int64_t p = l + (r - l) / 2;
    swap(Array_Item(a, p), Array_Item(a, r));

    int64_t s = l;
    for (int64_t i = l; i < r; i++) {
        if (f(Array_Get(a, $I(i)), Array_Item(a, r))) {
            swap(Array_Item(a, i), Array_Item(a, s));
            s++;
        }
    }

    swap(Array_Item(a, s), Array_Item(a, r));

    return s;
}

static void Array_Sort_Part(struct Array *a, int64_t l, int64_t r, bool (*f)(var, var)) {
    if (l < r) {
        int64_t s = Array_Sort_Partition(a, l, r, f);
        Array_Sort_Part(a, l, s - 1, f);
        Array_Sort_Part(a, s + 1, r, f);
    }
}

static void Array_Sort_By(var self, bool (*f)(var, var)) {
    Array_Sort_Part(self, 0, Array_Len(self) - 1, f);
}

static int Array_Show(var self, var output, int pos) {
    struct Array *a = self;
    pos = print_to(output, pos, "<'Array' At 0x%p [", self);
    for (size_t i = 0; i < a->nitems; i++) {
        pos = print_to(output, pos, "%$", Array_Item(a, i));
        if (i < a->nitems - 1) { pos = print_to(output, pos, ", ", ""); }
    }
    return print_to(output, pos, "]>", "");
}

static void Array_Resize(var self, size_t n) {
    struct Array *a = self;

    if (n is 0) {
        Array_Clear(self);
        return;
    }

    while (n < a->nitems) {
        destruct(Array_Item(a, a->nitems - 1));
        a->nitems--;
    }

    a->nslots = n;
    a->data = realloc(a->data, Array_Step(a) * a->nslots);

#if METADOT_C_MEMORY_CHECK == 1
    if (a->data is NULL) { throw(OutOfMemoryError, "Cannot grow Array, out of memory!", ""); }
#endif
}

static void Array_Mark(var self, var gc, void (*f)(var, void *)) {
    struct Array *a = self;
    for (size_t i = 0; i < a->nitems; i++) { f(gc, Array_Item(a, i)); }
}

var Array = MetaDotC(
        Array,
        Instance(Doc, Array_Name, Array_Brief, Array_Description, NULL, Array_Examples, NULL),
        Instance(New, Array_New, Array_Del), Instance(Assign, Array_Assign),
        Instance(Mark, Array_Mark), Instance(Cmp, Array_Cmp), Instance(Hash, Array_Hash),
        Instance(Push, Array_Push, Array_Pop, Array_Push_At, Array_Pop_At),
        Instance(Concat, Array_Concat, Array_Push), Instance(Len, Array_Len),
        Instance(Get, Array_Get, Array_Set, Array_Mem, Array_Rem),
        Instance(Iter, Array_Iter_Init, Array_Iter_Next, Array_Iter_Last, Array_Iter_Prev,
                 Array_Iter_Type),
        Instance(Sort, Array_Sort_By), Instance(Show, Array_Show, NULL),
        Instance(Resize, Array_Resize));

static const char *Assign_Name(void) { return "Assign"; }

static const char *Assign_Brief(void) { return "Assignment"; }

static const char *Assign_Description(void) {
    return "`Assign` is potentially the most important class in MetaDotC. It is used "
           "throughout MetaDotC to initialise objects using other objects. In C++ this is "
           "called the _copy constructor_ and it is used to assign the value of one "
           "object to another."
           "\n\n"
           "By default the `Assign` class uses the `Size` class to copy the memory "
           "from one object to another. But for more complex objects which maintain "
           "their own behaviours and state this may need to be overridden."
           "\n\n"
           "The most important thing about the `Assign` class is that it must work on "
           "the assumption that the target object may not have had it's constructor "
           "called and could be uninitialised with just zero'd memory. This is often "
           "the case when copying contents into containers.";
}

static const char *Assign_Definition(void) {
    return "struct Assign {\n"
           "  void (*assign)(var, var);\n"
           "};\n";
}

static struct Example *Assign_Examples(void) {

    static struct Example examples[] = {{"Usage", "var x = new(Int, $I(10));\n"
                                                  "var y = new(Int, $I(20));\n"
                                                  "\n"
                                                  "show(x); /* 10 */\n"
                                                  "show(y); /* 20 */\n"
                                                  "\n"
                                                  "assign(x, y);\n"
                                                  "\n"
                                                  "show(x); /* 20 */\n"
                                                  "show(y); /* 20 */\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Assign_Methods(void) {

    static struct Method methods[] = {
            {"assign", "var assign(var self, var obj);",
             "Assign the object `obj` to the object `self`. The assigned object "
             "`self` is returned."},
            {NULL, NULL, NULL}};

    return methods;
}

var Assign = MetaDotC(Assign, Instance(Doc, Assign_Name, Assign_Brief, Assign_Description,
                                       Assign_Definition, Assign_Examples, Assign_Methods));

var assign(var self, var obj) {

    struct Assign *a = instance(self, Assign);

    if (a and a->assign) {
        a->assign(self, obj);
        return self;
    }

    size_t s = size(type_of(self));
    if (type_of(self) is type_of(obj) and s) { return memcpy(self, obj, s); }

    return throw(TypeError, "Cannot assign type %s to type %s", type_of(obj), type_of(self));
}

static const char *Swap_Name(void) { return "Swap"; }

static const char *Swap_Brief(void) { return "Swapable"; }

static const char *Swap_Description(void) {
    return "The `Swap` class can be used to override the behaviour of swapping two "
           "objects. By default the `Swap` class simply swaps the memory of the "
           "two objects passed in as parameters making use of the `Size` class. "
           "In almost all cases this default behaviour should be fine, even if the "
           "objects have custom assignment functions."
           "\n\n"
           "Swapping can be used internally by various collections and algorithms.";
}

static const char *Swap_Definition(void) {
    return "struct Swap {\n"
           "  void (*swap)(var, var);\n"
           "};\n";
}

static struct Example *Swap_Examples(void) {

    static struct Example examples[] = {{"Usage", "var x = $S(\"Hello\");\n"
                                                  "var y = $S(\"World\");\n"
                                                  "show(x); /* Hello */\n"
                                                  "show(y); /* World */\n"
                                                  "swap(x, y);\n"
                                                  "show(x); /* World */\n"
                                                  "show(y); /* Hello */\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Swap_Methods(void) {

    static struct Method methods[] = {{"swap", "void swap(var self, var obj);",
                                       "Swap the object `self` for the object `obj`."},
                                      {NULL, NULL, NULL}};

    return methods;
}

var Swap = MetaDotC(Swap, Instance(Doc, Swap_Name, Swap_Brief, Swap_Description, Swap_Definition,
                                   Swap_Examples, Swap_Methods));

static void memswap(void *p0, void *p1, size_t s) {
    if (p0 == p1) { return; }
    for (size_t i = 0; i < s; i++) {
        char t = ((char *) p0)[i];
        ((char *) p0)[i] = ((char *) p1)[i];
        ((char *) p1)[i] = t;
    }
}

void swap(var self, var obj) {

    struct Swap *s = instance(self, Swap);
    if (s and s->swap) {
        s->swap(self, obj);
        return;
    }

    size_t n = size(type_of(self));
    if (type_of(self) is type_of(obj) and n) {
        memswap(self, obj, n);
        return;
    }

    throw(TypeError, "Cannot swap type %s and type %s", type_of(obj), type_of(self));
}

static const char *Cmp_Name(void) { return "Cmp"; }

static const char *Cmp_Brief(void) { return "Comparison"; }

static const char *Cmp_Description(void) {
    return "The `Cmp` class is used to define comparison between two object values. "
           "This class is important as it is used by many data structures to test "
           "equality or ordering of objects."
           "\n\n"
           "By default, if passed two objects of the same type, the `Cmp` class will "
           "simply compare the raw memory of both objects, using the `Size` "
           "class."
           "\n\n"
           "To implement this class a `cmp` function must be provided which returns "
           "`< 0` if the first object is _less than_ the second, `> 0` if the first "
           "object is _greater than_ the second, and `0` if they are _equal_. "
           "\n\n"
           "For objects that manage their own data this class may need to be "
           "overridden to ensure that objects of the same _value_ are still treated "
           "as equal. E.G. for string types."
           "\n\n"
           "This class to used to test for _value_ equality between objects, I.E. if "
           "they represent the same thing. For _object_ equality the `is` keyword can "
           "be used, which will return `true` only if two variables are pointing to "
           "the same object in memory.";
}

static const char *Cmp_Definition(void) {
    return "struct Cmp {\n"
           "  int (*cmp)(var, var);\n"
           "};\n";
}

static struct Example *Cmp_Examples(void) {

    static struct Example examples[] = {{"Usage 1",
                                         "show($I( eq($I(1), $I( 1)))); /* 1 */\n"
                                         "show($I(neq($I(2), $I(20)))); /* 1 */\n"
                                         "show($I(neq($S(\"Hello\"), $S(\"Hello\")))); /* 0 */\n"
                                         "show($I( eq($S(\"Hello\"), $S(\"There\")))); /* 0 */\n"
                                         "\n"
                                         "var a = $I(1); var b = $I(1);\n"
                                         "\n"
                                         "show($I(eq(a, b))); /* 1 */\n"
                                         "show($I(a is b));   /* 0 */\n"
                                         "show($I(a isnt b)); /* 1 */\n"},
                                        {"Usage 2", "show($I(gt($I(15), $I(3 )))); /* 1 */\n"
                                                    "show($I(lt($I(70), $I(81)))); /* 1 */\n"
                                                    "show($I(lt($I(71), $I(71)))); /* 0 */\n"
                                                    "show($I(ge($I(78), $I(71)))); /* 1 */\n"
                                                    "show($I(gt($I(32), $I(32)))); /* 0 */\n"
                                                    "show($I(le($I(21), $I(32)))); /* 1 */\n"
                                                    "\n"
                                                    "show($I(cmp($I(20), $I(20)))); /*  0 */\n"
                                                    "show($I(cmp($I(21), $I(20)))); /*  1 */\n"
                                                    "show($I(cmp($I(20), $I(21)))); /* -1 */\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Cmp_Methods(void) {

    static struct Method methods[] = {
            {"cmp", "int cmp(var self, var obj);",
             "The return value of `cmp` is `< 0` if `self` is less than `obj`, `> 0` "
             "if `self` is greater than `obj` and `0` if they are equal."},
            {"eq", "bool eq(var self, var obj);",
             "Returns true if the object `self` is equal to the object `obj`."},
            {"neq", "bool neq(var self, var obj);",
             "Returns false if the object `self` is equal to the object `obj`."},
            {"gt", "bool gt(var self, var obj);",
             "Returns true if the object `self` is greater than the object `obj`."},
            {"lt", "bool lt(var self, var obj);",
             "Returns false if the object `self` is less than the object `obj`."},
            {"ge", "bool ge(var self, var obj);",
             "Returns false if the object `self` is greater than or equal to the "
             "object `obj`."},
            {"le", "bool le(var self, var obj);",
             "Returns false if the object `self` is less than or equal to the "
             "object `obj`."},
            {NULL, NULL, NULL}};

    return methods;
}

var Cmp = MetaDotC(Cmp, Instance(Doc, Cmp_Name, Cmp_Brief, Cmp_Description, Cmp_Definition,
                                 Cmp_Examples, Cmp_Methods));

int cmp(var self, var obj) {

    struct Cmp *c = instance(self, Cmp);
    if (c and c->cmp) { return c->cmp(self, obj); }

    size_t s = size(type_of(self));
    if (type_of(self) is type_of(obj) and s) { return memcmp(self, obj, s); }

    throw(TypeError, "Cannot compare type %s to type %s", type_of(obj), type_of(self));

    return 0;
}

bool eq(var self, var obj) { return cmp(self, obj) is 0; }
bool neq(var self, var obj) { return not eq(self, obj); }
bool gt(var self, var obj) { return cmp(self, obj) > 0; }
bool lt(var self, var obj) { return cmp(self, obj) < 0; }
bool ge(var self, var obj) { return not lt(self, obj); }
bool le(var self, var obj) { return not gt(self, obj); }

static const char *Sort_Name(void) { return "Sort"; }

static const char *Sort_Brief(void) { return "Sortable"; }

static const char *Sort_Description(void) {
    return "The `Sort` class can be implemented by types which can be sorted in some "
           "way such as `Array`. By default the sorting function uses the `lt` method "
           "to compare elements, but a custom function can also be provided.";
}

static const char *Sort_Definition(void) {
    return "struct Sort {\n"
           "  void (*sort_by)(var,bool(*f)(var,var));\n"
           "};";
}

static struct Example *Sort_Examples(void) {

    static struct Example examples[] = {
            {"Usage", "var x = new(Array, Float, \n"
                      "  $F(5.2), $F(7.1), $F(2.2));\n"
                      "\n"
                      "show(x); /* <'Array' At 0x00414603 [5.2, 7.1, 2.2]> */\n"
                      "sort(x);\n"
                      "show(x); /* <'Array' At 0x00414603 [2.2, 5.2, 7.1]> */\n"},
            {NULL, NULL}};

    return examples;
}

static struct Method *Sort_Methods(void) {

    static struct Method methods[] = {{"sort", "void sort(var self);", "Sorts the object `self`."},
                                      {"sort_by", "void sort_by(var self, bool(*f)(var,var));",
                                       "Sorts the object `self` using the function `f`."},
                                      {NULL, NULL, NULL}};

    return methods;
}

var Sort = MetaDotC(Sort, Instance(Doc, Sort_Name, Sort_Brief, Sort_Description, Sort_Definition,
                                   Sort_Examples, Sort_Methods));

void sort(var self) { method(self, Sort, sort_by, lt); }

void sort_by(var self, bool (*f)(var, var)) { method(self, Sort, sort_by, f); }

static const char *Concat_Name(void) { return "Concat"; }

static const char *Concat_Brief(void) { return "Concatenate Objects"; }

static const char *Concat_Description(void) {
    return "The `Concat` class is implemented by objects that can have other objects "
           "either _appended_ to their, on _concatenated_ to them. For example "
           "collections or strings.";
}

static const char *Concat_Definition(void) {
    return "struct Concat {\n"
           "  void (*concat)(var, var);\n"
           "  void (*append)(var, var);\n"
           "};\n";
}

static struct Example *Concat_Examples(void) {

    static struct Example examples[] = {
            {"Usage", "var x = new(Array, Float, $F(9.9), $F(2.8));\n"
                      "var y = new(Array, Float, $F(1.1), $F(6.5));\n"
                      "\n"
                      "show(x); /* <'Array' At 0x00414603 [9.9, 2.8]> */\n"
                      "show(y); /* <'Array' At 0x00414603 [1.1, 6.5]> */\n"
                      "append(x, $F(2.5));\n"
                      "show(x); /* <'Array' At 0x00414603 [9.9, 2.8, 2.5]> */\n"
                      "concat(x, y);\n"
                      "show(x); /* <'Array' At 0x00414603 [9.9, 2.8, 2.5, 1.1, 6.5]> */\n"},
            {NULL, NULL}};

    return examples;
}

static struct Method *Concat_Methods(void) {

    static struct Method methods[] = {{"append", "void append(var self, var obj);",
                                       "Append the object `obj` to the object `self`."},
                                      {"concat", "void concat(var self, var obj);",
                                       "Concatenate the object `obj` to the object `self`."},
                                      {NULL, NULL, NULL}};

    return methods;
}

var Concat = MetaDotC(Concat, Instance(Doc, Concat_Name, Concat_Brief, Concat_Description,
                                       Concat_Definition, Concat_Examples, Concat_Methods));

void append(var self, var obj) { method(self, Concat, append, obj); }

void concat(var self, var obj) { method(self, Concat, concat, obj); }

static const char *Doc_Name(void) { return "Doc"; }

static const char *Doc_Brief(void) { return "Provides Documentation"; }

static const char *Doc_Description(void) {
    return "The `Doc` class can be used to give documentation to a certain class or "
           "type. This documentation can then be accessed using the `help` function "
           "or by other tools used to generate documentation such as for the MetaDotC "
           "website. Documentation can be written in Markdown."
           "\n\n"
           "The `examples` and `methods` entries should be provided as `NULL` "
           "terminated arrays allocated statically.";
}

static const char *Doc_Definition(void) {
    return "struct Example {\n"
           "  const char* name;\n"
           "  const char* body;\n"
           "};\n"
           "\n"
           "struct Method {\n"
           "  const char* name;\n"
           "  const char* definition;\n"
           "  const char* description;\n"
           "};\n"
           "\n"
           "struct Doc {\n"
           "  const char* (*name)(void);\n"
           "  const char* (*brief)(void);\n"
           "  const char* (*description)(void);\n"
           "  const char* (*definition)(void);\n"
           "  struct Example* (*examples)(void);\n"
           "  struct Method* (*methods)(void);\n"
           "};\n";
}

static struct Method *Doc_Methods(void) {

    static struct Method methods[] = {
            {"name", "const char* name(var type);", "Return the name of a given `type`."},
            {"brief", "const char* brief(var type);",
             "Return a brief description of a given `type`."},
            {"description", "const char* description(var type);",
             "Return a longer description of a given `type`."},
            {"definition", "const char* definition(var type);",
             "Return the C definition of a given `type`."},
            {NULL, NULL, NULL}};

    return methods;
}

static struct Example *Doc_Examples(void) {

    static struct Example examples[] = {{"Usage", "show($S(name(Int))); /* Int */\n"
                                                  "show($S(brief(Int))); /* Integer Object */\n"},
                                        {NULL, NULL}};

    return examples;
}

var Doc = MetaDotC(Doc, Instance(Doc, Doc_Name, Doc_Brief, Doc_Description, Doc_Definition,
                                 Doc_Examples, Doc_Methods));

const char *name(var type) {
    struct Doc *doc = type_instance(type, Doc);
    if (doc->name) { return doc->name(); }
    return c_str(type);
}

const char *brief(var type) { return type_method(type, Doc, brief); }

const char *description(var type) { return type_method(type, Doc, description); }

const char *definition(var type) { return type_method(type, Doc, definition); }

static const char *Help_Name(void) { return "Help"; }

static const char *Help_Brief(void) { return "Usage information"; }

static const char *Help_Description(void) {
    return "The `Help` class can be implemented to let an object provide helpful "
           "information about itself. In the standard library this class is "
           "implemented by `Type` and it prints out the documentation provided "
           "by the `Doc` class in a friendly way.";
}

static const char *Help_Definition(void) {
    return "struct Help {\n"
           "  int (*help_to)(var, int);\n"
           "};\n";
}

static struct Method *Help_Methods(void) {

    static struct Method methods[] = {
            {"help",
             "void help(var self);\n"
             "int help_to(var out, int pos, var self);",
             "Print help information about the object `self` either to `stdout` or "
             "to the object `out` at some position `pos`."},
            {NULL, NULL, NULL}};

    return methods;
}

static struct Example *Help_Examples(void) {

    static struct Example examples[] = {{"Usage", "help(Int);\n"}, {NULL, NULL}};

    return examples;
}

var Help = MetaDotC(Help, Instance(Doc, Help_Name, Help_Brief, Help_Description, Help_Definition,
                                   Help_Examples, Help_Methods));

int help_to(var out, int pos, var self) { return method(self, Help, help_to, out, pos); }

void help(var self) { help_to($(File, stdout), 0, self); }

#define EXCEPTION_TLS_KEY "__Exception"

enum {
    EXCEPTION_MAX_DEPTH = 2048,
    EXCEPTION_MAX_STRACE = 25
};

var TypeError = MetaDotCEmpty(TypeError);
var ValueError = MetaDotCEmpty(ValueError);
var ClassError = MetaDotCEmpty(ClassError);
var IndexOutOfBoundsError = MetaDotCEmpty(IndexOutOfBoundsError);
var KeyError = MetaDotCEmpty(KeyError);
var OutOfMemoryError = MetaDotCEmpty(OutOfMemoryError);
var IOError = MetaDotCEmpty(IOError);
var FormatError = MetaDotCEmpty(FormatError);
var BusyError = MetaDotCEmpty(BusyError);
var ResourceError = MetaDotCEmpty(ResourceError);

var ProgramAbortedError = MetaDotCEmpty(ProgramAbortedError);
var DivisionByZeroError = MetaDotCEmpty(DivisionByZeroError);
var IllegalInstructionError = MetaDotCEmpty(IllegalInstructionError);
var ProgramInterruptedError = MetaDotCEmpty(ProgramInterruptedError);
var SegmentationError = MetaDotCEmpty(SegmentationError);
var ProgramTerminationError = MetaDotCEmpty(ProgramTerminationError);

struct Exception
{
    var obj;
    var msg;
    size_t depth;
    bool active;
    jmp_buf *buffers[EXCEPTION_MAX_DEPTH];
};

static const char *Exception_Name(void) { return "Exception"; }

static const char *Exception_Brief(void) { return "Exception Object"; }

static const char *Exception_Description(void) {
    return "The `Exception` type provides an interface to the MetaDotC Exception System. "
           "One instance of this type is created for each `Thread` and stores the "
           "various bits of data required for the exception system. It can be "
           "retrieved using the `current` function, although not much can be done "
           "with it."
           "\n\n"
           "Exceptions are available via the `try`, `catch` and `throw` macros. It is "
           "important that the `catch` part of the exception block is always "
           "evaluated otherwise the internal state of the exception system can go out "
           "of sync. For this reason please never use `return` inside a `try` block. "
           "\n\n"
           "The `exception_signals` method can be used to register some exception to "
           "be thrown for any of the "
           "[standard C signals](https://en.wikipedia.org/wiki/C_signal_handling)."
           "\n\n"
           "To get the current exception object or message use the "
           "`exception_message` or `exception_object` methods.";
}

static struct Method *Exception_Methods(void) {

    static struct Method methods[] = {
            {"try", "#define try", "Start an exception `try` block."},
            {"catch", "#define catch(...)",
             "Start an exception `catch` block, catching any objects listed in `...` "
             "as the first name given. To catch any exception object leave argument "
             "list empty other than caught variable name."},
            {"#define throw", "throw(E, F, ...)",
             "Throw exception object `E` with format string `F` and arguments `...`."},
            {"exception_signals", "void exception_signals(void);",
             "Register the standard C signals to throw corresponding exceptions."},
            {"exception_object", "void exception_object(void);\n",
             "Retrieve the current exception object."},
            {"exception_message", "void exception_message(void);\n",
             "Retrieve the current exception message."},
            {NULL, NULL, NULL}};

    return methods;
}

static struct Example *Exception_Examples(void) {

    static struct Example examples[] = {{"Usage", "var x = new(Table, String, Int);\n"
                                                  "set(x, $S(\"Hello\"), $I(1));\n"
                                                  "set(x, $S(\"World\"), $I(2));\n"
                                                  "\n"
                                                  "try {\n"
                                                  "  get(x, $S(\"Missing\"));\n"
                                                  "} catch (e in KeyError) {\n"
                                                  "  println(\"Got Exception: %$\", e);\n"
                                                  "}\n"},
                                        {NULL, NULL}};

    return examples;
}

static void Exception_New(var self, var args) {
    struct Exception *e = self;
    e->active = false;
    e->depth = 0;
    e->obj = NULL;
    e->msg = new_raw(String, "");
    memset(e->buffers, 0, sizeof(jmp_buf *) * EXCEPTION_MAX_DEPTH);
    set(current(Thread), $S(EXCEPTION_TLS_KEY), self);
}

static void Exception_Del(var self) {
    struct Exception *e = self;
    del_raw(e->msg);
    rem(current(Thread), $S(EXCEPTION_TLS_KEY));
}

static void Exception_Assign(var self, var obj) {
    struct Exception *e = self;
    struct Exception *o = cast(obj, Exception);
    e->obj = o->obj;
    assign(e->msg, o->msg);
    e->depth = o->depth;
    e->active = o->active;
    memcpy(e->buffers, o->buffers, sizeof(jmp_buf *) * EXCEPTION_MAX_DEPTH);
}

static var Exception_Current(void) { return get(current(Thread), $S(EXCEPTION_TLS_KEY)); }

static void Exception_Signal(int sig) {
    switch (sig) {
        case SIGABRT:
            throw(ProgramAbortedError, "Program Aborted", "");
        case SIGFPE:
            throw(DivisionByZeroError, "Division by Zero", "");
        case SIGILL:
            throw(IllegalInstructionError, "Illegal Instruction", "");
        case SIGINT:
            throw(ProgramInterruptedError, "Program Interrupted", "");
        case SIGSEGV:
            throw(SegmentationError, "Segmentation fault", "");
        case SIGTERM:
            throw(ProgramTerminationError, "Program Terminated", "");
    }
}

static jmp_buf *Exception_Buffer(struct Exception *e) {
    if (e->depth == 0) {
        fprintf(stderr, "MetaDotC Fatal Error: Exception Buffer Out of Bounds!\n");
        abort();
    }
    return e->buffers[e->depth - 1];
}

static size_t Exception_Len(var self) {
    struct Exception *e = self;
    return e->depth;
}

static bool Exception_Running(var self) {
    struct Exception *e = self;
    return e->active;
}

#ifndef METADOT_C_NSTRACE
#if defined(METADOT_C_UNIX)

static void Exception_Backtrace(void) {

    var trace[EXCEPTION_MAX_STRACE];
    size_t trace_count = backtrace(trace, EXCEPTION_MAX_STRACE);
    char **symbols = backtrace_symbols(trace, trace_count);

    print_to($(File, stderr), 0, "!!\tStack Trace: \n", "");
    print_to($(File, stderr), 0, "!!\t\n", "");

    for (size_t i = 0; i < trace_count; i++) {
        print_to($(File, stderr), 0, "!!\t\t[%i] %s\n", $(Int, i), $(String, symbols[i]));
    }
    print_to($(File, stderr), 0, "!!\t\n", "");

    free(symbols);
}

#elif defined(METADOT_C_WINDOWS)

static void Exception_Backtrace(void) {

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    CONTEXT context;
    memset(&context, 0, sizeof(CONTEXT));
    context.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&context);

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
    SymInitialize(process, NULL, TRUE);

    DWORD image;
    STACKFRAME64 stackframe;
    ZeroMemory(&stackframe, sizeof(STACKFRAME64));

#ifdef _M_IX86
    image = IMAGE_FILE_MACHINE_I386;
    stackframe.AddrPC.Offset = context.Eip;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context.Ebp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context.Esp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
    image = IMAGE_FILE_MACHINE_AMD64;
    stackframe.AddrPC.Offset = context.Rip;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context.Rsp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context.Rsp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
    image = IMAGE_FILE_MACHINE_IA64;
    stackframe.AddrPC.Offset = context.StIIP;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context.IntSp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrBStore.Offset = context.RsBSP;
    stackframe.AddrBStore.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context.IntSp;
    stackframe.AddrStack.Mode = AddrModeFlat;
#endif

    print_to($(File, stderr), 0, "!!\tStack Trace: \n");
    print_to($(File, stderr), 0, "!!\t\n");

    for (size_t i = 0; i < EXCEPTION_MAX_STRACE; i++) {

        BOOL result = StackWalk64(image, process, thread, &stackframe, &context, NULL,
                                  SymFunctionTableAccess64, SymGetModuleBase64, NULL);

        if (!result) { break; }

        char *filename = "";
        char *symbolname = "???";
        int lineno = 0;

        char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO) buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 displacement = 0;
        if (SymFromAddr(process, stackframe.AddrPC.Offset, &displacement, symbol)) {
            symbolname = symbol->Name;
        } else {
            symbolname = "???";
        }

        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        DWORD displacementline = 0;
        if (SymGetLineFromAddr64(process, stackframe.AddrPC.Offset, &displacementline, &line)) {
            lineno = line.LineNumber;
            filename = line.FileName;
        } else {
            lineno = 0;
            filename = "";
        }

        if (strcmp(filename, "") == 0) {
            print_to($(File, stderr), 0, "!!\t\t[%i] %s\n", $I(i), $S(symbolname));
        } else {
            print_to($(File, stderr), 0, "!!\t\t[%i] %s:%i %s\n", $I(i), $S(filename), $I(lineno),
                     $S(symbolname));
        }
    }

    print_to($(File, stderr), 0, "!!\t\n");

    SymCleanup(process);
}

#else

static void Exception_Backtrace(void) {}

#endif

#else

static void Exception_Backtrace(void) {}

#endif

static void Exception_Error(struct Exception *e) {

    print_to($(File, stderr), 0, "\n", "");
    print_to($(File, stderr), 0, "!!\t\n", "");
    print_to($(File, stderr), 0, "!!\tUncaught %$\n", e->obj);
    print_to($(File, stderr), 0, "!!\t\n", "");
    print_to($(File, stderr), 0, "!!\t\t %s\n", e->msg);
    print_to($(File, stderr), 0, "!!\t\n", "");

    Exception_Backtrace();

    exit(EXIT_FAILURE);
}

static int Exception_Show(var self, var out, int pos) {
    struct Exception *e = self;
    return print_to(out, pos, "<'Exception' At 0x%p %$ - %$>", self, e->obj, e->msg);
}

var Exception = MetaDotC(
        Exception,
        Instance(Doc, Exception_Name, Exception_Brief, Exception_Description, NULL,
                 Exception_Examples, Exception_Methods),
        Instance(New, Exception_New, Exception_Del), Instance(Assign, Exception_Assign),
        Instance(Len, Exception_Len), Instance(Current, Exception_Current),
        Instance(Start, NULL, NULL, NULL, Exception_Running), Instance(Show, Exception_Show, NULL));

void exception_signals(void) {
    signal(SIGABRT, Exception_Signal);
    signal(SIGFPE, Exception_Signal);
    signal(SIGILL, Exception_Signal);
    signal(SIGINT, Exception_Signal);
    signal(SIGSEGV, Exception_Signal);
    signal(SIGTERM, Exception_Signal);
}

void exception_try(jmp_buf *env) {
    struct Exception *e = current(Exception);
    if (e->depth is EXCEPTION_MAX_DEPTH) {
        fprintf(stderr, "MetaDotC Fatal Error: Exception Buffer Overflow!\n");
        abort();
    }
    e->depth++;
    e->active = false;
    e->buffers[e->depth - 1] = env;
}

var exception_throw(var obj, const char *fmt, var args) {

    struct Exception *e = current(Exception);

    e->obj = obj;
    print_to_with(e->msg, 0, fmt, args);

    if (Exception_Len(e) >= 1) {
        longjmp(*Exception_Buffer(e), 1);
    } else {
        Exception_Error(e);
    }

    return NULL;
}

var exception_catch(var args) {

    struct Exception *e = current(Exception);

    if (not e->active) { return NULL; }

    /* If no Arguments catch all */
    if (len(args) is 0) { return e->obj; }

    /* Check Exception against Arguments */
    foreach (arg in args) {
        if (eq(arg, e->obj)) { return e->obj; }
    }

    /* No matches found. Propagate to outward block */
    if (e->depth >= 1) {
        longjmp(*Exception_Buffer(e), 1);
    } else {
        Exception_Error(e);
    }

    return NULL;
}

void exception_try_end(void) {
    struct Exception *e = current(Exception);
    if (e->depth == 0) {
        fprintf(stderr, "MetaDotC Fatal Error: Exception Buffer Underflow!\n");
        abort();
    }
    e->depth--;
}

void exception_try_fail(void) {
    struct Exception *e = current(Exception);
    e->active = true;
}

static const char *Stream_Name(void) { return "Stream"; }

static const char *Stream_Brief(void) { return "File-like"; }

static const char *Stream_Description(void) {
    return "The `Stream` class represents an abstract set of operations that can be "
           "performed on File-like objects.";
}

static const char *Stream_Definition(void) {
    return "struct Stream {\n"
           "  var  (*sopen)(var,var,var);\n"
           "  void (*sclose)(var);\n"
           "  void (*sseek)(var,int64_t,int);\n"
           "  int64_t (*stell)(var);\n"
           "  void (*sflush)(var);\n"
           "  bool (*seof)(var);\n"
           "  size_t (*sread)(var,void*,size_t);\n"
           "  size_t (*swrite)(var,void*,size_t);\n"
           "};\n";
}

static struct Example *Stream_Examples(void) {

    static struct Example examples[] = {
            {"Usage", "var f = sopen($(File, NULL), $S(\"test.bin\"), $S(\"r\"));\n"
                      "\n"
                      "char c;\n"
                      "while (!seof(f)) {\n"
                      "  sread(f, &c, 1);\n"
                      "  putc(c, stdout);\n"
                      "}\n"
                      "\n"
                      "sclose(f);\n"},
            {NULL, NULL}};

    return examples;
}

static struct Method *Stream_Methods(void) {

    static struct Method methods[] = {
            {"sopen", "var sopen(var self, var resource, var options);",
             "Open the stream `self` with a given `resource` and `options`."},
            {"sclose", "void sclose(var self);", "Close the stream `self`."},
            {"sseek", "void sseek(var self, int64_t pos, int origin);",
             "Seek to the position `pos` from some `origin` in the stream `self`."},
            {"stell", "int64_t stell(var self);",
             "Return the current position of the stream `stell`."},
            {"sflush", "void sflush(var self);", "Flush the buffered contents of stream `self`."},
            {"seof", "bool seof(var self);",
             "Returns true if there is no more information in the stream."},
            {"sread", "size_t sread(var self, void* output, size_t size);",
             "Read `size` bytes from the stream `self` and write them to `output`."},
            {"swrite", "size_t swrite(var self, void* input, size_t size);",
             "Write `size` bytes to the stream `self` and read them from `input`."},
            {NULL, NULL, NULL}};

    return methods;
}

var Stream = MetaDotC(Stream, Instance(Doc, Stream_Name, Stream_Brief, Stream_Description,
                                       Stream_Definition, Stream_Examples, Stream_Methods));

var sopen(var self, var resource, var options) {
    return method(self, Stream, sopen, resource, options);
}

void sclose(var self) { method(self, Stream, sclose); }

void sseek(var self, int64_t pos, int origin) { method(self, Stream, sseek, pos, origin); }

int64_t stell(var self) { return method(self, Stream, stell); }

void sflush(var self) { method(self, Stream, sflush); }

bool seof(var self) { return method(self, Stream, seof); }

size_t sread(var self, void *output, size_t size) {
    return method(self, Stream, sread, output, size);
}

size_t swrite(var self, void *input, size_t size) {
    return method(self, Stream, swrite, input, size);
}

static const char *File_Name(void) { return "File"; }

static const char *File_Brief(void) { return "Operating System File"; }

static const char *File_Description(void) {
    return "The `File` type is a wrapper of the native C `FILE` type representing a "
           "file in the operating system.";
}

static const char *File_Definition(void) {
    return "struct File {\n"
           "  FILE* file;\n"
           "};\n";
}

static struct Example *File_Examples(void) {

    static struct Example examples[] = {
            {"Usage", "var x = new(File, $S(\"test.bin\"), $S(\"wb\"));\n"
                      "char* data = \"hello\";\n"
                      "swrite(x, data, strlen(data));\n"
                      "sclose(x);\n"},
            {"Formatted Printing", "var x = $(File, NULL);\n"
                                   "sopen(x, $S(\"test.txt\"), $S(\"w\"));\n"
                                   "print_to(x, 0, \"%$ is %$ \", $S(\"Dan\"), $I(23));\n"
                                   "print_to(x, 0, \"%$ is %$ \", $S(\"Chess\"), $I(24));\n"
                                   "sclose(x);\n"},
            {"Automatic Closing", "with(f in new(File, $S(\"test.txt\"), $S(\"r\"))) {\n"
                                  "  var k = new(String); resize(k, 100);\n"
                                  "  var v = new(Int, $I(0));\n"
                                  "  foreach (i in range($I(2))) {\n"
                                  "    scan_from(f, 0, \"%$ is %$ \", k, v);\n"
                                  "    show(k); show(v);\n"
                                  "  }\n"
                                  "}\n"},
            {NULL, NULL}};

    return examples;
}

static var File_Open(var self, var filename, var access);
static void File_Close(var self);

static void File_New(var self, var args) {
    struct File *f = self;
    if (len(args) > 0) { File_Open(self, get(args, $I(0)), get(args, $I(1))); }
}

static void File_Del(var self) {
    struct File *f = self;
    if (f->file isnt NULL) { File_Close(self); }
}

static var File_Open(var self, var filename, var access) {
    struct File *f = self;

    if (f->file isnt NULL) { File_Close(self); }

    f->file = fopen(c_str(filename), c_str(access));

    if (f->file is NULL) { throw(IOError, "Could not open file: %s", filename); }

    return self;
}

static void File_Close(var self) {
    struct File *f = self;

    int err = fclose(f->file);
    if (err != 0) { throw(IOError, "Failed to close file: %i", $I(err)); }

    f->file = NULL;
}

static void File_Seek(var self, int64_t pos, int origin) {
    struct File *f = self;

    if (f->file is NULL) { throw(IOError, "Cannot seek file - no file open.", ""); }

    int err = fseek(f->file, pos, origin);
    if (err != 0) { throw(IOError, "Failed to seek in file: %i", $I(err)); }
}

static int64_t File_Tell(var self) {
    struct File *f = self;

    if (f->file is NULL) { throw(IOError, "Cannot tell file - no file open.", ""); }

    int64_t i = ftell(f->file);
    if (i == -1) { throw(IOError, "Failed to tell file: %i", $I(i)); }

    return i;
}

static void File_Flush(var self) {
    struct File *f = self;

    if (f->file is NULL) { throw(IOError, "Cannot flush file - no file open.", ""); }

    int err = fflush(f->file);
    if (err != 0) { throw(IOError, "Failed to flush file: %i", $I(err)); }
}

static bool File_EOF(var self) {
    struct File *f = self;

    if (f->file is NULL) { throw(IOError, "Cannot eof file - no file open.", ""); }

    return feof(f->file);
}

static size_t File_Read(var self, void *output, size_t size) {
    struct File *f = self;

    if (f->file is NULL) { throw(IOError, "Cannot read file - no file open.", ""); }

    size_t num = fread(output, size, 1, f->file);
    if (num isnt 1 and size isnt 0 and not feof(f->file)) {
        throw(IOError, "Failed to read from file: %i", $I(num));
        return num;
    }

    return num;
}

static size_t File_Write(var self, void *input, size_t size) {
    struct File *f = self;

    if (f->file is NULL) { throw(IOError, "Cannot write file - no file open.", ""); }

    size_t num = fwrite(input, size, 1, f->file);
    if (num isnt 1 and size isnt 0) { throw(IOError, "Failed to write to file: %i", $I(num)); }

    return num;
}

static int File_Format_To(var self, int pos, const char *fmt, va_list va) {
    struct File *f = self;

    if (f->file is NULL) { throw(IOError, "Cannot format to file - no file open.", ""); }

    return vfprintf(f->file, fmt, va);
}

static int File_Format_From(var self, int pos, const char *fmt, va_list va) {
    struct File *f = self;

    if (f->file is NULL) { throw(IOError, "Cannot format from file - no file open.", ""); }

    return vfscanf(f->file, fmt, va);
}

var File = MetaDotC(File,
                    Instance(Doc, File_Name, File_Brief, File_Description, File_Definition,
                             File_Examples, NULL),
                    Instance(New, File_New, File_Del), Instance(Start, NULL, File_Close, NULL),
                    Instance(Stream, File_Open, File_Close, File_Seek, File_Tell, File_Flush,
                             File_EOF, File_Read, File_Write),
                    Instance(Format, File_Format_To, File_Format_From));

static const char *Process_Name(void) { return "Process"; }

static const char *Process_Brief(void) { return "Operating System Process"; }

static const char *Process_Description(void) {
    return "The `Process` type is a wrapper for an operating system process as "
           "constructed by the unix-like call `popen`. In this sense it is much like "
           "a standard file in the operating system but that instead of writing data "
           "to a location you are writing it as input to a process.";
}

static const char *Process_Definition(void) {
    return "struct Process {\n"
           "  FILE* proc;\n"
           "};\n";
}

static struct Example *Process_Examples(void) {

    static struct Example examples[] = {{"Usage", "var x = new(Process, $S(\"ls\"), $S(\"r\"));\n"
                                                  "char c;\n"
                                                  "while (not seof(x)) {\n"
                                                  "  sread(x, &c, 1);\n"
                                                  "  print(\"%c\", $I(c));\n"
                                                  "}\n"
                                                  "sclose(x);\n"},
                                        {NULL, NULL}};

    return examples;
}

static var Process_Open(var self, var filename, var access);
static void Process_Close(var self);

static void Process_New(var self, var args) {
    struct Process *p = self;
    p->proc = NULL;
    Process_Open(self, get(args, $I(0)), get(args, $I(1)));
}

static void Process_Del(var self) {
    struct Process *p = self;
    if (p->proc isnt NULL) { Process_Close(self); }
}

static var Process_Open(var self, var filename, var access) {
    struct Process *p = self;

    if (p->proc isnt NULL) { Process_Close(self); }

    p->proc = popen(c_str(filename), c_str(access));

    if (p->proc is NULL) { throw(IOError, "Could not open process: %s", filename); }

    return self;
}

static void Process_Close(var self) {
    struct Process *p = self;

    int err = pclose(p->proc);
    if (err != 0) { throw(IOError, "Failed to close process: %i", $I(err)); }

    p->proc = NULL;
}

static void Process_Seek(var self, int64_t pos, int origin) {
    struct Process *p = self;

    if (p->proc is NULL) { throw(IOError, "Cannot seek process - no process open.", ""); }

    int err = fseek(p->proc, pos, origin);
    if (err != 0) { throw(IOError, "Failed to seek in process: %i", $I(err)); }
}

static int64_t Process_Tell(var self) {
    struct Process *p = self;

    if (p->proc is NULL) { throw(IOError, "Cannot tell process - no process open.", ""); }

    int64_t i = ftell(p->proc);
    if (i == -1) { throw(IOError, "Failed to tell process: %i", $I(i)); }

    return i;
}

static void Process_Flush(var self) {
    struct Process *p = self;

    if (p->proc is NULL) { throw(IOError, "Cannot flush process - no process open.", ""); }

    int err = fflush(p->proc);
    if (err != 0) { throw(IOError, "Failed to flush process: %i", $I(err)); }
}

static bool Process_EOF(var self) {
    struct Process *p = self;

    if (p->proc is NULL) { throw(IOError, "Cannot eof process - no process open.", ""); }

    return feof(p->proc);
}

static size_t Process_Read(var self, void *output, size_t size) {
    struct Process *p = self;

    if (p->proc is NULL) { throw(IOError, "Cannot read process - no process open.", ""); }

    size_t num = fread(output, size, 1, p->proc);
    if (num isnt 1 and size isnt 0 and not feof(p->proc)) {
        throw(IOError, "Failed to read from process: %i", $I(num));
        return num;
    }

    return num;
}

static size_t Process_Write(var self, void *input, size_t size) {
    struct Process *p = self;

    if (p->proc is NULL) { throw(IOError, "Cannot write process - no process open.", ""); }

    size_t num = fwrite(input, size, 1, p->proc);
    if (num isnt 1 and size isnt 0) { throw(IOError, "Failed to write to process: %i", $I(num)); }

    return num;
}

static int Process_Format_To(var self, int pos, const char *fmt, va_list va) {
    struct Process *p = self;

    if (p->proc is NULL) { throw(IOError, "Cannot format to process - no process open.", ""); }

    return vfprintf(p->proc, fmt, va);
}

static int Process_Format_From(var self, int pos, const char *fmt, va_list va) {
    struct Process *p = self;

    if (p->proc is NULL) { throw(IOError, "Cannot format from process - no process open.", ""); }

    return vfscanf(p->proc, fmt, va);
}

var Process = MetaDotC(Process,
                       Instance(Doc, Process_Name, Process_Brief, Process_Description,
                                Process_Definition, Process_Examples, NULL),
                       Instance(New, Process_New, Process_Del),
                       Instance(Start, NULL, Process_Close, NULL),
                       Instance(Stream, Process_Open, Process_Close, Process_Seek, Process_Tell,
                                Process_Flush, Process_EOF, Process_Read, Process_Write),
                       Instance(Format, Process_Format_To, Process_Format_From));

static const char *Call_Name(void) { return "Call"; }

static const char *Call_Brief(void) { return "Callable"; }

static const char *Call_Description(void) {
    return "The `Call` class is used by types which can be called as functions.";
}

static const char *Call_Definition(void) {
    return "struct Call {\n"
           "  var (*call_with)(var, var);\n"
           "};\n";
}

static struct Example *Call_Examples(void) {

    static struct Example examples[] = {{"Usage", "var increment(var args) {\n"
                                                  "  struct Int* i = get(args, $I(0));\n"
                                                  "  i->val++;\n"
                                                  "  return NULL;\n"
                                                  "}\n"
                                                  "\n"
                                                  "var x = $I(0);\n"
                                                  "show(x); /* 0 */\n"
                                                  "call($(Function, increment), x);\n"
                                                  "show(x); /* 1 */\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Call_Methods(void) {

    static struct Method methods[] = {{"call",
                                       "#define call(self, ...)\n"
                                       "var call_with(var self, var args);",
                                       "Call the object `self` with arguments `args`."},
                                      {NULL, NULL, NULL}};

    return methods;
}

var Call = MetaDotC(Call, Instance(Doc, Call_Name, Call_Brief, Call_Description, Call_Definition,
                                   Call_Examples, Call_Methods));

var call_with(var self, var args) { return method(self, Call, call_with, args); }

static const char *Function_Name(void) { return "Function"; }

static const char *Function_Brief(void) { return "Function Object"; }

static const char *Function_Description(void) {
    return "The `Function` type allows C function pointers to be treated as "
           "MetaDotC objects. They can be passed around, stored, and manipulated. Only C "
           "functions of the type `var(*)(var)` can be stored as a `Function` type "
           "and when called the arguments will be wrapped into an iterable and passed "
           "as the first argument, typically in the form of a `tuple`.";
}

static struct Example *Function_Examples(void) {

    static struct Example examples[] = {
            {"Usage", "var increment(var args) {\n"
                      "  struct Int* i = get(args, $I(0));\n"
                      "  i->val++;\n"
                      "  return NULL;\n"
                      "}\n"
                      "\n"
                      "var x = $I(0);\n"
                      "show(x); /* 0 */\n"
                      "call($(Function, increment), x);\n"
                      "show(x); /* 1 */\n"},
            {"Usage 2", "var hello_person(var args) {\n"
                        "  print(\"Hello %$!\", get(args, $I(0)));\n"
                        "  return NULL;\n"
                        "}\n"
                        "\n"
                        "call($(Function, hello_person), $S(\"Dan\"));\n"},
            {"Usage 3", "var add_print(var args) {\n"
                        "  int64_t fst = c_int(get(args, $I(0)));\n"
                        "  int64_t snd = c_int(get(args, $I(1)));\n"
                        "  println(\"%i + %i = %i\", $I(fst), $I(snd), $I(fst+snd));\n"
                        "  return NULL;\n"
                        "}\n"
                        "\n"
                        "call($(Function, add_print), $I(10), $I(21));\n"},
            {NULL, NULL}};

    return examples;
}

static const char *Function_Definition(void) {
    return "struct Function {\n"
           "  var (*func)(var);\n"
           "};\n";
}

static var Function_Call(var self, var args) {
    struct Function *f = self;
    return f->func(args);
}

var Function = MetaDotC(Function,
                        Instance(Doc, Function_Name, Function_Brief, Function_Description,
                                 Function_Definition, Function_Examples, NULL),
                        Instance(Call, Function_Call));

static const char *Mark_Name(void) { return "Mark"; }

static const char *Mark_Brief(void) { return "Markable by GC"; }

static const char *Mark_Description(void) {
    return "The `Mark` class can be overridden to customize the behaviour of the "
           "MetaDotC Garbage Collector on encountering a given type. By default the "
           "allocated memory for a structure is scanned for pointers to other MetaDotC "
           "objects, but if a type does its own memory allocation it may store "
           "pointers to MetaDotC objects in other locations."
           "\n\n"
           "If this is the case the `Mark` class can be overridden and the callback "
           "function `f` must be called on all pointers which might be MetaDotC objects "
           "which are managed by the class. Alternately the `mark` function can be "
           "called on any sub object to start a chain of recursive marking.";
}

static const char *Mark_Definition(void) {
    return "struct Mark {\n"
           "  void (*mark)(var, var, void(*)(var,void*));\n"
           "};\n";
}

static struct Method *Mark_Methods(void) {

    static struct Method methods[] = {
            {"mark", "void mark(var self, var gc, void(*f)(var,void*));",
             "Mark the object `self` with the Garbage Collector `gc` and the callback "
             "function `f`."},
            {NULL, NULL, NULL}};

    return methods;
}

var Mark = MetaDotC(Mark, Instance(Doc, Mark_Name, Mark_Brief, Mark_Description, Mark_Definition,
                                   NULL, Mark_Methods));

void mark(var self, var gc, void (*f)(var, void *)) {
    if (self is NULL) { return; }
    struct Mark *m = instance(self, Mark);
    if (m and m->mark) { m->mark(self, gc, f); }
}

#ifndef METADOT_C_NGC

#define GC_TLS_KEY "__GC"

enum {
    GC_PRIMES_COUNT = 24
};

static const size_t GC_Primes[GC_PRIMES_COUNT] = {
        0,     1,      5,      11,     23,      53,      101,     197,
        389,   683,    1259,   2417,   4733,    9371,    18617,   37097,
        74093, 148073, 296099, 592019, 1100009, 2200013, 4400021, 8800019};

static const char *GC_Name(void) { return "GC"; }

static const char *GC_Brief(void) { return "Garbage Collector"; }

static const char *GC_Description(void) {
    return "The `GC` type provides an interface to the MetaDotC Garbage Collector. One "
           "instance of this type is created for each thread and can be retrieved "
           "using the `current` function. The Garbage Collector can be stopped and "
           "started using `start` and `stop` and objects can be added or removed from "
           "the Garbage Collector using `set` and `rem`.";
}

static struct Example *GC_Examples(void) {

    static struct Example examples[] = {{"Starting & Stopping",
                                         "var gc = current(GC);\n"
                                         "stop(gc);\n"
                                         "var x = new(Int, $I(10)); /* Not added to GC */\n"
                                         "show($I(running(gc))); /* 0 */\n"
                                         "del(x); /* Must be deleted when done */\n"
                                         "start(gc);\n"},
                                        {NULL, NULL}};

    return examples;
}

struct GCEntry
{
    var ptr;
    uint64_t hash;
    bool root;
    bool marked;
};

struct GC
{
    struct GCEntry *entries;
    size_t nslots;
    size_t nitems;
    size_t mitems;
    uintptr_t maxptr;
    uintptr_t minptr;
    var bottom;
    bool running;
    uintptr_t freenum;
    var *freelist;
};

static uint64_t GC_Probe(struct GC *gc, uint64_t i, uint64_t h) {
    int64_t v = i - (h - 1);
    if (v < 0) { v = gc->nslots + v; }
    return v;
}

static const double GC_Load_Factor = 0.9;

static size_t GC_Ideal_Size(size_t size) {
    size = (size_t) ((double) (size + 1) / GC_Load_Factor);
    for (size_t i = 0; i < GC_PRIMES_COUNT; i++) {
        if (GC_Primes[i] >= size) { return GC_Primes[i]; }
    }
    size_t last = GC_Primes[GC_PRIMES_COUNT - 1];
    for (size_t i = 0;; i++) {
        if (last * i >= size) { return last * i; }
    }
}

static void GC_Set_Ptr(struct GC *gc, var ptr, bool root);

static void GC_Rehash(struct GC *gc, size_t new_size) {

    struct GCEntry *old_entries = gc->entries;
    size_t old_size = gc->nslots;

    gc->nslots = new_size;
    gc->entries = calloc(gc->nslots, sizeof(struct GCEntry));

#if METADOT_C_MEMORY_CHECK == 1
    if (gc->entries is NULL) {
        throw(OutOfMemoryError, "Cannot allocate GC Pointer Table, out of memory!", "");
        return;
    }
#endif

    for (size_t i = 0; i < old_size; i++) {
        if (old_entries[i].hash isnt 0) { GC_Set_Ptr(gc, old_entries[i].ptr, old_entries[i].root); }
    }

    free(old_entries);
}

static void GC_Resize_More(struct GC *gc) {
    size_t new_size = GC_Ideal_Size(gc->nitems);
    size_t old_size = gc->nslots;
    if (new_size > old_size) { GC_Rehash(gc, new_size); }
}

static void GC_Resize_Less(struct GC *gc) {
    size_t new_size = GC_Ideal_Size(gc->nitems);
    size_t old_size = gc->nslots;
    if (new_size < old_size) { GC_Rehash(gc, new_size); }
}

static uint64_t GC_Hash(var ptr) { return ((uintptr_t) ptr) >> 3; }

static void GC_Set_Ptr(struct GC *gc, var ptr, bool root) {

    uint64_t i = GC_Hash(ptr) % gc->nslots;
    uint64_t j = 0;
    uint64_t ihash = i + 1;
    struct GCEntry entry = {ptr, ihash, root, 0};

    while (true) {

        uint64_t h = gc->entries[i].hash;
        if (h is 0) {
            gc->entries[i] = entry;
            return;
        }
        if (gc->entries[i].ptr == entry.ptr) { return; }

        uint64_t p = GC_Probe(gc, i, h);
        if (j >= p) {
            struct GCEntry tmp = gc->entries[i];
            gc->entries[i] = entry;
            entry = tmp;
            j = p;
        }

        i = (i + 1) % gc->nslots;
        j++;
    }
}

static bool GC_Mem_Ptr(struct GC *gc, var ptr) {

    if (gc->nslots is 0) { return false; }

    uint64_t i = GC_Hash(ptr) % gc->nslots;
    uint64_t j = 0;

    while (true) {
        uint64_t h = gc->entries[i].hash;
        if (h is 0 or j > GC_Probe(gc, i, h)) { return false; }
        if (gc->entries[i].ptr == ptr) { return true; }
        i = (i + 1) % gc->nslots;
        j++;
    }
}

static void GC_Rem_Ptr(struct GC *gc, var ptr) {

    if (gc->nslots is 0) { return; }

    for (size_t i = 0; i < gc->freenum; i++) {
        if (gc->freelist[i] is ptr) { gc->freelist[i] = NULL; }
    }

    uint64_t i = GC_Hash(ptr) % gc->nslots;
    uint64_t j = 0;

    while (true) {

        uint64_t h = gc->entries[i].hash;
        if (h is 0 or j > GC_Probe(gc, i, h)) { return; }
        if (gc->entries[i].ptr is ptr) {

            var freeitem = gc->entries[i].ptr;
            memset(&gc->entries[i], 0, sizeof(struct GCEntry));

            j = i;
            while (true) {
                uint64_t nj = (j + 1) % gc->nslots;
                uint64_t nh = gc->entries[nj].hash;
                if (nh isnt 0 and GC_Probe(gc, nj, nh) > 0) {
                    memcpy(&gc->entries[j], &gc->entries[nj], sizeof(struct GCEntry));
                    memset(&gc->entries[nj], 0, sizeof(struct GCEntry));
                    j = nj;
                } else {
                    break;
                }
            }

            gc->nitems--;

            dealloc(destruct(freeitem));
            return;
        }

        i = (i + 1) % gc->nslots;
        j++;
    }
}

static void GC_Mark_Item(struct GC *gc, void *ptr);
static void GC_Recurse(struct GC *gc, var ptr);

static void GC_Mark_And_Recurse(struct GC *gc, void *ptr) {
    GC_Mark_Item(gc, ptr);
    GC_Recurse(gc, ptr);
}

static void GC_Recurse(struct GC *gc, var ptr) {

    var type = type_of(ptr);

    if (type is Int or type is Float or type is String or type is Type or type is File or
        type is Process or type is Function) {
        return;
    }

    struct Mark *m = type_instance(type, Mark);
    if (m and m->mark) {
        m->mark(ptr, gc, (void (*)(var, void *)) GC_Mark_And_Recurse);
        return;
    }

    for (size_t i = 0; i + sizeof(var) <= size(type); i += sizeof(var)) {
        var p = ((char *) ptr) + i;
        GC_Mark_Item(gc, *((var *) p));
    }
}

static void GC_Print(struct GC *gc);

static void GC_Mark_Item(struct GC *gc, void *ptr) {

    uintptr_t pval = (uintptr_t) ptr;
    if (pval % sizeof(var) isnt 0 or pval < gc->minptr or pval > gc->maxptr) { return; }

    uint64_t i = GC_Hash(ptr) % gc->nslots;
    uint64_t j = 0;

    while (true) {

        uint64_t h = gc->entries[i].hash;

        if (h is 0 or j > GC_Probe(gc, i, h)) { return; }

        if (gc->entries[i].ptr is ptr and not gc->entries[i].marked) {
            gc->entries[i].marked = true;
            GC_Recurse(gc, gc->entries[i].ptr);
            return;
        }

        i = (i + 1) % gc->nslots;
        j++;
    }
}

static void GC_Mark_Stack(struct GC *gc) {

    var stk = NULL;
    var bot = gc->bottom;
    var top = &stk;

    if (bot == top) { return; }

    if (bot < top) {
        for (var p = top; p >= bot; p = ((char *) p) - sizeof(var)) {
            GC_Mark_Item(gc, *((var *) p));
        }
    }

    if (bot > top) {
        for (var p = top; p <= bot; p = ((char *) p) + sizeof(var)) {
            GC_Mark_Item(gc, *((var *) p));
        }
    }
}

static void GC_Mark_Stack_Fake(struct GC *gc) {}

void GC_Mark(struct GC *gc) {

    if (gc is NULL or gc->nitems is 0) { return; }

    /* Mark Thread Local Storage */
    mark(current(Thread), gc, (void (*)(var, void *)) GC_Mark_Item);

    /* Mark Roots */
    for (size_t i = 0; i < gc->nslots; i++) {
        if (gc->entries[i].hash is 0) { continue; }
        if (gc->entries[i].marked) { continue; }
        if (gc->entries[i].root) {
            gc->entries[i].marked = true;
            GC_Recurse(gc, gc->entries[i].ptr);
        }
    }

    volatile int noinline = 1;

    /* Flush Registers to Stack */
    if (noinline) {
        jmp_buf env;
        memset(&env, 0, sizeof(jmp_buf));
        setjmp(env);
    }

    /* Avoid Inlining function call */
    void (*mark_stack)(struct GC * gc) =
            noinline ? GC_Mark_Stack : (void (*)(struct GC * gc))(NULL);

    /* Mark Stack */
    mark_stack(gc);
}

static int GC_Show(var self, var out, int pos) {
    struct GC *gc = self;

    pos = print_to(out, pos, "<'GC' At 0x%p\n", self);
    for (size_t i = 0; i < gc->nslots; i++) {
        if (gc->entries[i].hash is 0) {
            pos = print_to(out, pos, "| %i : \n", $I(i));
            continue;
        }
        pos = print_to(out, pos, "| %i : %15s %p %s %s\n", $I(i), type_of(gc->entries[i].ptr),
                       gc->entries[i].ptr, gc->entries[i].root ? $S("root") : $S("auto"),
                       gc->entries[i].marked ? $S("*") : $S(" "));
    }

    return print_to(out, pos, "+------------------->\n", "");
}

void GC_Sweep(struct GC *gc) {

    gc->freelist = realloc(gc->freelist, sizeof(var) * gc->nitems);
    gc->freenum = 0;

    size_t i = 0;
    while (i < gc->nslots) {

        if (gc->entries[i].hash is 0) {
            i++;
            continue;
        }
        if (gc->entries[i].marked) {
            i++;
            continue;
        }

        if (not gc->entries[i].root and not gc->entries[i].marked) {

            gc->freelist[gc->freenum] = gc->entries[i].ptr;
            gc->freenum++;
            memset(&gc->entries[i], 0, sizeof(struct GCEntry));

            uint64_t j = i;
            while (true) {
                uint64_t nj = (j + 1) % gc->nslots;
                uint64_t nh = gc->entries[nj].hash;
                if (nh isnt 0 and GC_Probe(gc, nj, nh) > 0) {
                    memcpy(&gc->entries[j], &gc->entries[nj], sizeof(struct GCEntry));
                    memset(&gc->entries[nj], 0, sizeof(struct GCEntry));
                    j = nj;
                } else {
                    break;
                }
            }

            gc->nitems--;
            continue;
        }

        i++;
    }

    for (size_t i = 0; i < gc->nslots; i++) {
        if (gc->entries[i].hash is 0) { continue; }
        if (gc->entries[i].marked) {
            gc->entries[i].marked = false;
            continue;
        }
    }

    GC_Resize_Less(gc);
    gc->mitems = gc->nitems + gc->nitems / 2 + 1;

    for (size_t i = 0; i < gc->freenum; i++) {
        if (gc->freelist[i]) { dealloc(destruct(gc->freelist[i])); }
    }

    free(gc->freelist);
    gc->freelist = NULL;
    gc->freenum = 0;
}

static var GC_Current(void) { return get(current(Thread), $S(GC_TLS_KEY)); }

static void GC_New(var self, var args) {
    struct GC *gc = self;
    struct Ref *bt = cast(get(args, $I(0)), Ref);
    gc->bottom = bt->val;
    gc->maxptr = 0;
    gc->minptr = UINTPTR_MAX;
    gc->running = true;
    gc->freelist = NULL;
    gc->freenum = 0;
    set(current(Thread), $S(GC_TLS_KEY), gc);
}

static void GC_Del(var self) {
    struct GC *gc = self;
    GC_Sweep(gc);
    free(gc->entries);
    free(gc->freelist);
    rem(current(Thread), $S(GC_TLS_KEY));
}

static void GC_Set(var self, var key, var val) {
    struct GC *gc = self;
    if (not gc->running) { return; }
    gc->nitems++;
    gc->maxptr = (uintptr_t) key > gc->maxptr ? (uintptr_t) key : gc->maxptr;
    gc->minptr = (uintptr_t) key < gc->minptr ? (uintptr_t) key : gc->minptr;
    GC_Resize_More(gc);
    GC_Set_Ptr(gc, key, (bool) c_int(val));
    if (gc->nitems > gc->mitems) {
        GC_Mark(gc);
        GC_Sweep(gc);
    }
}

static void GC_Rem(var self, var key) {
    struct GC *gc = self;
    if (not gc->running) { return; }
    GC_Rem_Ptr(gc, key);
    GC_Resize_Less(gc);
    gc->mitems = gc->nitems + gc->nitems / 2 + 1;
}

static bool GC_Mem(var self, var key) { return GC_Mem_Ptr(self, key); }

static void GC_Start(var self) {
    struct GC *gc = self;
    gc->running = true;
}

static void GC_Stop(var self) {
    struct GC *gc = self;
    gc->running = false;
}

static bool GC_Running(var self) {
    struct GC *gc = self;
    return gc->running;
}

var AutoC_GC =
        MetaDotC(GC, Instance(Doc, GC_Name, GC_Brief, GC_Description, NULL, GC_Examples, NULL),
                 Instance(New, GC_New, GC_Del), Instance(Get, NULL, GC_Set, GC_Mem, GC_Rem),
                 Instance(Start, GC_Start, GC_Stop, NULL, GC_Running),
                 Instance(Show, GC_Show, NULL), Instance(Current, GC_Current));

void MetaDotC_Exit(void) { del_raw(current(AutoC_GC)); }

#endif

static const char *Get_Name(void) { return "Get"; }

static const char *Get_Brief(void) { return "Gettable or Settable"; }

static const char *Get_Description(void) {
    return "The `Get` class provides a method to _get_ or _set_ certain properties "
           "of an object using keys and value. Typically it is implemented by "
           "data lookup structures such as `Table` or `Map` but it is also used "
           "more generally such as using indices to look up items in `Array`, or "
           "as thread local storage for the `Thread` object.";
}

static const char *Get_Definition(void) {
    return "struct Get {\n"
           "  var  (*get)(var, var);\n"
           "  void (*set)(var, var, var);\n"
           "  bool (*mem)(var, var);\n"
           "  void (*rem)(var, var);\n"
           "  var (*key_type)(var);\n"
           "  var (*val_type)(var);\n"
           "};\n";
}

static struct Example *Get_Examples(void) {

    static struct Example examples[] = {{"Usage 1", "var x = new(Array, String, \n"
                                                    "  $S(\"Hello\"), $S(\"There\"));\n"
                                                    "\n"
                                                    "show(get(x, $I(0))); /* Hello */\n"
                                                    "show(get(x, $I(1))); /* There */\n"
                                                    "set(x, $I(1), $S(\"Blah\"));\n"
                                                    "show(get(x, $I(1))); /* Blah */\n"},
                                        {"Usage 2",
                                         "var prices = new(Table, String, Int, \n"
                                         "  $S(\"Apple\"),  $I(12),\n"
                                         "  $S(\"Banana\"), $I( 6),\n"
                                         "  $S(\"Pear\"),   $I(55));\n"
                                         "\n"
                                         "var pear_price   = get(prices, $S(\"Pear\"));\n"
                                         "var banana_price = get(prices, $S(\"Banana\"));\n"
                                         "var apple_price  = get(prices, $S(\"Apple\"));\n"
                                         "\n"
                                         "show(pear_price);   /* 55 */\n"
                                         "show(banana_price); /*  6 */\n"
                                         "show(apple_price);  /* 12 */\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Get_Methods(void) {

    static struct Method methods[] = {
            {"get", "var get(var self, var key);",
             "Get the value at a given `key` for object `self`."},
            {"set", "void set(var self, var key, var val);",
             "Set the value at a given `key` for object `self`."},
            {"mem", "bool mem(var self, var key);",
             "Returns true if `key` is a member of the object `self`."},
            {"rem", "void rem(var self, var key);", "Removes the `key` from object `self`."},
            {"key_type", "var key_type(var self);", "Returns the key type for the object `self`."},
            {"val_type", "var val_type(var self);",
             "Returns the value type for the object `self`."},
            {NULL, NULL, NULL}};

    return methods;
}

var Get = MetaDotC(Get, Instance(Doc, Get_Name, Get_Brief, Get_Description, Get_Definition,
                                 Get_Examples, Get_Methods));

var get(var self, var key) { return method(self, Get, get, key); }

void set(var self, var key, var val) { method(self, Get, set, key, val); }

bool mem(var self, var key) { return method(self, Get, mem, key); }

void rem(var self, var key) { method(self, Get, rem, key); }

var key_type(var self) { return method(self, Get, key_type); }

var val_type(var self) { return method(self, Get, val_type); }

static const char *Hash_Name(void) { return "Hash"; }

static const char *Hash_Brief(void) { return "Hashable"; }

static const char *Hash_Description(void) {
    return "The `Hash` class provides a mechanism for hashing an object. This hash "
           "value should remain the same across objects that are also considered "
           "equal by the `Cmp` class. For objects that are not considered equal this "
           "value should aim to be evenly distributed across integers."
           "\n\n"
           "This is not a cryptographic hash. It is used for various objects or "
           "data structures that require fast hashing such as the `Table` type. Due "
           "to this it should not be used for cryptography or security."
           "\n\n"
           "By default an object is hashed by using its raw memory with the "
           "[Murmurhash](http://en.wikipedia.org/wiki/MurmurHash) algorithm. Due to "
           "the link between them it is recommended to only override `Hash` and "
           "`Cmp` in conjunction.";
}

static const char *Hash_Definition(void) {
    return "struct Hash {\n"
           "  uint64_t (*hash)(var);\n"
           "};\n";
}

static struct Example *Hash_Examples(void) {

    static struct Example examples[] = {
            {"Usage", "println(\"%li\", $I(hash($I(  1)))); /*   1 */\n"
                      "println(\"%li\", $I(hash($I(123)))); /* 123 */\n"
                      "\n"
                      "/* 866003103 */\n"
                      "println(\"%li\", $I(hash_data($I(123), size(Int))));\n"
                      "\n"
                      "println(\"%li\", $I(hash($S(\"Hello\"))));  /* -1838682532 */\n"
                      "println(\"%li\", $I(hash($S(\"There\"))));  /*   961387266 */\n"
                      "println(\"%li\", $I(hash($S(\"People\")))); /*   697467069 */\n"},
            {NULL, NULL}};

    return examples;
}

static struct Method *Hash_Methods(void) {

    static struct Method methods[] = {
            {"hash", "uint64_t hash(var self);", "Get the hash value for the object `self`."},
            {"hash_data", "uint64_t hash_data(void* data, size_t num);",
             "Hash `num` bytes pointed to by `data` using "
             "[Murmurhash](http://en.wikipedia.org/wiki/MurmurHash)."},
            {NULL, NULL, NULL}};

    return methods;
}

var Hash = MetaDotC(Hash, Instance(Doc, Hash_Name, Hash_Brief, Hash_Description, Hash_Definition,
                                   Hash_Examples, Hash_Methods));

uint64_t hash_data(const void *data, size_t size) {

    const uint64_t m = 0xc6a4a7935bd1e995;
    const int r = 47;
    const uint64_t *d = (const uint64_t *) data;
    const uint64_t *end = d + (size / 8);

    uint64_t h = 0xCe110 ^ (size * m);

    while (d != end) {
        uint64_t k = *d++;
        k *= m;
        k ^= k >> r;
        k *= m;
        h ^= k;
        h *= m;
    }

    const unsigned char *data2 = (const unsigned char *) d;

    switch (size & 7) {
        case 7:
            h ^= (uint64_t) (data2[6]) << 48;
        case 6:
            h ^= (uint64_t) (data2[5]) << 40;
        case 5:
            h ^= (uint64_t) (data2[4]) << 32;
        case 4:
            h ^= (uint64_t) (data2[3]) << 24;
        case 3:
            h ^= (uint64_t) (data2[2]) << 16;
        case 2:
            h ^= (uint64_t) (data2[1]) << 8;
        case 1:
            h ^= (uint64_t) (data2[0]);
            h *= m;
    };

    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

uint64_t hash(var self) {

    struct Hash *h = instance(self, Hash);
    if (h and h->hash) { return h->hash(self); }

    return hash_data(self, size(type_of(self)));
}

var _ = MetaDotCEmpty(_);
var Terminal = MetaDotCEmpty(Terminal);

static const char *Iter_Name(void) { return "Iter"; }

static const char *Iter_Brief(void) { return "Iterable"; }

static const char *Iter_Description(void) {
    return "The `Iter` class is implemented by types which can be looped over. This "
           "allows them to be used in conjunction with the `foreach` macro as well "
           "as various other components of MetaDotC."
           "\n\n"
           "To signal that an interation has finished an iteration should return the "
           "MetaDotC object `Terminal`. Due to this - the `Terminal` object cannot be "
           "placed inside of Tuples because it artificially shortens their length.";
}

static const char *Iter_Definition(void) {
    return "struct Iter {\n"
           "  var (*iter_init)(var);\n"
           "  var (*iter_next)(var, var);\n"
           "  var (*iter_prev)(var, var);\n"
           "  var (*iter_last)(var);\n"
           "  var (*iter_type)(var);\n"
           "};\n";
}

static struct Example *Iter_Examples(void) {

    static struct Example examples[] = {{"Usage", "var x = new(Array, Int, $I(1), $I(2), $I(5));\n"
                                                  "\n"
                                                  "foreach(o in x) {\n"
                                                  "  show(o); /* 1, 2, 5 */\n"
                                                  "}\n"},
                                        {"Table", "var prices = new(Table, String, Int);\n"
                                                  "set(prices, $S(\"Apple\"),  $I(12));\n"
                                                  "set(prices, $S(\"Banana\"), $I( 6));\n"
                                                  "set(prices, $S(\"Pear\"),   $I(55));\n"
                                                  "\n"
                                                  "foreach(key in prices) {\n"
                                                  "  var price = get(prices, key);\n"
                                                  "  print(\"Price of %$ is %$\\n\", key, price);\n"
                                                  "}\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Iter_Methods(void) {

    static struct Method methods[] = {
            {"foreach", "#define foreach(...)\n", "Iterate over elements in a loop."},
            {"iter_init",
             "var iter_init(var self);\n"
             "var iter_last(var self);",
             "Return the initial item (or final item) in the iteration over `self`."},
            {"iter_next",
             "var iter_next(var self, var curr);\n"
             "var iter_prev(var self, var curr);",
             "Given the current item `curr`, return the next (or previous) item in "
             "the iteration over `self`."},
            {"iter_type", "var iter_type(var self);",
             "Returns the type of item that can be expected to be returned by the "
             "iterable."},
            {NULL, NULL, NULL}};

    return methods;
}

var Iter = MetaDotC(Iter, Instance(Doc, Iter_Name, Iter_Brief, Iter_Description, Iter_Definition,
                                   Iter_Examples, Iter_Methods));

var iter_init(var self) { return method(self, Iter, iter_init); }

var iter_next(var self, var curr) { return method(self, Iter, iter_next, curr); }

var iter_last(var self) { return method(self, Iter, iter_last); }

var iter_prev(var self, var curr) { return method(self, Iter, iter_prev, curr); }

var iter_type(var self) { return method(self, Iter, iter_type); }

static const char *Range_Name(void) { return "Range"; }

static const char *Range_Brief(void) { return "Integer Sequence"; }

static const char *Range_Description(void) {
    return "The `Range` type is a basic iterable which acts as a virtual "
           "sequence of integers, starting from some value, stopping at some value "
           "and incrementing by some step."
           "\n\n"
           "This can be a useful replacement for the standard C `for` loop with "
           "decent performance but returning a MetaDotC `Int`. It is constructable on "
           "the stack with the `range` macro which makes it practical and easy to "
           "use.";
}

static const char *Range_Definition(void) {
    return "struct Range {\n"
           "  var value;\n"
           "  int64_t start;\n"
           "  int64_t stop;\n"
           "  int64_t step;\n"
           "};\n";
}

static struct Example *Range_Examples(void) {

    static struct Example examples[] = {{"Usage", "/* Iterate 0 to 10 */\n"
                                                  "foreach (i in range($I(10))) {\n"
                                                  "  print(\"%i\\n\", i);\n"
                                                  "}\n"
                                                  "\n"
                                                  "/* Iterate 10 to 20 */\n"
                                                  "foreach (i in range($I(10), $I(20))) {\n"
                                                  "  print(\"%i\\n\", i);\n"
                                                  "}\n"
                                                  "\n"
                                                  "/* Iterate 10 to 20 with a step of 5 */\n"
                                                  "foreach (i in range($I(10), $I(20), $I(5))) {\n"
                                                  "  print(\"%i\\n\", i);\n"
                                                  "}\n"
                                                  "\n"
                                                  "/* Iterate 20 to 10 */\n"
                                                  "foreach (i in range($I(10), $I(20), $I(-1))) {\n"
                                                  "  print(\"%i\\n\", i);\n"
                                                  "}\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Range_Methods(void) {

    static struct Method methods[] = {
            {"range", "#define range(...)", "Construct a `Range` object on the stack."},
            {NULL, NULL, NULL}};

    return methods;
}

var range_stack(var self, var args) {

    struct Range *r = self;
    size_t nargs = len(args);

    if (nargs > 3) { throw(FormatError, "Received too many arguments to Range constructor", ""); }

    switch (nargs) {
        case 0:
            r->start = 0;
            r->stop = 0;
            r->step = 1;
            break;
        case 1:
            r->start = 0;
            r->stop = c_int(get(args, $I(0)));
            r->step = 1;
            break;
        case 2:
            r->start = get(args, $I(0)) is _ ? 0 : c_int(get(args, $I(0)));
            r->stop = c_int(get(args, $I(1)));
            r->step = 1;
            break;
        case 3:
            r->start = get(args, $I(0)) is _ ? 0 : c_int(get(args, $I(0)));
            r->stop = c_int(get(args, $I(1)));
            r->step = get(args, $I(2)) is _ ? 1 : c_int(get(args, $I(2)));
            break;
    }

    return self;
}

static void Range_New(var self, var args) {
    struct Range *r = self;
    r->value = new (Int, NULL);
    range_stack(self, args);
}

static void Range_Del(var self) {
    struct Range *r = self;
    del(r->value);
}

static void Range_Assign(var self, var obj) {
    struct Range *r = self;
    struct Range *o = cast(obj, Range);
    assign(r->value, o->value);
    r->start = o->start;
    r->stop = o->stop;
    r->step = o->step;
}

static int Range_Cmp(var self, var obj) {
    struct Range *r = self;
    struct Range *o = cast(obj, Range);
    return memcmp(&r->start, &o->start, sizeof(int64_t) * 3);
}

static var Range_Iter_Init(var self) {
    struct Range *r = self;
    struct Int *i = r->value;
    if (r->step == 0) { return Terminal; }
    if (r->step > 0) { i->val = r->start; }
    if (r->step < 0) { i->val = r->stop - 1; }
    if (r->step > 0 and i->val >= r->stop) { return Terminal; }
    if (r->step < 0 and i->val < r->start) { return Terminal; }
    return i;
}

static var Range_Iter_Last(var self) {
    struct Range *r = self;
    struct Int *i = r->value;
    if (r->step == 0) { return Terminal; }
    if (r->step > 0) { i->val = r->stop - 1; }
    if (r->step < 0) { i->val = r->start; }
    if (r->step > 0 and i->val < r->start) { return Terminal; }
    if (r->step < 0 and i->val >= r->stop) { return Terminal; }
    return i;
}

static var Range_Iter_Next(var self, var curr) {
    struct Range *r = self;
    struct Int *i = r->value;
    i->val += r->step;
    if (r->step == 0) { return Terminal; }
    if (r->step > 0 and i->val >= r->stop) { return Terminal; }
    if (r->step < 0 and i->val < r->start) { return Terminal; }
    return i;
}

static var Range_Iter_Prev(var self, var curr) {
    struct Range *r = self;
    struct Int *i = r->value;
    i->val -= r->step;
    if (r->step == 0) { return Terminal; }
    if (r->step > 0 and i->val < r->start) { return Terminal; }
    if (r->step < 0 and i->val >= r->stop) { return Terminal; }
    return i;
}

static var Range_Iter_Type(var self) { return Int; }

static size_t Range_Len(var self) {
    struct Range *r = self;
    if (r->step == 0) { return 0; }
    if (r->step > 0) { return ((r->stop - 1) - r->start) / r->step + 1; }
    if (r->step < 0) { return ((r->stop - 1) - r->start) / -r->step + 1; }
    return 0;
}

static var Range_Get(var self, var key) {
    struct Range *r = self;
    struct Int *x = r->value;

    int64_t i = c_int(key);
    i = i < 0 ? Range_Len(r) + i : i;

    if (r->step == 0) {
        x->val = 0;
        return x;
    }

    if (r->step > 0 and (r->start + r->step * i) < r->stop) {
        x->val = r->start + r->step * i;
        return x;
    }

    if (r->step < 0 and (r->stop - 1 + r->step * i) >= r->start) {
        x->val = r->stop - 1 + r->step * i;
        return x;
    }

    return throw(IndexOutOfBoundsError,
                 "Index '%i' out of bounds for Range of start %i, stop %i and step %i.", key,
                 $I(r->start), $I(r->stop), $I(r->step));
}

static bool Range_Mem(var self, var key) {
    struct Range *r = self;
    int64_t i = c_int(key);
    i = i < 0 ? Range_Len(r) + i : i;
    if (r->step == 0) { return false; }
    if (r->step > 0) { return i >= r->start and i < r->stop and (i - r->start) % r->step is 0; }
    if (r->step < 0) {
        return i >= r->start and i < r->stop and (i - (r->stop - 1)) % -r->step is 0;
    }
    return false;
}

static int Range_Show(var self, var output, int pos) {
    struct Range *r = self;
    pos = print_to(output, pos, "<'Range' At 0x%p [", self);
    var curr = Range_Iter_Init(self);
    while (curr isnt Terminal) {
        pos = print_to(output, pos, "%i", curr);
        curr = Range_Iter_Next(self, curr);
        if (curr isnt Terminal) { pos = print_to(output, pos, ", ", ""); }
    }
    return print_to(output, pos, "]>", "");
}

var Range =
        MetaDotC(Range,
                 Instance(Doc, Range_Name, Range_Brief, Range_Description, Range_Definition,
                          Range_Examples, Range_Methods),
                 Instance(New, Range_New, Range_Del), Instance(Assign, Range_Assign),
                 Instance(Cmp, Range_Cmp), Instance(Len, Range_Len),
                 Instance(Get, Range_Get, NULL, Range_Mem, NULL), Instance(Show, Range_Show, NULL),
                 Instance(Iter, Range_Iter_Init, Range_Iter_Next, Range_Iter_Last, Range_Iter_Prev,
                          Range_Iter_Type));

static const char *Slice_Name(void) { return "Slice"; }

static const char *Slice_Brief(void) { return "Partial Iterable"; }

static const char *Slice_Description(void) {
    return "The `Slice` type is an iterable that allows one to only iterate over "
           "part of another iterable. Given some start, stop and step, only "
           "those entries described by the `Slice` are returned in the iteration."
           "\n\n"
           "Under the hood the `Slice` object still iterates over the whole iterable "
           "but it only returns those values in the range given.";
}

static const char *Slice_Definition(void) {
    return "struct Slice {\n"
           "  var iter;\n"
           "  var range;\n"
           "};\n";
}

static struct Example *Slice_Examples(void) {

    static struct Example examples[] = {
            {"Usage", "var x = tuple(\n"
                      "  $S(\"Hello\"), $S(\"There\"), $S(\"World\"), $S(\"!\"));\n"
                      "\n"
                      "/* Iterate over elements 0 to 2 */\n"
                      "foreach (s in slice(x, $I(2))) {\n"
                      "  print(\"%s\\n\", s);\n"
                      "}\n"
                      "\n"
                      "/* Iterate over elements 1 to 2 */\n"
                      "foreach (s in slice(x, $I(1), $I(2))) {\n"
                      "  print(\"%s\\n\", s);\n"
                      "}\n"
                      "\n"
                      "/* Iterate over every other element */\n"
                      "foreach (s in slice(x, _, _, $I(2))) {\n"
                      "  print(\"%s\\n\", s);\n"
                      "}\n"
                      "\n"
                      "/* Iterate backwards, starting from element 3 */\n"
                      "foreach (s in slice(x, _, $I(2), $I(-1))) {\n"
                      "  print(\"%s\\n\", s);\n"
                      "}\n"},
            {NULL, NULL}};

    return examples;
}

static struct Method *Slice_Methods(void) {

    static struct Method methods[] = {
            {"slice", "#define slice(I, ...)",
             "Construct a `Slice` object on the stack over iterable `I`."},
            {"reverse", "#define reverse(I)",
             "Construct a `Slice` object that iterates over iterable `I` in reverse "
             "order."},
            {NULL, NULL, NULL}};

    return methods;
}

static int64_t Slice_Arg(int part, size_t n, var arg) {

    if (arg is _) {
        if (part is 0) { return 0; }
        if (part is 1) { return n; }
        if (part is 2) { return 1; }
    }

    int64_t a = c_int(arg);

    if (part isnt 2) {
        a = a < 0 ? n + a : a;
        a = a > n ? n : a;
        a = a < 0 ? 0 : a;
    }

    return a;
}

var slice_stack(var self, var args) {

    size_t nargs = len(args);

    if (nargs > 4) { throw(FormatError, "Received too many arguments to Slice constructor", ""); }

    if (nargs < 1) { throw(FormatError, "Received too few arguments to Slice constructor", ""); }

    struct Slice *s = self;
    s->iter = get(args, $I(0));

    struct Range *r = s->range;
    size_t n = len(s->iter);

    switch (nargs) {
        case 1:
            r->start = 0;
            r->stop = n;
            r->step = 1;
            break;
        case 2:
            r->start = 0;
            r->stop = Slice_Arg(1, n, get(args, $I(1)));
            r->step = 1;
            break;
        case 3:
            r->start = Slice_Arg(0, n, get(args, $I(1)));
            r->stop = Slice_Arg(1, n, get(args, $I(2)));
            r->step = 1;
            break;
        case 4:
            r->start = Slice_Arg(0, n, get(args, $I(1)));
            r->stop = Slice_Arg(1, n, get(args, $I(2)));
            r->step = Slice_Arg(2, n, get(args, $I(3)));
            break;
    }

    return self;
}

static void Slice_New(var self, var args) {
    struct Slice *s = self;
    s->range = new (Range, NULL);
    slice_stack(self, args);
}

static void Slice_Del(var self) {
    struct Slice *s = self;
    del(s->range);
}

static void Slice_Assign(var self, var obj) {
    struct Slice *s = self;
    struct Slice *o = cast(obj, Slice);
    s->iter = o->iter;
    assign(s->range, o->range);
}

static int Slice_Cmp(var self, var obj) {
    struct Slice *s = self;
    struct Slice *o = cast(obj, Slice);
    if (s->iter > o->iter) { return 1; }
    if (s->iter < o->iter) { return -1; }
    return cmp(s->range, o->range);
}

static var Slice_Iter_Init(var self) {
    struct Slice *s = self;
    struct Range *r = s->range;

    if (r->step > 0) {
        var curr = iter_init(s->iter);
        for (int64_t i = 0; i < r->start; i++) { curr = iter_next(s->iter, curr); }
        return curr;
    }

    if (r->step < 0) {
        var curr = iter_last(s->iter);
        for (int64_t i = 0; i < (int64_t) len(s->iter) - r->stop; i++) {
            curr = iter_prev(s->iter, curr);
        }
        return curr;
    }

    return Terminal;
}

static var Slice_Iter_Next(var self, var curr) {
    struct Slice *s = self;
    struct Range *r = s->range;

    if (r->step > 0) {
        for (int64_t i = 0; i < r->step; i++) { curr = iter_next(s->iter, curr); }
    }

    if (r->step < 0) {
        for (int64_t i = 0; i < -r->step; i++) { curr = iter_prev(s->iter, curr); }
    }

    return curr;
}

static var Slice_Iter_Type(var self) {
    struct Slice *s = self;
    return iter_type(s->iter);
}

static var Slice_Iter_Last(var self) {
    struct Slice *s = self;
    struct Range *r = s->range;

    if (r->step > 0) {
        var curr = iter_last(s->iter);
        for (int64_t i = 0; i < (int64_t) len(s->iter) - r->stop; i++) {
            curr = iter_prev(s->iter, curr);
        }
        return curr;
    }

    if (r->step < 0) {
        var curr = iter_init(s->iter);
        for (int64_t i = 0; i < r->start; i++) { curr = iter_next(s->iter, curr); }
        return curr;
    }

    return Terminal;
}

static var Slice_Iter_Prev(var self, var curr) {
    struct Slice *s = self;
    struct Range *r = s->range;

    if (r->step > 0) {
        for (int64_t i = 0; i < r->step; i++) { curr = iter_prev(s->iter, curr); }
    }

    if (r->step < 0) {
        for (int64_t i = 0; i < -r->step; i++) { curr = iter_next(s->iter, curr); }
    }

    return curr;
}

static size_t Slice_Len(var self) {
    struct Slice *s = self;
    return Range_Len(s->range);
}

static var Slice_Get(var self, var key) {
    struct Slice *s = self;
    return get(s->iter, Range_Get(s->range, key));
}

static bool Slice_Mem(var self, var key) {
    var curr = Slice_Iter_Init(self);
    while (curr) {
        if (eq(curr, key)) { return true; }
        curr = Slice_Iter_Next(self, curr);
    }
    return false;
}

static int Slice_Show(var self, var output, int pos) {
    struct Slice *s = self;
    pos = print_to(output, pos, "<'Slice' At 0x%p [", self);
    var curr = Slice_Iter_Init(self);
    while (curr isnt Terminal) {
        pos = print_to(output, pos, "%$", curr);
        curr = Slice_Iter_Next(self, curr);
        if (curr isnt Terminal) { pos = print_to(output, pos, ", ", ""); }
    }
    return print_to(output, pos, "]>", "");
}

var Slice = MetaDotC(Slice,
                     Instance(Doc, Slice_Name, Slice_Brief, Slice_Description, Slice_Definition,
                              Slice_Examples, Slice_Methods),
                     Instance(New, Slice_New, Slice_Del), Instance(Assign, Slice_Assign),
                     Instance(Cmp, Slice_Cmp), Instance(Len, Slice_Len),
                     Instance(Get, Slice_Get, NULL, Slice_Mem, NULL),
                     Instance(Iter, Slice_Iter_Init, Slice_Iter_Next, Slice_Iter_Last,
                              Slice_Iter_Prev, Slice_Iter_Type),
                     Instance(Show, Slice_Show, NULL));

static const char *Zip_Name(void) { return "Zip"; }

static const char *Zip_Brief(void) { return "Multiple Iterator"; }

static const char *Zip_Description(void) {
    return "The `Zip` type can be used to combine multiple iterables into one which "
           "is then iterated over all at once and returned as a Tuple. The Zip object "
           "only iterates when all of it's sub iterators have valid items. More "
           "specifically the Zip iteration will terminate if _any_ of the sub "
           "iterators terminate.";
}

static const char *Zip_Definition(void) {
    return "struct Zip {\n"
           "  var iters;\n"
           "  var values;\n"
           "};\n";
}

static struct Example *Zip_Examples(void) {

    static struct Example examples[] = {
            {"Usage", "/* Iterate over two iterables at once */\n"
                      "var x = new(Array, Int, $I(100), $I(200), $I(130));\n"
                      "var y = new(Array, Float, $F(0.1), $F(0.2), $F(1.3));\n"
                      "foreach (pair in zip(x, y)) {\n"
                      "  print(\"x: %$\\n\", get(pair, $I(0)));\n"
                      "  print(\"y: %$\\n\", get(pair, $I(1)));\n"
                      "}\n"
                      "\n"
                      "/* Iterate over iterable with count */\n"
                      "foreach (pair in enumerate(x)) {\n"
                      "  print(\"%i: %$\\n\", get(pair, $I(0)), get(pair, $I(1)));\n"
                      "}\n"},
            {NULL, NULL}};

    return examples;
}

static struct Method *Zip_Methods(void) {

    static struct Method methods[] = {
            {"zip", "#define zip(...)", "Construct a `Zip` object on the stack."},
            {"enumerate", "#define enumerate(I)",
             "Zip the iterable `I` with a `Range` object of the same length."},
            {NULL, NULL, NULL}};

    return methods;
}

var zip_stack(var self) {
    struct Zip *z = self;
    size_t nargs = len(z->iters);
    struct Tuple *t = z->values;
    for (size_t i = 0; i < nargs; i++) { t->items[i] = _; }
    t->items[nargs] = Terminal;
    return z;
}

static void Zip_New(var self, var args) {
    struct Zip *z = self;
    z->iters = new (Tuple, NULL);
    z->values = new (Tuple, NULL);
    assign(z->iters, args);
    for (size_t i = 0; i < len(args); i++) { push(z->values, _); }
}

static void Zip_Del(var self) {
    struct Zip *z = self;
    del(z->iters);
    del(z->values);
}

static void Zip_Assign(var self, var obj) {
    struct Zip *z = self;
    struct Zip *o = cast(obj, Zip);
    assign(z->iters, o->iters);
    assign(z->values, o->values);
}

static var Zip_Iter_Init(var self) {
    struct Zip *z = self;
    struct Tuple *values = z->values;
    struct Tuple *iters = z->iters;
    size_t num = len(iters);
    if (num is 0) { return Terminal; }
    for (size_t i = 0; i < num; i++) {
        var init = iter_init(iters->items[i]);
        if (init is Terminal) { return Terminal; }
        values->items[i] = init;
    }
    return values;
}

static var Zip_Iter_Last(var self) {
    struct Zip *z = self;
    struct Tuple *values = z->values;
    struct Tuple *iters = z->iters;
    size_t num = len(iters);
    if (num is 0) { return Terminal; }
    for (size_t i = 0; i < num; i++) {
        var last = iter_last(iters->items[i]);
        if (last is Terminal) { return Terminal; }
        values->items[i] = last;
    }
    return values;
}

static var Zip_Iter_Next(var self, var curr) {
    struct Zip *z = self;
    struct Tuple *values = z->values;
    struct Tuple *iters = z->iters;
    size_t num = len(iters);
    if (num is 0) { return Terminal; }
    for (size_t i = 0; i < num; i++) {
        var next = iter_next(iters->items[i], get(curr, $I(i)));
        if (next is Terminal) { return Terminal; }
        values->items[i] = next;
    }
    return values;
}

static var Zip_Iter_Prev(var self, var curr) {
    struct Zip *z = self;
    struct Tuple *values = z->values;
    struct Tuple *iters = z->iters;
    size_t num = len(iters);
    if (num is 0) { return Terminal; }
    for (size_t i = 0; i < num; i++) {
        var prev = iter_prev(iters->items[i], get(curr, $I(i)));
        if (prev is Terminal) { return Terminal; }
        values->items[i] = prev;
    }
    return values;
}

static var Zip_Iter_Type(var self) { return Tuple; }

static size_t Zip_Len(var self) {
    struct Zip *z = self;
    struct Tuple *values = z->values;
    struct Tuple *iters = z->iters;
    size_t num = len(iters);
    if (num is 0) { return 0; }
    size_t mlen = len(iters->items[0]);
    for (size_t i = 1; i < num; i++) {
        size_t num = len(iters->items[i]);
        mlen = num < mlen ? num : mlen;
    }
    return mlen;
}

static var Zip_Get(var self, var key) {
    struct Zip *z = self;
    struct Tuple *values = z->values;
    struct Tuple *iters = z->iters;
    size_t num = len(iters);

    for (size_t i = 0; i < num; i++) { values->items[i] = get(iters->items[i], key); }

    return values;
}

static bool Zip_Mem(var self, var key) {
    foreach (item in self) {
        if (eq(item, key)) { return true; }
    }
    return false;
}

var Zip = MetaDotC(
        Zip,
        Instance(Doc, Zip_Name, Zip_Brief, Zip_Description, Zip_Definition, Zip_Examples,
                 Zip_Methods),
        Instance(New, Zip_New, Zip_Del), Instance(Assign, Zip_Assign), Instance(Len, Zip_Len),
        Instance(Get, Zip_Get, NULL, Zip_Mem, NULL),
        Instance(Iter, Zip_Iter_Init, Zip_Iter_Next, Zip_Iter_Last, Zip_Iter_Prev, Zip_Iter_Type));

var enumerate_stack(var self) {
    struct Zip *z = self;
    struct Range *r = get(z->iters, $I(0));
    r->stop = len(get(z->iters, $I(1)));
    return self;
}

static const char *Filter_Name(void) { return "Filter"; }

static const char *Filter_Brief(void) { return "Filtered Iterable"; }

static const char *Filter_Description(void) {
    return "The `Filter` type can be used to filter the results of some iterable. "
           "Given a callable object `Filter` iterable returns only those items in "
           "the original iterable for where calling the function returns a "
           "non-`NULL` value.";
}

static const char *Filter_Definition(void) {
    return "struct Filter {\n"
           "  var iter;\n"
           "  var func;\n"
           "};\n";
}

static struct Example *Filter_Examples(void) {

    static struct Example examples[] = {
            {"Usage", "var greater_than_two(var x) {\n"
                      "  return c_int(x) > 2 ? x : NULL;\n"
                      "}\n"
                      "\n"
                      "var x = new(Array, Int, $I(0), $I(5), $I(2), $I(9));\n"
                      "\n"
                      "foreach (n in filter(x, $(Function, greater_than_two))) {\n"
                      "  show(n); /* 5, 9 */\n"
                      "}\n"},
            {"Usage 2", "var mem_hello(var x) {\n"
                        "  return mem(x, $S(\"Hello\")) ? x : NULL;\n"
                        "}\n"
                        "\n"
                        "var x = new(Tuple, \n"
                        "  $S(\"Hello World\"), $S(\"Hello Dan\"), \n"
                        "  $S(\"Bonjour\"));\n"
                        "\n"
                        "var y = new(Tuple);\n"
                        "assign(y, filter(x, $(Function, mem_hello)));\n"
                        "show(y); /* tuple(\"Hello World\", \"Hello Dan\") */\n"},
            {NULL, NULL}};

    return examples;
}

static struct Method *Filter_Methods(void) {

    static struct Method methods[] = {
            {"filter", "#define filter(I, F)",
             "Construct a `Filter` object on the stack over iterable `I` with "
             "filter function `F`."},
            {NULL, NULL, NULL}};

    return methods;
}

static void Filter_New(var self, var args) {
    struct Filter *f = self;
    f->iter = get(args, $I(0));
    f->func = get(args, $I(1));
}

static var Filter_Iter_Init(var self) {
    struct Filter *f = self;
    var curr = iter_init(f->iter);
    while (true) {
        if (curr is Terminal or call_with(f->func, curr)) {
            return curr;
        } else {
            curr = iter_next(f->iter, curr);
        }
    }
    return Terminal;
}

static var Filter_Iter_Last(var self) {
    struct Filter *f = self;
    var curr = iter_last(f->iter);
    while (true) {
        if (curr is Terminal or call_with(f->func, curr)) {
            return curr;
        } else {
            curr = iter_prev(f->iter, curr);
        }
    }
    return Terminal;
}

static var Filter_Iter_Next(var self, var curr) {
    struct Filter *f = self;
    curr = iter_next(f->iter, curr);
    while (true) {
        if (curr is Terminal or call_with(f->func, curr)) {
            return curr;
        } else {
            curr = iter_next(f->iter, curr);
        }
    }
    return Terminal;
}

static var Filter_Iter_Prev(var self, var curr) {
    struct Filter *f = self;
    curr = iter_prev(f->iter, curr);
    while (true) {
        if (curr is Terminal or call_with(f->func, curr)) {
            return curr;
        } else {
            curr = iter_prev(f->iter, curr);
        }
    }
    return Terminal;
}

static var Filter_Iter_Type(var self) {
    struct Filter *f = self;
    return iter_type(f->iter);
}

static bool Filter_Mem(var self, var key) {
    foreach (item in self) {
        if (eq(item, key)) { return true; }
    }
    return false;
}

var Filter = MetaDotC(Filter,
                      Instance(Doc, Filter_Name, Filter_Brief, Filter_Description,
                               Filter_Definition, Filter_Examples, Filter_Methods),
                      Instance(New, Filter_New, NULL), Instance(Get, NULL, NULL, Filter_Mem, NULL),
                      Instance(Iter, Filter_Iter_Init, Filter_Iter_Next, Filter_Iter_Last,
                               Filter_Iter_Prev, Filter_Iter_Type));

static const char *Map_Name(void) { return "Map"; }

static const char *Map_Brief(void) { return "Apply Function to Iterable"; }

static const char *Map_Description(void) {
    return "The `Map` type is an iterable that applies some callable to to each "
           "item in another iterable and returns the result. This can be useful to "
           "make more concise iteration when there are callback functions available."
           "\n\n"
           "If the mapping callable is a purely side-effect callable it is possible "
           "to use the `call` function on the `Map` object directly for a quick way "
           "to perform the iteration."
           "\n\n"
           "One downside of `Map` is that the `iter_type` becomes unknown (there is "
           "no way to know what type the callable will return so some objects such "
           "as `Array`s may revert to using `Ref` as the object type when assigned a "
           "`Map`.";
}

static const char *Map_Definition(void) {
    return "struct Map {\n"
           "  var iter;\n"
           "  var curr;\n"
           "  var func;\n"
           "};\n";
}

static struct Method *Map_Methods(void) {

    static struct Method methods[] = {
            {"map", "#define map(I, F)",
             "Construct a `Map` object on the stack over iterable `I` applying "
             "function `F`."},
            {NULL, NULL, NULL}};

    return methods;
}

static struct Example *Map_Examples(void) {

    static struct Example examples[] = {
            {"Usage", "var convert_to_int(var x) {\n"
                      "  var y = new(Int);\n"
                      "  look_from(y, x, 0);\n"
                      "  return y;\n"
                      "}\n"
                      "\n"
                      "var x = tuple($S(\"1\"), $S(\"2\"), $S(\"3\"));\n"
                      "\n"
                      "foreach (y in map(x, $(Function, convert_to_int))) {\n"
                      "  show(y); /* 1, 2, 3 */\n"
                      "};\n"},
            {"Usage 2", "var print_object(var x) {\n"
                        "  println(\"Object %$ is of type %$\", x, type_of(x));\n"
                        "  return NULL;\n"
                        "}\n"
                        "\n"
                        "var x = tuple($I(0), $S(\"Hello!\"), $F(2.4));\n"
                        "\n"
                        "call(map(x, $(Function, print_object)));\n"},
            {NULL, NULL}};

    return examples;
}

static void Map_New(var self, var args) {
    struct Map *m = self;
    m->iter = get(args, $I(0));
    m->func = get(args, $I(1));
}

static var Map_Iter_Init(var self) {
    struct Map *m = self;
    m->curr = iter_init(m->iter);
    if (m->curr is Terminal) {
        return m->curr;
    } else {
        return call_with(m->func, m->curr);
    }
}

static var Map_Iter_Last(var self) {
    struct Map *m = self;
    m->curr = iter_last(m->iter);
    if (m->curr is Terminal) {
        return m->curr;
    } else {
        return call_with(m->func, m->curr);
    }
}

static var Map_Iter_Next(var self, var curr) {
    struct Map *m = self;
    m->curr = iter_next(m->iter, m->curr);
    if (m->curr is Terminal) {
        return m->curr;
    } else {
        return call_with(m->func, m->curr);
    }
}

static var Map_Iter_Prev(var self, var curr) {
    struct Map *m = self;
    m->curr = iter_prev(m->iter, m->curr);
    if (m->curr is Terminal) {
        return m->curr;
    } else {
        return call_with(m->func, m->curr);
    }
}

static size_t Map_Len(var self) {
    struct Map *m = self;
    return len(m->iter);
}

static var Map_Get(var self, var key) {
    struct Map *m = self;
    m->curr = get(m->iter, key);
    if (m->curr is Terminal) {
        return m->curr;
    } else {
        return call_with(m->func, m->curr);
    }
}

static bool Map_Mem(var self, var key) {
    foreach (item in self) {
        if (eq(item, key)) { return true; }
    }
    return false;
}

static var Map_Call(var self, var args) {
    foreach (item in self)
        ;
    return Terminal;
}

var Map =
        MetaDotC(Map,
                 Instance(Doc, Map_Name, Map_Brief, Map_Description, Map_Definition, Map_Examples,
                          Map_Methods),
                 Instance(New, Map_New, NULL), Instance(Len, Map_Len),
                 Instance(Get, Map_Get, NULL, Map_Mem, NULL), Instance(Call, Map_Call),
                 Instance(Iter, Map_Iter_Init, Map_Iter_Next, Map_Iter_Last, Map_Iter_Prev, NULL));

static const char *Len_Name(void) { return "Len"; }

static const char *Len_Brief(void) { return "Has a length"; }

static const char *Len_Description(void) {
    return "The `Len` class can be implemented by any type that has a length "
           "associated with it. It is typically implemented by collections "
           "and is often used in conjunction with `Iter` or `Get`.";
}

static const char *Len_Definition(void) {
    return "struct Len {\n"
           "  size_t (*len)(var);\n"
           "};\n";
}

static struct Example *Len_Examples(void) {

    static struct Example examples[] = {{"Usage", "var x = new(Array, Int, $I(1), $I(2), $I(5));\n"
                                                  "show($I(len(x))); /* 3 */\n"
                                                  "var y = $S(\"Test\");\n"
                                                  "show($I(len(y))); /* 4 */\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Len_Methods(void) {

    static struct Method methods[] = {
            {"len", "size_t len(var self);", "Returns the length of object `self`."},
            {NULL, NULL, NULL}};

    return methods;
}

var Len = MetaDotC(Len, Instance(Doc, Len_Name, Len_Brief, Len_Description, Len_Definition,
                                 Len_Examples, Len_Methods));

size_t len(var self) { return method(self, Len, len); }

bool empty(var self) { return len(self) is 0; }

static const char *List_Name(void) { return "List"; }

static const char *List_Brief(void) { return "Linked List"; }

static const char *List_Description(void) {
    return "The `List` type is a linked list data structure. Elements can be added "
           "and removed from the list and their memory is allocated and deallocated "
           "by the structure. Additionally destructors will be called on objects "
           "once removed."
           "\n\n"
           "Elements are copied into the List using `assign` and will initially have "
           "zero'd memory."
           "\n\n"
           "Lists can provide fast insertion and removal at arbitrary locations "
           "although most other operations will be slow due to having to traverse "
           "the linked list data structure."
           "\n\n"
           "This is largely equivalent to the C++ construct "
           "[std::list](http://www.cplusplus.com/reference/list/list/)";
}

static struct Example *List_Examples(void) {

    static struct Example examples[] = {
            {
                    "Construction & Deletion",
                    "var x = new(List, Int);\n"
                    "push(x, $I(32));\n"
                    "push(x, $I(6));\n"
                    "\n"
                    "/* <'List' At 0x0000000000414603 [32, 6]> */\n"
                    "show(x);\n",
            },
            {
                    "Element Access",
                    "var x = new(List, Float, $F(0.01), $F(5.12));\n"
                    "\n"
                    "show(get(x, $I(0))); /* 0.01 */\n"
                    "show(get(x, $I(1))); /* 5.12 */\n"
                    "\n"
                    "set(x, $I(0), $F(500.1));\n"
                    "show(get(x, $I(0))); /* 500.1 */\n",
            },
            {
                    "Membership",
                    "var x = new(List, Int, $I(1), $I(2), $I(3), $I(4));\n"
                    "\n"
                    "show($I(mem(x, $I(1)))); /* 1 */\n"
                    "show($I(len(x)));        /* 4 */\n"
                    "\n"
                    "rem(x, $I(3));\n"
                    "\n"
                    "show($I(mem(x, $I(3)))); /* 0 */\n"
                    "show($I(len(x)));        /* 3 */\n"
                    "show($I(empty(x)));      /* 0 */\n"
                    "\n"
                    "resize(x, 0);\n"
                    "\n"
                    "show($I(empty(x)));      /* 1 */\n",
            },
            {
                    "Iteration",
                    "var greetings = new(List, String, \n"
                    "  $S(\"Hello\"), $S(\"Bonjour\"), $S(\"Hej\"));\n"
                    "\n"
                    "foreach(greet in greetings) {\n"
                    "  show(greet);\n"
                    "}\n",
            },
            {NULL, NULL}};

    return examples;
}

struct List
{
    var type;
    var head;
    var tail;
    size_t tsize;
    size_t nitems;
};

static var List_Alloc(struct List *l) {
    var item = calloc(1, 2 * sizeof(var) + sizeof(struct Header) + l->tsize);

#if METADOT_C_MEMORY_CHECK == 1
    if (item is NULL) { throw(OutOfMemoryError, "Cannot allocate List entry, out of memory!", ""); }
#endif

    return header_init((struct Header *) ((char *) item + 2 * sizeof(var)), l->type, AllocData);
}

static void List_Free(struct List *l, var self) {
    free((char *) self - sizeof(struct Header) - 2 * sizeof(var));
}

static var *List_Next(struct List *l, var self) {
    return (var *) ((char *) self - sizeof(struct Header) - 1 * sizeof(var));
}

static var *List_Prev(struct List *l, var self) {
    return (var *) ((char *) self - sizeof(struct Header) - 2 * sizeof(var));
}

static var List_At(struct List *l, int64_t i) {

    i = i < 0 ? l->nitems + i : i;

#if METADOT_C_BOUND_CHECK == 1
    if (i < 0 or i >= (int64_t) l->nitems) {
        return throw(IndexOutOfBoundsError, "Index '%i' out of bounds for List of size %i.",
                     $(Int, i), $(Int, l->nitems));
    }
#endif

    var item;

    if (i <= (int64_t) (l->nitems / 2)) {
        item = l->head;
        while (i) {
            item = *List_Next(l, item);
            i--;
        }
    } else {
        i = l->nitems - i - 1;
        item = l->tail;
        while (i) {
            item = *List_Prev(l, item);
            i--;
        }
    }

    return item;
}

static void List_Push(var self, var obj);
static void List_Rem(var self, var obj);

static void List_New(var self, var args) {

    struct List *l = self;
    l->type = cast(get(args, $I(0)), Type);
    l->tsize = size(l->type);
    l->nitems = 0;
    l->head = NULL;
    l->tail = NULL;

    size_t nargs = len(args);
    for (size_t i = 0; i < nargs - 1; i++) { List_Push(self, get(args, $I(i + 1))); }
}

static void List_Clear(var self) {
    struct List *l = self;
    var item = l->head;
    while (item) {
        var next = *List_Next(l, item);
        destruct(item);
        List_Free(l, item);
        item = next;
    }
    l->tail = NULL;
    l->head = NULL;
    l->nitems = 0;
}

static void List_Del(var self) {
    struct List *l = self;
    List_Clear(self);
}

static void List_Assign(var self, var obj) {
    struct List *l = self;

    List_Clear(self);

    l->type = implements_method(obj, Iter, iter_type) ? iter_type(obj) : Ref;
    l->tsize = size(l->type);

    size_t nargs = len(obj);
    for (size_t i = 0; i < nargs; i++) { List_Push(self, get(obj, $I(i))); }
}

static void List_Concat(var self, var obj) {
    foreach (item in obj) { List_Push(self, item); }
}

static var List_Iter_Init(var self);
static var List_Iter_Next(var self, var curr);

static int List_Cmp(var self, var obj) {

    var item0 = List_Iter_Init(self);
    var item1 = iter_init(obj);

    while (true) {
        if (item0 is Terminal and item1 is Terminal) { return 0; }
        if (item0 is Terminal) { return -1; }
        if (item1 is Terminal) { return 1; }
        int c = cmp(item0, item1);
        if (c < 0) { return -1; }
        if (c > 0) { return 1; }
        item0 = List_Iter_Next(self, item0);
        item1 = iter_next(obj, item1);
    }

    return 0;
}

static uint64_t List_Hash(var self) {
    struct List *l = self;
    uint64_t h = 0;

    var item = l->head;
    for (size_t i = 0; i < l->nitems; i++) {
        h ^= hash(item);
        item = *List_Next(l, item);
    }

    return h;
}

static size_t List_Len(var self) {
    struct List *l = self;
    return l->nitems;
}

static bool List_Mem(var self, var obj) {
    struct List *l = self;
    var item = l->head;
    while (item) {
        if (eq(item, obj)) { return true; }
        item = *List_Next(l, item);
    }
    return false;
}

static void List_Unlink(struct List *l, var item) {

    var next = *List_Next(l, item);
    var prev = *List_Prev(l, item);

    if (item is l->head and item is l->tail) {
        l->head = NULL;
        l->tail = NULL;
    } else if (item is l->head) {
        l->head = next;
        *List_Prev(l, next) = NULL;
    } else if (item is l->tail) {
        l->tail = prev;
        *List_Next(l, prev) = NULL;
    } else {
        *List_Next(l, prev) = next;
        *List_Prev(l, next) = prev;
    }
}

static void List_Link(struct List *l, var item, var prev, var next) {
    if (prev is NULL) {
        l->head = item;
    } else {
        *List_Next(l, prev) = item;
    }
    if (next is NULL) {
        l->tail = item;
    } else {
        *List_Prev(l, next) = item;
    }
    *List_Next(l, item) = next;
    *List_Prev(l, item) = prev;
}

static void List_Pop_At(var self, var key) {

    struct List *l = self;
    int64_t i = c_int(key);

    var item = List_At(l, i);
    List_Unlink(l, item);
    destruct(item);
    List_Free(l, item);
    l->nitems--;
}

static void List_Rem(var self, var obj) {
    struct List *l = self;
    var item = l->head;
    while (item) {
        if (eq(item, obj)) {
            List_Unlink(l, item);
            destruct(item);
            List_Free(l, item);
            l->nitems--;
            return;
        }
        item = *List_Next(l, item);
    }

    throw(ValueError, "Object %$ not in List!", obj);
}

static void List_Push(var self, var obj) {
    struct List *l = self;
    var item = List_Alloc(l);
    assign(item, obj);
    List_Link(l, item, l->tail, NULL);
    l->nitems++;
}

static void List_Push_At(var self, var obj, var key) {
    struct List *l = self;

    var item = List_Alloc(l);
    assign(item, obj);

    int64_t i = c_int(key);
    if (i is 0) {
        List_Link(l, item, NULL, l->head);
    } else {
        var curr = List_At(l, i);
        List_Link(l, item, *List_Prev(l, curr), curr);
    }
    l->nitems++;
}

static void List_Pop(var self) {

    struct List *l = self;

#if METADOT_C_BOUND_CHECK == 1
    if (l->nitems is 0) {
        throw(IndexOutOfBoundsError, "Cannot pop. List is empty!", "");
        return;
    }
#endif

    var item = l->tail;
    List_Unlink(l, item);
    destruct(item);
    List_Free(l, item);
    l->nitems--;
}

static var List_Get(var self, var key) {
    struct List *l = self;
    return List_At(l, c_int(key));
}

static void List_Set(var self, var key, var val) {
    struct List *l = self;
    assign(List_At(l, c_int(key)), val);
}

static var List_Iter_Init(var self) {
    struct List *l = self;
    if (l->nitems is 0) { return Terminal; }
    return l->head;
}

static var List_Iter_Next(var self, var curr) {
    struct List *l = self;
    curr = *List_Next(l, curr);
    return curr ? curr : Terminal;
}

static var List_Iter_Last(var self) {
    struct List *l = self;
    if (l->nitems is 0) { return Terminal; }
    return l->tail;
}

static var List_Iter_Prev(var self, var curr) {
    struct List *l = self;
    curr = *List_Prev(l, curr);
    return curr ? curr : Terminal;
}

static var List_Iter_Type(var self) {
    struct List *l = self;
    return l->type;
}

static int List_Show(var self, var output, int pos) {
    struct List *l = self;
    pos = print_to(output, pos, "<'List' At 0x%p [", self);
    var item = l->head;
    while (item) {
        pos = print_to(output, pos, "%$", item);
        item = *List_Next(l, item);
        if (item) { pos = print_to(output, pos, ", ", ""); }
    }
    return print_to(output, pos, "]>", "");
}

static void List_Resize(var self, size_t n) {
    struct List *l = self;

    if (n is 0) {
        List_Clear(self);
        return;
    }

    while (n < l->nitems) {
        var item = l->tail;
        List_Unlink(l, item);
        destruct(item);
        List_Free(l, item);
        l->nitems--;
    }

    while (n > l->nitems) {
        var item = List_Alloc(l);
        List_Link(l, item, l->tail, NULL);
        l->nitems++;
    }
}

static void List_Mark(var self, var gc, void (*f)(var, void *)) {
    struct List *l = self;
    var item = l->head;
    while (item) {
        f(gc, item);
        item = *List_Next(l, item);
    }
}

var List = MetaDotC(
        List, Instance(Doc, List_Name, List_Brief, List_Description, NULL, List_Examples, NULL),
        Instance(New, List_New, List_Del), Instance(Assign, List_Assign), Instance(Mark, List_Mark),
        Instance(Cmp, List_Cmp), Instance(Hash, List_Hash),
        Instance(Push, List_Push, List_Pop, List_Push_At, List_Pop_At),
        Instance(Concat, List_Concat, List_Push), Instance(Len, List_Len),
        Instance(Get, List_Get, List_Set, List_Mem, List_Rem),
        Instance(Iter, List_Iter_Init, List_Iter_Next, List_Iter_Last, List_Iter_Prev,
                 List_Iter_Type),
        Instance(Show, List_Show, NULL), Instance(Resize, List_Resize));

static const char *C_Int_Name(void) { return "C_Int"; }

static const char *C_Int_Brief(void) { return "Interpret as C Integer"; }

static const char *C_Int_Description(void) {
    return "The `C_Int` class should be overridden by types which are representable "
           "as a C style Integer of the type `int64_t`.";
}

static const char *C_Int_Definition(void) {
    return "struct C_Int {\n"
           "  int64_t (*c_int)(var);\n"
           "};\n";
}

static struct Example *C_Int_Examples(void) {

    static struct Example examples[] = {{"Usage", "printf(\"%li\", c_int($I(5))); /* 5 */\n"
                                                  "printf(\"%li\", c_int($I(6))); /* 6 */\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *C_Int_Methods(void) {

    static struct Method methods[] = {{"c_int", "int64_t c_int(var self);",
                                       "Returns the object `self` represented as a `int64_t`."},
                                      {NULL, NULL, NULL}};

    return methods;
}

var C_Int = MetaDotC(C_Int, Instance(Doc, C_Int_Name, C_Int_Brief, C_Int_Description,
                                     C_Int_Definition, C_Int_Examples, C_Int_Methods));

static const char *C_Float_Name(void) { return "C_Float"; }

static const char *C_Float_Brief(void) { return "Interpret as C Float"; }

static const char *C_Float_Description(void) {
    return "The `C_Float` class should be overridden by types which are representable "
           "as a C style Float of the type `double`.";
}

static const char *C_Float_Definition(void) {
    return "struct C_Float {\n"
           "  double (*c_float)(var);\n"
           "};\n";
}

static struct Example *C_Float_Examples(void) {

    static struct Example examples[] = {{"Usage", "printf(\"%f\", c_float($F(5.1))); /* 5.1 */\n"
                                                  "printf(\"%f\", c_float($F(6.2))); /* 6.2 */\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *C_Float_Methods(void) {

    static struct Method methods[] = {{"c_float", "double c_float(var self);",
                                       "Returns the object `self` represented as a `double`."},
                                      {NULL, NULL, NULL}};

    return methods;
}

var C_Float = MetaDotC(C_Float, Instance(Doc, C_Float_Name, C_Float_Brief, C_Float_Description,
                                         C_Float_Definition, C_Float_Examples, C_Float_Methods));

int64_t c_int(var self) {

    if (type_of(self) is Int) { return ((struct Int *) self)->val; }

    return method(self, C_Int, c_int);
}

double c_float(var self) {

    if (type_of(self) is Float) { return ((struct Float *) self)->val; }

    return method(self, C_Float, c_float);
}

static const char *Int_Name(void) { return "Int"; }

static const char *Int_Brief(void) { return "Integer Object"; }

static const char *Int_Description(void) { return "64-bit signed integer Object."; }

static const char *Int_Definition(void) {
    return "struct Int {\n"
           "  int64_t val;\n"
           "};\n";
}

static struct Example *Int_Examples(void) {

    static struct Example examples[] = {{"Usage", "var i0 = $(Int, 1);\n"
                                                  "var i1 = new(Int, $I(24313));\n"
                                                  "var i2 = copy(i0);\n"
                                                  "\n"
                                                  "show(i0); /*     1 */\n"
                                                  "show(i1); /* 24313 */\n"
                                                  "show(i2); /*     1 */\n"},
                                        {NULL, NULL}};

    return examples;
}

static void Int_Assign(var self, var obj) {
    struct Int *i = self;
    i->val = c_int(obj);
}

static int64_t Int_C_Int(var self) {
    struct Int *i = self;
    return i->val;
}

static int Int_Cmp(var self, var obj) { return (int) (Int_C_Int(self) - c_int(obj)); }

static uint64_t Int_Hash(var self) { return (uint64_t) c_int(self); }

static int Int_Show(var self, var output, int pos) { return print_to(output, pos, "%li", self); }

static int Int_Look(var self, var input, int pos) { return scan_from(input, pos, "%li", self); }

var Int = MetaDotC(
        Int,
        Instance(Doc, Int_Name, Int_Brief, Int_Description, Int_Definition, Int_Examples, NULL),
        Instance(Assign, Int_Assign), Instance(Cmp, Int_Cmp), Instance(Hash, Int_Hash),
        Instance(C_Int, Int_C_Int), Instance(Show, Int_Show, Int_Look));

static const char *Float_Name(void) { return "Float"; }

static const char *Float_Brief(void) { return "Floating Point Object"; }

static const char *Float_Description(void) { return "64-bit double precision float point Object."; }

static const char *Float_Definition(void) {
    return "struct Float {\n"
           "  double val;\n"
           "};\n";
}

static struct Example *Float_Examples(void) {

    static struct Example examples[] = {{"Usage", "var f0 = $(Float, 1.0);\n"
                                                  "var f1 = new(Float, $F(24.313));\n"
                                                  "var f2 = copy(f0);\n"
                                                  "\n"
                                                  "show(f0); /*  1.000 */\n"
                                                  "show(f1); /* 24.313 */\n"
                                                  "show(f2); /*  1.000 */\n"},
                                        {NULL, NULL}};

    return examples;
}

static void Float_Assign(var self, var obj) {
    struct Float *f = self;
    f->val = c_float(obj);
}

static double Float_C_Float(var self) {
    struct Float *f = self;
    return f->val;
}

static int Float_Cmp(var self, var obj) {
    double c = Float_C_Float(self) - c_float(obj);
    return c > 0 ? 1 : c < 0 ? -1 : 0;
}

union interp_cast {
    double as_flt;
    uint64_t as_int;
};

static uint64_t Float_Hash(var self) {
    union interp_cast ic;
    ic.as_flt = c_float(self);
    return ic.as_int;
}

int Float_Show(var self, var output, int pos) { return print_to(output, pos, "%f", self); }

int Float_Look(var self, var input, int pos) { return scan_from(input, pos, "%f", self); }

var Float = MetaDotC(Float,
                     Instance(Doc, Float_Name, Float_Brief, Float_Description, Float_Definition,
                              Float_Examples, NULL),
                     Instance(Assign, Float_Assign), Instance(Cmp, Float_Cmp),
                     Instance(Hash, Float_Hash), Instance(C_Float, Float_C_Float),
                     Instance(Show, Float_Show, Float_Look));

static const char *Pointer_Name(void) { return "Pointer"; }

static const char *Pointer_Brief(void) { return "Reference to other object"; }

static const char *Pointer_Description(void) {
    return "The `Pointer` class is implemented by types which act as references to "
           "other objects. Primarily this class is implemented by `Ref` and `Box` "
           "which provide the two main pointer types in MetaDotC.";
}

static const char *Pointer_Definition(void) {
    return "struct Pointer {\n"
           "  void (*ref)(var, var);\n"
           "  var (*deref)(var);\n"
           "};\n";
}

static struct Example *Pointer_Examples(void) {

    static struct Example examples[] = {{"Usage", "var obj0 = $F(1.0), obj1 = $F(2.0);\n"
                                                  "var r = $(Ref, obj0);\n"
                                                  "show(r);\n"
                                                  "show(deref(r)); /* 1.0 */\n"
                                                  "ref(r, obj1);\n"
                                                  "show(deref(r)); /* 2.0 */\n"
                                                  "assign(r, obj0);\n"
                                                  "show(deref(r)); /* 1.0 */\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Pointer_Methods(void) {

    static struct Method methods[] = {
            {"ref", "void ref(var self, var item);",
             "Set the object `self` to reference the object `item`."},
            {"deref", "var deref(var self);", "Get the object referenced by `self`."},
            {NULL, NULL, NULL}};

    return methods;
}

var Pointer = MetaDotC(Pointer, Instance(Doc, Pointer_Name, Pointer_Brief, Pointer_Description,
                                         Pointer_Definition, Pointer_Examples, Pointer_Methods));

void ref(var self, var item) { method(self, Pointer, ref, item); }

var deref(var self) { return method(self, Pointer, deref); }

static const char *Ref_Name(void) { return "Ref"; }

static const char *Ref_Brief(void) { return "Shared Pointer"; }

static const char *Ref_Description(void) {
    return "The `Ref` type is a basic wrapper around a C pointer. It can be used "
           "as a type argument to collections to allow them to store generic types. "
           "It may also be useful in various circumstances where another level of "
           "indirection or mutability is required.";
}

static const char *Ref_Definition(void) {
    return "struct Ref {\n"
           "  var val;\n"
           "};\n";
}

static struct Example *Ref_Examples(void) {

    static struct Example examples[] = {{"Usage", "var obj0 = $F(1.0), obj1 = $F(2.0);\n"
                                                  "var r = $(Ref, obj0);\n"
                                                  "show(r);\n"
                                                  "show(deref(r)); /* 1.0 */\n"
                                                  "ref(r, obj1);\n"
                                                  "show(deref(r)); /* 2.0 */\n"
                                                  "assign(r, obj0);\n"
                                                  "show(deref(r)); /* 1.0 */\n"},
                                        {"Collections",
                                         "var i0 = new(Int, $I(100));\n"
                                         "var i1 = new(Int, $I(200));\n"
                                         "var x = new(Array, Ref, i0, i1);\n"
                                         "\n"
                                         "print(deref(get(x, $I(0)))); /* 100 */"
                                         "\n"
                                         "del(x); /* Contents of `x` still alive */\n"},
                                        {NULL, NULL}};

    return examples;
}

static void Ref_Ref(var self, var val);
static var Ref_Deref(var self);
static void Ref_Assign(var self, var obj);

static void Ref_Assign(var self, var obj) {
    struct Pointer *p = instance(obj, Pointer);
    if (p and p->deref) {
        Ref_Ref(self, p->deref(obj));
    } else {
        Ref_Ref(self, obj);
    }
}

static void Ref_Ref(var self, var val) {
    struct Ref *r = self;
    r->val = val;
}

static var Ref_Deref(var self) {
    struct Ref *r = self;
    return r->val;
}

var Ref = MetaDotC(
        Ref,
        Instance(Doc, Ref_Name, Ref_Brief, Ref_Description, Ref_Definition, Ref_Examples, NULL),
        Instance(Assign, Ref_Assign), Instance(Pointer, Ref_Ref, Ref_Deref));

static const char *Box_Name(void) { return "Box"; }

static const char *Box_Brief(void) { return "Unique Pointer"; }

static const char *Box_Description(void) {
    return "The `Box` type is another wrapper around a C pointer with one additional "
           "behaviour as compared to `Ref`. When a `Box` object is deleted it will "
           "also call `del` on the object it points to. The means a `Box` is "
           "considered a pointer type that _owns_ the object it points to, and so is "
           "responsible for it's destruction. Due to this `Box`s must point to valid "
           "MetaDotC objects and so can't be initalised with `NULL` or anything else "
           "invalid. "
           "\n\n"
           "While this might not seem that useful when there is Garbage Collection "
           "this can be very useful when Garbage Collection is turned off, and when "
           "used in conjunction with collections.";
}

static const char *Box_Definition(void) {
    return "struct Box {\n"
           "  var val;\n"
           "};\n";
}

static struct Example *Box_Examples(void) {

    static struct Example examples[] = {
            {"Usage", "var obj0 = $F(1.0), obj1 = $F(2.0);\n"
                      "var r = $(Box, obj0);\n"
                      "show(r);\n"
                      "show(deref(r)); /* 1.0 */\n"
                      "ref(r, obj1);\n"
                      "show(deref(r)); /* 2.0 */\n"
                      "assign(r, obj0);\n"
                      "show(deref(r)); /* 1.0 */\n"},
            {"Lifetimes", "var quote = $S(\"Life is long\");\n"
                          "\n"
                          "with (r in $B(new(String, quote))) {\n"
                          "  println(\"This reference is: %$\", r);\n"
                          "  println(\"This string is alive: '%s'\", deref(r));\n"
                          "}\n"
                          "\n"
                          "print(\"Now it has been cleared up!\\n\");\n"},
            {"Collection", "/* Multiple Types in one Collection */\n"
                           "var x = new(Array, Box, \n"
                           "  new(String, $S(\"Hello\")), \n"
                           "  new(String, $S(\"There\")), \n"
                           "  new(Int, $I(10)));\n"
                           "\n"
                           "print(deref(get(x, $I(0)))); /* Hello */ \n"
                           "\n"
                           "del(x); /* Contents of `x` deleted with it */\n"},
            {NULL, NULL}};

    return examples;
}

static void Box_Ref(var self, var val);
static var Box_Deref(var self);
static void Box_Assign(var self, var obj);

static void Box_New(var self, var args) { Box_Assign(self, get(args, $I(0))); }

static void Box_Del(var self) {
    var obj = Box_Deref(self);
    if (obj) { del(obj); }
    Box_Ref(self, NULL);
}

static void Box_Assign(var self, var obj) {
    struct Pointer *p = instance(obj, Pointer);
    if (p and p->deref) {
        Box_Ref(self, p->deref(obj));
    } else {
        Box_Ref(self, obj);
    }
}

static int Box_Show(var self, var output, int pos) {
    return print_to(output, pos, "<'Box' at 0x%p (%$)>", self, Box_Deref(self));
}

static void Box_Ref(var self, var val) {
    struct Box *b = self;
    b->val = val;
}

static var Box_Deref(var self) {
    struct Box *b = self;
    return b->val;
}

var Box = MetaDotC(
        Box,
        Instance(Doc, Box_Name, Box_Brief, Box_Description, Box_Definition, Box_Examples, NULL),
        Instance(New, Box_New, Box_Del), Instance(Assign, Box_Assign),
        Instance(Show, Box_Show, NULL), Instance(Pointer, Box_Ref, Box_Deref));

static const char *Push_Name(void) { return "Push"; }

static const char *Push_Brief(void) { return "Pushable and Popable object"; }

static const char *Push_Description(void) {
    return ""
           "The `Push` class provides an interface for the addition and removal of "
           "objects from another in a positional sense."
           "\n\n"
           "`push` can be used to add new objects to a collection and `pop` to remove "
           "them. Usage of `push` can require `assign` to be defined on the argument.";
}

static const char *Push_Definition(void) {
    return "struct Push {\n"
           "  void (*push)(var, var);\n"
           "  void (*pop)(var);\n"
           "  void (*push_at)(var, var, var);\n"
           "  void (*pop_at)(var, var);\n"
           "};\n";
}

static struct Example *Push_Examples(void) {

    static struct Example examples[] = {{"Usage", "var x = new(Array, Int);\n"
                                                  "\n"
                                                  "push(x, $I( 0));\n"
                                                  "push(x, $I( 5));\n"
                                                  "push(x, $I(10));\n"
                                                  "\n"
                                                  "show(get(x, $I(0))); /*  0 */\n"
                                                  "show(get(x, $I(1))); /*  5 */\n"
                                                  "show(get(x, $I(2))); /* 10 */\n"
                                                  "\n"
                                                  "pop_at(x, $I(1));\n"
                                                  "\n"
                                                  "show(get(x, $I(0))); /*  0 */\n"
                                                  "show(get(x, $I(1))); /* 10 */\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Push_Methods(void) {

    static struct Method methods[] = {
            {"push", "void push(var self, var obj);",
             "Push the object `obj` onto the top of object `self`."},
            {"pop", "void pop(var self);", "Pop the top item from the object `self`."},
            {"push_at", "void push_at(var self, var obj, var key);",
             "Push the object `obj` onto the object `self` at a given `key`."},
            {"pop_at", "void pop_at(var self, var key);",
             "Pop the object from the object `self` at a given `key`."},
            {NULL, NULL, NULL}};

    return methods;
}

var Push = MetaDotC(Push, Instance(Doc, Push_Name, Push_Brief, Push_Description, Push_Definition,
                                   Push_Examples, Push_Methods));

void push(var self, var val) { method(self, Push, push, val); }
void push_at(var self, var val, var i) { method(self, Push, push_at, val, i); }
void pop(var self) { method(self, Push, pop); }
void pop_at(var self, var i) { method(self, Push, pop_at, i); }

static const char *Resize_Name(void) { return "Reserve"; }

static const char *Resize_Brief(void) { return "Object can be resized"; }

static const char *Resize_Description(void) {
    return "The `Resize` class can be implemented by objects which can be resized in "
           "some way. Resizing to a larger size than the current may allow for some "
           "resource or other to be preallocated or reserved. For example this class "
           "is implemented by `Array` and `Table` to either remove a number of items "
           "quickly or to preallocate memory space if it is known that many items are "
           "going to be added at a later date.";
}

static const char *Resize_Definition(void) {
    return "struct Resize {\n"
           "  void (*resize)(var, size_t);\n"
           "};\n";
}

static struct Method *Resize_Methods(void) {

    static struct Method methods[] = {
            {"resize", "void resize(var self, size_t n);",
             "Resize to some size `n`, perhaps reserving some resource for object "
             "`self`."},
            {NULL, NULL, NULL}};

    return methods;
}

static struct Example *Resize_Examples(void) {

    static struct Example examples[] = {
            {"Usage", "var x = new(Array, Int);\n"
                      "resize(x, 10000); /* Reserve space in Array */ \n"
                      "for (size_t i = 0; i < 10000; i++) {\n"
                      "  push(x, $I(i));\n"
                      "}\n"},
            {"Usage 2", "var x = new(Array, Int, $I(0), $I(1), $I(2));\n"
                        "resize(x, 0); /* Clear Array of items */\n"},
            {NULL, NULL}};

    return examples;
}

var Resize = MetaDotC(Resize, Instance(Doc, Resize_Name, Resize_Brief, Resize_Description,
                                       Resize_Definition, Resize_Examples, Resize_Methods));

void resize(var self, size_t n) { method(self, Resize, resize, n); }

static const char *Format_Name(void) { return "Format"; }

static const char *Format_Brief(void) { return "Read or Write with Format String"; }

static const char *Format_Description(void) {
    return "Format abstracts the class of operations such as `scanf`, `sprintf` and "
           "`fprintf` with matching semantics. It provides general `printf` and "
           "`scanf` functionality for several different types objects in a "
           "uniform way. This class is essentially an in-between class, used by the "
           "`Show` class to read and write output."
           "\n\n"
           "It is important to note that the semantics of these operations match "
           "`printf` and not the newly defined `Show` class. For example it is "
           "perfectly valid to pass a C `int` to these functions, while the `println` "
           "function from `Show` must be passed only `var` objects.";
}

static const char *Format_Definition(void) {
    return "struct Format {\n"
           "  int (*format_to)(var,int,const char*,va_list);\n"
           "  int (*format_from)(var,int,const char*,va_list);\n"
           "};\n";
}

static struct Example *Format_Examples(void) {

    static struct Example examples[] = {
            {"Usage", "/* printf(\"Hello my name is %s, I'm %i\\n\", \"Dan\", 23); */\n"
                      "format_to($(File, stdout), 0, \n"
                      "  \"Hello my name is %s, I'm %i\\n\", \"Dan\", 23);\n"},
            {NULL, NULL}};

    return examples;
}

static struct Method *Format_Methods(void) {

    static struct Method methods[] = {
            {"format_to",
             "int format_to(var self, int pos, const char* fmt, ...);\n"
             "int format_to_va(var self, int pos, const char* fmt, va_list va);",
             "Write a formatted string `fmt` to the object `self` at position `pos`."},
            {"format_from",
             "int format_from(var self, int pos, const char* fmt, ...);\n"
             "int format_from_va(var self, int pos, const char* fmt, va_list va);",
             "Read a formatted string `fmt` from the object `self` at position `pos`."},
            {NULL, NULL, NULL}};

    return methods;
}

var Format = MetaDotC(Format, Instance(Doc, Format_Name, Format_Brief, Format_Description,
                                       Format_Definition, Format_Examples, Format_Methods));

int format_to_va(var self, int pos, const char *fmt, va_list va) {
    return method(self, Format, format_to, pos, fmt, va);
}

int format_from_va(var self, int pos, const char *fmt, va_list va) {
    return method(self, Format, format_from, pos, fmt, va);
}

int format_to(var self, int pos, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    int ret = format_to_va(self, pos, fmt, va);
    va_end(va);
    return ret;
}

int format_from(var self, int pos, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    int ret = format_from_va(self, pos, fmt, va);
    va_end(va);
    return ret;
}

static const char *Show_Name(void) { return "Show"; }

static const char *Show_Brief(void) { return "Convert To or From String"; }

static const char *Show_Description(void) {
    return "The `Show` class is used to convert objects to, and from, a `String` "
           "representation. Objects which implement `Show` should expect the "
           "input/output object to be one that support the `Format` class, such as "
           "`File` or `String`."
           "\n\n"
           "The `print`, `println` and `print_to` functions provide a mechanism for "
           "writing formatted strings with MetaDotC objects. To do this they provide a "
           "new format specifier `%$` which uses an object's `Show` functionality to "
           "write that part of the string. All objects which don't support `Show` can "
           "still be shown via a default implementation."
           "\n\n"
           "All the Show methods which are variable arguments only take `var` objects "
           "as input. To print native C types wrap them in MetaDotC types using `$`."
           "\n\n"
           "Standard format specifiers such as `%f` and `%d` will call functions such "
           "as `c_float` and `c_int` on their passed arguments to convert objects to "
           "C types before performing the standard C formatting behaviour."
           "\n\n"
           "See [printf](http://www.cplusplus.com/reference/cstdio/printf/) for more "
           "information on format specifiers.";
}

static const char *Show_Definition(void) {
    return "struct Show {\n"
           "  int (*show)(var, var, int);\n"
           "  int (*look)(var, var, int);\n"
           "};\n";
}

static struct Example *Show_Examples(void) {

    static struct Example examples[] = {
            {"Hello World", "println(\"Hello %s!\", $S(\"World\"));\n"},
            {"File Writing", "with (f in new(File, $S(\"prices.txt\"), $S(\"wb\"))) {\n"
                             "  print_to(f, 0, \"%$ :: %$\\n\", $S(\"Banana\"), $I(57));\n"
                             "  print_to(f, 0, \"%$ :: %$\\n\", $S(\"Apple\"),  $I(22));\n"
                             "  print_to(f, 0, \"%$ :: %$\\n\", $S(\"Pear\"),   $I(16));\n"
                             "}\n"},
            {"String Scanning", "var input = $S(\"1 and 52 then 78\");\n"
                                "\n"
                                "var i0 = $I(0), i1 = $I(0), i2 = $I(0);\n"
                                "scan_from(input, 0, \"%i and %i then %i\", i0, i1, i2);\n"
                                "\n"
                                "/* i0: 1, i1: 52, i2: 78 */\n"
                                "println(\"i0: %$, i1: %$, i2: %$\", i0, i1, i2);\n"},
            {"String Printing", "var greeting = new(String);\n"
                                "print_to(greeting, 0, \"Hello %s %s, %s?\", \n"
                                "  $S(\"Mr\"), $S(\"Johnson\"), $S(\"how are you?\"));\n"
                                "\n"
                                "/* Hello Mr Johnson, how are you? */\n"
                                "show(greeting);\n"},
            {NULL, NULL}};

    return examples;
}

static struct Method *Show_Methods(void) {

    static struct Method methods[] = {
            {"show",
             "int show(var self);\n"
             "int show_to(var self, var out, int pos);",
             "Show the object `self` either to `stdout` or to the object `output`."},
            {"look",
             "int look(var self);\n"
             "int look_from(var self, var input, int pos);",
             "Read the object `self` either from `stdout` or from the object `input`."},
            {"print",
             "#define print(fmt, ...)\n"
             "#define println(fmt, ...)\n"
             "#define print_to(out, pos, fmt, ...)\n"
             "int print_with(const char* fmt, var args);\n"
             "int println_with(const char* fmt, var args);\n"
             "int print_to_with(var out, int pos, const char* fmt, var args);",
             "Print the format string `fmt` either to `stdout` or to the object `out` "
             "at positions `pos`. Returns new position in output."},
            {"scan",
             "#define scan(fmt, ...)\n"
             "#define scanln(fmt, ...)\n"
             "#define scan_from(input, pos, fmt, ...)\n"
             "int scan_with(const char* fmt, var args);\n"
             "int scanln_with(const char* fmt, var args);\n"
             "int scan_from_with(var input, int pos, const char* fmt, var args);",
             "Scan the format string `fmt` either from `stdin` or from the object "
             "`input` at position `pos`. Returns new position in output."},
            {NULL, NULL, NULL}};

    return methods;
}

var Show = MetaDotC(Show, Instance(Doc, Show_Name, Show_Brief, Show_Description, Show_Definition,
                                   Show_Examples, Show_Methods));

int show(var self) { return show_to(self, $(File, stdout), 0); }

int show_to(var self, var out, int pos) {

    struct Show *s = instance(self, Show);
    if (s and s->show) { return s->show(self, out, pos); }

    return print_to(out, pos, "<'%s' At 0x%p>", type_of(self), self);
}

int print_with(const char *fmt, var args) { return print_to_with($(File, stdout), 0, fmt, args); }

int println_with(const char *fmt, var args) {
    int pos = 0;
    pos = print_to_with($(File, stdout), pos, fmt, args);
    pos = print_to($(File, stdout), pos, "\n", "");
    return pos;
}

int print_to_with(var out, int pos, const char *fmt, var args) {

    char *fmt_buf = malloc(strlen(fmt) + 1);
    size_t index = 0;

    while (true) {

        if (*fmt is '\0') { break; }

        const char *start = fmt;

        /* Match String */
        while (*fmt isnt '\0' and *fmt isnt '%') { fmt++; }

        if (start isnt fmt) {
            memcpy(fmt_buf, start, fmt - start);
            fmt_buf[fmt - start] = '\0';
            int off = format_to(out, pos, fmt_buf);
            if (off < 0) { throw(FormatError, "Unable to output format!", ""); }
            pos += off;
            continue;
        }

        /* Match %% */
        if (*fmt is '%' && *(fmt + 1) is '%') {
            int off = format_to(out, pos, "%%");
            if (off < 0) { throw(FormatError, "Unable to output '%%%%'!", ""); }
            pos += off;
            fmt += 2;
            continue;
        }

        /* Match Format Specifier */
        while (not strchr("diuoxXfFeEgGaAxcsp$", *fmt)) { fmt++; }

        if (start isnt fmt) {

            memcpy(fmt_buf, start, fmt - start + 1);
            fmt_buf[fmt - start + 1] = '\0';

            if (index >= len(args)) {
                throw(FormatError, "Not enough arguments to Format String!", "");
            }

            var a = get(args, $I(index));
            index++;

            if (*fmt is '$') { pos = show_to(a, out, pos); }

            if (*fmt is 's') {
                int off = format_to(out, pos, fmt_buf, c_str(a));
                if (off < 0) { throw(FormatError, "Unable to output String!", ""); }
                pos += off;
            }

            if (strchr("diouxX", *fmt)) {
                int off = format_to(out, pos, fmt_buf, c_int(a));
                if (off < 0) { throw(FormatError, "Unable to output Int!", ""); }
                pos += off;
            }

            if (strchr("fFeEgGaA", *fmt)) {
                int off = format_to(out, pos, fmt_buf, c_float(a));
                if (off < 0) { throw(FormatError, "Unable to output Real!", ""); }
                pos += off;
            }

            if (*fmt is 'c') {
                int off = format_to(out, pos, fmt_buf, c_int(a));
                if (off < 0) { throw(FormatError, "Unable to output Char!", ""); }
                pos += off;
            }

            if (*fmt is 'p') {
                int off = format_to(out, pos, fmt_buf, a);
                if (off < 0) { throw(FormatError, "Unable to output Object!", ""); }
                pos += off;
            }

            fmt++;
            continue;
        }

        throw(FormatError, "Invalid Format String!", "");
    }

    free(fmt_buf);

    return pos;
}

int look(var self) { return look_from(self, $(File, stdin), 0); }

int look_from(var self, var input, int pos) { return method(self, Show, look, input, pos); }

int scan_with(const char *fmt, var args) { return scan_from_with($(File, stdin), 0, fmt, args); }

int scanln_with(const char *fmt, var args) {
    int pos = 0;
    pos = scan_from_with($(File, stdin), pos, fmt, args);
    pos = scan_from($(File, stdin), pos, "\n", "");
    return pos;
}

int scan_from_with(var input, int pos, const char *fmt, var args) {

    char *fmt_buf = malloc(strlen(fmt) + 4);
    size_t index = 0;

    while (true) {

        if (*fmt is '\0') { break; }

        const char *start = fmt;

        /* Match String */
        while (*fmt isnt '\0' and *fmt isnt '%') { fmt++; }

        if (start isnt fmt) {
            memcpy(fmt_buf, start, fmt - start);
            fmt_buf[fmt - start] = '\0';
            format_from(input, pos, fmt_buf);
            pos += (int) (fmt - start);
            continue;
        }

        /* Match %% */
        if (*fmt is '%' and *(fmt + 1) is '%') {
            int err = format_from(input, pos, "%%");
            if (err < 0) { throw(FormatError, "Unable to input '%%%%'!", ""); }
            pos += 2;
            fmt += 2;
            continue;
        }

        /* Match Format Specifier */
        while (not strchr("diuoxXfFeEgGaAxcsp$[^]", *fmt)) { fmt++; }

        if (start isnt fmt) {

            int off = 0;
            memcpy(fmt_buf, start, fmt - start + 1);
            fmt_buf[fmt - start + 1] = '\0';
            strcat(fmt_buf, "%n");

            if (index >= len(args)) {
                throw(FormatError, "Not enough arguments to Format String!", "");
            }

            var a = get(args, $I(index));
            index++;

            if (*fmt is '$') {
                pos = look_from(a, input, pos);
            }

            else if (*fmt is 's') {
                int err = format_from(input, pos, fmt_buf, c_str(a), &off);
                if (err < 1) { throw(FormatError, "Unable to input String!", ""); }
                pos += off;
            }

            /* TODO: Test */
            else if (*fmt is ']') {
                int err = format_from(input, pos, fmt_buf, c_str(a), &off);
                if (err < 1) { throw(FormatError, "Unable to input Scanset!", ""); }
                pos += off;
            }

            else if (strchr("diouxX", *fmt)) {
                long tmp = 0;
                int err = format_from(input, pos, fmt_buf, &tmp, &off);
                if (err < 1) { throw(FormatError, "Unable to input Int!", ""); }
                pos += off;
                assign(a, $I(tmp));
            }

            else if (strchr("fFeEgGaA", *fmt)) {
                if (strchr(fmt_buf, 'l')) {
                    double tmp = 0;
                    int err = format_from(input, pos, fmt_buf, &tmp, &off);
                    if (err < 1) { throw(FormatError, "Unable to input Float!", ""); }
                    pos += off;
                    assign(a, $F(tmp));
                } else {
                    float tmp = 0;
                    int err = format_from(input, pos, fmt_buf, &tmp, &off);
                    if (err < 1) { throw(FormatError, "Unable to input Float!", ""); }
                    pos += off;
                    assign(a, $F(tmp));
                }
            }

            else if (*fmt is 'c') {
                char tmp = '\0';
                int err = format_from(input, pos, fmt_buf, &tmp, &off);
                if (err < 1) { throw(FormatError, "Unable to input Char!", ""); }
                pos += off;
                assign(a, $I(tmp));
            }

            else if (*fmt is 'p') {
                void *tmp = NULL;
                int err = format_from(input, pos, fmt_buf, &tmp, &off);
                if (err < 1) { throw(FormatError, "Unable to input Ref!", ""); }
                pos += off;
                assign(a, $R(tmp));
            }

            else {
                /* TODO: Report Better */
                throw(FormatError, "Invalid Format Specifier!", "");
            }

            fmt++;
            continue;
        }
    }

    free(fmt_buf);

    return pos;
}

static const char *Start_Name(void) { return "Start"; }

static const char *Start_Brief(void) { return "Can be started or stopped"; }

static const char *Start_Description(void) {
    return "The `Start` class can be implemented by types which provide an abstract "
           "notion of a started and stopped state. This can be real processes such "
           "as `Thread`, or something like `File` where the on/off correspond to "
           "if the file is open or not."
           "\n\n"
           "The main nicety of the `Start` class is that it allows use of the `with` "
           "macro which performs the `start` function at the opening of a scope block "
           "and the `stop` function at the end.";
}

static const char *Start_Definition(void) {
    return "struct Start {\n"
           "  void (*start)(var);\n"
           "  void (*stop)(var);\n"
           "  void (*join)(var);\n"
           "  bool (*running)(var);\n"
           "};\n";
}

static struct Example *Start_Examples(void) {

    static struct Example examples[] = {{"Usage", "var x = new(Mutex);\n"
                                                  "start(x); /* Lock Mutex */ \n"
                                                  "print(\"Inside Mutex!\\n\");\n"
                                                  "stop(x); /* unlock Mutex */"},
                                        {"Scoped", "var x = new(Mutex);\n"
                                                   "with (mut in x) { /* Lock Mutex */ \n"
                                                   "  print(\"Inside Mutex!\\n\");\n"
                                                   "} /* unlock Mutex */"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Start_Methods(void) {

    static struct Method methods[] = {
            {"with", "#define with(...)", "Perform operations in between `start` and `stop`."},
            {"start", "void start(var self);", "Start the object `self`."},
            {"stop", "void stop(var self);", "Stop the object `self`."},
            {"join", "void join(var self);", "Block and wait for the object `self` to stop."},
            {"running", "bool running(var self);", "Check if the object `self` is running."},
            {NULL, NULL, NULL}};

    return methods;
}

var Start = MetaDotC(Start, Instance(Doc, Start_Name, Start_Brief, Start_Description,
                                     Start_Definition, Start_Examples, Start_Methods));

void start(var self) { method(self, Start, start); }

void stop(var self) { method(self, Start, stop); }

void join(var self) { method(self, Start, join); }

bool running(var self) { return method(self, Start, running); }

var start_in(var self) {
    struct Start *s = instance(self, Start);
    if (s and s->start) { s->start(self); }
    return self;
}

var stop_in(var self) {
    struct Start *s = instance(self, Start);
    if (s and s->stop) { s->stop(self); }
    return NULL;
}

static const char *C_Str_Name(void) { return "C_Str"; }

static const char *C_Str_Brief(void) { return "Interpret as C String"; }

static const char *C_Str_Description(void) {
    return "The `C_Str` class should be overridden by types which are representable "
           "as a C style String.";
}

static const char *C_Str_Definition(void) {
    return "struct C_Str {\n"
           "  char* (*c_str)(var);\n"
           "};\n";
}

static struct Example *C_Str_Examples(void) {

    static struct Example examples[] = {{"Usage", "puts(c_str($S(\"Hello\"))); /* Hello */\n"
                                                  "puts(c_str($S(\"There\"))); /* There */\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *C_Str_Methods(void) {

    static struct Method methods[] = {{"c_str", "char* c_str(var self);",
                                       "Returns the object `self` represented as a `char*`."},
                                      {NULL, NULL, NULL}};

    return methods;
}

var C_Str = MetaDotC(C_Str, Instance(Doc, C_Str_Name, C_Str_Brief, C_Str_Description,
                                     C_Str_Definition, C_Str_Examples, C_Str_Methods));

char *c_str(var self) {

    if (type_of(self) is String) { return ((struct String *) self)->val; }

    return method(self, C_Str, c_str);
}

static const char *String_Name(void) { return "String"; }

static const char *String_Brief(void) { return "String Object"; }

static const char *String_Description(void) {
    return "The `String` type is a wrapper around the native C string type. This "
           "includes strings that are allocated on either the Stack or the Heap."
           "\n\n"
           "For strings allocated on the heap a number of extra operations are "
           "provided overs standard C strings such as concatenation.";
}

static const char *String_Definition(void) {
    return "struct String {\n"
           "  char* val;\n"
           "};\n";
}

static struct Example *String_Examples(void) {

    static struct Example examples[] = {{"Usage", "var s0 = $(String, \"Hello\");\n"
                                                  "var s1 = new(String, $S(\"Hello\"));\n"
                                                  "append(s1, $S(\" There\"));\n"
                                                  "show(s0); /* Hello */\n"
                                                  "show(s1); /* Hello There */\n"},
                                        {"Manipulation",
                                         "var s0 = new(String, $S(\"Balloons\"));\n"
                                         "\n"
                                         "show($I(len(s0))); /* 8 */\n"
                                         "show($I(mem(s0, $S(\"Ball\"))));     /* 1 */\n"
                                         "show($I(mem(s0, $S(\"oon\"))));      /* 1 */\n"
                                         "show($I(mem(s0, $S(\"Balloons\")))); /* 1 */\n"
                                         "show($I(mem(s0, $S(\"l\"))));        /* 1 */\n"
                                         "\n"
                                         "rem(s0, $S(\"oons\"));\n"
                                         "\n"
                                         "show($I(eq(s0, $S(\"Ball\")))); /* 1 */\n"
                                         "\n"
                                         "resize(s0, 0);\n"
                                         "\n"
                                         "show($I(len(s0))); /* 0 */\n"
                                         "show($I(eq(s0, $S(\"\")))); /* 1 */\n"},
                                        {NULL, NULL}};

    return examples;
}

static void String_Assign(var self, var obj);

static void String_New(var self, var args) {
    struct String *s = self;
    if (len(args) > 0) {
        String_Assign(self, get(args, $I(0)));
    } else {
        s->val = calloc(1, 1);
    }

#if METADOT_C_MEMORY_CHECK == 1
    if (s->val is NULL) { throw(OutOfMemoryError, "Cannot allocate String, out of memory!", ""); }
#endif
}

static void String_Del(var self) {
    struct String *s = self;

#if METADOT_C_ALLOC_CHECK == 1
    if (header(self)->alloc is(var) AllocStack or header(self)->alloc is(var) AllocStatic) {
        throw(ValueError, "Cannot destruct String, not on heap!", "");
    }
#endif

    free(s->val);
}

static void String_Assign(var self, var obj) {
    struct String *s = self;
    char *val = c_str(obj);

#if METADOT_C_ALLOC_CHECK == 1
    if (header(self)->alloc is(var) AllocStack or header(self)->alloc is(var) AllocStatic) {
        throw(ValueError, "Cannot reallocate String, not on heap!", "");
    }
#endif

    s->val = realloc(s->val, strlen(val) + 1);

#if METADOT_C_MEMORY_CHECK == 1
    if (s->val is NULL) { throw(OutOfMemoryError, "Cannot allocate String, out of memory!", ""); }
#endif

    strcpy(s->val, val);
}

static char *String_C_Str(var self) {
    struct String *s = self;
    return s->val;
}

static int String_Cmp(var self, var obj) { return strcmp(String_C_Str(self), c_str(obj)); }

static size_t String_Len(var self) {
    struct String *s = self;
    return strlen(s->val);
}

static void String_Clear(var self) {
    struct String *s = self;

#if METADOT_C_ALLOC_CHECK == 1
    if (header(self)->alloc is(var) AllocStack or header(self)->alloc is(var) AllocStatic) {
        throw(ValueError, "Cannot reallocate String, not on heap!", "");
    }
#endif

    s->val = realloc(s->val, 1);

#if METADOT_C_MEMORY_CHECK == 1
    if (s->val is NULL) { throw(OutOfMemoryError, "Cannot allocate String, out of memory!", ""); }
#endif

    s->val[0] = '\0';
}

static bool String_Mem(var self, var obj) {

    struct C_Str *c = instance(obj, C_Str);
    if (c and c->c_str) { return strstr(String_C_Str(self), c->c_str(obj)); }

    return false;
}

static void String_Rem(var self, var obj) {

    struct C_Str *c = instance(obj, C_Str);
    if (c and c->c_str) {
        char *pos = strstr(String_C_Str(self), c->c_str(obj));
        size_t count = strlen(String_C_Str(self)) - strlen(pos) - strlen(c->c_str(obj)) + 1;
        memmove((char *) pos, pos + strlen(c->c_str(obj)), count);
    }
}

static uint64_t String_Hash(var self) {
    struct String *s = self;
    return hash_data(s->val, strlen(s->val));
}

static void String_Concat(var self, var obj) {
    struct String *s = self;

#if METADOT_C_ALLOC_CHECK == 1
    if (header(self)->alloc is(var) AllocStack or header(self)->alloc is(var) AllocStatic) {
        throw(ValueError, "Cannot reallocate String, not on heap!", "");
    }
#endif

    s->val = realloc(s->val, strlen(s->val) + strlen(c_str(obj)) + 1);

#if METADOT_C_MEMORY_CHECK == 1
    if (s->val is NULL) { throw(OutOfMemoryError, "Cannot allocate String, out of memory!", ""); }
#endif

    strcat(s->val, c_str(obj));
}

static void String_Resize(var self, size_t n) {
    struct String *s = self;

#if METADOT_C_ALLOC_CHECK == 1
    if (header(self)->alloc is(var) AllocStack or header(self)->alloc is(var) AllocStatic) {
        throw(ValueError, "Cannot reallocate String, not on heap!", "");
    }
#endif

    size_t m = String_Len(self);
    s->val = realloc(s->val, n + 1);

    if (n > m) {
        memset(&s->val[m], 0, n - m);
    } else {
        s->val[n] = '\0';
    }

#if METADOT_C_MEMORY_CHECK == 1
    if (s->val is NULL) { throw(OutOfMemoryError, "Cannot allocate String, out of memory!", ""); }
#endif
}

static int String_Format_To(var self, int pos, const char *fmt, va_list va) {

    struct String *s = self;

#ifdef METADOT_C_WINDOWS

    va_list va_tmp;
    va_copy(va_tmp, va);
    int size = _vscprintf(fmt, va_tmp);
    va_end(va_tmp);

#if METADOT_C_ALLOC_CHECK == 1
    if (header(self)->alloc is(var) AllocStack or header(self)->alloc is(var) AllocStatic) {
        throw(ValueError, "Cannot reallocate String, not on heap!");
    }
#endif

    s->val = realloc(s->val, pos + size + 1);

#if METADOT_C_MEMORY_CHECK == 1
    if (s->val is NULL) { throw(OutOfMemoryError, "Cannot allocate String, out of memory!"); }
#endif

    return vsprintf(s->val + pos, fmt, va);

#elif defined(METADOT_C_MAC)

    va_list va_tmp;
    va_copy(va_tmp, va);
    char *tmp = NULL;
    int size = vasprintf(&tmp, fmt, va_tmp);
    va_end(va_tmp);

#if METADOT_C_ALLOC_CHECK == 1
    if (header(self)->alloc is(var) AllocStack or header(self)->alloc is(var) AllocStatic) {
        throw(ValueError, "Cannot reallocate String, not on heap!", "");
    }
#endif

    s->val = realloc(s->val, pos + size + 1);

#if METADOT_C_MEMORY_CHECK == 1
    if (s->val is NULL) { throw(OutOfMemoryError, "Cannot allocate String, out of memory!", ""); }
#endif

    s->val[pos] = '\0';
    strcat(s->val, tmp);
    free(tmp);

    return size;

#else

    va_list va_tmp;
    va_copy(va_tmp, va);
    int size = vsnprintf(NULL, 0, fmt, va_tmp);
    va_end(va_tmp);

#if METADOT_C_ALLOC_CHECK == 1
    if (header(self)->alloc is(var) AllocStack or header(self)->alloc is(var) AllocStatic) {
        throw(ValueError, "Cannot reallocate String, not on heap!");
    }
#endif

    s->val = realloc(s->val, pos + size + 1);

#if METADOT_C_MEMORY_CHECK == 1
    if (s->val is NULL) { throw(OutOfMemoryError, "Cannot allocate String, out of memory!"); }
#endif

    return vsprintf(s->val + pos, fmt, va);

#endif
}

static int String_Format_From(var self, int pos, const char *fmt, va_list va) {
    struct String *s = self;
    return vsscanf(s->val + pos, fmt, va);
}

static int String_Show(var self, var out, int pos) {
    struct String *s = self;
    pos = print_to(out, pos, "\"", self);
    char *v = s->val;
    while (*v) {
        switch (*v) {
            case '\a':
                pos = print_to(out, pos, "\\a", "");
                break;
            case '\b':
                pos = print_to(out, pos, "\\b", "");
                break;
            case '\f':
                pos = print_to(out, pos, "\\f", "");
                break;
            case '\n':
                pos = print_to(out, pos, "\\n", "");
                break;
            case '\r':
                pos = print_to(out, pos, "\\r", "");
                break;
            case '\t':
                pos = print_to(out, pos, "\\t", "");
                break;
            case '\v':
                pos = print_to(out, pos, "\\v", "");
                break;
            case '\\':
                pos = print_to(out, pos, "\\\\", "");
                break;
            case '\'':
                pos = print_to(out, pos, "\\'", "");
                break;
            case '\"':
                pos = print_to(out, pos, "\\\"", "");
                break;
            case '\?':
                pos = print_to(out, pos, "\\?", "");
                break;
            default:
                pos = print_to(out, pos, "%c", $I(*v));
        }
        v++;
    }
    return print_to(out, pos, "\"", self);
}

static int String_Look(var self, var input, int pos) {

    String_Clear(self);

    var chr = $I(0);
    pos = scan_from(input, pos, "%c", chr);

    if (c_int(chr) isnt '\"') {
        throw(FormatError, "String literal does not start with quotation marks!", "");
    }

    while (true) {

        pos = scan_from(input, pos, "%c", chr);

        if (c_int(chr) == '"') { break; }

        if (c_int(chr) == '\\') {
            pos = scan_from(input, pos, "%c", chr);
            switch (c_int(chr)) {
                case 'a':
                    String_Concat(self, $S("\a"));
                    break;
                case 'b':
                    String_Concat(self, $S("\b"));
                    break;
                case 'f':
                    String_Concat(self, $S("\f"));
                    break;
                case 'n':
                    String_Concat(self, $S("\n"));
                    break;
                case 'r':
                    String_Concat(self, $S("\r"));
                    break;
                case 't':
                    String_Concat(self, $S("\t"));
                    break;
                case 'v':
                    String_Concat(self, $S("\v"));
                    break;
                case '\\':
                    String_Concat(self, $S("\\"));
                    break;
                case '\'':
                    String_Concat(self, $S("\'"));
                    break;
                case '"':
                    String_Concat(self, $S("\""));
                    break;
                case '?':
                    String_Concat(self, $S("\?"));
                    break;
                default:
                    throw(FormatError, "Unknown Escape Sequence '\\%c'!", chr);
            }
        }

        char buffer[2];
        buffer[0] = (char) c_int(chr);
        buffer[1] = '\0';

        String_Concat(self, $S(buffer));
    }

    return pos;
}

var String =
        MetaDotC(String,
                 Instance(Doc, String_Name, String_Brief, String_Description, String_Definition,
                          String_Examples, NULL),
                 Instance(New, String_New, String_Del), Instance(Assign, String_Assign),
                 Instance(Cmp, String_Cmp), Instance(Hash, String_Hash), Instance(Len, String_Len),
                 Instance(Get, NULL, NULL, String_Mem, String_Rem), Instance(Resize, String_Resize),
                 Instance(Concat, String_Concat, String_Concat), Instance(C_Str, String_C_Str),
                 Instance(Format, String_Format_To, String_Format_From),
                 Instance(Show, String_Show, String_Look));

static const char *Table_Name(void) { return "Table"; }

static const char *Table_Brief(void) { return "Hash table"; }

static const char *Table_Description(void) {
    return "The `Table` type is a hash table data structure that maps keys to values. "
           "It uses an open-addressing robin-hood hashing scheme which requires "
           "`Hash` and `Cmp` to be defined on the key type. Keys and values are "
           "copied into the collection using the `Assign` class and intially have "
           "zero'd memory."
           "\n\n"
           "Hash tables provide `O(1)` lookup, insertion and removal can but require "
           "long pauses when the table must be _rehashed_ and all entries processed."
           "\n\n"
           "This is largely equivalent to the C++ construct "
           "[std::unordered_map](http://www.cplusplus.com/reference/unordered_map/unordered_map/)";
}

static struct Example *Table_Examples(void) {

    static struct Example examples[] = {{"Usage", "var prices = new(Table, String, Int);\n"
                                                  "set(prices, $S(\"Apple\"),  $I(12));\n"
                                                  "set(prices, $S(\"Banana\"), $I( 6));\n"
                                                  "set(prices, $S(\"Pear\"),   $I(55));\n"
                                                  "\n"
                                                  "foreach (key in prices) {\n"
                                                  "  var price = get(prices, key);\n"
                                                  "  println(\"Price of %$ is %$\", key, price);\n"
                                                  "}\n"},
                                        {"Manipulation",
                                         "var t = new(Table, String, Int);\n"
                                         "set(t, $S(\"Hello\"), $I(2));\n"
                                         "set(t, $S(\"There\"), $I(5));\n"
                                         "\n"
                                         "show($I(len(t))); /* 2 */\n"
                                         "show($I(mem(t, $S(\"Hello\")))); /* 1 */\n"
                                         "\n"
                                         "rem(t, $S(\"Hello\"));\n"
                                         "\n"
                                         "show($I(len(t))); /* 1 */\n"
                                         "show($I(mem(t, $S(\"Hello\")))); /* 0 */\n"
                                         "show($I(mem(t, $S(\"There\")))); /* 1 */\n"
                                         "\n"
                                         "resize(t, 0);\n"
                                         "\n"
                                         "show($I(len(t))); /* 0 */\n"
                                         "show($I(mem(t, $S(\"Hello\")))); /* 0 */\n"
                                         "show($I(mem(t, $S(\"There\")))); /* 0 */\n"},
                                        {NULL, NULL}};

    return examples;
}

struct Table
{
    var data;
    var ktype;
    var vtype;
    size_t ksize;
    size_t vsize;
    size_t nslots;
    size_t nitems;
    var sspace0;
    var sspace1;
};

enum {
    TABLE_PRIMES_COUNT = 24
};

static const size_t Table_Primes[TABLE_PRIMES_COUNT] = {
        0,     1,      5,      11,     23,      53,      101,     197,
        389,   683,    1259,   2417,   4733,    9371,    18617,   37097,
        74093, 148073, 296099, 592019, 1100009, 2200013, 4400021, 8800019};

static const double Table_Load_Factor = 0.9;

static size_t Table_Ideal_Size(size_t size) {
    size = (size_t) ((double) (size + 1) / Table_Load_Factor);
    for (size_t i = 0; i < TABLE_PRIMES_COUNT; i++) {
        if (Table_Primes[i] >= size) { return Table_Primes[i]; }
    }
    size_t last = Table_Primes[TABLE_PRIMES_COUNT - 1];
    for (size_t i = 0;; i++) {
        if (last * i >= size) { return last * i; }
    }
}

static size_t Table_Step(struct Table *t) {
    return sizeof(uint64_t) + sizeof(struct Header) + t->ksize + sizeof(struct Header) + t->vsize;
}

static uint64_t Table_Key_Hash(struct Table *t, uint64_t i) {
    return *(uint64_t *) ((char *) t->data + i * Table_Step(t));
}

static var Table_Key(struct Table *t, uint64_t i) {
    return (char *) t->data + i * Table_Step(t) + sizeof(uint64_t) + sizeof(struct Header);
}

static var Table_Val(struct Table *t, uint64_t i) {
    return (char *) t->data + i * Table_Step(t) + sizeof(uint64_t) + sizeof(struct Header) +
           t->ksize + sizeof(struct Header);
}

static uint64_t Table_Probe(struct Table *t, uint64_t i, uint64_t h) {
    int64_t v = i - (h - 1);
    if (v < 0) { v = t->nslots + v; }
    return v;
}

static void Table_Set(var self, var key, var val);
static void Table_Set_Move(var self, var key, var val, bool move);

static size_t Table_Size_Round(size_t s) {
    return ((s + sizeof(var) - 1) / sizeof(var)) * sizeof(var);
}

static void Table_New(var self, var args) {

    struct Table *t = self;
    t->ktype = cast(get(args, $(Int, 0)), Type);
    t->vtype = cast(get(args, $(Int, 1)), Type);
    t->ksize = Table_Size_Round(size(t->ktype));
    t->vsize = Table_Size_Round(size(t->vtype));

    size_t nargs = len(args);
    if (nargs % 2 isnt 0) {
        throw(FormatError, "Received non multiple of two argument count to Table constructor.", "");
    }

    t->nslots = Table_Ideal_Size((nargs - 2) / 2);
    t->nitems = 0;

    if (t->nslots is 0) {
        t->data = NULL;
        return;
    }

    t->data = calloc(t->nslots, Table_Step(t));
    t->sspace0 = calloc(1, Table_Step(t));
    t->sspace1 = calloc(1, Table_Step(t));

#if METADOT_C_MEMORY_CHECK == 1
    if (t->data is NULL or t->sspace0 is NULL or t->sspace1 is NULL) {
        throw(OutOfMemoryError, "Cannot allocate Table, out of memory!", "");
    }
#endif

    for (size_t i = 0; i < (nargs - 2) / 2; i++) {
        var key = get(args, $(Int, 2 + (i * 2) + 0));
        var val = get(args, $(Int, 2 + (i * 2) + 1));
        Table_Set_Move(t, key, val, false);
    }
}

static void Table_Del(var self) {
    struct Table *t = self;

    for (size_t i = 0; i < t->nslots; i++) {
        if (Table_Key_Hash(t, i) isnt 0) {
            destruct(Table_Key(t, i));
            destruct(Table_Val(t, i));
        }
    }

    free(t->data);
    free(t->sspace0);
    free(t->sspace1);
}

static var Table_Key_Type(var self) {
    struct Table *t = self;
    return t->ktype;
}

static var Table_Val_Type(var self) {
    struct Table *t = self;
    return t->vtype;
}

static void Table_Clear(var self) {
    struct Table *t = self;

    for (size_t i = 0; i < t->nslots; i++) {
        if (Table_Key_Hash(t, i) isnt 0) {
            destruct(Table_Key(t, i));
            destruct(Table_Val(t, i));
        }
    }

    free(t->data);

    t->nslots = 0;
    t->nitems = 0;
    t->data = NULL;
}

static void Table_Assign(var self, var obj) {
    struct Table *t = self;
    Table_Clear(t);

    t->ktype = implements_method(obj, Get, key_type) ? key_type(obj) : Ref;
    t->vtype = implements_method(obj, Get, val_type) ? val_type(obj) : Ref;
    t->ksize = Table_Size_Round(size(t->ktype));
    t->vsize = Table_Size_Round(size(t->vtype));
    t->nitems = 0;
    t->nslots = Table_Ideal_Size(len(obj));

    if (t->nslots is 0) {
        t->data = NULL;
        return;
    }

    t->data = calloc(t->nslots, Table_Step(t));
    t->sspace0 = realloc(t->sspace0, Table_Step(t));
    t->sspace1 = realloc(t->sspace1, Table_Step(t));

#if METADOT_C_MEMORY_CHECK == 1
    if (t->data is NULL or t->sspace0 is NULL or t->sspace1 is NULL) {
        throw(OutOfMemoryError, "Cannot allocate Table, out of memory!", "");
    }
#endif

    memset(t->sspace0, 0, Table_Step(t));
    memset(t->sspace1, 0, Table_Step(t));

    foreach (key in obj) { Table_Set_Move(t, key, get(obj, key), false); }
}

static var Table_Iter_Init(var self);
static var Table_Iter_Next(var self, var curr);

static bool Table_Mem(var self, var key);
static var Table_Get(var self, var key);

static int Table_Cmp(var self, var obj) {

    int c;
    var item0 = Table_Iter_Init(self);
    var item1 = iter_init(obj);

    while (true) {
        if (item0 is Terminal and item1 is Terminal) { return 0; }
        if (item0 is Terminal) { return -1; }
        if (item1 is Terminal) { return 1; }
        c = cmp(item0, item1);
        if (c < 0) { return -1; }
        if (c > 0) { return 1; }
        c = cmp(Table_Get(self, item0), get(obj, item1));
        if (c < 0) { return -1; }
        if (c > 0) { return 1; }
        item0 = Table_Iter_Next(self, item0);
        item1 = iter_next(obj, item1);
    }

    return 0;
}

static uint64_t Table_Hash(var self) {
    struct Table *t = self;
    uint64_t h = 0;

    var curr = Table_Iter_Init(self);
    while (curr isnt Terminal) {
        var vurr = (char *) curr + t->ksize + sizeof(struct Header);
        h = h ^ hash(curr) ^ hash(vurr);
        curr = Table_Iter_Next(self, curr);
    }

    return h;
}

static size_t Table_Len(var self) {
    struct Table *t = self;
    return t->nitems;
}

static uint64_t Table_Swapspace_Hash(struct Table *t, var space) { return *((uint64_t *) space); }

static var Table_Swapspace_Key(struct Table *t, var space) {
    return (char *) space + sizeof(uint64_t) + sizeof(struct Header);
}

static var Table_Swapspace_Val(struct Table *t, var space) {
    return (char *) space + sizeof(uint64_t) + sizeof(struct Header) + t->ksize +
           sizeof(struct Header);
}

static void Table_Set_Move(var self, var key, var val, bool move) {

    struct Table *t = self;
    key = cast(key, t->ktype);
    val = cast(val, t->vtype);

    uint64_t i = hash(key) % t->nslots;
    uint64_t j = 0;

    memset(t->sspace0, 0, Table_Step(t));
    memset(t->sspace1, 0, Table_Step(t));

    if (move) {

        uint64_t ihash = i + 1;
        memcpy((char *) t->sspace0, &ihash, sizeof(uint64_t));
        memcpy((char *) t->sspace0 + sizeof(uint64_t), (char *) key - sizeof(struct Header),
               t->ksize + sizeof(struct Header));
        memcpy((char *) t->sspace0 + sizeof(uint64_t) + sizeof(struct Header) + t->ksize,
               (char *) val - sizeof(struct Header), t->vsize + sizeof(struct Header));

    } else {

        struct Header *khead = (struct Header *) ((char *) t->sspace0 + sizeof(uint64_t));
        struct Header *vhead = (struct Header *) ((char *) t->sspace0 + sizeof(uint64_t) +
                                                  sizeof(struct Header) + t->ksize);

        header_init(khead, t->ktype, AllocData);
        header_init(vhead, t->vtype, AllocData);

        uint64_t ihash = i + 1;
        memcpy((char *) t->sspace0, &ihash, sizeof(uint64_t));
        assign((char *) t->sspace0 + sizeof(uint64_t) + sizeof(struct Header), key);
        assign((char *) t->sspace0 + sizeof(uint64_t) + sizeof(struct Header) + t->ksize +
                       sizeof(struct Header),
               val);
    }

    while (true) {

        uint64_t h = Table_Key_Hash(t, i);
        if (h is 0) {
            memcpy((char *) t->data + i * Table_Step(t), t->sspace0, Table_Step(t));
            t->nitems++;
            return;
        }

        if (eq(Table_Key(t, i), Table_Swapspace_Key(t, t->sspace0))) {
            destruct(Table_Key(t, i));
            destruct(Table_Val(t, i));
            memcpy((char *) t->data + i * Table_Step(t), t->sspace0, Table_Step(t));
            return;
        }

        uint64_t p = Table_Probe(t, i, h);
        if (j >= p) {
            memcpy((char *) t->sspace1, (char *) t->data + i * Table_Step(t), Table_Step(t));
            memcpy((char *) t->data + i * Table_Step(t), (char *) t->sspace0, Table_Step(t));
            memcpy((char *) t->sspace0, (char *) t->sspace1, Table_Step(t));
            j = p;
        }

        i = (i + 1) % t->nslots;
        j++;
    }
}

static void Table_Rehash(struct Table *t, size_t new_size) {

    var old_data = t->data;
    size_t old_size = t->nslots;

    t->nslots = new_size;
    t->nitems = 0;
    t->data = calloc(t->nslots, Table_Step(t));

#if METADOT_C_MEMORY_CHECK == 1
    if (t->data is NULL) { throw(OutOfMemoryError, "Cannot allocate Table, out of memory!", ""); }
#endif

    for (size_t i = 0; i < old_size; i++) {

        uint64_t h = *(uint64_t *) ((char *) old_data + i * Table_Step(t));

        if (h isnt 0) {
            var key = (char *) old_data + i * Table_Step(t) + sizeof(uint64_t) +
                      sizeof(struct Header);
            var val = (char *) old_data + i * Table_Step(t) + sizeof(uint64_t) +
                      sizeof(struct Header) + t->ksize + sizeof(struct Header);
            Table_Set_Move(t, key, val, true);
        }
    }

    free(old_data);
}

static void Table_Resize_More(struct Table *t) {
    size_t new_size = Table_Ideal_Size(t->nitems);
    size_t old_size = t->nslots;
    if (new_size > old_size) { Table_Rehash(t, new_size); }
}

static void Table_Resize_Less(struct Table *t) {
    size_t new_size = Table_Ideal_Size(t->nitems);
    size_t old_size = t->nslots;
    if (new_size < old_size) { Table_Rehash(t, new_size); }
}

static bool Table_Mem(var self, var key) {
    struct Table *t = self;
    key = cast(key, t->ktype);

    if (t->nslots is 0) { return false; }

    uint64_t i = hash(key) % t->nslots;
    uint64_t j = 0;

    while (true) {

        uint64_t h = Table_Key_Hash(t, i);
        if (h is 0 or j > Table_Probe(t, i, h)) { return false; }

        if (eq(Table_Key(t, i), key)) { return true; }

        i = (i + 1) % t->nslots;
        j++;
    }

    return false;
}

static void Table_Rem(var self, var key) {
    struct Table *t = self;
    key = cast(key, t->ktype);

    if (t->nslots is 0) { throw(KeyError, "Key %$ not in Table!", key); }

    uint64_t i = hash(key) % t->nslots;
    uint64_t j = 0;

    while (true) {

        uint64_t h = Table_Key_Hash(t, i);
        if (h is 0 or j > Table_Probe(t, i, h)) { throw(KeyError, "Key %$ not in Table!", key); }

        if (eq(Table_Key(t, i), key)) {

            destruct(Table_Key(t, i));
            destruct(Table_Val(t, i));
            memset((char *) t->data + i * Table_Step(t), 0, Table_Step(t));

            while (true) {

                uint64_t ni = (i + 1) % t->nslots;
                uint64_t nh = Table_Key_Hash(t, ni);
                if (nh isnt 0 and Table_Probe(t, ni, nh) > 0) {
                    memcpy((char *) t->data + i * Table_Step(t),
                           (char *) t->data + ni * Table_Step(t), Table_Step(t));
                    memset((char *) t->data + ni * Table_Step(t), 0, Table_Step(t));
                    i = ni;
                } else {
                    break;
                }
            }

            t->nitems--;
            Table_Resize_Less(t);
            return;
        }

        i = (i + 1) % t->nslots;
        j++;
    }
}

static var Table_Get(var self, var key) {
    struct Table *t = self;

    if (key >= t->data and ((char *) key) < ((char *) t->data) + t->nslots * Table_Step(self)) {
        return Table_Val(self, (((char *) key) - ((char *) t->data)) / Table_Step(self));
    }

    key = cast(key, t->ktype);

    if (t->nslots is 0) { throw(KeyError, "Key %$ not in Table!", key); }

    uint64_t i = hash(key) % t->nslots;
    uint64_t j = 0;

    while (true) {

        uint64_t h = Table_Key_Hash(t, i);
        if (h is 0 or j > Table_Probe(t, i, h)) { throw(KeyError, "Key %$ not in Table!", key); }

        if (eq(Table_Key(t, i), key)) { return Table_Val(t, i); }

        i = (i + 1) % t->nslots;
        j++;
    }

    return NULL;
}

static void Table_Set(var self, var key, var val) {
    Table_Set_Move(self, key, val, false);
    Table_Resize_More(self);
}

static var Table_Iter_Init(var self) {
    struct Table *t = self;
    if (t->nitems is 0) { return Terminal; }

    for (size_t i = 0; i < t->nslots; i++) {
        if (Table_Key_Hash(t, i) isnt 0) { return Table_Key(t, i); }
    }

    return Terminal;
}

static var Table_Iter_Next(var self, var curr) {
    struct Table *t = self;

    curr = (char *) curr + Table_Step(t);

    while (true) {

        if (curr > Table_Key(t, t->nslots - 1)) { return Terminal; }
        uint64_t h = *(uint64_t *) ((char *) curr - sizeof(struct Header) - sizeof(uint64_t));
        if (h isnt 0) { return curr; }
        curr = (char *) curr + Table_Step(t);
    }

    return Terminal;
}

static var Table_Iter_Last(var self) {

    struct Table *t = self;
    if (t->nitems is 0) { return Terminal; }

    size_t i = t->nslots - 1;
    while (true) {
        if (Table_Key_Hash(t, i) isnt 0) { return Table_Key(t, i); }
        if (i == 0) { break; }
        i--;
    }

    return Terminal;
}

static var Table_Iter_Prev(var self, var curr) {
    struct Table *t = self;

    curr = (char *) curr - Table_Step(t);

    while (true) {

        if (curr < Table_Key(t, 0)) { return Terminal; }
        uint64_t h = *(uint64_t *) ((char *) curr - sizeof(struct Header) - sizeof(uint64_t));
        if (h isnt 0) { return curr; }
        curr = (char *) curr - Table_Step(t);
    }

    return Terminal;
}

static var Table_Iter_Type(var self) {
    struct Table *t = self;
    return t->ktype;
}

static int Table_Show(var self, var output, int pos) {
    struct Table *t = self;

    pos = print_to(output, pos, "<'Table' At 0x%p {", self);

    size_t j = 0;
    for (size_t i = 0; i < t->nslots; i++) {
        if (Table_Key_Hash(t, i) isnt 0) {
            pos = print_to(output, pos, "%$:%$", Table_Key(t, i), Table_Val(t, i));
            if (j < Table_Len(t) - 1) { pos = print_to(output, pos, ", ", ""); }
            j++;
        }
    }

    return print_to(output, pos, "}>", "");
}

static void Table_Resize(var self, size_t n) {
    struct Table *t = self;

    if (n is 0) {
        Table_Clear(t);
        return;
    }

#if METADOT_C_BOUND_CHECK == 1
    if (n < t->nitems) {
        throw(FormatError, "Cannot resize Table to make it smaller than %li items", $I(t->nitems));
    }
#endif

    Table_Rehash(t, Table_Ideal_Size(n));
}

static void Table_Mark(var self, var gc, void (*f)(var, void *)) {
    struct Table *t = self;
    for (size_t i = 0; i < t->nslots; i++) {
        if (Table_Key_Hash(t, i) isnt 0) {
            f(gc, Table_Key(t, i));
            f(gc, Table_Val(t, i));
        }
    }
}

var Table = MetaDotC(
        Table,
        Instance(Doc, Table_Name, Table_Brief, Table_Description, NULL, Table_Examples, NULL),
        Instance(New, Table_New, Table_Del), Instance(Assign, Table_Assign),
        Instance(Mark, Table_Mark), Instance(Cmp, Table_Cmp), Instance(Hash, Table_Hash),
        Instance(Len, Table_Len),
        Instance(Get, Table_Get, Table_Set, Table_Mem, Table_Rem, Table_Key_Type, Table_Val_Type),
        Instance(Iter, Table_Iter_Init, Table_Iter_Next, Table_Iter_Last, Table_Iter_Prev,
                 Table_Iter_Type),
        Instance(Show, Table_Show, NULL), Instance(Resize, Table_Resize));

static const char *Current_Name(void) { return "Current"; }

static const char *Current_Brief(void) { return "Implicit Object"; }

static const char *Current_Description(void) {
    return "The `Current` class can be implemented by types which have implicit "
           "instances associated with them. For example it can be used to retrieve "
           "the _current_ `Thread`, or it could be used to get the _current_ Garbage "
           "Collector."
           "\n\n"
           "This class may be implemented by types which express the [Singleton "
           "Design Pattern](http://en.wikipedia.org/wiki/Singleton_pattern)";
}

static const char *Current_Definition(void) {
    return "struct Current {\n"
           "  var (*current)(void);\n"
           "};\n";
}

static struct Example *Current_Examples(void) {

    static struct Example examples[] = {{"Usage", "var gc = current(GC);\n"
                                                  "show(gc);\n"
                                                  "var thread = current(Thread);\n"
                                                  "show(thread);\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Current_Methods(void) {

    static struct Method methods[] = {{"current", "var current(var type);",
                                       "Returns the current active object of the given `type`."},
                                      {NULL, NULL, NULL}};

    return methods;
}

var Current = MetaDotC(Current, Instance(Doc, Current_Name, Current_Brief, Current_Description,
                                         Current_Definition, Current_Examples, Current_Methods));

var current(var type) { return type_method(type, Current, current); }

struct Thread
{

    var func;
    var args;
    var tls;

    bool is_main;
    bool is_running;

#if defined(METADOT_C_UNIX)
    pthread_t thread;
#elif defined(METADOT_C_WINDOWS)
    DWORD id;
    HANDLE thread;
#endif
};

static const char *Thread_Name(void) { return "Thread"; }

static const char *Thread_Brief(void) { return "Concurrent Execution"; }

static const char *Thread_Description(void) {
    return "The `Thread` type provides a basic primitive for concurrent "
           "execution. It acts as a basic wrapper around operating system threads, "
           "using WinThreads on Windows and pthreads otherwise.";
}

static struct Example *Thread_Examples(void) {

    static struct Example examples[] = {{"Usage", "var set_value(var args) {\n"
                                                  "  assign(get(args, $I(0)), $I(1));\n"
                                                  "  return NULL;\n"
                                                  "}\n"
                                                  "\n"
                                                  "var i = $I(0);\n"
                                                  "\n"
                                                  "var x = new(Thread, $(Function, set_value));\n"
                                                  "call(x, i);\n"
                                                  "join(x);\n"
                                                  "\n"
                                                  "show(i); /* 1 */\n"},
                                        {"Exclusive Resource",
                                         "var increment(var args) {\n"
                                         "  var mut = get(args, $I(0));\n"
                                         "  var tot = get(args, $I(1));\n"
                                         "  lock(mut);\n"
                                         "  assign(tot, $I(c_int(tot)+1));\n"
                                         "  unlock(mut);\n"
                                         "  return NULL;\n"
                                         "}\n"
                                         "\n"
                                         "var mutex = new(Mutex);\n"
                                         "var total = $I(0);\n"
                                         "\n"
                                         "var threads = new(Array, Box,\n"
                                         "  new(Thread, $(Function, increment)),\n"
                                         "  new(Thread, $(Function, increment)),\n"
                                         "  new(Thread, $(Function, increment)));\n"
                                         "\n"
                                         "show(total); /* 0 */\n"
                                         "\n"
                                         "foreach (t in threads) {\n"
                                         "  call(deref(t), mutex, total);\n"
                                         "}\n"
                                         "\n"
                                         "foreach (t in threads) {\n"
                                         "  join(deref(t));\n"
                                         "}\n"
                                         "\n"
                                         "show(total); /* 3 */\n"},
                                        {NULL, NULL}};

    return examples;
}

static void Thread_New(var self, var args) {
    struct Thread *t = self;
    t->func = empty(args) ? NULL : get(args, $I(0));
    t->args = NULL;
    t->is_main = false;
    t->is_running = false;
    t->tls = new_raw(Table, String, Ref);
}

static void Thread_Del(var self) {
    struct Thread *t = self;

#ifdef METADOT_C_WINDOWS
    CloseHandle(t->thread);
#endif

    if (t->args isnt NULL) { del_raw(t->args); }
    del_raw(t->tls);
}

static int64_t Thread_C_Int(var self) {
    struct Thread *t = self;

    if (not t->is_running) { throw(ValueError, "Cannot get thread ID, thread not running!", ""); }

#if defined(METADOT_C_UNIX)
    return (int64_t) t->thread;
#elif defined(METADOT_C_WINDOWS)
    return (int64_t) t->id;
#else
    return 0;
#endif
}

static void Thread_Assign(var self, var obj) {
    struct Thread *t = self;
    struct Thread *o = cast(obj, Thread);
    t->func = o->func;
    t->tls = t->tls ? t->tls : alloc_raw(Table);
    assign(t->tls, o->tls);
}

static int Thread_Cmp(var self, var obj) { return (int) (Thread_C_Int(self) - c_int(obj)); }

static uint64_t Thread_Hash(var self) { return Thread_C_Int(self); }

static bool Thread_TLS_Key_Created = false;

#if defined(METADOT_C_UNIX)

static pthread_key_t Thread_Key_Wrapper;

static void Thread_TLS_Key_Create(void) { pthread_key_create(&Thread_Key_Wrapper, NULL); }
static void Thread_TLS_Key_Delete(void) { pthread_key_delete(Thread_Key_Wrapper); }

static var Thread_Init_Run(var self) {

    struct Thread *t = self;
    pthread_setspecific(Thread_Key_Wrapper, t);
    t->is_running = true;

#ifndef METADOT_C_NGC
    var bottom = NULL;
    var gc = new_raw(AutoC_GC, $R(&bottom));
#endif

    var exc = new_raw(Exception, NULL);

    var x = call_with(t->func, t->args);
    del_raw(t->args);
    t->args = NULL;

    del_raw(exc);

#ifndef METADOT_C_NGC
    del_raw(gc);
#endif

    return x;
}

#elif defined(METADOT_C_WINDOWS)

static DWORD Thread_Key_Wrapper;

static void Thread_TLS_Key_Create(void) { Thread_Key_Wrapper = TlsAlloc(); }
static void Thread_TLS_Key_Delete(void) { TlsFree(Thread_Key_Wrapper); }

static DWORD Thread_Init_Run(var self) {

    struct Thread *t = self;
    TlsSetValue(Thread_Key_Wrapper, t);
    t->is_running = true;

    var ex = new_raw(Exception);

#ifndef METADOT_C_NGC
    var bottom = NULL;
    var gc = new_raw(GC, $R(&bottom));
#endif

    call_with(t->func, t->args);
    del_raw(t->args);
    t->args = NULL;

#ifndef METADOT_C_NGC
    del_raw(gc);
#endif

    del_raw(ex);

    return 0;
}

#endif

static var Thread_Call(var self, var args) {

    struct Thread *t = self;

    t->args = assign(alloc_raw(type_of(args)), args);

    /* Call Init Thread & Run */

#if defined(METADOT_C_UNIX)

    /* Setup Thread Local Storage */

    if (not Thread_TLS_Key_Created) {
        Thread_TLS_Key_Create();
        Thread_TLS_Key_Created = true;
        atexit(Thread_TLS_Key_Delete);
    }

    int err = pthread_create(&t->thread, NULL, Thread_Init_Run, t);

    if (err is EINVAL) { throw(ValueError, "Invalid Argument to Thread Creation", ""); }

    if (err is EAGAIN) {
        throw(OutOfMemoryError, "Not enough resources to create another Thread", "");
    }

    if (err is EBUSY) { throw(BusyError, "System is too busy to create thread", ""); }

#elif defined(METADOT_C_WINDOWS)

    /* Setup Thread Local Storage */

    if (not Thread_TLS_Key_Created) {
        Thread_TLS_Key_Create();
        Thread_TLS_Key_Created = true;
        atexit(Thread_TLS_Key_Delete);
    }

    t->thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) Thread_Init_Run, t, 0, &t->id);

    if (t->thread is NULL) { throw(ValueError, "Unable to Create WinThread"); }

#else

    throw(ResourceError, "Unsupported Threading Environment");

#endif

    return self;
}

static var Thread_Main = NULL;
static var Exception_Main = NULL;

static void Thread_Main_Del(void) {
    del_raw(Exception_Main);
    del_raw(Thread_Main);
}

static var Thread_Current(void) {

    if (not Thread_TLS_Key_Created) {
        Thread_TLS_Key_Create();
        Thread_TLS_Key_Created = true;
        atexit(Thread_TLS_Key_Delete);
    }

#if defined(METADOT_C_UNIX)
    var wrapper = pthread_getspecific(Thread_Key_Wrapper);
#elif defined(METADOT_C_WINDOWS)
    var wrapper = TlsGetValue(Thread_Key_Wrapper);
#else
    var wrapper = NULL;
#endif

    /*
  ** Here is a nasty one. On OSX instead of
  ** returning NULL for an unset key it
  ** decides to return uninitialized rubbish
  ** (even though the spec says otherwise).
  **
  ** Luckily we can test directly for the main
  ** thread on OSX using this non-portable method
  */
#ifdef METADOT_C_MAC
    if (pthread_main_np()) { wrapper = NULL; }
#endif

    if (wrapper is NULL) {

        if (Thread_Main is NULL) {
            Thread_Main = new_raw(Thread, NULL);
            Exception_Main = new_raw(Exception, NULL);
            atexit(Thread_Main_Del);
        }

        struct Thread *t = Thread_Main;
        t->is_main = true;
        t->is_running = true;

#if defined(METADOT_C_UNIX)
        t->thread = pthread_self();
#elif defined(METADOT_C_WINDOWS)
        t->thread = GetCurrentThread();
#endif

        return Thread_Main;
    }

    return wrapper;
}

static void Thread_Start(var self) { call(self, NULL); }

static void Thread_Stop(var self) {
    struct Thread *t = self;

#if defined(METADOT_C_UNIX)
    if (not t->thread) { return; }
    int err = pthread_kill(t->thread, SIGINT);
    if (err is EINVAL) { throw(ValueError, "Invalid Argument to Thread Stop", ""); }
    if (err is ESRCH) { throw(ValueError, "Invalid Thread", ""); }
#elif defined(METADOT_C_WINDOWS)
    if (not t->thread) { return; }
    TerminateThread(t->thread, FALSE);
#endif
}

static void Thread_Join(var self) {
    struct Thread *t = self;

#if defined(METADOT_C_UNIX)
    if (not t->thread) { return; }
    int err = pthread_join(t->thread, NULL);
    if (err is EINVAL) { throw(ValueError, "Invalid Argument to Thread Join", ""); }
    if (err is ESRCH) { throw(ValueError, "Invalid Thread", ""); }
#elif defined(METADOT_C_WINDOWS)
    if (not t->thread) { return; }
    WaitForSingleObject(t->thread, INFINITE);
#endif
}

static bool Thread_Running(var self) {
    struct Thread *t = self;
    return t->is_running;
}

static var Thread_Get(var self, var key) {
    struct Thread *t = self;
    return deref(get(t->tls, key));
}

static void Thread_Set(var self, var key, var val) {
    struct Thread *t = self;
    set(t->tls, key, $R(val));
}

static bool Thread_Mem(var self, var key) {
    struct Thread *t = self;
    return mem(t->tls, key);
}

static void Thread_Rem(var self, var key) {
    struct Thread *t = self;
    rem(t->tls, key);
}

static var Thread_Key_Type(var self) {
    struct Thread *t = self;
    return key_type(t->tls);
}

static var Thread_Val_Type(var self) {
    struct Thread *t = self;
    return val_type(t->tls);
}

static void Thread_Mark(var self, var gc, void (*f)(var, void *)) {
    struct Thread *t = self;
    mark(t->tls, gc, f);
}

var Thread = MetaDotC(
        Thread,
        Instance(Doc, Thread_Name, Thread_Brief, Thread_Description, NULL, Thread_Examples, NULL),
        Instance(New, Thread_New, Thread_Del), Instance(Assign, Thread_Assign),
        Instance(Cmp, Thread_Cmp), Instance(Hash, Thread_Hash), Instance(Call, Thread_Call),
        Instance(Current, Thread_Current), Instance(Mark, Thread_Mark),
        Instance(Start, Thread_Start, Thread_Stop, Thread_Join, Thread_Running),
        Instance(C_Int, Thread_C_Int),
        Instance(Get, Thread_Get, Thread_Set, Thread_Mem, Thread_Rem));

static const char *Lock_Name(void) { return "Lock"; }

static const char *Lock_Brief(void) { return "Exclusive Resource"; }

static const char *Lock_Description(void) {
    return "The `Lock` class can be implemented by types to limit the access to them. "
           "For example this class is implemented by the `Mutex` type to provide "
           "mutual exclusion across Threads.";
}

static const char *Lock_Definition(void) {
    return "struct Lock {\n"
           "  void (*lock)(var);\n"
           "  void (*unlock)(var);\n"
           "  bool (*trylock)(var);\n"
           "};\n";
}

static struct Method *Lock_Methods(void) {

    static struct Method methods[] = {
            {"lock", "void lock(var self);", "Wait until a lock can be aquired on object `self`."},
            {"trylock", "bool trylock(var self);",
             "Try to acquire a lock on object `self`. Returns `true` on success and "
             "`false` if the resource is busy."},
            {"unlock", "void unlock(var self);", "Release lock on object `self`."},
            {NULL, NULL, NULL}};

    return methods;
}

static struct Example *Lock_Examples(void) {

    static struct Example examples[] = {{"Usage", "var x = new(Mutex);\n"
                                                  "lock(x);   /* Lock Mutex */ \n"
                                                  "print(\"Inside Mutex!\\n\");\n"
                                                  "unlock(x); /* Unlock Mutex */"},
                                        {NULL, NULL}};

    return examples;
}

var Lock = MetaDotC(Lock, Instance(Doc, Lock_Name, Lock_Brief, Lock_Description, Lock_Definition,
                                   Lock_Examples, Lock_Methods));

void lock(var self) { method(self, Lock, lock); }

void unlock(var self) { method(self, Lock, unlock); }

bool trylock(var self) { return method(self, Lock, trylock); }

struct Mutex
{
#if defined(METADOT_C_UNIX)
    pthread_mutex_t mutex;
#elif defined(METADOT_C_WINDOWS)
    HANDLE mutex;
#endif
};

static const char *Mutex_Name(void) { return "Mutex"; }

static const char *Mutex_Brief(void) { return "Mutual Exclusion Lock"; }

static const char *Mutex_Description(void) {
    return "The `Mutex` type can be used to gain mutual exclusion across Threads for "
           "access to some resource.";
}

static struct Example *Mutex_Examples(void) {

    static struct Example examples[] = {{"Usage", "var x = new(Mutex);\n"
                                                  "with (mut in x) { /* Lock Mutex */ \n"
                                                  "  print(\"Inside Mutex!\\n\");\n"
                                                  "} /* Unlock Mutex */"},
                                        {NULL, NULL}};

    return examples;
}

static void Mutex_New(var self, var args) {
    struct Mutex *m = self;
#if defined(METADOT_C_UNIX)
    pthread_mutex_init(&m->mutex, NULL);
#elif defined(METADOT_C_WINDOWS)
    m->mutex = CreateMutex(NULL, false, NULL);
#endif
}

static void Mutex_Del(var self) {
    struct Mutex *m = self;
#if defined(METADOT_C_UNIX)
    pthread_mutex_destroy(&m->mutex);
#elif defined(METADOT_C_WINDOWS)
    CloseHandle(m->mutex);
#endif
}

static void Mutex_Lock(var self) {
    struct Mutex *m = self;
#if defined(METADOT_C_UNIX)
    int err = pthread_mutex_lock(&m->mutex);

    if (err is EINVAL) { throw(ValueError, "Invalid Argument to Mutex Lock", ""); }

    if (err is EDEADLK) { throw(ResourceError, "Attempt to relock already held mutex", ""); }
#elif defined(METADOT_C_WINDOWS)
    WaitForSingleObject(m->mutex, INFINITE);
#endif
}

static bool Mutex_Trylock(var self) {
    struct Mutex *m = self;
#if defined(METADOT_C_UNIX)
    int err = pthread_mutex_trylock(&m->mutex);
    if (err == EBUSY) { return false; }
    if (err is EINVAL) { throw(ValueError, "Invalid Argument to Mutex Lock Try", ""); }
    return true;
#elif defined(METADOT_C_WINDOWS)
    return not(WaitForSingleObject(m->mutex, 0) is WAIT_TIMEOUT);
#else
    return true;
#endif
}

static void Mutex_Unlock(var self) {
    struct Mutex *m = cast(self, Mutex);
#if defined(METADOT_C_UNIX)
    int err = pthread_mutex_unlock(&m->mutex);
    if (err is EINVAL) { throw(ValueError, "Invalid Argument to Mutex Unlock", ""); }
    if (err is EPERM) { throw(ResourceError, "Mutex cannot be held by caller", ""); }
#elif defined(METADOT_C_WINDOWS)
    ReleaseMutex(m->mutex);
#endif
}

var Mutex = MetaDotC(
        Mutex,
        Instance(Doc, Mutex_Name, Mutex_Brief, Mutex_Description, NULL, Mutex_Examples, NULL),
        Instance(New, Mutex_New, Mutex_Del),
        Instance(Lock, Mutex_Lock, Mutex_Unlock, Mutex_Trylock),
        Instance(Start, Mutex_Lock, Mutex_Unlock, NULL));

static const char *Tree_Name(void) { return "Tree"; }

static const char *Tree_Brief(void) { return "Balanced Binary Tree"; }

static const char *Tree_Description(void) {
    return "The `Tree` type is a self balancing binary tree implemented as a red-black "
           "tree. It provides key-value access and requires the `Cmp` class to be "
           "defined on the key type."
           "\n\n"
           "Element lookup and insertion are provided as an `O(log(n))` operation. "
           "This means in general a `Tree` is slower than a `Table` but it has several "
           "other nice properties such as being able to iterate over the items in "
           "order and not having large pauses for rehashing on some insertions."
           "\n\n"
           "This is largely equivalent to the C++ construct "
           "[std::map](http://www.cplusplus.com/reference/map/map/)";
}

static struct Example *Tree_Examples(void) {

    static struct Example examples[] = {{"Usage", "var prices = new(Tree, String, Int);\n"
                                                  "set(prices, $S(\"Apple\"),  $I(12));\n"
                                                  "set(prices, $S(\"Banana\"), $I( 6));\n"
                                                  "set(prices, $S(\"Pear\"),   $I(55));\n"
                                                  "\n"
                                                  "foreach (key in prices) {\n"
                                                  "  var price = get(prices, key);\n"
                                                  "  println(\"Price of %$ is %$\", key, price);\n"
                                                  "}\n"},
                                        {"Manipulation",
                                         "var t = new(Tree, String, Int);\n"
                                         "set(t, $S(\"Hello\"), $I(2));\n"
                                         "set(t, $S(\"There\"), $I(5));\n"
                                         "\n"
                                         "show($I(len(t))); /* 2 */\n"
                                         "show($I(mem(t, $S(\"Hello\")))); /* 1 */\n"
                                         "\n"
                                         "rem(t, $S(\"Hello\"));\n"
                                         "\n"
                                         "show($I(len(t))); /* 1 */\n"
                                         "show($I(mem(t, $S(\"Hello\")))); /* 0 */\n"
                                         "show($I(mem(t, $S(\"There\")))); /* 1 */\n"
                                         "\n"
                                         "resize(t, 0);\n"
                                         "\n"
                                         "show($I(len(t))); /* 0 */\n"
                                         "show($I(mem(t, $S(\"Hello\")))); /* 0 */\n"
                                         "show($I(mem(t, $S(\"There\")))); /* 0 */\n"},
                                        {NULL, NULL}};

    return examples;
}

struct Tree
{
    var root;
    var ktype;
    var vtype;
    size_t ksize;
    size_t vsize;
    size_t nitems;
};

static bool Tree_Is_Red(struct Tree *m, var node);

static var *Tree_Left(struct Tree *m, var node) {
    return (var *) ((char *) node + 0 * sizeof(var));
}

static var *Tree_Right(struct Tree *m, var node) {
    return (var *) ((char *) node + 1 * sizeof(var));
}

static var Tree_Get_Parent(struct Tree *m, var node) {
    var ptr = *(var *) ((char *) node + 2 * sizeof(var));
    return (var) (((uintptr_t) ptr) & (~1));
}

static void Tree_Set_Parent(struct Tree *m, var node, var ptr) {
    if (Tree_Is_Red(m, node)) {
        *(var *) ((char *) node + 2 * sizeof(var)) = (var) (((uintptr_t) ptr) | 1);
    } else {
        *(var *) ((char *) node + 2 * sizeof(var)) = ptr;
    }
}

static var Tree_Key(struct Tree *m, var node) {
    return (char *) node + 3 * sizeof(var) + sizeof(struct Header);
}

static var Tree_Val(struct Tree *m, var node) {
    return (char *) node + 3 * sizeof(var) + sizeof(struct Header) + m->ksize +
           sizeof(struct Header);
}

static void Tree_Set_Color(struct Tree *m, var node, bool col) {
    var ptr = Tree_Get_Parent(m, node);
    if (col) {
        *(var *) ((char *) node + 2 * sizeof(var)) = (var) (((uintptr_t) ptr) | 1);
    } else {
        *(var *) ((char *) node + 2 * sizeof(var)) = ptr;
    }
}

static bool Tree_Get_Color(struct Tree *m, var node) {
    if (node is NULL) { return 0; }
    var ptr = *(var *) ((char *) node + 2 * sizeof(var));
    return ((uintptr_t) ptr) & 1;
}

static void Tree_Set_Black(struct Tree *m, var node) { Tree_Set_Color(m, node, false); }

static void Tree_Set_Red(struct Tree *m, var node) { Tree_Set_Color(m, node, true); }

static bool Tree_Is_Red(struct Tree *m, var node) { return Tree_Get_Color(m, node); }

static bool Tree_Is_Black(struct Tree *m, var node) { return not Tree_Get_Color(m, node); }

static var Tree_Alloc(struct Tree *m) {
    var node = calloc(1, 3 * sizeof(var) + sizeof(struct Header) + m->ksize +
                                 sizeof(struct Header) + m->vsize);

#if METADOT_C_MEMORY_CHECK == 1
    if (node is NULL) { throw(OutOfMemoryError, "Cannot allocate Tree entry, out of memory!", ""); }
#endif

    var key = header_init((struct Header *) ((char *) node + 3 * sizeof(var)), m->ktype, AllocData);
    var val = header_init(
            (struct Header *) ((char *) node + 3 * sizeof(var) + sizeof(struct Header) + m->ksize),
            m->vtype, AllocData);

    *Tree_Left(m, node) = NULL;
    *Tree_Right(m, node) = NULL;
    Tree_Set_Parent(m, node, NULL);
    Tree_Set_Red(m, node);

    return node;
}

static void Tree_Set(var self, var key, var val);

static void Tree_New(var self, var args) {
    struct Tree *m = self;
    m->ktype = get(args, $I(0));
    m->vtype = get(args, $I(1));
    m->ksize = size(m->ktype);
    m->vsize = size(m->vtype);
    m->nitems = 0;
    m->root = NULL;

    size_t nargs = len(args);
    if (nargs % 2 isnt 0) {
        throw(FormatError, "Received non multiple of two argument count to Tree constructor.", "");
    }

    for (size_t i = 0; i < (nargs - 2) / 2; i++) {
        var key = get(args, $I(2 + (i * 2) + 0));
        var val = get(args, $I(2 + (i * 2) + 1));
        Tree_Set(m, key, val);
    }
}

static void Tree_Clear_Entry(struct Tree *m, var node) {
    if (node isnt NULL) {
        Tree_Clear_Entry(m, *Tree_Left(m, node));
        Tree_Clear_Entry(m, *Tree_Right(m, node));
        destruct(Tree_Key(m, node));
        destruct(Tree_Val(m, node));
        free(node);
    }
}

static void Tree_Clear(var self) {
    struct Tree *m = self;
    Tree_Clear_Entry(m, m->root);
    m->nitems = 0;
    m->root = NULL;
}

static void Tree_Del(var self) {
    struct Tree *m = self;
    Tree_Clear(self);
}

static void Tree_Assign(var self, var obj) {
    struct Tree *m = self;
    Tree_Clear(self);
    m->ktype = implements_method(obj, Get, key_type) ? key_type(obj) : Ref;
    m->vtype = implements_method(obj, Get, val_type) ? val_type(obj) : Ref;
    m->ksize = size(m->ktype);
    m->vsize = size(m->vtype);
    foreach (key in obj) { Tree_Set(self, key, get(obj, key)); }
}

static var Tree_Iter_Init(var self);
static var Tree_Iter_Next(var self, var curr);

static bool Tree_Mem(var self, var key);
static var Tree_Get(var self, var key);

static int Tree_Cmp(var self, var obj) {

    int c;
    var item0 = Tree_Iter_Init(self);
    var item1 = iter_init(obj);

    while (true) {
        if (item0 is Terminal and item1 is Terminal) { return 0; }
        if (item0 is Terminal) { return -1; }
        if (item1 is Terminal) { return 1; }
        c = cmp(item0, item1);
        if (c < 0) { return -1; }
        if (c > 0) { return 1; }
        c = cmp(Tree_Get(self, item0), get(obj, item1));
        if (c < 0) { return -1; }
        if (c > 0) { return 1; }
        item0 = Tree_Iter_Next(self, item0);
        item1 = iter_next(obj, item1);
    }

    return 0;
}

static uint64_t Tree_Hash(var self) {
    struct Tree *m = self;
    uint64_t h = 0;

    var curr = Tree_Iter_Init(self);
    while (curr isnt Terminal) {
        var node = (char *) curr - sizeof(struct Header) - 3 * sizeof(var);
        h = h ^ hash(Tree_Key(m, node)) ^ hash(Tree_Val(m, node));
        curr = Tree_Iter_Next(self, curr);
    }

    return h;
}

static size_t Tree_Len(var self) {
    struct Tree *m = self;
    return m->nitems;
}

static bool Tree_Mem(var self, var key) {
    struct Tree *m = self;
    key = cast(key, m->ktype);

    var node = m->root;
    while (node isnt NULL) {
        int c = cmp(Tree_Key(m, node), key);
        if (c is 0) { return true; }
        node = c < 0 ? *Tree_Left(m, node) : *Tree_Right(m, node);
    }

    return false;
}

static var Tree_Get(var self, var key) {
    struct Tree *m = self;
    key = cast(key, m->ktype);

    var node = m->root;
    while (node isnt NULL) {
        int c = cmp(Tree_Key(m, node), key);
        if (c is 0) { return Tree_Val(m, node); }
        node = c < 0 ? *Tree_Left(m, node) : *Tree_Right(m, node);
    }

    return throw(KeyError, "Key %$ not in Tree!", key);
}

static var Tree_Key_Type(var self) {
    struct Tree *m = self;
    return m->ktype;
}

static var Tree_Val_Type(var self) {
    struct Tree *m = self;
    return m->vtype;
}

static var Tree_Maximum(struct Tree *m, var node) {
    while (*Tree_Right(m, node) isnt NULL) { node = *Tree_Right(m, node); }
    return node;
}

static var Tree_Sibling(struct Tree *m, var node) {

    if (node is NULL or Tree_Get_Parent(m, node) is NULL) { return NULL; }

    if (node is * Tree_Left(m, Tree_Get_Parent(m, node))) {
        return *Tree_Right(m, Tree_Get_Parent(m, node));
    } else {
        return *Tree_Left(m, Tree_Get_Parent(m, node));
    }
}

static var Tree_Grandparent(struct Tree *m, var node) {
    if ((node isnt NULL) and (Tree_Get_Parent(m, node) isnt NULL)) {
        return Tree_Get_Parent(m, Tree_Get_Parent(m, node));
    } else {
        return NULL;
    }
}

static var Tree_Uncle(struct Tree *m, var node) {
    var gpar = Tree_Grandparent(m, node);
    if (gpar is NULL) { return NULL; }
    if (Tree_Get_Parent(m, node) is * Tree_Left(m, gpar)) {
        return *Tree_Right(m, gpar);
    } else {
        return *Tree_Left(m, gpar);
    }
}

void Tree_Replace(struct Tree *m, var oldn, var newn) {
    if (Tree_Get_Parent(m, oldn) is NULL) {
        m->root = newn;
    } else {
        if (oldn is * Tree_Left(m, Tree_Get_Parent(m, oldn))) {
            *Tree_Left(m, Tree_Get_Parent(m, oldn)) = newn;
        } else {
            *Tree_Right(m, Tree_Get_Parent(m, oldn)) = newn;
        }
    }
    if (newn isnt NULL) { Tree_Set_Parent(m, newn, Tree_Get_Parent(m, oldn)); }
}

static void Tree_Rotate_Left(struct Tree *m, var node) {
    var r = *Tree_Right(m, node);
    Tree_Replace(m, node, r);
    *Tree_Right(m, node) = *Tree_Left(m, r);
    if (*Tree_Left(m, r) isnt NULL) { Tree_Set_Parent(m, *Tree_Left(m, r), node); }
    *Tree_Left(m, r) = node;
    Tree_Set_Parent(m, node, r);
}

static void Tree_Rotate_Right(struct Tree *m, var node) {
    var l = *Tree_Left(m, node);
    Tree_Replace(m, node, l);
    *Tree_Left(m, node) = *Tree_Right(m, l);
    if (*Tree_Right(m, l) isnt NULL) { Tree_Set_Parent(m, *Tree_Right(m, l), node); }
    *Tree_Right(m, l) = node;
    Tree_Set_Parent(m, node, l);
}

static void Tree_Set_Fix(struct Tree *m, var node) {

    while (true) {

        if (Tree_Get_Parent(m, node) is NULL) {
            Tree_Set_Black(m, node);
            return;
        }

        if (Tree_Is_Black(m, Tree_Get_Parent(m, node))) { return; }

        if ((Tree_Uncle(m, node) isnt NULL) and (Tree_Is_Red(m, Tree_Uncle(m, node)))) {
            Tree_Set_Black(m, Tree_Get_Parent(m, node));
            Tree_Set_Black(m, Tree_Uncle(m, node));
            Tree_Set_Red(m, Tree_Grandparent(m, node));
            node = Tree_Grandparent(m, node);
            continue;
        }

        if ((node is * Tree_Right(m, Tree_Get_Parent(m, node))) and
            (Tree_Get_Parent(m, node) is * Tree_Left(m, Tree_Grandparent(m, node)))) {
            Tree_Rotate_Left(m, Tree_Get_Parent(m, node));
            node = *Tree_Left(m, node);
        }

        else

                if ((node is * Tree_Left(m, Tree_Get_Parent(m, node))) and
                    (Tree_Get_Parent(m, node) is * Tree_Right(m, Tree_Grandparent(m, node)))) {
            Tree_Rotate_Right(m, Tree_Get_Parent(m, node));
            node = *Tree_Right(m, node);
        }

        Tree_Set_Black(m, Tree_Get_Parent(m, node));
        Tree_Set_Red(m, Tree_Grandparent(m, node));

        if (node is * Tree_Left(m, Tree_Get_Parent(m, node))) {
            Tree_Rotate_Right(m, Tree_Grandparent(m, node));
        } else {
            Tree_Rotate_Left(m, Tree_Grandparent(m, node));
        }

        return;
    }
}

static void Tree_Set(var self, var key, var val) {
    struct Tree *m = self;
    key = cast(key, m->ktype);
    val = cast(val, m->vtype);

    var node = m->root;

    if (node is NULL) {
        var node = Tree_Alloc(m);
        assign(Tree_Key(m, node), key);
        assign(Tree_Val(m, node), val);
        m->root = node;
        m->nitems++;
        Tree_Set_Fix(m, node);
        return;
    }

    while (true) {

        int c = cmp(Tree_Key(m, node), key);

        if (c is 0) {
            assign(Tree_Key(m, node), key);
            assign(Tree_Val(m, node), val);
            return;
        }

        if (c < 0) {

            if (*Tree_Left(m, node) is NULL) {
                var newn = Tree_Alloc(m);
                assign(Tree_Key(m, newn), key);
                assign(Tree_Val(m, newn), val);
                *Tree_Left(m, node) = newn;
                Tree_Set_Parent(m, newn, node);
                Tree_Set_Fix(m, newn);
                m->nitems++;
                return;
            }

            node = *Tree_Left(m, node);
        }

        if (c > 0) {

            if (*Tree_Right(m, node) is NULL) {
                var newn = Tree_Alloc(m);
                assign(Tree_Key(m, newn), key);
                assign(Tree_Val(m, newn), val);
                *Tree_Right(m, node) = newn;
                Tree_Set_Parent(m, newn, node);
                Tree_Set_Fix(m, newn);
                m->nitems++;
                return;
            }

            node = *Tree_Right(m, node);
        }
    }
}

static void Tree_Rem_Fix(struct Tree *m, var node) {

    while (true) {

        if (Tree_Get_Parent(m, node) is NULL) { return; }

        if (Tree_Is_Red(m, Tree_Sibling(m, node))) {
            Tree_Set_Red(m, Tree_Get_Parent(m, node));
            Tree_Set_Black(m, Tree_Sibling(m, node));
            if (node is * Tree_Left(m, Tree_Get_Parent(m, node))) {
                Tree_Rotate_Left(m, Tree_Get_Parent(m, node));
            } else {
                Tree_Rotate_Right(m, Tree_Get_Parent(m, node));
            }
        }

        if (Tree_Is_Black(m, Tree_Get_Parent(m, node)) and
            Tree_Is_Black(m, Tree_Sibling(m, node)) and
            Tree_Is_Black(m, *Tree_Left(m, Tree_Sibling(m, node))) and
            Tree_Is_Black(m, *Tree_Right(m, Tree_Sibling(m, node)))) {
            Tree_Set_Red(m, Tree_Sibling(m, node));
            node = Tree_Get_Parent(m, node);
            continue;
        }

        if (Tree_Is_Red(m, Tree_Get_Parent(m, node)) and Tree_Is_Black(m, Tree_Sibling(m, node)) and
            Tree_Is_Black(m, *Tree_Left(m, Tree_Sibling(m, node))) and
            Tree_Is_Black(m, *Tree_Right(m, Tree_Sibling(m, node)))) {
            Tree_Set_Red(m, Tree_Sibling(m, node));
            Tree_Set_Black(m, Tree_Get_Parent(m, node));
            return;
        }

        if (Tree_Is_Black(m, Tree_Sibling(m, node))) {

            if (node is * Tree_Left(m, Tree_Get_Parent(m, node)) and
                Tree_Is_Red(m, *Tree_Left(m, Tree_Sibling(m, node))) and
                Tree_Is_Black(m, *Tree_Right(m, Tree_Sibling(m, node)))) {
                Tree_Set_Red(m, Tree_Sibling(m, node));
                Tree_Set_Black(m, *Tree_Left(m, Tree_Sibling(m, node)));
                Tree_Rotate_Right(m, Tree_Sibling(m, node));
            }

            else

                    if (node is * Tree_Right(m, Tree_Get_Parent(m, node)) and
                        Tree_Is_Red(m, *Tree_Right(m, Tree_Sibling(m, node))) and
                        Tree_Is_Black(m, *Tree_Left(m, Tree_Sibling(m, node)))) {
                Tree_Set_Red(m, Tree_Sibling(m, node));
                Tree_Set_Black(m, *Tree_Right(m, Tree_Sibling(m, node)));
                Tree_Rotate_Left(m, Tree_Sibling(m, node));
            }
        }

        Tree_Set_Color(m, Tree_Sibling(m, node), Tree_Get_Color(m, Tree_Get_Parent(m, node)));

        Tree_Set_Black(m, Tree_Get_Parent(m, node));

        if (node is * Tree_Left(m, Tree_Get_Parent(m, node))) {
            Tree_Set_Black(m, *Tree_Right(m, Tree_Sibling(m, node)));
            Tree_Rotate_Left(m, Tree_Get_Parent(m, node));
        } else {
            Tree_Set_Black(m, *Tree_Left(m, Tree_Sibling(m, node)));
            Tree_Rotate_Right(m, Tree_Get_Parent(m, node));
        }

        return;
    }
}

static void Tree_Rem(var self, var key) {
    struct Tree *m = self;

    key = cast(key, m->ktype);

    bool found = false;
    var node = m->root;
    while (node isnt NULL) {
        int c = cmp(Tree_Key(m, node), key);
        if (c is 0) {
            found = true;
            break;
        }
        node = c < 0 ? *Tree_Left(m, node) : *Tree_Right(m, node);
    }

    if (not found) {
        throw(KeyError, "Key %$ not in Tree!", key);
        return;
    }

    destruct(Tree_Key(m, node));
    destruct(Tree_Val(m, node));

    if ((*Tree_Left(m, node) isnt NULL) and (*Tree_Right(m, node) isnt NULL)) {
        var pred = Tree_Maximum(m, *Tree_Left(m, node));
        bool ncol = Tree_Get_Color(m, node);
        memcpy((char *) node + 3 * sizeof(var), (char *) pred + 3 * sizeof(var),
               sizeof(struct Header) + m->ksize + sizeof(struct Header) + m->vsize);
        Tree_Set_Color(m, node, ncol);
        node = pred;
    }

    var chld = *Tree_Right(m, node) is NULL ? *Tree_Left(m, node) : *Tree_Right(m, node);

    if (Tree_Is_Black(m, node)) {
        Tree_Set_Color(m, node, Tree_Get_Color(m, chld));
        Tree_Rem_Fix(m, node);
    }

    Tree_Replace(m, node, chld);

    if ((Tree_Get_Parent(m, node) is NULL) and (chld isnt NULL)) { Tree_Set_Black(m, chld); }

    m->nitems--;
    free(node);
}

static var Tree_Iter_Init(var self) {
    struct Tree *m = self;
    if (m->nitems is 0) { return Terminal; }
    var node = m->root;
    while (*Tree_Left(m, node) isnt NULL) { node = *Tree_Left(m, node); }
    return Tree_Key(m, node);
}

static var Tree_Iter_Next(var self, var curr) {
    struct Tree *m = self;

    var node = (char *) curr - sizeof(struct Header) - 3 * sizeof(var);
    var prnt = Tree_Get_Parent(m, node);

    if (*Tree_Right(m, node) isnt NULL) {
        node = *Tree_Right(m, node);
        while (*Tree_Left(m, node) isnt NULL) { node = *Tree_Left(m, node); }
        return Tree_Key(m, node);
    }

    while (true) {
        if (prnt is NULL) { return Terminal; }
        if (node is * Tree_Left(m, prnt)) { return Tree_Key(m, prnt); }
        if (node is * Tree_Right(m, prnt)) {
            prnt = Tree_Get_Parent(m, prnt);
            node = Tree_Get_Parent(m, node);
        }
    }

    return Terminal;
}

static var Tree_Iter_Last(var self) {
    struct Tree *m = self;
    if (m->nitems is 0) { return Terminal; }
    var node = m->root;
    while (*Tree_Right(m, node) isnt NULL) { node = *Tree_Right(m, node); }
    return Tree_Key(m, node);
}

static var Tree_Iter_Prev(var self, var curr) {
    struct Tree *m = self;

    var node = (char *) curr - sizeof(struct Header) - 3 * sizeof(var);
    var prnt = Tree_Get_Parent(m, node);

    if (*Tree_Left(m, node) isnt NULL) {
        node = *Tree_Left(m, node);
        while (*Tree_Right(m, node) isnt NULL) { node = *Tree_Right(m, node); }
        return Tree_Key(m, node);
    }

    while (true) {
        if (prnt is NULL) { return Terminal; }
        if (node is * Tree_Right(m, prnt)) { return Tree_Key(m, prnt); }
        if (node is * Tree_Left(m, prnt)) {
            prnt = Tree_Get_Parent(m, prnt);
            node = Tree_Get_Parent(m, node);
        }
    }

    return Terminal;
}

static var Tree_Iter_Type(var self) {
    struct Tree *m = self;
    return m->ktype;
}

static int Tree_Show(var self, var output, int pos) {
    struct Tree *m = self;

    pos = print_to(output, pos, "<'Tree' At 0x%p {", self);

    var curr = Tree_Iter_Init(self);

    while (curr isnt Terminal) {
        var node = (char *) curr - sizeof(struct Header) - 3 * sizeof(var);
        pos = print_to(output, pos, "%$:%$", Tree_Key(m, node), Tree_Val(m, node));
        curr = Tree_Iter_Next(self, curr);
        if (curr isnt Terminal) { pos = print_to(output, pos, ", ", ""); }
    }

    return print_to(output, pos, "}>", "");
}

static void Tree_Mark(var self, var gc, void (*f)(var, void *)) {
    struct Tree *m = self;

    var curr = Tree_Iter_Init(self);

    while (curr isnt Terminal) {
        var node = (char *) curr - sizeof(struct Header) - 3 * sizeof(var);
        f(gc, Tree_Key(m, node));
        f(gc, Tree_Val(m, node));
        curr = Tree_Iter_Next(self, curr);
    }
}

static void Tree_Resize(var self, size_t n) {

    if (n is 0) {
        Tree_Clear(self);
    } else {
        throw(FormatError, "Cannot resize Tree to %li items. Trees can only be resized to 0 items.",
              $I(n));
    }
}

var Tree = MetaDotC(
        Tree, Instance(Doc, Tree_Name, Tree_Brief, Tree_Description, NULL, Tree_Examples, NULL),
        Instance(New, Tree_New, Tree_Del), Instance(Assign, Tree_Assign), Instance(Mark, Tree_Mark),
        Instance(Cmp, Tree_Cmp), Instance(Hash, Tree_Hash), Instance(Len, Tree_Len),
        Instance(Get, Tree_Get, Tree_Set, Tree_Mem, Tree_Rem, Tree_Key_Type, Tree_Val_Type),
        Instance(Resize, Tree_Resize),
        Instance(Iter, Tree_Iter_Init, Tree_Iter_Next, Tree_Iter_Last, Tree_Iter_Prev,
                 Tree_Iter_Type),
        Instance(Show, Tree_Show, NULL));

static const char *Tuple_Name(void) { return "Tuple"; }

static const char *Tuple_Brief(void) { return "Basic Collection"; }

static const char *Tuple_Description(void) {
    return "The `Tuple` type provides a basic way to create a simple collection of "
           "objects. Its main use is the fact that it can be constructed on the "
           "stack using the `tuple` macro. This makes it suitable for a number of "
           "purposes such as use in functions that take a variable number of "
           "arguments."
           "\n\n"
           "Tuples can also be constructed on the heap and stored in collections. "
           "This makes them also useful as a simple _untyped_ list of objects."
           "\n\n"
           "Internally Tuples are just an array of pointers terminated with a pointer "
           "to the MetaDotC `Terminal` object. This makes positional access fast, but "
           "many other operations slow including iteration and counting the number of "
           "elements. Due to this it is only recommended Tuples are used for small "
           "collections. "
           "\n\n"
           "Because Tuples are terminated with the MetaDotC `Terminal` object this can't "
           "naturally be included within them. This object should therefore only be "
           "returned from iteration functions.";
}

static const char *Tuple_Definition(void) {
    return "struct Tuple {\n"
           "  var* items;\n"
           "};\n";
}

static struct Example *Tuple_Examples(void) {

    static struct Example examples[] = {{"Usage",
                                         "var x = tuple($I(100), $I(200), $S(\"Hello\"));\n"
                                         "show(x);\n"
                                         "var y = tuple(Int, $I(10), $I(20));\n"
                                         "var z = new_with(Array, y);\n"
                                         "show(z);\n"
                                         "\n"
                                         "foreach (item in x) {\n"
                                         "  println(\"%$\", item);\n"
                                         "}\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Tuple_Methods(void) {

    static struct Method methods[] = {
            {"tuple", "#define tuple(...)", "Construct a `Tuple` object on the stack."},
            {NULL, NULL, NULL}};

    return methods;
}

static void Tuple_New(var self, var args) {
    struct Tuple *t = self;
    size_t nargs = len(args);

    t->items = malloc(sizeof(var) * (nargs + 1));

#if METADOT_C_MEMORY_CHECK == 1
    if (t->items is NULL) { throw(OutOfMemoryError, "Cannot create Tuple, out of memory!", ""); }
#endif

    for (size_t i = 0; i < nargs; i++) { t->items[i] = get(args, $I(i)); }

    t->items[nargs] = Terminal;
}

static void Tuple_Del(var self) {
    struct Tuple *t = self;

#if METADOT_C_ALLOC_CHECK == 1
    if (header(self)->alloc is(var) AllocStack or header(self)->alloc is(var) AllocStatic) {
        throw(ValueError, "Cannot destruct Tuple, not on heap!", "");
    }
#endif

    free(t->items);
}

static void Tuple_Push(var self, var obj);

static void Tuple_Assign(var self, var obj) {
    struct Tuple *t = self;

    if (implements_method(obj, Len, len) and implements_method(obj, Get, get)) {

        size_t nargs = len(obj);

#if METADOT_C_ALLOC_CHECK == 1
        if (header(self)->alloc is(var) AllocStack or header(self)->alloc is(var) AllocStatic) {
            throw(ValueError, "Cannot reallocate Tuple, not on heap!", "");
        }
#endif

        t->items = realloc(t->items, sizeof(var) * (nargs + 1));

#if METADOT_C_MEMORY_CHECK == 1
        if (t->items is NULL) {
            throw(OutOfMemoryError, "Cannot allocate Tuple, out of memory!", "");
        }
#endif

        for (size_t i = 0; i < nargs; i++) { t->items[i] = get(obj, $I(i)); }

        t->items[nargs] = Terminal;

    } else {

        foreach (item in obj) { Tuple_Push(self, item); }
    }
}

static size_t Tuple_Len(var self) {
    struct Tuple *t = self;
    size_t i = 0;
    while (t->items and t->items[i] isnt Terminal) { i++; }
    return i;
}

static var Tuple_Iter_Init(var self) {
    struct Tuple *t = self;
    return t->items[0];
}

static var Tuple_Iter_Next(var self, var curr) {
    struct Tuple *t = self;
    size_t i = 0;
    while (t->items[i] isnt Terminal) {
        if (t->items[i] is curr) { return t->items[i + 1]; }
        i++;
    }
    return Terminal;
}

static var Tuple_Iter_Last(var self) {
    struct Tuple *t = self;
    return t->items[Tuple_Len(t) - 1];
}

static var Tuple_Iter_Prev(var self, var curr) {
    struct Tuple *t = self;
    if (curr is t->items[0]) { return Terminal; }
    size_t i = 0;
    while (t->items[i] isnt Terminal) {
        if (t->items[i] is curr) { return t->items[i - 1]; }
        i++;
    }
    return Terminal;
}

static var Tuple_Get(var self, var key) {
    struct Tuple *t = self;
    size_t nitems = Tuple_Len(t);

    int64_t i = c_int(key);
    i = i < 0 ? nitems + i : i;

#if METADOT_C_BOUND_CHECK == 1
    if (i < 0 or i >= (int64_t) nitems) {
        return throw(IndexOutOfBoundsError, "Index '%i' out of bounds for Tuple of size %i.", key,
                     $I(Tuple_Len(t)));
    }
#endif

    return t->items[i];
}

static void Tuple_Set(var self, var key, var val) {
    struct Tuple *t = self;
    size_t nitems = Tuple_Len(t);

    int64_t i = c_int(key);
    i = i < 0 ? nitems + i : i;

#if METADOT_C_BOUND_CHECK == 1
    if (i < 0 or i >= (int64_t) nitems) {
        throw(IndexOutOfBoundsError, "Index '%i' out of bounds for Tuple of size %i.", key,
              $I(Tuple_Len(t)));
        return;
    }
#endif

    t->items[i] = val;
}

static bool Tuple_Mem(var self, var item) {
    foreach (obj in self) {
        if (eq(obj, item)) { return true; }
    }
    return false;
}

static void Tuple_Pop_At(var self, var key);

static void Tuple_Rem(var self, var item) {
    struct Tuple *t = self;
    size_t i = 0;
    while (t->items[i] isnt Terminal) {
        if (eq(item, t->items[i])) {
            Tuple_Pop_At(self, $I(i));
            return;
        }
        i++;
    }
}

static int Tuple_Show(var self, var output, int pos) {
    struct Tuple *t = self;
    pos = print_to(output, pos, "tuple(", self);
    size_t i = 0;
    while (t->items[i] isnt Terminal) {
        pos = print_to(output, pos, "%$", t->items[i]);
        if (t->items[i + 1] isnt Terminal) { pos = print_to(output, pos, ", ", ""); }
        i++;
    }
    return print_to(output, pos, ")", "");
}

static void Tuple_Push(var self, var obj) {

    struct Tuple *t = self;
    size_t nitems = Tuple_Len(t);

#if METADOT_C_ALLOC_CHECK == 1
    if (header(self)->alloc is(var) AllocStack or header(self)->alloc is(var) AllocStatic) {
        throw(ValueError, "Cannot reallocate Tuple, not on heap!", "");
    }
#endif

    t->items = realloc(t->items, sizeof(var) * (nitems + 2));

#if METADOT_C_MEMORY_CHECK == 1
    if (t->items is NULL) { throw(OutOfMemoryError, "Cannot grow Tuple, out of memory!", ""); }
#endif

    t->items[nitems + 0] = obj;
    t->items[nitems + 1] = Terminal;
}

static void Tuple_Pop(var self) {

    struct Tuple *t = self;
    size_t nitems = Tuple_Len(t);

#if METADOT_C_BOUND_CHECK == 1
    if (nitems is 0) {
        throw(IndexOutOfBoundsError, "Cannot pop. Tuple is empty!", "");
        return;
    }
#endif

#if METADOT_C_ALLOC_CHECK == 1
    if (header(self)->alloc is(var) AllocStack or header(self)->alloc is(var) AllocStatic) {
        throw(ValueError, "Cannot reallocate Tuple, not on heap!", "");
    }
#endif

    t->items = realloc(t->items, sizeof(var) * nitems);
    t->items[nitems - 1] = Terminal;
}

static void Tuple_Push_At(var self, var obj, var key) {

    struct Tuple *t = self;
    size_t nitems = Tuple_Len(t);

    int64_t i = c_int(key);
    i = i < 0 ? nitems + i : i;

#if METADOT_C_BOUND_CHECK == 1
    if (i < 0 or i >= (int64_t) nitems) {
        throw(IndexOutOfBoundsError, "Index '%i' out of bounds for Tuple of size %i.", key,
              $I(nitems));
    }
#endif

#if METADOT_C_ALLOC_CHECK == 1
    if (header(self)->alloc is(var) AllocStack or header(self)->alloc is(var) AllocStatic) {
        throw(ValueError, "Cannot reallocate Tuple, not on heap!", "");
    }
#endif

    t->items = realloc(t->items, sizeof(var) * (nitems + 2));

#if METADOT_C_MEMORY_CHECK == 1
    if (t->items is NULL) { throw(OutOfMemoryError, "Cannot grow Tuple, out of memory!", ""); }
#endif

    memmove(&t->items[i + 1], &t->items[i + 0], sizeof(var) * (nitems - (size_t) i + 1));

    t->items[i] = obj;
}

static void Tuple_Pop_At(var self, var key) {

    struct Tuple *t = self;
    size_t nitems = Tuple_Len(t);

    int64_t i = c_int(key);
    i = i < 0 ? nitems + i : i;

#if METADOT_C_BOUND_CHECK == 1
    if (i < 0 or i >= (int64_t) nitems) {
        throw(IndexOutOfBoundsError, "Index '%i' out of bounds for Tuple of size %i.", key,
              $I(nitems));
    }
#endif

    memmove(&t->items[i + 0], &t->items[i + 1], sizeof(var) * (nitems - (size_t) i));

#if METADOT_C_ALLOC_CHECK == 1
    if (header(self)->alloc is(var) AllocStack or header(self)->alloc is(var) AllocStatic) {
        throw(ValueError, "Cannot reallocate Tuple, not on heap!", "");
    }
#endif

    t->items = realloc(t->items, sizeof(var) * nitems);
}

static void Tuple_Concat(var self, var obj) {

    struct Tuple *t = self;
    size_t nitems = Tuple_Len(t);
    size_t objlen = len(obj);

#if METADOT_C_ALLOC_CHECK == 1
    if (header(self)->alloc is(var) AllocStack or header(self)->alloc is(var) AllocStatic) {
        throw(ValueError, "Cannot reallocate Tuple, not on heap!", "");
    }
#endif

    t->items = realloc(t->items, sizeof(var) * (nitems + 1 + objlen));

#if METADOT_C_MEMORY_CHECK == 1
    if (t->items is NULL) { throw(OutOfMemoryError, "Cannot grow Tuple, out of memory!", ""); }
#endif

    size_t i = nitems;
    foreach (item in obj) {
        t->items[i] = item;
        i++;
    }

    t->items[nitems + objlen] = Terminal;
}

static void Tuple_Resize(var self, size_t n) {
    struct Tuple *t = self;

#if METADOT_C_ALLOC_CHECK == 1
    if (header(self)->alloc is(var) AllocStack or header(self)->alloc is(var) AllocStatic) {
        throw(ValueError, "Cannot reallocate Tuple, not on heap!", "");
    }
#endif

    size_t m = Tuple_Len(self);

    if (n < m) {
        t->items = realloc(t->items, sizeof(var) * (n + 1));
        t->items[n] = Terminal;
    } else {
        throw(FormatError, "Cannot resize Tuple to %li as it only contains %li items", $I(n),
              $I(m));
    }
}

static void Tuple_Mark(var self, var gc, void (*f)(var, void *)) {
    struct Tuple *t = self;
    size_t i = 0;
    if (t->items is NULL) { return; }
    while (t->items[i] isnt Terminal) {
        f(gc, t->items[i]);
        i++;
    }
}

static void Tuple_Swap(struct Tuple *t, size_t i, size_t j) {
    var tmp = t->items[i];
    t->items[i] = t->items[j];
    t->items[j] = tmp;
}

static size_t Tuple_Sort_Partition(struct Tuple *t, int64_t l, int64_t r, bool (*f)(var, var)) {

    int64_t p = l + (r - l) / 2;
    Tuple_Swap(t, p, r);

    int64_t s = l;
    for (int64_t i = l; i < r; i++) {
        if (f(t->items[i], t->items[r])) {
            Tuple_Swap(t, i, s);
            s++;
        }
    }

    Tuple_Swap(t, s, r);
    return s;
}

static void Tuple_Sort_Part(struct Tuple *t, int64_t l, int64_t r, bool (*f)(var, var)) {
    if (l < r) {
        int64_t s = Tuple_Sort_Partition(t, l, r, f);
        Tuple_Sort_Part(t, l, s - 1, f);
        Tuple_Sort_Part(t, s + 1, r, f);
    }
}

static void Tuple_Sort_By(var self, bool (*f)(var, var)) {
    Tuple_Sort_Part(self, 0, Tuple_Len(self) - 1, f);
}

static int Tuple_Cmp(var self, var obj) {
    struct Tuple *t = self;

    size_t i = 0;
    var item0 = t->items[i];
    var item1 = iter_init(obj);

    while (true) {
        if (item0 is Terminal and item1 is Terminal) { return 0; }
        if (item0 is Terminal) { return -1; }
        if (item1 is Terminal) { return 1; }
        int c = cmp(item0, item1);
        if (c < 0) { return -1; }
        if (c > 0) { return 1; }
        i++;
        item0 = t->items[i];
        item1 = iter_next(obj, item1);
    }

    return 0;
}

static uint64_t Tuple_Hash(var self) {
    struct Tuple *t = self;
    uint64_t h = 0;

    size_t n = Tuple_Len(self);
    for (size_t i = 0; i < n; i++) { h ^= hash(t->items[i]); }

    return h;
}

var Tuple = MetaDotC(
        Tuple,
        Instance(Doc, Tuple_Name, Tuple_Brief, Tuple_Description, Tuple_Definition, Tuple_Examples,
                 Tuple_Methods),
        Instance(New, Tuple_New, Tuple_Del), Instance(Assign, Tuple_Assign),
        Instance(Cmp, Tuple_Cmp), Instance(Hash, Tuple_Hash), Instance(Len, Tuple_Len),
        Instance(Get, Tuple_Get, Tuple_Set, Tuple_Mem, Tuple_Rem),
        Instance(Push, Tuple_Push, Tuple_Pop, Tuple_Push_At, Tuple_Pop_At),
        Instance(Concat, Tuple_Concat, Tuple_Push), Instance(Resize, Tuple_Resize),
        Instance(Iter, Tuple_Iter_Init, Tuple_Iter_Next, Tuple_Iter_Last, Tuple_Iter_Prev, NULL),
        Instance(Mark, Tuple_Mark), Instance(Sort, Tuple_Sort_By),
        Instance(Show, Tuple_Show, NULL));

static const char *Cast_Name(void) { return "Cast"; }

static const char *Cast_Brief(void) { return "Runtime Type Checking"; }

static const char *Cast_Description(void) {
    return "The `Cast` class provides a rudimentary run-time type checking. By "
           "default it simply checks that the passed in object is of a given type "
           "but it can be overridden by types which have to do more complex checking "
           "to ensure the types are correct.";
}

static const char *Cast_Definition(void) {
    return "struct Cast {\n"
           "  var (*cast)(var, var);\n"
           "};\n";
}

static struct Example *Cast_Examples(void) {

    static struct Example examples[] = {{"Usage", "var x = $I(100);\n"
                                                  "struct Int* y = cast(x, Int);\n"
                                                  "show(y);\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Cast_Methods(void) {

    static struct Method methods[] = {
            {"cast", "var cast(var self, var type);",
             "Ensures the object `self` is of the given `type` and returns it if it "
             "is."},
            {NULL, NULL, NULL}};

    return methods;
}

var Cast = MetaDotC(Cast, Instance(Doc, Cast_Name, Cast_Brief, Cast_Description, Cast_Definition,
                                   Cast_Examples, Cast_Methods));

var cast(var self, var type) {

    struct Cast *c = instance(self, Cast);
    if (c and c->cast) { return c->cast(self, type); }

    if (type_of(self) is type) {
        return self;
    } else {
        return throw(ValueError, "cast expected type %s, got type %s", type_of(self), type);
    }
}

static const char *Type_Name(void) { return "Type"; }

static const char *Type_Brief(void) { return "Metadata Object"; }

static const char *Type_Description(void) {
    return "The `Type` type is one of the most important types in MetaDotC. It is the "
           "object which specifies the meta-data associated with a particular object. "
           "Most importantly this says what classes an object implements and what "
           "their instances are."
           "\n\n"
           "One can get the type of an object using the `type_of` function."
           "\n\n"
           "To see if an object implements a class `implements` can be used. To "
           "call a member of a class with an object `method` can be used."
           "\n\n"
           "To see if a type implements a class `type_implements` can be used. To "
           "call a member of a class, implemented `type_method` can be used.";
}

static struct Example *Type_Examples(void) {

    static struct Example examples[] = {{"Usage",
                                         "var t = type_of($I(5));\n"
                                         "show(t); /* Int */\n"
                                         "\n"
                                         "show($I(type_implements(t, New)));  /* 1 */\n"
                                         "show($I(type_implements(t, Cmp)));  /* 1 */\n"
                                         "show($I(type_implements(t, Hash))); /* 1 */\n"
                                         "\n"
                                         "show($I(type_method(t, Cmp, cmp, $I(5), $I(6))));\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Type_Methods(void) {

    static struct Method methods[] = {
            {"type_of", "var type_of(var self);", "Returns the `Type` of an object `self`."},
            {"instance",
             "var instance(var self, var cls);\n"
             "var type_instance(var type, var cls);",
             "Returns the instance of class `cls` implemented by object `self` or "
             "type `type`. If class is not implemented then returns `NULL`."},
            {"implements",
             "bool implements(var self, var cls);\n"
             "bool type_implements(var type, var cls);",
             "Returns if the object `self` or type `type` implements the class `cls`."},
            {"method",
             "#define method(X, C, M, ...)\n"
             "#define type_method(T, C, M, ...)",
             "Returns the result of the call to method `M` of class `C` for object `X`"
             "or type `T`. If class is not implemented then an error is thrown."},
            {"implements_method",
             "#define implements_method(X, C, M)\n"
             "#define type_implements_method(T, C, M)",
             "Returns if the type `T` or object `X` implements the method `M` of "
             "class C."},
            {NULL, NULL, NULL}};

    return methods;
}

enum {
    METADOT_C_NBUILTINS = 2 + (METADOT_C_CACHE_NUM / 3),
    METADOT_C_MAX_INSTANCES = 256
};

static var Type_Alloc(void) {

    struct Header *head =
            calloc(1, sizeof(struct Header) + sizeof(struct Type) * (METADOT_C_NBUILTINS +
                                                                     METADOT_C_MAX_INSTANCES + 1));

#if METADOT_C_MEMORY_CHECK == 1
    if (head is NULL) { throw(OutOfMemoryError, "Cannot create new 'Type', out of memory!", ""); }
#endif

    return header_init(head, Type, AllocHeap);
}

static void Type_New(var self, var args) {

    struct Type *t = self;

    var name = get(args, $I(0));
    var size = get(args, $I(1));

#if METADOT_C_MEMORY_CHECK == 1
    if (len(args) - 2 > METADOT_C_MAX_INSTANCES) {
        throw(OutOfMemoryError, "Cannot construct 'Type' with %i instances, maximum is %i.",
              $I(len(args)), $I(METADOT_C_MAX_INSTANCES));
    }
#endif

    size_t cache_entries = METADOT_C_CACHE_NUM / 3;
    for (size_t i = 0; i < cache_entries; i++) { t[i] = (struct Type){NULL, NULL, NULL}; }

    t[cache_entries + 0] = (struct Type){NULL, "__Name", (var) c_str(name)};
    t[cache_entries + 1] = (struct Type){NULL, "__Size", (var) (uintptr_t) c_int(size)};

    for (size_t i = 2; i < len(args); i++) {
        var ins = get(args, $I(i));
        t[METADOT_C_NBUILTINS - 2 + i] = (struct Type){NULL, (var) c_str(type_of(ins)), ins};
    }

    t[METADOT_C_NBUILTINS + len(args) - 2] = (struct Type){NULL, NULL, NULL};
}

static char *Type_Builtin_Name(struct Type *t) { return t[(METADOT_C_CACHE_NUM / 3) + 0].inst; }

static size_t Type_Builtin_Size(struct Type *t) {
    return (size_t) t[(METADOT_C_CACHE_NUM / 3) + 1].inst;
}

static int Type_Show(var self, var output, int pos) {
    return format_to(output, pos, "%s", Type_Builtin_Name(self));
}

static int Type_Cmp(var self, var obj) {
    struct Type *objt = cast(obj, Type);
    return strcmp(Type_Builtin_Name(self), Type_Builtin_Name(objt));
}

static uint64_t Type_Hash(var self) {
    const char *name = Type_Builtin_Name(self);
    return hash_data(name, strlen(name));
}

static char *Type_C_Str(var self) { return Type_Builtin_Name(self); }

static void Type_Assign(var self, var obj) {
    throw(ValueError, "Type objects cannot be assigned.", "");
}

static var Type_Copy(var self) { return throw(ValueError, "Type objects cannot be copied.", ""); }

static int print_indent(var out, int pos, const char *str) {
    pos = print_to(out, pos, "    ", "");
    while (*str) {
        if (*str is '\n') {
            pos = print_to(out, pos, "\n    ", "");
        } else {
            pos = print_to(out, pos, "%c", $I(*str));
        }
        str++;
    }
    return pos;
}

static int Type_Help_To(var self, var out, int pos) {

    struct Doc *doc = type_instance(self, Doc);

    if (doc is NULL) { return print_to(out, pos, "\nNo Documentation Found for Type %s\n", self); }

    pos = print_to(out, pos, "\n", "");
    pos = print_to(out, pos, "# %s ", self);

    if (doc->brief) { pos = print_to(out, pos, " - %s\n\n", $S((char *) doc->brief())); }

    if (doc->description) { pos = print_to(out, pos, "%s\n\n", $S((char *) doc->description())); }

    if (doc->definition) {
        pos = print_to(out, pos, "\n### Definition\n\n", "");
        pos = print_indent(out, pos, doc->definition());
        pos = print_to(out, pos, "\n\n", "");
    }

    if (doc->methods) {
        pos = print_to(out, pos, "\n### Methods\n\n", "");
        struct Method *methods = doc->methods();
        while (methods[0].name) {
            pos = print_to(out, pos, "__%s__\n\n", $S((char *) methods[0].name));
            pos = print_indent(out, pos, methods[0].definition);
            pos = print_to(out, pos, "\n\n%s\n\n", $S((char *) methods[0].description));
            methods++;
        }
    }

    if (doc->examples) {
        pos = print_to(out, pos, "\n### Examples\n\n", "");
        struct Example *examples = doc->examples();
        while (examples[0].name) {
            pos = print_to(out, pos, "__%s__\n\n", $S((char *) examples[0].name));
            pos = print_indent(out, pos, examples[0].body);
            pos = print_to(out, pos, "\n\n", "");
            examples++;
        }
        pos = print_to(out, pos, "\n\n", "");
    }

    return pos;
}

var Type = MetaDotCEmpty(
        Type,
        Instance(Doc, Type_Name, Type_Brief, Type_Description, NULL, Type_Examples, Type_Methods),
        Instance(Assign, Type_Assign), Instance(Copy, Type_Copy), Instance(Alloc, Type_Alloc, NULL),
        Instance(New, Type_New, NULL), Instance(Cmp, Type_Cmp), Instance(Hash, Type_Hash),
        Instance(Show, Type_Show, NULL), Instance(C_Str, Type_C_Str), Instance(Help, Type_Help_To));

static var Type_Scan(var self, var cls) {

#if METADOT_C_METHOD_CHECK == 1
    if (type_of(self) isnt Type) {
        return throw(TypeError, "Method call got non type '%s'", type_of(self));
    }
#endif

    struct Type *t;

    t = (struct Type *) self + METADOT_C_NBUILTINS;
    while (t->name) {
        if (t->cls is cls) { return t->inst; }
        t++;
    }

    t = (struct Type *) self + METADOT_C_NBUILTINS;
    while (t->name) {
        if (strcmp(t->name, Type_Builtin_Name(cls)) is 0) {
            t->cls = cls;
            return t->inst;
        }
        t++;
    }

    return NULL;
}

static bool Type_Implements(var self, var cls) { return Type_Scan(self, cls) isnt NULL; }

bool type_implements(var self, var cls) { return Type_Implements(self, cls); }

static var Type_Instance(var self, var cls);

static var Type_Method_At_Offset(var self, var cls, size_t offset, const char *method_name) {

    var inst = Type_Instance(self, cls);

#if METADOT_C_METHOD_CHECK == 1
    if (inst is NULL) {
        return throw(ClassError, "Type '%s' does not implement class '%s'", self, cls);
    }
#endif

#if METADOT_C_METHOD_CHECK == 1
    var meth = *((var *) (((char *) inst) + offset));

    if (meth is NULL) {
        return throw(ClassError, "Type '%s' implements class '%s' but not the method '%s' required",
                     self, cls, $(String, (char *) method_name));
    }
#endif

    return inst;
}

var type_method_at_offset(var self, var cls, size_t offset, const char *method_name) {
    return Type_Method_At_Offset(self, cls, offset, method_name);
}

static bool Type_Implements_Method_At_Offset(var self, var cls, size_t offset) {
    var inst = Type_Scan(self, cls);
    if (inst is NULL) { return false; }
    var meth = *((var *) (((char *) inst) + offset));
    if (meth is NULL) { return false; }
    return true;
}

bool type_implements_method_at_offset(var self, var cls, size_t offset) {
    return Type_Implements_Method_At_Offset(self, cls, offset);
}

/*
**  Doing the lookup of a class instances is fairly fast
**  but still too slow to be done inside a tight inner loop.
**  This is because there could be any number of instances 
**  and they could be in any order, so each time a linear 
**  search must be done to find the correct instance.
**
**  We can remove the need for a linear search by placing
**  some common class instances at known locations. These 
**  are the _Type Cache Entries_ and are located at some
**  preallocated space at the beginning of every type object.
**
**  The only problem is that these instances are not filled 
**  at compile type, so we must dynamically fill them if they
**  are empty. But this can be done with a standard call to 
**  `Type_Scan` the first time.
**
**  The main advantage of this method is that it gives the compiler
**  a better chance of inlining the code up to the call of the 
**  instance function pointer, and removes the overhead 
**  associated with setting up the call to `Type_Scan` which is 
**  too complex a call to be effectively inlined.
**
*/

#define Type_Cache_Entry(i, lit)                                                                   \
    if (cls is lit) {                                                                              \
        var inst = ((var *) self)[i];                                                              \
        if (inst is NULL) {                                                                        \
            inst = Type_Scan(self, lit);                                                           \
            ((var *) self)[i] = inst;                                                              \
        }                                                                                          \
        return inst;                                                                               \
    }

static var Type_Instance(var self, var cls) {

#if METADOT_C_CACHE == 1
    Type_Cache_Entry(0, Size);
    Type_Cache_Entry(1, Alloc);
    Type_Cache_Entry(2, New);
    Type_Cache_Entry(3, Assign);
    Type_Cache_Entry(4, Cmp);
    Type_Cache_Entry(5, Mark);
    Type_Cache_Entry(6, Hash);
    Type_Cache_Entry(7, Len);
    Type_Cache_Entry(8, Iter);
    Type_Cache_Entry(9, Push);
    Type_Cache_Entry(10, Concat);
    Type_Cache_Entry(11, Get);
    Type_Cache_Entry(12, C_Str);
    Type_Cache_Entry(13, C_Int);
    Type_Cache_Entry(14, C_Float);
    Type_Cache_Entry(15, Current);
    Type_Cache_Entry(16, Cast);
    Type_Cache_Entry(17, Pointer);
#endif

    return Type_Scan(self, cls);
}

#undef Type_Cache_Entry

var type_instance(var self, var cls) { return Type_Instance(self, cls); }

static var Type_Of(var self) {

    /*
  **  The type of a Type object is just `Type` again. But because `Type` is 
  **  extern it isn't a constant expression. This means it cannot be set at 
  **  compile time.
  **
  **  But we really want to be able to construct types statically. So by 
  **  convention at compile time the type of a Type object is set to `NULL`.
  **  So if we access a statically allocated object and it tells us `NULL` 
  **  is the type, we assume the type is `Type`.
  */

#if METADOT_C_NULL_CHECK == 1
    if (self is NULL) { return throw(ValueError, "Received NULL as value to 'type_of'", ""); }
#endif

    struct Header *head = (struct Header *) ((char *) self - sizeof(struct Header));

#if METADOT_C_MAGIC_CHECK == 1
    if (head->magic is(var) 0xDeadCe110) {
        throw(ValueError,
              "Pointer '%p' passed to 'type_of' "
              "has bad magic number, it looks like it was already deallocated.",
              self);
    }

    if (head->magic isnt((var) METADOT_C_MAGIC_NUM)) {
        throw(ValueError,
              "Pointer '%p' passed to 'type_of' "
              "has bad magic number, perhaps it wasn't allocated by MetaDotC.",
              self);
    }
#endif

    if (head->type is NULL) { head->type = Type; }

    return head->type;
}

var type_of(var self) { return Type_Of(self); }

var instance(var self, var cls) { return Type_Instance(Type_Of(self), cls); }

bool implements(var self, var cls) { return Type_Implements(Type_Of(self), cls); }

var method_at_offset(var self, var cls, size_t offset, const char *method_name) {
    return Type_Method_At_Offset(Type_Of(self), cls, offset, method_name);
}

bool implements_method_at_offset(var self, var cls, size_t offset) {
    return Type_Implements_Method_At_Offset(Type_Of(self), cls, offset);
}

static const char *Size_Name(void) { return "Size"; }

static const char *Size_Brief(void) { return "Type Size"; }

static const char *Size_Description(void) {
    return "The `Size` class is a very important class in MetaDotC because it gives the "
           "size in bytes you can expect an object of a given type to be. This is "
           "used by many methods to allocate, assign, or compare various objects."
           "\n\n"
           "By default this size is automatically found and recorded by the `MetaDotC` "
           "macro, but if the type does it's own allocation, or the size cannot be "
           "found naturally then it may be necessary to override this method.";
}

static const char *Size_Definition(void) {
    return "struct Size {\n"
           "  size_t (*size)(void);\n"
           "};\n";
}

static struct Example *Size_Examples(void) {

    static struct Example examples[] = {{"Usage", "show($I(size(Int)));\n"
                                                  "show($I(size(Float)));\n"
                                                  "show($I(size(Array)));\n"},
                                        {NULL, NULL}};

    return examples;
}

static struct Method *Size_Methods(void) {

    static struct Method methods[] = {{"size", "size_t size(var type);",
                                       "Returns the associated size of a given `type` in bytes."},
                                      {NULL, NULL, NULL}};

    return methods;
}

var Size = MetaDotC(Size, Instance(Doc, Size_Name, Size_Brief, Size_Description, Size_Definition,
                                   Size_Examples, Size_Methods));

size_t size(var type) {

    struct Size *s = type_instance(type, Size);
    if (s and s->size) { return s->size(); }

    return Type_Builtin_Size(type);
}
