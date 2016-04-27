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

#include <bm/bm_apps/packet_pipe.h>

#include <nanomsg/pair.h>

#include <string>
#include <thread>
#include <mutex>
#include <memory>
#include <iostream>

#include "nn.h"

namespace bm_apps {

namespace {

struct packet_hdr_t {
  int type;
  int port;
  int more;
} __attribute__((packed));

}  // namespace

class PacketInjectImp final {
  typedef PacketInject::PacketReceiveCb PacketReceiveCb;

 public:
  explicit PacketInjectImp(const std::string &addr)
      : addr(addr), s(AF_SP, NN_PAIR) {
    s.connect(addr.c_str());
    int rcv_timeout_ms = 100;
    s.setsockopt(NN_SOL_SOCKET, NN_RCVTIMEO,
                 &rcv_timeout_ms, sizeof(rcv_timeout_ms));
  }

  void start() {
    if (started || stop_receive_thread)
      return;
    receive_thread = std::thread(&PacketInjectImp::receive_loop, this);
    started = true;
  }

  void stop() {
    {
      std::unique_lock<std::mutex> lock(mutex);
      stop_receive_thread = true;
    }
    receive_thread.join();
  }

  void set_packet_receiver(const PacketReceiveCb &cb, void *cookie) {
    std::unique_lock<std::mutex> lock(mutex);
    cb_fn = cb;
    cb_cookie = cookie;
  }

  void send(int port_num, const char *buffer, int len) {
    struct nn_msghdr msghdr;
    std::memset(&msghdr, 0, sizeof(msghdr));
    struct nn_iovec iov;

    packet_hdr_t packet_hdr;
    packet_hdr.type = MSG_TYPE_PACKET_IN;
    packet_hdr.port = port_num;
    packet_hdr.more = len;

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

  // these 4 port_* functions are optional, depending on receiver configuration
  void port_add(int port_num) {
    send_port_msg(MSG_TYPE_PORT_ADD, port_num, 0);
  }

  void port_remove(int port_num) {
    send_port_msg(MSG_TYPE_PORT_REMOVE, port_num, 0);
  }

  void port_bring_up(int port_num) {
    send_port_msg(MSG_TYPE_PORT_SET_STATUS, port_num, MSG_PORT_STATUS_UP);
  }

  void port_bring_down(int port_num) {
    send_port_msg(MSG_TYPE_PORT_SET_STATUS, port_num, MSG_PORT_STATUS_DOWN);
  }

 private:
  enum MsgType {
    MSG_TYPE_PORT_ADD = 0,
    MSG_TYPE_PORT_REMOVE,
    MSG_TYPE_PORT_SET_STATUS,
    MSG_TYPE_PACKET_IN,
    MSG_TYPE_PACKET_OUT
  };

  enum MsgPortStatus {
    MSG_PORT_STATUS_DOWN = 0,
    MSG_PORT_STATUS_UP
  };

  void receive_loop();

  void send_port_msg(MsgType type, int port_num, int more) {
    struct nn_msghdr msghdr;
    std::memset(&msghdr, 0, sizeof(msghdr));
    struct nn_iovec iov;

    packet_hdr_t packet_hdr;
    packet_hdr.type = type;
    packet_hdr.port = port_num;
    packet_hdr.more = more;

    iov.iov_base = static_cast<void *>(&packet_hdr);
    iov.iov_len = sizeof(packet_hdr);
    msghdr.msg_iov = &iov;
    msghdr.msg_iovlen = 1;

    s.sendmsg(&msghdr, 0);
  }

 private:
  std::string addr;
  nn::socket s;

  PacketReceiveCb cb_fn{};
  void *cb_cookie{nullptr};
  std::thread receive_thread{};
  bool stop_receive_thread{false};
  bool started{false};
  mutable std::mutex mutex{};
};

void
PacketInjectImp::receive_loop() {
  struct nn_msghdr msghdr;
  struct nn_iovec iov;
  packet_hdr_t packet_hdr;
  void *msg = nullptr;
  iov.iov_base = &msg;
  iov.iov_len = NN_MSG;
  std::memset(&msghdr, 0, sizeof(msghdr));
  msghdr.msg_iov = &iov;
  msghdr.msg_iovlen = 1;
  while (true) {
    if (s.recvmsg(&msghdr, 0) <= 0) {
      std::unique_lock<std::mutex> lock(mutex);
      if (stop_receive_thread) return;
      continue;
    }
    assert(msg);

    // I choose to make copies instead of holding the lock for the callback
    PacketReceiveCb cb_fn_;
    void *cb_cookie_;
    {
      std::unique_lock<std::mutex> lock(mutex);
      // I don't believe this is expensive
      cb_fn_ = cb_fn;
      cb_cookie_ = cb_cookie;
    }

    if (cb_fn_) {
      std::memcpy(&packet_hdr, msg, sizeof(packet_hdr));
      // others are ignored
      if (packet_hdr.type == MSG_TYPE_PACKET_OUT) {
        char *data = static_cast<char *>(msg) + sizeof(packet_hdr);
        std::cout << "packet in received on port " << packet_hdr.port
                  << std::endl;
        cb_fn_(packet_hdr.port, data, packet_hdr.more, cb_cookie_);
      }
    }
    nn::freemsg(msg);
    msg = nullptr;
  }
}

PacketInject::PacketInject(const std::string &addr):
    pimp(new PacketInjectImp(addr)) { }

PacketInject::~PacketInject() {
  pimp->stop();
}

void
PacketInject::start() {
  pimp->start();
}

void
PacketInject::set_packet_receiver(const PacketReceiveCb &cb, void *cookie) {
  pimp->set_packet_receiver(cb, cookie);
}

void
PacketInject::send(int port_num, const char *buffer, int len) {
  pimp->send(port_num, buffer, len);
}

void
PacketInject::port_add(int port_num) {
  pimp->port_add(port_num);
}

void
PacketInject::port_remove(int port_num) {
  pimp->port_remove(port_num);
}

void
PacketInject::port_bring_up(int port_num) {
  pimp->port_bring_up(port_num);
}

void
PacketInject::port_bring_down(int port_num) {
  pimp->port_bring_down(port_num);
}

}  // namespace bm_apps
