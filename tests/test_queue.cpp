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

#include <bm/bm_sim/queue.h>

using std::unique_ptr;

using std::thread;

using bm::Queue;

using ::testing::TestWithParam;
using ::testing::Values;
using ::testing::Combine;

class QueueTest : public TestWithParam< std::tuple<size_t, int> > {
protected:
  int iterations;
  size_t queue_size;

  unique_ptr<Queue<int> > queue;
  unique_ptr<int []> values;

  virtual void SetUp() {
    queue_size = std::get<0>(GetParam());
    iterations = std::get<1>(GetParam());

    queue = unique_ptr<Queue<int> >(new Queue<int>(queue_size));
    values = unique_ptr<int []>(new int[iterations]);

    for(int i = 0; i < iterations; i++) {
      values[i] = rand();
    }
  }

public:
  void produce() {
    for(int i = 0; i < iterations; i++) {
      queue->push_front(values[i]);
    }
  }

  // virtual void TearDown() {}
};

static void producer(QueueTest *qt) {
  qt->produce();
}


TEST_P(QueueTest, ProducerConsumer) {

  thread producer_thread(producer, this);

  int value;
  for(int i = 0; i < iterations; i++) {
    queue->pop_back(&value);
    ASSERT_EQ(values[i], value);
  }

  producer_thread.join();
}


INSTANTIATE_TEST_CASE_P(TestParameters,
                        QueueTest,
                        Combine(Values(16, 1024, 20000),
				Values(1000, 200000)));
