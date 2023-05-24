#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_PROGRAM_INFO_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_PROGRAM_INFO_H_

#include <cstddef>
#include <optional>
#include <vector>

#include "backends/p4tools/common/compiler/reachability.h"
#include "backends/p4tools/common/lib/arch_spec.h"
#include "ir/declaration.h"
#include "ir/ir.h"
#include "lib/castable.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/lib/concolic.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"

namespace P4Tools::P4Testgen {

/// Stores target-specific information about a P4 program.
class ProgramInfo : public ICastable {
 protected:
    explicit ProgramInfo(const IR::P4Program *program);

    /// The list of concolic methods implemented by the target. This list is assembled during
    /// initialization.
    ConcolicMethodImpls concolicMethodImpls;

    /// Set of all visitable nodes in the input P4 program.
    P4::Coverage::CoverageSet coverableNodes;

    /// The execution sequence of the P4 program.
    std::vector<Continuation::Command> pipelineSequence;

    /// The constraints imposed by the target.
    std::optional<const IR::Expression *> targetConstraints = std::nullopt;

    /// Maps the programmable blocks in the P4 program to their canonical counterpart.
    ordered_map<cstring, cstring> blockMap;

 public:
    ProgramInfo(const ProgramInfo &) = default;

    ProgramInfo(ProgramInfo &&) = default;

    ProgramInfo &operator=(const ProgramInfo &) = default;

    ProgramInfo &operator=(ProgramInfo &&) = default;

    ~ProgramInfo() override = default;

    /// The P4 program from which this object is derived.
    const IR::P4Program *program;

    /// The generated dcg.
    const NodesCallGraph *dcg;

    /// @returns the series of nodes that has been computed by this particular target.
    [[nodiscard]] const std::vector<Continuation::Command> *getPipelineSequence() const;

    /// @returns the constraints of this target.
    /// These constraints can influence the execution of the interpreter
    [[nodiscard]] std::optional<const IR::Expression *> getTargetConstraints() const;

    /// @returns the metadata member corresponding to the ingress port
    [[nodiscard]] virtual const IR::StateVariable &getTargetInputPortVar() const = 0;

    /// @returns the metadata member corresponding to the final output port
    [[nodiscard]] virtual const IR::StateVariable &getTargetOutputPortVar() const = 0;

    /// @returns an expression that checks whether the packet is to be dropped.
    /// The computation is target specific.
    [[nodiscard]] virtual const IR::Expression *dropIsActive() const = 0;

    /// @returns the default value for uninitialized variables for this particular target. This can
    /// be a taint variable or simply 0 (bits) or false (booleans).
    /// If @param forceTaint is active, this function always returns a taint variable.
    virtual const IR::Expression *createTargetUninitialized(const IR::Type *type,
                                                            bool forceTaint) const;

    /// Getter to access coverableNodes.
    [[nodiscard]] const P4::Coverage::CoverageSet &getCoverableNodes() const;

    /// @returns the list of implemented concolic methods for this particular program.
    [[nodiscard]] const ConcolicMethodImpls *getConcolicMethodImpls() const;

    // @returns the width of the parser error for this specific target.
    [[nodiscard]] virtual const IR::Type_Bits *getParserErrorType() const = 0;

    /// Looks up a declaration from a path. A BUG occurs if no declaration is found.
    static const IR::IDeclaration *findProgramDecl(const IR::IGeneralNamespace *ns,
                                                   const IR::Path *path);

    /// Looks up a declaration from a path expression. A BUG occurs if no declaration is found.
    static const IR::IDeclaration *findProgramDecl(const IR::IGeneralNamespace *ns,
                                                   const IR::PathExpression *pathExpr);

    /// Resolves a Type_Name in the top-level namespace.
    static const IR::Type_Declaration *resolveProgramType(const IR::IGeneralNamespace *ns,
                                                          const IR::Type_Name *type);

    /// @returns the canonical name of the program block that is passed in.
    /// Throws a BUG, if the name can not be found.
    [[nodiscard]] cstring getCanonicalBlockName(cstring programBlockName) const;

    /// Helper function to produce copy-in and copy-out helper calls.
    /// Copy-in and copy-out is needed to correctly model the value changes of data when it is
    /// copied in and out of a programmable block. In many cases, data is reset here or not even
    /// copied.
    /// TODO: Find a more efficient way to implement copy-in/copy-out. These functions are very
    /// expensive.
    void produceCopyInOutCall(const IR::Parameter *param, size_t paramIdx,
                              const ArchSpec::ArchMember *archMember,
                              std::vector<Continuation::Command> *copyIns,
                              std::vector<Continuation::Command> *copyOuts) const;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_PROGRAM_INFO_H_ */
