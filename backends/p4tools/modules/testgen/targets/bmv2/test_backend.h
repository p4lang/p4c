#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_H_

#include <cstdint>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/trace_events.h"
#include "gsl/gsl-lite.hpp"
#include "ir/ir.h"
#include "lib/big_int_util.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_backend.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"

namespace P4Tools::P4Testgen::Bmv2 {

class Bmv2TestBackend : public TestBackEnd {
 private:
    /// These three constants are used for a special case. When the output packet is 0 in BMv2, the
    /// packet is not dropped, instead you receive garbage (02000000) to be precise.
    /// These values model this output.
    ///  See also: https://github.com/p4lang/behavioral-model/issues/977
    static const int ZERO_PKT_WIDTH = 32;
    static const big_int ZERO_PKT_VAL;
    static const big_int ZERO_PKT_MAX;
    /// List of the supported back ends.
    static const std::set<std::string> SUPPORTED_BACKENDS;

 public:
    explicit Bmv2TestBackend(const ProgramInfo &programInfo, SymbolicExecutor &symbex,
                             const std::filesystem::path &testPath, std::optional<uint32_t> seed);

    TestBackEnd::TestInfo produceTestInfo(
        const ExecutionState *executionState, const Model *completedModel,
        const IR::Expression *outputPacketExpr, const IR::Expression *outputPortExpr,
        const std::vector<gsl::not_null<const TraceEvent *>> *programTraces) override;

    const TestSpec *createTestSpec(const ExecutionState *executionState,
                                   const Model *completedModel, const TestInfo &testInfo) override;
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_H_ */
