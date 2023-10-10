#ifndef IR_SOLVER_H_
#define IR_SOLVER_H_

#include <optional>
#include <vector>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

#include "ir/ir.h"
#include "lib/castable.h"
#include "lib/cstring.h"

/// Represents a constraint that can be shipped to and asserted within a solver.
// TODO: This should implement AbstractRepCheckedNode<Constraint>.
using Constraint = IR::Expression;

/// Comparator to compare SymbolicVariable pointers.
struct SymbolicVarComp {
    bool operator()(const IR::SymbolicVariable *s1, const IR::SymbolicVariable *s2) const {
        return s1->operator<(*s2);
    }
};

/// This type maps symbolic variables to their value assigned by the solver.
using SymbolicMapping = boost::container::flat_map<const IR::SymbolicVariable *,
                                                   const IR::Expression *, SymbolicVarComp>;

using SymbolicSet = boost::container::flat_set<const IR::SymbolicVariable *, SymbolicVarComp>;

/// Provides a higher-level interface for an SMT solver.
class AbstractSolver : public ICastable {
 public:
    /// Adds a comment to any log file that might be produced. Useful for understanding the
    /// sequence of calls made to the solver when debugging.
    virtual void comment(cstring comment) = 0;

    /// Seeds the pseudo-random number generator in the solver. The solver's PRNG is used to
    /// randomize the models produced by @getModel.
    virtual void seed(unsigned seed) = 0;

    /// Sets timeout for solver in milliseconds.
    virtual void timeout(unsigned tm) = 0;

    /// Determines whether the set of assertions given to the solver are consistent.
    ///
    /// @return true if the given assertions are consistent.
    /// @return false if the given assertions are inconsistent.
    /// @return std::nullopt if the solver times out, or is otherwise unable to provide an answer.
    virtual std::optional<bool> checkSat(const std::vector<const Constraint *> &asserts) = 0;

    /// Obtains the first solution found by the solver in the last call to @checkSat.
    ///
    /// A BUG occurs if the solver has no available solution. This can happen if the last call to
    /// @checkSat returned anything other than true, if there was no such previous call, or if the
    /// state in the solver has changed since the last such call (e.g., more assertions have been
    /// made).
    [[nodiscard]] virtual const SymbolicMapping &getSymbolicMapping() const = 0;

    /// Saves solver state to the given JSON generator.
    virtual void toJSON(JSONGenerator &) const = 0;

    /// @returns whether this solver is incremental.
    [[nodiscard]] virtual bool isInIncrementalMode() const = 0;

    /// Cast an abstract solver to its specific sub type.
    /// Casts involving "const" have to explicit in this case. This is necessary because abstract
    /// solvers have no straightforward mechanism to support clone operations.
    template <typename T>
    T *to() {
        return dynamic_cast<T *>(this);
    }
};

#endif /* IR_SOLVER_H_ */
