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

#include "backends/tofino/bf-asm/stage.h"
#include "backends/tofino/bf-asm/bfas.h"
#include <gtest/gtest.h>

namespace {

// Verify that the next table registers are correctly configured for a standalone gateway with a
// miss next table and no hit next table
TEST(gateway, standalone_miss_next_table) {
    const char *gateway_str = R"GATEWAY_CFG(
version:
  target: Tofino2
phv ingress:
  ig_intr_md_for_dprsr.mirror_type.$valid: B1(0)
  ig_intr_md.ingress_port: {  stage 0: W0(16..24) }
  hdr.data.h1: MH4
  hdr.data.b1: MB1
  ig_intr_md_for_tm.ucast_egress_port: {  stage 1..20: W0(0..8) }
  ig_intr_md_for_tm.ucast_egress_port.$valid: {  stage 1..20: B1(1) }
  ig_intr_md_for_dprsr.mirror_type: {  stage 20: MB0(0..3) }
  hdr.data.$valid: B1(2)
stage 0 ingress:
  gateway cond-1 0:
    name: cond-1
    input_xbar:
      exact group 0: { 16: hdr.data.b1 }
    row: 7
    bus: 0
    unit: 0
    match: { 0: hdr.data.b1 }
    0x12:
      next:  END
    miss:
      next:  test_0
    condition:
      expression: "(hdr.data.b1 != 18)"
      true:  test_0
      false:  END
stage 2 ingress:
  dependency: match
  mpr_stage_id: 1
  mpr_bus_dep_glob_exec: 0x0
  mpr_bus_dep_long_brch: 0x0
  mpr_always_run: 0x0
  mpr_next_table_lut:
    0: 0xff
  ternary_match test_0 0:
    always_run: true
    p4: { name: ingress.test, size: 512 }
    p4_param_order:
      hdr.data.h1: { type: ternary, size: 16, full_size: 16 }
    row: 0
    bus: 0
    column: 0
    input_xbar:
      ternary group 0: { 0: hdr.data.h1 }
    match:
    - { group: 0, byte_config: 3, dirtcam: 0x5 }
    hit: [  END ]
    miss:  END
    indirect: test_0$tind
  ternary_indirect test_0$tind:
    row: 0
    bus: 0
    column: 2
    input_xbar:
      ternary group 0: { 0: hdr.data.h1 }
    format: { action: 0..1, immediate: 2..9 }
    action_bus: { 0 : immediate(0..7) }
    instruction: test_0$tind(action, $DEFAULT)
    actions:
      ingress.setb1(1, 1):
      - p4_param_order: { val: 8 }
      - hit_allowed: { allowed: true }
      - default_action: { allowed: true }
      - handle: 0x20000002
      - next_table: 0
      - { val_1: immediate(0..7), val: val_1 }
      - set MB1, val
      ingress.noop(2, 0):
      - hit_allowed: { allowed: true }
      - default_action: { allowed: true }
      - handle: 0x20000003
      - next_table: 0
      - {  }
    default_action: ingress.setb1
    default_action_parameters:
      val: "0xAA"
)GATEWAY_CFG";

    asm_parse_string(gateway_str);

    Section::process_all();

    Target::JBay::mau_regs regs;
    auto &stages = AsmStage::stages(INGRESS);
    stages[0].write_regs(regs, false);
    for (auto table : stages[0].tables) {
        table->write_regs(regs);
    }

    EXPECT_EQ(regs.rams.match.merge.pred_is_a_brch, 0x01);
}

}  // namespace
