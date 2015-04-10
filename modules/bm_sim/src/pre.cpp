#include "bm_sim/pre.h"

using std::vector;
using std::copy;
using std::string;

McPre::McReturnCode
McPre::mc_mgrp_create(const mgrp_t mgid, mgrp_hdl_t *mgrp_hdl)
{
    boost::unique_lock<boost::shared_mutex> lock(mgid_mutex);
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
    boost::unique_lock<boost::shared_mutex> lock(mgid_mutex);
    mgid_entries.erase(mgrp_hdl);
    return SUCCESS;
}

McPre::McReturnCode
McPre::mc_l1_node_create(const rid_t rid, l1_hdl_t *l1_hdl)
{
    boost::unique_lock<boost::shared_mutex> lock(l1_mutex);
    size_t num_entries = l1_entries.size();
    if (num_entries >= L1_MAX_ENTRIES) return TABLE_FULL;
    if (l1_handles.get_handle(l1_hdl)) return ERROR;
    L1Entry l1_entry(rid);
    l1_entries.insert(std::make_pair(*l1_hdl, std::move(l1_entry)));
    return SUCCESS;
}

McPre::McReturnCode
McPre::mc_l1_node_associate(const mgrp_hdl_t mgrp_hdl, const l1_hdl_t l1_hdl)
{
    boost::unique_lock<boost::shared_mutex> lock1(mgid_mutex);
    boost::unique_lock<boost::shared_mutex> lock2(l1_mutex);
    if (!l1_handles.valid_handle(l1_hdl)) return INVALID_HANDLE;
    auto mgid_entry_it = mgid_entries.find(mgrp_hdl);
    auto l1_entry_it = l1_entries.find(l1_hdl);
    if (mgid_entry_it == mgid_entries.end()) return ERROR;
    if (l1_entry_it == l1_entries.end()) return ERROR;
    MgidEntry mgid_entry = mgid_entry_it->second;
    L1Entry l1_entry = l1_entry_it->second;
    mgid_entry.l1_list.push_back(l1_entry_it->first);
    l1_entry.mgrp_hdl = mgrp_hdl;
    l1_entry_it->second = l1_entry;
    mgid_entry_it->second = mgid_entry;
    return SUCCESS;
}

McPre::McReturnCode
McPre::mc_l1_node_destroy(const l1_hdl_t l1_hdl)
{
    boost::unique_lock<boost::shared_mutex> lock(l1_mutex);
    if (!l1_handles.valid_handle(l1_hdl)) return INVALID_HANDLE;
    l1_entries.erase(l1_hdl);
    if (l1_handles.release_handle(l1_hdl)) return INVALID_HANDLE;
    return SUCCESS;
}

McPre::McReturnCode
McPre::mc_l2_node_create(const l1_hdl_t l1_hdl, l2_hdl_t *l2_hdl, std::bitset<512> port_map)
{
    boost::unique_lock<boost::shared_mutex> lock1(l1_mutex);
    boost::unique_lock<boost::shared_mutex> lock2(l2_mutex);
    size_t num_entries = l2_entries.size();
    if (num_entries >= L2_MAX_ENTRIES) return TABLE_FULL;
    if (l2_handles.get_handle(l2_hdl)) return ERROR;
    if (!l1_handles.valid_handle(l1_hdl)) return INVALID_HANDLE;
    auto l1_entry_it = l1_entries.find(l1_hdl);
    L1Entry l1_entry = l1_entry_it->second;
    l1_entry.l2_hdl = *l2_hdl;
    l1_entry_it->second = l1_entry;
    L2Entry l2_entry(l1_hdl, port_map);
    l2_entries.insert(std::make_pair(*l2_hdl, std::move(l2_entry)));
    return SUCCESS;
}

McPre::McReturnCode
McPre::mc_l2_node_update(const l2_hdl_t l2_hdl, std::bitset<512> port_map)
{
    boost::unique_lock<boost::shared_mutex> lock2(l2_mutex);
    if (!l2_handles.valid_handle(l2_hdl)) return INVALID_HANDLE;
    auto l2_entry_it = l2_entries.find(l2_hdl);
    if (l2_entry_it == l2_entries.end()) return ERROR;
    L2Entry l2_entry = l2_entry_it->second;
    l2_entry.port_map = port_map;
    l2_entry_it->second = l2_entry;
    return SUCCESS;
}

McPre::McReturnCode
McPre::mc_l2_node_destroy(const l2_hdl_t l2_hdl)
{
    boost::unique_lock<boost::shared_mutex> lock2(l2_mutex);
    if (!l2_handles.valid_handle(l2_hdl)) return INVALID_HANDLE;
    l2_entries.erase(l2_hdl);
    if (l2_handles.release_handle(l2_hdl)) return INVALID_HANDLE;
    return SUCCESS;
}

std::vector<McPre_Out>
McPre::replicate(const McPre_In ingress_info)
{
    std::vector<McPre_Out> egress_info_list;
    egress_port_t port_id;
    McPre_Out egress_info;
    boost::shared_lock<boost::shared_mutex> lock1(mgid_mutex);
    boost::shared_lock<boost::shared_mutex> lock2(l1_mutex);
    boost::shared_lock<boost::shared_mutex> lock3(l2_mutex);
    auto mgid_entry_it = mgid_entries.find(ingress_info.mgid);
    MgidEntry mgid_entry = mgid_entry_it->second;
    for (std::vector<l1_hdl_t>::iterator it = mgid_entry.l1_list.begin();
         it != mgid_entry.l1_list.end(); ++it) {
        auto l1_entry_it = l1_entries.find(*it);
        L1Entry l1_entry = l1_entry_it->second;
        auto l2_entry_it = l2_entries.find(l1_entry.l2_hdl);
        egress_info.rid = l1_entry.rid;
        L2Entry l2_entry = l2_entry_it->second;
        for (port_id = 0; port_id < 512; port_id++) {
            if (l2_entry.port_map[port_id]) {
                egress_info.egress_port = port_id;
                egress_info_list.push_back(egress_info);
            }
        }
    }
    return egress_info_list;
}
