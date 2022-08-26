#ifndef TESTGEN_TEST_Z3_SOLVER_ACCESSOR_H_
#define TESTGEN_TEST_Z3_SOLVER_ACCESSOR_H_

#include <vector>

#include "backends/p4tools/common/core/z3_solver.h"
#include "gsl/gsl-lite.hpp"

namespace P4Tools {

// The main class for access to a private members
class Z3SolverAccessor {
 public:
    /// Default constructor.
    explicit Z3SolverAccessor(gsl::not_null<Z3Solver*> solver) : solver(solver) {}

    /// Gets all assertions. Used by GTests only.
    z3::expr_vector getAssertions(boost::optional<bool> assertionType = boost::none) {
        if (!assertionType) {
            return solver->isIncremental ? solver->z3solver.assertions() : solver->z3Assertions;
        } else if (assertionType.value()) {
            return solver->z3solver.assertions();
        }
        return solver->z3Assertions;
    }

    /// Gets all P4 assertions. Used by GTests only.
    safe_vector<const Constraint*> getP4Assertions() { return solver->p4Assertions; }

    /// Get Z3 context. Used by GTests only.
    z3::context& getContext() { return solver->z3context; }

    /// Gets checkpoints that have been made. Used by GTests only.
    std::vector<size_t>& getCheckpoints() { return solver->checkpoints; }

 private:
    /// Pointer to a solver.
    gsl::not_null<Z3Solver*> solver;
};

}  // namespace P4Tools

#endif /* TESTGEN_TEST_Z3_SOLVER_ACCESSOR_H_ */
