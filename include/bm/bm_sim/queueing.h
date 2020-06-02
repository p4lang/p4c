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

#ifndef BM_BM_SIM_QUEUEING_H_
#define BM_BM_SIM_QUEUEING_H_

#include <algorithm>  // for std::max
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <queue>
#include <tuple>  // for std::forward_as_tuple
#include <unordered_map>
#include <utility>  // for std::piecewise_construct
#include <vector>

namespace bm {

// These queueing implementations used to have one lock for each worker, which
// meant that as long as 2 queues were assigned to different workers, they could
// operate (push / pop) in parallel. Since we added support for arbitrary port
// ids (and the port id is used as the queue id), we no longer have a reasonable
// upper bound on the maximum possible port id at construction time and we can
// no longer use a vector indexed by the queue id to store queue
// information. Each push / pop operation can potentially insert a new entry
// into the map. In order to accomodate for this, we had to start using a single
// lock, shared by all the workers. It's unlikely that contention for this lock
// will be a bottleneck.

//! One of the most basic queueing block possible. Supports an arbitrary number
//! of logical queues (identified by arbitrary integer ids). Lets you choose (at
//! runtime) the number of worker threads that will be reading from these
//! queues. I write "logical queues" because the implementation actually uses as
//! many physical queues as there are worker threads. However, each logical
//! queue still has its own maximum capacity.  As of now, the behavior is
//! blocking for both read (pop_back()) and write (push_front()), but we may
//! offer additional options if there is interest expressed in the future.
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
  using MutexType = std::mutex;
  using LockType = std::unique_lock<MutexType>;

 public:
  //! \p nb_workers is the number of threads that will be consuming from the
  //! queues; they will be identified by an id in the range `[0,
  //! nb_workers)`. \p capacity is the number of objects that each logical queue
  //! can hold. Because we need to be able to map each queue id to a worker id,
  //! the user has to provide a callable object of type `FMap`, \p
  //! map_to_worker, that can do this mapping. See the QueueingLogic class
  //! description for more information about the `FMap` template parameter.
  QueueingLogic(size_t nb_workers, size_t capacity, FMap map_to_worker)
      : nb_workers(nb_workers),
        capacity(capacity),
        workers_info(nb_workers),
        map_to_worker(std::move(map_to_worker)) { }

  //! Makes a copy of \p item and pushes it to the front of the logical queue
  //! with id \p queue_id.
  void push_front(size_t queue_id, const T &item) {
    size_t worker_id = map_to_worker(queue_id);
    LockType lock(mutex);
    auto &q_info = get_queue(queue_id);
    auto &w_info = workers_info.at(worker_id);
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
    LockType lock(mutex);
    auto &q_info = get_queue(queue_id);
    auto &w_info = workers_info.at(worker_id);
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
    LockType lock(mutex);
    auto &w_info = workers_info.at(worker_id);
    auto &queue = w_info.queue;
    while (queue.size() == 0) {
      w_info.q_not_empty.wait(lock);
    }
    *queue_id = queue.back().queue_id;
    *pItem = std::move(queue.back().e);
    queue.pop_back();
    auto &q_info = get_queue_or_throw(*queue_id);
    q_info.size--;
    q_info.q_not_full.notify_one();
  }

  //! Get the occupancy of the logical queue with id \p queue_id.
  size_t size(size_t queue_id) const {
    LockType lock(mutex);
    auto it = queues_info.find(queue_id);
    if (it == queues_info.end()) return 0;
    auto &q_info = it->second;
    return q_info.size;
  }

  //! Set the capacity of the logical queue with id \p queue_id to \p c
  //! elements.
  void set_capacity(size_t queue_id, size_t c) {
    LockType lock(mutex);
    auto &q_info = get_queue(queue_id);
    q_info.capacity = c;
  }

  //! Set the capacity of all logical queues to \p c elements.
  void set_capacity_for_all(size_t c) {
    LockType lock(mutex);
    for (auto &p : queues_info) p.second.capacity = c;
    capacity = c;
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
    explicit QueueInfo(size_t capacity)
        : capacity(capacity) { }

    size_t size{0};
    size_t capacity{0};
    mutable std::condition_variable q_not_full{};
  };

  struct WorkerInfo {
    MyQ queue{};
    mutable std::condition_variable q_not_empty{};
  };

  QueueInfo &get_queue(size_t queue_id) {
    auto it = queues_info.find(queue_id);
    if (it != queues_info.end()) return it->second;
    // piecewise_construct because QueueInfo is not copyable (because of mutex
    // member)
    auto p = queues_info.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(queue_id),
        std::forward_as_tuple(capacity));
    return p.first->second;
  }

  const QueueInfo &get_queue_or_throw(size_t queue_id) const {
    return queues_info.at(queue_id);
  }

  QueueInfo &get_queue_or_throw(size_t queue_id) {
    return queues_info.at(queue_id);
  }

  mutable MutexType mutex{};
  size_t nb_workers;
  size_t capacity;  // default capacity
  std::unordered_map<size_t, QueueInfo> queues_info;
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
  using MutexType = std::mutex;
  using LockType = std::unique_lock<MutexType>;

 public:
  //! @copydoc QueueingLogic::QueueingLogic()
  //!
  //! Initially, none of the logical queues will be rate-limited, i.e. the
  //! instance will behave as an instance of QueueingLogic.
  QueueingLogicRL(size_t nb_workers, size_t capacity, FMap map_to_worker)
      : nb_workers(nb_workers),
        capacity(capacity),
        workers_info(nb_workers),
        map_to_worker(std::move(map_to_worker)) { }

  //! If the logical queue with id \p queue_id is full, the function will return
  //! `0` immediately. Otherwise, \p item will be copied to the front of the
  //! logical queue and the function will return `1`.
  int push_front(size_t queue_id, const T &item) {
    size_t worker_id = map_to_worker(queue_id);
    LockType lock(mutex);
    auto &q_info = get_queue(queue_id);
    auto &w_info = workers_info.at(worker_id);
    if (q_info.size >= q_info.capacity) return 0;
    q_info.last_sent = get_next_tp(q_info);
    w_info.queue.emplace(
        item, queue_id, q_info.last_sent, w_info.wrapping_counter++);
    q_info.size++;
    w_info.q_not_empty.notify_one();
    return 1;
  }

  //! Same as push_front(size_t queue_id, const T &item), but \p item is moved
  //! instead of copied.
  int push_front(size_t queue_id, T &&item) {
    size_t worker_id = map_to_worker(queue_id);
    LockType lock(mutex);
    auto &q_info = get_queue(queue_id);
    auto &w_info = workers_info.at(worker_id);
    if (q_info.size >= q_info.capacity) return 0;
    q_info.last_sent = get_next_tp(q_info);
    w_info.queue.emplace(
        std::move(item), queue_id, q_info.last_sent, w_info.wrapping_counter++);
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
    LockType lock(mutex);
    auto &w_info = workers_info.at(worker_id);
    auto &queue = w_info.queue;
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
    auto &q_info = get_queue_or_throw(*queue_id);
    q_info.size--;
  }

  //! @copydoc QueueingLogic::size
  size_t size(size_t queue_id) const {
    LockType lock(mutex);
    auto it = queues_info.find(queue_id);
    if (it == queues_info.end()) return 0;
    auto &q_info = it->second;
    return q_info.size;
  }

  //! Set the capacity of the logical queue with id \p queue_id to \p c
  //! elements.
  void set_capacity(size_t queue_id, size_t c) {
    LockType lock(mutex);
    auto &q_info = get_queue(queue_id);
    q_info.capacity = c;
  }

  //! Set the capacity of all logical queues to \p c elements.
  void set_capacity_for_all(size_t c) {
    LockType lock(mutex);
    for (auto &p : queues_info) p.second.capacity = c;
    capacity = c;
  }

  //! Set the maximum rate of the logical queue with id \p queue_id to \p
  //! pps. \p pps is expressed in "number of elements per second". Until this
  //! function is called, there will be no rate limit for the queue. The same
  //! behavior (no rate limit) can be achieved by calling this method with a
  //! rate of 0.
  void set_rate(size_t queue_id, uint64_t pps) {
    LockType lock(mutex);
    auto &q_info = get_queue(queue_id);
    q_info.queue_rate_pps = pps;
    q_info.pkt_delay_ticks = rate_to_ticks(pps);
  }

  //! Set the maximum rate of all logical queues to \p pps.
  void set_rate_for_all(uint64_t pps) {
    using std::chrono::duration;
    using std::chrono::duration_cast;
    LockType lock(mutex);
    for (auto &p : queues_info) {
      auto &q_info = p.second;
      q_info.queue_rate_pps = pps;
      q_info.pkt_delay_ticks = rate_to_ticks(pps);
    }
    queue_rate_pps = pps;
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

  static constexpr ticks rate_to_ticks(uint64_t pps) {
    using std::chrono::duration;
    using std::chrono::duration_cast;
    return (pps == 0) ?
        ticks(0) : duration_cast<ticks>(duration<double>(1. / pps));
  }

  struct QE {
    // QE(T e, size_t queue_id, const clock::time_point &send, size_t id)
    //     : e(std::move(e)), queue_id(queue_id), send(send), id(id) { }
    QE(T e, size_t queue_id, const clock::time_point &send, size_t id)
        : e(std::move(e)), queue_id(queue_id), send(send), id(id) { }

    T e;
    size_t queue_id;
    clock::time_point send;
    size_t id;
  };

  struct QEComp {
    bool operator()(const QE &lhs, const QE &rhs) const {
      // the point of the id is to avoid re-orderings when the send timestamp is
      // the same for 2 items, which seems to happen (when the pps rate is 0)
      // with both the steady_clock and the high_resolution_clock on my Linux
      // VM.
      return (lhs.send == rhs.send) ? lhs.id > rhs.id : lhs.send > rhs.send;
    }
  };

  // performance seems to be roughly the same for deque vs vector
  using MyQ = std::priority_queue<QE, std::deque<QE>, QEComp>;
  // using MyQ = std::priority_queue<QE, std::vector<QE>, QEComp>;

  struct QueueInfo {
    QueueInfo(size_t capacity, uint64_t queue_rate_pps)
        : capacity(capacity),
          queue_rate_pps(queue_rate_pps),
          pkt_delay_ticks(rate_to_ticks(queue_rate_pps)),
          last_sent(clock::now()) { }

    size_t size{0};
    size_t capacity;
    uint64_t queue_rate_pps;
    ticks pkt_delay_ticks;
    clock::time_point last_sent;
  };

  struct WorkerInfo {
    MyQ queue{};
    mutable std::condition_variable q_not_empty{};
    size_t wrapping_counter{0};
  };

  QueueInfo &get_queue(size_t queue_id) {
    auto it = queues_info.find(queue_id);
    if (it != queues_info.end()) return it->second;
    auto p = queues_info.emplace(queue_id, QueueInfo(capacity, queue_rate_pps));
    return p.first->second;
  }

  const QueueInfo &get_queue_or_throw(size_t queue_id) const {
    return queues_info.at(queue_id);
  }

  QueueInfo &get_queue_or_throw(size_t queue_id) {
    return queues_info.at(queue_id);
  }

  clock::time_point get_next_tp(const QueueInfo &q_info) {
    return std::max(clock::now(), q_info.last_sent + q_info.pkt_delay_ticks);
  }

  mutable MutexType mutex{};
  size_t nb_workers;
  size_t capacity;  // default capacity
  uint64_t queue_rate_pps{0};  // default rate
  std::unordered_map<size_t, QueueInfo> queues_info;
  std::vector<WorkerInfo> workers_info;
  FMap map_to_worker;
};


//! This class is slightly more advanced than QueueingLogicRL. The difference
//! between the 2 is that this one offers the ability to set several priority
//! queues for each logical queue. Priority queues are numbered from `0` to
//! `nb_priorities` (see QueueingLogicPriRL::QueueingLogicPriRL()). Priority `0`
//! is the highest priority queue. Each priority queue can have its own rate and
//! its own capacity. Queues will be served in order of priority, until their
//! respective maximum rate is reached. If no maximum rate is set, queues with a
//! high priority can starve lower-priority queues. For example, if the queue
//! with priority `0` always contains at least one element, the other queues
//! will never be served.
//! As for QueueingLogicRL, the write behavior (push_front()) is not blocking:
//! once a logical queue is full, subsequent incoming elements will be dropped
//! until the queue starts draining again.
//! Look at the documentation for QueueingLogic for more information about the
//! template parameters (they are the same).
template <typename T, typename FMap>
class QueueingLogicPriRL {
  using MutexType = std::mutex;
  using LockType = std::unique_lock<MutexType>;

 public:
  //! See QueueingLogic::QueueingLogicRL() for an introduction. The difference
  //! here is that each logical queue can receive several priority queues (as
  //! determined by \p nb_priorities, which is set to `2` by default). Each of
  //! these priority queues will initially be able to hold \p capacity
  //! elements. The capacity of each priority queue can be changed later by
  //! using set_capacity(size_t queue_id, size_t priority, size_t c).
  QueueingLogicPriRL(size_t nb_workers, size_t capacity,
                     FMap map_to_worker, size_t nb_priorities = 2)
      : nb_workers(nb_workers),
        capacity(capacity),
        workers_info(nb_workers),
        map_to_worker(std::move(map_to_worker)),
        nb_priorities(nb_priorities) { }

  //! If priority queue \p priority of logical queue \p queue_id is full, the
  //! function will return `0` immediately. Otherwise, \p item will be copied to
  //! the queue and the function will return `1`. If \p queue_id or \p priority
  //! are incorrect, an exception of type std::out_of_range will be thrown (same
  //! if the FMap object provided to the constructor does not behave correctly).
  int push_front(size_t queue_id, size_t priority, const T &item) {
    size_t worker_id = map_to_worker(queue_id);
    LockType lock(mutex);
    auto &q_info = get_queue(queue_id);
    auto &w_info = workers_info.at(worker_id);
    auto &q_info_pri = q_info.at(priority);
    if (q_info_pri.size >= q_info_pri.capacity) return 0;
    q_info_pri.last_sent = get_next_tp(q_info_pri);
    w_info.queues[priority].emplace(
        item, queue_id, q_info_pri.last_sent, w_info.wrapping_counter++);
    q_info_pri.size++;
    q_info.size++;
    w_info.size++;
    w_info.q_not_empty.notify_one();
    return 1;
  }

  int push_front(size_t queue_id, const T &item) {
    return push_front(queue_id, 0, item);
  }

  //! Same as push_front(size_t queue_id, size_t priority, const T &item), but
  //! \p item is moved instead of copied.
  int push_front(size_t queue_id, size_t priority, T &&item) {
    size_t worker_id = map_to_worker(queue_id);
    LockType lock(mutex);
    auto &q_info = get_queue(queue_id);
    auto &w_info = workers_info.at(worker_id);
    auto &q_info_pri = q_info.at(priority);
    if (q_info_pri.size >= q_info_pri.capacity) return 0;
    q_info_pri.last_sent = get_next_tp(q_info_pri);
    w_info.queues[priority].emplace(
        std::move(item),
        queue_id,
        q_info_pri.last_sent,
        w_info.wrapping_counter++);
    q_info_pri.size++;
    q_info.size++;
    w_info.size++;
    w_info.q_not_empty.notify_one();
    return 1;
  }

  int push_front(size_t queue_id, T &&item) {
    return push_front(queue_id, 0, std::move(item));
  }

  //! Retrieves an element for the worker thread indentified by \p worker_id and
  //! moves it to \p pItem. The id of the logical queue which contained this
  //! element is copied to \p queue_id and the priority value of the served
  //! queue is copied to \p priority.
  //! Elements are retrieved according to the priority queue they are in
  //! (highest priorities, i.e. lowest priority values, are served first). Once
  //! a given priority queue reaches its maximum rate, the next queue is served.
  //! If no elements are available (either the queues are empty or they have
  //! exceeded their rate already), the function will block.
  void pop_back(size_t worker_id, size_t *queue_id, size_t *priority,
                T *pItem) {
    LockType lock(mutex);
    auto &w_info = workers_info.at(worker_id);
    MyQ *queue = nullptr;
    size_t pri;
    while (true) {
      if (w_info.size == 0) {
        w_info.q_not_empty.wait(lock);
      } else {
        auto now = clock::now();
        auto next = clock::time_point::max();
        for (pri = 0; pri < nb_priorities; pri++) {
          auto &q = w_info.queues[pri];
          if (q.size() == 0) continue;
          if (q.top().send <= now) {
            queue = &q;
            break;
          }
          next = std::min(next, q.top().send);
        }
        if (queue) break;
        w_info.q_not_empty.wait_until(lock, next);
      }
    }
    *queue_id = queue->top().queue_id;
    *priority = pri;
    // TODO(antonin): improve / document this
    // http://stackoverflow.com/questions/20149471/move-out-element-of-std-priority-queue-in-c11
    *pItem = std::move(const_cast<QE &>(queue->top()).e);
    queue->pop();
    auto &q_info = get_queue_or_throw(*queue_id);
    auto &q_info_pri = q_info.at(*priority);
    q_info_pri.size--;
    q_info.size--;
    w_info.size--;
  }

  //! Same as
  //! pop_back(size_t worker_id, size_t *queue_id, size_t *priority, T *pItem),
  //! but the priority of the popped element is discarded.
  void pop_back(size_t worker_id, size_t *queue_id, T *pItem) {
    size_t priority;
    return pop_back(worker_id, queue_id, &priority, pItem);
  }

  //! @copydoc QueueingLogic::size
  //! The occupancies of all the priority queues for this logical queue are
  //! added.
  size_t size(size_t queue_id) const {
    LockType lock(mutex);
    auto it = queues_info.find(queue_id);
    if (it == queues_info.end()) return 0;
    auto &q_info = it->second;
    return q_info.size;
  }

  //! Get the occupancy of priority queue \p priority for logical queue with id
  //! \p queue_id.
  size_t size(size_t queue_id, size_t priority) const {
    LockType lock(mutex);
    auto it = queues_info.find(queue_id);
    if (it == queues_info.end()) return 0;
    auto &q_info = it->second;
    auto &q_info_pri = q_info.at(priority);
    return q_info_pri.size;
  }

  //! Set the capacity of all the priority queues for logical queue \p queue_id
  //! to \p c elements.
  void set_capacity(size_t queue_id, size_t c) {
    LockType lock(mutex);
    for_each_q(queue_id, SetCapacityFn(c));
  }

  //! Set the capacity of priority queue \p priority for logical queue \p
  //! queue_id to \p c elements.
  void set_capacity(size_t queue_id, size_t priority, size_t c) {
    LockType lock(mutex);
    for_one_q(queue_id, priority, SetCapacityFn(c));
  }

  //! Set the capacity of all the priority queues of all logical queues to \p c
  //! elements.
  void set_capacity_for_all(size_t c) {
    LockType lock(mutex);
    for (auto &p : queues_info) for_each_q(p.first, SetCapacityFn(c));
    capacity = c;
  }

  //! Set the maximum rate of all the priority queues for logical queue \p
  //! queue_id to \p pps. \p pps is expressed in "number of elements per
  //! second". Until this function is called, there will be no rate limit for
  //! the queue. The same behavior (no rate limit) can be achieved by calling
  //! this method with a rate of 0.
  void set_rate(size_t queue_id, uint64_t pps) {
    LockType lock(mutex);
    for_each_q(queue_id, SetRateFn(pps));
  }

  //! Same as set_rate(size_t queue_id, uint64_t pps) but only applies to the
  //! given priority queue.
  void set_rate(size_t queue_id, size_t priority, uint64_t pps) {
    LockType lock(mutex);
    for_one_q(queue_id, priority, SetRateFn(pps));
  }

  //! Set the rate of all the priority queues of all logical queues to \p pps.
  void set_rate_for_all(uint64_t pps) {
    LockType lock(mutex);
    for (auto &p : queues_info) for_each_q(p.first, SetRateFn(pps));
    queue_rate_pps = pps;
  }

  //! Deleted copy constructor
  QueueingLogicPriRL(const QueueingLogicPriRL &) = delete;
  //! Deleted copy assignment operator
  QueueingLogicPriRL &operator =(const QueueingLogicPriRL &) = delete;

  //! Deleted move constructor
  QueueingLogicPriRL(QueueingLogicPriRL &&) = delete;
  //! Deleted move assignment operator
  QueueingLogicPriRL &&operator =(QueueingLogicPriRL &&) = delete;

 private:
  using ticks = std::chrono::nanoseconds;
  // clock choice? switch to steady if observing re-ordering
  // in my Linux VM, it seems that both clocks behave the same (can sometimes
  // stop increasing for a bit but do not go backwards).
  // using clock = std::chrono::steady_clock;
  using clock = std::chrono::high_resolution_clock;

  static constexpr ticks rate_to_ticks(uint64_t pps) {
    using std::chrono::duration;
    using std::chrono::duration_cast;
    return (pps == 0) ?
        ticks(0) : duration_cast<ticks>(duration<double>(1. / pps));
  }

  struct QE {
    QE(T e, size_t queue_id, const clock::time_point &send, size_t id)
        : e(std::move(e)), queue_id(queue_id), send(send), id(id) { }

    T e;
    size_t queue_id;
    clock::time_point send;
    size_t id;
  };

  struct QEComp {
    bool operator()(const QE &lhs, const QE &rhs) const {
      return (lhs.send == rhs.send) ? lhs.id > rhs.id : lhs.send > rhs.send;
    }
  };

  using MyQ = std::priority_queue<QE, std::deque<QE>, QEComp>;

  struct QueueInfoPri {
    QueueInfoPri(size_t capacity, uint64_t queue_rate_pps)
        : capacity(capacity),
          queue_rate_pps(queue_rate_pps),
          pkt_delay_ticks(rate_to_ticks(queue_rate_pps)),
          last_sent(clock::now()) { }

    size_t size{0};
    size_t capacity;
    uint64_t queue_rate_pps;
    ticks pkt_delay_ticks;
    clock::time_point last_sent;
  };

  struct QueueInfo : public std::vector<QueueInfoPri> {
    QueueInfo(size_t capacity, uint64_t queue_rate_pps, size_t nb_priorities)
        : std::vector<QueueInfoPri>(
              nb_priorities, QueueInfoPri(capacity, queue_rate_pps)) { }

    size_t size{0};
  };

  struct WorkerInfo {
    mutable std::condition_variable q_not_empty{};
    size_t size{0};
    std::array<MyQ, 32> queues;
    size_t wrapping_counter{0};
  };

  QueueInfo &get_queue(size_t queue_id) {
    auto it = queues_info.find(queue_id);
    if (it != queues_info.end()) return it->second;
    auto p = queues_info.emplace(
        queue_id, QueueInfo(capacity, queue_rate_pps, nb_priorities));
    return p.first->second;
  }

  const QueueInfo &get_queue_or_throw(size_t queue_id) const {
    return queues_info.at(queue_id);
  }

  QueueInfo &get_queue_or_throw(size_t queue_id) {
    return queues_info.at(queue_id);
  }

  clock::time_point get_next_tp(const QueueInfoPri &q_info_pri) {
    return std::max(clock::now(),
                    q_info_pri.last_sent + q_info_pri.pkt_delay_ticks);
  }

  template <typename Function>
  Function for_each_q(size_t queue_id, Function fn) {
    auto &q_info = get_queue(queue_id);
    for (auto &q_info_pri : q_info) fn(q_info_pri);
    return std::move(fn);
  }

  template <typename Function>
  Function for_one_q(size_t queue_id, size_t priority, Function fn) {
    auto &q_info = get_queue(queue_id);
    auto &q_info_pri = q_info.at(priority);
    fn(q_info_pri);
    return std::move(fn);
  }

  struct SetCapacityFn {
    explicit SetCapacityFn(size_t c)
        : c(c) { }

    void operator ()(QueueInfoPri &info) const {  // NOLINT(runtime/references)
      info.capacity = c;
    }

    size_t c;
  };

  struct SetRateFn {
    explicit SetRateFn(uint64_t pps)
        : pps(pps) {
      using std::chrono::duration;
      using std::chrono::duration_cast;
      pkt_delay_ticks = rate_to_ticks(pps);
    }

    void operator ()(QueueInfoPri &info) const {  // NOLINT(runtime/references)
      info.queue_rate_pps = pps;
      info.pkt_delay_ticks = pkt_delay_ticks;
    }

    uint64_t pps;
    ticks pkt_delay_ticks;
  };

  mutable MutexType mutex;
  size_t nb_workers;
  size_t capacity;  // default capacity
  uint64_t queue_rate_pps{0};  // default rate
  std::unordered_map<size_t, QueueInfo> queues_info{};
  std::vector<WorkerInfo> workers_info{};
  std::vector<MyQ> queues{};
  FMap map_to_worker;
  size_t nb_priorities;
};

}  // namespace bm

#endif  // BM_BM_SIM_QUEUEING_H_
