/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "parser_query.h"

#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/phv/phv_fields.h"

bool ParserQuery::same_const_source(const IR::BFN::ParserPrimitive *pp,
                                    const IR::BFN::ParserPrimitive *qp) const {
    auto p = pp->to<IR::BFN::Extract>();
    auto q = qp->to<IR::BFN::Extract>();
    if (!p || !q)
        return false;
    auto pc = p->source->to<IR::BFN::ConstantRVal>();
    auto qc = q->source->to<IR::BFN::ConstantRVal>();

    return (pc && qc) ? *(pc->constant) == *(qc->constant) : false;
}

bool ParserQuery::is_before(const ordered_set<const IR::BFN::ParserPrimitive *> &writes,
                            const IR::BFN::Parser *parser, const IR::BFN::ParserPrimitive *p,
                            const IR::BFN::ParserState *ps, const IR::BFN::ParserPrimitive *q,
                            const IR::BFN::ParserState *qs) const {
    // case 0. resolve the cases when one of the writes is ChecksumResidualDeposit and
    // therefore its effect is at the very end of the parser and not at the spot it is
    // written at
    // (we don't need to handle both p and q being residuals as there should not be two
    // residual writes into the same field on the same parser path)
    if (p->is<IR::BFN::ChecksumResidualDeposit>()) {
        // checksum residual happens at the very end of the parser, so anything else
        // happens before it
        return false;
    }
    if (q->is<IR::BFN::ChecksumResidualDeposit>()) {
        return true;  // q always happens after p
    }

    if (!ps)
        ps = field_to_states.write_to_state.at(p);
    if (!qs)
        qs = field_to_states.write_to_state.at(q);

    // case 1. p is in a parser state that is an ancestor to q (including loops)
    if (parser_info.graph(parser).is_reachable(ps, qs)) {
        return true;
    }

    // case 2. p and q are in the same parser state but p happens before q
    if (p != q && ps == qs) {
        for (auto o : writes) {
            if (o == p)
                return true;
            if (o == q)
                return false;
        }
    }
    return false;
}

ordered_set<const IR::BFN::ParserPrimitive *> ParserQuery::get_previous_writes(
        const IR::BFN::ParserPrimitive *p,
        const ordered_set<const IR::BFN::ParserPrimitive *> &writes) const {
    ordered_set<const IR::BFN::ParserPrimitive*> rv;

    auto ps = field_to_states.write_to_state.at(p);
    auto parser = field_to_states.state_to_parser.at(ps);

    for (auto q : writes) {
        if (!same_const_source(p, q) &&
            is_before(writes, parser, q, nullptr, p, ps)) {
            rv.insert(q);
        }
    }

    return rv;
}

bool ParserQuery::is_single_write(
    const ordered_set<const IR::BFN::ParserPrimitive *> &writes) const {
    for (auto x : writes) {
        auto prev = get_previous_writes(x, writes);

        if (!prev.empty())
            return false;
    }

    return true;
}

ordered_set<const IR::BFN::ParserPrimitive*>
ParserQuery::find_inits(const ordered_set<const IR::BFN::ParserPrimitive*>& writes) const {
    ordered_set<const IR::BFN::ParserPrimitive*> inits;

    for (auto p : writes) {
        bool has_predecessor = false;

        auto ps = field_to_states.write_to_state.at(p);
        auto parser = field_to_states.state_to_parser.at(ps);

        for (auto q : writes) {
            if ((has_predecessor = is_before(writes, parser, q, nullptr, p, ps)))
                break;
        }

        if (!has_predecessor)
           inits.insert(p);
    }

    return inits;
}

ordered_set<const IR::BFN::ParserPrimitive*> ParserQuery::find_inits(PHV::Container c) const {
    if (!field_to_states.container_to_writes.count(c))
        return ordered_set<const IR::BFN::ParserPrimitive *>();

    return find_inits(field_to_states.container_to_writes.at(c));
}

