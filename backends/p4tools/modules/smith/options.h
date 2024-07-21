#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_OPTIONS_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_OPTIONS_H_
#include <vector>

#include "backends/p4tools/common/options.h"

namespace p4c::P4Tools {

class SmithOptions : public AbstractP4cToolOptions {
 public:
    ~SmithOptions() override = default;
    static SmithOptions &get();

    const char *getIncludePath() const override;
    void processArgs(const std::vector<const char *> &args);

 private:
    SmithOptions();
};

// using P4toZ3Context = P4CContextWithOptions<SmithOptions>;

}  // namespace p4c::P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_OPTIONS_H_ */
