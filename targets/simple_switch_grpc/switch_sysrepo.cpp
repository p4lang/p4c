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

#include "switch_sysrepo.h"

#include <bm/bm_sim/dev_mgr.h>
#include <bm/bm_sim/logger.h>

#include <iostream>
#include <string>

extern "C" {
#include "sysrepo.h"
#include "sysrepo/values.h"
#include "sysrepo/xpath.h"
}

namespace sswitch_grpc {

namespace {

using bm::Logger;

int module_change_cb(sr_session_ctx_t *session, const char *module_name,
                     sr_notif_event_t event, void *private_ctx) {
  (void) session;
  (void) module_name;
  (void) event;
  (void) private_ctx;
  Logger::get()->debug("YANG config change");
  return SR_ERR_OK;
}

}  // namespace

constexpr const char *const SysrepoSubscriber::app_name;
constexpr const char *const SysrepoSubscriber::module_names[];

bool
SysrepoSubscriber::start() {
  int rc = SR_ERR_OK;
  rc = sr_connect(app_name, SR_CONN_DEFAULT, &connection);
  if (rc != SR_ERR_OK) {
    Logger::get()->error("Error by sr_connect: {}", sr_strerror(rc));
    return false;
  }
  rc = sr_session_start(connection, SR_DS_STARTUP, SR_SESS_DEFAULT, &session);
  if (rc != SR_ERR_OK) {
    Logger::get()->error("Error by sr_session_start: {}", sr_strerror(rc));
    return false;
  }
  // TODO(antonin): needed?
  // rc = sr_copy_config(session, module_name, SR_DS_STARTUP, SR_DS_RUNNING);
  // if (rc != SR_ERR_OK) {
  //   Logger::get()->error("Error by sr_copy_config: {}", sr_strerror(rc));
  //   return false;
  // }
  for (const auto &module_name : module_names) {
    // SR_SUBSCR_CTX_REUSE:
    // This option enables the application to re-use an already existing
    // subscription context previously returned from any sr_*_subscribe call
    // instead of requesting the creation of a new one. In that case a single
    // sr_unsubscribe call unsubscribes from all subscriptions filed within the
    // context.
    rc = sr_module_change_subscribe(
        session, module_name, module_change_cb, static_cast<void *>(this),
        0, SR_SUBSCR_CTX_REUSE, &subscription);
    if (rc != SR_ERR_OK) {
      Logger::get()->error("Error by sr_module_change_subscribe for '{}': {}",
                           module_name, sr_strerror(rc));
      return false;
    }
    Logger::get()->info("Successfully subscribed to '{}'", module_name);
  }
  Logger::get()->info("Successfully started YANG config subscriber");
  return true;
}

SysrepoSubscriber::~SysrepoSubscriber() {
  if (subscription != NULL) {
    sr_unsubscribe(session, subscription);
    subscription = NULL;
  }
  if (session != NULL) {
    sr_session_stop(session);
    session = NULL;
  }
  if (connection != NULL) {
    sr_disconnect(connection);
    connection = NULL;
  }
}

namespace {

int data_provider_cb(const char *xpath, sr_val_t **values, size_t *values_cnt,
                     void *private_ctx) {
  Logger::get()->debug("YANG oper data requested for {}", xpath);
  auto *tester = static_cast<SysrepoTest *>(private_ctx);
  tester->provide_oper_state(values, values_cnt);
  return SR_ERR_OK;
}

}  // namespace

constexpr const char *const SysrepoTest::app_name;

SysrepoTest::SysrepoTest(const bm::DevMgr *dev_mgr)
    : dev_mgr(dev_mgr) { }

bool
SysrepoTest::connect() {
  int rc = SR_ERR_OK;
  rc = sr_connect(app_name, SR_CONN_DEFAULT, &connection);
  if (rc != SR_ERR_OK) {
    Logger::get()->error("Error by sr_connect: {}", sr_strerror(rc));
    return false;
  }
  rc = sr_session_start(connection, SR_DS_RUNNING, SR_SESS_DEFAULT, &session);
  if (rc != SR_ERR_OK) {
    Logger::get()->error("Error by sr_session_start: {}", sr_strerror(rc));
    return false;
  }
  rc = sr_dp_get_items_subscribe(
      session, "/openconfig-interfaces:interfaces/interface/state",
      data_provider_cb, static_cast<void *>(this),
      SR_SUBSCR_DEFAULT, &subscription);
  if (rc != SR_ERR_OK) {
    Logger::get()->error("Error by sr_dp_get_items_subscribe: {}",
                         sr_strerror(rc));
    return false;
  }
  Logger::get()->info("Successfully started YANG oper state provider");
  return true;
}

void
SysrepoTest::refresh_ports() {
  auto port_info = dev_mgr->get_port_info();
  for (const auto &p : port_info) add_iface(p.second.iface_name);
}

void
SysrepoTest::add_iface(const std::string &name) {
  int rc = SR_ERR_OK;
  std::string prefix("/openconfig-interfaces:interfaces/interface");
  prefix.append("[name='");
  prefix.append(name);
  prefix.append("']");
  prefix.append("/config");
  {
    std::string path = prefix + "/name";
    rc = sr_set_item_str(session, path.c_str(), name.c_str(), SR_EDIT_DEFAULT);
  }
  {
    sr_val_t value = { 0 };
    value.type = SR_UINT16_T;
    value.data.uint16_val = 1500;
    std::string path = prefix + "/mtu";
    rc = sr_set_item(session, path.c_str(), &value, SR_EDIT_DEFAULT);
  }
  {
    sr_val_t value = { 0 };
    value.type = SR_IDENTITYREF_T;
    value.data.string_val = const_cast<char *>("iana-if-type:ethernetCsmacd");
    std::string path = prefix + "/type";
    rc = sr_set_item(session, path.c_str(), &value, SR_EDIT_DEFAULT);
  }
  {
    sr_val_t value = { 0 };
    value.type = SR_BOOL_T;
    value.data.bool_val = true;
    std::string path = prefix + "/enabled";
    rc = sr_set_item(session, path.c_str(), &value, SR_EDIT_DEFAULT);
  }
  rc = sr_commit(session);
  if (rc != SR_ERR_OK) {
    Logger::get()->error("Error by sr_commit: {}", sr_strerror(rc));
    return;
  }
}

void
SysrepoTest::provide_oper_state(sr_val_t **values, size_t *values_cnt) {
  const auto port_info = dev_mgr->get_port_info();
  int entries_per_iface = 6;
  int num_entries = entries_per_iface * port_info.size();
  int rc = SR_ERR_OK;
  sr_val_t *varray = NULL;
  rc = sr_new_values(num_entries, &varray);
  if (rc != SR_ERR_OK) return;
  sr_val_t *v = varray;
  for (const auto &p : port_info) {
    const auto &iface = p.second.iface_name;
    std::string prefix("/openconfig-interfaces:interfaces/interface");
    prefix.append("[name='");
    prefix.append(iface);
    prefix.append("']");
    prefix.append("/state");
    {
      std::string path = prefix + "/mtu";
      sr_val_set_xpath(v, path.c_str());
      v->type = SR_UINT16_T;
      v->data.uint16_val = 1500;
      v++;
    }
    {
      std::string path = prefix + "/type";
      sr_val_set_xpath(v, path.c_str());
      sr_val_set_str_data(v, SR_IDENTITYREF_T, "iana-if-type:ethernetCsmacd");
      v++;
    }
    {
      std::string path = prefix + "/enabled";
      sr_val_set_xpath(v, path.c_str());
      v->type = SR_BOOL_T;
      v->data.bool_val = true;
      v++;
    }
    {
      std::string path = prefix + "/ifindex";
      sr_val_set_xpath(v, path.c_str());
      v->type = SR_UINT32_T;
      v->data.uint32_val = p.first;
      v++;
    }
    {
      std::string path = prefix + "/admin-status";
      sr_val_set_xpath(v, path.c_str());
      sr_val_set_str_data(v, SR_ENUM_T, "UP");
      v++;
    }
    {
      std::string path = prefix + "/oper-status";
      sr_val_set_xpath(v, path.c_str());
      sr_val_set_str_data(v, SR_ENUM_T, "UP");
      v++;
    }
  }
  *values = varray;
  *values_cnt = num_entries;
}

SysrepoTest::~SysrepoTest() {
  if (subscription != NULL) {
    sr_unsubscribe(session, subscription);
    subscription = NULL;
  }
  if (session != NULL) {
    sr_session_stop(session);
    session = NULL;
  }
  if (connection != NULL) {
    sr_disconnect(connection);
    connection = NULL;
  }
}

}  // namespace sswitch_grpc
