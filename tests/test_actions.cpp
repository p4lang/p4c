#include <gtest/gtest.h>

#include <memory>
#include <string>

#include <cassert>

#include "bm_sim/actions.h"

using std::string;

int pull_test_actions() { return 0; }

// some primitive definitions for testing

class SetField : public ActionPrimitive<Field &, const Data &> {
  void operator ()(Field &f, const Data &d) {
    f.set(d);
  }
};

REGISTER_PRIMITIVE(SetField);

class Add : public ActionPrimitive<Field &, const Data &, const Data &> {
  void operator ()(Field &f, const Data &d1, const Data &d2) {
    f.add(d1, d2);
  }
};

REGISTER_PRIMITIVE(Add);

class RemoveHeader : public ActionPrimitive<Header &> {
  void operator ()(Header &hdr) {
    hdr.mark_invalid();
  }
};

REGISTER_PRIMITIVE(RemoveHeader);

class CopyHeader : public ActionPrimitive<Header &, const Header &> {
  void operator ()(Header &dst, const Header &src) {
    if(!src.is_valid()) return;
    dst.mark_valid();
    assert(dst.get_header_type_id() == src.get_header_type_id());
    for(unsigned int i = 0; i < dst.size(); i++) {
      dst[i].set(src[i]);
    }
  }
};

REGISTER_PRIMITIVE(CopyHeader);

/* this is an example of primitive that can be used to set known control
   registers, e.g. a drop() primitive that would set a drop metadata bit flag */
class CRSet : public ActionPrimitive<> {
  void operator ()() {
    get_field("test1.f16").set(666);
  }
};

REGISTER_PRIMITIVE(CRSet);

// Google Test fixture for actions tests
class ActionsTest : public ::testing::Test {
protected:

  PHV phv;

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};

  ActionFn testActionFn;
  ActionFnEntry testActionFnEntry;

  ActionsTest()
    : phv(2),
      testHeaderType("test_t", 0),
      testActionFn("test_action", 0),
      testActionFnEntry(&testActionFn) {
    testHeaderType.push_back_field("f32", 32);
    testHeaderType.push_back_field("f48", 48);
    testHeaderType.push_back_field("f8", 8);
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f128", 128);
    phv.push_back_header("test1", testHeader1, testHeaderType);
    phv.push_back_header("test2", testHeader2, testHeaderType);
  }

  virtual void SetUp() {
  }

  // virtual void TearDown() {}
};

TEST_F(ActionsTest, SetFromConst) {
  Data value(0xaba);
  SetField primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_field(testHeader1, 3); // f16
  testActionFn.parameter_push_back_const(value);

  Field &f = phv.get_field(testHeader1, 3); // f16
  f.set(0);

  ASSERT_EQ((unsigned) 0, f.get_uint());

  testActionFnEntry(phv);

  ASSERT_EQ((unsigned) 0xaba, f.get_uint());
}

TEST_F(ActionsTest, SetFromActionData) {
  Data value(0xaba);
  SetField primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_field(testHeader1, 3); // f16
  testActionFn.parameter_push_back_action_data(0);
  testActionFnEntry.push_back_action_data(value);

  Field &f = phv.get_field(testHeader1, 3); // f16
  f.set(0);

  ASSERT_EQ((unsigned) 0, f.get_uint());

  testActionFnEntry(phv);

  ASSERT_EQ((unsigned) 0xaba, f.get_uint());
}


TEST_F(ActionsTest, SetFromField) {
  SetField primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_field(testHeader1, 3); // f16
  testActionFn.parameter_push_back_field(testHeader1, 0); // f32

  Field &src = phv.get_field(testHeader1, 0); // 32
  src.set(0xaba);

  Field &dst = phv.get_field(testHeader1, 3); // f16
  dst.set(0);

  ASSERT_EQ((unsigned) 0, dst.get_uint());

  testActionFnEntry(phv);

  ASSERT_EQ((unsigned) 0xaba, dst.get_uint());
}

TEST_F(ActionsTest, SetFromConstStress) {
  Data value(0xaba);
  SetField primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_field(testHeader1, 3); // f16
  testActionFn.parameter_push_back_const(value);

  Field &f = phv.get_field(testHeader1, 3); // f16

  for(int i = 0; i < 100000; i++) {
    f.set(0);
    ASSERT_EQ((unsigned) 0, f.get_uint());
    testActionFnEntry(phv);
    ASSERT_EQ((unsigned) 0xaba, f.get_uint());
  }
}

TEST_F(ActionsTest, CopyHeader) {
  Header &hdr1 = phv.get_header(testHeader1);
  ASSERT_FALSE(hdr1.is_valid());

  Header &hdr2 = phv.get_header(testHeader2);
  ASSERT_FALSE(hdr2.is_valid());

  for(unsigned int i = 0; i < hdr1.size(); i++) {
    hdr1[i].set(0);
  }

  for(unsigned int i = 0; i < hdr2.size(); i++) {
    hdr2[i].set(i + 1);
  }

  CopyHeader primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_header(testHeader1);
  testActionFn.parameter_push_back_header(testHeader2);
  
  testActionFnEntry(phv);
  ASSERT_FALSE(hdr1.is_valid());
  ASSERT_FALSE(hdr2.is_valid());
  for(unsigned int i = 0; i < hdr2.size(); i++) {
    ASSERT_EQ(0, hdr1[i]);
  }

  hdr2.mark_valid();
  testActionFnEntry(phv);
  ASSERT_TRUE(hdr1.is_valid());
  for(unsigned int i = 0; i < hdr1.size(); i++) {
    ASSERT_EQ(i + 1, hdr1[i]);
  }
}


TEST_F(ActionsTest, CRSet) {
  CRSet primitive;
  testActionFn.push_back_primitive(&primitive);

  Field &f = phv.get_field(testHeader1, 3); // f16
  f.set(0);

  ASSERT_EQ((unsigned) 0, f.get_uint());

  phv.get_field("test1.f16").set(666);
  // testActionFnEntry(phv);

  ASSERT_EQ((unsigned) 666, f.get_uint());
}
