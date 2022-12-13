

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
