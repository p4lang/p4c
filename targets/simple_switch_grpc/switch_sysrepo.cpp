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

#include <boost/optional.hpp>

#include <chrono>
#include <iostream>
#include <iterator>  // for std::distance
#include <mutex>
#include <regex>
#include <string>
#include <unordered_map>

extern "C" {
#include "sysrepo.h"
#include "sysrepo/values.h"
#include "sysrepo/xpath.h"
}

namespace sswitch_grpc {

using clock = std::chrono::high_resolution_clock;

// This struct is used to store the information needed to provide operational
// state to sysrepo. These YANG nodes are stubbed out in bmv2 so we don't have
// any bmv2 API to call to retrieve the state.
// We use boost::optional for leaves that don't have defaults.
struct PortState {
  // has the interface been created (name node)?
  // useful because we never remove ports from the PortStateMap: leaf values
  // are changed out-of-order so it's better not to make any assumptions
  bool valid{false};
  const std::string type{"iana-if-type:ethernetCsmacd"};
  boost::optional<uint16_t> mtu;
  boost::optional<std::string> description;
  bool enabled{false};
  boost::optional<std::string> mac_address;
  bool auto_negotiate{true};
  boost::optional<std::string> duplex_mode;
  boost::optional<std::string> port_speed;
  bool enable_flow_control{false};
  // We use this for a very naive implementation of carrier transitions and last
  // change. Every time the StateProvider is invoked, we check if the new
  // operational status is different from the old one, and if yes, we increment
  // carrier_transitions. We rely on the fact that the StateProvider is invoked
  // fairly often. An alternative implementation would be to register a status
  // CB with DevMgr / PortMonitor.
  size_t carrier_transitions{0u};
  boost::optional<std::string> last_oper_status;
  clock::time_point last_change_tp{clock::now()};
  clock::time_point last_clear_tp{clock::now()};

  // called when an interface is deleted since we do not remove the PortState
  // struct object from the PortStateMap.
  void reset() {
    valid = false;
    mtu.reset();
    description.reset();
    enabled = false;
    mac_address.reset();
    auto_negotiate = true;
    duplex_mode.reset();
    port_speed.reset();
    enable_flow_control = false;
    carrier_transitions = 0;
    last_oper_status.reset();
    last_change_tp = clock::now();
    last_clear_tp = clock::now();
  }
};

// Maps the interface string representation (<device id>-<port num>@<name>) to
// the corresponding PortState struct. Client is responsible for acquiring the
// lock.
class PortStateMap {
 public:
  using Lock = std::unique_lock<std::mutex>;

  Lock lock() { return Lock(m); }

  PortState &operator[](const std::string &iface_name) {
    return map[iface_name];
  }

 private:
  mutable std::mutex m{};
  std::unordered_map<std::string, PortState> map{};
};

// C++ wrapper around a sysrepo session
class SysrepoSession {
 public:
  ~SysrepoSession();

  bool open(const std::string &app_name);

  sr_session_ctx_t *get() const { return sess; }

  sr_session_ctx_t *operator *() { return get(); }

 private:
  sr_conn_ctx_t *conn{nullptr};
  sr_session_ctx_t *sess{nullptr};
};

// Subscribes to sysrepo config changes.
// It is in charge of updating the PortStateMap appropriately and calling
// DevMgr::port_add / DevMgr::port_remove when needed (i.e. when the 'enabled'
// node is modified).
class SysrepoSubscriber {
 public:
  SysrepoSubscriber(bm::device_id_t device_id, bm::DevMgr *dev_mgr,
                    PortStateMap *port_state_map);
  ~SysrepoSubscriber();
  bool start();
  int change_cb(sr_session_ctx_t *session, const char *module_name,
                sr_notif_event_t event);

 private:
  int change_name(sr_session_ctx_t *session, sr_notif_event_t event,
                  sr_change_oper_t oper, const std::string &iface_name,
                  sr_val_t *old_value, sr_val_t *new_value);
  int change_type(sr_session_ctx_t *session, sr_notif_event_t event,
                  sr_change_oper_t oper, const std::string &iface_name,
                  sr_val_t *old_value, sr_val_t *new_value);
  int change_mtu(sr_session_ctx_t *session, sr_notif_event_t event,
                 sr_change_oper_t oper, const std::string &iface_name,
                 sr_val_t *old_value, sr_val_t *new_value);
  int change_description(sr_session_ctx_t *session, sr_notif_event_t event,
                         sr_change_oper_t oper, const std::string &iface_name,
                         sr_val_t *old_value, sr_val_t *new_value);
  int change_enabled(sr_session_ctx_t *session, sr_notif_event_t event,
                     sr_change_oper_t oper, const std::string &iface_name,
                     sr_val_t *old_value, sr_val_t *new_value);
  int change_mac_address(sr_session_ctx_t *session, sr_notif_event_t event,
                         sr_change_oper_t oper, const std::string &iface_name,
                         sr_val_t *old_value, sr_val_t *new_value);
  int change_auto_negotiate(sr_session_ctx_t *session, sr_notif_event_t event,
                            sr_change_oper_t oper,
                            const std::string &iface_name,
                            sr_val_t *old_value, sr_val_t *new_value);
  int change_duplex_mode(sr_session_ctx_t *session, sr_notif_event_t event,
                         sr_change_oper_t oper, const std::string &iface_name,
                         sr_val_t *old_value, sr_val_t *new_value);
  int change_port_speed(sr_session_ctx_t *session, sr_notif_event_t event,
                        sr_change_oper_t oper, const std::string &iface_name,
                        sr_val_t *old_value, sr_val_t *new_value);
  int change_enable_flow_control(sr_session_ctx_t *session,
                                 sr_notif_event_t event,
                                 sr_change_oper_t oper,
                                 const std::string &iface_name,
                                 sr_val_t *old_value, sr_val_t *new_value);

  const bm::device_id_t my_device_id;
  bm::DevMgr *dev_mgr;  // non-owning pointer
  PortStateMap *port_state_map;
  SysrepoSession session{};
  sr_subscription_ctx_t *subscription{nullptr};
  static constexpr const char *const app_name = "subscriber";
  static constexpr const char *const module_names[] =
      {"openconfig-interfaces", "openconfig-platform"};
};

// Provides operational state (from PortStateMap) to sysrepo.
class SysrepoStateProvider {
 public:
  SysrepoStateProvider(const bm::DevMgr *dev_mgr, PortStateMap *port_state_map);
  ~SysrepoStateProvider();
  bool start();
  void provide_oper_state_interface(
      const std::string &iface_name, sr_val_t **values, size_t *values_cnt);
  void provide_oper_state_ethernet(
      const std::string &iface_name, sr_val_t **values, size_t *values_cnt);

 private:
  template <typename ts_res>
  static ts_res get_time_since_epoch(const clock::time_point &tp) {
    using std::chrono::duration_cast;
    return duration_cast<ts_res>(tp.time_since_epoch());
  }

  const bm::DevMgr *dev_mgr;  // non-owning pointer
  PortStateMap *port_state_map;
  SysrepoSession session{};
  sr_subscription_ctx_t *subscription{nullptr};
  // This is used for
  // openconfig-interfaces/interfaces/interface/state/counters/last-clear. I
  // could have used SimpleSwitch's clock, but I wanted this code to be
  // completely independent of SimpleSwitch in case it has to be re-used for
  // another (bmv2) target in the future. It doesn't exactly correspond to when
  // the switch starts processing packets, but it is good enough.
  clock::time_point start_tp;
  static constexpr const char *const app_name = "test";
};

namespace {

using bm::Logger;

// Subscription callback for all changes
int module_change_cb(sr_session_ctx_t *session, const char *module_name,
                     sr_notif_event_t event, void *private_ctx) {
  Logger::get()->trace("YANG config change");
  auto *subscriber = static_cast<SysrepoSubscriber *>(private_ctx);
  return subscriber->change_cb(session, module_name, event);
}

// Used to parse an interface string representation (<dev id>-<port num>@<name>)
// Returns true iff the string can be parsed successfully.
bool
parse_iface_name(const std::string &iface_name,
                 bm::device_id_t *device_id, int *port,
                 std::string *name) {
  static const std::regex iface_regex("(\\d+)-(\\d+)@([\\w-]+)");
  std::smatch sm;
  if (!std::regex_match(iface_name, sm, iface_regex,
                        std::regex_constants::match_default)) {
    Logger::get()->error(
        "YANG subscriber: '{}' is not a valid interface string", iface_name);
    return false;
  }
  assert(sm.size() == 4);

  bm::device_id_t device_id_;
  try {
    device_id_ = std::stoull(sm[1]);
  } catch (...) {
    Logger::get()->error(
        "YANG subscriber: '{}' is not a valid interface string (bad device)",
        iface_name);
    return false;
  }
  if (device_id != nullptr) *device_id = device_id_;

  int port_;
  try {
    port_ = std::stoi(sm[2]);
  } catch (...) {
    Logger::get()->error(
        "YANG subscriber: '{}' is not a valid interface string (bad port)",
        iface_name);
    return false;
  }
  if (port != nullptr) *port = port_;

  if (name != nullptr) *name = sm[3];

  return true;
}

}  // namespace

SysrepoSession::~SysrepoSession() {
  if (sess != nullptr) sr_session_stop(sess);
  if (conn != nullptr) sr_disconnect(conn);
}

bool
SysrepoSession::open(const std::string &app_name) {
  int rc = SR_ERR_OK;
  rc = sr_connect(app_name.c_str(), SR_CONN_DEFAULT, &conn);
  if (rc != SR_ERR_OK) {
    Logger::get()->error("Error by sr_connect: {}", sr_strerror(rc));
    return false;
  }
  rc = sr_session_start(conn, SR_DS_RUNNING, SR_SESS_DEFAULT, &sess);
  if (rc != SR_ERR_OK) {
    Logger::get()->error("Error by sr_session_start: {}", sr_strerror(rc));
    return false;
  }
  return true;
}

constexpr const char *const SysrepoSubscriber::app_name;
constexpr const char *const SysrepoSubscriber::module_names[];

int
SysrepoSubscriber::change_name(
    sr_session_ctx_t *session, sr_notif_event_t event,
    sr_change_oper_t oper, const std::string &iface_name,
    sr_val_t *, sr_val_t *new_value) {
  if (event == SR_EV_VERIFY && oper == SR_OP_MODIFIED) {
    sr_set_error(session,
                 "Cannot modify interface name without deleting it first",
                 new_value->xpath);
    return SR_ERR_UNSUPPORTED;
  }
  if (event != SR_EV_APPLY) return SR_ERR_OK;
  int port;
  parse_iface_name(iface_name, nullptr, &port, nullptr);
  auto lock = port_state_map->lock();
  auto &port_state = (*port_state_map)[iface_name];
  if (oper == SR_OP_CREATED) {
    port_state.valid = true;
  } else if (oper == SR_OP_DELETED) {
    port_state.reset();
    // For the BMI DevMgr, the stats are also cleared during port_add, so this
    // may be redundant...
    dev_mgr->clear_port_stats(port);
  }
  return SR_ERR_OK;
}

int
SysrepoSubscriber::change_type(
    sr_session_ctx_t *session, sr_notif_event_t event,
    sr_change_oper_t oper, const std::string &,
    sr_val_t *, sr_val_t *new_value) {
  if (event != SR_EV_VERIFY) return SR_ERR_OK;
  if (oper != SR_OP_CREATED && oper != SR_OP_MODIFIED) return SR_ERR_OK;
  if (strcmp(new_value->data.enum_val, "iana-if-type:ethernetCsmacd")) {
    sr_set_error(session,
                 "Only ethernetCsmacd interfaces are supported",
                 new_value->xpath);
    return SR_ERR_INVAL_ARG;
  }
  return SR_ERR_OK;
}

int
SysrepoSubscriber::change_mtu(
    sr_session_ctx_t *, sr_notif_event_t event,
    sr_change_oper_t oper, const std::string &iface_name,
    sr_val_t *, sr_val_t *new_value) {
  if (event != SR_EV_APPLY) return SR_ERR_OK;
  auto lock = port_state_map->lock();
  auto &port_state = (*port_state_map)[iface_name];
  if (oper == SR_OP_CREATED || oper == SR_OP_MODIFIED)
    port_state.mtu = new_value->data.uint16_val;
  else if (oper == SR_OP_DELETED)
    port_state.mtu.reset();
  return SR_ERR_OK;
}

int
SysrepoSubscriber::change_description(
    sr_session_ctx_t *, sr_notif_event_t event,
    sr_change_oper_t oper, const std::string &iface_name,
    sr_val_t *, sr_val_t *new_value) {
  if (event != SR_EV_APPLY) return SR_ERR_OK;
  auto lock = port_state_map->lock();
  auto &port_state = (*port_state_map)[iface_name];
  if (oper == SR_OP_CREATED || oper == SR_OP_MODIFIED)
    port_state.description = new_value->data.string_val;
  else if (oper == SR_OP_DELETED)
    port_state.description.reset();
  return SR_ERR_OK;
}

int
SysrepoSubscriber::change_enabled(
    sr_session_ctx_t *, sr_notif_event_t event,
    sr_change_oper_t, const std::string &iface_name,
    sr_val_t *, sr_val_t *new_value) {
  if (event != SR_EV_APPLY) return SR_ERR_OK;
  int port;
  std::string iface;
  parse_iface_name(iface_name, nullptr, &port, &iface);
  // true if enabled, false otherwise
  auto get_status = [](sr_val_t *v) {
    return (v == NULL) ? false : v->data.bool_val;
  };
  auto enable = get_status(new_value);
  auto lock = port_state_map->lock();
  auto &port_state = (*port_state_map)[iface_name];
  if (enable) {
    auto rc = dev_mgr->port_add(iface, port, {});
    if (rc == bm::DevMgr::ReturnCode::SUCCESS) port_state.enabled = true;
  } else {
    auto rc = dev_mgr->port_remove(port);
    if (rc == bm::DevMgr::ReturnCode::SUCCESS) port_state.enabled = false;
  }
  return SR_ERR_OK;
}

int
SysrepoSubscriber::change_mac_address(
    sr_session_ctx_t *, sr_notif_event_t event,
    sr_change_oper_t oper, const std::string &iface_name,
    sr_val_t *, sr_val_t *new_value) {
  if (event != SR_EV_APPLY) return SR_ERR_OK;
  auto lock = port_state_map->lock();
  auto &port_state = (*port_state_map)[iface_name];
  if (oper == SR_OP_CREATED || oper == SR_OP_MODIFIED)
    port_state.mac_address = new_value->data.string_val;
  else if (oper == SR_OP_DELETED)
    port_state.mac_address.reset();
  return SR_ERR_OK;
}

int
SysrepoSubscriber::change_auto_negotiate(
    sr_session_ctx_t *, sr_notif_event_t event,
    sr_change_oper_t oper, const std::string &iface_name,
    sr_val_t *, sr_val_t *new_value) {
  if (event != SR_EV_APPLY) return SR_ERR_OK;
  auto lock = port_state_map->lock();
  auto &port_state = (*port_state_map)[iface_name];
  if (oper == SR_OP_CREATED || oper == SR_OP_MODIFIED)
    port_state.auto_negotiate = new_value->data.bool_val;
  else if (oper == SR_OP_DELETED)
    port_state.auto_negotiate = true;
  return SR_ERR_OK;
}

int
SysrepoSubscriber::change_duplex_mode(
    sr_session_ctx_t *, sr_notif_event_t event,
    sr_change_oper_t oper, const std::string &iface_name,
    sr_val_t *, sr_val_t *new_value) {
  if (event != SR_EV_APPLY) return SR_ERR_OK;
  auto lock = port_state_map->lock();
  auto &port_state = (*port_state_map)[iface_name];
  if (oper == SR_OP_CREATED || oper == SR_OP_MODIFIED)
    port_state.duplex_mode = new_value->data.string_val;
  else if (oper == SR_OP_DELETED)
    port_state.duplex_mode.reset();
  return SR_ERR_OK;
}

int
SysrepoSubscriber::change_port_speed(
    sr_session_ctx_t *, sr_notif_event_t event,
    sr_change_oper_t oper, const std::string &iface_name,
    sr_val_t *, sr_val_t *new_value) {
  if (event != SR_EV_APPLY) return SR_ERR_OK;
  auto lock = port_state_map->lock();
  auto &port_state = (*port_state_map)[iface_name];
  if (oper == SR_OP_CREATED || oper == SR_OP_MODIFIED)
    port_state.port_speed = new_value->data.string_val;
  else if (oper == SR_OP_DELETED)
    port_state.port_speed.reset();
  return SR_ERR_OK;
}

int
SysrepoSubscriber::change_enable_flow_control(
    sr_session_ctx_t *, sr_notif_event_t event,
    sr_change_oper_t oper, const std::string &iface_name,
    sr_val_t *, sr_val_t *new_value) {
  if (event != SR_EV_APPLY) return SR_ERR_OK;
  auto lock = port_state_map->lock();
  auto &port_state = (*port_state_map)[iface_name];
  if (oper == SR_OP_CREATED || oper == SR_OP_MODIFIED)
    port_state.enable_flow_control = new_value->data.bool_val;
  else if (oper == SR_OP_DELETED)
    port_state.enable_flow_control = false;
  return SR_ERR_OK;
}

int
SysrepoSubscriber::change_cb(sr_session_ctx_t *session, const char *module_name,
                             sr_notif_event_t event) {
  if (strcmp(module_name, "openconfig-interfaces")) {
    Logger::get()->warn(
        "Subscription CB for non-implemented module {}", module_name);
    return SR_ERR_UNSUPPORTED;
  }

  using ChangeFn = std::function<int(SysrepoSubscriber *,
                                     sr_session_ctx_t *, sr_notif_event_t,
                                     sr_change_oper_t, const std::string &,
                                     sr_val_t *, sr_val_t *)>;
  static const std::unordered_map<std::string, ChangeFn> fn_map = {
    {"name", &SysrepoSubscriber::change_name},
    {"type", &SysrepoSubscriber::change_type},
    {"mtu", &SysrepoSubscriber::change_mtu},
    {"description", &SysrepoSubscriber::change_description},
    {"enabled", &SysrepoSubscriber::change_enabled},
    {"mac-address", &SysrepoSubscriber::change_mac_address},
    {"auto-negotiate", &SysrepoSubscriber::change_auto_negotiate},
    {"duplex-mode", &SysrepoSubscriber::change_duplex_mode},
    {"port-speed", &SysrepoSubscriber::change_port_speed},
    {"enable_flow_control", &SysrepoSubscriber::change_enable_flow_control}};

  if (event != SR_EV_VERIFY && event != SR_EV_APPLY) return SR_ERR_OK;

  sr_change_iter_t *it = nullptr;
  int rc = SR_ERR_OK;
  int rc_change = SR_ERR_OK;
  sr_change_oper_t oper;
  sr_val_t *old_value = nullptr;
  sr_val_t *new_value = nullptr;

  static const std::regex xpath_regex(
      "/openconfig-interfaces:interfaces/interface\\[name='(.*)'\\]/"
      "(.+/)?config/(.*)");

  std::string change_path("/");
  change_path.append(module_name).append(":*");

  rc = sr_get_changes_iter(session, change_path.c_str(), &it);
  assert(rc == SR_ERR_OK);
  while ((rc = sr_get_change_next(session, it, &oper, &old_value, &new_value))
         == SR_ERR_OK) {
    std::string xpath;
    bool skip = true;
    if (oper == SR_OP_CREATED) {
      xpath = std::string(new_value->xpath);
      skip = (new_value->type < SR_LEAF_EMPTY_T || new_value->dflt);
    } else if (oper == SR_OP_MODIFIED) {
      assert(std::string(new_value->xpath) == std::string(old_value->xpath));
      xpath = std::string(new_value->xpath);
      skip = (new_value->type < SR_LEAF_EMPTY_T || new_value->dflt);
    } else if (oper == SR_OP_DELETED) {
      xpath = std::string(old_value->xpath);
      skip = (old_value->type < SR_LEAF_EMPTY_T || old_value->dflt);
    }

    if (skip) {
      sr_free_val(old_value);
      sr_free_val(new_value);
      continue;
    }

    std::smatch sm;
    std::regex_match(xpath, sm, xpath_regex,
                     std::regex_constants::match_default);
    if (!sm.empty()) {
      assert(sm.size() == 4);
      const auto &iface_name = sm[1].str();
      const auto &augment = sm[2].str();
      const auto &node = sm[3].str();
      Logger::get()->trace("YANG subscriber: changing node '{}' for iface '{}'",
                           node, iface_name);

      // ignore change if device id does not match ours (all the devices on the
      // "chassis" receive all the notifications with the current design).
      bm::device_id_t device_id;
      if (!parse_iface_name(iface_name, &device_id, nullptr, nullptr))
        return SR_ERR_INVAL_ARG;
      else if (device_id != my_device_id)
        return SR_ERR_OK;

      auto it = fn_map.find(node);
      if (it != fn_map.end()) {
        rc_change = it->second(
            this, session, event, oper, iface_name, old_value, new_value);
      } else {
        sr_set_error(session, "Unsupported YANG node", xpath.c_str());
        rc_change = SR_ERR_UNSUPPORTED;
      }
      // We can just set aside the augment for now as there is no overlap
      // between leaf names
      (void) augment;
    }

    sr_free_val(old_value);
    sr_free_val(new_value);
    if (rc_change != SR_ERR_OK) break;
  }

  sr_free_change_iter(it);
  return rc_change;
}

bool
SysrepoSubscriber::start() {
  int rc = SR_ERR_OK;
  if (!session.open(app_name)) return false;
  for (const auto &module_name : module_names) {
    // SR_SUBSCR_CTX_REUSE:
    // This option enables the application to re-use an already existing
    // subscription context previously returned from any sr_*_subscribe call
    // instead of requesting the creation of a new one. In that case a single
    // sr_unsubscribe call unsubscribes from all subscriptions filed within the
    // context.
    rc = sr_module_change_subscribe(
        *session, module_name, module_change_cb, static_cast<void *>(this),
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

SysrepoSubscriber::SysrepoSubscriber(bm::device_id_t device_id,
                                     bm::DevMgr *dev_mgr,
                                     PortStateMap *port_state_map)
    : my_device_id(device_id), dev_mgr(dev_mgr),
      port_state_map(port_state_map) { }

SysrepoSubscriber::~SysrepoSubscriber() {
  if (subscription != nullptr) {
    sr_unsubscribe(*session, subscription);
    subscription = nullptr;
  }
}

namespace {

int data_provider_cb(const char *xpath, sr_val_t **values, size_t *values_cnt,
                     uint64_t request_id, void *private_ctx) {
  // request_id was added to sysrepo recently as a way to ensure consistency of
  // reported state across multiple requests generated by sysrepo for the same
  // YANG path; we do not use it at the moment.
  // See https://github.com/sysrepo/sysrepo/pull/1227
  (void) request_id;
  // Turning this on can trigger a lot of logging messages when sysrepo is being
  // polled for changes (which the current implementation for gNMI subscription
  // streams in the p4lang P4Runtime server)
  // Logger::get()->trace("YANG oper data requested for {}", xpath);
  auto *tester = static_cast<SysrepoStateProvider *>(private_ctx);
  *values_cnt = 0;
  {
    const std::regex xpath_regex(
        "/openconfig-interfaces:interfaces/interface\\[name='(.*)'\\]/"
        "state(.*)");
    std::smatch sm;
    std::string xpath_(xpath);
    std::regex_match(xpath_, sm, xpath_regex,
                     std::regex_constants::match_default);
    if (!sm.empty()) {
      assert(sm.size() == 3);
      const auto &iface_name = sm[1].str();
      const auto &node = sm[2].str();
      (void) node;
      tester->provide_oper_state_interface(iface_name, values, values_cnt);
    }
  }
  {
    const std::regex xpath_regex(
        "/openconfig-interfaces:interfaces/interface\\[name='(.*)'\\]/"
        "openconfig-if-ethernet:ethernet/state(.*)");
    std::smatch sm;
    std::string xpath_(xpath);
    std::regex_match(xpath_, sm, xpath_regex,
                     std::regex_constants::match_default);
    if (!sm.empty()) {
      assert(sm.size() == 3);
      const auto &iface_name = sm[1].str();
      const auto &node = sm[2].str();
      (void) node;
      tester->provide_oper_state_ethernet(iface_name, values, values_cnt);
    }
  }
  return SR_ERR_OK;
}

}  // namespace

constexpr const char *const SysrepoStateProvider::app_name;

SysrepoStateProvider::SysrepoStateProvider(const bm::DevMgr *dev_mgr,
                                           PortStateMap *port_state_map)
    : dev_mgr(dev_mgr), port_state_map(port_state_map),
      start_tp(clock::now()) { }

bool
SysrepoStateProvider::start() {
  int rc = SR_ERR_OK;
  if (!session.open(app_name)) return false;
  rc = sr_dp_get_items_subscribe(
      *session, "/openconfig-interfaces:interfaces/interface",
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
SysrepoStateProvider::provide_oper_state_interface(
    const std::string &iface_str, sr_val_t **values, size_t *values_cnt) {
  // name, type, mtu, description, enabled, ifindex, admin-status, oper-status
  int max_num_entries = 9;
  max_num_entries += 12;  // counters
  int rc = SR_ERR_OK;
  sr_val_t *varray = nullptr;
  rc = sr_new_values(max_num_entries, &varray);
  if (rc != SR_ERR_OK) return;
  sr_val_t *v = varray;

  int port;
  parse_iface_name(iface_str, nullptr, &port, nullptr);

  auto lock = port_state_map->lock();
  auto &port_state = (*port_state_map)[iface_str];
  if (!port_state.valid) return;

  std::string prefix("/openconfig-interfaces:interfaces/interface");
  prefix.append("[name='");
  prefix.append(iface_str);
  prefix.append("']");
  prefix.append("/state");
  {
    std::string path = prefix + "/name";
    sr_val_set_xpath(v, path.c_str());
    sr_val_set_str_data(v, SR_STRING_T, iface_str.c_str());
    v++;
  }
  {
    std::string path = prefix + "/type";
    sr_val_set_xpath(v, path.c_str());
    sr_val_set_str_data(v, SR_IDENTITYREF_T, port_state.type.c_str());
    v++;
  }
  {
    std::string path = prefix + "/mtu";
    if (port_state.mtu != boost::none) {
      sr_val_set_xpath(v, path.c_str());
      v->type = SR_UINT16_T;
      v->data.uint16_val = port_state.mtu.get();
      v++;
    }
  }
  {
    std::string path = prefix + "/description";
    if (port_state.description != boost::none) {
      sr_val_set_xpath(v, path.c_str());
      sr_val_set_str_data(v, SR_STRING_T, port_state.description->c_str());
      v++;
    }
  }
  {
    std::string path = prefix + "/enabled";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_BOOL_T;
    v->data.bool_val = port_state.enabled;
    v++;
  }
  {
    std::string path = prefix + "/ifindex";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_UINT32_T;
    v->data.uint32_val = port;
    v++;
  }
  {
    std::string path = prefix + "/admin-status";
    sr_val_set_xpath(v, path.c_str());
    if (port_state.enabled)
      sr_val_set_str_data(v, SR_ENUM_T, "UP");
    else
      sr_val_set_str_data(v, SR_ENUM_T, "DOWN");
    v++;
  }
  {
    std::string path = prefix + "/oper-status";
    sr_val_set_xpath(v, path.c_str());
    std::string status = dev_mgr->port_is_up(port) ? "UP" : "DOWN";
    // See comments for PortState declaration.
    if (port_state.last_oper_status != boost::none &&
        port_state.last_oper_status != status) {
      port_state.carrier_transitions += 1;
      port_state.last_change_tp = clock::now();
    }
    port_state.last_oper_status = status;
    sr_val_set_str_data(v, SR_ENUM_T, status.c_str());
    v++;
  }
  {
    // "This timestamp indicates the time of the last state change of the
    // interface (e.g., up-to-down transition). This corresponds to the
    // ifLastChange object in the standard interface MIB. The value is the
    // timestamp in nanoseconds relative to the Unix Epoch (Jan 1, 1970 00:00:00
    // UTC)."
    std::string path = prefix + "/last-change";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_UINT64_T;
    v->data.uint64_val = get_time_since_epoch<std::chrono::nanoseconds>(
        port_state.last_change_tp).count();
    v++;
  }
  // COUNTERS
  // These can potentially change must faster than the rest of the operational
  // state. Currently the PI server code queries the operational state every
  // 100ms. The client will probably want to have different subscriptions for
  // counters and for the rest of the state.
  // TODO(antonin): investigate if we can register a separate data provider for
  // counters.
  auto port_stats = dev_mgr->get_port_stats(port);
  {
    std::string path = prefix + "/counters/in-octets";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_UINT64_T;
    v->data.uint64_val = port_stats.in_octets;
    v++;
  }
  {
    std::string path = prefix + "/counters/in-unicast-pkts";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_UINT64_T;
    v->data.uint64_val = port_stats.in_packets;
    v++;
  }
  {
    std::string path = prefix + "/counters/in-discards";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_UINT64_T;
    v->data.uint64_val = 0u;
    v++;
  }
  {
    std::string path = prefix + "/counters/in-errors";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_UINT64_T;
    v->data.uint64_val = 0u;
    v++;
  }
  {
    std::string path = prefix + "/counters/in-unknown-protos";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_UINT64_T;
    v->data.uint64_val = 0u;
    v++;
  }
  {
    std::string path = prefix + "/counters/in-fcs-errors";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_UINT64_T;
    v->data.uint64_val = 0u;
    v++;
  }
  {
    std::string path = prefix + "/counters/out-octets";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_UINT64_T;
    v->data.uint64_val = port_stats.out_octets;
    v++;
  }
  {
    std::string path = prefix + "/counters/out-unicast-pkts";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_UINT64_T;
    v->data.uint64_val = port_stats.out_packets;
    v++;
  }
  {
    std::string path = prefix + "/counters/out-discards";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_UINT64_T;
    v->data.uint64_val = 0u;
    v++;
  }
  {
    std::string path = prefix + "/counters/out-errors";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_UINT64_T;
    v->data.uint64_val = 0u;
    v++;
  }
  {
    std::string path = prefix + "/counters/carrier-transitions";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_UINT64_T;
    v->data.uint64_val = port_state.carrier_transitions;
    v++;
  }
  {
    // "Timestamp of the last time the interface counters were cleared. The
    // value is the timestamp in nanoseconds relative to the Unix Epoch (Jan 1,
    // 1970 00:00:00 UTC)."
    std::string path = prefix + "/counters/last-clear";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_UINT64_T;
    v->data.uint64_val =
        get_time_since_epoch<std::chrono::nanoseconds>(start_tp).count();
    v++;
  }

  // shrink value arrays to match actual count
  size_t num_entries = std::distance(varray, v);
  sr_realloc_values(max_num_entries, num_entries, &varray);
  *values = varray;
  *values_cnt = num_entries;
}

void
SysrepoStateProvider::provide_oper_state_ethernet(
    const std::string &iface_str, sr_val_t **values, size_t *values_cnt) {
  // mac-address, auto-negotiate, duplex-mode, port-speed, enable-flow-control,
  // hw-mac-address, negotiated-dupelx-mode, negotiated-port-speed
  int max_num_entries = 8;
  int rc = SR_ERR_OK;
  sr_val_t *varray = nullptr;
  rc = sr_new_values(max_num_entries, &varray);
  if (rc != SR_ERR_OK) return;
  sr_val_t *v = varray;

  auto lock = port_state_map->lock();
  auto &port_state = (*port_state_map)[iface_str];
  if (!port_state.valid) return;

  std::string prefix("/openconfig-interfaces:interfaces/interface");
  prefix.append("[name='");
  prefix.append(iface_str);
  prefix.append("']");
  prefix.append("/openconfig-if-ethernet:ethernet/state");

  {
    std::string path = prefix + "/mac-address";
    if (port_state.mac_address != boost::none) {
      sr_val_set_xpath(v, path.c_str());
      sr_val_set_str_data(v, SR_STRING_T, port_state.mac_address->c_str());
      v++;
    }
  }
  {
    std::string path = prefix + "/auto-negotiate";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_BOOL_T;
    v->data.bool_val = port_state.auto_negotiate;
    v++;
  }
  {
    std::string path = prefix + "/duplex-mode";
    if (port_state.duplex_mode != boost::none) {
      sr_val_set_xpath(v, path.c_str());
      sr_val_set_str_data(v, SR_ENUM_T, port_state.duplex_mode->c_str());
      v++;
    }
  }
  {
    std::string path = prefix + "/port-speed";
    if (port_state.port_speed != boost::none) {
      sr_val_set_xpath(v, path.c_str());
      sr_val_set_str_data(v, SR_IDENTITYREF_T, port_state.port_speed->c_str());
      v++;
    }
  }
  {
    std::string path = prefix + "/enable-flow-control";
    sr_val_set_xpath(v, path.c_str());
    v->type = SR_BOOL_T;
    v->data.bool_val = port_state.enable_flow_control;
    v++;
  }
  {
    std::string path = prefix + "/hw-mac-address";
    sr_val_set_xpath(v, path.c_str());
    sr_val_set_str_data(v, SR_STRING_T, "ab:00:00:00:00:00");
    v++;
  }
  {
    std::string path = prefix + "/negotiated-duplex-mode";
    if (port_state.auto_negotiate) {
      sr_val_set_xpath(v, path.c_str());
      sr_val_set_str_data(v, SR_ENUM_T, "FULL");
      v++;
    }
  }
  {
    std::string path = prefix + "/negotiated-port-speed";
    if (port_state.auto_negotiate) {
      sr_val_set_xpath(v, path.c_str());
      sr_val_set_str_data(v, SR_IDENTITYREF_T, "SPEED_10GB");
      v++;
    }
  }

  // shrink value arrays to match actual count
  size_t num_entries = std::distance(varray, v);
  sr_realloc_values(max_num_entries, num_entries, &varray);
  *values = varray;
  *values_cnt = num_entries;
}

SysrepoStateProvider::~SysrepoStateProvider() {
  if (subscription != nullptr) {
    sr_unsubscribe(*session, subscription);
    subscription = nullptr;
  }
}

SysrepoDriver::SysrepoDriver(bm::device_id_t device_id,
                             bm::DevMgr *dev_mgr)
    : my_device_id(device_id), dev_mgr(dev_mgr),
      port_state_map(new PortStateMap()),
      sysrepo_subscriber(
          new SysrepoSubscriber(device_id, dev_mgr, port_state_map.get())),
      sysrepo_state_provider(
          new SysrepoStateProvider(dev_mgr, port_state_map.get())) { }

SysrepoDriver::~SysrepoDriver() = default;

bool
SysrepoDriver::start() {
  return sysrepo_subscriber->start() && sysrepo_state_provider->start();
}

void
SysrepoDriver::add_iface(int port, const std::string &name) {
  // combine device id, port number and name to build the full string
  // representation (used in the YANG datastore) for the interface.
  std::string iface_str = std::to_string(my_device_id) + "-"
      + std::to_string(port) + "@" + name;
  SysrepoSession session;
  if (!session.open("add_iface")) return;
  int rc = SR_ERR_OK;
  std::string prefix("/openconfig-interfaces:interfaces/interface");
  prefix.append("[name='");
  prefix.append(iface_str);
  prefix.append("']");
  prefix.append("/config");
  {
    std::string path = prefix + "/name";
    rc = sr_set_item_str(*session, path.c_str(), iface_str.c_str(),
                         SR_EDIT_DEFAULT);
  }
  {
    sr_val_t value = {};
    value.type = SR_IDENTITYREF_T;
    value.data.string_val = const_cast<char *>("iana-if-type:ethernetCsmacd");
    std::string path = prefix + "/type";
    rc = sr_set_item(*session, path.c_str(), &value, SR_EDIT_DEFAULT);
  }
  {
    sr_val_t value = {};
    value.type = SR_BOOL_T;
    value.data.bool_val = true;
    std::string path = prefix + "/enabled";
    rc = sr_set_item(*session, path.c_str(), &value, SR_EDIT_DEFAULT);
  }
  rc = sr_commit(*session);
  if (rc != SR_ERR_OK) {
    Logger::get()->error("Error by sr_commit: {}", sr_strerror(rc));
    return;
  }
}

}  // namespace sswitch_grpc
