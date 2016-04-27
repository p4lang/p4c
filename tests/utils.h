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

#ifndef TESTS_UTILS_H_
#define TESTS_UTILS_H_

#include <mutex>
#include <condition_variable>

#include <bm/bm_sim/transport.h>

class MemoryAccessor : public bm::TransportIface {
 public:
  enum class Status { CAN_READ, CAN_WRITE };
 public:
  MemoryAccessor(size_t max_size)
    : max_size(max_size), status(Status::CAN_WRITE) {
    buffer_.reserve(max_size);
  }

  int read(char *dst, size_t len) const {
    len = (len > max_size) ? max_size : len;
    std::unique_lock<std::mutex> lock(mutex);
    while(status != Status::CAN_READ) {
      can_read.wait(lock);
    }
    std::copy(buffer_.begin(), buffer_.begin() + len, dst);
    buffer_.clear();
    status = Status::CAN_WRITE;
    can_write.notify_one();
    return 0;
  }

  Status check_status() {
    std::unique_lock<std::mutex> lock(mutex);
    return status;
  }

 private:
  int open_() override {
    return 0;
  }

  int send_(const char *buffer, int len) const override {
    if(len > max_size) return -1;
    std::unique_lock<std::mutex> lock(mutex);
    while(status != Status::CAN_WRITE) {
      can_write.wait(lock);
    }
    buffer_.insert(buffer_.end(), buffer, buffer + len);
    status = Status::CAN_READ;
    can_read.notify_one();
    return 0;
  }

  int send_(const std::string &msg) const override {
    // unused for learning
    assert(0);
    return 0;
  }

  int send_msgs_(const std::initializer_list<TransportIface::MsgBuf> &msgs)
      const override {
    size_t len = 0;
    for(const auto &msg : msgs) {
      len += msg.len;
    }
    if(len > max_size) return -1;
    std::unique_lock<std::mutex> lock(mutex);
    while(status != Status::CAN_WRITE) {
      can_write.wait(lock);
    }
    for(const auto &msg : msgs) {
      buffer_.insert(buffer_.end(), msg.buf, msg.buf + msg.len);
    }
    status = Status::CAN_READ;
    can_read.notify_one();
    return 0;
  }

  int send_msgs_(
      const std::initializer_list<std::string> &msgs) const override {
    // unused for learning
    assert(0);
    return 0;
  }

 private:
  // dirty trick (mutable) to make sure that send() const is override
  mutable std::vector<char> buffer_;
  size_t max_size;
  mutable Status status;
  mutable std::mutex mutex;
  mutable std::condition_variable can_write;
  mutable std::condition_variable can_read;
};

class CLIWrapperImp;

class CLIWrapper {
 public:
  CLIWrapper(int thrift_port, bool silent = false);
  ~CLIWrapper();

  void send_cmd(const std::string &cmd);

 private:
  std::unique_ptr<CLIWrapperImp> imp;
};

#endif  // TESTS_UTILS_H_
