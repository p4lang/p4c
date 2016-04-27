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

#include <random>

#include <bm/bm_sim/counters.h>

using namespace bm;

// Google Test fixture for counter tests
class CountersTest : public ::testing::Test {
 protected:
  typedef Counter::counter_value_t counter_value_t;

 protected:
  PHVFactory phv_factory;

  std::mt19937 gen;
  std::uniform_int_distribution<size_t> dis;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  static constexpr size_t min_pkt_size = 64;
  static constexpr size_t max_pkt_size = 4096;

  CountersTest()
      : dis(min_pkt_size, max_pkt_size),
        phv_source(PHVSourceIface::make_phv_source()) { }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
  }

  // virtual void TearDown() { }

  Packet get_pkt(size_t pkt_size) {
    // dummy packet, won't be parsed
    return Packet::make_new(pkt_size, PacketBuffer(pkt_size * 2),
                            phv_source.get());
  }
};

// Really simple tests, but then it is a really simple object

TEST_F(CountersTest, SimpleTest) {
  counter_value_t bytes, packets;

  Counter c;

  c.query_counter(&bytes, &packets);
  ASSERT_EQ(0u, bytes);
  ASSERT_EQ(0u, packets);

  const size_t nb_pkts = 10;
  size_t byte_count = 0;
  for(size_t i = 0; i < nb_pkts; ++i) {
    const size_t pkt_size = dis(gen);
    const Packet pkt = get_pkt(pkt_size);
    byte_count += pkt_size;

    c.increment_counter(pkt);
    c.query_counter(&bytes, &packets);
    ASSERT_EQ(byte_count, bytes);
    ASSERT_EQ(i + 1, packets);
  }

  c.reset_counter();
  c.query_counter(&bytes, &packets);
  ASSERT_EQ(0u, bytes);
  ASSERT_EQ(0u, packets);
}

// pretty useless ...
TEST_F(CountersTest, CounterArray) {
  counter_value_t bytes, packets;

  CounterArray c_array("counter", 0, 128);

  for(Counter &c : c_array) {
    c.query_counter(&bytes, &packets);
    ASSERT_EQ(0u, bytes);
    ASSERT_EQ(0u, packets);
  }
}
