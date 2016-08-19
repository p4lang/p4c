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
#include <chrono>

#include <bm/bm_sim/meters.h>

using namespace bm;

using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::this_thread::sleep_for;
using std::this_thread::sleep_until;

// Google Test fixture for meters tests
class MetersTest : public ::testing::Test {
 protected:
  typedef std::chrono::high_resolution_clock clock;
  typedef Meter::MeterType MeterType;
  typedef Meter::color_t color_t;

 protected:
  PHVFactory phv_factory;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  MetersTest()
      : phv_source(PHVSourceIface::make_phv_source()) { }

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

// 2 rate 3 color marker
TEST_F(MetersTest, trTCM) {
  const color_t GREEN = 0;
  const color_t YELLOW = 1;
  const color_t RED = 2;

  Meter meter(MeterType::PACKETS, 2);
  // committed : 2 packets per second, burst size of 3
  Meter::rate_config_t committed_rate = {0.000002, 3};
  // peak : 10 packets per second, burst size of 1
  Meter::rate_config_t peak_rate = {0.00001, 1};
  meter.set_rates({committed_rate, peak_rate});

  std::vector<color_t> output;
  output.reserve(32);

  Packet pkt = get_pkt(128);

  Meter::reset_global_clock();

  clock::time_point next_stop = clock::now();

  color_t color;
  for(size_t i = 0; i < 9; i++) {
    color = meter.execute(pkt);
    output.push_back(color);
    next_stop += milliseconds(90);
    sleep_until(next_stop);
  }
  for(size_t i = 0; i < 3; i++) {
    color = meter.execute(pkt);
    output.push_back(color);
    next_stop += milliseconds(10);
    sleep_until(next_stop);
  }
  next_stop += milliseconds(200);
  sleep_until(next_stop);
  color = meter.execute(pkt);
  output.push_back(color);

  // I chose a step interval of 90 ms, because I never get to close to multiple
  // of 100ms, which should make the test somewhat robust. However, the test
  // still fails when run with Valgrind, which is why when running the tests
  // with Valgrind, one needs to use the --valgrind flag, which will ensure that
  // this test is skipped
  // Maybe in the future, I will not skip the test when Valgrind is used but
  // instead relax the success conditions (e.g. use bounds on the number of
  // color occurrences

  // expect output
  // i = 0, t = 0ms, GREEN
  // ptoks: 1 - 1 = 0, ctoks: 3 - 1 = 2
  // i = 1, t = 90 ms, RED
  // ptoks: 0, ctoks: 2
  // !!! 100ms -> ptoks: 0 + 1 = 1
  // i = 2, t = 180ms, GREEN
  // ptoks: 1 - 1 = 0, ctoks: 2 - 1 = 1
  // !!! 200ms -> ptoks: 0 + 1 = 1
  // i = 3, t = 270ms, GREEN
  // ptoks: 1 - 1 = 0, ctoks: 1 - 1 = 0
  // !!! 300ms -> ptoks: 0 + 1 = 1
  // i = 4, t = 360ms, YELLOW
  // ptoks: 1 - 1 = 0, ctoks: 0
  // !!! 400ms -> ptoks: 0 + 1 = 1
  // i = 5, t = 450ms, YELLOW
  // ptoks: 1 - 1 = 0, ctoks: 0
  // !!! 500ms -> ptoks: 0 + 1 = 1, ctoks: 0 + 1 = 1
  // i = 6, t = 540ms, GREEN
  // ptoks: 1 - 1 = 0, ctoks: 1 - 1 = 0
  // !!! 600ms -> ptoks: 0 + 1 = 1
  // i = 7, t = 630ms, YELLOW
  // ptoks: 1 - 1 = 0, ctoks: 0
  // !!! 700ms -> ptoks: 0 + 1 = 1
  // i = 8, t = 720ms, YELLOW
  // ptoks: 1 - 1 = 0, ctoks: 0
  // !!! 800ms -> ptoks: 0 + 1 = 1

  // i = 0, t = 810ms, YELLOW
  // ptoks: 1 - 1 = 0, ctoks: 0
  // i = 1, t = 830ms, RED
  // ptoks: 0, ctoks: 0
  // i = 2, t = 850ms, RED
  // ptoks: 0, ctoks: 0

  // then sleep for 200ms, i.e. past 1s so GREEN

  std::vector<color_t> expected = {
    GREEN, RED, GREEN, GREEN, YELLOW, YELLOW, GREEN, YELLOW, YELLOW,
    YELLOW, RED, RED,
    GREEN
  };

  ASSERT_EQ(expected, output);
}

TEST_F(MetersTest, GetRates) {
  Meter meter(MeterType::PACKETS, 2);
  // committed : 2 packets per second, burst size of 3
  Meter::rate_config_t committed_rate = {0.000002, 3};
  // peak : 10 packets per second, burst size of 1
  Meter::rate_config_t peak_rate = {0.00001, 1};
  const std::vector<Meter::rate_config_t> rates = {committed_rate, peak_rate};
  meter.set_rates(rates);

  const auto retrieved_rates = meter.get_rates();
  ASSERT_EQ(rates.size(), retrieved_rates.size());
  for (size_t i = 0; i < rates.size(); i++) {
    ASSERT_EQ(rates[i].info_rate, retrieved_rates[i].info_rate);
    ASSERT_EQ(rates[i].burst_size, retrieved_rates[i].burst_size);
  }
}
