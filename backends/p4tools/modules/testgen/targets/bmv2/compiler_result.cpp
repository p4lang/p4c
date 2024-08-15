#include "backends/p4tools/modules/testgen/targets/bmv2/compiler_result.h"

#include <optional>
#include <string>
#include <utility>

#include "backends/bmv2/common/annotations.h"
#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/midend.h"
#include "frontends/common/options.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "midend/coverage.h"

#include "backends/p4tools/modules/testgen/options.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/map_direct_externs.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/p4_asserts_parser.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/p4_refers_to_parser.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/p4runtime_translation.h"

namespace P4::P4Tools::P4Testgen::Bmv2 {

BMv2V1ModelCompilerResult::BMv2V1ModelCompilerResult(TestgenCompilerResult compilerResult,
                                                     P4::P4RuntimeAPI p4runtimeApi,
                                                     DirectExternMap directExternMap,
                                                     ConstraintsVector p4ConstraintsRestrictions)
    : TestgenCompilerResult(std::move(compilerResult)),
      p4runtimeApi(p4runtimeApi),
      directExternMap(std::move(directExternMap)),
      p4ConstraintsRestrictions(std::move(p4ConstraintsRestrictions)) {}

const P4::P4RuntimeAPI &BMv2V1ModelCompilerResult::getP4RuntimeApi() const { return p4runtimeApi; }

ConstraintsVector BMv2V1ModelCompilerResult::getP4ConstraintsRestrictions() const {
    return p4ConstraintsRestrictions;
}

const DirectExternMap &BMv2V1ModelCompilerResult::getDirectExternMap() const {
    return directExternMap;
}

}  // namespace P4::P4Tools::P4Testgen::Bmv2
