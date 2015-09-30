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

#ifndef _BM_TRANSPORT_H_
#define _BM_TRANSPORT_H_

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

  virtual int open(const std::string &name) = 0;

  virtual int send(const std::string &msg) const = 0;
  virtual int send(const char *msg, int len) const = 0;

  virtual int send_msgs(const std::initializer_list<std::string> &msgs) const = 0;
  virtual int send_msgs(const std::initializer_list<MsgBuf> &msgs) const = 0;

  template <typename T>
  static std::unique_ptr<T> create_instance(const std::string &name) {
    T *transport = new T();
    transport->open(name);
    return std::unique_ptr<T>(transport);
  }
};

class TransportNanomsg : public TransportIface {
public:
  TransportNanomsg();

  int open(const std::string &name) override;

  int send(const std::string &msg) const override;
  int send(const char *msg, int len) const override;

  int send_msgs(const std::initializer_list<std::string> &msgs) const override;
  int send_msgs(const std::initializer_list<MsgBuf> &msgs) const override;

private:
  nn::socket s;
};

class TransportSTDOUT : public TransportIface {
public:
  int open(const std::string &name) override;

  int send(const std::string &msg) const override;
  int send(const char *msg, int len) const override;

  int send_msgs(const std::initializer_list<std::string> &msgs) const override;
  int send_msgs(const std::initializer_list<MsgBuf> &msgs) const override;
};

class TransportNULL : public TransportIface {
public:
  int open(const std::string &name) override {
    (void) name; // compiler warning
    return 0;
  }

  int send(const std::string &msg) const override {
    (void) msg; // compiler warning
    return 0;
  }

  int send(const char *msg, int len) const override {
    (void) msg; // compiler warning
    (void) len;
    return 0;
  }

  int send_msgs(const std::initializer_list<std::string> &msgs) const override {
    (void) msgs; // compiler warning
    return 0;
  }

  int send_msgs(const std::initializer_list<MsgBuf> &msgs) const override {
    (void) msgs; // compiler warning
    return 0;
  }
};

#endif
