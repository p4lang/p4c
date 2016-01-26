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

#include <gtest/gtest.h>

#include <thread>
#include <memory>
#include <vector>

#include "bm_sim/queueing.h"

using std::unique_ptr;

using std::thread;

using bm::QueueingLogic;
using bm::QueueingLogicRL;

struct WorkerMapper {
  WorkerMapper(size_t nb_workers)
      : nb_workers(nb_workers) { }

  size_t operator()(size_t queue_id) const {
    return queue_id % nb_workers;
  }

  size_t nb_workers;
};

struct RndInput {
  size_t queue_id;
  int v;
};

template <class QType>
class QueueingTest : public ::testing::Test {
protected:
  static constexpr size_t nb_queues = 3u;
  static constexpr size_t nb_workers = 2u;
  static constexpr size_t capacity = 128u;
  static constexpr size_t iterations = 200000;
  // QueueingLogic<T, WorkerMapper> queue;
  QType queue;
  std::vector<RndInput> values;

  QueueingTest()
      : queue(nb_queues, nb_workers, capacity, WorkerMapper(nb_workers)),
        values(iterations) { }

  virtual void SetUp() {
    for(size_t i = 0; i < iterations; i++) {
      values[i] = {rand() % nb_queues, rand()};
    }
  }

 public:
  void produce();

  // virtual void TearDown() {}
};

typedef std::unique_ptr<int> QEm;

template <typename QType>
void QueueingTest<QType>::produce() {
  for(size_t i = 0; i < iterations; i++) {
    queue.push_front(values[i].queue_id,
                     unique_ptr<int>(new int(values[i].v)));
  }
}

template <>
void QueueingTest<QueueingLogicRL<QEm, WorkerMapper> >::produce() {
  for(size_t i = 0; i < iterations; i++) {
    size_t queue_id = values[i].queue_id;
    // we have a dropping behavior for QueueingLogicRL when the queue is full
    // this is to avoid drops
    // kind of makes me question if using type parameterization is really useful
    // for this
    while (queue.size(queue_id) > capacity / 2) {
      // originally, I just had an empty loop, but Valgrind was running forever
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    queue.push_front(queue_id, unique_ptr<int>(new int(values[i].v)));
  }
}

using testing::Types;

typedef Types<QueueingLogic<QEm, WorkerMapper>,
              QueueingLogicRL<QEm, WorkerMapper> > QueueingTypes;

TYPED_TEST_CASE(QueueingTest, QueueingTypes);

TYPED_TEST(QueueingTest, ProducerConsummer) {
  thread producer_thread(&QueueingTest<TypeParam>::produce, this);

  WorkerMapper mapper(this->nb_workers);

  for(size_t i = 0; i < this->iterations; i++) {
    size_t queue_id;
    size_t expected_queue_id = this->values[i].queue_id;
    unique_ptr<int> v;
    size_t worker_id = mapper(expected_queue_id);
    this->queue.pop_back(worker_id, &queue_id, &v);
    ASSERT_EQ(expected_queue_id, queue_id);
    ASSERT_EQ(this->values[i].v, *v);
  }

  producer_thread.join();
}


class QueueingRLTest : public ::testing::Test {
protected:
  typedef std::unique_ptr<int> T;
  static constexpr size_t nb_queues = 1u;
  static constexpr size_t nb_workers = 1u;
  static constexpr size_t capacity = 1024u;
  static constexpr size_t iterations = 512u;
  static constexpr uint64_t pps = 100u;
  QueueingLogicRL<T, WorkerMapper> queue;
  std::vector<RndInput> values;

  static_assert(capacity >= iterations,
                "for RL test, capacity needs to be greater or equal to # pkts");

  QueueingRLTest()
      : queue(nb_queues, nb_workers, capacity, WorkerMapper(nb_workers)),
        values(iterations) { }

  virtual void SetUp() {
    for(size_t i = 0; i < iterations; i++) {
      values[i] = {rand() % nb_queues, rand()};
    }

    queue.set_rate(0u, pps);
  }

 public:
  void produce() {
    for(size_t i = 0; i < iterations; i++) {
      queue.push_front(values[i].queue_id,
                       unique_ptr<int>(new int(values[i].v)));
    }
  }

  // virtual void TearDown() {}
};

TEST_F(QueueingRLTest, RateLimiter) {
  thread producer_thread(&QueueingRLTest::produce, this);

  using std::chrono::duration_cast;
  using std::chrono::milliseconds;
  using clock = std::chrono::high_resolution_clock;

  std::vector<int> times;
  times.reserve(iterations);

  auto start = clock::now();

  for(size_t i = 0; i < iterations; i++) {
    size_t queue_id;
    unique_ptr<int> v;
    this->queue.pop_back(0u, &queue_id, &v);
    ASSERT_EQ(0u, queue_id);
    ASSERT_EQ(values[i].v, *v);

    auto now = clock::now();
    times.push_back(duration_cast<milliseconds>(now - start).count());
  }

  producer_thread.join();

  int elapsed = times.back();
  int expected = (iterations * 1000) / pps;

  ASSERT_GT(elapsed, expected * 0.9);
  ASSERT_LT(elapsed, expected * 1.1);

  // TODO(antonin): better check of times vector?
}
