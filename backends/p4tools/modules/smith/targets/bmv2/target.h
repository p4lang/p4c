#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_TARGET_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_TARGET_H_

#include <stdint.h>

#include <optional>
#include <string>

#include "backends/p4tools/modules/smith/core/target.h"
#include "ir/ir.h"

namespace P4Tools::P4Smith::BMv2 {

class AbstractBMv2SmithTarget : public SmithTarget {
 protected:
    explicit AbstractBMv2SmithTarget(std::string deviceName, std::string archName);
};

class BMv2V1modelSmithTarget : public AbstractBMv2SmithTarget {
 public:
    /// Registers this target.
    static void make();

    //  const ArchSpec* getArchSpecImpl() const override;

 protected:
 private:
    BMv2V1modelSmithTarget();

    //  static const ArchSpec archSpec;
};

}  // namespace P4Tools::P4Smith::BMv2

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_TARGETS_BMV2_TARGET_H_ */
