/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hashexpr.h"

#include "bfas.h"
#include "gtest/gtest.h"
#include "backends/tofino/bf-asm/stage.h"

namespace {

// TEST(hashexpr, slice_with_rand_alg)
//
// Verify that a slice with a random algorithm doesn't loop forever
//
// Warning: If it does loop forever, then the test will hang :( Running through ctest should
// result in an eventual timeout, but running from the command line will hang until Ctrl-C.
TEST(hashexpr, slice_with_rand_alg) {
    const char *hash_str = R"HASH_CFG(
version:
  target: Tofino2
phv ingress:
  Field1: MW0
  Field2: MW1
  Field3: MH8(0..8)
  Field4: MB9
  Hdr.$valid: B3(4)
stage 0 ingress:
  hash_action _HashTable 0:
    always_run: true
    p4: { name: HashTable, size: 1, disable_atomic_modify : true }
    row: 0
    result_bus: 1
    hash_dist:
      1: { hash: 1, mask: 0xffff, shift: 0 }
    input_xbar:
      exact group 2: { 0: Field1, 32: Field2, 64: Field3, 80: Field4 }
      hash 4:
        16..31: slice(stripe(crc_rev(0xc002, 0x0, 0x0, 81, { 9: Field2, 41: Field1 }, { })), 0..15)
      hash 5:
        16..31: slice(stripe(crc_rev(0xc002, 0x0, 0x0, 81, { 0: Field3, 73: Field4 }, { })), 0..15)
      hash group 1:
        table: [4, 5]
        seed: 0x0
    gateway:
      name: cond-81
      input_xbar:
        exact group 1: { 36: Hdr.$valid }
      row: 1
      bus: 0
      unit: 0
      payload_row: 0
      payload_unit: 1
      payload: 0x1
      format: { action(0): 0..0 }
      match: { 4: Hdr.$valid }
      0b***1:  END
      miss: run_table
      condition:
        expression: "(Hdr.$valid == 1)"
        true:  END
        false:  END
    next:  END
    action_bus: { 108..111 : hash_dist(1) }
    instruction: _HashTable(action, $DEFAULT)
    actions:
      MyAction(1, 7):
      - hit_allowed: { allowed: true }
      - default_action: { allowed: true }
      - handle: 0x20000063
      - next_table: 0
      - set W15(0..15), hash_dist(1, 0..15)
    default_action: MyAction
)HASH_CFG";

    asm_parse_string(hash_str);

    Stage *stage = Stage::stage(INGRESS, 0);
    Table *table = stage->tables[0];
    InputXbar &ixbar = *table->input_xbar[0];
    for (auto &kv1 : ixbar.get_hash_tables()) {
        // Grab the hash table map
        auto &htmap = kv1.second;
        for (auto &kv2 : htmap) {
            // Get the hash column/hash expression and change the hash algorithm
            auto &hc = kv2.second;
            auto *he = hc.fn;
            he->hash_algorithm.hash_alg = RANDOM_DYN;
        }
    }

    std::cerr << std::endl
              << "If this test hangs then there is a problem with handling of RANDOM_DYN at the "
                 "hash slice level. Terminate the hang with Ctrl-C."
              << std::endl
              << std::endl;
    Section::process_all();

    // Reset the target type for future tests
    options.target = NO_TARGET;
}

}  // namespace
