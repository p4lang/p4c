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
 * Hari Thantry (thantry@barefootnetworks.com)
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#ifndef BM_SIM_INCLUDE_BM_SIM_PORT_MONITOR_H_
#define BM_SIM_INCLUDE_BM_SIM_PORT_MONITOR_H_

#include <memory>

class PortMonitorIface {
 public:
  typedef unsigned int port_t;

  enum class PortStatus { PORT_ADDED, PORT_REMOVED, PORT_UP, PORT_DOWN };

  typedef std::function<void(port_t port_num, const PortStatus status)>
      PortStatusCb;

  typedef std::function<bool(port_t port_num)> PortStatusFn;

  void notify(port_t port_num, const PortStatus evt) {
    notify_(port_num, evt);
  }

  void register_cb(const PortStatus evt, const PortStatusCb &cb) {
    register_cb_(evt, cb);
  }

  void start(const PortStatusFn &fn) {
    start_(fn);
  }

  void stop() {
    stop_();
  }

  virtual ~PortMonitorIface() { }

  // all calls are no-op
  static std::unique_ptr<PortMonitorIface> make_dummy();
  // a passive monitor does not periodically query the port status
  static std::unique_ptr<PortMonitorIface> make_passive();
  // an active monitor periodically queries the port status (using separate
  // thread)
  static std::unique_ptr<PortMonitorIface> make_active();

 private:
  virtual void notify_(port_t port_num, const PortStatus evt) = 0;

  virtual void register_cb_(const PortStatus evt, const PortStatusCb &cb) = 0;

  virtual void start_(const PortStatusFn &fn) = 0;

  virtual void stop_() = 0;
};

#endif  // BM_SIM_INCLUDE_BM_SIM_PORT_MONITOR_H_
