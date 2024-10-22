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

#include "parser_critical_path.h"

#include "ir/ir.h"
#include "lib/log.h"

std::ostream &operator<<(std::ostream &out, const ParserCriticalPathResult &rst) {
    out << "Critical Path Length: " << rst.path.size() << " states (" << rst.length << " bits)"
        << std::endl;
    for (const auto &s : rst.path) {
        out << s.first->name << "(" << s.second << ")" << "\n";
    }
    return out;
}

bool CalcParserCriticalPath::is_on_critical_path(const IR::BFN::ParserState *state,
                                                 const ParserCriticalPathResult &result) {
    for (auto &kv : result.path) {
        if (kv.first->name == state->name) return true;
    }

    return false;
}

bool CalcParserCriticalPath::is_on_critical_path(const IR::BFN::ParserState *state) const {
    return is_on_critical_path(state, ingress_result) || is_on_critical_path(state, egress_result);
}

bool CalcParserCriticalPath::is_user_specified_critical_state(
    const IR::BFN::ParserState *state, const ordered_set<const IR::BFN::ParserState *> &result) {
    for (auto s : result) {
        if (s->name == state->name) return true;
    }

    return false;
}

bool CalcParserCriticalPath::is_user_specified_critical_state(
    const IR::BFN::ParserState *state) const {
    return is_user_specified_critical_state(state, ingress_user_critical_states) ||
           is_user_specified_critical_state(state, egress_user_critical_states);
}

ordered_set<const PHV::Field *> CalcParserCriticalPath::calc_all_critical_fields() const {
    auto ingress = calc_critical_fields(ingress_result);
    auto egress = calc_critical_fields(egress_result);
    ingress.insert(egress.begin(), egress.end());
    return ingress;
}

ordered_set<const PHV::Field *> CalcParserCriticalPath::calc_critical_fields(
    const ParserCriticalPathResult &critical_path) const {
    ordered_set<const PHV::Field *> rtn;
    for (const auto &v : critical_path.path) {
        for (const auto &st : v.first->statements) {
            if (auto *ex = st->to<IR::BFN::Extract>()) {
                if (auto lval = ex->dest->to<IR::BFN::FieldLVal>()) {
                    rtn.insert(phv.field(lval->field));
                }
            }
        }
    }
    return rtn;
}

bool ParserCriticalPath::preorder(const IR::BFN::ParserState *state) {
    int total_extracted = 0;
    for (const IR::BFN::ParserPrimitive *prim : state->statements) {
        // In this backend stage,
        // set_metadata is Extract as well
        if (auto *ex = prim->to<IR::BFN::Extract>()) {
            if (auto lval = ex->dest->to<IR::BFN::FieldLVal>()) {
                total_extracted += lval->field->type->width_bits();
            }
        }
    }

    result.length += total_extracted;
    result.path.push_back(std::make_pair(state, total_extracted));
    return true;
}

void ParserCriticalPath::flow_merge(Visitor &other_) {
    auto other = dynamic_cast<ParserCriticalPath &>(other_);
    if (other.result.length > result.length) {
        result = other.result;
    }
}

void ParserCriticalPath::flow_copy(::ControlFlowVisitor &other_) {
    auto other = dynamic_cast<ParserCriticalPath &>(other_);
    result = other.result;
}

void ParserCriticalPath::end_apply() {
    if (LOGGING(4)) {
        LOG4("Critical Path Result of " << gress_);
        LOG4(result);
    }
    final_result = result;
}
