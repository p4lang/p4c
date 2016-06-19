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

#include <bm/bm_sim/nn.h>
#include <bm/pdfixed/int/pd_notifications.h>

#include <nanomsg/pubsub.h>

#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <type_traits>
#include <functional>
#include <cstring>

#define NUM_DEVICES 256

namespace {

class NotificationsListener {
 public:
  typedef std::function<void(const char *hdr, const char *data)> NotificationCb;

  NotificationsListener(int dev_id, const char *nn_addr)
      : dev_id(dev_id), s(AF_SP, NN_SUB) {
    if (nn_addr) {
      strncpy(addr, nn_addr, sizeof(addr));
      enabled = true;
    }
  }

  ~NotificationsListener() {
    {
      std::unique_lock<std::mutex> lock(mutex);
      if (!started) return;
      stop_required = true;
    }
    receiver_thread.join();
  }

  void register_ageing_cb(const NotificationCb &cb) {
    std::unique_lock<std::mutex> lock(mutex);
    age_cb = cb;
  }

  void register_learning_cb(const NotificationCb &cb) {
    std::unique_lock<std::mutex> lock(mutex);
    learn_cb = cb;
  }

  void start() {
    std::unique_lock<std::mutex> lock(mutex);
    if (!enabled) return;
    if (started || stop_required) return;
    // subscribe to all notifications
    s.setsockopt(NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
    int rcv_timeout_ms = 200;
    s.setsockopt(NN_SOL_SOCKET, NN_RCVTIMEO,
                 &rcv_timeout_ms, sizeof(rcv_timeout_ms));
    s.connect(addr);
    receiver_thread = std::thread(&NotificationsListener::receive_loop, this);
    started = true;
  }

  void receive_loop() {
    (void) dev_id;
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
          if (stop_required) return;
          continue;
      }

      const char *hdr = reinterpret_cast<char *>(&storage);
      if (!memcmp("AGE|", hdr, 4)) {
        std::cout << "Received ageing notification\n";
        if (!age_cb) {
          std::cout << "No cb registered\n";
          continue;
        }
        NotificationCb cb;
        {
          std::unique_lock<std::mutex> lock(mutex);
          cb = age_cb;
        }
        cb(hdr, data);
      } else if (!memcmp("LEA|", hdr, 4)) {
        std::cout << "Received learning notification\n";
        if (!learn_cb) {
          std::cout << "No cb registered\n";
          continue;
        }
        NotificationCb cb;
        {
          std::unique_lock<std::mutex> lock(mutex);
          cb = learn_cb;
        }
        cb(hdr, data);

      } else {
        std::cout << "Unknow notification type\n";
      }
    }
  }

 private:
  int dev_id;
  bool enabled{false};
  char addr[128];
  nn::socket s;
  bool started{false};
  bool stop_required{false};
  std::thread receiver_thread{};
  mutable std::mutex mutex{};
  NotificationCb age_cb{};
  NotificationCb learn_cb{};
};

NotificationsListener *listeners[NUM_DEVICES];

}  // namespace

int pd_notifications_add_device(int dev_id, const char *notifications_addr,
                                NotificationCb ageing_cb,
                                NotificationCb learning_cb) {
  assert(!listeners[dev_id]);
  listeners[dev_id] = new NotificationsListener(dev_id, notifications_addr);
  listeners[dev_id]->register_ageing_cb(ageing_cb);
  listeners[dev_id]->register_learning_cb(learning_cb);
  listeners[dev_id]->start();
  return 0;
}

int pd_notifications_remove_device(int dev_id) {
  assert(listeners[dev_id]);
  delete listeners[dev_id];
  listeners[dev_id] = nullptr;
  return 0;
}
