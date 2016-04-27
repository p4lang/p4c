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
 * Srikrishna Gopu (krishna@barefootnetworks.com)
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#ifndef BM_BM_SIM_PRE_H_
#define BM_BM_SIM_PRE_H_

#include <string>
#include <bitset>

namespace bm {

namespace McPre {

template <size_t set_size>
class Set {
 public:
  typedef typename std::bitset<set_size>::reference reference;

 public:
  constexpr Set() noexcept { }

  explicit Set(const std::string &str)
    : port_map(str) { }

  bool operator[] (size_t pos) const { return port_map[pos]; }
  reference operator[] (size_t pos) { return port_map[pos]; }

  constexpr size_t size() const noexcept { return port_map.size(); }

 private:
  std::bitset<set_size> port_map{};
};

}  // namespace McPre

}  // namespace bm

#endif  // BM_BM_SIM_PRE_H_
