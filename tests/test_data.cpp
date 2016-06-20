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

#include <bm/bm_sim/data.h>

using bm::Data;

TEST(Data, ConstructorFromUInt) {
  const Data d(0xaba);
  ASSERT_EQ((unsigned) 0xaba, d.get_uint());
}

TEST(Data, ConstructorFromBytes) {
  unsigned char bytes[2] = {0x0a, 0xba};
  const Data d((char *) bytes, sizeof(bytes));
  ASSERT_EQ((unsigned) 0xaba, d.get_uint());
}

TEST(Data, ConstructorFromHexStr) {
  const Data d1(std::string("0xaba"));
  ASSERT_EQ((unsigned) 0xaba, d1.get_uint());

  const Data d2("-0xaba");
  ASSERT_EQ(-0xaba, d2.get_int());
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
  ASSERT_EQ(d1, d2);
  d2.set(0);
  ASSERT_EQ((unsigned) 0xaba, d1.get_uint());
}

TEST(Data, AddDestIsSrc) {
  Data d1(11);
  Data d2(22);
  d1.add(d1, d2);
  ASSERT_EQ((unsigned) 33, d1.get_uint());
}

TEST(Data, Add) {
  Data d1(11);
  Data d2(22);
  Data d3;
  d3.add(d1, d2);
  ASSERT_EQ((unsigned) 33, d3.get_uint());
}

TEST(Data, Sub) {
  Data d1(22);
  Data d2(10);
  Data d3;
  d3.sub(d1, d2);
  ASSERT_EQ((unsigned) 12, d3.get_uint());
}

TEST(Data, BitNeg) {
  Data d1(0xaba);
  Data d2;
  d2.bit_neg(d1);
  ASSERT_EQ(~0xaba, d2.get_int());
}

TEST(Data, ShiftLeft) {
  Data d1(0xaba);
  Data d2(3);
  Data d3;
  d3.shift_left(d1, d2);
  ASSERT_EQ((0xaba) << 3, d3.get_int());
}

TEST(Data, ShiftRight) {
  Data d1(0xabababa);
  Data d2(3);
  Data d3;
  d3.shift_right(d1, d2);
  ASSERT_EQ((0xabababa) >> 3, d3.get_int());
}

namespace {

// negative operands are not supported
void test_divide(int a, int b) {
  int q_exp = a / b;
  int r_exp = a % b;
  Data q, r;
  q.divide(Data(a), Data(b));
  r.mod(Data(a), Data(b));
  ASSERT_EQ(q_exp, q.get_int());
  ASSERT_EQ(r_exp, r.get_int());
}

}  // namespace

TEST(Data, Divide) {
  test_divide(7, 3);
  test_divide(56343, 45);
  test_divide(5, 5);
  test_divide(0, 3);
}

namespace {

void test_two_comp_mod(int src, unsigned int width, int expected) {
  Data dst;
  dst.two_comp_mod(Data(src), Data(width));
  ASSERT_EQ(expected, dst.get_int());
}

}  // namespace

TEST(Data, TwoCompMod) {
  test_two_comp_mod(3, 2, -1);
  test_two_comp_mod(10, 8, 10);
  test_two_comp_mod(-10, 8, -10);
  test_two_comp_mod(256, 8, 0);
  test_two_comp_mod(257, 8, 1);
  test_two_comp_mod(128, 8, -128);
  test_two_comp_mod(129, 8, -127);
  test_two_comp_mod(127, 8, 127);
  test_two_comp_mod(126, 8, 126);
  test_two_comp_mod(-129, 8, 127);
}

TEST(Data, TwoCompModStress) {
  Data dst;
  Data src(129);
  Data width(8);
  for (size_t iter = 0; iter < 100000; iter++) {
    dst.two_comp_mod(src, width);
    ASSERT_EQ(-127, dst.get_int());
  }
}

TEST(Data, TestEq) {
  int v = 99;
  Data d(v);
  ASSERT_TRUE(d.test_eq(v));
}

TEST(Data, GetString) {
  std::string s("\xab\x11\xcd\x99", 4);
  Data d(s.data(), s.size());
  ASSERT_EQ(s, d.get_string());
}
