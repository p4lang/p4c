#include "backends/p4tools/modules/smith/core/target.h"

#include <string>

#include "backends/p4tools/common/compiler/compiler_target.h"
#include "backends/p4tools/common/core/target.h"

namespace P4C::P4Tools::P4Smith {

SmithTarget::SmithTarget(const std::string &deviceName, const std::string &archName)
    : P4Tools::CompilerTarget("smith", deviceName, archName) {}

const SmithTarget &SmithTarget::get() { return P4Tools::Target::get<SmithTarget>("smith"); }

}  // namespace P4C::P4Tools::P4Smith
