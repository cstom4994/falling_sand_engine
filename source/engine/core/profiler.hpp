
#ifndef ME_PROFILER_HPP
#define ME_PROFILER_HPP

#define ME_SCOPES_MAX (16 * 1024)
#define ME_TEXT_MAX (1024 * 1024)
#define ME_DRAW_THREADS_MAX (16)

#include <map>
#include <string>

#include "engine/core/basic_types.h"
#include "engine/core/core.hpp"
#include "engine/core/platform.h"
// #include "engine/ui/surface.h"

struct MEsurface_context;

// Structures describing profiling data
typedef struct profiler_scope_stats {
    u64 m_inclusiveTime;
    u64 m_exclusiveTime;
    u64 m_inclusiveTimeTotal;
    u64 m_exclusiveTimeTotal;
    u32 m_occurences;

} profiler_scope_stats;

typedef struct profiler_scope_t {
    u64 m_start;
    u64 m_end;
    u64 m_threadID;
    const char *m_name;
    const char *m_file;
    u32 m_line;
    u32 m_level;
    profiler_scope_stats *m_stats;

} profiler_scope;

typedef struct profiler_thread {
    u64 m_threadID;
    const char *m_name;

} profiler_thread;

typedef struct profiler_frame_t {
    u32 m_numScopes;
    profiler_scope *m_scopes;
    u32 m_numThreads;
    profiler_thread *m_threads;
    u64 m_startTime;
    u64 m_endtime;
    u64 m_prevFrameTime;
    u64 m_CPUFrequency;
    f32 m_timeThreshold;
    u32 m_levelThreshold;
    u32 m_numScopesStats;
    profiler_scope *m_scopesStats;
    profiler_scope_stats *m_scopeStatsInfo;

} profiler_frame;

// Initialize profiling library.
void ME_profiler_init();

// Shutdown profiling library and release all resources.
void ME_profiler_shutdown();

// Sets the minimum time (in ms) to trigger a capture call back.
//_ms    - time in ms to use as minimum, if set to 0 (defauult) then every frame triggers a call back
//_level - scope depth in which to look for scopes longer than _ms threshold, 0 is for entire frame
void ME_profiler_set_threshold(f32 _ms, int _level = 0);

// Registers thread name.
//_name     - name to use for this thread
//_threadID - ID of thread to register, 0 for current thread.
void ME_profiler_register_thread(const char *_name, u64 _threadID = 0);

// Unregisters thread name and releases name string.
//_threadID - thread ID
void ME_profiler_unregister_thread(u64 _threadID);

// Must be called once per frame at the frame start
void ME_profiler_begin_frame();

// Begins a profiling scope/block.
//_file - name of source file
//_line - line of source file
//_name - name of the scope
// Returns: scope handle
uintptr_t ME_profiler_begin_scope(const char *_file, int _line, const char *_name);

// Stops a profiling scope/block.
//_scopeHandle  - handle of the scope to be closed
void ME_profiler_end_scope(uintptr_t _scopeHandle);

// Returns non zero value if profiling is paused.
int ME_profiler_is_paused();

// Returns non zero value if the last captured frame has crossed threshold.
int ME_profiler_was_threshold_crossed();

// Pauses profiling if the value passed is 0, otherwise resumes profiling.
void ME_profiler_set_paused(int _paused);

// Fetches data of the last saved frame (either threshold exceeded or profiling is paused).
void ME_profiler_get_frame(profiler_frame *_data);

// Saves profiler data to a binary buffer.
//_data       - profiler data / single frame capture
//_buffer     - buffer to store data to
//_bufferSize - maximum size of buffer, in bytes
// Returns: number of bytes written to buffer. 0 for failure.
int ME_profiler_save(profiler_frame *_data, void *_buffer, size_t _bufferSize);

// Loads a single frame capture from a binary buffer.
//_data       - [in/out] profiler data / single frame capture. User is responsible to release memory using ProfilerRelease.
//_buffer     - buffer to store data to
//_bufferSize - maximum size of buffer, in bytes
void ME_profiler_load(profiler_frame *_data, void *_buffer, size_t _bufferSize);

// Loads a only time in miliseconds for a single frame capture from a binary buffer.
//_time       - [in/out] frame timne in ms.
//_buffer     - buffer to store data to
//_bufferSize - maximum size of buffer, in bytes
void ME_profiler_load_time_only(f32 *_time, void *_buffer, size_t _bufferSize);

// Releases resources for a single frame capture. Only valid for data loaded with ProfilerLoad.
//_data       - data to be released
void ME_profiler_release(profiler_frame *_data);

// Returns CPU clock.
u64 ME_profiler_get_clock();

// Returns CPU frequency.
u64 profiler_get_clock_frequency();

// Calculates miliseconds from CPU clock.
f32 profiler_clock2ms(u64 _clock, u64 _frequency);

struct profiler_scoped {
    uintptr_t m_scope;

    profiler_scoped(const char *_file, int _line, const char *_name) { m_scope = ME_profiler_begin_scope(_file, _line, _name); }

    ~profiler_scoped() { ME_profiler_end_scope(m_scope); }
};

// Macro used to profile on a scope basis
#ifndef ME_DISABLE_PROFILING

#define ME_profiler_init() ME_profiler_init()
#define ME_profiler_scope_auto(x, ...) profiler_scoped ME_CONCAT(profileScope, __LINE__)(__FILE__, __LINE__, x)
#define ME_profiler_scope_begin(n, ...) \
    uintptr_t profileid_##n;            \
    profileid_##n = ProfilerBeginScope(__FILE__, __LINE__, #n);
#define ME_profiler_scope_end(n) ProfilerEndScope(profileid_##n);
#define ME_profiler_begin() ME_profiler_begin_frame()
#define ME_profiler_thread(n) ME_profiler_register_thread(n)
#define ME_profiler_shutdown() ME_profiler_shutdown()
#else
#define ME_profiler_init() void()
#define ME_profiler_begin() void()
#define ME_profiler_thread(n) void()
#define ME_profiler_shutdown() void()
#endif  // ME_DISABLE_PROFILING

struct profiler_free_list_t {
    u32 m_maxBlocks;
    u32 m_blockSize;
    u32 m_blocksFree;
    u32 m_blocksAlllocated;
    u8 *m_buffer;
    u8 *m_next;
};

void ME_profiler_free_list_create(size_t _blockSize, u32 _maxBlocks, profiler_free_list_t *_freeList);
void ME_profiler_free_list_destroy(profiler_free_list_t *_freeList);
void *ME_profiler_free_list_alloc(profiler_free_list_t *_freeList);
void ME_profiler_free_list_free(profiler_free_list_t *_freeList, void *_ptr);
int ME_profiler_free_list_check_ptr(profiler_free_list_t *_freeList, void *_ptr);

namespace ME::profiler {

class profiler_context {
    enum buffer_use {
        Capture,
        Display,
        Open,

        Count
    };

    ME::pthread_mutex m_mutex;
    profiler_free_list_t m_scopesAllocator;
    u32 m_scopesOpen;
    profiler_scope *m_scopesCapture[ME_SCOPES_MAX];
    profiler_scope m_scopesDisplay[ME_SCOPES_MAX];
    u32 m_displayScopes;
    u64 m_frameStartTime;
    u64 m_frameEndTime;
    bool m_thresholdCrossed;
    f32 m_timeThreshold;
    u32 m_levelThreshold;
    bool m_pauseProfiling;
    char m_namesDataBuffers[buffer_use::Count][ME_TEXT_MAX];
    char *m_namesData[buffer_use::Count];
    int m_namesSize[buffer_use::Count];
    u32 m_tlsLevel;

    std::map<u64, std::string> m_threadNames;

public:
    profiler_context();
    ~profiler_context();

    void set_threshold(f32 _ms, int _levelThreshold);
    bool is_paused();
    bool was_threshold_crossed();
    void set_paused(bool _paused);
    void register_thread(u64 _threadID, const char *_name);
    void unregister_thread(u64 _threadID);
    void begin_frame();
    int inc_level();
    void dec_level();
    profiler_scope *begin_ccope(const char *_file, int _line, const char *_name);
    void end_scope(profiler_scope *_scope);
    const char *add_string(const char *_name, buffer_use _buffer);
    void get_frame_data(profiler_frame *_data);
};

}  // namespace ME::profiler

#define GRAPH_HISTORY_COUNT 100
#define GPU_QUERY_COUNT 5

enum GraphrenderStyle {
    GRAPH_RENDER_FPS,
    GRAPH_RENDER_MS,
    GRAPH_RENDER_PERCENT,
};

struct profiler_graph {
    int style;
    char name[32];
    float values[GRAPH_HISTORY_COUNT];
    int head;
};
typedef struct profiler_graph profiler_graph;

void ME_profiler_graph_init(profiler_graph *fps, int style, const char *name);
void ME_profiler_graph_update(profiler_graph *fps, float frameTime);
void ME_profiler_graph_render(MEsurface_context *surface, float x, float y, profiler_graph *fps);
float ME_profiler_graph_avg(profiler_graph *fps);

#endif