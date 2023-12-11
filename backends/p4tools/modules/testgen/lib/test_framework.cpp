#include "backends/p4tools/modules/testgen/lib/test_framework.h"

#include <optional>
#include <utility>

namespace P4Tools::P4Testgen {

TestFramework::TestFramework(std::filesystem::path basePath,
                             std::optional<unsigned int> seed = std::nullopt)
    : basePath(std::move(basePath)), seed(seed) {}

}  // namespace P4Tools::P4Testgen
