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

#include <bm/bm_sim/checksums.h>
#include <bm/bm_sim/logger.h>

#include <cstdint>
#include <string>

namespace bm {

namespace {

// TODO(antonin): remove this ? it is duplicated in calculations.cpp
uint16_t cksum16(char *buf, size_t len) {
  uint64_t sum = 0;
  uint64_t *b = reinterpret_cast<uint64_t *>(buf);
  uint32_t t1, t2;
  uint16_t t3, t4;
  uint8_t *tail;
  /* Main loop - 8 bytes at a time */
  while (len >= sizeof(uint64_t)) {
    uint64_t s = *b++;
    sum += s;
    if (sum < s) sum++;
    len -= 8;
  }
  /* Handle tail less than 8-bytes long */
  tail = reinterpret_cast<uint8_t *>(b);
  if (len & 4) {
    uint32_t s = *reinterpret_cast<uint32_t *>(tail);
    sum += s;
    if (sum < s) sum++;
    tail += 4;
  }
  if (len & 2) {
    uint16_t s = *reinterpret_cast<uint16_t *>(tail);
    sum += s;
    if (sum < s) sum++;
    tail += 2;
  }
  if (len & 1) {
    uint8_t s = *reinterpret_cast<uint8_t *>(tail);
    sum += s;
    if (sum < s) sum++;
  }
  /* Fold down to 16 bits */
  t1 = sum;
  t2 = sum >> 32;
  t1 += t2;
  if (t1 < t2) t1++;
  t3 = t1;
  t4 = t1 >> 16;
  t3 += t4;
  if (t3 < t4) t3++;
  return ~t3;
}

}  // namespace

Checksum::Checksum(const std::string &name, p4object_id_t id,
                   header_id_t header_id, int field_offset)
  : NamedP4Object(name, id),
    header_id(header_id), field_offset(field_offset) { }

void
Checksum::update(Packet *pkt) const {
  // is it a good idea to put this in the implementation; should the deparser be
  // aware that nothing was updated?
  if (!is_checksum_condition_met(*pkt)) {
    BMLOG_TRACE_PKT(
        *pkt, "Skipping checksum '{}' update because condition not met",
        get_name());
  } else {
    BMLOG_DEBUG_PKT(*pkt, "Updating checksum '{}'", get_name());
    update_(pkt);
  }
}

bool
Checksum::verify(const Packet &pkt) const {
  if (!is_checksum_condition_met(pkt)) {
    BMLOG_TRACE_PKT(
        pkt, "Skipping checksum '{}' verification because condition not met",
        get_name());
    return true;
  } else {
    bool valid = verify_(pkt);
    BMLOG_DEBUG_PKT(pkt, "Verifying checksum '{}': {}", get_name(), valid);
    return valid;
  }
}

void
Checksum::set_checksum_condition(std::unique_ptr<Expression> cksum_condition) {
  condition = std::move(cksum_condition);
}

bool
Checksum::is_checksum_condition_met(const Packet &pkt) const {
  return (!condition) || (condition->eval_bool(*pkt.get_phv()));
}

CalcBasedChecksum::CalcBasedChecksum(const std::string &name, p4object_id_t id,
                                     header_id_t header_id, int field_offset,
                                     const NamedCalculation *calculation)
  : Checksum(name, id, header_id, field_offset),
    calculation(calculation) { }

void
CalcBasedChecksum::update_(Packet *pkt) const {
  const uint64_t cksum = calculation->output(*pkt);
  Field &f_cksum = pkt->get_phv()->get_field(header_id, field_offset);
  f_cksum.set(cksum);
}

bool
CalcBasedChecksum::verify_(const Packet &pkt) const {
  const uint64_t cksum = calculation->output(pkt);
  const Field &f_cksum = pkt.get_phv()->get_field(header_id, field_offset);
  return (cksum == f_cksum.get<uint64_t>());
}

IPv4Checksum::IPv4Checksum(const std::string &name, p4object_id_t id,
                           header_id_t header_id, int field_offset)
  : Checksum(name, id, header_id, field_offset) { }

#define IPV4_HDR_MAX_LEN 60
#define IPV4_CKSUM_OFFSET 10  // byte offset

void IPv4Checksum::update_(Packet *pkt) const {
  char buffer[60];
  PHV *phv = pkt->get_phv();
  Header &ipv4_hdr = phv->get_header(header_id);
  if (!ipv4_hdr.is_valid()) return;
  Field &ipv4_cksum = ipv4_hdr[field_offset];
  ipv4_hdr.deparse(buffer);
  buffer[IPV4_CKSUM_OFFSET] = 0; buffer[IPV4_CKSUM_OFFSET + 1] = 0;
  uint16_t cksum = cksum16(buffer, ipv4_hdr.get_nbytes_packet());
  // cksum is in network byte order
  ipv4_cksum.set_bytes(reinterpret_cast<char *>(&cksum), 2);
}

bool
IPv4Checksum::verify_(const Packet &pkt) const {
  char buffer[60];
  const PHV &phv = *(pkt.get_phv());
  const Header &ipv4_hdr = phv.get_header(header_id);
  if (!ipv4_hdr.is_valid()) return true;  // return true if no header... TODO ?
  const Field &ipv4_cksum = ipv4_hdr[field_offset];
  ipv4_hdr.deparse(buffer);
  buffer[IPV4_CKSUM_OFFSET] = 0; buffer[IPV4_CKSUM_OFFSET + 1] = 0;
  uint16_t cksum = cksum16(buffer, ipv4_hdr.get_nbytes_packet());
  // TODO(antonin): improve this?
  return !memcmp(reinterpret_cast<char *>(&cksum),
                 ipv4_cksum.get_bytes().data(), 2);
}

#undef IPV4_HDR_MAX_LEN
#undef IPV4_CKSUM_OFFSET

}  // namespace bm
