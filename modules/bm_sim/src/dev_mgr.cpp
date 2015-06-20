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

#include <cassert>

#include "bm_sim/dev_mgr.h"

DevMgr::DevMgr() {
  assert(!bmi_port_create_mgr(&port_mgr));
}

DevMgr::~DevMgr() {
  bmi_port_destroy_mgr(port_mgr);
}

DevMgr::ReturnCode
DevMgr::port_add(const std::string &iface_name, port_t port_num,
		 const char *pcap)
{
  // TODO: check if port is taken...
  assert(!bmi_port_interface_add(port_mgr, iface_name.c_str(), port_num, pcap));
  return ReturnCode::SUCCESS;
}

#include <iostream>

DevMgr::ReturnCode
DevMgr::set_packet_handler(PacketHandler handler, void *cookie)
{
  typedef void function_t(int, const char *, int, void *);
  function_t** ptr_fun = handler.target<function_t *>();
  assert(ptr_fun);
  assert(*ptr_fun);
  assert(!bmi_set_packet_handler(port_mgr, *ptr_fun, cookie));
  return ReturnCode::SUCCESS;
}
