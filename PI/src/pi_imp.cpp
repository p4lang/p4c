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

#include <bm/bm_sim/_assert.h>
#include <bm/bm_sim/switch.h>

#include <PI/int/pi_int.h>
#include <PI/pi.h>
#include <PI/target/pi_imp.h>

#include <iostream>
#include <string>

#include <cstring>  // for memset
#include "common.h"
#include "device_state.h"

namespace {

bm::PortMonitorIface::PortStatusCb port_status_cb;

void register_port_status_cb(pi_dev_id_t dev_id) {
  auto cb = [dev_id](bm::PortMonitorIface::port_t port,
                     bm::PortMonitorIface::PortStatus status) {
    auto port_ = static_cast<pi_port_t>(port);
    pi_port_status_t status_;
    if (status == bm::PortMonitorIface::PortStatus::PORT_UP) {
      status_ = PI_PORT_STATUS_UP;
    } else if (status == bm::PortMonitorIface::PortStatus::PORT_DOWN) {
      status_ = PI_PORT_STATUS_DOWN;
    } else {
      // TODO(antonin): log error?
      return;
    }
    pi_port_status_event_notify(dev_id, port_, status_);
  };

  using PortStatus = bm::PortMonitorIface::PortStatus;
  port_status_cb = cb;
  pibmv2::switch_->register_status_cb(PortStatus::PORT_UP, port_status_cb);
  pibmv2::switch_->register_status_cb(PortStatus::PORT_DOWN, port_status_cb);
}

}  // namespace

extern "C" {

pi_status_t _pi_init(int *abi_version, void *extra) {
  _BM_UNUSED(extra);
  pibmv2::device_lock = new pibmv2::DeviceLock();
  *abi_version = PI_ABI_VERSION;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_assign_device(pi_dev_id_t dev_id, const pi_p4info_t *p4info,
                              pi_assign_extra_t *extra) {
  _BM_UNUSED(p4info);
  _BM_UNUSED(extra);
  // check that the device id is correct, i.e. matches the one used when
  // starting the bmv2 process (--device-id command-line parameter)
  if (pibmv2::switch_->get_device_id() != dev_id) {
    // TODO(antonin): define better error code
    return PI_STATUS_DEV_OUT_OF_RANGE;
  }
  // register callback for port oper status updates
  // note that bmv2 currently does not offer the ability to unregister the
  // callback...
  register_port_status_cb(dev_id);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_update_device_start(pi_dev_id_t dev_id,
                                    const pi_p4info_t *p4info,
                                    const char *device_data,
                                    size_t device_data_size) {
  _BM_UNUSED(dev_id);
  _BM_UNUSED(p4info);
  auto lock = pibmv2::device_lock->unique_lock();
  auto error_code = pibmv2::switch_->load_new_config(
      std::string(device_data, device_data_size));
  if (error_code != bm::RuntimeInterface::ErrorCode::SUCCESS)
    return PI_STATUS_TARGET_ERROR;
  if (pibmv2::device_state != nullptr) delete pibmv2::device_state;
  pibmv2::device_state = new pibmv2::DeviceState(p4info);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_update_device_end(pi_dev_id_t dev_id) {
  _BM_UNUSED(dev_id);
  auto error_code = pibmv2::switch_->swap_configs();
  if (error_code != bm::RuntimeInterface::ErrorCode::SUCCESS)
    return PI_STATUS_TARGET_ERROR;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_remove_device(pi_dev_id_t dev_id) {
  _BM_UNUSED(dev_id);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_destroy() {
  if (pibmv2::device_state != nullptr) delete pibmv2::device_state;
  delete pibmv2::device_lock;
  return PI_STATUS_SUCCESS;
}

// bmv2 does not support transaction and has no use for the session_handle
pi_status_t _pi_session_init(pi_session_handle_t *session_handle) {
  static pi_session_handle_t sess = 0;
  *session_handle = sess++;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_session_cleanup(pi_session_handle_t session_handle) {
  _BM_UNUSED(session_handle);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_batch_begin(pi_session_handle_t session_handle) {
  _BM_UNUSED(session_handle);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_batch_end(pi_session_handle_t session_handle, bool hw_sync) {
  _BM_UNUSED(session_handle);
  _BM_UNUSED(hw_sync);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_packetout_send(pi_dev_id_t dev_id, const char *pkt,
                               size_t size) {
  _BM_UNUSED(dev_id);
  if (pibmv2::cpu_port == 0) return PI_STATUS_PACKETOUT_SEND_ERROR;
  pibmv2::switch_->receive(pibmv2::cpu_port, pkt, static_cast<int>(size));
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_port_status_get(pi_dev_id_t dev_id, pi_port_t port,
                                pi_port_status_t *status) {
  _BM_UNUSED(dev_id);
  auto is_up = pibmv2::switch_->port_is_up(
      static_cast<bm::PortMonitorIface::port_t>(port));
  *status = is_up ? PI_PORT_STATUS_UP : PI_PORT_STATUS_DOWN;
  return PI_STATUS_SUCCESS;
}

}
