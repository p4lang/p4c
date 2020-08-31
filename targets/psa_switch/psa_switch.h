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

#ifndef PSA_SWITCH_PSA_SWITCH_H_
#define PSA_SWITCH_PSA_SWITCH_H_

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
#include <functional>

#include "externs/psa_counter.h"
#include "externs/psa_meter.h"

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

namespace bm {

namespace psa {

class PsaSwitch : public Switch {
 public:
  using mirror_id_t = int;

  using TransmitFn = std::function<void(port_t, packet_id_t,
                                        const char *, int)>;

  struct MirroringSessionConfig {
    port_t egress_port;
    bool egress_port_valid;
    unsigned int mgid;
    bool mgid_valid;
  };

 private:
  using clock = std::chrono::high_resolution_clock;

 public:
  // by default, swapping is off
  explicit PsaSwitch(bool enable_swap = false);

  ~PsaSwitch();

  int receive_(port_t port_num, const char *buffer, int len) override;

  void start_and_return_() override;

  void reset_target_state_() override;

  bool mirroring_add_session(mirror_id_t mirror_id,
                             const MirroringSessionConfig &config);
  bool mirroring_delete_session(mirror_id_t mirror_id);
  bool mirroring_get_session(mirror_id_t mirror_id,
                             MirroringSessionConfig *config) const;

  int mirroring_mapping_add(mirror_id_t mirror_id, port_t egress_port) {
    mirroring_map[mirror_id] = egress_port;
    return 0;
  }
  int mirroring_mapping_delete(mirror_id_t mirror_id) {
    return mirroring_map.erase(mirror_id);
  }
  bool mirroring_mapping_get(mirror_id_t mirror_id, port_t *port) const {
    return get_mirroring_mapping(mirror_id, port);
  }

  int set_egress_queue_depth(size_t port, const size_t depth_pkts);
  int set_all_egress_queue_depths(const size_t depth_pkts);

  int set_egress_queue_rate(size_t port, const uint64_t rate_pps);
  int set_all_egress_queue_rates(const uint64_t rate_pps);

  // returns the number of microseconds elapsed since the switch started
  uint64_t get_time_elapsed_us() const;

  // returns the number of microseconds elasped since the clock's epoch
  uint64_t get_time_since_epoch_us() const;

  // returns the packet id of most recently received packet. Not thread-safe.
  static packet_id_t get_packet_id() {
    return (packet_id-1);
  }

  void set_transmit_fn(TransmitFn fn);

  // overriden interfaces
  Counter::CounterErrorCode
  read_counters(cxt_id_t cxt_id,
                const std::string &counter_name,
                size_t index,
                MatchTableAbstract::counter_value_t *bytes,
                MatchTableAbstract::counter_value_t *packets) override {
    auto *context = get_context(cxt_id);
    auto *ex = context->get_extern_instance(counter_name).get();
    if (!ex) return Counter::CounterErrorCode::INVALID_COUNTER_NAME;
    auto *counter = static_cast<PSA_Counter*>(ex);
    if (index >= counter->size())
      return Counter::CounterErrorCode::INVALID_INDEX;
    counter->get_counter(index).query_counter(bytes, packets);
    return Counter::CounterErrorCode::SUCCESS;
  }

  Counter::CounterErrorCode
  write_counters(cxt_id_t cxt_id,
                 const std::string &counter_name,
                 size_t index,
                 MatchTableAbstract::counter_value_t bytes,
                 MatchTableAbstract::counter_value_t packets) override {
    auto *context = get_context(cxt_id);
    auto *ex = context->get_extern_instance(counter_name).get();
    if (!ex) return Counter::CounterErrorCode::INVALID_COUNTER_NAME;
    auto *counter = static_cast<PSA_Counter*>(ex);
    if (index >= counter->size())
      return Counter::CounterErrorCode::INVALID_INDEX;
    counter->get_counter(index).write_counter(bytes, packets);
    return Counter::CounterErrorCode::SUCCESS;
  }

  Counter::CounterErrorCode
  reset_counters(cxt_id_t cxt_id,
                 const std::string &counter_name) override {
    Context *context = get_context(cxt_id);
    ExternType *ex = context->get_extern_instance(counter_name).get();
    if (!ex) return Counter::CounterErrorCode::INVALID_COUNTER_NAME;
    PSA_Counter *counter = static_cast<PSA_Counter*>(ex);
    return counter->reset_counters();
  }

  MeterErrorCode
  meter_array_set_rates(
      cxt_id_t cxt_id, const std::string &meter_name,
      const std::vector<Meter::rate_config_t> &configs) override {
    auto *context = get_context(cxt_id);
    auto *ex = context->get_extern_instance(meter_name).get();
    if (!ex) return Meter::MeterErrorCode::INVALID_METER_NAME;
    auto *meter = static_cast<PSA_Meter*>(ex);
    meter->set_rates(configs);
    return Meter::MeterErrorCode::SUCCESS;
  }

  MeterErrorCode
  meter_set_rates(cxt_id_t cxt_id,
                  const std::string &meter_name, size_t idx,
                  const std::vector<Meter::rate_config_t> &configs) override {
    auto *context = get_context(cxt_id);
    auto *ex = context->get_extern_instance(meter_name).get();
    if (!ex) return Meter::MeterErrorCode::INVALID_METER_NAME;
    auto *meter = static_cast<PSA_Meter*>(ex);
    if (idx >= meter->size())
      return Meter::MeterErrorCode::INVALID_INDEX;
    meter->get_meter(idx).set_rates(configs);
    return Meter::MeterErrorCode::SUCCESS;
  }

  MeterErrorCode
  meter_get_rates(cxt_id_t cxt_id,
                  const std::string &meter_name, size_t idx,
                  std::vector<Meter::rate_config_t> *configs) override {
    auto *context = get_context(cxt_id);
    auto *ex = context->get_extern_instance(meter_name).get();
    if (!ex) return Meter::MeterErrorCode::INVALID_METER_NAME;
    auto *meter = static_cast<PSA_Meter*>(ex);
    if (idx >= meter->size())
      return Meter::MeterErrorCode::INVALID_INDEX;
    auto conf_vec = meter->get_meter(idx).get_rates();
    for (auto conf : conf_vec)
        configs->push_back(conf);
    return Meter::MeterErrorCode::SUCCESS;
  }

  // TODO(derek): override RuntimeInterface methods not yet supported
  //              by psa_switch and log an error msg / return error code

 private:
  static constexpr size_t nb_egress_threads = 4u;
  static constexpr port_t PSA_PORT_RECIRCULATE = 0xfffffffa;
  static packet_id_t packet_id;

  class MirroringSessions;

  enum PktInstanceType {
    PACKET_PATH_NORMAL,
    PACKET_PATH_NORMAL_UNICAST,
    PACKET_PATH_NORMAL_MULTICAST,
    PACKET_PATH_CLONE_I2E,
    PACKET_PATH_CLONE_E2E,
    PACKET_PATH_RESUBMIT,
    PACKET_PATH_RECIRCULATE,
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

  void multicast(Packet *packet, unsigned int mgid, PktInstanceType path, unsigned int class_of_service);

  bool get_mirroring_mapping(mirror_id_t mirror_id, port_t *port) const {
    const auto it = mirroring_map.find(mirror_id);
    if (it != mirroring_map.end()) {
      *port = it->second;
      return true;
    }
    return false;
  }

  ts_res get_ts() const;

  // TODO(antonin): switch to pass by value?
  void enqueue(port_t egress_port, std::unique_ptr<Packet> &&packet);

  void check_queueing_metadata();

 private:
  std::vector<std::thread> threads_;
  Queue<std::unique_ptr<Packet> > input_buffer;
#ifdef SSWITCH_PRIORITY_QUEUEING_ON
  bm::QueueingLogicPriRL<std::unique_ptr<Packet>, EgressThreadMapper>
#else
  bm::QueueingLogicRL<std::unique_ptr<Packet>, EgressThreadMapper>
#endif
  egress_buffers;
  Queue<std::unique_ptr<Packet> > output_buffer;
  TransmitFn my_transmit_fn;
  std::shared_ptr<McSimplePreLAG> pre;
  clock::time_point start;
  std::unordered_map<mirror_id_t, port_t> mirroring_map;
  std::unique_ptr<MirroringSessions> mirroring_sessions;
  bool with_queueing_metadata{false};
};

}  // namespace bm::psa

}  // namespace bm

#endif  // PSA_SWITCH_PSA_SWITCH_H_
