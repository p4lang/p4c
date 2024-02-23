#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_PROGRAM_INFO_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_PROGRAM_INFO_H_

#include <cstddef>
#include <optional>
#include <vector>

#include "backends/p4tools/common/compiler/reachability.h"
#include "backends/p4tools/common/lib/arch_spec.h"
#include "ir/ir.h"
#include "lib/castable.h"
#include "lib/rtti.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/core/compiler_target.h"
#include "backends/p4tools/modules/testgen/lib/concolic.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"

namespace P4Tools::P4Testgen {

/// Stores target-specific information about a P4 program.
class ProgramInfo : public ICastable {
 private:
    /// The program info object stores the results of the compilation, which includes the P4 program
    /// and any information extracted from the program using static analysis.
    std::reference_wrapper<const TestgenCompilerResult> compilerResult;

 protected:
    explicit ProgramInfo(const TestgenCompilerResult &compilerResult);

    /// The list of concolic methods implemented by the target. This list is assembled during
    /// initialization.
    ConcolicMethodImpls concolicMethodImpls;

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

    /// A vector that maps the architecture parameters of each pipe to the corresponding
    /// global architecture variables. For example, this map specifies which parameter of each pipe
    /// refers to the input header.
    // The arch map needs to be public to be subclassed.
    /// @returns a reference to the architecture map defined in this target
    [[nodiscard]] virtual const ArchSpec &getArchSpec() const = 0;

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

    /// @returns the canonical name of the program block that is passed in.
    /// Throws a BUG, if the name can not be found.
    [[nodiscard]] cstring getCanonicalBlockName(cstring programBlockName) const;

    /// @returns a reference to the compiler result that this program info object was initialized
    /// with.
    [[nodiscard]] virtual const TestgenCompilerResult &getCompilerResult() const;

    /// @returns the P4 program associated with this ProgramInfo.
    [[nodiscard]] const IR::P4Program &getP4Program() const;

    /// @returns the call graph associated with this ProgramInfo.
    [[nodiscard]] const NodesCallGraph &getCallGraph() const;

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

    DECLARE_TYPEINFO(ProgramInfo);
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_PROGRAM_INFO_H_ */
