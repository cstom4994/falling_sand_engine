
#include "datapackage.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libs/physfs/physfs.h"

#ifndef _WIN32
#include <unistd.h> /* for ftruncate */
#else
#include <io.h>
#define ftruncate(x, y) _chsize(x, y)
#endif
#include <errno.h>
#include <fcntl.h>     /* for 'open' */
#include <sys/stat.h>  /* for 'open' */
#include <sys/types.h> /* for 'open' */
#ifndef _WIN32
#include <inttypes.h> /* uint32_t, uint64_t, etc */
#else
typedef unsigned short ushort;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#endif

#ifndef S_ISREG
#define S_ISREG(mode) (((mode)&S_IFMT) == S_IFREG)
#endif

#if (defined __CYGWIN__ || defined __MINGW32__ || defined _WIN32)
#include "win/mman.h" /* mmap */
#else
#include <sys/mman.h> /* mmap */
#endif

void TracePhysFSError(const char *detail) {}

unsigned char *LoadFileDataFromPhysFS(const char *fileName, unsigned int *bytesRead) {
    if (!FileExistsInPhysFS(fileName)) {

        *bytesRead = 0;
        return 0;
    }

    void *handle = PHYSFS_openRead(fileName);
    if (handle == 0) {
        TracePhysFSError(fileName);
        *bytesRead = 0;
        return 0;
    }

    int size = PHYSFS_fileLength(handle);
    if (size == -1) {
        *bytesRead = 0;
        PHYSFS_close(handle);

        return 0;
    }

    if (size == 0) {
        PHYSFS_close(handle);
        *bytesRead = 0;
        return 0;
    }

    void *buffer = malloc(size);
    int read = PHYSFS_readBytes(handle, buffer, size);
    if (read < 0) {
        *bytesRead = 0;
        free(buffer);
        PHYSFS_close(handle);
        TracePhysFSError(fileName);
        return 0;
    }

    PHYSFS_close(handle);
    *bytesRead = read;
    return buffer;
}

R_bool InitPhysFS() {

    if (PHYSFS_init(0) == 0) {
        TracePhysFSError("InitPhysFS() failed");
        return false;
    }

    // SetPhysFSWriteDirectory(GetWorkingDirectory());

    return true;
}

R_bool InitPhysFSEx(const char *newDir, const char *mountPoint) {
    if (InitPhysFS()) {
        return MountPhysFS(newDir, mountPoint);
    }
    return false;
}

R_bool IsPhysFSReady() { return PHYSFS_isInit() != 0; }

R_bool MountPhysFS(const char *newDir, const char *mountPoint) {
    if (PHYSFS_mount(newDir, mountPoint, 1) == 0) {
        TracePhysFSError(mountPoint);
        return false;
    }

    return true;
}

R_bool MountPhysFSFromMemory(const unsigned char *fileData, int dataSize, const char *newDir, const char *mountPoint) {
    if (dataSize <= 0) {
        return false;
    }

    if (PHYSFS_mountMemory(fileData, dataSize, 0, newDir, mountPoint, 1) == 0) {
        // TracePhysFSError(sprintf("Failed to mount '%s' at '%s'", newDir, mountPoint));
        return false;
    }

    return true;
}

R_bool UnmountPhysFS(const char *oldDir) {
    if (PHYSFS_unmount(oldDir) == 0) {
        return false;
    }

    return true;
}

R_bool FileExistsInPhysFS(const char *fileName) {
    PHYSFS_Stat stat;
    if (PHYSFS_stat(fileName, &stat) == 0) {
        return false;
    }
    return stat.filetype == PHYSFS_FILETYPE_REGULAR;
}

R_bool DirectoryExistsInPhysFS(const char *dirPath) {
    PHYSFS_Stat stat;
    if (PHYSFS_stat(dirPath, &stat) == 0) {
        return false;
    }
    return stat.filetype == PHYSFS_FILETYPE_DIRECTORY;
}

R_bool SetPhysFSWriteDirectory(const char *newDir) {
    if (PHYSFS_setWriteDir(newDir) == 0) {
        TracePhysFSError(newDir);
        return false;
    }

    return true;
}

R_bool SaveFileDataToPhysFS(const char *fileName, void *data, unsigned int bytesToWrite) {

    if (bytesToWrite == 0) {
        return true;
    }

    void *handle = PHYSFS_openWrite(fileName);
    if (handle == 0) {
        TracePhysFSError(fileName);
        return false;
    }

    if (PHYSFS_writeBytes(handle, data, bytesToWrite) < 0) {
        PHYSFS_close(handle);
        TracePhysFSError(fileName);
        return false;
    }

    PHYSFS_close(handle);
    return true;
}

R_bool SaveFileTextToPhysFS(const char *fileName, char *text) { return SaveFileDataToPhysFS(fileName, text, strlen(text)); }

long GetFileModTimeFromPhysFS(const char *fileName) {
    PHYSFS_Stat stat;
    if (PHYSFS_stat(fileName, &stat) == 0) {
        return -1;
    }

    return stat.modtime;
}

R_bool ClosePhysFS() {
    if (PHYSFS_deinit() == 0) {
        TracePhysFSError("ClosePhysFS() unsuccessful");
        return false;
    }

    return true;
}

void SetPhysFSCallbacks() {
    // SetLoadFileDataCallback(LoadFileDataFromPhysFS);
    // SetSaveFileDataCallback(SaveFileDataToPhysFS);
    // SetLoadFileTextCallback(LoadFileTextFromPhysFS);
    // SetSaveFileTextCallback(SaveFileTextToPhysFS);
}

const char *GetPerfDirectory(const char *organization, const char *application) {
    const char *output = PHYSFS_getPrefDir(organization, application);
    if (output == 0) {
        TracePhysFSError("Failed to get perf directory");
        return 0;
    }

    return output;
}

#define DATAPACK_GATHER_BUFLEN 8192
#define DATAPACK_MAGIC "yzx"
#define DATAPACK_MAGIC_LENGTH 3

/* macro to add a structure to a doubly-linked list */
#define DL_ADD(head, add)               \
    do {                                \
        if (head) {                     \
            (add)->prev = (head)->prev; \
            (head)->prev->next = (add); \
            (head)->prev = (add);       \
            (add)->next = NULL;         \
        } else {                        \
            (head) = (add);             \
            (head)->prev = (head);      \
            (head)->next = NULL;        \
        }                               \
    } while (0);

#define fatal_oom() datapack_hook.fatal("out of memory\n")

/* bit flags (internal). preceded by the external flags in datapack.h */
#define DATAPACK_WRONLY (1 << 9)          /* app has initiated datapack packing  */
#define DATAPACK_RDONLY (1 << 10)         /* datapack was loaded (for unpacking) */
#define DATAPACK_XENDIAN (1 << 11)        /* swap endianness when unpacking */
#define DATAPACK_OLD_STRING_FMT (1 << 12) /* datapack has strings in 1.2 format */

/* values for the flags byte that appears after the magic prefix */
#define DATAPACK_SUPPORTED_BITFLAGS 3
#define DATAPACK_FL_BIGENDIAN (1 << 0)
#define DATAPACK_FL_NULLSTRINGS (1 << 1)

/* char values for node type */
#define DATAPACK_TYPE_ROOT 0
#define DATAPACK_TYPE_INT32 1
#define DATAPACK_TYPE_UINT32 2
#define DATAPACK_TYPE_BYTE 3
#define DATAPACK_TYPE_STR 4
#define DATAPACK_TYPE_ARY 5
#define DATAPACK_TYPE_BIN 6
#define DATAPACK_TYPE_DOUBLE 7
#define DATAPACK_TYPE_INT64 8
#define DATAPACK_TYPE_UINT64 9
#define DATAPACK_TYPE_INT16 10
#define DATAPACK_TYPE_UINT16 11
#define DATAPACK_TYPE_POUND 12

/* error codes */
#define ERR_NOT_MINSIZE (-1)
#define ERR_MAGIC_MISMATCH (-2)
#define ERR_INCONSISTENT_SZ (-3)
#define ERR_FMT_INVALID (-4)
#define ERR_FMT_MISSING_NUL (-5)
#define ERR_FMT_MISMATCH (-6)
#define ERR_FLEN_MISMATCH (-7)
#define ERR_INCONSISTENT_SZ2 (-8)
#define ERR_INCONSISTENT_SZ3 (-9)
#define ERR_INCONSISTENT_SZ4 (-10)
#define ERR_UNSUPPORTED_FLAGS (-11)

/* access to A(...) nodes by index */
typedef struct datapack_pidx {
    struct datapack_node *node;
    struct datapack_pidx *next, *prev;
} datapack_pidx;

/* A(...) node datum */
typedef struct datapack_atyp {
    uint32_t num; /* num elements */
    size_t sz;    /* size of each backbone's datum */
    struct datapack_backbone *bb, *bbtail;
    void *cur;
} datapack_atyp;

/* backbone to extend A(...) lists dynamically */
typedef struct datapack_backbone {
    struct datapack_backbone *next;
    /* when this structure is malloc'd, extra space is alloc'd at the
     * end to store the backbone "datum", and data points to it. */
#if __STDC_VERSION__ < 199901
    char *data;
#else
    char data[];
#endif
} datapack_backbone;

/* mmap record */
typedef struct datapack_mmap_rec {
    int fd;
    void *text;
    size_t text_sz;
} datapack_mmap_rec;

/* root node datum */
typedef struct datapack_root_data {
    int flags;
    datapack_pidx *pidx;
    datapack_mmap_rec mmap;
    char *fmt;
    int *fxlens, num_fxlens;
} datapack_root_data;

/* node type to size mapping */
struct datapack_type_t {
    char c;
    int sz;
};

/* Internal prototypes */
static datapack_node *datapack_node_new(datapack_node *parent);
static datapack_node *datapack_find_i(datapack_node *n, int i);
static void *datapack_cpv(void *datav, const void *data, size_t sz);
static void *datapack_extend_backbone(datapack_node *n);
static char *datapack_fmt(datapack_node *r);
static void *datapack_dump_atyp(datapack_node *n, datapack_atyp *at, void *dv);
static size_t datapack_ser_osz(datapack_node *n);
static void datapack_free_atyp(datapack_node *n, datapack_atyp *atyp);
static int datapack_dump_to_mem(datapack_node *r, void *addr, size_t sz);
static int datapack_mmap_file(char *filename, datapack_mmap_rec *map_rec);
static int datapack_mmap_output_file(char *filename, size_t sz, void **text_out);
static int datapack_cpu_bigendian(void);
static int datapack_needs_endian_swap(void *);
static void datapack_byteswap(void *word, int len);
static void datapack_fatal(const char *fmt, ...);
static int datapack_serlen(datapack_node *r, datapack_node *n, void *dv, size_t *serlen);
static int datapack_unpackA0(datapack_node *r);
static int datapack_gather_mem(char *buf, size_t len, datapack_gather_t **gs, datapack_gather_cb *cb, void *data);
static int datapack_gather_nonblocking(int fd, datapack_gather_t **gs, datapack_gather_cb *cb, void *data);
static int datapack_gather_blocking(int fd, void **img, size_t *sz);

/* This is used internally to help calculate padding when a 'double'
 * follows a smaller datatype in a structure. Normally under gcc
 * on x86, d will be aligned at +4, however use of -malign-double
 * causes d to be aligned at +8 (this is actually faster on x86).
 * Also SPARC and x86_64 seem to align always on +8.
 */
struct datapack_double_alignment_detector {
    char a;
    double d; /* some platforms align this on +4, others on +8 */
};

/* this is another case where alignment varies. mac os x/gcc was observed
 * to align the int64_t at +4 under -m32 and at +8 under -m64 */
struct datapack_int64_alignment_detector {
    int i;
    int64_t j; /* some platforms align this on +4, others on +8 */
};

typedef struct {
    size_t inter_elt_len;           /* padded inter-element len; i.e. &a[1].field - &a[0].field */
    datapack_node *iter_start_node; /* node to jump back to, as we start each new iteration */
    size_t iternum;                 /* current iteration number (total req'd. iter's in n->num) */
} datapack_pound_data;

/* Hooks for customizing datapack mem alloc, error handling, etc. Set defaults. */
datapack_hook_t datapack_hook = {
        /* .malloc =     */ malloc,
        /* .realloc =    */ realloc,
        /* .free =       */ free,
        /* .fatal =      */ datapack_fatal,
        /* .gather_max = */ 0 /* max datapack size (bytes) for datapack_gather */
};

static const char datapack_fmt_chars[] = "AS($)BiucsfIUjv#";  /* valid format chars */
static const char datapack_S_fmt_chars[] = "iucsfIUjv#$()";   /* valid within S(...) */
static const char datapack_datapeek_ok_chars[] = "iucsfIUjv"; /* valid in datapeek */
static const struct datapack_type_t datapack_types[] = {
        /* [DATAPACK_TYPE_ROOT] =   */ {'r', 0},
        /* [DATAPACK_TYPE_INT32] =  */ {'i', sizeof(int32_t)},
        /* [DATAPACK_TYPE_UINT32] = */ {'u', sizeof(uint32_t)},
        /* [DATAPACK_TYPE_BYTE] =   */ {'c', sizeof(char)},
        /* [DATAPACK_TYPE_STR] =    */ {'s', sizeof(char *)},
        /* [DATAPACK_TYPE_ARY] =    */ {'A', 0},
        /* [DATAPACK_TYPE_BIN] =    */ {'B', 0},
        /* [DATAPACK_TYPE_DOUBLE] = */ {'f', 8}, /* not sizeof(double) as that varies */
        /* [DATAPACK_TYPE_INT64] =  */ {'I', sizeof(int64_t)},
        /* [DATAPACK_TYPE_UINT64] = */ {'U', sizeof(uint64_t)},
        /* [DATAPACK_TYPE_INT16] =  */ {'j', sizeof(int16_t)},
        /* [DATAPACK_TYPE_UINT16] = */ {'v', sizeof(uint16_t)},
        /* [DATAPACK_TYPE_POUND] =  */ {'#', 0},
};

static datapack_node *datapack_node_new(datapack_node *parent) {
    datapack_node *n;
    if ((n = datapack_hook.malloc(sizeof(datapack_node))) == NULL) {
        fatal_oom();
    }
    n->addr = NULL;
    n->data = NULL;
    n->num = 1;
    n->ser_osz = 0;
    n->children = NULL;
    n->next = NULL;
    n->parent = parent;
    return n;
}

/* Used in S(..) formats to pack several fields from a structure based on
 * only the structure address. We need to calculate field addresses
 * manually taking into account the size of the fields and intervening padding.
 * The wrinkle is that double is not normally aligned on x86-32 but the
 * -malign-double compiler option causes it to be. Double are aligned
 * on Sparc, and apparently on 64 bit x86. We use a helper structure
 * to detect whether double is aligned in this compilation environment.
 */
char *calc_field_addr(datapack_node *parent, int type, char *struct_addr, int ordinal) {
    datapack_node *prev;
    int offset;
    int align_sz;

    if (ordinal == 1) return struct_addr; /* first field starts on structure address */

    /* generate enough padding so field addr is divisible by it's align_sz. 4, 8, etc */
    prev = parent->children->prev;
    switch (type) {
        case DATAPACK_TYPE_DOUBLE:
            align_sz = sizeof(struct datapack_double_alignment_detector) > 12 ? 8 : 4;
            break;
        case DATAPACK_TYPE_INT64:
        case DATAPACK_TYPE_UINT64:
            align_sz = sizeof(struct datapack_int64_alignment_detector) > 12 ? 8 : 4;
            break;
        default:
            align_sz = datapack_types[type].sz;
            break;
    }
    offset = ((uintptr_t)prev->addr - (uintptr_t)struct_addr) + (datapack_types[prev->type].sz * prev->num);
    offset = (offset + align_sz - 1) / align_sz * align_sz;
    return struct_addr + offset;
}

datapack_node *datapack_map(char *fmt, ...) {
    va_list ap;
    datapack_node *tn;

    va_start(ap, fmt);
    tn = datapack_map_va(fmt, ap);
    va_end(ap);
    return tn;
}

datapack_node *datapack_map_va(char *fmt, va_list ap) {
    int lparen_level = 0, expect_lparen = 0, t = 0, in_structure = 0, ordinal = 0;
    int in_nested_structure = 0;
    char *c, *peek, *struct_addr = NULL, *struct_next;
    datapack_node *root, *parent, *n = NULL, *preceding, *iter_start_node = NULL, *struct_widest_node = NULL, *np;
    datapack_pidx *pidx;
    datapack_pound_data *pd;
    int *fxlens, num_fxlens, pound_num, pound_prod, applies_to_struct;
    int contig_fxlens[10]; /* temp space for contiguous fxlens */
    int num_contig_fxlens, i, j;
    ptrdiff_t inter_elt_len = 0; /* padded element length of contiguous structs in array */

    root = datapack_node_new(NULL);
    root->type = DATAPACK_TYPE_ROOT;
    root->data = (datapack_root_data *)datapack_hook.malloc(sizeof(datapack_root_data));
    if (!root->data) fatal_oom();
    memset((datapack_root_data *)root->data, 0, sizeof(datapack_root_data));

    /* set up root nodes special ser_osz to reflect overhead of preamble */
    root->ser_osz = sizeof(uint32_t); /* datapack leading length */
    root->ser_osz += strlen(fmt) + 1; /* fmt + NUL-terminator */
    root->ser_osz += 4;               /* 'datapack' magic prefix + flags byte */

    parent = root;

    c = fmt;
    while (*c != '\0') {
        switch (*c) {
            case 'c':
            case 'i':
            case 'u':
            case 'j':
            case 'v':
            case 'I':
            case 'U':
            case 'f':
                if (*c == 'c')
                    t = DATAPACK_TYPE_BYTE;
                else if (*c == 'i')
                    t = DATAPACK_TYPE_INT32;
                else if (*c == 'u')
                    t = DATAPACK_TYPE_UINT32;
                else if (*c == 'j')
                    t = DATAPACK_TYPE_INT16;
                else if (*c == 'v')
                    t = DATAPACK_TYPE_UINT16;
                else if (*c == 'I')
                    t = DATAPACK_TYPE_INT64;
                else if (*c == 'U')
                    t = DATAPACK_TYPE_UINT64;
                else if (*c == 'f')
                    t = DATAPACK_TYPE_DOUBLE;

                if (expect_lparen) goto fail;
                n = datapack_node_new(parent);
                n->type = t;
                if (in_structure) {
                    if (ordinal == 1) {
                        /* for S(...)# iteration. Apply any changes to case 's' too!!! */
                        iter_start_node = n;
                        struct_widest_node = n;
                    }
                    if (datapack_types[n->type].sz > datapack_types[struct_widest_node->type].sz) {
                        struct_widest_node = n;
                    }
                    n->addr = calc_field_addr(parent, n->type, struct_addr, ordinal++);
                } else
                    n->addr = (void *)va_arg(ap, void *);
                n->data = datapack_hook.malloc(datapack_types[t].sz);
                if (!n->data) fatal_oom();
                if (n->parent->type == DATAPACK_TYPE_ARY) ((datapack_atyp *)(n->parent->data))->sz += datapack_types[t].sz;
                DL_ADD(parent->children, n);
                break;
            case 's':
                if (expect_lparen) goto fail;
                n = datapack_node_new(parent);
                n->type = DATAPACK_TYPE_STR;
                if (in_structure) {
                    if (ordinal == 1) {
                        iter_start_node = n; /* for S(...)# iteration */
                        struct_widest_node = n;
                    }
                    if (datapack_types[n->type].sz > datapack_types[struct_widest_node->type].sz) {
                        struct_widest_node = n;
                    }
                    n->addr = calc_field_addr(parent, n->type, struct_addr, ordinal++);
                } else
                    n->addr = (void *)va_arg(ap, void *);
                n->data = datapack_hook.malloc(sizeof(char *));
                if (!n->data) fatal_oom();
                *(char **)(n->data) = NULL;
                if (n->parent->type == DATAPACK_TYPE_ARY) ((datapack_atyp *)(n->parent->data))->sz += sizeof(void *);
                DL_ADD(parent->children, n);
                break;
            case '#':
                /* apply a 'num' to preceding atom */
                if (!parent->children) goto fail;
                preceding = parent->children->prev; /* first child's prev is 'last child'*/
                t = preceding->type;
                applies_to_struct = (*(c - 1) == ')') ? 1 : 0;
                if (!applies_to_struct) {
                    if (!(t == DATAPACK_TYPE_BYTE || t == DATAPACK_TYPE_INT32 || t == DATAPACK_TYPE_UINT32 || t == DATAPACK_TYPE_DOUBLE || t == DATAPACK_TYPE_UINT64 || t == DATAPACK_TYPE_INT64 ||
                          t == DATAPACK_TYPE_UINT16 || t == DATAPACK_TYPE_INT16 || t == DATAPACK_TYPE_STR))
                        goto fail;
                }
                /* count up how many contiguous # and form their product */
                pound_prod = 1;
                num_contig_fxlens = 0;
                for (peek = c; *peek == '#'; peek++) {
                    pound_num = va_arg(ap, int);
                    if (pound_num < 1) {
                        datapack_hook.fatal("non-positive iteration count %d\n", pound_num);
                    }
                    if (num_contig_fxlens >= (sizeof(contig_fxlens) / sizeof(contig_fxlens[0]))) {
                        datapack_hook.fatal("contiguous # exceeds hardcoded limit\n");
                    }
                    contig_fxlens[num_contig_fxlens++] = pound_num;
                    pound_prod *= pound_num;
                }
                /* increment c to skip contiguous # so its points to last one */
                c = peek - 1;
                /* differentiate atom-# from struct-# by noting preceding rparen */
                if (applies_to_struct) { /* insert # node to induce looping */
                    n = datapack_node_new(parent);
                    n->type = DATAPACK_TYPE_POUND;
                    n->num = pound_prod;
                    n->data = datapack_hook.malloc(sizeof(datapack_pound_data));
                    if (!n->data) fatal_oom();
                    pd = (datapack_pound_data *)n->data;
                    pd->inter_elt_len = inter_elt_len;
                    pd->iter_start_node = iter_start_node;
                    pd->iternum = 0;
                    DL_ADD(parent->children, n);
                    /* multiply the 'num' and data space on each atom in the structure */
                    for (np = iter_start_node; np != n; np = np->next) {
                        if (n->parent->type == DATAPACK_TYPE_ARY) {
                            ((datapack_atyp *)(n->parent->data))->sz += datapack_types[np->type].sz * (np->num * (n->num - 1));
                        }
                        np->data = datapack_hook.realloc(np->data, datapack_types[np->type].sz * np->num * n->num);
                        if (!np->data) fatal_oom();
                        memset(np->data, 0, datapack_types[np->type].sz * np->num * n->num);
                    }
                } else { /* simple atom-# form does not require a loop */
                    preceding->num = pound_prod;
                    preceding->data = datapack_hook.realloc(preceding->data, datapack_types[t].sz * preceding->num);
                    if (!preceding->data) fatal_oom();
                    memset(preceding->data, 0, datapack_types[t].sz * preceding->num);
                    if (n->parent->type == DATAPACK_TYPE_ARY) {
                        ((datapack_atyp *)(n->parent->data))->sz += datapack_types[t].sz * (preceding->num - 1);
                    }
                }
                root->ser_osz += (sizeof(uint32_t) * num_contig_fxlens);

                j = ((datapack_root_data *)root->data)->num_fxlens; /* before incrementing */
                (((datapack_root_data *)root->data)->num_fxlens) += num_contig_fxlens;
                num_fxlens = ((datapack_root_data *)root->data)->num_fxlens; /* new value */
                fxlens = ((datapack_root_data *)root->data)->fxlens;
                fxlens = datapack_hook.realloc(fxlens, sizeof(int) * num_fxlens);
                if (!fxlens) fatal_oom();
                ((datapack_root_data *)root->data)->fxlens = fxlens;
                for (i = 0; i < num_contig_fxlens; i++) fxlens[j++] = contig_fxlens[i];

                break;
            case 'B':
                if (expect_lparen) goto fail;
                if (in_structure) goto fail;
                n = datapack_node_new(parent);
                n->type = DATAPACK_TYPE_BIN;
                n->addr = (datapack_bin *)va_arg(ap, void *);
                n->data = datapack_hook.malloc(sizeof(datapack_bin *));
                if (!n->data) fatal_oom();
                *((datapack_bin **)n->data) = NULL;
                if (n->parent->type == DATAPACK_TYPE_ARY) ((datapack_atyp *)(n->parent->data))->sz += sizeof(datapack_bin);
                DL_ADD(parent->children, n);
                break;
            case 'A':
                if (in_structure) goto fail;
                n = datapack_node_new(parent);
                n->type = DATAPACK_TYPE_ARY;
                DL_ADD(parent->children, n);
                parent = n;
                expect_lparen = 1;
                pidx = (datapack_pidx *)datapack_hook.malloc(sizeof(datapack_pidx));
                if (!pidx) fatal_oom();
                pidx->node = n;
                pidx->next = NULL;
                DL_ADD(((datapack_root_data *)(root->data))->pidx, pidx);
                /* set up the A's datapack_atyp */
                n->data = (datapack_atyp *)datapack_hook.malloc(sizeof(datapack_atyp));
                if (!n->data) fatal_oom();
                ((datapack_atyp *)(n->data))->num = 0;
                ((datapack_atyp *)(n->data))->sz = 0;
                ((datapack_atyp *)(n->data))->bb = NULL;
                ((datapack_atyp *)(n->data))->bbtail = NULL;
                ((datapack_atyp *)(n->data))->cur = NULL;
                if (n->parent->type == DATAPACK_TYPE_ARY) ((datapack_atyp *)(n->parent->data))->sz += sizeof(void *);
                break;
            case 'S':
                if (in_structure) goto fail;
                expect_lparen = 1;
                ordinal = 1;                     /* index upcoming atoms in S(..) */
                in_structure = 1 + lparen_level; /* so we can tell where S fmt ends */
                struct_addr = (char *)va_arg(ap, void *);
                break;
            case '$': /* nested structure */
                if (!in_structure) goto fail;
                expect_lparen = 1;
                in_nested_structure++;
                break;
            case ')':
                lparen_level--;
                if (lparen_level < 0) goto fail;
                if (*(c - 1) == '(') goto fail;
                if (in_nested_structure)
                    in_nested_structure--;
                else if (in_structure && (in_structure - 1 == lparen_level)) {
                    /* calculate delta between contiguous structures in array */
                    struct_next = calc_field_addr(parent, struct_widest_node->type, struct_addr, ordinal++);
                    inter_elt_len = struct_next - struct_addr;
                    in_structure = 0;
                } else
                    parent = parent->parent; /* rparen ends A() type, not S() type */
                break;
            case '(':
                if (!expect_lparen) goto fail;
                expect_lparen = 0;
                lparen_level++;
                break;
            default:
                METADOT_ERROR("Datapack: unsupported option %c\n", *c);
                goto fail;
        }
        c++;
    }
    if (lparen_level != 0) goto fail;

    /* copy the format string, save for convenience */
    ((datapack_root_data *)(root->data))->fmt = datapack_hook.malloc(strlen(fmt) + 1);
    if (((datapack_root_data *)(root->data))->fmt == NULL) fatal_oom();
    memcpy(((datapack_root_data *)(root->data))->fmt, fmt, strlen(fmt) + 1);

    return root;

fail:
    METADOT_ERROR("Datapack: failed to parse %s\n", fmt);
    datapack_free(root);
    return NULL;
}

static int datapack_unmap_file(datapack_mmap_rec *mr) {

    if (munmap(mr->text, mr->text_sz) == -1) {
        METADOT_ERROR("Datapack: Failed to munmap: %s\n", strerror(errno));
    }
    close(mr->fd);
    mr->text = NULL;
    mr->text_sz = 0;
    return 0;
}

static void datapack_free_keep_map(datapack_node *r) {
    int mmap_bits = (DATAPACK_RDONLY | DATAPACK_FILE);
    int ufree_bits = (DATAPACK_MEM | DATAPACK_UFREE);
    datapack_node *nxtc, *c;
    int find_next_node = 0, looking, i;
    size_t sz;

    /* For mmap'd files, or for 'ufree' memory images , do appropriate release */
    if ((((datapack_root_data *)(r->data))->flags & mmap_bits) == mmap_bits) {
        datapack_unmap_file(&((datapack_root_data *)(r->data))->mmap);
    } else if ((((datapack_root_data *)(r->data))->flags & ufree_bits) == ufree_bits) {
        datapack_hook.free(((datapack_root_data *)(r->data))->mmap.text);
    }

    c = r->children;
    if (c) {
        while (c->type != DATAPACK_TYPE_ROOT) { /* loop until we come back to root node */
            switch (c->type) {
                case DATAPACK_TYPE_BIN:
                    /* free any binary buffer hanging from datapack_bin */
                    if (*((datapack_bin **)(c->data))) {
                        if ((*((datapack_bin **)(c->data)))->addr) {
                            datapack_hook.free((*((datapack_bin **)(c->data)))->addr);
                        }
                        *((datapack_bin **)c->data) = NULL; /* reset datapack_bin */
                    }
                    find_next_node = 1;
                    break;
                case DATAPACK_TYPE_STR:
                    /* free any packed (copied) string */
                    for (i = 0; i < c->num; i++) {
                        char *str = ((char **)c->data)[i];
                        if (str) {
                            datapack_hook.free(str);
                            ((char **)c->data)[i] = NULL;
                        }
                    }
                    find_next_node = 1;
                    break;
                case DATAPACK_TYPE_INT32:
                case DATAPACK_TYPE_UINT32:
                case DATAPACK_TYPE_INT64:
                case DATAPACK_TYPE_UINT64:
                case DATAPACK_TYPE_BYTE:
                case DATAPACK_TYPE_DOUBLE:
                case DATAPACK_TYPE_INT16:
                case DATAPACK_TYPE_UINT16:
                case DATAPACK_TYPE_POUND:
                    find_next_node = 1;
                    break;
                case DATAPACK_TYPE_ARY:
                    c->ser_osz = 0; /* zero out the serialization output size */

                    sz = ((datapack_atyp *)(c->data))->sz; /* save sz to use below */
                    datapack_free_atyp(c, c->data);

                    /* make new atyp */
                    c->data = (datapack_atyp *)datapack_hook.malloc(sizeof(datapack_atyp));
                    if (!c->data) fatal_oom();
                    ((datapack_atyp *)(c->data))->num = 0;
                    ((datapack_atyp *)(c->data))->sz = sz; /* restore bb datum sz */
                    ((datapack_atyp *)(c->data))->bb = NULL;
                    ((datapack_atyp *)(c->data))->bbtail = NULL;
                    ((datapack_atyp *)(c->data))->cur = NULL;

                    c = c->children;
                    break;
                default:
                    datapack_hook.fatal("unsupported format character\n");
                    break;
            }

            if (find_next_node) {
                find_next_node = 0;
                looking = 1;
                while (looking) {
                    if (c->next) {
                        nxtc = c->next;
                        c = nxtc;
                        looking = 0;
                    } else {
                        if (c->type == DATAPACK_TYPE_ROOT)
                            break; /* root node */
                        else {
                            nxtc = c->parent;
                            c = nxtc;
                        }
                    }
                }
            }
        }
    }

    ((datapack_root_data *)(r->data))->flags = 0; /* reset flags */
}

void datapack_free(datapack_node *r) {
    int mmap_bits = (DATAPACK_RDONLY | DATAPACK_FILE);
    int ufree_bits = (DATAPACK_MEM | DATAPACK_UFREE);
    datapack_node *nxtc, *c;
    int find_next_node = 0, looking, num, i;
    datapack_pidx *pidx, *pidx_nxt;

    /* For mmap'd files, or for 'ufree' memory images , do appropriate release */
    if ((((datapack_root_data *)(r->data))->flags & mmap_bits) == mmap_bits) {
        datapack_unmap_file(&((datapack_root_data *)(r->data))->mmap);
    } else if ((((datapack_root_data *)(r->data))->flags & ufree_bits) == ufree_bits) {
        datapack_hook.free(((datapack_root_data *)(r->data))->mmap.text);
    }

    c = r->children;
    if (c) {
        while (c->type != DATAPACK_TYPE_ROOT) { /* loop until we come back to root node */
            switch (c->type) {
                case DATAPACK_TYPE_BIN:
                    /* free any binary buffer hanging from datapack_bin */
                    if (*((datapack_bin **)(c->data))) {
                        if ((*((datapack_bin **)(c->data)))->sz != 0) {
                            datapack_hook.free((*((datapack_bin **)(c->data)))->addr);
                        }
                        datapack_hook.free(*((datapack_bin **)c->data)); /* free datapack_bin */
                    }
                    datapack_hook.free(c->data); /* free datapack_bin* */
                    find_next_node = 1;
                    break;
                case DATAPACK_TYPE_STR:
                    /* free any packed (copied) string */
                    num = 1;
                    nxtc = c->next;
                    while (nxtc) {
                        if (nxtc->type == DATAPACK_TYPE_POUND) {
                            num = nxtc->num;
                        }
                        nxtc = nxtc->next;
                    }
                    for (i = 0; i < c->num * num; i++) {
                        char *str = ((char **)c->data)[i];
                        if (str) {
                            datapack_hook.free(str);
                            ((char **)c->data)[i] = NULL;
                        }
                    }
                    datapack_hook.free(c->data);
                    find_next_node = 1;
                    break;
                case DATAPACK_TYPE_INT32:
                case DATAPACK_TYPE_UINT32:
                case DATAPACK_TYPE_INT64:
                case DATAPACK_TYPE_UINT64:
                case DATAPACK_TYPE_BYTE:
                case DATAPACK_TYPE_DOUBLE:
                case DATAPACK_TYPE_INT16:
                case DATAPACK_TYPE_UINT16:
                case DATAPACK_TYPE_POUND:
                    datapack_hook.free(c->data);
                    find_next_node = 1;
                    break;
                case DATAPACK_TYPE_ARY:
                    datapack_free_atyp(c, c->data);
                    if (c->children)
                        c = c->children; /* normal case */
                    else
                        find_next_node = 1; /* edge case, handle bad format A() */
                    break;
                default:
                    datapack_hook.fatal("unsupported format character\n");
                    break;
            }

            if (find_next_node) {
                find_next_node = 0;
                looking = 1;
                while (looking) {
                    if (c->next) {
                        nxtc = c->next;
                        datapack_hook.free(c);
                        c = nxtc;
                        looking = 0;
                    } else {
                        if (c->type == DATAPACK_TYPE_ROOT)
                            break; /* root node */
                        else {
                            nxtc = c->parent;
                            datapack_hook.free(c);
                            c = nxtc;
                        }
                    }
                }
            }
        }
    }

    /* free root */
    for (pidx = ((datapack_root_data *)(r->data))->pidx; pidx; pidx = pidx_nxt) {
        pidx_nxt = pidx->next;
        datapack_hook.free(pidx);
    }
    datapack_hook.free(((datapack_root_data *)(r->data))->fmt);
    if (((datapack_root_data *)(r->data))->num_fxlens > 0) {
        datapack_hook.free(((datapack_root_data *)(r->data))->fxlens);
    }
    datapack_hook.free(r->data); /* datapack_root_data */
    datapack_hook.free(r);
}

/* Find the i'th packable ('A' node) */
static datapack_node *datapack_find_i(datapack_node *n, int i) {
    int j = 0;
    datapack_pidx *pidx;
    if (n->type != DATAPACK_TYPE_ROOT) return NULL;
    if (i == 0) return n; /* packable 0 is root */
    for (pidx = ((datapack_root_data *)(n->data))->pidx; pidx; pidx = pidx->next) {
        if (++j == i) return pidx->node;
    }
    return NULL;
}

static void *datapack_cpv(void *datav, const void *data, size_t sz) {
    if (sz > 0) memcpy(datav, data, sz);
    return (void *)((uintptr_t)datav + sz);
}

static void *datapack_extend_backbone(datapack_node *n) {
    datapack_backbone *bb;
    bb = (datapack_backbone *)datapack_hook.malloc(sizeof(datapack_backbone) + ((datapack_atyp *)(n->data))->sz); /* datum hangs on coattails of bb */
    if (!bb) fatal_oom();
#if __STDC_VERSION__ < 199901
    bb->data = (char *)((uintptr_t)bb + sizeof(datapack_backbone));
#endif
    memset(bb->data, 0, ((datapack_atyp *)(n->data))->sz);
    bb->next = NULL;
    /* Add the new backbone to the tail, also setting head if necessary  */
    if (((datapack_atyp *)(n->data))->bb == NULL) {
        ((datapack_atyp *)(n->data))->bb = bb;
        ((datapack_atyp *)(n->data))->bbtail = bb;
    } else {
        ((datapack_atyp *)(n->data))->bbtail->next = bb;
        ((datapack_atyp *)(n->data))->bbtail = bb;
    }

    ((datapack_atyp *)(n->data))->num++;
    return bb->data;
}

/* Get the format string corresponding to a given datapack (root node) */
static char *datapack_fmt(datapack_node *r) { return ((datapack_root_data *)(r->data))->fmt; }

/* Get the fmt # lengths as a contiguous buffer of ints (length num_fxlens) */
static int *datapack_fxlens(datapack_node *r, int *num_fxlens) {
    *num_fxlens = ((datapack_root_data *)(r->data))->num_fxlens;
    return ((datapack_root_data *)(r->data))->fxlens;
}

/* called when serializing an 'A' type node into a buffer which has
 * already been set up with the proper space. The backbone is walked
 * which was obtained from the datapack_atyp header passed in.
 */
static void *datapack_dump_atyp(datapack_node *n, datapack_atyp *at, void *dv) {
    datapack_backbone *bb;
    datapack_node *c;
    void *datav;
    uint32_t slen;
    datapack_bin *binp;
    char *strp;
    datapack_atyp *atypp;
    datapack_pound_data *pd;
    int i;
    size_t itermax;

    /* handle 'A' nodes */
    dv = datapack_cpv(dv, &at->num, sizeof(uint32_t)); /* array len */
    for (bb = at->bb; bb; bb = bb->next) {
        datav = bb->data;
        c = n->children;
        while (c) {
            switch (c->type) {
                case DATAPACK_TYPE_BYTE:
                case DATAPACK_TYPE_DOUBLE:
                case DATAPACK_TYPE_INT32:
                case DATAPACK_TYPE_UINT32:
                case DATAPACK_TYPE_INT64:
                case DATAPACK_TYPE_UINT64:
                case DATAPACK_TYPE_INT16:
                case DATAPACK_TYPE_UINT16:
                    dv = datapack_cpv(dv, datav, datapack_types[c->type].sz * c->num);
                    datav = (void *)((uintptr_t)datav + datapack_types[c->type].sz * c->num);
                    break;
                case DATAPACK_TYPE_BIN:
                    /* dump the buffer length followed by the buffer */
                    memcpy(&binp, datav, sizeof(datapack_bin *)); /* cp to aligned */
                    slen = binp->sz;
                    dv = datapack_cpv(dv, &slen, sizeof(uint32_t));
                    dv = datapack_cpv(dv, binp->addr, slen);
                    datav = (void *)((uintptr_t)datav + sizeof(datapack_bin *));
                    break;
                case DATAPACK_TYPE_STR:
                    /* dump the string length followed by the string */
                    for (i = 0; i < c->num; i++) {
                        memcpy(&strp, datav, sizeof(char *)); /* cp to aligned */
                        slen = strp ? (strlen(strp) + 1) : 0;
                        dv = datapack_cpv(dv, &slen, sizeof(uint32_t));
                        if (slen > 1) dv = datapack_cpv(dv, strp, slen - 1);
                        datav = (void *)((uintptr_t)datav + sizeof(char *));
                    }
                    break;
                case DATAPACK_TYPE_ARY:
                    memcpy(&atypp, datav, sizeof(datapack_atyp *)); /* cp to aligned */
                    dv = datapack_dump_atyp(c, atypp, dv);
                    datav = (void *)((uintptr_t)datav + sizeof(void *));
                    break;
                case DATAPACK_TYPE_POUND:
                    /* iterate over the preceding nodes */
                    pd = (datapack_pound_data *)c->data;
                    itermax = c->num;
                    if (++(pd->iternum) < itermax) {
                        c = pd->iter_start_node;
                        continue;
                    } else { /* loop complete. */
                        pd->iternum = 0;
                    }
                    break;
                default:
                    datapack_hook.fatal("unsupported format character\n");
                    break;
            }
            c = c->next;
        }
    }
    return dv;
}

/* figure the serialization output size needed for datapack whose root is n*/
static size_t datapack_ser_osz(datapack_node *n) {
    datapack_node *c, *np;
    size_t sz, itermax;
    datapack_bin *binp;
    char *strp;
    datapack_pound_data *pd;
    int i;

    /* handle the root node ONLY (subtree's ser_osz have been bubbled-up) */
    if (n->type != DATAPACK_TYPE_ROOT) {
        datapack_hook.fatal("internal error: datapack_ser_osz on non-root node\n");
    }

    sz = n->ser_osz; /* start with fixed overhead, already stored */
    c = n->children;
    while (c) {
        switch (c->type) {
            case DATAPACK_TYPE_BYTE:
            case DATAPACK_TYPE_DOUBLE:
            case DATAPACK_TYPE_INT32:
            case DATAPACK_TYPE_UINT32:
            case DATAPACK_TYPE_INT64:
            case DATAPACK_TYPE_UINT64:
            case DATAPACK_TYPE_INT16:
            case DATAPACK_TYPE_UINT16:
                sz += datapack_types[c->type].sz * c->num;
                break;
            case DATAPACK_TYPE_BIN:
                sz += sizeof(uint32_t);                         /* binary buf len */
                memcpy(&binp, c->data, sizeof(datapack_bin *)); /* cp to aligned */
                sz += binp->sz;
                break;
            case DATAPACK_TYPE_STR:
                for (i = 0; i < c->num; i++) {
                    sz += sizeof(uint32_t);                                /* string len */
                    memcpy(&strp, &((char **)c->data)[i], sizeof(char *)); /* cp to aligned */
                    sz += strp ? strlen(strp) : 0;
                }
                break;
            case DATAPACK_TYPE_ARY:
                sz += sizeof(uint32_t); /* array len */
                sz += c->ser_osz;       /* bubbled-up child array ser_osz */
                break;
            case DATAPACK_TYPE_POUND:
                /* iterate over the preceding nodes */
                itermax = c->num;
                pd = (datapack_pound_data *)c->data;
                if (++(pd->iternum) < itermax) {
                    for (np = pd->iter_start_node; np != c; np = np->next) {
                        np->data = (char *)(np->data) + (datapack_types[np->type].sz * np->num);
                    }
                    c = pd->iter_start_node;
                    continue;
                } else { /* loop complete. */
                    pd->iternum = 0;
                    for (np = pd->iter_start_node; np != c; np = np->next) {
                        np->data = (char *)(np->data) - ((itermax - 1) * datapack_types[np->type].sz * np->num);
                    }
                }
                break;
            default:
                datapack_hook.fatal("unsupported format character\n");
                break;
        }
        c = c->next;
    }
    return sz;
}

int datapack_dump(datapack_node *r, int mode, ...) {
    va_list ap;
    char *filename, *bufv;
    void **addr_out, *buf, *pa_addr;
    int fd, rc = 0;
    size_t sz, *sz_out, pa_sz;
    struct stat sbuf;

    if (((datapack_root_data *)(r->data))->flags & DATAPACK_RDONLY) { /* unusual */
        METADOT_ERROR("Datapack: error: datapack_dump called for a loaded datapack\n");
        return -1;
    }

    sz = datapack_ser_osz(r); /* compute the size needed to serialize  */

    va_start(ap, mode);
    if (mode & DATAPACK_FILE) {
        filename = va_arg(ap, char *);
        fd = datapack_mmap_output_file(filename, sz, &buf);
        if (fd == -1)
            rc = -1;
        else {
            rc = datapack_dump_to_mem(r, buf, sz);
            if (msync(buf, sz, MS_SYNC) == -1) {
                METADOT_ERROR("Datapack: msync failed on fd %d: %s\n", fd, strerror(errno));
            }
            if (munmap(buf, sz) == -1) {
                METADOT_ERROR("Datapack: munmap failed on fd %d: %s\n", fd, strerror(errno));
            }
            close(fd);
        }
    } else if (mode & DATAPACK_FD) {
        fd = va_arg(ap, int);
        if ((buf = datapack_hook.malloc(sz)) == NULL) fatal_oom();
        datapack_dump_to_mem(r, buf, sz);
        bufv = buf;
        do {
            rc = write(fd, bufv, sz);
            if (rc > 0) {
                sz -= rc;
                bufv += rc;
            } else if (rc == -1) {
                if (errno == EINTR || errno == EAGAIN) continue;
                METADOT_ERROR("Datapack: error writing to fd %d: %s\n", fd, strerror(errno));
                /* attempt to rewind partial write to a regular file */
                if (fstat(fd, &sbuf) == 0 && S_ISREG(sbuf.st_mode)) {
                    if (ftruncate(fd, sbuf.st_size - (bufv - (char *)buf)) == -1) {
                        METADOT_ERROR("Datapack: can't rewind: %s\n", strerror(errno));
                    }
                }
                free(buf);
                return -1;
            }
        } while (sz > 0);
        free(buf);
        rc = 0;
    } else if (mode & DATAPACK_MEM) {
        if (mode & DATAPACK_PREALLOCD) { /* caller allocated */
            pa_addr = (void *)va_arg(ap, void *);
            pa_sz = va_arg(ap, size_t);
            if (pa_sz < sz) {
                METADOT_ERROR("Datapack: datapack_dump: buffer too small, need %d bytes\n", sz);
                return -1;
            }
            rc = datapack_dump_to_mem(r, pa_addr, sz);
        } else { /* we allocate */
            addr_out = (void **)va_arg(ap, void *);
            sz_out = va_arg(ap, size_t *);
            if ((buf = datapack_hook.malloc(sz)) == NULL) fatal_oom();
            *sz_out = sz;
            *addr_out = buf;
            rc = datapack_dump_to_mem(r, buf, sz);
        }
    } else if (mode & DATAPACK_GETSIZE) {
        sz_out = va_arg(ap, size_t *);
        *sz_out = sz;
    } else {
        METADOT_ERROR("Datapack: unsupported datapack_dump mode %d\n", mode);
        rc = -1;
    }
    va_end(ap);
    return rc;
}

/* This function expects the caller to have set up a memory buffer of
 * adequate size to hold the serialized datapack. The sz parameter must be
 * the result of datapack_ser_osz(r).
 */
static int datapack_dump_to_mem(datapack_node *r, void *addr, size_t sz) {
    uint32_t slen, sz32;
    int *fxlens, num_fxlens, i;
    void *dv;
    char *fmt, flags;
    datapack_node *c, *np;
    datapack_pound_data *pd;
    size_t itermax;

    fmt = datapack_fmt(r);
    flags = 0;
    if (datapack_cpu_bigendian()) flags |= DATAPACK_FL_BIGENDIAN;
    if (strchr(fmt, 's')) flags |= DATAPACK_FL_NULLSTRINGS;
    sz32 = sz;

    dv = addr;
    dv = datapack_cpv(dv, DATAPACK_MAGIC, DATAPACK_MAGIC_LENGTH); /* copy datapack magic prefix */
    dv = datapack_cpv(dv, &flags, 1);                             /* copy flags byte */
    dv = datapack_cpv(dv, &sz32, sizeof(uint32_t));               /* overall length (inclusive) */
    dv = datapack_cpv(dv, fmt, strlen(fmt) + 1);                  /* copy format with NUL-term */
    fxlens = datapack_fxlens(r, &num_fxlens);
    dv = datapack_cpv(dv, fxlens, num_fxlens * sizeof(uint32_t)); /* fmt # lengths */

    /* serialize the datapack content, iterating over direct children of root */
    c = r->children;
    while (c) {
        switch (c->type) {
            case DATAPACK_TYPE_BYTE:
            case DATAPACK_TYPE_DOUBLE:
            case DATAPACK_TYPE_INT32:
            case DATAPACK_TYPE_UINT32:
            case DATAPACK_TYPE_INT64:
            case DATAPACK_TYPE_UINT64:
            case DATAPACK_TYPE_INT16:
            case DATAPACK_TYPE_UINT16:
                dv = datapack_cpv(dv, c->data, datapack_types[c->type].sz * c->num);
                break;
            case DATAPACK_TYPE_BIN:
                slen = (*(datapack_bin **)(c->data))->sz;
                dv = datapack_cpv(dv, &slen, sizeof(uint32_t));                   /* buffer len */
                dv = datapack_cpv(dv, (*(datapack_bin **)(c->data))->addr, slen); /* buf */
                break;
            case DATAPACK_TYPE_STR:
                for (i = 0; i < c->num; i++) {
                    char *str = ((char **)c->data)[i];
                    slen = str ? strlen(str) + 1 : 0;
                    dv = datapack_cpv(dv, &slen, sizeof(uint32_t));     /* string len */
                    if (slen > 1) dv = datapack_cpv(dv, str, slen - 1); /*string*/
                }
                break;
            case DATAPACK_TYPE_ARY:
                dv = datapack_dump_atyp(c, (datapack_atyp *)c->data, dv);
                break;
            case DATAPACK_TYPE_POUND:
                pd = (datapack_pound_data *)c->data;
                itermax = c->num;
                if (++(pd->iternum) < itermax) {

                    /* in start or midst of loop. advance data pointers. */
                    for (np = pd->iter_start_node; np != c; np = np->next) {
                        np->data = (char *)(np->data) + (datapack_types[np->type].sz * np->num);
                    }
                    /* do next iteration */
                    c = pd->iter_start_node;
                    continue;

                } else { /* loop complete. */

                    /* reset iteration index and addr/data pointers. */
                    pd->iternum = 0;
                    for (np = pd->iter_start_node; np != c; np = np->next) {
                        np->data = (char *)(np->data) - ((itermax - 1) * datapack_types[np->type].sz * np->num);
                    }
                }
                break;
            default:
                datapack_hook.fatal("unsupported format character\n");
                break;
        }
        c = c->next;
    }

    return 0;
}

static int datapack_cpu_bigendian() {
    unsigned i = 1;
    char *c;
    c = (char *)&i;
    return (c[0] == 1 ? 0 : 1);
}

/*
 * algorithm for sanity-checking a datapack image:
 * scan the datapack whilst not exceeding the buffer size (bufsz) ,
 * formulating a calculated (expected) size of the datapack based
 * on walking its data. When calcsize has been calculated it
 * should exactly match the buffer size (bufsz) and the internal
 * recorded size (intlsz)
 */
static int datapack_sanity(datapack_node *r, int excess_ok) {
    uint32_t intlsz;
    int found_nul = 0, rc, octothorpes = 0, num_fxlens, *fxlens, flen;
    void *d, *dv;
    char intlflags, *fmt, c, *mapfmt;
    size_t bufsz, serlen;

    d = ((datapack_root_data *)(r->data))->mmap.text;
    bufsz = ((datapack_root_data *)(r->data))->mmap.text_sz;

    dv = d;
    if (bufsz < (4 + sizeof(uint32_t) + 1)) return ERR_NOT_MINSIZE;                        /* min sz: magic+flags+len+nul */
    if (memcmp(dv, DATAPACK_MAGIC, DATAPACK_MAGIC_LENGTH) != 0) return ERR_MAGIC_MISMATCH; /* missing datapack magic prefix */
    if (datapack_needs_endian_swap(dv)) ((datapack_root_data *)(r->data))->flags |= DATAPACK_XENDIAN;
    dv = (void *)((uintptr_t)dv + 3);
    memcpy(&intlflags, dv, sizeof(char)); /* extract flags */
    if (intlflags & ~DATAPACK_SUPPORTED_BITFLAGS) return ERR_UNSUPPORTED_FLAGS;
    /* TPL1.3 stores strings with a "length+1" prefix to discern NULL strings from
       empty strings from non-empty strings; TPL1.2 only handled the latter two.
       So we need to be mindful of which string format we're reading from. */
    if (!(intlflags & DATAPACK_FL_NULLSTRINGS)) {
        ((datapack_root_data *)(r->data))->flags |= DATAPACK_OLD_STRING_FMT;
    }
    dv = (void *)((uintptr_t)dv + 1);
    memcpy(&intlsz, dv, sizeof(uint32_t)); /* extract internal size */
    if (((datapack_root_data *)(r->data))->flags & DATAPACK_XENDIAN) datapack_byteswap(&intlsz, sizeof(uint32_t));
    if (!excess_ok && (intlsz != bufsz)) return ERR_INCONSISTENT_SZ; /* inconsisent buffer/internal size */
    dv = (void *)((uintptr_t)dv + sizeof(uint32_t));

    /* dv points to the start of the format string. Look for nul w/in buf sz */
    fmt = (char *)dv;
    while ((uintptr_t)dv - (uintptr_t)d < bufsz && !found_nul) {
        if ((c = *(char *)dv) != '\0') {
            if (strchr(datapack_fmt_chars, c) == NULL) return ERR_FMT_INVALID; /* invalid char in format string */
            if ((c = *(char *)dv) == '#') octothorpes++;
            dv = (void *)((uintptr_t)dv + 1);
        } else
            found_nul = 1;
    }
    if (!found_nul) return ERR_FMT_MISSING_NUL; /* runaway format string */
    dv = (void *)((uintptr_t)dv + 1);           /* advance to octothorpe lengths buffer */

    /* compare the map format to the format of this datapack image */
    mapfmt = datapack_fmt(r);
    rc = strcmp(mapfmt, fmt);
    if (rc != 0) return ERR_FMT_MISMATCH;

    /* compare octothorpe lengths in image to the mapped values */
    if ((((uintptr_t)dv + (octothorpes * 4)) - (uintptr_t)d) > bufsz) return ERR_INCONSISTENT_SZ4;
    fxlens = datapack_fxlens(r, &num_fxlens); /* mapped fxlens */
    while (num_fxlens--) {
        memcpy(&flen, dv, sizeof(uint32_t)); /* stored flen */
        if (((datapack_root_data *)(r->data))->flags & DATAPACK_XENDIAN) datapack_byteswap(&flen, sizeof(uint32_t));
        if (flen != *fxlens) return ERR_FLEN_MISMATCH;
        dv = (void *)((uintptr_t)dv + sizeof(uint32_t));
        fxlens++;
    }

    /* dv now points to beginning of data */
    rc = datapack_serlen(r, r, dv, &serlen);   /* get computed serlen of data part */
    if (rc == -1) return ERR_INCONSISTENT_SZ2; /* internal inconsistency in datapack image */
    serlen += ((uintptr_t)dv - (uintptr_t)d);  /* add back serlen of preamble part */
    if (excess_ok && (bufsz < serlen)) return ERR_INCONSISTENT_SZ3;
    if (!excess_ok && (serlen != bufsz)) return ERR_INCONSISTENT_SZ3; /* buffer/internal sz exceeds serlen */
    return 0;
}

static void *datapack_find_data_start(void *d) {
    int octothorpes = 0;
    d = (void *)((uintptr_t)d + 1 + DATAPACK_MAGIC_LENGTH); /* skip DATAPACK_MAGIC and flags byte */
    d = (void *)((uintptr_t)d + 4);                         /* skip int32 overall len */
    while (*(char *)d != '\0') {
        if (*(char *)d == '#') octothorpes++;
        d = (void *)((uintptr_t)d + 1);
    }
    d = (void *)((uintptr_t)d + 1);                                /* skip NUL */
    d = (void *)((uintptr_t)d + (octothorpes * sizeof(uint32_t))); /* skip # array lens */
    return d;
}

static int datapack_needs_endian_swap(void *d) {
    char *c;
    int cpu_is_bigendian;
    c = (char *)d;
    cpu_is_bigendian = datapack_cpu_bigendian();
    return ((c[3] & DATAPACK_FL_BIGENDIAN) == cpu_is_bigendian) ? 0 : 1;
}

static size_t datapack_size_for(char c) {
    int i;
    for (i = 0; i < sizeof(datapack_types) / sizeof(datapack_types[0]); i++) {
        if (datapack_types[i].c == c) return datapack_types[i].sz;
    }
    return 0;
}

char *datapack_peek(int mode, ...) {
    va_list ap;
    int xendian = 0, found_nul = 0, old_string_format = 0;
    char *filename = NULL, *datapeek_f = NULL, *datapeek_c, *datapeek_s;
    void *addr = NULL, *dv, *datapeek_p = NULL;
    size_t sz = 0, fmt_len, first_atom, num_fxlens = 0;
    uint32_t datapeek_ssz, datapeek_csz, datapeek_flen;
    datapack_mmap_rec mr = {0, NULL, 0};
    char *fmt, *fmt_cpy = NULL, c;
    uint32_t intlsz, **fxlens = NULL, *num_fxlens_out = NULL, *fxlensv;

    va_start(ap, mode);
    if ((mode & DATAPACK_FXLENS) && (mode & DATAPACK_DATAPEEK)) {
        METADOT_ERROR("Datapack: DATAPACK_FXLENS and DATAPACK_DATAPEEK mutually exclusive\n");
        goto fail;
    }
    if (mode & DATAPACK_FILE)
        filename = va_arg(ap, char *);
    else if (mode & DATAPACK_MEM) {
        addr = va_arg(ap, void *);
        sz = va_arg(ap, size_t);
    } else {
        METADOT_ERROR("Datapack: unsupported datapack_peek mode %d\n", mode);
        goto fail;
    }
    if (mode & DATAPACK_DATAPEEK) {
        datapeek_f = va_arg(ap, char *);
    }
    if (mode & DATAPACK_FXLENS) {
        num_fxlens_out = va_arg(ap, uint32_t *);
        fxlens = va_arg(ap, uint32_t **);
        *num_fxlens_out = 0;
        *fxlens = NULL;
    }

    if (mode & DATAPACK_FILE) {
        if (datapack_mmap_file(filename, &mr) != 0) {
            METADOT_ERROR("Datapack: datapack_peek failed for file %s\n", filename);
            goto fail;
        }
        addr = mr.text;
        sz = mr.text_sz;
    }

    dv = addr;
    if (sz < (4 + sizeof(uint32_t) + 1)) goto fail;                        /* min sz */
    if (memcmp(dv, DATAPACK_MAGIC, DATAPACK_MAGIC_LENGTH) != 0) goto fail; /* missing datapack magic prefix */
    if (datapack_needs_endian_swap(dv)) xendian = 1;
    if ((((char *)dv)[3] & DATAPACK_FL_NULLSTRINGS) == 0) old_string_format = 1;
    dv = (void *)((uintptr_t)dv + 4);
    memcpy(&intlsz, dv, sizeof(uint32_t)); /* extract internal size */
    if (xendian) datapack_byteswap(&intlsz, sizeof(uint32_t));
    if (intlsz != sz) goto fail; /* inconsisent buffer/internal size */
    dv = (void *)((uintptr_t)dv + sizeof(uint32_t));

    /* dv points to the start of the format string. Look for nul w/in buf sz */
    fmt = (char *)dv;
    while ((uintptr_t)dv - (uintptr_t)addr < sz && !found_nul) {
        if ((c = *(char *)dv) == '\0') {
            found_nul = 1;
        } else if (c == '#') {
            num_fxlens++;
        }
        dv = (void *)((uintptr_t)dv + 1);
    }
    if (!found_nul) goto fail;  /* runaway format string */
    fmt_len = (char *)dv - fmt; /* include space for \0 */
    fmt_cpy = datapack_hook.malloc(fmt_len);
    if (fmt_cpy == NULL) {
        fatal_oom();
    }
    memcpy(fmt_cpy, fmt, fmt_len);

    /* retrieve the octothorpic lengths if requested */
    if (num_fxlens > 0) {
        if (sz < ((uintptr_t)dv + (num_fxlens * sizeof(uint32_t)) - (uintptr_t)addr)) {
            goto fail;
        }
    }
    if ((mode & DATAPACK_FXLENS) && (num_fxlens > 0)) {
        *fxlens = datapack_hook.malloc(num_fxlens * sizeof(uint32_t));
        if (*fxlens == NULL) datapack_hook.fatal("out of memory");
        *num_fxlens_out = num_fxlens;
        fxlensv = *fxlens;
        while (num_fxlens--) {
            memcpy(fxlensv, dv, sizeof(uint32_t));
            if (xendian) datapack_byteswap(fxlensv, sizeof(uint32_t));
            dv = (void *)((uintptr_t)dv + sizeof(uint32_t));
            fxlensv++;
        }
    }
    /* if caller requested, peek into the specified data elements */
    if (mode & DATAPACK_DATAPEEK) {

        first_atom = strspn(fmt, "S()"); /* skip any leading S() */

        datapeek_flen = strlen(datapeek_f);
        if (strspn(datapeek_f, datapack_datapeek_ok_chars) < datapeek_flen) {
            METADOT_ERROR("Datapack: invalid DATAPACK_DATAPEEK format: %s\n", datapeek_f);
            datapack_hook.free(fmt_cpy);
            fmt_cpy = NULL; /* fail */
            goto fail;
        }

        if (strncmp(&fmt[first_atom], datapeek_f, datapeek_flen) != 0) {
            METADOT_ERROR("Datapack: DATAPACK_DATAPEEK format mismatches datapack iamge\n");
            datapack_hook.free(fmt_cpy);
            fmt_cpy = NULL; /* fail */
            goto fail;
        }

        /* advance to data start, then copy out requested elements */
        dv = (void *)((uintptr_t)dv + (num_fxlens * sizeof(uint32_t)));
        for (datapeek_c = datapeek_f; *datapeek_c != '\0'; datapeek_c++) {
            datapeek_p = va_arg(ap, void *);
            if (*datapeek_c == 's') { /* special handling for strings */
                if ((uintptr_t)dv - (uintptr_t)addr + sizeof(uint32_t) > sz) {
                    METADOT_ERROR("Datapack: datapack_peek: datapack has insufficient length\n");
                    datapack_hook.free(fmt_cpy);
                    fmt_cpy = NULL; /* fail */
                    goto fail;
                }
                memcpy(&datapeek_ssz, dv, sizeof(uint32_t)); /* get slen */
                if (xendian) datapack_byteswap(&datapeek_ssz, sizeof(uint32_t));
                if (old_string_format) datapeek_ssz++;
                dv = (void *)((uintptr_t)dv + sizeof(uint32_t)); /* adv. to str */
                if (datapeek_ssz == 0)
                    datapeek_s = NULL;
                else {
                    if ((uintptr_t)dv - (uintptr_t)addr + datapeek_ssz - 1 > sz) {
                        METADOT_ERROR("Datapack: datapack_peek: datapack has insufficient length\n");
                        datapack_hook.free(fmt_cpy);
                        fmt_cpy = NULL; /* fail */
                        goto fail;
                    }
                    datapeek_s = datapack_hook.malloc(datapeek_ssz);
                    if (datapeek_s == NULL) fatal_oom();
                    memcpy(datapeek_s, dv, datapeek_ssz - 1);
                    datapeek_s[datapeek_ssz - 1] = '\0';
                    dv = (void *)((uintptr_t)dv + datapeek_ssz - 1);
                }
                *(char **)datapeek_p = datapeek_s;
            } else {
                datapeek_csz = datapack_size_for(*datapeek_c);
                if ((uintptr_t)dv - (uintptr_t)addr + datapeek_csz > sz) {
                    METADOT_ERROR("Datapack: datapack_peek: datapack has insufficient length\n");
                    datapack_hook.free(fmt_cpy);
                    fmt_cpy = NULL; /* fail */
                    goto fail;
                }
                memcpy(datapeek_p, dv, datapeek_csz);
                if (xendian) datapack_byteswap(datapeek_p, datapeek_csz);
                dv = (void *)((uintptr_t)dv + datapeek_csz);
            }
        }
    }

fail:
    va_end(ap);
    if ((mode & DATAPACK_FILE) && mr.text != NULL) datapack_unmap_file(&mr);
    return fmt_cpy;
}

/* datapack_jot(DATAPACK_FILE, "file.datapack", "si", &s, &i); */
/* datapack_jot(DATAPACK_MEM, &buf, &sz, "si", &s, &i); */
/* datapack_jot(DATAPACK_FD, fd, "si", &s, &i); */
int datapack_jot(int mode, ...) {
    va_list ap;
    char *filename, *fmt;
    size_t *sz;
    int fd, rc = 0;
    void **buf;
    datapack_node *tn;

    va_start(ap, mode);
    if (mode & DATAPACK_FILE) {
        filename = va_arg(ap, char *);
        fmt = va_arg(ap, char *);
        tn = datapack_map_va(fmt, ap);
        if (tn == NULL) {
            rc = -1;
            goto fail;
        }
        datapack_pack(tn, 0);
        rc = datapack_dump(tn, DATAPACK_FILE, filename);
        datapack_free(tn);
    } else if (mode & DATAPACK_MEM) {
        buf = va_arg(ap, void *);
        sz = va_arg(ap, size_t *);
        fmt = va_arg(ap, char *);
        tn = datapack_map_va(fmt, ap);
        if (tn == NULL) {
            rc = -1;
            goto fail;
        }
        datapack_pack(tn, 0);
        rc = datapack_dump(tn, DATAPACK_MEM, buf, sz);
        datapack_free(tn);
    } else if (mode & DATAPACK_FD) {
        fd = va_arg(ap, int);
        fmt = va_arg(ap, char *);
        tn = datapack_map_va(fmt, ap);
        if (tn == NULL) {
            rc = -1;
            goto fail;
        }
        datapack_pack(tn, 0);
        rc = datapack_dump(tn, DATAPACK_FD, fd);
        datapack_free(tn);
    } else {
        datapack_hook.fatal("invalid datapack_jot mode\n");
    }

fail:
    va_end(ap);
    return rc;
}

int datapack_load(datapack_node *r, int mode, ...) {
    va_list ap;
    int rc = 0, fd = 0;
    char *filename = NULL;
    void *addr;
    size_t sz;

    va_start(ap, mode);
    if (mode & DATAPACK_FILE)
        filename = va_arg(ap, char *);
    else if (mode & DATAPACK_MEM) {
        addr = va_arg(ap, void *);
        sz = va_arg(ap, size_t);
    } else if (mode & DATAPACK_FD) {
        fd = va_arg(ap, int);
    } else {
        METADOT_ERROR("Datapack: unsupported datapack_load mode %d\n", mode);
        return -1;
    }
    va_end(ap);

    if (r->type != DATAPACK_TYPE_ROOT) {
        METADOT_ERROR("Datapack: error: datapack_load to non-root node\n");
        return -1;
    }
    if (((datapack_root_data *)(r->data))->flags & (DATAPACK_WRONLY | DATAPACK_RDONLY)) {
        /* already packed or loaded, so reset it as if newly mapped */
        datapack_free_keep_map(r);
    }
    if (mode & DATAPACK_FILE) {
        if (datapack_mmap_file(filename, &((datapack_root_data *)(r->data))->mmap) != 0) {
            METADOT_ERROR("Datapack: datapack_load failed for file %s\n", filename);
            return -1;
        }
        if ((rc = datapack_sanity(r, (mode & DATAPACK_EXCESS_OK))) != 0) {
            if (rc == ERR_FMT_MISMATCH) {
                METADOT_ERROR("Datapack: %s: format signature mismatch\n", filename);
            } else if (rc == ERR_FLEN_MISMATCH) {
                METADOT_ERROR("Datapack: %s: array lengths mismatch\n", filename);
            } else {
                METADOT_ERROR("Datapack: %s: not a valid datapack file\n", filename);
            }
            datapack_unmap_file(&((datapack_root_data *)(r->data))->mmap);
            return -1;
        }
        ((datapack_root_data *)(r->data))->flags = (DATAPACK_FILE | DATAPACK_RDONLY);
    } else if (mode & DATAPACK_MEM) {
        ((datapack_root_data *)(r->data))->mmap.text = addr;
        ((datapack_root_data *)(r->data))->mmap.text_sz = sz;
        if ((rc = datapack_sanity(r, (mode & DATAPACK_EXCESS_OK))) != 0) {
            if (rc == ERR_FMT_MISMATCH) {
                METADOT_ERROR("Datapack: format signature mismatch\n");
            } else {
                METADOT_ERROR("Datapack: not a valid datapack file\n");
            }
            return -1;
        }
        ((datapack_root_data *)(r->data))->flags = (DATAPACK_MEM | DATAPACK_RDONLY);
        if (mode & DATAPACK_UFREE) ((datapack_root_data *)(r->data))->flags |= DATAPACK_UFREE;
    } else if (mode & DATAPACK_FD) {
        /* if fd read succeeds, resulting mem img is used for load */
        if (datapack_gather(DATAPACK_GATHER_BLOCKING, fd, &addr, &sz) > 0) {
            return datapack_load(r, DATAPACK_MEM | DATAPACK_UFREE, addr, sz);
        } else
            return -1;
    } else {
        METADOT_ERROR("Datapack: invalid datapack_load mode %d\n", mode);
        return -1;
    }
    /* this applies to DATAPACK_MEM or DATAPACK_FILE */
    if (datapack_needs_endian_swap(((datapack_root_data *)(r->data))->mmap.text)) ((datapack_root_data *)(r->data))->flags |= DATAPACK_XENDIAN;
    datapack_unpackA0(r); /* prepare root A nodes for use */
    return 0;
}

int datapack_Alen(datapack_node *r, int i) {
    datapack_node *n;

    n = datapack_find_i(r, i);
    if (n == NULL) {
        METADOT_ERROR("Datapack: invalid index %d to datapack_unpack\n", i);
        return -1;
    }
    if (n->type != DATAPACK_TYPE_ARY) return -1;
    return ((datapack_atyp *)(n->data))->num;
}

static void datapack_free_atyp(datapack_node *n, datapack_atyp *atyp) {
    datapack_backbone *bb, *bbnxt;
    datapack_node *c;
    void *dv;
    datapack_bin *binp;
    datapack_atyp *atypp;
    char *strp;
    size_t itermax;
    datapack_pound_data *pd;
    int i;

    bb = atyp->bb;
    while (bb) {
        bbnxt = bb->next;
        dv = bb->data;
        c = n->children;
        while (c) {
            switch (c->type) {
                case DATAPACK_TYPE_BYTE:
                case DATAPACK_TYPE_DOUBLE:
                case DATAPACK_TYPE_INT32:
                case DATAPACK_TYPE_UINT32:
                case DATAPACK_TYPE_INT64:
                case DATAPACK_TYPE_UINT64:
                case DATAPACK_TYPE_INT16:
                case DATAPACK_TYPE_UINT16:
                    dv = (void *)((uintptr_t)dv + datapack_types[c->type].sz * c->num);
                    break;
                case DATAPACK_TYPE_BIN:
                    memcpy(&binp, dv, sizeof(datapack_bin *));      /* cp to aligned */
                    if (binp->addr) datapack_hook.free(binp->addr); /* free buf */
                    datapack_hook.free(binp);                       /* free datapack_bin */
                    dv = (void *)((uintptr_t)dv + sizeof(datapack_bin *));
                    break;
                case DATAPACK_TYPE_STR:
                    for (i = 0; i < c->num; i++) {
                        memcpy(&strp, dv, sizeof(char *));  /* cp to aligned */
                        if (strp) datapack_hook.free(strp); /* free string */
                        dv = (void *)((uintptr_t)dv + sizeof(char *));
                    }
                    break;
                case DATAPACK_TYPE_POUND:
                    /* iterate over the preceding nodes */
                    itermax = c->num;
                    pd = (datapack_pound_data *)c->data;
                    if (++(pd->iternum) < itermax) {
                        c = pd->iter_start_node;
                        continue;
                    } else { /* loop complete. */
                        pd->iternum = 0;
                    }
                    break;
                case DATAPACK_TYPE_ARY:
                    memcpy(&atypp, dv, sizeof(datapack_atyp *)); /* cp to aligned */
                    datapack_free_atyp(c, atypp);                /* free atyp */
                    dv = (void *)((uintptr_t)dv + sizeof(void *));
                    break;
                default:
                    datapack_hook.fatal("unsupported format character\n");
                    break;
            }
            c = c->next;
        }
        datapack_hook.free(bb);
        bb = bbnxt;
    }
    datapack_hook.free(atyp);
}

/* determine (by walking) byte length of serialized r/A node at address dv
 * returns 0 on success, or -1 if the datapack isn't trustworthy (fails consistency)
 */
static int datapack_serlen(datapack_node *r, datapack_node *n, void *dv, size_t *serlen) {
    uint32_t slen;
    int num = 0, fidx;
    datapack_node *c;
    size_t len = 0, alen, buf_past, itermax;
    datapack_pound_data *pd;

    buf_past = ((uintptr_t)((datapack_root_data *)(r->data))->mmap.text + ((datapack_root_data *)(r->data))->mmap.text_sz);

    if (n->type == DATAPACK_TYPE_ROOT)
        num = 1;
    else if (n->type == DATAPACK_TYPE_ARY) {
        if ((uintptr_t)dv + sizeof(uint32_t) > buf_past) return -1;
        memcpy(&num, dv, sizeof(uint32_t));
        if (((datapack_root_data *)(r->data))->flags & DATAPACK_XENDIAN) datapack_byteswap(&num, sizeof(uint32_t));
        dv = (void *)((uintptr_t)dv + sizeof(uint32_t));
        len += sizeof(uint32_t);
    } else
        datapack_hook.fatal("internal error in datapack_serlen\n");

    while (num-- > 0) {
        c = n->children;
        while (c) {
            switch (c->type) {
                case DATAPACK_TYPE_BYTE:
                case DATAPACK_TYPE_DOUBLE:
                case DATAPACK_TYPE_INT32:
                case DATAPACK_TYPE_UINT32:
                case DATAPACK_TYPE_INT64:
                case DATAPACK_TYPE_UINT64:
                case DATAPACK_TYPE_INT16:
                case DATAPACK_TYPE_UINT16:
                    for (fidx = 0; fidx < c->num; fidx++) { /* octothorpe support */
                        if ((uintptr_t)dv + datapack_types[c->type].sz > buf_past) return -1;
                        dv = (void *)((uintptr_t)dv + datapack_types[c->type].sz);
                        len += datapack_types[c->type].sz;
                    }
                    break;
                case DATAPACK_TYPE_BIN:
                    len += sizeof(uint32_t);
                    if ((uintptr_t)dv + sizeof(uint32_t) > buf_past) return -1;
                    memcpy(&slen, dv, sizeof(uint32_t));
                    if (((datapack_root_data *)(r->data))->flags & DATAPACK_XENDIAN) datapack_byteswap(&slen, sizeof(uint32_t));
                    len += slen;
                    dv = (void *)((uintptr_t)dv + sizeof(uint32_t));
                    if ((uintptr_t)dv + slen > buf_past) return -1;
                    dv = (void *)((uintptr_t)dv + slen);
                    break;
                case DATAPACK_TYPE_STR:
                    for (fidx = 0; fidx < c->num; fidx++) { /* octothorpe support */
                        len += sizeof(uint32_t);
                        if ((uintptr_t)dv + sizeof(uint32_t) > buf_past) return -1;
                        memcpy(&slen, dv, sizeof(uint32_t));
                        if (((datapack_root_data *)(r->data))->flags & DATAPACK_XENDIAN) datapack_byteswap(&slen, sizeof(uint32_t));
                        if (!(((datapack_root_data *)(r->data))->flags & DATAPACK_OLD_STRING_FMT)) slen = (slen > 1) ? (slen - 1) : 0;
                        len += slen;
                        dv = (void *)((uintptr_t)dv + sizeof(uint32_t));
                        if ((uintptr_t)dv + slen > buf_past) return -1;
                        dv = (void *)((uintptr_t)dv + slen);
                    }
                    break;
                case DATAPACK_TYPE_ARY:
                    if (datapack_serlen(r, c, dv, &alen) == -1) return -1;
                    dv = (void *)((uintptr_t)dv + alen);
                    len += alen;
                    break;
                case DATAPACK_TYPE_POUND:
                    /* iterate over the preceding nodes */
                    itermax = c->num;
                    pd = (datapack_pound_data *)c->data;
                    if (++(pd->iternum) < itermax) {
                        c = pd->iter_start_node;
                        continue;
                    } else { /* loop complete. */
                        pd->iternum = 0;
                    }
                    break;
                default:
                    datapack_hook.fatal("unsupported format character\n");
                    break;
            }
            c = c->next;
        }
    }
    *serlen = len;
    return 0;
}

static int datapack_mmap_output_file(char *filename, size_t sz, void **text_out) {
    void *text;
    int fd, perms;

#ifndef _WIN32
    perms = S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IROTH; /* ug+w o+r */
    fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, perms);
#else
    perms = _S_IWRITE;
    fd = _open(filename, _O_CREAT | _O_TRUNC | _O_RDWR, perms);
#endif

    if (fd == -1) {
        METADOT_ERROR("Datapack: Couldn't open file %s: %s\n", filename, strerror(errno));
        return -1;
    }

    text = mmap(0, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (text == MAP_FAILED) {
        METADOT_ERROR("Datapack: Failed to mmap %s: %s\n", filename, strerror(errno));
        close(fd);
        return -1;
    }
    if (ftruncate(fd, sz) == -1) {
        METADOT_ERROR("Datapack: ftruncate failed: %s\n", strerror(errno));
        munmap(text, sz);
        close(fd);
        return -1;
    }
    *text_out = text;
    return fd;
}

static int datapack_mmap_file(char *filename, datapack_mmap_rec *mr) {
    struct stat stat_buf;

    if ((mr->fd = open(filename, O_RDONLY)) == -1) {
        METADOT_ERROR("Datapack: Couldn't open file %s: %s\n", filename, strerror(errno));
        return -1;
    }

    if (fstat(mr->fd, &stat_buf) == -1) {
        close(mr->fd);
        METADOT_ERROR("Datapack: Couldn't stat file %s: %s\n", filename, strerror(errno));
        return -1;
    }

    mr->text_sz = (size_t)stat_buf.st_size;
    mr->text = mmap(0, stat_buf.st_size, PROT_READ, MAP_PRIVATE, mr->fd, 0);
    if (mr->text == MAP_FAILED) {
        close(mr->fd);
        METADOT_ERROR("Datapack: Failed to mmap %s: %s\n", filename, strerror(errno));
        return -1;
    }

    return 0;
}

int datapack_pack(datapack_node *r, int i) {
    datapack_node *n, *child, *np;
    void *datav = NULL;
    size_t sz, itermax;
    uint32_t slen;
    char *str;
    datapack_bin *bin;
    datapack_pound_data *pd;
    int fidx;

    n = datapack_find_i(r, i);
    if (n == NULL) {
        METADOT_ERROR("Datapack: invalid index %d to datapack_pack\n", i);
        return -1;
    }

    if (((datapack_root_data *)(r->data))->flags & DATAPACK_RDONLY) {
        /* convert to an writeable datapack, initially empty */
        datapack_free_keep_map(r);
    }

    ((datapack_root_data *)(r->data))->flags |= DATAPACK_WRONLY;

    if (n->type == DATAPACK_TYPE_ARY) datav = datapack_extend_backbone(n);
    child = n->children;
    while (child) {
        switch (child->type) {
            case DATAPACK_TYPE_BYTE:
            case DATAPACK_TYPE_DOUBLE:
            case DATAPACK_TYPE_INT32:
            case DATAPACK_TYPE_UINT32:
            case DATAPACK_TYPE_INT64:
            case DATAPACK_TYPE_UINT64:
            case DATAPACK_TYPE_INT16:
            case DATAPACK_TYPE_UINT16:
                /* no need to use fidx iteration here; we can copy multiple values in one memcpy */
                memcpy(child->data, child->addr, datapack_types[child->type].sz * child->num);
                if (datav) datav = datapack_cpv(datav, child->data, datapack_types[child->type].sz * child->num);
                if (n->type == DATAPACK_TYPE_ARY) n->ser_osz += datapack_types[child->type].sz * child->num;
                break;
            case DATAPACK_TYPE_BIN:
                /* copy the buffer to be packed */
                slen = ((datapack_bin *)child->addr)->sz;
                if (slen > 0) {
                    str = datapack_hook.malloc(slen);
                    if (!str) fatal_oom();
                    memcpy(str, ((datapack_bin *)child->addr)->addr, slen);
                } else
                    str = NULL;
                /* and make a datapack_bin to point to it */
                bin = datapack_hook.malloc(sizeof(datapack_bin));
                if (!bin) fatal_oom();
                bin->addr = str;
                bin->sz = slen;
                /* now pack its pointer, first deep freeing any pre-existing bin */
                if (*(datapack_bin **)(child->data) != NULL) {
                    if ((*(datapack_bin **)(child->data))->sz != 0) {
                        datapack_hook.free((*(datapack_bin **)(child->data))->addr);
                    }
                    datapack_hook.free(*(datapack_bin **)(child->data));
                }
                memcpy(child->data, &bin, sizeof(datapack_bin *));
                if (datav) {
                    datav = datapack_cpv(datav, &bin, sizeof(datapack_bin *));
                    *(datapack_bin **)(child->data) = NULL;
                }
                if (n->type == DATAPACK_TYPE_ARY) {
                    n->ser_osz += sizeof(uint32_t); /* binary buf len word */
                    n->ser_osz += bin->sz;          /* binary buf */
                }
                break;
            case DATAPACK_TYPE_STR:
                for (fidx = 0; fidx < child->num; fidx++) {
                    /* copy the string to be packed. slen includes \0. this
                     block also works if the string pointer is NULL. */
                    char *caddr = ((char **)child->addr)[fidx];
                    char **cdata = &((char **)child->data)[fidx];
                    slen = caddr ? (strlen(caddr) + 1) : 0;
                    if (slen) {
                        str = datapack_hook.malloc(slen);
                        if (!str) fatal_oom();
                        memcpy(str, caddr, slen); /* include \0 */
                    } else {
                        str = NULL;
                    }
                    /* now pack its pointer, first freeing any pre-existing string */
                    if (*cdata != NULL) {
                        datapack_hook.free(*cdata);
                    }
                    memcpy(cdata, &str, sizeof(char *));
                    if (datav) {
                        datav = datapack_cpv(datav, &str, sizeof(char *));
                        *cdata = NULL;
                    }
                    if (n->type == DATAPACK_TYPE_ARY) {
                        n->ser_osz += sizeof(uint32_t);       /* string len word */
                        if (slen > 1) n->ser_osz += slen - 1; /* string (without nul) */
                    }
                }
                break;
            case DATAPACK_TYPE_ARY:
                /* copy the child's datapack_atype* and reset it to empty */
                if (datav) {
                    sz = ((datapack_atyp *)(child->data))->sz;
                    datav = datapack_cpv(datav, &child->data, sizeof(void *));
                    child->data = datapack_hook.malloc(sizeof(datapack_atyp));
                    if (!child->data) fatal_oom();
                    ((datapack_atyp *)(child->data))->num = 0;
                    ((datapack_atyp *)(child->data))->sz = sz;
                    ((datapack_atyp *)(child->data))->bb = NULL;
                    ((datapack_atyp *)(child->data))->bbtail = NULL;
                }
                /* parent is array? then bubble up child array's ser_osz */
                if (n->type == DATAPACK_TYPE_ARY) {
                    n->ser_osz += sizeof(uint32_t); /* array len word */
                    n->ser_osz += child->ser_osz;   /* child array ser_osz */
                    child->ser_osz = 0;             /* reset child array ser_osz */
                }
                break;

            case DATAPACK_TYPE_POUND:
                /* we need to iterate n times over preceding nodes in S(...).
                 * we may be in the midst of an iteration each time or starting. */
                pd = (datapack_pound_data *)child->data;
                itermax = child->num;

                /* itermax is total num of iterations needed  */
                /* pd->iternum is current iteration index  */
                /* pd->inter_elt_len is element-to-element len of contiguous structs */
                /* pd->iter_start_node is where we jump to at each iteration. */

                if (++(pd->iternum) < itermax) {

                    /* in start or midst of loop. advance addr/data pointers. */
                    for (np = pd->iter_start_node; np != child; np = np->next) {
                        np->data = (char *)(np->data) + (datapack_types[np->type].sz * np->num);
                        np->addr = (char *)(np->addr) + pd->inter_elt_len;
                    }
                    /* do next iteration */
                    child = pd->iter_start_node;
                    continue;

                } else { /* loop complete. */

                    /* reset iteration index and addr/data pointers. */
                    pd->iternum = 0;
                    for (np = pd->iter_start_node; np != child; np = np->next) {
                        np->data = (char *)(np->data) - ((itermax - 1) * datapack_types[np->type].sz * np->num);
                        np->addr = (char *)(np->addr) - ((itermax - 1) * pd->inter_elt_len);
                    }
                }
                break;
            default:
                datapack_hook.fatal("unsupported format character\n");
                break;
        }
        child = child->next;
    }
    return 0;
}

int datapack_unpack(datapack_node *r, int i) {
    datapack_node *n, *c, *np;
    uint32_t slen;
    int rc = 1, fidx;
    char *str;
    void *dv = NULL, *caddr;
    size_t A_bytes, itermax;
    datapack_pound_data *pd;
    void *img;
    size_t sz;

    /* handle unusual case of datapack_pack,datapack_unpack without an
     * intervening datapack_dump. do a dump/load implicitly. */
    if (((datapack_root_data *)(r->data))->flags & DATAPACK_WRONLY) {
        if (datapack_dump(r, DATAPACK_MEM, &img, &sz) != 0) return -1;
        if (datapack_load(r, DATAPACK_MEM | DATAPACK_UFREE, img, sz) != 0) {
            datapack_hook.free(img);
            return -1;
        };
    }

    n = datapack_find_i(r, i);
    if (n == NULL) {
        METADOT_ERROR("Datapack: invalid index %d to datapack_unpack\n", i);
        return -1;
    }

    /* either root node or an A node */
    if (n->type == DATAPACK_TYPE_ROOT) {
        dv = datapack_find_data_start(((datapack_root_data *)(n->data))->mmap.text);
    } else if (n->type == DATAPACK_TYPE_ARY) {
        if (((datapack_atyp *)(n->data))->num <= 0)
            return 0; /* array consumed */
        else
            rc = ((datapack_atyp *)(n->data))->num--;
        dv = ((datapack_atyp *)(n->data))->cur;
        if (!dv) datapack_hook.fatal("must unpack parent of node before node itself\n");
    }

    c = n->children;
    while (c) {
        switch (c->type) {
            case DATAPACK_TYPE_BYTE:
            case DATAPACK_TYPE_DOUBLE:
            case DATAPACK_TYPE_INT32:
            case DATAPACK_TYPE_UINT32:
            case DATAPACK_TYPE_INT64:
            case DATAPACK_TYPE_UINT64:
            case DATAPACK_TYPE_INT16:
            case DATAPACK_TYPE_UINT16:
                /* unpack elements of cross-endian octothorpic array individually */
                if (((datapack_root_data *)(r->data))->flags & DATAPACK_XENDIAN) {
                    for (fidx = 0; fidx < c->num; fidx++) {
                        caddr = (void *)((uintptr_t)c->addr + (fidx * datapack_types[c->type].sz));
                        memcpy(caddr, dv, datapack_types[c->type].sz);
                        datapack_byteswap(caddr, datapack_types[c->type].sz);
                        dv = (void *)((uintptr_t)dv + datapack_types[c->type].sz);
                    }
                } else {
                    /* bulk unpack ok if not cross-endian */
                    memcpy(c->addr, dv, datapack_types[c->type].sz * c->num);
                    dv = (void *)((uintptr_t)dv + datapack_types[c->type].sz * c->num);
                }
                break;
            case DATAPACK_TYPE_BIN:
                memcpy(&slen, dv, sizeof(uint32_t));
                if (((datapack_root_data *)(r->data))->flags & DATAPACK_XENDIAN) datapack_byteswap(&slen, sizeof(uint32_t));
                if (slen > 0) {
                    str = (char *)datapack_hook.malloc(slen);
                    if (!str) fatal_oom();
                } else
                    str = NULL;
                dv = (void *)((uintptr_t)dv + sizeof(uint32_t));
                if (slen > 0) memcpy(str, dv, slen);
                memcpy(&(((datapack_bin *)c->addr)->addr), &str, sizeof(void *));
                memcpy(&(((datapack_bin *)c->addr)->sz), &slen, sizeof(uint32_t));
                dv = (void *)((uintptr_t)dv + slen);
                break;
            case DATAPACK_TYPE_STR:
                for (fidx = 0; fidx < c->num; fidx++) {
                    memcpy(&slen, dv, sizeof(uint32_t));
                    if (((datapack_root_data *)(r->data))->flags & DATAPACK_XENDIAN) datapack_byteswap(&slen, sizeof(uint32_t));
                    if (((datapack_root_data *)(r->data))->flags & DATAPACK_OLD_STRING_FMT) slen += 1;
                    dv = (void *)((uintptr_t)dv + sizeof(uint32_t));
                    if (slen) { /* slen includes \0 */
                        str = (char *)datapack_hook.malloc(slen);
                        if (!str) fatal_oom();
                        if (slen > 1) memcpy(str, dv, slen - 1);
                        str[slen - 1] = '\0'; /* nul terminate */
                        dv = (void *)((uintptr_t)dv + slen - 1);
                    } else
                        str = NULL;
                    memcpy(&((char **)c->addr)[fidx], &str, sizeof(char *));
                }
                break;
            case DATAPACK_TYPE_POUND:
                /* iterate over preceding nodes */
                pd = (datapack_pound_data *)c->data;
                itermax = c->num;
                if (++(pd->iternum) < itermax) {
                    /* in start or midst of loop. advance addr/data pointers. */
                    for (np = pd->iter_start_node; np != c; np = np->next) {
                        np->addr = (char *)(np->addr) + pd->inter_elt_len;
                    }
                    /* do next iteration */
                    c = pd->iter_start_node;
                    continue;

                } else { /* loop complete. */

                    /* reset iteration index and addr/data pointers. */
                    pd->iternum = 0;
                    for (np = pd->iter_start_node; np != c; np = np->next) {
                        np->addr = (char *)(np->addr) - ((itermax - 1) * pd->inter_elt_len);
                    }
                }
                break;
            case DATAPACK_TYPE_ARY:
                if (datapack_serlen(r, c, dv, &A_bytes) == -1) datapack_hook.fatal("internal error in unpack\n");
                memcpy(&((datapack_atyp *)(c->data))->num, dv, sizeof(uint32_t));
                if (((datapack_root_data *)(r->data))->flags & DATAPACK_XENDIAN) datapack_byteswap(&((datapack_atyp *)(c->data))->num, sizeof(uint32_t));
                ((datapack_atyp *)(c->data))->cur = (void *)((uintptr_t)dv + sizeof(uint32_t));
                dv = (void *)((uintptr_t)dv + A_bytes);
                break;
            default:
                datapack_hook.fatal("unsupported format character\n");
                break;
        }

        c = c->next;
    }
    if (n->type == DATAPACK_TYPE_ARY) ((datapack_atyp *)(n->data))->cur = dv; /* next element */
    return rc;
}

/* Specialized function that unpacks only the root's A nodes, after datapack_load  */
static int datapack_unpackA0(datapack_node *r) {
    datapack_node *n, *c;
    uint32_t slen;
    int rc = 1, fidx, i;
    void *dv;
    size_t A_bytes, itermax;
    datapack_pound_data *pd;

    n = r;
    dv = datapack_find_data_start(((datapack_root_data *)(r->data))->mmap.text);

    c = n->children;
    while (c) {
        switch (c->type) {
            case DATAPACK_TYPE_BYTE:
            case DATAPACK_TYPE_DOUBLE:
            case DATAPACK_TYPE_INT32:
            case DATAPACK_TYPE_UINT32:
            case DATAPACK_TYPE_INT64:
            case DATAPACK_TYPE_UINT64:
            case DATAPACK_TYPE_INT16:
            case DATAPACK_TYPE_UINT16:
                for (fidx = 0; fidx < c->num; fidx++) {
                    dv = (void *)((uintptr_t)dv + datapack_types[c->type].sz);
                }
                break;
            case DATAPACK_TYPE_BIN:
                memcpy(&slen, dv, sizeof(uint32_t));
                if (((datapack_root_data *)(r->data))->flags & DATAPACK_XENDIAN) datapack_byteswap(&slen, sizeof(uint32_t));
                dv = (void *)((uintptr_t)dv + sizeof(uint32_t));
                dv = (void *)((uintptr_t)dv + slen);
                break;
            case DATAPACK_TYPE_STR:
                for (i = 0; i < c->num; i++) {
                    memcpy(&slen, dv, sizeof(uint32_t));
                    if (((datapack_root_data *)(r->data))->flags & DATAPACK_XENDIAN) datapack_byteswap(&slen, sizeof(uint32_t));
                    if (((datapack_root_data *)(r->data))->flags & DATAPACK_OLD_STRING_FMT) slen += 1;
                    dv = (void *)((uintptr_t)dv + sizeof(uint32_t));
                    if (slen > 1) dv = (void *)((uintptr_t)dv + slen - 1);
                }
                break;
            case DATAPACK_TYPE_POUND:
                /* iterate over the preceding nodes */
                itermax = c->num;
                pd = (datapack_pound_data *)c->data;
                if (++(pd->iternum) < itermax) {
                    c = pd->iter_start_node;
                    continue;
                } else { /* loop complete. */
                    pd->iternum = 0;
                }
                break;
            case DATAPACK_TYPE_ARY:
                if (datapack_serlen(r, c, dv, &A_bytes) == -1) datapack_hook.fatal("internal error in unpackA0\n");
                memcpy(&((datapack_atyp *)(c->data))->num, dv, sizeof(uint32_t));
                if (((datapack_root_data *)(r->data))->flags & DATAPACK_XENDIAN) datapack_byteswap(&((datapack_atyp *)(c->data))->num, sizeof(uint32_t));
                ((datapack_atyp *)(c->data))->cur = (void *)((uintptr_t)dv + sizeof(uint32_t));
                dv = (void *)((uintptr_t)dv + A_bytes);
                break;
            default:
                datapack_hook.fatal("unsupported format character\n");
                break;
        }
        c = c->next;
    }
    return rc;
}

/* In-place byte order swapping of a word of length "len" bytes */
static void datapack_byteswap(void *word, int len) {
    int i;
    char c, *w;
    w = (char *)word;
    for (i = 0; i < len / 2; i++) {
        c = w[i];
        w[i] = w[len - 1 - i];
        w[len - 1 - i] = c;
    }
}

static void datapack_fatal(const char *fmt, ...) {
    va_list ap;
    char exit_msg[100];

    va_start(ap, fmt);
    vsnprintf(exit_msg, 100, fmt, ap);
    va_end(ap);

    METADOT_ERROR("Datapack: %s", exit_msg);
    exit(-1);
}

int datapack_gather(int mode, ...) {
    va_list ap;
    int fd, rc = 0;
    size_t *szp, sz;
    void **img, *addr, *data;
    datapack_gather_t **gs;
    datapack_gather_cb *cb;

    va_start(ap, mode);
    switch (mode) {
        case DATAPACK_GATHER_BLOCKING:
            fd = va_arg(ap, int);
            img = va_arg(ap, void *);
            szp = va_arg(ap, size_t *);
            rc = datapack_gather_blocking(fd, img, szp);
            break;
        case DATAPACK_GATHER_NONBLOCKING:
            fd = va_arg(ap, int);
            gs = (datapack_gather_t **)va_arg(ap, void *);
            cb = (datapack_gather_cb *)va_arg(ap, datapack_gather_cb *);
            data = va_arg(ap, void *);
            rc = datapack_gather_nonblocking(fd, gs, cb, data);
            break;
        case DATAPACK_GATHER_MEM:
            addr = va_arg(ap, void *);
            sz = va_arg(ap, size_t);
            gs = (datapack_gather_t **)va_arg(ap, void *);
            cb = (datapack_gather_cb *)va_arg(ap, datapack_gather_cb *);
            data = va_arg(ap, void *);
            rc = datapack_gather_mem(addr, sz, gs, cb, data);
            break;
        default:
            datapack_hook.fatal("unsupported datapack_gather mode %d\n", mode);
            break;
    }
    va_end(ap);
    return rc;
}

/* dequeue a datapack by reading until one full datapack image is obtained.
 * We take care not to read past the end of the datapack.
 * This is intended as a blocking call i.e. for use with a blocking fd.
 * It can be given a non-blocking fd, but the read spins if we have to wait.
 */
static int datapack_gather_blocking(int fd, void **img, size_t *sz) {
    char preamble[8];
    int i = 0, rc;
    uint32_t datapacklen;

    do {
        rc = read(fd, &preamble[i], 8 - i);
        i += (rc > 0) ? rc : 0;
    } while ((rc == -1 && (errno == EINTR || errno == EAGAIN)) || (rc > 0 && i < 8));

    if (rc < 0) {
        METADOT_ERROR("Datapack: datapack_gather_fd_blocking failed: %s\n", strerror(errno));
        return -1;
    } else if (rc == 0) {
        /* METADOT_ERROR("Datapack: datapack_gather_fd_blocking: eof\n"); */
        return 0;
    } else if (i != 8) {
        METADOT_ERROR("Datapack: internal error\n");
        return -1;
    }

    if (preamble[0] == 't' && preamble[1] == 'p' && preamble[2] == 'l') {
        memcpy(&datapacklen, &preamble[4], 4);
        if (datapack_needs_endian_swap(preamble)) datapack_byteswap(&datapacklen, 4);
    } else {
        METADOT_ERROR("Datapack: datapack_gather_fd_blocking: non-datapack input\n");
        return -1;
    }

    /* malloc space for remainder of datapack image (overall length datapacklen)
     * and read it in
     */
    if (datapack_hook.gather_max > 0 && datapacklen > datapack_hook.gather_max) {
        METADOT_ERROR("Datapack: datapack exceeds max length %d\n", datapack_hook.gather_max);
        return -2;
    }
    *sz = datapacklen;
    if ((*img = datapack_hook.malloc(datapacklen)) == NULL) {
        fatal_oom();
    }

    memcpy(*img, preamble, 8); /* copy preamble to output buffer */
    i = 8;
    do {
        rc = read(fd, &((*(char **)img)[i]), datapacklen - i);
        i += (rc > 0) ? rc : 0;
    } while ((rc == -1 && (errno == EINTR || errno == EAGAIN)) || (rc > 0 && i < datapacklen));

    if (rc < 0) {
        METADOT_ERROR("Datapack: datapack_gather_fd_blocking failed: %s\n", strerror(errno));
        datapack_hook.free(*img);
        return -1;
    } else if (rc == 0) {
        /* METADOT_ERROR("Datapack: datapack_gather_fd_blocking: eof\n"); */
        datapack_hook.free(*img);
        return 0;
    } else if (i != datapacklen) {
        METADOT_ERROR("Datapack: internal error\n");
        datapack_hook.free(*img);
        return -1;
    }

    return 1;
}

/* Used by select()-driven apps which want to gather datapack images piecemeal */
/* the file descriptor must be non-blocking for this functino to work. */
static int datapack_gather_nonblocking(int fd, datapack_gather_t **gs, datapack_gather_cb *cb, void *data) {
    char buf[DATAPACK_GATHER_BUFLEN], *img, *datapack;
    int rc, keep_looping, cbrc = 0;
    size_t catlen;
    uint32_t datapacklen;

    while (1) {
        rc = read(fd, buf, DATAPACK_GATHER_BUFLEN);
        if (rc == -1) {
            if (errno == EINTR) continue; /* got signal during read, ignore */
            if (errno == EAGAIN)
                return 1; /* nothing to read right now */
            else {
                METADOT_ERROR("Datapack: datapack_gather failed: %s\n", strerror(errno));
                if (*gs) {
                    datapack_hook.free((*gs)->img);
                    datapack_hook.free(*gs);
                    *gs = NULL;
                }
                return -1; /* error, caller should close fd  */
            }
        } else if (rc == 0) {
            if (*gs) {
                METADOT_ERROR("Datapack: datapack_gather: partial datapack image precedes EOF\n");
                datapack_hook.free((*gs)->img);
                datapack_hook.free(*gs);
                *gs = NULL;
            }
            return 0; /* EOF, caller should close fd */
        } else {
            /* concatenate any partial datapack from last read with new buffer */
            if (*gs) {
                catlen = (*gs)->len + rc;
                if (datapack_hook.gather_max > 0 && catlen > datapack_hook.gather_max) {
                    datapack_hook.free((*gs)->img);
                    datapack_hook.free((*gs));
                    *gs = NULL;
                    METADOT_ERROR("Datapack: datapack exceeds max length %d\n", datapack_hook.gather_max);
                    return -2; /* error, caller should close fd */
                }
                if ((img = datapack_hook.realloc((*gs)->img, catlen)) == NULL) {
                    fatal_oom();
                }
                memcpy(img + (*gs)->len, buf, rc);
                datapack_hook.free(*gs);
                *gs = NULL;
            } else {
                img = buf;
                catlen = rc;
            }
            /* isolate any full datapack(s) in img and invoke cb for each */
            datapack = img;
            keep_looping = (datapack + 8 < img + catlen) ? 1 : 0;
            while (keep_looping) {
                if (strncmp("datapack", datapack, 3) != 0) {
                    METADOT_ERROR("Datapack: datapack prefix invalid\n");
                    if (img != buf) datapack_hook.free(img);
                    datapack_hook.free(*gs);
                    *gs = NULL;
                    return -3; /* error, caller should close fd */
                }
                memcpy(&datapacklen, &datapack[4], 4);
                if (datapack_needs_endian_swap(datapack)) datapack_byteswap(&datapacklen, 4);
                if (datapack + datapacklen <= img + catlen) {
                    cbrc = (cb)(datapack, datapacklen, data); /* invoke cb for datapack image */
                    datapack += datapacklen;                  /* point to next datapack image */
                    if (cbrc < 0)
                        keep_looping = 0;
                    else
                        keep_looping = (datapack + 8 < img + catlen) ? 1 : 0;
                } else
                    keep_looping = 0;
            }
            /* check if app callback requested closure of datapack source */
            if (cbrc < 0) {
                METADOT_ERROR("Datapack: datapack_fd_gather aborted by app callback\n");
                if (img != buf) datapack_hook.free(img);
                if (*gs) datapack_hook.free(*gs);
                *gs = NULL;
                return -4;
            }
            /* store any leftover, partial datapack fragment for next read */
            if (datapack == img && img != buf) {
                /* consumed nothing from img!=buf */
                if ((*gs = datapack_hook.malloc(sizeof(datapack_gather_t))) == NULL) {
                    fatal_oom();
                }
                (*gs)->img = datapack;
                (*gs)->len = catlen;
            } else if (datapack < img + catlen) {
                /* consumed 1+ datapack(s) from img!=buf or 0 from img==buf */
                if ((*gs = datapack_hook.malloc(sizeof(datapack_gather_t))) == NULL) {
                    fatal_oom();
                }
                if (((*gs)->img = datapack_hook.malloc(img + catlen - datapack)) == NULL) {
                    fatal_oom();
                }
                (*gs)->len = img + catlen - datapack;
                memcpy((*gs)->img, datapack, img + catlen - datapack);
                /* free partially consumed concat buffer if used */
                if (img != buf) datapack_hook.free(img);
            } else { /* datapack(s) fully consumed */
                /* free consumed concat buffer if used */
                if (img != buf) datapack_hook.free(img);
            }
        }
    }
}

/* gather datapack piecemeal from memory buffer (not fd) e.g., from a lower-level api */
static int datapack_gather_mem(char *buf, size_t len, datapack_gather_t **gs, datapack_gather_cb *cb, void *data) {
    char *img, *datapack;
    int keep_looping, cbrc = 0;
    size_t catlen;
    uint32_t datapacklen;

    /* concatenate any partial datapack from last read with new buffer */
    if (*gs) {
        catlen = (*gs)->len + len;
        if (datapack_hook.gather_max > 0 && catlen > datapack_hook.gather_max) {
            datapack_hook.free((*gs)->img);
            datapack_hook.free((*gs));
            *gs = NULL;
            METADOT_ERROR("Datapack: datapack exceeds max length %d\n", datapack_hook.gather_max);
            return -2; /* error, caller should stop accepting input from source*/
        }
        if ((img = datapack_hook.realloc((*gs)->img, catlen)) == NULL) {
            fatal_oom();
        }
        memcpy(img + (*gs)->len, buf, len);
        datapack_hook.free(*gs);
        *gs = NULL;
    } else {
        img = buf;
        catlen = len;
    }
    /* isolate any full datapack(s) in img and invoke cb for each */
    datapack = img;
    keep_looping = (datapack + 8 < img + catlen) ? 1 : 0;
    while (keep_looping) {
        if (strncmp("datapack", datapack, 3) != 0) {
            METADOT_ERROR("Datapack: datapack prefix invalid\n");
            if (img != buf) datapack_hook.free(img);
            datapack_hook.free(*gs);
            *gs = NULL;
            return -3; /* error, caller should stop accepting input from source*/
        }
        memcpy(&datapacklen, &datapack[4], 4);
        if (datapack_needs_endian_swap(datapack)) datapack_byteswap(&datapacklen, 4);
        if (datapack + datapacklen <= img + catlen) {
            cbrc = (cb)(datapack, datapacklen, data); /* invoke cb for datapack image */
            datapack += datapacklen;                  /* point to next datapack image */
            if (cbrc < 0)
                keep_looping = 0;
            else
                keep_looping = (datapack + 8 < img + catlen) ? 1 : 0;
        } else
            keep_looping = 0;
    }
    /* check if app callback requested closure of datapack source */
    if (cbrc < 0) {
        METADOT_ERROR("Datapack: datapack_mem_gather aborted by app callback\n");
        if (img != buf) datapack_hook.free(img);
        if (*gs) datapack_hook.free(*gs);
        *gs = NULL;
        return -4;
    }
    /* store any leftover, partial datapack fragment for next read */
    if (datapack == img && img != buf) {
        /* consumed nothing from img!=buf */
        if ((*gs = datapack_hook.malloc(sizeof(datapack_gather_t))) == NULL) {
            fatal_oom();
        }
        (*gs)->img = datapack;
        (*gs)->len = catlen;
    } else if (datapack < img + catlen) {
        /* consumed 1+ datapack(s) from img!=buf or 0 from img==buf */
        if ((*gs = datapack_hook.malloc(sizeof(datapack_gather_t))) == NULL) {
            fatal_oom();
        }
        if (((*gs)->img = datapack_hook.malloc(img + catlen - datapack)) == NULL) {
            fatal_oom();
        }
        (*gs)->len = img + catlen - datapack;
        memcpy((*gs)->img, datapack, img + catlen - datapack);
        /* free partially consumed concat buffer if used */
        if (img != buf) datapack_hook.free(img);
    } else { /* datapack(s) fully consumed */
        /* free consumed concat buffer if used */
        if (img != buf) datapack_hook.free(img);
    }
    return 1;
}
