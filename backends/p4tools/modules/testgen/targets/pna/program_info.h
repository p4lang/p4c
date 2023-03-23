#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_PROGRAM_INFO_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_PROGRAM_INFO_H_

#include <cstddef>
#include <map>
#include <vector>

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"

namespace P4Tools::P4Testgen::Pna {

class PnaDpdkProgramInfo : public ProgramInfo {
 private:
    /// The program's top level blocks: the parser, the checksum verifier, the MAU pipeline, the
    /// checksum calculator, and the deparser.
    const ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;

    /// The bit width of standard_metadata.parser_error.
    static const IR::Type_Bits parserErrBits;

    /// This function contains an imperative specification of the inter-pipe interaction in the
    /// target.
    std::vector<Continuation::Command> processDeclaration(const IR::Type_Declaration *typeDecl,
                                                          size_t blockIdx) const;

 public:
    PnaDpdkProgramInfo(const IR::P4Program *program,
                       ordered_map<cstring, const IR::Type_Declaration *> inputBlocks);

    /// @returns the programmable blocks of the program. Should be 6.
    const ordered_map<cstring, const IR::Type_Declaration *> *getProgrammableBlocks() const;

    /// @returns the name of the parameter for a given programmable-block label and the parameter
    /// index. This is the name of the parameter that is used in the P4 program.
    const IR::PathExpression *getBlockParam(cstring blockLabel, size_t paramIndex) const;

    const IR::Member *getTargetInputPortVar() const override;

    const IR::Member *getTargetOutputPortVar() const override;

    const IR::Expression *dropIsActive() const override;

    const IR::Expression *createTargetUninitialized(const IR::Type *type,
                                                    bool forceTaint) const override;

    const IR::Type_Bits *getParserErrorType() const override;

    const IR::Member *getParserParamVar(const IR::P4Parser *parser, const IR::Type *type,
                                        size_t paramIndex, cstring paramLabel) const override;
};

}  // namespace P4Tools::P4Testgen::Pna

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_PROGRAM_INFO_H_ */
