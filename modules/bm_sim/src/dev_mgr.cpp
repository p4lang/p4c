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

namespace bm {

////////////////////////////////////////////////////////////////////////////////

// Implementation which uses Pcap files to read/write packets
class FilesDevMgrImp : public DevMgrIface {
 public:
  FilesDevMgrImp(bool respectTiming, unsigned wait_time_in_seconds)
      : reader(respectTiming, wait_time_in_seconds) {
    p_monitor = PortMonitorIface::make_dummy();
  }

 private:
  ReturnCode port_add_(const std::string &iface_name, port_t port_num,
                       const char *in_pcap, const char *out_pcap) {
    UNUSED(iface_name);
    reader.addFile(port_num, std::string(in_pcap));
    writer.addFile(port_num, std::string(out_pcap));
    ports.insert(port_num);
    return ReturnCode::SUCCESS;
  }

  ReturnCode port_remove_(port_t port_num) override {
    UNUSED(port_num);
    Logger::get()->warn("Removing ports not possible when reading pcap files");
    return ReturnCode::UNSUPPORTED;
  }

  void transmit_fn_(int port_num, const char *buffer, int len) override {
    writer.send_packet(port_num, buffer, len);
  }

  void start_() override {
    reader_thread = std::thread(&PcapFilesReader::start, &reader);
    reader_thread.detach();
  }

  ReturnCode set_packet_handler_(const PacketHandler &handler, void *cookie)
      override {
    reader.set_packet_handler(handler, cookie);
    return ReturnCode::SUCCESS;
  }

  bool port_is_up_(port_t port)
      override {
    return ports.find(port) != ports.end();
  }

 private:
  PcapFilesReader reader;
  PcapFilesWriter writer;
  std::thread reader_thread;
  std::unordered_set<port_t> ports{};
};

////////////////////////////////////////////////////////////////////////////////

DevMgrIface::~DevMgrIface() {
  p_monitor->stop();
}

PacketDispatcherIface::ReturnCode
DevMgrIface::port_add(const std::string &iface_name, port_t port_num,
                      const char *in_pcap, const char *out_pcap) {
  assert(p_monitor);
  p_monitor->notify(port_num, PortStatus::PORT_ADDED);
  return port_add_(iface_name, port_num, in_pcap, out_pcap);
}

PacketDispatcherIface::ReturnCode
DevMgrIface::port_remove(port_t port_num) {
  assert(p_monitor);
  p_monitor->notify(port_num, PortStatus::PORT_REMOVED);
  return port_remove_(port_num);
}

void
DevMgrIface::start() {
  assert(p_monitor);
  p_monitor->start(
      std::bind(&DevMgrIface::port_is_up, this, std::placeholders::_1));
  start_();
}

PacketDispatcherIface::ReturnCode
DevMgrIface::set_packet_handler(const PacketHandler &handler, void *cookie) {
  return set_packet_handler_(handler, cookie);
}

bool
DevMgrIface::port_is_up(port_t port_num) {
  return port_is_up_(port_num);
}

PacketDispatcherIface::ReturnCode
DevMgrIface::register_status_cb(const PortStatus &type,
                                const PortStatusCb &port_cb) {
  assert(p_monitor);
  p_monitor->register_cb(type, port_cb);
  return ReturnCode::SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////

DevMgr::DevMgr() { }

DevMgr::~DevMgr() { }

void
DevMgr::set_dev_mgr(std::unique_ptr<DevMgrIface> my_pimp) {
  assert(!pimp);
  pimp = std::move(my_pimp);
}

void
DevMgr::set_dev_mgr_files(unsigned wait_time_in_seconds) {
  assert(!pimp);
  pimp = std::unique_ptr<DevMgrIface>(new FilesDevMgrImp(
      false /* no real-time packet replay */, wait_time_in_seconds));
}

void
DevMgr::start() {
  assert(pimp);
  pimp->start();
}

PacketDispatcherIface::ReturnCode
DevMgr::port_add(const std::string &iface_name, port_t port_num,
                 const char *pcap_in, const char *pcap_out) {
  assert(pimp);
  BMLOG_DEBUG("Adding interface {} as port {}", iface_name, port_num);
  ReturnCode rc = pimp->port_add(iface_name, port_num, pcap_in, pcap_out);
  if (rc != ReturnCode::SUCCESS)
    Logger::get()->error("Add port operation failed");
  return rc;
}

void
DevMgr::transmit_fn(int port_num, const char *buffer, int len) {
  assert(pimp);
  pimp->transmit_fn(port_num, buffer, len);
}

PacketDispatcherIface::ReturnCode
DevMgr::port_remove(port_t port_num) {
  assert(pimp);
  BMLOG_DEBUG("Removing port {}", port_num);
  ReturnCode rc = pimp->port_remove(port_num);
  if (rc != ReturnCode::SUCCESS)
    Logger::get()->error("Remove port operation failed");
  return rc;
}

PacketDispatcherIface::ReturnCode
DevMgr::set_packet_handler(const PacketHandler &handler, void *cookie) {
  assert(pimp);
  return pimp->set_packet_handler(handler, cookie);
}

PacketDispatcherIface::ReturnCode
DevMgr::register_status_cb(const PortStatus &type,
                           const PortStatusCb &port_cb) {
  assert(pimp);
  return pimp->register_status_cb(type, port_cb);
}

bool
DevMgr::port_is_up(port_t port_num) {
  assert(pimp);
  return pimp->port_is_up(port_num);
}

}  // namespace bm
