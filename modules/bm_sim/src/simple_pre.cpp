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

#include "bm_sim/simple_pre.h"

using std::vector;
using std::copy;
using std::string;

McSimplePre::McReturnCode
McSimplePre::mc_mgrp_create(const mgrp_t mgid, mgrp_hdl_t *mgrp_hdl)
{
  boost::unique_lock<boost::shared_mutex> lock(mgid_lock);
  size_t num_entries = mgid_entries.size();
  if (num_entries >= MGID_TABLE_SIZE) {
    std::cout << "mgrp create failed. mgid table full!\n";
    return TABLE_FULL;
  }
  *mgrp_hdl = mgid;
  MgidEntry mgid_entry(mgid);
  mgid_entries.insert(std::make_pair(*mgrp_hdl, std::move(mgid_entry)));
  if (pre_debug) {
    std::cout << "mgrp node created for mgid : " << mgid << "\n";
  }
  return SUCCESS;
}

McSimplePre::McReturnCode
McSimplePre::mc_mgrp_destroy(mgrp_hdl_t mgrp_hdl)
{
  boost::unique_lock<boost::shared_mutex> lock(mgid_lock);
  mgid_entries.erase(mgrp_hdl);
  if (pre_debug) {
    std::cout << "mgrp node deleted for mgid : " << mgrp_hdl << "\n";
  }
  return SUCCESS;
}

McSimplePre::McReturnCode
McSimplePre::mc_node_create(const rid_t rid,
                            const PortMap &portmap,
                            l1_hdl_t *l1_hdl)
{
  boost::unique_lock<boost::shared_mutex> lock1(l1_lock);
  boost::unique_lock<boost::shared_mutex> lock2(l2_lock);
  l2_hdl_t l2_hdl;
  size_t num_l1_entries = l1_entries.size();
  size_t num_l2_entries = l2_entries.size();
  if (num_l1_entries >= L1_MAX_ENTRIES) {
    std::cout << "node create failed. l1 table full!\n"; 
    return TABLE_FULL;
  }
  if (num_l2_entries >= L2_MAX_ENTRIES) {
    std::cout << "node create failed. l2 table full!\n"; 
    return TABLE_FULL;
  }
  if (l1_handles.get_handle(l1_hdl)) {
    std::cout << "node create failed. l1 handle failed!\n";
    return ERROR;
  }
  if (l2_handles.get_handle(&l2_hdl)) {
    std::cout << "node create failed. l2 handle failed!\n";
    return ERROR;
  }
  l1_entries.insert(std::make_pair(*l1_hdl, L1Entry(rid)));
  L1Entry &l1_entry = l1_entries[*l1_hdl];
  l1_entry.l2_hdl = l2_hdl;
  l2_entries.insert(std::make_pair(l2_hdl, L2Entry(*l1_hdl, portmap)));
  if (pre_debug) {
    std::cout << "node created for rid : " << rid << "\n";
  }
  return SUCCESS;
}

McSimplePre::McReturnCode
McSimplePre::mc_node_associate(const mgrp_hdl_t mgrp_hdl, const l1_hdl_t l1_hdl)
{
  boost::unique_lock<boost::shared_mutex> lock1(mgid_lock);
  boost::unique_lock<boost::shared_mutex> lock2(l1_lock);
  if (!l1_handles.valid_handle(l1_hdl)) {
    std::cout << "node associate failed. invalid l1 handle\n";
    return INVALID_L1_HANDLE;
  }
  MgidEntry &mgid_entry = mgid_entries[mgrp_hdl];
  L1Entry &l1_entry = l1_entries[l1_hdl];
  mgid_entry.l1_list.push_back(l1_hdl);
  l1_entry.mgrp_hdl = mgrp_hdl;
  if (pre_debug) {
    std::cout << "node associated with mgid : " << mgrp_hdl << "\n";
  }
  return SUCCESS;
}

McSimplePre::McReturnCode
McSimplePre::mc_node_dissociate(const mgrp_hdl_t mgrp_hdl, const l1_hdl_t l1_hdl)
{
  boost::unique_lock<boost::shared_mutex> lock1(mgid_lock);
  boost::unique_lock<boost::shared_mutex> lock2(l1_lock);
  if (!l1_handles.valid_handle(l1_hdl)) {
    std::cout << "node dissociate failed. invalid l1 handle\n";
    return INVALID_L1_HANDLE;
  }
  MgidEntry &mgid_entry = mgid_entries[mgrp_hdl];
  L1Entry &l1_entry = l1_entries[l1_hdl];
  mgid_entry.l1_list.erase(std::remove(mgid_entry.l1_list.begin(),
                                       mgid_entry.l1_list.end(),
                                       l1_hdl),
                           mgid_entry.l1_list.end());
  l1_entry.mgrp_hdl = 0;
  if (pre_debug) {
    std::cout << "node dissociated with mgid : " << mgrp_hdl << "\n";
  }
  return SUCCESS;
}

McSimplePre::McReturnCode
McSimplePre::mc_node_destroy(const l1_hdl_t l1_hdl)
{
  boost::unique_lock<boost::shared_mutex> lock1(l1_lock);
  boost::unique_lock<boost::shared_mutex> lock2(l2_lock);
  rid_t rid;
  if (!l1_handles.valid_handle(l1_hdl)) {
    std::cout << "node destroy failed. invalid l1 handle!\n";
    return INVALID_L1_HANDLE;
  }
  L1Entry &l1_entry = l1_entries[l1_hdl];
  rid = l1_entry.rid;
  l2_entries.erase(l1_entry.l2_hdl);
  if (l2_handles.release_handle(l1_entry.l2_hdl)) {
    std::cout << "node destroy failed. invalid l2 handle!\n";
    return INVALID_L2_HANDLE;
  }
  l1_entries.erase(l1_hdl);
  if (l1_handles.release_handle(l1_hdl)) {
    std::cout << "node destroy failed. invalid l2 handle!\n";
    return INVALID_L1_HANDLE;
  }
  if (pre_debug) {
    std::cout << "node destroyed for rid : " << rid << "\n";
  }
  return SUCCESS;
}

McSimplePre::McReturnCode
McSimplePre::mc_node_update(const l1_hdl_t l1_hdl,
			    const PortMap &port_map)
{
  boost::unique_lock<boost::shared_mutex> lock1(l1_lock);
  boost::unique_lock<boost::shared_mutex> lock2(l2_lock);
  if (!l1_handles.valid_handle(l1_hdl)) {
    std::cout << "node update failed. invalid l1 handle!\n";
    return INVALID_L1_HANDLE;
  }
  L1Entry &l1_entry = l1_entries[l1_hdl];
  l2_hdl_t l2_hdl = l1_entry.l2_hdl;
  L2Entry &l2_entry = l2_entries[l2_hdl];
  l2_entry.port_map = port_map;
  if (pre_debug) {
    std::cout << "node updated for rid : " << l1_entry.rid << "\n";
  }
  return SUCCESS;
}

std::vector<McSimplePre::McOut>
McSimplePre::replicate(const McSimplePre::McIn ingress_info) const
{
  std::vector<McSimplePre::McOut> egress_info_list;
  egress_port_t port_id;
  McSimplePre::McOut egress_info;
  boost::shared_lock<boost::shared_mutex> lock1(mgid_lock);
  boost::shared_lock<boost::shared_mutex> lock2(l1_lock);
  boost::shared_lock<boost::shared_mutex> lock3(l2_lock);
  const MgidEntry mgid_entry = mgid_entries.at(ingress_info.mgid);
  for (l1_hdl_t l1_hdl : mgid_entry.l1_list) {
    const L1Entry l1_entry = l1_entries.at(l1_hdl);
    egress_info.rid = l1_entry.rid;
    const L2Entry l2_entry = l2_entries.at(l1_entry.l2_hdl);
    // Port replication
    for (port_id = 0; port_id < l2_entry.port_map.size(); port_id++) {
      if (l2_entry.port_map[port_id]) {
	egress_info.egress_port = port_id;
	egress_info_list.push_back(egress_info);
      }
    }
  }
  if (pre_debug) {
    std::cout << "number of packets replicated : " << egress_info_list.size() << "\n";
  }
  return egress_info_list;
}
