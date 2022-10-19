#ifndef TESTGEN_TARGETS_EBPF_PROGRAM_INFO_H_
#define TESTGEN_TARGETS_EBPF_PROGRAM_INFO_H_

#include <stddef.h>

#include <map>
#include <vector>

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/testgen/core/program_info.h"
#include "backends/p4tools/testgen/lib/continuation.h"

namespace P4Tools {

namespace P4Testgen {

namespace EBPF {

class EBPFProgramInfo : public ProgramInfo {
 private:
    /// The program's top level blocks: the parser, the checksum verifier, the MAU pipeline, the
    /// checksum calculator, and the deparser.
    const ordered_map<cstring, const IR::Type_Declaration*> programmableBlocks;

    /// The bit width of standard_metadata.parser_error.
    static const IR::Type_Bits parserErrBits;

    /// This function contains an imperative specification of the inter-pipe interaction in the
    /// target.
    std::vector<Continuation::Command> processDeclaration(const IR::Type_Declaration* cstrType,
                                                          size_t pipeIdx) const;

 public:
    EBPFProgramInfo(const IR::P4Program* program,
                    ordered_map<cstring, const IR::Type_Declaration*> inputBlocks);

    /// @returns the programmable blocks of the program. Should be 6.
    const ordered_map<cstring, const IR::Type_Declaration*>* getProgrammableBlocks() const;

    const IR::Member* getTargetInputPortVar() const override;

    const IR::Member* getTargetOutputPortVar() const override;

    const IR::Expression* dropIsActive() const override;

    const IR::Expression* createTargetUninitialized(const IR::Type* type,
                                                    bool forceTaint) const override;

    const IR::Type_Bits* getParserErrorType() const override;

    const IR::Member* getParserParamVar(const IR::P4Parser* parser, const IR::Type* type,
                                        size_t paramIndex, cstring paramLabel) const override;
};

}  // namespace EBPF

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_TARGETS_EBPF_PROGRAM_INFO_H_ */
