#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/expressions.h"
#include "backends/p4tools/modules/smith/common/generator.h"
#include "backends/p4tools/modules/smith/common/parser.h"
#include "backends/p4tools/modules/smith/common/probabilities.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "backends/p4tools/modules/smith/common/table.h"
#include "backends/p4tools/modules/smith/smith.h"
#include "backends/p4tools/modules/smith/targets/generic/target.h"
#include "gtest/gtest.h"
#include "ir/ir.h"

namespace p4c::Test {

class P4SmithForLoopTest : public ::testing::Test {
 protected:
    P4Tools::P4Smith::StatementGenerator *generator;

    // Set up a test fixture, which allows us to reuse the same configuration of objects
    // for several different tests.
    void SetUp() override {
        P4Tools::P4Smith::Generic::GenericCoreSmithTarget *target =
            P4Tools::P4Smith::Generic::GenericCoreSmithTarget::getInstance();
        generator = &target->statementGenerator();
    }
    void TearDown() override {
        delete generator;
        generator = nullptr;
    }
};

/// @brief Test the generation of a for-loop statement.
TEST_F(P4SmithForLoopTest, ForLoopGeneration) {
    auto forLoopStmt = generator->genForLoopStatement(false);
    ASSERT_NE(forLoopStmt, nullptr);
    EXPECT_TRUE(forLoopStmt->is<IR::ForStatement>());
}

/// @brief Test the for-loop's initialization.
TEST_F(P4SmithForLoopTest, CheckForLoopInitialization) {
    auto forLoopStmt = generator->genForLoopStatement(false);
    ASSERT_NE(forLoopStmt, nullptr);
    EXPECT_TRUE(forLoopStmt->is<IR::ForStatement>());

    auto forStmt = forLoopStmt->to<IR::ForStatement>();
    ASSERT_FALSE(forStmt->init.empty());
}

/// @brief Test the for-loop's condition.
TEST_F(P4SmithForLoopTest, CheckForLoopCondition) {
    auto forLoopStmt = generator->genForLoopStatement(false);
    ASSERT_NE(forLoopStmt, nullptr);
    EXPECT_TRUE(forLoopStmt->is<IR::ForStatement>());

    auto forStmt = forLoopStmt->to<IR::ForStatement>();
    ASSERT_NE(forStmt->condition, nullptr);
    EXPECT_TRUE(forStmt->condition->is<IR::Expression>());
}

/// @brief Test the for-loop's update.
TEST_F(P4SmithForLoopTest, CheckForLoopUpdate) {
    auto forLoopStmt = generator->genForLoopStatement(false);
    ASSERT_NE(forLoopStmt, nullptr);
    EXPECT_TRUE(forLoopStmt->is<IR::ForStatement>());

    auto forStmt = forLoopStmt->to<IR::ForStatement>();
    ASSERT_FALSE(forStmt->updates.empty());
}

/// @brief Test the for-loop's body.
TEST_F(P4SmithForLoopTest, CheckForLoopBody) {
    auto forLoopStmt = generator->genForLoopStatement(false);
    ASSERT_NE(forLoopStmt, nullptr);
    EXPECT_TRUE(forLoopStmt->is<IR::ForStatement>());

    auto forStmt = forLoopStmt->to<IR::ForStatement>();
    ASSERT_NE(forStmt->body, nullptr);
    EXPECT_FALSE(forStmt->body->is<IR::EmptyStatement>());
}

}  // namespace p4c::Test
