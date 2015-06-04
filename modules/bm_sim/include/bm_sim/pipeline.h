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

#ifndef _BM_PIPELINE_H_
#define _BM_PIPELINE_H_

#include "control_flow.h"
#include "named_p4object.h"

class Pipeline : public NamedP4Object
{
public:
  Pipeline(const std::string &name, p4object_id_t id,
	   ControlFlowNode *first_node)
    : NamedP4Object(name, id), first_node(first_node) {}

  void apply(Packet *pkt);

  Pipeline(const Pipeline &other) = delete;
  Pipeline &operator=(const Pipeline &other) = delete;

  Pipeline(Pipeline &&other) /*noexcept*/ = default;
  Pipeline &operator=(Pipeline &&other) /*noexcept*/ = default;

private:
  ControlFlowNode *first_node;
};

#endif
