#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_PROGRAM_INFO_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_PROGRAM_INFO_H_

#include <cstddef>
#include <map>
#include <vector>

#include "control-plane/p4RuntimeSerializer.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/bmv2.h"

namespace P4Tools::P4Testgen::Bmv2 {

class Bmv2V1ModelProgramInfo : public ProgramInfo {
 private:
    /// The program's top level blocks: the parser, the checksum verifier, the MAU pipeline, the
    /// checksum calculator, and the deparser.
    const ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;

    /// The declid of each top-level parser type, mapped to its corresponding gress.
    const std::map<int, int> declIdToGress;

    /// The bit width of standard_metadata.parser_error.
    static const IR::Type_Bits PARSER_ERR_BITS;

    /// This function contains an imperative specification of the inter-pipe interaction in the
    /// target.
    std::vector<Continuation::Command> processDeclaration(const IR::Type_Declaration *typeDecl,
                                                          size_t blockIdx) const;

 public:
    Bmv2V1ModelProgramInfo(const BMv2V1ModelCompilerResult &compilerResult,
                           ordered_map<cstring, const IR::Type_Declaration *> inputBlocks,
                           std::map<int, int> declIdToGress);

    /// @returns the gress associated with the given parser.
    int getGress(const IR::Type_Declaration *) const;

    /// @returns the P4Runtime API produced by the compiler.
    [[nodiscard]] P4::P4RuntimeAPI getP4RuntimeAPI() const;

    /// @returns the table associated with the direct extern
    const IR::P4Table *getTableofDirectExtern(const IR::IDeclaration *directExternDecl) const;

    /// @see ProgramInfo::getArchSpec
    [[nodiscard]] const ArchSpec &getArchSpec() const override;

    /// @returns the programmable blocks of the program. Should be 6.
    [[nodiscard]] const ordered_map<cstring, const IR::Type_Declaration *> *getProgrammableBlocks()
        const;

    /// @returns the name of the parameter for a given programmable-block label and the parameter
    /// index. This is the name of the parameter that is used in the P4 program.
    [[nodiscard]] const IR::PathExpression *getBlockParam(cstring blockLabel,
                                                          size_t paramIndex) const;

    [[nodiscard]] const IR::StateVariable &getTargetInputPortVar() const override;

    /// @returns the constraint expression for a given port variable.
    static const IR::Expression *getPortConstraint(
        const IR::StateVariable &portVar,
        const std::vector<std::pair<int, int>> &permittedPortRanges);

    [[nodiscard]] const IR::StateVariable &getTargetOutputPortVar() const override;

    [[nodiscard]] const IR::Expression *dropIsActive() const override;

    [[nodiscard]] const IR::Type_Bits *getParserErrorType() const override;

    [[nodiscard]] const BMv2V1ModelCompilerResult &getCompilerResult() const override;

    /// @returns the Member variable corresponding to the parameter index for the given parameter.
    /// The Member variable uses the parameter struct label as parent and the @param paramLabel as
    /// member. @param type is the type of the member. If the parser does not have this parameter
    /// (meaning we are dealing with optional parameters) return the canonical name of this
    /// variable.
    static const IR::Member *getParserParamVar(const IR::P4Parser *parser, const IR::Type *type,
                                               size_t paramIndex, cstring paramLabel);

    /// @see ProgramInfo::getArchSpec
    static const ArchSpec ARCH_SPEC;

    DECLARE_TYPEINFO(Bmv2V1ModelProgramInfo, ProgramInfo);
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_PROGRAM_INFO_H_ */
