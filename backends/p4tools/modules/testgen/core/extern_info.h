#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXTERN_INFO_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXTERN_INFO_H_

#include <sys/types.h>

#include "ir/id.h"
#include "ir/ir.h"
#include "ir/vector.h"

namespace P4Tools::P4Testgen {

class ExternInfo {
 public:
    const IR::MethodCallExpression &originalCall;
    const IR::PathExpression &externObjectRef;
    const IR::ID &methodName;
    const IR::Vector<IR::Argument> *externArgs;

    ExternInfo(const IR::MethodCallExpression &originalCall,
               const IR::PathExpression &externObjectRef, const IR::ID &methodName,
               const IR::Vector<IR::Argument> *externArgs)
        : originalCall(originalCall),
          externObjectRef(externObjectRef),
          methodName(methodName),
          externArgs(externArgs) {}

    /// Do not accidentally copy-assign the extern info.
    ExternInfo &operator=(const ExternInfo &) = delete;

    ExternInfo &operator=(ExternInfo &&) = delete;
};

}  // namespace P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXTERN_INFO_H_ */
