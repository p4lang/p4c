#ifndef _BM_PRE_H_
#define _BM_PRE_H_

#include <string>
#include <unordered_map>
#include <bitset>

#include "handle_mgr.h"
#include <boost/thread/shared_mutex.hpp>

class McPre {
    public:
        typedef unsigned int mgrp_t;
        typedef unsigned int rid_t;
        typedef unsigned int egress_port_t;
        typedef uintptr_t mgrp_hdl_t;
        typedef uintptr_t l1_hdl_t;
        typedef uintptr_t l2_hdl_t;
        static constexpr int PORT_MAP_SIZE = 512;

    private:
        static constexpr int MGID_TABLE_SIZE = 4096;
        static constexpr int L1_MAX_ENTRIES = 4096;
        static constexpr int L2_MAX_ENTRIES = 8192;

        struct L1Entry {
            mgrp_hdl_t mgrp_hdl;
            rid_t rid;
            l2_hdl_t l2_hdl;

            L1Entry() {}
            L1Entry(rid_t rid) : rid(rid) {}
        };

        struct L2Entry {
            l1_hdl_t l1_hdl;
            std::bitset<PORT_MAP_SIZE> port_map;

            L2Entry() {}
            L2Entry(l1_hdl_t l1_hdl, const std::bitset<PORT_MAP_SIZE> &port_map)
	    : l1_hdl(l1_hdl), port_map(port_map) {}
        };

        struct MgidEntry {
            mgrp_t mgid;
            std::vector<l1_hdl_t> l1_list;

            MgidEntry() {}
            MgidEntry(mgrp_t mgid) : mgid(mgid) {}
        };

        std::unordered_map<mgrp_hdl_t, MgidEntry> mgid_entries;
        std::unordered_map<l1_hdl_t, L1Entry> l1_entries;
        std::unordered_map<l2_hdl_t, L2Entry> l2_entries;
        HandleMgr l1_handles;
        HandleMgr l2_handles;
        mutable boost::shared_mutex mgid_lock;
        mutable boost::shared_mutex l1_lock;
        mutable boost::shared_mutex l2_lock;

    public:

        enum McReturnCode {
            SUCCESS = 0,
            TABLE_FULL,
            INVALID_MGID,
            INVALID_L1_HANDLE,
            INVALID_L2_HANDLE,
            ERROR
        };

        struct McPre_In {
            mgrp_t mgid;
        };

        struct McPre_Out {
            rid_t rid;
            egress_port_t egress_port;
        };

        McPre() {}
        McReturnCode mc_mgrp_create(const mgrp_t, mgrp_hdl_t *);
        McReturnCode mc_mgrp_destroy(const mgrp_hdl_t);
        McReturnCode mc_l1_node_create(const rid_t, l1_hdl_t *);
        McReturnCode mc_l1_node_associate(const mgrp_hdl_t, const l1_hdl_t);
        McReturnCode mc_l1_node_destroy(const l1_hdl_t);
        McReturnCode mc_l2_node_create(const l1_hdl_t, l2_hdl_t *,
				       const std::bitset<PORT_MAP_SIZE> &port_map);
        McReturnCode mc_l2_node_update(const l2_hdl_t l2_hdl,
				       const std::bitset<PORT_MAP_SIZE> &port_map);
        McReturnCode mc_l2_node_destroy(const l2_hdl_t);
        std::vector<McPre_Out> replicate(const McPre_In);
};
#endif
