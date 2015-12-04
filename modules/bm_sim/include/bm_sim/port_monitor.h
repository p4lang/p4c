#ifndef __PORT_MONITOR_H
#define __PORT_MONITOR_H
#include "dev_mgr.h"
#include <mutex>
#include <thread>
#include <unordered_map>

class PortMonitor {
public:
  PortMonitor();
  void notify(DevMgrInterface::port_t port_num,
              const DevMgrInterface::PortStatus &evt);
  void register_cb(const DevMgrInterface::PortStatus &evt,
                   const DevMgrInterface::PortStatusCB &cb);
  void start(DevMgrInterface *dev_mgr);
  void stop(void);
  ~PortMonitor();

private:
  void port_handler(DevMgrInterface::port_t port,
                    const DevMgrInterface::PortStatus &evt);
  void monitor(void);

  bool run_monitor;
  std::unique_ptr<std::thread> p_monitor;
  std::unordered_multimap<unsigned int, const DevMgrInterface::PortStatusCB &>
      cb_map;
  std::unordered_map<DevMgrInterface::port_t, bool> curr_ports;
  std::mutex cb_map_mutex;
  std::mutex port_mutex;
  DevMgrInterface *peer;
};
#endif
