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

#ifndef SIMPLE_SWITCH_SIMPLE_SWITCH_H_
#define SIMPLE_SWITCH_SIMPLE_SWITCH_H_

#include <bm/bm_sim/queue.h>
#include <bm/bm_sim/queueing.h>
#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/switch.h>
#include <bm/bm_sim/event_logger.h>
#include <bm/bm_sim/simple_pre_lag.h>

#include <memory>
#include <chrono>
#include <thread>
#include <vector>

// TODO(antonin)
// experimental support for priority queueing
// to enable it, uncomment this flag
// you can also choose the field from which the priority value will be read, as
// well as the number of priority queues per port
// PRIORITY 0 IS THE LOWEST PRIORITY
// #define SSWITCH_PRIORITY_QUEUEING_ON

#ifdef SSWITCH_PRIORITY_QUEUEING_ON
#define SSWITCH_PRIORITY_QUEUEING_NB_QUEUES 8
#define SSWITCH_PRIORITY_QUEUEING_SRC "intrinsic_metadata.priority"
#endif

using ts_res = std::chrono::microseconds;
using std::chrono::duration_cast;
using ticks = std::chrono::nanoseconds;

using bm::Switch;
using bm::Queue;
using bm::Packet;
using bm::PHV;
using bm::Parser;
using bm::Deparser;
using bm::Pipeline;
using bm::McSimplePreLAG;
using bm::Field;
using bm::FieldList;
using bm::packet_id_t;
using bm::p4object_id_t;


class SimpleSwitch : public Switch {
 public:
  using mirror_id_t = int;

 private:
  using clock = std::chrono::high_resolution_clock;

 public:
  // by default, swapping is off
  explicit SimpleSwitch(int max_port = 256, bool enable_swap = false);

  int receive_(int port_num, const char *buffer, int len) override;

  void start_and_return_() override;

  void reset_target_state_() override;

  int mirroring_mapping_add(mirror_id_t mirror_id, int egress_port) {
    mirroring_map[mirror_id] = egress_port;
    return 0;
  }

  int mirroring_mapping_delete(mirror_id_t mirror_id) {
    return mirroring_map.erase(mirror_id);
  }

  int mirroring_mapping_get(mirror_id_t mirror_id) const {
    return get_mirroring_mapping(mirror_id);
  }

  int set_egress_queue_depth(int port, const size_t depth_pkts);
  int set_all_egress_queue_depths(const size_t depth_pkts);

  int set_egress_queue_rate(int port, const uint64_t rate_pps);
  int set_all_egress_queue_rates(const uint64_t rate_pps);

  // returns the number of microseconds elapsed since the switch started
  uint64_t get_time_elapsed_us() const;

  // returns the number of microseconds elasped since the clock's epoch
  uint64_t get_time_since_epoch_us() const;

 private:
  static constexpr size_t nb_egress_threads = 4u;

  enum PktInstanceType {
    PKT_INSTANCE_TYPE_NORMAL,
    PKT_INSTANCE_TYPE_INGRESS_CLONE,
    PKT_INSTANCE_TYPE_EGRESS_CLONE,
    PKT_INSTANCE_TYPE_COALESCED,
    PKT_INSTANCE_TYPE_RECIRC,
    PKT_INSTANCE_TYPE_REPLICATION,
    PKT_INSTANCE_TYPE_RESUBMIT,
  };

  struct EgressThreadMapper {
    explicit EgressThreadMapper(size_t nb_threads)
        : nb_threads(nb_threads) { }

    size_t operator()(size_t egress_port) const {
      return egress_port % nb_threads;
    }

    size_t nb_threads;
  };

 private:
  void ingress_thread();
  void egress_thread(size_t worker_id);
  void transmit_thread();

  int get_mirroring_mapping(mirror_id_t mirror_id) const {
    const auto it = mirroring_map.find(mirror_id);
    if (it == mirroring_map.end()) return -1;
    return it->second;
  }

  ts_res get_ts() const;

  // TODO(antonin): switch to pass by value?
  void enqueue(int egress_port, std::unique_ptr<Packet> &&pkt);

  std::unique_ptr<Packet> copy_ingress_pkt(
      const std::unique_ptr<Packet> &pkt,
      PktInstanceType copy_type, p4object_id_t field_list_id);

  void check_queueing_metadata();

 private:
  int max_port;
  Queue<std::unique_ptr<Packet> > input_buffer;
#ifdef SSWITCH_PRIORITY_QUEUEING_ON
  bm::QueueingLogicPriRL<std::unique_ptr<Packet>, EgressThreadMapper>
#else
  bm::QueueingLogicRL<std::unique_ptr<Packet>, EgressThreadMapper>
#endif
  egress_buffers;
  Queue<std::unique_ptr<Packet> > output_buffer;
  std::shared_ptr<McSimplePreLAG> pre;
  clock::time_point start;
  std::unordered_map<mirror_id_t, int> mirroring_map;
  bool with_queueing_metadata{false};
};

#endif  // SIMPLE_SWITCH_SIMPLE_SWITCH_H_
