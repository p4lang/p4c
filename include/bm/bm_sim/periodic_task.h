/* Copyright 2018 Boon Thau Loo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BM_BM_SIM_PERIODIC_TASK_H_
#define BM_BM_SIM_PERIODIC_TASK_H_

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <queue>
#include <chrono>
#include <functional>
#include <condition_variable>
#include <mutex>

namespace bm {

//! Initializes and registers a task that is to be executed periodically
//! at a fixed interval. Task will execute during the lifetime of the object.
//! If a PeriodicTask object is made a member of an ActionPrimitive class,
//! this will ensure that the task is only executed if the extern represented
//! by the ActionPrimitive is instantiated in P4.
//! @code
//! class MyExtern : public ActionPrimitive<> {
//!   MyExtern() : task("my_task",
//!                     std::bind(&MyExtern::periodic_fn, this),
//!                     std::chrono::seconds(1)) {}
//!
//!   void operator ()() {
//!     // This will execute when extern is called
//!   }
//!
//!   void periodic_fn() {
//!     // This will execute once a second
//!   }
//!
//!   // Note: If `task` is not the last declared member, periodic_fn
//!   // may be called before all members have been initialized
//!   PeriodicTask task;
//! }
//! @endcode
class PeriodicTask {
 public:
  // Friend so that PeriodicTaskList can call execute()
  friend class PeriodicTaskList;

  PeriodicTask(const std::string &name,
               std::function<void()> fn,
               std::chrono::milliseconds interval);
  ~PeriodicTask();

  // Deleting copy-constructor and copy-assignment
  PeriodicTask(const PeriodicTask&) = delete;
  PeriodicTask& operator= (const PeriodicTask&) = delete;
  // Deleting move constructor and move assignment
  PeriodicTask(PeriodicTask&&) = delete;
  PeriodicTask& operator= (PeriodicTask&&) = delete;

  const std::string name;
  const std::chrono::milliseconds interval;

 private:
  //! Executes the stored function and sets `next` to the next desired
  //! execution time. To be called by PeriodicTaskList
  void execute();

  void reset_next();
  void cancel();

  const std::function<void()> fn;
  std::chrono::system_clock::time_point next;
};

//! Singleton which stores and executes periodic tasks.
//! Registration and unregistration are handled automatically
//! in the PeriodicTask constructor.
class PeriodicTaskList {
 public:
  static PeriodicTaskList &get_instance();

  // Returns true if task was successfully registered
  bool register_task(PeriodicTask *task);
  bool unregister_task(PeriodicTask *task);

  //! Starts the loop which executes the tasks in a new thread
  void start();
  void join();

 private:
  class PeriodCompare {
   public:
    bool
    operator() (const PeriodicTask *lhs, const PeriodicTask *rhs) {
      return lhs->next > rhs->next;
    }
  };
  using TaskQueue = std::priority_queue<PeriodicTask*,
                                        std::vector<PeriodicTask*>,
                                        PeriodCompare>;

  PeriodicTaskList() = default;
  ~PeriodicTaskList();

  void loop();

  // The queue of PeriodicTasks, ordered by next execution time
  TaskQueue task_queue;

  std::thread periodic_thread;
  bool running;
  mutable std::mutex queue_mutex;
  mutable std::condition_variable cv;
};

}  // namespace bm

#endif  // BM_BM_SIM_PERIODIC_TASK_H_
