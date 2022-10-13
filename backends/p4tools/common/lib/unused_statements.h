#ifndef BACKENDS_P4TOOLS_COMMON_LIB_UNUSED_STATEMENTS_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_UNUSED_STATEMENTS_H_

#include <algorithm>
#include <ctime>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "backends/p4tools/common/lib/coverage.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "frontends/common/options.h"
#include "ir/ir.h"
#include "lib/compile_context.h"

namespace P4Tools {

namespace UnusedStatements {

class CollectStatements : public Inspector {
    IR::StatementList& statements;

 public:
    explicit CollectStatements(IR::StatementList& output);
    bool preorder(const IR::AssignmentStatement* stmt) override;
    bool preorder(const IR::MethodCallStatement* stmt) override;
    bool preorder(const IR::SwitchStatement* stmt) override;
    bool preorder(const IR::IfStatement* stmt) override;
    bool preorder(const IR::SelectExpression* stmt) override;
    bool preorder(const IR::P4Table* stmt) override;
    bool preorder(const IR::ExitStatement* stmt) override;
};

class RemoveStatements : public Transform {
    IR::Vector<IR::Node> branch;
    std::set<cstring> includes;

 public:
    explicit RemoveStatements(IR::Vector<IR::Node> branch);
    const IR::Node* preorder(IR::Statement* stmt) override;
    const IR::Node* preorder(IR::P4Table* table) override;
    const IR::Node* preorder(IR::SelectExpression* expr) override;
    const IR::Node* preorder(IR::Node* node) override;
};

class GenerateBranches : public Inspector {
    std::vector<std::tuple<const IR::Node*, size_t, bool>>& unusedStmt;
    std::set<cstring> includes;
    std::vector<std::pair<const IR::Node*, IR::Vector<IR::Node>>>& branches;
    int testCount = 0;

 public:
    explicit GenerateBranches(
        std::vector<std::tuple<const IR::Node*, size_t, bool>>& output,
        std::vector<std::pair<const IR::Node*, IR::Vector<IR::Node>>>& branches, int testCount);
    std::vector<IR::Vector<IR::Node>> genAllPathes(IR::Vector<IR::Node> initialPath);
    std::vector<IR::Vector<IR::Node>> genPathChains();
    bool preorder(const IR::Statement* stmt) override;
    bool preorder(const IR::P4Table* table) override;
    bool preorder(const IR::SelectExpression* expr) override;
};

std::vector<std::tuple<const IR::Node*, size_t, bool>> getUnusedStatements(
    const std::vector<gsl::not_null<const TraceEvent*>>& traces,
    const IR::StatementList& allstatements, const Coverage::CoverageSet visited);
void generateTests(std::vector<std::tuple<const IR::Node*, size_t, bool>> unusedStatements,
                   int testCount, size_t testIdx);

}  // namespace UnusedStatements

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_UNUSED_STATEMENTS_H_ */
