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

//! @file simple_pre.h

#ifndef BM_BM_SIM_SIMPLE_PRE_H_
#define BM_BM_SIM_SIMPLE_PRE_H_

#include <boost/thread/shared_mutex.hpp>

#include <unordered_map>
#include <string>
#include <vector>

#include "handle_mgr.h"
#include "pre.h"

// forward declaration of Json::Value
namespace Json {

class Value;

}  // namespace Json

namespace bm {

//! A simple, 2-level, packet replication engine, configurable by the control
//! plane. See replicate() for more information.
class McSimplePre {
 public:
  //! NC
  typedef unsigned int mgrp_t;
  //! NC
  typedef unsigned int rid_t;
  //! NC
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

  //! Input to replicate() method
  struct McIn {
    //! Multicast group id to use for replication
    mgrp_t mgid;
  };

  //! Output of replicate() method
  struct McOut {
    //! replication id of multicast copy
    rid_t rid;
    //! egress port of multicast copy
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

  std::string mc_get_entries() const;

  void reset_state();

  //! This is the only "dataplane" method for this class. It takes as input a
  //! multicast group id (set during pipeline processing) and returns a vector
  //! of McOut instances (rid + egress port).
  //!
  //! The mgid (multicast group id) points to a L1 (level 1) node. Each L1 node
  //! has a unique rid and points to a L2 node. The L2 node includes a list of
  //! ports which we return in a vector (along with thr L1 node rid). It is the
  //! target responsibility to use this information to properly "clone" the
  //! packet. For example:
  //! @code
  //! Field &f_mgid = phv->get_field("intrinsic_metadata.mcast_grp");
  //! unsigned int mgid = f_mgid.get_uint();
  //! if (mgid != 0) {
  //!   const auto pre_out = pre->replicate({mgid});
  //!   for (const auto &out : pre_out) {
  //!     egress_port = out.egress_port;
  //!     std::unique_ptr<Packet> packet_copy = packet->clone_with_phv_ptr();
  //!     // ...
  //!     // send packet_copy to egress_port
  //!   }
  //! }
  //! @endcode
  std::vector<McOut> replicate(const McIn) const;

  //! Deleted copy constructor
  McSimplePre(const McSimplePre &other) = delete;
  //! Deleted move assignment operator
  McSimplePre &operator=(const McSimplePre &other) = delete;

  //! Deleted move constructor
  McSimplePre(McSimplePre &&other) = delete;
  //! Deleted move assignment operator
  McSimplePre &operator=(McSimplePre &&other) = delete;

 protected:
  static constexpr int MGID_TABLE_SIZE = 4096;
  static constexpr int L1_MAX_ENTRIES = 4096;
  static constexpr int L2_MAX_ENTRIES = 8192;

  struct MgidEntry {
    mgrp_t mgid{};
    std::vector<l1_hdl_t> l1_list{};

    MgidEntry() {}
    explicit MgidEntry(mgrp_t mgid) : mgid(mgid) {}
  };

  struct L1Entry {
    mgrp_hdl_t mgrp_hdl{};
    rid_t rid{};
    l2_hdl_t l2_hdl{};

    L1Entry() {}
    explicit L1Entry(rid_t rid) : rid(rid) {}
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

  // internal version, which does not acquire the lock
  void reset_state_();

  // does not acquire lock
  void get_entries_common(Json::Value *root) const;

  std::unordered_map<mgrp_hdl_t, MgidEntry> mgid_entries{};
  std::unordered_map<l1_hdl_t, L1Entry> l1_entries{};
  std::unordered_map<l2_hdl_t, L2Entry> l2_entries{};
  HandleMgr l1_handles{};
  HandleMgr l2_handles{};
  mutable boost::shared_mutex mutex{};
};

}  // namespace bm

#endif  // BM_BM_SIM_SIMPLE_PRE_H_
