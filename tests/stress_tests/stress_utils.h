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

#ifndef TESTS_STRESS_TESTS_UTILS_H_
#define TESTS_STRESS_TESTS_UTILS_H_

#include <bm/bm_sim/switch.h>

#include <chrono>
#include <vector>
#include <memory>
#include <string>

using std::chrono::milliseconds;
using std::chrono::duration_cast;

namespace stress_tests_utils {

class RandomGenImp;

class RandomGen {
 public:
  RandomGen();
  ~RandomGen();

  bool get_bool(double p_true);
  int get_int(int a, int b);

 private:
  std::unique_ptr<RandomGenImp> imp;
};

class SwitchTest : public bm::Switch {
 public:
  int receive(int port_num, const char *buffer, int len) override {
    (void) port_num; (void) buffer; (void) len;
    return 0;
  }

  void start_and_return() override {
  }

  // using pointers as most targets are expected to do that
  std::vector<std::unique_ptr<bm::Packet> > read_traffic(
      const std::string &path);
};

class TestChrono {
 public:
  typedef std::chrono::high_resolution_clock clock;

  TestChrono(size_t packet_cnt);

  void start();
  void end();

  void print_summary();

 private:
  size_t packet_cnt;
  clock::time_point start_tp{};
  clock::time_point end_tp{};
};

}  // namespace stress_tests_utils

#endif  // TESTS_STRESS_TESTS_UTILS_H_
