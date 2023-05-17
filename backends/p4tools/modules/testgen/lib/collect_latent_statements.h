#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_COLLECT_LATENT_STATEMENTS_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_COLLECT_LATENT_STATEMENTS_H_

#include <set>

#include "ir/ir.h"
#include "ir/visitor.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools::P4Testgen {

/// CollectLatentStatements is similar to @ref CollectStatements. It collects all the statements
/// present in a particular node. However, compared to CollectStatements, it traverses the entire
/// subsequent parser DAG for a particular parser state. If there is a loop in the parser state, it
/// will terminate.
class CollectLatentStatements : public Inspector {
    P4::Coverage::CoverageSet &statements;
    const ExecutionState &state;
    std::set<int> seenParserIds;

 public:
    explicit CollectLatentStatements(P4::Coverage::CoverageSet &output,
                                     const ExecutionState &state);

    explicit CollectLatentStatements(P4::Coverage::CoverageSet &output, const ExecutionState &state,
                                     const std::set<int> &seenParserIds);

    bool preorder(const IR::ParserState *parserState) override;

    bool preorder(const IR::AssignmentStatement *stmt) override;

    bool preorder(const IR::MethodCallStatement *stmt) override;

    bool preorder(const IR::ExitStatement *stmt) override;
};

}  // namespace P4Tools::P4Testgen

#endif /*BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_COLLECT_LATENT_STATEMENTS_H_*/
