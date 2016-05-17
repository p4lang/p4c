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
#include <bm/bm_sim/simple_pre.h>
#include <bm/bm_sim/simple_pre_lag.h>
#include <bitset>

using namespace bm;

TEST(McSimplePre, Replicate) {
  McSimplePre pre;
  McSimplePre::mgrp_t mgid = 0x400;
  McSimplePre::mgrp_hdl_t mgrp_hdl;
  std::vector<McSimplePre::l1_hdl_t> l1_hdl_list;
  McSimplePre::rid_t rid_list[] = {0x200, 0x211, 0x221};
  std::vector<std::vector<McSimplePre::egress_port_t>> port_list1 = {{1, 4}, {5, 6}, {2, 7, 8, 9}};
  std::vector<std::vector<McSimplePre::egress_port_t>> port_list2 = {{1, 4}, {5, 6}, {2, 9}};
  McSimplePre::McReturnCode rc;
  McSimplePre::McIn ingress_info;
  std::vector<McSimplePre::McOut> egress_info;
  unsigned int count = 0;
  constexpr size_t nodes = 3;

  rc = pre.mc_mgrp_create(mgid, &mgrp_hdl);
  ASSERT_EQ(rc, McSimplePre::SUCCESS);

  McSimplePre::PortMap port_maps[nodes];
  for (unsigned int i = 0; i < nodes; i++) {
    McSimplePre::l1_hdl_t l1_hdl;
    McSimplePre::PortMap &port_map = port_maps[i];
    for (unsigned int j = 0; j < port_list1[i].size(); j++) {
      port_map[port_list1[i][j]] = 1;
    }
    rc = pre.mc_node_create(rid_list[i], port_map, &l1_hdl);
    ASSERT_EQ(rc, McSimplePre::SUCCESS);
    l1_hdl_list.push_back(l1_hdl);
    rc = pre.mc_node_associate(mgrp_hdl, l1_hdl);
    ASSERT_EQ(rc, McSimplePre::SUCCESS);
  }

  ingress_info.mgid = mgid;
/*
 *             ---------------
 *             | mgid = 0x400|
 *             ---------------
 *                    |
 *                    V
 *             ---------------    ---------------    ---------------
 *             | rid = 0x200 | -> | rid = 0x211 | -> | rid = 0x221 |
 *             ---------------    ---------------    ---------------
 *                    |                  |                  |
 *                    V                  V                  V
 * ----------   ---------------   ---------------     ---------------
 * port_map |   | 1, 4        |   | 5, 6        |     | 2, 7, 8, 9  |
 * ----------   ---------------   ---------------     ---------------
 */

  egress_info = pre.replicate(ingress_info);
  count = 0;
  for (unsigned int i = 0; i < nodes; i++) {
    for (unsigned int j = 0; j < port_list1[i].size(); j++) {
      ASSERT_EQ(egress_info[count].rid, rid_list[i]);
      ASSERT_EQ(egress_info[count].egress_port, port_list1[i][j]);
      count++;
    }
  }
  // number of replicated copies = 8
  ASSERT_EQ(egress_info.size(), count);

  // remove ports 7 and 8 from 3rd node port_map
  McSimplePre::PortMap &port_map_2 = port_maps[2];
  port_map_2[7] = 0;
  port_map_2[8] = 0;
  rc = pre.mc_node_update(l1_hdl_list[2], port_map_2);
  ASSERT_EQ(rc, McSimplePre::SUCCESS);
  ingress_info.mgid = mgid;
  
/*
 * Multicast tree view
 *
 *             ---------------
 *             | mgid = 0x400|
 *             ---------------
 *                    |
 *                    V
 *             ---------------    ---------------    ---------------
 *             | rid = 0x200 | -> | rid = 0x211 | -> | rid = 0x221 |
 *             ---------------    ---------------    ---------------
 *                    |                  |                  |
 *                    V                  V                  V
 * ----------   ---------------   ---------------     ---------------
 * port_map |   | 1, 4        |   | 5, 6        |     | 2, 9        |
 * ----------   ---------------   ---------------     ---------------
 */
  egress_info = pre.replicate(ingress_info);
  count = 0;
  for (unsigned int i = 0; i < nodes; i++) {
    for (unsigned int j = 0; j < port_list2[i].size(); j++) {
      ASSERT_EQ(egress_info[count].rid, rid_list[i]);
      ASSERT_EQ(egress_info[count].egress_port, port_list2[i][j]);
      count++;
    }
  }
  // number of replicated copies = 6
  ASSERT_EQ(egress_info.size(), count);
  
  // cleanup
  for (unsigned int i = 0; i < l1_hdl_list.size(); i++) {
    rc = pre.mc_node_dissociate(mgrp_hdl, l1_hdl_list[i]);
    ASSERT_EQ(rc, McSimplePre::SUCCESS);
    rc = pre.mc_node_destroy(l1_hdl_list[i]);
    ASSERT_EQ(rc, McSimplePre::SUCCESS);
  }
  rc = pre.mc_mgrp_destroy(mgrp_hdl);
  ASSERT_EQ(rc, McSimplePre::SUCCESS);
}


TEST(McSimplePreLAG, Replicate) {
  McSimplePreLAG pre;
  McSimplePre::mgrp_t mgid = 0x400;
  McSimplePre::mgrp_hdl_t mgrp_hdl;
  std::vector<McSimplePre::l1_hdl_t> l1_hdl_list;
  McSimplePre::rid_t rid_list[] = {0x200, 0x211, 0x221};
  std::vector<std::vector<McSimplePre::egress_port_t>> port_list1 = {{1, 4}, {5, 6}, {2, 7, 8, 9}};
  std::vector<std::vector<McSimplePre::egress_port_t>> port_list2 = {{1, 4}, {5, 6}, {2, 9}};
  std::vector<std::vector<McSimplePre::egress_port_t>> lag_port_list1 = {{10, 12, 20, 21}, {11, 14, 16, 24}, {34, 38, 41, 42}};
  std::vector<std::vector<McSimplePre::egress_port_t>> lag_port_list2 = {{10, 12}, {11, 16}};
  std::vector<McSimplePreLAG::lag_id_t> lag_id_slist1 = {2, 22, 24};
  std::vector<McSimplePreLAG::lag_id_t> lag_id_slist2 = {2, 22};
  std::vector<std::vector<McSimplePreLAG::lag_id_t>> lag_id_list1 = {{2, 22}, {24}};
  std::vector<std::vector<McSimplePreLAG::lag_id_t>> lag_id_list2 = {{2, 22}};
  McSimplePre::McReturnCode rc;
  McSimplePre::McIn ingress_info;
  std::vector<McSimplePre::McOut> egress_info;
  unsigned int count = 0;
  unsigned int member_count = 0;
  constexpr unsigned int nodes = 3;

  rc = pre.mc_mgrp_create(mgid, &mgrp_hdl);
  ASSERT_EQ(rc, McSimplePre::SUCCESS);

  // Update lag membership
  for (unsigned int i = 0; i < lag_id_slist1.size(); i++) {
    McSimplePre::PortMap lag_port_map;
    for (unsigned int j = 0; j < lag_port_list1[i].size(); j++) {
      lag_port_map[lag_port_list1[i][j]] = 1;
    }
    rc = pre.mc_set_lag_membership(lag_id_slist1[i], lag_port_map);
    ASSERT_EQ(rc, McSimplePre::SUCCESS);
  }

  McSimplePre::PortMap port_maps[nodes];
  McSimplePre::LagMap lag_maps[nodes];
  for (unsigned int i = 0; i < 3; i++) {
    McSimplePre::l1_hdl_t l1_hdl;
    McSimplePre::PortMap &port_map = port_maps[i];
    for (unsigned int j = 0; j < port_list1[i].size(); j++) {
      port_map[port_list1[i][j]] = 1;
    }

    if (i < lag_id_list1.size()) {
      McSimplePre::LagMap &lag_map = lag_maps[i];
      for (unsigned int j = 0; j < lag_id_list1[i].size(); j++) {
        lag_map[lag_id_list1[i][j]] = 1;
      }
    }

    rc = pre.mc_node_create(rid_list[i], port_maps[i], lag_maps[i], &l1_hdl);
    ASSERT_EQ(rc, McSimplePre::SUCCESS);
    l1_hdl_list.push_back(l1_hdl);
    rc = pre.mc_node_associate(mgrp_hdl, l1_hdl);
    ASSERT_EQ(rc, McSimplePre::SUCCESS);
  }

/*
 * Multicast tree view
 *
 *             ---------------
 *             | mgid = 0x400|
 *             ---------------
 *                    |
 *                    V
 *             ---------------    ---------------    ---------------
 *             | rid = 0x200 | -> | rid = 0x211 | -> | rid = 0x221 |
 *             ---------------    ---------------    ---------------
 *                    |                  |                  |
 *                    V                  V                  V
 *-----------   ---------------   ---------------    ---------------
 * port_map |   | 1, 4        |   | 5, 6        |    | 2, 7, 8, 9  |
 *-----------   ---------------   ---------------    ---------------
 * lag_map  |   | 2, 22       |   | 24          |    |             |
 *-----------   ---------------   ---------------    ---------------
 *
 * Lag table
 *
 *-----------   ---------------   ---------------    ---------------
 * lag_index|   | 2           |   | 22          |    | 24          |   
 *-----------   ---------------   ---------------    --------------- 
 *                     |                 |                  |
 *                     V                 V                  V
 *-----------   ----------------  ----------------   ----------------
 * port_map |   |10, 12, 20, 21|  |11, 14, 16, 24|   |34, 38, 41, 42|
 *-----------   ----------------  ----------------   ----------------
 */
  ingress_info.mgid = mgid;
  egress_info = pre.replicate(ingress_info);

  count = 0;
  member_count = 0;
  for (unsigned int i = 0; i < 3; i++) {
    for (unsigned int j = 0; j < port_list1[i].size(); j++) {
      ASSERT_EQ(egress_info[count].rid, rid_list[i]);
      ASSERT_EQ(egress_info[count].egress_port, port_list1[i][j]);
      count++;
    }
    if (i < lag_id_list1.size()) {
      for (unsigned int j = 0; j < lag_id_list1[i].size(); j++) {
        ASSERT_EQ(egress_info[count].rid, rid_list[i]);
        ASSERT_NE(std::find(lag_port_list1[member_count].begin(),
                          lag_port_list1[member_count].end(),
                          egress_info[count].egress_port),
                  lag_port_list1[member_count].end());
        count++;
        member_count++;
      }
    }
  }
  ASSERT_EQ(egress_info.size(), count);

  // update lag membership
  for (unsigned int i = 0; i < lag_id_slist2.size(); i++) {
    McSimplePre::PortMap lag_port_map;
    for (unsigned int j = 0; j < lag_port_list2[i].size(); j++) {
      lag_port_map[lag_port_list2[i][j]] = 1;
    }
    rc = pre.mc_set_lag_membership(lag_id_slist2[i], lag_port_map);
    ASSERT_EQ(rc, McSimplePre::SUCCESS);
  }

  // remove ports 7 and 8 from 3rd node port_map
  McSimplePre::PortMap &port_map_2 = port_maps[2];
  port_map_2[7] = 0;
  port_map_2[8] = 0;
  rc = pre.mc_node_update(l1_hdl_list[2], port_maps[2], lag_maps[2]);
  ASSERT_EQ(rc, McSimplePre::SUCCESS);

  McSimplePre::LagMap &lag_map_1 = lag_maps[1];
  lag_map_1[24] = 0;
  rc = pre.mc_node_update(l1_hdl_list[1], port_maps[1], lag_maps[1]);
  ASSERT_EQ(rc, McSimplePre::SUCCESS);

/*
 * Multicast tree view
 *
 *             ---------------
 *             | mgid = 0x400|
 *             ---------------
 *                    |
 *                    V
 *             ---------------    ---------------    ---------------
 *             | rid = 0x200 | -> | rid = 0x211 | -> | rid = 0x221 |
 *             ---------------    ---------------    ---------------
 *                    |                  |                  |
 *                    V                  V                  V
 *-----------   ---------------   ---------------    ---------------
 * port_map |   | 1, 4        |   | 5, 6        |    | 2, 9        |
 *-----------   ---------------   ---------------    ---------------
 * lag_map  |   | 2, 22       |   |             |    |             |
 *-----------   ---------------   ---------------    ---------------
 *
 * Lag table
 *-----------   ---------------   ---------------
 * lag_index|   | 2           |   | 22          |   
 *-----------   ---------------   ---------------
 *                     |                 |
 *                     V                 V
 *-----------   ----------------  ----------------
 * port_map |   |10, 12, 20, 21|  |11, 14, 16, 24|
 *-----------   ----------------  ----------------
 */
  ingress_info.mgid = mgid;
  egress_info = pre.replicate(ingress_info);

  count = 0;
  member_count = 0;
  for (unsigned int i = 0; i < 3; i++) {
    for (unsigned int j = 0; j < port_list2[i].size(); j++) {
      ASSERT_EQ(egress_info[count].rid, rid_list[i]);
      ASSERT_EQ(egress_info[count].egress_port, port_list2[i][j]);
      count++;
    }
    if (i < lag_id_list2.size()) {
      for (unsigned int j = 0; j < lag_id_list2[i].size(); j++) {
        ASSERT_EQ(egress_info[count].rid, rid_list[i]);
        ASSERT_NE(std::find(lag_port_list2[member_count].begin(),
                          lag_port_list2[member_count].end(),
                          egress_info[count].egress_port),
                  lag_port_list2[member_count].end());
        count++;
        member_count++;
      }
    }
  }
  ASSERT_EQ(egress_info.size(), count);
  
  // cleanup
  for (unsigned int i = 0; i < l1_hdl_list.size(); i++) {
    rc = pre.mc_node_dissociate(mgrp_hdl, l1_hdl_list[i]);
    ASSERT_EQ(rc, McSimplePre::SUCCESS);
    rc = pre.mc_node_destroy(l1_hdl_list[i]);
    ASSERT_EQ(rc, McSimplePre::SUCCESS);
  }
  rc = pre.mc_mgrp_destroy(mgrp_hdl);
  ASSERT_EQ(rc, McSimplePre::SUCCESS);
}

TEST(McSimplePreLAG, ResetState) {
  McSimplePreLAG pre;
  McSimplePreLAG::mgrp_hdl_t mgrp;
  McSimplePreLAG::l1_hdl_t l1h;
  ASSERT_EQ(McSimplePre::SUCCESS, pre.mc_mgrp_create(1, &mgrp));
  McSimplePreLAG::PortMap port_map;
  port_map[1] = 1;
  ASSERT_EQ(McSimplePre::SUCCESS, pre.mc_node_create(0, port_map, {}, &l1h));
  ASSERT_EQ(McSimplePre::SUCCESS, pre.mc_node_associate(mgrp, l1h));
  McSimplePreLAG::LagMap lag_map;
  lag_map[2] = 1;
  ASSERT_EQ(McSimplePre::SUCCESS, pre.mc_node_create(0, {}, lag_map, &l1h));
  ASSERT_EQ(McSimplePre::SUCCESS, pre.mc_node_associate(mgrp, l1h));
  ASSERT_EQ(McSimplePre::SUCCESS, pre.mc_set_lag_membership(2, port_map));

  auto mc_out_1 = pre.replicate({1});
  ASSERT_EQ(2u, mc_out_1.size());

  // probably not the best test for reset
  pre.reset_state();
  auto mc_out_2 = pre.replicate({1});
  ASSERT_EQ(0u, mc_out_2.size());

  ASSERT_EQ(McSimplePre::SUCCESS, pre.mc_mgrp_create(1, &mgrp));
  auto mc_out_3 = pre.replicate({1});
  ASSERT_EQ(0u, mc_out_3.size());
}
