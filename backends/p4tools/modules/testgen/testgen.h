#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TESTGEN_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TESTGEN_H_

#include "backends/p4tools/common/p4ctool.h"
#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/options.h"

namespace P4Tools {

namespace P4Testgen {

/// This is main implementation of the P4Testgen tool.
class Testgen : public AbstractP4cTool<TestgenOptions> {
 protected:
    void registerTarget() override;

    int mainImpl(const IR::P4Program* program) override;
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TESTGEN_H_ */
