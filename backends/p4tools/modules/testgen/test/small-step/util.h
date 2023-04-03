#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TEST_SMALL_STEP_UTIL_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TEST_SMALL_STEP_UTIL_H_

#include <gtest/gtest.h>

#include <functional>
#include <optional>
#include <stack>
#include <string>
#include <variant>
#include <vector>

#include "backends/p4tools/common/core/z3_solver.h"
#include "backends/p4tools/common/lib/symbolic_env.h"
#include "gsl/gsl-lite.hpp"
#include "ir/declaration.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/enumerator.h"

#include "backends/p4tools/modules/testgen/core/small_step/small_step.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/namespace_context.h"
#include "backends/p4tools/modules/testgen/test/gtest_utils.h"

namespace Test {

using Body = P4Tools::P4Testgen::Continuation::Body;
using Continuation = P4Tools::P4Testgen::Continuation;
using ExecutionState = P4Tools::P4Testgen::ExecutionState;
using NamespaceContext = P4Tools::P4Testgen::NamespaceContext;
using Return = P4Tools::P4Testgen::Continuation::Return;
using SmallStepEvaluator = P4Tools::P4Testgen::SmallStepEvaluator;
using TestgenTarget = P4Tools::P4Testgen::TestgenTarget;
using Z3Solver = P4Tools::Z3Solver;

class SmallStepTest : public P4ToolsTest {
 public:
    /// Creates an execution state out of a continuation body.
    static ExecutionState mkState(Body body) { return ExecutionState(body); }
};

namespace SmallStepUtil {

/// Creates a test case with the given header fields for
/// stepping on a given expression.
std::optional<const P4ToolsTestCase> createSmallStepExprTest(const std::string &,
                                                             const std::string &);

/// Extract the expression from the P4Program.
template <class T>
const T *extractExpr(const IR::P4Program *program) {
    // Get the mau declarations in the P4Program.
    const auto *decls = program->getDeclsByName("mau")->toVector();
    if (decls->size() != 1) return nullptr;

    // Convert the mau declaration to a control and ensure that
    // there is a single statement in the body.
    const auto *control = (*decls)[0]->to<IR::P4Control>();
    if (control->body->components.size() != 1) return nullptr;

    // Ensure that the control body statement is a method call statement.
    auto *mcStmt = control->body->components[0]->to<IR::MethodCallStatement>();
    if (!mcStmt) return nullptr;

    // Ensure that there is only one argument to the method call and return it.
    auto *mcArgs = mcStmt->methodCall->arguments;
    if (mcArgs->size() != 1) return nullptr;
    return (*mcArgs)[0]->expression->to<T>();
}

/// Step on the @value, and examine the resulting state in the @program.
template <class T>
void stepAndExamineValue(const T *value, const IR::P4Program *program) {
    // Produce a ProgramInfo, which is needed to create a SmallStepEvaluator.
    const auto *progInfo = TestgenTarget::initProgram(program);
    ASSERT_TRUE(progInfo);

    // Create a base state with a parameter continuation to apply the value on.
    const auto *v = Continuation::genParameter(value->type, "v", NamespaceContext::Empty);
    Body bodyBase({Return(v->param)});
    Continuation continuationBase(v, bodyBase);
    ExecutionState esBase = SmallStepTest::mkState(bodyBase);

    // Step on the value.
    Z3Solver solver;
    auto *testState = new ExecutionState(esBase);
    Body body({Return(value)});
    testState->replaceBody(body);
    testState->pushContinuation(
        new ExecutionState::StackFrame(continuationBase, esBase.getNamespaceContext()));
    SmallStepEvaluator eval(solver, *progInfo);
    auto successors = eval.step(*testState);
    ASSERT_EQ(successors->size(), 1u);

    // Examine the resulting execution state.
    const auto branch = (*successors)[0];
    auto constraint = branch.constraint;
    auto executionState = branch.nextState;
    ASSERT_TRUE(constraint->checkedTo<IR::BoolLiteral>()->value);
    ASSERT_EQ(executionState->getNamespaceContext(), NamespaceContext::Empty);
    ASSERT_TRUE(executionState->getSymbolicEnv().getInternalMap().empty());

    // Examine the resulting body.
    Body finalBody = executionState->getBody();
    ASSERT_EQ(finalBody, Body({Return(value)}));

    // Examine the resulting stack.
    ASSERT_EQ(executionState->getStack().empty(), true);
}

/// Step on the @op, and examine the resulting state for the @subexpr
/// in the @program.
/// @param rebuildNode
///     Rebuilds the pushed continuation body with the given parameter.
template <class T>
void stepAndExamineOp(
    const T *op, const IR::Expression *subexpr, const IR::P4Program *program,
    std::function<const IR::Expression *(const IR::PathExpression *)> rebuildNode) {
    // Produce a ProgramInfo, which is needed to create a SmallStepEvaluator.
    const auto *progInfo = TestgenTarget::initProgram(program);
    ASSERT_TRUE(progInfo);

    // Step on the operation.
    Z3Solver solver;
    Body body({Return(op)});
    ExecutionState es = SmallStepTest::mkState(body);
    SmallStepEvaluator eval(solver, *progInfo);
    auto successors = eval.step(es);
    ASSERT_EQ(successors->size(), 1u);

    // Examine the resulting execution state.
    const auto branch = (*successors)[0];
    auto constraint = branch.constraint;
    auto executionState = branch.nextState;
    ASSERT_TRUE(constraint->checkedTo<IR::BoolLiteral>()->value);
    ASSERT_EQ(executionState->getNamespaceContext(), NamespaceContext::Empty);
    ASSERT_TRUE(executionState->getSymbolicEnv().getInternalMap().empty());

    // Examine the resulting body.
    Body finalBody = executionState->getBody();
    ASSERT_EQ(finalBody, Body({Return(subexpr)}));

    // Examine the resulting stack.
    ASSERT_EQ(executionState->getStack().size(), 1u);
    const auto stackFrame = executionState->getStack().top();
    ASSERT_TRUE(stackFrame->exceptionHandlers.empty());
    ASSERT_EQ(stackFrame->namespaces, NamespaceContext::Empty);

    // Examine the pushed continuation.
    Continuation pushedContinuation = stackFrame->normalContinuation;
    ASSERT_TRUE(pushedContinuation.parameterOpt);
    Body pushedBody = pushedContinuation.body;
    ASSERT_EQ(pushedBody, Body({Return(rebuildNode(*pushedContinuation.parameterOpt))}));
}

}  // namespace SmallStepUtil

}  // namespace Test

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TEST_SMALL_STEP_UTIL_H_ */
