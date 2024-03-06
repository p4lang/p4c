#ifndef BACKENDS_P4TOOLS_MODULES_SMITH_SMITH_H_
#define BACKENDS_P4TOOLS_MODULES_SMITH_SMITH_H_

#include <vector>

#include "backends/p4tools/common/p4ctool.h"
#include "backends/p4tools/modules/smith/options.h"
#include "ir/ir.h"

namespace P4Tools {

namespace P4Smith {

class Smith : public AbstractP4cTool<SmithOptions> {
 protected:
    void registerTarget() override;

    int mainImpl(const IR::P4Program *program) override;

 public:
    virtual ~Smith() = default;
    int main(const std::vector<const char *> &args);
};

}  // namespace P4Smith

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_MODULES_SMITH_SMITH_H_ */
