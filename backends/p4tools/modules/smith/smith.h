#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_SMITH_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_SMITH_H_

#include <vector>

#include "backends/p4tools/common/compiler/compiler_result.h"
#include "backends/p4tools/common/p4ctool.h"
#include "backends/p4tools/modules/smith/options.h"

namespace P4Tools::P4Smith {

class Smith : public AbstractP4cTool<SmithOptions> {
 protected:
    void registerTarget() override;

    int mainImpl(const CompilerResult &compilerResult) override;

 public:
    virtual ~Smith() = default;
    int main(const std::vector<const char *> &args);
};

}  // namespace P4Tools::P4Smith

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_SMITH_H_ */
