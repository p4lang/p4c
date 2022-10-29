#ifndef BACKENDS_P4TOOLS_COMMON_LIB_COVERAGE_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_COVERAGE_H_

#include <set>
#include <vector>

#include "ir/ir.h"
#include "lib/source_file.h"

namespace P4Tools {

namespace Coverage {

struct SourceIdCmp {
    bool operator()(const IR::Statement* s1, const IR::Statement* s2) const {
        return s1->srcInfo < s2->srcInfo;
    }
};

// Set of statements used for coverage purposes. Compares statements based on their
// clone_id to take statement modifications into account.
using CoverageSet = std::set<const IR::Statement*, SourceIdCmp>;

class CollectStatements : public Inspector {
    CoverageSet& statements;

 public:
    explicit CollectStatements(CoverageSet& output);
    bool preorder(const IR::AssignmentStatement* stmt) override;
    bool preorder(const IR::MethodCallStatement* stmt) override;
    bool preorder(const IR::ExitStatement* stmt) override;
};

/// Produces detailed final coverage log.
void coverageReportFinal(const CoverageSet& all, const CoverageSet& visited);

/// Logs statements from @p new_ which have not yet been visited (are not members of @p visited).
void logCoverage(const CoverageSet& all, const CoverageSet& visited,
                 const std::vector<const IR::Statement*>& new_);

}  // namespace Coverage

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_COVERAGE_H_ */
