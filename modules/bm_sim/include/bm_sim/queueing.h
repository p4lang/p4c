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

//! @file queueing.h
//! This file contains convenience classes that can be useful for targets that
//! wish to queue packets at some point during processing (for example, between
//! an ingress pipeline and an egress pipeline, as is the case for the standard
//! simple switch target). We realized that if one decided to use the bm::Queue
//! class (in queue.h) to achieve this, quite a lot of work was required, even
//! for the standard, basic case: one queue per egress port, with a limited
//! number of threads processing all the queues.

#ifndef BM_SIM_INCLUDE_BM_SIM_QUEUEING_H_
#define BM_SIM_INCLUDE_BM_SIM_QUEUEING_H_

#include <deque>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <algorithm>  // for std::max

namespace bm {

//! One of the most basic queueing block possible. Lets you choose (at runtime)
//! the desired number of logical queues and the number of worker threads that
//! will be reading from these queues. I write "logical queues" because the
//! implementation actually uses as many physical queues as there are worker
//! threads. However, each logical queue still has its own maximum capacity.
//! As of now, the behavior is blocking for both read (pop_back()) and write
//! (push_front()), but we may offer additional options if there is interest
//! expressed in the future.
//!
//! Template parameter `T` is the type (has to be movable) of the objects that
//! will be stored in the queues. Template parameter `FMap` is a callable object
//! that has to be able to map every logical queue id to a worker id. The
//! following is a good example of functor that meets the requirements:
//! @code
//! struct WorkerMapper {
//!   WorkerMapper(size_t nb_workers)
//!       : nb_workers(nb_workers) { }
//!
//!   size_t operator()(size_t queue_id) const {
//!     return queue_id % nb_workers;
//!   }
//!
//!   size_t nb_workers;
//! };
//! @endcode
template <typename T, typename FMap>
class QueueingLogic {
 public:
  //! \p nb_queues is the number of logical queues; each queue is identified by
  //! an id in the range `[0, nb_queues)` when pushing to the queue. \p
  //! nb_workers is the number of threads that will be consuming from the
  //! queues; they will be identified by an id in the range `[0,
  //! nb_workers)`. \p capacity is the number of objects that each logical queue
  //! can hold. Because we need to be able to map each queue id to a worker id,
  //! the user has to provide a callable object of type `FMap`, \p
  //! map_to_worker, that can do this mapping. See the QueueingLogic class
  //! description for more information about the `FMap` template parameter.
  QueueingLogic(size_t nb_queues, size_t nb_workers, size_t capacity,
                FMap map_to_worker)
      : nb_queues(nb_queues), nb_workers(nb_workers),
        queues_info(nb_queues), workers_info(nb_workers),
        map_to_worker(std::move(map_to_worker)) {
    for (auto &q_info : queues_info)
      q_info.capacity = capacity;
  }

  //! Makes a copy of \p item and pushes it to the front of the logical queue
  //! with id \p queue_id.
  void push_front(size_t queue_id, const T &item) {
    size_t worker_id = map_to_worker(queue_id);
    auto &q_info = queues_info.at(queue_id);
    auto &w_info = workers_info.at(worker_id);
    std::unique_lock<std::mutex> lock(w_info.q_mutex);
    while (q_info.size >= q_info.capacity) {
      q_info.q_not_full.wait(lock);
    }
    w_info.queue.emplace_front(item, queue_id);
    q_info.size++;
    w_info.q_not_empty.notify_one();
  }

  //! Moves \p item to the front of the logical queue with id \p queue_id.
  void push_front(size_t queue_id, T &&item) {
    size_t worker_id = map_to_worker(queue_id);
    auto &q_info = queues_info.at(queue_id);
    auto &w_info = workers_info.at(worker_id);
    std::unique_lock<std::mutex> lock(w_info.q_mutex);
    while (q_info.size >= q_info.capacity) {
      q_info.q_not_full.wait(lock);
    }
    w_info.queue.emplace_front(std::move(item), queue_id);
    q_info.size++;
    w_info.q_not_empty.notify_one();
  }

  //! Retrieves the oldest element for the worker thread indentified by \p
  //! worker_id and moves it to \p pItem. The id of the logical queue which
  //! contained this element is copied to \p queue_id. As a remainder, the
  //! `map_to_worker` argument provided when constructing the class is used to
  //! map every queue id to the corresponding worker id. Therefore, if an
  //! element `E` was pushed to queue `queue_id`, you need to use the worker id
  //! `map_to_worker(queue_id)` to retrieve it with this function.
  void pop_back(size_t worker_id, size_t *queue_id, T *pItem) {
    auto &w_info = workers_info.at(worker_id);
    auto &queue = w_info.queue;
    std::unique_lock<std::mutex> lock(w_info.q_mutex);
    while (queue.size() == 0) {
      w_info.q_not_empty.wait(lock);
    }
    *queue_id = queue.back().queue_id;
    *pItem = std::move(queue.back().e);
    queue.pop_back();
    auto &q_info = queues_info.at(*queue_id);
    q_info.size--;
    q_info.q_not_full.notify_one();
  }

  //! Get the occupancy of the logical queue with id \p queue_id.
  size_t size(size_t queue_id) const {
    size_t worker_id = map_to_worker(queue_id);
    auto &q_info = queues_info.at(queue_id);
    auto &w_info = workers_info.at(worker_id);
    std::unique_lock<std::mutex> lock(w_info.q_mutex);
    return q_info.size;
  }

  //! Set the capacity of the logical queue with id \id queue_id to \p c
  //! elements.
  void set_capacity(size_t queue_id, size_t c) {
    size_t worker_id = map_to_worker(queue_id);
    auto &q_info = queues_info.at(queue_id);
    auto &w_info = workers_info.at(worker_id);
    std::unique_lock<std::mutex> lock(w_info.q_mutex);
    q_info.capacity = c;
  }

  //! Deleted copy constructor
  QueueingLogic(const QueueingLogic &) = delete;
  //! Deleted copy assignment operator
  QueueingLogic &operator =(const QueueingLogic &) = delete;

  //! Deleted move constructor
  QueueingLogic(QueueingLogic &&) = delete;
  //! Deleted move assignment operator
  QueueingLogic &&operator =(QueueingLogic &&) = delete;

 private:
  struct QE {
    QE(T e, size_t queue_id)
        : e(std::move(e)), queue_id(queue_id) { }

    T e;
    size_t queue_id;
  };

  using MyQ = std::deque<QE>;

  struct QueueInfo {
    size_t size{0};
    size_t capacity{0};
    mutable std::condition_variable q_not_full{};
  };

  struct WorkerInfo {
    MyQ queue{};
    mutable std::mutex q_mutex{};
    mutable std::condition_variable q_not_empty{};
  };

  size_t nb_queues;
  size_t nb_workers;
  std::vector<QueueInfo> queues_info;
  std::vector<WorkerInfo> workers_info;
  FMap map_to_worker;
};


//! This class is slightly more advanced than QueueingLogic. The difference
//! between the 2 is that this one offers the ability to rate-limit every
//! logical queue, by providing a maximum number of elements consumed per
//! second. If the rate is too small compared to the incoming packet rate, or if
//! the worker thread cannot sustain the desired rate, elements are buffered in
//! the queue. However, the write behavior (push_front()) for this class is
//! different than the one for QueueingLogic. It is not blocking: if the queue
//! is full, the function will return immediately and the element will not be
//! queued. Look at the documentation for QueueingLogic for more information
//! about the template parameters (they are the same).
//! This is the queueing logic used by the standard simple_switch target.
template <typename T, typename FMap>
class QueueingLogicRL {
 public:
  //! @copydoc QueueingLogic::QueueingLogic()
  //!
  //! Initially, none of the logical queues will be rate-limited, i.e. the
  //! instance will behave as an instance of QueueingLogic.
  QueueingLogicRL(size_t nb_queues, size_t nb_workers, size_t capacity,
                  FMap map_to_worker)
      : nb_queues(nb_queues), nb_workers(nb_workers),
        queues_info(nb_queues), workers_info(nb_workers),
        map_to_worker(std::move(map_to_worker)) {
    auto now = clock::now();
    for (auto &q_info : queues_info) {
      q_info.capacity = capacity;
      q_info.last_sent = now;
    }
  }

  //! If the logical queue qith id \p queue_id is full, the function will return
  //! `0` immediately. Otherwise, \p item will be copied to the front of the
  //! logical queue and the function will return `1`.
  int push_front(size_t queue_id, const T &item) {
    size_t worker_id = map_to_worker(queue_id);
    auto &q_info = queues_info.at(queue_id);
    auto &w_info = workers_info.at(worker_id);
    std::unique_lock<std::mutex> lock(w_info.q_mutex);
    if (q_info.size >= q_info.capacity) return 0;
    q_info.last_sent = get_next_tp(q_info);
    // w_info.queue.emplace(item, queue_id, q_info.last_sent, id++);
    w_info.queue.emplace(item, queue_id, q_info.last_sent);
    q_info.size++;
    w_info.q_not_empty.notify_one();
    return 1;
  }

  //! Same as push_front(size_t queue_id, const T &item), but \p item is moved
  //! instead of copied.
  int push_front(size_t queue_id, T &&item) {
    size_t worker_id = map_to_worker(queue_id);
    auto &q_info = queues_info.at(queue_id);
    auto &w_info = workers_info.at(worker_id);
    std::unique_lock<std::mutex> lock(w_info.q_mutex);
    if (q_info.size >= q_info.capacity) return 0;
    q_info.last_sent = get_next_tp(q_info);
    // w_info.queue.emplace(std::move(item), queue_id, q_info.last_sent, id++);
    w_info.queue.emplace(std::move(item), queue_id, q_info.last_sent);
    q_info.size++;
    w_info.q_not_empty.notify_one();
    return 1;
  }

  //! Retrieves the oldest element for the worker thread indentified by \p
  //! worker_id and moves it to \p pItem. The id of the logical queue which
  //! contained this element is copied to \p queue_id. Note that this function
  //! will block until 1) an element is available 2) this element is free to
  //! leave the queue according to the rate limiter.
  void pop_back(size_t worker_id, size_t *queue_id, T *pItem) {
    auto &w_info = workers_info.at(worker_id);
    auto &queue = w_info.queue;
    using std::chrono::time_point_cast;
    using std::chrono::nanoseconds;
    std::unique_lock<std::mutex> lock(w_info.q_mutex);
    while (true) {
      if (queue.size() == 0) {
        w_info.q_not_empty.wait(lock);
      } else {
        if (queue.top().send <= clock::now()) break;
        w_info.q_not_empty.wait_until(lock, queue.top().send);
      }
    }
    *queue_id = queue.top().queue_id;
    // TODO(antonin): improve / document this
    // http://stackoverflow.com/questions/20149471/move-out-element-of-std-priority-queue-in-c11
    *pItem = std::move(const_cast<QE &>(queue.top()).e);
    queue.pop();
    auto &q_info = queues_info.at(*queue_id);
    q_info.size--;
  }

  //! @copydoc QueueingLogic::size
  size_t size(size_t queue_id) const {
    size_t worker_id = map_to_worker(queue_id);
    auto &q_info = queues_info.at(queue_id);
    auto &w_info = workers_info.at(worker_id);
    std::unique_lock<std::mutex> lock(w_info.q_mutex);
    return q_info.size;
  }

  //! @copydoc QueueingLogic::set_capacity
  void set_capacity(size_t queue_id, size_t c) {
    size_t worker_id = map_to_worker(queue_id);
    auto &q_info = queues_info.at(queue_id);
    auto &w_info = workers_info.at(worker_id);
    std::unique_lock<std::mutex> lock(w_info.q_mutex);
    q_info.capacity = c;
  }

  //! Set the maximum rate of the logical queue with id \p queue_id to \p
  //! pps. \p pps is expressed in "number of elements per second". Until this
  //! function is called, there will be no rate limit for the queue.
  void set_rate(size_t queue_id, uint64_t pps) {
    using std::chrono::duration;
    using std::chrono::duration_cast;
    size_t worker_id = map_to_worker(queue_id);
    auto &q_info = queues_info.at(queue_id);
    auto &w_info = workers_info.at(worker_id);
    std::unique_lock<std::mutex> lock(w_info.q_mutex);
    q_info.queue_rate_pps = pps;
    q_info.pkt_delay_ticks = duration_cast<ticks>(duration<double>(1. / pps));
  }

  //! Deleted copy constructor
  QueueingLogicRL(const QueueingLogicRL &) = delete;
  //! Deleted copy assignment operator
  QueueingLogicRL &operator =(const QueueingLogicRL &) = delete;

  //! Deleted move constructor
  QueueingLogicRL(QueueingLogicRL &&) = delete;
  //! Deleted move assignment operator
  QueueingLogicRL &&operator =(QueueingLogicRL &&) = delete;

 private:
  using ticks = std::chrono::nanoseconds;
  // clock choice? switch to steady if observing re-ordering
  // using clock = std::chrono::steady_clock;
  using clock = std::chrono::high_resolution_clock;

  struct QE {
    // QE(T e, size_t queue_id, const clock::time_point &send, size_t id)
    //     : e(std::move(e)), queue_id(queue_id), send(send), id(id) { }
    QE(T e, size_t queue_id, const clock::time_point &send)
        : e(std::move(e)), queue_id(queue_id), send(send) { }

    T e;
    size_t queue_id;
    clock::time_point send;
    // size_t id;
  };

  struct QEComp {
    bool operator()(const QE &lhs, const QE &rhs) const {
      // return (lhs.send == rhs.send) ? lhs.id > rhs.id : lhs.send > rhs.send;
      return lhs.send > rhs.send;
    }
  };

  // performance seems to be roughly the same for deque vs vector
  using MyQ = std::priority_queue<QE, std::deque<QE>, QEComp>;
  // using MyQ = std::priority_queue<QE, std::vector<QE>, QEComp>;

  struct QueueInfo {
    size_t size{0};
    size_t capacity{0};
    uint64_t queue_rate_pps{};
    // interesting to note that {0} fails with g++4.8, but not with g++4.9
    // did not get to the root of it, but could be because of an explicit
    // constructor somewhere in the std lib implementation used
    ticks pkt_delay_ticks{ticks::zero()};
    clock::time_point last_sent{};
  };

  struct WorkerInfo {
    MyQ queue{};
    mutable std::mutex q_mutex{};
    mutable std::condition_variable q_not_empty{};
  };

  clock::time_point get_next_tp(const QueueInfo &q_info) {
    return std::max(clock::now(), q_info.last_sent + q_info.pkt_delay_ticks);
  }

  size_t nb_queues;
  size_t nb_workers;
  std::vector<QueueInfo> queues_info;
  std::vector<WorkerInfo> workers_info;
  FMap map_to_worker;
  // size_t id{0};
};

}  // namespace bm

#endif  // BM_SIM_INCLUDE_BM_SIM_QUEUEING_H_
