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
 *
 */

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
  std::vector<std::vector<McPre::egress_port_t>> port_list1 = {{1, 4}, {5, 6}, {2, 7, 8, 9}};
  std::vector<std::vector<McPre::egress_port_t>> port_list2 = {{1, 4}, {5, 6}, {2, 9}};
  McPre::McReturnCode rc;
  McPre::McPre_In ingress_info;
  std::vector<McPre::McPre_Out> egress_info;
  unsigned int count = 0;

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

  McPre::PortMap port_maps[3];
  for (unsigned int i = 0; i < 3; i++) {
    McPre::PortMap &port_map = port_maps[i];
    for (unsigned int j = 0; j < port_list1[i].size(); j++) {
      port_map[port_list1[i][j]] = 1;
    }
    McPre::l2_hdl_t l2_hdl;
    rc = pre.mc_l2_node_create(l1_hdl_list[i], &l2_hdl, port_map);
    ASSERT_EQ(rc, McPre::SUCCESS);
    l2_hdl_list.push_back(l2_hdl);
  }

  ingress_info.mgid = mgid;
  egress_info = pre.replicate(ingress_info);
  count = 0;
  for (unsigned int i = 0; i < 3; i++) {
    for (unsigned int j = 0; j < port_list1[i].size(); j++) {
      ASSERT_EQ(egress_info[count].rid, rid_list[i]);
      ASSERT_EQ(egress_info[count].egress_port, port_list1[i][j]);
      count++;
    }
  }
  ASSERT_EQ(egress_info.size(), count);

  McPre::PortMap &port_map_2 = port_maps[2];
  port_map_2[7] = 0;
  port_map_2[8] = 0;
  rc = pre.mc_l2_node_update(l2_hdl_list[2], port_map_2);
  ASSERT_EQ(rc, McPre::SUCCESS);
  ingress_info.mgid = mgid;
  
  egress_info = pre.replicate(ingress_info);
  count = 0;
  for (unsigned int i = 0; i < 3; i++) {
    for (unsigned int j = 0; j < port_list2[i].size(); j++) {
      ASSERT_EQ(egress_info[count].rid, rid_list[i]);
      ASSERT_EQ(egress_info[count].egress_port, port_list2[i][j]);
      count++;
    }
  }
  ASSERT_EQ(egress_info.size(), count);
  
  for (unsigned int i = 0; i < l1_hdl_list.size(); i++) {
    rc = pre.mc_l2_node_destroy(l2_hdl_list[i]);
    ASSERT_EQ(rc, McPre::SUCCESS);
    rc = pre.mc_l1_node_destroy(l1_hdl_list[i]);
    ASSERT_EQ(rc, McPre::SUCCESS);
  }
  rc = pre.mc_mgrp_destroy(mgrp_hdl);
  ASSERT_EQ(rc, McPre::SUCCESS);
}
