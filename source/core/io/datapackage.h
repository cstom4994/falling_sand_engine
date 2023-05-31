
#ifndef ME_DATAPACKAGE_H
#define ME_DATAPACKAGE_H

#include <stdarg.h>
#include <stddef.h>

//#include "core/core.h"

#ifdef __INTEL_COMPILER
#include <tbb/tbbmalloc_proxy.h>
#endif /* Intel Compiler efficient memcpy etc */

#ifdef _MSC_VER
typedef unsigned int uint32_t;
#else
#include <inttypes.h> /* uint32_t */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* bit flags (external) */
#define DATAPACK_FILE (1 << 0)
#define DATAPACK_MEM (1 << 1)
#define DATAPACK_PREALLOCD (1 << 2)
#define DATAPACK_EXCESS_OK (1 << 3)
#define DATAPACK_FD (1 << 4)
#define DATAPACK_UFREE (1 << 5)
#define DATAPACK_DATAPEEK (1 << 6)
#define DATAPACK_FXLENS (1 << 7)
#define DATAPACK_GETSIZE (1 << 8)
/* do not add flags here without renumbering the internal flags! */

/* flags for datapack_gather mode */
#define DATAPACK_GATHER_BLOCKING 1
#define DATAPACK_GATHER_NONBLOCKING 2
#define DATAPACK_GATHER_MEM 3

/* Hooks for error logging, memory allocation functions and fatal */
typedef void *(datapack_malloc_fcn)(size_t sz);
typedef void *(datapack_realloc_fcn)(void *ptr, size_t sz);
typedef void(datapack_free_fcn)(void *ptr);
typedef void(datapack_fatal_fcn)(const char *fmt, ...);

typedef struct datapack_hook_t {
    datapack_malloc_fcn *malloc;
    datapack_realloc_fcn *realloc;
    datapack_free_fcn *free;
    datapack_fatal_fcn *fatal;
    size_t gather_max;
} datapack_hook_t;

typedef struct datapack_node {
    int type;
    void *addr;
    void *data;                        /* r:datapack_root_data*. A:datapack_atyp*. ow:szof type */
    int num;                           /* length of type if its a C array */
    size_t ser_osz;                    /* serialization output size for subtree */
    struct datapack_node *children;    /* my children; linked-list */
    struct datapack_node *next, *prev; /* my siblings (next child of my parent) */
    struct datapack_node *parent;      /* my parent */
} datapack_node;

/* used when un/packing 'B' type (binary buffers) */
typedef struct datapack_bin {
    void *addr;
    uint32_t sz;
} datapack_bin;

/* for async/piecemeal reading of datapack images */
typedef struct datapack_gather_t {
    char *img;
    int len;
} datapack_gather_t;

/* Callback used when datapack_gather has read a full datapack image */
typedef int(datapack_gather_cb)(void *img, size_t sz, void *data);

/* Prototypes */
datapack_node *datapack_map(char *fmt, ...);        /* define datapack using format */
void datapack_free(datapack_node *r);               /* free a datapack map */
int datapack_pack(datapack_node *r, int i);         /* pack the n'th packable */
int datapack_unpack(datapack_node *r, int i);       /* unpack the n'th packable */
int datapack_dump(datapack_node *r, int mode, ...); /* serialize to mem/file */
int datapack_load(datapack_node *r, int mode, ...); /* set mem/file to unpack */
int datapack_Alen(datapack_node *r, int i);         /* array len of packable i */
char *datapack_peek(int mode, ...);                 /* sneak peek at format string */
int datapack_gather(int mode, ...);                 /* non-blocking image gather */
int datapack_jot(int mode, ...);                    /* quick write a simple datapack */

datapack_node *datapack_map_va(char *fmt, va_list ap);

#if defined __cplusplus
}
#endif

#endif
