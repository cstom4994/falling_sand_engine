// Copyright(c) 2023, KaoruXun All rights reserved.

#include "utility.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>

#include "core/core.hpp"
#include "core/memory.h"
#include "core/platform.h"
#include "scripting/scripting.hpp"
// #include "runtime/ui/ui.hpp"

#define TIME_COUNT(x) std::chrono::time_point_cast<std::chrono::microseconds>(x).time_since_epoch().count()

void ME::Timer::start() noexcept { startPos = std::chrono::high_resolution_clock::now(); }

void ME::Timer::stop() noexcept {
    auto endTime = std::chrono::high_resolution_clock::now();
    duration = static_cast<double>(TIME_COUNT(endTime) - TIME_COUNT(startPos)) * 0.001;
}

ME::Timer::~Timer() noexcept { stop(); }

double ME::Timer::get() const noexcept { return duration; }

void ME::logger::set_crash_on_error(bool bError) noexcept { loggerInternal.using_errors = bError; }

void ME::logger::set_current_log_file(const char *file) noexcept {
    loggerInternal.shutdown_file_stream();
    loggerInternal.fileout = std::ofstream(file);
}

void ME::logger::set_log_operation(log_operations op) noexcept { loggerInternal.operation_type = op; }

void ME::logger_internal::writeline(std::string &msg) {
    // char msgbuf[512];
    // sprintf(msgbuf, "%s", msg.c_str());
    OutputDebugStringA(msg.c_str());
}

std::string ME::logger_internal::get_current_time() noexcept {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::string realTime = std::ctime(&now);
    realTime.erase(24);

    return realTime;
}

void ME::logger_internal::shutdown_file_stream() noexcept { fileout.close(); }

ME::logger_internal::logger_internal() noexcept {}

ME::logger_internal::~logger_internal() noexcept { shutdown_file_stream(); }

// --------------- Generic list Functions ---------------

list ME_create_list(unsigned size) {
    list l;
    l.first = NULL;
    l.last = NULL;
    l.elementSize = size;
    l.length = 0;

    return l;
}

void ME_destory_list(list *list) {
    while (list->first) {
        ME_list_remove_start(list);
    }
}

int ME_list_is_empty(list list) { return list.first == NULL ? 1 : 0; }

void ME_list_insert_end(list *list, void *e) {
    list_cell_pointer newCell = (list_cell_pointer)ME_MALLOC(sizeof(list_cell));
    newCell->previous = list->last;
    newCell->next = NULL;

    newCell->element = ME_MALLOC(list->elementSize);
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

void ME_list_insert_start(list *list, void *e) {
    list_cell_pointer newCell = (list_cell_pointer)ME_MALLOC(sizeof(list_cell));
    newCell->previous = NULL;
    newCell->next = list->first;

    newCell->element = ME_MALLOC(list->elementSize);
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

void ME_list_insert_index(list *list, void *e, int index) {
    int i;

    if (index < 0)  // Support acessing from the end of the list
        index += list->length;

    // Get the element that will go after the element to be inserted
    list_cell_pointer current = list->first;
    for (i = 0; i < index; i++) {
        current = ME_list_get_cell_next(current);
    }

    // If the index is already ocupied
    if (current != NULL) {
        list_cell_pointer newCell = (list_cell_pointer)ME_MALLOC(sizeof(list_cell));
        newCell->element = ME_MALLOC(list->elementSize);
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
        ME_list_insert_end(list, e);
    }
}

void ME_list_remove_cell(list *list, list_cell_pointer cell) {
    if (!cell->previous)
        return ME_list_remove_start(list);
    else if (!cell->next)
        return ME_list_remove_end(list);

    cell->next->previous = cell->previous;
    cell->previous->next = cell->next;

    ME_FREE(cell->element);
    ME_FREE(cell);

    list->length -= 1;
}

void ME_list_remove_end(list *list) {
    if (list->last->previous) {
        list_cell_pointer aux = list->last->previous;
        ME_FREE(list->last->element);
        ME_FREE(list->last);

        aux->next = NULL;
        list->last = aux;
    } else {
        ME_FREE(list->last->element);
        ME_FREE(list->last);

        list->last = NULL;
        list->first = NULL;
    }

    list->length -= 1;
}

void ME_list_remove_start(list *list) {
    if (ME_list_is_empty(*list)) return;

    if (list->first->next) {
        list_cell_pointer aux = list->first->next;
        ME_FREE(list->first->element);
        ME_FREE(list->first);

        aux->previous = NULL;
        list->first = aux;
    } else {
        ME_FREE(list->first->element);
        ME_FREE(list->first);

        list->first = NULL;
        list->last = NULL;
    }

    list->length -= 1;
}

void ME_list_remove_index(list *list, int index) {
    int i;

    if (index < 0)  // Support acessing from the end of the list
        index += list->length;

    if (index == 0)
        return ME_list_remove_start(list);
    else if (index == list->length - 1)
        return ME_list_remove_end(list);

    list_cell_pointer current = list->first;
    for (i = 0; i < index; i++) {
        current = ME_list_get_cell_next(current);
    }

    current->next->previous = current->previous;
    current->previous->next = current->next;

    ME_FREE(current->element);
    ME_FREE(current);

    list->length -= 1;
}

void *ME_list_get_element(list_cell c) { return c.element; }

void *ME_list_get_last(list list) {
    if (!list.last) return NULL;
    return list.last->element;
}

void *ME_list_get_first(list list) {
    if (!list.first) return NULL;
    return list.first->element;
}

void *ME_list_get_at(list list, int index) {
    int i;

    if (index < 0)  // Support acessing from the end of the list
        index += list.length;

    list_cell_pointer current = list.first;
    for (i = 0; i < index; i++) {
        current = ME_list_get_cell_next(current);
    }

    return current->element;
}

list_cell_pointer ME_list_get_cell_next(list_cell_pointer c) {
    if (!c) return NULL;
    return c->next;
}

list_cell_pointer ME_list_get_cell_previous(list_cell_pointer c) {
    if (!c) return NULL;
    return c->previous;
}

list_cell_pointer ME_list_get_cell_first(list list) { return list.first; }

list_cell_pointer ME_list_get_cell_last(list list) { return list.last; }

list_cell_pointer ME_list_get_cell_at(list list, int index) {
    int i;

    list_cell_pointer current = list.first;
    for (i = 0; i < index; i++) {
        current = ME_list_get_cell_next(current);
    }

    return current;
}

unsigned ME_list_get_element_size(list list) { return list.elementSize; }

unsigned ME_list_get_length(list list) { return list.length; }

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

long long ME_gettime() {
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return ms;
}

double ME_gettime_d() {
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

time_t ME_gettime_mkgmtime(struct tm *unixdate) {
    assert(unixdate != nullptr);
    time_t fakeUnixtime = mktime(unixdate);
    struct tm *fakeDate = gmtime(&fakeUnixtime);

    int32_t nOffSet = fakeDate->tm_hour - unixdate->tm_hour;
    if (nOffSet > 12) {
        nOffSet = 24 - nOffSet;
    }
    return fakeUnixtime - nOffSet * 3600;
}

const int *ME_str_utf8to_codepointsarray(const char *c, int *length) {
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
    auto o = (int *)ME_MALLOC(out.size() * sizeof(int));
    memcpy(o, out.data(), out.size() * sizeof(int));
    return o;
}

std::u32string ME_str_utf8to_codepoints(const char *c) {
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

std::string ME_str_u32stringto_string(std::u32string_view s) {
    std::string out;
    for (auto c : s) out += (char)c;
    return out;
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

void ME_FREETrieInternal(Trie *trie) {
    if (!trie) return;
    if (trie->branch['\0']) {
        ME_FREE(trie->branch['\0']);
        trie->branch['\0'] = NULL;
    }
    for (int i = 0; i < TRIE_ALPHABET_SIZE; i++) {
        ME_FREETrieInternal((Trie *)trie->branch[i]);
        trie->branch[i] = NULL;
    }
    ME_FREE(trie);
}

void ME_FREETrie(Trie *trie) {
    if (!trie) return;
    for (int i = 0; i < TRIE_ALPHABET_SIZE; i++) {
        ME_FREETrieInternal((Trie *)trie->branch[i]);
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
        ME_FREE(cell->branch['\0']);
        trie->numberOfElements--;
    }

    cell->elementSize = size;
    cell->elementType = valueType;
    cell->branch['\0'] = ME_MALLOC(size);
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

TRIE_TYPE_FUNCTION_TEMPLATE_MACRO(MEvec3)
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
        char *keystr = (char *)ME_MALLOC((depth + 2) * sizeof(char));
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
    TrieElement *array = (TrieElement *)ME_MALLOC(trie.numberOfElements * sizeof(TrieElement));

    char *key = (char *)ME_MALLOC((trie.maxKeySize + 1) * sizeof(char));

    int position = 0;
    for (int i = 1; i < TRIE_ALPHABET_SIZE; i++) {
        if (trie.branch[i]) {
            key[0] = i;
            position += GetTrieElementsArrayInternal((Trie *)trie.branch[i], key, 0, array, position);
        }
    }

    ME_FREE(key);
    return array;
}

void FreeTrieElementsArray(TrieElement *elementsArray, int elementsCount) {
    assert(elementsArray);
    int i;
    for (i = 0; i < elementsCount; i++) {
        ME_FREE(elementsArray[i].key);
    }
    ME_FREE(elementsArray);
}

// --------------- Lua stack manipulation functions ---------------

// Creates an table with the xyz entries and populate with the vector values
void Vector3ToTable(lua_State *L, MEvec3 vector) {

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
