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
#include <bm/bm_sim/nn.h>

#include <nanomsg/pubsub.h>

#include <iostream>
#include <memory>
#include <string>

namespace bm {

class TransportNanomsg : public TransportIface {
 public:
  explicit TransportNanomsg(const std::string &addr)
      : addr(addr), s(AF_SP, NN_PUB) { }

 private:
  int open_() override {
    // TODO(antonin): catch exception
    s.bind(addr.c_str());
    // TODO(antonin): review if linger is actually needed
    int linger = 0;
    s.setsockopt(NN_SOL_SOCKET, NN_LINGER, &linger, sizeof (linger));
    return 0;
  }

  int send_(const std::string &msg) const override {
    s.send(msg.data(), msg.size(), 0);
    return 0;
  }

  int send_(const char *msg, int len) const override {
    s.send(msg, len, 0);
    return 0;
  }

  // do not use this function, I think I should just remove it...
  int send_msgs_(const std::initializer_list<std::string> &msgs)
      const override {
    size_t num_msgs = msgs.size();
    if (num_msgs == 0) return 0;
    struct nn_msghdr msghdr;
    std::memset(&msghdr, 0, sizeof(msghdr));
    struct nn_iovec iov[num_msgs];
    int i = 0;
    for (const auto &msg : msgs) {
      // wish I could write this, but string.data() return const char *
      // iov[i].iov_base = msg.data();
      std::unique_ptr<char[]> data(new char[msg.size()]);
      msg.copy(data.get(), msg.size());
      iov[i].iov_base = data.get();
      iov[i].iov_len = msg.size();
      i++;
    }
    msghdr.msg_iov = iov;
    msghdr.msg_iovlen = num_msgs;
    s.sendmsg(&msghdr, 0);
    return 0;
  }

  int send_msgs_(const std::initializer_list<MsgBuf> &msgs) const override {
    size_t num_msgs = msgs.size();
    if (num_msgs == 0) return 0;
    struct nn_msghdr msghdr;
    std::memset(&msghdr, 0, sizeof(msghdr));
    struct nn_iovec iov[num_msgs];
    int i = 0;
    for (const auto &msg : msgs) {
      iov[i].iov_base = msg.buf;
      iov[i].iov_len = msg.len;
      i++;
    }
    msghdr.msg_iov = iov;
    msghdr.msg_iovlen = num_msgs;
    s.sendmsg(&msghdr, 0);
    return 0;
  }

 private:
  std::string addr;
  nn::socket s;
};

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
TransportIface::make_nanomsg(const std::string &addr) {
  return std::unique_ptr<TransportIface>(new TransportNanomsg(addr));
}

std::unique_ptr<TransportIface>
TransportIface::make_dummy() {
  return std::unique_ptr<TransportDummy>(new TransportDummy());
}

std::unique_ptr<TransportIface>
TransportIface::make_stdout() {
  return std::unique_ptr<TransportStdout>(new TransportStdout());
}

}  // namespace bm
