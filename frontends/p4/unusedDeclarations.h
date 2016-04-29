#ifndef _P4_UNUSEDDECLARATIONS_H_
#define _P4_UNUSEDDECLARATIONS_H_

#include "ir/ir.h"
#include "../common/resolveReferences/referenceMap.h"

namespace P4 {

class RemoveUnusedDeclarations : public Transform {
    const ReferenceMap* refMap;
    const IR::Node* process(const IR::IDeclaration* decl);

 public:
    explicit RemoveUnusedDeclarations(const ReferenceMap* refMap) : refMap(refMap) {}

    using Transform::postorder;
    using Transform::preorder;
    using Transform::init_apply;

    Visitor::profile_t init_apply(const IR::Node *root) override;

    const IR::Node* preorder(IR::P4Control* cont) override;
    const IR::Node* preorder(IR::P4Parser* cont) override;
    const IR::Node* preorder(IR::P4Table* cont) override;
    const IR::Node* preorder(IR::ParserState* state)  override;
    const IR::Node* preorder(IR::Type_Enum* type)  override;

    const IR::Node* preorder(IR::Declaration_Instance* decl) override
    // don't scan the initializer
    { auto result = process(decl); prune(); return result; }
    const IR::Node* preorder(IR::Declaration_Errors* decl) override
    { prune(); return decl; }
    const IR::Node* preorder(IR::Declaration_MatchKind* decl) override
    { prune(); return decl; }
    const IR::Node* preorder(IR::Type_StructLike* type) override
    { prune(); return type; }
    const IR::Node* preorder(IR::Type_Extern* type) override
    { prune(); return type; }
    const IR::Node* preorder(IR::Type_Method* type) override
    { prune(); return type; }

    const IR::Node* preorder(IR::Declaration_Variable* decl)  override;
    const IR::Node* preorder(IR::Parameter* param) override { return param; }  // never dead
    const IR::Node* preorder(IR::Declaration* decl) override { return process(decl); }
    const IR::Node* preorder(IR::Type_Declaration* decl) override { return process(decl); }
};

}  // namespace P4

#endif /* _P4_UNUSEDDECLARATIONS_H_ */
