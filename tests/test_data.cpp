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

#include "bm_sim/data.h"

TEST(Data, ConstructorFromUInt) {
  const Data d(0xaba);
  EXPECT_EQ((unsigned) 0xaba, d.get_uint());
}

TEST(Data, ConstructorFromBytes) {
  unsigned char bytes[2] = {0x0a, 0xba};
  const Data d((char *) bytes, sizeof(bytes));
  EXPECT_EQ((unsigned) 0xaba, d.get_uint());
}

TEST(Data, ConstructorFromHexStr) {
  const Data d1(std::string("0xaba"));
  EXPECT_EQ((unsigned) 0xaba, d1.get_uint());

  const Data d2("-0xaba");
  EXPECT_EQ(-0xaba, d2.get_int());
}

TEST(Data, EqualityOp) {
  const Data d1(0xaba);
  const Data d2(0xaba);
  ASSERT_TRUE(d1 == d2);
}

TEST(Data, InEqualityOp) {
  const Data d1(0xaba);
  const Data d2(0xab0);
  ASSERT_TRUE(d1 != d2);
}

TEST(Data, CopyOp) {
  const Data d1(0xaba);
  Data d2;
  d2 = d1;
  EXPECT_EQ(d1, d2);
  d2.set(0);
  EXPECT_EQ((unsigned) 0xaba, d1.get_uint());
}

TEST(Data, AddDestIsSrc) {
  Data d1(11);
  Data d2(22);
  d1.add(d1, d2);
  EXPECT_EQ((unsigned) 33, d1.get_uint());
}

TEST(Data, Add) {
  Data d1(11);
  Data d2(22);
  Data d3;
  d3.add(d1, d2);
  EXPECT_EQ((unsigned) 33, d3.get_uint());
}

TEST(Data, Sub) {
  Data d1(22);
  Data d2(10);
  Data d3;
  d3.sub(d1, d2);
  EXPECT_EQ((unsigned) 12, d3.get_uint());
}

TEST(Data, BitNeg) {
  Data d1(0xaba);
  Data d2;
  d2.bit_neg(d1);
  EXPECT_EQ(~0xaba, d2.get_int());
}

TEST(Data, ShiftLeft) {
  Data d1(0xaba);
  Data d2(3);
  Data d3;
  d3.shift_left(d1, d2);
  EXPECT_EQ((0xaba) << 3, d3.get_int());
}

TEST(Data, ShiftRight) {
  Data d1(0xabababa);
  Data d2(3);
  Data d3;
  d3.shift_right(d1, d2);
  EXPECT_EQ((0xabababa) >> 3, d3.get_int());
}
