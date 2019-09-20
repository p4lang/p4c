/* Copyright 2019-RT-RK Computer Based System
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

#include <gtest/gtest.h>

#include <bm/bm_sim/actions.h>
#include <bm/bm_sim/core/primitives.h>
#include <bm/bm_sim/logger.h>
#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/packet.h>

using namespace bm;

class AssertAssumeTest : public ::testing::TestWithParam<const char *> {
 protected:
  PHVFactory phv_factory;

  ActionFn testActionFn;
  ActionFnEntry testActionFnEntry;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};
  std::unique_ptr<Packet> pkt{nullptr};

  AssertAssumeTest()
      : testActionFn("test_primitive", 0, 1),
        testActionFnEntry(&testActionFn),
        phv_source(PHVSourceIface::make_phv_source()) { }

  void SetUp() override {
    phv_source->set_phv_factory(0, &phv_factory);
    pkt = std::unique_ptr<Packet>(new Packet(
        Packet::make_new(phv_source.get())));
  }

  ArithExpression *build_expression(bool value) {
    ArithExpression* condition = new ArithExpression();
    condition->push_back_load_bool(value);
    condition->push_back_op(ExprOpcode::BOOL_TO_DATA);
    condition->build();
    return condition;
  }
};

TEST_P(AssertAssumeTest, ConditionTrue) {
  auto primitive = ActionOpcodesMap::get_instance()->get_primitive(GetParam());
  ASSERT_NE(nullptr, primitive);

  testActionFn.push_back_primitive(primitive.get());
  auto expr = build_expression(true);
  std::unique_ptr<ArithExpression> condition(expr);
  testActionFn.parameter_push_back_expression(std::move(condition));

  testActionFnEntry(pkt.get());
}

using AssertAssumeDeathTest = AssertAssumeTest;

extern bool WITH_VALGRIND;  // defined in main.cpp

TEST_P(AssertAssumeDeathTest, ConditionFalse) {
  // TODO(antonin): use GTEST_SKIP once we update the version of googletest used
  // by bmv2.
  if (WITH_VALGRIND) {
    SUCCEED();
    return;
  }

  auto primitive = ActionOpcodesMap::get_instance()->get_primitive(GetParam());
  ASSERT_NE(nullptr, primitive);

  testActionFn.push_back_primitive(primitive.get());
  auto expr = build_expression(false);
  std::unique_ptr<ArithExpression> condition(expr);
  testActionFn.parameter_push_back_expression(std::move(condition));

  EXPECT_DEATH(testActionFnEntry(pkt.get()), "");
}

INSTANTIATE_TEST_CASE_P(AssertAssumeTest,
                        AssertAssumeTest,
                        ::testing::Values("assert", "assume"));
INSTANTIATE_TEST_CASE_P(AssertAssumeDeathTest,
                        AssertAssumeDeathTest,
                        ::testing::Values("assert", "assume"));
