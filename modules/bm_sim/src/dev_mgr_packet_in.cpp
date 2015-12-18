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

#include <atomic>
#include <thread>
#include <mutex>
#include <string>

#include "bm_sim/dev_mgr.h"
#include "bm_sim/logger.h"
#include "bm_sim/nn.h"

// private implementation

// Implementation which uses a nanomsg PAIR socket to receive / send packets
// TODO(antonin): moved lower-level NN code to separate class ?
class PacketInDevMgrPimpementation : public DevMgrInterface {
 public:
  explicit PacketInDevMgrPimpementation(const std::string &addr)
      : addr(addr), s(AF_SP, NN_PAIR) {
    s.bind(addr.c_str());
    int rcv_timeout_ms = 100;
    s.setsockopt(NN_SOL_SOCKET, NN_RCVTIMEO,
                 &rcv_timeout_ms, sizeof(rcv_timeout_ms));
  }

  ~PacketInDevMgrPimpementation() {
    if (started) {
      stop_receive_thread = true;
      receive_thread.join();
    }
  }

  ReturnCode port_add(const std::string &iface_name, port_t port_num,
                      const char *in_pcap, const char *out_pcap) override {
    // TODO(antonin): add a special message to add ports
    (void) iface_name;
    (void) port_num;
    (void) in_pcap;
    (void) out_pcap;
    Logger::get()->warn("When using packet in, port_add is done "
                        "through IPC messages");
    return ReturnCode::UNSUPPORTED;
  }

  ReturnCode port_remove(port_t port_num) override {
    // TODO(antonin): add a special message to remove ports
    (void) port_num;
    Logger::get()->warn("When using packet in, port_remove is done "
                        "through IPC messages");
    return ReturnCode::UNSUPPORTED;
  }

  void transmit_fn(int port_num, const char *buffer, int len) override;

  void start() override {
    if (started || stop_receive_thread)
      return;
    receive_thread = std::thread(
        &PacketInDevMgrPimpementation::receive_loop, this);
    started = true;
  }

  ReturnCode set_packet_handler(const PacketHandler &handler, void *cookie)
      override {
    pkt_handler = handler;
    pkt_cookie = cookie;
    return ReturnCode::SUCCESS;
  }

  bool port_is_up(port_t port) override {
    // TODO(antonin)
    (void) port;
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
PacketInDevMgrPimpementation::transmit_fn(int port_num,
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
PacketInDevMgrPimpementation::receive_loop() {
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

void
DevMgr::set_dev_mgr_packet_in(const std::string &addr) {
  assert(!pimp);
  (void) addr;
  pimp = std::unique_ptr<DevMgrInterface>(
      new PacketInDevMgrPimpementation(addr));
  p_monitor = PortMonitorIface::make_passive();
}
