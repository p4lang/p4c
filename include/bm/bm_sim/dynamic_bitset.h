/* Copyright 2021 VMware, Inc.
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
 * Antonin Bas
 *
 */

#ifndef BM_BM_SIM_DYNAMIC_BITSET_H_
#define BM_BM_SIM_DYNAMIC_BITSET_H_

#include <bitset>
#include <cassert>
#include <limits>
#include <vector>

namespace bm {

template <typename T>
inline int find_lowest_bit(T v);

template<>
inline int find_lowest_bit<uint32_t>(uint32_t v) {
  static const int kMultiplyDeBruijnBitPosition[32] = {
    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
  };
  return kMultiplyDeBruijnBitPosition[((v & -v) * 0x077CB531U) >> 27];
}

template<>
inline int find_lowest_bit<uint64_t>(uint64_t v) {
  auto sw = static_cast<uint32_t>(v & 0xFFFFFFFF);
  return (sw == 0) ?
      32 + find_lowest_bit<uint32_t>(static_cast<uint32_t>(v >> 32)) :
      find_lowest_bit<uint32_t>(sw);
}

class DynamicBitset {
 public:
  using Block = unsigned long;  // NOLINT(runtime/int)
  using Backend = std::vector<Block>;
  using size_type = Backend::size_type;

  static constexpr size_type npos = static_cast<size_type>(-1);

  size_type size() const {
    return num_bits;
  }

  size_type count() const {
    return num_bits_set;
  }

  bool test(size_type idx) const {
    assert(idx < num_bits);
    auto word_idx = static_cast<size_type>(idx / block_width);
    auto bit_idx = static_cast<int>(idx % block_width);
    return words[word_idx] & static_cast<Block>(1) << bit_idx;
  }

  bool set(size_type idx) {
    assert(idx < num_bits);
    auto word_idx = static_cast<size_type>(idx / block_width);
    auto bit_idx = static_cast<int>(idx % block_width);
    auto mask = static_cast<Block>(1) << bit_idx;
    auto bit = words[word_idx] & mask;
    words[word_idx] |= mask;
    num_bits_set += (bit >> bit_idx) ^ 1;
    return (bit == 0);
  }

  bool reset(size_type idx) {
    assert(idx < num_bits);
    auto word_idx = static_cast<size_type>(idx / block_width);
    auto bit_idx = static_cast<int>(idx % block_width);
    auto mask = static_cast<Block>(1) << bit_idx;
    auto bit = words[word_idx] & mask;
    words[word_idx] &= ~mask;
    num_bits_set -= (bit >> bit_idx);
    return (bit != 0);
  }

  void push_back(bool v) {
    auto bit_idx = static_cast<int>(num_bits % block_width);
    if (bit_idx == 0) words.push_back(0);
    num_bits++;
    if (v) set(num_bits - 1);
  }

  void clear() {
    words.clear();
    num_bits = 0;
    num_bits_set = 0;
  }

  size_type find_first() const {
    return find_from_block(0);
  }

  size_type find_next(size_type idx) const {
    idx++;
    if (idx >= num_bits) return npos;
    auto word_idx = static_cast<size_type>(idx / block_width);
    auto bit_idx = static_cast<int>(idx % block_width);
    auto sw = words[word_idx] & (ones << bit_idx);
    if (sw == 0) return find_from_block(word_idx + 1);
    return word_idx * block_width + find_lowest_bit(sw);
  }

  size_type find_unset_first() const {
    return find_unset_from_block(0);
  }

  size_type find_unset_next(size_type idx) const {
    idx++;
    if (idx >= num_bits) return npos;
    auto word_idx = static_cast<size_type>(idx / block_width);
    auto bit_idx = static_cast<int>(idx % block_width);
    auto sw = words[word_idx] | ~(ones << bit_idx);
    if (sw == ones) return find_unset_from_block(word_idx + 1);
    return word_idx * block_width + find_lowest_bit(~sw);
  }

  bool operator==(const DynamicBitset &other) const {
    return (num_bits == other.num_bits) && (words == other.words);
  }

  bool operator!=(const DynamicBitset &other) const {
    return !(*this == other);
  }

  void resize(size_type new_num_bits) {
    num_bits = new_num_bits;
    words.resize((num_bits + block_width - 1) / block_width);
  }

 private:
  size_type find_from_block(size_type word_idx) const {
    for (size_type i = word_idx; i < words.size(); i++) {
      if (words[i] == 0) continue;
      return i * block_width + find_lowest_bit(words[i]);
    }
    return npos;
  }

  size_type find_unset_from_block(size_type word_idx) const {
    for (size_type i = word_idx; i < words.size(); i++) {
      if (words[i] == ones) continue;
      return i * block_width + find_lowest_bit(~words[i]);
    }
    return npos;
  }

  static constexpr size_type block_width = std::numeric_limits<Block>::digits;
  static constexpr Block ones = ~static_cast<Block>(0);
  Backend words;
  size_type num_bits{0};
  size_type num_bits_set{0};
};

}  // namespace bm

#endif  // BM_BM_SIM_DYNAMIC_BITSET_H_
