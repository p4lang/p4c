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

#include <nanomsg/pair.h>

#include <cassert>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <string>
#include <atomic>

#define UNUSED(x) (void)(x)

#include "bm_sim/dev_mgr.h"
#include "bm_sim/logger.h"

#include "bm_sim/nn.h"

////////////////////////////////////////////////////////////////////////////////

// These are private implementations

// Implementation which uses Pcap files to read/write packets
class FilesDevMgrImplementation : public DevMgrInterface {
 public:
  FilesDevMgrImplementation(bool respectTiming, unsigned wait_time_in_seconds)
      : reader(respectTiming, wait_time_in_seconds) {}

  ReturnCode port_add(const std::string &iface_name, port_t port_num,
                      const char *in_pcap, const char *out_pcap) {
    UNUSED(iface_name);
    reader.addFile(port_num, std::string(in_pcap));
    writer.addFile(port_num, std::string(out_pcap));
    return ReturnCode::SUCCESS;
  }

  ReturnCode port_remove(port_t port_num) {
    UNUSED(port_num);
    bm_fatal_error("Removing ports not implemented when reading pcap files");
    return ReturnCode::ERROR;  // unreachable
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
  ReturnCode register_status_cb(const PortStatus &type,
                                const PortStatusCB &port_cb) {
    UNUSED(type);
    UNUSED(port_cb);
    bm_fatal_error(
        "Port Status Callback not implemented when reading pcap files");
    return ReturnCode::ERROR;  // unreachable
  }
  bool port_is_up(port_t port) {
    UNUSED(port);
    bm_fatal_error(
        "Port monitoring callback not implemented when reading pcap files");
    return false;  // unreachable
  }

 private:
  PcapFilesReader reader;
  PcapFilesWriter writer;
  std::thread reader_thread;
};

////////////////////////////////////////////////////////////////////////////////

// Implementation which uses a nanomsg PAIR socket to receive / send packets
// TODO(antonin): moved lower-level NN code to separate class ?
class PacketInDevMgrImplementation : public DevMgrInterface {
 public:
  explicit PacketInDevMgrImplementation(const std::string &addr)
      : addr(addr), s(AF_SP, NN_PAIR) {
    s.bind(addr.c_str());
    int rcv_timeout_ms = 100;
    s.setsockopt(NN_SOL_SOCKET, NN_RCVTIMEO,
                 &rcv_timeout_ms, sizeof(rcv_timeout_ms));
  }

  ~PacketInDevMgrImplementation() {
    if (started) {
      stop_receive_thread = true;
      receive_thread.join();
    }
  }

  ReturnCode port_add(const std::string &iface_name, port_t port_num,
                      const char *in_pcap, const char *out_pcap) {
    // TODO(antonin): add a special message to add ports
    UNUSED(iface_name);
    UNUSED(port_num);
    UNUSED(in_pcap);
    UNUSED(out_pcap);
    return ReturnCode::SUCCESS;
  }

  ReturnCode port_remove(port_t port_num) {
    // TODO(antonin): add a special message to remove ports
    UNUSED(port_num);
    return ReturnCode::SUCCESS;
  }

  void transmit_fn(int port_num, const char *buffer, int len);

  void start() {
    if (started || stop_receive_thread)
      return;
    receive_thread = std::thread(
        &PacketInDevMgrImplementation::receive_loop, this);
    started = true;
  }

  ReturnCode set_packet_handler(const PacketHandler &handler, void *cookie) {
    pkt_handler = handler;
    pkt_cookie = cookie;
    return ReturnCode::SUCCESS;
  }

  ReturnCode register_status_cb(const PortStatus &type,
                                const PortStatusCB &port_cb) {
    // TODO(antonin)
    UNUSED(type);
    UNUSED(port_cb);
    return ReturnCode::SUCCESS;
  }

  bool port_is_up(port_t port) {
    // TODO(antonin)
    UNUSED(port);
    return true;
  }

 private:
  void receive_loop();

 private:
  struct packet_hdr_t {
    int port;
    int len;
  } __attribute__((packed));

 private:
  std::string addr{};
  nn::socket s;
  PacketHandler pkt_handler{};
  void *pkt_cookie{nullptr};
  std::thread receive_thread{};
  std::atomic<bool> stop_receive_thread{false};
  std::atomic<bool> started{false};
};

void
PacketInDevMgrImplementation::transmit_fn(int port_num,
                                          const char *buffer, int len) {
  struct nn_msghdr msghdr;
  std::memset(&msghdr, 0, sizeof(msghdr));
  struct nn_iovec iov;

  packet_hdr_t packet_hdr;
  packet_hdr.port = port_num;
  packet_hdr.len = len;

  // not sure I can do better than this here
  void *msg = nn::allocmsg(sizeof(packet_hdr) + len, 0);
  std::memcpy(msg, &packet_hdr, sizeof(packet_hdr));
  std::memcpy(static_cast<char *>(msg) + sizeof(packet_hdr), buffer, len);
  iov.iov_base = &msg;
  iov.iov_len = NN_MSG;

  msghdr.msg_iov = &iov;
  msghdr.msg_iovlen = 1;

  s.sendmsg(&msghdr, 0);
  std::cout << "packet send for port " << port_num << std::endl;
}

void
PacketInDevMgrImplementation::receive_loop() {
  struct nn_msghdr msghdr;
  struct nn_iovec iov;
  packet_hdr_t packet_hdr;
  void *msg = nullptr;
  iov.iov_base = &msg;
  iov.iov_len = NN_MSG;
  std::memset(&msghdr, 0, sizeof(msghdr));
  msghdr.msg_iov = &iov;
  msghdr.msg_iovlen = 1;
  while (!stop_receive_thread) {
    int rc = s.recvmsg(&msghdr, 0);
    if (rc < 0) continue;
    assert(msg);
    if (pkt_handler) {
      std::memcpy(&packet_hdr, msg, sizeof(packet_hdr));
      char *data = static_cast<char *>(msg) + sizeof(packet_hdr);
      std::cout << "packet in received on port " << packet_hdr.port
                << std::endl;
      pkt_handler(packet_hdr.port, data, packet_hdr.len, pkt_cookie);
    }
    nn::freemsg(msg);
    msg = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////

DevMgr::DevMgr() : impl(nullptr) {}

void
DevMgr::set_dev_mgr(std::unique_ptr<DevMgrInterface> my_impl) {
  assert(!impl);
  impl = std::move(my_impl);
}

void
DevMgr::set_dev_mgr_files(unsigned wait_time_in_seconds) {
  assert(!impl);
  impl = std::unique_ptr<DevMgrInterface>(new FilesDevMgrImplementation(
      false /* no real-time packet replay */, wait_time_in_seconds));
}

void
DevMgr::set_dev_mgr_packet_in(const std::string &addr) {
  assert(!impl);
  (void) addr;
  impl = std::unique_ptr<DevMgrInterface>(
      new PacketInDevMgrImplementation(addr));
}

void
DevMgr::start() {
  assert(impl);
  impl->start();
}

PacketDispatcherInterface::ReturnCode
DevMgr::port_add(const std::string &iface_name, port_t port_num,
                 const char *pcap_in, const char *pcap_out) {
  // TODO(antonin): check if port is taken...
  assert(impl);
  BMLOG_DEBUG("Adding interface {} as port {}", iface_name, port_num);
  ReturnCode rc = impl->port_add(iface_name, port_num, pcap_in, pcap_out);
  if (rc != ReturnCode::SUCCESS)
    Logger::get()->error("Add port operation failed");
  return rc;
}

void
DevMgr::transmit_fn(int port_num, const char *buffer, int len) {
  assert(impl);
  impl->transmit_fn(port_num, buffer, len);
}

PacketDispatcherInterface::ReturnCode
DevMgr::port_remove(port_t port_num) {
  assert(impl);
  BMLOG_DEBUG("Removing port {}", port_num);
  ReturnCode rc = impl->port_remove(port_num);
  if (rc != ReturnCode::SUCCESS)
    Logger::get()->error("Remove port operation failed");
  return rc;
}

PacketDispatcherInterface::ReturnCode
DevMgr::set_packet_handler(const PacketHandler &handler, void *cookie) {
  assert(impl);
  return impl->set_packet_handler(handler, cookie);
}

PacketDispatcherInterface::ReturnCode
DevMgr::register_status_cb(const PortStatus &type,
                           const PortStatusCB &port_cb) {
  assert(impl);
  return impl->register_status_cb(type, port_cb);
}

bool
DevMgr::port_is_up(port_t port_num) {
  assert(impl);
  return impl->port_is_up(port_num);
}
