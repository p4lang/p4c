#ifndef _MIDEND_HSINDEXSIMPLIFY_H_
#define _MIDEND_HSINDEXSIMPLIFY_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

/// The main class for finding of non-concrete haader stack/arrya index.
class HSIndexFinder : public Transform {
    friend class HSIndexSimplifier;
    const IR::ArrayIndex* arrayIndex;
    bool isFinder;
    int index;

 public:
    HSIndexFinder() : arrayIndex(nullptr), isFinder(true) {}
    HSIndexFinder(const IR::ArrayIndex* arrayIndex, int index)
        : arrayIndex(arrayIndex), isFinder(false), index(index) {}
    const IR::Node* postorder(IR::ArrayIndex* curArrayIndex) override;
    size_t getArraySize();
    static const IR::ArrayIndex* exprIndex2Member(const IR::Type* type,
                                                  const IR::Expression* expression,
                                                  const IR::Constant* constant);
};

/// The main class for concretization of array index.
class HSIndexSimplifier : public Transform {
 public:
    HSIndexSimplifier(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) {}
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
