#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_OPTIONS_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_OPTIONS_H_
#include <vector>

#include "backends/p4tools/common/options.h"
#include "lib/cstring.h"
namespace P4Tools {

class SmithOptions : public AbstractP4cToolOptions {
 public:
    static SmithOptions &get();

    const char *getIncludePath() override;
    void processArgs(const std::vector<const char *> &args);

    cstring compiler_version;

 private:
    SmithOptions();
};

// using P4toZ3Context = P4CContextWithOptions<SmithOptions>;

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_OPTIONS_H_ */
