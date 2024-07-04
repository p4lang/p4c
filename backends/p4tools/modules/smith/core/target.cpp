#include "backends/p4tools/modules/smith/core/target.h"

#include <string>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/compiler/context.h"
#include "backends/p4tools/common/core/target.h"
#include "backends/p4tools/modules/smith/options.h"

namespace P4::P4Tools::P4Smith {

SmithTarget::SmithTarget(const std::string &deviceName, const std::string &archName)
    : P4Tools::CompilerTarget("smith", deviceName, archName) {}

ICompileContext *SmithTarget::makeContext() const {
    return new P4Tools::CompileContext<SmithOptions>;
}

const SmithTarget &SmithTarget::get() { return P4Tools::Target::get<SmithTarget>("smith"); }

}  // namespace P4::P4Tools::P4Smith
