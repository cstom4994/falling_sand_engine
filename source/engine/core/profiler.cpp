
#include "profiler.hpp"

// #include <GLFW/glfw3.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "engine/core/base_memory.h"
#include "engine/core/core.hpp"
#include "engine/core/platform.h"
#include "engine/renderer/gpu.hpp"
#include "engine/ui/surface.h"
#include "engine/utils/utility.hpp"
#include "libs/lz4/lz4.h"

u64 profiler_get_clock_frequency();

// Data load/save functions

template <typename T>
static inline void write_var(u8 *&_buffer, T _var) {
    memcpy(_buffer, &_var, sizeof(T));
    _buffer += sizeof(T);
}

static inline void write_str(u8 *&_buffer, const char *_str) {
    u32 len = (u32)strlen(_str);
    write_var(_buffer, len);
    memcpy(_buffer, _str, len);
    _buffer += len;
}

template <typename T>
static inline void read_var(u8 *&_buffer, T &_var) {
    memcpy(&_var, _buffer, sizeof(T));
    _buffer += sizeof(T);
}

static inline char *read_string(u8 *&_buffer) {
    u32 len;
    read_var(_buffer, len);
    char *str = new char[len + 1];
    memcpy(str, _buffer, len);
    str[len] = 0;
    _buffer += len;
    return str;
}

static inline const char *duplicate_string(const char *_str) {
    char *str = new char[strlen(_str) + 1];
    strcpy(str, _str);
    return str;
}

struct string_store {
    typedef std::unordered_map<std::string, u32> string_to_index_type;
    typedef std::unordered_map<u32, std::string> index_to_string_type;

    u32 m_totalSize;
    string_to_index_type m_stringIndexMap;
    index_to_string_type m_strings;

    string_store() : m_totalSize(0) {}

    void add_string(const char *_str) {
        string_to_index_type::iterator it = m_stringIndexMap.find(_str);
        if (it == m_stringIndexMap.end()) {
            u32 index = (u32)m_stringIndexMap.size();
            m_totalSize += 4 + (u32)strlen(_str);  // see writeStr for details
            m_stringIndexMap[_str] = index;
            m_strings[index] = _str;
        }
    }

    u32 get_string(const char *_str) { return m_stringIndexMap[_str]; }
};

ME::profiler::profiler_context *g_context = 0;

void ME_profiler_init() { g_context = new ME::profiler::profiler_context(); }

void ME_profiler_shutdown() {
    delete g_context;
    g_context = 0;
}

void ME_profiler_set_threshold(f32 _ms, int _level) { g_context->set_threshold(_ms, _level); }

void ME_profiler_register_thread(const char *_name, u64 _threadID) {
    if (_threadID == 0) _threadID = ME_get_thread_id();

    g_context->register_thread(_threadID, _name);
}

void ME_profiler_unregister_thread(u64 _threadID) { g_context->unregister_thread(_threadID); }

void ME_profiler_begin_frame() { g_context->begin_frame(); }

uintptr_t ME_profiler_begin_scope(const char *_file, int _line, const char *_name) { return (uintptr_t)g_context->begin_ccope(_file, _line, _name); }

void ME_profiler_end_scope(uintptr_t _scopeHandle) { g_context->end_scope((profiler_scope *)_scopeHandle); }

int ME_profiler_is_paused() { return g_context->is_paused() ? 1 : 0; }

int ME_profiler_was_threshold_crossed() { return g_context->was_threshold_crossed() ? 1 : 0; }

void ME_profiler_set_paused(int _paused) { return g_context->set_paused(_paused != 0); }

void ME_profiler_get_frame(profiler_frame *_data) {
    g_context->get_frame_data(_data);

    // clamp scopes crossing frame boundary
    const u32 numScopes = _data->m_numScopes;
    for (u32 i = 0; i < numScopes; ++i) {
        profiler_scope &cs = _data->m_scopes[i];

        if (cs.m_start == cs.m_end) {
            cs.m_end = _data->m_endtime;
            if (cs.m_start < _data->m_startTime) cs.m_start = _data->m_startTime;
        }
    }
}

int ME_profiler_save(profiler_frame *_data, void *_buffer, size_t _bufferSize) {
    // fill string data
    string_store strStore;
    for (u32 i = 0; i < _data->m_numScopes; ++i) {
        profiler_scope &scope = _data->m_scopes[i];
        strStore.add_string(scope.m_name);
        strStore.add_string(scope.m_file);
    }
    for (u32 i = 0; i < _data->m_numThreads; ++i) {
        strStore.add_string(_data->m_threads[i].m_name);
    }

    // calc data size
    u32 totalSize = _data->m_numScopes * sizeof(profiler_scope) + _data->m_numThreads * sizeof(profiler_thread) + sizeof(profiler_frame) + strStore.m_totalSize;

    u8 *buffer = new u8[totalSize];
    u8 *bufPtr = buffer;

    write_var(buffer, _data->m_startTime);
    write_var(buffer, _data->m_endtime);
    write_var(buffer, _data->m_prevFrameTime);
    // writeVar(buffer, _data->m_platformID);
    write_var(buffer, profiler_get_clock_frequency());

    // write scopes
    write_var(buffer, _data->m_numScopes);
    for (u32 i = 0; i < _data->m_numScopes; ++i) {
        profiler_scope &scope = _data->m_scopes[i];
        write_var(buffer, scope.m_start);
        write_var(buffer, scope.m_end);
        write_var(buffer, scope.m_threadID);
        write_var(buffer, strStore.get_string(scope.m_name));
        write_var(buffer, strStore.get_string(scope.m_file));
        write_var(buffer, scope.m_line);
        write_var(buffer, scope.m_level);
    }

    // write thread info
    write_var(buffer, _data->m_numThreads);
    for (u32 i = 0; i < _data->m_numThreads; ++i) {
        profiler_thread &t = _data->m_threads[i];
        write_var(buffer, t.m_threadID);
        write_var(buffer, strStore.get_string(t.m_name));
    }

    // write string data
    u32 numStrings = (u32)strStore.m_strings.size();
    write_var(buffer, numStrings);

    for (u32 i = 0; i < strStore.m_strings.size(); ++i) write_str(buffer, strStore.m_strings[i].c_str());

    int compSize = LZ4_compress_default((const char *)bufPtr, (char *)_buffer, (int)(buffer - bufPtr), (int)_bufferSize);
    delete[] bufPtr;
    return compSize;
}

void ME_profiler_load(profiler_frame *_data, void *_buffer, size_t _bufferSize) {
    size_t bufferSize = _bufferSize;
    u8 *buffer = 0;
    u8 *bufferPtr;

    int decomp = -1;
    do {
        delete[] buffer;
        bufferSize *= 2;
        buffer = new u8[bufferSize];
        decomp = LZ4_decompress_safe((const char *)_buffer, (char *)buffer, (int)_bufferSize, (int)bufferSize);

    } while (decomp < 0);

    bufferPtr = buffer;

    u32 strIdx;

    read_var(buffer, _data->m_startTime);
    read_var(buffer, _data->m_endtime);
    read_var(buffer, _data->m_prevFrameTime);
    // readVar(buffer, _data->m_platformID);
    read_var(buffer, _data->m_CPUFrequency);

    // read scopes
    read_var(buffer, _data->m_numScopes);

    _data->m_scopes = new profiler_scope[_data->m_numScopes * 2];  // extra space for viewer - m_scopesStats
    _data->m_scopesStats = &_data->m_scopes[_data->m_numScopes];
    _data->m_scopeStatsInfo = new profiler_scope_stats[_data->m_numScopes * 2];

    for (u32 i = 0; i < _data->m_numScopes * 2; ++i) _data->m_scopes[i].m_stats = &_data->m_scopeStatsInfo[i];

    for (u32 i = 0; i < _data->m_numScopes; ++i) {
        profiler_scope &scope = _data->m_scopes[i];
        read_var(buffer, scope.m_start);
        read_var(buffer, scope.m_end);
        read_var(buffer, scope.m_threadID);
        read_var(buffer, strIdx);
        scope.m_name = (const char *)(uintptr_t)strIdx;
        read_var(buffer, strIdx);
        scope.m_file = (const char *)(uintptr_t)strIdx;
        read_var(buffer, scope.m_line);
        read_var(buffer, scope.m_level);

        scope.m_stats->m_inclusiveTime = scope.m_end - scope.m_start;
        scope.m_stats->m_exclusiveTime = scope.m_stats->m_inclusiveTime;
        scope.m_stats->m_occurences = 0;
    }

    // read thread info
    read_var(buffer, _data->m_numThreads);
    _data->m_threads = new profiler_thread[_data->m_numThreads];
    for (u32 i = 0; i < _data->m_numThreads; ++i) {
        profiler_thread &t = _data->m_threads[i];
        read_var(buffer, t.m_threadID);
        read_var(buffer, strIdx);
        t.m_name = (const char *)(uintptr_t)strIdx;
    }

    // read string data
    u32 numStrings;
    read_var(buffer, numStrings);

    const char *strings[ME_SCOPES_MAX];
    for (u32 i = 0; i < numStrings; ++i) strings[i] = read_string(buffer);

    for (u32 i = 0; i < _data->m_numScopes; ++i) {
        profiler_scope &scope = _data->m_scopes[i];
        uintptr_t idx = (uintptr_t)scope.m_name;
        scope.m_name = duplicate_string(strings[(u32)idx]);

        idx = (uintptr_t)scope.m_file;
        scope.m_file = duplicate_string(strings[(u32)idx]);
    }

    for (u32 i = 0; i < _data->m_numThreads; ++i) {
        profiler_thread &t = _data->m_threads[i];
        uintptr_t idx = (uintptr_t)t.m_name;
        t.m_name = duplicate_string(strings[(u32)idx]);
    }

    for (u32 i = 0; i < numStrings; ++i) delete[] strings[i];

    delete[] bufferPtr;

    // process frame data

    for (u32 i = 0; i < _data->m_numScopes; ++i)
        for (u32 j = 0; j < _data->m_numScopes; ++j) {
            profiler_scope &scopeI = _data->m_scopes[i];
            profiler_scope &scopeJ = _data->m_scopes[j];

            if ((scopeJ.m_start > scopeI.m_start) && (scopeJ.m_end < scopeI.m_end) && (scopeJ.m_level == scopeI.m_level + 1) && (scopeJ.m_threadID == scopeI.m_threadID))
                scopeI.m_stats->m_exclusiveTime -= scopeJ.m_stats->m_inclusiveTime;
        }

    _data->m_numScopesStats = 0;

    for (u32 i = 0; i < _data->m_numScopes; ++i) {
        profiler_scope &scopeI = _data->m_scopes[i];

        scopeI.m_stats->m_inclusiveTimeTotal = scopeI.m_stats->m_inclusiveTime;
        scopeI.m_stats->m_exclusiveTimeTotal = scopeI.m_stats->m_exclusiveTime;

        int foundIndex = -1;
        for (u32 j = 0; j < _data->m_numScopesStats; ++j) {
            profiler_scope &scopeJ = _data->m_scopesStats[j];
            if (strcmp(scopeI.m_name, scopeJ.m_name) == 0) {
                foundIndex = j;
                break;
            }
        }

        if (foundIndex == -1) {
            int index = _data->m_numScopesStats++;
            profiler_scope &scope = _data->m_scopesStats[index];
            scope = scopeI;
            scope.m_stats->m_occurences = 1;
        } else {
            profiler_scope &scope = _data->m_scopesStats[foundIndex];
            scope.m_stats->m_inclusiveTimeTotal += scopeI.m_stats->m_inclusiveTime;
            scope.m_stats->m_exclusiveTimeTotal += scopeI.m_stats->m_exclusiveTime;
            scope.m_stats->m_occurences++;
        }
    }
}

void ME_profiler_load_time_only(f32 *_time, void *_buffer, size_t _bufferSize) {
    size_t bufferSize = _bufferSize;
    u8 *buffer = 0;
    u8 *bufferPtr;

    int decomp = -1;
    do {
        delete[] buffer;
        bufferSize *= 2;
        buffer = new u8[bufferSize];
        decomp = LZ4_decompress_safe((const char *)_buffer, (char *)buffer, (int)_bufferSize, (int)bufferSize);

    } while (decomp < 0);

    bufferPtr = buffer;

    u64 startTime;
    u64 endtime;
    u8 dummy8;
    u64 frequency;

    read_var(buffer, startTime);
    read_var(buffer, endtime);
    read_var(buffer, frequency);  // dummy
    read_var(buffer, dummy8);     // dummy
    read_var(buffer, frequency);
    *_time = profiler_clock2ms(endtime - startTime, frequency);

    delete[] buffer;
}

void ME_profiler_release(profiler_frame *_data) {
    for (u32 i = 0; i < _data->m_numScopes; ++i) {
        profiler_scope &scope = _data->m_scopes[i];
        delete[] scope.m_name;
        delete[] scope.m_file;
    }

    for (u32 i = 0; i < _data->m_numThreads; ++i) {
        profiler_thread &t = _data->m_threads[i];
        delete[] t.m_name;
    }

    delete[] _data->m_scopes;
    delete[] _data->m_threads;
    delete[] _data->m_scopeStatsInfo;
}

u64 ME_profiler_get_clock() {
#ifdef ME_PLATFORM_WINDOWS
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
    u64 q = __rdtsc();
#else
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    int64_t q = li.QuadPart;
#endif
#else
    struct timeval now;
    gettimeofday(&now, 0);
    int64_t q = now.tv_sec * 1000000 + now.tv_usec;
#endif
    return q;
}

u64 profiler_get_clock_frequency() {
#ifdef ME_PLATFORM_WINDOWS
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#ifndef ME_RELEASE
    static LONGLONG frequency = 1;
    static bool initialized = false;
    if (!initialized) {
        LARGE_INTEGER li1, li2;
        QueryPerformanceCounter(&li1);
        LONGLONG tsc1 = __rdtsc();
        for (int i = 0; i < 230000000; ++i)
            ;
        LONGLONG tsc2 = __rdtsc();
        QueryPerformanceCounter(&li2);

        LARGE_INTEGER lif;
        QueryPerformanceFrequency(&lif);
        LONGLONG time = ((li2.QuadPart - li1.QuadPart) * 1000) / lif.QuadPart;
        frequency = (LONGLONG)(1000 * ((tsc2 - tsc1) / time));
        initialized = true;
    }
    return frequency;
#else
    return 2300000000;
#endif
#else
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return li.QuadPart;
#endif
#else
    return 1000000;
#endif
}

f32 profiler_clock2ms(u64 _clock, u64 _frequency) { return (f32(_clock) / f32(_frequency)) * 1000.0f; }

void ME_profiler_free_list_create(size_t _blockSize, u32 _maxBlocks, profiler_free_list_t *_freeList) {
    _freeList->m_maxBlocks = _maxBlocks;
    _freeList->m_blockSize = (u32)_blockSize;
    _freeList->m_blocksFree = _maxBlocks;
    _freeList->m_blocksAlllocated = 0;
    _freeList->m_buffer = (u8 *)ME_MALLOC(_blockSize * _maxBlocks);
    _freeList->m_next = _freeList->m_buffer;
}

void ME_profiler_free_list_destroy(profiler_free_list_t *_freeList) { ME_FREE(_freeList->m_buffer); }

void *ME_profiler_free_list_alloc(profiler_free_list_t *_freeList) {
    if (_freeList->m_blocksAlllocated < _freeList->m_maxBlocks) {
        u32 *p = (u32 *)(_freeList->m_buffer + (_freeList->m_blocksAlllocated * _freeList->m_blockSize));
        *p = _freeList->m_blocksAlllocated + 1;
        _freeList->m_blocksAlllocated++;
    }

    void *ret = 0;
    if (_freeList->m_blocksFree) {
        ret = _freeList->m_next;
        --_freeList->m_blocksFree;
        if (_freeList->m_blocksFree)
            _freeList->m_next = _freeList->m_buffer + (*(u32 *)_freeList->m_next * _freeList->m_blockSize);
        else
            _freeList->m_next = 0;
    }
    return ret;
}

void ME_profiler_free_list_free(profiler_free_list_t *_freeList, void *_ptr) {
    if (_freeList->m_next) {
        u32 index = ((u32)(_freeList->m_next - _freeList->m_buffer)) / _freeList->m_blockSize;
        *(u32 *)_ptr = index;
        _freeList->m_next = (u8 *)_ptr;
    } else {
        *(u32 *)_ptr = _freeList->m_maxBlocks;
        _freeList->m_next = (u8 *)_ptr;
    }
    ++_freeList->m_blocksFree;
}

int ME_profiler_free_list_check_ptr(profiler_free_list_t *_freeList, void *_ptr) {
    return ((uintptr_t)_freeList->m_maxBlocks * (uintptr_t)_freeList->m_blockSize) > (uintptr_t)(((u8 *)_ptr) - _freeList->m_buffer) ? 1 : 0;
}

namespace ME::profiler {

profiler_context::profiler_context()
    : m_scopesOpen(0), m_displayScopes(0), m_frameStartTime(0), m_frameEndTime(0), m_thresholdCrossed(false), m_timeThreshold(0.0f), m_levelThreshold(0), m_pauseProfiling(false) {

    m_tlsLevel = ME_tls_allocate();

    ME_profiler_free_list_create(sizeof(profiler_scope), ME_SCOPES_MAX, &m_scopesAllocator);

    for (int i = 0; i < buffer_use::Count; ++i) {
        m_namesSize[i] = 0;
        m_namesData[i] = m_namesDataBuffers[i];
    }
}

profiler_context::~profiler_context() {
    ME_profiler_free_list_destroy(&m_scopesAllocator);
    ME_tls_free(m_tlsLevel);
}

void profiler_context::set_threshold(f32 _ms, int _levelThreshold) {
    m_timeThreshold = _ms;
    m_levelThreshold = _levelThreshold;
}

bool profiler_context::is_paused() { return m_pauseProfiling; }

bool profiler_context::was_threshold_crossed() { return !m_pauseProfiling && m_thresholdCrossed; }

void profiler_context::set_paused(bool _paused) { m_pauseProfiling = _paused; }

void profiler_context::register_thread(u64 _threadID, const char *_name) { m_threadNames[_threadID] = _name; }

void profiler_context::unregister_thread(u64 _threadID) { m_threadNames.erase(_threadID); }

void profiler_context::begin_frame() {
    ME::scoped_mutex_locker lock(m_mutex);

    u64 frameBeginTime, frameEndTime;
    static u64 beginPrevFrameTime = ME_profiler_get_clock();
    frameBeginTime = beginPrevFrameTime;
    frameEndTime = ME_profiler_get_clock();
    beginPrevFrameTime = frameEndTime;

    static u64 frameTime = frameEndTime - frameBeginTime;

    m_thresholdCrossed = false;

    int level = (int)m_levelThreshold - 1;

    u32 scopesToRestart = 0;

    m_namesSize[buffer_use::Open] = 0;

    static profiler_scope scopesDisplay[ME_SCOPES_MAX];
    for (u32 i = 0; i < m_scopesOpen; ++i) {
        profiler_scope *scope = m_scopesCapture[i];

        if (scope->m_start == scope->m_end) scope->m_name = add_string(scope->m_name, buffer_use::Open);

        scopesDisplay[i] = *scope;

        // scope that was not closed, spans frame boundary
        // keep it for next frame
        if (scope->m_start == scope->m_end)
            m_scopesCapture[scopesToRestart++] = scope;
        else {
            ME_profiler_free_list_free(&m_scopesAllocator, scope);
            scope = &scopesDisplay[i];
        }

        // did scope cross threshold?
        if (level == (int)scope->m_level) {
            u64 scopeEnd = scope->m_end;
            if (scope->m_start == scope->m_end) scopeEnd = frameEndTime;

            if (m_timeThreshold <= profiler_clock2ms(scopeEnd - scope->m_start, profiler_get_clock_frequency())) m_thresholdCrossed = true;
        }
    }

    // did frame cross threshold ?
    f32 prevFrameTime = profiler_clock2ms(frameEndTime - frameBeginTime, profiler_get_clock_frequency());
    if ((level == -1) && (m_timeThreshold <= prevFrameTime)) m_thresholdCrossed = true;

    if (m_thresholdCrossed && !m_pauseProfiling) {
        std::swap(m_namesData[buffer_use::Capture], m_namesData[buffer_use::Display]);

        memcpy(m_scopesDisplay, scopesDisplay, sizeof(profiler_scope) * m_scopesOpen);

        m_displayScopes = m_scopesOpen;
        m_frameStartTime = frameBeginTime;
        m_frameEndTime = frameEndTime;
    }

    m_namesSize[buffer_use::Capture] = 0;
    for (u32 i = 0; i < scopesToRestart; ++i) m_scopesCapture[i]->m_name = add_string(m_scopesCapture[i]->m_name, buffer_use::Capture);

    m_scopesOpen = scopesToRestart;
    frameTime = frameEndTime - frameBeginTime;
}

int profiler_context::inc_level() {
    // may be a first call on this thread
    void *tl = ME_tls_get_value(m_tlsLevel);
    if (!tl) {
        // we'd like to start with -1 but then the ++ operator below
        // would result in NULL value for tls so we offset by 2
        tl = (void *)1;
        ME_tls_set_value(m_tlsLevel, tl);
    }
    intptr_t threadLevel = (intptr_t)tl - 1;
    ME_tls_set_value(m_tlsLevel, (void *)(threadLevel + 2));
    return (int)threadLevel;
}

void profiler_context::dec_level() {
    intptr_t threadLevel = (intptr_t)ME_tls_get_value(m_tlsLevel);
    --threadLevel;
    ME_tls_set_value(m_tlsLevel, (void *)threadLevel);
}

profiler_scope *profiler_context::begin_ccope(const char *_file, int _line, const char *_name) {
    profiler_scope *scope = 0;
    {
        ME::scoped_mutex_locker lock(m_mutex);
        if (m_scopesOpen == ME_SCOPES_MAX) return 0;

        scope = (profiler_scope *)ME_profiler_free_list_alloc(&m_scopesAllocator);
        m_scopesCapture[m_scopesOpen++] = scope;

        scope->m_name = add_string(_name, buffer_use::Capture);
        scope->m_start = ME_profiler_get_clock();
        scope->m_end = scope->m_start;
    }

    scope->m_threadID = ME_get_thread_id();
    scope->m_file = _file;
    scope->m_line = _line;
    scope->m_level = inc_level();

    return scope;
}

void profiler_context::end_scope(profiler_scope *_scope) {
    if (!_scope) return;

    _scope->m_end = ME_profiler_get_clock();
    dec_level();
}

const char *profiler_context::add_string(const char *_name, buffer_use _buffer) {
    char *nameData = m_namesData[_buffer];
    int &nameSize = m_namesSize[_buffer];

    char c, *ret = &nameData[nameSize];
    while ((c = *_name++) && (nameSize < ME_TEXT_MAX)) nameData[nameSize++] = c;

    if (nameSize < ME_TEXT_MAX)
        nameData[nameSize++] = 0;
    else
        nameData[ME_TEXT_MAX - 1] = 0;

    return ret;
}

void profiler_context::get_frame_data(profiler_frame *_data) {
    ME::scoped_mutex_locker lock(m_mutex);

    static profiler_thread threadData[ME_DRAW_THREADS_MAX];

    u32 numThreads = (u32)m_threadNames.size();
    if (numThreads > ME_DRAW_THREADS_MAX) numThreads = ME_DRAW_THREADS_MAX;

    _data->m_numScopes = m_displayScopes;
    _data->m_scopes = m_scopesDisplay;
    _data->m_numThreads = numThreads;
    _data->m_threads = threadData;
    _data->m_startTime = m_frameStartTime;
    _data->m_endtime = m_frameEndTime;
    _data->m_prevFrameTime = m_frameEndTime - m_frameStartTime;
    _data->m_CPUFrequency = profiler_get_clock_frequency();
    _data->m_timeThreshold = m_timeThreshold;
    _data->m_levelThreshold = m_levelThreshold;

    std::map<u64, std::string>::iterator it = m_threadNames.begin();
    for (u32 i = 0; i < numThreads; ++i) {
        threadData[i].m_threadID = it->first;
        threadData[i].m_name = it->second.c_str();
        ++it;
    }
}

}  // namespace ME::profiler

void ME_profiler_graph_init(profiler_graph *fps, int style, const char *name) {
    memset(fps, 0, sizeof(profiler_graph));
    fps->style = style;
    strncpy(fps->name, name, sizeof(fps->name));
    fps->name[sizeof(fps->name) - 1] = '\0';
}

void ME_profiler_graph_update(profiler_graph *fps, float frameTime) {
    fps->head = (fps->head + 1) % GRAPH_HISTORY_COUNT;
    fps->values[fps->head] = frameTime;
}

float ME_profiler_graph_avg(profiler_graph *fps) {
    int i;
    float avg = 0;
    for (i = 0; i < GRAPH_HISTORY_COUNT; i++) {
        avg += fps->values[i];
    }
    return avg / (float)GRAPH_HISTORY_COUNT;
}

void ME_profiler_graph_render(MEsurface_context *surface, float x, float y, profiler_graph *fps) {
    int i;
    float avg, w, h;
    char str[64];

    avg = ME_profiler_graph_avg(fps);

    w = 200;
    h = 35;

    ME_surface_BeginPath(surface);
    ME_surface_Rect(surface, x, y, w, h);
    ME_surface_FillColor(surface, ME_surface_RGBA(0, 0, 0, 128));
    ME_surface_Fill(surface);

    ME_surface_BeginPath(surface);
    ME_surface_MoveTo(surface, x, y + h);
    if (fps->style == GRAPH_RENDER_FPS) {
        for (i = 0; i < GRAPH_HISTORY_COUNT; i++) {
            float v = 1.0f / (0.00001f + fps->values[(fps->head + i) % GRAPH_HISTORY_COUNT]);
            float vx, vy;
            if (v > 250.0f) v = 250.0f;
            vx = x + ((float)i / (GRAPH_HISTORY_COUNT - 1)) * w;
            vy = y + h - ((v / 250.0f) * h);
            ME_surface_LineTo(surface, vx, vy);
        }
    } else if (fps->style == GRAPH_RENDER_PERCENT) {
        for (i = 0; i < GRAPH_HISTORY_COUNT; i++) {
            float v = fps->values[(fps->head + i) % GRAPH_HISTORY_COUNT] * 1.0f;
            float vx, vy;
            if (v > 100.0f) v = 100.0f;
            vx = x + ((float)i / (GRAPH_HISTORY_COUNT - 1)) * w;
            vy = y + h - ((v / 100.0f) * h);
            ME_surface_LineTo(surface, vx, vy);
        }
    } else {
        for (i = 0; i < GRAPH_HISTORY_COUNT; i++) {
            float v = fps->values[(fps->head + i) % GRAPH_HISTORY_COUNT] * 1000.0f;
            float vx, vy;
            if (v > 20.0f) v = 20.0f;
            vx = x + ((float)i / (GRAPH_HISTORY_COUNT - 1)) * w;
            vy = y + h - ((v / 20.0f) * h);
            ME_surface_LineTo(surface, vx, vy);
        }
    }
    ME_surface_LineTo(surface, x + w, y + h);
    ME_surface_FillColor(surface, ME_surface_RGBA(255, 192, 0, 128));
    ME_surface_Fill(surface);

    ME_surface_FontFace(surface, "fusion-pixel");

    if (fps->name[0] != '\0') {
        ME_surface_FontSize(surface, 12.0f);
        ME_surface_TextAlign(surface, ME_SURFACE_ALIGN_LEFT | ME_SURFACE_ALIGN_TOP);
        ME_surface_FillColor(surface, ME_surface_RGBA(240, 240, 240, 192));
        ME_surface_Text(surface, x + 3, y + 3, fps->name, NULL);
    }

    if (fps->style == GRAPH_RENDER_FPS) {
        ME_surface_FontSize(surface, 15.0f);
        ME_surface_TextAlign(surface, ME_SURFACE_ALIGN_RIGHT | ME_SURFACE_ALIGN_TOP);
        ME_surface_FillColor(surface, ME_surface_RGBA(240, 240, 240, 255));
        sprintf(str, "%.2f FPS", 1.0f / avg);
        ME_surface_Text(surface, x + w - 3, y + 3, str, NULL);

        ME_surface_FontSize(surface, 13.0f);
        ME_surface_TextAlign(surface, ME_SURFACE_ALIGN_RIGHT | ME_SURFACE_ALIGN_BASELINE);
        ME_surface_FillColor(surface, ME_surface_RGBA(240, 240, 240, 160));
        sprintf(str, "%.2f ms", avg * 1000.0f);
        ME_surface_Text(surface, x + w - 3, y + h - 3, str, NULL);
    } else if (fps->style == GRAPH_RENDER_PERCENT) {
        ME_surface_FontSize(surface, 15.0f);
        ME_surface_TextAlign(surface, ME_SURFACE_ALIGN_RIGHT | ME_SURFACE_ALIGN_TOP);
        ME_surface_FillColor(surface, ME_surface_RGBA(240, 240, 240, 255));
        sprintf(str, "%.1f %%", avg * 1.0f);
        ME_surface_Text(surface, x + w - 3, y + 3, str, NULL);
    } else {
        ME_surface_FontSize(surface, 15.0f);
        ME_surface_TextAlign(surface, ME_SURFACE_ALIGN_RIGHT | ME_SURFACE_ALIGN_TOP);
        ME_surface_FillColor(surface, ME_surface_RGBA(240, 240, 240, 255));
        sprintf(str, "%.2f ms", avg * 1000.0f);
        ME_surface_Text(surface, x + w - 3, y + 3, str, NULL);
    }
}
