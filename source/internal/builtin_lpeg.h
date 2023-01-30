// Metadot lpeg is enhanced based on lpeg modification
// Metadot code Copyright(c) 2022-2023, KaoruXun All rights reserved.
// lpeg code by Lua.org, PUC-Rio licensed under following license
// http://www.inf.puc-rio.br/~roberto/lpeg/

// Copyright 2007-2019 Lua.org, PUC-Rio.
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to
// do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED
// "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
// THE USE OR OTHER DEALINGS IN THE SOFTWARE.

/*
** LPeg - PEG pattern matching for Lua
** Copyright 2007-2019, Lua.org & PUC-Rio
** written by Roberto Ierusalimschy
*/

#if !defined(lpeg_h)
#define lpeg_h

#include <assert.h>
#include <limits.h>

#include "scripting/lua_wrapper.h"

#define VERSION "1.0.2"

#define PATTERN_T "lpeg-pattern"
#define MAXSTACKIDX "lpeg-maxstack"

/*
** compatibility with Lua 5.1
*/
#if (LUA_VERSION_NUM == 501)

#define lp_equal lua_equal

#define lua_getuservalue lua_getfenv
#define lua_setuservalue lua_setfenv

#define lua_rawlen lua_objlen

#define luaL_setfuncs(L, f, n) luaL_register(L, NULL, f)
#define luaL_newlib(L, f) luaL_register(L, "lpeg", f)

#endif

#if !defined(lp_equal)
#define lp_equal(L, idx1, idx2) lua_compare(L, (idx1), (idx2), LUA_OPEQ)
#endif

/* default maximum size for call/backtrack stack */
#if !defined(MAXBACK)
#define MAXBACK 400
#endif

/* maximum number of rules in a grammar (limited by 'unsigned char') */
#if !defined(MAXRULES)
#define MAXRULES 250
#endif

/* initial size for capture's list */
#define INITCAPSIZE 32

/* index, on Lua stack, for subject */
#define SUBJIDX 2

/* number of fixed arguments to 'match' (before capture arguments) */
#define FIXEDARGS 3

/* index, on Lua stack, for capture list */
#define caplistidx(ptop) ((ptop) + 2)

/* index, on Lua stack, for pattern's ktable */
#define ktableidx(ptop) ((ptop) + 3)

/* index, on Lua stack, for backtracking stack */
#define stackidx(ptop) ((ptop) + 4)

typedef unsigned char byte;

#define BITSPERCHAR 8

#define CHARSETSIZE ((UCHAR_MAX / BITSPERCHAR) + 1)

typedef struct Charset {
    byte cs[CHARSETSIZE];
} Charset;

#define loopset(v, b)                       \
    {                                       \
        int v;                              \
        for (v = 0; v < CHARSETSIZE; v++) { \
            b;                              \
        }                                   \
    }

/* access to charset */
#define treebuffer(t) ((byte *)((t) + 1))

/* number of slots needed for 'n' bytes */
#define bytes2slots(n) (((n)-1) / sizeof(TTree) + 1)

/* set 'b' bit in charset 'cs' */
#define setchar(cs, b) ((cs)[(b) >> 3] |= (1 << ((b)&7)))

/*
** in capture instructions, 'kind' of capture and its offset are
** packed in field 'aux', 4 bits for each
*/
#define getkind(op) ((op)->i.aux & 0xF)
#define getoff(op) (((op)->i.aux >> 4) & 0xF)
#define joinkindoff(k, o) ((k) | ((o) << 4))

#define MAXOFF 0xF
#define MAXAUX 0xFF

/* maximum number of bytes to look behind */
#define MAXBEHIND MAXAUX

/* maximum size (in elements) for a pattern */
#define MAXPATTSIZE (SHRT_MAX - 10)

/* size (in elements) for an instruction plus extra l bytes */
#define instsize(l) (((l) + sizeof(Instruction) - 1) / sizeof(Instruction) + 1)

/* size (in elements) for a ISet instruction */
#define CHARSETINSTSIZE instsize(CHARSETSIZE)

/* size (in elements) for a IFunc instruction */
#define funcinstsize(p) ((p)->i.aux + 2)

#define testchar(st, c) (((int)(st)[((c) >> 3)] & (1 << ((c)&7))))

/* kinds of captures */
typedef enum CapKind {
    Cclose, /* not used in trees */
    Cposition,
    Cconst,    /* ktable[key] is Lua constant */
    Cbackref,  /* ktable[key] is "name" of group to get capture */
    Carg,      /* 'key' is arg's number */
    Csimple,   /* next node is pattern */
    Ctable,    /* next node is pattern */
    Cfunction, /* ktable[key] is function; next node is pattern */
    Cquery,    /* ktable[key] is table; next node is pattern */
    Cstring,   /* ktable[key] is string; next node is pattern */
    Cnum,      /* numbered capture; 'key' is number of value to return */
    Csubst,    /* substitution capture; next node is pattern */
    Cfold,     /* ktable[key] is function; next node is pattern */
    Cruntime,  /* not used in trees (is uses another type for tree) */
    Cgroup     /* ktable[key] is group's "name" */
} CapKind;

typedef struct Capture {
    const char *s;      /* subject position */
    unsigned short idx; /* extra info (group name, arg index, etc.) */
    byte kind;          /* kind of capture */
    byte siz;           /* size of full capture + 1 (0 = not a full capture) */
} Capture;

typedef struct CapState {
    Capture *cap;  /* current capture */
    Capture *ocap; /* (original) capture list */
    lua_State *L;
    int ptop;        /* index of last argument to 'match' */
    const char *s;   /* original string */
    int valuecached; /* value stored in cache slot */
    int reclevel;    /* recursion level */
} CapState;

int runtimecap(CapState *cs, Capture *close, const char *s, int *rem);
int getcaptures(lua_State *L, const char *s, const char *r, int ptop);
int finddyncap(Capture *cap, Capture *last);

/* Virtual Machine's instructions */
typedef enum Opcode {
    IAny,           /* if no char, fail */
    IChar,          /* if char != aux, fail */
    ISet,           /* if char not in buff, fail */
    ITestAny,       /* in no char, jump to 'offset' */
    ITestChar,      /* if char != aux, jump to 'offset' */
    ITestSet,       /* if char not in buff, jump to 'offset' */
    ISpan,          /* read a span of chars in buff */
    IBehind,        /* walk back 'aux' characters (fail if not possible) */
    IRet,           /* return from a rule */
    IEnd,           /* end of pattern */
    IChoice,        /* stack a choice; next fail will jump to 'offset' */
    IJmp,           /* jump to 'offset' */
    ICall,          /* call rule at 'offset' */
    IOpenCall,      /* call rule number 'key' (must be closed to a ICall) */
    ICommit,        /* pop choice and jump to 'offset' */
    IPartialCommit, /* update top choice to current position and jump */
    IBackCommit,    /* "fails" but jump to its own 'offset' */
    IFailTwice,     /* pop one choice and then fail */
    IFail,          /* go back to saved state on choice and jump to saved offset */
    IGiveup,        /* internal use */
    IFullCapture,   /* complete capture of last 'off' chars */
    IOpenCapture,   /* start a capture */
    ICloseCapture,
    ICloseRunTime
} Opcode;

typedef union Instruction {
    struct Inst {
        byte code;
        byte aux;
        short key;
    } i;
    int offset;
    byte buff[1];
} Instruction;

void printpatt(Instruction *p, int n);
const char *match(lua_State *L, const char *o, const char *s, const char *e, Instruction *op, Capture *capture, int ptop);

/*
** types of trees
*/
typedef enum TTag {
    TChar = 0, /* 'n' = char */
    TSet,      /* the set is stored in next CHARSETSIZE bytes */
    TAny,
    TTrue,
    TFalse,
    TRep,      /* 'sib1'* */
    TSeq,      /* 'sib1' 'sib2' */
    TChoice,   /* 'sib1' / 'sib2' */
    TNot,      /* !'sib1' */
    TAnd,      /* &'sib1' */
    TCall,     /* ktable[key] is rule's key; 'sib2' is rule being called */
    TOpenCall, /* ktable[key] is rule's key */
    TRule,     /* ktable[key] is rule's key (but key == 0 for unused rules);
             'sib1' is rule's pattern;
             'sib2' is next rule; 'cap' is rule's sequential number */
    TGrammar,  /* 'sib1' is initial (and first) rule */
    TBehind,   /* 'sib1' is pattern, 'n' is how much to go back */
    TCapture,  /* captures: 'cap' is kind of capture (enum 'CapKind');
                ktable[key] is Lua value associated with capture;
                'sib1' is capture body */
    TRunTime   /* run-time capture: 'key' is Lua function;
               'sib1' is capture body */
} TTag;

/*
** Tree trees
** The first child of a tree (if there is one) is immediately after
** the tree.  A reference to a second child (ps) is its position
** relative to the position of the tree itself.
*/
typedef struct TTree {
    byte tag;
    byte cap;           /* kind of capture (if it is a capture) */
    unsigned short key; /* key in ktable for Lua data (0 if no key) */
    union {
        int ps; /* occasional second child */
        int n;  /* occasional counter */
    } u;
} TTree;

/*
** A complete pattern has its tree plus, if already compiled,
** its corresponding code
*/
typedef struct Pattern {
    union Instruction *code;
    int codesize;
    TTree tree[1];
} Pattern;

/* number of children for each tree */
extern const byte numsiblings[];

/* access to children */
#define sib1(t) ((t) + 1)
#define sib2(t) ((t) + (t)->u.ps)

#if defined(LPEG_DEBUG)

void printpatt(Instruction *p, int n);
void printtree(TTree *tree, int ident);
void printktable(lua_State *L, int idx);
void printcharset(const byte *st);
void printcaplist(Capture *cap, Capture *limit);
void printinst(const Instruction *op, const Instruction *p);

#else

#define printktable(L, idx) luaL_error(L, "function only implemented in debug mode")
#define printtree(tree, i) luaL_error(L, "function only implemented in debug mode")
#define printpatt(p, n) luaL_error(L, "function only implemented in debug mode")

#endif

int tocharset(TTree *tree, Charset *cs);
int checkaux(TTree *tree, int pred);
int fixedlen(TTree *tree);
int hascaptures(TTree *tree);
int lp_gc(lua_State *L);
Instruction *compile(lua_State *L, Pattern *p);
void realloccode(lua_State *L, Pattern *p, int nsize);
int sizei(const Instruction *i);

#define PEnullable 0
#define PEnofail 1

/*
** nofail(t) implies that 't' cannot fail with any input
*/
#define nofail(t) checkaux(t, PEnofail)

/*
** (not nullable(t)) implies 't' cannot match without consuming
** something
*/
#define nullable(t) checkaux(t, PEnullable)

#if defined(__cplusplus)
extern "C" {
#endif
int luaopen_lpeg(lua_State *L);
#if defined(__cplusplus)
}
#endif

#endif
