#ifndef _BM_PHV_H_
#define _BM_PHV_H_

#include <memory>
#include <iostream>

using std::unique_ptr;

typedef unsigned long long packet_id_t;

struct Packet {
  packet_id_t id;

  Packet(packet_id_t id)
    : id(id) {}
};

#endif
