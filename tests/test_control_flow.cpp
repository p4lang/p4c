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

#include <bm/bm_sim/actions.h>
#include <bm/bm_sim/control_action.h>
#include <bm/bm_sim/packet.h>

using namespace bm;

namespace {

class count : public ActionPrimitive<> {
 public:
  size_t get() { return c; }

 private:
  void operator ()() override { c++; }
  size_t c{0};
};

struct DummyNode: public ControlFlowNode {
  DummyNode()
      : ControlFlowNode("", 0) { }

  const ControlFlowNode *operator()(Packet *) const { return nullptr; }
};

}  // namespace

class ControlActionTest : public ::testing::Test {
 protected:
  count count_primitive;
  ActionFn action_fn;
  ControlAction action_call;
  DummyNode dummy_next_node;

  PHVFactory phv_factory;
  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  ControlActionTest()
      : action_fn("action", 0, 0  /* num_params */),
        action_call("action_call", 0),
        phv_source(PHVSourceIface::make_phv_source()) { }

  Packet get_pkt() {
    // dummy packet, won't be parsed
    return Packet::make_new(64, PacketBuffer(128), phv_source.get());
  }

  virtual void SetUp() {
    action_fn.push_back_primitive(&count_primitive);

    action_call.set_next_node(&dummy_next_node);
    action_call.set_action(&action_fn);

    phv_source->set_phv_factory(0, &phv_factory);
  }
};

TEST_F(ControlActionTest, Call) {
  auto pkt = get_pkt();
  auto next_node = action_call(&pkt);
  EXPECT_EQ(&dummy_next_node, next_node);
  EXPECT_EQ(1u, count_primitive.get());
}
