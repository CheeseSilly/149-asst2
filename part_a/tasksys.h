#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

// thread pool
class ThreadPool {
public:
  ThreadPool(size_t numThreads) : taskCount(0), stop(false) {
    for (int i = 0; i < numThreads; i++) {
      threads.emplace_back([this] {
        while (true) {
          std::function<void()> task;

          {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this] { return stop || !tasks.empty(); });
            if (stop) {
              return;
            }
            task = tasks.front();
            tasks.pop();
          }
          task();
          {
            std::unique_lock<std::mutex> lock(mutex);
            taskCount--;
            if (taskCount == 0) {
              cv.notify_all();
            }
          }
        }
      });
    }
  }

  void waitAllTasks() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [this] { return taskCount == 0; });
  }

  template <typename Func, typename... Args>
  void enqueue(Func &&func, Args &&...args) {
    {
      std::unique_lock<std::mutex> lock(mutexCount);
      taskCount++;
    }

    {
      std::unique_lock<std::mutex> lock(mutex);
      tasks.emplace([func, args...] { func(args...); });
    }
    cv.notify_one();
  }

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(mutex);
      stop = true;
    }
    cv.notify_all();
    for (auto &thread : threads) {
      thread.join();
    }
  }

private:
  size_t taskCount; // num of tasks
  bool stop;        // signol of stop
  std::mutex mutex;
  std::mutex mutexCount;
  std::condition_variable cv;

  std::vector<std::thread> threads;
  std::queue<std::function<void()>> tasks;
};
/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial : public ITaskSystem {
public:
  TaskSystemSerial(int num_threads);
  ~TaskSystemSerial();
  const char *name();
  void run(IRunnable *runnable, int num_total_tasks);
  TaskID runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                          const std::vector<TaskID> &deps);
  void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn : public ITaskSystem {
public:
  TaskSystemParallelSpawn(int num_threads);
  ~TaskSystemParallelSpawn();
  const char *name();
  // note threads
  void run(IRunnable *runnable, int num_total_tasks);
  TaskID runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                          const std::vector<TaskID> &deps);
  void sync();

private:
  int num_threads;
  std::vector<std::thread> threads_;
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning : public ITaskSystem {
public:
  TaskSystemParallelThreadPoolSpinning(int num_threads);
  ~TaskSystemParallelThreadPoolSpinning();
  const char *name();
  void run(IRunnable *runnable, int num_total_tasks);
  TaskID runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                          const std::vector<TaskID> &deps);
  void sync();

private:
  int num_threads;
  ThreadPool *pool_;
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSleeping : public ITaskSystem {
public:
  TaskSystemParallelThreadPoolSleeping(int num_threads);
  ~TaskSystemParallelThreadPoolSleeping();
  const char *name();
  void run(IRunnable *runnable, int num_total_tasks);
  TaskID runAsyncWithDeps(IRunnable *runnable, int num_total_tasks,
                          const std::vector<TaskID> &deps);
  void sync();

private:
  int num_threads;
};

#endif
