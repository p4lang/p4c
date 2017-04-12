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


class HeaderVLTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;
  std::unique_ptr<PHV> phv;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};

  const std::string data_1{"\xaa\xbb"};
  const std::string data_2{"\xcc"};
  const int VL_nbits_1{16};
  const int VL_nbits_2{8};

  Header *hdr_1{nullptr};
  Header *hdr_2{nullptr};
  Field *f_1{nullptr};
  Field *f_2{nullptr};

  HeaderVLTest()
      : testHeaderType("test_t", 0) {
    testHeaderType.push_back_VL_field("VLf",
                                      4  /* max header bytes */,
                                      nullptr  /* field length expr */);
    testHeaderType.push_back_field("f16", 16);
    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);
  }

  virtual void SetUp() {
    phv = phv_factory.create();
    hdr_1 = &phv->get_header(testHeader1);
    hdr_2 = &phv->get_header(testHeader2);
    f_1 = &hdr_1->get_field(0);
    f_2 = &hdr_2->get_field(0);
  }

  // virtual void TearDown() {}

  void extract_both() {
    // we need to append 16 bits for second field of the header (non VL field)
    std::string data_1_ = data_1 + std::string("\x11\x11");
    std::string data_2_ = data_2 + std::string("\x11\x11");
    hdr_1->extract_VL(data_1_.data(), VL_nbits_1);
    hdr_2->extract_VL(data_2_.data(), VL_nbits_2);
    EXPECT_EQ(data_1.data(), f_1->get_string());
    EXPECT_EQ(data_2.data(), f_2->get_string());
    EXPECT_EQ(2 + 2, hdr_1->get_nbytes_packet());
    EXPECT_EQ(2 + 1, hdr_2->get_nbytes_packet());
  }
};

TEST_F(HeaderVLTest, Attributes) {
  EXPECT_TRUE(hdr_1->is_VL_header());
  const auto &hdr_type = hdr_1->get_header_type();
  EXPECT_TRUE(hdr_type.is_VL_header());
  EXPECT_FALSE(hdr_type.has_VL_expr());
  EXPECT_EQ(0, hdr_type.get_VL_offset());
  EXPECT_EQ(4, hdr_type.get_VL_max_header_bytes());
  EXPECT_EQ(2, hdr_1->recompute_nbytes_packet());
}

TEST_F(HeaderVLTest, Extract) {
  std::string data_1_ = data_1 + std::string("\x11\x11");
  hdr_1->extract_VL(data_1_.data(), VL_nbits_1);
  ASSERT_EQ(data_1.data(), f_1->get_string());
}

TEST_F(HeaderVLTest, Reset) {
  std::string data_1_ = data_1 + std::string("\x11\x11");
  EXPECT_EQ(2, hdr_1->get_nbytes_packet());
  hdr_1->extract_VL(data_1_.data(), VL_nbits_1);
  EXPECT_EQ(2 + 2, hdr_1->get_nbytes_packet());
  hdr_1->reset_VL_header();
  EXPECT_EQ(2, hdr_1->get_nbytes_packet());
}

TEST_F(HeaderVLTest, Swap) {
  extract_both();

  hdr_1->swap_values(hdr_2);
  ASSERT_EQ(data_2.data(), f_1->get_string());
  ASSERT_EQ(data_1.data(), f_2->get_string());
  EXPECT_EQ(2 + 1, hdr_1->get_nbytes_packet());
  EXPECT_EQ(2 + 2, hdr_2->get_nbytes_packet());
}

TEST_F(HeaderVLTest, Copy) {
  extract_both();

  hdr_1->copy_fields(*hdr_2);
  ASSERT_EQ(data_2.data(), f_1->get_string());
  ASSERT_EQ(data_2.data(), f_2->get_string());
  EXPECT_EQ(2 + 1, hdr_1->get_nbytes_packet());
  EXPECT_EQ(2 + 1, hdr_2->get_nbytes_packet());
}

TEST_F(HeaderVLTest, AssignField) {
  extract_both();

  f_1->assign_VL(*f_2);
  ASSERT_EQ(data_2.data(), f_1->get_string());
  ASSERT_EQ(data_2.data(), f_2->get_string());
  EXPECT_EQ(2 + 1, hdr_1->get_nbytes_packet());
  EXPECT_EQ(2 + 1, hdr_2->get_nbytes_packet());
}
