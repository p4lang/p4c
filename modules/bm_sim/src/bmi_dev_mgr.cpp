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

#include <string>

#include "bm_sim/dev_mgr.h"
#include "bm_sim/port_monitor.h"

extern "C" {
#include "BMI/bmi_port.h"
}

// These are private implementations

// Implementation that uses the BMI to send/receive packets
// from true interfaces

// I am putting this in its own cpp file to avoid having to link with the BMI
// library in other DevMgr tests

class BmiDevMgrImplementation : public DevMgrInterface {
 public:
  BmiDevMgrImplementation() {
    assert(!bmi_port_create_mgr(&port_mgr));
    p_monitor.start(this);
  }

  ~BmiDevMgrImplementation() {
    p_monitor.stop();
    bmi_port_destroy_mgr(port_mgr);
  }

  ReturnCode port_add(const std::string &iface_name, port_t port_num,
                      const char *in_pcap, const char *out_pcap) {
    if (bmi_port_interface_add(port_mgr, iface_name.c_str(), port_num, in_pcap,
                               out_pcap))
      return ReturnCode::ERROR;
    p_monitor.notify(port_num, PortStatus::PORT_ADDED);
    return ReturnCode::SUCCESS;
  }

  ReturnCode port_remove(port_t port_num) {
    if (bmi_port_interface_remove(port_mgr, port_num))
      return ReturnCode::ERROR;
    p_monitor.notify(port_num, PortStatus::PORT_REMOVED);
    return ReturnCode::SUCCESS;
  }

  void transmit_fn(int port_num, const char *buffer, int len) {
    bmi_port_send(port_mgr, port_num, buffer, len);
  }

  void start() {
    assert(port_mgr);
    int exitCode = bmi_start_mgr(port_mgr);
    if (exitCode != 0)
      bm_fatal_error("Could not start thread receiving packets");
  }

  ReturnCode register_status_cb(const PortStatus &type,
                                const PortStatusCB &port_cb) {
    p_monitor.register_cb(type, port_cb);
    return ReturnCode::SUCCESS;
  }

  ReturnCode set_packet_handler(const PacketHandler &handler, void *cookie) {
    typedef void function_t(int, const char *, int, void *);
    function_t * const*ptr_fun = handler.target<function_t *>();
    assert(ptr_fun);
    assert(*ptr_fun);
    assert(!bmi_set_packet_handler(port_mgr, *ptr_fun, cookie));
    return ReturnCode::SUCCESS;
  }

  bool port_is_up(port_t port) {
    bool is_up = false;
    assert(port_mgr);
    int rval = bmi_port_interface_is_up(port_mgr, port, &is_up);
    is_up &= !(rval);
    return is_up;
  }

 private:
  bmi_port_mgr_t *port_mgr{nullptr};
  PortMonitor p_monitor;
};

void
DevMgr::set_dev_mgr_bmi() {
  assert(!impl);
  impl = std::unique_ptr<DevMgrInterface>(new BmiDevMgrImplementation());
}
