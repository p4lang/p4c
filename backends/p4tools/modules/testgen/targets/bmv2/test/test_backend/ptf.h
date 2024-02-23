#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_TEST_BACKEND_PTF_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_TEST_BACKEND_PTF_H_

#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend/ptf.h"
#include "backends/p4tools/modules/testgen/test/gtest_utils.h"

namespace Test {

using TestBackendConfiguration = P4Tools::P4Testgen::TestBackendConfiguration;
using Packet = P4Tools::P4Testgen::Packet;
using ActionArg = P4Tools::P4Testgen::ActionArg;
using ActionCall = P4Tools::P4Testgen::ActionCall;
using Exact = P4Tools::P4Testgen::Exact;
using Ternary = P4Tools::P4Testgen::Ternary;
using TableMatch = P4Tools::P4Testgen::TableMatch;
using TableMatchMap = P4Tools::P4Testgen::TableMatchMap;
using TableRule = P4Tools::P4Testgen::TableRule;
using TableConfig = P4Tools::P4Testgen::TableConfig;
using TestSpec = P4Tools::P4Testgen::TestSpec;
using PTF = P4Tools::P4Testgen::Bmv2::PTF;

/// Helper methods to build configurations for PTF Tests.
class PTFTest : public P4ToolsTest {
 public:
    TableConfig getForwardTableConfig();
    TableConfig getIPRouteTableConfig();
    TableConfig gettest1TableConfig();
    TableConfig gettest1TableConfig2();
};

}  // namespace Test

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_TEST_BACKEND_PTF_H_ */
