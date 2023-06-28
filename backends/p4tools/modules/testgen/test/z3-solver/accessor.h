#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TEST_Z3_SOLVER_ACCESSOR_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TEST_Z3_SOLVER_ACCESSOR_H_

#include <vector>

#include "backends/p4tools/common/core/z3_solver.h"

namespace P4Tools {

// The main class for access to a private members
class Z3SolverAccessor {
 public:
    /// Default constructor.
    explicit Z3SolverAccessor(Z3Solver &solver) : solver(solver) {}

    /// Gets all assertions. Used by GTests only.
    z3::expr_vector getAssertions(std::optional<bool> assertionType = std::nullopt) {
        if (!assertionType) {
            return solver.isIncremental ? solver.z3solver.assertions() : solver.z3Assertions;
        }
        if (assertionType.value()) {
            return solver.z3solver.assertions();
        }
        return solver.z3Assertions;
    }

    /// Gets all P4 assertions. Used by GTests only.
    safe_vector<const Constraint *> getP4Assertions() { return solver.p4Assertions; }

    /// Get Z3 context. Used by GTests only.
    const z3::context &getContext() { return solver.getZ3Ctx(); }

    /// Gets checkpoints that have been made. Used by GTests only.
    std::vector<size_t> &getCheckpoints() { return solver.checkpoints; }

 private:
    /// Pointer to a solver.
    Z3Solver &solver;
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TEST_Z3_SOLVER_ACCESSOR_H_ */
