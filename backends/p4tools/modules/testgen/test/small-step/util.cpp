// SPDX-FileCopyrightText: 2022 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include "util.h"

#include <memory>

namespace P4::P4Tools::Test {

TEST_F(SmallStepTest, CoverageCacheFollowsExecutionBranches) {
    auto context = P4TestgenTest::SetUp("bmv2", "v1model");
    ASSERT_NE(context, nullptr);
    auto first = mkState(Body({}));
    auto second = mkState(Body({}));
    std::unique_ptr<ExecutionState> branch(&first.clone());
    auto *node = new IR::Constant(1);
    first.getCoverageCache()[node].insert(node);
    EXPECT_TRUE(branch->getCoverageCache().at(node).count(node));
    EXPECT_TRUE(second.getCoverageCache().empty());
}

}  // namespace P4::P4Tools::Test
