

#ifndef ME_PLATFORM_H
#define ME_PLATFORM_H

#include <filesystem>

#include "engine/core/core.hpp"
#include "engine/core/macros.hpp"
#include "engine/core/sdl_wrapper.h"

/*--------------------------------------------------------------------------
 * Platform specific headers
 *------------------------------------------------------------------------*/
#ifdef ME_PLATFORM_WINDOWS
#define WINDOWS_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x601
#endif
#include <windows.h>
#elif defined(ME_PLATFORM_POSIX)
#include <pthread.h>
#include <sys/time.h>
#else
#error "Unsupported platform!"
#endif

#if defined(ME_PLATFORM_WINDOWS)
#include <Windows.h>
#include <io.h>
#include <shobjidl.h>

#if defined(_MSC_VER)
#define __func__ __FUNCTION__
#endif

#undef min
#undef max

#define PATH_SEP '\\'
#ifndef PATH_MAX
#define PATH_MAX 260
#endif

#elif defined(ME_PLATFORM_LINUX)
#include <bits/types/struct_tm.h>
#include <bits/types/time_t.h>
#include <dirent.h>
#include <limits.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <sys/types.h>
#define PATH_SEP '/'
#elif defined(ME_PLATFORM_APPLE)
#include <TargetConditionals.h>
#include <mach-o/dyld.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#endif

#ifdef ME_PLATFORM_WINDOWS
#define getFullPath(a, b) GetFullPathName((LPCWSTR)a, MAX_PATH, (LPWSTR)b, NULL)
#define rmdir(a) _rmdir(a)
#define PATH_SEPARATOR '\\'
#else
#define getFullPath(a, b) realpath(a, b)
#define PATH_SEPARATOR '/'
#endif

#ifndef MAX_PATH
#include <limits.h>
#ifdef PATH_MAX
#define MAX_PATH PATH_MAX
#else
#define MAX_PATH 256
#endif
#endif

#if MAX_PATH > 1024
#undef MAX_PATH
#define MAX_PATH 1024
#endif

/*--------------------------------------------------------------------------*/

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

static inline void ME_tls_set_value(pthread_key_t _handle, void *_value) { pthread_setspecific(_handle, _value); }

static inline void *ME_tls_get_value(pthread_key_t _handle) { return pthread_getspecific(_handle); }

static inline void ME_tls_free(pthread_key_t _handle) { pthread_key_delete(_handle); }

#endif

namespace ME {

#if defined(ME_PLATFORM_WINDOWS)
typedef CRITICAL_SECTION ME_mutex;

static inline void ME_mutex_init(ME_mutex* _mutex) { InitializeCriticalSection(_mutex); }

static inline void ME_mutex_destroy(ME_mutex* _mutex) { DeleteCriticalSection(_mutex); }

static inline void ME_mutex_lock(ME_mutex* _mutex) { EnterCriticalSection(_mutex); }

static inline int ME_mutex_trylock(ME_mutex* _mutex) { return TryEnterCriticalSection(_mutex) ? 0 : 1; }

static inline void ME_mutex_unlock(ME_mutex* _mutex) { LeaveCriticalSection(_mutex); }

#elif defined(ME_PLATFORM_POSIX)
typedef pthread_mutex_t ME_mutex;

static inline void ME_mutex_init(ME_mutex *_mutex) { pthread_mutex_init(_mutex, NULL); }

static inline void ME_mutex_destroy(ME_mutex *_mutex) { pthread_mutex_destroy(_mutex); }

static inline void ME_mutex_lock(ME_mutex *_mutex) { pthread_mutex_lock(_mutex); }

static inline int ME_mutex_trylock(ME_mutex *_mutex) { return pthread_mutex_trylock(_mutex); }

static inline void ME_mutex_unlock(ME_mutex *_mutex) { pthread_mutex_unlock(_mutex); }

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

}  // namespace ME

typedef enum engine_displaymode { WINDOWED, BORDERLESS, FULLSCREEN } engine_displaymode;

typedef enum engine_windowflashaction { START, START_COUNT, START_UNTIL_FG, STOP } engine_windowflashaction;

typedef struct engine_platform {

} engine_platform;

#define RUNNER_EXIT 2

int ParseRunArgs(int argc, char* argv[]);
int metadot_initwindow();
void metadot_endwindow();
void metadot_set_displaymode(engine_displaymode mode);
void metadot_set_windowflash(engine_windowflashaction action, int count, int period);
void metadot_set_VSync(bool vsync);
void metadot_set_minimize_onlostfocus(bool minimize);
void metadot_set_windowtitle(const char* title);
char* metadot_clipboard_get();
void metadot_clipboard_set(const char* string);

ME_INLINE void metadot_platform_init_dpi() {
#ifdef _WIN32
#include <Windows.h>
    SetProcessDPIAware();
#endif
}

#pragma region strings

#ifndef _WIN32

#pragma message("this strinengine.h implementation is for Windows only!")

#else

static int bcmp(const void *s1, const void *s2, size_t n) { return memcmp(s1, s2, n); }

static void bcopy(const void *src, void *dest, size_t n) { memcpy(dest, src, n); }

static void bzero(void *s, size_t n) { memset(s, 0, n); }

static void explicit_bzero(void *s, size_t n) {
    volatile char *vs = (volatile char *)s;
    while (n) {
        *vs++ = 0;
        n--;
    }
}

static const char *index(const char *s, int c) { return strchr(s, c); }

static const char *rindex(const char *s, int c) { return strrchr(s, c); }

static int ffs(int i) {
    int bit;

    if (0 == i) return 0;

    for (bit = 1; !(i & 1); ++bit) i >>= 1;
    return bit;
}

static int ffsl(long i) {
    int bit;

    if (0 == i) return 0;

    for (bit = 1; !(i & 1); ++bit) i >>= 1;
    return bit;
}

static int ffsll(long long i) {
    int bit;

    if (0 == i) return 0;

    for (bit = 1; !(i & 1); ++bit) i >>= 1;
    return bit;
}

#ifndef __MINGW32__

static int strcasecmp(const char *s1, const char *s2) {
    const unsigned char *u1 = (const unsigned char *)s1;
    const unsigned char *u2 = (const unsigned char *)s2;
    int result;

    while ((result = tolower(*u1) - tolower(*u2)) == 0 && *u1 != 0) {
        *u1++;
        *u2++;
    }

    return result;
}

static int strncasecmp(const char *s1, const char *s2, size_t n) {
    const unsigned char *u1 = (const unsigned char *)s1;
    const unsigned char *u2 = (const unsigned char *)s2;
    int result;

    for (; n != 0; n--) {
        result = tolower(*u1) - tolower(*u2);
        if (result) return result;
        if (*u1 == 0) return 0;
    }
    return 0;
}

static int strcasecmp_l(const char *s1, const char *s2, _locale_t loc) {
    const unsigned char *u1 = (const unsigned char *)s1;
    const unsigned char *u2 = (const unsigned char *)s2;
    int result;

    while ((result = _tolower_l(*u1, loc) - _tolower_l(*u2, loc)) == 0 && *u1 != 0) {
        *u1++;
        *u2++;
    }

    return result;
}

static int strncasecmp_l(const char *s1, const char *s2, size_t n, _locale_t loc) {
    const unsigned char *u1 = (const unsigned char *)s1;
    const unsigned char *u2 = (const unsigned char *)s2;
    int result;

    for (; n != 0; n--) {
        result = _tolower_l(*u1, loc) - _tolower_l(*u2, loc);
        if (result) return result;
        if (*u1 == 0) return 0;
    }
    return 0;
}

#endif

#endif /* _WIN32 */

#pragma endregion strings

#if __linux__
#define openFile(filePath, mode) fopen(filePath, mode)
#define seekFile(file, offset, whence) fseeko(file, offset, whence)
#define tellFile(file) ftello(file)
#elif _WIN32
inline static FILE *openFile(const char *filePath, const char *mode) {
    FILE *file;

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

#ifdef ME_PLATFORM_WINDOWS
#define S_ISREG(m) (((m)&0170000) == (0100000))
#define S_ISDIR(m) (((m)&0170000) == (0040000))
#endif

inline bool ME_fs_exists(const char* path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0 || S_ISDIR(buffer.st_mode));
}

inline const char* ME_fs_get_filename(const char* path) {
    int len = strlen(path);
    int flag = 0;

    for (int i = len - 1; i > 0; i--) {
        if (path[i] == '\\' || path[i] == '//' || path[i] == '/') {
            flag = 1;
            path = path + i + 1;
            break;
        }
    }
    return path;
}

char* ME_fs_readfilestring(const char* path);
void ME_fs_freestring(void* ptr);
std::string ME_fs_normalize_path(const std::string& messyPath);
bool ME_fs_directory_exists(const std::filesystem::path& path, std::filesystem::file_status status = std::filesystem::file_status{});
void ME_fs_create_directory(const std::string& directory_name);
std::string ME_fs_readfile(const std::string& filename);

#endif  // ME_PLATFORM_H
