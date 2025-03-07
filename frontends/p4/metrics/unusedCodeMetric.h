#ifndef FRONTENDS_P4_UNUSED_CODE_H_
#define FRONTENDS_P4_UNUSED_CODE_H_

#include "../ir/ir.h"
#include "metrics.h"

namespace P4 {

class UnusedCodeMetricPass : public Inspector {
 public:
    UnusedCodeMetricPass() { setName("UnusedCodeMetricPass"); }
    bool preorder(const IR::P4Program *program) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_UNUSED_CODE_H_ */
