#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_COLLECT_COVERABLE_NODES_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_COLLECT_COVERABLE_NODES_H_

#include <set>

#include "ir/ir.h"
#include "ir/visitor.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools::P4Testgen {

/// A cache of already computed nodes to avoid superfluous computation.
using NodeCache = std::map<const IR::Node *, P4::Coverage::CoverageSet>;

/// CoverableNodesScanner is similar to @ref CollectNodes. It collects all the nodes
/// present in a particular node. However, compared to CollectNodes, it traverses the entire
/// subsequent parser DAG for a particular parser state. If there is a loop in the parser state, it
/// will terminate.
/// TODO: Consider caching this information.
class CoverableNodesScanner : public Inspector {
    /// The set of nodes in the program that could potentially be covered.
    P4::Coverage::CoverageSet coverableNodes;

    /// The current execution state. Used to look up parser states.
    std::reference_wrapper<const ExecutionState> state;

    /// IDs of already visited parser states. To avoid loops.
    std::set<int> seenParserIds;

    /// Specifies, which IR nodes to track with this particular visitor.
    P4::Coverage::CoverageOptions coverageOptions;

    /// Statement coverage.
    bool preorder(const IR::ParserState *parserState) override;
    bool preorder(const IR::AssignmentStatement *stmt) override;
    bool preorder(const IR::MethodCallStatement *stmt) override;
    bool preorder(const IR::ExitStatement *stmt) override;
    bool preorder(const IR::MethodCallExpression *call) override;

    /// Actions coverage.
    bool preorder(const IR::P4Action *act) override;

 public:
    explicit CoverableNodesScanner(const ExecutionState &state);

    /// Apply the input node @param node and insert the newly covered nodes into the provided
    /// coverage set.
    void updateNodeCoverage(const IR::Node *node, P4::Coverage::CoverageSet &nodes);

    /// @return the set of coverable nodes in the program.
    const P4::Coverage::CoverageSet &getCoverableNodes();
};

}  // namespace P4Tools::P4Testgen

#endif /*BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_COLLECT_COVERABLE_NODES_H_*/
