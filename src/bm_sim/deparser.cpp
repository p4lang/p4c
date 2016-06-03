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

#include <bm/bm_sim/deparser.h>
#include <bm/bm_sim/debugger.h>
#include <bm/bm_sim/logger.h>

namespace bm {

size_t
Deparser::get_headers_size(const PHV &phv) const {
  size_t headers_size = 0;
  for (auto it = headers.begin(); it != headers.end(); ++it) {
    const Header &header = phv.get_header(*it);
    if (header.is_valid()) {
      headers_size += header.get_nbytes_packet();
    }
  }
  return headers_size;
}

void
Deparser::deparse(Packet *pkt) const {
  PHV *phv = pkt->get_phv();
  BMELOG(deparser_start, *pkt, *this);
  // TODO(antonin)
  // this is temporary while we experiment with the debugger
  DEBUGGER_NOTIFY_CTR(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      DBG_CTR_DEPARSER | get_id());
  BMLOG_DEBUG_PKT(*pkt, "Deparser '{}': start", get_name());
  update_checksums(pkt);
  char *data = pkt->prepend(get_headers_size(*phv));
  int bytes_parsed = 0;
  // invalidating headers, and resetting header stacks is done in the Packet
  // destructor, when the PHV is released
  for (auto it = headers.begin(); it != headers.end(); ++it) {
    Header &header = phv->get_header(*it);
    if (header.is_valid()) {
      BMELOG(deparser_emit, *pkt, *it);
      BMLOG_DEBUG_PKT(*pkt, "Deparsing header '{}'", header.get_name());
      header.deparse(data + bytes_parsed);
      bytes_parsed += header.get_nbytes_packet();
      // header.mark_invalid();
    }
  }
  // phv->reset_header_stacks();
  BMELOG(deparser_done, *pkt, *this);
  DEBUGGER_NOTIFY_CTR(
      Debugger::PacketId::make(pkt->get_packet_id(), pkt->get_copy_id()),
      DBG_CTR_EXIT(DBG_CTR_DEPARSER) | get_id());
  BMLOG_DEBUG_PKT(*pkt, "Deparser '{}': end", get_name());
}

void
Deparser::update_checksums(Packet *pkt) const {
  for (const Checksum *checksum : checksums) {
    checksum->update(pkt);
    BMELOG(checksum_update, *pkt, *checksum);
  }
}

}  // namespace bm
