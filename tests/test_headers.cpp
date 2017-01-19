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

#include <bm/bm_sim/phv.h>

#include <string>

using namespace bm;

// Google Test fixture for header tests
class HeaderTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;
  std::unique_ptr<PHV> phv;

  HeaderType testHeaderType1, testHeaderType2;
  header_id_t testHeader10{0}, testHeader11{1}, testHeader20{2};

  HeaderTest()
      : testHeaderType1("test1_t", 0),
        testHeaderType2("test2_t", 0) {
    testHeaderType1.push_back_field("f16", 16);
    testHeaderType1.push_back_field("f48", 48);
    phv_factory.push_back_header("test10", testHeader10, testHeaderType1);
    phv_factory.push_back_header("test11", testHeader11, testHeaderType1);
    phv_factory.push_back_header("test20", testHeader20, testHeaderType2);
  }

  virtual void SetUp() {
    phv = phv_factory.create();
  }

  // virtual void TearDown() {}
};

TEST_F(HeaderTest, CmpSameType) {
  auto &h10 = phv->get_header(testHeader10);
  auto &h11 = phv->get_header(testHeader11);
  ASSERT_FALSE(h10.is_valid());
  ASSERT_FALSE(h11.is_valid());

  ASSERT_FALSE(h10.cmp(h11)); ASSERT_FALSE(h11.cmp(h10));

  h10.mark_valid(); h11.mark_valid();
  ASSERT_TRUE(h10.cmp(h11)); ASSERT_TRUE(h11.cmp(h10));

  h10.get_field(0).set(1);
  ASSERT_FALSE(h10.cmp(h11)); ASSERT_FALSE(h11.cmp(h10));
}

TEST_F(HeaderTest, CmpDiffType) {
  auto &h10 = phv->get_header(testHeader10);
  auto &h20 = phv->get_header(testHeader20);
  ASSERT_FALSE(h10.is_valid());
  ASSERT_FALSE(h20.is_valid());

  ASSERT_FALSE(h10.cmp(h20)); ASSERT_FALSE(h20.cmp(h10));

  h10.mark_valid(); h20.mark_valid();
  ASSERT_FALSE(h10.cmp(h20)); ASSERT_FALSE(h20.cmp(h10));
}
