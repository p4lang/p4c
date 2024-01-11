#include "backends/p4tools/modules/testgen/lib/collect_coverable_nodes.h"

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
    // Already have seen this executionState. We might be in a loop.
    if (seenParserIds.count(parserState->clone_id) != 0) {
        return false;
    }

    // Only bother looking up cases that are not accept or reject.
    if (parserState->name == IR::ParserState::accept ||
        parserState->name == IR::ParserState::reject) {
        return true;
    }
    seenParserIds.emplace(parserState->clone_id);

    CHECK_NULL(parserState->selectExpression);
    const auto &executionState = state.get();

    if (const auto *selectExpr = parserState->selectExpression->to<IR::SelectExpression>()) {
        for (const auto *selectCase : selectExpr->selectCases) {
            const auto *decl = executionState.findDecl(selectCase->state)->getNode();
            decl->apply_visitor_preorder(*this);
        }
    } else if (const auto *pathExpression =
                   parserState->selectExpression->to<IR::PathExpression>()) {
        // Only bother looking up cases that are not accept or reject.
        // If we are referencing a parser state, step into the executionState.
        const auto *decl = executionState.findDecl(pathExpression)->getNode();
        decl->apply_visitor_preorder(*this);
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

bool CoverableNodesScanner::preorder(const IR::MethodCallExpression *call) {
    const auto &executionState = state.get();
    // Handle method calls. These are either table invocations or extern calls.
    if (call->method->type->is<IR::Type_Method>()) {
        if (const auto *member = call->method->to<IR::Member>()) {
            // Handle table calls.
            const auto *table = executionState.findTable(member);
            if (table == nullptr) {
                return true;
            }
            TableUtils::TableProperties properties;
            TableUtils::checkTableImmutability(*table, properties);
            auto tableActionList = TableUtils::buildTableActionList(*table);
            if (properties.tableIsImmutable) {
                const auto *entries = table->getEntries();
                if (entries != nullptr) {
                    auto entryVector = entries->entries;
                    for (const auto *entry : entryVector) {
                        if (coverageOptions.coverTableEntries && entry->getSourceInfo().isValid()) {
                            coverableNodes.insert(entry);
                        }
                        const auto *tableAction =
                            entry->action->checkedTo<IR::MethodCallExpression>();
                        const auto *actionType = executionState.getP4Action(tableAction);
                        actionType->apply_visitor_preorder(*this);
                    }
                }
            } else {
                for (const auto &action : tableActionList) {
                    const auto *tableAction =
                        action->expression->checkedTo<IR::MethodCallExpression>();
                    const auto *actionType = executionState.getP4Action(tableAction);
                    actionType->apply_visitor_preorder(*this);
                }
            }
            const auto *defaultAction = table->getDefaultAction();
            const auto *tableAction = defaultAction->checkedTo<IR::MethodCallExpression>();
            const auto *actionType = executionState.getP4Action(tableAction);
            actionType->apply_visitor_preorder(*this);
        }
        return false;
    }
    if (call->method->type->is<IR::Type_Action>()) {
        const auto *actionType = executionState.getP4Action(call);
        actionType->apply_visitor_preorder(*this);
        return false;
    }
    return false;
}

bool CoverableNodesScanner::preorder(const IR::MethodCallStatement *stmt) {
    // Only track statements, which have a valid source position in the P4 program.
    if (coverageOptions.coverStatements && stmt->getSourceInfo().isValid()) {
        coverableNodes.insert(stmt);
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

bool CoverableNodesScanner::preorder(const IR::P4Action *act) {
    // Only track actions, which have a valid source position in the P4 program.
    if (coverageOptions.coverActions && act->getSourceInfo().isValid()) {
        coverableNodes.insert(act);
    }
    // Visit body only.
    act->body->apply_visitor_preorder(*this);
    return false;
}

const P4::Coverage::CoverageSet &CoverableNodesScanner::getCoverableNodes() {
    return coverableNodes;
}

void CoverableNodesScanner::updateNodeCoverage(const IR::Node *node,
                                               P4::Coverage::CoverageSet &nodes) {
    CHECK_NULL(node);

    static NodeCache CACHED_NODES;
    // If the node is already in the cache, return it.
    auto it = CACHED_NODES.find(node);
    if (it != CACHED_NODES.end()) {
        nodes.insert(it->second.begin(), it->second.end());
        return;
    }
    node->apply(*this);
    nodes.insert(coverableNodes.begin(), coverableNodes.end());
    // Store the result in the cache.
    CACHED_NODES.emplace(node, coverableNodes);
}

CoverableNodesScanner::CoverableNodesScanner(const ExecutionState &state)
    : state(state), coverageOptions(TestgenOptions::get().coverageOptions) {}
}  // namespace P4Tools::P4Testgen
