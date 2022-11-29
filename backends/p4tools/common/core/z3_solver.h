#ifndef BACKENDS_P4TOOLS_COMMON_CORE_Z3_SOLVER_H_
#define BACKENDS_P4TOOLS_COMMON_CORE_Z3_SOLVER_H_

#include <stddef.h>
#include <z3++.h>

#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/lib/formulae.h"
#include "backends/p4tools/common/lib/model.h"
#include "ir/ir.h"
#include "ir/json_generator.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"
#include "lib/safe_vector.h"

namespace P4Tools {

/// A stack of maps, which map Z3-internal expression IDs of declared Z3 variables to their
/// corresponding P4 state variable. The maps are pushed and pop according to the solver push() and
/// pop() operations.
using Z3DeclareVariablesMap = std::vector<ordered_map<unsigned, const IR::StateVariable>>;

/// A Z3-based implementation of AbstractSolver. Encapsulates a z3::solver and a z3::context.
class Z3Solver : public AbstractSolver {
    friend class Z3Translator;
    friend class Z3JSON;
    friend class Z3SolverAccessor;

 public:
    virtual ~Z3Solver() = default;

    explicit Z3Solver(bool isIncremental = true,
                      std::optional<std::istream *> inOpt = std::nullopt);

    void comment(cstring comment) override;

    void seed(unsigned seed) override;

    void timeout(unsigned tm) override;

    std::optional<bool> checkSat(const std::vector<const Constraint *> &asserts) override;

    const Model *getModel() const override;

    void toJSON(JSONGenerator & /*json*/) const override;

    bool isInIncrementalMode() const override;

    /// Get the actual Z3 solver backing this class.
    const z3::solver &getZ3Solver() const;

    /// Get the actual Z3 context that this class uses.
    const z3::context &getZ3Ctx() const;

    /// @returns the list of active assertions on this solver.
    safe_vector<const Constraint *> getAssertions() const;

 private:
    /// Resets the internal state: pops all assertions from previous solver
    /// invocation, removes variable declarations.
    void reset();

    /// Pushes new (empty) solver context.
    void push();

    /// Removes the last solver context.
    void pop();

    /// Inserts an assertion into the topmost solver context.
    void asrt(const Constraint *assertion);

    /// Converts a P4 type to a Z3 sort.
    z3::sort toSort(const IR::Type *type);

    /// Declares the given state variable to Z3.
    ///
    /// @returns the resulting Z3 variable.
    z3::expr declareVar(const IR::StateVariable& var);

    /// Generates a Z3 name for the given variable.
    std::string generateName(const IR::StateVariable& var) const;

    /// Generates a Z3 name for the given variable. The generated name is written to the given
    /// output stream.
    void generateName(std::ostringstream& ostr, const IR::StateVariable& var) const;

    /// Converts a Z3 expression to a value with the given type.
    static const Value *toValue(const z3::expr &e, const IR::Type *type);

    /// Adds pushes for incremental solver.
    /// Increments @a chkIndex and calls push for each occurrence
    ///            of @a asrtIndex in @a checkpoints.
    /// @chkIndex is assumed to point to the first entry in checkpoints
    //             that is greater than or equal to @asrtIndex.
    /// Helps to restore a state of incremental solver in a constructor.
    void addZ3Pushes(size_t &chkIndex, size_t asrtIndex);

    /// Main Z3 context.
    z3::context z3context;

    /// The underlying Z3 instance.
    z3::solver z3solver;

    /// For each state variable declared in the solver, this maps the variable's Z3 expression ID
    /// to the original state variable.
    Z3DeclareVariablesMap declaredVarsById;

    /// The sequence of P4 assertions that have been made to the solver.
    safe_vector<const Constraint *> p4Assertions;

    /// Indicates whether the incremental Z3 solver is being used. When this is false, this class
    /// manages push and pop operations explicitly by restarting Z3 as needed.
    bool isIncremental;

    /// The stack of checkpoints that have been made. Each checkpoint is represented by the size of
    /// @ref p4Assertions at the time the checkpoint was made.
    std::vector<size_t> checkpoints;

    /// The Z3 counterpart to @ref p4Assertions. This is only used when @a isIncremental is false.
    z3::expr_vector z3Assertions;

    /// Stores the RNG seed, as last set by @ref seed.
    std::optional<unsigned> seed_;

    /// Stores the timeout, as last set by @ref timeout.
    std::optional<unsigned> timeout_;
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_CORE_Z3_SOLVER_H_ */
