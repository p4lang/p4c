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

#include <mutex>
#include <thread>
#include <unordered_map>
#include <map>
#include <atomic>
#include <utility>  // for pair<>

#include "bm_sim/port_monitor.h"
#include "bm_sim/logger.h"

class PortMonitorDummy : public PortMonitorIface {
 private:
  void notify_(port_t port_num, const PortStatus evt) override {
    (void) port_num;
    (void) evt;
  }

  void register_cb_(const PortStatus evt, const PortStatusCb &cb) override {
    (void) evt;
    (void) cb;
  }

  void start_(const PortStatusFn &fn) override {
    (void) fn;
  }

  void stop_() override { }
};

class PortMonitorPassive : public PortMonitorIface {
 protected:
  // call with cb_map_mutex locked
  void event_handler(port_t port, const PortStatus type) {
    std::lock_guard<std::mutex> lock(cb_map_mutex);
    auto range = cb_map.equal_range(static_cast<unsigned int>(type));
    for (auto e = range.first; e != range.second; ++e) {
      e->second(port, type);
    }
  }

  std::unordered_multimap<unsigned int, const PortStatusCb &> cb_map{};
  mutable std::mutex cb_map_mutex;
  std::unordered_map<port_t, bool> curr_ports;
  mutable std::mutex port_mutex;

 private:
  void notify_(port_t port_num, const PortStatus evt) override {
    {
      std::lock_guard<std::mutex> lock(port_mutex);
      auto it = curr_ports.find(port_num);
      if (evt == PortStatus::PORT_ADDED && it == curr_ports.end()) {
        curr_ports.insert(std::make_pair(port_num, false));
      } else if (evt == PortStatus::PORT_REMOVED && it != curr_ports.end()) {
        curr_ports.erase(it);
      } else if (evt == PortStatus::PORT_UP &&
                 it != curr_ports.end() && it->second == false) {
        it->second = true;
      } else if (evt == PortStatus::PORT_DOWN &&
                 it != curr_ports.end() && it->second == true) {
        it->second = false;
      } else {
        Logger::get()->warn("Invalid event received by port monitor");
        return;
      }
    }
    event_handler(port_num, evt);
  }

  void register_cb_(const PortStatus evt, const PortStatusCb &cb) override {
    std::lock_guard<std::mutex> lock(cb_map_mutex);
    // cannot use make_pair because of the reference
    cb_map.insert(std::pair<unsigned int, const PortStatusCb &>(
        static_cast<unsigned int>(evt), cb));
  }

  void start_(const PortStatusFn &fn) override {
    // fn ignored for passive monitor
    (void) fn;
  }

  void stop_() override { }
};

class PortMonitorActive : public PortMonitorPassive {
 private:
  ~PortMonitorActive() {
    stop_();
  }

  // same as PortMonitorPassive
  // void notify_(port_t port_num, const PortStatus evt) override {
  //   PortMonitorPassive::notify_(port_num, evt);
  // }

  // void register_cb_(const PortStatus evt, const PortStatusCb &cb) override {
  //   PortMonitorPassive::notify_(evt, cb);
  // }

  void start_(const PortStatusFn &fn) override {
    p_status_fn = fn;
    run_monitor = true;
    p_monitor = std::thread(&PortMonitorActive::monitor, this);
  }

  void stop_() override {
    if (!run_monitor) return;

    run_monitor = false;

    if (p_monitor.joinable())
      p_monitor.join();
  }

  void monitor() {
    std::map<port_t, PortStatus> cbs_todo;
    while (run_monitor) {
      std::this_thread::sleep_for(std::chrono::milliseconds(ms_sleep));
      {
        std::lock_guard<std::mutex> lock(port_mutex);
        for (auto &port_e : curr_ports) {
          bool is_up = p_status_fn(port_e.first);
          if (!is_up && port_e.second) {
            cbs_todo.insert(
                std::make_pair(port_e.first, PortStatus::PORT_DOWN));
          } else if (is_up && !(port_e.second)) {
            cbs_todo.insert(
                std::make_pair(port_e.first, PortStatus::PORT_UP));
          }
          port_e.second = is_up;
        }
      }
      // callbacks, without the port lock
      for (const auto &cb_todo : cbs_todo) {
        event_handler(cb_todo.first, cb_todo.second);
      }
      cbs_todo.clear();
    }
  }

  uint32_t ms_sleep{200};
  std::atomic<bool> run_monitor{false};
  std::thread p_monitor{};
  PortStatusFn p_status_fn{};
};

std::unique_ptr<PortMonitorIface>
PortMonitorIface::make_dummy() {
  return std::unique_ptr<PortMonitorIface>(new PortMonitorDummy());
}

std::unique_ptr<PortMonitorIface>
PortMonitorIface::make_passive() {
  return std::unique_ptr<PortMonitorIface>(new PortMonitorPassive());
}

std::unique_ptr<PortMonitorIface>
PortMonitorIface::make_active() {
  return std::unique_ptr<PortMonitorIface>(new PortMonitorActive());
}
