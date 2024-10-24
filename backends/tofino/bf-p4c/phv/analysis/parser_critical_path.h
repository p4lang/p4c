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

#ifndef BF_P4C_PHV_ANALYSIS_PARSER_CRITICAL_PATH_H_
#define BF_P4C_PHV_ANALYSIS_PARSER_CRITICAL_PATH_H_

#include "bf-p4c/ir/control_flow_visitor.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"
#include "lib/cstring.h"

struct ParserCriticalPathResult {
    /// A vector of pairs where the first of a pair is
    /// the pointer to IR::BFN::ParserState,
    /// the second is the bits extracted in that state.
    std::vector<std::pair<const IR::BFN::ParserState *, int>> path;
    int length;

    ParserCriticalPathResult() : path(), length(0) {}

    // Clear the state stored in the ParserCriticalPathResult struct.
    void clear() {
        path.clear();
        length = 0;
    }
};

/** Produces a ParserCriticalPathResult that contains names and statistics of states
 * on the critical path, and the length (total bits extracted to PHV) of the critical path.
 *
 * The $valid bits are counted as well, because they are stored in PHV.
 * INGRESS/EGRESS need to be specified in the constructor.
 *
 * @pre This is a backend pass.
 */
class ParserCriticalPath : public BFN::ControlFlowVisitor, public ParserInspector {
 private:
    profile_t init_apply(const IR::Node *root) override {
        profile_t rv = Inspector::init_apply(root);
        result.clear();
        return rv;
    }

    bool preorder(const IR::BFN::AbstractParser *parser) override {
        return gress_ == parser->gress;
    };

    bool preorder(const IR::BFN::ParserPrimitive *) override { return false; };

    bool preorder(const IR::BFN::ParserState *state) override;

    void flow_merge(Visitor &) override;
    void flow_copy(::ControlFlowVisitor &) override;
    void end_apply() override;
    gress_t gress_;
    ParserCriticalPathResult &final_result;

 public:
    ParserCriticalPath(gress_t gress, ParserCriticalPathResult &rst)
        : gress_(gress), final_result(rst) {
        joinFlows = true;
        visitDagOnce = false;
    }

    ParserCriticalPath *clone() const override { return new ParserCriticalPath(*this); }

 public:
    ParserCriticalPathResult result;
};

class CollectUserSpecifiedCriticalStates : public Inspector {
    gress_t gress;
    ordered_set<const IR::BFN::ParserState *> &critical_states;

    bool preorder(const IR::BFN::ParserState *state) override {
        if (state->gress != gress) return false;

        for (const auto *p4State : state->p4States) {
            for (auto annot : p4State->annotations->annotations) {
                if (annot->name.name == "critical") {
                    auto &exprs = annot->expr;
                    if (exprs.size() == 1) {
                        auto gress = exprs[0]->to<IR::StringLiteral>();
                        if (!gress) {
                            error(
                                "Invalid use of %1%, correct usage is: "
                                "@pragma critical [ingress/egress]",
                                annot);
                        }

                        if (gress && gress->value == toString(state->gress)) {
                            LOG3("@critical specified on " << state->name);
                            critical_states.insert(state);
                            return true;
                        }
                    } else if (exprs.size() == 0) {
                        LOG3("@critical specified on " << state->name);
                        critical_states.insert(state);
                        return true;
                    }
                }
            }
        }

        return true;
    }

 public:
    CollectUserSpecifiedCriticalStates(gress_t gr, ordered_set<const IR::BFN::ParserState *> &cs)
        : gress(gr), critical_states(cs) {}
};

/** Calculate critical path of both ingress and egress parser.
 */
class CalcParserCriticalPath : public PassManager {
 public:
    explicit CalcParserCriticalPath(const PhvInfo &phv) : phv(phv) {
        addPasses({new CollectUserSpecifiedCriticalStates(INGRESS, ingress_user_critical_states),
                   new CollectUserSpecifiedCriticalStates(EGRESS, egress_user_critical_states),
                   new ParserCriticalPath(INGRESS, ingress_result),
                   new ParserCriticalPath(EGRESS, egress_result)});
    }

    ordered_set<const PHV::Field *> calc_all_critical_fields() const;

    const ParserCriticalPathResult &get_ingress_result() const { return ingress_result; }

    const ParserCriticalPathResult &get_egress_result() const { return egress_result; }

    const ordered_set<const IR::BFN::ParserState *> &get_ingress_user_critical_states() const {
        return ingress_user_critical_states;
    }

    const ordered_set<const IR::BFN::ParserState *> &get_egress_user_critical_states() const {
        return egress_user_critical_states;
    }

    bool is_on_critical_path(const IR::BFN::ParserState *state) const;
    bool is_user_specified_critical_state(const IR::BFN::ParserState *state) const;

 private:
    static bool is_on_critical_path(const IR::BFN::ParserState *state,
                                    const ParserCriticalPathResult &result);

    static bool is_user_specified_critical_state(
        const IR::BFN::ParserState *state, const ordered_set<const IR::BFN::ParserState *> &result);

    ordered_set<const PHV::Field *> calc_critical_fields(
        const ParserCriticalPathResult &critical_path) const;

 private:
    const PhvInfo &phv;

    ParserCriticalPathResult ingress_result;
    ParserCriticalPathResult egress_result;

    ordered_set<const IR::BFN::ParserState *> ingress_user_critical_states;
    ordered_set<const IR::BFN::ParserState *> egress_user_critical_states;
};

std::ostream &operator<<(std::ostream &out, const ParserCriticalPathResult &rst);

#endif /* BF_P4C_PHV_ANALYSIS_PARSER_CRITICAL_PATH_H_ */
