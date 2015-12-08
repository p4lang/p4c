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

/* -*-c++-*-
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#ifndef BM_SIM_INCLUDE_BM_SIM_DEV_MGR_H_
#define BM_SIM_INCLUDE_BM_SIM_DEV_MGR_H_

#include <functional>
#include <string>

#include "bm_sim/packet_handler.h"
#include "bm_sim/pcap_file.h"

class DevMgrInterface : public PacketDispatcherInterface {
 public:
  enum class PortStatus { PORT_ADDED, PORT_REMOVED, PORT_UP, PORT_DOWN };

  typedef unsigned int port_t;
  typedef std::function<void(port_t port_num, const PortStatus &status)>
      PortStatusCB;

  virtual ReturnCode port_add(const std::string &iface_name, port_t port_num,
                              const char *in_pcap, const char *out_pcap) = 0;

  virtual ReturnCode port_remove(port_t port_num) = 0;

  virtual void transmit_fn(int port_num, const char *buffer, int len) = 0;

  // start the thread that performs packet processing
  virtual void start() = 0;
  virtual ReturnCode register_status_cb(const PortStatus &status,
                                        const PortStatusCB &cb) = 0;

  virtual ReturnCode set_packet_handler(PacketHandler handler,
                                        void *cookie) = 0;

  virtual bool port_is_up(port_t port_num) = 0;

  virtual ~DevMgrInterface() {}
};

class DevMgr : public DevMgrInterface {
 public:
  DevMgr();

  // If useFiles is true I/O is performed from PCAP files.
  // The interface names are instead interpreted as file names.
  // Should be called before port_add and port_remove.
  // wait_time_in_seconds indicate how long the starting thread should
  // wait before starting to process packets.
  void setUseFiles(bool useFiles, unsigned wait_time_in_seconds);

  ReturnCode port_add(const std::string &iface_name, port_t port_num,
                      const char *in_pcap, const char *out_pcap);

  ReturnCode port_remove(port_t port_num);

  bool port_is_up(port_t port_num) override;

  void transmit_fn(int port_num, const char *buffer, int len);

  // start the thread that performs packet processing
  void start();
  ReturnCode register_status_cb(const PortStatus &status,
                                const PortStatusCB &cb) override;

  DevMgr(const DevMgr &other) = delete;
  DevMgr &operator=(const DevMgr &other) = delete;

  DevMgr(DevMgr &&other) = delete;
  DevMgr &operator=(DevMgr &&other) = delete;

 protected:
  virtual ~DevMgr() {}
  ReturnCode set_packet_handler(PacketHandler handler, void *cookie);

 private:
  // Actual implementation (private)
  std::unique_ptr<DevMgrInterface> impl;
};

#endif  // BM_SIM_INCLUDE_BM_SIM_DEV_MGR_H_
