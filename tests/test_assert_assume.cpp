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

class AssertAssume_DeathTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;

  ActionFn testActionFn;
  ActionFnEntry testActionFnEntry;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};
  std::unique_ptr<Packet> pkt{nullptr};

  AssertAssume_DeathTest()
      : testActionFn("test_primitive", 0, 1),
        testActionFnEntry(&testActionFn),
        phv_source(PHVSourceIface::make_phv_source()) { }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
    pkt = std::unique_ptr<Packet>(new Packet(
        Packet::make_new(phv_source.get())));
  }

  virtual void TearDown() { }

  ArithExpression* build_expression(bool value) {
    ArithExpression* condition = new ArithExpression();
    condition->push_back_load_bool(value);
    condition->push_back_op(ExprOpcode::BOOL_TO_DATA);
    condition->build();
    return condition;
  }

  void verify_test(bool isAssert, bool withError) {
    std::unique_ptr<ActionPrimitive_> primitive;
    if (isAssert) {
      primitive = ActionOpcodesMap::get_instance()->get_primitive("assert");
    } else {
      primitive = ActionOpcodesMap::get_instance()->get_primitive("assume");
    }
    ASSERT_NE(nullptr, primitive);

    testActionFn.push_back_primitive(primitive.get());
    auto expr = build_expression(!withError);
    std::unique_ptr<ArithExpression> condition(expr);
    testActionFn.parameter_push_back_expression(std::move(condition));

    EXPECT_DEATH(testActionFnEntry(pkt.get()), "");
  }
};

TEST_F(AssertAssume_DeathTest, AssumeBoolConstError) {
  verify_test(false, true);
}

TEST_F(AssertAssume_DeathTest, AssertBoolConstError) {
  verify_test(true, true);
}
