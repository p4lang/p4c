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
#include <type_traits>
#include <vector>

namespace bm {

namespace testing {

// Because we want to test both implementations, we cannot use
// PHVFactory::push_back_header_stack. We therefore subclass
// bm::detail::StackLegacy<bm::Header> to access the set_next_element method and
// build the header stacks ourselves.

class HeaderStackLegacy : public detail::StackLegacy<Header> {
 public:
  HeaderStackLegacy(const std::string &name, p4object_id_t id)
      : detail::StackLegacy<bm::Header>(name, id) { }

  void set_next_header(Header &hdr) {  // NOLINT(runtime/references)
    this->set_next_element(hdr);
  }
};

// if PushValid is true, emulate legacy behavior by making pushed headers valid
template <bool PushValid>
class HeaderStackP4_16 : public detail::StackP4_16<Header> {
 public:
  HeaderStackP4_16(const std::string &name, p4object_id_t id)
      : detail::StackP4_16<bm::Header>(name, id) { }

  void set_next_header(Header &hdr) {  // NOLINT(runtime/references)
    this->set_next_element(hdr);
  }

  template <bool B = PushValid>
  typename std::enable_if<B, size_t>::type push_front() {
    auto s = detail::StackP4_16<Header>::push_front();
    this->at(0).mark_valid();
    return s;
  }

  template <bool B = PushValid>
  typename std::enable_if<B, size_t>::type push_front(size_t num) {
    auto s = detail::StackP4_16<Header>::push_front(num);
    for (size_t i = 0; i < s; i++) this->at(i).mark_valid();
    return s;
  }

  template <bool B = PushValid>
  typename std::enable_if<!B, size_t>::type push_front() {
    return detail::StackP4_16<Header>::push_front();
  }

  template <bool B = PushValid>
  typename std::enable_if<!B, size_t>::type push_front(size_t num) {
    return detail::StackP4_16<Header>::push_front(num);
  }
};

// Google Test fixture for header stack tests
template <typename HSType>
class HeaderStackTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;
  std::unique_ptr<PHV> phv;

  HeaderType testHeaderType;
  header_id_t testHeader_0{0}, testHeader_1{1}, testHeader_2{2};

  header_stack_id_t testHeaderStack{0};
  HSType stack;

  size_t stack_depth{3};

  HeaderStackTest()
      : testHeaderType("test_t", 0),
        stack("test_stack", testHeaderStack) {
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f48", 48);
    phv_factory.push_back_header("test_0", testHeader_0, testHeaderType);
    phv_factory.push_back_header("test_1", testHeader_1, testHeaderType);
    phv_factory.push_back_header("test_2", testHeader_2, testHeaderType);

    // Cannot use this if we want to test both stack implementations
    // independently of whether preprocessor flag BM_WP4_16_STACKS is used.

    // const std::vector<header_id_t> headers =
    //   {testHeader_0, testHeader_1, testHeader_2};
    // phv_factory.push_back_header_stack("test_stack", testHeaderStack,
    //                                    testHeaderType, headers);
  }

  virtual void SetUp() {
    phv = phv_factory.create();
    for (auto header_id : {testHeader_0, testHeader_1, testHeader_2})
      stack.set_next_header(phv->get_header(header_id));
  }

  // virtual void TearDown() {}
};

using HeaderStackTypes =
    ::testing::Types<HeaderStackLegacy, HeaderStackP4_16<true> >;

TYPED_TEST_CASE(HeaderStackTest, HeaderStackTypes);

TYPED_TEST(HeaderStackTest, Basic) {
  auto &stack = this->stack;

  ASSERT_EQ(this->stack_depth, stack.get_depth());

  ASSERT_EQ(0u, stack.get_count());
}

TYPED_TEST(HeaderStackTest, PushBack) {
  auto &stack = this->stack;
  auto *phv = this->phv.get();

  const auto &h0 = phv->get_header(this->testHeader_0);
  const auto &h1 = phv->get_header(this->testHeader_1);
  const auto &h2 = phv->get_header(this->testHeader_2);

  ASSERT_FALSE(h0.is_valid());
  ASSERT_FALSE(h1.is_valid());
  ASSERT_FALSE(h2.is_valid());

  ASSERT_EQ(0u, stack.get_count());

  for (size_t i = 0; i < this->stack_depth; i++) {
    ASSERT_EQ(1u, stack.push_back());
    ASSERT_EQ(i + 1, stack.get_count());
  }

  ASSERT_EQ(0u, stack.push_back());
  ASSERT_EQ(this->stack_depth, stack.get_count());

  ASSERT_TRUE(h0.is_valid());
  ASSERT_TRUE(h1.is_valid());
  ASSERT_TRUE(h2.is_valid());
}

TYPED_TEST(HeaderStackTest, PopBack) {
  auto &stack = this->stack;
  auto *phv = this->phv.get();

  const auto &h0 = phv->get_header(this->testHeader_0);

  ASSERT_FALSE(h0.is_valid());
  ASSERT_EQ(1u, stack.push_back());
  ASSERT_TRUE(h0.is_valid());

  ASSERT_EQ(1u, stack.pop_back());
  ASSERT_FALSE(h0.is_valid());

  ASSERT_EQ(0u, stack.pop_back());  // empty so nothing to pop
}

TYPED_TEST(HeaderStackTest, PushFront) {
  auto &stack = this->stack;
  auto *phv = this->phv.get();

  auto &h0 = phv->get_header(this->testHeader_0);
  auto &h1 = phv->get_header(this->testHeader_1);
  auto &h2 = phv->get_header(this->testHeader_2);

  ASSERT_FALSE(h0.is_valid());

  auto &f0_0 = h0.get_field(0);
  auto &f1_0 = h1.get_field(0);
  auto &f2_0 = h2.get_field(0);

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

TYPED_TEST(HeaderStackTest, PushFrontNum) {
  auto &stack = this->stack;
  auto *phv = this->phv.get();

  auto &h0 = phv->get_header(this->testHeader_0);
  auto &h1 = phv->get_header(this->testHeader_1);
  auto &h2 = phv->get_header(this->testHeader_2);

  auto &f0_0 = h0.get_field(0);
  auto &f2_0 = h2.get_field(0);

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

TYPED_TEST(HeaderStackTest, PopFront) {
  auto &stack = this->stack;
  auto *phv = this->phv.get();

  ASSERT_EQ(2u, stack.push_front(2));  // add 2 headers

  auto &h0 = phv->get_header(this->testHeader_0);
  auto &h1 = phv->get_header(this->testHeader_1);
  auto &h2 = phv->get_header(this->testHeader_2);

  ASSERT_TRUE(h0.is_valid());
  ASSERT_TRUE(h1.is_valid());
  ASSERT_FALSE(h2.is_valid());

  auto &f0_0 = h0.get_field(0);
  auto &f1_0 = h1.get_field(0);

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

  // not true for P4_16 stacks for which we always shift independently of the
  // value of next
  // ASSERT_EQ(0u, stack.pop_front());  // empty so nothing popped
  stack.pop_front();
  ASSERT_EQ(0u, stack.get_count());
}

TYPED_TEST(HeaderStackTest, PopFrontNum) {
  auto &stack = this->stack;
  auto *phv = this->phv.get();

  ASSERT_EQ(3u, stack.push_front(3));  // add 3 headers

  auto &h0 = phv->get_header(this->testHeader_0);
  auto &h1 = phv->get_header(this->testHeader_1);
  auto &h2 = phv->get_header(this->testHeader_2);

  ASSERT_TRUE(h0.is_valid());
  ASSERT_TRUE(h1.is_valid());
  ASSERT_TRUE(h2.is_valid());

  auto &f0_0 = h0.get_field(0);
  auto &f1_0 = h1.get_field(0);
  auto &f2_0 = h2.get_field(0);

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

  // not true for P4_16 stacks for which we always shift independently of the
  // value of next
  // ASSERT_EQ(1u, stack.pop_front(2));
  stack.pop_front(2);
  ASSERT_EQ(0u, stack.get_count());
  ASSERT_FALSE(h0.is_valid());
  ASSERT_FALSE(h1.is_valid());
  ASSERT_FALSE(h2.is_valid());
}

// inheritance to reuse members, constructor, etc
class HeaderStackP4_16Test
    : public HeaderStackTest<HeaderStackP4_16<false> > { };

TEST_F(HeaderStackP4_16Test, PushFront) {
  auto &stack = this->stack;
  auto *phv = this->phv.get();

  auto &h0 = phv->get_header(this->testHeader_0);
  auto &h1 = phv->get_header(this->testHeader_1);
  auto &h2 = phv->get_header(this->testHeader_2);

  h1.mark_valid();
  EXPECT_EQ(1u, stack.push_front());
  EXPECT_FALSE(h0.is_valid());  // push doesn't make valid
  EXPECT_FALSE(h1.is_valid());
  EXPECT_TRUE(h2.is_valid());

  h0.mark_valid();
  EXPECT_EQ(2u, stack.push_front(2));
  EXPECT_FALSE(h0.is_valid());
  EXPECT_FALSE(h1.is_valid());
  EXPECT_TRUE(h2.is_valid());
}

TEST_F(HeaderStackP4_16Test, PopFront) {
  auto &stack = this->stack;
  auto *phv = this->phv.get();

  auto &h0 = phv->get_header(this->testHeader_0);
  auto &h1 = phv->get_header(this->testHeader_1);
  auto &h2 = phv->get_header(this->testHeader_2);

  h0.mark_valid(); h2.mark_valid();
  EXPECT_EQ(1u, stack.pop_front());
  EXPECT_FALSE(h0.is_valid());
  EXPECT_TRUE(h1.is_valid());
  EXPECT_FALSE(h2.is_valid());

  h2.mark_valid();
  EXPECT_EQ(2u, stack.pop_front(2));
  EXPECT_TRUE(h0.is_valid());
  EXPECT_FALSE(h1.is_valid());
  EXPECT_FALSE(h2.is_valid());
}

}  // namespace testing

}  // namespace bm
