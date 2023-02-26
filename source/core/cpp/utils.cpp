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

long long metadot_gettime() {
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return ms;
}

double metadot_gettime_d() {
    double t;
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    t = (ft.dwHighDateTime * 4294967296.0 / 1e7) + ft.dwLowDateTime / 1e7;
    t -= 11644473600.0;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    t = tv.tv_sec + tv.tv_usec / 1e6;
#endif
    return t;
}

time_t metadot_gettime_mkgmtime(struct tm *unixdate) {
    assert(unixdate != nullptr);
    time_t fakeUnixtime = mktime(unixdate);
    struct tm *fakeDate = gmtime(&fakeUnixtime);

    int32_t nOffSet = fakeDate->tm_hour - unixdate->tm_hour;
    if (nOffSet > 12) {
        nOffSet = 24 - nOffSet;
    }
    return fakeUnixtime - nOffSet * 3600;
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

TRIE_TYPE_FUNCTION_TEMPLATE_MACRO(vec3)
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

vec3 JSON_GetObjectVector3(cJSON *object, char *string, vec3 defaultValue) {

    cJSON *arr = cJSON_GetObjectItem(object, string);
    if (!arr) return defaultValue;

    vec3 v = VECTOR3_ZERO;

    cJSON *item = cJSON_GetArrayItem(arr, 0);
    if (item) v.x = item->valuedouble;

    item = cJSON_GetArrayItem(arr, 1);
    if (item) v.y = item->valuedouble;

    item = cJSON_GetArrayItem(arr, 2);
    if (item) v.z = item->valuedouble;

    return v;
}

cJSON *JSON_CreateVector3(vec3 value) {

    cJSON *v = cJSON_CreateArray();
    cJSON_AddItemToArray(v, cJSON_CreateNumber(value.x));
    cJSON_AddItemToArray(v, cJSON_CreateNumber(value.y));
    cJSON_AddItemToArray(v, cJSON_CreateNumber(value.z));

    return v;
}

// --------------- Lua stack manipulation functions ---------------

// Creates an table with the xyz entries and populate with the vector values
void Vector3ToTable(lua_State *L, vec3 vector) {

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

void Timer::start() {
    m_StartTime = std::chrono::steady_clock::now();
    m_bRunning = true;
}

double Timer::elapsedMilliseconds() {
    std::chrono::time_point<std::chrono::steady_clock> endTime;

    if (m_bRunning) {
        endTime = std::chrono::steady_clock::now();
    } else {
        endTime = m_EndTime;
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_StartTime).count();
}

double Timer::elapsedSeconds() { return elapsedMilliseconds() / 1000.0; }

void Timer::stop() {
    m_EndTime = std::chrono::steady_clock::now();
    m_bRunning = false;
}
