#include "bm_sim/packet.h"

#include "xxhash.h"

Packet::Packet(int ingress_port, packet_id_t id, packet_id_t copy_id,
	       PacketBuffer &&buffer)
  : ingress_port(ingress_port), packet_id(id), copy_id(copy_id),
    buffer(std::move(buffer)) {
  signature = XXH64(buffer.start(), buffer.get_data_size(), 0);
  // phv = phv_pool->get();
}

Packet::Packet(int ingress_port, packet_id_t id, packet_id_t copy_id,
	       PacketBuffer &&buffer, const PHV &src_phv)
  : ingress_port(ingress_port), packet_id(id), copy_id(copy_id),
    buffer(std::move(buffer)) {
  signature = XXH64(buffer.start(), buffer.get_data_size(), 0);
  // phv = phv_pool->get();
  // phv->copy_headers(src_phv);
}
  
Packet::~Packet() {
  // phv_pool->add(std::move(phv));
}

PHVPool *Packet::phv_pool = nullptr;

void
Packet::init_phv_pool(size_t size) {
  phv_pool = new PHVPool(size);
}

void
Packet::add_to_phv_pool(std::unique_ptr<PHV> phv) {
  phv_pool->add(std::move(phv));
}
