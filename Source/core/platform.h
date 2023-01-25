

#ifndef METADOT_PLATFORM_H
#define METADOT_PLATFORM_H

#include "core/macros.h"

/*--------------------------------------------------------------------------
 * Platform specific headers
 *------------------------------------------------------------------------*/
#ifdef METADOT_PLATFORM_WINDOWS
#define WINDOWS_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x601
#endif
#include <windows.h>
#elif defined(METADOT_PLATFORM_POSIX)
#include <pthread.h>
#include <sys/time.h>
#else
#error "Unsupported platform!"
#endif

/*--------------------------------------------------------------------------*/
static inline uint64_t getThreadID() {
#if defined(METADOT_PLATFORM_WINDOWS)
    return (uint64_t)GetCurrentThreadId();
#elif defined(METADOT_PLATFORM_LINUX)
    return (uint64_t)syscall(SYS_gettid);
#elif defined(METADOT_PLATFORM_APPLE)
    return (mach_port_t)::pthread_mach_thread_np(pthread_self());
#else
#error "Unsupported platform!"
#endif
}

/*--------------------------------------------------------------------------*/
static inline const char* getPlatformName(uint8_t _platformID) {
    switch (_platformID) {
        case 1:
            return "Windows";
        case 2:
            return "Linux";
        case 3:
            return "iOS";
        case 4:
            return "OSX";
        case 5:
            return "PlayStation 3";
        case 6:
            return "PlayStation 4";
        case 7:
            return "Android";
        case 8:
            return "XboxOne";
        case 9:
            return "WebGL";
        case 10:
            return "Nintendo Switch";
        default:
            return "Unknown platform";
    };
}

// Thread Local Storage(TLS)
// msvc: https://learn.microsoft.com/en-us/cpp/parallel/thread-local-storage-tls

static inline pthread_key_t tlsAllocate() {
    pthread_key_t handle;
    pthread_key_create(&handle, NULL);
    return handle;
}

static inline void tlsSetValue(pthread_key_t _handle, void* _value) { pthread_setspecific(_handle, _value); }

static inline void* tlsGetValue(pthread_key_t _handle) { return pthread_getspecific(_handle); }

static inline void tlsFree(pthread_key_t _handle) { pthread_key_delete(_handle); }

namespace MetaEngine {

#if defined(METADOT_PLATFORM_WINDOWS)
typedef CRITICAL_SECTION metadot_mutex;

static inline void metadot_mutex_init(metadot_mutex* _mutex) { InitializeCriticalSection(_mutex); }

static inline void metadot_mutex_destroy(metadot_mutex* _mutex) { DeleteCriticalSection(_mutex); }

static inline void metadot_mutex_lock(metadot_mutex* _mutex) { EnterCriticalSection(_mutex); }

static inline int metadot_mutex_trylock(metadot_mutex* _mutex) { return TryEnterCriticalSection(_mutex) ? 0 : 1; }

static inline void metadot_mutex_unlock(metadot_mutex* _mutex) { LeaveCriticalSection(_mutex); }

#elif defined(METADOT_PLATFORM_POSIX)
typedef pthread_mutex_t metadot_mutex;

static inline void metadot_mutex_init(metadot_mutex* _mutex) { pthread_mutex_init(_mutex, NULL); }

static inline void metadot_mutex_destroy(metadot_mutex* _mutex) { pthread_mutex_destroy(_mutex); }

static inline void metadot_mutex_lock(metadot_mutex* _mutex) { pthread_mutex_lock(_mutex); }

static inline int metadot_mutex_trylock(metadot_mutex* _mutex) { return pthread_mutex_trylock(_mutex); }

static inline void metadot_mutex_unlock(metadot_mutex* _mutex) { pthread_mutex_unlock(_mutex); }

#else
#error "Unsupported platform!"
#endif

class Mutex {
    metadot_mutex m_mutex;

    Mutex(const Mutex& _rhs);
    Mutex& operator=(const Mutex& _rhs);

public:
    inline Mutex() { metadot_mutex_init(&m_mutex); }
    inline ~Mutex() { metadot_mutex_destroy(&m_mutex); }
    inline void lock() { metadot_mutex_lock(&m_mutex); }
    inline void unlock() { metadot_mutex_unlock(&m_mutex); }
    inline bool tryLock() { return (metadot_mutex_trylock(&m_mutex) == 0); }
};

class ScopedMutexLocker {
    Mutex& m_mutex;

    ScopedMutexLocker();
    ScopedMutexLocker(const ScopedMutexLocker&);
    ScopedMutexLocker& operator=(const ScopedMutexLocker&);

public:
    inline ScopedMutexLocker(Mutex& _mutex) : m_mutex(_mutex) { m_mutex.lock(); }
    inline ~ScopedMutexLocker() { m_mutex.unlock(); }
};

}  // namespace Profiler

#endif  // METADOT_PLATFORM_H
