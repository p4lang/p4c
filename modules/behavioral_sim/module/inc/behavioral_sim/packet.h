#ifndef _BM_PACKET_H_
#define _BM_PACKET_H_

#include <memory>
#include <iostream>

#include <cassert>

#include "packet_buffer.h"

using std::string;
using std::to_string;
using std::unique_ptr;

typedef unsigned long long packet_id_t;

class Packet {
public:
  Packet() {}

  Packet(packet_id_t id, packet_id_t copy_id, PacketBuffer &&buffer)
    : packet_id(id), copy_id(copy_id), buffer(std::move(buffer)) {}

  packet_id_t get_packet_id() const { return packet_id; }

  packet_id_t get_copy_id() const { return copy_id; }

  const string get_unique_id() const {
    return to_string(packet_id) + "." + to_string(copy_id);
  }

  void set_copy_id(packet_id_t id) { copy_id = id; }

  void set_payload_size(size_t size) { payload_size = size; }

  size_t get_payload_size() const { return payload_size; }

  size_t get_data_size() const { return buffer.get_data_size(); }

  char *data() { return buffer.start(); }

  char *payload() { 
    assert(payload_size > 0);
    return buffer.end() - payload_size;
  }

  char *prepend(size_t bytes) { return buffer.push(bytes); }

  char *remove(size_t bytes) {
    assert(buffer.get_data_size() >= payload_size + bytes);
    return buffer.pop(bytes);
  }

  const PacketBuffer &get_packet_buffer() const { return buffer; }

private:
  packet_id_t packet_id;
  packet_id_t copy_id;

  PacketBuffer buffer;

  size_t payload_size{0};
};

#endif
