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

#include <memory>
#include <string>

#include <cassert>

#include "bm_sim/phv.h"

// Google Test fixture for PHV tests
class PHVTest : public ::testing::Test {
protected:

  PHVFactory phv_factory;
  std::unique_ptr<PHV> phv;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};

  PHVTest()
    : testHeaderType("test_t", 0) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);
  }

  virtual void SetUp() {
    phv = phv_factory.create();
  }

  // virtual void TearDown() {}
};

TEST_F(PHVTest, CopyHeaders) {
  std::unique_ptr<PHV> phv_2 = phv_factory.create();

  phv->get_header(testHeader1).mark_valid();
  phv->get_header(testHeader2).mark_invalid();

  // check default values
  ASSERT_FALSE(phv_2->get_header(testHeader1).is_valid());
  ASSERT_FALSE(phv_2->get_header(testHeader2).is_valid());

  Field &f16 = phv->get_field(testHeader1, 0);
  f16.set(0xaba);
  Field &f48 = phv->get_field(testHeader1, 1);
  f48.set("0xaabbccddeeff");

  Field &f16_2 = phv_2->get_field(testHeader1, 0);
  f16_2.set(0);
  Field &f48_2 = phv_2->get_field(testHeader1, 1);
  f48_2.set(0);

  ASSERT_NE(f16, f16_2);
  ASSERT_NE(f48, f48_2);

  phv_2->copy_headers(*phv);

  ASSERT_TRUE(phv_2->get_header(testHeader1).is_valid());
  ASSERT_FALSE(phv_2->get_header(testHeader2).is_valid());

  ASSERT_EQ(f16, f16_2);
  ASSERT_EQ(f48, f48_2);
}

