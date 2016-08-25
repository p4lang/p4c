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

#ifdef P4THRIFT
#include <p4thrift/protocol/TBinaryProtocol.h>
#include <p4thrift/transport/TSocket.h>
#include <p4thrift/transport/TTransportUtils.h>
#include <p4thrift/protocol/TMultiplexedProtocol.h>

namespace thrift_provider = p4::thrift;
#else
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/protocol/TMultiplexedProtocol.h>

namespace thrift_provider = apache::thrift;
#endif

#include <bm/bm_apps/learn.h>
#include <bm/Standard.h>
#include <bm/SimplePre.h>

#include <nanomsg/pubsub.h>

#include <iostream>
#include <string>

#include <cassert>

#include "nn.h"

namespace bm_apps {

namespace runtime = bm_runtime::standard;

namespace {

typedef struct {
  char sub_topic[4];
  int switch_id;
  int cxt_id;
  int list_id;
  uint64_t buffer_id;
  unsigned int num_samples;
  char _padding[4];
} __attribute__((packed)) learn_hdr_t;

}  // namespace

LearnListener::LearnListener(const std::string &learn_socket,
                             const std::string &thrift_addr,
                             const int thrift_port)
  : socket_name(learn_socket),
    thrift_addr(thrift_addr), thrift_port(thrift_port) {
  (void) this->thrift_port;  // clang unused private field warning
}

LearnListener::~LearnListener() {
  {
    std::unique_lock<std::mutex> lock(mutex);
    stop_listen_thread = true;
  }
  listen_thread.join();
}

void
LearnListener::register_cb(const LearnCb &cb, void *cookie) {
  std::unique_lock<std::mutex> lock(mutex);
  cb_fn = cb;
  cb_cookie = cookie;
}

void
LearnListener::ack_buffer(cxt_id_t cxt_id, list_id_t list_id,
                          buffer_id_t buffer_id) {
  assert(bm_client);
  bm_client->bm_learning_ack_buffer(cxt_id, list_id, buffer_id);
}

void
LearnListener::start() {
  using thrift_provider::protocol::TProtocol;
  using thrift_provider::protocol::TBinaryProtocol;
  using thrift_provider::protocol::TMultiplexedProtocol;
  using thrift_provider::transport::TSocket;
  using thrift_provider::transport::TTransport;
  using thrift_provider::transport::TBufferedTransport;

  boost::shared_ptr<TTransport> tsocket(new TSocket("localhost", 9090));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(tsocket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

  boost::shared_ptr<TMultiplexedProtocol> standard_protocol(
      new TMultiplexedProtocol(protocol, "standard"));

  bm_client = boost::shared_ptr<runtime::StandardClient>(
      new runtime::StandardClient(standard_protocol));

  transport->open();

  listen_thread = std::thread(&LearnListener::listen_loop, this);
}

void
LearnListener::listen_loop() {
  nn::socket s(AF_SP, NN_SUB);
  // only subscribe to learn notifications
  s.setsockopt(NN_SUB, NN_SUB_SUBSCRIBE, "LEA", 3);
  int to = 200;
  s.setsockopt(NN_SOL_SOCKET, NN_RCVTIMEO, &to, sizeof(to));
  s.connect(socket_name.c_str());

  struct nn_msghdr msghdr;
  struct nn_iovec iov[2];
  learn_hdr_t learn_hdr;
  char data[4096];
  iov[0].iov_base = &learn_hdr;
  iov[0].iov_len = sizeof(learn_hdr);
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

    LearnCb cb_fn_;
    void *cb_cookie_;
    std::cout << "I received " << learn_hdr.num_samples << " samples\n";

    {
      std::unique_lock<std::mutex> lock(mutex);
      // I don't believe this is expensive
      cb_fn_ = cb_fn;
      cb_cookie_ = cb_cookie;
    }

    if (!cb_fn_) {
      std::cout << "No callback\n";
      continue;
    }

    std::cout << "Calling callback function\n";
    MsgInfo info = {learn_hdr.switch_id, learn_hdr.cxt_id, learn_hdr.list_id,
                    learn_hdr.buffer_id, learn_hdr.num_samples};
    cb_fn_(info, data, cb_cookie_);
  }
}

}  // namespace bm_apps
