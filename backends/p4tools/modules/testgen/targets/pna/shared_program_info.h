#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_SHARED_PROGRAM_INFO_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_SHARED_PROGRAM_INFO_H_

#include <cstddef>

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"

namespace P4Tools::P4Testgen::Pna {

class SharedPnaProgramInfo : public ProgramInfo {
 private:
    /// The program's top level blocks as defined in the package.
    ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;

    /// The bit width of the parser_error.
    static const IR::Type_Bits PARSER_ERR_BITS;

 public:
    SharedPnaProgramInfo(const TestgenCompilerResult &compilerResult,
                         ordered_map<cstring, const IR::Type_Declaration *> inputBlocks);

    /// @returns the programmable blocks of the program.
    [[nodiscard]] const ordered_map<cstring, const IR::Type_Declaration *> *getProgrammableBlocks()
        const;

    [[nodiscard]] const IR::StateVariable &getTargetInputPortVar() const override;

    [[nodiscard]] const IR::StateVariable &getTargetOutputPortVar() const override;

    [[nodiscard]] const IR::Expression *dropIsActive() const override;

    [[nodiscard]] const IR::Type_Bits *getParserErrorType() const override;

    /// @returns the name of the parameter for a given programmable-block label and the parameter
    /// index. This is the name of the parameter that is used in the P4 program.
    [[nodiscard]] const IR::PathExpression *getBlockParam(cstring blockLabel,
                                                          size_t paramIndex) const;

    DECLARE_TYPEINFO(SharedPnaProgramInfo, ProgramInfo);
};

}  // namespace P4Tools::P4Testgen::Pna

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_SHARED_PROGRAM_INFO_H_ */
