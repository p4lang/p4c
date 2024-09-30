#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_OPTIONS_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_OPTIONS_H_
#include <vector>

#include "backends/p4tools/common/options.h"

namespace P4::P4Tools {

class SmithOptions : public AbstractP4cToolOptions {
 public:
    SmithOptions();
    ~SmithOptions() override = default;
    static SmithOptions &get();

    void processArgs(const std::vector<const char *> &args);
};

}  // namespace P4::P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_OPTIONS_H_ */
