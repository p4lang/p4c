#include "bm_sim/port_monitor.h"

PortMonitor::PortMonitor(uint32_t msec_dur)
    : ms_sleep{msec_dur}, run_monitor{false}, p_monitor{nullptr}, peer{nullptr} {}

PortMonitor::~PortMonitor() { stop(); }

void PortMonitor::start(DevMgrInterface *parent) {
  assert(parent != nullptr);
  peer = parent;
  run_monitor = true;
  p_monitor = std::unique_ptr<std::thread>(
      new std::thread(&PortMonitor::monitor, this));
}

void PortMonitor::stop(void) {
  run_monitor = false;

  if (p_monitor && p_monitor->joinable()) {
    p_monitor->join();
    p_monitor = nullptr;
    peer = nullptr;
  }
}

void PortMonitor::notify(DevMgrInterface::port_t port_num,
                         const DevMgrInterface::PortStatus &event) {
  std::lock_guard<std::mutex> lock(port_mutex);
  if (event == DevMgrInterface::PortStatus::PORT_ADDED) {
    curr_ports.insert(std::make_pair(port_num, false));
  } else if (event == DevMgrInterface::PortStatus::PORT_REMOVED) {
    curr_ports.erase(port_num);
  }
  port_handler(port_num, event);
}
void PortMonitor::register_cb(const DevMgrInterface::PortStatus &cb_type,
                              const DevMgrInterface::PortStatusCB &port_cb) {
  {
    std::lock_guard<std::mutex> lock(cb_map_mutex);
    cb_map.insert(
        std::pair<unsigned int, const DevMgrInterface::PortStatusCB &>(
            static_cast<unsigned int>(cb_type), port_cb));
  }
}

/*
 * Private functions
 */

void PortMonitor::port_handler(DevMgrInterface::port_t port,
                               const DevMgrInterface::PortStatus &type) {
  auto range = cb_map.equal_range(static_cast<unsigned int>(type));
  for (auto elem = range.first; elem != range.second; ++elem) {
    elem->second(port, type);
  }
}

void PortMonitor::monitor(void) {
  bool is_up = false;
  while (run_monitor) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms_sleep));
    {
      std::lock_guard<std::mutex> lock(port_mutex);
      for (auto port_elem : curr_ports) {
        is_up = peer->port_is_up(port_elem.first);
        if (!is_up && port_elem.second) {
          port_handler(port_elem.first, DevMgrInterface::PortStatus::PORT_DOWN);
        } else if (is_up && !(port_elem.second)) {
          port_handler(port_elem.first, DevMgrInterface::PortStatus::PORT_UP);
        }
        curr_ports[port_elem.first] = is_up;
      }
    }
  }
}
