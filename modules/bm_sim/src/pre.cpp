#include "bm_sim/pre.h"

using std::vector;
using std::copy;
using std::string;

McPre::McReturnCode
McPre::mc_mgrp_create(const mgrp_t mgid, mgrp_hdl_t *mgrp_hdl)
{
  boost::unique_lock<boost::shared_mutex> lock(mgid_lock);
  size_t num_entries = mgid_entries.size();
  if (num_entries >= MGID_TABLE_SIZE) return TABLE_FULL;
  *mgrp_hdl = mgid;
  MgidEntry mgid_entry(mgid);
  mgid_entries.insert(std::make_pair(*mgrp_hdl, std::move(mgid_entry)));
  return SUCCESS;
}

McPre::McReturnCode
McPre::mc_mgrp_destroy(mgrp_hdl_t mgrp_hdl)
{
  boost::unique_lock<boost::shared_mutex> lock(mgid_lock);
  mgid_entries.erase(mgrp_hdl);
  return SUCCESS;
}

McPre::McReturnCode
McPre::mc_l1_node_create(const rid_t rid, l1_hdl_t *l1_hdl)
{
  boost::unique_lock<boost::shared_mutex> lock(l1_lock);
  size_t num_entries = l1_entries.size();
  if (num_entries >= L1_MAX_ENTRIES) return TABLE_FULL;
  if (l1_handles.get_handle(l1_hdl)) return ERROR;
  l1_entries.insert(std::make_pair(*l1_hdl, L1Entry(rid)));
  return SUCCESS;
}

McPre::McReturnCode
McPre::mc_l1_node_associate(const mgrp_hdl_t mgrp_hdl, const l1_hdl_t l1_hdl)
{
  boost::unique_lock<boost::shared_mutex> lock1(mgid_lock);
  boost::unique_lock<boost::shared_mutex> lock2(l1_lock);
  if (!l1_handles.valid_handle(l1_hdl)) return INVALID_L1_HANDLE;
  MgidEntry &mgid_entry = mgid_entries[mgrp_hdl];
  L1Entry &l1_entry = l1_entries[l1_hdl];
  mgid_entry.l1_list.push_back(l1_hdl);
  l1_entry.mgrp_hdl = mgrp_hdl;
  return SUCCESS;
}

McPre::McReturnCode
McPre::mc_l1_node_destroy(const l1_hdl_t l1_hdl)
{
  boost::unique_lock<boost::shared_mutex> lock(l1_lock);
  if (!l1_handles.valid_handle(l1_hdl)) return INVALID_L1_HANDLE;
  l1_entries.erase(l1_hdl);
  if (l1_handles.release_handle(l1_hdl)) return INVALID_L1_HANDLE;
  return SUCCESS;
}

McPre::McReturnCode
McPre::mc_l2_node_create(const l1_hdl_t l1_hdl, l2_hdl_t *l2_hdl,
			 const PortMap &port_map)
{
  boost::unique_lock<boost::shared_mutex> lock1(l1_lock);
  boost::unique_lock<boost::shared_mutex> lock2(l2_lock);
  size_t num_entries = l2_entries.size();
  if (num_entries >= L2_MAX_ENTRIES) return TABLE_FULL;
  if (l2_handles.get_handle(l2_hdl)) return ERROR;
  if (!l1_handles.valid_handle(l1_hdl)) return INVALID_L1_HANDLE;
  L1Entry &l1_entry = l1_entries[l1_hdl];
  l1_entry.l2_hdl = *l2_hdl;
  l2_entries.insert(std::make_pair(*l2_hdl, L2Entry(l1_hdl, port_map)));
  return SUCCESS;
}

McPre::McReturnCode
McPre::mc_l2_node_update(const l2_hdl_t l2_hdl,
			 const PortMap &port_map)
{
  boost::unique_lock<boost::shared_mutex> lock2(l2_lock);
  if (!l2_handles.valid_handle(l2_hdl)) return INVALID_L2_HANDLE;
  L2Entry &l2_entry = l2_entries[l2_hdl];
  l2_entry.port_map = port_map;
  return SUCCESS;
}

McPre::McReturnCode
McPre::mc_l2_node_destroy(const l2_hdl_t l2_hdl)
{
  boost::unique_lock<boost::shared_mutex> lock2(l2_lock);
  if (!l2_handles.valid_handle(l2_hdl)) return INVALID_L2_HANDLE;
  l2_entries.erase(l2_hdl);
  if (l2_handles.release_handle(l2_hdl)) return INVALID_L2_HANDLE;
  return SUCCESS;
}

std::vector<McPre::McPre_Out>
McPre::replicate(const McPre::McPre_In ingress_info) const
{
  std::vector<McPre::McPre_Out> egress_info_list;
  egress_port_t port_id;
  McPre::McPre_Out egress_info;
  boost::shared_lock<boost::shared_mutex> lock1(mgid_lock);
  boost::shared_lock<boost::shared_mutex> lock2(l1_lock);
  boost::shared_lock<boost::shared_mutex> lock3(l2_lock);
  const MgidEntry mgid_entry = mgid_entries.at(ingress_info.mgid);
  for (l1_hdl_t l1_hdl : mgid_entry.l1_list) {
    const L1Entry l1_entry = l1_entries.at(l1_hdl);
    egress_info.rid = l1_entry.rid;
    const L2Entry l2_entry = l2_entries.at(l1_entry.l2_hdl);
    for (port_id = 0; port_id < l2_entry.port_map.size(); port_id++) {
      if (l2_entry.port_map[port_id]) {
	egress_info.egress_port = port_id;
	egress_info_list.push_back(egress_info);
      }
    }
  }
  return egress_info_list;
}
