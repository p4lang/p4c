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

#include <bm/bm_sim/expressions.h>
#include <bm/bm_sim/phv.h>

// expressions are mostly tested in test_conditionals.cpp. This file is only
// used for some edge case testing.

using namespace::bm;

class ExpressionsTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;
  std::unique_ptr<PHV> phv;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};

  ExpressionsTest()
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

TEST_F(ExpressionsTest, EmptyArith) {
  Expression expr;
  expr.build();
  ASSERT_TRUE(expr.empty());
  const auto data = expr.eval_arith(*phv.get());
  ASSERT_EQ(0, data.get<int>());
}

TEST_F(ExpressionsTest, EmptyBool) {
  Expression expr;
  expr.build();
  ASSERT_TRUE(expr.empty());
  const auto b = expr.eval_bool(*phv.get());
  ASSERT_FALSE(b);
}
