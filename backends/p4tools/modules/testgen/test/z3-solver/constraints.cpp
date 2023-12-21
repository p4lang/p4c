#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/variables.h"
#include "ir/ir-generated.h"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "lib/cstring.h"

namespace Test {
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
    const auto *eightBitType = IR::getBitType(8);
    const auto *fooVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "foo");
    const auto *barVar = P4Tools::ToolsVariables::getSymbolicVariable(eightBitType, "bar");
    {
        auto *expression =
            new IR::Equ(IR::getConstant(eightBitType, 1), IR::getConstant(eightBitType, 1));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression =
            new IR::Equ(IR::getConstant(eightBitType, 1), IR::getConstant(eightBitType, 2));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, false);
    }
    {
        auto *expression = new IR::Equ(fooVar, IR::getConstant(eightBitType, 1));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, IR::getConstant(eightBitType, 1));
        auto *constraint1 = new IR::Equ(fooVar, IR::getConstant(eightBitType, 1));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, IR::getConstant(eightBitType, 1));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, IR::getConstant(eightBitType, 1));
        auto *constraint2 = new IR::Equ(barVar, IR::getConstant(eightBitType, 1));
        ConstraintVector inputExpression = {expression, constraint1, constraint2};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, IR::getConstant(eightBitType, 1));
        auto *constraint1 = new IR::Equ(fooVar, IR::getConstant(eightBitType, 2));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, false);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, IR::getConstant(eightBitType, 1));
        auto *constraint2 = new IR::Equ(barVar, IR::getConstant(eightBitType, 2));
        ConstraintVector inputExpression = {expression, constraint1, constraint2};
        testCheckSat(inputExpression, false);
    }
}

TEST_F(Z3SolverSatisfiabilityChecks, Strings) {
    const auto *stringType = IR::Type_String::get();
    const auto *fooVar = P4Tools::ToolsVariables::getSymbolicVariable(stringType, "foo");
    const auto *barVar = P4Tools::ToolsVariables::getSymbolicVariable(stringType, "bar");
    {
        auto *expression = new IR::Equ(new IR::StringLiteral(stringType, "dead"),
                                       new IR::StringLiteral(stringType, "dead"));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(new IR::StringLiteral(stringType, "dead"),
                                       new IR::StringLiteral(stringType, "beef"));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, false);
    }
    {
        auto *expression = new IR::Equ(fooVar, new IR::StringLiteral(stringType, "dead"));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, new IR::StringLiteral(stringType, "dead"));
        auto *constraint1 = new IR::Equ(fooVar, new IR::StringLiteral(stringType, "dead"));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, new IR::StringLiteral(stringType, "dead"));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, new IR::StringLiteral(stringType, "dead"));
        auto *constraint2 = new IR::Equ(barVar, new IR::StringLiteral(stringType, "dead"));
        ConstraintVector inputExpression = {expression, constraint1, constraint2};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, new IR::StringLiteral(stringType, "dead"));
        auto *constraint1 = new IR::Equ(fooVar, new IR::StringLiteral(stringType, "beef"));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, false);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, new IR::StringLiteral(stringType, "dead"));
        auto *constraint2 = new IR::Equ(barVar, new IR::StringLiteral(stringType, "beef"));
        ConstraintVector inputExpression = {expression, constraint1, constraint2};
        testCheckSat(inputExpression, false);
    }
}

TEST_F(Z3SolverSatisfiabilityChecks, Bools) {
    const auto *boolType = IR::Type_Boolean::get();
    const auto *fooVar = P4Tools::ToolsVariables::getSymbolicVariable(boolType, "foo");
    const auto *barVar = P4Tools::ToolsVariables::getSymbolicVariable(boolType, "bar");
    {
        auto *expression = new IR::Equ(IR::getBoolLiteral(true), IR::getBoolLiteral(true));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(IR::getBoolLiteral(true), IR::getBoolLiteral(false));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, false);
    }
    {
        auto *expression = new IR::Equ(fooVar, IR::getBoolLiteral(true));
        ConstraintVector inputExpression = {expression};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, IR::getBoolLiteral(true));
        auto *constraint1 = new IR::Equ(fooVar, IR::getBoolLiteral(true));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, IR::getBoolLiteral(true));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, IR::getBoolLiteral(true));
        auto *constraint2 = new IR::Equ(barVar, IR::getBoolLiteral(true));
        ConstraintVector inputExpression = {expression, constraint1, constraint2};
        testCheckSat(inputExpression, true);
    }
    {
        auto *expression = new IR::Equ(fooVar, IR::getBoolLiteral(true));
        auto *constraint1 = new IR::Equ(fooVar, IR::getBoolLiteral(false));
        ConstraintVector inputExpression = {expression, constraint1};
        testCheckSat(inputExpression, false);
    }
    {
        auto *expression = new IR::Equ(fooVar, barVar);
        auto *constraint1 = new IR::Equ(fooVar, IR::getBoolLiteral(true));
        auto *constraint2 = new IR::Equ(barVar, IR::getBoolLiteral(false));
        ConstraintVector inputExpression = {expression, constraint1, constraint2};
        testCheckSat(inputExpression, false);
    }
}

}  // namespace Test
