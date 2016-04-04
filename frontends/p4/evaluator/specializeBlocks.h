#ifndef _EVALUATOR_SPECIALIZEBLOCKS_H_
#define _EVALUATOR_SPECIALIZEBLOCKS_H_

// TODO: this is not general enough; this algorithm should be
// removed and replaced with a better one.  In particular, this
// does not handle constructor parameters that are externs.

#include <iostream>

#include "lib/cstring.h"
#include "lib/map.h"
#include "ir/ir.h"
#include "../../common/typeMap.h"
#include "../../common/resolveReferences/referenceMap.h"

namespace P4 {

/*
  Consider a parameterized block, e.g.:

control p(in bit<2> data)(bit param) { ... x = param; ... }

  This block can be instantiated using a constructor:

control parent() {
   p(1w1) pinstance;
   apply { ... }
}

   Specialization will clone p into p_1 and replace all instances of
   'param' with the constructor-supplied value (1w1).  The specialized
   clone is used in the instantiation instead of the original call
   (which probably becomes dead code).

control p_1(in bit<2> data) { ... x = 1w1; ... }
control parent() {
    p_1() pinstance;
    apply { ... }
}

   Also, specialization must be done from the "outside-in": one cannot
   specialize an instantiation that occurs within a parameterized
   block, because not all parameters may be constant yet.  E.g.,

control q()(bit param) {
   z(param) zinst;
}

   We can specialize zinst only after specializing q.
*/

struct BlockSpecialization final {
    const IR::Node* constructor;  // IR node where block is instantiated
                                  // (Declaration_Instance or ConstructorCallExpression).
    const IR::IContainer* cont;   // Container that is being specialized.
    const IR::Vector<IR::Expression>* arguments;  // Constructor arguments to use for specialization
                                  // (all must be compile-time constants).
    const IR::Vector<IR::Type>* typeArguments;  // Type arguments to use.
    IR::ID cloneId;               // Identifier for the clone
    IR::ID useId;                 // Identifier for the constructor
    const IR::Type_Name* type;    // Type_Name for the newly created type

    BlockSpecialization(const IR::Node* constructor, const IR::IContainer* cont,
                        const IR::Vector<IR::Expression>* arguments,
                        const IR::Vector<IR::Type>* typeArguments,
                        cstring newName);
    void dbprint(std::ostream& out) const;
};

class BlockSpecList final {
    std::vector<BlockSpecialization*> toSpecialize;
    std::map<const IR::Node*, const BlockSpecialization*> constructorMap;
    std::map<const IR::IContainer*, std::vector<const BlockSpecialization*>*> containerMap;
 public:
    void clear() { constructorMap.clear(); toSpecialize.clear(); containerMap.clear(); }
    void specialize(const IR::Node* node, const IR::IContainer* cont,
                    const IR::Vector<IR::Expression>* args,
                    const IR::Vector<IR::Type>* typeArgs, cstring newname);
    const BlockSpecialization* findInvocation(const IR::Node* node)
    { return get(constructorMap, node); }
    std::vector<const BlockSpecialization*>* findReplacements(const IR::IContainer* cont)
    { return get(containerMap, cont); }
    void dbprint(std::ostream& out) const;
};

//////////////////////////////////////////////////////////////////////////////////

class FindBlocksToSpecialize final : public Inspector {
    ReferenceMap*  refMap;    // input
    TypeMap*       typeMap;  // input
    BlockSpecList* blocks;   // output

    void processNode(const IR::Node* node, const IR::Vector<IR::Expression>* args,
                     const IR::Type* decl);
    bool isConstant(const IR::Expression* expr);
 public:
    FindBlocksToSpecialize(ReferenceMap* refMap, TypeMap* typeMap, BlockSpecList* blocks) :
            refMap(refMap), typeMap(typeMap), blocks(blocks) { visitDagOnce = false; }

    using Inspector::preorder;

    Visitor::profile_t init_apply(const IR::Node* node) override;
    bool preorder(const IR::Declaration_Instance* inst) override;
    bool preorder(const IR::ConstructorCallExpression* expr) override;
};

// Takes control or parser blocks that are parameterized
// and specializes them for the constructor parameters that are used.
class SpecializeBlocks final : public Transform {
    ReferenceMap* refMap;   // input
    BlockSpecList* blocks;  // input
 public:
    SpecializeBlocks(ReferenceMap* refMap, BlockSpecList* blocks) :
            refMap(refMap), blocks(blocks) { visitDagOnce = false; }

    using Transform::preorder;

    profile_t init_apply(const IR::Node* node) override;
    const IR::Node* postorder(IR::ControlContainer* cont) override;
    const IR::Node* postorder(IR::ParserContainer* cont) override;
    const IR::Node* postorder(IR::Declaration_Instance* inst) override;
    const IR::Node* postorder(IR::ConstructorCallExpression* expr) override;
    const IR::Node* postorder(IR::P4Program* prog) override;
};

}  // namespace P4

#endif /* _EVALUATOR_SPECIALIZEBLOCKS_H_ */
