
#ifndef METADOT_PROFILER_H
#define METADOT_PROFILER_H

#include <cstdlib>
#include <map>
#include <string>

#define METADOT_SCOPES_MAX (16 * 1024)
#define METADOT_TEXT_MAX (1024 * 1024)
#define METADOT_DRAW_THREADS_MAX (16)

#include "core/platform.h"
#include "profiler.h"

typedef struct ProfilerFreeList_t {
    uint32_t m_maxBlocks;
    uint32_t m_blockSize;
    uint32_t m_blocksFree;
    uint32_t m_blocksAlllocated;
    uint8_t* m_buffer;
    uint8_t* m_next;

} ProfilerFreeList_t;

void ProfilerFreeListCreate(size_t _blockSize, uint32_t _maxBlocks, struct ProfilerFreeList_t* _freeList);
void ProfilerFreeListDestroy(struct ProfilerFreeList_t* _freeList);
void* ProfilerFreeListAlloc(struct ProfilerFreeList_t* _freeList);
void ProfilerFreeListFree(struct ProfilerFreeList_t* _freeList, void* _ptr);
int ProfilerFreeListCheckPtr(struct ProfilerFreeList_t* _freeList, void* _ptr);

namespace MetaEngine::Profiler {

class ProfilerContext {
    enum BufferUse {
        Capture,
        Display,
        Open,

        Count
    };

    MetaEngine::pthread_Mutex m_mutex;
    ProfilerFreeList_t m_scopesAllocator;
    uint32_t m_scopesOpen;
    ProfilerScope* m_scopesCapture[METADOT_SCOPES_MAX];
    ProfilerScope m_scopesDisplay[METADOT_SCOPES_MAX];
    uint32_t m_displayScopes;
    uint64_t m_frameStartTime;
    uint64_t m_frameEndTime;
    bool m_thresholdCrossed;
    float m_timeThreshold;
    uint32_t m_levelThreshold;
    bool m_pauseProfiling;
    char m_namesDataBuffers[BufferUse::Count][METADOT_TEXT_MAX];
    char* m_namesData[BufferUse::Count];
    int m_namesSize[BufferUse::Count];
    uint32_t m_tlsLevel;

    std::map<uint64_t, std::string> m_threadNames;

public:
    ProfilerContext();
    ~ProfilerContext();

    void setThreshold(float _ms, int _levelThreshold);
    bool isPaused();
    bool wasThresholdCrossed();
    void setPaused(bool _paused);
    void registerThread(uint64_t _threadID, const char* _name);
    void unregisterThread(uint64_t _threadID);
    void beginFrame();
    int incLevel();
    void decLevel();
    ProfilerScope* beginScope(const char* _file, int _line, const char* _name);
    void endScope(ProfilerScope* _scope);
    const char* addString(const char* _name, BufferUse _buffer);
    void getFrameData(ProfilerFrame* _data);
};

}  // namespace MetaEngine::Profiler

#endif  // METADOT_LIB_H
