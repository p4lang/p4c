#include "backends/p4tools/modules/testgen/lib/collect_coverable_statements.h"

#include <string>
#include <vector>

#include "backends/p4tools/common/lib/table_utils.h"
#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/null.h"
#include "lib/source_file.h"

#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools::P4Testgen {

bool CoverableNodesScanner::preorder(const IR::ParserState *parserState) {
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

    parserState->components.apply_visitor_preorder(*this);

    if (const auto *selectExpr = parserState->selectExpression->to<IR::SelectExpression>()) {
        for (const auto *selectCase : selectExpr->selectCases) {
            const auto *decl = state.findDecl(selectCase->state)->getNode();
            CoverableNodesScanner(state).updateNodeCoverage(decl, coverableNodes);
        }
    } else if (const auto *pathExpression =
                   parserState->selectExpression->to<IR::PathExpression>()) {
        // Only bother looking up cases that are not accept or reject.
        // If we are referencing a parser state, step into the state.
        const auto *decl = state.findDecl(pathExpression)->getNode();
        CoverableNodesScanner(state).updateNodeCoverage(decl, coverableNodes);
    }
    return true;
}

bool CoverableNodesScanner::preorder(const IR::AssignmentStatement *stmt) {
    // Only track statements, which have a valid source position in the P4 program.
    if (coverageOptions.coverStatements && stmt->getSourceInfo().isValid()) {
        coverableNodes.insert(stmt);
    }
    return true;
}

bool CoverableNodesScanner::preorder(const IR::MethodCallStatement *stmt) {
    // Only track statements, which have a valid source position in the P4 program.
    if (coverageOptions.coverStatements && stmt->getSourceInfo().isValid()) {
        coverableNodes.insert(stmt);
    }
    const auto *call = stmt->methodCall;
    // Handle method calls. These are either table invocations or extern calls.
    if (call->method->type->is<IR::Type_Method>()) {
        if (const auto *method = call->method->to<IR::Member>()) {
            // Handle table calls.
            const auto *table = state.getTableType(method);
            if (table == nullptr) {
                return false;
            }
            TableUtils::TableProperties properties;
            TableUtils::checkTableImmutability(table, properties);
            auto tableActionList = TableUtils::buildTableActionList(table);
            if (properties.tableIsImmutable) {
                const auto *entries = table->getEntries();
                if (entries != nullptr) {
                    auto entryVector = entries->entries;
                    for (const auto &entry : entryVector) {
                        if (coverageOptions.coverTableEntries && entry->getSourceInfo().isValid()) {
                            coverableNodes.insert(entry);
                        }
                    }
                }
            } else {
                for (const auto &action : tableActionList) {
                    const auto *tableAction =
                        action->expression->checkedTo<IR::MethodCallExpression>();
                    const auto *actionType = state.getActionDecl(tableAction);
                    CoverableNodesScanner(state).updateNodeCoverage(actionType->body,
                                                                    coverableNodes);
                }
            }
            const auto *defaultAction = table->getDefaultAction();
            const auto *tableAction = defaultAction->checkedTo<IR::MethodCallExpression>();
            const auto *actionType = state.getActionDecl(tableAction);
            CoverableNodesScanner(state).updateNodeCoverage(actionType->body, coverableNodes);
        }
        return false;
    }
    if (call->method->type->is<IR::Type_Action>()) {
        const auto *action = state.getActionDecl(call);
        CoverableNodesScanner(state).updateNodeCoverage(action->body, coverableNodes);
        return false;
    }

    return true;
}

bool CoverableNodesScanner::preorder(const IR::ExitStatement *stmt) {
    // Only track statements, which have a valid source position in the P4 program.
    if (coverageOptions.coverStatements && stmt->getSourceInfo().isValid()) {
        coverableNodes.insert(stmt);
    }
    return true;
}

const P4::Coverage::CoverageSet &CoverableNodesScanner::getCoverableNodes() {
    return coverableNodes;
}

void CoverableNodesScanner::updateNodeCoverage(const IR::Node *node,
                                               P4::Coverage::CoverageSet &nodes) {
    node->apply(*this);
    nodes.insert(coverableNodes.begin(), coverableNodes.end());
}

CoverableNodesScanner::CoverableNodesScanner(const ExecutionState &state)
    : state(state), coverageOptions(TestgenOptions::get().coverageOptions) {}
}  // namespace P4Tools::P4Testgen
