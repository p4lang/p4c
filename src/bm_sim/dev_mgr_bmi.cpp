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

#include <bm/bm_sim/dev_mgr.h>
#include <bm/bm_sim/logger.h>

#include <cassert>
#include <cstdlib>
#include <mutex>
#include <map>
#include <string>

extern "C" {
#include "BMI/bmi_port.h"
}

namespace bm {

// These are private implementations

// Implementation that uses the BMI to send/receive packets
// from true interfaces

// I am putting this in its own cpp file to avoid having to link with the BMI
// library in other DevMgr tests

class BmiDevMgrImp : public DevMgrIface {
 public:
  BmiDevMgrImp(device_id_t device_id,
               int max_port_count,
               std::shared_ptr<TransportIface> notifications_transport) {
    if (bmi_port_create_mgr(&port_mgr, max_port_count)) {
      Logger::get()->critical("Could not initialize BMI port manager");
      std::exit(1);
    }

    p_monitor = PortMonitorIface::make_active(device_id,
                                              notifications_transport);
  }

 private:
  ~BmiDevMgrImp() override {
    bmi_port_destroy_mgr(port_mgr);
  }

  ReturnCode port_add_(const std::string &iface_name, port_t port_num,
                       const PortExtras &port_extras) override {
    auto it_in_pcap = port_extras.find(kPortExtraInPcap);
    auto it_out_pcap = port_extras.find(kPortExtraOutPcap);
    const char *in_pcap = (it_in_pcap == port_extras.end()) ?
        NULL : it_in_pcap->second.c_str();
    const char *out_pcap = (it_out_pcap == port_extras.end()) ?
        NULL : it_out_pcap->second.c_str();
    if (bmi_port_interface_add(port_mgr, iface_name.c_str(), port_num, in_pcap,
                               out_pcap))
      return ReturnCode::ERROR;

    PortInfo p_info(port_num, iface_name, port_extras);

    Lock lock(mutex);
    port_info.emplace(port_num, std::move(p_info));

    return ReturnCode::SUCCESS;
  }

  ReturnCode port_remove_(port_t port_num) override {
    if (bmi_port_interface_remove(port_mgr, port_num))
      return ReturnCode::ERROR;

    Lock lock(mutex);
    port_info.erase(port_num);

    return ReturnCode::SUCCESS;
  }

  void transmit_fn_(port_t port_num, const char *buffer, int len) override {
    bmi_port_send(port_mgr, port_num, buffer, len);
  }

  void start_() override {
    assert(port_mgr);
    if (bmi_start_mgr(port_mgr))
      Logger::get()->critical("Could not start BMI port manager");
  }

  ReturnCode set_packet_handler_(const PacketHandler &handler, void *cookie)
      override {
    using function_t = void(int, const char *, int, void *);
    function_t * const*ptr_fun = handler.target<function_t *>();
    assert(ptr_fun);
    assert(*ptr_fun);
    if (bmi_set_packet_handler(port_mgr, *ptr_fun, cookie)) {
      Logger::get()->critical("Could not set BMI packet handler");
      return ReturnCode::ERROR;
    }
    return ReturnCode::SUCCESS;
  }

  bool port_is_up_(port_t port) const override {
    bool is_up = false;
    assert(port_mgr);
    int rval = bmi_port_interface_is_up(port_mgr, port, &is_up);
    is_up &= !(rval);
    return is_up;
  }

  std::map<port_t, PortInfo> get_port_info_() const override {
    std::map<port_t, PortInfo> info;
    {
      Lock lock(mutex);
      info = port_info;
    }
    for (auto &pi : info) {
      pi.second.is_up = port_is_up_(pi.first);
    }
    return info;
  }

  PortStats get_port_stats_(port_t port) const override {
    bmi_port_stats_t port_stats;
    bmi_port_get_stats(port_mgr, port, &port_stats);
    return {port_stats.in_packets, port_stats.in_octets,
          port_stats.out_packets, port_stats.out_octets};
  }

  PortStats clear_port_stats_(port_t port) override {
    bmi_port_stats_t port_stats;
    bmi_port_clear_stats(port_mgr, port, &port_stats);
    return {port_stats.in_packets, port_stats.in_octets,
          port_stats.out_packets, port_stats.out_octets};
  }

 private:
  using Mutex = std::mutex;
  using Lock = std::lock_guard<std::mutex>;

  bmi_port_mgr_t *port_mgr{nullptr};
  mutable Mutex mutex;
  std::map<port_t, DevMgrIface::PortInfo> port_info;
};

void
DevMgr::set_dev_mgr_bmi(
    device_id_t device_id,
    int max_port_count,
    std::shared_ptr<TransportIface> notifications_transport) {
  assert(!pimp);
  pimp = std::unique_ptr<DevMgrIface>(
      new BmiDevMgrImp(device_id, max_port_count, notifications_transport));
}

}  // namespace bm
