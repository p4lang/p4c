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

#include "bm_sim/packet.h"

#include "xxhash.h"

void
Packet::update_signature(unsigned long long seed) {
  signature = XXH64(buffer.start(), buffer.get_data_size(), seed);
}

void
Packet::set_ingress_ts() {
  ingress_ts = clock::now();
  ingress_ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    ingress_ts.time_since_epoch()
  ).count();
}

Packet::Packet() {
  assert(phv_pool);
  phv = phv_pool->get(); // needed ?
}

Packet::Packet(int ingress_port, packet_id_t id, packet_id_t copy_id,
	       int ingress_length, PacketBuffer &&buffer)
  : ingress_port(ingress_port), packet_id(id), copy_id(copy_id),
    ingress_length(ingress_length), buffer(std::move(buffer)) {
  assert(phv_pool);
  update_signature();
  set_ingress_ts();
  phv = phv_pool->get();
}

Packet::Packet(int ingress_port, packet_id_t id, packet_id_t copy_id,
	       int ingress_length, PacketBuffer &&buffer, const PHV &src_phv)
  : ingress_port(ingress_port), packet_id(id), copy_id(copy_id),
    ingress_length(ingress_length), buffer(std::move(buffer)) {
  assert(phv_pool);
  update_signature();
  set_ingress_ts();
  phv = phv_pool->get();
  phv->copy_headers(src_phv);
}
  
Packet::~Packet() {
  assert(phv);
  // corner case: phv_pool has already been destroyed
  if(phv_pool) {
    phv->reset();
    phv->reset_header_stacks();
    phv_pool->release(std::move(phv));
  }
}

Packet
Packet::clone(packet_id_t new_copy_id) const {
  Packet pkt(ingress_port, packet_id, new_copy_id, ingress_length,
	     buffer.clone(buffer.get_data_size()));
  pkt.phv->copy_headers(*phv);
  return pkt;
}

Packet
Packet::clone_and_reset_metadata(packet_id_t new_copy_id) const {
  Packet pkt(ingress_port, packet_id, new_copy_id, ingress_length,
	     buffer.clone(buffer.get_data_size()));
  // TODO: optimize this
  pkt.phv->copy_headers(*phv);
  pkt.phv->reset_metadata();
  return pkt;
}

Packet
Packet::clone_no_phv(packet_id_t new_copy_id) const {
  Packet pkt(ingress_port, packet_id, new_copy_id, ingress_length,
	     buffer.clone(buffer.get_data_size()));
  return pkt;
}

/* Cannot get away with defaults here, we need to swap the phvs, otherwise we
   could "leak" the old phv (i.e. not put it back into the pool) */

Packet::Packet(Packet &&other) noexcept
  : ingress_port(other.ingress_port), egress_port(other.egress_port),
    packet_id(other.packet_id), copy_id(other.copy_id),
    ingress_length(other.ingress_length),
    signature(other.signature), payload_size(other.payload_size) {
  buffer = std::move(buffer);
  std::swap(phv, other.phv);
}

Packet &
Packet::operator=(Packet &&other) noexcept {
  ingress_port = other.ingress_port;
  egress_port = other.egress_port;
  packet_id = other.packet_id;
  copy_id = other.copy_id;
  ingress_length = other.ingress_length;
  signature = other.signature;
  payload_size = other.payload_size;

  std::swap(buffer, other.buffer);
  std::swap(phv, other.phv);

  return *this;
}

Packet::PHVPool *Packet::phv_pool = nullptr;

void
Packet::set_phv_factory(const PHVFactory &phv_factory) {
  assert(!phv_pool);
  phv_pool = new PHVPool(phv_factory);
}

void
Packet::unset_phv_factory() {
  assert(phv_pool);
  delete phv_pool;
  phv_pool = nullptr;
}

void
Packet::swap_phv_factory(const PHVFactory &phv_factory) {
  assert(phv_pool);
  delete phv_pool;
  phv_pool = new PHVPool(phv_factory);
}

Packet::PHVPool::PHVPool(const PHVFactory &phv_factory)
  : phv_factory(phv_factory) {
  phvs.push_back(std::move(phv_factory.create()));
}

std::unique_ptr<PHV>
Packet::PHVPool::get() {
  std::unique_lock<std::mutex> lock(mutex);
  if(phvs.size() == 0) {
    lock.unlock();
    return phv_factory.create();
  }
  std::unique_ptr<PHV> phv = std::move(phvs.back());
  phvs.pop_back();
  return phv;
}

void
Packet::PHVPool::release(std::unique_ptr<PHV> phv) {
  std::unique_lock<std::mutex> lock(mutex);
  phvs.push_back(std::move(phv));
}
