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
#include <vector>

using namespace bm;

class HeaderUnionBaseTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;
  std::unique_ptr<PHV> phv;

  HeaderType testHeaderType0, testHeaderType1;
  header_id_t testHeader00{0}, testHeader01{1};
  header_id_t testHeader10{2}, testHeader11{3};

  header_union_id_t testHeaderUnion0{0};
  header_union_id_t testHeaderUnion1{1};

  HeaderUnionBaseTest()
      : testHeaderType0("test0_t", 0), testHeaderType1("test1_t", 1) { }

  // A little ugly: the subclass needs to push the fields to the header types
  // before PHVFactory::push_back_header is called. Subclasses can call this
  // method at the end of their constrcutor after pushing the fields to the
  // header types.
  void set_up_phv_factory() {
    phv_factory.push_back_header("test00", testHeader00, testHeaderType0);
    phv_factory.push_back_header("test01", testHeader01, testHeaderType1);
    phv_factory.push_back_header("test10", testHeader10, testHeaderType0);
    phv_factory.push_back_header("test11", testHeader11, testHeaderType1);

    const std::vector<header_id_t> headers0({testHeader00, testHeader01});
    phv_factory.push_back_header_union(
        "test_union0", testHeaderUnion0, headers0);
    const std::vector<header_id_t> headers1({testHeader10, testHeader11});
    phv_factory.push_back_header_union(
        "test_union1", testHeaderUnion1, headers1);
  }

  virtual void SetUp() {
    phv = phv_factory.create();
  }
};


class HeaderUnionTest : public HeaderUnionBaseTest {
 protected:
  HeaderUnionTest()
      : HeaderUnionBaseTest() {
    testHeaderType0.push_back_field("f16", 16);
    testHeaderType0.push_back_field("f48", 48);
    testHeaderType1.push_back_field("f32", 32);

    set_up_phv_factory();
  }
};

TEST_F(HeaderUnionTest, Validity) {
  auto &header_union = phv->get_header_union(testHeaderUnion0);
  auto &header0 = phv->get_header(testHeader00);
  auto &header1 = phv->get_header(testHeader01);
  EXPECT_EQ(nullptr, header_union.get_valid_header());
  EXPECT_FALSE(header_union.is_valid());

  header_union.make_header_valid(1);
  EXPECT_EQ(&header1, header_union.get_valid_header());
  EXPECT_TRUE(header_union.is_valid());

  header_union.make_header_invalid(0);
  EXPECT_EQ(&header1, header_union.get_valid_header());

  header_union.make_header_valid(0);
  EXPECT_EQ(&header0, header_union.get_valid_header());

  header_union.make_header_invalid(0);
  EXPECT_EQ(nullptr, header_union.get_valid_header());
}

TEST_F(HeaderUnionTest, ValidityThruHeader) {
  auto &header_union = phv->get_header_union(testHeaderUnion0);
  auto &header0 = phv->get_header(testHeader00);
  auto &header1 = phv->get_header(testHeader01);
  EXPECT_EQ(nullptr, header_union.get_valid_header());
  EXPECT_FALSE(header0.is_valid());
  EXPECT_FALSE(header1.is_valid());

  header1.mark_valid();
  EXPECT_FALSE(header0.is_valid());
  EXPECT_TRUE(header1.is_valid());
  EXPECT_EQ(&header1, header_union.get_valid_header());

  // make the same header valid again, nothing should change
  header1.mark_valid();
  EXPECT_FALSE(header0.is_valid());
  EXPECT_TRUE(header1.is_valid());
  EXPECT_EQ(&header1, header_union.get_valid_header());

  header0.mark_invalid();
  EXPECT_FALSE(header0.is_valid());
  EXPECT_TRUE(header1.is_valid());
  EXPECT_EQ(&header1, header_union.get_valid_header());

  header0.mark_valid();
  EXPECT_TRUE(header0.is_valid());
  EXPECT_FALSE(header1.is_valid());
  EXPECT_EQ(&header0, header_union.get_valid_header());

  header0.mark_invalid();
  EXPECT_FALSE(header0.is_valid());
  EXPECT_FALSE(header1.is_valid());
  EXPECT_EQ(nullptr, header_union.get_valid_header());
}

TEST_F(HeaderUnionTest, SwapValues) {
  auto &header_union0 = phv->get_header_union(testHeaderUnion0);
  auto &header00 = phv->get_header(testHeader00);
  auto &header01 = phv->get_header(testHeader01);
  auto &header_union1 = phv->get_header_union(testHeaderUnion1);
  auto &header10 = phv->get_header(testHeader10);
  auto &header11 = phv->get_header(testHeader11);
  unsigned int v(0xaba);
  auto &f01_32 = header01.get_field(0);
  auto &f11_32 = header11.get_field(0);
  f01_32.set(v);

  header01.mark_valid();
  header10.mark_valid();
  EXPECT_FALSE(header00.is_valid());
  EXPECT_TRUE(header01.is_valid());
  EXPECT_EQ(&header01, header_union0.get_valid_header());
  EXPECT_TRUE(header10.is_valid());
  EXPECT_FALSE(header11.is_valid());
  EXPECT_EQ(&header10, header_union1.get_valid_header());

  header_union0.swap_values(&header_union1);
  EXPECT_TRUE(header00.is_valid());
  EXPECT_FALSE(header01.is_valid());
  EXPECT_EQ(&header00, header_union0.get_valid_header());
  EXPECT_FALSE(header10.is_valid());
  EXPECT_TRUE(header11.is_valid());
  EXPECT_EQ(&header11, header_union1.get_valid_header());
  EXPECT_EQ(v, f11_32.get<decltype(v)>());
}

TEST_F(HeaderUnionTest, AssignUnion) {
  auto primitive = ActionOpcodesMap::get_instance()->get_primitive(
      "assign_union");
  ASSERT_NE(nullptr, primitive);
  auto assign_union = dynamic_cast<core::assign_union *>(primitive.get());
  ASSERT_NE(nullptr, assign_union);

  auto &header_union0 = phv->get_header_union(testHeaderUnion0);
  auto &header00 = phv->get_header(testHeader00);
  auto &header01 = phv->get_header(testHeader01);
  auto &header_union1 = phv->get_header_union(testHeaderUnion1);
  auto &header10 = phv->get_header(testHeader10);
  auto &header11 = phv->get_header(testHeader11);
  const std::string data00(8, '\xab');
  const std::string data01(4, '\xcd');
  header00.extract(data00.data(), *phv);
  EXPECT_TRUE(header00.is_valid());
  // extracting to h01 makes h00 invalid because both headers are in a union
  header01.extract(data01.data(), *phv);
  EXPECT_FALSE(header00.is_valid());
  EXPECT_TRUE(header01.is_valid());

  (*assign_union)(header_union1, header_union0);
  EXPECT_FALSE(header10.is_valid());
  EXPECT_TRUE(header11.is_valid());
  EXPECT_FALSE(header10.cmp(header00));
  EXPECT_TRUE(header11.cmp(header01));

  header10.mark_valid();
  (*assign_union)(header_union0, header_union1);
  EXPECT_TRUE(header00.is_valid());
  EXPECT_FALSE(header01.is_valid());
  EXPECT_TRUE(header00.cmp(header10));
  EXPECT_FALSE(header01.cmp(header11));
}

// Special case where one of the headers in the union has a VL field
class HeaderUnionVLTest : public HeaderUnionBaseTest {
 protected:
  HeaderUnionVLTest()
      : HeaderUnionBaseTest() {
    testHeaderType0.push_back_field("f16", 16);
    testHeaderType0.push_back_field("f48", 48);
    testHeaderType1.push_back_VL_field("fVL", 4, nullptr);
    testHeaderType1.push_back_field("f32", 32);

    set_up_phv_factory();
  }
};

TEST_F(HeaderUnionVLTest, AssignUnion) {
  auto primitive = ActionOpcodesMap::get_instance()->get_primitive(
      "assign_union");
  ASSERT_NE(nullptr, primitive);
  auto assign_union = dynamic_cast<core::assign_union *>(primitive.get());
  ASSERT_NE(nullptr, assign_union);

  auto &header_union0 = phv->get_header_union(testHeaderUnion0);
  auto &header00 = phv->get_header(testHeader00);
  auto &header01 = phv->get_header(testHeader01);
  auto &header_union1 = phv->get_header_union(testHeaderUnion1);
  auto &header10 = phv->get_header(testHeader10);
  auto &header11 = phv->get_header(testHeader11);
  auto &f01_VL = header01.get_field(0);
  auto &f11_VL = header11.get_field(0);
  std::string data00(8, '\xab');
  std::string data01("\xcd\xab");
  unsigned int VL_v(0xcdab);
  data01.append(4, '\xef');
  header00.extract(data00.data(), *phv);
  EXPECT_TRUE(header00.is_valid());
  // extracting to h01 makes h00 invalid because both headers are in a union
  header01.extract_VL(data01.data(), 16);  // 2-byte VL field
  EXPECT_FALSE(header00.is_valid());
  EXPECT_TRUE(header01.is_valid());
  EXPECT_EQ(VL_v, f01_VL.get<decltype(VL_v)>());

  (*assign_union)(header_union1, header_union0);
  EXPECT_FALSE(header10.is_valid());
  EXPECT_TRUE(header11.is_valid());
  EXPECT_FALSE(header10.cmp(header00));
  EXPECT_TRUE(header11.cmp(header01));
  EXPECT_EQ(VL_v, f11_VL.get<decltype(VL_v)>());

  header10.mark_valid();
  (*assign_union)(header_union0, header_union1);
  EXPECT_TRUE(header00.is_valid());
  EXPECT_FALSE(header01.is_valid());
  EXPECT_TRUE(header00.cmp(header10));
  EXPECT_FALSE(header01.cmp(header11));
}

// most of the stack code is common for header stacks and union stacks (see
// Stack<T>), so we do not have a lot of unit tests specific to union stacks.
class HeaderUnionStackTest : public HeaderUnionTest {
 protected:
  header_union_stack_id_t testHeaderUnionStack{0};
  size_t stack_depth{2};

  HeaderUnionStackTest() {
    const std::vector<header_union_id_t> unions(
        {testHeaderUnion0, testHeaderUnion1});
    phv_factory.push_back_header_union_stack(
        "test_union_stack", testHeaderUnionStack, unions);
  }
};

TEST_F(HeaderUnionStackTest, PushAndPop) {
  auto &union_stack = phv->get_header_union_stack(testHeaderUnionStack);
  auto &header_union0 = phv->get_header_union(testHeaderUnion0);
  auto &header00 = phv->get_header(testHeader00);
  auto &header01 = phv->get_header(testHeader01);
  auto &header_union1 = phv->get_header_union(testHeaderUnion1);
  auto &header10 = phv->get_header(testHeader10);
  auto &header11 = phv->get_header(testHeader11);

  EXPECT_EQ(stack_depth, union_stack.get_depth());
  EXPECT_EQ(0u, union_stack.get_count());

  union_stack.push_front();
  EXPECT_EQ(1u, union_stack.get_count());
  header01.mark_valid();
  EXPECT_EQ(&header01, header_union0.get_valid_header());
  EXPECT_EQ(nullptr, header_union1.get_valid_header());

  union_stack.push_front();
  EXPECT_EQ(2u, union_stack.get_count());
  EXPECT_TRUE(header11.is_valid());
  EXPECT_FALSE(header01.is_valid());
  EXPECT_EQ(nullptr, header_union0.get_valid_header());
  EXPECT_EQ(&header11, header_union1.get_valid_header());

  header10.mark_valid();  // swap second union
  EXPECT_EQ(&header10, header_union1.get_valid_header());
  EXPECT_FALSE(header11.is_valid());

  union_stack.pop_front();
  EXPECT_EQ(1u, union_stack.get_count());
  EXPECT_TRUE(header00.is_valid());
  EXPECT_FALSE(header01.is_valid());
  EXPECT_FALSE(header10.is_valid());
  EXPECT_FALSE(header11.is_valid());
  EXPECT_EQ(&header00, header_union0.get_valid_header());
  EXPECT_EQ(nullptr, header_union1.get_valid_header());
}
