

#ifndef _METADOT_THREADPOOL_H_
#define _METADOT_THREADPOOL_H_

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct metadot_thpool_ *ThreadPoolC;

    ThreadPoolC metadot_thpool_init(int num_threads);
    int metadot_thpool_addwork(ThreadPoolC, void (*function_p)(void *), void *arg_p);
    void metadot_thpool_wait(ThreadPoolC);
    void metadot_thpool_pause(ThreadPoolC);
    void metadot_thpool_resume(ThreadPoolC);
    void metadot_thpool_destroy(ThreadPoolC);
    int metadot_thpool_workingcounts(ThreadPoolC);

#ifdef __cplusplus
}
#endif

#endif
