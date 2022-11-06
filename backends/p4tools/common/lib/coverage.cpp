#include "backends/p4tools/common/lib/coverage.h"

#include <ostream>

#include "lib/log.h"

namespace P4Tools {

namespace Coverage {

CollectStatements::CollectStatements(CoverageSet& output) : statements(output) {}

bool CollectStatements::preorder(const IR::AssignmentStatement* stmt) {
    statements.insert(stmt);
    return true;
}
bool CollectStatements::preorder(const IR::MethodCallStatement* stmt) {
    statements.insert(stmt);
    return true;
}
bool CollectStatements::preorder(const IR::ExitStatement* stmt) {
    statements.insert(stmt);
    return true;
}

void coverageReportFinal(const CoverageSet& all, const CoverageSet& visited) {
    LOG_FEATURE("coverage", 4, "Not covered statements:");
    for (const IR::Statement* stmt : all) {
        if (visited.count(stmt) == 0) {
            LOG2('\t' << *stmt);
        }
    }
    LOG_FEATURE("coverage", 4, "Covered statements:");
    for (const IR::Statement* stmt : visited) {
        LOG_FEATURE("coverage", 4, '\t' << *stmt);
    }
}

void logCoverage(const CoverageSet& all, const CoverageSet& visited,
                 const std::vector<const IR::Statement*>& new_) {
    for (const IR::Statement* stmt : new_) {
        if (all.count(stmt) == 0) {
            // Do not log internal statements, which don't correspond to any statement
            // in the original P4 program.
            continue;
        }
        if (visited.count(stmt) == 0) {
            // Search for the original statement - `stmt` might have been transformed.
            const IR::Statement* originalStatement = *all.find(stmt);
            LOG_FEATURE(
                "coverage", 4,
                "============ Covered new statement " << originalStatement << "============");
        }
    }
}

}  // namespace Coverage

}  // namespace P4Tools
