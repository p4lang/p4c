#include "midend/coverage.h"

#include <ostream>

#include "lib/log.h"

namespace P4::Coverage {

bool SourceIdCmp::operator()(const IR::Node *s1, const IR::Node *s2) const {
    // We use source information as comparison operator since we want to trace coverage back to the
    // original program.
    // TODO: Some compiler passes may not preserve source information correctly (e.g.,
    // side effect ordering). Fix these passes.
    return s1->srcInfo < s2->srcInfo;
}

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
            const auto &srcInfo = node->getSourceInfo();
            auto sourceLine = srcInfo.toPosition().sourceLine;
            LOG_FEATURE("coverage", 4,
                        '\t' << srcInfo.getSourceFile() << "\\" << sourceLine << ": " << *node);
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
