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

#include "bm_sim/pipeline.h"
#include "bm_sim/event_logger.h"
#include "bm_sim/logger.h"

void
Pipeline::apply(Packet *pkt) {
  ELOGGER->pipeline_start(*pkt, *this);
  BMLOG_DEBUG_PKT(*pkt, "Pipeline '{}': start", get_name());
  const ControlFlowNode *node = first_node;
  while(node) {
    node = (*node)(pkt); 
  }
  ELOGGER->pipeline_done(*pkt, *this);
  BMLOG_DEBUG_PKT(*pkt, "Pipeline '{}': end", get_name());
}
