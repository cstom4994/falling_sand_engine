// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "utils.h"

#include <ctype.h>
#include <stdio.h>

#include "core/const.h"
#include "core/core.h"

/* Definitions: */

#define MAX_REGEXP_OBJECTS 30 /* Max number of regex symbols in expression. */
#define MAX_CHAR_CLASS_LEN 40 /* Max length of character-class buffer in.   */

enum { UNUSED, DOT, BEGIN, END, QUESTIONMARK, STAR, PLUS, CHAR, CHAR_CLASS, INV_CHAR_CLASS, DIGIT, NOT_DIGIT, ALPHA, NOT_ALPHA, WHITESPACE, NOT_WHITESPACE, /* BRANCH */ };

typedef struct regex_t {
    unsigned char type; /* CHAR, STAR, etc.                      */
    union {
        unsigned char ch;   /*      the character itself             */
        unsigned char *ccl; /*  OR  a pointer to characters in class */
    } u;
} regex_t;

/* Private function declarations: */
static int matchpattern(regex_t *pattern, const char *text, int *matchlength);
static int matchcharclass(char c, const char *str);
static int matchstar(regex_t p, regex_t *pattern, const char *text, int *matchlength);
static int matchplus(regex_t p, regex_t *pattern, const char *text, int *matchlength);
static int matchone(regex_t p, char c);
static int matchdigit(char c);
static int matchalpha(char c);
static int matchwhitespace(char c);
static int matchmetachar(char c, const char *str);
static int matchrange(char c, const char *str);
static int matchdot(char c);
static int ismetachar(char c);

/* Public functions: */
int re_match(const char *pattern, const char *text, int *matchlength) { return re_matchp(re_compile(pattern), text, matchlength); }

int re_matchp(re_t pattern, const char *text, int *matchlength) {
    *matchlength = 0;
    if (pattern != 0) {
        if (pattern[0].type == BEGIN) {
            return ((matchpattern(&pattern[1], text, matchlength)) ? 0 : -1);
        } else {
            int idx = -1;

            do {
                idx += 1;

                if (matchpattern(pattern, text, matchlength)) {
                    if (text[0] == '\0') return -1;

                    return idx;
                }
            } while (*text++ != '\0');
        }
    }
    return -1;
}

re_t re_compile(const char *pattern) {
    /* The sizes of the two static arrays below substantiates the static RAM usage of this module.
       MAX_REGEXP_OBJECTS is the max number of symbols in the expression.
       MAX_CHAR_CLASS_LEN determines the size of buffer for chars in all char-classes in the expression. */
    static regex_t re_compiled[MAX_REGEXP_OBJECTS];
    static unsigned char ccl_buf[MAX_CHAR_CLASS_LEN];
    int ccl_bufidx = 1;

    char c;    /* current char in pattern   */
    int i = 0; /* index into pattern        */
    int j = 0; /* index into re_compiled    */

    while (pattern[i] != '\0' && (j + 1 < MAX_REGEXP_OBJECTS)) {
        c = pattern[i];

        switch (c) {
            /* Meta-characters: */
            case '^': {
                re_compiled[j].type = BEGIN;
            } break;
            case '$': {
                re_compiled[j].type = END;
            } break;
            case '.': {
                re_compiled[j].type = DOT;
            } break;
            case '*': {
                re_compiled[j].type = STAR;
            } break;
            case '+': {
                re_compiled[j].type = PLUS;
            } break;
            case '?': {
                re_compiled[j].type = QUESTIONMARK;
            } break;
                /*    case '|': {    re_compiled[j].type = BRANCH;          } break; <-- not working properly */

            /* Escaped character-classes (\s \w ...): */
            case '\\': {
                if (pattern[i + 1] != '\0') {
                    /* Skip the escape-char '\\' */
                    i += 1;
                    /* ... and check the next */
                    switch (pattern[i]) {
                        /* Meta-character: */
                        case 'd': {
                            re_compiled[j].type = DIGIT;
                        } break;
                        case 'D': {
                            re_compiled[j].type = NOT_DIGIT;
                        } break;
                        case 'w': {
                            re_compiled[j].type = ALPHA;
                        } break;
                        case 'W': {
                            re_compiled[j].type = NOT_ALPHA;
                        } break;
                        case 's': {
                            re_compiled[j].type = WHITESPACE;
                        } break;
                        case 'S': {
                            re_compiled[j].type = NOT_WHITESPACE;
                        } break;

                        /* Escaped character, e.g. '.' or '$' */
                        default: {
                            re_compiled[j].type = CHAR;
                            re_compiled[j].u.ch = pattern[i];
                        } break;
                    }
                }
                /* '\\' as last char in pattern -> invalid regular expression. */
                /*
                        else
                        {
                          re_compiled[j].type = CHAR;
                          re_compiled[j].ch = pattern[i];
                        }
                */
            } break;

            /* Character class: */
            case '[': {
                /* Remember where the char-buffer starts. */
                int buf_begin = ccl_bufidx;

                /* Look-ahead to determine if negated */
                if (pattern[i + 1] == '^') {
                    re_compiled[j].type = INV_CHAR_CLASS;
                    i += 1;                  /* Increment i to avoid including '^' in the char-buffer */
                    if (pattern[i + 1] == 0) /* incomplete pattern, missing non-zero char after '^' */
                    {
                        return 0;
                    }
                } else {
                    re_compiled[j].type = CHAR_CLASS;
                }

                /* Copy characters inside [..] to buffer */
                while ((pattern[++i] != ']') && (pattern[i] != '\0')) /* Missing ] */
                {
                    if (pattern[i] == '\\') {
                        if (ccl_bufidx >= MAX_CHAR_CLASS_LEN - 1) {
                            // fputs("exceeded internal buffer!\n", stderr);
                            return 0;
                        }
                        if (pattern[i + 1] == 0) /* incomplete pattern, missing non-zero char after '\\' */
                        {
                            return 0;
                        }
                        ccl_buf[ccl_bufidx++] = pattern[i++];
                    } else if (ccl_bufidx >= MAX_CHAR_CLASS_LEN) {
                        // fputs("exceeded internal buffer!\n", stderr);
                        return 0;
                    }
                    ccl_buf[ccl_bufidx++] = pattern[i];
                }
                if (ccl_bufidx >= MAX_CHAR_CLASS_LEN) {
                    /* Catches cases such as [00000000000000000000000000000000000000][ */
                    // fputs("exceeded internal buffer!\n", stderr);
                    return 0;
                }
                /* Null-terminate string end */
                ccl_buf[ccl_bufidx++] = 0;
                re_compiled[j].u.ccl = &ccl_buf[buf_begin];
            } break;

            /* Other characters: */
            default: {
                re_compiled[j].type = CHAR;
                re_compiled[j].u.ch = c;
            } break;
        }
        /* no buffer-out-of-bounds access on invalid patterns - see https://github.com/kokke/tiny-regex-c/commit/1a279e04014b70b0695fba559a7c05d55e6ee90b */
        if (pattern[i] == 0) {
            return 0;
        }

        i += 1;
        j += 1;
    }
    /* 'UNUSED' is a sentinel used to indicate end-of-pattern */
    re_compiled[j].type = UNUSED;

    return (re_t)re_compiled;
}

void re_print(regex_t *pattern) {
    const char *types[] = {"UNUSED",         "DOT",   "BEGIN",     "END",   "QUESTIONMARK", "STAR",       "PLUS",           "CHAR",  "CHAR_CLASS",
                           "INV_CHAR_CLASS", "DIGIT", "NOT_DIGIT", "ALPHA", "NOT_ALPHA",    "WHITESPACE", "NOT_WHITESPACE", "BRANCH"};

    int i;
    int j;
    char c;
    for (i = 0; i < MAX_REGEXP_OBJECTS; ++i) {
        if (pattern[i].type == UNUSED) {
            break;
        }

        printf("type: %s", types[pattern[i].type]);
        if (pattern[i].type == CHAR_CLASS || pattern[i].type == INV_CHAR_CLASS) {
            printf(" [");
            for (j = 0; j < MAX_CHAR_CLASS_LEN; ++j) {
                c = pattern[i].u.ccl[j];
                if ((c == '\0') || (c == ']')) {
                    break;
                }
                printf("%c", c);
            }
            printf("]");
        } else if (pattern[i].type == CHAR) {
            printf(" '%c'", pattern[i].u.ch);
        }
        printf("\n");
    }
}

/* Private functions: */
static int matchdigit(char c) { return isdigit(c); }
static int matchalpha(char c) { return isalpha(c); }
static int matchwhitespace(char c) { return isspace(c); }
static int matchalphanum(char c) { return ((c == '_') || matchalpha(c) || matchdigit(c)); }
static int matchrange(char c, const char *str) { return ((c != '-') && (str[0] != '\0') && (str[0] != '-') && (str[1] == '-') && (str[2] != '\0') && ((c >= str[0]) && (c <= str[2]))); }
static int matchdot(char c) {
#if defined(RE_DOT_MATCHES_NEWLINE) && (RE_DOT_MATCHES_NEWLINE == 1)
    (void)c;
    return 1;
#else
    return c != '\n' && c != '\r';
#endif
}
static int ismetachar(char c) { return ((c == 's') || (c == 'S') || (c == 'w') || (c == 'W') || (c == 'd') || (c == 'D')); }

static int matchmetachar(char c, const char *str) {
    switch (str[0]) {
        case 'd':
            return matchdigit(c);
        case 'D':
            return !matchdigit(c);
        case 'w':
            return matchalphanum(c);
        case 'W':
            return !matchalphanum(c);
        case 's':
            return matchwhitespace(c);
        case 'S':
            return !matchwhitespace(c);
        default:
            return (c == str[0]);
    }
}

static int matchcharclass(char c, const char *str) {
    do {
        if (matchrange(c, str)) {
            return 1;
        } else if (str[0] == '\\') {
            /* Escape-char: increment str-ptr and match on next char */
            str += 1;
            if (matchmetachar(c, str)) {
                return 1;
            } else if ((c == str[0]) && !ismetachar(c)) {
                return 1;
            }
        } else if (c == str[0]) {
            if (c == '-') {
                return ((str[-1] == '\0') || (str[1] == '\0'));
            } else {
                return 1;
            }
        }
    } while (*str++ != '\0');

    return 0;
}

static int matchone(regex_t p, char c) {
    switch (p.type) {
        case DOT:
            return matchdot(c);
        case CHAR_CLASS:
            return matchcharclass(c, (const char *)p.u.ccl);
        case INV_CHAR_CLASS:
            return !matchcharclass(c, (const char *)p.u.ccl);
        case DIGIT:
            return matchdigit(c);
        case NOT_DIGIT:
            return !matchdigit(c);
        case ALPHA:
            return matchalphanum(c);
        case NOT_ALPHA:
            return !matchalphanum(c);
        case WHITESPACE:
            return matchwhitespace(c);
        case NOT_WHITESPACE:
            return !matchwhitespace(c);
        default:
            return (p.u.ch == c);
    }
}

static int matchstar(regex_t p, regex_t *pattern, const char *text, int *matchlength) {
    int prelen = *matchlength;
    const char *prepoint = text;
    while ((text[0] != '\0') && matchone(p, *text)) {
        text++;
        (*matchlength)++;
    }
    while (text >= prepoint) {
        if (matchpattern(pattern, text--, matchlength)) return 1;
        (*matchlength)--;
    }

    *matchlength = prelen;
    return 0;
}

static int matchplus(regex_t p, regex_t *pattern, const char *text, int *matchlength) {
    const char *prepoint = text;
    while ((text[0] != '\0') && matchone(p, *text)) {
        text++;
        (*matchlength)++;
    }
    while (text > prepoint) {
        if (matchpattern(pattern, text--, matchlength)) return 1;
        (*matchlength)--;
    }

    return 0;
}

static int matchquestion(regex_t p, regex_t *pattern, const char *text, int *matchlength) {
    if (p.type == UNUSED) return 1;
    if (matchpattern(pattern, text, matchlength)) return 1;
    if (*text && matchone(p, *text++)) {
        if (matchpattern(pattern, text, matchlength)) {
            (*matchlength)++;
            return 1;
        }
    }
    return 0;
}

/* Iterative matching */
static int matchpattern(regex_t *pattern, const char *text, int *matchlength) {
    int pre = *matchlength;
    do {
        if ((pattern[0].type == UNUSED) || (pattern[1].type == QUESTIONMARK)) {
            return matchquestion(pattern[0], &pattern[2], text, matchlength);
        } else if (pattern[1].type == STAR) {
            return matchstar(pattern[0], &pattern[2], text, matchlength);
        } else if (pattern[1].type == PLUS) {
            return matchplus(pattern[0], &pattern[2], text, matchlength);
        } else if ((pattern[0].type == END) && pattern[1].type == UNUSED) {
            return (text[0] == '\0');
        }
        /*  Branching is not working properly
            else if (pattern[1].type == BRANCH)
            {
              return (matchpattern(pattern, text) || matchpattern(&pattern[2], text));
            }
        */
        (*matchlength)++;
    } while ((text[0] != '\0') && matchone(*pattern++, *text++));

    *matchlength = pre;
    return 0;
}

// --------------- Trie data structure ---------------

Trie InitTrie() {
    Trie t;
    t.numberOfElements = 0;
    t.maxKeySize = 0;

    int i;
    for (i = 0; i < TRIE_ALPHABET_SIZE; i++) {
        t.branch[i] = NULL;
    }

    return t;
}

void FreeTrieInternal(Trie *trie) {
    if (!trie) return;
    if (trie->branch['\0']) {
        free(trie->branch['\0']);
        trie->branch['\0'] = NULL;
    }
    for (int i = 0; i < TRIE_ALPHABET_SIZE; i++) {
        FreeTrieInternal(trie->branch[i]);
        trie->branch[i] = NULL;
    }
    free(trie);
}

void FreeTrie(Trie *trie) {
    if (!trie) return;
    for (int i = 0; i < TRIE_ALPHABET_SIZE; i++) {
        FreeTrieInternal(trie->branch[i]);
        trie->branch[i] = NULL;
    }

    trie->numberOfElements = 0;
    trie->maxKeySize = 0;
}

void InsertTrie(Trie *trie, const char *key, const void *value, int size, TrieType valueType) {
    if (!trie) return;
    int i = 0;

    Trie *cell = trie;
    while (cell->branch[(unsigned char)key[i]] && key[i] != '\0') {
        cell = cell->branch[(unsigned char)key[i++]];
    }

    // If this key is not contained in the trie
    if (!cell->branch[(unsigned char)key[i]]) {
        // Insert the branches that are missing
        while (key[i] != '\0') {
            Trie *newCell = calloc(1, sizeof(Trie));
            cell->branch[(unsigned char)key[i++]] = newCell;
            cell = newCell;
        }
        // cell now points to the '\0' cell
    }

    // If this key is already in the trie, replace the data
    else if (key[i] == '\0') {
        free(cell->branch['\0']);
        trie->numberOfElements--;
    }

    cell->elementSize = size;
    cell->elementType = valueType;
    cell->branch['\0'] = malloc(size);
    memcpy(cell->branch['\0'], value, size);

    trie->numberOfElements++;

    int keyLength = strlen(key);
    if (trie->maxKeySize < keyLength) trie->maxKeySize = keyLength;
}

inline void InsertTrieString(Trie *trie, const char *key, const char *value) { InsertTrie(trie, key, value, (strlen(value) + 1) * sizeof(char), Trie_String); }

Trie *TrieGetDataCell(Trie *trie, const char *key) {
    int i = 0;
    Trie *cell = trie;
    while (cell->branch[(unsigned char)key[i]] && key[i] != '\0') {
        cell = cell->branch[(unsigned char)key[i++]];
    }

    return (key[i] == '\0') ? cell : NULL;
}

int TrieContainsKey(Trie trie, const char *key) { return TrieGetDataCell(&trie, key) ? 1 : 0; }

void *GetTrieElement(Trie trie, const char *key) {
    Trie *cell = TrieGetDataCell(&trie, key);
    return cell ? cell->branch['\0'] : NULL;
}

void *GetTrieElementWithProperties(Trie trie, const char *key, int *sizeOut, TrieType *typeOut) {
    Trie *cell = TrieGetDataCell(&trie, key);
    if (sizeOut) *sizeOut = cell ? cell->elementSize : 0;
    if (typeOut) *typeOut = cell ? cell->elementType : Trie_None;

    return cell ? cell->branch['\0'] : NULL;
}

void *GetTrieElementAsPointer(Trie trie, const char *key, void *defaultValue) {
    int size = 0;
    void **data = GetTrieElementWithProperties(trie, key, &size, NULL);
    if (!data || size != sizeof(void *))
        return defaultValue;
    else
        return *data;
}

char *GetTrieElementAsString(Trie trie, const char *key, char *defaultValue) {
    TrieType elementType;
    char *data = GetTrieElementWithProperties(trie, key, NULL, &elementType);
    if (!data || elementType != Trie_String)
        return defaultValue;
    else
        return data;
}

// Macro to generate insertion and retrieval functions for different types
// This was made to avoid copying and pasting the same function many times
// Remember to call the header generation macro on utils.h when adding more types
#define TRIE_TYPE_FUNCTION_TEMPLATE_MACRO(type)                                                                                   \
    void InsertTrie_##type(Trie *trie, const char *key, type value) { InsertTrie(trie, key, &value, sizeof(type), Trie_##type); } \
                                                                                                                                  \
    type GetTrieElementAs_##type(Trie trie, const char *key, type defaultValue) {                                                 \
        TrieType elementType;                                                                                                     \
        type *data = GetTrieElementWithProperties(trie, key, NULL, &elementType);                                                 \
        if (!data || elementType != Trie_##type)                                                                                  \
            return defaultValue;                                                                                                  \
        else                                                                                                                      \
            return *data;                                                                                                         \
    }

TRIE_TYPE_FUNCTION_TEMPLATE_MACRO(metadot_vec3)
TRIE_TYPE_FUNCTION_TEMPLATE_MACRO(double)
TRIE_TYPE_FUNCTION_TEMPLATE_MACRO(float)
TRIE_TYPE_FUNCTION_TEMPLATE_MACRO(char)
TRIE_TYPE_FUNCTION_TEMPLATE_MACRO(int)

int GetTrieElementsArrayInternal(Trie *trie, char *key, int depth, TrieElement *elementsArray, int position) {
    if (!trie) return 0;

    int usedPositions = 0;
    int hasData = trie->branch['\0'] ? 1 : 0;
    if (hasData) {
        key[depth + 1] = '\0';
        char *keystr = malloc((depth + 2) * sizeof(char));
        strncpy(keystr, key, depth + 2);

        TrieElement newElement = {keystr, trie->elementType, trie->elementSize, trie->branch['\0']};
        elementsArray[position] = newElement;
        usedPositions++;
    }

    for (int i = 1; i < TRIE_ALPHABET_SIZE; i++) {
        if (trie->branch[i]) {
            key[depth + 1] = i;
            usedPositions += GetTrieElementsArrayInternal(trie->branch[i], key, depth + 1, elementsArray, position + usedPositions);
        }
    }

    return usedPositions;
}

TrieElement *GetTrieElementsArray(Trie trie, int *outElementsCount) {
    assert(outElementsCount);

    *outElementsCount = trie.numberOfElements;
    TrieElement *array = malloc(trie.numberOfElements * sizeof(TrieElement));

    char *key = malloc((trie.maxKeySize + 1) * sizeof(char));

    int position = 0;
    for (int i = 1; i < TRIE_ALPHABET_SIZE; i++) {
        if (trie.branch[i]) {
            key[0] = i;
            position += GetTrieElementsArrayInternal(trie.branch[i], key, 0, array, position);
        }
    }

    free(key);
    return array;
}

void FreeTrieElementsArray(TrieElement *elementsArray, int elementsCount) {
    assert(elementsArray);
    int i;
    for (i = 0; i < elementsCount; i++) {
        free(elementsArray[i].key);
    }
    free(elementsArray);
}

// --------------- cJSON wrapper functions ---------------

cJSON *OpenJSON(char path[], char name[]) {

    char fullPath[512 + 256];
    strncpy(fullPath, path, 512);
    if (path[strlen(path) - 1] != '/') {
        strcat(fullPath, "/");
    }
    strcat(fullPath, name);
    METADOT_TRACE("Opening JSON: (%s)", fullPath);
    FILE *file = fopen(fullPath, "rb");

    if (file) {
        fseek(file, 0, SEEK_END);
        unsigned size = ftell(file);
        rewind(file);

        char *jsonString = malloc((size + 1) * sizeof(char));
        fread(jsonString, sizeof(char), size, file);
        jsonString[size] = '\0';
        fclose(file);

        cJSON *json = cJSON_Parse(jsonString);
        if (!json) {
            // Error treatment
            const char *error_ptr = cJSON_GetErrorPtr();
            if (error_ptr != NULL) {
                METADOT_ERROR("OpenJSON: JSON error: %s", error_ptr);
            }
            free(jsonString);
            return NULL;

        } else {
            free(jsonString);
            return json;
        }

    } else {
        METADOT_ERROR("OpenJSON: Failed to open json file!");
    }
    return NULL;
}

F64 JSON_GetObjectDouble(cJSON *object, char *string, F64 defaultValue) {
    cJSON *obj = cJSON_GetObjectItem(object, string);
    if (obj)
        return obj->valuedouble;
    else
        return defaultValue;
}

metadot_vec3 JSON_GetObjectVector3(cJSON *object, char *string, metadot_vec3 defaultValue) {

    cJSON *arr = cJSON_GetObjectItem(object, string);
    if (!arr) return defaultValue;

    metadot_vec3 v = VECTOR3_ZERO;

    cJSON *item = cJSON_GetArrayItem(arr, 0);
    if (item) v.X = item->valuedouble;

    item = cJSON_GetArrayItem(arr, 1);
    if (item) v.Y = item->valuedouble;

    item = cJSON_GetArrayItem(arr, 2);
    if (item) v.Z = item->valuedouble;

    return v;
}

cJSON *JSON_CreateVector3(metadot_vec3 value) {

    cJSON *v = cJSON_CreateArray();
    cJSON_AddItemToArray(v, cJSON_CreateNumber(value.X));
    cJSON_AddItemToArray(v, cJSON_CreateNumber(value.Y));
    cJSON_AddItemToArray(v, cJSON_CreateNumber(value.Z));

    return v;
}

// --------------- Lua stack manipulation functions ---------------

// Creates an table with the xyz entries and populate with the vector values
void Vector3ToTable(lua_State *L, metadot_vec3 vector) {

    lua_newtable(L);
    lua_pushliteral(L, "x");      // x index
    lua_pushnumber(L, vector.X);  // x value
    lua_rawset(L, -3);            // Store x in table

    lua_pushliteral(L, "y");      // y index
    lua_pushnumber(L, vector.Y);  // y value
    lua_rawset(L, -3);            // Store y in table

    lua_pushliteral(L, "z");      // z index
    lua_pushnumber(L, vector.Z);  // z value
    lua_rawset(L, -3);            // Store z in table
}

// --------------- Misc. functions ---------------

// Compare if two zero terminated strings are exactly equal
int StringCompareEqual(char *stringA, char *stringB) {
    int i, isEqual = 1;

    // Check characters until the string A ends
    for (i = 0; stringA[i] != '\0'; i++) {
        if (stringB[i] != stringA[i]) {
            isEqual = 0;
            break;
        }
    }

    // Check if B ends in the same point as A, if not B contains A, but is longer than A
    return isEqual ? (stringB[i] == '\0' ? 1 : 0) : 0;
}

// Same as above, but case insensitive
int StringCompareEqualCaseInsensitive(char *stringA, char *stringB) {
    int i, isEqual = 1;

    // Check characters until the string A ends
    for (i = 0; stringA[i] != '\0'; i++) {
        if (tolower(stringB[i]) != tolower(stringA[i])) {
            isEqual = 0;
            break;
        }
    }

    // Check if B ends in the same point as A, if not B contains A, but is longer than A
    return isEqual ? (stringB[i] == '\0' ? 1 : 0) : 0;
}

typedef struct {
    U8 *buffer;
    size_t buffer_offset;
} nbt__read_stream_t;

static U8 nbt__get_byte(nbt__read_stream_t *stream) { return stream->buffer[stream->buffer_offset++]; }

static I16 nbt__get_int16(nbt__read_stream_t *stream) {
    U8 bytes[2];
    for (int i = 1; i >= 0; i--) {
        bytes[i] = nbt__get_byte(stream);
    }
    return *(I16 *)(bytes);
}

static I32 nbt__get_int32(nbt__read_stream_t *stream) {
    U8 bytes[4];
    for (int i = 3; i >= 0; i--) {
        bytes[i] = nbt__get_byte(stream);
    }
    return *(I32 *)(bytes);
}

static I64 nbt__get_int64(nbt__read_stream_t *stream) {
    U8 bytes[8];
    for (int i = 7; i >= 0; i--) {
        bytes[i] = nbt__get_byte(stream);
    }
    return *(I64 *)(bytes);
}

static F32 nbt__get_float(nbt__read_stream_t *stream) {
    U8 bytes[4];
    for (int i = 3; i >= 0; i--) {
        bytes[i] = nbt__get_byte(stream);
    }
    return *(F32 *)(bytes);
}

static F64 nbt__get_double(nbt__read_stream_t *stream) {
    U8 bytes[8];
    for (int i = 7; i >= 0; i--) {
        bytes[i] = nbt__get_byte(stream);
    }
    return *(F64 *)(bytes);
}

static nbt_tag_t *nbt__parse(nbt__read_stream_t *stream, int parse_name, nbt_tag_type_t override_type) {

    nbt_tag_t *tag = (nbt_tag_t *)gc_malloc(&gc, sizeof(nbt_tag_t));

    if (override_type == NBT_NO_OVERRIDE) {
        tag->type = nbt__get_byte(stream);
    } else {
        tag->type = override_type;
    }

    if (parse_name && tag->type != NBT_TYPE_END) {
        tag->name_size = nbt__get_int16(stream);
        tag->name = (char *)gc_malloc(&gc, tag->name_size + 1);
        for (size_t i = 0; i < tag->name_size; i++) {
            tag->name[i] = nbt__get_byte(stream);
        }
        tag->name[tag->name_size] = '\0';
    } else {
        tag->name = NULL;
        tag->name_size = 0;
    }

    switch (tag->type) {
        case NBT_TYPE_END: {
            // Don't do anything.
            break;
        }
        case NBT_TYPE_BYTE: {
            tag->tag_byte.value = nbt__get_byte(stream);
            break;
        }
        case NBT_TYPE_SHORT: {
            tag->tag_short.value = nbt__get_int16(stream);
            break;
        }
        case NBT_TYPE_INT: {
            tag->tag_int.value = nbt__get_int32(stream);
            break;
        }
        case NBT_TYPE_LONG: {
            tag->tag_long.value = nbt__get_int64(stream);
            break;
        }
        case NBT_TYPE_FLOAT: {
            tag->tag_float.value = nbt__get_float(stream);
            break;
        }
        case NBT_TYPE_DOUBLE: {
            tag->tag_double.value = nbt__get_double(stream);
            break;
        }
        case NBT_TYPE_BYTE_ARRAY: {
            tag->tag_byte_array.size = nbt__get_int32(stream);
            tag->tag_byte_array.value = (int8_t *)gc_malloc(&gc, tag->tag_byte_array.size);
            for (size_t i = 0; i < tag->tag_byte_array.size; i++) {
                tag->tag_byte_array.value[i] = nbt__get_byte(stream);
            }
            break;
        }
        case NBT_TYPE_STRING: {
            tag->tag_string.size = nbt__get_int16(stream);
            tag->tag_string.value = (char *)gc_malloc(&gc, tag->tag_string.size + 1);
            for (size_t i = 0; i < tag->tag_string.size; i++) {
                tag->tag_string.value[i] = nbt__get_byte(stream);
            }
            tag->tag_string.value[tag->tag_string.size] = '\0';
            break;
        }
        case NBT_TYPE_LIST: {
            tag->tag_list.type = nbt__get_byte(stream);
            tag->tag_list.size = nbt__get_int32(stream);
            tag->tag_list.value = (nbt_tag_t **)gc_malloc(&gc, tag->tag_list.size * sizeof(nbt_tag_t *));
            for (size_t i = 0; i < tag->tag_list.size; i++) {
                tag->tag_list.value[i] = nbt__parse(stream, 0, tag->tag_list.type);
            }
            break;
        }
        case NBT_TYPE_COMPOUND: {
            tag->tag_compound.size = 0;
            tag->tag_compound.value = NULL;
            for (;;) {
                nbt_tag_t *inner_tag = nbt__parse(stream, 1, NBT_NO_OVERRIDE);

                if (inner_tag->type == NBT_TYPE_END) {
                    nbt__free_tag(inner_tag);
                    break;
                } else {
                    tag->tag_compound.value = (nbt_tag_t **)gc_realloc(&gc, tag->tag_compound.value, (tag->tag_compound.size + 1) * sizeof(nbt_tag_t *));
                    tag->tag_compound.value[tag->tag_compound.size] = inner_tag;
                    tag->tag_compound.size++;
                }
            }
            break;
        }
        case NBT_TYPE_INT_ARRAY: {
            tag->tag_int_array.size = nbt__get_int32(stream);
            tag->tag_int_array.value = (I32 *)gc_malloc(&gc, tag->tag_int_array.size * sizeof(I32));
            for (size_t i = 0; i < tag->tag_int_array.size; i++) {
                tag->tag_int_array.value[i] = nbt__get_int32(stream);
            }
            break;
        }
        case NBT_TYPE_LONG_ARRAY: {
            tag->tag_long_array.size = nbt__get_int32(stream);
            tag->tag_long_array.value = (I64 *)gc_malloc(&gc, tag->tag_long_array.size * sizeof(I64));
            for (size_t i = 0; i < tag->tag_long_array.size; i++) {
                tag->tag_long_array.value[i] = nbt__get_int64(stream);
            }
            break;
        }
        default: {
            gc_free(&gc, tag);
            return NULL;
        }
    }

    return tag;
}

nbt_tag_t *nbt_parse(nbt_reader_t reader, int parse_flags) {

    int compressed;
    int gzip_format;
    switch (parse_flags & 3) {
        case 0: {  // Automatic detection (not yet implemented).
            compressed = 1;
            gzip_format = 1;
            break;
        }
        case 1: {  // gzip
            compressed = 1;
            gzip_format = 1;
            break;
        }
        case 2: {  // zlib
            compressed = 1;
            gzip_format = 0;
            break;
        }
        case 3: {  // raw
            compressed = 0;
            gzip_format = 0;
            break;
        }
    }

    U8 *buffer = NULL;
    size_t buffer_size = 0;

    nbt__read_stream_t stream;

    if (compressed) {
        z_stream stream;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;
        stream.avail_in = 0;
        stream.next_in = Z_NULL;

        if (gzip_format) {
            U8 header[10];
            reader.read(reader.userdata, header, 10);
            int fhcrc = header[3] & 2;
            int fextra = header[3] & 4;
            int fname = header[3] & 8;
            int fcomment = header[3] & 16;

            (void)fextra;  // I don't think many files use this.

            if (fname) {
                U8 byte = 0;
                do {
                    reader.read(reader.userdata, &byte, 1);
                } while (byte != 0);
            }

            if (fcomment) {
                U8 byte = 0;
                do {
                    reader.read(reader.userdata, &byte, 1);
                } while (byte != 0);
            }

            U16 crc;
            if (fhcrc) {
                reader.read(reader.userdata, (U8 *)&crc, 2);
            }

            (void)crc;
        }

        int ret = inflateInit2(&stream, gzip_format ? -Z_DEFAULT_WINDOW_BITS : Z_DEFAULT_WINDOW_BITS);
        if (ret != Z_OK) {
            gc_free(&gc, buffer);
            return NULL;
        }

        U8 in_buffer[NBT_BUFFER_SIZE];
        U8 out_buffer[NBT_BUFFER_SIZE];
        do {
            stream.avail_in = reader.read(reader.userdata, in_buffer, NBT_BUFFER_SIZE);
            stream.next_in = in_buffer;

            do {
                stream.avail_out = NBT_BUFFER_SIZE;
                stream.next_out = out_buffer;

                ret = inflate(&stream, Z_NO_FLUSH);

                size_t have = NBT_BUFFER_SIZE - stream.avail_out;
                buffer = (U8 *)gc_realloc(&gc, buffer, buffer_size + have);
                memcpy(buffer + buffer_size, out_buffer, have);
                buffer_size += have;

            } while (stream.avail_out == 0);

        } while (ret != Z_STREAM_END);

        inflateEnd(&stream);

    } else {

        U8 in_buffer[NBT_BUFFER_SIZE];
        size_t bytes_read;
        do {
            bytes_read = reader.read(reader.userdata, in_buffer, NBT_BUFFER_SIZE);
            buffer = (U8 *)gc_realloc(&gc, buffer, buffer_size + bytes_read);
            memcpy(buffer + buffer_size, in_buffer, bytes_read);
            buffer_size += bytes_read;
        } while (bytes_read == NBT_BUFFER_SIZE);
    }

    stream.buffer = buffer;
    stream.buffer_offset = 0;

    nbt_tag_t *tag = nbt__parse(&stream, 1, NBT_NO_OVERRIDE);

    gc_free(&gc, buffer);

    return tag;
}

typedef struct {
    U8 *buffer;
    size_t offset;
    size_t size;
    size_t alloc_size;
} nbt__write_stream_t;

void nbt__put_byte(nbt__write_stream_t *stream, U8 value) {
    if (stream->offset >= stream->alloc_size - 1) {
        stream->buffer = (U8 *)gc_realloc(&gc, stream->buffer, stream->alloc_size * 2);
        stream->alloc_size *= 2;
    }

    stream->buffer[stream->offset++] = value;
    stream->size++;
}

void nbt__put_int16(nbt__write_stream_t *stream, I16 value) {
    U8 *value_array = (U8 *)&value;
    for (int i = 1; i >= 0; i--) {
        nbt__put_byte(stream, value_array[i]);
    }
}

void nbt__put_int32(nbt__write_stream_t *stream, I32 value) {
    U8 *value_array = (U8 *)&value;
    for (int i = 3; i >= 0; i--) {
        nbt__put_byte(stream, value_array[i]);
    }
}

void nbt__put_int64(nbt__write_stream_t *stream, I64 value) {
    U8 *value_array = (U8 *)&value;
    for (int i = 7; i >= 0; i--) {
        nbt__put_byte(stream, value_array[i]);
    }
}

void nbt__put_float(nbt__write_stream_t *stream, F32 value) {
    U8 *value_array = (U8 *)&value;
    for (int i = 3; i >= 0; i--) {
        nbt__put_byte(stream, value_array[i]);
    }
}

void nbt__put_double(nbt__write_stream_t *stream, F64 value) {
    U8 *value_array = (U8 *)&value;
    for (int i = 7; i >= 0; i--) {
        nbt__put_byte(stream, value_array[i]);
    }
}

void nbt__write_tag(nbt__write_stream_t *stream, nbt_tag_t *tag, int write_name, int write_type) {

    if (write_type) {
        nbt__put_byte(stream, tag->type);
    }

    if (write_name && tag->type != NBT_TYPE_END) {
        nbt__put_int16(stream, tag->name_size);
        for (size_t i = 0; i < tag->name_size; i++) {
            nbt__put_byte(stream, tag->name[i]);
        }
    }

    switch (tag->type) {
        case NBT_TYPE_END: {
            // Do nothing.
            break;
        }
        case NBT_TYPE_BYTE: {
            nbt__put_byte(stream, tag->tag_byte.value);
            break;
        }
        case NBT_TYPE_SHORT: {
            nbt__put_int16(stream, tag->tag_short.value);
            break;
        }
        case NBT_TYPE_INT: {
            nbt__put_int32(stream, tag->tag_int.value);
            break;
        }
        case NBT_TYPE_LONG: {
            nbt__put_int64(stream, tag->tag_long.value);
            break;
        }
        case NBT_TYPE_FLOAT: {
            nbt__put_float(stream, tag->tag_float.value);
            break;
        }
        case NBT_TYPE_DOUBLE: {
            nbt__put_double(stream, tag->tag_double.value);
            break;
        }
        case NBT_TYPE_BYTE_ARRAY: {
            nbt__put_int32(stream, tag->tag_byte_array.size);
            for (size_t i = 0; i < tag->tag_byte_array.size; i++) {
                nbt__put_byte(stream, tag->tag_byte_array.value[i]);
            }
            break;
        }
        case NBT_TYPE_STRING: {
            nbt__put_int16(stream, tag->tag_string.size);
            for (size_t i = 0; i < tag->tag_string.size; i++) {
                nbt__put_byte(stream, tag->tag_string.value[i]);
            }
            break;
        }
        case NBT_TYPE_LIST: {
            nbt__put_byte(stream, tag->tag_list.type);
            nbt__put_int32(stream, tag->tag_list.size);
            for (size_t i = 0; i < tag->tag_list.size; i++) {
                nbt__write_tag(stream, tag->tag_list.value[i], 0, 0);
            }
            break;
        }
        case NBT_TYPE_COMPOUND: {
            for (size_t i = 0; i < tag->tag_compound.size; i++) {
                nbt__write_tag(stream, tag->tag_compound.value[i], 1, 1);
            }
            nbt__put_byte(stream, 0);  // End tag.
            break;
        }
        case NBT_TYPE_INT_ARRAY: {
            nbt__put_int32(stream, tag->tag_int_array.size);
            for (size_t i = 0; i < tag->tag_int_array.size; i++) {
                nbt__put_int32(stream, tag->tag_int_array.value[i]);
            }
            break;
        }
        case NBT_TYPE_LONG_ARRAY: {
            nbt__put_int32(stream, tag->tag_long_array.size);
            for (size_t i = 0; i < tag->tag_long_array.size; i++) {
                nbt__put_int64(stream, tag->tag_long_array.value[i]);
            }
            break;
        }
        default: {
            break;
        }
    }
}

U32 nbt__crc_table[256];

int nbt__crc_table_computed = 0;

void nbt__make_crc_table(void) {
    unsigned long c;
    int n, k;

    for (n = 0; n < 256; n++) {
        c = (U32)n;
        for (k = 0; k < 8; k++) {
            if (c & 1) {
                c = 0xedb88320L ^ (c >> 1);
            } else {
                c = c >> 1;
            }
        }
        nbt__crc_table[n] = c;
    }
    nbt__crc_table_computed = 1;
}

static U32 nbt__update_crc(U32 crc, U8 *buf, size_t len) {
    U32 c = crc ^ 0xffffffffL;
    size_t n;

    if (!nbt__crc_table_computed) {
        nbt__make_crc_table();
    }

    for (n = 0; n < len; n++) {
        c = nbt__crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
    }
    return c ^ 0xffffffffL;
}

void nbt_write(nbt_writer_t writer, nbt_tag_t *tag, int write_flags) {

    int compressed;
    int gzip_format;

    switch (write_flags & 3) {
        case 1: {  // gzip
            compressed = 1;
            gzip_format = 1;
            break;
        }
        case 2: {  // zlib
            compressed = 1;
            gzip_format = 0;
            break;
        }
        case 3: {  // raw
            compressed = 0;
            gzip_format = 0;
            break;
        }
    }

    nbt__write_stream_t write_stream;
    write_stream.buffer = (U8 *)gc_malloc(&gc, NBT_BUFFER_SIZE);
    write_stream.offset = 0;
    write_stream.size = 0;
    write_stream.alloc_size = NBT_BUFFER_SIZE;

    nbt__write_tag(&write_stream, tag, 1, 1);

    if (compressed) {

        z_stream stream;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;

        int window_bits = gzip_format ? -Z_DEFAULT_WINDOW_BITS : Z_DEFAULT_WINDOW_BITS;

        if (deflateInit2(&stream, NBT_COMPRESSION_LEVEL, Z_DEFLATED, window_bits, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
            gc_free(&gc, write_stream.buffer);
            return;
        }

        if (gzip_format) {
            U8 header[10] = {31, 139, 8, 0, 0, 0, 0, 0, 2, 255};
            writer.write(writer.userdata, header, 10);
        }

        U8 in_buffer[NBT_BUFFER_SIZE];
        U8 out_buffer[NBT_BUFFER_SIZE];
        int flush;
        U32 crc = 0;

        write_stream.offset = 0;

        do {

            flush = Z_NO_FLUSH;
            size_t bytes_read = 0;
            for (size_t i = 0; i < NBT_BUFFER_SIZE; i++) {

                in_buffer[i] = write_stream.buffer[write_stream.offset++];

                bytes_read++;

                if (write_stream.offset >= write_stream.size) {
                    flush = Z_FINISH;
                    break;
                }
            }

            stream.avail_in = bytes_read;
            stream.next_in = in_buffer;

            do {
                stream.avail_out = NBT_BUFFER_SIZE;
                stream.next_out = out_buffer;

                deflate(&stream, flush);

                size_t have = NBT_BUFFER_SIZE - stream.avail_out;
                writer.write(writer.userdata, out_buffer, have);

                crc = nbt__update_crc(crc, out_buffer, have);

            } while (stream.avail_out == 0);

        } while (flush != Z_FINISH);

        deflateEnd(&stream);

        if (gzip_format) {
            writer.write(writer.userdata, (U8 *)&crc, 4);
            writer.write(writer.userdata, (U8 *)&write_stream.size, 4);
        }

    } else {
        size_t bytes_left = write_stream.size;
        size_t offset = 0;
        while (bytes_left > 0) {
            size_t bytes_written = writer.write(writer.userdata, write_stream.buffer + offset, bytes_left);
            offset += bytes_written;
            bytes_left -= bytes_written;
        }
    }

    gc_free(&gc, write_stream.buffer);
}

static nbt_tag_t *nbt__new_tag_base(void) {
    nbt_tag_t *tag = (nbt_tag_t *)gc_malloc(&gc, sizeof(nbt_tag_t));
    tag->name = NULL;
    tag->name_size = 0;

    return tag;
}

nbt_tag_t *nbt_new_tag_byte(int8_t value) {
    nbt_tag_t *tag = nbt__new_tag_base();

    tag->type = NBT_TYPE_BYTE;
    tag->tag_byte.value = value;

    return tag;
}

nbt_tag_t *nbt_new_tag_short(I16 value) {
    nbt_tag_t *tag = nbt__new_tag_base();

    tag->type = NBT_TYPE_SHORT;
    tag->tag_short.value = value;

    return tag;
}

nbt_tag_t *nbt_new_tag_int(I32 value) {
    nbt_tag_t *tag = nbt__new_tag_base();

    tag->type = NBT_TYPE_INT;
    tag->tag_int.value = value;

    return tag;
}

nbt_tag_t *nbt_new_tag_long(I64 value) {
    nbt_tag_t *tag = nbt__new_tag_base();

    tag->type = NBT_TYPE_LONG;
    tag->tag_long.value = value;

    return tag;
}

nbt_tag_t *nbt_new_tag_float(F32 value) {
    nbt_tag_t *tag = nbt__new_tag_base();

    tag->type = NBT_TYPE_FLOAT;
    tag->tag_float.value = value;

    return tag;
}

nbt_tag_t *nbt_new_tag_double(F64 value) {
    nbt_tag_t *tag = nbt__new_tag_base();

    tag->type = NBT_TYPE_DOUBLE;
    tag->tag_double.value = value;

    return tag;
}

nbt_tag_t *nbt_new_tag_byte_array(int8_t *value, size_t size) {
    nbt_tag_t *tag = nbt__new_tag_base();

    tag->type = NBT_TYPE_BYTE_ARRAY;
    tag->tag_byte_array.size = size;
    tag->tag_byte_array.value = (int8_t *)gc_malloc(&gc, size);

    memcpy(tag->tag_byte_array.value, value, size);

    return tag;
}

nbt_tag_t *nbt_new_tag_string(const char *value, size_t size) {
    nbt_tag_t *tag = nbt__new_tag_base();

    tag->type = NBT_TYPE_STRING;
    tag->tag_string.size = size;
    tag->tag_string.value = (char *)gc_malloc(&gc, size + 1);

    memcpy(tag->tag_string.value, value, size);
    tag->tag_string.value[tag->tag_string.size] = '\0';

    return tag;
}

nbt_tag_t *nbt_new_tag_list(nbt_tag_type_t type) {
    nbt_tag_t *tag = nbt__new_tag_base();

    tag->type = NBT_TYPE_LIST;
    tag->tag_list.type = type;
    tag->tag_list.size = 0;
    tag->tag_list.value = NULL;

    return tag;
}

nbt_tag_t *nbt_new_tag_compound(void) {
    nbt_tag_t *tag = nbt__new_tag_base();

    tag->type = NBT_TYPE_COMPOUND;
    tag->tag_compound.size = 0;
    tag->tag_compound.value = NULL;

    return tag;
}

nbt_tag_t *nbt_new_tag_int_array(I32 *value, size_t size) {
    nbt_tag_t *tag = nbt__new_tag_base();

    tag->type = NBT_TYPE_INT_ARRAY;
    tag->tag_int_array.size = size;
    tag->tag_int_array.value = (I32 *)gc_malloc(&gc, size * sizeof(I32));

    memcpy(tag->tag_int_array.value, value, size * sizeof(I32));

    return tag;
}

nbt_tag_t *nbt_new_tag_long_array(I64 *value, size_t size) {
    nbt_tag_t *tag = nbt__new_tag_base();

    tag->type = NBT_TYPE_LONG_ARRAY;
    tag->tag_long_array.size = size;
    tag->tag_long_array.value = (I64 *)gc_malloc(&gc, size * sizeof(I64));

    memcpy(tag->tag_long_array.value, value, size * sizeof(I64));

    return tag;
}

void nbt_set_tag_name(nbt_tag_t *tag, const char *name, size_t size) {
    if (tag->name) {
        gc_free(&gc, tag->name);
    }
    tag->name_size = size;
    tag->name = (char *)gc_malloc(&gc, size + 1);
    memcpy(tag->name, name, size);
    tag->name[tag->name_size] = '\0';
}

void nbt_tag_list_append(nbt_tag_t *list, nbt_tag_t *value) {
    list->tag_list.value = gc_realloc(&gc, list->tag_list.value, (list->tag_list.size + 1) * sizeof(nbt_tag_t *));
    list->tag_list.value[list->tag_list.size] = value;
    list->tag_list.size++;
}

nbt_tag_t *nbt_tag_list_get(nbt_tag_t *tag, size_t index) { return tag->tag_list.value[index]; }

void nbt_tag_compound_append(nbt_tag_t *compound, nbt_tag_t *value) {
    compound->tag_compound.value = gc_realloc(&gc, compound->tag_compound.value, (compound->tag_compound.size + 1) * sizeof(nbt_tag_t *));
    compound->tag_compound.value[compound->tag_compound.size] = value;
    compound->tag_compound.size++;
}

nbt_tag_t *nbt_tag_compound_get(nbt_tag_t *tag, const char *key) {
    for (size_t i = 0; i < tag->tag_compound.size; i++) {
        nbt_tag_t *compare_tag = tag->tag_compound.value[i];

        if (memcmp(compare_tag->name, key, compare_tag->name_size) == 0) {
            return compare_tag;
        }
    }

    return NULL;
}

void nbt__free_tag(nbt_tag_t *tag) {
    switch (tag->type) {
        case NBT_TYPE_BYTE_ARRAY: {
            gc_free(&gc, tag->tag_byte_array.value);
            break;
        }
        case NBT_TYPE_STRING: {
            gc_free(&gc, tag->tag_string.value);
            break;
        }
        case NBT_TYPE_LIST: {
            for (size_t i = 0; i < tag->tag_list.size; i++) {
                nbt__free_tag(tag->tag_list.value[i]);
            }
            gc_free(&gc, tag->tag_list.value);
            break;
        }
        case NBT_TYPE_COMPOUND: {
            for (size_t i = 0; i < tag->tag_compound.size; i++) {
                nbt__free_tag(tag->tag_compound.value[i]);
            }
            gc_free(&gc, tag->tag_compound.value);
            break;
        }
        case NBT_TYPE_INT_ARRAY: {
            gc_free(&gc, tag->tag_int_array.value);
            break;
        }
        case NBT_TYPE_LONG_ARRAY: {
            gc_free(&gc, tag->tag_long_array.value);
            break;
        }
        default: {
            break;
        }
    }

    if (tag->name) {
        gc_free(&gc, tag->name);
    }

    gc_free(&gc, tag);
}
