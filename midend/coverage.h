#ifndef MIDEND_COVERAGE_H_
#define MIDEND_COVERAGE_H_

#include <set>
#include <vector>

#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/source_file.h"

/// This file is a collection of utilities for coverage tracking in P4 programs. Currently, this
/// class merely tracks statement coverage. The utilities here can be used by interpreters that need
/// to walk the IR of the P4 program and track which percentage of statement they have visited. The
/// p4tools (backends/p4tools) framework uses this coverage visitor.

namespace P4 {

namespace Coverage {

struct SourceIdCmp {
    bool operator()(const IR::Statement *s1, const IR::Statement *s2) const {
        return s1->srcInfo < s2->srcInfo;
    }
};

/// Set of statements used for coverage purposes. Compares statements based on their
/// clone_id to take statement modifications into account.
using CoverageSet = std::set<const IR::Statement *, SourceIdCmp>;

/// CollectStatements iterates across all assignment, method call, and exit statements in the P4
/// program and collects them in a "CoverageSet".
class CollectStatements : public Inspector {
    CoverageSet &statements;

 public:
    explicit CollectStatements(CoverageSet &output);
    bool preorder(const IR::AssignmentStatement *stmt) override;
    bool preorder(const IR::MethodCallStatement *stmt) override;
    bool preorder(const IR::ExitStatement *stmt) override;
};

/// Produces detailed final coverage log.
void coverageReportFinal(const CoverageSet &all, const CoverageSet &visited);

/// Logs statements from @p new_ which have not yet been visited (are not members of @p visited).
void logCoverage(const CoverageSet &all, const CoverageSet &visited,
                 const std::vector<const IR::Statement *> &new_);

}  // namespace Coverage

}  // namespace P4

#endif /* MIDEND_COVERAGE_H_ */
