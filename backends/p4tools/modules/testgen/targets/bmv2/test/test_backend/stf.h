#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_TEST_BACKEND_STF_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_TEST_BACKEND_STF_H_

#include <gtest/gtest.h>

#include "backends/p4tools/modules/testgen/lib/test_spec.h"
#include "backends/p4tools/modules/testgen/targets/bmv2/test_backend/stf.h"

namespace P4::P4Tools::Test {

using TestBackendConfiguration = P4Testgen::TestBackendConfiguration;
using Packet = P4Testgen::Packet;
using ActionArg = P4Testgen::ActionArg;
using ActionCall = P4Testgen::ActionCall;
using Exact = P4Testgen::Exact;
using Ternary = P4Testgen::Ternary;
using TableMatch = P4Testgen::TableMatch;
using TableMatchMap = P4Testgen::TableMatchMap;
using TableRule = P4Testgen::TableRule;
using TableConfig = P4Testgen::TableConfig;
using TestSpec = P4Testgen::TestSpec;
using STF = P4Testgen::Bmv2::STF;

/// Helper methods to build configurations for STF Tests.
class STFTest : public testing::Test {
 public:
    TableConfig getForwardTableConfig();
    TableConfig getIPRouteTableConfig();
    TableConfig gettest1TableConfig();
    TableConfig gettest1TableConfig2();
};

}  // namespace P4::P4Tools::Test

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_TEST_BACKEND_STF_H_ */
