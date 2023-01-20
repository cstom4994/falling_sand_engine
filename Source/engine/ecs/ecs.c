#include "ecs.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine/scripting/lua_wrapper.h"

#define ENTITY_INIT_SIZE 4096

void entity_id_deinit(struct entity_id *e) {
    free(e->id);
    e->id = NULL;
}

static int find_eid_(struct entity_id *e, uint64_t eid, int begin, int end) {
    const uint64_t *id = e->id;
    while (begin < end) {
        int mid = (begin + end) / 2;
        if (eid == id[mid]) {
            return mid;
        }
        if (eid < id[mid])
            end = mid;
        else
            begin = mid + 1;
    }
    int p = begin > e->n / 2 ? begin : begin + 1;
    return -p;
}

int entity_id_find_guessrange(struct entity_id *e, uint64_t eid, int begin, int end) {
    if (end >= e->n) {
        return find_eid_(e, eid, begin, e->n);
    } else {
        uint64_t end_id = e->id[end];
        if (eid == end_id) return end;
        if (eid > end_id) return find_eid_(e, eid, end + 1, e->n);
        return find_eid_(e, eid, begin, end);
    }
}

int entity_id_find(struct entity_id *e, uint64_t eid) {
    unsigned h = (unsigned)(2654435761 * (uint32_t)eid) % ENTITY_ID_LOOKUP;
    entity_index_t p = e->lookup[h];
    int index = index_(p);
    int begin = 0;
    int end;
    if (index >= e->n) {
        end = e->n;
    } else {
        uint64_t v = e->id[index];
        if (v == eid) {
            return index;
        }
        if (v > eid) {
            end = index;
        } else {
            begin = index + 1;
            end = e->n;
        }
    }
    index = find_eid_(e, eid, begin, end);
    if (index < 0) {
        e->lookup[h] = make_index_(-index);
        return -1;
    } else {
        e->lookup[h] = make_index_(index);
        return index;
    }
}

int entity_id_find_last(struct entity_id *e, uint64_t eid) {
    if (e->n == 0) return -1;
    int n = e->n - 1;
    if (e->id[n] == eid) return n;
    if (e->id[n] < eid) return -1;
    return find_eid_(e, eid, 0, n);
}

size_t entity_id_memsize(struct entity_id *e) { return sizeof(uint64_t) * e->cap; }

int entity_id_alloc(struct entity_id *e, uint64_t *eid) {
    int n = e->n;
    if (n >= MAX_ENTITY) {
        return -1;
    }
    e->n++;

    if (n >= e->cap) {
        if (e->id == NULL) {
            e->cap = ENTITY_INIT_SIZE;
            e->id = (uint64_t *)malloc(e->cap * sizeof(uint64_t));
        } else {
            int newcap = e->cap * 3 / 2 + 1;
            e->id = (uint64_t *)realloc(e->id, newcap * sizeof(uint64_t));
            e->cap = newcap;
        }
    }
    *eid = ++e->last_id;
    e->id[n] = *eid;

    return n;
}

static void remove_dup(struct component_pool *c, int index) {
    int i;
    entity_index_t eid = c->id[index];
    int to = index;
    for (i = index + 1; i < c->n; i++) {
        if (ENTITY_INDEX_CMP(c->id[i], eid) != 0) {
            eid = c->id[i];
            c->id[to] = eid;
            ++to;
        }
    }
    c->n = to;
}

void *entity_iter_(struct entity_world *w, int cid, int index) {
    if (cid < 0) {
        assert(cid == ENTITYID_TAG);
        if (index >= w->eid.n) return NULL;
        return (void *)w->eid.id[index];
    }
    struct component_pool *c = &w->c[cid];
    assert(index >= 0);
    if (index >= c->n) return NULL;
    if (c->stride == STRIDE_TAG) {
        // it's a tag
        entity_index_t eid = c->id[index];
        if (index < c->n - 1 && ENTITY_INDEX_CMP(eid, c->id[index + 1]) == 0) {
            remove_dup(c, index + 1);
        }
        return DUMMY_PTR;
    }
    return get_ptr(c, index);
}

void entity_clear_type_(struct entity_world *w, int cid) {
    struct component_pool *c = &w->c[cid];
    c->n = 0;
}

static inline entity_index_t get_index_(struct entity_world *w, int cid, int index) {
    if (cid < 0) {
        return make_index_(index);
    } else {
        struct component_pool *c = &w->c[cid];
        assert(index >= 0 && index < c->n);
        return c->id[index];
    }
}

int entity_sibling_index_(struct entity_world *w, int cid, int index, int silbling_id) {
    entity_index_t eid;
    if (cid < 0) {
        assert(cid == ENTITYID_TAG);
        eid = make_index_(index);
    } else {
        struct component_pool *c = &w->c[cid];
        if (index < 0 || index >= c->n) return 0;
        eid = c->id[index];
    }
    if (silbling_id < 0) {
        assert(silbling_id == ENTITYID_TAG);
        return index_(eid) + 1;
    }
    struct component_pool *c = &w->c[silbling_id];
    int result_index = ecs_lookup_component_(c, eid, c->last_lookup);
    if (result_index >= 0) {
        c->last_lookup = result_index;
        return result_index + 1;
    }
    return 0;
}

static void *add_component_(struct entity_world *w, int cid, entity_index_t eid, const void *buffer) {
    int index = ecs_add_component_id_(w, cid, eid);
    if (index < 0) return NULL;
    struct component_pool *pool = &w->c[cid];
    void *ret = get_ptr(pool, index);
    if (buffer) {
        assert(pool->stride >= 0);
        memcpy(ret, buffer, pool->stride);
    }
    return ret;
}

void *entity_add_sibling_(struct entity_world *w, int cid, int index, int silbling_id, const void *buffer) {
    entity_index_t eid = get_index_(w, cid, index);
    // todo: pcall add_component_
    return add_component_(w, silbling_id, eid, buffer);
}

int entity_new_(struct entity_world *w, int cid, const void *buffer) {
    entity_index_t eid = ecs_new_entityid_(w);
    if (INVALID_ENTITY_INDEX(eid)) {
        return -1;
    }
    struct component_pool *c = &w->c[cid];
    assert(c->cap > 0);
    if (buffer == NULL) {
        return ecs_add_component_id_(w, cid, eid);
    } else {
        assert(c->stride >= 0);
        int index = ecs_add_component_id_(w, cid, eid);
        assert(index >= 0);
        void *ret = get_ptr(c, index);
        memcpy(ret, buffer, c->stride);
        return index;
    }
}

void entity_remove_(struct entity_world *w, int cid, int index) { entity_enable_tag_(w, cid, index, ENTITY_REMOVED); }

static void insert_id(struct entity_world *w, int cid, entity_index_t eindex) {
    struct component_pool *c = &w->c[cid];
    assert(c->stride == STRIDE_TAG);
    int from = 0;
    int to = c->n;
    const uint64_t *map = w->eid.id;
    uint64_t eid = map[index_(eindex)];
    while (from < to) {
        int mid = (from + to) / 2;
        entity_index_t aa_index = c->id[mid];
        uint64_t aa = map[index_(aa_index)];
        if (aa == eid)
            return;
        else if (aa < eid) {
            from = mid + 1;
        } else {
            to = mid;
        }
    }
    // insert eid at [from]
    if (from < c->n - 1) {
        int i;
        // Any dup id ?
        for (i = from; i < c->n - 1; i++) {
            if (ENTITY_INDEX_CMP(c->id[i], c->id[i + 1]) == 0) {
                memmove(c->id + from + 1, c->id + from, sizeof(entity_index_t) * (i - from));
                c->id[from] = eindex;
                return;
            }
        }
    }
    ecs_add_component_id_(w, cid, eindex);
}

void entity_enable_tag_(struct entity_world *w, int cid, int index, int tag_id) {
    entity_index_t eid = get_index_(w, cid, index);
    insert_id(w, tag_id, eid);
}

static inline void replace_id(struct component_pool *c, int from, int to, entity_index_t eid) {
    int i;
    for (i = from; i < to; i++) {
        c->id[i] = eid;
    }
}

void entity_disable_tag_(struct entity_world *w, int cid, int index, int tag_id) {
    struct component_pool *c = &w->c[tag_id];
    entity_index_t eid = get_index_(w, cid, index);
    if (cid != tag_id) {
        index = ecs_lookup_component_(c, eid, c->last_lookup);
        if (index < 0) return;
        c->last_lookup = index;
    }
    assert(c->stride == STRIDE_TAG);
    int from, to;
    // find next tag. You may disable subsquent tags in iteration.
    // For example, The sequence is 1 3 5 7 9 . We are now on 5 , and disable 7 .
    // We should change 7 to 9 ( 1 3 5 9 9 ) rather than 7 to 5 ( 1 3 5 5 9 )
    //                   iterator ->   ^                                ^
    for (to = index + 1; to < c->n; to++) {
        if (ENTITY_INDEX_CMP(c->id[to], eid) != 0) {
            for (from = index - 1; from >= 0; from--) {
                if (ENTITY_INDEX_CMP(c->id[from], eid) != 0) break;
            }
            replace_id(c, from + 1, to, c->id[to]);
            return;
        }
    }
    for (from = index - 1; from >= 0; from--) {
        if (ENTITY_INDEX_CMP(c->id[from], eid) != 0) break;
    }
    c->n = from + 1;
}

int entity_get_lua_(struct entity_world *w, int cid, int index, void *wL, int world_index, void *L_) {
    lua_State *L = (lua_State *)L_;
    struct component_pool *c = &w->c[cid];
    ++index;
    if (c->stride != STRIDE_LUA || index <= 0 || index > c->n) {
        return LUA_TNIL;
    }
    if (lua_getiuservalue(wL, world_index, cid) != LUA_TTABLE) {
        lua_pop(wL, 1);
        return LUA_TNIL;
    }
    int t = lua_rawgeti(wL, -1, index);
    if (t == LUA_TNIL) {
        lua_pop(wL, 2);
        return LUA_TNIL;
    }
    lua_xmove(wL, L, 1);
    lua_pop(wL, 1);
    return t;
}

#define DEFAULT_GROUP_SIZE 1024
#define GROUP_COMBINE 1024

struct entity_iterator {
    int last_pos;
    int decode_pos;
    int encode_pos;
    int n;
    uint64_t last;
    uint64_t eid;
};

struct entity_group {
    int n;
    int cap;
    int groupid;
    uint64_t last;
    uint8_t *s;
};

void entity_group_deinit_(struct entity_group_arena *G) {
    int i;
    for (i = 0; i < G->n; i++) {
        free(G->g[i]);
    }
    free(G->g);
}

size_t entity_group_memsize_(struct entity_group_arena *G) {
    size_t sz = G->cap * sizeof(struct entity_group *);
    int i;
    for (i = 0; i < G->n; i++) {
        sz += sizeof(struct entity_group) + G->g[i]->cap;
    }
    return sz;
}

static inline void add_byte(struct entity_group *g, uint8_t b) {
    if (g->n >= g->cap) {
        if (g->s == NULL) {
            g->cap = DEFAULT_GROUP_SIZE;
            g->s = (uint8_t *)malloc(DEFAULT_GROUP_SIZE);
        } else {
            int newcap = g->cap * 3 / 2 + 1;
            g->s = (uint8_t *)realloc(g->s, newcap);
            g->cap = newcap;
        }
    }
    g->s[g->n++] = b;
}

static void add_eid(struct entity_group *g, uint64_t eid) {
    uint64_t eid_diff = eid - g->last - 1;
    if (eid_diff < 128) {
        add_byte(g, eid_diff);
    } else {
        do {
            add_byte(g, (eid_diff & 0x7f) | 0x80);
            eid_diff >>= 7;
        } while (eid_diff >= 128);
        add_byte(g, eid_diff);
    }
    g->last = eid;
}

static inline void foreach_begin(struct entity_group *g, struct entity_iterator *iter) {
    memset(iter, 0, sizeof(*iter));
    iter->n = g->n;
}

static inline void decode_eid(struct entity_group *g, struct entity_iterator *iter) {
    uint8_t *s = g->s;
    int i = iter->decode_pos++;
    if (s[i] < 128) {
        iter->eid += s[i] + 1;
        return;
    }
    int shift = 7;
    uint64_t diff = s[i] & 0x7f;
    for (;;) {
        ++i;
        assert(i < iter->n);
        if (s[i] < 128) {
            diff |= s[i] << shift;
            iter->eid += diff + 1;
            iter->decode_pos = i + 1;
            return;
        } else {
            diff |= (s[i] & 0x7f) << shift;
        }
        shift += 7;
    }
}

static int foreach_end(struct entity_group *g, struct entity_iterator *iter) {
    if (iter->decode_pos >= iter->n) return 0;
    iter->last_pos = iter->decode_pos;
    iter->last = iter->eid;
    decode_eid(g, iter);
    return 1;
}

static int insert_group(struct entity_group_arena *G, int groupid, int begin, int end) {
    while (begin < end) {
        int mid = (begin + end) / 2;
        int v = G->g[mid]->groupid;
        if (v == groupid) return mid;
        if (v < groupid)
            begin = mid + 1;
        else
            end = mid;
    }
    // insert at begin
    if (G->n >= G->cap) {
        if (G->g == NULL) {
            G->cap = DEFAULT_GROUP_SIZE;
            G->g = (struct entity_group **)malloc(G->cap * sizeof(struct entity_group *));
        } else {
            G->cap = G->cap * 3 / 2 + 1;
            struct entity_group **g = (struct entity_group **)malloc(G->cap * sizeof(struct entity_group *));
            memcpy(g, G->g, begin * sizeof(struct entity_group *));
            memcpy(g + begin + 1, G->g + begin, (G->n - begin) * sizeof(struct entity_group *));
            free(G->g);
            G->g = g;
        }
    } else {
        memmove(G->g + begin + 1, G->g + begin, (G->n - begin) * sizeof(struct entity_group *));
    }
    ++G->n;
    struct entity_group *group = (struct entity_group *)malloc(sizeof(struct entity_group));
    memset(group, 0, sizeof(*group));
    group->groupid = groupid;

    G->g[begin] = group;

    return begin;
}

static struct entity_group *find_group(struct entity_group_arena *G, int groupid) {
    int h = (uint32_t)(2654435769 * (uint32_t)groupid) >> (32 - ENTITY_GROUP_CACHE_BITS);
    int index;
    if (h >= G->n) {
        index = insert_group(G, groupid, 0, G->n);
    } else {
        struct entity_group *g = G->g[G->cache[h]];
        if (g->groupid == groupid) {
            return g;
        } else if (g->groupid < groupid) {
            index = insert_group(G, groupid, h + 1, G->n);
        } else {
            index = insert_group(G, groupid, 0, h);
        }
    }

    // cache miss
    G->cache[h] = index;
    return G->g[index];
}

int entity_group_add_(struct entity_group_arena *G, int groupid, uint64_t eid) {
    struct entity_group *g = find_group(G, groupid);
    if (eid <= g->last) {
        return 0;
    } else {
        add_eid(g, eid);
        return 1;
    }
}

struct tag_index_context {
    struct entity_group *group[GROUP_COMBINE];
    struct entity_iterator iter[GROUP_COMBINE];
    uint64_t lastid;
    int n;
    int pos;
    int index[GROUP_COMBINE];
};

static int tag_index(struct entity_world *w, struct tag_index_context *ctx) {
    int i;
    uint64_t min_id = ctx->iter[ctx->index[0]].eid;
    int j = 0;
    for (i = 1; i < ctx->n; i++) {
        uint64_t eid = ctx->iter[ctx->index[i]].eid;
        if (eid < min_id) {
            min_id = eid;
            j = i;
        }
    }
    int ii = ctx->index[j];
    struct entity_iterator *iter = &ctx->iter[ii];
    struct entity_group *group = ctx->group[ii];
    uint64_t diff = min_id - ctx->lastid + 1;
    int index = entity_id_find_guessrange(&w->eid, min_id, ctx->pos, ctx->pos + diff);
    int need_encode = iter->encode_pos != iter->last_pos;
    if (index >= 0) {
        if (need_encode) {
            // previous eid removed, encode current eid
            add_eid(group, min_id);
            iter->encode_pos = group->n;
        } else {
            iter->encode_pos = ctx->iter[ii].decode_pos;
        }
        ctx->lastid = min_id;
        ctx->pos = index + 1;
    } else if (!need_encode) {
        group->n = ctx->iter[ii].last_pos;
        group->last = ctx->iter[ii].last;
    }
    if (!foreach_end(group, iter)) {
        // This group is end, remove j from index
        --ctx->n;
        memmove(ctx->index + j, ctx->index + j + 1, (ctx->n - j) * sizeof(int));
    }
    return index;
}

static inline void dump_(struct entity_group_arena *G) {
    int i;
    for (i = 0; i < G->n; i++) {
        struct entity_group *g = G->g[i];
        printf("Group %d:\n", g->groupid);
        struct entity_iterator iter;
        for (foreach_begin(g, &iter); foreach_end(g, &iter);) {
            printf("\t%llu\n", iter.eid);
        }
    }
}

static void enable_(struct entity_world *w, int tagid, int n, int groupid[GROUP_COMBINE]) {
    struct tag_index_context ctx;
    ctx.n = 0;
    ctx.pos = 0;
    ctx.lastid = 0;
    int i;
    for (i = 0; i < n; i++) {
        ctx.group[i] = find_group(&w->group, groupid[i]);
        foreach_begin(ctx.group[i], &ctx.iter[i]);
        if (foreach_end(ctx.group[i], &ctx.iter[i])) {
            ctx.index[ctx.n++] = i;
        }
    }
    while (ctx.n > 0) {
        int index = tag_index(w, &ctx);
        if (index >= 0) ecs_add_component_id_(w, tagid, make_index_(index));
    }
}

void entity_group_enable_(struct entity_world *w, int tagid, int n, int groupid[]) {
    entity_clear_type_(w, tagid);
    int *p = groupid;
    while (n > GROUP_COMBINE) {
        enable_(w, tagid, GROUP_COMBINE, p);
        p += GROUP_COMBINE;
        n -= GROUP_COMBINE;
    }
    if (n > 0) enable_(w, tagid, n, p);
}

void entity_group_id_(struct entity_group_arena *G, int groupid, lua_State *L) {
    struct entity_group *g = find_group(G, groupid);
    lua_createtable(L, g->n, 0);
    struct entity_iterator iter;
    int i = 0;
    for (foreach_begin(g, &iter); foreach_end(g, &iter);) {
        lua_pushinteger(L, iter.eid);
        lua_rawseti(L, -2, ++i);
    }
    assert(iter.eid == g->last);
}

#ifdef TEST_GROUP_CODEC

static void test_add_item() {
    struct entity_group g;
    memset(&g, 0, sizeof(g));
    uint64_t eid = 1;
    int i;
    for (i = 0; i < 10; i++) {
        add_eid(&g, eid);
        eid += i * 2;
    }

    eid = 1;
    struct entity_iterator iter;
    i = 0;
    for (foreach_begin(&g, &iter); foreach_end(&g, &iter);) {
        assert(iter.eid == eid);
        eid += i * 2;
        ++i;
    }
}

static void test_group_add() {
    struct entity_group_arena g;
    memset(&g, 0, sizeof(g));
    int groupid = 10000;
    int i;
    for (i = 0; i < 100000; i++) {
        entity_group_add(&g, groupid, i);
        groupid -= 3;
    }

    groupid = 10000;
    for (i = 0; i < 100000; i++) {
        struct entity_group *group = find_group(&g, groupid);
        struct entity_iterator iter;
        foreach_begin(group, &iter);
        foreach_end(group, &iter);
        assert(iter.eid == i);
        groupid -= 3;
    }
}

int main() {
    test_add_item();
    test_group_add();
    return 0;
}

#endif

struct file_reader {
    FILE *f;
};

static entity_index_t read_id(lua_State *L, FILE *f, entity_index_t *id, int n) {
    size_t r = fread(id, sizeof(entity_index_t), n, f);
    if (r != n) luaL_error(L, "Read id error");
    int i;
    uint32_t last_id = 0;
    for (i = 0; i < n; i++) {
        last_id += index_(id[i]);
        id[i] = make_index_(last_id);
    }
    return make_index_(last_id);
}

static void read_data(lua_State *L, FILE *f, void *buffer, int stride, int n) {
    size_t r = fread(buffer, stride, n, f);
    if (r != n) luaL_error(L, "Read data error");
}

static entity_index_t read_section(lua_State *L, struct file_reader *reader, struct component_pool *c, size_t offset, int stride, int n) {
    if (reader->f == NULL) luaL_error(L, "Invalid reader");
    if (fseek(reader->f, offset, SEEK_SET) != 0) {
        luaL_error(L, "Reader seek error");
    }
    entity_index_t maxid;
    if (stride > 0) {
        maxid = read_id(L, reader->f, c->id, n);
        read_data(L, reader->f, c->buffer, stride, n);
    } else {
        maxid = read_id(L, reader->f, c->id, n);
    }
    return maxid;
}

static void read_section_eid(lua_State *L, struct file_reader *reader, uint64_t *eid, size_t offset, int n) {
    if (reader->f == NULL) luaL_error(L, "Invalid reader");
    if (fseek(reader->f, offset, SEEK_SET) != 0) {
        luaL_error(L, "Reader seek error");
    }
    size_t r = fread(eid, sizeof(uint64_t), n, reader->f);
    if (r != n) luaL_error(L, "Read eid error");
    int i;
    uint64_t last_id = (uint64_t)-1;
    for (i = 0; i < n; i++) {
        last_id += eid[i] + 1;
        eid[i] = last_id;
    }
}

static int ecs_persistence_generate_eid(lua_State *L) {
    struct entity_world *w = getW(L);
    int i;
    int maxid = -1;
    for (i = 0; i < MAX_COMPONENT; i++) {
        struct component_pool *c = &w->c[i];
        if (c->n > 0) {
            int m = index_(c->id[c->n - 1]);
            if (m > maxid) maxid = m;
        }
    }
    if (maxid < 0) {
        return 0;
    }
    maxid++;
    ecs_reserve_eid_(w, maxid);
    w->eid.last_id = w->eid.n = maxid;
    for (i = 0; i < maxid; i++) {
        w->eid.id[i] = (uint64_t)i + 1;
    }
    return 0;
}

int ecs_persistence_readcomponent(lua_State *L) {
    struct entity_world *w = getW(L);
    struct file_reader *reader = luaL_checkudata(L, 2, "LUAECS_READER");
    int cid = luaL_checkinteger(L, 3);
    size_t offset = luaL_checkinteger(L, 4);
    int stride = luaL_optinteger(L, 5, -1);
    int n = luaL_checkinteger(L, 6);

    if (cid == ENTITYID_TAG) {
        if (stride != -1) return luaL_error(L, "Invalid eid");
        ecs_reserve_eid_(w, n);
        read_section_eid(L, reader, w->eid.id, offset, n);
        w->eid.n = n;
        w->eid.last_id = (n > 0) ? w->eid.id[n - 1] : 0;
        lua_pushinteger(L, n);
        return 1;
    } else {
        check_cid_valid(L, w, cid);
        struct component_pool *c = &w->c[cid];
        if (c->n != 0) {
            return luaL_error(L, "Component %d exists", cid);
        }
        if (c->stride != stride) {
            return luaL_error(L, "Invalid component %d (%d != %d)", cid, c->stride, stride);
        }
        ecs_reserve_component_(c, cid, n);
        entity_index_t maxid = read_section(L, reader, c, offset, stride, n);
        c->n = n;
        lua_pushinteger(L, index_(maxid));
        return 1;
    }
}

struct file_section {
    size_t offset;
    int stride;
    int n;
};

struct file_writer {
    FILE *f;
    int n;
    struct file_section c[MAX_COMPONENT];
};

static size_t get_length(struct file_section *s) {
    if (s->stride < 0)
        return s->offset + s->n * sizeof(uint64_t);
    else
        return s->offset + s->n * (sizeof(entity_index_t) + s->stride);
}

static uint32_t write_id_(lua_State *L, struct file_writer *w, entity_index_t *id, int n, uint32_t last_id) {
    entity_index_t buffer[1024];
    int i;
    for (i = 0; i < n; i++) {
        uint32_t t = index_(id[i]);
        uint32_t diff = t - last_id;
        last_id = t;
        buffer[i] = make_index_(diff);
    }
    size_t r = fwrite(buffer, sizeof(entity_index_t), n, w->f);
    if (r != n) {
        luaL_error(L, "Can't write section id");
    }
    return last_id;
}

static void write_id(lua_State *L, struct file_writer *w, struct component_pool *c) {
    int i;
    uint32_t last_id = 0;
    for (i = 0; i < c->n; i += 1024) {
        int n = c->n - i;
        if (n > 1024) n = 1024;
        last_id = write_id_(L, w, c->id + i, n, last_id);
    }
}

static void write_data(lua_State *L, struct file_writer *w, struct component_pool *c) {
    size_t s = fwrite(c->buffer, c->stride, c->n, w->f);
    if (s != c->n) {
        luaL_error(L, "Can't write section data %d:%d", c->n, c->stride);
    }
}

static uint64_t write_eid_(lua_State *L, struct file_writer *w, const uint64_t *id, int n, uint64_t last_id) {
    uint64_t buffer[1024];
    int i;
    for (i = 0; i < n; i++) {
        uint64_t diff = id[i] - last_id - 1;
        last_id = id[i];
        buffer[i] = diff;
    }
    size_t r = fwrite(buffer, sizeof(uint64_t), n, w->f);
    if (r != n) {
        luaL_error(L, "Can't write section id");
    }
    return last_id;
}

static void write_eid(lua_State *L, struct file_writer *w, const struct entity_id *eid) {
    int i;
    uint64_t last_id = (uint64_t)-1;
    for (i = 0; i < eid->n; i += 1024) {
        int n = eid->n - i;
        if (n > 1024) n = 1024;
        last_id = write_eid_(L, w, eid->id + i, n, last_id);
    }
}

static int lwrite_section(lua_State *L) {
    struct file_writer *w = (struct file_writer *)luaL_checkudata(L, 1, "LUAECS_WRITER");
    struct entity_world *world = (struct entity_world *)lua_touserdata(L, 2);
    if (w->f == NULL) return luaL_error(L, "Invalid writer");
    if (world == NULL) return luaL_error(L, "Invalid world");
    if (w->n >= MAX_COMPONENT) return luaL_error(L, "Too many sections");

    struct file_section *s = &w->c[w->n];
    if (w->n == 0) {
        s->offset = 0;
    } else {
        s->offset = get_length(&w->c[w->n - 1]);
    }

    int cid = luaL_checkinteger(L, 3);
    if (cid == ENTITYID_TAG) {
        s->stride = -1;  // It's eid
        s->n = world->eid.n;
        ++w->n;
        write_eid(L, w, &world->eid);
    } else {
        check_cid_valid(L, world, cid);
        struct component_pool *c = &world->c[cid];
        if (c->stride < 0) {
            return luaL_error(L, "The component is not writable");
        }
        s->stride = c->stride;
        s->n = c->n;
        ++w->n;
        if (s->stride > 0) {
            write_id(L, w, c);
            write_data(L, w, c);
        } else {
            write_id(L, w, c);
        }
    }
    return 0;
}

static int lrawclose_writer(lua_State *L) {
    struct file_writer *w = (struct file_writer *)lua_touserdata(L, 1);
    if (w->f) {
        fclose(w->f);
        w->f = NULL;
    }
    return 0;
}

static int lclose_writer(lua_State *L) {
    struct file_writer *w = (struct file_writer *)luaL_checkudata(L, 1, "LUAECS_WRITER");
    if (w->f == NULL) return luaL_error(L, "Invalid writer");
    lrawclose_writer(L);
    lua_createtable(L, w->n, 0);
    int i;
    for (i = 0; i < w->n; i++) {
        lua_createtable(L, 0, 3);
        struct file_section *s = &w->c[i];
        lua_pushinteger(L, s->offset);
        lua_setfield(L, -2, "offset");
        if (s->stride >= 0) {
            lua_pushinteger(L, s->stride);
            lua_setfield(L, -2, "stride");
        }
        lua_pushinteger(L, s->n);
        lua_setfield(L, -2, "n");
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

static FILE *fileopen(lua_State *L, int idx, const char *mode) {
    if (lua_type(L, idx) == LUA_TSTRING) {
        const char *filename = lua_tostring(L, idx);
        FILE *f = fopen(filename, mode);
        if (f == NULL) {
            luaL_error(L, "Can't open %s (%s)", filename, mode);
        }
        return f;
    }
    int fd = luaL_checkinteger(L, idx);
    FILE *f = fdopen(fd, mode);
    if (f == NULL) luaL_error(L, "Can't open %d (%s)", fd, mode);
    return f;
}

int ecs_persistence_writer(lua_State *L) {
    struct file_writer *w = (struct file_writer *)lua_newuserdatauv(L, sizeof(*w), 0);
    w->f = NULL;
    w->n = 0;
    w->f = fileopen(L, 1, "wb");
    if (luaL_newmetatable(L, "LUAECS_WRITER")) {
        luaL_Reg l[] = {
                {"write", lwrite_section}, {"close", lclose_writer}, {"__gc", lrawclose_writer}, {"__index", NULL}, {NULL, NULL},
        };
        luaL_setfuncs(L, l, 0);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);
    return 1;
}

static int lclose_reader(lua_State *L) {
    struct file_reader *r = (struct file_reader *)lua_touserdata(L, 1);
    if (r != NULL && r->f != NULL) {
        fclose(r->f);
        r->f = NULL;
    }
    return 0;
}

int ecs_persistence_reader(lua_State *L) {
    struct file_reader *r = (struct file_reader *)lua_newuserdatauv(L, sizeof(*r), 0);
    r->f = fileopen(L, 1, "rb");
    if (luaL_newmetatable(L, "LUAECS_READER")) {
        luaL_Reg l[] = {
                {"close", lclose_reader},
                {"__gc", lclose_reader},
                {"__index", NULL},
                {NULL, NULL},
        };
        luaL_setfuncs(L, l, 0);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);
    return 1;
}

int lpersistence_methods(lua_State *L) {
    luaL_Reg m[] = {
            {"_readcomponent", ecs_persistence_readcomponent}, {"generate_eid", ecs_persistence_generate_eid}, {"writer", ecs_persistence_writer}, {"reader", ecs_persistence_reader}, {NULL, NULL},
    };
    luaL_newlib(L, m);

    return 1;
}

// varint :
//	[0,0x7f]  one byte
//	[0x80, 0x3fff] two bytes : v >> 7,  0x80 | (v & 0x7f)
//	[0x4000, 0x1fffff] three bytes
static inline void varint_encode(luaL_Buffer *b, size_t v) {
    while (v > 0x7f) {
        luaL_addchar(b, (char)((v & 0x7f) | 0x80));
        v >>= 7;
    }
    luaL_addchar(b, (char)v);
}

static size_t varint_decode(lua_State *L, uint8_t *buffer, size_t sz, size_t *r) {
    *r = 0;
    int shift = 0;
    size_t rsize = 0;
    while (sz > rsize) {
        int s = buffer[rsize] & 0x7f;
        *r |= (size_t)s << shift;
        if (s == buffer[rsize]) {
            return rsize + 1;
        }
        ++rsize;
        shift += 7;
    }
    return luaL_error(L, "Invalid varint");
}

int ecs_serialize_object(lua_State *L) {
    struct group_iter *iter = luaL_checkudata(L, 1, "ENTITY_GROUPITER");
    int cid = iter->k[0].id;
    struct entity_world *w = iter->world;
    if (cid < 0 || cid >= MAX_COMPONENT) {
        return luaL_error(L, "Invalid object %d", cid);
    }
    lua_settop(L, 2);
    struct component_pool *c = &w->c[cid];
    if (c->stride <= 0) {
        if (c->stride == STRIDE_LUA) return luaL_error(L, "Can't serialize lua object");
        return luaL_error(L, "Can't serialize tag");
    }

    luaL_Buffer b;
    luaL_buffinit(L, &b);
    varint_encode(&b, c->stride);

    void *buffer = luaL_prepbuffsize(&b, c->stride);
    lua_pushvalue(L, 2);
    ecs_write_component_object_(L, iter->k[0].field_n, iter->f, buffer);
    luaL_addsize(&b, c->stride);
    luaL_pushresult(&b);
    return 1;
}

int ecs_template_create(lua_State *L) {
    struct entity_world *w = getW(L);
    luaL_checktype(L, 2, LUA_TTABLE);
    int i = 1;
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    for (;;) {
        if (lua_geti(L, 2, i++) == LUA_TNIL) {
            lua_pop(L, 1);
            break;
        }
        int cid = check_cid(L, w, -1);
        lua_pop(L, 1);

        varint_encode(&b, cid);
        switch (lua_geti(L, 2, i++)) {
            case LUA_TSTRING:
                luaL_addvalue(&b);
                break;
            case LUA_TUSERDATA: {
                size_t sz = lua_rawlen(L, -1);
                void *buf = lua_touserdata(L, -1);
                luaL_addlstring(&b, (const char *)buf, sz);
                lua_pop(L, 1);
                break;
            }
            default:
                return luaL_error(L, "Invalid valid with cid = %d", (int)cid);
        }
    }
    luaL_pushresult(&b);
    return 1;
}

// 1 : world
// 2 : component id
// 3 : index
// 4 : value
int ecs_template_instance_component(lua_State *L) {
    struct entity_world *w = getW(L);
    int cid = check_cid(L, w, 2);
    int index = luaL_checkinteger(L, 3) - 1;
    struct component_pool *c = &w->c[cid];
    if (c->stride == STRIDE_LUA) {
        if (lua_getiuservalue(L, 1, cid) != LUA_TTABLE) {
            return luaL_error(L, "Missing lua table for %d", cid);
        }
        lua_pushvalue(L, 4);
        lua_rawseti(L, -2, ecs_get_eid(w, cid, index));
    } else {
        size_t sz;
        void *s;
        switch (lua_type(L, 4)) {
            case LUA_TSTRING:
                s = (void *)lua_tolstring(L, 4, &sz);
                break;
            case LUA_TLIGHTUSERDATA:
                s = lua_touserdata(L, 4);
                sz = luaL_checkinteger(L, 5);
                break;
            default:
                lua_pushboolean(L, 1);
                return 1;
        }
        if (sz != c->stride) {
            return luaL_error(L, "Invalid unmarshal result");
        }
        memcpy(get_ptr(c, index), s, sz);
    }
    return 0;
}

// 1 : string data
// 2 : int offset
int ecs_template_extract(lua_State *L) {
    size_t sz;
    const char *buffer = luaL_checklstring(L, 1, &sz);
    int offset = luaL_optinteger(L, 2, 0);
    if (offset >= sz) {
        if (offset > sz) return luaL_error(L, "Invalid offset %d", offset);
        return 0;
    }
    sz -= offset;
    buffer += offset;
    size_t cid;
    size_t r = varint_decode(L, (uint8_t *)buffer, sz, &cid);
    lua_pushinteger(L, cid);
    sz -= r;
    buffer += r;
    size_t slen;
    size_t r2 = varint_decode(L, (uint8_t *)buffer, sz, &slen);
    if (slen == 0 && r2 == 2) {  // magic number 0x80 0x00 , string
        r2 += varint_decode(L, (uint8_t *)buffer + 2, sz - 2, &slen);
        sz -= r2;
        buffer += r2;
        if (slen > sz) {
            return luaL_error(L, "Invalid template");
        }
        // return id, offset, string
        lua_pushinteger(L, offset + r + r2 + slen);
        lua_pushlstring(L, buffer, slen);
        return 3;
    } else {
        // return id, offset, lightuserdata, sz
        sz -= r2;
        buffer += r2;
        lua_pushinteger(L, offset + r + r2 + slen);
        lua_pushlightuserdata(L, (void *)buffer);
        lua_pushinteger(L, slen);
        return 4;
    }
}

int ecs_serialize_lua(lua_State *L) {
    luaL_Buffer b;
    if (lua_type(L, 1) == LUA_TSTRING) {
        size_t sz;
        const char *s = lua_tolstring(L, 1, &sz);
        luaL_buffinitsize(L, &b, sz + 8);
        luaL_addchar(&b, 0x80);
        luaL_addchar(&b, 0);  // 0x80 0x00 : magic number, it's string
        varint_encode(&b, sz);
        luaL_addlstring(&b, s, sz);
    } else {
        // Support seri function in ltask : https://github.com/cloudwu/ltask/blob/master/src/lua-seri.c
        luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
        const char *buf = (const char *)lua_touserdata(L, 1);
        size_t sz = luaL_checkinteger(L, 2);
        luaL_buffinitsize(L, &b, sz + 8);
        varint_encode(&b, sz);
        luaL_addlstring(&b, buf, sz);
        free((void *)buf);  // lightuserdata, free
    }
    luaL_pushresult(&b);
    return 1;
}

int ltemplate_methods(lua_State *L) {
    luaL_Reg m[] = {
            {"_serialize", ecs_serialize_object},
            {"_serialize_lua", ecs_serialize_lua},
            {"_template_extract", ecs_template_extract},
            {"_template_create", ecs_template_create},
            {"_template_instance_component", ecs_template_instance_component},
            {NULL, NULL},
    };
    luaL_newlib(L, m);

    return 1;
}