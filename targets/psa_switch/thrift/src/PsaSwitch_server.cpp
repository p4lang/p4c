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

#include <bm/config.h>
#include <bm/PsaSwitch.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

namespace thrift_provider = apache::thrift;

#include <bm/bm_sim/switch.h>
#include <bm/bm_sim/logger.h>
#include <bm/thrift/stdcxx.h>

#include "psa_switch.h"

using namespace bm::psa;

namespace pswitch_runtime {

class PsaSwitchHandler : virtual public PsaSwitchIf {
 public:
  explicit PsaSwitchHandler(PsaSwitch *sw)
    : switch_(sw) { }

  int32_t mirroring_mapping_add(const int32_t mirror_id,
                                const int32_t egress_port) {
    bm::Logger::get()->trace("mirroring_mapping_add");
    return switch_->mirroring_mapping_add(mirror_id, egress_port);
  }

  int32_t mirroring_mapping_delete(const int32_t mirror_id) {
    bm::Logger::get()->trace("mirroring_mapping_delete");
    return switch_->mirroring_mapping_delete(mirror_id);
  }

  int32_t mirroring_mapping_get_egress_port(const int32_t mirror_id) {
    bm::Logger::get()->trace("mirroring_mapping_get_egress_port");
    bm::port_t port;
    if (switch_->mirroring_mapping_get(mirror_id, &port)) {
      return port;
    }
    return -1;
  }

  int32_t set_egress_queue_depth(const int32_t port_num,
                                 const int32_t depth_pkts) {
    bm::Logger::get()->trace("set_egress_queue_depth");
    return switch_->set_egress_queue_depth(port_num,
                                           static_cast<uint32_t>(depth_pkts));
  }

  int32_t set_all_egress_queue_depths(const int32_t depth_pkts) {
    bm::Logger::get()->trace("set_all_egress_queue_depths");
    return switch_->set_all_egress_queue_depths(
        static_cast<uint32_t>(depth_pkts));
  }

  int32_t set_egress_queue_rate(const int32_t port_num,
                                const int64_t rate_pps) {
    bm::Logger::get()->trace("set_egress_queue_rate");
    return switch_->set_egress_queue_rate(port_num,
                                          static_cast<uint64_t>(rate_pps));
  }

  int32_t set_all_egress_queue_rates(const int64_t rate_pps) {
    bm::Logger::get()->trace("set_all_egress_queue_rates");
    return switch_->set_all_egress_queue_rates(static_cast<uint64_t>(rate_pps));
  }

  int64_t get_time_elapsed_us() {
    bm::Logger::get()->trace("get_time_elapsed_us");
    // cast from unsigned to signed
    return static_cast<int64_t>(switch_->get_time_elapsed_us());
  }

  int64_t get_time_since_epoch_us() {
    bm::Logger::get()->trace("get_time_since_epoch_us");
    // cast from unsigned to signed
    return static_cast<int64_t>(switch_->get_time_since_epoch_us());
  }

 private:
  PsaSwitch *switch_;
};

stdcxx::shared_ptr<PsaSwitchIf> get_handler(PsaSwitch *sw) {
  return stdcxx::shared_ptr<PsaSwitchHandler>(new PsaSwitchHandler(sw));
}

}  // namespace pswitch_runtime
