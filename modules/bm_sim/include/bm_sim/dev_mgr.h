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

#ifndef _BM_DEV_MGR_H_
#define _BM_DEV_MGR_H_

#include <string>
#include <functional>

extern "C" {
#include "BMI/bmi_port.h"
}

class DevMgr {
public:
  typedef unsigned int port_t;
  enum class ReturnCode {
    SUCCESS,
    ERROR
  };
  typedef std::function<void(int port_num, const char *buffer, int len, void *cookie)> PacketHandler;

public:
  DevMgr();
  
  ReturnCode port_add(const std::string &iface_name, port_t port_num,
		      const char *pcap);

  ReturnCode port_remove(port_t port_num);

  void transmit_fn(int port_num, const char *buffer, int len) {
    bmi_port_send(port_mgr, port_num, buffer, len);
  }

  DevMgr(const DevMgr &other) = delete;
  DevMgr &operator=(const DevMgr &other) = delete;

  DevMgr(DevMgr &&other) = delete;
  DevMgr &operator=(DevMgr &&other) = delete;

protected:
  ~DevMgr();

  ReturnCode set_packet_handler(PacketHandler handler, void *cookie);
  
private:
  bmi_port_mgr_t *port_mgr{nullptr};
};

#endif
