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

#ifndef BM_BM_APPS_LEARN_H_
#define BM_BM_APPS_LEARN_H_

#include <boost/shared_ptr.hpp>

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>

namespace bm_runtime {
namespace standard {

class StandardClient;

}
}

namespace bm_apps {

class LearnListener {
 public:
  typedef uint64_t buffer_id_t;
  typedef int switch_id_t;
  typedef int cxt_id_t;
  typedef int list_id_t;

  struct MsgInfo {
    switch_id_t switch_id;
    cxt_id_t cxt_id;
    list_id_t list_id;
    buffer_id_t buffer_id;
    unsigned int num_samples;
  };

  typedef std::function<void(const MsgInfo &msg_info,
                             const char *, void *)> LearnCb;

 public:
  LearnListener(
    const std::string &learn_socket = "ipc:///tmp/bmv2-0-notifications.ipc",
    const std::string &thrift_addr = "localhost",
    const int thrift_port = 9090);

  ~LearnListener();

  void start();

  void register_cb(const LearnCb &cb, void *cookie);

  void ack_buffer(cxt_id_t cxt_id, list_id_t list_id, buffer_id_t buffer_id);

  boost::shared_ptr<bm_runtime::standard::StandardClient> get_client() {
    return bm_client;
  }

 private:
  void listen_loop();

 private:
  std::string socket_name{};
  std::string thrift_addr{};
  int thrift_port{};
  LearnCb cb_fn{};
  void *cb_cookie{nullptr};
  std::thread listen_thread{};
  bool stop_listen_thread{false};
  mutable std::mutex mutex{};
  boost::shared_ptr<bm_runtime::standard::StandardClient> bm_client{nullptr};
};

}  // namespace bm_apps

#endif  // BM_BM_APPS_LEARN_H_
