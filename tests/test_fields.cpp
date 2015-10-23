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

#include <string>
#include <vector>

#include "bm_sim/fields.h"

using ::testing::TestWithParam;
using ::testing::Range;
using ::testing::Combine;

class BitInStream {
public:
  void append_one(bool bit) {
    int offset = nbits_ % 8;
    if(offset == 0)
      bits_.push_back(0);
    if(bit)
      bits_.back() |= (1 << (7 - offset));
    nbits_ += 1;
  }

  size_t nbits() const {
    return nbits_;
  }

  size_t nbytes() const {
    return bits_.size();
  }

  ByteContainer &bytes() {
    return bits_;
  }

  const ByteContainer &bytes() const {
    return bits_;
  }

  void clear() {
    bits_.clear();
    nbits_ = 0;
  }

private:
  ByteContainer bits_{};
  int nbits_{0};
};

class FieldSerializeTest : public TestWithParam< std::tuple<int, int> > {
protected:
  // I wanted to use size_t, but GTest Range() was complaining
  int bitwidth{0};
  int hdr_offset{0};
  
  virtual void SetUp() {
    bitwidth = std::get<0>(GetParam());
    hdr_offset = std::get<1>(GetParam());
  }
};

TEST_P(FieldSerializeTest, Extract) {
  int max_input = 1 << bitwidth;
  for(int v = 0; v < max_input; v++) {
    BitInStream input;
    for(int i = 0; i < hdr_offset; i++) {
      input.append_one(0);
    }
    for(int i = bitwidth - 1; i >= 0; i--) {
      input.append_one(v & (1 << i));
    }

    ASSERT_EQ(bitwidth + hdr_offset, static_cast<int>(input.nbits()));

    Field f(bitwidth);
    f.extract(input.bytes().data(), hdr_offset);

    ASSERT_EQ(v, f.get_int());
  }
}

TEST_P(FieldSerializeTest, Deparse) {
  int max_input = 1 << bitwidth;
  for(int v = 0; v < max_input; v++) {
    BitInStream expected;
    for(int i = 0; i < hdr_offset; i++) {
      expected.append_one(0);
    }
    for(int i = bitwidth - 1; i >= 0; i--) {
      expected.append_one(v & (1 << i));
    }
    for(int i = bitwidth + hdr_offset; i < 24; i++) {
      expected.append_one(0);
    }

    Field f(bitwidth);
    f.set(v);

    ByteContainer output(3);

    f.deparse(output.data(), hdr_offset);

    ASSERT_EQ(expected.bytes(), output);
  }
}

INSTANTIATE_TEST_CASE_P(TestParameters,
                        FieldSerializeTest,
                        Combine(Range(1, 17), Range(0, 8)));
