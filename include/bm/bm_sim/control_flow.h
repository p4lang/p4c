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

#ifndef BM_BM_SIM_CONTROL_FLOW_H_
#define BM_BM_SIM_CONTROL_FLOW_H_

#include <string>

#include "packet.h"
#include "named_p4object.h"

namespace bm {

class ControlFlowNode : public NamedP4Object {
 public:
  ControlFlowNode(const std::string &name, p4object_id_t id)
      : NamedP4Object(name, id) { }
  virtual ~ControlFlowNode() { }
  virtual const ControlFlowNode *operator()(Packet *pkt) const = 0;
};

}  // namespace bm

#endif  // BM_BM_SIM_CONTROL_FLOW_H_
