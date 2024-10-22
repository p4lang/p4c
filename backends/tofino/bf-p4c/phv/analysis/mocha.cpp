/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bf-p4c/phv/analysis/mocha.h"

#include "bf-p4c/common/pragma/all_pragmas.h"

Visitor::profile_t CollectMochaCandidates::init_apply(const IR::Node *root) {
    profile_t rv = Inspector::init_apply(root);
    mochaCount = 0;
    mochaSize = 0;
    pov_on_mocha = false;

    BUG_CHECK(root->is<IR::BFN::Pipe>(), "IR root is not a BFN::Pipe: %s", root);
    const auto *root_ = root->to<IR::BFN::Pipe>();
    Device::phvSpec().applyGlobalPragmas(root_->global_pragmas);

    for (auto *anno : root_->global_pragmas) {
        if (anno->name.name == PragmaAllowPOVonMocha::name) {
            pov_on_mocha = true;
            LOG1(" ---  ALLOWING POV ON MOCHA  ---");
        }
    }
    return rv;
}

bool CollectMochaCandidates::preorder(const IR::MAU::Action *act) {
    auto *tbl = findContext<IR::MAU::Table>();
    ActionAnalysis aa(phv, false, false, tbl, red_info);
    ActionAnalysis::FieldActionsMap fieldActionsMap;
    aa.set_field_actions_map(&fieldActionsMap);
    act->apply(aa);
    return true;
}

void CollectMochaCandidates::end_apply() {
    for (PHV::Field &f : phv) {
        std::stringstream ss;
        ss << "    Examining field: " << f << std::endl;
        // Padding field should be marked as mocha candidate. For example, the
        // following slice list consist of two fields: a header field that is a
        // mocha candidate and a padding field.
        //
        //  [ ingress::Wanamassa.Yorkshire.Weinert<1> ^0 bridge deparsed
        //                                      exact_containers mocha [0:0]
        //    ingress::Wanamassa.Yorkshire.__pad_17<7> ^1 padding deparsed
        //                                      exact_containers mocha [0:6] ]
        //
        // This slice list can be allocated to a mocha container if and only if
        // all slices in the slice list are mocha candidate. Therefore, the
        // padding field should be marked as mocha candidate.
        if (f.is_padding()) {
            f.set_mocha_candidate(true);
            ss << "    ...mark padding as mocha candidate.";
            LOG5(ss.str());
            continue;
        }

        ss << " ... isNotMocha: " << nonMochaDark.isNotMocha(&f, nullptr);
        if (nonMochaDark.isNotMocha(&f, nullptr).isReadWrite()) {
            ss << "    ...write by action data/constant/speciality operation/nonset operation";
            LOG5(ss.str());
            continue;
        }
        // Mocha fields that are part of packets need to start at byte boundaries (for now).
        // E.g. trafficClass in ipv4 starts at bit 4 and so, must start at bit 4 of a container.
        // The fact that it shares a container with another non-mocha field becomes an issue if
        // trafficClass is to be allocated to a mocha container.
        if (isPacketField(&f) && f.offset % 8 != 0) {
            ss << "    ...packet field starts at nonzero bit-in-byte offset.";
            LOG5(ss.str());
            continue;
        }
        // If field size is not in byte multiples, we must perform extra analysis to determine if
        // the exact containers requirement for the mocha candidate field can be satisfied. For now,
        // we ignore those fields for mocha allocation.
        // TODO: Add analysis to determine if the exact_containers requirement can be satisfied
        // for nonbyte-aligned packet fields.
        //
        // TODO: commented out, does this constraint make any sense?
        // Whether the program analysis 'think' a field is mocha candidate is
        // orthogonal to whether a mocha field can be allocated to a mocha
        // container. Filter out valid candidate during program analysis seems
        // very conservative.
        //
        // if (isPacketField(&f) && f.size % 8 != 0) {
        //     ss << "    ...packet field size is not in multiple of bytes.";
        //     LOG5(ss.str());
        //     continue;
        // }
        if (f.pov) {
            ss << "    ...pov field ";
            if (uses.is_written_mau(phv.field(f.id))) ss << "(written in MAU)";
            if (!pov_on_mocha) {
                if (uses.is_written_mau(phv.field(f.id))) {
                    LOG5(ss.str());
                    continue;
                }
            }
        }

        ss << "    ...mocha candidate found.";
        LOG5(ss.str());
        ++mochaCount;
        mochaSize += f.size;
        f.set_mocha_candidate(true);
    }
    LOG2("\tNumber of mocha candidates identified: " << mochaCount);
    LOG2("\tBit width of mocha candidates        : " << mochaSize);
    LOG2("\tMocha candidates:");
    for (const PHV::Field &f : phv)
        if (f.is_mocha_candidate()) LOG2("\t  " << f);
}
