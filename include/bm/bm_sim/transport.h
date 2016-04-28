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

#ifndef BM_BM_SIM_TRANSPORT_H_
#define BM_BM_SIM_TRANSPORT_H_

#include <string>
#include <initializer_list>
#include <memory>

namespace bm {

class TransportIface {
 public:
  struct MsgBuf {
    char *buf;
    unsigned int len;
  };

 public:
  virtual ~TransportIface() { }

  int open() {
    if (opened) return 1;
    opened = true;
    return open_();
  }

  int send(const std::string &msg) const {
    return send_(msg);
  }

  int send(const char *msg, int len) const {
    return send_(msg, len);
  }

  int send_msgs(const std::initializer_list<std::string> &msgs) const {
    return send_msgs_(msgs);
  }

  int send_msgs(const std::initializer_list<MsgBuf> &msgs) const {
    return send_msgs_(msgs);
  }

  static std::unique_ptr<TransportIface> make_nanomsg(const std::string &addr);
  static std::unique_ptr<TransportIface> make_dummy();
  static std::unique_ptr<TransportIface> make_stdout();

 private:
  virtual int open_() = 0;

  virtual int send_(const std::string &msg) const = 0;
  virtual int send_(const char *msg, int len) const = 0;

  virtual int send_msgs_(
      const std::initializer_list<std::string> &msgs) const = 0;
  virtual int send_msgs_(const std::initializer_list<MsgBuf> &msgs) const = 0;

  bool opened{false};
};

}  // namespace bm

#endif  // BM_BM_SIM_TRANSPORT_H_
