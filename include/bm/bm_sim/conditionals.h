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

#ifndef BM_BM_SIM_CONDITIONALS_H_
#define BM_BM_SIM_CONDITIONALS_H_

#include <string>

#include "phv.h"
#include "control_flow.h"
#include "expressions.h"

namespace bm {

class Conditional
  : public ControlFlowNode, public Expression {
 public:
  Conditional(const std::string &name, p4object_id_t id)
    : ControlFlowNode(name, id) {}

  bool eval(const PHV &phv) const {
    return eval_bool(phv);
  }

  void set_next_node_if_true(ControlFlowNode *next_node) {
    true_next = next_node;
  }

  void set_next_node_if_false(ControlFlowNode *next_node) {
    false_next = next_node;
  }

  // return pointer to next control flow node
  const ControlFlowNode *operator()(Packet *pkt) const override;

  Conditional(const Conditional &other) = delete;
  Conditional &operator=(const Conditional &other) = delete;

  Conditional(Conditional &&other) /*noexcept*/ = default;
  Conditional &operator=(Conditional &&other) /*noexcept*/ = default;

 private:
  ControlFlowNode *true_next{nullptr};
  ControlFlowNode *false_next{nullptr};
};

}  // namespace bm

#endif  // BM_BM_SIM_CONDITIONALS_H_
