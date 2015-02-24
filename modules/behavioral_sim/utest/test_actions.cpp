#include <gtest/gtest.h>

#include <memory>

#include "actions.h"

int pull_test_actions() { return 0; }

using std::unique_ptr;

// Two primitives definitions

class SetField : public ActionPrimitive<Field &, Data &> {
  void operator ()(Field &f, Data &d) {
    f.set(d);
  }
};

class Add : public ActionPrimitive<Field &, Data &, Data &> {
  void operator ()(Field &f, Data &d1, Data &d2) {
    f.add(d1, d2);
  }
};

// Google Test fixture for actions tests
class ActionsTest : public ::testing::Test {
protected:

  PHV phv;

  HeaderType testHeaderType;
  header_id_t testHeader;

  ActionFn testActionFn;
  ActionFnEntry testActionFnEntry;

  ActionsTest()
    : testHeaderType(0, "test_t"),
      testActionFnEntry(&testActionFn) {
    testHeaderType.push_back_field("f32", 32);
    testHeaderType.push_back_field("f48", 48);
    testHeaderType.push_back_field("f8", 8);
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f128", 128);
    testHeader = phv.push_back_header("test", testHeaderType);
  }

  virtual void SetUp() {
  }

  // virtual void TearDown() {}
};

TEST_F(ActionsTest, SetFromConst) {
  Data value(0xaba);
  auto primitive = unique_ptr<ActionPrimitive_>(new SetField());
  testActionFn.push_back_primitive(std::move(primitive));
  testActionFn.parameter_push_back_field(testHeader, 3); // f16
  testActionFn.parameter_push_back_const(value);

  Field &f = phv.get_field(testHeader, 3); // f16
  f.set(0);

  ASSERT_EQ((unsigned) 0, f.get_ui());

  testActionFnEntry(phv);

  ASSERT_EQ((unsigned) 0xaba, f.get_ui());
}

TEST_F(ActionsTest, SetFromActionData) {
  Data value(0xaba);
  auto primitive = unique_ptr<ActionPrimitive_>(new SetField());
  testActionFn.push_back_primitive(std::move(primitive));
  testActionFn.parameter_push_back_field(testHeader, 3); // f16
  testActionFn.parameter_push_back_action_data(0);
  testActionFnEntry.push_back_action_data(value);

  Field &f = phv.get_field(testHeader, 3); // f16
  f.set(0);

  ASSERT_EQ((unsigned) 0, f.get_ui());

  testActionFnEntry(phv);

  ASSERT_EQ((unsigned) 0xaba, f.get_ui());
}


TEST_F(ActionsTest, SetFromField) {
  auto primitive = unique_ptr<ActionPrimitive_>(new SetField());
  testActionFn.push_back_primitive(std::move(primitive));
  testActionFn.parameter_push_back_field(testHeader, 3); // f16
  testActionFn.parameter_push_back_field(testHeader, 0); // f32

  Field &src = phv.get_field(testHeader, 0); // 32
  src.set(0xaba);

  Field &dst = phv.get_field(testHeader, 3); // f16
  dst.set(0);

  ASSERT_EQ((unsigned) 0, dst.get_ui());

  testActionFnEntry(phv);

  ASSERT_EQ((unsigned) 0xaba, dst.get_ui());
}

TEST_F(ActionsTest, SetFromConstStress) {
  Data value(0xaba);
  auto primitive = unique_ptr<ActionPrimitive_>(new SetField());
  testActionFn.push_back_primitive(std::move(primitive));
  testActionFn.parameter_push_back_field(testHeader, 3); // f16
  testActionFn.parameter_push_back_const(value);

  Field &f = phv.get_field(testHeader, 3); // f16

  for(int i = 0; i < 100000; i++) {
    f.set(0);
    ASSERT_EQ((unsigned) 0, f.get_ui());
    testActionFnEntry(phv);
    ASSERT_EQ((unsigned) 0xaba, f.get_ui());
  }
}
