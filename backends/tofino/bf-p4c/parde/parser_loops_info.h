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

#ifndef EXTENSIONS_BF_P4C_PARDE_PARSER_LOOPS_INFO_H_
#define EXTENSIONS_BF_P4C_PARDE_PARSER_LOOPS_INFO_H_

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "logging/pass_manager.h"
#include "bf-p4c/ir/gress.h"


namespace BFN {

struct ParserPragmas : public Inspector {
    static bool checkNumArgs(cstring pragma, const IR::Vector<IR::Expression>& exprs,
                             unsigned expected);

    static bool checkGress(cstring pragma, const IR::StringLiteral* gress);

    bool preorder(const IR::Annotation *annot) override;

    std::set<const IR::ParserState*> terminate_parsing;
    std::map<const IR::ParserState*, unsigned> force_shift;
    std::map<const IR::ParserState*, unsigned> max_loop_depth;

    std::set<cstring> dont_unroll;
};

/// Collect loop information in the frontend parser IR.
///   - Where are the loops?
///   - What is the depth (max iterations) of each loop?
struct ParserLoopsInfo {
    /// Infer loop depth by looking at the stack size of stack references in the
    /// state.
    struct GetMaxLoopDepth;

    ParserLoopsInfo(P4::ReferenceMap* refMap, const IR::BFN::TnaParser* parser,
            const ParserPragmas& pm);

    const ParserPragmas& parserPragmas;

    std::set<std::set<cstring>> loops;
    std::map<cstring, int> max_loop_depth;
    std::set<cstring> has_next;   // states that have stack "next" references

    const std::set<cstring>* find_loop(cstring state) const;

    /// Returns true if the state is on loop that has "next" reference.
    bool has_next_on_loop(cstring state) const;

    bool dont_unroll(cstring state) const;

    /// state is on loop that requires strided allocation
    bool need_strided_allocation(cstring state) const;
};

}  // namespace BFN

#endif  /* EXTENSIONS_BF_P4C_PARDE_PARSER_LOOPS_INFO_H_ */
