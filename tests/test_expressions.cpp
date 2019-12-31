/* Copyright 2013-2019 Barefoot Networks, Inc.
 * Copyright 2019 VMware, Inc.
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

#include <gtest/gtest.h>

#include <bm/bm_sim/expressions.h>
#include <bm/bm_sim/phv.h>

#include <memory>
#include <stdexcept>

// expressions are mostly tested in test_conditionals.cpp. This file is only
// used for some edge case testing.

using namespace::bm;

class ExpressionsTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;
  std::unique_ptr<PHV> phv;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};

  header_stack_id_t testHeaderStack{0};

  ExpressionsTest()
      : testHeaderType("test_t", 0) {
    testHeaderType.push_back_field("f32", 32);
    testHeaderType.push_back_field("f48", 48);
    testHeaderType.push_back_field("f8", 8);
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f128", 128);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);

    phv_factory.push_back_header_stack(
        "test_stack", testHeaderStack, testHeaderType,
        {testHeader1, testHeader2});
  }

  virtual void SetUp() {
    phv = phv_factory.create();
  }

  // virtual void TearDown() {}
};

TEST_F(ExpressionsTest, EmptyArith) {
  Expression expr;
  expr.build();
  ASSERT_TRUE(expr.empty());
  const auto data = expr.eval_arith(*phv.get());
  EXPECT_EQ(0, data.get<int>());
}

TEST_F(ExpressionsTest, EmptyBool) {
  Expression expr;
  expr.build();
  ASSERT_TRUE(expr.empty());
  const auto b = expr.eval_bool(*phv.get());
  EXPECT_FALSE(b);
}

// TODO(antonin): test the union variants

TEST_F(ExpressionsTest, HeaderRef) {
  Expression expr;
  expr.push_back_load_header(testHeader1);
  expr.build();

  auto &hdr = expr.eval_header(phv.get());
  EXPECT_EQ(&phv->get_header(testHeader1), &hdr);
}

TEST_F(ExpressionsTest, HeaderStackRef) {
  Expression expr;
  expr.push_back_load_header_stack(testHeaderStack);
  expr.build();

  auto &stack = expr.eval_header_stack(phv.get());
  EXPECT_EQ(&phv->get_header_stack(testHeaderStack), &stack);
}

TEST_F(ExpressionsTest, FieldRef) {
  Expression expr;
  expr.push_back_load_header_stack(testHeaderStack);
  expr.push_back_load_const(Data(1));  // testHeader2
  expr.push_back_op(ExprOpcode::DEREFERENCE_HEADER_STACK);
  expr.push_back_access_field(3);  // f16
  expr.build();

  auto &data = expr.eval_arith_lvalue(phv.get());
  EXPECT_EQ(&phv->get_field(testHeader2, 3), &data);
}

TEST_F(ExpressionsTest, OutOfBoundsStackAccess) {
  Expression expr;
  expr.push_back_load_header_stack(testHeaderStack);
  expr.push_back_load_const(Data(2));  // out-of-bounds access
  expr.push_back_op(ExprOpcode::DEREFERENCE_HEADER_STACK);
  expr.build();

  // At the moment, we simply throw an exception which should crash the bmv2
  // process unless it is caught somewhere (e.g. in the target code). In the
  // future, we may log an error message as well.
  EXPECT_THROW(expr.eval_header(phv.get()), std::out_of_range);
}
