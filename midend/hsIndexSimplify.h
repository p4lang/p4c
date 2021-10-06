#ifndef _MIDEND_HSINDEXSIMPLIFY_H_
#define _MIDEND_HSINDEXSIMPLIFY_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/// This class finds a header stack with non-concrete index 
/// which should be eliminated in unfolding if @a isFinder is true.
/// Otherwise it substitutes index of a header stack in all occuarence of @a arrayIndex.
class HSIndexFindOrTransform : public Transform {
    friend class HSIndexSimplifier;
    const IR::ArrayIndex* arrayIndex;
    bool isFinder;
    int index;

 public:
    HSIndexFindOrTransform() : arrayIndex(nullptr), isFinder(true) {}
    HSIndexFindOrTransform(const IR::ArrayIndex* arrayIndex, int index)
        : arrayIndex(arrayIndex), isFinder(false), index(index) {}
    const IR::Node* postorder(IR::ArrayIndex* curArrayIndex) override;
    size_t getArraySize();
};

/// This class eliminates all non-concrete indexes of the header stacks in the controls.
/// It generates all  
class HSIndexSimplifier : public Transform {
 public:
    HSIndexSimplifier() {}
    IR::Node* preorder(IR::IfStatement* ifStatement) override;
    IR::Node* preorder(IR::AssignmentStatement* assignmentStatement) override;
    IR::Node* preorder(IR::ConstructorCallExpression* expr) override;
    IR::Node* preorder(IR::MethodCallStatement* methodCallStatement) override;
    IR::Node* preorder(IR::SelectExpression* selectExpression) override;
    IR::Node* preorder(IR::SwitchStatement* switchStatement) override;

 protected:
    IR::Node* eliminateArrayIndexes(IR::Statement* statement);
};

}  // namespace P4

#endif /* _MIDEND_HSINDEXSIMPLIFY_H_ */
