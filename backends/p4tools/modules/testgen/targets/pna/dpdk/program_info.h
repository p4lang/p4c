#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_DPDK_PROGRAM_INFO_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_DPDK_PROGRAM_INFO_H_

#include <cstddef>
#include <vector>

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"

#include "backends/p4tools/modules/testgen/lib/continuation.h"
#include "backends/p4tools/modules/testgen/targets/pna/shared_program_info.h"

namespace P4Tools::P4Testgen::Pna {

class PnaDpdkProgramInfo : public SharedPnaProgramInfo {
 private:
    /// This function contains an imperative specification of the inter-pipe interaction in the
    /// target.
    std::vector<Continuation::Command> processDeclaration(const IR::Type_Declaration *typeDecl,
                                                          size_t blockIdx) const;

 public:
    PnaDpdkProgramInfo(const TestgenCompilerResult &compilerResult,
                       ordered_map<cstring, const IR::Type_Declaration *> inputBlocks);

    /// @see ProgramInfo::getArchSpec
    const ArchSpec &getArchSpec() const override;

    /// @see ProgramInfo::getArchSpec
    static const ArchSpec ARCH_SPEC;

    DECLARE_TYPEINFO(PnaDpdkProgramInfo, SharedPnaProgramInfo);
};

}  // namespace P4Tools::P4Testgen::Pna

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_DPDK_PROGRAM_INFO_H_ */
