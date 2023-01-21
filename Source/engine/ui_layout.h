#ifndef METADOT_INCLUDE_HEADER
#define METADOT_INCLUDE_HEADER

#include <stdint.h>

#include "core/core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t layout_id;
#if LAYOUT_FLOAT == 1
typedef float layout_scalar;
#else
typedef int16_t layout_scalar;
#endif

#define LAYOUT_INVALID_ID UINT32_MAX

// GCC and Clang allow us to create vectors based on a type with the
// vector_size extension. This will allow us to access individual components of
// the vector via indexing operations.
#if defined(__GNUC__) || defined(__clang__)

// Using floats for coordinates takes up more space than using int16. 128 bits
// for a four-component vector.
#ifdef LAYOUT_FLOAT
typedef float layout_vec4 __attribute__((__vector_size__(16), aligned(4)));
typedef float layout_vec2 __attribute__((__vector_size__(8), aligned(4)));
// Integer version uses 64 bits for a four-component vector.
#else
typedef int16_t layout_vec4 __attribute__((__vector_size__(8), aligned(2)));
typedef int16_t layout_vec2 __attribute__((__vector_size__(4), aligned(2)));
#endif  // LAYOUT_FLOAT

// Note that we're not actually going to make any explicit use of any
// platform's SIMD instructions -- we're just using the vector extension for
// more convenient syntax. Therefore, we can specify more relaxed alignment
// requirements. See the end of this file for some notes about this.

// MSVC doesn't have the vetor_size attribute, but we want convenient indexing
// operators for our layout logic code. Therefore, we force C++ compilation in
// MSVC, and use C++ operator overloading.
#elif defined(_MSC_VER)
struct layout_vec4 {
    layout_scalar xyzw[4];
    const layout_scalar &operator[](int index) const { return xyzw[index]; }
    layout_scalar &operator[](int index) { return xyzw[index]; }
};
struct layout_vec2 {
    layout_scalar xy[2];
    const layout_scalar &operator[](int index) const { return xy[index]; }
    layout_scalar &operator[](int index) { return xy[index]; }
};
#endif  // __GNUC__/__clang__ or _MSC_VER

typedef struct layout_item_t {
    uint32_t flags;
    layout_id first_child;
    layout_id next_sibling;
    layout_vec4 margins;
    layout_vec2 size;
} layout_item_t;

typedef struct layout_context {
    layout_item_t *items;
    layout_vec4 *rects;
    layout_id capacity;
    layout_id count;
} layout_context;

// Container flags to pass to layout_set_container()
typedef enum layout_box_flags {
    // flex-direction (bit 0+1)

    // left to right
    LAYOUT_ROW = 0x002,
    // top to bottom
    LAYOUT_COLUMN = 0x003,

    // model (bit 1)

    // free layout
    LAYOUT_LAYOUT = 0x000,
    // flex model
    LAYOUT_FLEX = 0x002,

    // flex-wrap (bit 2)

    // single-line
    LAYOUT_NOWRAP = 0x000,
    // multi-line, wrap left to right
    LAYOUT_WRAP = 0x004,

    // justify-content (start, end, center, space-between)
    // at start of row/column
    LAYOUT_START = 0x008,
    // at center of row/column
    LAYOUT_MIDDLE = 0x000,
    // at end of row/column
    LAYOUT_END = 0x010,
    // insert spacing to stretch across whole row/column
    LAYOUT_JUSTIFY = 0x018

    // align-items
    // can be implemented by putting a flex container in a layout container,
    // then using LAYOUT_TOP, LAYOUT_BOTTOM, LAYOUT_VFILL, LAYOUT_VCENTER, etc.
    // FILL is equivalent to stretch/grow

    // align-content (start, end, center, stretch)
    // can be implemented by putting a flex container in a layout container,
    // then using LAYOUT_TOP, LAYOUT_BOTTOM, LAYOUT_VFILL, LAYOUT_VCENTER, etc.
    // FILL is equivalent to stretch; space-between is not supported.
} layout_box_flags;

// child layout flags to pass to layout_set_behave()
typedef enum layout_layout_flags {
    // attachments (bit 5-8)
    // fully valid when parent uses LAYOUT_LAYOUT model
    // partially valid when in LAYOUT_FLEX model

    // anchor to left item or left side of parent
    LAYOUT_LEFT = 0x020,
    // anchor to top item or top side of parent
    LAYOUT_TOP = 0x040,
    // anchor to right item or right side of parent
    LAYOUT_RIGHT = 0x080,
    // anchor to bottom item or bottom side of parent
    LAYOUT_BOTTOM = 0x100,
    // anchor to both left and right item or parent borders
    LAYOUT_HFILL = 0x0a0,
    // anchor to both top and bottom item or parent borders
    LAYOUT_VFILL = 0x140,
    // center horizontally, with left margin as offset
    LAYOUT_HCENTER = 0x000,
    // center vertically, with top margin as offset
    LAYOUT_VCENTER = 0x000,
    // center in both directions, with left/top margin as offset
    LAYOUT_CENTER = 0x000,
    // anchor to all four directions
    LAYOUT_FILL = 0x1e0,
    // When in a wrapping container, put this element on a new line. Wrapping
    // layout code auto-inserts LAYOUT_BREAK flags as needed. See GitHub issues for
    // TODO related to this.
    //
    // Drawing routines can read this via item pointers as needed after
    // performing layout calculations.
    LAYOUT_BREAK = 0x200
} layout_layout_flags;

enum {
    // these bits, starting at bit 16, can be safely assigned by the
    // application, e.g. as item types, other event types, drop targets, etc.
    // this is not yet exposed via API functions, you'll need to get/set these
    // by directly accessing item pointers.
    //
    // (In reality we have more free bits than this, TODO)
    //
    // TODO fix int/unsigned size mismatch (clang issues warning for this),
    // should be all bits as 1 instead of INT_MAX
    LAYOUT_USERMASK = 0x7fff0000,

    // a special mask passed to layout_find_item() (currently does not exist, was
    // not ported from oui)
    LAYOUT_ANY = 0x7fffffff
};

enum {
    // extra item flags

    // bit 0-2
    LAYOUT_ITEM_BOX_MODEL_MASK = 0x000007,
    // bit 0-4
    LAYOUT_ITEM_BOX_MASK = 0x00001F,
    // bit 5-9
    LAYOUT_ITEM_LAYOUT_MASK = 0x0003E0,
    // item has been inserted (bit 10)
    LAYOUT_ITEM_INSERTED = 0x400,
    // horizontal size has been explicitly set (bit 11)
    LAYOUT_ITEM_HFIXED = 0x800,
    // vertical size has been explicitly set (bit 12)
    LAYOUT_ITEM_VFIXED = 0x1000,
    // bit 11-12
    LAYOUT_ITEM_FIXED_MASK = LAYOUT_ITEM_HFIXED | LAYOUT_ITEM_VFIXED,

    // which flag bits will be compared
    LAYOUT_ITEM_COMPARE_MASK = LAYOUT_ITEM_BOX_MODEL_MASK | (LAYOUT_ITEM_LAYOUT_MASK & ~LAYOUT_BREAK) | LAYOUT_USERMASK
};

static_inline layout_vec4 layout_vec4_xyzw(layout_scalar x, layout_scalar y, layout_scalar z, layout_scalar w) {
#if (defined(__GNUC__) || defined(__clang__)) && !defined(__cplusplus)
    return (layout_vec4){x, y, z, w};
#else
    layout_vec4 result;
    result[0] = x;
    result[1] = y;
    result[2] = z;
    result[3] = w;
    return result;
#endif
}

// Call this on a context before using it. You must also call this on a context
// if you would like to use it again after calling layout_destroy_context() on it.
void layout_init_context(layout_context *ctx);

// Reserve enough heap memory to contain `count` items without needing to
// reallocate. The initial layout_init_context() call does not allocate any heap
// memory, so if you init a context and then call this once with a large enough
// number for the number of items you'll create, there will not be any further
// reallocations.
void layout_reserve_items_capacity(layout_context *ctx, layout_id count);

// Frees any heap allocated memory used by a context. Don't call this on a
// context that did not have layout_init_context() call on it. To reuse a context
// after destroying it, you will need to call layout_init_context() on it again.
void layout_destroy_context(layout_context *ctx);

// Clears all of the items in a context, setting its count to 0. Use this when
// you want to re-declare your layout starting from the root item. This does not
// free any memory or perform allocations. It's safe to use the context again
// after calling this. You should probably use this instead of init/destroy if
// you are recalculating your layouts in a loop.
void layout_reset_context(layout_context *ctx);

// Performs the layout calculations, starting at the root item (id 0). After
// calling this, you can use layout_get_rect() to query for an item's calculated
// rectangle. If you use procedures such as layout_append() or layout_insert() after
// calling this, your calculated data may become invalid if a reallocation
// occurs.
//
// You should prefer to recreate your items starting from the root instead of
// doing fine-grained updates to the existing context.
//
// However, it's safe to use layout_set_size on an item, and then re-run
// layout_run_context. This might be useful if you are doing a resizing animation
// on items in a layout without any contents changing.
void layout_run_context(layout_context *ctx);

// Like layout_run_context(), this procedure will run layout calculations --
// however, it lets you specify which item you want to start from.
// layout_run_context() always starts with item 0, the first item, as the root.
// Running the layout calculations from a specific item is useful if you want
// need to iteratively re-run parts of your layout hierarchy, or if you are only
// interested in updating certain subsets of it. Be careful when using this --
// it's easy to generated bad output if the parent items haven't yet had their
// output rectangles calculated, or if they've been invalidated (e.g. due to
// re-allocation).
void layout_run_item(layout_context *ctx, layout_id item);

// Performing a layout on items where wrapping is enabled in the parent
// container can cause flags to be modified during the calculations. If you plan
// to call layout_run_context or layout_run_item multiple times without calling
// layout_reset, and if you have a container that uses wrapping, and if the width
// or height of the container may have changed, you should call
// layout_clear_item_break on all of the children of a container before calling
// layout_run_context or layout_run_item again. If you don't, the layout calculations
// may perform unnecessary wrapping.
//
// This requirement may be changed in the future.
//
// Calling this will also reset any manually-specified breaking. You will need
// to set the manual breaking again, or simply not call this on any items that
// you know you wanted to break manually.
//
// If you clear your context every time you calculate your layout, or if you
// don't use wrapping, you don't need to call this.
void layout_clear_item_break(layout_context *ctx, layout_id item);

// Returns the number of items that have been created in a context.
layout_id layout_items_count(layout_context *ctx);

// Returns the number of items the context can hold without performing a
// reallocation.
layout_id layout_items_capacity(layout_context *ctx);

// Create a new item, which can just be thought of as a rectangle. Returns the
// id (handle) used to identify the item.
layout_id layout_item(layout_context *ctx);

// Inserts an item into another item, forming a parent - child relationship. An
// item can contain any number of child items. Items inserted into a parent are
// put at the end of the ordering, after any existing siblings.
void layout_insert(layout_context *ctx, layout_id parent, layout_id child);

// layout_append inserts an item as a sibling after another item. This allows
// inserting an item into the middle of an existing list of items within a
// parent. It's also more efficient than repeatedly using layout_insert(ctx,
// parent, new_child) in a loop to create a list of items in a parent, because
// it does not need to traverse the parent's children each time. So if you're
// creating a long list of children inside of a parent, you might prefer to use
// this after using layout_insert to insert the first child.
void layout_append(layout_context *ctx, layout_id earlier, layout_id later);

// Like layout_insert, but puts the new item as the first child in a parent instead
// of as the last.
void layout_push(layout_context *ctx, layout_id parent, layout_id child);

// Gets the size that was set with layout_set_size or layout_set_size_xy. The _xy
// version writes the output values to the specified addresses instead of
// returning the values in a layout_vec2.
layout_vec2 layout_get_size(layout_context *ctx, layout_id item);
void layout_get_size_xy(layout_context *ctx, layout_id item, layout_scalar *x, layout_scalar *y);

// Sets the size of an item. The _xy version passes the width and height as
// separate arguments, but functions the same.
void layout_set_size(layout_context *ctx, layout_id item, layout_vec2 size);
void layout_set_size_xy(layout_context *ctx, layout_id item, layout_scalar width, layout_scalar height);

// Set the flags on an item which determines how it behaves as a parent. For
// example, setting LAYOUT_COLUMN will make an item behave as if it were a column
// -- it will lay out its children vertically.
void layout_set_contain(layout_context *ctx, layout_id item, uint32_t flags);

// Set the flags on an item which determines how it behaves as a child inside of
// a parent item. For example, setting LAYOUT_VFILL will make an item try to fill
// up all available vertical space inside of its parent.
void layout_set_behave(layout_context *ctx, layout_id item, uint32_t flags);

// Get the margins that were set by layout_set_margins. The _ltrb version writes
// the output values to the specified addresses instead of returning the values
// in a layout_vec4.
// l: left, t: top, r: right, b: bottom
layout_vec4 layout_get_margins(layout_context *ctx, layout_id item);
void layout_get_margins_ltrb(layout_context *ctx, layout_id item, layout_scalar *l, layout_scalar *t, layout_scalar *r, layout_scalar *b);

// Set the margins on an item. The components of the vector are:
// 0: left, 1: top, 2: right, 3: bottom.
void layout_set_margins(layout_context *ctx, layout_id item, layout_vec4 ltrb);

// Same as layout_set_margins, but the components are passed as separate arguments
// (left, top, right, bottom).
void layout_set_margins_ltrb(layout_context *ctx, layout_id item, layout_scalar l, layout_scalar t, layout_scalar r, layout_scalar b);

// Get the pointer to an item in the buffer by its id. Don't keep this around --
// it will become invalid as soon as any reallocation occurs. Just store the id
// instead (it's smaller, anyway, and the lookup cost will be nothing.)
static_inline layout_item_t *layout_get_item(const layout_context *ctx, layout_id id) {
    METADOT_ASSERT_E(id != LAYOUT_INVALID_ID && id < ctx->count);
    return ctx->items + id;
}

// Get the id of first child of an item, if any. Returns LAYOUT_INVALID_ID if there
// is no child.
static_inline layout_id layout_first_child(const layout_context *ctx, layout_id id) {
    const layout_item_t *pitem = layout_get_item(ctx, id);
    return pitem->first_child;
}

// Get the id of the next sibling of an item, if any. Returns LAYOUT_INVALID_ID if
// there is no next sibling.
static_inline layout_id layout_next_sibling(const layout_context *ctx, layout_id id) {
    const layout_item_t *pitem = layout_get_item(ctx, id);
    return pitem->next_sibling;
}

// Returns the calculated rectangle of an item. This is only valid after calling
// layout_run_context and before any other reallocation occurs. Otherwise, the
// result will be undefined. The vector components are:
// 0: x starting position, 1: y starting position
// 2: width, 3: height
static_inline layout_vec4 layout_get_rect(const layout_context *ctx, layout_id id) {
    METADOT_ASSERT_E(id != LAYOUT_INVALID_ID && id < ctx->count);
    return ctx->rects[id];
}

// The same as layout_get_rect, but writes the x,y positions and width,height
// values to the specified addresses instead of returning them in a layout_vec4.
static_inline void layout_get_rect_xywh(const layout_context *ctx, layout_id id, layout_scalar *x, layout_scalar *y, layout_scalar *width, layout_scalar *height) {
    METADOT_ASSERT_E(id != LAYOUT_INVALID_ID && id < ctx->count);
    layout_vec4 rect = ctx->rects[id];
    *x = rect[0];
    *y = rect[1];
    *width = rect[2];
    *height = rect[3];
}

#ifdef __cplusplus
}
#endif

#endif