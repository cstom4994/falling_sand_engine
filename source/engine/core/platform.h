
#ifndef ME_PLATFORM_H
#define ME_PLATFORM_H

#include <filesystem>

#include "engine/core/core.hpp"
#include "engine/core/macros.hpp"
#include "engine/core/sdl_wrapper.h"

#ifdef ME_PLATFORM_WINDOWS
#ifndef WINDOWS_LEAN_AND_MEAN
#define WINDOWS_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x601
#endif
#include <Windows.h>
#include <io.h>
#include <shobjidl.h>
#include <windows.h>

#if defined(_MSC_VER)
#define __func__ __FUNCTION__
#endif

#undef min
#undef max

#define PATH_SEP '\\'
#ifndef PATH_MAX
#define PATH_MAX 260
#endif

#define S_ISREG(m) (((m)&0170000) == (0100000))
#define S_ISDIR(m) (((m)&0170000) == (0040000))

#elif defined(ME_PLATFORM_POSIX) || defined(ME_PLATFORM_LINUX)
#include <bits/types/struct_tm.h>
#include <bits/types/time_t.h>
#include <dirent.h>
#include <limits.h>
#include <pthread.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#define PATH_SEP '/'

#elif defined(ME_PLATFORM_APPLE)
#include <TargetConditionals.h>
#include <mach-o/dyld.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#else
#error "Unsupported platform!"
#endif

#ifdef ME_PLATFORM_WINDOWS
#define getFullPath(a, b) GetFullPathName((LPCWSTR)a, MAX_PATH, (LPWSTR)b, NULL)
#define rmdir(a) _rmdir(a)
#define PATH_SEPARATOR '\\'
#else
#define getFullPath(a, b) realpath(a, b)
#define PATH_SEPARATOR '/'
#endif

/*--------------------------------------------------------------------------*/

namespace ME {

typedef void* (*ME_gl_loader_fn)(const char* name);

static inline uint64_t ME_get_thread_id() {
#if defined(ME_PLATFORM_WINDOWS)
    return (uint64_t)GetCurrentThreadId();
#elif defined(ME_PLATFORM_LINUX)
    return (uint64_t)syscall(SYS_gettid);
#elif defined(ME_PLATFORM_APPLE)
    return (mach_port_t)::pthread_mach_thread_np(pthread_self());
#else
#error "Unsupported platform!"
#endif
}

// Thread Local Storage(TLS)
// msvc: https://learn.microsoft.com/en-us/cpp/parallel/thread-local-storage-tls

#ifdef ME_PLATFORM_WINDOWS

static inline uint32_t ME_tls_allocate() { return (uint32_t)TlsAlloc(); }

static inline void ME_tls_set_value(uint32_t _handle, void* _value) { TlsSetValue(_handle, _value); }

static inline void* ME_tls_get_value(uint32_t _handle) { return TlsGetValue(_handle); }

static inline void ME_tls_free(uint32_t _handle) { TlsFree(_handle); }

#else

static inline pthread_key_t ME_tls_allocate() {
    pthread_key_t handle;
    pthread_key_create(&handle, NULL);
    return handle;
}

static inline void ME_tls_set_value(pthread_key_t _handle, void* _value) { pthread_setspecific(_handle, _value); }

static inline void* ME_tls_get_value(pthread_key_t _handle) { return pthread_getspecific(_handle); }

static inline void ME_tls_free(pthread_key_t _handle) { pthread_key_delete(_handle); }

#endif

#if defined(ME_PLATFORM_WINDOWS)
typedef CRITICAL_SECTION ME_mutex;

static inline void ME_mutex_init(ME_mutex* _mutex) { InitializeCriticalSection(_mutex); }

static inline void ME_mutex_destroy(ME_mutex* _mutex) { DeleteCriticalSection(_mutex); }

static inline void ME_mutex_lock(ME_mutex* _mutex) { EnterCriticalSection(_mutex); }

static inline int ME_mutex_trylock(ME_mutex* _mutex) { return TryEnterCriticalSection(_mutex) ? 0 : 1; }

static inline void ME_mutex_unlock(ME_mutex* _mutex) { LeaveCriticalSection(_mutex); }

#elif defined(ME_PLATFORM_POSIX)
typedef pthread_mutex_t ME_mutex;

static inline void ME_mutex_init(ME_mutex* _mutex) { pthread_mutex_init(_mutex, NULL); }

static inline void ME_mutex_destroy(ME_mutex* _mutex) { pthread_mutex_destroy(_mutex); }

static inline void ME_mutex_lock(ME_mutex* _mutex) { pthread_mutex_lock(_mutex); }

static inline int ME_mutex_trylock(ME_mutex* _mutex) { return pthread_mutex_trylock(_mutex); }

static inline void ME_mutex_unlock(ME_mutex* _mutex) { pthread_mutex_unlock(_mutex); }

#else
#error "Unsupported platform!"
#endif

class pthread_mutex {
    ME_mutex m_mutex;

    pthread_mutex(const pthread_mutex& _rhs);
    pthread_mutex& operator=(const pthread_mutex& _rhs);

public:
    inline pthread_mutex() { ME_mutex_init(&m_mutex); }
    inline ~pthread_mutex() { ME_mutex_destroy(&m_mutex); }
    inline void lock() { ME_mutex_lock(&m_mutex); }
    inline void unlock() { ME_mutex_unlock(&m_mutex); }
    inline bool tryLock() { return (ME_mutex_trylock(&m_mutex) == 0); }
};

class scoped_mutex_locker {
    pthread_mutex& m_mutex;

    scoped_mutex_locker();
    scoped_mutex_locker(const scoped_mutex_locker&);
    scoped_mutex_locker& operator=(const scoped_mutex_locker&);

public:
    inline scoped_mutex_locker(pthread_mutex& _mutex) : m_mutex(_mutex) { m_mutex.lock(); }
    inline ~scoped_mutex_locker() { m_mutex.unlock(); }
};

typedef enum E_DisplayMode { WINDOWED, BORDERLESS, FULLSCREEN } E_DisplayMode;
typedef enum E_WindowFlashaction { START, START_COUNT, START_UNTIL_FG, STOP } E_WindowFlashaction;

#define RUNNER_EXIT 2

int ParseRunArgs(int argc, char* argv[]);
int ME_initwindow();
void ME_endwindow();
void ME_win_set_displaymode(E_DisplayMode mode);
void ME_win_set_windowflash(E_WindowFlashaction action, int count, int period);
void ME_set_vsync(bool vsync);
void ME_win_set_minimize_onlostfocus(bool minimize);
void ME_win_set_windowtitle(const char* title);
char* ME_clipboard_get();
void ME_clipboard_set(const char* string);

ME_INLINE void ME_win_init_dpi() {
#ifdef _WIN32
    SetProcessDPIAware();
#endif
}

#if __linux__
#define openFile(filePath, mode) fopen(filePath, mode)
#define seekFile(file, offset, whence) fseeko(file, offset, whence)
#define tellFile(file) ftello(file)
#elif _WIN32
inline static FILE* openFile(const char* filePath, const char* mode) {
    FILE* file;
    errno_t error = fopen_s(&file, filePath, mode);
    if (error != 0) return NULL;
    return file;
}
#define seekFile(file, offset, whence) _fseeki64(file, offset, whence)
#define tellFile(file) _ftelli64(file)
#else
#error Unsupported operating system
#endif

#define closeFile(file) fclose(file)

}  // namespace ME

#endif  // ME_PLATFORM_H
