#include "midend/coverage.h"

#include <ostream>

#include "lib/log.h"

namespace P4::Coverage {

CollectNodes::CollectNodes(CoverageOptions coverageOptions) : coverageOptions(coverageOptions) {}

bool CollectNodes::preorder(const IR::AssignmentStatement *stmt) {
    // Only track statements, which have a valid source position in the P4 program.
    if (coverageOptions.coverStatements && stmt->getSourceInfo().isValid()) {
        coverableNodes.insert(stmt);
    }
    return true;
}

bool CollectNodes::preorder(const IR::Entry *entry) {
    // Only track entries, which have a valid source position in the P4 program.
    if (coverageOptions.coverTableEntries && entry->getSourceInfo().isValid()) {
        coverableNodes.insert(entry);
    }
    return true;
}

bool CollectNodes::preorder(const IR::MethodCallStatement *stmt) {
    // Only track statements, which have a valid source position in the P4 program.
    if (coverageOptions.coverStatements && stmt->getSourceInfo().isValid()) {
        coverableNodes.insert(stmt);
    }
    return true;
}

bool CollectNodes::preorder(const IR::ExitStatement *stmt) {
    // Only track statements, which have a valid source position in the P4 program.
    if (coverageOptions.coverStatements && stmt->getSourceInfo().isValid()) {
        coverableNodes.insert(stmt);
    }
    return true;
}

void printCoverageReport(const CoverageSet &all, const CoverageSet &visited) {
    if (all.empty()) {
        return;
    }
    LOG_FEATURE("coverage", 4, "Not covered program nodes:");
    for (const auto *node : all) {
        if (visited.count(node) == 0) {
            auto sourceLine = node->getSourceInfo().toPosition().sourceLine;
            LOG_FEATURE("coverage", 4, '\t' << sourceLine << ": " << *node);
        }
    }
    // Do not really need to know which program nodes we have covered. Increase the log level here.
    LOG_FEATURE("coverage", 5, "Covered program nodes:");
    for (const auto *node : visited) {
        auto sourceLine = node->getSourceInfo().toPosition().sourceLine;
        LOG_FEATURE("coverage", 5, '\t' << sourceLine << ": " << *node);
    }
}

void logCoverage(const CoverageSet &all, const CoverageSet &visited, const CoverageSet &new_) {
    for (const auto *node : new_) {
        if (visited.count(node) == 0) {
            // Search for the original statement - `node` might have been transformed.
            const auto *originalNode = *all.find(node);
            LOG_FEATURE("coverage", 4,
                        "============ Covered new node " << originalNode << "============");
        }
    }
}

const CoverageSet &CollectNodes::getCoverableNodes() { return coverableNodes; }

}  // namespace P4::Coverage
