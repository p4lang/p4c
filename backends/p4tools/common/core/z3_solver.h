#ifndef BACKENDS_P4TOOLS_COMMON_CORE_Z3_SOLVER_H_
#define BACKENDS_P4TOOLS_COMMON_CORE_Z3_SOLVER_H_

#include <z3++.h>

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

#include "ir/ir.h"
#include "ir/json_generator.h"
#include "ir/solver.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"
#include "lib/rtti.h"
#include "lib/safe_vector.h"

namespace P4Tools {

/// A stack of maps, which map Z3-internal expression IDs of declared Z3 variables to their
/// corresponding P4 state variable. The maps are pushed and pop according to the solver push() and
/// pop() operations.
using Z3DeclaredVariablesMap = std::vector<ordered_map<unsigned, const IR::SymbolicVariable *>>;

/// A Z3-based implementation of AbstractSolver. Encapsulates a z3::solver and a z3::context.
class Z3Solver : public AbstractSolver {
    friend class Z3Translator;
    friend class Z3JSON;
    friend class Z3SolverAccessor;

 public:
    ~Z3Solver() override = default;

    explicit Z3Solver(bool isIncremental = true,
                      std::optional<std::istream *> inOpt = std::nullopt);

    void comment(cstring comment) override;

    void seed(unsigned seed) override;

    void timeout(unsigned tm) override;

    std::optional<bool> checkSat(const std::vector<const Constraint *> &asserts) override;

    /// Z3Solver specific checkSat function. Calls check on the input z3::expr_vector.
    /// Only relies on the incrementality mode of the Z3 solver.
    std::optional<bool> checkSat(const z3::expr_vector &asserts);

    /// Z3Solver specific checkSat function. Calls check on the solver.
    /// Only useful in incremental mode.
    std::optional<bool> checkSat();

    [[nodiscard]] const SymbolicMapping &getSymbolicMapping() const override;

    void toJSON(JSONGenerator & /*json*/) const override;

    [[nodiscard]] bool isInIncrementalMode() const override;

    /// Get the actual Z3 solver backing this class.
    [[nodiscard]] const z3::solver &getZ3Solver() const;

    /// Get the actual Z3 context that this class uses.
    [[nodiscard]] const z3::context &getZ3Ctx() const;

    /// @returns the list of active assertions on this solver.
    [[nodiscard]] safe_vector<const Constraint *> getAssertions() const;

    /// Resets the internal state: pops all assertions from previous solver
    /// invocation, removes variable declarations.
    void reset();

    /// Pushes new (empty) solver context.
    void push();

    /// Removes the last solver context.
    void pop();

    /// Reset the internal Z3 solver state (memory and active assertions).
    /// In incremental state, all active assertions are reapplied after resetting.
    void clearMemory();

    /// Adds a Z3 assertion to the solver context.
    void asrt(const z3::expr &assert);

    /// Inserts an assertion into the topmost solver context.
    void asrt(const Constraint *assertion);

 private:
    /// Converts a P4 type to a Z3 sort.
    z3::sort toSort(const IR::Type *type);

    /// Get the actual Z3 context that this class uses. This context can be manipulated.
    [[nodiscard]] z3::context &ctx() const;

    /// Declares the given symbolic variable to Z3.
    ///
    /// @returns the resulting Z3 variable.
    z3::expr declareVar(const IR::SymbolicVariable &var);

    /// Generates a Z3 name for the given symbolic variable.
    [[nodiscard]] static std::string generateName(const IR::SymbolicVariable &var);

    /// Converts a Z3 expression to an IR::Literal with the given type.
    static const IR::Literal *toLiteral(const z3::expr &e, const IR::Type *type);

    /// Adds pushes for incremental solver.
    /// Increments @a chkIndex and calls push for each occurrence
    ///            of @a asrtIndex in @a checkpoints.
    /// @chkIndex is assumed to point to the first entry in checkpoints
    //             that is greater than or equal to @asrtIndex.
    /// Helps to restore a state of incremental solver in a constructor.
    void addZ3Pushes(size_t &chkIndex, size_t asrtIndex);

    /// Helper function which converts a z3::check_result to a std::optional<bool>.
    static std::optional<bool> interpretSolverResult(z3::check_result result);

    /// The underlying Z3 instance.
    z3::solver z3solver;

    /// For each state variable declared in the solver, this maps the variable's Z3 expression ID
    /// to the original state variable.
    Z3DeclaredVariablesMap declaredVarsById;

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

    DECLARE_TYPEINFO(Z3Solver, AbstractSolver);
};

/// Translates P4 expressions into Z3. Any variables encountered are declared to a Z3 instance.
class Z3Translator : public virtual Inspector {
 public:
    /// Creates a Z3 translator. Any variables encountered during translation will be declared to
    /// the Z3 instance encapsulated within the given solver.
    explicit Z3Translator(Z3Solver &solver);

    /// Handles unexpected nodes.
    bool preorder(const IR::Node *node) override;

    /// Translates casts.
    bool preorder(const IR::Cast *cast) override;

    /// Translates constants.
    bool preorder(const IR::Constant *constant) override;
    bool preorder(const IR::BoolLiteral *boolLiteral) override;
    bool preorder(const IR::StringLiteral *stringLiteral) override;

    /// Translates variables.
    bool preorder(const IR::SymbolicVariable *var) override;

    // Translations for unary operations.
    bool preorder(const IR::Neg *op) override;
    bool preorder(const IR::Cmpl *op) override;
    bool preorder(const IR::LNot *op) override;

    // Translations for binary operations.
    bool preorder(const IR::Equ *op) override;
    bool preorder(const IR::Neq *op) override;
    bool preorder(const IR::Lss *op) override;
    bool preorder(const IR::Leq *op) override;
    bool preorder(const IR::Grt *op) override;
    bool preorder(const IR::Geq *op) override;
    bool preorder(const IR::Mod *op) override;
    bool preorder(const IR::Add *op) override;
    bool preorder(const IR::Sub *op) override;
    bool preorder(const IR::Mul *op) override;
    bool preorder(const IR::Div *op) override;
    bool preorder(const IR::Shl *op) override;
    bool preorder(const IR::Shr *op) override;
    bool preorder(const IR::BAnd *op) override;
    bool preorder(const IR::BOr *op) override;
    bool preorder(const IR::BXor *op) override;
    bool preorder(const IR::LAnd *op) override;
    bool preorder(const IR::LOr *op) override;
    bool preorder(const IR::Concat *op) override;

    // Translations for ternary operations.
    bool preorder(const IR::Mux *op) override;
    bool preorder(const IR::Slice *op) override;

    /// @returns the result of the translation.
    z3::expr getResult();

    /// Translate an P4C IR expression and return the Z3 equivalent.
    z3::expr translate(const IR::Expression *expression);

 private:
    /// Function type for a unary operator.
    using Z3UnaryOp = z3::expr (*)(const z3::expr &);

    /// Function type for a binary operator.
    using Z3BinaryOp = z3::expr (*)(const z3::expr &, const z3::expr &);

    /// Function type for a ternary operator.
    using Z3TernaryOp = z3::expr (*)(const z3::expr &, const z3::expr &, const z3::expr &);

    /// Handles recursion into unary operations.
    ///
    /// @returns false.
    bool recurseUnary(const IR::Operation_Unary *unary, Z3UnaryOp f);

    /// Handles recursion into binary operations.
    ///
    /// @returns false.
    bool recurseBinary(const IR::Operation_Binary *binary, Z3BinaryOp f);

    /// Handles recursion into ternary operations.
    ///
    /// @returns false.
    bool recurseTernary(const IR::Operation_Ternary *ternary, Z3TernaryOp f);

    /// Rewrites a shift operation so that the type of the shift amount matches that of the number
    /// being shifted.
    ///
    /// P4 allows shift operands to have different types: when the number being shifted is a bit
    /// vector, the shift amount can be an infinite-precision integer. This rewrites such
    /// expressions so that the shift amount is a bit vector.
    template <class ShiftType>
    const ShiftType *rewriteShift(const ShiftType *shift) const;

    /// The output of the translation.
    z3::expr result;

    /// The Z3 solver instance, to which variables will be declared as they are encountered.
    std::reference_wrapper<Z3Solver> solver;
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_CORE_Z3_SOLVER_H_ */
