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

#include <bm/bm_sim/conditionals.h>

// This is where expressions are tested (essentially the same thing as
// conditionals)

using namespace bm;

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

TEST_F(ConditionalsTest, Divide) {
  Conditional c("ctest", 0);

  // we check that (2 == 7 / 3) && (1 == 7 % 3) evaluates to true
  c.push_back_load_const(Data(2));
  c.push_back_load_const(Data(7));
  c.push_back_load_const(Data(3));
  c.push_back_op(ExprOpcode::DIV);
  c.push_back_op(ExprOpcode::EQ_DATA);

  c.push_back_load_const(Data(1));
  c.push_back_load_const(Data(7));
  c.push_back_load_const(Data(3));
  c.push_back_op(ExprOpcode::MOD);
  c.push_back_op(ExprOpcode::EQ_DATA);

  c.push_back_op(ExprOpcode::AND);

  c.build();
  ASSERT_TRUE(c.eval(*phv));
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

TEST_F(ConditionalsTest, TwoCompMod) {
  int v1 = -129;
  int v2 = 8;
  int res = 127;

  Conditional c("ctest", 0);
  c.push_back_load_const(Data(v1));
  c.push_back_load_const(Data(v2));
  c.push_back_op(ExprOpcode::TWO_COMP_MOD);
  c.push_back_load_const(Data(res));
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

TEST_F(ConditionalsTest, RegisterRef) {
  Conditional c("ctest", 0);
  constexpr size_t register_size = 1024;
  constexpr int register_bw = 16;
  RegisterArray register_array("register_test", 0, register_size, register_bw);
  const unsigned int register_idx = 68;
  const unsigned int value_start = 0x33;
  const unsigned int value_add = 0xa5;
  c.push_back_load_register_ref(&register_array, register_idx);
  c.push_back_load_const(Data(value_add));
  c.push_back_op(ExprOpcode::ADD);
  c.push_back_load_const(Data(value_start + value_add));
  c.push_back_op(ExprOpcode::EQ_DATA);
  c.build();

  Data &reg = register_array.at(register_idx);

  reg.set(value_start);
  ASSERT_TRUE(c.eval(*phv));

  reg.set(value_start + 1);
  ASSERT_FALSE(c.eval(*phv));
}

TEST_F(ConditionalsTest, RegisterGen) {
  Conditional c("ctest", 0);
  constexpr size_t register_size = 1024;
  constexpr int register_bw = 16;
  RegisterArray register_array("register_test", 0, register_size, register_bw);
  const unsigned int register_idx = 68;
  const unsigned int value_start = 0x33;
  const unsigned int value_add = 0xa5;
  c.push_back_load_const(Data(register_idx));
  c.push_back_load_register_gen(&register_array);
  c.push_back_load_const(Data(value_add));
  c.push_back_op(ExprOpcode::ADD);
  c.push_back_load_const(Data(value_start + value_add));
  c.push_back_op(ExprOpcode::EQ_DATA);
  c.build();

  Data &reg = register_array.at(register_idx);

  reg.set(value_start);
  ASSERT_TRUE(c.eval(*phv));

  reg.set(value_start + 1);
  ASSERT_FALSE(c.eval(*phv));
}

// This test was written to stress-test the Expression::assign_dest_registers(),
// for which my first implementation was faulty...
TEST_F(ConditionalsTest, Add2) {
  Conditional c("ctest", 0);
  constexpr size_t nb_sub_adds = 16;
  static_assert(nb_sub_adds > 4, "try a larger nb_sub_adds");
  const unsigned int f1_v = 1;
  const unsigned int f2_v = 2;
  const unsigned int expected_v = (f1_v + f2_v) * nb_sub_adds;
  for (size_t i = 0; i < nb_sub_adds; i++) {
    c.push_back_load_field(testHeader1, 3); // f16
    c.push_back_load_field(testHeader2, 1); // f48
    c.push_back_op(ExprOpcode::ADD);
  }
  for (size_t i = 0; i < nb_sub_adds - 1; i++) {
    c.push_back_op(ExprOpcode::ADD);
  }
  c.push_back_load_const(Data(expected_v));
  c.push_back_op(ExprOpcode::EQ_DATA);
  c.build();

  Field &f1 = phv->get_field(testHeader1, 3); // f16
  Field &f2 = phv->get_field(testHeader2, 1); // f48
  f1.set(f1_v);
  f2.set(f2_v);

  ASSERT_TRUE(c.eval(*phv));
}

TEST_F(ConditionalsTest, TernaryOp) {
  // testHeader1.f16 == ((testHeader2.f16 == 0x33) ? (0xab + 1) : 0xcd)
  Conditional c("ctest", 0);

  const unsigned int value_ternary_test = 0x33;
  const unsigned int value_ternary_1_0 = 0xab;
  const unsigned int value_ternary_1 = value_ternary_1_0 + 1;
  const unsigned int value_ternary_2 = 0xcd;

  Expression ternary_e1;
  ternary_e1.push_back_load_const(Data(value_ternary_1_0));
  ternary_e1.push_back_load_const(Data(1));
  ternary_e1.push_back_op(ExprOpcode::ADD);
  // build not necessary for "sub-expressions"
  // ternary_e1.build();

  Expression ternary_e2;
  ternary_e2.push_back_load_const(Data(value_ternary_2));
  // ternary_e2.build();

  c.push_back_load_field(testHeader1, 3); // f16
  c.push_back_load_field(testHeader2, 3); // f16
  c.push_back_load_const(Data(value_ternary_test));
  c.push_back_op(ExprOpcode::EQ_DATA);
  c.push_back_ternary_op(ternary_e1, ternary_e2);
  c.push_back_op(ExprOpcode::EQ_DATA);
  c.build();

  Field &testHeader1_f16 = phv->get_field(testHeader1, 3); // f16
  Field &testHeader2_f16 = phv->get_field(testHeader2, 3); // f16
  testHeader1_f16.set(value_ternary_1);
  testHeader2_f16.set(value_ternary_test);

  ASSERT_TRUE(c.eval(*phv));

  testHeader2_f16.set(value_ternary_test + 1);

  ASSERT_FALSE(c.eval(*phv));

  testHeader1_f16.set(value_ternary_2);

  ASSERT_TRUE(c.eval(*phv));
}

TEST_F(ConditionalsTest, TernaryOp2) {
  // 1 == (((testHeader1.f16 < testHeader2.f16) ? 1 : 0) & 0x1) & 0xff
  Conditional c("ctest", 0);

  Expression ternary_e1;
  ternary_e1.push_back_load_const(Data(1));

  Expression ternary_e2;
  ternary_e2.push_back_load_const(Data(0));

  c.push_back_load_field(testHeader1, 3); // f16
  c.push_back_load_field(testHeader2, 3); // f16
  c.push_back_op(ExprOpcode::LT_DATA);
  c.push_back_ternary_op(ternary_e1, ternary_e2);
  c.push_back_load_const(Data(0x1));
  c.push_back_op(ExprOpcode::BIT_AND);
  c.push_back_load_const(Data(0xff));
  c.push_back_op(ExprOpcode::BIT_AND);
  c.push_back_load_const(Data(1));
  c.push_back_op(ExprOpcode::EQ_DATA);
  c.build();

  Field &testHeader1_f16 = phv->get_field(testHeader1, 3); // f16
  Field &testHeader2_f16 = phv->get_field(testHeader2, 3); // f16
  testHeader1_f16.set(3);
  testHeader2_f16.set(3);

  ASSERT_FALSE(c.eval(*phv));

  testHeader1_f16.set(1);

  ASSERT_TRUE(c.eval(*phv));
}

TEST_F(ConditionalsTest, D2B) {
  Conditional c("ctest", 0);
  c.push_back_load_field(testHeader1, 3);  // f16
  c.push_back_op(ExprOpcode::DATA_TO_BOOL);
  c.build();

  Field &testHeader1_f16 = phv->get_field(testHeader1, 3);

  testHeader1_f16.set(0);
  ASSERT_FALSE(c.eval(*phv));

  testHeader1_f16.set(1);
  ASSERT_TRUE(c.eval(*phv));

  testHeader1_f16.set(7);
  ASSERT_TRUE(c.eval(*phv));
}

TEST_F(ConditionalsTest, B2D) {
  auto build_condition = [](const std::string &name, int id, bool bool_v,
                            int data_v) {
    Conditional c(name, id);
    c.push_back_load_bool(bool_v);
    c.push_back_op(ExprOpcode::BOOL_TO_DATA);
    c.push_back_load_const(Data(data_v));
    c.push_back_op(ExprOpcode::EQ_DATA);
    c.build();
    return c;
  };

  Conditional c1 = build_condition("c1", 0, true, 0);
  ASSERT_FALSE(c1.eval(*phv));
  Conditional c2 = build_condition("c2", 1, true, 1);
  ASSERT_TRUE(c2.eval(*phv));
  Conditional c3 = build_condition("c3", 2, true, 7);
  ASSERT_FALSE(c3.eval(*phv));
  Conditional c4 = build_condition("c4", 3, false, 0);
  ASSERT_TRUE(c4.eval(*phv));
  Conditional c5 = build_condition("c5", 4, false, 1);
  ASSERT_FALSE(c5.eval(*phv));
  Conditional c6 = build_condition("c6", 5, false, 7);
  ASSERT_FALSE(c6.eval(*phv));
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
