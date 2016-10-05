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

#include <bm/bm_apps/notifications.h>

#include <nanomsg/pubsub.h>

#include <string>
#include <thread>
#include <mutex>
#include <iostream>

#include "nn.h"

namespace bm_apps {

namespace {

typedef struct {
  char sub_topic[4];
  int switch_id;
  int cxt_id;
  int list_id;
  uint64_t buffer_id;
  unsigned int num_samples;
  char _padding[4];
} __attribute__((packed)) LEA_hdr_t;

typedef struct {
  char sub_topic[4];
  int switch_id;
  int cxt_id;
  uint64_t buffer_id;
  int table_id;
  unsigned int num_entries;
  char _padding[4];
} __attribute__((packed)) AGE_hdr_t;

typedef struct {
  char sub_topic[4];
  int switch_id;
  unsigned int num_statuses;
  char _padding[20];
} __attribute__((packed)) PRT_hdr_t;

}  // namespace

class NotificationsListenerImp {
 public:
  using LearnCb = NotificationsListener::LearnCb;
  using LearnMsgInfo = NotificationsListener::LearnMsgInfo;
  using AgeingCb = NotificationsListener::AgeingCb;
  using AgeingMsgInfo = NotificationsListener::AgeingMsgInfo;
  using PortEventCb = NotificationsListener::PortEventCb;
  using PortEvent = NotificationsListener::PortEvent;

  explicit NotificationsListenerImp(const std::string &socket_name)
      : socket_name(socket_name), s(AF_SP, NN_SUB) {
    // subscribe to all notifications. In theory we could just subscribe to the
    // notifications for which the client has provided a callback; we can
    // implement this later if needed.
    s.setsockopt(NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
    int to_ms = 200;
    s.setsockopt(NN_SOL_SOCKET, NN_RCVTIMEO, &to_ms, sizeof(to_ms));
    s.connect(socket_name.c_str());
  }

  ~NotificationsListenerImp() {
    {
      std::unique_lock<std::mutex> lock(mutex);
      stop_listen_thread = true;
    }
    listen_thread.join();
  }

  void start() {
    listen_thread = std::thread(&NotificationsListenerImp::listen_loop, this);
  }

  void register_learn_cb(const LearnCb &cb, void *cookie) {
    std::unique_lock<std::mutex> lock(mutex);
    LEA_cb_fn = cb;
    LEA_cb_cookie = cookie;
  }

  void register_ageing_cb(const AgeingCb &cb, void *cookie) {
    std::unique_lock<std::mutex> lock(mutex);
    AGE_cb_fn = cb;
    AGE_cb_cookie = cookie;
  }

  void register_port_event_cb(const PortEventCb &cb, void *cookie) {
    std::unique_lock<std::mutex> lock(mutex);
    PRT_cb_fn = cb;
    PRT_cb_cookie = cookie;
  }

 private:
  void listen_loop();
  void receive_AGE(const char *hdr, const char *data);
  void receive_LEA(const char *hdr, const char *data);
  void receive_PRT(const char *hdr, const char *data);

  std::string socket_name{};
  nn::socket s;

  LearnCb LEA_cb_fn{};
  void *LEA_cb_cookie{nullptr};
  AgeingCb AGE_cb_fn{};
  void *AGE_cb_cookie{nullptr};
  PortEventCb PRT_cb_fn{};
  void *PRT_cb_cookie{nullptr};

  std::thread listen_thread{};
  bool stop_listen_thread{false};
  mutable std::mutex mutex{};
};

void
NotificationsListenerImp::receive_AGE(const char *hdr, const char *data) {
  std::cout << "Received ageing notification\n";
  if (!AGE_cb_fn) {
    std::cout << "No cb registered\n";
    return;
  }
  AgeingCb cb;
  {
    std::unique_lock<std::mutex> lock(mutex);
    cb = AGE_cb_fn;
  }
  auto hdr_ = reinterpret_cast<const AGE_hdr_t *>(hdr);
  AgeingMsgInfo info = {hdr_->switch_id, hdr_->cxt_id, hdr_->table_id,
                        hdr_->buffer_id, hdr_->num_entries};
  cb(info, data, AGE_cb_cookie);
}

void
NotificationsListenerImp::receive_LEA(const char *hdr, const char *data) {
  std::cout << "Received learning notification\n";
  if (!LEA_cb_fn) {
    std::cout << "No cb registered\n";
    return;
  }
  LearnCb cb;
  {
    std::unique_lock<std::mutex> lock(mutex);
    cb = LEA_cb_fn;
  }
  auto hdr_ = reinterpret_cast<const LEA_hdr_t *>(hdr);
  LearnMsgInfo info = {hdr_->switch_id, hdr_->cxt_id, hdr_->list_id,
                       hdr_->buffer_id, hdr_->num_samples};
  cb(info, data, LEA_cb_cookie);
}

void
NotificationsListenerImp::receive_PRT(const char *hdr, const char *data) {
  std::cout << "Received port notification\n";
  if (!PRT_cb_fn) {
    std::cout << "No cb registered\n";
    return;
  }
  PortEventCb cb;
  {
    std::unique_lock<std::mutex> lock(mutex);
    cb = PRT_cb_fn;
  }
  auto hdr_ = reinterpret_cast<const PRT_hdr_t *>(hdr);

  typedef struct {
    int port;
    int status;
  } __attribute__((packed)) one_status_t;
  auto statuses = reinterpret_cast<const one_status_t *>(data);

  for (unsigned int i = 0; i < hdr_->num_statuses; i++) {
    PortEvent status = (statuses[i].status) ?
        PortEvent::PORT_UP : PortEvent::PORT_DOWN;
    cb(hdr_->switch_id, statuses[i].port, status, PRT_cb_cookie);
  }
}

void
NotificationsListenerImp::listen_loop() {
  struct nn_msghdr msghdr;
  struct nn_iovec iov[2];
  // all notification headers have size 32 bytes, padded at the end if needed
  std::aligned_storage<32>::type storage;
  char data[4096];
  iov[0].iov_base = reinterpret_cast<char *>(&storage);
  iov[0].iov_len = sizeof(storage);
  iov[1].iov_base = data;
  iov[1].iov_len = sizeof(data);  // apparently only max size needed ?
  memset(&msghdr, 0, sizeof(msghdr));
  msghdr.msg_iov = iov;
  msghdr.msg_iovlen = 2;

  while (true) {
    if (s.recvmsg(&msghdr, 0) <= 0) {
      std::unique_lock<std::mutex> lock(mutex);
      if (stop_listen_thread) return;
      continue;
    }

    const char *hdr = reinterpret_cast<char *>(&storage);
    if (!memcmp("AGE|", hdr, 4)) {
      receive_AGE(hdr, data);
    } else if (!memcmp("LEA|", hdr, 4)) {
      receive_LEA(hdr, data);
    } else if (!memcmp("PRT|", hdr, 4)) {
      receive_PRT(hdr, data);
    } else {
      std::cout << "Unknown notification type\n";
    }
  }
}

// PIMPL forwarding

NotificationsListener::NotificationsListener(const std::string &socket_name) {
  pimp = std::unique_ptr<NotificationsListenerImp>(
      new NotificationsListenerImp(socket_name));
}

NotificationsListener::~NotificationsListener() { }

void
NotificationsListener::start() {
  pimp->start();
}

void
NotificationsListener::register_learn_cb(const LearnCb &cb, void *cookie) {
  pimp->register_learn_cb(cb, cookie);
}

void
NotificationsListener::register_ageing_cb(const AgeingCb &cb, void *cookie) {
  pimp->register_ageing_cb(cb, cookie);
}

void
NotificationsListener::register_port_event_cb(const PortEventCb &cb,
                                              void *cookie) {
  pimp->register_port_event_cb(cb, cookie);
}

}  // namespace bm_apps
