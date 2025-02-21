#include "backends/p4tools/modules/testgen/targets/tofino/compiler_result.h"

#include <utility>

namespace P4::P4Tools::P4Testgen::Tofino {

TofinoCompilerResult::TofinoCompilerResult(TestgenCompilerResult compilerResult,
                                           DirectExternMap directExternMap)
    : TestgenCompilerResult(std::move(compilerResult)),
      directExternMap(std::move(directExternMap)) {}

const DirectExternMap &TofinoCompilerResult::getDirectExternMap() const { return directExternMap; }

}  // namespace P4::P4Tools::P4Testgen::Tofino
