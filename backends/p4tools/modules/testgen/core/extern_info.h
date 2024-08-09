#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXTERN_INFO_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXTERN_INFO_H_

#include <sys/types.h>

#include "ir/id.h"
#include "ir/ir.h"
#include "ir/vector.h"

namespace P4::P4Tools::P4Testgen {

/// This class defines parameters useful for the invocation of P4 extern and P4Testgen-internal
/// functions.
class ExternInfo {
 public:
    /// Reference to the original P4 extern call.
    const IR::MethodCallExpression &originalCall;
    /// Name of the extern object the call was a member of, if any.
    const IR::PathExpression &externObjectRef;
    /// Name of the extern method.
    const IR::ID &methodName;
    /// Arguments to the extern method.
    const IR::Vector<IR::Argument> &externArguments;

    ExternInfo(const IR::MethodCallExpression &originalCall,
               const IR::PathExpression &externObjectRef, const IR::ID &methodName,
               const IR::Vector<IR::Argument> &externArguments)
        : originalCall(originalCall),
          externObjectRef(externObjectRef),
          methodName(methodName),
          externArguments(externArguments) {}

    /// Do not accidentally copy-assign the extern info. It is only passed as reference.
    ExternInfo &operator=(const ExternInfo &) = delete;
    ExternInfo &operator=(ExternInfo &&) = delete;
};

}  // namespace P4::P4Tools::P4Testgen

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_CORE_EXTERN_INFO_H_ */
