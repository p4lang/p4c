#ifndef TESTGEN_TESTGEN_H_
#define TESTGEN_TESTGEN_H_

#include "backends/p4tools/common/p4ctool.h"
#include "ir/ir.h"
#include "lib/cstring.h"

#include "backends/p4tools/testgen/lib/final_state.h"
#include "backends/p4tools/testgen/options.h"

namespace P4Tools {

namespace P4Testgen {

/// This is p4testgen.
class Testgen : public AbstractP4cTool<TestgenOptions> {
 protected:
    void registerTarget() override;

    int mainImpl(const IR::P4Program* program) override;

    cstring getProgramName();

    cstring getOutputDir();

    void printTraceAndBranches(int testCount, const FinalState& state);
};

}  // namespace P4Testgen

}  // namespace P4Tools

#endif /* TESTGEN_TESTGEN_H_ */
