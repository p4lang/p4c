#ifndef _MIDEND_HSINDEXSIMPLIFY_H_
#define _MIDEND_HSINDEXSIMPLIFY_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

typedef std::map<cstring, const IR::PathExpression*> GeneratedVariablesMap;

/// This class finds innermost header stack with non-concrete index.
/// For each found innermost header stack it generates new local variable and
/// adds it to the corresponded local definitions.
class HSIndexFinder : public Inspector {
    friend class HSIndexTransform;
    friend class HSIndexSimplifier;
    IR::IndexedVector<IR::Declaration>* locals;
    ReferenceMap* refMap;
    TypeMap* typeMap;
    const IR::ArrayIndex* arrayIndex;
    const IR::PathExpression* newVariable;
    GeneratedVariablesMap* generatedVariables;
    std::set<cstring> storedMember;
    std::list<IR::Member*> dependedMembers;

 public:
    HSIndexFinder(IR::IndexedVector<IR::Declaration>* locals, ReferenceMap* refMap,
                  TypeMap* typeMap, GeneratedVariablesMap* generatedVariables)
        : locals(locals),
          refMap(refMap),
          typeMap(typeMap),
          arrayIndex(nullptr),
          newVariable(nullptr),
          generatedVariables(generatedVariables) {}
    void postorder(const IR::ArrayIndex* curArrayIndex) override;

 protected:
    void addNewVariable();
};

/// This class substitutes index of a header stack in all occurence of found header stack.
class HSIndexTransform : public Transform {
    friend class HSIndexSimplifier;
    int index;
    HSIndexFinder& hsIndexFinder;

 public:
    HSIndexTransform(HSIndexFinder& finder, int index) : index(index), hsIndexFinder(finder) {}
    const IR::Node* postorder(IR::ArrayIndex* curArrayIndex) override;
};

/// This class eliminates all non-concrete indexes of the header stacks in the controls.
/// It generates new variables for all expressions in the header stacks indexes and
/// checks their values for substitution of concrete values.
/// Each new variable is unique in a scope.
/// Restriction : in/out parameters should be replaced by correspondent assignments.
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
    IR::IndexedVector<IR::Declaration>* locals;
    GeneratedVariablesMap* generatedVariables;

 public:
    HSIndexSimplifier(ReferenceMap* refMap, TypeMap* typeMap,
                      IR::IndexedVector<IR::Declaration>* locals = nullptr,
                      GeneratedVariablesMap* generatedVariables = nullptr)
        : refMap(refMap), typeMap(typeMap), locals(locals), generatedVariables(generatedVariables) {
        if (generatedVariables == nullptr) generatedVariables = new GeneratedVariablesMap();
    }
    IR::Node* preorder(IR::IfStatement* ifStatement) override;
    IR::Node* preorder(IR::AssignmentStatement* assignmentStatement) override;
    IR::Node* preorder(IR::BlockStatement* assignmentStatement) override;
    IR::Node* preorder(IR::MethodCallStatement* methodCallStatement) override;
    IR::Node* preorder(IR::P4Control* control) override;
    IR::Node* preorder(IR::P4Parser* parser) override;
    IR::Node* preorder(IR::SwitchStatement* switchStatement) override;

 protected:
    IR::Node* eliminateArrayIndexes(HSIndexFinder& aiFinder, IR::Statement* statement,
                                    const IR::Expression* expr);
};

}  // namespace P4

#endif /*  _MIDEND_HSINDEXSIMPLIFY_H_ */
