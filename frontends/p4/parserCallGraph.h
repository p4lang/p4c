/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_P4_PARSERCALLGRAPH_H_
#define FRONTENDS_P4_PARSERCALLGRAPH_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/callGraph.h"
#include "ir/ir.h"

namespace P4 {

typedef CallGraph<const IR::ParserState *> ParserCallGraph;

/** @brief Builds a CallGraph of ParserState nodes.
 */
class ComputeParserCG : public Inspector, public ResolutionContext {
    ParserCallGraph *transitions;

 public:
    explicit ComputeParserCG(/* out */ ParserCallGraph *transitions) : transitions(transitions) {
        CHECK_NULL(transitions);
        setName("ComputeParserCG");
    }
    bool preorder(const IR::PathExpression *expression) override;
    void postorder(const IR::SelectExpression *expression) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_PARSERCALLGRAPH_H_ */
