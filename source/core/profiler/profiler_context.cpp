

#include "profiler_context.h"

#include <cstdlib>
#include <unordered_map>
#include <vector>

#include "libs/lz4/lz4.h"
#include "profiler.h"
#include "profiler_context.h"

extern "C" uint64_t ProfilerGetClockFrequency();

/*--------------------------------------------------------------------------
 * Data load/save functions
 *------------------------------------------------------------------------*/

template <typename T>
static inline void writeVar(uint8_t*& _buffer, T _var) {
    memcpy(_buffer, &_var, sizeof(T));
    _buffer += sizeof(T);
}

static inline void writeStr(uint8_t*& _buffer, const char* _str) {
    uint32_t len = (uint32_t)strlen(_str);
    writeVar(_buffer, len);
    memcpy(_buffer, _str, len);
    _buffer += len;
}

template <typename T>
static inline void readVar(uint8_t*& _buffer, T& _var) {
    memcpy(&_var, _buffer, sizeof(T));
    _buffer += sizeof(T);
}

static inline char* readString(uint8_t*& _buffer) {
    uint32_t len;
    readVar(_buffer, len);
    char* str = new char[len + 1];
    memcpy(str, _buffer, len);
    str[len] = 0;
    _buffer += len;
    return str;
}

const char* duplicateString(const char* _str) {
    char* str = new char[strlen(_str) + 1];
    strcpy(str, _str);
    return str;
}

struct StringStore {
    typedef std::unordered_map<std::string, uint32_t> StringToIndexType;
    typedef std::unordered_map<uint32_t, std::string> IndexToStringType;

    uint32_t m_totalSize;
    StringToIndexType m_stringIndexMap;
    IndexToStringType m_strings;

    StringStore() : m_totalSize(0) {}

    void addString(const char* _str) {
        StringToIndexType::iterator it = m_stringIndexMap.find(_str);
        if (it == m_stringIndexMap.end()) {
            uint32_t index = (uint32_t)m_stringIndexMap.size();
            m_totalSize += 4 + (uint32_t)strlen(_str);  // see writeStr for details
            m_stringIndexMap[_str] = index;
            m_strings[index] = _str;
        }
    }

    uint32_t getString(const char* _str) { return m_stringIndexMap[_str]; }
};

/*--------------------------------------------------------------------------
 * API functions
 *------------------------------------------------------------------------*/

MetaEngine::Profiler::ProfilerContext* g_context = 0;

extern "C" {

void ProfilerInit() { g_context = new MetaEngine::Profiler::ProfilerContext(); }

void ProfilerShutDown() {
    delete g_context;
    g_context = 0;
}

void ProfilerSetThreshold(float _ms, int _level) { g_context->setThreshold(_ms, _level); }

void ProfilerRegisterThread(const char* _name, uint64_t _threadID) {
    if (_threadID == 0) _threadID = getThreadID();

    g_context->registerThread(_threadID, _name);
}

void ProfilerUnregisterThread(uint64_t _threadID) { g_context->unregisterThread(_threadID); }

void ProfilerBeginFrame() { g_context->beginFrame(); }

uintptr_t ProfilerBeginScope(const char* _file, int _line, const char* _name) { return (uintptr_t)g_context->beginScope(_file, _line, _name); }

void ProfilerEndScope(uintptr_t _scopeHandle) { g_context->endScope((ProfilerScope*)_scopeHandle); }

int ProfilerIsPaused() { return g_context->isPaused() ? 1 : 0; }

int ProfilerWasThresholdCrossed() { return g_context->wasThresholdCrossed() ? 1 : 0; }

void ProfilerSetPaused(int _paused) { return g_context->setPaused(_paused != 0); }

void ProfilerGetFrame(ProfilerFrame* _data) {
    g_context->getFrameData(_data);

    // clamp scopes crossing frame boundary
    const uint32_t numScopes = _data->m_numScopes;
    for (uint32_t i = 0; i < numScopes; ++i) {
        ProfilerScope& cs = _data->m_scopes[i];

        if (cs.m_start == cs.m_end) {
            cs.m_end = _data->m_endtime;
            if (cs.m_start < _data->m_startTime) cs.m_start = _data->m_startTime;
        }
    }
}

int ProfilerSave(ProfilerFrame* _data, void* _buffer, size_t _bufferSize) {
    // fill string data
    StringStore strStore;
    for (uint32_t i = 0; i < _data->m_numScopes; ++i) {
        ProfilerScope& scope = _data->m_scopes[i];
        strStore.addString(scope.m_name);
        strStore.addString(scope.m_file);
    }
    for (uint32_t i = 0; i < _data->m_numThreads; ++i) {
        strStore.addString(_data->m_threads[i].m_name);
    }

    // calc data size
    uint32_t totalSize = _data->m_numScopes * sizeof(ProfilerScope) + _data->m_numThreads * sizeof(ProfilerThread) + sizeof(ProfilerFrame) + strStore.m_totalSize;

    uint8_t* buffer = new uint8_t[totalSize];
    uint8_t* bufPtr = buffer;

    writeVar(buffer, _data->m_startTime);
    writeVar(buffer, _data->m_endtime);
    writeVar(buffer, _data->m_prevFrameTime);
    // writeVar(buffer, _data->m_platformID);
    writeVar(buffer, ProfilerGetClockFrequency());

    // write scopes
    writeVar(buffer, _data->m_numScopes);
    for (uint32_t i = 0; i < _data->m_numScopes; ++i) {
        ProfilerScope& scope = _data->m_scopes[i];
        writeVar(buffer, scope.m_start);
        writeVar(buffer, scope.m_end);
        writeVar(buffer, scope.m_threadID);
        writeVar(buffer, strStore.getString(scope.m_name));
        writeVar(buffer, strStore.getString(scope.m_file));
        writeVar(buffer, scope.m_line);
        writeVar(buffer, scope.m_level);
    }

    // write thread info
    writeVar(buffer, _data->m_numThreads);
    for (uint32_t i = 0; i < _data->m_numThreads; ++i) {
        ProfilerThread& t = _data->m_threads[i];
        writeVar(buffer, t.m_threadID);
        writeVar(buffer, strStore.getString(t.m_name));
    }

    // write string data
    uint32_t numStrings = (uint32_t)strStore.m_strings.size();
    writeVar(buffer, numStrings);

    for (uint32_t i = 0; i < strStore.m_strings.size(); ++i) writeStr(buffer, strStore.m_strings[i].c_str());

    int compSize = LZ4_compress_default((const char*)bufPtr, (char*)_buffer, (int)(buffer - bufPtr), (int)_bufferSize);
    delete[] bufPtr;
    return compSize;
}

void ProfilerLoad(ProfilerFrame* _data, void* _buffer, size_t _bufferSize) {
    size_t bufferSize = _bufferSize;
    uint8_t* buffer = 0;
    uint8_t* bufferPtr;

    int decomp = -1;
    do {
        delete[] buffer;
        bufferSize *= 2;
        buffer = new uint8_t[bufferSize];
        decomp = LZ4_decompress_safe((const char*)_buffer, (char*)buffer, (int)_bufferSize, (int)bufferSize);

    } while (decomp < 0);

    bufferPtr = buffer;

    uint32_t strIdx;

    readVar(buffer, _data->m_startTime);
    readVar(buffer, _data->m_endtime);
    readVar(buffer, _data->m_prevFrameTime);
    // readVar(buffer, _data->m_platformID);
    readVar(buffer, _data->m_CPUFrequency);

    // read scopes
    readVar(buffer, _data->m_numScopes);

    _data->m_scopes = new ProfilerScope[_data->m_numScopes * 2];  // extra space for viewer - m_scopesStats
    _data->m_scopesStats = &_data->m_scopes[_data->m_numScopes];
    _data->m_scopeStatsInfo = new ProfilerScopeStats[_data->m_numScopes * 2];

    for (uint32_t i = 0; i < _data->m_numScopes * 2; ++i) _data->m_scopes[i].m_stats = &_data->m_scopeStatsInfo[i];

    for (uint32_t i = 0; i < _data->m_numScopes; ++i) {
        ProfilerScope& scope = _data->m_scopes[i];
        readVar(buffer, scope.m_start);
        readVar(buffer, scope.m_end);
        readVar(buffer, scope.m_threadID);
        readVar(buffer, strIdx);
        scope.m_name = (const char*)(uintptr_t)strIdx;
        readVar(buffer, strIdx);
        scope.m_file = (const char*)(uintptr_t)strIdx;
        readVar(buffer, scope.m_line);
        readVar(buffer, scope.m_level);

        scope.m_stats->m_inclusiveTime = scope.m_end - scope.m_start;
        scope.m_stats->m_exclusiveTime = scope.m_stats->m_inclusiveTime;
        scope.m_stats->m_occurences = 0;
    }

    // read thread info
    readVar(buffer, _data->m_numThreads);
    _data->m_threads = new ProfilerThread[_data->m_numThreads];
    for (uint32_t i = 0; i < _data->m_numThreads; ++i) {
        ProfilerThread& t = _data->m_threads[i];
        readVar(buffer, t.m_threadID);
        readVar(buffer, strIdx);
        t.m_name = (const char*)(uintptr_t)strIdx;
    }

    // read string data
    uint32_t numStrings;
    readVar(buffer, numStrings);

    const char* strings[METADOT_SCOPES_MAX];
    for (uint32_t i = 0; i < numStrings; ++i) strings[i] = readString(buffer);

    for (uint32_t i = 0; i < _data->m_numScopes; ++i) {
        ProfilerScope& scope = _data->m_scopes[i];
        uintptr_t idx = (uintptr_t)scope.m_name;
        scope.m_name = duplicateString(strings[(uint32_t)idx]);

        idx = (uintptr_t)scope.m_file;
        scope.m_file = duplicateString(strings[(uint32_t)idx]);
    }

    for (uint32_t i = 0; i < _data->m_numThreads; ++i) {
        ProfilerThread& t = _data->m_threads[i];
        uintptr_t idx = (uintptr_t)t.m_name;
        t.m_name = duplicateString(strings[(uint32_t)idx]);
    }

    for (uint32_t i = 0; i < numStrings; ++i) delete[] strings[i];

    delete[] bufferPtr;

    // process frame data

    for (uint32_t i = 0; i < _data->m_numScopes; ++i)
        for (uint32_t j = 0; j < _data->m_numScopes; ++j) {
            ProfilerScope& scopeI = _data->m_scopes[i];
            ProfilerScope& scopeJ = _data->m_scopes[j];

            if ((scopeJ.m_start > scopeI.m_start) && (scopeJ.m_end < scopeI.m_end) && (scopeJ.m_level == scopeI.m_level + 1) && (scopeJ.m_threadID == scopeI.m_threadID))
                scopeI.m_stats->m_exclusiveTime -= scopeJ.m_stats->m_inclusiveTime;
        }

    _data->m_numScopesStats = 0;

    for (uint32_t i = 0; i < _data->m_numScopes; ++i) {
        ProfilerScope& scopeI = _data->m_scopes[i];

        scopeI.m_stats->m_inclusiveTimeTotal = scopeI.m_stats->m_inclusiveTime;
        scopeI.m_stats->m_exclusiveTimeTotal = scopeI.m_stats->m_exclusiveTime;

        int foundIndex = -1;
        for (uint32_t j = 0; j < _data->m_numScopesStats; ++j) {
            ProfilerScope& scopeJ = _data->m_scopesStats[j];
            if (strcmp(scopeI.m_name, scopeJ.m_name) == 0) {
                foundIndex = j;
                break;
            }
        }

        if (foundIndex == -1) {
            int index = _data->m_numScopesStats++;
            ProfilerScope& scope = _data->m_scopesStats[index];
            scope = scopeI;
            scope.m_stats->m_occurences = 1;
        } else {
            ProfilerScope& scope = _data->m_scopesStats[foundIndex];
            scope.m_stats->m_inclusiveTimeTotal += scopeI.m_stats->m_inclusiveTime;
            scope.m_stats->m_exclusiveTimeTotal += scopeI.m_stats->m_exclusiveTime;
            scope.m_stats->m_occurences++;
        }
    }
}

void ProfilerLoadTimeOnly(float* _time, void* _buffer, size_t _bufferSize) {
    size_t bufferSize = _bufferSize;
    uint8_t* buffer = 0;
    uint8_t* bufferPtr;

    int decomp = -1;
    do {
        delete[] buffer;
        bufferSize *= 2;
        buffer = new uint8_t[bufferSize];
        decomp = LZ4_decompress_safe((const char*)_buffer, (char*)buffer, (int)_bufferSize, (int)bufferSize);

    } while (decomp < 0);

    bufferPtr = buffer;

    uint64_t startTime;
    uint64_t endtime;
    uint8_t dummy8;
    uint64_t frequency;

    readVar(buffer, startTime);
    readVar(buffer, endtime);
    readVar(buffer, frequency);  // dummy
    readVar(buffer, dummy8);     // dummy
    readVar(buffer, frequency);
    *_time = ProfilerClock2ms(endtime - startTime, frequency);

    delete[] buffer;
}

void ProfilerRelease(ProfilerFrame* _data) {
    for (uint32_t i = 0; i < _data->m_numScopes; ++i) {
        ProfilerScope& scope = _data->m_scopes[i];
        delete[] scope.m_name;
        delete[] scope.m_file;
    }

    for (uint32_t i = 0; i < _data->m_numThreads; ++i) {
        ProfilerThread& t = _data->m_threads[i];
        delete[] t.m_name;
    }

    delete[] _data->m_scopes;
    delete[] _data->m_threads;
    delete[] _data->m_scopeStatsInfo;
}

uint64_t ProfilerGetClock() {
#if METADOT_PLATFORM_WINDOWS
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
    uint64_t q = __rdtsc();
#else
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    int64_t q = li.QuadPart;
#endif
#elif METADOT_PLATFORM_XBOXONE
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    int64_t q = li.QuadPart;
#elif METADOT_PLATFORM_PS4
    int64_t q = sceKernelReadTsc();
#elif METADOT_PLATFORM_ANDROID
    int64_t q = ::clock();
#elif METADOT_PLATFORM_EMSCRIPTEN
    int64_t q = (int64_t)(emscripten_get_now() * 1000.0);
#elif METADOT_PLATFORM_SWITCH
    int64_t q = nn::os::GetSystemTick().GetInt64Value();
#else
    struct timeval now;
    gettimeofday(&now, 0);
    int64_t q = now.tv_sec * 1000000 + now.tv_usec;
#endif
    return q;
}

uint64_t ProfilerGetClockFrequency() {
#if METADOT_PLATFORM_WINDOWS
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
    static uint64_t frequency = 1;
    static bool initialized = false;
    if (!initialized) {
        LARGE_INTEGER li1, li2;
        QueryPerformanceCounter(&li1);
        uint64_t tsc1 = __rdtsc();
        for (int i = 0; i < 230000000; ++i)
            ;
        uint64_t tsc2 = __rdtsc();
        QueryPerformanceCounter(&li2);

        LARGE_INTEGER lif;
        QueryPerformanceFrequency(&lif);
        uint64_t time = ((li2.QuadPart - li1.QuadPart) * 1000) / lif.QuadPart;
        frequency = (uint64_t)(1000 * ((tsc2 - tsc1) / time));
        initialized = true;
    }
    return frequency;
#else
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return li.QuadPart;
#endif
#elif METADOT_PLATFORM_XBOXONE
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return li.QuadPart;
#elif METADOT_PLATFORM_ANDROID
    return CLOCKS_PER_SEC;
#elif METADOT_PLATFORM_PS4
    return sceKernelGetTscFrequency();
#elif METADOT_PLATFORM_SWITCH
    return nn::os::GetSystemTickFrequency();
#else
    return 1000000;
#endif
}

float ProfilerClock2ms(uint64_t _clock, uint64_t _frequency) { return (float(_clock) / float(_frequency)) * 1000.0f; }

}  // extern "C"

void ProfilerFreeListCreate(size_t _blockSize, uint32_t _maxBlocks, struct ProfilerFreeList_t* _freeList) {
    _freeList->m_maxBlocks = _maxBlocks;
    _freeList->m_blockSize = (uint32_t)_blockSize;
    _freeList->m_blocksFree = _maxBlocks;
    _freeList->m_blocksAlllocated = 0;
    _freeList->m_buffer = (uint8_t*)malloc(_blockSize * _maxBlocks);
    _freeList->m_next = _freeList->m_buffer;
}

void ProfilerFreeListDestroy(struct ProfilerFreeList_t* _freeList) { free(_freeList->m_buffer); }

void* ProfilerFreeListAlloc(struct ProfilerFreeList_t* _freeList) {
    if (_freeList->m_blocksAlllocated < _freeList->m_maxBlocks) {
        uint32_t* p = (uint32_t*)(_freeList->m_buffer + (_freeList->m_blocksAlllocated * _freeList->m_blockSize));
        *p = _freeList->m_blocksAlllocated + 1;
        _freeList->m_blocksAlllocated++;
    }

    void* ret = 0;
    if (_freeList->m_blocksFree) {
        ret = _freeList->m_next;
        --_freeList->m_blocksFree;
        if (_freeList->m_blocksFree)
            _freeList->m_next = _freeList->m_buffer + (*(uint32_t*)_freeList->m_next * _freeList->m_blockSize);
        else
            _freeList->m_next = 0;
    }
    return ret;
}

void ProfilerFreeListFree(struct ProfilerFreeList_t* _freeList, void* _ptr) {
    if (_freeList->m_next) {
        uint32_t index = ((uint32_t)(_freeList->m_next - _freeList->m_buffer)) / _freeList->m_blockSize;
        *(uint32_t*)_ptr = index;
        _freeList->m_next = (uint8_t*)_ptr;
    } else {
        *(uint32_t*)_ptr = _freeList->m_maxBlocks;
        _freeList->m_next = (uint8_t*)_ptr;
    }
    ++_freeList->m_blocksFree;
}

int ProfilerFreeListCheckPtr(struct ProfilerFreeList_t* _freeList, void* _ptr) {
    return ((uintptr_t)_freeList->m_maxBlocks * (uintptr_t)_freeList->m_blockSize) > (uintptr_t)(((uint8_t*)_ptr) - _freeList->m_buffer) ? 1 : 0;
}

extern "C" uint64_t ProfilerGetClockFrequency();

namespace MetaEngine::Profiler {

ProfilerContext::ProfilerContext()
    : m_scopesOpen(0), m_displayScopes(0), m_frameStartTime(0), m_frameEndTime(0), m_thresholdCrossed(false), m_timeThreshold(0.0f), m_levelThreshold(0), m_pauseProfiling(false) {

    m_tlsLevel = tlsAllocate();

    ProfilerFreeListCreate(sizeof(ProfilerScope), METADOT_SCOPES_MAX, &m_scopesAllocator);

    for (int i = 0; i < BufferUse::Count; ++i) {
        m_namesSize[i] = 0;
        m_namesData[i] = m_namesDataBuffers[i];
    }
}

ProfilerContext::~ProfilerContext() {
    ProfilerFreeListDestroy(&m_scopesAllocator);
    tlsFree(m_tlsLevel);
}

void ProfilerContext::setThreshold(float _ms, int _levelThreshold) {
    m_timeThreshold = _ms;
    m_levelThreshold = _levelThreshold;
}

bool ProfilerContext::isPaused() { return m_pauseProfiling; }

bool ProfilerContext::wasThresholdCrossed() { return !m_pauseProfiling && m_thresholdCrossed; }

void ProfilerContext::setPaused(bool _paused) { m_pauseProfiling = _paused; }

void ProfilerContext::registerThread(uint64_t _threadID, const char* _name) { m_threadNames[_threadID] = _name; }

void ProfilerContext::unregisterThread(uint64_t _threadID) { m_threadNames.erase(_threadID); }

void ProfilerContext::beginFrame() {
    MetaEngine::ScopedMutexLocker lock(m_mutex);

    uint64_t frameBeginTime, frameEndTime;
    static uint64_t beginPrevFrameTime = ProfilerGetClock();
    frameBeginTime = beginPrevFrameTime;
    frameEndTime = ProfilerGetClock();
    beginPrevFrameTime = frameEndTime;

    static uint64_t frameTime = frameEndTime - frameBeginTime;

    m_thresholdCrossed = false;

    int level = (int)m_levelThreshold - 1;

    uint32_t scopesToRestart = 0;

    m_namesSize[BufferUse::Open] = 0;

    static ProfilerScope scopesDisplay[METADOT_SCOPES_MAX];
    for (uint32_t i = 0; i < m_scopesOpen; ++i) {
        ProfilerScope* scope = m_scopesCapture[i];

        if (scope->m_start == scope->m_end) scope->m_name = addString(scope->m_name, BufferUse::Open);

        scopesDisplay[i] = *scope;

        // scope that was not closed, spans frame boundary
        // keep it for next frame
        if (scope->m_start == scope->m_end)
            m_scopesCapture[scopesToRestart++] = scope;
        else {
            ProfilerFreeListFree(&m_scopesAllocator, scope);
            scope = &scopesDisplay[i];
        }

        // did scope cross threshold?
        if (level == (int)scope->m_level) {
            uint64_t scopeEnd = scope->m_end;
            if (scope->m_start == scope->m_end) scopeEnd = frameEndTime;

            if (m_timeThreshold <= ProfilerClock2ms(scopeEnd - scope->m_start, ProfilerGetClockFrequency())) m_thresholdCrossed = true;
        }
    }

    // did frame cross threshold ?
    float prevFrameTime = ProfilerClock2ms(frameEndTime - frameBeginTime, ProfilerGetClockFrequency());
    if ((level == -1) && (m_timeThreshold <= prevFrameTime)) m_thresholdCrossed = true;

    if (m_thresholdCrossed && !m_pauseProfiling) {
        std::swap(m_namesData[BufferUse::Capture], m_namesData[BufferUse::Display]);

        memcpy(m_scopesDisplay, scopesDisplay, sizeof(ProfilerScope) * m_scopesOpen);

        m_displayScopes = m_scopesOpen;
        m_frameStartTime = frameBeginTime;
        m_frameEndTime = frameEndTime;
    }

    m_namesSize[BufferUse::Capture] = 0;
    for (uint32_t i = 0; i < scopesToRestart; ++i) m_scopesCapture[i]->m_name = addString(m_scopesCapture[i]->m_name, BufferUse::Capture);

    m_scopesOpen = scopesToRestart;
    frameTime = frameEndTime - frameBeginTime;
}

int ProfilerContext::incLevel() {
    // may be a first call on this thread
    void* tl = tlsGetValue(m_tlsLevel);
    if (!tl) {
        // we'd like to start with -1 but then the ++ operator below
        // would result in NULL value for tls so we offset by 2
        tl = (void*)1;
        tlsSetValue(m_tlsLevel, tl);
    }
    intptr_t threadLevel = (intptr_t)tl - 1;
    tlsSetValue(m_tlsLevel, (void*)(threadLevel + 2));
    return (int)threadLevel;
}

void ProfilerContext::decLevel() {
    intptr_t threadLevel = (intptr_t)tlsGetValue(m_tlsLevel);
    --threadLevel;
    tlsSetValue(m_tlsLevel, (void*)threadLevel);
}

ProfilerScope* ProfilerContext::beginScope(const char* _file, int _line, const char* _name) {
    ProfilerScope* scope = 0;
    {
        MetaEngine::ScopedMutexLocker lock(m_mutex);
        if (m_scopesOpen == METADOT_SCOPES_MAX) return 0;

        scope = (ProfilerScope*)ProfilerFreeListAlloc(&m_scopesAllocator);
        m_scopesCapture[m_scopesOpen++] = scope;

        scope->m_name = addString(_name, BufferUse::Capture);
        scope->m_start = ProfilerGetClock();
        scope->m_end = scope->m_start;
    }

    scope->m_threadID = getThreadID();
    scope->m_file = _file;
    scope->m_line = _line;
    scope->m_level = incLevel();

    return scope;
}

void ProfilerContext::endScope(ProfilerScope* _scope) {
    if (!_scope) return;

    _scope->m_end = ProfilerGetClock();
    decLevel();
}

const char* ProfilerContext::addString(const char* _name, BufferUse _buffer) {
    char* nameData = m_namesData[_buffer];
    int& nameSize = m_namesSize[_buffer];

    char c, *ret = &nameData[nameSize];
    while ((c = *_name++) && (nameSize < METADOT_TEXT_MAX)) nameData[nameSize++] = c;

    if (nameSize < METADOT_TEXT_MAX)
        nameData[nameSize++] = 0;
    else
        nameData[METADOT_TEXT_MAX - 1] = 0;

    return ret;
}

void ProfilerContext::getFrameData(ProfilerFrame* _data) {
    MetaEngine::ScopedMutexLocker lock(m_mutex);

    static ProfilerThread threadData[METADOT_DRAW_THREADS_MAX];

    uint32_t numThreads = (uint32_t)m_threadNames.size();
    if (numThreads > METADOT_DRAW_THREADS_MAX) numThreads = METADOT_DRAW_THREADS_MAX;

    _data->m_numScopes = m_displayScopes;
    _data->m_scopes = m_scopesDisplay;
    _data->m_numThreads = numThreads;
    _data->m_threads = threadData;
    _data->m_startTime = m_frameStartTime;
    _data->m_endtime = m_frameEndTime;
    _data->m_prevFrameTime = m_frameEndTime - m_frameStartTime;
    _data->m_CPUFrequency = ProfilerGetClockFrequency();
    _data->m_timeThreshold = m_timeThreshold;
    _data->m_levelThreshold = m_levelThreshold;

    std::map<uint64_t, std::string>::iterator it = m_threadNames.begin();
    for (uint32_t i = 0; i < numThreads; ++i) {
        threadData[i].m_threadID = it->first;
        threadData[i].m_name = it->second.c_str();
        ++it;
    }
}

}  // namespace MetaEngine::Profiler
