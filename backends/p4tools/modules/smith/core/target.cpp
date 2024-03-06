#include "backends/p4tools/common/core/target.h"

#include <memory>
#include <string>
#include <utility>

#include "backends/p4tools/modules/smith/core/target.h"
#include "ir/ir.h"
#include "lib/enumerator.h"

namespace P4Tools {

namespace P4Smith {

SmithTarget::SmithTarget(std::string deviceName, std::string archName)
    : P4Tools::Target("smith", deviceName, archName) {}

const SmithTarget &SmithTarget::get() { return P4Tools::Target::get<SmithTarget>("smith"); }

}  // namespace P4Smith

}  // namespace P4Tools
