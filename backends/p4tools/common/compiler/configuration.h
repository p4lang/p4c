#ifndef COMMON_COMPILER_CONFIGURATION_H_
#define COMMON_COMPILER_CONFIGURATION_H_

#include <climits>
#include <limits>

#include "ir/configuration.h"

namespace P4Tools {

/// A P4CConfiguration implementation that doesn't limit the maximum width for a bit field or
/// integer.
class CompilerConfiguration : public DefaultP4CConfiguration {
 public:
    int maximumWidthSupported() const override { return INT_MAX; }

    /// @return the singleton instance.
    static const CompilerConfiguration& get() {
        static CompilerConfiguration instance;
        return instance;
    }
    virtual ~CompilerConfiguration() = default;

 protected:
    CompilerConfiguration() = default;
};

}  // namespace P4Tools

#endif /* COMMON_COMPILER_CONFIGURATION_H_ */
