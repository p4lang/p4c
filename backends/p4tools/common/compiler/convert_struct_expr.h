#ifndef BACKENDS_P4TOOLS_COMMON_COMPILER_CONVERT_STRUCT_EXPR_H_
#define BACKENDS_P4TOOLS_COMMON_COMPILER_CONVERT_STRUCT_EXPR_H_

#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"

namespace P4Tools {

/// This pass does two things. First, it converts any type member of struct expression that is a
/// Type_Name into a Tupe_StructLike. Second, if the type is a Type_Header, it converts the
/// StructExpression into a HeaderExpression.
class ConvertStructExpr : public Transform {
 private:
    const P4::TypeMap *typeMap;

 public:
    explicit ConvertStructExpr(const P4::TypeMap *typeMap);

    const IR::Node *postorder(IR::StructExpression *expr) override;
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_COMPILER_CONVERT_STRUCT_EXPR_H_ */
