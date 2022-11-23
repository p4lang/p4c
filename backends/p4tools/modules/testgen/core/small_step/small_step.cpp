#include "backends/p4tools/modules/testgen/core/small_step/small_step.h"

#include <iosfwd>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/none.hpp>
#include <boost/optional/optional.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

#include "backends/p4tools/common/lib/symbolic_env.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"
#include "ir/irutils.h"
#include "ir/node.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/null.h"
#include "p4tools/common/compiler/reachability.h"
#include "p4tools/common/core/solver.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/cmd_stepper.h"
#include "backends/p4tools/modules/testgen/core/small_step/expr_stepper.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools {

namespace P4Testgen {

SmallStepEvaluator::SmallStepEvaluator(AbstractSolver& solver, const ProgramInfo& programInfo)
    : programInfo(programInfo), solver(solver) {
    if (!TestgenOptions::get().pattern.empty()) {
        reachabilityEngine =
            new ReachabilityEngine(programInfo.dcg, TestgenOptions::get().pattern, true);
    }
}

SmallStepEvaluator::Result SmallStepEvaluator::step(ExecutionState& state) {
    BUG_CHECK(!state.isTerminal(), "Tried to step from a terminal state.");

    if (const auto cmdOpt = state.getNextCmd()) {
        struct CommandVisitor : public boost::static_visitor<Result> {
         private:
            SmallStepEvaluator& self;
            ExecutionState& state;

         public:
            using REngineType = std::pair<ReachabilityResult, std::vector<Branch>*>;
            REngineType renginePreprocessing(const IR::Node* node) {
                ReachabilityResult rresult = std::make_pair(true, nullptr);
                std::vector<Branch>* branches = nullptr;
                // Current node should be inside DCG.
                if (self.reachabilityEngine->getDCG()->isCaller(node)) {
                    // Move reachability engine to next state.
                    rresult = self.reachabilityEngine->next(state.reachabilityEngineState, node);
                    if (!rresult.first) {
                        // Reachability property was failed.
                        const IR::Expression* cond = IR::getBoolLiteral(false);
                        branches = new std::vector<Branch>({Branch(cond, state, &state)});
                    }
                } else if (const auto* method = node->to<IR::MethodCallStatement>()) {
                    return renginePreprocessing(method->methodCall);
                }
                return std::make_pair(rresult, branches);
            }

            static void renginePostprocessing(ReachabilityResult& result,
                                              std::vector<Branch>* branches) {
                // All Reachability engine state for branch should be copied.
                if (branches->size() > 1 || result.second != nullptr) {
                    for (auto& n : *branches) {
                        if (result.second != nullptr) {
                            n.constraint =
                                new IR::BAnd(IR::Type_Boolean::get(), n.constraint, result.second);
                        }
                        if (branches->size() > 1) {
                            // Copy reachability engine state
                            n.nextState->reachabilityEngineState =
                                n.nextState->reachabilityEngineState->copy();
                        }
                    }
                }
            }

            Result operator()(const IR::Node* node) {
                // Step on the given node as a command.
                BUG_CHECK(node, "Attempted to evaluate null node.");
                REngineType r;
                if (self.reachabilityEngine != nullptr) {
                    r = renginePreprocessing(node);
                    if (r.second != nullptr) {
                        return r.second;
                    }
                }
                auto* stepper = TestgenTarget::getCmdStepper(state, self.solver, self.programInfo);
                auto* result = stepper->step(node);
                if (self.reachabilityEngine != nullptr) {
                    renginePostprocessing(r.first, result);
                }
                return result;
            }

            Result operator()(const TraceEvent* event) {
                CHECK_NULL(event);
                event = event->subst(state.getSymbolicEnv());

                state.add(event);
                state.popBody();
                return new std::vector<Branch>({Branch(&state)});
            }

            Result operator()(Continuation::Return ret) {
                if (ret.expr) {
                    // Step on the returned expression.
                    const auto* expr = *ret.expr;
                    BUG_CHECK(expr, "Attempted to evaluate null expr.");
                    // Do not bother with the stepper, if the expression is already symbolic.
                    if (SymbolicEnv::isSymbolicValue(expr)) {
                        state.popContinuation(expr);
                        return new std::vector<Branch>({Branch(&state)});
                    }
                    auto* stepper =
                        TestgenTarget::getExprStepper(state, self.solver, self.programInfo);
                    auto* result = stepper->step(expr);
                    if (self.reachabilityEngine != nullptr) {
                        ReachabilityResult rresult = std::make_pair(true, nullptr);
                        renginePostprocessing(rresult, result);
                    }
                    return result;
                }

                // Step on valueless return.
                state.popContinuation();
                return new std::vector<Branch>({Branch(&state)});
            }

            Result operator()(Continuation::Exception e) {
                state.handleException(e);
                return new std::vector<Branch>({Branch(&state)});
            }

            Result operator()(const Continuation::PropertyUpdate& e) {
                state.setProperty(e.propertyName, e.property);
                state.popBody();
                return new std::vector<Branch>({Branch(&state)});
            }

            Result operator()(const Continuation::Guard& guard) {
                // Check whether we exceed the number of maximum permitted guard violations.
                // This usually indicates that we have many branches that produce an invalid state.
                // The P4 program should be fixed in that case, because we can not generate useful
                // tests.
                if (self.violatedGuardConditions > SmallStepEvaluator::MAX_GUARD_VIOLATIONS) {
                    BUG("Condition %1% exceeded the maximum number of permitted guard "
                        "violations for this run."
                        " This implies that the P4 program produces an output that violates"
                        " test variants. For example, it may set an output port that is not "
                        "testable.",
                        guard.cond);
                }

                // Evaluate the guard condition by directly using the solver.
                const auto* cond = guard.cond;
                boost::optional<bool> solverResult = boost::none;

                // If the guard condition is tainted, treat it equivalent to an invalid state.
                if (!state.hasTaint(cond)) {
                    cond = state.getSymbolicEnv().subst(cond);
                    cond = IR::optimizeExpression(cond);
                    // Check whether the condition is satisfiable in the current execution state.
                    auto pathConstraints = state.getPathConstraint();
                    pathConstraints.push_back(cond);
                    solverResult = self.solver.checkSat(pathConstraints);
                }

                auto* nextState = new ExecutionState(state);
                nextState->popBody();
                // If we can not solve the guard (either we time out or the solver can not solve the
                // problem) we increment the count of violatedGuardConditions and stop executing
                // this branch.
                if (solverResult == boost::none || !solverResult.get()) {
                    std::stringstream condStream;
                    guard.cond->dbprint(condStream);
                    ::warning(
                        "Guard %1% was not satisfiable."
                        " Incrementing number of guard violations.",
                        condStream.str().c_str());
                    self.violatedGuardConditions++;
                    return new std::vector<Branch>({{IR::getBoolLiteral(false), state, nextState}});
                }
                // Otherwise, we proceed as usual.
                return new std::vector<Branch>({{cond, state, nextState}});
            }

            explicit CommandVisitor(SmallStepEvaluator& self, ExecutionState& state)
                : self(self), state(state) {}
        } cmdVisitor(*this, state);

        return boost::apply_visitor(cmdVisitor, *cmdOpt);
    }

    // State has an empty body. Pop the continuation stack.
    state.popContinuation();
    return new std::vector<Branch>({Branch(&state)});
}

}  // namespace P4Testgen

}  // namespace P4Tools
