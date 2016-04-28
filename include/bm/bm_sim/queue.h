/* Copyright 2013-present Barefoot Networks, Inc.
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

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

//! @file queue.h

#ifndef BM_BM_SIM_QUEUE_H_
#define BM_BM_SIM_QUEUE_H_

#include <deque>
#include <mutex>
#include <condition_variable>

namespace bm {

/* TODO(antonin): implement non blocking read behavior */

//! A utility queueing class made available to all targets. It could be used
//! -for example- to queue packets between an ingress thread and an egress
//! thread. This is a very simple class, which does not implement anything fancy
//! (e.g. rate limiting, priority queueing, fair scheduling, ...) but can be
//! used as a base class to build something more advanced.
//! Queue includes a mutex and is thread-safe.
template <class T>
class Queue {
 public:
  //! Implementation behavior when an item is pushed to a full queue
  enum WriteBehavior {
    //! block and wait until a slot is available
    WriteBlock,
    //! return immediately
    WriteReturn
  };
  //! Implementation behavior when an element is popped from an empty queue
  enum ReadBehavior {
    //! block and wait until the queue becomes non-empty
    ReadBlock,
    //! not implemented yet
    ReadReturn
  };

 public:
  Queue()
    : capacity(1024), wb(WriteBlock), rb(ReadBlock) { }

  //! Constructs a queue with specified \p capacity and read / write behaviors
  Queue(size_t capacity,
        WriteBehavior wb = WriteBlock, ReadBehavior rb = ReadBlock)
    : capacity(capacity), wb(wb), rb(rb) {}

  //! Makes a copy of \p item and pushes it to the front of the queue
  void push_front(const T &item) {
    std::unique_lock<std::mutex> lock(q_mutex);
    while (!is_not_full()) {
      if (wb == WriteReturn) return;
      q_not_full.wait(lock);
    }
    queue.push_front(item);
    lock.unlock();
    q_not_empty.notify_one();
  }

  //! Moves \p item to the front of the queue
  void push_front(T &&item) {
    std::unique_lock<std::mutex> lock(q_mutex);
    while (!is_not_full()) {
      if (wb == WriteReturn) return;
      q_not_full.wait(lock);
    }
    queue.push_front(std::move(item));
    lock.unlock();
    q_not_empty.notify_one();
  }

  //! Pops an element from the back of the queue: moves the element to `*pItem`.
  void pop_back(T* pItem) {
    std::unique_lock<std::mutex> lock(q_mutex);
    while (!is_not_empty())
      q_not_empty.wait(lock);
    *pItem = std::move(queue.back());
    queue.pop_back();
    lock.unlock();
    q_not_full.notify_one();
  }

  //! Get queue occupancy
  size_t size() const {
    std::unique_lock<std::mutex> lock(q_mutex);
    return queue.size();
  }

  //! Change the capacity of the queue
  void set_capacity(const size_t c) {
    // change capacity but does not discard elements
    std::unique_lock<std::mutex> lock(q_mutex);
    capacity = c;
  }

  //! Deleted copy constructor
  Queue(const Queue &) = delete;
  //! Deleted copy assignment operator
  Queue &operator =(const Queue &) = delete;

  //! Deleted move constructor (class includes mutex)
  Queue(Queue &&) = delete;
  //! Deleted move assignment operator (class includes mutex)
  Queue &&operator =(Queue &&) = delete;

 private:
  bool is_not_empty() const { return queue.size() > 0; }
  bool is_not_full() const { return queue.size() < capacity; }

  size_t capacity;
  std::deque<T> queue;
  WriteBehavior wb;
  ReadBehavior rb;

  mutable std::mutex q_mutex;
  mutable std::condition_variable q_not_empty;
  mutable std::condition_variable q_not_full;
};

}  // namespace bm

#endif  // BM_BM_SIM_QUEUE_H_
