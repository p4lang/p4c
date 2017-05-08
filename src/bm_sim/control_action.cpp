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

#include <bm/bm_sim/actions.h>
#include <bm/bm_sim/control_action.h>
#include <bm/bm_sim/packet.h>

#include <string>

namespace bm {

ControlAction::ControlAction(const std::string &name, p4object_id_t id)
    : ControlFlowNode(name, id) { }

ControlAction::ControlAction(const std::string &name, p4object_id_t id,
                             std::unique_ptr<SourceInfo> source_info)
    : ControlFlowNode(name, id, std::move(source_info)) { }

void
ControlAction::set_next_node(ControlFlowNode *next_node) {
  this->next_node = next_node;
}

void
ControlAction::set_action(ActionFn *action) {
  this->action = action;
}

const ControlFlowNode *
ControlAction::operator()(Packet *pkt) const {
  assert(action);
  ActionFnEntry action_entry(action);
  // TODO(unknown): log action call with source information, or ActionFnEntry
  // log is sufficient?
  action_entry(pkt);
  return next_node;
}

}  // namespace bm
