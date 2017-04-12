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

#include <bm/bm_sim/core/primitives.h>
#include <bm/bm_sim/phv.h>

#include <string>

using namespace bm;

class AssignPrimitivesTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;
  std::unique_ptr<PHV> phv;

  HeaderType testHeaderType1, testHeaderType2;
  header_id_t testHeader10{0}, testHeader11{1};
  header_id_t testHeader20{2}, testHeader21{2};

  AssignPrimitivesTest()
      : testHeaderType1("test1_t", 0),
        testHeaderType2("test2_t", 1) {
    testHeaderType1.push_back_VL_field("VLf",
                                       4  /* max header bytes */,
                                       nullptr  /* field length expr */);
    testHeaderType1.push_back_field("f16", 16);
    testHeaderType2.push_back_field("f16", 16);
    phv_factory.push_back_header("test10", testHeader10, testHeaderType1);
    phv_factory.push_back_header("test11", testHeader11, testHeaderType1);
    phv_factory.push_back_header("test20", testHeader20, testHeaderType2);
    phv_factory.push_back_header("test21", testHeader21, testHeaderType2);
  }

  virtual void SetUp() {
    phv = phv_factory.create();
  }

  // virtual void TearDown() {}
};

TEST_F(AssignPrimitivesTest, Assign) {
  auto &f20 = phv->get_field(testHeader20, 0);
  Data src("0xaabb");
  auto primitive = ActionOpcodesMap::get_instance()->get_primitive("assign");
  ASSERT_NE(nullptr, primitive);
  auto assign = dynamic_cast<core::assign *>(primitive.get());
  ASSERT_NE(nullptr, assign);
  (*assign)(f20, src);
  EXPECT_EQ(src, f20);
}

TEST_F(AssignPrimitivesTest, AssignVL) {
  auto &hdr10 = phv->get_header(testHeader10);
  auto &hdr11 = phv->get_header(testHeader11);
  auto &f10 = hdr10.get_field(0);
  auto &f11 = hdr11.get_field(0);
  const std::string data("\xaa\xbb");
  // we need to append 16 bits for second field of the header (non VL field)
  std::string data_ = data + std::string("\x11\x11");
  hdr10.extract_VL(data_.data(), data.size() * 8);
  EXPECT_EQ(data.data(), f10.get_string());
  EXPECT_EQ(2 + 2, hdr10.get_nbytes_packet());
  EXPECT_EQ(2, hdr11.get_nbytes_packet());

  auto primitive = ActionOpcodesMap::get_instance()->get_primitive("assign_VL");
  ASSERT_NE(nullptr, primitive);
  auto assign_VL = dynamic_cast<core::assign_VL *>(primitive.get());
  ASSERT_NE(nullptr, assign_VL);

  (*assign_VL)(f11, f10);
  EXPECT_EQ(f10, f11);
  EXPECT_EQ(2 + 2, hdr10.get_nbytes_packet());
  EXPECT_EQ(2 + 2, hdr11.get_nbytes_packet());
}

TEST_F(AssignPrimitivesTest, AssignHeader) {
  auto primitive = ActionOpcodesMap::get_instance()->get_primitive(
      "assign_header");
  ASSERT_NE(nullptr, primitive);
  auto assign_header = dynamic_cast<core::assign_header *>(primitive.get());
  ASSERT_NE(nullptr, assign_header);

  // VL case
  {
    auto &hdr10 = phv->get_header(testHeader10);
    auto &hdr11 = phv->get_header(testHeader11);
    const std::string data("\xaa\xbb");
    // we need to append 16 bits for second field of the header (non VL field)
    std::string data_ = data + std::string("\x11\x11");
    hdr10.extract_VL(data_.data(), data.size() * 8);
    EXPECT_TRUE(hdr10.is_valid());
    (*assign_header)(hdr11, hdr10);
    EXPECT_TRUE(hdr11.is_valid());
    EXPECT_TRUE(hdr11.cmp(hdr10));
  }

  // non-VL case
  {
    auto &hdr20 = phv->get_header(testHeader20);
    auto &hdr21 = phv->get_header(testHeader21);
    const std::string data("\xaa\xbb");
    hdr20.extract(data.data(), *phv);
    EXPECT_TRUE(hdr20.is_valid());
    (*assign_header)(hdr21, hdr20);
    EXPECT_TRUE(hdr21.is_valid());
    EXPECT_TRUE(hdr21.cmp(hdr20));
  }
}
