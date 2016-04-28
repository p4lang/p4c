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

#include <bm/bm_sim/port_monitor.h>
#include <bm/bm_sim/logger.h>
#include <bm/bm_sim/transport.h>

#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <map>
#include <atomic>
#include <utility>  // for pair<>

namespace bm {

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
 public:
  PortMonitorPassive(int device_id,
                     std::shared_ptr<TransportIface> notifications_writer)
      : device_id(device_id), notifications_writer(notifications_writer) { }

 protected:
  void event_handler(port_t port, const PortStatus type) {
    std::lock_guard<std::mutex> lock(cb_map_mutex);
    auto range = cb_map.equal_range(static_cast<unsigned int>(type));
    for (auto e = range.first; e != range.second; ++e) {
      e->second(port, type);
    }
  }

  // originally I was sending one message per status, but this is probably less
  // likely to overrun a nanomsg subscriber
  void send_notifications(std::vector<one_status_t> *statuses) const {
    if (!notifications_writer || statuses->empty()) return;

    msg_hdr_t msg_hdr;
    char *msg_hdr_ = reinterpret_cast<char *>(&msg_hdr);
    memset(msg_hdr_, 0, sizeof(msg_hdr));
    memcpy(msg_hdr_, "PRT|", 4);
    msg_hdr.switch_id = device_id;
    msg_hdr.num_statuses = statuses->size();

    unsigned int size = static_cast<unsigned int>(
        statuses->size() * sizeof(one_status_t));
    TransportIface::MsgBuf buf_hdr = {msg_hdr_, sizeof(msg_hdr)};
    TransportIface::MsgBuf buf_statuses =
      {reinterpret_cast<char *>(statuses->data()), size};
    notifications_writer->send_msgs({buf_hdr, buf_statuses});
  }

  static one_status_t make_one_status(int port, PortStatus evt) {
    return {port, (evt == PortStatus::PORT_UP)};
  }

  std::unordered_multimap<unsigned int, const PortStatusCb &> cb_map{};
  mutable std::mutex cb_map_mutex{};
  std::unordered_map<port_t, bool> curr_ports{};
  mutable std::mutex port_mutex{};
  int device_id{};
  std::shared_ptr<TransportIface> notifications_writer{nullptr};

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

    if (evt == PortStatus::PORT_UP || evt == PortStatus::PORT_DOWN) {
      std::vector<one_status_t> v = {make_one_status(port_num, evt)};
      send_notifications(&v);
    }
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
 public:
  PortMonitorActive(int device_id,
                    std::shared_ptr<TransportIface> notifications_writer)
      : PortMonitorPassive(device_id, notifications_writer) { }

 private:
  ~PortMonitorActive() {
    stop_();
  }

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
      if (notifications_writer) {
        std::vector<one_status_t> v;
        for (const auto &cb_todo : cbs_todo) {
          if (cb_todo.second == PortStatus::PORT_UP ||
              cb_todo.second == PortStatus::PORT_DOWN) {
            v.push_back(make_one_status(cb_todo.first, cb_todo.second));
          }
        }
        send_notifications(&v);
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
PortMonitorIface::make_passive(
    int device_id, std::shared_ptr<TransportIface> notifications_writer) {
  return std::unique_ptr<PortMonitorIface>(
      new PortMonitorPassive(device_id, notifications_writer));
}

std::unique_ptr<PortMonitorIface>
PortMonitorIface::make_active(
    int device_id, std::shared_ptr<TransportIface> notifications_writer) {
  return std::unique_ptr<PortMonitorIface>(
      new PortMonitorActive(device_id, notifications_writer));
}

}  // namespace bm
