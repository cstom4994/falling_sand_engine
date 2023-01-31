// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "core/cpp/utils.hpp"

#include <ctype.h>
#include <stdio.h>

#include <chrono>
#include <cstdint>
#include <ctime>

#include "core/const.h"
#include "core/core.h"
#include "core/global.hpp"
#include "meta/meta.hpp"


std::vector<std::string> split(std::string strToSplit, char delimeter) {
    std::stringstream ss(strToSplit);
    std::string item;
    std::vector<std::string> splittedStrings;
    while (getline(ss, item, delimeter)) {
        splittedStrings.push_back(item);
    }
    return splittedStrings;
}

std::vector<std::string> string_split(std::string s, const char delimiter) {
    size_t start = 0;
    size_t end = s.find_first_of(delimiter);

    std::vector<std::string> output;

    while (end <= std::string::npos) {
        output.emplace_back(s.substr(start, end - start));

        if (end == std::string::npos) break;

        start = end + 1;
        end = s.find_first_of(delimiter, start);
    }

    return output;
}

std::vector<std::string> split2(std::string const &original, char separator) {
    std::vector<std::string> results;
    std::string::const_iterator start = original.begin();
    std::string::const_iterator end = original.end();
    std::string::const_iterator next = std::find(start, end, separator);
    while (next != end) {
        results.push_back(std::string(start, next));
        start = next + 1;
        next = std::find(start, end, separator);
    }
    results.push_back(std::string(start, next));
    return results;
}

long long Time::millis() {
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return ms;
}

time_t Time::mkgmtime(struct tm *unixdate) {
    assert(unixdate != nullptr);
    time_t fakeUnixtime = mktime(unixdate);
    struct tm *fakeDate = gmtime(&fakeUnixtime);

    int32_t nOffSet = fakeDate->tm_hour - unixdate->tm_hour;
    if (nOffSet > 12) {
        nOffSet = 24 - nOffSet;
    }
    return fakeUnixtime - nOffSet * 3600;
}

///////////////////////////////////////////////////////////////////////////////

Timer::Timer() : myOffSet(GetTime()), myPause(false), myPauseTime(0), myLastUpdate(0) { METADOT_ASSERT_E(sizeof(I64) == 8); }

//.............................................................................

Timer::~Timer() {}

///////////////////////////////////////////////////////////////////////////////

Timer &Timer::operator=(const Timer &other) {
    // BUGBUG	Does not work because the myBaseTimer might be different from
    //			the one where these where copied
    myOffSet = other.myOffSet;
    myPause = other.myPause;
    myPauseTime = other.myPauseTime;
    myLastUpdate = other.myLastUpdate;

    return *this;
}

//=============================================================================

Timer Timer::operator-(const Timer &other) { return (Timer(*this) -= (other)); }

//=============================================================================

Timer Timer::operator-(Timer::Ticks time) { return (Timer(*this) -= (time)); }

//=============================================================================

Timer &Timer::operator-=(const Timer &other) { return operator-=(other.GetTime()); }

//=============================================================================

Timer &Timer::operator-=(Timer::Ticks time) {
    myOffSet += time;
    myPauseTime -= time;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////

void Timer::SetTime(Timer::Ticks time) {
    myOffSet = (I64)GetTime() - (I64)time;
    myPauseTime = time;
}

//=============================================================================

Timer::Ticks Timer::GetTime() const {
    if (myPause) {
        if (myPauseTime < 0) return 0;

        return (Timer::Ticks)myPauseTime;
    }

    if ((unsigned)myOffSet > GetTime()) return 0;

    return (GetTime() - (Timer::Ticks)myOffSet);
}

//.............................................................................

float Timer::GetSeconds() const { return ((float)GetTime() / 1000.0f); }

//.............................................................................

Timer::Ticks Timer::GetDerivate() const { return (GetTime() - myLastUpdate); }

//.............................................................................

float Timer::GetDerivateSeconds() const { return ((float)GetDerivate() / 1000.0f); }

//.............................................................................

void Timer::Updated() { myLastUpdate = GetTime(); }

//.............................................................................

void Timer::Pause() {
    if (myPause == false) {
        myPauseTime = GetTime();
        myPause = true;
    }
}

//.............................................................................

void Timer::Resume() {
    if (myPause == true) {
        myPause = false;
        SetTime((Timer::Ticks)myPauseTime);
        // myOffSet += ( GetTime() - myPauseTime );
    }
}

//.............................................................................

void Timer::Reset() {
    // myPause = false;
    myPauseTime = 0;
    myOffSet = GetTime();
    myLastUpdate = 0;
}

namespace SUtil {
const int *utf8toCodePointsArray(const char *c, int *length) {
    // todo use something better than std::vector
    std::vector<int> out;
    char byte1 = 0;
    while ((byte1 = *c++) != 0) {
        if (!(byte1 & 0b10000000)) {
            out.push_back(byte1);
        }
        if ((byte1 & 0b11100000) == 0b11000000) {
            // starts with 110
            char byte2 = *c++;
            int b1 = (int)byte1 & 0b00011111;
            int b2 = (int)byte2 & 0b00111111;

            out.push_back(b2 | (b1 << 6));
        } else if ((byte1 & 0b11110000) == 0b11100000) {
            // starts with 1110
            char byte2 = *c++;
            char byte3 = *c++;

            int b1 = (int)byte1 & 0b00001111;
            int b2 = (int)byte2 & 0b00111111;
            int b3 = (int)byte3 & 0b00111111;

            out.push_back(b3 | (b2 << 6) | (b1 << 6));
        } else if ((byte1 & 0b11111000) == 0b11110000) {
            // starts with 1110
            char byte2 = *c++;
            char byte3 = *c++;
            char byte4 = *c++;

            int b1 = (int)byte1 & 0b00000111;
            int b2 = (int)byte2 & 0b00111111;
            int b3 = (int)byte3 & 0b00111111;
            int b4 = (int)byte4 & 0b00111111;

            out.push_back(b4 | (b3 << 6) | (b2 << 12) | (b1 << 18));
        }
    }
    if (length) *length = out.size();
    if (out.empty()) return nullptr;

    out.push_back(0);
    auto o = (int *)METAENGINE_MALLOC(out.size() * sizeof(int));
    memcpy(o, out.data(), out.size() * sizeof(int));
    return o;
}

std::u32string utf8toCodePoints(const char *c) {
    // todo use something better than std::vector
    std::u32string out;
    uint8_t byte1;
    while ((byte1 = *c++) != 0) {
        if (!(byte1 & 0b10000000)) {
            out.push_back(byte1);
        }
        if ((byte1 & 0b11100000) == 0b11000000) {
            // starts with 110
            uint8_t byte2 = *c++;
            int b1 = (int)byte1 & 0b00011111;
            int b2 = (int)byte2 & 0b00111111;

            out.push_back(b2 | (b1 << 6));
        } else if ((byte1 & 0b11110000) == 0b11100000) {
            // starts with 1110
            uint8_t byte2 = *c++;
            uint8_t byte3 = *c++;

            int b1 = (int)byte1 & 0b00001111;
            int b2 = (int)byte2 & 0b00111111;
            int b3 = (int)byte3 & 0b00111111;

            out.push_back(b3 | (b2 << 6) | (b1 << 12));
        } else if ((byte1 & 0b11111000) == 0b11110000) {
            // starts with 11110
            uint8_t byte2 = *c++;
            uint8_t byte3 = *c++;
            uint8_t byte4 = *c++;

            int b1 = (int)byte1 & 0b00000111;
            int b2 = (int)byte2 & 0b00111111;
            int b3 = (int)byte3 & 0b00111111;
            int b4 = (int)byte4 & 0b00111111;

            out.push_back(b4 | (b3 << 6) | (b2 << 12) | (b1 << 18));
        }
    }
    return out;
}

std::string u32StringToString(std::u32string_view s) {
    std::string out;
    for (auto c : s) out += (char)c;
    return out;
}
}  // namespace SUtil

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
        FreeTrieInternal((Trie *)trie->branch[i]);
        trie->branch[i] = NULL;
    }
    free(trie);
}

void FreeTrie(Trie *trie) {
    if (!trie) return;
    for (int i = 0; i < TRIE_ALPHABET_SIZE; i++) {
        FreeTrieInternal((Trie *)trie->branch[i]);
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
        cell = (Trie *)cell->branch[(unsigned char)key[i++]];
    }

    // If this key is not contained in the trie
    if (!cell->branch[(unsigned char)key[i]]) {
        // Insert the branches that are missing
        while (key[i] != '\0') {
            Trie *newCell = (Trie *)calloc(1, sizeof(Trie));
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
        cell = (Trie *)cell->branch[(unsigned char)key[i++]];
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
    void **data = (void **)GetTrieElementWithProperties(trie, key, &size, NULL);
    if (!data || size != sizeof(void *))
        return defaultValue;
    else
        return *data;
}

char *GetTrieElementAsString(Trie trie, const char *key, char *defaultValue) {
    TrieType elementType;
    char *data = (char *)GetTrieElementWithProperties(trie, key, NULL, &elementType);
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
        type *data = (type *)GetTrieElementWithProperties(trie, key, NULL, &elementType);                                         \
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
        char *keystr = (char *)malloc((depth + 2) * sizeof(char));
        strncpy(keystr, key, depth + 2);

        TrieElement newElement = {keystr, trie->elementType, trie->elementSize, trie->branch['\0']};
        elementsArray[position] = newElement;
        usedPositions++;
    }

    for (int i = 1; i < TRIE_ALPHABET_SIZE; i++) {
        if (trie->branch[i]) {
            key[depth + 1] = i;
            usedPositions += GetTrieElementsArrayInternal((Trie *)trie->branch[i], key, depth + 1, elementsArray, position + usedPositions);
        }
    }

    return usedPositions;
}

TrieElement *GetTrieElementsArray(Trie trie, int *outElementsCount) {
    assert(outElementsCount);

    *outElementsCount = trie.numberOfElements;
    TrieElement *array = (TrieElement *)malloc(trie.numberOfElements * sizeof(TrieElement));

    char *key = (char *)malloc((trie.maxKeySize + 1) * sizeof(char));

    int position = 0;
    for (int i = 1; i < TRIE_ALPHABET_SIZE; i++) {
        if (trie.branch[i]) {
            key[0] = i;
            position += GetTrieElementsArrayInternal((Trie *)trie.branch[i], key, 0, array, position);
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

        char *jsonString = (char *)malloc((size + 1) * sizeof(char));
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
