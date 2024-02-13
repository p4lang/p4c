#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_TEST_BACKEND_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_TEST_BACKEND_H_

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/trace_event.h"
#include "ir/ir.h"

#include "backends/p4tools/modules/testgen/core/program_info.h"
#include "backends/p4tools/modules/testgen/core/symbolic_executor/symbolic_executor.h"
#include "backends/p4tools/modules/testgen/core/target.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"
#include "backends/p4tools/modules/testgen/lib/test_backend.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"

namespace P4Tools::P4Testgen::Pna {

class PnaTestBackend : public TestBackEnd {
 private:
    /// List of the supported back ends.
    static const std::set<std::string> SUPPORTED_BACKENDS;

 public:
    explicit PnaTestBackend(const ProgramInfo &programInfo,
                            const TestBackendConfiguration &testBackendConfiguration,
                            SymbolicExecutor &symbex);

    TestBackEnd::TestInfo produceTestInfo(
        const ExecutionState *executionState, const Model *finalModel,
        const IR::Expression *outputPacketExpr, const IR::Expression *outputPortExpr,
        const std::vector<std::reference_wrapper<const TraceEvent>> *programTraces) override;

    const TestSpec *createTestSpec(const ExecutionState *executionState, const Model *finalModel,
                                   const TestInfo &testInfo) override;
};

}  // namespace P4Tools::P4Testgen::Pna

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_PNA_TEST_BACKEND_H_ */
