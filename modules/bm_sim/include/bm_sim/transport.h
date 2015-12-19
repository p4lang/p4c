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

#ifndef BM_SIM_INCLUDE_BM_SIM_TRANSPORT_H_
#define BM_SIM_INCLUDE_BM_SIM_TRANSPORT_H_

#include <string>
#include <mutex>
#include <initializer_list>

#include "nn.h"

class TransportIface {
 public:
  struct MsgBuf {
    char *buf;
    unsigned int len;
  };

 public:
  virtual ~TransportIface() { }

  int open(const std::string &name) {
    if (opened) return 1;
    opened = true;
    return open_(name);
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

  template <typename T>
  static std::unique_ptr<T> create_instance(const std::string &name) {
    T *transport = new T();
    transport->open(name);
    return std::unique_ptr<T>(transport);
  }

 private:
  virtual int open_(const std::string &name) = 0;

  virtual int send_(const std::string &msg) const = 0;
  virtual int send_(const char *msg, int len) const = 0;

  virtual int send_msgs_(
      const std::initializer_list<std::string> &msgs) const = 0;
  virtual int send_msgs_(const std::initializer_list<MsgBuf> &msgs) const = 0;

  bool opened{false};
};

class TransportNanomsg : public TransportIface {
 public:
  TransportNanomsg();

 private:
  int open_(const std::string &name) override;

  int send_(const std::string &msg) const override;
  int send_(const char *msg, int len) const override;

  int send_msgs_(const std::initializer_list<std::string> &msgs) const override;
  int send_msgs_(const std::initializer_list<MsgBuf> &msgs) const override;

 private:
  nn::socket s;
};

class TransportSTDOUT : public TransportIface {
 public:
  TransportSTDOUT() { }

 private:
  int open_(const std::string &name) override;

  int send_(const std::string &msg) const override;
  int send_(const char *msg, int len) const override;

  int send_msgs_(const std::initializer_list<std::string> &msgs) const override;
  int send_msgs_(const std::initializer_list<MsgBuf> &msgs) const override;
};

class TransportNULL : public TransportIface {
 public:
  TransportNULL() { }

 private:
  int open_(const std::string &name) override {
    (void) name;  // compiler warning
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

#endif  // BM_SIM_INCLUDE_BM_SIM_TRANSPORT_H_
