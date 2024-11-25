/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

#include "backends/p4tools/modules/testgen/targets/tofino/constants.h"

#include "ir/irutils.h"

namespace P4::P4Tools::P4Testgen::Tofino {

const IR::Member TofinoConstants::INGRESS_DROP_VAR =
    IR::Member(IR::Type_Bits::get(3), new IR::PathExpression("*ig_intr_md_for_dprsr"), "drop_ctl");

const IR::Member JBayConstants::INGRESS_DROP_VAR =
    IR::Member(IR::Type_Bits::get(3), new IR::PathExpression("*ig_intr_md_for_dprsr"), "drop_ctl");

const IR::Member TofinoConstants::EGRESS_DROP_VAR =
    IR::Member(IR::Type_Bits::get(3), new IR::PathExpression("*eg_intr_md_for_dprsr"), "drop_ctl");

const IR::Member JBayConstants::EGRESS_DROP_VAR =
    IR::Member(IR::Type_Bits::get(3), new IR::PathExpression("*eg_intr_md_for_dprsr"), "drop_ctl");

}  // namespace P4::P4Tools::P4Testgen::Tofino
