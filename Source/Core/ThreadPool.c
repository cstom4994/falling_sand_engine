// Copyright(c) 2022, KaoruXun All rights reserved.

#if defined(__APPLE__)
#include <AvailabilityMacros.h>
#else
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/prctl.h>
#endif

#include "Core/Core.h"
#include "ThreadPool.h"

/* Platform specific includes */
#if defined(METADOT_PLATFORM_POSIX)
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#elif defined(METADOT_PLATFORM_WIN32)
#include <process.h>
#include <sys/timeb.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    int mtx_init(mtx_t *mtx, int type) {
#if defined(METADOT_PLATFORM_WIN32)
        mtx->mAlreadyLocked = FALSE;
        mtx->mRecursive = type & mtx_recursive;
        mtx->mTimed = type & mtx_timed;
        if (!mtx->mTimed) {
            InitializeCriticalSection(&(mtx->mHandle.cs));
        } else {
            mtx->mHandle.mut = CreateMutex(NULL, FALSE, NULL);
            if (mtx->mHandle.mut == NULL) { return thrd_error; }
        }
        return thrd_success;
#else
    int ret;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    if (type & mtx_recursive) { pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); }
    ret = pthread_mutex_init(mtx, &attr);
    pthread_mutexattr_destroy(&attr);
    return ret == 0 ? thrd_success : thrd_error;
#endif
    }

    void mtx_destroy(mtx_t *mtx) {
#if defined(METADOT_PLATFORM_WIN32)
        if (!mtx->mTimed) {
            DeleteCriticalSection(&(mtx->mHandle.cs));
        } else {
            CloseHandle(mtx->mHandle.mut);
        }
#else
    pthread_mutex_destroy(mtx);
#endif
    }

    int mtx_lock(mtx_t *mtx) {
#if defined(METADOT_PLATFORM_WIN32)
        if (!mtx->mTimed) {
            EnterCriticalSection(&(mtx->mHandle.cs));
        } else {
            switch (WaitForSingleObject(mtx->mHandle.mut, INFINITE)) {
                case WAIT_OBJECT_0:
                    break;
                case WAIT_ABANDONED:
                default:
                    return thrd_error;
            }
        }

        if (!mtx->mRecursive) {
            while (mtx->mAlreadyLocked) Sleep(1); /* Simulate deadlock... */
            mtx->mAlreadyLocked = TRUE;
        }
        return thrd_success;
#else
    return pthread_mutex_lock(mtx) == 0 ? thrd_success : thrd_error;
#endif
    }

    int mtx_timedlock(mtx_t *mtx, const struct timespec *ts) {
#if defined(METADOT_PLATFORM_WIN32)
        struct timespec current_ts;
        DWORD timeoutMs;

        if (!mtx->mTimed) { return thrd_error; }

        timespec_get(&current_ts, TIME_UTC);

        if ((current_ts.tv_sec > ts->tv_sec) ||
            ((current_ts.tv_sec == ts->tv_sec) && (current_ts.tv_nsec >= ts->tv_nsec))) {
            timeoutMs = 0;
        } else {
            timeoutMs = (DWORD) (ts->tv_sec - current_ts.tv_sec) * 1000;
            timeoutMs += (ts->tv_nsec - current_ts.tv_nsec) / 1000000;
            timeoutMs += 1;
        }

        /* TODO: the timeout for WaitForSingleObject doesn't include time
     while the computer is asleep. */
        switch (WaitForSingleObject(mtx->mHandle.mut, timeoutMs)) {
            case WAIT_OBJECT_0:
                break;
            case WAIT_TIMEOUT:
                return thrd_timedout;
            case WAIT_ABANDONED:
            default:
                return thrd_error;
        }

        if (!mtx->mRecursive) {
            while (mtx->mAlreadyLocked) Sleep(1); /* Simulate deadlock... */
            mtx->mAlreadyLocked = TRUE;
        }

        return thrd_success;
#elif defined(_POSIX_TIMEOUTS) && (_POSIX_TIMEOUTS >= 200112L) && defined(_POSIX_THREADS) &&       \
        (_POSIX_THREADS >= 200112L)
    switch (pthread_mutex_timedlock(mtx, ts)) {
        case 0:
            return thrd_success;
        case ETIMEDOUT:
            return thrd_timedout;
        default:
            return thrd_error;
    }
#else
    int rc;
    struct timespec cur, dur;

    /* Try to acquire the lock and, if we fail, sleep for 5ms. */
    while ((rc = pthread_mutex_trylock(mtx)) == EBUSY) {
        timespec_get(&cur, TIME_UTC);

        if ((cur.tv_sec > ts->tv_sec) ||
            ((cur.tv_sec == ts->tv_sec) && (cur.tv_nsec >= ts->tv_nsec))) {
            break;
        }

        dur.tv_sec = ts->tv_sec - cur.tv_sec;
        dur.tv_nsec = ts->tv_nsec - cur.tv_nsec;
        if (dur.tv_nsec < 0) {
            dur.tv_sec--;
            dur.tv_nsec += 1000000000;
        }

        if ((dur.tv_sec != 0) || (dur.tv_nsec > 5000000)) {
            dur.tv_sec = 0;
            dur.tv_nsec = 5000000;
        }

        nanosleep(&dur, NULL);
    }

    switch (rc) {
        case 0:
            return thrd_success;
        case ETIMEDOUT:
        case EBUSY:
            return thrd_timedout;
        default:
            return thrd_error;
    }
#endif
    }

    int mtx_trylock(mtx_t *mtx) {
#if defined(METADOT_PLATFORM_WIN32)
        int ret;

        if (!mtx->mTimed) {
            ret = TryEnterCriticalSection(&(mtx->mHandle.cs)) ? thrd_success : thrd_busy;
        } else {
            ret = (WaitForSingleObject(mtx->mHandle.mut, 0) == WAIT_OBJECT_0) ? thrd_success
                                                                              : thrd_busy;
        }

        if ((!mtx->mRecursive) && (ret == thrd_success)) {
            if (mtx->mAlreadyLocked) {
                LeaveCriticalSection(&(mtx->mHandle.cs));
                ret = thrd_busy;
            } else {
                mtx->mAlreadyLocked = TRUE;
            }
        }
        return ret;
#else
    return (pthread_mutex_trylock(mtx) == 0) ? thrd_success : thrd_busy;
#endif
    }

    int mtx_unlock(mtx_t *mtx) {
#if defined(METADOT_PLATFORM_WIN32)
        mtx->mAlreadyLocked = FALSE;
        if (!mtx->mTimed) {
            LeaveCriticalSection(&(mtx->mHandle.cs));
        } else {
            if (!ReleaseMutex(mtx->mHandle.mut)) { return thrd_error; }
        }
        return thrd_success;
#else
    return pthread_mutex_unlock(mtx) == 0 ? thrd_success : thrd_error;
    ;
#endif
    }

#if defined(METADOT_PLATFORM_WIN32)
#define _CONDITION_EVENT_ONE 0
#define _CONDITION_EVENT_ALL 1
#endif

    int cnd_init(cnd_t *cond) {
#if defined(METADOT_PLATFORM_WIN32)
        cond->mWaitersCount = 0;

        /* Init critical section */
        InitializeCriticalSection(&cond->mWaitersCountLock);

        /* Init events */
        cond->mEvents[_CONDITION_EVENT_ONE] = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (cond->mEvents[_CONDITION_EVENT_ONE] == NULL) {
            cond->mEvents[_CONDITION_EVENT_ALL] = NULL;
            return thrd_error;
        }
        cond->mEvents[_CONDITION_EVENT_ALL] = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (cond->mEvents[_CONDITION_EVENT_ALL] == NULL) {
            CloseHandle(cond->mEvents[_CONDITION_EVENT_ONE]);
            cond->mEvents[_CONDITION_EVENT_ONE] = NULL;
            return thrd_error;
        }

        return thrd_success;
#else
    return pthread_cond_init(cond, NULL) == 0 ? thrd_success : thrd_error;
#endif
    }

    void cnd_destroy(cnd_t *cond) {
#if defined(METADOT_PLATFORM_WIN32)
        if (cond->mEvents[_CONDITION_EVENT_ONE] != NULL) {
            CloseHandle(cond->mEvents[_CONDITION_EVENT_ONE]);
        }
        if (cond->mEvents[_CONDITION_EVENT_ALL] != NULL) {
            CloseHandle(cond->mEvents[_CONDITION_EVENT_ALL]);
        }
        DeleteCriticalSection(&cond->mWaitersCountLock);
#else
    pthread_cond_destroy(cond);
#endif
    }

    int cnd_signal(cnd_t *cond) {
#if defined(METADOT_PLATFORM_WIN32)
        int haveWaiters;

        /* Are there any waiters? */
        EnterCriticalSection(&cond->mWaitersCountLock);
        haveWaiters = (cond->mWaitersCount > 0);
        LeaveCriticalSection(&cond->mWaitersCountLock);

        /* If we have any waiting threads, send them a signal */
        if (haveWaiters) {
            if (SetEvent(cond->mEvents[_CONDITION_EVENT_ONE]) == 0) { return thrd_error; }
        }

        return thrd_success;
#else
    return pthread_cond_signal(cond) == 0 ? thrd_success : thrd_error;
#endif
    }

    int cnd_broadcast(cnd_t *cond) {
#if defined(METADOT_PLATFORM_WIN32)
        int haveWaiters;

        /* Are there any waiters? */
        EnterCriticalSection(&cond->mWaitersCountLock);
        haveWaiters = (cond->mWaitersCount > 0);
        LeaveCriticalSection(&cond->mWaitersCountLock);

        /* If we have any waiting threads, send them a signal */
        if (haveWaiters) {
            if (SetEvent(cond->mEvents[_CONDITION_EVENT_ALL]) == 0) { return thrd_error; }
        }

        return thrd_success;
#else
    return pthread_cond_broadcast(cond) == 0 ? thrd_success : thrd_error;
#endif
    }

#if defined(METADOT_PLATFORM_WIN32)
    static int _cnd_timedwait_win32(cnd_t *cond, mtx_t *mtx, DWORD timeout) {
        DWORD result;
        int lastWaiter;

        /* Increment number of waiters */
        EnterCriticalSection(&cond->mWaitersCountLock);
        ++cond->mWaitersCount;
        LeaveCriticalSection(&cond->mWaitersCountLock);

        /* Release the mutex while waiting for the condition (will decrease
     the number of waiters when done)... */
        mtx_unlock(mtx);

        /* Wait for either event to become signaled due to cnd_signal() or
     cnd_broadcast() being called */
        result = WaitForMultipleObjects(2, cond->mEvents, FALSE, timeout);
        if (result == WAIT_TIMEOUT) {
            /* The mutex is locked again before the function returns, even if an error occurred */
            mtx_lock(mtx);
            return thrd_timedout;
        } else if (result == WAIT_FAILED) {
            /* The mutex is locked again before the function returns, even if an error occurred */
            mtx_lock(mtx);
            return thrd_error;
        }

        /* Check if we are the last waiter */
        EnterCriticalSection(&cond->mWaitersCountLock);
        --cond->mWaitersCount;
        lastWaiter =
                (result == (WAIT_OBJECT_0 + _CONDITION_EVENT_ALL)) && (cond->mWaitersCount == 0);
        LeaveCriticalSection(&cond->mWaitersCountLock);

        /* If we are the last waiter to be notified to stop waiting, reset the event */
        if (lastWaiter) {
            if (ResetEvent(cond->mEvents[_CONDITION_EVENT_ALL]) == 0) {
                /* The mutex is locked again before the function returns, even if an error occurred */
                mtx_lock(mtx);
                return thrd_error;
            }
        }

        /* Re-acquire the mutex */
        mtx_lock(mtx);

        return thrd_success;
    }
#endif

    int cnd_wait(cnd_t *cond, mtx_t *mtx) {
#if defined(METADOT_PLATFORM_WIN32)
        return _cnd_timedwait_win32(cond, mtx, INFINITE);
#else
    return pthread_cond_wait(cond, mtx) == 0 ? thrd_success : thrd_error;
#endif
    }

    int cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts) {
#if defined(METADOT_PLATFORM_WIN32)
        struct timespec now;
        if (timespec_get(&now, TIME_UTC) == TIME_UTC) {
            unsigned long long nowInMilliseconds = now.tv_sec * 1000 + now.tv_nsec / 1000000;
            unsigned long long tsInMilliseconds = ts->tv_sec * 1000 + ts->tv_nsec / 1000000;
            DWORD delta = (tsInMilliseconds > nowInMilliseconds)
                                  ? (DWORD) (tsInMilliseconds - nowInMilliseconds)
                                  : 0;
            return _cnd_timedwait_win32(cond, mtx, delta);
        } else
            return thrd_error;
#else
    int ret;
    ret = pthread_cond_timedwait(cond, mtx, ts);
    if (ret == ETIMEDOUT) { return thrd_timedout; }
    return ret == 0 ? thrd_success : thrd_error;
#endif
    }

#if defined(METADOT_PLATFORM_WIN32)
    struct TinyCThreadTSSData
    {
        void *value;
        tss_t key;
        struct TinyCThreadTSSData *next;
    };

    static tss_dtor_t _tinycthread_tss_dtors[1088] = {
            NULL,
    };

    static _Thread_local struct TinyCThreadTSSData *_tinycthread_tss_head = NULL;
    static _Thread_local struct TinyCThreadTSSData *_tinycthread_tss_tail = NULL;

    static void _tinycthread_tss_cleanup(void);

    static void _tinycthread_tss_cleanup(void) {
        struct TinyCThreadTSSData *data;
        int iteration;
        unsigned int again = 1;
        void *value;

        for (iteration = 0; iteration < TSS_DTOR_ITERATIONS && again > 0; iteration++) {
            again = 0;
            for (data = _tinycthread_tss_head; data != NULL; data = data->next) {
                if (data->value != NULL) {
                    value = data->value;
                    data->value = NULL;

                    if (_tinycthread_tss_dtors[data->key] != NULL) {
                        again = 1;
                        _tinycthread_tss_dtors[data->key](value);
                    }
                }
            }
        }

        while (_tinycthread_tss_head != NULL) {
            data = _tinycthread_tss_head->next;
            free(_tinycthread_tss_head);
            _tinycthread_tss_head = data;
        }
        _tinycthread_tss_head = NULL;
        _tinycthread_tss_tail = NULL;
    }

    static void NTAPI _tinycthread_tss_callback(PVOID h, DWORD dwReason, PVOID pv) {
        (void) h;
        (void) pv;

        if (_tinycthread_tss_head != NULL &&
            (dwReason == DLL_THREAD_DETACH || dwReason == DLL_PROCESS_DETACH)) {
            _tinycthread_tss_cleanup();
        }
    }

#if defined(_MSC_VER)
#ifdef _M_X64
#pragma const_seg(".CRT$XLB")
#else
#pragma data_seg(".CRT$XLB")
#endif
    PIMAGE_TLS_CALLBACK p_thread_callback = _tinycthread_tss_callback;
#ifdef _M_X64
#pragma data_seg()
#else
#pragma const_seg()
#endif
#else
    PIMAGE_TLS_CALLBACK p_thread_callback __attribute__((section(".CRT$XLB"))) =
            _tinycthread_tss_callback;
#endif

#endif /* defined(METADOT_PLATFORM_WIN32) */

    /** Information to pass to the new thread (what to run). */
    typedef struct
    {
        thrd_start_t mFunction; /**< Pointer to the function to be executed. */
        void *mArg;             /**< Function argument for the thread function. */
    } _thread_start_info;

/* Thread wrapper function. */
#if defined(METADOT_PLATFORM_WIN32)
    static DWORD WINAPI _thrd_wrapper_function(LPVOID aArg)
#elif defined(METADOT_PLATFORM_POSIX)
static void *_thrd_wrapper_function(void *aArg)
#endif
    {
        thrd_start_t fun;
        void *arg;
        int res;

        /* Get thread startup information */
        _thread_start_info *ti = (_thread_start_info *) aArg;
        fun = ti->mFunction;
        arg = ti->mArg;

        /* The thread is responsible for freeing the startup information */
        free((void *) ti);

        /* Call the actual client thread function */
        res = fun(arg);

#if defined(METADOT_PLATFORM_WIN32)
        if (_tinycthread_tss_head != NULL) { _tinycthread_tss_cleanup(); }

        return (DWORD) res;
#else
    return (void *) (intptr_t) res;
#endif
    }

    int thrd_create(thrd_t *thr, thrd_start_t func, void *arg) {
        /* Fill out the thread startup information (passed to the thread wrapper,
     which will eventually free it) */
        _thread_start_info *ti = (_thread_start_info *) malloc(sizeof(_thread_start_info));
        if (ti == NULL) { return thrd_nomem; }
        ti->mFunction = func;
        ti->mArg = arg;

        /* Create the thread */
#if defined(METADOT_PLATFORM_WIN32)
        *thr = CreateThread(NULL, 0, _thrd_wrapper_function, (LPVOID) ti, 0, NULL);
#elif defined(METADOT_PLATFORM_POSIX)
    if (pthread_create(thr, NULL, _thrd_wrapper_function, (void *) ti) != 0) { *thr = 0; }
#endif

        /* Did we fail to create the thread? */
        if (!*thr) {
            free(ti);
            return thrd_error;
        }

        return thrd_success;
    }

    thrd_t thrd_current(void) {
#if defined(METADOT_PLATFORM_WIN32)
        return GetCurrentThread();
#else
    return pthread_self();
#endif
    }

    int thrd_detach(thrd_t thr) {
#if defined(METADOT_PLATFORM_WIN32)
        /* https://stackoverflow.com/questions/12744324/how-to-detach-a-thread-on-windows-c#answer-12746081 */
        return CloseHandle(thr) != 0 ? thrd_success : thrd_error;
#else
    return pthread_detach(thr) == 0 ? thrd_success : thrd_error;
#endif
    }

    int thrd_equal(thrd_t thr0, thrd_t thr1) {
#if defined(METADOT_PLATFORM_WIN32)
        return GetThreadId(thr0) == GetThreadId(thr1);
#else
    return pthread_equal(thr0, thr1);
#endif
    }

    void thrd_exit(int res) {
#if defined(METADOT_PLATFORM_WIN32)
        if (_tinycthread_tss_head != NULL) { _tinycthread_tss_cleanup(); }

        ExitThread((DWORD) res);
#else
    pthread_exit((void *) (intptr_t) res);
#endif
    }

    int thrd_join(thrd_t thr, int *res) {
#if defined(METADOT_PLATFORM_WIN32)
        DWORD dwRes;

        if (WaitForSingleObject(thr, INFINITE) == WAIT_FAILED) { return thrd_error; }
        if (res != NULL) {
            if (GetExitCodeThread(thr, &dwRes) != 0) {
                *res = (int) dwRes;
            } else {
                return thrd_error;
            }
        }
        CloseHandle(thr);
#elif defined(METADOT_PLATFORM_POSIX)
    void *pres;
    if (pthread_join(thr, &pres) != 0) { return thrd_error; }
    if (res != NULL) { *res = (int) (intptr_t) pres; }
#endif
        return thrd_success;
    }

    int thrd_sleep(const struct timespec *duration, struct timespec *remaining) {
#if !defined(METADOT_PLATFORM_WIN32)
        int res = nanosleep(duration, remaining);
        if (res == 0) {
            return 0;
        } else if (errno == EINTR) {
            return -1;
        } else {
            return -2;
        }
#else
    struct timespec start;
    DWORD t;

    timespec_get(&start, TIME_UTC);

    t = SleepEx((DWORD) (duration->tv_sec * 1000 + duration->tv_nsec / 1000000 +
                         (((duration->tv_nsec % 1000000) == 0) ? 0 : 1)),
                TRUE);

    if (t == 0) {
        return 0;
    } else {
        if (remaining != NULL) {
            timespec_get(remaining, TIME_UTC);
            remaining->tv_sec -= start.tv_sec;
            remaining->tv_nsec -= start.tv_nsec;
            if (remaining->tv_nsec < 0) {
                remaining->tv_nsec += 1000000000;
                remaining->tv_sec -= 1;
            }
        }

        return (t == WAIT_IO_COMPLETION) ? -1 : -2;
    }
#endif
    }

    void thrd_yield(void) {
#if defined(METADOT_PLATFORM_WIN32)
        Sleep(0);
#else
    sched_yield();
#endif
    }

    int tss_create(tss_t *key, tss_dtor_t dtor) {
#if defined(METADOT_PLATFORM_WIN32)
        *key = TlsAlloc();
        if (*key == TLS_OUT_OF_INDEXES) { return thrd_error; }
        _tinycthread_tss_dtors[*key] = dtor;
#else
    if (pthread_key_create(key, dtor) != 0) { return thrd_error; }
#endif
        return thrd_success;
    }

    void tss_delete(tss_t key) {
#if defined(METADOT_PLATFORM_WIN32)
        struct TinyCThreadTSSData *data = (struct TinyCThreadTSSData *) TlsGetValue(key);
        struct TinyCThreadTSSData *prev = NULL;
        if (data != NULL) {
            if (data == _tinycthread_tss_head) {
                _tinycthread_tss_head = data->next;
            } else {
                prev = _tinycthread_tss_head;
                if (prev != NULL) {
                    while (prev->next != data) { prev = prev->next; }
                }
            }

            if (data == _tinycthread_tss_tail) { _tinycthread_tss_tail = prev; }

            free(data);
        }
        _tinycthread_tss_dtors[key] = NULL;
        TlsFree(key);
#else
    pthread_key_delete(key);
#endif
    }

    void *tss_get(tss_t key) {
#if defined(METADOT_PLATFORM_WIN32)
        struct TinyCThreadTSSData *data = (struct TinyCThreadTSSData *) TlsGetValue(key);
        if (data == NULL) { return NULL; }
        return data->value;
#else
    return pthread_getspecific(key);
#endif
    }

    int tss_set(tss_t key, void *val) {
#if defined(METADOT_PLATFORM_WIN32)
        struct TinyCThreadTSSData *data = (struct TinyCThreadTSSData *) TlsGetValue(key);
        if (data == NULL) {
            data = (struct TinyCThreadTSSData *) malloc(sizeof(struct TinyCThreadTSSData));
            if (data == NULL) { return thrd_error; }

            data->value = NULL;
            data->key = key;
            data->next = NULL;

            if (_tinycthread_tss_tail != NULL) {
                _tinycthread_tss_tail->next = data;
            } else {
                _tinycthread_tss_tail = data;
            }

            if (_tinycthread_tss_head == NULL) { _tinycthread_tss_head = data; }

            if (!TlsSetValue(key, data)) {
                free(data);
                return thrd_error;
            }
        }
        data->value = val;
#else
    if (pthread_setspecific(key, val) != 0) { return thrd_error; }
#endif
        return thrd_success;
    }

#if defined(_TTHREAD_EMULATE_TIMESPEC_GET_)
    int _tthread_timespec_get(struct timespec *ts, int base) {
#if defined(METADOT_PLATFORM_WIN32)
        struct _timeb tb;
#elif !defined(CLOCK_REALTIME)
        struct timeval tv;
#endif

        if (base != TIME_UTC) { return 0; }

#if defined(METADOT_PLATFORM_WIN32)
        _ftime_s(&tb);
        ts->tv_sec = (time_t) tb.time;
        ts->tv_nsec = 1000000L * (long) tb.millitm;
#elif defined(CLOCK_REALTIME)
        base = (clock_gettime(CLOCK_REALTIME, ts) == 0) ? base : 0;
#else
        gettimeofday(&tv, NULL);
        ts->tv_sec = (time_t) tv.tv_sec;
        ts->tv_nsec = 1000L * (long) tv.tv_usec;
#endif

        return base;
    }
#endif /* _TTHREAD_EMULATE_TIMESPEC_GET_ */

#if defined(METADOT_PLATFORM_WIN32)
    void call_once(once_flag *flag, void (*func)(void)) {
        /* The idea here is that we use a spin lock (via the
     InterlockedCompareExchange function) to restrict access to the
     critical section until we have initialized it, then we use the
     critical section to block until the callback has completed
     execution. */
        while (flag->status < 3) {
            switch (flag->status) {
                case 0:
                    if (InterlockedCompareExchange(&(flag->status), 1, 0) == 0) {
                        InitializeCriticalSection(&(flag->lock));
                        EnterCriticalSection(&(flag->lock));
                        flag->status = 2;
                        func();
                        flag->status = 3;
                        LeaveCriticalSection(&(flag->lock));
                        return;
                    }
                    break;
                case 1:
                    break;
                case 2:
                    EnterCriticalSection(&(flag->lock));
                    LeaveCriticalSection(&(flag->lock));
                    break;
            }
        }
    }
#endif /* defined(METADOT_PLATFORM_WIN32) */

#ifdef __cplusplus
}
#endif

static volatile int threads_keepalive;
static volatile int threads_on_hold;

/* ========================== STRUCTURES ============================ */

// Binary semaphore
typedef struct bsem
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int v;
} bsem;

typedef struct job
{
    struct job *prev;            /* pointer to previous job   */
    void (*function)(void *arg); /* function pointer          */
    void *arg;                   /* function's argument       */
} job;

typedef struct jobqueue
{
    pthread_mutex_t rwmutex; /* used for queue r/w access */
    job *front;              /* pointer to front of queue */
    job *rear;               /* pointer to rear  of queue */
    bsem *has_jobs;          /* flag as binary semaphore  */
    int len;                 /* number of jobs in queue   */
} jobqueue;

typedef struct thread
{
    int id;                       /* friendly id               */
    pthread_t pthread;            /* pointer to actual thread  */
    struct metadot_thpool_ *tp_p; /* access to thpool          */
} thread;

typedef struct metadot_thpool_
{
    thread **threads;                 /* pointer to threads        */
    volatile int num_threads_alive;   /* threads currently alive   */
    volatile int num_threads_working; /* threads currently working */
    pthread_mutex_t thcount_lock;     /* used for thread count etc */
    pthread_cond_t threads_all_idle;  /* signal to thpool_wait     */
    jobqueue jobqueue;                /* job queue                 */
} metadot_thpool_;

/* ========================== PROTOTYPES ============================ */

static int thread_init(metadot_thpool_ *tp_p, struct thread **thread_p, int id);
static void *thread_do(struct thread *thread_p);
static void thread_hold(int sig_id);
static void thread_destroy(struct thread *thread_p);

static int jobqueue_init(jobqueue *jobqueue_p);
static void jobqueue_clear(jobqueue *jobqueue_p);
static void jobqueue_push(jobqueue *jobqueue_p, struct job *newjob_p);
static struct job *jobqueue_pull(jobqueue *jobqueue_p);
static void jobqueue_destroy(jobqueue *jobqueue_p);

static void bsem_init(struct bsem *bsem_p, int value);
static void bsem_reset(struct bsem *bsem_p);
static void bsem_post(struct bsem *bsem_p);
static void bsem_post_all(struct bsem *bsem_p);
static void bsem_wait(struct bsem *bsem_p);

/* ========================== THREADPOOL ============================ */

struct metadot_thpool_ *metadot_thpool_init(int num_threads)
{

    threads_on_hold = 0;
    threads_keepalive = 1;

    if (num_threads < 0) { num_threads = 0; }

    metadot_thpool_ *tp_p;
    tp_p = (struct metadot_thpool_ *) malloc(sizeof(struct metadot_thpool_));
    if (tp_p == NULL) {
        METADOT_ERROR("Could not allocate memory for thread pool");
        return NULL;
    }
    tp_p->num_threads_alive = 0;
    tp_p->num_threads_working = 0;

    if (jobqueue_init(&tp_p->jobqueue) == -1) {
        METADOT_ERROR("Could not allocate memory for job queue");
        free(tp_p);
        return NULL;
    }

    tp_p->threads = (struct thread **) malloc(num_threads * sizeof(struct thread *));
    if (tp_p->threads == NULL) {
        METADOT_ERROR("Could not allocate memory for threads");
        jobqueue_destroy(&tp_p->jobqueue);
        free(tp_p);
        return NULL;
    }

    pthread_mutex_init(&(tp_p->thcount_lock), NULL);
    pthread_cond_init(&tp_p->threads_all_idle, NULL);

    int n;
    for (n = 0; n < num_threads; n++) {
        thread_init(tp_p, &tp_p->threads[n], n);
        METADOT_BUG("Created thread %d in pool", n);
    }

    while (tp_p->num_threads_alive != num_threads) {}

    return tp_p;
}

int metadot_thpool_addwork(metadot_thpool_ *tp_p, void (*function_p)(void *), void *arg_p) {
    job *newjob;

    newjob = (struct job *) malloc(sizeof(struct job));
    if (newjob == NULL) {
        METADOT_ERROR("Could not allocate memory for new job");
        return -1;
    }

    newjob->function = function_p;
    newjob->arg = arg_p;

    jobqueue_push(&tp_p->jobqueue, newjob);

    return 0;
}

void metadot_thpool_wait(metadot_thpool_ *tp_p) {
    pthread_mutex_lock(&tp_p->thcount_lock);
    while (tp_p->jobqueue.len || tp_p->num_threads_working) {
        pthread_cond_wait(&tp_p->threads_all_idle, &tp_p->thcount_lock);
    }
    pthread_mutex_unlock(&tp_p->thcount_lock);
}

void metadot_thpool_destroy(metadot_thpool_ *tp_p) {

    if (tp_p == NULL) return;

    volatile int threads_total = tp_p->num_threads_alive;

    threads_keepalive = 0;

    double TIMEOUT = 1.0;
    time_t start, end;
    double tpassed = 0.0;
    time(&start);
    while (tpassed < TIMEOUT && tp_p->num_threads_alive) {
        bsem_post_all(tp_p->jobqueue.has_jobs);
        time(&end);
        tpassed = difftime(end, start);
    }

    while (tp_p->num_threads_alive) {
        bsem_post_all(tp_p->jobqueue.has_jobs);
        sleep(1);
    }

    jobqueue_destroy(&tp_p->jobqueue);

    int n;
    for (n = 0; n < threads_total; n++) { thread_destroy(tp_p->threads[n]); }
    free(tp_p->threads);
    free(tp_p);
}

void metadot_thpool_pause(metadot_thpool_ *tp_p) {
    int n;
    for (n = 0; n < tp_p->num_threads_alive; n++) {
        pthread_kill(tp_p->threads[n]->pthread, SIGUSR1);
    }
}

void metadot_thpool_resume(metadot_thpool_ *tp_p) {

    (void) tp_p;

    threads_on_hold = 0;
}

int metadot_thpool_workingcounts(metadot_thpool_ *tp_p) { return tp_p->num_threads_working; }

static int thread_init(metadot_thpool_ *tp_p, struct thread **thread_p, int id) {

    *thread_p = (struct thread *) malloc(sizeof(struct thread));
    if (*thread_p == NULL) {
        METADOT_ERROR("Could not allocate memory for thread");
        return -1;
    }

    (*thread_p)->tp_p = tp_p;
    (*thread_p)->id = id;

    pthread_create(&(*thread_p)->pthread, NULL, (void *(*) (void *) ) thread_do, (*thread_p));
    pthread_detach((*thread_p)->pthread);
    return 0;
}

static void thread_hold(int sig_id) {
    (void) sig_id;
    threads_on_hold = 1;
    while (threads_on_hold) { sleep(1); }
}

static void *thread_do(struct thread *thread_p) {

    char thread_name[16] = {0};
    snprintf(thread_name, 16, "thpool-%d", thread_p->id);

#if defined(__linux__)

    prctl(PR_SET_NAME, thread_name);
#elif defined(__APPLE__) && defined(__MACH__)
    pthread_setname_np(thread_name);
#else
    err("thread_do(): pthread_setname_np is not supported on this system");
#endif

    metadot_thpool_ *tp_p = thread_p->tp_p;

    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = thread_hold;
    if (sigaction(SIGUSR1, &act, NULL) == -1) { METADOT_ERROR("cannot handle SIGUSR1"); }

    pthread_mutex_lock(&tp_p->thcount_lock);
    tp_p->num_threads_alive += 1;
    pthread_mutex_unlock(&tp_p->thcount_lock);

    while (threads_keepalive) {

        bsem_wait(tp_p->jobqueue.has_jobs);

        if (threads_keepalive) {

            pthread_mutex_lock(&tp_p->thcount_lock);
            tp_p->num_threads_working++;
            pthread_mutex_unlock(&tp_p->thcount_lock);

            void (*func_buff)(void *);
            void *arg_buff;
            job *job_p = jobqueue_pull(&tp_p->jobqueue);
            if (job_p) {
                func_buff = job_p->function;
                arg_buff = job_p->arg;
                func_buff(arg_buff);
                free(job_p);
            }

            pthread_mutex_lock(&tp_p->thcount_lock);
            tp_p->num_threads_working--;
            if (!tp_p->num_threads_working) { pthread_cond_signal(&tp_p->threads_all_idle); }
            pthread_mutex_unlock(&tp_p->thcount_lock);
        }
    }
    pthread_mutex_lock(&tp_p->thcount_lock);
    tp_p->num_threads_alive--;
    pthread_mutex_unlock(&tp_p->thcount_lock);

    return NULL;
}

static void thread_destroy(thread *thread_p) { free(thread_p); }

static int jobqueue_init(jobqueue *jobqueue_p) {
    jobqueue_p->len = 0;
    jobqueue_p->front = NULL;
    jobqueue_p->rear = NULL;

    jobqueue_p->has_jobs = (struct bsem *) malloc(sizeof(struct bsem));
    if (jobqueue_p->has_jobs == NULL) { return -1; }

    pthread_mutex_init(&(jobqueue_p->rwmutex), NULL);
    bsem_init(jobqueue_p->has_jobs, 0);

    return 0;
}

static void jobqueue_clear(jobqueue *jobqueue_p) {

    while (jobqueue_p->len) { free(jobqueue_pull(jobqueue_p)); }

    jobqueue_p->front = NULL;
    jobqueue_p->rear = NULL;
    bsem_reset(jobqueue_p->has_jobs);
    jobqueue_p->len = 0;
}

static void jobqueue_push(jobqueue *jobqueue_p, struct job *newjob) {

    pthread_mutex_lock(&jobqueue_p->rwmutex);
    newjob->prev = NULL;

    switch (jobqueue_p->len) {

        case 0:
            jobqueue_p->front = newjob;
            jobqueue_p->rear = newjob;
            break;

        default:
            jobqueue_p->rear->prev = newjob;
            jobqueue_p->rear = newjob;
    }
    jobqueue_p->len++;

    bsem_post(jobqueue_p->has_jobs);
    pthread_mutex_unlock(&jobqueue_p->rwmutex);
}

static struct job *jobqueue_pull(jobqueue *jobqueue_p) {

    pthread_mutex_lock(&jobqueue_p->rwmutex);
    job *job_p = jobqueue_p->front;

    switch (jobqueue_p->len) {

        case 0:
            break;

        case 1:
            jobqueue_p->front = NULL;
            jobqueue_p->rear = NULL;
            jobqueue_p->len = 0;
            break;

        default:
            jobqueue_p->front = job_p->prev;
            jobqueue_p->len--;

            bsem_post(jobqueue_p->has_jobs);
    }

    pthread_mutex_unlock(&jobqueue_p->rwmutex);
    return job_p;
}

static void jobqueue_destroy(jobqueue *jobqueue_p) {
    jobqueue_clear(jobqueue_p);
    free(jobqueue_p->has_jobs);
}

static void bsem_init(bsem *bsem_p, int value) {
    if (value < 0 || value > 1) {
        METADOT_ERROR("Binary semaphore can take only values 1 or 0");
        exit(1);
    }
    pthread_mutex_init(&(bsem_p->mutex), NULL);
    pthread_cond_init(&(bsem_p->cond), NULL);
    bsem_p->v = value;
}

static void bsem_reset(bsem *bsem_p) { bsem_init(bsem_p, 0); }

static void bsem_post(bsem *bsem_p) {
    pthread_mutex_lock(&bsem_p->mutex);
    bsem_p->v = 1;
    pthread_cond_signal(&bsem_p->cond);
    pthread_mutex_unlock(&bsem_p->mutex);
}

static void bsem_post_all(bsem *bsem_p) {
    pthread_mutex_lock(&bsem_p->mutex);
    bsem_p->v = 1;
    pthread_cond_broadcast(&bsem_p->cond);
    pthread_mutex_unlock(&bsem_p->mutex);
}

static void bsem_wait(bsem *bsem_p) {
    pthread_mutex_lock(&bsem_p->mutex);
    while (bsem_p->v != 1) { pthread_cond_wait(&bsem_p->cond, &bsem_p->mutex); }
    bsem_p->v = 0;
    pthread_mutex_unlock(&bsem_p->mutex);
}
