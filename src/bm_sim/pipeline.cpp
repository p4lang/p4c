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

#include <bm/bm_sim/pipeline.h>
#include <bm/bm_sim/event_logger.h>
#include <bm/bm_sim/logger.h>
#include <bm/bm_sim/debugger.h>

namespace bm {

void
Pipeline::apply(Packet *pkt) {
  BMELOG(pipeline_start, *pkt, *this);
  // TODO(antonin)
  // this is temporary while we experiment with the debugger
  DEBUGGER_NOTIFY_CTR(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      DBG_CTR_CONTROL | get_id());
  BMLOG_DEBUG_PKT(*pkt, "Pipeline '{}': start", get_name());
  const ControlFlowNode *node = first_node;
  while (node) {
    if (pkt->is_marked_for_exit()) {
      BMLOG_DEBUG_PKT(*pkt, "Packet is marked for exit, interrupting pipeline");
      break;
    }
    node = (*node)(pkt);
  }
  BMELOG(pipeline_done, *pkt, *this);
  DEBUGGER_NOTIFY_CTR(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      DBG_CTR_EXIT(DBG_CTR_CONTROL) | get_id());
  BMLOG_DEBUG_PKT(*pkt, "Pipeline '{}': end", get_name());
}

}  // namespace bm
