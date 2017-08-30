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

#include <chrono>
#include <string>
#include <vector>
#include <algorithm>  // for std::copy, std::min

#include "utils.h"

NNEventListener::NNEventListener(const std::string &addr)
    : addr(addr), s(AF_SP, NN_SUB) {
  s.connect(addr.c_str());
  int rcv_timeout_ms = 100;
  s.setsockopt(NN_SOL_SOCKET, NN_RCVTIMEO,
               &rcv_timeout_ms, sizeof(rcv_timeout_ms));
  s.setsockopt(NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
}

NNEventListener::~NNEventListener() {
  {
    std::unique_lock<std::mutex> lock(mutex);
    if (!started) return;
    stop_receive_thread = true;
  }
  receive_thread.join();
}

void
NNEventListener::start() {
  if (started || stop_receive_thread)
    return;
  receive_thread = std::thread(&NNEventListener::receive_loop, this);
  started = true;
}

void
NNEventListener::get_and_remove_events(const std::string &pid,
                                       std::vector<NNEvent> *pevents,
                                       size_t num_events,
                                       unsigned int timeout_ms) {
  using clock = std::chrono::system_clock;
  clock::time_point tp_start = clock::now();
  clock::time_point tp_end = tp_start + std::chrono::milliseconds(timeout_ms);
  std::unique_lock<std::mutex> lock(mutex);
  while (true) {
    auto it = events.find(pid);
    if (clock::now() > tp_end) {
      if (it != events.end()) {
        *pevents = it->second;
        events.erase(it);
      }
      return;
    }
    if (it == events.end() || it->second.size() < num_events) {
      cond_new_event.wait_until(lock, tp_end);
    } else {
      *pevents = it->second;
      events.erase(it);
      return;
    }
  }
}

void
NNEventListener::receive_loop() {
  struct msg_hdr_t {
    int type;
    uint64_t switch_id;
    uint32_t cxt_id;
    uint64_t sig;
    uint64_t id;
    uint64_t copy_id;
  } __attribute__((packed));

  while (true) {
    std::aligned_storage<128>::type storage;
    msg_hdr_t *msg_hdr = reinterpret_cast<msg_hdr_t *>(&storage);
    char *buf = reinterpret_cast<char *>(&storage);

    if (s.recv(buf, sizeof(storage), 0) <= 0) {
      std::unique_lock<std::mutex> lock(mutex);
      if (stop_receive_thread) return;
      continue;
    }

    int object_id;

    switch (msg_hdr->type) {
      case TABLE_HIT:
        {
          struct msg_t : msg_hdr_t {
            int table_id;
            int entry_hdl;
          } __attribute__((packed));
          msg_t *msg = reinterpret_cast<msg_t *>(&storage);
          object_id = msg->table_id;
        }
        break;
      case TABLE_MISS:
        {
          struct msg_t : msg_hdr_t {
            int table_id;
          } __attribute__((packed));
          msg_t *msg = reinterpret_cast<msg_t *>(&storage);
          object_id = msg->table_id;
        }
        break;
      case ACTION_EXECUTE:
        {
          struct msg_t : msg_hdr_t {
            int action_id;
          } __attribute__((packed));
          msg_t *msg = reinterpret_cast<msg_t *>(&storage);
          object_id = msg->action_id;
        }
        break;
      default:
        continue;
    }

    std::string pid =
        std::to_string(msg_hdr->id) + "." + std::to_string(msg_hdr->copy_id);

    std::unique_lock<std::mutex> lock(mutex);
    events[pid].push_back({static_cast<NNEventType>(msg_hdr->type), object_id});
    cond_new_event.notify_all();
  }
}


PacketInReceiver::PacketInReceiver() { }

void
PacketInReceiver::receive(int port_num, const char *buffer, int len,
                          void *cookie) {
  (void) cookie;
  std::unique_lock<std::mutex> lock(mutex);
  while (status != Status::CAN_RECEIVE) {
    can_receive.wait(lock);
  }
  buffer_.insert(buffer_.end(), buffer, buffer + len);
  port = port_num;
  status = Status::CAN_READ;
  can_read.notify_one();
}

size_t
PacketInReceiver::read(char *dst, size_t max_size, int *recv_port) {
  std::unique_lock<std::mutex> lock(mutex);
  while (status != Status::CAN_READ) {
    can_read.wait(lock);
  }
  size_t size = std::min(max_size, buffer_.size());
  std::copy(buffer_.begin(), buffer_.begin() + size, dst);
  buffer_.clear();
  *recv_port = port;
  status = Status::CAN_RECEIVE;
  can_receive.notify_one();
  return size;
}

PacketInReceiver::Status
PacketInReceiver::check_status() {
  std::unique_lock<std::mutex> lock(mutex);
  return status;
}
