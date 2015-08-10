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
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include "SimplePreLAG.h"

namespace bm_runtime { namespace simple_pre_lag {

class SimplePreLAGHandler : virtual public SimplePreLAGIf {
public:
  SimplePreLAGHandler() { }

  BmMcMgrpHandle bm_mc_mgrp_create(const BmMcMgrp mgrp) {
    // Your implementation goes here
    printf("bm_mc_mgrp_create\n");
    return 0;
  }

  void bm_mc_mgrp_destroy(const BmMcMgrpHandle mgrp_handle) {
    // Your implementation goes here
    printf("bm_mc_mgrp_destroy\n");
  }

  BmMcL1Handle bm_mc_node_create(const BmMcRid rid,
                                 const BmMcPortMap& port_map,
                                 const BmMcLagMap& lag_map) {
    std::string port_str(port_map.rbegin(), port_map.rbegin() + 16);
    std::string lag_str(lag_map.rbegin(), lag_map.rbegin() + 16);
    std::cout << "bm_mc_node_create" << std::endl
	      << rid << std::endl
	      << port_str << std::endl
	      << lag_str << std::endl;
    return 0;
  }

  void bm_mc_node_associate(const BmMcMgrpHandle mgrp_handle, const BmMcL1Handle l1_handle) {
    // Your implementation goes here
    printf("bm_mc_node_associate\n");
  }

  void bm_mc_node_destroy(const BmMcL1Handle l1_handle) {
    // Your implementation goes here
    printf("bm_mc_l1_node_destroy\n");
  }

  void bm_mc_node_update(const BmMcL1Handle l1_handle,
                         const BmMcPortMap& port_map,
                         const BmMcLagMap& lag_map) {
    // Your implementation goes here
    printf("bm_mc_node_update\n");
  }

  void bm_mc_set_lag_membership(const BmMcLagIndex lag_index,
                                const BmMcPortMap& port_map) {
    // Your implementation goes here
    printf("bm_mc_set_lag_membership\n");
  }
};

} }
