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

#include <bm/bm_sim/packet.h>

#include <algorithm>  // for swap
#include <atomic>

#include "xxhash.h"

namespace bm {

typedef Debugger::PacketId PacketId;

constexpr size_t Packet::nb_registers;

copy_id_t
CopyIdGenerator::add_one(packet_id_t packet_id) {
  int idx = static_cast<int>(packet_id % W);
  Cntr old_cntr = arr[idx].load();
  Cntr new_cntr;
  do {
    new_cntr.num = old_cntr.num + 1;
    new_cntr.max = old_cntr.max + 1;
  } while (!arr[idx].compare_exchange_weak(old_cntr, new_cntr));
  return new_cntr.max;
}

void
CopyIdGenerator::remove_one(packet_id_t packet_id) {
  int idx = static_cast<int>(packet_id % W);
  Cntr old_cntr = arr[idx].load();
  Cntr new_cntr;
  do {
    if (old_cntr.num == 0) {
      new_cntr.max = 0;
    } else {
      new_cntr.max = old_cntr.max;
      new_cntr.num = old_cntr.num - 1;
    }
  } while (!arr[idx].compare_exchange_weak(old_cntr, new_cntr));
}

copy_id_t
CopyIdGenerator::get(packet_id_t packet_id) {
  int idx = static_cast<int>(packet_id % W);
  return arr[idx].load().max;
}

void
CopyIdGenerator::reset(packet_id_t packet_id) {
  int idx = static_cast<int>(packet_id % W);
  Cntr old_cntr = arr[idx].load();
  Cntr new_cntr;
  do {
    new_cntr.num = 0;
    new_cntr.max = 0;
  } while (!arr[idx].compare_exchange_weak(old_cntr, new_cntr));
}

void
Packet::update_signature(uint64_t seed) {
  signature = XXH64(buffer.start(), buffer.get_data_size(), seed);
}

void
Packet::set_ingress_ts() {
  ingress_ts = clock::now();
  ingress_ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    ingress_ts.time_since_epoch()).count();
}

Packet::Packet(size_t cxt_id, int ingress_port, packet_id_t id,
               copy_id_t copy_id, int ingress_length, PacketBuffer &&buffer,
               PHVSourceIface *phv_source)
    : cxt_id(cxt_id), ingress_port(ingress_port), packet_id(id),
      copy_id(copy_id), ingress_length(ingress_length),
      buffer(std::move(buffer)), phv_source(phv_source) {
  assert(phv_source);
  update_signature();
  set_ingress_ts();
  phv = phv_source->get(cxt_id);
  phv->set_packet_id(packet_id, copy_id);
  DEBUGGER_PACKET_IN(PacketId::make(packet_id, copy_id), ingress_port);
}

Packet::~Packet() {
  assert(phv_source);
  // Compiling and running the tests with g++5 exposed this issue
  // When the move constructor is called from a temporary, the PHV is stolen
  // from the temporary and then the temporary is destroyed
  // I don't know why this was not observed with g++-4.8 / g++-4.9
  // assert(phv);
  if (phv) {
    copy_id_gen->remove_one(packet_id);
    phv->reset();
    phv->reset_header_stacks();
    phv_source->release(cxt_id, std::move(phv));
    DEBUGGER_PACKET_OUT(PacketId::make(packet_id, copy_id), egress_port);
  }
}

void
Packet::change_context(size_t new_cxt) {
  if (cxt_id == new_cxt) return;
  assert(phv);
  phv->reset();
  phv->reset_header_stacks();
  phv_source->release(cxt_id, std::move(phv));
  phv = phv_source->get(new_cxt);
  cxt_id = new_cxt;
}

/* It is important to understand that with NRVO, the following are "equivalent"
   and both generate only a call to the constructor:

   A make() {
     A a;
     return a;
   }

   std::unique_ptr<A> c1() {
     return std::unique_ptr<A>(new A(build()));
   }

   std::unique_ptr<A> c2() {
     std::unique_ptr<A> a(new A());
     return a;
   }
*/

Packet
Packet::clone_with_phv() const {
  copy_id_t new_copy_id = copy_id_gen->add_one(packet_id);
  Packet pkt(cxt_id, ingress_port, packet_id, new_copy_id, ingress_length,
             buffer.clone(buffer.get_data_size()), phv_source);
  pkt.phv->copy_headers(*phv);
  // return std::move(pkt);
  // Enable NRVO
  return pkt;
}

std::unique_ptr<Packet>
Packet::clone_with_phv_ptr() const {
  return std::unique_ptr<Packet>(new Packet(clone_with_phv()));
}

Packet
Packet::clone_with_phv_reset_metadata() const {
  copy_id_t new_copy_id = copy_id_gen->add_one(packet_id);
  Packet pkt(cxt_id, ingress_port, packet_id, new_copy_id, ingress_length,
             buffer.clone(buffer.get_data_size()), phv_source);
  // TODO(antonin): optimize this
  pkt.phv->copy_headers(*phv);
  pkt.phv->reset_metadata();
  // return std::move(pkt);
  // Enable NRVO
  return pkt;
}

std::unique_ptr<Packet>
Packet::clone_with_phv_reset_metadata_ptr() const {
  return std::unique_ptr<Packet>(new Packet(clone_with_phv_reset_metadata()));
}

Packet
Packet::clone_choose_context(size_t new_cxt) const {
  copy_id_t new_copy_id = copy_id_gen->add_one(packet_id);
  Packet pkt(new_cxt, ingress_port, packet_id, new_copy_id, ingress_length,
             buffer.clone(buffer.get_data_size()), phv_source);
  // return std::move(pkt);
  // Enable NRVO
  return pkt;
}

std::unique_ptr<Packet>
Packet::clone_choose_context_ptr(size_t new_cxt) const {
  return std::unique_ptr<Packet>(new Packet(clone_choose_context(new_cxt)));
}

Packet
Packet::clone_no_phv() const {
  return clone_choose_context(cxt_id);
}

std::unique_ptr<Packet>
Packet::clone_no_phv_ptr() const {
  return clone_choose_context_ptr(cxt_id);
}

/* Cannot get away with defaults here, we need to swap the phvs, otherwise we
   could "leak" the old phv (i.e. not put it back into the pool) */

Packet::Packet(Packet &&other) noexcept
    : cxt_id(other.cxt_id), ingress_port(other.ingress_port),
      egress_port(other.egress_port), packet_id(other.packet_id),
      copy_id(other.copy_id), ingress_length(other.ingress_length),
      signature(other.signature), payload_size(other.payload_size),
      ingress_ts(other.ingress_ts), ingress_ts_ms(other.ingress_ts_ms),
  phv_source(other.phv_source), registers(other.registers) {
  buffer = std::move(other.buffer);
  phv = std::move(other.phv);
}

Packet &
Packet::operator=(Packet &&other) noexcept {
  cxt_id = other.cxt_id;
  ingress_port = other.ingress_port;
  egress_port = other.egress_port;
  packet_id = other.packet_id;
  copy_id = other.copy_id;
  ingress_length = other.ingress_length;
  signature = other.signature;
  payload_size = other.payload_size;
  ingress_ts = other.ingress_ts;
  ingress_ts_ms = other.ingress_ts_ms;
  phv_source = other.phv_source;
  registers = other.registers;

  std::swap(buffer, other.buffer);
  std::swap(phv, other.phv);

  return *this;
}

Packet
Packet::make_new(PHVSourceIface *phv_source) {
  return Packet(0, 0, 0, 0, 0, PacketBuffer(), phv_source);
}

Packet
Packet::make_new(int ingress_length, PacketBuffer &&buffer,
                 PHVSourceIface *phv_source) {
  return Packet(0, 0, 0, 0, ingress_length, std::move(buffer), phv_source);
}

Packet
Packet::make_new(size_t cxt, int ingress_port, packet_id_t id,
                 copy_id_t copy_id, int ingress_length,
                 PacketBuffer &&buffer, PHVSourceIface *phv_source) {
  return Packet(cxt, ingress_port, id, copy_id, ingress_length,
                std::move(buffer), phv_source);
}

CopyIdGenerator *Packet::copy_id_gen = new CopyIdGenerator();

}  // namespace bm
