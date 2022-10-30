#ifndef BACKENDS_P4TOOLS_COMMON_COMPILER_COPY_HEADERS_H_
#define BACKENDS_P4TOOLS_COMMON_COMPILER_COPY_HEADERS_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4Tools {

/// Converts assignments of headers, header stacks, and structs into assignments of their
/// individual fields.
///
/// Header assignments are converted into an `if` statement. If the source header is valid, then
/// the validity bit in the destination header is set and the component fields are copied.
/// Otherwise, if the source header is invalid, then the validity bit in the destination header is
/// cleared.
class DoCopyHeaders : public Transform {
    P4::TypeMap* typeMap;
    P4::ReferenceMap* refMap;

 public:
    explicit DoCopyHeaders(P4::ReferenceMap* refMap, P4::TypeMap* typeMap);
    const IR::Node* postorder(IR::AssignmentStatement* statement) override;
};

class CopyHeaders : public PassRepeated {
 public:
    CopyHeaders(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
                P4::TypeChecking* typeChecking = nullptr);
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_COMPILER_COPY_HEADERS_H_ */
