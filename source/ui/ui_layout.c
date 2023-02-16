
#include "ui/ui_layout.h"

// 仅为语法方便而使用VECTOR_SIZE注意事项：
//
// 当前布局计算程序的编写方式不是。
// 将受益于SIMD指令的使用。
//
//(使用__vetorcall*传递128位浮点4向量*可能*会得到一些。
// 在非常特定的情况下收益很小，但不太可能值得 Hassle。
//
// 我相信只有在你编译了这个库的时候才需要这个。
// 在复制时阻止编译器使用内联的方式。
// 矩形/大小数据

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Useful math utilities
static METADOT_FORCE_INLINE layout_scalar layout_scalar_max(layout_scalar a, layout_scalar b) { return a > b ? a : b; }
static METADOT_FORCE_INLINE layout_scalar layout_scalar_min(layout_scalar a, layout_scalar b) { return a < b ? a : b; }
static METADOT_FORCE_INLINE float layout_float_max(float a, float b) { return a > b ? a : b; }
static METADOT_FORCE_INLINE float layout_float_min(float a, float b) { return a < b ? a : b; }

void layout_init_context(layout_context *ctx) {
    ctx->capacity = 0;
    ctx->count = 0;
    ctx->items = NULL;
    ctx->rects = NULL;
}

void layout_reserve_items_capacity(layout_context *ctx, layout_id count) {
    if (count >= ctx->capacity) {
        ctx->capacity = count;
        const size_t item_size = sizeof(layout_item_t) + sizeof(layout_vec4);
        ctx->items = (layout_item_t *)realloc(ctx->items, ctx->capacity * item_size);
        const layout_item_t *past_last = ctx->items + ctx->capacity;
        ctx->rects = (layout_vec4 *)past_last;
    }
}

void layout_destroy_context(layout_context *ctx) {
    if (ctx->items != NULL) {
        free(ctx->items);
        ctx->items = NULL;
        ctx->rects = NULL;
    }
}

void layout_reset_context(layout_context *ctx) { ctx->count = 0; }

static void layout_calc_size(layout_context *ctx, layout_id item, int dim);
static void layout_arrange(layout_context *ctx, layout_id item, int dim);

void layout_run_context(layout_context *ctx) {
    METADOT_ASSERT_E(ctx != NULL);

    if (ctx->count > 0) {
        layout_run_item(ctx, 0);
    }
}

void layout_run_item(layout_context *ctx, layout_id item) {
    METADOT_ASSERT_E(ctx != NULL);

    layout_calc_size(ctx, item, 0);
    layout_arrange(ctx, item, 0);
    layout_calc_size(ctx, item, 1);
    layout_arrange(ctx, item, 1);
}

// Alternatively, we could use a flag bit to indicate whether an item's children
// have already been wrapped and may need re-wrapping. If we do that, in the
// future, this would become deprecated and we could make it a no-op.

void layout_clear_item_break(layout_context *ctx, layout_id item) {
    METADOT_ASSERT_E(ctx != NULL);
    layout_item_t *pitem = layout_get_item(ctx, item);
    pitem->flags = pitem->flags & ~(uint32_t)LAYOUT_BREAK;
}

layout_id layout_items_count(layout_context *ctx) {
    METADOT_ASSERT_E(ctx != NULL);
    return ctx->count;
}

layout_id layout_items_capacity(layout_context *ctx) {
    METADOT_ASSERT_E(ctx != NULL);
    return ctx->capacity;
}

layout_id layout_item(layout_context *ctx) {
    layout_id idx = ctx->count++;

    if (idx >= ctx->capacity) {
        ctx->capacity = ctx->capacity < 1 ? 32 : (ctx->capacity * 4);
        const size_t item_size = sizeof(layout_item_t) + sizeof(layout_vec4);
        ctx->items = (layout_item_t *)realloc(ctx->items, ctx->capacity * item_size);
        const layout_item_t *past_last = ctx->items + ctx->capacity;
        ctx->rects = (layout_vec4 *)past_last;
    }

    layout_item_t *item = layout_get_item(ctx, idx);
    // We can either do this here, or when creating/resetting buffer
    memset(item, 0, sizeof(layout_item_t));
    item->first_child = LAYOUT_INVALID_ID;
    item->next_sibling = LAYOUT_INVALID_ID;
    // hmm
    memset(&ctx->rects[idx], 0, sizeof(layout_vec4));
    return idx;
}

static METADOT_FORCE_INLINE void layout_append_by_ptr(layout_item_t *METADOT_RESTRICT pearlier, layout_id later, layout_item_t *METADOT_RESTRICT plater) {
    plater->next_sibling = pearlier->next_sibling;
    plater->flags |= LAYOUT_ITEM_INSERTED;
    pearlier->next_sibling = later;
}

layout_id layout_last_child(const layout_context *ctx, layout_id parent) {
    layout_item_t *pparent = layout_get_item(ctx, parent);
    layout_id child = pparent->first_child;
    if (child == LAYOUT_INVALID_ID) return LAYOUT_INVALID_ID;
    layout_item_t *pchild = layout_get_item(ctx, child);
    layout_id result = child;
    for (;;) {
        layout_id next = pchild->next_sibling;
        if (next == LAYOUT_INVALID_ID) break;
        result = next;
        pchild = layout_get_item(ctx, next);
    }
    return result;
}

void layout_append(layout_context *ctx, layout_id earlier, layout_id later) {
    METADOT_ASSERT_E(later != 0);        // Must not be root item
    METADOT_ASSERT_E(earlier != later);  // Must not be same item id
    layout_item_t *METADOT_RESTRICT pearlier = layout_get_item(ctx, earlier);
    layout_item_t *METADOT_RESTRICT plater = layout_get_item(ctx, later);
    layout_append_by_ptr(pearlier, later, plater);
}

void layout_insert(layout_context *ctx, layout_id parent, layout_id child) {
    METADOT_ASSERT_E(child != 0);       // Must not be root item
    METADOT_ASSERT_E(parent != child);  // Must not be same item id
    layout_item_t *METADOT_RESTRICT pparent = layout_get_item(ctx, parent);
    layout_item_t *METADOT_RESTRICT pchild = layout_get_item(ctx, child);
    METADOT_ASSERT_E(!(pchild->flags & LAYOUT_ITEM_INSERTED));
    // Parent has no existing children, make inserted item the first child.
    if (pparent->first_child == LAYOUT_INVALID_ID) {
        pparent->first_child = child;
        pchild->flags |= LAYOUT_ITEM_INSERTED;
        // Parent has existing items, iterate to find the last child and append the
        // inserted item after it.
    } else {
        layout_id next = pparent->first_child;
        layout_item_t *METADOT_RESTRICT pnext = layout_get_item(ctx, next);
        for (;;) {
            next = pnext->next_sibling;
            if (next == LAYOUT_INVALID_ID) break;
            pnext = layout_get_item(ctx, next);
        }
        layout_append_by_ptr(pnext, child, pchild);
    }
}

void layout_push(layout_context *ctx, layout_id parent, layout_id new_child) {
    METADOT_ASSERT_E(new_child != 0);       // Must not be root item
    METADOT_ASSERT_E(parent != new_child);  // Must not be same item id
    layout_item_t *METADOT_RESTRICT pparent = layout_get_item(ctx, parent);
    layout_id old_child = pparent->first_child;
    layout_item_t *METADOT_RESTRICT pchild = layout_get_item(ctx, new_child);
    METADOT_ASSERT_E(!(pchild->flags & LAYOUT_ITEM_INSERTED));
    pparent->first_child = new_child;
    pchild->flags |= LAYOUT_ITEM_INSERTED;
    pchild->next_sibling = old_child;
}

layout_vec2 layout_get_size(layout_context *ctx, layout_id item) {
    layout_item_t *pitem = layout_get_item(ctx, item);
    return pitem->size;
}

void layout_get_size_xy(layout_context *ctx, layout_id item, layout_scalar *x, layout_scalar *y) {
    layout_item_t *pitem = layout_get_item(ctx, item);
    layout_vec2 size = pitem->size;
    *x = size[0];
    *y = size[1];
}

void layout_set_size(layout_context *ctx, layout_id item, layout_vec2 size) {
    layout_item_t *pitem = layout_get_item(ctx, item);
    pitem->size = size;
    uint32_t flags = pitem->flags;
    if (size[0] == 0)
        flags &= ~(uint32_t)LAYOUT_ITEM_HFIXED;
    else
        flags |= LAYOUT_ITEM_HFIXED;
    if (size[1] == 0)
        flags &= ~(uint32_t)LAYOUT_ITEM_VFIXED;
    else
        flags |= LAYOUT_ITEM_VFIXED;
    pitem->flags = flags;
}

void layout_set_size_xy(layout_context *ctx, layout_id item, layout_scalar width, layout_scalar height) {
    layout_item_t *pitem = layout_get_item(ctx, item);
    pitem->size[0] = width;
    pitem->size[1] = height;
    // Kinda redundant, whatever
    uint32_t flags = pitem->flags;
    if (width == 0)
        flags &= ~(uint32_t)LAYOUT_ITEM_HFIXED;
    else
        flags |= LAYOUT_ITEM_HFIXED;
    if (height == 0)
        flags &= ~(uint32_t)LAYOUT_ITEM_VFIXED;
    else
        flags |= LAYOUT_ITEM_VFIXED;
    pitem->flags = flags;
}

void layout_set_behave(layout_context *ctx, layout_id item, uint32_t flags) {
    METADOT_ASSERT_E((flags & LAYOUT_ITEM_LAYOUT_MASK) == flags);
    layout_item_t *pitem = layout_get_item(ctx, item);
    pitem->flags = (pitem->flags & ~(uint32_t)LAYOUT_ITEM_LAYOUT_MASK) | flags;
}

void layout_set_contain(layout_context *ctx, layout_id item, uint32_t flags) {
    METADOT_ASSERT_E((flags & LAYOUT_ITEM_BOX_MASK) == flags);
    layout_item_t *pitem = layout_get_item(ctx, item);
    pitem->flags = (pitem->flags & ~(uint32_t)LAYOUT_ITEM_BOX_MASK) | flags;
}
void layout_set_margins(layout_context *ctx, layout_id item, layout_vec4 ltrb) {
    layout_item_t *pitem = layout_get_item(ctx, item);
    pitem->margins = ltrb;
}
void layout_set_margins_ltrb(layout_context *ctx, layout_id item, layout_scalar l, layout_scalar t, layout_scalar r, layout_scalar b) {
    layout_item_t *pitem = layout_get_item(ctx, item);
    // Alternative, uses stack and addressed writes
    // pitem->margins = layout_vec4_xyzw(l, t, r, b);
    // Alternative, uses rax and left-shift
    // pitem->margins = (layout_vec4){l, t, r, b};
    // Fewest instructions, but uses more addressed writes?
    pitem->margins[0] = l;
    pitem->margins[1] = t;
    pitem->margins[2] = r;
    pitem->margins[3] = b;
}

layout_vec4 layout_get_margins(layout_context *ctx, layout_id item) { return layout_get_item(ctx, item)->margins; }

void layout_get_margins_ltrb(layout_context *ctx, layout_id item, layout_scalar *l, layout_scalar *t, layout_scalar *r, layout_scalar *b) {
    layout_item_t *pitem = layout_get_item(ctx, item);
    layout_vec4 margins = pitem->margins;
    *l = margins[0];
    *t = margins[1];
    *r = margins[2];
    *b = margins[3];
}

// TODO restrict item ptrs correctly
static METADOT_FORCE_INLINE layout_scalar layout_calc_overlayed_size(layout_context *ctx, layout_id item, int dim) {
    const int wdim = dim + 2;
    layout_item_t *METADOT_RESTRICT pitem = layout_get_item(ctx, item);
    layout_scalar need_size = 0;
    layout_id child = pitem->first_child;
    while (child != LAYOUT_INVALID_ID) {
        layout_item_t *pchild = layout_get_item(ctx, child);
        layout_vec4 rect = ctx->rects[child];
        // width = start margin + calculated width + end margin
        layout_scalar child_size = rect[dim] + rect[2 + dim] + pchild->margins[wdim];
        need_size = layout_scalar_max(need_size, child_size);
        child = pchild->next_sibling;
    }
    return need_size;
}

static METADOT_FORCE_INLINE layout_scalar layout_calc_stacked_size(layout_context *ctx, layout_id item, int dim) {
    const int wdim = dim + 2;
    layout_item_t *METADOT_RESTRICT pitem = layout_get_item(ctx, item);
    layout_scalar need_size = 0;
    layout_id child = pitem->first_child;
    while (child != LAYOUT_INVALID_ID) {
        layout_item_t *pchild = layout_get_item(ctx, child);
        layout_vec4 rect = ctx->rects[child];
        need_size += rect[dim] + rect[2 + dim] + pchild->margins[wdim];
        child = pchild->next_sibling;
    }
    return need_size;
}

static METADOT_FORCE_INLINE layout_scalar layout_calc_wrapped_overlayed_size(layout_context *ctx, layout_id item, int dim) {
    const int wdim = dim + 2;
    layout_item_t *METADOT_RESTRICT pitem = layout_get_item(ctx, item);
    layout_scalar need_size = 0;
    layout_scalar need_size2 = 0;
    layout_id child = pitem->first_child;
    while (child != LAYOUT_INVALID_ID) {
        layout_item_t *pchild = layout_get_item(ctx, child);
        layout_vec4 rect = ctx->rects[child];
        if (pchild->flags & LAYOUT_BREAK) {
            need_size2 += need_size;
            need_size = 0;
        }
        layout_scalar child_size = rect[dim] + rect[2 + dim] + pchild->margins[wdim];
        need_size = layout_scalar_max(need_size, child_size);
        child = pchild->next_sibling;
    }
    return need_size2 + need_size;
}

// Equivalent to uiComputeWrappedStackedSize
static METADOT_FORCE_INLINE layout_scalar layout_calc_wrapped_stacked_size(layout_context *ctx, layout_id item, int dim) {
    const int wdim = dim + 2;
    layout_item_t *METADOT_RESTRICT pitem = layout_get_item(ctx, item);
    layout_scalar need_size = 0;
    layout_scalar need_size2 = 0;
    layout_id child = pitem->first_child;
    while (child != LAYOUT_INVALID_ID) {
        layout_item_t *pchild = layout_get_item(ctx, child);
        layout_vec4 rect = ctx->rects[child];
        if (pchild->flags & LAYOUT_BREAK) {
            need_size2 = layout_scalar_max(need_size2, need_size);
            need_size = 0;
        }
        need_size += rect[dim] + rect[2 + dim] + pchild->margins[wdim];
        child = pchild->next_sibling;
    }
    return layout_scalar_max(need_size2, need_size);
}

static void layout_calc_size(layout_context *ctx, layout_id item, int dim) {
    layout_item_t *pitem = layout_get_item(ctx, item);

    layout_id child = pitem->first_child;
    while (child != LAYOUT_INVALID_ID) {
        // NOTE: this is recursive and will run out of stack space if items are
        // nested too deeply.
        layout_calc_size(ctx, child, dim);
        layout_item_t *pchild = layout_get_item(ctx, child);
        child = pchild->next_sibling;
    }

    // Set the mutable rect output data to the starting input data
    ctx->rects[item][dim] = pitem->margins[dim];

    // If we have an explicit input size, just set our output size (which other
    // calc_size and arrange procedures will use) to it.
    if (pitem->size[dim] != 0) {
        ctx->rects[item][2 + dim] = pitem->size[dim];
        return;
    }

    // Calculate our size based on children items. Note that we've already
    // called layout_calc_size on our children at this point.
    layout_scalar cal_size;
    switch (pitem->flags & LAYOUT_ITEM_BOX_MODEL_MASK) {
        case LAYOUT_COLUMN | LAYOUT_WRAP:
            // flex model
            if (dim)  // direction
                cal_size = layout_calc_stacked_size(ctx, item, 1);
            else
                cal_size = layout_calc_overlayed_size(ctx, item, 0);
            break;
        case LAYOUT_ROW | LAYOUT_WRAP:
            // flex model
            if (!dim)  // direction
                cal_size = layout_calc_wrapped_stacked_size(ctx, item, 0);
            else
                cal_size = layout_calc_wrapped_overlayed_size(ctx, item, 1);
            break;
        case LAYOUT_COLUMN:
        case LAYOUT_ROW:
            // flex model
            if ((pitem->flags & 1) == (uint32_t)dim)  // direction
                cal_size = layout_calc_stacked_size(ctx, item, dim);
            else
                cal_size = layout_calc_overlayed_size(ctx, item, dim);
            break;
        default:
            // layout model
            cal_size = layout_calc_overlayed_size(ctx, item, dim);
            break;
    }

    // Set our output data size. Will be used by parent calc_size procedures.,
    // and by arrange procedures.
    ctx->rects[item][2 + dim] = cal_size;
}

static METADOT_FORCE_INLINE void layout_arrange_stacked(layout_context *ctx, layout_id item, int dim, bool wrap) {
    const int wdim = dim + 2;
    layout_item_t *pitem = layout_get_item(ctx, item);

    const uint32_t item_flags = pitem->flags;
    layout_vec4 rect = ctx->rects[item];
    layout_scalar space = rect[2 + dim];

    float max_x2 = (float)(rect[dim] + space);

    layout_id start_child = pitem->first_child;
    while (start_child != LAYOUT_INVALID_ID) {
        layout_scalar used = 0;
        uint32_t count = 0;           // count of fillers
        uint32_t squeezed_count = 0;  // count of squeezable elements
        uint32_t total = 0;
        bool hardbreak = false;
        // first pass: count items that need to be expanded,
        // and the space that is used
        layout_id child = start_child;
        layout_id end_child = LAYOUT_INVALID_ID;
        while (child != LAYOUT_INVALID_ID) {
            layout_item_t *pchild = layout_get_item(ctx, child);
            const uint32_t child_flags = pchild->flags;
            const uint32_t flags = (child_flags & LAYOUT_ITEM_LAYOUT_MASK) >> dim;
            const uint32_t fflags = (child_flags & LAYOUT_ITEM_FIXED_MASK) >> dim;
            const layout_vec4 child_margins = pchild->margins;
            layout_vec4 child_rect = ctx->rects[child];
            layout_scalar extend = used;
            if ((flags & LAYOUT_HFILL) == LAYOUT_HFILL) {
                ++count;
                extend += child_rect[dim] + child_margins[wdim];
            } else {
                if ((fflags & LAYOUT_ITEM_HFIXED) != LAYOUT_ITEM_HFIXED) ++squeezed_count;
                extend += child_rect[dim] + child_rect[2 + dim] + child_margins[wdim];
            }
            // wrap on end of line or manual flag
            if (wrap && (total && ((extend > space) || (child_flags & LAYOUT_BREAK)))) {
                end_child = child;
                hardbreak = (child_flags & LAYOUT_BREAK) == LAYOUT_BREAK;
                // add marker for subsequent queries
                pchild->flags = child_flags | LAYOUT_BREAK;
                break;
            } else {
                used = extend;
                child = pchild->next_sibling;
            }
            ++total;
        }

        layout_scalar extra_space = space - used;
        float filler = 0.0f;
        float spacer = 0.0f;
        float extra_margin = 0.0f;
        float eater = 0.0f;

        if (extra_space > 0) {
            if (count > 0)
                filler = (float)extra_space / (float)count;
            else if (total > 0) {
                switch (item_flags & LAYOUT_JUSTIFY) {
                    case LAYOUT_JUSTIFY:
                        // justify when not wrapping or not in last line,
                        // or not manually breaking
                        if (!wrap || ((end_child != LAYOUT_INVALID_ID) && !hardbreak)) spacer = (float)extra_space / (float)(total - 1);
                        break;
                    case LAYOUT_START:
                        break;
                    case LAYOUT_END:
                        extra_margin = extra_space;
                        break;
                    default:
                        extra_margin = extra_space / 2.0f;
                        break;
                }
            }
        }
#ifdef METADOT_FLOAT
        // In floating point, it's possible to end up with some small negative
        // value for extra_space, while also have a 0.0 squeezed_count. This
        // would cause divide by zero. Instead, we'll check to see if
        // squeezed_count is > 0. I believe this produces the same results as
        // the original oui int-only code. However, I don't have any tests for
        // it, so I'll leave it if-def'd for now.
        else if (!wrap && (squeezed_count > 0))
#else
        // This is the original oui code
        else if (!wrap && (extra_space < 0))
#endif
            eater = (float)extra_space / (float)squeezed_count;

        // distribute width among items
        float x = (float)rect[dim];
        float x1;
        // second pass: distribute and rescale
        child = start_child;
        while (child != end_child) {
            layout_scalar ix0, ix1;
            layout_item_t *pchild = layout_get_item(ctx, child);
            const uint32_t child_flags = pchild->flags;
            const uint32_t flags = (child_flags & LAYOUT_ITEM_LAYOUT_MASK) >> dim;
            const uint32_t fflags = (child_flags & LAYOUT_ITEM_FIXED_MASK) >> dim;
            const layout_vec4 child_margins = pchild->margins;
            layout_vec4 child_rect = ctx->rects[child];

            x += (float)child_rect[dim] + extra_margin;
            if ((flags & LAYOUT_HFILL) == LAYOUT_HFILL)  // grow
                x1 = x + filler;
            else if ((fflags & LAYOUT_ITEM_HFIXED) == LAYOUT_ITEM_HFIXED)
                x1 = x + (float)child_rect[2 + dim];
            else  // squeeze
                x1 = x + layout_float_max(0.0f, (float)child_rect[2 + dim] + eater);

            ix0 = (layout_scalar)x;
            if (wrap)
                ix1 = (layout_scalar)layout_float_min(max_x2 - (float)child_margins[wdim], x1);
            else
                ix1 = (layout_scalar)x1;
            child_rect[dim] = ix0;            // pos
            child_rect[dim + 2] = ix1 - ix0;  // size
            ctx->rects[child] = child_rect;
            x = x1 + (float)child_margins[wdim];
            child = pchild->next_sibling;
            extra_margin = spacer;
        }

        start_child = end_child;
    }
}

static METADOT_FORCE_INLINE void layout_arrange_overlay(layout_context *ctx, layout_id item, int dim) {
    const int wdim = dim + 2;
    layout_item_t *pitem = layout_get_item(ctx, item);
    const layout_vec4 rect = ctx->rects[item];
    const layout_scalar offset = rect[dim];
    const layout_scalar space = rect[2 + dim];

    layout_id child = pitem->first_child;
    while (child != LAYOUT_INVALID_ID) {
        layout_item_t *pchild = layout_get_item(ctx, child);
        const uint32_t b_flags = (pchild->flags & LAYOUT_ITEM_LAYOUT_MASK) >> dim;
        const layout_vec4 child_margins = pchild->margins;
        layout_vec4 child_rect = ctx->rects[child];

        switch (b_flags & LAYOUT_HFILL) {
            case LAYOUT_HCENTER:
                child_rect[dim] += (space - child_rect[2 + dim]) / 2 - child_margins[wdim];
                break;
            case LAYOUT_RIGHT:
                child_rect[dim] += space - child_rect[2 + dim] - child_margins[dim] - child_margins[wdim];
                break;
            case LAYOUT_HFILL:
                child_rect[2 + dim] = layout_scalar_max(0, space - child_rect[dim] - child_margins[wdim]);
                break;
            default:
                break;
        }

        child_rect[dim] += offset;
        ctx->rects[child] = child_rect;
        child = pchild->next_sibling;
    }
}

static METADOT_FORCE_INLINE void layout_arrange_overlayout_squeezed_range(layout_context *ctx, int dim, layout_id start_item, layout_id end_item, layout_scalar offset, layout_scalar space) {
    int wdim = dim + 2;
    layout_id item = start_item;
    while (item != end_item) {
        layout_item_t *pitem = layout_get_item(ctx, item);
        const uint32_t b_flags = (pitem->flags & LAYOUT_ITEM_LAYOUT_MASK) >> dim;
        const layout_vec4 margins = pitem->margins;
        layout_vec4 rect = ctx->rects[item];
        layout_scalar min_size = layout_scalar_max(0, space - rect[dim] - margins[wdim]);
        switch (b_flags & LAYOUT_HFILL) {
            case LAYOUT_HCENTER:
                rect[2 + dim] = layout_scalar_min(rect[2 + dim], min_size);
                rect[dim] += (space - rect[2 + dim]) / 2 - margins[wdim];
                break;
            case LAYOUT_RIGHT:
                rect[2 + dim] = layout_scalar_min(rect[2 + dim], min_size);
                rect[dim] = space - rect[2 + dim] - margins[wdim];
                break;
            case LAYOUT_HFILL:
                rect[2 + dim] = min_size;
                break;
            default:
                rect[2 + dim] = layout_scalar_min(rect[2 + dim], min_size);
                break;
        }
        rect[dim] += offset;
        ctx->rects[item] = rect;
        item = pitem->next_sibling;
    }
}

static METADOT_FORCE_INLINE layout_scalar layout_arrange_wrapped_overlayout_squeezed(layout_context *ctx, layout_id item, int dim) {
    const int wdim = dim + 2;
    layout_item_t *pitem = layout_get_item(ctx, item);
    layout_scalar offset = ctx->rects[item][dim];
    layout_scalar need_size = 0;
    layout_id child = pitem->first_child;
    layout_id start_child = child;
    while (child != LAYOUT_INVALID_ID) {
        layout_item_t *pchild = layout_get_item(ctx, child);
        if (pchild->flags & LAYOUT_BREAK) {
            layout_arrange_overlayout_squeezed_range(ctx, dim, start_child, child, offset, need_size);
            offset += need_size;
            start_child = child;
            need_size = 0;
        }
        const layout_vec4 rect = ctx->rects[child];
        layout_scalar child_size = rect[dim] + rect[2 + dim] + pchild->margins[wdim];
        need_size = layout_scalar_max(need_size, child_size);
        child = pchild->next_sibling;
    }
    layout_arrange_overlayout_squeezed_range(ctx, dim, start_child, LAYOUT_INVALID_ID, offset, need_size);
    offset += need_size;
    return offset;
}

static void layout_arrange(layout_context *ctx, layout_id item, int dim) {
    layout_item_t *pitem = layout_get_item(ctx, item);

    const uint32_t flags = pitem->flags;
    switch (flags & LAYOUT_ITEM_BOX_MODEL_MASK) {
        case LAYOUT_COLUMN | LAYOUT_WRAP:
            if (dim != 0) {
                layout_arrange_stacked(ctx, item, 1, true);
                layout_scalar offset = layout_arrange_wrapped_overlayout_squeezed(ctx, item, 0);
                ctx->rects[item][2 + 0] = offset - ctx->rects[item][0];
            }
            break;
        case LAYOUT_ROW | LAYOUT_WRAP:
            if (dim == 0)
                layout_arrange_stacked(ctx, item, 0, true);
            else
                // discard return value
                layout_arrange_wrapped_overlayout_squeezed(ctx, item, 1);
            break;
        case LAYOUT_COLUMN:
        case LAYOUT_ROW:
            if ((flags & 1) == (uint32_t)dim) {
                layout_arrange_stacked(ctx, item, dim, false);
            } else {
                const layout_vec4 rect = ctx->rects[item];
                layout_arrange_overlayout_squeezed_range(ctx, dim, pitem->first_child, LAYOUT_INVALID_ID, rect[dim], rect[2 + dim]);
            }
            break;
        default:
            layout_arrange_overlay(ctx, item, dim);
            break;
    }
    layout_id child = pitem->first_child;
    while (child != LAYOUT_INVALID_ID) {
        // NOTE: this is recursive and will run out of stack space if items are
        // nested too deeply.
        layout_arrange(ctx, child, dim);
        layout_item_t *pchild = layout_get_item(ctx, child);
        child = pchild->next_sibling;
    }
}