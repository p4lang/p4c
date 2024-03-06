#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_CORE_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_CORE_TARGET_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "backends/p4tools/common/core/target.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace P4Tools {

namespace P4Smith {

class SmithTarget : public P4Tools::Target {
 public:
    /// @returns the singleton instance for the current target.
    static const SmithTarget &get();

 protected:
    explicit SmithTarget(std::string deviceName, std::string archName);

 private:
};

}  // namespace P4Smith

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_CORE_TARGET_H_ */
