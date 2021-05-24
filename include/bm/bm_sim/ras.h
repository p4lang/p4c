/* Copyright 2013-present Barefoot Networks, Inc.
 * Copyright 2021 VMware, Inc.
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

#ifndef BM_BM_SIM_RAS_H_
#define BM_BM_SIM_RAS_H_

#include <bm/config.h>

#include <algorithm>  // for std::swap
#include <utility>

#include <cassert>

#include <boost/container/flat_set.hpp>

namespace bm {

class RandAccessUIntSet {
 public:
  using mbr_t = uintptr_t;
  using container = boost::container::flat_set<mbr_t>;
  using iterator = container::iterator;
  using const_iterator = container::const_iterator;

  RandAccessUIntSet() = default;

  RandAccessUIntSet(const RandAccessUIntSet &other) = delete;
  RandAccessUIntSet &operator=(const RandAccessUIntSet &other) = delete;

  RandAccessUIntSet(RandAccessUIntSet &&other) = default;
  RandAccessUIntSet &operator=(RandAccessUIntSet &&other) = default;

  // returns 0 if already present (0 element added), 1 otherwise
  int add(mbr_t mbr) {
    auto p = members.insert(mbr);
    return p.second ? 1 : 0;
  }

  // returns 0 if not present (0 element removed), 1 otherwise
  int remove(mbr_t mbr) {
    auto c = members.erase(mbr);
    return static_cast<int>(c);
  }

  bool contains(mbr_t mbr) const {
    return members.find(mbr) != members.end();
  }

  size_t count() const {
    return members.size();
  }

  // n >= 0, 0 is first element in set
  mbr_t get_nth(size_t n) const {
    auto it = members.nth(n);
    return *it;
  }

  iterator begin() {
    return members.begin();
  }

  const_iterator begin() const {
    return members.begin();
  }

  iterator end() {
    return members.end();
  }

  const_iterator end() const {
    return members.end();
  }

 private:
  container members;
};

}  // namespace bm

#endif  // BM_BM_SIM_RAS_H_
