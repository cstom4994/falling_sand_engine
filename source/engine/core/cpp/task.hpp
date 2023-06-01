

#ifndef ME_TASK_HPP
#define ME_TASK_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <list>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include "engine/core/cpp/promise.hpp"

namespace MetaEngine {

class Service {
    using Defer = Promise::Defer;
    using Promise = Promise::Promise;
    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
    using Timers = std::multimap<TimePoint, Defer>;
    using Tasks = std::deque<Defer>;
#if METADOT_PROMISE_MULTITHREAD
    using Mutex = MetaEngine::Promise::Mutex;
#endif

    Timers timers_;
    Tasks tasks_;
#if METADOT_PROMISE_MULTITHREAD
    // std::recursive_mutex mutex_;
    ME::ref<Mutex> mutex_;
#endif
    std::condition_variable_any cond_;
    std::atomic<bool> isAutoStop_;
    std::atomic<bool> isStop_;
    // Unlock and then lock
#if METADOT_PROMISE_MULTITHREAD
    struct unlock_guard_t {
        inline unlock_guard_t(ME::ref<Mutex> mutex) : mutex_(mutex), lock_count_(mutex->lock_count()) { mutex_->unlock(lock_count_); }
        inline ~unlock_guard_t() { mutex_->lock(lock_count_); }
        ME::ref<Mutex> mutex_;
        size_t lock_count_;
    };
#endif
public:
    Service()
        : isAutoStop_(true),
          isStop_(false)
#if METADOT_PROMISE_MULTITHREAD
          ,
          mutex_(ME::create_ref<Mutex>())
#endif
    {
    }

    // delay for milliseconds
    Promise delay(uint64_t time_ms) {
        return MetaEngine::Promise::newPromise([&](Defer &defer) {
            TimePoint now = std::chrono::steady_clock::now();
            TimePoint time = now + std::chrono::milliseconds(time_ms);
            timers_.emplace(time, defer);
        });
    }

    // yield for other tasks to run
    Promise yield() {
        return MetaEngine::Promise::newPromise([&](Defer &defer) {
#if METADOT_PROMISE_MULTITHREAD
            std::lock_guard<Mutex> lock(*mutex_);
#endif
            tasks_.push_back(defer);
            cond_.notify_one();
        });
    }

    // Resolve the defer object in this io thread
    void runInIoThread(const std::function<void()> &func) {
        MetaEngine::Promise::newPromise([=](Defer &defer) {
#if METADOT_PROMISE_MULTITHREAD
            std::lock_guard<Mutex> lock(*mutex_);
#endif
            tasks_.push_back(defer);
            cond_.notify_one();
        }).then([func]() { func(); });
    }

    // Set if the io thread will auto exist if no waiting tasks and timers.
    void setAutoStop(bool isAutoExit) {
#if METADOT_PROMISE_MULTITHREAD
        std::lock_guard<Mutex> lock(*mutex_);
#endif
        isAutoStop_ = isAutoExit;
        cond_.notify_one();
    }

    // run the service loop
    void run() {
#if METADOT_PROMISE_MULTITHREAD
        std::unique_lock<Mutex> lock(*mutex_);
#endif

        while (!isStop_ && (!isAutoStop_ || tasks_.size() > 0 || timers_.size() > 0)) {

            if (tasks_.size() == 0 && timers_.size() == 0) {
                cond_.wait(lock);
                continue;
            }

            while (!isStop_ && timers_.size() > 0) {
                TimePoint now = std::chrono::steady_clock::now();
                TimePoint time = timers_.begin()->first;
                if (time <= now) {
                    Defer &defer = timers_.begin()->second;
                    tasks_.push_back(defer);
                    timers_.erase(timers_.begin());
                } else if (tasks_.size() == 0) {
                    // std::this_thread::sleep_for(time - now);
                    cond_.wait_for(lock, time - now);
                } else {
                    break;
                }
            }

            // Check fixed size of tasks in this loop, so that timer have a chance to run.
            if (!isStop_ && tasks_.size() > 0) {
                size_t size = tasks_.size();
                for (size_t i = 0; i < size; ++i) {
                    Defer defer = tasks_.front();
                    tasks_.pop_front();

#if METADOT_PROMISE_MULTITHREAD
                    unlock_guard_t unlock(mutex_);
                    defer.resolve();
                    break;  // for only once
#else
                    defer.resolve();  // loop with the size
#endif
                }
            }
        }

        // Clear pending timers and tasks
        while (timers_.size() > 0 || tasks_.size()) {
            while (timers_.size() > 0) {
                Defer defer = timers_.begin()->second;
                timers_.erase(timers_.begin());
#if METADOT_PROMISE_MULTITHREAD
                unlock_guard_t unlock(mutex_);
#endif
                defer.reject(std::runtime_error("service stopped"));
            }
            while (tasks_.size() > 0) {
                Defer defer = tasks_.front();
                tasks_.pop_front();
#if METADOT_PROMISE_MULTITHREAD
                unlock_guard_t unlock(mutex_);
#endif
                defer.reject(std::runtime_error("service stopped"));
            }
        }
    }

    // stop the service loop
    void stop() {
#if METADOT_PROMISE_MULTITHREAD
        std::lock_guard<Mutex> lock(*mutex_);
#endif
        isStop_ = true;
        cond_.notify_one();
    }
};
}  // namespace MetaEngine

#endif
