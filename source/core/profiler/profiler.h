
#ifndef METADOT_METADOT_H
#define METADOT_METADOT_H

#include <stddef.h> /* size_t  */
#include <stdint.h> /* uint*_t */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*--------------------------------------------------------------------------
 * Structures describing profiling data
 *------------------------------------------------------------------------*/
typedef struct ProfilerScopeStats {
    uint64_t m_inclusiveTime;
    uint64_t m_exclusiveTime;
    uint64_t m_inclusiveTimeTotal;
    uint64_t m_exclusiveTimeTotal;
    uint32_t m_occurences;

} ProfilerScopeStats;

typedef struct ProfilerScope {
    uint64_t m_start;
    uint64_t m_end;
    uint64_t m_threadID;
    const char* m_name;
    const char* m_file;
    uint32_t m_line;
    uint32_t m_level;
    ProfilerScopeStats* m_stats;

} ProfilerScope;

typedef struct ProfilerThread {
    uint64_t m_threadID;
    const char* m_name;

} ProfilerThread;

typedef struct ProfilerFrame {
    uint32_t m_numScopes;
    ProfilerScope* m_scopes;
    uint32_t m_numThreads;
    ProfilerThread* m_threads;
    uint64_t m_startTime;
    uint64_t m_endtime;
    uint64_t m_prevFrameTime;
    uint64_t m_CPUFrequency;
    float m_timeThreshold;
    uint32_t m_levelThreshold;
    uint32_t m_numScopesStats;
    ProfilerScope* m_scopesStats;
    ProfilerScopeStats* m_scopeStatsInfo;

} ProfilerFrame;

/*--------------------------------------------------------------------------
 * API
 *------------------------------------------------------------------------*/

/* Initialize profiling library. */
void ProfilerInit();

/* Shut down profiling library and release all resources. */
void ProfilerShutDown();

/* Sets the minimum time (in ms) to trigger a capture call back. */
/* _ms    - time in ms to use as minimum, if set to 0 (defauult) then every frame triggers a call back */
/* _level - scope depth in which to look for scopes longer than _ms threshold, 0 is for entire frame */
void ProfilerSetThreshold(float _ms, int _level = 0);

/* Registers thread name. */
/* _name     - name to use for this thread */
/* _threadID - ID of thread to register, 0 for current thread. */
void ProfilerRegisterThread(const char* _name, uint64_t _threadID = 0);

/* Unregisters thread name and releases name string. */
/* _threadID - thread ID */
void ProfilerUnregisterThread(uint64_t _threadID);

/* Must be called once per frame at the frame start */
void ProfilerBeginFrame();

/* Begins a profiling scope/block. */
/* _file - name of source file */
/* _line - line of source file */
/* _name - name of the scope */
/* Returns: scope handle */
uintptr_t ProfilerBeginScope(const char* _file, int _line, const char* _name);

/* Stops a profiling scope/block. */
/* _scopeHandle	- handle of the scope to be closed */
void ProfilerEndScope(uintptr_t _scopeHandle);

/* Returns non zero value if profiling is paused. */
int ProfilerIsPaused();

/* Returns non zero value if the last captured frame has crossed threshold. */
int ProfilerWasThresholdCrossed();

/* Pauses profiling if the value passed is 0, otherwise resumes profiling. */
void ProfilerSetPaused(int _paused);

/* Fetches data of the last saved frame (either threshold exceeded or profiling is paused). */
void ProfilerGetFrame(ProfilerFrame* _data);

/* Saves profiler data to a binary buffer. */
/* _data       - profiler data / single frame capture */
/* _buffer     - buffer to store data to */
/* _bufferSize - maximum size of buffer, in bytes */
/* Returns: number of bytes written to buffer. 0 for failure. */
int ProfilerSave(ProfilerFrame* _data, void* _buffer, size_t _bufferSize);

/* Loads a single frame capture from a binary buffer. */
/* _data       - [in/out] profiler data / single frame capture. User is responsible to release memory using ProfilerRelease. */
/* _buffer     - buffer to store data to */
/* _bufferSize - maximum size of buffer, in bytes */
void ProfilerLoad(ProfilerFrame* _data, void* _buffer, size_t _bufferSize);

/* Loads a only time in miliseconds for a single frame capture from a binary buffer. */
/* _time       - [in/out] frame timne in ms. */
/* _buffer     - buffer to store data to */
/* _bufferSize - maximum size of buffer, in bytes */
void ProfilerLoadTimeOnly(float* _time, void* _buffer, size_t _bufferSize);

/* Releases resources for a single frame capture. Only valid for data loaded with ProfilerLoad. */
/* _data       - data to be released */
void ProfilerRelease(ProfilerFrame* _data);

/* Returns CPU clock. */
uint64_t ProfilerGetClock();

/* Returns CPU frequency. */
uint64_t ProfilerGetClockFrequency();

/* Calculates miliseconds from CPU clock. */
float ProfilerClock2ms(uint64_t _clock, uint64_t _frequency);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#ifdef __cplusplus

struct ProfilerScoped {
    uintptr_t m_scope;

    ProfilerScoped(const char* _file, int _line, const char* _name) { m_scope = ProfilerBeginScope(_file, _line, _name); }

    ~ProfilerScoped() { ProfilerEndScope(m_scope); }
};

/*--------------------------------------------------------------------------
 * Macro used to profile on a scope basis
 *------------------------------------------------------------------------*/
#ifndef METADOT_DISABLE_PROFILING

#define METADOT_CONCAT2(_x, _y) _x##_y
#define METADOT_CONCAT(_x, _y) METADOT_CONCAT2(_x, _y)

#define METADOT_INIT() ProfilerInit()
#define METADOT_SCOPE_AUTO(x, ...) ProfilerScoped METADOT_CONCAT(profileScope, __LINE__)(__FILE__, __LINE__, x)
#define METADOT_SCOPE_BEGIN(n, ...) \
    uintptr_t profileid_##n;                    \
    profileid_##n = ProfilerBeginScope(__FILE__, __LINE__, #n);
#define METADOT_SCOPE_END(n) ProfilerEndScope(profileid_##n);
#define METADOT_BEGIN_FRAME() ProfilerBeginFrame()
#define METADOT_REGISTER_THREAD(n) ProfilerRegisterThread(n)
#define METADOT_SHUTDOWN() ProfilerShutDown()
#else
#define METADOT_INIT() void()
#define METADOT_SCOPE(...) void()
#define METADOT_BEGIN_FRAME() void()
#define METADOT_REGISTER_THREAD(n) void()
#define METADOT_SHUTDOWN() void()
#endif /* METADOT_DISABLE_PROFILING */

#endif /* __cplusplus */

#endif /* METADOT_METADOT_H */
