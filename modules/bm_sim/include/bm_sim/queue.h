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

#ifndef _BM_QUEUE_H_
#define _BM_QUEUE_H_

#include <deque>
#include <mutex>
#include <condition_variable>

/* TODO: implement non blocking behavior */

template <class T>
class Queue {
public:
  enum WriteBehavior { WriteBlock, WriteReturn };
  enum ReadBehavior { ReadBlock, ReadReturn };

public:
 Queue()
   : capacity(1024), wb(WriteBlock), rb(ReadBlock) { }

  Queue(size_t capacity,
	WriteBehavior wb = WriteBlock, ReadBehavior rb = ReadBlock)
    : capacity(capacity), wb(wb), rb(rb) {}

  void push_front(const T &item) {
    std::unique_lock<std::mutex> lock(q_mutex);
    while(!is_not_full()) {
      if(wb == WriteReturn) return;
      q_not_full.wait(lock);
    }
    queue.push_front(item);
    lock.unlock();
    q_not_empty.notify_one();
  }

  void push_front(T &&item) {
    std::unique_lock<std::mutex> lock(q_mutex);
    while(!is_not_full()) {
      if(wb == WriteReturn) return;
      q_not_full.wait(lock);
    }
    queue.push_front(std::move(item));
    lock.unlock();
    q_not_empty.notify_one();
  }

  void pop_back(T* pItem) {
    std::unique_lock<std::mutex> lock(q_mutex);
    while(!is_not_empty())
      q_not_empty.wait(lock);
    *pItem = std::move(queue.back());
    queue.pop_back();
    lock.unlock();
    q_not_full.notify_one();
  }

  size_t size() {
    std::unique_lock<std::mutex> lock(q_mutex);
    return queue.size();
  }

  void set_capacity(const size_t c) {
    // change capacity but does not discard elements
    std::unique_lock<std::mutex> lock(q_mutex);
    capacity = c;
  }

  Queue(const Queue &) = delete;
  Queue &operator =(const Queue &) = delete;

private:
  bool is_not_empty() const { return queue.size() > 0; }
  bool is_not_full() const { return queue.size() < capacity; }

  size_t capacity;
  std::deque<T> queue;
  WriteBehavior wb;
  ReadBehavior rb;

  std::mutex q_mutex;
  std::condition_variable q_not_empty;
  std::condition_variable q_not_full;
};

#endif
