#ifndef SMITH_SMITH_H_
#define SMITH_SMITH_H_

#include "backends/p4tools/common/p4ctool.h"
#include "backends/p4tools/smith/options.h"
#include "ir/ir.h"

namespace P4Tools {

namespace P4Smith {

class Smith : public AbstractP4cTool<SmithOptions> {
 protected:
    void registerTarget() override;

    int mainImpl(const IR::P4Program* program) override;
};

}  // namespace P4Smith

}  // namespace P4Tools

#endif /* SMITH_SMITH_H_ */
