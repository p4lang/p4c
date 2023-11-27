#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_COMMON_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_COMMON_H_

#include <filesystem>
#include <optional>
#include <vector>

#include <inja/inja.hpp>

#include "backends/p4tools/modules/testgen/lib/test_framework.h"
#include "backends/p4tools/modules/testgen/lib/test_object.h"
#include "backends/p4tools/modules/testgen/lib/test_spec.h"

namespace P4Tools::P4Testgen::Bmv2 {

/// Bmv2TestFramework provides common utility functions for BMv2-style test frameworks.
class Bmv2TestFramework : public TestFramework {
 public:
    explicit Bmv2TestFramework(std::filesystem::path basePath,
                               std::optional<unsigned int> seed = std::nullopt);

 protected:
    /// Converts all the control plane objects into Inja format.
    virtual inja::json getControlPlane(const TestSpec *testSpec) const;

    /// Returns the configuration for a cloned packet configuration.
    virtual inja::json getClone(const TestObjectMap &cloneSpecs) const;

    /// @returns the configuration for a meter call (may set the meter to GREEN, YELLOW, or RED)
    virtual inja::json::array_t getMeter(const TestObjectMap &meterValues) const;

    /// Helper function for the control plane table inja objects.
    virtual inja::json getControlPlaneForTable(const TableMatchMap &matches,
                                               const std::vector<ActionArg> &args) const;

    /// Converts the input packet and port into Inja format.
    virtual inja::json getSend(const TestSpec *testSpec) const;

    /// Converts the output packet, port, and mask into Inja format.
    virtual inja::json getExpectedPacket(const TestSpec *testSpec) const;
};
}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_TEST_BACKEND_COMMON_H_ */
