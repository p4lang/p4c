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

#include <bm/bm_sim/transport.h>

#include <iostream>
#include <memory>
#include <string>

namespace bm {

// TODO(antonin): remove this?
class TransportStdout : public TransportIface {
 public:
  TransportStdout() { }

 private:
  int open_() override {
    return 0;
  }

  int send_(const std::string &msg) const override {
    std::cout << msg << std::endl;
    return 0;
  }

  int send_(const char *msg, int len) const override {
    (void) len;  // compiler warning
    std::cout << msg << std::endl;
    return 0;
  }

  int send_msgs_(const std::initializer_list<std::string> &msgs)
      const override {
    for (const auto &msg : msgs) {
      send(msg);
    }
    return 0;
  }

  int send_msgs_(const std::initializer_list<MsgBuf> &msgs) const override {
    for (const auto &msg : msgs) {
      send(msg.buf, msg.len);
    }
    return 0;
  }
};

class TransportDummy : public TransportIface {
 public:
  TransportDummy() { }

 private:
  int open_() override {
    return 0;
  }

  int send_(const std::string &msg) const override {
    (void) msg;  // compiler warning
    return 0;
  }

  int send_(const char *msg, int len) const override {
    (void) msg;  // compiler warning
    (void) len;
    return 0;
  }

  int send_msgs_(
      const std::initializer_list<std::string> &msgs) const override {
    (void) msgs;  // compiler warning
    return 0;
  }

  int send_msgs_(
      const std::initializer_list<MsgBuf> &msgs) const override {
    (void) msgs;  // compiler warning
    return 0;
  }
};

std::unique_ptr<TransportIface>
TransportIface::make_dummy() {
  return std::unique_ptr<TransportDummy>(new TransportDummy());
}

std::unique_ptr<TransportIface>
TransportIface::make_stdout() {
  return std::unique_ptr<TransportStdout>(new TransportStdout());
}

}  // namespace bm
