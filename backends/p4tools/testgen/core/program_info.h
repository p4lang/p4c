#ifndef TESTGEN_CORE_PROGRAM_INFO_H_
#define TESTGEN_CORE_PROGRAM_INFO_H_

#include <cstddef>
#include <vector>

#include "ir/ir.h"
#include "lib/castable.h"
#include "lib/cstring.h"
#include "lib/null.h"

#include "backends/p4tools/testgen/core/arch_spec.h"
#include "backends/p4tools/common/lib/coverage.h"
#include "backends/p4tools/testgen/lib/concolic.h"
#include "backends/p4tools/testgen/lib/continuation.h"
#include "backends/p4tools/testgen/lib/namespace_context.h"

namespace P4Tools {

namespace P4Testgen {

/// Stores target-specific information about a P4 program.
class ProgramInfo : public Castable {
 private:
    const NamespaceContext* globalNameSpaceContext;

 protected:
    explicit ProgramInfo(const IR::P4Program* program);

    /// The list of concolic methods implemented by the target. This list is assembled during
    /// initialization.
    ConcolicMethodImpls concolicMethodImpls;

    /// Set of all statements in the input P4 program.
    Coverage::CoverageSet allStatements;

    std::vector<Continuation::Command> pipelineSequence;

    boost::optional<const Constraint*> targetConstraints = boost::none;

 public:
    ProgramInfo(const ProgramInfo&) = default;

    ProgramInfo(ProgramInfo&&) = default;

    ProgramInfo& operator=(const ProgramInfo&) = default;

    ProgramInfo& operator=(ProgramInfo&&) = default;

    virtual ~ProgramInfo() = default;

    /// The P4 program from which this object is derived.
    const IR::P4Program* program;

    /// @returns the series of nodes that has been computed by this particular target.
    const std::vector<Continuation::Command>* getPipelineSequence() const;

    /// @returns the constraints of this target.
    /// These constraints can influence the execution of the interpreter
    boost::optional<const Constraint*> getTargetConstraints() const;

    /// @returns the metadata member corresponding to the ingress port
    virtual const IR::Member* getTargetInputPortVar() const = 0;

    /// @returns the metadata member corresponding to the final output port
    virtual const IR::Member* getTargetOutputPortVar() const = 0;

    /// @returns an expression that checks whether the packet is to be dropped.
    /// The computation is target specific.
    virtual const IR::Expression* dropIsActive() const = 0;

    /// @returns the default value for uninitialized variables for this particular target. This can
    /// be a taint variable or simply 0 (bits) or false (booleans).
    /// If @param forceTaint is active, this function always returns a taint variable.
    virtual const IR::Expression* createTargetUninitialized(const IR::Type* type,
                                                            bool forceTaint) const = 0;

    /// Getter to access allStatements.
    const Coverage::CoverageSet& getAllStatements() const;

    /// @returns the list of implemented concolic methods for this particular program.
    const ConcolicMethodImpls* getConcolicMethodImpls() const;

    // @returns the width of the parser error for this specific target.
    virtual const IR::Type_Bits* getParserErrorType() const = 0;

    /// @returns the Member variable corresponding to the parameter index for the given parameter.
    /// The Member variable uses the parameter struct label as parent and the @param paramLabel as
    /// member. @param type is the type of the member. If the parser does not have this parameter
    /// (meaning we are dealing with optional parameters) return the canonical name of this
    /// variable.
    virtual const IR::Member* getParserParamVar(const IR::P4Parser* parser, const IR::Type* type,
                                                size_t paramIndex, cstring paramLabel) const = 0;

    /// Looks up a declaration from a path. A BUG occurs if no declaration is found.
    const IR::IDeclaration* findProgramDecl(const IR::Path* path) const;

    /// Looks up a declaration from a path expression. A BUG occurs if no declaration is found.
    const IR::IDeclaration* findProgramDecl(const IR::PathExpression* pathExpr) const;

    /// Resolves a Type_Name in the current environment.
    const IR::Type_Declaration* resolveProgramType(const IR::Type_Name* type) const;

    /// Helper function to produce copy-in and copy-out helper calls.
    /// Copy-in and copy-out is needed to correctly model the value changes of data when it is
    /// copied in and out of a programmable block. In many cases, data is reset here or not even
    /// copied.
    /// TODO: Find a more efficient way to implement copy-in/copy-out. These functions are very
    /// expensive.
    void produceCopyInOutCall(const IR::Parameter* param, size_t paramIdx,
                              const ArchSpec::ArchMember* archMember,
                              std::vector<Continuation::Command>* copyIns,
                              std::vector<Continuation::Command>* copyOuts) const;
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_CORE_PROGRAM_INFO_H_ */
