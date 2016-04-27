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

#include <string>
#include <cassert>
#include <mutex>
#include <map>

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
  BmiDevMgrImp(int device_id,
               std::shared_ptr<TransportIface> notifications_transport) {
    assert(!bmi_port_create_mgr(&port_mgr));

    p_monitor = PortMonitorIface::make_active(device_id,
                                              notifications_transport);
  }

 private:
  ~BmiDevMgrImp() override {
    bmi_port_destroy_mgr(port_mgr);
  }

  ReturnCode port_add_(const std::string &iface_name, port_t port_num,
                       const char *in_pcap, const char *out_pcap) override {
    if (bmi_port_interface_add(port_mgr, iface_name.c_str(), port_num, in_pcap,
                               out_pcap))
      return ReturnCode::ERROR;

    PortInfo p_info(port_num, iface_name);
    if (in_pcap) p_info.add_extra("in_pcap", std::string(in_pcap));
    if (out_pcap) p_info.add_extra("out_pcap", std::string(out_pcap));

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

  void transmit_fn_(int port_num, const char *buffer, int len) override {
    bmi_port_send(port_mgr, port_num, buffer, len);
  }

  void start_() override {
    assert(port_mgr);
    assert(!bmi_start_mgr(port_mgr));
  }

  ReturnCode set_packet_handler_(const PacketHandler &handler, void *cookie)
      override {
    typedef void function_t(int, const char *, int, void *);
    function_t * const*ptr_fun = handler.target<function_t *>();
    assert(ptr_fun);
    assert(*ptr_fun);
    assert(!bmi_set_packet_handler(port_mgr, *ptr_fun, cookie));
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

 private:
  using Mutex = std::mutex;
  using Lock = std::lock_guard<std::mutex>;

  bmi_port_mgr_t *port_mgr{nullptr};
  mutable Mutex mutex;
  std::map<port_t, DevMgrIface::PortInfo> port_info;
};

void
DevMgr::set_dev_mgr_bmi(
    int device_id, std::shared_ptr<TransportIface> notifications_transport) {
  assert(!pimp);
  pimp = std::unique_ptr<DevMgrIface>(
      new BmiDevMgrImp(device_id, notifications_transport));
}

}  // namespace bm
