#ifndef BACKENDS_P4TOOLS_COMMON_COMPILER_CONFIGURATION_H_
#define BACKENDS_P4TOOLS_COMMON_COMPILER_CONFIGURATION_H_

#include <climits>

#include "ir/configuration.h"

namespace p4c::P4Tools {

/// A P4CConfiguration implementation that increases the maximum width for a bit field or
/// integer.
class CompilerConfiguration : public DefaultP4CConfiguration {
 public:
    [[nodiscard]] int maximumWidthSupported() const override { return INT_MAX; }

    /// @return the singleton instance.
    static const CompilerConfiguration &get() {
        static CompilerConfiguration instance;
        return instance;
    }
    virtual ~CompilerConfiguration() = default;

 protected:
    CompilerConfiguration() = default;
};

}  // namespace p4c::P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_COMPILER_CONFIGURATION_H_ */
