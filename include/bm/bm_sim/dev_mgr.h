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

//! @file dev_mgr.h
//! Includes the definitions for the DevMgrIface and DevMgr classes. DevMgr is a
//! base class for SwitchWContexts (and by extension Switch). It provides
//! facilities to send and receive packets, as well as manage ports. DevMgr
//! follows the pimpl design. Different implementations are available as
//! `backend` and they all implements the DevMgrIface interface. The different
//! implementations are:
//!   - BmiDevMgrImp: uses the BMI library (a libpcap wrapper) to send and
//! receive packets
//!   - PacketInDevMgrImp: uses a nanomsg PAIR socket to send and receive
//! packets
//!   - FilesDevMgrImp: reads incoming packets from pcap files and writes
//! outgoing packet to different pcap files

#ifndef BM_BM_SIM_DEV_MGR_H_
#define BM_BM_SIM_DEV_MGR_H_

#include <functional>
#include <string>
#include <map>

#include "packet_handler.h"
#include "port_monitor.h"

namespace bm {

class DevMgrIface : public PacketDispatcherIface {
 public:
  typedef PortMonitorIface::port_t port_t;
  typedef PortMonitorIface::PortStatus PortStatus;
  typedef PortMonitorIface::PortStatusCb PortStatusCb;

  struct PortInfo {
    PortInfo(port_t port_num, const std::string &iface_name)
        : port_num(port_num), iface_name(iface_name) { }

    void set_is_up(bool status) {
      is_up = status;
    }

    void add_extra(const std::string &key, const std::string &value) {
      extra.emplace(key, value);
    }

    port_t port_num{};
    std::string iface_name{};
    bool is_up{true};
    std::map<std::string, std::string> extra{};
  };

  virtual ~DevMgrIface();

  ReturnCode port_add(const std::string &iface_name, port_t port_num,
                      const char *in_pcap, const char *out_pcap);

  ReturnCode port_remove(port_t port_num);

  // TODO(antonin): add this?
  // ReturnCode set_port_status(port_t port_num, PortStatus status);

  void transmit_fn(int port_num, const char *buffer, int len) {
    transmit_fn_(port_num, buffer, len);
  }

  // start the thread that performs packet processing
  void start();

  ReturnCode set_packet_handler(const PacketHandler &handler, void *cookie);

  bool port_is_up(port_t port_num) const;

  ReturnCode register_status_cb(const PortStatus &type,
                                const PortStatusCb &port_cb);

  std::map<port_t, PortInfo> get_port_info() const;

 protected:
  std::unique_ptr<PortMonitorIface> p_monitor{nullptr};

 private:
  virtual ReturnCode port_add_(const std::string &iface_name, port_t port_num,
                               const char *in_pcap, const char *out_pcap) = 0;

  virtual ReturnCode port_remove_(port_t port_num) = 0;

  virtual void transmit_fn_(int port_num, const char *buffer, int len) = 0;

  virtual void start_() = 0;

  virtual ReturnCode set_packet_handler_(const PacketHandler &handler,
                                         void *cookie) = 0;

  virtual bool port_is_up_(port_t port_num) const = 0;

  virtual std::map<port_t, PortInfo> get_port_info_() const = 0;
};

// TODO(antonin): should DevMgr and DevMgrIface somehow inherit from a common
// interface, or is it not worth the trouble?

//! Base class for SwitchWContexts (and by extension Switch). It provides
//! facilities to send and receive packets, as well as manage ports.
class DevMgr : public PacketDispatcherIface {
 public:
  //! @copydoc PortMonitorIface::port_t
  typedef PortMonitorIface::port_t port_t;
  //! @copydoc PortMonitorIface::PortStatus
  typedef PortMonitorIface::PortStatus PortStatus;
  //! @copydoc PortMonitorIface::PortStatusCb
  typedef PortMonitorIface::PortStatusCb PortStatusCb;

  DevMgr();

  // set_dev_* : should be called before port_add and port_remove.

  // meant for testing
  void set_dev_mgr(std::unique_ptr<DevMgrIface> my_pimp);

  void set_dev_mgr_bmi(
      int device_id,
      std::shared_ptr<TransportIface> notifications_transport = nullptr);

  // The interface names are instead interpreted as file names.
  // wait_time_in_seconds indicate how long the starting thread should
  // wait before starting to process packets.
  void set_dev_mgr_files(unsigned wait_time_in_seconds);

  // if enforce ports is set to true, packets coming in on un-registered ports
  // are dropped
  void set_dev_mgr_packet_in(
      int device_id, const std::string &addr,
      std::shared_ptr<TransportIface> notifications_transport = nullptr,
      bool enforce_ports = false);

  ReturnCode port_add(const std::string &iface_name, port_t port_num,
                      const char *in_pcap, const char *out_pcap);

  ReturnCode port_remove(port_t port_num);

  bool port_is_up(port_t port_num) const;

  std::map<port_t, DevMgrIface::PortInfo> get_port_info() const;

  //! Transmits a data packet out of port \p port_num
  void transmit_fn(int port_num, const char *buffer, int len);

  ReturnCode set_packet_handler(const PacketHandler &handler, void *cookie)
      override;

  //! Register a callback function to be called every time the status of a port
  //! changes.
  ReturnCode register_status_cb(const PortStatus &type,
                                const PortStatusCb &port_cb);

  // start the thread that performs packet processing
  void start();

  DevMgr(const DevMgr &other) = delete;
  DevMgr &operator=(const DevMgr &other) = delete;

  DevMgr(DevMgr &&other) = delete;
  DevMgr &operator=(DevMgr &&other) = delete;

 protected:
  ~DevMgr();

 private:
  // Actual implementation (private)
  std::unique_ptr<DevMgrIface> pimp{nullptr};
};

}  // namespace bm

#endif  // BM_BM_SIM_DEV_MGR_H_
