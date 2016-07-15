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
 * Srikrishna Gopu (krishna@barefootnetworks.com)
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include <bm/bm_sim/simple_pre_lag.h>
#include <bm/bm_sim/logger.h>

#include <string>
#include <vector>

#include "jsoncpp/json.h"

namespace bm {

McSimplePre::McReturnCode
McSimplePreLAG::mc_node_create(const rid_t rid,
                               const PortMap &port_map,
                               const LagMap &lag_map,
                               l1_hdl_t *l1_hdl) {
  boost::unique_lock<boost::shared_mutex> lock(mutex);
  l2_hdl_t l2_hdl;
  size_t num_l1_entries = l1_entries.size();
  size_t num_l2_entries = l2_entries.size();
  if (num_l1_entries >= L1_MAX_ENTRIES) {
    Logger::get()->error("node create failed, l1 table full");
    return TABLE_FULL;
  }
  if (num_l2_entries >= L2_MAX_ENTRIES) {
    Logger::get()->error("node create failed, l2 table full");
    return TABLE_FULL;
  }
  if (l1_handles.get_handle(l1_hdl)) {
    Logger::get()->error("node create failed, failed to obtain l1 handle");
    return ERROR;
  }
  if (l2_handles.get_handle(&l2_hdl)) {
    Logger::get()->error("node create failed, failed to obtain l2 handle");
    return ERROR;
  }
  l1_entries.insert(std::make_pair(*l1_hdl, L1Entry(rid)));
  L1Entry &l1_entry = l1_entries[*l1_hdl];
  l1_entry.l2_hdl = l2_hdl;
  l2_entries.insert(
    std::make_pair(l2_hdl, L2Entry(*l1_hdl, port_map, lag_map)));
  Logger::get()->debug("node created for rid {}", rid);
  return SUCCESS;
}

McSimplePre::McReturnCode
McSimplePreLAG::mc_node_update(const l1_hdl_t l1_hdl,
                               const PortMap &port_map,
                               const LagMap &lag_map) {
  boost::unique_lock<boost::shared_mutex> lock(mutex);
  if (!l1_handles.valid_handle(l1_hdl)) {
    Logger::get()->error("node update failed, invalid l1 handle");
    return INVALID_L1_HANDLE;
  }
  L1Entry &l1_entry = l1_entries[l1_hdl];
  l2_hdl_t l2_hdl = l1_entry.l2_hdl;
  L2Entry &l2_entry = l2_entries[l2_hdl];
  l2_entry.port_map = port_map;
  l2_entry.lag_map = lag_map;
  Logger::get()->debug("node updated for rid {}", l1_entry.rid);
  return SUCCESS;
}

McSimplePre::McReturnCode
McSimplePreLAG::mc_set_lag_membership(const lag_id_t lag_index,
                                      const PortMap &port_map) {
  boost::unique_lock<boost::shared_mutex> lock(mutex);
  uint16_t member_count = 0;
  if (lag_index > LAG_MAX_ENTRIES) {
    Logger::get()->error("lag membership set failed, invalid lag index");
    return ERROR;
  }
  for (unsigned int j = 0; j < port_map.size(); j++) {
    if (port_map[j]) {
      member_count++;
    }
  }
  LagEntry &lag_entry = lag_entries[lag_index];
  lag_entry.member_count = member_count;
  lag_entry.port_map = port_map;
  Logger::get()->debug("lag membership set for lag index {}", lag_index);
  return SUCCESS;
}

std::string
McSimplePreLAG::mc_get_entries() const {
  Json::Value root(Json::objectValue);

  {
    boost::unique_lock<boost::shared_mutex> lock(mutex);
    get_entries_common(&root);

    Json::Value lags(Json::arrayValue);
    for (const auto p : lag_entries) {
      Json::Value lag(Json::objectValue);
      lag["id"] = Json::Value(Json::UInt(p.first));

      const auto &entry = p.second;
      Json::Value ports(Json::arrayValue);
      for (size_t i = 0; i < entry.port_map.size(); i++) {
        if (entry.port_map[i]) ports.append(Json::Value(Json::UInt(i)));
      }
      lag["ports"] = ports;

      lags.append(lag);
    }

    root["lags"] = lags;
  }

  std::stringstream ss;
  ss << root;
  return ss.str();
}

void
McSimplePreLAG::reset_state() {
  Logger::get()->debug("resetting PRE state");
  boost::unique_lock<boost::shared_mutex> lock(mutex);
  McSimplePre::reset_state_();
  lag_entries.clear();
}

std::vector<McSimplePre::McOut>
McSimplePreLAG::replicate(const McSimplePre::McIn ingress_info) const {
  std::vector<McSimplePre::McOut> egress_info_list;
  egress_port_t port_id;
  PortMap port_map;
  lag_id_t lag_index;
  int lag_hash = 0xFF;  // TODO(unknown): get lag hash from metadata
  int port_count1 = 0, port_count2 = 0;
  McSimplePre::McOut egress_info;
  LagEntry lag_entry;
  boost::shared_lock<boost::shared_mutex> lock(mutex);
  auto mgid_it = mgid_entries.find(ingress_info.mgid);
  if (mgid_it == mgid_entries.end()) {
    Logger::get()->warn("Replication requested for mgid {}, which is not known "
                        "to the PRE", ingress_info.mgid);
    return {};
  }
  const MgidEntry &mgid_entry = mgid_it->second;
  for (const l1_hdl_t l1_hdl : mgid_entry.l1_list) {
    const L1Entry &l1_entry = l1_entries.at(l1_hdl);
    egress_info.rid = l1_entry.rid;
    // Port replication
    const L2Entry &l2_entry = l2_entries.at(l1_entry.l2_hdl);
    for (port_id = 0; port_id < l2_entry.port_map.size(); port_id++) {
      if (l2_entry.port_map[port_id]) {
        egress_info.egress_port = port_id;
        egress_info_list.push_back(egress_info);
      }
    }
    // Lag replication
    for (lag_index = 0; lag_index < l2_entry.lag_map.size(); lag_index++) {
      if (l2_entry.lag_map[lag_index]) {
        lag_entry = lag_entries.at(lag_index);
        port_map = lag_entry.port_map;
        port_count1 = (lag_hash % lag_entry.member_count) + 1;
        port_count2 = 0;
        for (port_id = 0; port_id < port_map.size(); port_id++) {
          if (port_map[port_id]) {
            port_count2++;
          }
          if (port_count1 == port_count2) {
            egress_info.egress_port = port_id;
            egress_info_list.push_back(egress_info);
            break;
          }
        }
      }
    }
  }
  BMLOG_DEBUG("number of packets replicated : {}", egress_info_list.size());
  return egress_info_list;
}

}  // namespace bm
