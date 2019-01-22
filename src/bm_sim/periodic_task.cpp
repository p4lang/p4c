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

#include <bm/bm_sim/periodic_task.h>
#include <bm/bm_sim/logger.h>

#include <string>
#include <iostream>

namespace bm {

PeriodicTask::PeriodicTask(
    const std::string &name, std::function<void()> fn,
    std::chrono::milliseconds interval)
    : name(name), interval(interval), fn(fn),
      next(std::chrono::system_clock::now() + interval) {
  PeriodicTaskList::get_instance().register_task(this);
}

PeriodicTask::~PeriodicTask() {
  // Destructor automatically unregisters the task
  cancel();
}

void
PeriodicTask::cancel() {
  PeriodicTaskList::get_instance().unregister_task(this);
}

void
PeriodicTask::reset_next() {
  next = std::chrono::system_clock::now() + interval;
}

void
PeriodicTask::execute() {
  fn();
  reset_next();
}

bool
PeriodicTaskList::register_task(PeriodicTask *task) {
  std::lock_guard<std::mutex> lock(queue_mutex);

  // Iterate over task queue to check if it already contains the task
  TaskQueue tmp;
  bool contains = false;
  while (!task_queue.empty()) {
    if (task_queue.top() == task) {
      contains = true;
    }
    tmp.push(task_queue.top());
    task_queue.pop();
  }
  task_queue.swap(tmp);

  // Do not add a task twice
  if (contains) {
    Logger::get()->warn("Task {} already exists in periodic task queue",
                        task->name);
    return false;
  }

  task_queue.push(task);
  cv.notify_all();
  return true;
}

bool
PeriodicTaskList::unregister_task(PeriodicTask *task) {
  std::lock_guard<std::mutex> lock(queue_mutex);

  TaskQueue tmp;
  bool removed = false;
  // Removal from a priority queue necessitates iterating over elements
  while (!task_queue.empty()) {
    if (task_queue.top() == task) {
      removed = true;
    } else {
      tmp.push(task_queue.top());
    }
    task_queue.pop();
  }
  task_queue.swap(tmp);

  cv.notify_all();
  if (!removed) {
    Logger::get()->warn("Task {} does not exist in periodic task queue",
                        task->name);
    return false;
  }
  return true;
}

void
PeriodicTaskList::start() {
  std::lock_guard<std::mutex> lock(queue_mutex);
  if (running) {
    Logger::get()->warn("Tried to start already-running periodic task loop");
    return;
  }
  running = true;
  periodic_thread = std::thread(&PeriodicTaskList::loop, this);
}

void
PeriodicTaskList::join() {
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    if (!running) {
      Logger::get()->warn("Tried to join non-running periodic thread");
      return;
    }
    running = false;
    cv.notify_all();
  }
  periodic_thread.join();
}

void
PeriodicTaskList::loop() {
  while (true) {
    std::unique_lock<std::mutex> lk(queue_mutex);
    if (!running) {
      break;
    }
    if (task_queue.empty()) {
      cv.wait(lk);
      continue;
    }
    std::chrono::system_clock::time_point& next = task_queue.top()->next;
    if (cv.wait_until(lk, next) == std::cv_status::timeout) {
      if (!task_queue.empty()) {
        auto task = task_queue.top();
        task_queue.pop();
        task->execute();
        task_queue.push(task);
      }
    }
  }
}

PeriodicTaskList&
PeriodicTaskList::get_instance() {
    static PeriodicTaskList instance;
    return instance;
}

PeriodicTaskList::~PeriodicTaskList() {
    join();
}

}  // namespace bm
