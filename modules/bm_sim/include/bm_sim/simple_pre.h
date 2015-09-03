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

#ifndef _BM_SIMPLE_PRE_H_
#define _BM_SIMPLE_PRE_H_

#include <unordered_map>

#include "handle_mgr.h"
#include "pre.h"
#include <boost/thread/shared_mutex.hpp>

class McSimplePre {
public:
  typedef unsigned int mgrp_t;
  typedef unsigned int rid_t;
  typedef unsigned int egress_port_t;
  typedef uintptr_t mgrp_hdl_t;
  typedef uintptr_t l1_hdl_t;
  typedef uintptr_t l2_hdl_t;
  
public:
  enum McReturnCode {
    SUCCESS = 0,
    TABLE_FULL,
    INVALID_MGID,
    INVALID_L1_HANDLE,
    INVALID_L2_HANDLE,
    ERROR
  };
  
  struct McIn {
    mgrp_t mgid;
  };
  
  struct McOut {
    rid_t rid;
    egress_port_t egress_port;
  };

public:
  static constexpr size_t PORT_MAP_SIZE = 256;
  typedef McPre::Set<PORT_MAP_SIZE> PortMap;

  static constexpr size_t LAG_MAP_SIZE = 256;
  typedef McPre::Set<LAG_MAP_SIZE> LagMap;
  
  McSimplePre() {}
  McReturnCode mc_mgrp_create(const mgrp_t, mgrp_hdl_t *);
  McReturnCode mc_mgrp_destroy(const mgrp_hdl_t);
  McReturnCode mc_node_create(const rid_t,
		              const PortMap &port_map,
                              l1_hdl_t *l1_hdl);
  McReturnCode mc_node_associate(const mgrp_hdl_t, const l1_hdl_t);
  McReturnCode mc_node_dissociate(const mgrp_hdl_t, const l1_hdl_t);
  McReturnCode mc_node_disassociate(const mgrp_hdl_t, const l1_hdl_t);
  McReturnCode mc_node_destroy(const l1_hdl_t);
  McReturnCode mc_node_update(const l1_hdl_t l1_hdl,
			      const PortMap &port_map);
  std::vector<McOut> replicate(const McIn) const;
  
  McSimplePre(const McSimplePre &other) = delete;
  McSimplePre &operator=(const McSimplePre &other) = delete;
  McSimplePre(McSimplePre &&other) = delete;
  McSimplePre &operator=(McSimplePre &&other) = delete;

protected:
  static constexpr int MGID_TABLE_SIZE = 4096;
  static constexpr int L1_MAX_ENTRIES = 4096;
  static constexpr int L2_MAX_ENTRIES = 8192;
  bool pre_debug{0};
  
  struct MgidEntry {
    mgrp_t mgid{};
    std::vector<l1_hdl_t> l1_list{};
    
    MgidEntry() {}
    MgidEntry(mgrp_t mgid) : mgid(mgid) {}
  };

  struct L1Entry {
    mgrp_hdl_t mgrp_hdl{};
    rid_t rid{};
    l2_hdl_t l2_hdl{};
    
    L1Entry() {}
    L1Entry(rid_t rid) : rid(rid) {}
  };

  struct L2Entry {
    l1_hdl_t l1_hdl{};
    PortMap port_map{};
    LagMap lag_map{};
    
    L2Entry() {}
    L2Entry(l1_hdl_t l1_hdl,
            const PortMap &port_map) :
            l1_hdl(l1_hdl),
            port_map(port_map) {}

    L2Entry(l1_hdl_t l1_hdl,
            const PortMap &port_map,
            const LagMap &lag_map) :
            l1_hdl(l1_hdl),
            port_map(port_map),
            lag_map(lag_map) {}
  };

  std::unordered_map<mgrp_hdl_t, MgidEntry> mgid_entries{};
  std::unordered_map<l1_hdl_t, L1Entry> l1_entries{};
  std::unordered_map<l2_hdl_t, L2Entry> l2_entries{};
  HandleMgr l1_handles{};
  HandleMgr l2_handles{};
  mutable boost::shared_mutex mgid_lock{};
  mutable boost::shared_mutex l1_lock{};
  mutable boost::shared_mutex l2_lock{};
};

#endif
