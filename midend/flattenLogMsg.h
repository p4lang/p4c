#ifndef _MIDEND_FLATTENILOGMSG_H_
#define _MIDEND_FLATTENILOGMSG_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

using TypeLogMsgParams = std::pair<IR::IndexedVector<IR::NamedExpression>, std::string>;

/**
This pass translates all occuarence of log_msg function with non-flattend arguments into
set of the flatten calls of log_smg.
*/
class FlattenLogMsg : public Transform {
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;

 public:
    FlattenLogMsg(P4::ReferenceMap* refMap, P4::TypeMap* typeMap);
    const IR::Node* preorder(IR::BlockStatement* blockStatement) override;
    const IR::Node* postorder(IR::MethodCallStatement* methodCallStatement) override;

 protected:
    TypeLogMsgParams unfoldStruct(const IR::Expression* expr, std::string strParam,
                                  std::string curName = "");
    const IR::Type_StructLike* generateNewStructType(const IR::Type_StructLike* structType,
                                                     IR::IndexedVector<IR::NamedExpression> &v);
};

}  // namespace P4

#endif /* _MIDEND_FLATTENILOGMSG_H_ */