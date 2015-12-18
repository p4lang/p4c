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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or pimpied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include <cassert>
#include <thread>
#include <string>
#include <unordered_set>

#define UNUSED(x) (void)(x)

#include "bm_sim/dev_mgr.h"
#include "bm_sim/logger.h"
#include "bm_sim/pcap_file.h"
#include "bm_sim/nn.h"

////////////////////////////////////////////////////////////////////////////////

// Implementation which uses Pcap files to read/write packets
class FilesDevMgrPimpementation : public DevMgrInterface {
 public:
  FilesDevMgrPimpementation(bool respectTiming, unsigned wait_time_in_seconds)
      : reader(respectTiming, wait_time_in_seconds) {}

  ReturnCode port_add(const std::string &iface_name, port_t port_num,
                      const char *in_pcap, const char *out_pcap) {
    UNUSED(iface_name);
    reader.addFile(port_num, std::string(in_pcap));
    writer.addFile(port_num, std::string(out_pcap));
    ports.insert(port_num);
    return ReturnCode::SUCCESS;
  }

  ReturnCode port_remove(port_t port_num) {
    UNUSED(port_num);
    Logger::get()->warn("Removing ports not possible when reading pcap files");
    return ReturnCode::UNSUPPORTED;
  }

  void transmit_fn(int port_num, const char *buffer, int len) {
    writer.send_packet(port_num, buffer, len);
  }

  void start() {
    reader_thread = std::thread(&PcapFilesReader::start, &reader);
    reader_thread.detach();
  }

  ReturnCode set_packet_handler(const PacketHandler &handler, void *cookie) {
    reader.set_packet_handler(handler, cookie);
    return ReturnCode::SUCCESS;
  }

  bool port_is_up(port_t port) {
    return ports.find(port) != ports.end();
  }

 private:
  PcapFilesReader reader;
  PcapFilesWriter writer;
  std::thread reader_thread;
  std::unordered_set<port_t> ports{};
};

////////////////////////////////////////////////////////////////////////////////

DevMgr::DevMgr() { }

DevMgr::~DevMgr() {
  p_monitor->stop();
}

void
DevMgr::set_dev_mgr(std::unique_ptr<DevMgrInterface> my_pimp,
                    std::unique_ptr<PortMonitorIface> my_p_monitor) {
  assert(!pimp);
  pimp = std::move(my_pimp);
  p_monitor = std::move(my_p_monitor);
}

void
DevMgr::set_dev_mgr_files(unsigned wait_time_in_seconds) {
  assert(!pimp);
  pimp = std::unique_ptr<DevMgrInterface>(new FilesDevMgrPimpementation(
      false /* no real-time packet replay */, wait_time_in_seconds));
  p_monitor = PortMonitorIface::make_dummy();
}

void
DevMgr::start() {
  assert(pimp);
  p_monitor->start(std::bind(&DevMgr::port_is_up, this, std::placeholders::_1));
  pimp->start();
}

PacketDispatcherInterface::ReturnCode
DevMgr::port_add(const std::string &iface_name, port_t port_num,
                 const char *pcap_in, const char *pcap_out) {
  // TODO(antonin): check if port is taken...
  assert(pimp);
  BMLOG_DEBUG("Adding interface {} as port {}", iface_name, port_num);
  ReturnCode rc = pimp->port_add(iface_name, port_num, pcap_in, pcap_out);
  if (rc != ReturnCode::SUCCESS)
    Logger::get()->error("Add port operation failed");
  else
    p_monitor->notify(port_num, PortStatus::PORT_ADDED);
  return rc;
}

void
DevMgr::transmit_fn(int port_num, const char *buffer, int len) {
  assert(pimp);
  pimp->transmit_fn(port_num, buffer, len);
}

PacketDispatcherInterface::ReturnCode
DevMgr::port_remove(port_t port_num) {
  assert(pimp);
  BMLOG_DEBUG("Removing port {}", port_num);
  ReturnCode rc = pimp->port_remove(port_num);
  if (rc != ReturnCode::SUCCESS)
    Logger::get()->error("Remove port operation failed");
  else
    p_monitor->notify(port_num, PortStatus::PORT_REMOVED);
  return rc;
}

PacketDispatcherInterface::ReturnCode
DevMgr::set_packet_handler(const PacketHandler &handler, void *cookie) {
  assert(pimp);
  return pimp->set_packet_handler(handler, cookie);
}

PacketDispatcherInterface::ReturnCode
DevMgr::register_status_cb(const PortStatus &type,
                           const PortStatusCb &port_cb) {
  assert(pimp);
  p_monitor->register_cb(type, port_cb);
  return ReturnCode::SUCCESS;
}

bool
DevMgr::port_is_up(port_t port_num) {
  assert(pimp);
  return pimp->port_is_up(port_num);
}
