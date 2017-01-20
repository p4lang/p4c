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

#include <bm/bm_sim/tables.h>
#include <bm/bm_sim/debugger.h>
#include <bm/bm_sim/logger.h>

#include <string>

namespace bm {

MatchActionTable::MatchActionTable(
    const std::string &name, p4object_id_t id,
    std::unique_ptr<MatchTableAbstract> match_table)
    : ControlFlowNode(name, id),
      match_table(std::move(match_table)) { }

const ControlFlowNode *
MatchActionTable::operator()(Packet *pkt) const {
  // TODO(antonin)
  // this is temporary while we experiment with the debugger
  DEBUGGER_NOTIFY_CTR(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      DBG_CTR_TABLE | get_id());
  BMLOG_TRACE_PKT(*pkt, "Applying table '{}'", get_name());
  const auto next = match_table->apply_action(pkt);
  DEBUGGER_NOTIFY_CTR(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      DBG_CTR_EXIT(DBG_CTR_TABLE) | get_id());
  return next;
}

}  // namespace bm
