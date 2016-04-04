#ifndef _P4_CREATEBUILTINS_H_
#define _P4_CREATEBUILTINS_H_

#include "ir/ir.h"

namespace P4 {
class CreateBuiltins final : public Modifier {
 public:
    using Modifier::postorder;
    void postorder(IR::ParserContainer* parser) override;
};
}  // namespace P4

#endif /* _P4_CREATEBUILTINS_H_ */
