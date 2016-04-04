#ifndef _BACKENDS_BMV2_MIDEND_H_
#define _BACKENDS_BMV2_MIDEND_H_

#include "ir/ir.h"
#include "frontends/common/options.h"
#include "frontends/p4/evaluator/evaluator.h"

namespace BMV2 {

class MidEnd {
 public:
    P4::BlockMap* process(CompilerOptions& options, const IR::P4Program* program);
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_MIDEND_H_ */
