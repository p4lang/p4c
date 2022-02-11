#ifndef _MIDEND_FLATTENILOGMSG_H_
#define _MIDEND_FLATTENILOGMSG_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

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
    std::pair<IR::Vector<IR::Expression>, std::string> unfoldStruct(const IR::Expression* expr,
      const IR::Type_StructLike* typeStruct, std::string& strParam);
};

}  // namespace P4

#endif /* _MIDEND_FLATTENILOGMSG_H_ */