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

#include "parser_loops_info.h"

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include "ir/ir.h"
#include "lib/log.h"
#include "bf-p4c/midend/parser_graph.h"
#include "bf-p4c/device.h"
#include "bf-p4c/arch/arch.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/common/asm_output.h"
#include "bf-p4c/parde/add_parde_metadata.h"
#include "bf-p4c/parde/dump_parser.h"

namespace BFN {

bool ParserPragmas::checkNumArgs(cstring pragma, const IR::Vector<IR::Expression>& exprs,
                         unsigned expected) {
    if (exprs.size() != expected) {
        warning("@pragma %1% must have %2% argument, %3% are found, skipped",
                   pragma, expected, exprs.size());
        return false;
    }

    return true;
}

bool ParserPragmas::checkGress(cstring pragma, const IR::StringLiteral* gress) {
    if (!gress || (gress->value != "ingress" && gress->value != "egress")) {
        warning("@pragma %1% must be applied to ingress/egress", pragma);
        return false;
    }
    return true;
}

bool ParserPragmas::preorder(const IR::Annotation *annot) {
    auto pragma_name = annot->name.name;
    auto p = findContext<IR::BFN::TnaParser>();
    auto ps = findContext<IR::ParserState>();

    if (!p || !ps)
        return false;

    if (pragma_name == "terminate_parsing") {
        auto& exprs = annot->expr;
        if (!checkNumArgs(pragma_name, exprs, 1))
            return false;

        auto gress = exprs[0]->to<IR::StringLiteral>();
        if (!checkGress(pragma_name, gress))
            return false;

        if (gress->value == toString(p->thread)) {
            terminate_parsing.insert(ps);
        }
    } else if (pragma_name == "force_shift") {
        auto& exprs = annot->expr;
        if (!checkNumArgs(pragma_name, exprs, 2))
            return false;

        auto gress = exprs[0]->to<IR::StringLiteral>();
        if (!checkGress(pragma_name, gress))
            return false;

        auto shift_amt = exprs[1]->to<IR::Constant>();
        if (!shift_amt || shift_amt->asInt() < 0) {
            warning("@pragma force_shift must have positive shift amount(bits)");
            return false;
        }

        if (gress->value == toString(p->thread)) {
            force_shift[ps] = shift_amt->asInt();
        }
    } else if (pragma_name == "max_loop_depth") {
        auto& exprs = annot->expr;
        if (!checkNumArgs(pragma_name, exprs, 1))
        return false;

        auto max_loop = exprs[0]->to<IR::Constant>();
        if (!max_loop || max_loop->asUnsigned() == 0) {
            warning("@pragma max_loop_depth must be greater than one, skipping: %1%", annot);
            return false;
        }

        warning("Parser state %1% will be unrolled up to %2% "
                  "times due to @pragma max_loop_depth.", ps->name, max_loop->asInt());

        max_loop_depth[ps] = max_loop->asUnsigned();
    } else if (pragma_name == "dont_unroll") {
        warning("Parser state %1% will not be unrolled because of @pragma dont_unroll",
                   ps->name);

        dont_unroll.insert(ps->name);
    }

    return false;
}

/// Infer loop depth by looking at the stack size of stack references in the
/// state.
struct ParserLoopsInfo::GetMaxLoopDepth : public Inspector {
    bool has_next = false;
    int max_loop_depth = -1;

    /// Identify stack depth prior to SimplifyReferences
    bool preorder(const IR::MethodCallStatement *mc) override {
        if (mc->methodCall->arguments->size() == 0) return mc;
        auto *dest = mc->methodCall->arguments->at(0)->expression->to<IR::Member>();
        if (!dest) return true;
        auto *expr = dest->expr->to<IR::Member>();
        if (!expr) return true;
        auto *type_stack = expr->type->to<IR::Type_Stack>();
        if (!type_stack) return true;

        auto stack_size = type_stack->size->to<IR::Constant>()->asInt();

        if (max_loop_depth == -1)
            max_loop_depth = stack_size;
        else
            max_loop_depth = std::min(max_loop_depth, stack_size);

        return true;
    }

    /// Identify stack depth after SimplifyReferences
    bool preorder(const IR::HeaderStackItemRef* ref) override {
        auto stack_size = ref->base()->type->to<IR::Type_Stack>()
                                     ->size->to<IR::Constant>()->asInt();

        if (max_loop_depth == -1)
            max_loop_depth = stack_size;
        else
            max_loop_depth = std::min(max_loop_depth, stack_size);

        return true;
    }

    bool preorder(const IR::BFN::UnresolvedHeaderStackIndex* unresolved) override {
        if (unresolved->index == "next")
            has_next = true;

        return false;
    }
};

ParserLoopsInfo::ParserLoopsInfo(P4::ReferenceMap* refMap, const IR::BFN::TnaParser* parser,
        const ParserPragmas& pm) : parserPragmas(pm) {
    P4ParserGraphs pg(refMap, false);
    parser->apply(pg);

    loops = pg.compute_loops(parser);

    LOG2("detected " << loops.size() << " loops in " << parser->thread << " parser");

    if (LOGGING(2)) {
        unsigned id = 0;
        for (auto& loop : loops) {
            std::clog << "loop " << id++ << " : [ ";
            for (auto s : loop)
                std::clog << s << " ";
            std::clog << " ]" << std::endl;
        }
    }

    for (const auto& loop : loops) {
        for (const auto &s : loop) {
            auto state = pg.get_state(parser, s);
            GetMaxLoopDepth mld;
            state->apply(mld);

            max_loop_depth[s] = mld.max_loop_depth;
            LOG3("inferred loop depth " << max_loop_depth[s] << " for " << s);

            if (mld.max_loop_depth > 1 && mld.has_next)
                has_next.insert(s);
        }
    }
}

const std::set<cstring>* ParserLoopsInfo::find_loop(cstring state) const {
    for (auto& loop : loops) {
        if (loop.count(state))
            return &loop;
    }

    return nullptr;
}

bool ParserLoopsInfo::has_next_on_loop(cstring state) const {
    auto loop = find_loop(state);
    if (!loop) return false;

    for (auto s : *loop) {
        if (has_next.count(s))
            return true;
    }

    return false;
}

bool ParserLoopsInfo::dont_unroll(cstring state) const {
    auto loop = find_loop(state);
    if (!loop) return false;

    for (auto s : *loop) {
        if (parserPragmas.dont_unroll.count(s))
            return true;
    }

    return !has_next_on_loop(state);
}

bool ParserLoopsInfo::need_strided_allocation(cstring state) const {
    return dont_unroll(state) && has_next.count(state);
}

}  // namespace BFN
