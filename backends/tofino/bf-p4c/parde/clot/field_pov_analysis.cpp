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

#include "field_pov_analysis.h"

bool FieldPovAnalysis::preorder(const IR::BFN::EmitField *emit) {
    auto field = phv.field(emit->source->field);
    auto pov = phv.field(emit->povBit->field);
    pov_to_fields[pov].insert(field);
    return true;
}

bool FieldPovAnalysis::preorder(const IR::BFN::Parser *parser) {
    parsers.insert(parser);
    return true;
}

void FieldPovAnalysis::end_apply() {
    for (auto parser : parsers) {
        auto graph = clotInfo.parserInfo.graph(parser);
        auto topo = graph.topological_sort();
        for (auto state : topo) {
            bitvec curr_field_vec, temp_pov_vec;
            // Loop below will perform the following operations:
            // 1. Populate temp_pov_vec with all the valid bits that are set in the current
            //    state and used in deparser
            // 2. Populate curr_field_vec with all fields that are extracted in current
            //    state and deparsed
            for (auto statement : state->statements) {
                if (auto extract = statement->to<IR::BFN::Extract>()) {
                    if (auto fieldLVal = extract->dest->to<IR::BFN::FieldLVal>()) {
                        auto field = phv.field(fieldLVal->field);
                        CHECK_NULL(field);
                        if (field->pov && pov_to_fields.count(field)) {
                            auto const_src = extract->source->to<IR::BFN::ConstantRVal>()->constant;
                            if (const_src->asInt()) {
                                temp_pov_vec.setbit(field->id);
                            }
                        } else if (field->deparsed() &&
                                   extract->source->is<IR::BFN::PacketRVal>()) {
                            curr_field_vec.setbit(field->id);
                        }
                    }
                }
            }
            // Loop below will perform bitwise-and operation on all predecessors' field_vec
            // and save the result in temp_field_vec. This is to capture only those fields that
            // are set in all predecessors' field_vec. It will also perform bitwise-or operation
            // on all the predecessors' pov_vec and store the result in temp_pov_vec. This is to
            // capture all the valid bits set in every predecessor's pov_vec.

            bool firstIter = true;
            bitvec temp_field_vec;

            if (graph.predecessors().count(state)) {
                for (auto pred : graph.predecessors().at(state)) {
                    if (firstIter) {
                        temp_field_vec = state_to_field_pov[pred->name].first;
                    } else {
                        temp_field_vec &= state_to_field_pov[pred->name].first;
                    }
                    firstIter = false;
                    temp_pov_vec |= state_to_field_pov[pred->name].second;
                }
            }

            // curr_field_vec must contain field extracted in current state and fields that are
            // present in all predecessors' field_vec
            curr_field_vec |= temp_field_vec;

            // For every valid bit in temp_pov_vec, if the corresponding fields is not present
            // in curr_field_vec then add the valid bit in curr_pov_vec
            bitvec curr_pov_vec;
            for (auto pov_id : temp_pov_vec) {
                auto pov = phv.field(pov_id);
                bool is_field_extracted = std::any_of(
                    pov_to_fields.at(pov).begin(), pov_to_fields.at(pov).end(),
                    [&](const PHV::Field *field) { return curr_field_vec.getbit(field->id); });
                if (!is_field_extracted) {
                    curr_pov_vec.setbit(pov_id);
                }
            }

            // If the parser can end at current state, add all the povs in curr_pov_vec in
            // pov_extracted_without_fields
            for (auto transition : state->transitions) {
                if (!transition->next) {
                    for (auto pov_id : curr_pov_vec) {
                        auto pov = phv.field(pov_id);
                        clotInfo.pov_extracted_without_fields.insert(pov);
                    }
                }
            }
            state_to_field_pov[state->name] = std::make_pair(curr_field_vec, curr_pov_vec);
        }
    }
}
