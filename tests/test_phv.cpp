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

#include <memory>
#include <string>
#include <vector>

#include <cassert>

using namespace bm;

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

TEST_F(PHVTest, NumHeaders) {
  ASSERT_EQ(2u, phv->num_headers());
}

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

TEST_F(PHVTest, CopyHeadersWithStack) {
  header_stack_id_t testHeaderStack(0);
  phv.reset(nullptr);
  const std::vector<header_id_t> headers = {testHeader1, testHeader2};
  phv_factory.push_back_header_stack("test_stack", testHeaderStack,
                                     testHeaderType, headers);
  phv = phv_factory.create();
  std::unique_ptr<PHV> phv_2 = phv_factory.create();

  auto &stack = phv->get_header_stack(testHeaderStack);
  ASSERT_EQ(1u, stack.push_back());
  // needs to work for both legacy stacks and P4_16 stacks, so we explicitly
  // mark the header valid
  phv->get_header(testHeader1).mark_valid();
  EXPECT_TRUE(phv->get_header(testHeader1).is_valid());
  EXPECT_FALSE(phv_2->get_header(testHeader1).is_valid());
  EXPECT_EQ(stack.get_count(), 1u);

  phv_2->copy_headers(*phv);

  const auto &stack_2 = phv_2->get_header_stack(testHeaderStack);
  EXPECT_EQ(stack_2.get_count(), stack.get_count());
  EXPECT_TRUE(phv_2->get_header(testHeader1).is_valid());
}

TEST_F(PHVTest, CopyHeadersWithUnion) {
  header_union_id_t testHeaderUnion(0);
  phv.reset(nullptr);
  const std::vector<header_id_t> headers = {testHeader1, testHeader2};
  phv_factory.push_back_header_union("test_union", testHeaderUnion, headers);
  phv = phv_factory.create();
  std::unique_ptr<PHV> phv_2 = phv_factory.create();

  auto &header_union = phv->get_header_union(testHeaderUnion);
  auto &hdr = phv->get_header(testHeader2);
  EXPECT_FALSE(header_union.is_valid());
  hdr.mark_valid();
  EXPECT_EQ(&hdr, header_union.get_valid_header());

  phv_2->copy_headers(*phv);

  auto &header_union_2 = phv_2->get_header_union(testHeaderUnion);
  auto &hdr_2 = phv_2->get_header(testHeader2);
  EXPECT_TRUE(header_union_2.is_valid());
  EXPECT_EQ(&hdr_2, header_union_2.get_valid_header());
}

// we are testing that the $valid$ hidden field is properly added internally by
// the HeaderType class, that it is properly accessible and that it is updated
// properly.
TEST_F(PHVTest, HiddenValid) {
  ASSERT_EQ(2u, testHeaderType.get_hidden_offset(HeaderType::HiddenF::VALID));

  ASSERT_EQ(2u, testHeaderType.get_field_offset("$valid$"));

  const auto &finfo = testHeaderType.get_finfo(2u);
  ASSERT_EQ("$valid$", finfo.name);
  ASSERT_EQ(1, finfo.bitwidth);
  ASSERT_FALSE(finfo.is_signed);
  ASSERT_TRUE(finfo.is_hidden);

  const Field &f = phv->get_field("test1.$valid$");
  ASSERT_EQ(0, f.get_int());
  Header &h = phv->get_header(testHeader1);
  h.mark_valid();
  ASSERT_EQ(1, f.get_int());
  h.mark_invalid();
  ASSERT_EQ(0, f.get_int());

  // check that both point to the same thing
  ASSERT_EQ(&f, &phv->get_field(testHeader1, 2u));
}

TEST_F(PHVTest, FieldAlias) {
  phv_factory.add_field_alias("best.alias.ever", "test1.f16");
  std::unique_ptr<PHV> phv_2 = phv_factory.create();

  const Field &f = phv_2->get_field("test1.f16");
  const Field &f_alias = phv_2->get_field("best.alias.ever");
  const Field &f_other = phv_2->get_field("test1.f48");

  ASSERT_NE(&f_other, &f);
  ASSERT_NE(&f_other, &f_alias);

  ASSERT_EQ(&f, &f_alias);
}

TEST_F(PHVTest, FieldAliasDup) {
  // in this test we create an alias with the same name as an actual field;
  // currently bmv2 gives priority to the alias
  phv_factory.add_field_alias("test1.f16", "test1.f48");
  std::unique_ptr<PHV> phv_2 = phv_factory.create();

  const Field &f = phv_2->get_field("test1.f48");
  const Field &f_alias = phv_2->get_field("test1.f16");

  ASSERT_EQ(&f, &f_alias);
}

TEST_F(PHVTest, WrittenTo) {
  auto &f = phv->get_field("test1.f16");
  auto reset = [&f]() {
    f.set_written_to(false);
    ASSERT_FALSE(f.get_written_to());
  };
  ASSERT_FALSE(f.get_written_to());

  // testing different ways of modifying a field
  f.set(0xab);
  ASSERT_TRUE(f.get_written_to());
  reset();
  f.add(Data(1), Data(1));
  ASSERT_TRUE(f.get_written_to());
  reset();
  const char data[] = {'a', 'b'};
  f.set_bytes(data, sizeof(data));
  ASSERT_TRUE(f.get_written_to());
  reset();
  f.extract(data, 0);
  ASSERT_TRUE(f.get_written_to());
  reset();

  // modifying flag directly
  f.set_written_to(true);
  ASSERT_TRUE(f.get_written_to());
  reset();

  // through header method
  auto &hdr = phv->get_header("test1");
  hdr.set_written_to(true);
  ASSERT_TRUE(f.get_written_to());
  hdr.set_written_to(false);
  ASSERT_FALSE(f.get_written_to());

  // through phv method
  phv->set_written_to(true);
  ASSERT_TRUE(f.get_written_to());
  phv->set_written_to(false);
  ASSERT_FALSE(f.get_written_to());
}

using testing::Types;

template <typename IteratorType>
struct PHVRef { };

template <>
struct PHVRef<PHV::header_name_iterator> { using type = PHV&; };

template <>
struct PHVRef<PHV::const_header_name_iterator> { using type = const PHV&; };

template <typename IteratorType>
class PHVHeaderNameIteratorTest : public PHVTest {
  virtual void SetUp() {
    PHVTest::SetUp();
    phv = phv_factory.create();
  }
};

using NameIteratorTypes = Types<PHV::header_name_iterator,
                                PHV::const_header_name_iterator>;

TYPED_TEST_CASE(PHVHeaderNameIteratorTest, NameIteratorTypes);

TYPED_TEST(PHVHeaderNameIteratorTest, Iterate) {
  typename PHVRef<TypeParam>::type phv_ref = *(this->phv).get();
  for (TypeParam it = phv_ref.header_name_begin();
       it != phv_ref.header_name_end(); ++it) {
    const std::string &header_name = it->first;
    Header &header = it->second;
    const std::string &header_name_2 = header.get_name();
    ASSERT_EQ(header_name, header_name_2);
  }
  ASSERT_EQ(
      phv_ref.num_headers(),
      std::distance(phv_ref.header_name_begin(), phv_ref.header_name_end()));
}

template <>
struct PHVRef<PHV::header_iterator> { using type = PHV&; };

template <>
struct PHVRef<PHV::const_header_iterator> { using type = const PHV&; };

template <typename IteratorType>
class PHVHeaderIteratorTest : public PHVTest {
  virtual void SetUp() {
    PHVTest::SetUp();
    phv = phv_factory.create();
  }
};

using IteratorTypes = Types<PHV::header_iterator, PHV::const_header_iterator>;

TYPED_TEST_CASE(PHVHeaderIteratorTest, IteratorTypes);

template <typename IteratorType>
struct HeaderRef { };

template <>
struct HeaderRef<PHV::header_iterator> { using type = Header&; };

template <>
struct HeaderRef<PHV::const_header_iterator> { using type = const Header&; };

TYPED_TEST(PHVHeaderIteratorTest, Iterate) {
  typename PHVRef<TypeParam>::type phv_ref = *(this->phv).get();
  for (TypeParam it = phv_ref.header_begin();
       it != phv_ref.header_end(); ++it) {
    typename HeaderRef<TypeParam>::type &header = *it;
    (void) header;
  }
  ASSERT_EQ(
      phv_ref.num_headers(),
      std::distance(phv_ref.header_name_begin(), phv_ref.header_name_end()));
}
