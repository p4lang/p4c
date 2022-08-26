#ifndef MUTATE_MUTATE_H_
#define MUTATE_MUTATE_H_

#include "backends/p4tools/common/options.h"
#include "backends/p4tools/common/p4ctool.h"
#include "backends/p4tools/mutate/options.h"
#include "ir/ir.h"

namespace P4Tools {

namespace P4Mutate {

class Mutate : public AbstractP4cTool<MutateOptions> {
 protected:
    void registerTarget() override;

    int mainImpl(const IR::P4Program* program) override;
};

}  // namespace P4Mutate

}  // namespace P4Tools

#endif /* MUTATE_MUTATE_H_ */
