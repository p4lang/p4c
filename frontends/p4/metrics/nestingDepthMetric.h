#ifndef FRONTENDS_P4_NESTING_DEPTH_H_
#define FRONTENDS_P4_NESTING_DEPTH_H_

#include "../ir/ir.h"
#include "metrics.h"

namespace P4 {

class NestingDepthMetricPass : public Inspector {
 public:
    NestingDepthMetricPass() { setName("NestingDepthPass"); }
    bool preorder(const IR::P4Program *program) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_NESTING_DEPTH_H_ */
