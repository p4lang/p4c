#ifndef MIDEND_FLATTENLOGMSG_H_
#define MIDEND_FLATTENLOGMSG_H_

#include <stddef.h>

#include <string>
#include <utility>
#include <vector>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/pass_manager.h"
#include "ir/visitor.h"
#include "lib/cstring.h"
#include "lib/null.h"
#include "lib/ordered_map.h"
#include "lib/safe_vector.h"

namespace P4 {

using TypeLogMsgParams = std::pair<IR::IndexedVector<IR::NamedExpression>, std::string>;

/*
   Find types in log_msg function for replace.
*/
class FindTypesInLogMsgInvocationToReplace : public Inspector {
    P4::TypeMap *typeMap;
    ordered_map<cstring, const IR::Type_StructLike *> replacement;
    // Map with new log_msg methods where is each key is a original identifier of IR::Node.
    ordered_map<unsigned, const IR::MethodCallStatement *> logMsgReplacement;
    size_t index;

 public:
    explicit FindTypesInLogMsgInvocationToReplace(P4::TypeMap *typeMap) : typeMap(typeMap) {
        setName("FindTypesInLogMsgInvocationToReplace");
        CHECK_NULL(typeMap);
    }
    bool preorder(const IR::MethodCallStatement *methodCallStatement) override;
    void createReplacement(const IR::Type_StructLike *type);
    const IR::MethodCallStatement *prepareLogMsgStatement(
        const IR::MethodCallStatement *methodCallStatement);
    const IR::Type_StructLike *getReplacement(const cstring name) const {
        return ::get(replacement, name);
    }
    const IR::MethodCallStatement *getReplacementMethodCall(unsigned id) const {
        return ::get(logMsgReplacement, id);
    }
    bool empty() const { return replacement.empty(); }
    bool hasStructInParameter(const IR::MethodCallStatement *methodCallStatement);

 protected:
    TypeLogMsgParams unfoldStruct(const IR::Expression *expr, std::string strParam,
                                  std::string curName = "");
    const IR::Type_StructLike *generateNewStructType(const IR::Type_StructLike *structType,
                                                     IR::IndexedVector<IR::NamedExpression> &v);
    IR::ID newName();
};

/**
This pass translates all occuarence of log_msg function with non-flattend arguments into
set of the flatten calls of log_smg.
For example,
struct alt_t {
    bit<1> valid;
    bit<7> port;
}
...
t : slt_t;
...
log_msg("t={}", {t});

The flattened log_msg is shown below.

log_msg("t=(valid:{},port:{})", {t.valid, t.port});
*/
class ReplaceLogMsg : public Transform, P4WriteContext {
    P4::TypeMap *typeMap;
    FindTypesInLogMsgInvocationToReplace *findTypesInLogMsgInvocationToReplace;

 public:
    explicit ReplaceLogMsg(
        P4::TypeMap *typeMap,
        FindTypesInLogMsgInvocationToReplace *findTypesInLogMsgInvocationToReplace)
        : typeMap(typeMap),
          findTypesInLogMsgInvocationToReplace(findTypesInLogMsgInvocationToReplace) {
        CHECK_NULL(typeMap);
        CHECK_NULL(findTypesInLogMsgInvocationToReplace);
        setName("ReplaceLogMsg");
    }
    const IR::Node *preorder(IR::P4Program *program) override;
    const IR::Node *postorder(IR::MethodCallStatement *methodCallStatement) override;
    const IR::Node *postorder(IR::Type_Struct *typeStruct) override;
};

class FlattenLogMsg final : public PassManager {
 public:
    FlattenLogMsg(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
        auto findTypesInLogMsgInvocationToReplace =
            new FindTypesInLogMsgInvocationToReplace(typeMap);
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(findTypesInLogMsgInvocationToReplace);
        passes.push_back(new ReplaceLogMsg(typeMap, findTypesInLogMsgInvocationToReplace));
        passes.push_back(new ClearTypeMap(typeMap));
        setName("FlattenLogMsg");
    }
};

}  // namespace P4

#endif /* MIDEND_FLATTENLOGMSG_H_ */
