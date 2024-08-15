#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/cstring.h"

namespace P4::Test {
using namespace P4::literals;
using ConstraintVector = const std::vector<const Constraint *>;

class Z3SolverSatisfiabilityChecks : public ::testing::Test {
    P4Tools::Z3Solver solver;

 public:
    /// Checks whether the result of the solver calculating @param expression matches @param
    /// expectedResult.
    void testCheckSat(const ConstraintVector &expression, std::optional<bool> expectedResult) {
        auto result = solver.checkSat(expression);
        EXPECT_EQ(result, expectedResult);
    }
};

TEST_F(Z3SolverSatisfiabilityChecks, BitVectors) {
    const auto *eightBitType = IR::Type_Bits::get(8);
    const auto *fooVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "foo"_cs);
    const auto *barVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "bar"_cs);
    {
        auto *expression =
            new IR::Equ(IR::Constant::get(eightBitType, 1), IR::Constant::get(eightBitType, 1));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression =
            new IR::Equ(IR::Constant::get(eightBitType, 1), IR::Constant::get(eightBitType, 2));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, false);
    }
    {
        auto *expression = new IR::Equ(fooVar, IR::Constant::get(eightBitType, 1));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, IR::Constant::get(eightBitType, 1));
        auto *constraint1 = new IR::Equ(fooVar, IR::Constant::get(eightBitType, 1));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, IR::Constant::get(eightBitType, 1));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, IR::Constant::get(eightBitType, 1));
        auto *constraint2 = new IR::Equ(barVar, IR::Constant::get(eightBitType, 1));
        ConstraintVector inputExpression = {expression, constraint1, constraint2};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, IR::Constant::get(eightBitType, 1));
        auto *constraint1 = new IR::Equ(fooVar, IR::Constant::get(eightBitType, 2));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, false);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, IR::Constant::get(eightBitType, 1));
        auto *constraint2 = new IR::Equ(barVar, IR::Constant::get(eightBitType, 2));
        ConstraintVector inputExpression = {expression, constraint1, constraint2};
        testCheckSat(inputExpression, false);
    }
}

TEST_F(Z3SolverSatisfiabilityChecks, Strings) {
    const auto *stringType = IR::Type_String::get();
    const auto *fooVar = P4Tools::ToolsVariables::getSymbolicVariable(stringType, "foo"_cs);
    const auto *barVar = P4Tools::ToolsVariables::getSymbolicVariable(stringType, "bar"_cs);
    {
        auto *expression =
            new IR::Equ(IR::StringLiteral::get("dead"_cs), IR::StringLiteral::get("dead"_cs));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression =
            new IR::Equ(IR::StringLiteral::get("dead"_cs), IR::StringLiteral::get("beef"_cs));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, false);
    }
    {
        auto *expression = new IR::Equ(fooVar, IR::StringLiteral::get("dead"_cs));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, IR::StringLiteral::get("dead"_cs));
        auto *constraint1 = new IR::Equ(fooVar, IR::StringLiteral::get("dead"_cs));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, IR::StringLiteral::get("dead"_cs));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, IR::StringLiteral::get("dead"_cs));
        auto *constraint2 = new IR::Equ(barVar, IR::StringLiteral::get("dead"_cs));
        ConstraintVector inputExpression = {expression, constraint1, constraint2};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, IR::StringLiteral::get("dead"_cs));
        auto *constraint1 = new IR::Equ(fooVar, IR::StringLiteral::get("beef"_cs));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, false);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, IR::StringLiteral::get("dead"_cs));
        auto *constraint2 = new IR::Equ(barVar, IR::StringLiteral::get("beef"_cs));
        ConstraintVector inputExpression = {expression, constraint1, constraint2};
        testCheckSat(inputExpression, false);
    }
}

TEST_F(Z3SolverSatisfiabilityChecks, Bools) {
    const auto *boolType = IR::Type_Boolean::get();
    const auto *fooVar = P4Tools::ToolsVariables::getSymbolicVariable(boolType, "foo"_cs);
    const auto *barVar = P4Tools::ToolsVariables::getSymbolicVariable(boolType, "bar"_cs);
    {
        auto *expression = new IR::Equ(IR::BoolLiteral::get(true), IR::BoolLiteral::get(true));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(IR::BoolLiteral::get(true), IR::BoolLiteral::get(false));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, false);
    }
    {
        auto *expression = new IR::Equ(fooVar, IR::BoolLiteral::get(true));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, IR::BoolLiteral::get(true));
        auto *constraint1 = new IR::Equ(fooVar, IR::BoolLiteral::get(true));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, IR::BoolLiteral::get(true));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, IR::BoolLiteral::get(true));
        auto *constraint2 = new IR::Equ(barVar, IR::BoolLiteral::get(true));
        ConstraintVector inputExpression = {expression, constraint1, constraint2};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, IR::BoolLiteral::get(true));
        auto *constraint1 = new IR::Equ(fooVar, IR::BoolLiteral::get(false));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, false);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, IR::BoolLiteral::get(true));
        auto *constraint2 = new IR::Equ(barVar, IR::BoolLiteral::get(false));
        ConstraintVector inputExpression = {expression, constraint1, constraint2};
        testCheckSat(inputExpression, false);
    }
}

}  // namespace P4::Test
