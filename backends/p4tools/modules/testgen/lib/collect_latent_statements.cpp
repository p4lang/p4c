#include "backends/p4tools/modules/testgen/lib/collect_latent_statements.h"

#include <string>
#include <vector>

#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/null.h"
#include "lib/source_file.h"

namespace P4Tools::P4Testgen {

bool CollectLatentStatements::preorder(const IR::ParserState *parserState) {
    // Only bother looking up cases that are not accept or reject.
    if (parserState->name == IR::ParserState::accept ||
        parserState->name == IR::ParserState::reject) {
        return true;
    }

    // Already have seen this state. We might be in a loop.
    if (seenParserIds.count(parserState->clone_id) != 0) {
        return true;
    }

    seenParserIds.emplace(parserState->clone_id);
    CHECK_NULL(parserState->selectExpression);

    parserState->components.apply(CollectLatentStatements(statements, state, seenParserIds));

    if (const auto *selectExpr = parserState->selectExpression->to<IR::SelectExpression>()) {
        for (const auto *selectCase : selectExpr->selectCases) {
            const auto *decl = state.findDecl(selectCase->state)->getNode();
            decl->apply(CollectLatentStatements(statements, state, seenParserIds));
        }
    } else if (const auto *pathExpression =
                   parserState->selectExpression->to<IR::PathExpression>()) {
        // Only bother looking up cases that are not accept or reject.
        // If we are referencing a parser state, step into the state.
        const auto *decl = state.findDecl(pathExpression)->getNode();
        decl->apply(CollectLatentStatements(statements, state, seenParserIds));
    }
    return true;
}

bool CollectLatentStatements::preorder(const IR::AssignmentStatement *stmt) {
    // Only track statements, which have a valid source position in the P4 program.
    if (stmt->getSourceInfo().isValid()) {
        statements.insert(stmt);
    }
    return true;
}

bool CollectLatentStatements::preorder(const IR::MethodCallStatement *stmt) {
    // Only track statements, which have a valid source position in the P4 program.
    if (stmt->getSourceInfo().isValid()) {
        statements.insert(stmt);
    }
    return true;
}

bool CollectLatentStatements::preorder(const IR::ExitStatement *stmt) {
    // Only track statements, which have a valid source position in the P4 program.
    if (stmt->getSourceInfo().isValid()) {
        statements.insert(stmt);
    }
    return true;
}

CollectLatentStatements::CollectLatentStatements(P4::Coverage::CoverageSet &output,
                                                 const ExecutionState &state)
    : statements(output), state(state) {}

CollectLatentStatements::CollectLatentStatements(P4::Coverage::CoverageSet &output,
                                                 const ExecutionState &state,
                                                 const std::set<int> &seenParserIds)
    : statements(output), state(state), seenParserIds(seenParserIds) {}

}  // namespace P4Tools::P4Testgen
