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

#ifndef BM_BM_APPS_NOTIFICATIONS_H_
#define BM_BM_APPS_NOTIFICATIONS_H_

#include <string>
#include <functional>
#include <memory>

namespace bm_apps {

class NotificationsListenerImp;

// This class is meant to replace bm_apps::LearnListener in the long term. It
// will provide support for all types of notifications (not just
// learning). Unlike bm_apps::LearnListener it does not provide a way to ack
// learning notifications, as it does not open a Thrift connection.

class NotificationsListener {
 public:
  typedef uint64_t buffer_id_t;
  typedef int switch_id_t;
  typedef int cxt_id_t;
  typedef int list_id_t;
  typedef int table_id_t;

  struct LearnMsgInfo {
    switch_id_t switch_id;
    cxt_id_t cxt_id;
    list_id_t list_id;
    buffer_id_t buffer_id;
    unsigned int num_samples;
  };

  typedef std::function<void(const LearnMsgInfo &msg_info,
                             const char *, void *)> LearnCb;

  struct AgeingMsgInfo {
    switch_id_t switch_id;
    cxt_id_t cxt_id;
    table_id_t table_id;
    buffer_id_t buffer_id;
    unsigned int num_entries;
  };

  typedef std::function<void(const AgeingMsgInfo &msg_info,
                             const char *, void *)> AgeingCb;

  enum class PortEvent { PORT_UP, PORT_DOWN };

  typedef std::function<void(switch_id_t, int port_num, PortEvent,
                             void *)> PortEventCb;

 public:
  explicit NotificationsListener(
      const std::string &socket_name = "ipc:///tmp/bmv2-0-notifications.ipc");

  ~NotificationsListener();

  void start();

  void register_learn_cb(const LearnCb &cb, void *cookie);

  void register_ageing_cb(const AgeingCb &cb, void *cookie);

  void register_port_event_cb(const PortEventCb &cb, void *cookie);

 private:
  std::unique_ptr<NotificationsListenerImp> pimp;
};

}  // namespace bm_apps

#endif  // BM_BM_APPS_NOTIFICATIONS_H_
