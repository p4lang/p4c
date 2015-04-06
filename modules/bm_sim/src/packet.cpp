#include "bm_sim/packet.h"

#include "xxhash.h"

Packet::Packet() {
  phv = phv_pool->get(); // needed ?
}

Packet::Packet(int ingress_port, packet_id_t id, packet_id_t copy_id,
	       PacketBuffer &&buffer)
  : ingress_port(ingress_port), packet_id(id), copy_id(copy_id),
    buffer(std::move(buffer)) {
  assert(phv_pool);
  signature = XXH64(buffer.start(), buffer.get_data_size(), 0);
  phv = phv_pool->get();
}

Packet::Packet(int ingress_port, packet_id_t id, packet_id_t copy_id,
	       PacketBuffer &&buffer, const PHV &src_phv)
  : ingress_port(ingress_port), packet_id(id), copy_id(copy_id),
    buffer(std::move(buffer)) {
  assert(phv_pool);
  signature = XXH64(buffer.start(), buffer.get_data_size(), 0);
  phv = phv_pool->get();
  // phv->copy_headers(src_phv);
}
  
Packet::~Packet() {
  // could have been "moved"
  if(phv) phv_pool->release(std::move(phv));
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

Packet::PHVPool::PHVPool(const PHVFactory &phv_factory)
  : phv_factory(phv_factory) {
  phvs.push_back(std::move(phv_factory.create()));
}

std::unique_ptr<PHV>
Packet::PHVPool::get() {
  if(phvs.size() == 0) {
    return phv_factory.create();
  }
  std::unique_ptr<PHV> phv = std::move(phvs.back());
  phvs.pop_back();
  return phv;
}

void
Packet::PHVPool::release(std::unique_ptr<PHV> phv) {
  phvs.push_back(std::move(phv));
}
