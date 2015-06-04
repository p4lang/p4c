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

#include "bm_sim/conditionals.h"

TEST(ExprOpcodesMap, GetOpcode) {
  ASSERT_EQ(ExprOpcode::LOAD_FIELD, ExprOpcodesMap::get_opcode("load_field"));
  ASSERT_EQ(ExprOpcode::ADD, ExprOpcodesMap::get_opcode("+"));
}

// Google Test fixture for conditionals tests
class ConditionalsTest : public ::testing::Test {
protected:

  PHVFactory phv_factory;
  std::unique_ptr<PHV> phv;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};

  ConditionalsTest()
    : testHeaderType("test_t", 0) {
    testHeaderType.push_back_field("f32", 32);
    testHeaderType.push_back_field("f48", 48);
    testHeaderType.push_back_field("f8", 8);
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f128", 128);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);
  }

  virtual void SetUp() {
    phv = phv_factory.create();
  }

  // virtual void TearDown() {}
};

TEST_F(ConditionalsTest, EqData) {
  Conditional c("ctest", 0);
  c.push_back_load_field(testHeader1, 3); // f16
  c.push_back_load_const(Data(0xaba));
  c.push_back_op(ExprOpcode::EQ_DATA);
  c.build();

  Field &f = phv->get_field(testHeader1, 3); // f16
  f.set(0xaba);

  ASSERT_TRUE(c.eval(*phv));

  f.set(0xabb);

  ASSERT_FALSE(c.eval(*phv));
}

TEST_F(ConditionalsTest, NeqData) {
  Conditional c("ctest", 0);
  c.push_back_load_field(testHeader1, 3); // f16
  c.push_back_load_const(Data(0xaba));
  c.push_back_op(ExprOpcode::NEQ_DATA);
  c.build();

  Field &f = phv->get_field(testHeader1, 3); // f16
  f.set(0xaba);

  ASSERT_FALSE(c.eval(*phv));

  f.set(0xabb);

  ASSERT_TRUE(c.eval(*phv));
}

TEST_F(ConditionalsTest, GtData) {
  Conditional c("ctest", 0);
  c.push_back_load_field(testHeader1, 3); // f16
  c.push_back_load_const(Data(0x1001));
  c.push_back_op(ExprOpcode::GT_DATA);
  c.build();

  Field &f = phv->get_field(testHeader1, 3); // f16
  f.set(0x1002);

  ASSERT_TRUE(c.eval(*phv));

  f.set(0x1000);

  ASSERT_FALSE(c.eval(*phv));
}

TEST_F(ConditionalsTest, LtData) {
  Conditional c("ctest", 0);
  c.push_back_load_field(testHeader1, 3); // f16
  c.push_back_load_const(Data(0x1001));
  c.push_back_op(ExprOpcode::LT_DATA);
  c.build();

  Field &f = phv->get_field(testHeader1, 3); // f16
  f.set(0x1002);

  ASSERT_FALSE(c.eval(*phv));

  f.set(0x1000);

  ASSERT_TRUE(c.eval(*phv));
}

TEST_F(ConditionalsTest, GetData) {
  Conditional c("ctest", 0);
  c.push_back_load_field(testHeader1, 3); // f16
  c.push_back_load_const(Data(0x1001));
  c.push_back_op(ExprOpcode::GET_DATA);
  c.build();

  Field &f = phv->get_field(testHeader1, 3); // f16
  f.set(0x1002);

  ASSERT_TRUE(c.eval(*phv));

  f.set(0x1001);

  ASSERT_TRUE(c.eval(*phv));

  f.set(0x1000);

  ASSERT_FALSE(c.eval(*phv));
}

TEST_F(ConditionalsTest, LetData) {
  Conditional c("ctest", 0);
  c.push_back_load_field(testHeader1, 3); // f16
  c.push_back_load_const(Data(0x1001));
  c.push_back_op(ExprOpcode::LET_DATA);
  c.build();

  Field &f = phv->get_field(testHeader1, 3); // f16
  f.set(0x1002);

  ASSERT_FALSE(c.eval(*phv));

  f.set(0x1001);

  ASSERT_TRUE(c.eval(*phv));

  f.set(0x1000);

  ASSERT_TRUE(c.eval(*phv));
}

TEST_F(ConditionalsTest, Add) {
  Conditional c("ctest", 0);
  c.push_back_load_field(testHeader1, 3); // f16
  c.push_back_load_field(testHeader2, 1); // f48
  c.push_back_op(ExprOpcode::ADD);
  c.push_back_load_const(Data(0x33));
  c.push_back_op(ExprOpcode::EQ_DATA);
  c.build();

  Field &f1 = phv->get_field(testHeader1, 3); // f16
  Field &f2 = phv->get_field(testHeader2, 1); // f48
  f1.set(0x11);
  f2.set(0x22);

  ASSERT_TRUE(c.eval(*phv));

  f2.set(0x21);

  ASSERT_FALSE(c.eval(*phv));
}

TEST_F(ConditionalsTest, And) {
  Conditional c1("c1test", 0);
  c1.push_back_load_bool(true);
  c1.push_back_load_bool(true);
  c1.push_back_op(ExprOpcode::AND);
  c1.build();

  ASSERT_TRUE(c1.eval(*phv));

  Conditional c2("c2test", 1);
  c2.push_back_load_bool(true);
  c2.push_back_load_bool(false);
  c2.push_back_op(ExprOpcode::AND);
  c2.build();

  ASSERT_FALSE(c2.eval(*phv));
}

TEST_F(ConditionalsTest, Or) {
  Conditional c1("c1test", 0);
  c1.push_back_load_bool(true);
  c1.push_back_load_bool(false);
  c1.push_back_op(ExprOpcode::OR);
  c1.build();

  ASSERT_TRUE(c1.eval(*phv));

  Conditional c2("c2test", 1);
  c2.push_back_load_bool(false);
  c2.push_back_load_bool(false);
  c2.push_back_op(ExprOpcode::OR);
  c2.build();

  ASSERT_FALSE(c2.eval(*phv));
}

TEST_F(ConditionalsTest, Not) {
  Conditional c1("c1test", 0);
  c1.push_back_load_bool(false);
  c1.push_back_op(ExprOpcode::NOT);
  c1.build();

  ASSERT_TRUE(c1.eval(*phv));

  Conditional c2("c2test", 1);
  c2.push_back_load_bool(true);
  c2.push_back_op(ExprOpcode::NOT);
  c2.build();

  ASSERT_FALSE(c2.eval(*phv));
}

TEST_F(ConditionalsTest, BitAnd) {
  int v1 = 0xababa;
  int v2 = 0x123456;

  Conditional c("ctest", 0);
  c.push_back_load_const(Data(v1));
  c.push_back_load_const(Data(v2));
  c.push_back_op(ExprOpcode::BIT_AND);
  c.push_back_load_const(Data(v1 & v2));
  c.push_back_op(ExprOpcode::EQ_DATA);
  c.build();

  ASSERT_TRUE(c.eval(*phv));
}

TEST_F(ConditionalsTest, BitOr) {
  int v1 = 0xababa;
  int v2 = 0x123456;

  Conditional c("ctest", 0);
  c.push_back_load_const(Data(v1));
  c.push_back_load_const(Data(v2));
  c.push_back_op(ExprOpcode::BIT_OR);
  c.push_back_load_const(Data(v1 | v2));
  c.push_back_op(ExprOpcode::EQ_DATA);
  c.build();

  ASSERT_TRUE(c.eval(*phv));
}

TEST_F(ConditionalsTest, BitXor) {
  int v1 = 0xababa;
  int v2 = 0x123456;

  Conditional c("ctest", 0);
  c.push_back_load_const(Data(v1));
  c.push_back_load_const(Data(v2));
  c.push_back_op(ExprOpcode::BIT_XOR);
  c.push_back_load_const(Data(v1 ^ v2));
  c.push_back_op(ExprOpcode::EQ_DATA);
  c.build();

  ASSERT_TRUE(c.eval(*phv));
}

TEST_F(ConditionalsTest, BitNeg) {
  int v = 0xababa;

  Conditional c("ctest", 0);
  c.push_back_load_const(Data(v));
  c.push_back_op(ExprOpcode::BIT_NEG);
  c.push_back_load_const(Data(~v));
  c.push_back_op(ExprOpcode::EQ_DATA);
  c.build();

  ASSERT_TRUE(c.eval(*phv));
}

TEST_F(ConditionalsTest, ValidHeader) {
  Conditional c("ctest", 0);
  c.push_back_load_header(testHeader1);
  c.push_back_op(ExprOpcode::VALID_HEADER);
  c.build();

  Header &hdr1 = phv->get_header(testHeader1);
  hdr1.mark_valid();

  ASSERT_TRUE(c.eval(*phv));

  hdr1.mark_invalid();

  ASSERT_FALSE(c.eval(*phv));
}

TEST_F(ConditionalsTest, Stress) {
  Conditional c("ctest", 0);
  // (valid(testHeader1) && (false || (testHeader1.f16 + 1 == 2))) && !valid(testHeader2)
  c.push_back_load_header(testHeader1);
  c.push_back_op(ExprOpcode::VALID_HEADER);
  c.push_back_load_bool(false);
  c.push_back_load_field(testHeader1, 3); // f16
  c.push_back_load_const(Data(1));
  c.push_back_op(ExprOpcode::ADD);
  c.push_back_load_const(Data(2));
  c.push_back_op(ExprOpcode::EQ_DATA);
  c.push_back_op(ExprOpcode::OR);
  c.push_back_op(ExprOpcode::AND);
  c.push_back_load_header(testHeader2);
  c.push_back_op(ExprOpcode::VALID_HEADER);
  c.push_back_op(ExprOpcode::NOT);
  c.push_back_op(ExprOpcode::AND);
  c.build();

  Header &hdr1 = phv->get_header(testHeader1);
  hdr1.mark_valid();

  Field &f1 = phv->get_field(testHeader1, 3); // f16

  Header &hdr2 = phv->get_header(testHeader2);
  hdr2.mark_invalid();

  for(int i = 0; i < 100000; i++) {
    if(i % 2 == 0) {
      f1.set(1);
      ASSERT_TRUE(c.eval(*phv));
    }
    else {
      f1.set(0);
      ASSERT_FALSE(c.eval(*phv));
    }
  }
}
