#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_CMD_STEPPER_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_CMD_STEPPER_H_

#include <map>
#include <optional>
#include <vector>

#include "backends/p4tools/common/core/solver.h"
#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/small_step/abstract_stepper.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools::P4Testgen {

/// Implements small-step operational semantics for commands.
class CmdStepper : public AbstractStepper {
 public:
    CmdStepper(ExecutionState &state, AbstractSolver &solver, const ProgramInfo &programInfo);

    bool preorder(const IR::AssignmentStatement *assign) override;
    bool preorder(const IR::P4Parser *p4parser) override;
    bool preorder(const IR::P4Control *p4control) override;
    bool preorder(const IR::EmptyStatement *empty) override;
    bool preorder(const IR::IfStatement *ifStatement) override;
    bool preorder(const IR::MethodCallStatement *methodCallStatement) override;
    bool preorder(const IR::P4Program *program) override;
    bool preorder(const IR::ParserState *parserState) override;
    bool preorder(const IR::BlockStatement *block) override;
    bool preorder(const IR::ExitStatement *e) override;
    bool preorder(const IR::SwitchStatement *switchStatement) override;

 protected:
    /// This call replaces the action labels of cases in a switch statement with the corresponding
    /// indices. We need this to match the executed action with the appropriate label.
    IR::SwitchStatement *replaceSwitchLabels(const IR::SwitchStatement *switchStatement);

    /// Initializes the given state for entry into the given parser.
    ///
    /// @returns constraints for associating packet data with symbolic state.
    const Constraint *startParser(const IR::P4Parser *parser, ExecutionState &state);

    /// @see startParser. Implementations can assume that the parser has been registered, and the
    /// cursor position has been initialized.
    virtual std::optional<const Constraint *> startParserImpl(const IR::P4Parser *parser,
                                                              ExecutionState &state) const = 0;

    /// Initializes variables and adds constraints for the program initialization, which is target
    /// specific.
    virtual void initializeTargetEnvironment(ExecutionState &state) const = 0;

    /// Provides exception-handler implementations for the given parser.
    ///
    /// @param normalContinuation is the continuation that would be executed if the parser finishes
    ///     normally.
    virtual std::map<Continuation::Exception, Continuation> getExceptionHandlers(
        const IR::P4Parser *parser, Continuation::Body normalContinuation,
        const ExecutionState &state) const = 0;

    /// Helper function that initializes a given type declaration using the given block parameter
    /// cstring list as prefixes.All the members of the Type_Declaration are initialized using the
    /// createTargetUninitialized of the respective target.
    void initializeBlockParams(const IR::Type_Declaration *typeDecl,
                               const std::vector<cstring> *blockParams, ExecutionState &state,
                               bool forceTaint = false) const;

    /// Add a variable to the symbolic interpreter. This looks up the full control-plane name of a
    /// variable defined in @param decl and declares in the symbolic environment of @param
    /// nextState.
    void declareVariable(ExecutionState &state, const IR::Declaration_Variable *decl);
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_SMALL_STEP_CMD_STEPPER_H_ */
