#ifndef TESTGEN_CORE_TARGET_H_
#define TESTGEN_CORE_TARGET_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/optional/optional.hpp>

#include "backends/p4tools/common/core/solver.h"
#include "backends/p4tools/common/core/target.h"
#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/testgen/core/exploration_strategy/exploration_strategy.h"
#include "backends/p4tools/testgen/core/program_info.h"
#include "backends/p4tools/testgen/core/small_step/cmd_stepper.h"
#include "backends/p4tools/testgen/core/small_step/expr_stepper.h"
#include "backends/p4tools/testgen/lib/execution_state.h"
#include "backends/p4tools/testgen/lib/namespace_context.h"
#include "backends/p4tools/testgen/lib/test_backend.h"

namespace P4Tools {

namespace P4Testgen {

/// Specifies a canonical representation of the target pipeline as documented in P4 code.
class ArchSpec {
 public:
    /// An ArchMember represents a construct in the pipe. It has a name and parameters.
    struct ArchMember {
        cstring blockName;
        std::vector<cstring> blockParams;
    };

 private:
    /// The ArchSpec has a name that typically corresponds with the name of the package for main.
    cstring packageName;

    /// The ordered list of members in the architecture specification.
    std::vector<ArchMember> archVector;

    /// Keeps track of the block indices in the architecture specification.
    /// This is useful to lookup the index for a particular block label.
    std::map<cstring, size_t> blockIndices;

 public:
    explicit ArchSpec(cstring packageName, const std::vector<ArchMember>& archVectorInput);

    /// @returns the index that corresponds to the given name in this architecture specification.
    /// A bug is thrown if the index does not exist.
    size_t getBlockIndex(cstring blockName) const;

    /// @returns the architecture member that corresponds to the given index in this architecture
    /// specification. A bug is thrown if the index does not exist.
    const ArchMember* getArchMember(size_t blockIndex) const;

    /// @returns name of the parameter for the given block and parameter index in this architecture
    /// specification. A bug is thrown if the indices are out of range.
    cstring getParamName(size_t blockIndex, size_t paramIndex) const;

    /// @returns name of the parameter for the given block label and parameter index in this
    /// architecture specification. A bug is thrown if the index is out of range or the block label
    /// does not exist.
    cstring getParamName(cstring blockName, size_t paramIndex) const;

    /// @returns the size of the architecture specification vector.
    size_t getArchVectorSize() const;

    /// @returns the label of the architecture specification.
    cstring getPackageName() const;
};

class TestgenTarget : public Target {
 public:
    /// @returns the singleton instance for the current target.
    static const TestgenTarget& get();

    /// Produces a @ProgramInfo for the given P4 program.
    ///
    /// @returns nullptr if the program is not supported by this target.
    static const ProgramInfo* initProgram(const IR::P4Program* program) {
        return get().initProgram_impl(program);
    }

    /// Returns the test back end associated with this P4Testgen target.
    static TestBackEnd* getTestBackend(const ProgramInfo& programInfo, ExplorationStrategy& symbex,
                                       const boost::filesystem::path& testPath,
                                       boost::optional<uint32_t> seed) {
        return get().getTestBackend_impl(programInfo, symbex, testPath, seed);
    }

    /// The width of a port number, in bits.
    static int getPortNumWidth_bits() { return get().getPortNumWidth_bits_impl(); }

    /// Provides a CmdStepper implementation for this target.
    static CmdStepper* getCmdStepper(ExecutionState& state, AbstractSolver& solver,
                                     const ProgramInfo& programInfo) {
        return get().getCmdStepper_impl(state, solver, programInfo);
    }

    /// Provides a ExprStepper implementation for this target.
    static ExprStepper* getExprStepper(ExecutionState& state, AbstractSolver& solver,
                                       const ProgramInfo& programInfo) {
        return get().getExprStepper_impl(state, solver, programInfo);
    }

    /// A vector that maps the architecture parameters of each pipe to the corresponding
    /// global architecture variables. For example, this map specifies which parameter of each pipe
    /// refers to the input header.
    // The arch map needs to be public to be subclassed.
    /// @returns a reference to the architecture map defined in this target
    static const ArchSpec* getArchSpec() { return get().getArchSpecImpl(); }

 protected:
    /// @see @initProgram.
    const ProgramInfo* initProgram_impl(const IR::P4Program* program) const;

    /// @see @initProgram.
    virtual const ProgramInfo* initProgram_impl(const IR::P4Program* program,
                                                const IR::Declaration_Instance* mainDecl) const = 0;

    /// @see getPortNumWidth_bits.
    virtual int getPortNumWidth_bits_impl() const = 0;

    /// @see getTestBackend.
    virtual TestBackEnd* getTestBackend_impl(const ProgramInfo& programInfo,
                                             ExplorationStrategy& symbex,
                                             const boost::filesystem::path& testPath,
                                             boost::optional<uint32_t> seed) const = 0;

    /// @see getCmdStepper.
    virtual CmdStepper* getCmdStepper_impl(ExecutionState& state, AbstractSolver& solver,
                                           const ProgramInfo& programInfo) const = 0;

    /// @see getExprStepper.
    virtual ExprStepper* getExprStepper_impl(ExecutionState& state, AbstractSolver& solver,
                                             const ProgramInfo& programInfo) const = 0;

    /// @see getArchSpec
    virtual const ArchSpec* getArchSpecImpl() const = 0;

    /// Utility function. Converts the list of arguments @inputArgs to a list of type declarations
    ///  and appends the result to @v. Any names appearing in the arguments are
    /// resolved with @ns.
    //
    static void argumentsToTypeDeclarations(const NamespaceContext* ns,
                                            const IR::Vector<IR::Argument>* inputArgs,
                                            std::vector<const IR::Type_Declaration*>& resultDecls);

    explicit TestgenTarget(std::string deviceName, std::string archName);

 private:
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_CORE_TARGET_H_ */
