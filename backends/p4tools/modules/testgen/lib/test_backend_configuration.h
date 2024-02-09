#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_TEST_BACKEND_CONFIGURATION_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_TEST_BACKEND_CONFIGURATION_H_

#include <filesystem>
#include <optional>

#include "lib/cstring.h"

namespace P4Tools::P4Testgen {

/// A file path which may not be set.
using OptionalFilePath = std::optional<std::filesystem::path>;

/// Configuration parameters for P4Testgen test back ends.
/// These parameters may influence how tests are generated.
struct TestBackendConfiguration {
    /// The base name of the test.
    cstring testBaseName;

    /// The maximum number of tests to produce. Defaults to one test.
    int64_t maxTests = 1;

    /// The base path to be used when writing out tests.
    /// May not be set, which means no tests are written to file.
    OptionalFilePath fileBasePath;

    /// The initial seed used to generate tests. If it is not set, no seed was used.
    std::optional<unsigned int> seed;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_LIB_TEST_BACKEND_CONFIGURATION_H_ */
