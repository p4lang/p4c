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
    IR::IndexedVector<IR::Declaration> *locals;
    const IR::ArrayIndex* arrayIndex;
    const IR::ArrayIndex* tmpArrayIndex;
    bool isFinder;
    int index;
    ReferenceMap* refMap;
    TypeMap* typeMap;
 public:
    HSIndexFindOrTransform(IR::IndexedVector<IR::Declaration> *locals,
                           ReferenceMap* refMap,TypeMap* typeMap) :
        locals(locals), arrayIndex(nullptr), tmpArrayIndex(nullptr), isFinder(true),
        refMap(refMap), typeMap(typeMap) {}
    HSIndexFindOrTransform(const IR::ArrayIndex* arrayIndex, int index) :
        arrayIndex(arrayIndex), tmpArrayIndex(nullptr), isFinder(false), index(index), 
        refMap(nullptr), typeMap(nullptr) {}
    const IR::Node* postorder(IR::ArrayIndex* curArrayIndex) override;
    size_t getArraySize();
};

/// This class eliminates all non-concrete indexes of the header stacks in the controls.
/// It generates new variables for all expressions in the header stacks indexes and
/// checks their values for substitution of concrete values.
/// Let
/// header h_index { bit<32> index;}
/// header h_stack { bit<32>  a;}
/// struct headers { h_stack[2] h; h_index    i;}
/// headers hdr;
/// Then the assignment hdr.h[hdr.i] = 1 will be translated into
/// bit<32> hdivr0;
/// hdivr0 = hdr.i;
/// if (hdivr0 == 0) { hdr.h[0] = 1;}
/// else if (hdivr0 == 1){hdr.h[1] = 1;} 
class HSIndexSimplifier : public Transform {
    ReferenceMap* refMap;
    TypeMap* typeMap;
    IR::IndexedVector<IR::Declaration> *locals;
 public:
    HSIndexSimplifier(ReferenceMap* refMap, TypeMap* typeMap,
        IR::IndexedVector<IR::Declaration> *locals = nullptr) :
        refMap(refMap), typeMap(typeMap), locals(locals) {}
    IR::Node* preorder(IR::IfStatement* ifStatement) override;
    IR::Node* preorder(IR::AssignmentStatement* assignmentStatement) override;
    IR::Node* preorder(IR::BlockStatement* assignmentStatement) override;
    IR::Node* preorder(IR::ConstructorCallExpression* expr) override;
    IR::Node* preorder(IR::MethodCallStatement* methodCallStatement) override;
    IR::Node* preorder(IR::P4Control* control) override;
    IR::Node* preorder(IR::P4Parser* parser) override;
    IR::Node* preorder(IR::SelectExpression* selectExpression) override;
    IR::Node* preorder(IR::SwitchStatement* switchStatement) override;

 protected:
    IR::Node* eliminateArrayIndexes(IR::Statement* statement);
};

}  // namespace P4

#endif /* _MIDEND_HSINDEXSIMPLIFY_H_ */
