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
    IR::IndexedVector<IR::StatOrDecl> *components;
    const IR::ArrayIndex* arrayIndex;
    const IR::ArrayIndex* tmpArrayIndex;
    bool isFinder;
    int index;
    ReferenceMap* refMap;
    TypeMap* typeMap;
 public:
    HSIndexFindOrTransform(IR::IndexedVector<IR::StatOrDecl> *components,
                           ReferenceMap* refMap,TypeMap* typeMap) :
        components(components), arrayIndex(nullptr), tmpArrayIndex(nullptr), isFinder(true),
        refMap(refMap), typeMap(typeMap) {}
    HSIndexFindOrTransform(const IR::ArrayIndex* arrayIndex, int index) :
        arrayIndex(arrayIndex), tmpArrayIndex(nullptr), isFinder(false), index(index), 
        refMap(nullptr), typeMap(nullptr) {}
    const IR::Node* postorder(IR::ArrayIndex* curArrayIndex) override;
    size_t getArraySize();
};

/// This class eliminates all non-concrete indexes of the header stacks in the controls.
/// It generates new variables for all expressions in the header stacks indexes and
/// checks thier values for substitution of concrete values. 
class HSIndexSimplifier : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;
    IR::IndexedVector<IR::StatOrDecl> *components;
 public:
    HSIndexSimplifier(ReferenceMap* refMap, TypeMap* typeMap,
        IR::IndexedVector<IR::StatOrDecl> *components = nullptr) :
        refMap(refMap), typeMap(typeMap), components(components) {}
    IR::Node* preorder(IR::IfStatement* ifStatement) override;
    IR::Node* preorder(IR::AssignmentStatement* assignmentStatement) override;
    IR::Node* preorder(IR::BlockStatement* blockStatement) override;
    IR::Node* preorder(IR::ConstructorCallExpression* expr) override;
    IR::Node* preorder(IR::MethodCallStatement* methodCallStatement) override;
    IR::Node* preorder(IR::SelectExpression* selectExpression) override;
    IR::Node* preorder(IR::SwitchStatement* switchStatement) override;

 protected:
    IR::Node* eliminateArrayIndexes(IR::Statement* statement);
};

}  // namespace P4

#endif /* _MIDEND_HSINDEXSIMPLIFY_H_ */
