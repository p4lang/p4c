#ifndef TESTGEN_TARGETS_EBPF_PROGRAM_INFO_H_
#define TESTGEN_TARGETS_EBPF_PROGRAM_INFO_H_

#include <vector>

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/lib/continuation.h"

namespace P4Tools::P4Testgen::EBPF {

class EBPFProgramInfo : public ProgramInfo {
 private:
    /// The program's top level blocks: the parser, the checksum verifier, the MAU pipeline, the
    /// checksum calculator, and the deparser.
    const ordered_map<cstring, const IR::Type_Declaration *> programmableBlocks;

    /// The bit width of standard_metadata.parser_error.
    static const IR::Type_Bits PARSER_ERR_BITS;

    /// This function contains an imperative specification of the inter-pipe interaction in the
    /// target.
    std::vector<Continuation::Command> processDeclaration(const IR::Type_Declaration *cstrType,
                                                          size_t blockIdx) const;

 public:
    EBPFProgramInfo(const TestgenCompilerResult &compilerResult,
                    ordered_map<cstring, const IR::Type_Declaration *> inputBlocks);

    /// @see ProgramInfo::getArchSpec
    [[nodiscard]] const ArchSpec &getArchSpec() const override;

    /// @returns the programmable blocks of the program. Should be 6.
    [[nodiscard]] const ordered_map<cstring, const IR::Type_Declaration *> *getProgrammableBlocks()
        const;

    [[nodiscard]] const IR::StateVariable &getTargetInputPortVar() const override;

    [[nodiscard]] const IR::StateVariable &getTargetOutputPortVar() const override;

    [[nodiscard]] const IR::Expression *dropIsActive() const override;

    [[nodiscard]] const IR::Type_Bits *getParserErrorType() const override;

    /// @see ProgramInfo::getArchSpec
    static const ArchSpec ARCH_SPEC;

    DECLARE_TYPEINFO(EBPFProgramInfo, ProgramInfo);
};

}  // namespace P4Tools::P4Testgen::EBPF

#endif /* TESTGEN_TARGETS_EBPF_PROGRAM_INFO_H_ */
