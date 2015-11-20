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

#ifndef _BM_SIMPLE_SWITCH_SIMPLE_SWITCH_H_
#define _BM_SIMPLE_SWITCH_SIMPLE_SWITCH_H_

#include <memory>
#include <chrono>
#include <thread>

#include "bm_sim/queue.h"
#include "bm_sim/packet.h"
#include "bm_sim/switch.h"
#include "bm_sim/event_logger.h"
#include "bm_sim/simple_pre_lag.h"

using ts_res = std::chrono::microseconds;
using std::chrono::duration_cast;
using ticks = std::chrono::nanoseconds;

class PacketQueue : public Queue<std::unique_ptr<Packet> >
{
private:
  typedef std::chrono::high_resolution_clock clock;

public:
  PacketQueue()
    : Queue<std::unique_ptr<Packet> >(1024, WriteReturn, ReadBlock),
      last_sent(clock::now()), pkt_delay_ticks(0) { }

  void set_queue_rate(const uint64_t pps) {
    using std::chrono::duration;
    if(pps == 0) {
      pkt_delay_ticks = ticks::zero();
      return;
    }
    queue_rate_pps = pps;
    pkt_delay_ticks = duration_cast<ticks>(duration<double>(1. / pps));
  }

  void pop_back(std::unique_ptr<Packet> *pkt) {
    Queue<std::unique_ptr<Packet> >::pop_back(pkt);
    // enforce rate
    clock::time_point next_send = last_sent + pkt_delay_ticks;
    std::this_thread::sleep_until(next_send);
    last_sent = clock::now();
  }

private:
  uint64_t queue_rate_pps;
  clock::time_point last_sent;
  ticks pkt_delay_ticks{};
};

class SimpleSwitch : public Switch {
public:
  typedef int mirror_id_t;

private:
  typedef std::chrono::high_resolution_clock clock;

public:
  SimpleSwitch(int max_port = 256);

  int receive(int port_num, const char *buffer, int len) {
    static int pkt_id = 0;

    Packet *packet =
      new Packet(port_num, pkt_id++, 0, len, PacketBuffer(2048, buffer, len));

    ELOGGER->packet_in(*packet);

    // many current P4 programs assume this
    // it is also part of the original P4 spec
    packet->get_phv()->reset_metadata();

    input_buffer.push_front(std::unique_ptr<Packet>(packet));
    return 0;
  }

  void start_and_return();

  int mirroring_mapping_add(mirror_id_t mirror_id, int egress_port) {
    mirroring_map[mirror_id] = egress_port;
    return 0;
  }

  int mirroring_mapping_delete(mirror_id_t mirror_id) {
    return mirroring_map.erase(mirror_id);
  }

  int set_egress_queue_depth(const size_t depth_pkts) {
    for(int i = 0; i < max_port; i++) {
      egress_buffers[i].set_capacity(depth_pkts);
    }
    return 0;
  }

  int set_egress_queue_rate(const uint64_t rate_pps) {
    for(int i = 0; i < max_port; i++) {
      egress_buffers[i].set_queue_rate(rate_pps);
    }
    return 0;
  }

private:
  enum PktInstanceType {
    PKT_INSTANCE_TYPE_NORMAL,
    PKT_INSTANCE_TYPE_INGRESS_CLONE,
    PKT_INSTANCE_TYPE_EGRESS_CLONE,
    PKT_INSTANCE_TYPE_COALESCED,
    PKT_INSTANCE_TYPE_INGRESS_RECIRC,
    PKT_INSTANCE_TYPE_REPLICATION,
    PKT_INSTANCE_TYPE_RESUBMIT,
  };

private:
  void ingress_thread();
  void egress_thread(int port);
  void transmit_thread();

  int get_mirroring_mapping(mirror_id_t mirror_id) const {
    const auto it = mirroring_map.find(mirror_id);
    if(it == mirroring_map.end()) return -1;
    return it->second;
  }

  ts_res get_ts() const;

  void enqueue(int egress_port, std::unique_ptr<Packet> &&pkt);

private:
  int max_port;
  Queue<std::unique_ptr<Packet> > input_buffer;
  std::vector<PacketQueue> egress_buffers{};
  Queue<std::unique_ptr<Packet> > output_buffer;
  std::shared_ptr<McSimplePreLAG> pre;
  clock::time_point start;
  std::unordered_map<mirror_id_t, int> mirroring_map;
  std::mt19937 gen;
  std::uniform_int_distribution<packet_id_t> copy_id_dis{};
};

#endif
