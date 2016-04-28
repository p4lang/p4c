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

using namespace bm;

// Google Test fixture for header stack tests
class HeaderStackTest : public ::testing::Test {
protected:

  PHVFactory phv_factory;
  std::unique_ptr<PHV> phv;

  HeaderType testHeaderType;
  header_id_t testHeader_0{0}, testHeader_1{1}, testHeader_2{2};

  header_stack_id_t testHeaderStack{0};

  size_t stack_depth{3};

  HeaderStackTest()
    : testHeaderType("test_t", 0) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    phv_factory.push_back_header("test_0", testHeader_0, testHeaderType);
    phv_factory.push_back_header("test_1", testHeader_1, testHeaderType);
    phv_factory.push_back_header("test_2", testHeader_2, testHeaderType);

    const std::vector<header_id_t> headers =
      {testHeader_0, testHeader_1, testHeader_2};
    phv_factory.push_back_header_stack("test_stack", testHeaderStack,
				       testHeaderType, headers);
  }

  virtual void SetUp() {
    phv = phv_factory.create();
  }

  // virtual void TearDown() {}
};

TEST_F(HeaderStackTest, Basic) {
  const HeaderStack &stack = phv->get_header_stack(testHeaderStack);

  ASSERT_EQ(stack_depth, stack.get_depth());

  ASSERT_EQ(0u, stack.get_count());
}

TEST_F(HeaderStackTest, PushBack) {
  HeaderStack &stack = phv->get_header_stack(testHeaderStack);

  const Header &h0 = phv->get_header(testHeader_0);
  const Header &h1 = phv->get_header(testHeader_1);
  const Header &h2 = phv->get_header(testHeader_2);

  ASSERT_FALSE(h0.is_valid());
  ASSERT_FALSE(h1.is_valid());
  ASSERT_FALSE(h2.is_valid());

  ASSERT_EQ(0u, stack.get_count());

  for(size_t i = 0; i < stack_depth; i++) {
    ASSERT_EQ(1u, stack.push_back());
    ASSERT_EQ(i + 1, stack.get_count());
  }
  
  ASSERT_EQ(0u, stack.push_back());
  ASSERT_EQ(stack_depth, stack.get_count());

  ASSERT_TRUE(h0.is_valid());
  ASSERT_TRUE(h1.is_valid());
  ASSERT_TRUE(h2.is_valid());
}

TEST_F(HeaderStackTest, PopBack) {
  HeaderStack &stack = phv->get_header_stack(testHeaderStack);

  const Header &h0 = phv->get_header(testHeader_0);

  ASSERT_FALSE(h0.is_valid());
  ASSERT_EQ(1u, stack.push_back());
  ASSERT_TRUE(h0.is_valid());

  ASSERT_EQ(1u, stack.pop_back());
  ASSERT_FALSE(h0.is_valid());

  ASSERT_EQ(0u, stack.pop_back()); // empty so nothing to pop
}

TEST_F(HeaderStackTest, PushFront) {
  HeaderStack &stack = phv->get_header_stack(testHeaderStack);

  Header &h0 = phv->get_header(testHeader_0);
  Header &h1 = phv->get_header(testHeader_1);
  Header &h2 = phv->get_header(testHeader_2);

  ASSERT_FALSE(h0.is_valid());

  Field &f0_0 = h0.get_field(0);
  Field &f1_0 = h1.get_field(0);
  Field &f2_0 = h2.get_field(0);

  unsigned int v0 = 10u; unsigned int v1 = 11u; unsigned int v2 = 12u;

  ASSERT_EQ(0u, stack.get_count());

  ASSERT_EQ(1u, stack.push_front());
  ASSERT_EQ(1u, stack.get_count());
  ASSERT_TRUE(h0.is_valid());
  f0_0.set(v0);

  ASSERT_EQ(1u, stack.push_front());
  ASSERT_EQ(2u, stack.get_count());
  ASSERT_TRUE(h0.is_valid());
  ASSERT_TRUE(h1.is_valid());
  ASSERT_EQ(v0, f1_0.get_uint());
  f0_0.set(v1);

  ASSERT_EQ(1u, stack.push_front());
  ASSERT_EQ(3u, stack.get_count());
  ASSERT_TRUE(h0.is_valid());
  ASSERT_TRUE(h1.is_valid());
  ASSERT_TRUE(h2.is_valid());
  ASSERT_EQ(v0, f2_0.get_uint());
  ASSERT_EQ(v1, f1_0.get_uint());
  f0_0.set(v2);

  // we can do another push front, the last header will be discarded
  ASSERT_EQ(1u, stack.push_front());
  ASSERT_EQ(3u, stack.get_count());
  ASSERT_TRUE(h0.is_valid());
  ASSERT_TRUE(h1.is_valid());
  ASSERT_TRUE(h2.is_valid());
  ASSERT_EQ(v1, f2_0.get_uint());
  ASSERT_EQ(v2, f1_0.get_uint());
}

TEST_F(HeaderStackTest, PushFrontNum) {
  HeaderStack &stack = phv->get_header_stack(testHeaderStack);

  Header &h0 = phv->get_header(testHeader_0);
  Header &h1 = phv->get_header(testHeader_1);
  Header &h2 = phv->get_header(testHeader_2);

  Field &f0_0 = h0.get_field(0);
  Field &f1_0 = h1.get_field(0);
  Field &f2_0 = h2.get_field(0);

  unsigned int v0 = 10u;

  ASSERT_EQ(0u, stack.get_count());

  ASSERT_EQ(1u, stack.push_front());
  ASSERT_EQ(1u, stack.get_count());
  ASSERT_TRUE(h0.is_valid());
  f0_0.set(v0);

  ASSERT_EQ(2u, stack.push_front(2));
  ASSERT_EQ(3u, stack.get_count());
  ASSERT_TRUE(h0.is_valid());
  ASSERT_TRUE(h1.is_valid());
  ASSERT_TRUE(h2.is_valid());
  ASSERT_EQ(v0, f2_0.get_uint());
}

TEST_F(HeaderStackTest, PopFront) {
  HeaderStack &stack = phv->get_header_stack(testHeaderStack);

  ASSERT_EQ(2u, stack.push_front(2)); // add 2 headers

  Header &h0 = phv->get_header(testHeader_0);
  Header &h1 = phv->get_header(testHeader_1);
  Header &h2 = phv->get_header(testHeader_2);

  ASSERT_TRUE(h0.is_valid());
  ASSERT_TRUE(h1.is_valid());
  ASSERT_FALSE(h2.is_valid());

  Field &f0_0 = h0.get_field(0);
  Field &f1_0 = h1.get_field(0);
  Field &f2_0 = h2.get_field(0);

  const unsigned int v0 = 10u; const unsigned int v1 = 11u;
  const std::string v1_hex("0x000b");
  f0_0.set(v0); f1_0.set(v1);

  ASSERT_EQ(2u, stack.get_count());

  ASSERT_EQ(1u, stack.pop_front());
  ASSERT_EQ(1u, stack.get_count());
  ASSERT_FALSE(h2.is_valid());
  ASSERT_FALSE(h1.is_valid());
  ASSERT_EQ(v1, f0_0.get_uint());
  ASSERT_EQ(ByteContainer(v1_hex), f0_0.get_bytes());

  ASSERT_EQ(1u, stack.pop_front());
  ASSERT_EQ(0u, stack.get_count());
  ASSERT_FALSE(h2.is_valid());
  ASSERT_FALSE(h1.is_valid());
  ASSERT_FALSE(h0.is_valid());

  ASSERT_EQ(0u, stack.pop_front()); // empty so nothing popped
  ASSERT_EQ(0u, stack.get_count());
}

TEST_F(HeaderStackTest, PopFrontNum) {
  HeaderStack &stack = phv->get_header_stack(testHeaderStack);

  ASSERT_EQ(3u, stack.push_front(3)); // add 3 headers

  Header &h0 = phv->get_header(testHeader_0);
  Header &h1 = phv->get_header(testHeader_1);
  Header &h2 = phv->get_header(testHeader_2);

  ASSERT_TRUE(h0.is_valid());
  ASSERT_TRUE(h1.is_valid());
  ASSERT_TRUE(h2.is_valid());

  Field &f0_0 = h0.get_field(0);
  Field &f1_0 = h1.get_field(0);
  Field &f2_0 = h2.get_field(0);

  unsigned int v0 = 10u; unsigned int v1 = 11u; unsigned int v2 = 12u;
  f0_0.set(v0); f1_0.set(v1); f2_0.set(v2);

  ASSERT_EQ(3u, stack.get_count());

  ASSERT_EQ(0u, stack.pop_front(0));
  ASSERT_EQ(3u, stack.get_count());
  ASSERT_TRUE(h0.is_valid());
  ASSERT_TRUE(h1.is_valid());
  ASSERT_TRUE(h2.is_valid());

  ASSERT_EQ(2u, stack.pop_front(2));
  ASSERT_EQ(1u, stack.get_count());
  ASSERT_TRUE(h0.is_valid());
  ASSERT_FALSE(h1.is_valid());
  ASSERT_FALSE(h2.is_valid());
  ASSERT_EQ(v2, f0_0.get_uint());

  ASSERT_EQ(1u, stack.pop_front(2));
  ASSERT_EQ(0u, stack.get_count());
  ASSERT_FALSE(h0.is_valid());
  ASSERT_FALSE(h1.is_valid());
  ASSERT_FALSE(h2.is_valid());
}
