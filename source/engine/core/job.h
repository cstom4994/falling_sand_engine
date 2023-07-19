
#ifndef ME_JOB_H
#define ME_JOB_H

#include <functional>

namespace ME {

// https://turanszkij.wordpress.com/2018/11/24/simple-job-system-using-standard-c/

// A Dispatched job will receive this as function argument:
struct job_dispatch_args {
    uint32_t jobIndex;
    uint32_t groupIndex;
};

class job {
public:
    // Create the internal resources such as worker threads, etc. Call it once when initializing the application.
    static void init();

    // Add a job to execute asynchronously. Any idle thread will execute this job.
    static void execute(const std::function<void()>& job);

    // Divide a job onto multiple jobs and execute in parallel.
    //  jobCount    : how many jobs to generate for this task.
    //  groupSize   : how many jobs to execute per thread. Jobs inside a group execute serially. It might be worth to increase for small jobs
    //  func        : receives a JobDispatchArgs as parameter
    static void dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(job_dispatch_args)>& job);

    // Check if any threads are working currently or not
    static bool is_busy();

    // Wait until all threads become idle
    static void wait();
};

}  // namespace ME

#endif  // !ME_JOB_H
