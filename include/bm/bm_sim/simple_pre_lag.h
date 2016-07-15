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

//! @file simple_pre_lag.h

#ifndef BM_BM_SIM_SIMPLE_PRE_LAG_H_
#define BM_BM_SIM_SIMPLE_PRE_LAG_H_

#include <string>
#include <vector>

#include "pre.h"
#include "simple_pre.h"

namespace bm {

//! Enhances McSimplePre with LAG (link aggregation) support.
class McSimplePreLAG : public McSimplePre {
 public:
  static constexpr int LAG_MAX_ENTRIES = 256;
  typedef uint16_t lag_id_t;

  McReturnCode mc_node_create(const rid_t rid,
                              const PortMap &port_map,
                              const LagMap &lag_map,
                              l1_hdl_t *l1_hdl);
  McReturnCode mc_node_update(const l1_hdl_t l1_hdl,
                              const PortMap &port_map,
                              const LagMap &lag_map);

  McReturnCode mc_set_lag_membership(const lag_id_t lag_index,
                                     const PortMap &port_map);

  std::string mc_get_entries() const;

  void reset_state();

  //! TODO(unknown)
  std::vector<McOut> replicate(const McIn) const;

 private:
  struct LagEntry {
    uint16_t member_count;
    PortMap port_map{};

    LagEntry() {}
    LagEntry(uint16_t member_count,
            const PortMap &port_map) :
            member_count(member_count),
            port_map(port_map) {}
  };

  std::unordered_map<lag_id_t, LagEntry> lag_entries{};
};

}  // namespace bm

#endif  // BM_BM_SIM_SIMPLE_PRE_LAG_H_
