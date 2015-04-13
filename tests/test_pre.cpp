#include <gtest/gtest.h>
#include "bm_sim/pre.h"
#include <bitset>

TEST(McPre, Replicate)
{
    McPre pre;
    McPre::mgrp_t mgid = 0x400;
    McPre::mgrp_hdl_t mgrp_hdl;
    std::vector<McPre::l1_hdl_t> l1_hdl_list;
    std::vector<McPre::l2_hdl_t> l2_hdl_list;
    McPre::rid_t rid_list[] = {0x200, 0x211, 0x221};
    std::vector<std::vector<McPre::egress_port_t>> port_list = {{1, 4}, {5, 6}, {2, 7, 8, 9}};
    McPre::McReturnCode rc;
    McPre::McPre_In ingress_info;
    std::vector<McPre::McPre_Out> egress_info;

    rc = pre.mc_mgrp_create(mgid, &mgrp_hdl);
    ASSERT_EQ(rc, McPre::SUCCESS);

    for (unsigned int i = 0; i < 3; i++) {
        McPre::l1_hdl_t l1_hdl;
        rc = pre.mc_l1_node_create(rid_list[i], &l1_hdl);
        ASSERT_EQ(rc, McPre::SUCCESS);
        l1_hdl_list.push_back(l1_hdl);
        rc = pre.mc_l1_node_associate(mgrp_hdl, l1_hdl);
        ASSERT_EQ(rc, McPre::SUCCESS);
    }

    std::bitset<McPre::PORT_MAP_SIZE> port_map[3];
    for (unsigned int i = 0; i < 3; i++) {
        for (unsigned int j = 0; j < port_list[i].size(); j++) {
            port_map[i][port_list[i][j]] = 1;
        }
        McPre::l2_hdl_t l2_hdl;
        rc = pre.mc_l2_node_create(l1_hdl_list[i], &l2_hdl, &port_map[i]);
        ASSERT_EQ(rc, McPre::SUCCESS);
        l2_hdl_list.push_back(l2_hdl);
    }

    ingress_info.mgid = mgid;
    egress_info = pre.replicate(ingress_info);
    int count = 0;
    for (unsigned int i = 0; i < 3; i++) {
        for (unsigned int j = 0; j < port_list[i].size(); j++) {
            ASSERT_EQ(egress_info[count].rid, rid_list[i]);
            ASSERT_EQ(egress_info[count].egress_port, port_list[i][j]);
            count++;
        }
    }
    ASSERT_EQ(egress_info.size(), count);
    //TODO: Add testcase for l2 node update
    for (unsigned int i = 0; i < l1_hdl_list.size(); i++) {
        rc = pre.mc_l2_node_destroy(l2_hdl_list[i]);
        ASSERT_EQ(rc, McPre::SUCCESS);
        rc = pre.mc_l1_node_destroy(l1_hdl_list[i]);
        ASSERT_EQ(rc, McPre::SUCCESS);
    }
    rc = pre.mc_mgrp_destroy(mgrp_hdl);
    ASSERT_EQ(rc, McPre::SUCCESS);
}
