#include "midend/coverage.h"

#include <ostream>

#include "lib/log.h"

namespace P4::Coverage {

CollectStatements::CollectStatements(CoverageSet &output) : statements(output) {}

bool CollectStatements::preorder(const IR::AssignmentStatement *stmt) {
    // Only track statements, which have a valid source position in the P4 program.
    if (stmt->getSourceInfo().isValid()) {
        statements.insert(stmt);
    }
    return true;
}

bool CollectStatements::preorder(const IR::MethodCallStatement *stmt) {
    // Only track statements, which have a valid source position in the P4 program.
    if (stmt->getSourceInfo().isValid()) {
        statements.insert(stmt);
    }
    return true;
}

bool CollectStatements::preorder(const IR::ExitStatement *stmt) {
    // Only track statements, which have a valid source position in the P4 program.
    if (stmt->getSourceInfo().isValid()) {
        statements.insert(stmt);
    }
    return true;
}

void coverageReportFinal(const CoverageSet &all, const CoverageSet &visited) {
    LOG_FEATURE("coverage", 4, "Not covered statements:");
    for (const auto *stmt : all) {
        if (visited.count(stmt) == 0) {
            auto sourceLine = stmt->getSourceInfo().toPosition().sourceLine;
            LOG_FEATURE("coverage", 4, '\t' << sourceLine << ": " << *stmt);
        }
    }
    // Do not really need to know which statements we have covered. Increase the log level here.
    LOG_FEATURE("coverage", 5, "Covered statements:");
    for (const IR::Statement *stmt : visited) {
        auto sourceLine = stmt->getSourceInfo().toPosition().sourceLine;
        LOG_FEATURE("coverage", 5, '\t' << sourceLine << ": " << *stmt);
    }
}

void logCoverage(const CoverageSet &all, const CoverageSet &visited, const CoverageSet &new_) {
    for (const IR::Statement *stmt : new_) {
        if (visited.count(stmt) == 0) {
            // Search for the original statement - `stmt` might have been transformed.
            const IR::Statement *originalStatement = *all.find(stmt);
            LOG_FEATURE(
                "coverage", 4,
                "============ Covered new statement " << originalStatement << "============");
        }
    }
}

}  // namespace P4::Coverage
