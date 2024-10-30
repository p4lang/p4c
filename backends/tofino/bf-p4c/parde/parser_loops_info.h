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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_PARSER_LOOPS_INFO_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_PARSER_LOOPS_INFO_H_

#include "bf-p4c/ir/gress.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "logging/pass_manager.h"

namespace BFN {

struct ParserPragmas : public Inspector {
    static bool checkNumArgs(cstring pragma, const IR::Vector<IR::Expression> &exprs,
                             unsigned expected);

    static bool checkGress(cstring pragma, const IR::StringLiteral *gress);

    bool preorder(const IR::Annotation *annot) override;

    std::set<const IR::ParserState *> terminate_parsing;
    std::map<const IR::ParserState *, unsigned> force_shift;
    std::map<const IR::ParserState *, unsigned> max_loop_depth;

    std::set<cstring> dont_unroll;
};

/// Collect loop information in the frontend parser IR.
///   - Where are the loops?
///   - What is the depth (max iterations) of each loop?
struct ParserLoopsInfo {
    /// Infer loop depth by looking at the stack size of stack references in the
    /// state.
    struct GetMaxLoopDepth;

    ParserLoopsInfo(P4::ReferenceMap *refMap, const IR::BFN::TnaParser *parser,
                    const ParserPragmas &pm);

    const ParserPragmas &parserPragmas;

    std::set<std::set<cstring>> loops;
    std::map<cstring, int> max_loop_depth;
    std::set<cstring> has_next;  // states that have stack "next" references

    const std::set<cstring> *find_loop(cstring state) const;

    /// Returns true if the state is on loop that has "next" reference.
    bool has_next_on_loop(cstring state) const;

    bool dont_unroll(cstring state) const;

    /// state is on loop that requires strided allocation
    bool need_strided_allocation(cstring state) const;
};

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_PARSER_LOOPS_INFO_H_ */
