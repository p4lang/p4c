#include "gtest/gtest.h"
#include "backends/p4tools/modules/smith/smith.h"
#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/expressions.h"
#include "backends/p4tools/modules/smith/common/generator.h"
#include "backends/p4tools/modules/smith/common/parser.h"
#include "backends/p4tools/modules/smith/common/probabilities.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "backends/p4tools/modules/smith/common/table.h"
#include "backends/p4tools/modules/smith/core/target.h"
#include "ir/ir.h"
#include "ir/ir-generated.h"

namespace Test {

class P4SmithForLoopTest : public ::testing::Test {
protected:
    P4Tools::P4Smith::StatementGenerator* generator;

    // TODO(zzmic): Figure out how to initialize and clean up the test object.
    P4SmithForLoopTest() {}
    ~P4SmithForLoopTest() override {}
};

/// @brief Test the generation of a for loop statement.
TEST_F(P4SmithForLoopTest, ForLoopGeneration) {
    auto forLoopStmt = generator->genForLoopStatement(false);
    ASSERT_NE(forLoopStmt, nullptr);
    EXPECT_TRUE(forLoopStmt->is<IR::ForStatement>());
}

/// @brief Test whether the for loop contains an initialization.
TEST_F(P4SmithForLoopTest, ForLoopContainsInitialization) {
    auto forLoopStmt = generator->genForLoopStatement(false);
    ASSERT_NE(forLoopStmt, nullptr);
    EXPECT_TRUE(forLoopStmt->is<IR::ForStatement>());

    // TODO(zzmic): Figure out whether this "static_cast" is necessary.
    auto forStmt = forLoopStmt->to<IR::ForStatement>();
    ASSERT_NE(forStmt->init, nullptr);    
    EXPECT_FALSE(forStmt->init.empty());
}

/// @brief Test whether the for loop contains a condition. 
TEST_F(P4SmithForLoopTest, ForLoopContainsCondition) {
    auto forLoopStmt = generator->genForLoopStatement(false);
    ASSERT_NE(forLoopStmt, nullptr);
    EXPECT_TRUE(forLoopStmt->is<IR::ForStatement>());

    // TODO(zzmic): Figure out whether this "static_cast" is necessary.
    auto forStmt = forLoopStmt->to<IR::ForStatement>();
    ASSERT_NE(forStmt->condition, nullptr);
    EXPECT_TRUE(forStmt->condition->is<IR::Expression>());
}

/// @brief Test whether the for loop contains an update.
TEST_F(P4SmithForLoopTest, ForLoopContainsUpdate) {
    auto forLoopStmt = generator->genForLoopStatement(false);
    ASSERT_NE(forLoopStmt, nullptr);
    EXPECT_TRUE(forLoopStmt->is<IR::ForStatement>());

    // TODO(zzmic): Figure out whether this "static_cast" is necessary.
    auto forStmt = forLoopStmt->to<IR::ForStatement>();
    ASSERT_NE(forStmt->updates, nullptr);
    EXPECT_FALSE(forStmt->updates.empty());
}

/// @brief Test whether the for loop contains a body.
TEST_F(P4SmithForLoopTest, ForLoopContainsBody) {
    auto forLoopStmt = generator->genForLoopStatement(false);
    ASSERT_NE(forLoopStmt, nullptr);
    EXPECT_TRUE(forLoopStmt->is<IR::ForStatement>());

    // TODO(zzmic): Figure out whether this "static_cast" is necessary.
    auto forStmt = forLoopStmt->to<IR::ForStatement>();
    ASSERT_NE(forStmt->body, nullptr);
    EXPECT_FALSE(forStmt->body->is<IR::Statement>());
}

} // namespace Test