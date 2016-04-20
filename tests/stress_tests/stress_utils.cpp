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

#include <iostream>
#include <fstream>
#include <random>

#include <cassert>

#include "stress_utils.h"

namespace stress_tests_utils {

class RandomGenImp {
 public:
  RandomGenImp() { }

  bool get_bool(double p_true) {
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    double number = distribution(generator);
    return (number < p_true);
  }

  int get_int(int a, int b) {
    std::uniform_int_distribution<int> distribution(a, b);
    int number = distribution(generator);
    return number;
  }

 private:
  // TODO(antonin): seed needed?
  std::default_random_engine generator{};
};

RandomGen::RandomGen() {
  imp = std::unique_ptr<RandomGenImp>(new RandomGenImp());
}

RandomGen::~RandomGen() { }

bool
RandomGen::get_bool(double p_true) {
  return imp->get_bool(p_true);
}

int
RandomGen::get_int(int a, int b) {
  return imp->get_int(a, b);
}


std::vector<std::unique_ptr<bm::Packet> >
SwitchTest::read_traffic(const std::string &path) {
  std::vector<std::unique_ptr<bm::Packet> > packets;

  std::ifstream fs(path, std::ios::in);
  assert(fs);
  size_t packet_cnt, packet_size;
  fs >> packet_cnt >> packet_size;
  packets.reserve(packet_cnt);
  assert(fs.peek() == '\n');
  fs.ignore();  // new line
  std::vector<char> buffer(packet_size);
  for (size_t i = 0; i < packet_cnt; i++) {
    fs.read(&buffer[0], packet_size);
    packets.push_back(new_packet_ptr(
        0, 0, i, packet_size,
        bm::PacketBuffer(packet_size, buffer.data(), packet_size)));
  }

  std::cout << "Imported " << packet_cnt << " packets of size " << packet_size
            << " (" << (packet_cnt * packet_size) << " bytes)\n";

  return packets;
}


TestChrono::TestChrono(size_t packet_cnt)
    : packet_cnt(packet_cnt) { }

void
TestChrono::start() {
  std::cout << "Processing " << packet_cnt << " packets...\n";
  start_tp = clock::now();
}

void
TestChrono::end() {
  end_tp = clock::now();
  std::cout << "Done.\n";
}

void
TestChrono::print_summary() {
  unsigned int elapsed = duration_cast<milliseconds>(end_tp - start_tp).count();
  std::cout << "Test took " << elapsed << " ms.\n";
  std::cout << "Switch was processing "
            << (packet_cnt * 1000) / elapsed
            << " packets per second.\n";
}

}  // namespace stress_tests_utils
