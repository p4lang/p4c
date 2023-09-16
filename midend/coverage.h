#ifndef MIDEND_COVERAGE_H_
#define MIDEND_COVERAGE_H_

#include <set>
#include <vector>

#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/source_file.h"

/// This file is a collection of utilities for coverage tracking in P4 programs.
/// Currently, this class tracks statement and constant table entry coverage.
/// The utilities here can be used by interpreters that need to walk the IR of the P4 program and
/// track which percentage of nodes they have visited.
/// The p4tools (backends/p4tools) framework uses this coverage visitor.

namespace P4::Coverage {

/// Utility function to compare IR nodes in a set. We use their source info.
struct SourceIdCmp {
    bool operator()(const IR::Node *s1, const IR::Node *s2) const;
};

/// Specifies general options and which IR nodes to track with this particular visitor.
struct CoverageOptions {
    /// Cover IR::Statement.
    bool coverStatements = false;
    /// Cover IR::Entry
    bool coverTableEntries = false;
    /// Skip tests which do not increase coverage.
    bool onlyCoveringTests = false;
};

/// Set of nodes used for coverage purposes. Compares nodes based on their
/// clone_id to take node modifications into account.
using CoverageSet = std::set<const IR::Node *, SourceIdCmp>;

/// CollectNodes iterates across selected nodes in the P4 program and collects them in a
/// "CoverageSet". The nodes to collect are specified as options to the collector.
class CollectNodes : public Inspector {
    /// The set of nodes in the program that could potentially be covered.
    CoverageSet coverableNodes;

    /// Specifies, which IR nodes to track with this particular visitor.
    CoverageOptions coverageOptions;

    /// Statement coverage.
    bool preorder(const IR::AssignmentStatement *stmt) override;
    bool preorder(const IR::MethodCallStatement *stmt) override;
    bool preorder(const IR::ExitStatement *stmt) override;

    /// Table entry coverage.
    bool preorder(const IR::Entry *entry) override;

 public:
    explicit CollectNodes(CoverageOptions coverageOptions);

    /// @return the set of coverable nodes in the program.
    const CoverageSet &getCoverableNodes();
};

/// Produces detailed final coverage log.
void printCoverageReport(const CoverageSet &all, const CoverageSet &visited);

/// Logs nodes from @p new_ which have not yet been visited (are not members of @p visited).
void logCoverage(const CoverageSet &all, const CoverageSet &visited, const CoverageSet &new_);

}  // namespace P4::Coverage

#endif /* MIDEND_COVERAGE_H_ */
