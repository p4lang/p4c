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

#include <bm/bm_sim/conditionals.h>
#include <bm/bm_sim/event_logger.h>

#include <cassert>

namespace bm {

const ControlFlowNode *
Conditional::operator()(Packet *pkt) const {
  // TODO(antonin)
  // this is temporary while we experiment with the debugger
  DEBUGGER_NOTIFY_CTR(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      DBG_CTR_CONDITION | get_id());
  PHV *phv = pkt->get_phv();
  bool result = eval(*phv);
  BMELOG(condition_eval, *pkt, *this, result);
  DEBUGGER_NOTIFY_UPDATE_V(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      Debugger::FIELD_COND, result);
  DEBUGGER_NOTIFY_CTR(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      DBG_CTR_EXIT(DBG_CTR_CONDITION) | get_id());
  return result ? true_next : false_next;
}

}  // namespace bm
