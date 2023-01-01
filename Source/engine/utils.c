// Copyright(c) 2022, KaoruXun All rights reserved.

#include "utils.h"

#include <ctype.h>
#include <stdio.h>

#include "core/core.h"

// --------------- FPS counter functions ---------------

#define STORED_FRAMES 10
U32 frameTimes[STORED_FRAMES];
U32 frameTicksLast;
U32 frameCount;
float framesPerSecond;

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

float GetFPS() { return framesPerSecond; }

void InitFPS() {
    // Initialize FPS at 0
    memset(frameTimes, 0, sizeof(frameTimes));
    frameCount = 0;
    framesPerSecond = 0;
    frameTicksLast = SDL_GetTicks();
}

void ProcessFPS() {
    U32 frameTimesIndex;
    U32 currentTicks;
    U32 count;
    U32 i;

    frameTimesIndex = frameCount % STORED_FRAMES;

    currentTicks = SDL_GetTicks();
    // save the frame time value
    frameTimes[frameTimesIndex] = currentTicks - frameTicksLast;

    // save the last frame time for the next fpsthink
    frameTicksLast = currentTicks;

    // increment the frame count
    frameCount++;

    // Work out the current framerate
    // I've included a test to see if the whole array has been written to or not. This will stop
    // strange values on the first few (STORED_FRAMES) frames.
    if (frameCount < STORED_FRAMES) {
        count = frameCount;
    } else {
        count = STORED_FRAMES;
    }

    // add up all the values and divide to get the average frame time.
    framesPerSecond = 0;
    for (i = 0; i < count; i++) {
        framesPerSecond += frameTimes[i];
    }

    framesPerSecond /= count;

    // now to make it an actual frames per second value...
    framesPerSecond = 1000.f / framesPerSecond;
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

TRIE_TYPE_FUNCTION_TEMPLATE_MACRO(Vector3)
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

// --------------- Generic List Functions ---------------

List InitList(unsigned size) {
    List l;
    l.first = NULL;
    l.last = NULL;
    l.elementSize = size;
    l.length = 0;

    return l;
}

void FreeList(List *list) {
    while (list->first) {
        RemoveListStart(list);
    }
}

int IsListEmpty(List list) { return list.first == NULL ? 1 : 0; }

void InsertListEnd(List *list, void *e) {
    ListCellPointer newCell = malloc(sizeof(ListCell));
    newCell->previous = list->last;
    newCell->next = NULL;

    newCell->element = malloc(list->elementSize);
    memcpy(newCell->element, e, list->elementSize);

    if (list->last) {
        list->last->next = newCell;
        list->last = newCell;
    } else {
        list->first = newCell;
        list->last = newCell;
    }

    list->length += 1;
}

void InsertListStart(List *list, void *e) {
    ListCellPointer newCell = malloc(sizeof(ListCell));
    newCell->previous = NULL;
    newCell->next = list->first;

    newCell->element = malloc(list->elementSize);
    memcpy(newCell->element, e, list->elementSize);

    if (list->first) {
        list->first->previous = newCell;
        list->first = newCell;
    } else {
        list->first = newCell;
        list->last = newCell;
    }

    list->length += 1;
}

void InsertListIndex(List *list, void *e, int index) {
    int i;

    if (index < 0)  // Support acessing from the end of the list
        index += list->length;

    // Get the element that will go after the element to be inserted
    ListCellPointer current = list->first;
    for (i = 0; i < index; i++) {
        current = GetNextCell(current);
    }

    // If the index is already ocupied
    if (current != NULL) {
        ListCellPointer newCell = malloc(sizeof(ListCell));
        newCell->element = malloc(list->elementSize);
        memcpy(newCell->element, e, list->elementSize);
        newCell->next = current;

        // Connect the cells to their new parents
        newCell->previous = current->previous;
        current->previous = newCell;

        // If the index is 0 (first), set newCell as first
        if (list->first == current) {
            list->first = newCell;
        }

        // If the index is list length (last), set newCell as last
        if (list->last == current) {
            list->last = newCell;
        }

        // If the previous is not null, point his next to newCell
        if (newCell->previous) {
            newCell->previous->next = newCell;
        }

        list->length += 1;

    } else {
        // Index is list length or off bounds (consider as insertion in the end)
        InsertListEnd(list, e);
    }
}

void RemoveListCell(List *list, ListCellPointer cell) {
    if (!cell->previous)
        return RemoveListStart(list);
    else if (!cell->next)
        return RemoveListEnd(list);

    cell->next->previous = cell->previous;
    cell->previous->next = cell->next;

    free(cell->element);
    free(cell);

    list->length -= 1;
}

void RemoveListEnd(List *list) {
    if (list->last->previous) {
        ListCellPointer aux = list->last->previous;
        free(list->last->element);
        free(list->last);

        aux->next = NULL;
        list->last = aux;
    } else {
        free(list->last->element);
        free(list->last);

        list->last = NULL;
        list->first = NULL;
    }

    list->length -= 1;
}

void RemoveListStart(List *list) {
    if (IsListEmpty(*list)) return;

    if (list->first->next) {
        ListCellPointer aux = list->first->next;
        free(list->first->element);
        free(list->first);

        aux->previous = NULL;
        list->first = aux;
    } else {
        free(list->first->element);
        free(list->first);

        list->first = NULL;
        list->last = NULL;
    }

    list->length -= 1;
}

void RemoveListIndex(List *list, int index) {
    int i;

    if (index < 0)  // Support acessing from the end of the list
        index += list->length;

    if (index == 0)
        return RemoveListStart(list);
    else if (index == list->length - 1)
        return RemoveListEnd(list);

    ListCellPointer current = list->first;
    for (i = 0; i < index; i++) {
        current = GetNextCell(current);
    }

    current->next->previous = current->previous;
    current->previous->next = current->next;

    free(current->element);
    free(current);

    list->length -= 1;
}

void *GetElement(ListCell c) { return c.element; }

void *GetLastElement(List list) {
    if (!list.last) return NULL;
    return list.last->element;
}

void *GetFirstElement(List list) {
    if (!list.first) return NULL;
    return list.first->element;
}

void *GetElementAt(List list, int index) {
    int i;

    if (index < 0)  // Support acessing from the end of the list
        index += list.length;

    ListCellPointer current = list.first;
    for (i = 0; i < index; i++) {
        current = GetNextCell(current);
    }

    return current->element;
}

ListCellPointer GetNextCell(ListCellPointer c) {
    if (!c) return NULL;
    return c->next;
}

ListCellPointer GetPreviousCell(ListCellPointer c) {
    if (!c) return NULL;
    return c->previous;
}

ListCellPointer GetFirstCell(List list) { return list.first; }

ListCellPointer GetLastCell(List list) { return list.last; }

ListCellPointer GetCellAt(List list, int index) {
    int i;

    ListCellPointer current = list.first;
    for (i = 0; i < index; i++) {
        current = GetNextCell(current);
    }

    return current;
}

unsigned GetElementSize(List list) { return list.elementSize; }

unsigned GetLength(List list) { return list.length; }

// --------------- Vector Functions ---------------

Vector3 NormalizeVector(Vector3 v) {
    float l = sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
    if (l == 0) return VECTOR3_ZERO;

    v.x *= 1 / l;
    v.y *= 1 / l;
    v.z *= 1 / l;
    return v;
}

Vector3 Add(Vector3 a, Vector3 b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

Vector3 Subtract(Vector3 a, Vector3 b) {
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return a;
}

Vector3 ScalarMult(Vector3 v, float s) { return (Vector3){v.x * s, v.y * s, v.z * s}; }

double Distance(Vector3 a, Vector3 b) {
    Vector3 AMinusB = Subtract(a, b);
    return sqrt(UTIL_dot(AMinusB, AMinusB));
}

Vector3 VectorProjection(Vector3 a, Vector3 b) {
    // https://en.wikipedia.org/wiki/Vector_projection
    Vector3 normalizedB = NormalizeVector(b);
    double a1 = UTIL_dot(a, normalizedB);
    return ScalarMult(normalizedB, a1);
}

Vector3 Reflection(Vector3 *v1, Vector3 *v2) {
    float dotpr = UTIL_dot(*v2, *v1);
    Vector3 result;
    result.x = v2->x * 2 * dotpr;
    result.y = v2->y * 2 * dotpr;
    result.z = v2->z * 2 * dotpr;

    result.x = v1->x - result.x;
    result.y = v1->y - result.y;
    result.z = v1->z - result.z;

    return result;
}

Vector3 RotatePoint(Vector3 p, Vector3 r, Vector3 pivot) { return Add(RotateVector(Subtract(p, pivot), EulerAnglesToMatrix3x3(r)), pivot); }

double DistanceFromPointToLine2D(Vector3 lP1, Vector3 lP2, Vector3 p) {
    // https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
    return fabsf((lP2.y - lP1.y) * p.x - (lP2.x - lP1.x) * p.y + lP2.x * lP1.y - lP2.y * lP1.x) / Distance(lP1, lP2);
}

// --------------- Matrix3x3 type ---------------

inline Matrix3x3 Transpose(Matrix3x3 m) {
    Matrix3x3 t;

    t.m[0][1] = m.m[1][0];
    t.m[1][0] = m.m[0][1];

    t.m[0][2] = m.m[2][0];
    t.m[2][0] = m.m[0][2];

    t.m[1][2] = m.m[2][1];
    t.m[2][1] = m.m[1][2];

    t.m[0][0] = m.m[0][0];
    t.m[1][1] = m.m[1][1];
    t.m[2][2] = m.m[2][2];

    return t;
}

Matrix3x3 Identity() {
    Matrix3x3 m = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    return m;
}

// Based on the article: Extracting Euler Angles from a Rotation Matrix - Mike Day, Insomniac Games
Vector3 Matrix3x3ToEulerAngles(Matrix3x3 m) {
    Vector3 rotation = VECTOR3_ZERO;
    rotation.x = atan2(m.m[1][2], m.m[2][2]);

    float c2 = sqrt(m.m[0][0] * m.m[0][0] + m.m[0][1] * m.m[0][1]);
    rotation.y = atan2(-m.m[0][2], c2);

    float s1 = sin(rotation.x);
    float c1 = cos(rotation.x);
    rotation.z = atan2(s1 * m.m[2][0] - c1 * m.m[1][0], c1 * m.m[1][1] - s1 * m.m[2][1]);

    return ScalarMult(rotation, 180.0 / PI);
}

Matrix3x3 EulerAnglesToMatrix3x3(Vector3 rotation) {

    float s1 = sin(rotation.x * PI / 180.0);
    float c1 = cos(rotation.x * PI / 180.0);
    float s2 = sin(rotation.y * PI / 180.0);
    float c2 = cos(rotation.y * PI / 180.0);
    float s3 = sin(rotation.z * PI / 180.0);
    float c3 = cos(rotation.z * PI / 180.0);

    Matrix3x3 m = {{{c2 * c3, c2 * s3, -s2}, {s1 * s2 * c3 - c1 * s3, s1 * s2 * s3 + c1 * c3, s1 * c2}, {c1 * s2 * c3 + s1 * s3, c1 * s2 * s3 - s1 * c3, c1 * c2}}};

    return m;
}

// Vectors are interpreted as rows
inline Vector3 RotateVector(Vector3 v, Matrix3x3 m) {
    return (Vector3){v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0], v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1], v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2]};
}

Matrix3x3 MultiplyMatrix3x3(Matrix3x3 a, Matrix3x3 b) {
    Matrix3x3 r;
    int i, j, k;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            r.m[i][j] = 0;
            for (k = 0; k < 3; k++) {
                r.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }

    return r;
}

Matrix4x4 Identity4x4() { return (Matrix4x4){{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}}; }

Matrix4x4 GetProjectionMatrix(float rightPlane, float leftPlane, float topPlane, float bottomPlane, float nearPlane, float farPlane) {
    Matrix4x4 matrix = {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}};

    matrix.m[0][0] = 2.0f / (rightPlane - leftPlane);
    matrix.m[1][1] = 2.0f / (topPlane - bottomPlane);
    matrix.m[2][2] = -2.0f / (farPlane - nearPlane);
    matrix.m[3][3] = 1;
    matrix.m[3][0] = -(rightPlane + leftPlane) / (rightPlane - leftPlane);
    matrix.m[3][1] = -(topPlane + bottomPlane) / (topPlane - bottomPlane);
    matrix.m[3][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);

    return matrix;
}

// --------------- Numeric functions ---------------

float Lerp(double t, float a, float b) { return (1 - t) * a + t * b; }

int Step(float edge, float x) { return x < edge ? 0 : 1; }

float Smoothstep(float edge0, float edge1, float x) {
    // Scale, bias and saturate x to 0..1 range
    x = UTIL_clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    // Evaluate polynomial
    return x * x * (3 - 2 * x);
}

// Modulus function, returning only positive values
int Modulus(int a, int b) {
    int r = a % b;
    return r < 0 ? r + b : r;
}

float fModulus(float a, float b) {
    float r = fmod(a, b);
    return r < 0 ? r + b : r;
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

double JSON_GetObjectDouble(cJSON *object, char *string, double defaultValue) {
    cJSON *obj = cJSON_GetObjectItem(object, string);
    if (obj)
        return obj->valuedouble;
    else
        return defaultValue;
}

Vector3 JSON_GetObjectVector3(cJSON *object, char *string, Vector3 defaultValue) {

    cJSON *arr = cJSON_GetObjectItem(object, string);
    if (!arr) return defaultValue;

    Vector3 v = VECTOR3_ZERO;

    cJSON *item = cJSON_GetArrayItem(arr, 0);
    if (item) v.x = item->valuedouble;

    item = cJSON_GetArrayItem(arr, 1);
    if (item) v.y = item->valuedouble;

    item = cJSON_GetArrayItem(arr, 2);
    if (item) v.z = item->valuedouble;

    return v;
}

cJSON *JSON_CreateVector3(Vector3 value) {

    cJSON *v = cJSON_CreateArray();
    cJSON_AddItemToArray(v, cJSON_CreateNumber(value.x));
    cJSON_AddItemToArray(v, cJSON_CreateNumber(value.y));
    cJSON_AddItemToArray(v, cJSON_CreateNumber(value.z));

    return v;
}

// --------------- Lua stack manipulation functions ---------------

// Creates an table with the xyz entries and populate with the vector values
void Vector3ToTable(lua_State *L, Vector3 vector) {

    lua_newtable(L);
    lua_pushliteral(L, "x");      // x index
    lua_pushnumber(L, vector.x);  // x value
    lua_rawset(L, -3);            // Store x in table

    lua_pushliteral(L, "y");      // y index
    lua_pushnumber(L, vector.y);  // y value
    lua_rawset(L, -3);            // Store y in table

    lua_pushliteral(L, "z");      // z index
    lua_pushnumber(L, vector.z);  // z value
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
