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
 * Gordon Bailey (gb@gordonbailey.net)
 *
 */

#ifndef BM_BM_SIM_MATCH_KEY_TYPES_H_
#define BM_BM_SIM_MATCH_KEY_TYPES_H_

#include <limits>
#include <vector>

#include "bytecontainer.h"

namespace bm {

typedef uintptr_t internal_handle_t;
typedef uint32_t entry_handle_t;

enum class MatchUnitType {
  EXACT, LPM, TERNARY, RANGE
};

// Entry types.
struct MatchKey {
  MatchKey() {}
  MatchKey(ByteContainer data, uint32_t version)
      : data(std::move(data)), version(version) {}

  ByteContainer data{};
  uint32_t version{0};

 protected:
  // disabling polymorphic deletion by making the destructor protected
  ~MatchKey() { }
};

struct ExactMatchKey : public MatchKey {
  static constexpr MatchUnitType mut = MatchUnitType::EXACT;
};

struct LPMMatchKey : public MatchKey {
  LPMMatchKey() {}
  LPMMatchKey(ByteContainer data, int prefix_length, uint32_t version)
      : MatchKey(std::move(data), version), prefix_length(prefix_length) {}

  int prefix_length{0};

  static constexpr MatchUnitType mut = MatchUnitType::LPM;
};

struct TernaryMatchKey : public MatchKey {
  TernaryMatchKey() {}
  TernaryMatchKey(ByteContainer data, ByteContainer mask, int priority,
                  uint32_t version)
      : MatchKey(std::move(data), version), mask(std::move(mask)),
        priority(priority) {}

  ByteContainer mask{};
  // This is initialized to `max` because lookups search for the matching
  // key with the minimum priority, this ensures that default constructed
  // TernaryMatchKey's will never be matched.
  int priority{std::numeric_limits<decltype(priority)>::max()};

  static constexpr MatchUnitType mut = MatchUnitType::TERNARY;
};

struct RangeMatchKey : public TernaryMatchKey {
  RangeMatchKey() {}
  RangeMatchKey(ByteContainer data, ByteContainer mask, int priority,
                std::vector<size_t> range_widths, uint32_t version)
      : TernaryMatchKey(std::move(data), std::move(mask), priority, version),
        range_widths(std::move(range_widths)) {}

  std::vector<size_t> range_widths;

  static constexpr MatchUnitType mut = MatchUnitType::RANGE;
};

}  // namespace bm

#endif  // BM_BM_SIM_MATCH_KEY_TYPES_H_
