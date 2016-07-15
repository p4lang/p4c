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

#include <bm/bm_sim/simple_pre.h>
#include <bm/bm_sim/logger.h>

#include <string>
#include <vector>
#include <sstream>

#include "jsoncpp/json.h"

namespace bm {

McSimplePre::McReturnCode
McSimplePre::mc_mgrp_create(const mgrp_t mgid, mgrp_hdl_t *mgrp_hdl) {
  boost::unique_lock<boost::shared_mutex> lock(mutex);
  size_t num_entries = mgid_entries.size();
  if (num_entries >= MGID_TABLE_SIZE) {
    Logger::get()->error("mgrp create failed, mgid table full");
    return TABLE_FULL;
  }
  *mgrp_hdl = mgid;
  MgidEntry mgid_entry(mgid);
  mgid_entries.insert(std::make_pair(*mgrp_hdl, std::move(mgid_entry)));
  Logger::get()->debug("mgrp node created for mgid {}", mgid);
  return SUCCESS;
}

McSimplePre::McReturnCode
McSimplePre::mc_mgrp_destroy(mgrp_hdl_t mgrp_hdl) {
  boost::unique_lock<boost::shared_mutex> lock(mutex);
  mgid_entries.erase(mgrp_hdl);
  Logger::get()->debug("mgrp node deleted for mgid {}", mgrp_hdl);
  return SUCCESS;
}

McSimplePre::McReturnCode
McSimplePre::mc_node_create(const rid_t rid,
                            const PortMap &portmap,
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
  l2_entries.insert(std::make_pair(l2_hdl, L2Entry(*l1_hdl, portmap)));
  Logger::get()->debug("node created for rid {}", rid);
  return SUCCESS;
}

McSimplePre::McReturnCode
McSimplePre::mc_node_associate(const mgrp_hdl_t mgrp_hdl,
                               const l1_hdl_t l1_hdl) {
  boost::unique_lock<boost::shared_mutex> lock(mutex);
  if (!l1_handles.valid_handle(l1_hdl)) {
    Logger::get()->error("node associate failed, invalid l1 handle");
    return INVALID_L1_HANDLE;
  }
  MgidEntry &mgid_entry = mgid_entries[mgrp_hdl];
  L1Entry &l1_entry = l1_entries[l1_hdl];
  mgid_entry.l1_list.push_back(l1_hdl);
  l1_entry.mgrp_hdl = mgrp_hdl;
  Logger::get()->debug("node associated with mgid {}", mgrp_hdl);
  return SUCCESS;
}

McSimplePre::McReturnCode
McSimplePre::mc_node_dissociate(const mgrp_hdl_t mgrp_hdl,
                                const l1_hdl_t l1_hdl) {
  boost::unique_lock<boost::shared_mutex> lock(mutex);
  if (!l1_handles.valid_handle(l1_hdl)) {
    Logger::get()->error("node dissociate failed, invalid l1 handle");
    return INVALID_L1_HANDLE;
  }
  MgidEntry &mgid_entry = mgid_entries[mgrp_hdl];
  L1Entry &l1_entry = l1_entries[l1_hdl];
  mgid_entry.l1_list.erase(std::remove(mgid_entry.l1_list.begin(),
                                       mgid_entry.l1_list.end(),
                                       l1_hdl),
                           mgid_entry.l1_list.end());
  l1_entry.mgrp_hdl = 0;
  Logger::get()->debug("node dissociated with mgid {}", mgrp_hdl);
  return SUCCESS;
}

McSimplePre::McReturnCode
McSimplePre::mc_node_destroy(const l1_hdl_t l1_hdl) {
  boost::unique_lock<boost::shared_mutex> lock(mutex);
  rid_t rid;
  if (!l1_handles.valid_handle(l1_hdl)) {
    Logger::get()->error("node destroy failed, invalid l1 handle");
    return INVALID_L1_HANDLE;
  }
  L1Entry &l1_entry = l1_entries[l1_hdl];
  rid = l1_entry.rid;
  l2_entries.erase(l1_entry.l2_hdl);
  if (l2_handles.release_handle(l1_entry.l2_hdl)) {
    Logger::get()->error("node destroy failed, invalid l2 handle");
    return INVALID_L2_HANDLE;
  }
  l1_entries.erase(l1_hdl);
  assert(!l1_handles.release_handle(l1_hdl));
  Logger::get()->debug("node destroyed for rid {}", rid);
  return SUCCESS;
}

McSimplePre::McReturnCode
McSimplePre::mc_node_update(const l1_hdl_t l1_hdl,
                            const PortMap &port_map) {
  boost::unique_lock<boost::shared_mutex> lock(mutex);
  if (!l1_handles.valid_handle(l1_hdl)) {
    Logger::get()->error("node update failed, invalid l1 handle");
    return INVALID_L1_HANDLE;
  }
  L1Entry &l1_entry = l1_entries[l1_hdl];
  l2_hdl_t l2_hdl = l1_entry.l2_hdl;
  L2Entry &l2_entry = l2_entries[l2_hdl];
  l2_entry.port_map = port_map;
  Logger::get()->debug("node updated for rid {}", l1_entry.rid);
  return SUCCESS;
}

void
McSimplePre::get_entries_common(Json::Value *root) const {
  Json::Value mgrps(Json::arrayValue);
  for (const auto p : mgid_entries) {
    Json::Value mgrp(Json::objectValue);
    mgrp["id"] = Json::Value(Json::UInt(p.first));

    Json::Value l1_handles(Json::arrayValue);
    for (const auto h : p.second.l1_list) {
      l1_handles.append(Json::Value(Json::UInt(h)));
    }
    mgrp["l1_handles"] = l1_handles;

    mgrps.append(mgrp);
  }

  (*root)["mgrps"] = mgrps;

  Json::Value l1_handles(Json::arrayValue);
  for (const auto p : l1_entries) {
    Json::Value handle(Json::objectValue);
    handle["handle"] = Json::Value(Json::UInt(p.first));
    handle["rid"] = Json::Value(Json::UInt(p.second.rid));
    handle["l2_handle"] = Json::Value(Json::UInt(p.second.l2_hdl));

    l1_handles.append(handle);
  }

  (*root)["l1_handles"] = l1_handles;

  Json::Value l2_handles(Json::arrayValue);
  for (const auto p : l2_entries) {
    Json::Value handle(Json::objectValue);
    handle["handle"] = Json::Value(Json::UInt(p.first));
    const auto &entry = p.second;

    Json::Value ports(Json::arrayValue);
    for (size_t i = 0; i < entry.port_map.size(); i++) {
      if (entry.port_map[i]) ports.append(Json::Value(Json::UInt(i)));
    }
    handle["ports"] = ports;

    Json::Value lags(Json::arrayValue);
    for (size_t i = 0; i < entry.lag_map.size(); i++) {
      if (entry.lag_map[i]) lags.append(Json::Value(Json::UInt(i)));
    }
    handle["lags"] = lags;

    l2_handles.append(handle);
  }

  (*root)["l2_handles"] = l2_handles;
}

std::string
McSimplePre::mc_get_entries() const {
  Json::Value root(Json::objectValue);

  {
    boost::unique_lock<boost::shared_mutex> lock(mutex);
    get_entries_common(&root);
  }

  std::stringstream ss;
  ss << root;
  return ss.str();
}

void
McSimplePre::reset_state_() {
  mgid_entries.clear();
  l1_entries.clear();
  l2_entries.clear();
  l1_handles.clear();
  l2_handles.clear();
}

void
McSimplePre::reset_state() {
  Logger::get()->debug("resetting PRE state");
  boost::unique_lock<boost::shared_mutex> lock(mutex);
  reset_state_();
}

std::vector<McSimplePre::McOut>
McSimplePre::replicate(const McSimplePre::McIn ingress_info) const {
  std::vector<McSimplePre::McOut> egress_info_list;
  egress_port_t port_id;
  McSimplePre::McOut egress_info;
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
    const L2Entry &l2_entry = l2_entries.at(l1_entry.l2_hdl);
    // Port replication
    for (port_id = 0; port_id < l2_entry.port_map.size(); port_id++) {
      if (l2_entry.port_map[port_id]) {
        egress_info.egress_port = port_id;
        egress_info_list.push_back(egress_info);
      }
    }
  }
  BMLOG_DEBUG("number of packets replicated : {}", egress_info_list.size());
  return egress_info_list;
}

}  // namespace bm
