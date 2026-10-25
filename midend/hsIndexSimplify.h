#ifndef MIDEND_HSINDEXSIMPLIFY_H_
#define MIDEND_HSINDEXSIMPLIFY_H_

#include <memory>

#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"

namespace P4 {

using GeneratedVariablesMap = std::map<cstring, const IR::PathExpression *>;

/// This class finds innermost header stack with non-concrete index.
/// For each found innermost header stack it generates new local variable and
/// adds it to the corresponded local definitions.
class HSIndexFinder : public Inspector {
    friend class HSIndexTransform;
    friend class HSIndexContretizer;
    IR::IndexedVector<IR::Declaration> *locals;
    std::shared_ptr<NameGenerator> nameGen;
    TypeMap *typeMap;
    const IR::ArrayIndex *arrayIndex = nullptr;
    const IR::PathExpression *newVariable = nullptr;
    GeneratedVariablesMap *generatedVariables;
    std::set<cstring> storedMember;
    std::list<IR::Member *> dependedMembers;

 public:
    HSIndexFinder(IR::IndexedVector<IR::Declaration> *locals,
                  std::shared_ptr<NameGenerator> nameGen, TypeMap *typeMap,
                  GeneratedVariablesMap *generatedVariables)
        : locals(locals),
          nameGen(nameGen),
          typeMap(typeMap),
          generatedVariables(generatedVariables) {}
    void postorder(const IR::ArrayIndex *curArrayIndex) override;

 protected:
    void addNewVariable();
};

/// This class substitutes index of a header stack in all occurences of found header stack.
class HSIndexTransform : public Transform {
    friend class HSIndexContretizer;
    size_t index;
    HSIndexFinder &hsIndexFinder;

 public:
    HSIndexTransform(HSIndexFinder &finder, size_t index) : index(index), hsIndexFinder(finder) {}
    const IR::Node *postorder(IR::ArrayIndex *curArrayIndex) override;
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
class HSIndexContretizer : public Transform {
    TypeMap *typeMap;
    std::shared_ptr<MinimalNameGenerator> nameGen;
    IR::IndexedVector<IR::Declaration> *locals;
    GeneratedVariablesMap *generatedVariables;
    size_t expansion = 0, maxExpansion;
    int id;
    static int idCtr;

 public:
    explicit HSIndexContretizer(TypeMap *typeMap, size_t maxExpansion,
                                std::shared_ptr<MinimalNameGenerator> nameGen = nullptr,
                                IR::IndexedVector<IR::Declaration> *locals = nullptr,
                                GeneratedVariablesMap *generatedVariables = nullptr)
        : typeMap(typeMap),
          nameGen(nameGen),
          locals(locals),
          generatedVariables(generatedVariables),
          maxExpansion(maxExpansion) {
        id = ++idCtr;
        if (generatedVariables == nullptr) generatedVariables = new GeneratedVariablesMap();
        LOG5("HSIndexContretizer(" << id << ") maxExpansion = " << maxExpansion);
    }
    Visitor::profile_t init_apply(const IR::Node *node) override {
        auto rv = Transform::init_apply(node);

        if (!nameGen) {
            nameGen = std::make_shared<MinimalNameGenerator>();
            node->apply(*nameGen);
        }

        return rv;
    }

    IR::Node *preorder(IR::IfStatement *ifStatement) override;
    IR::Node *preorder(IR::BaseAssignmentStatement *assignmentStatement) override;
    IR::Node *preorder(IR::BlockStatement *blockStatement) override;
    IR::Node *preorder(IR::MethodCallStatement *methodCallStatement) override;
    IR::Node *preorder(IR::P4Control *control) override;
    IR::Node *preorder(IR::P4Parser *parser) override;
    IR::Node *preorder(IR::SwitchStatement *switchStatement) override;

 protected:
    IR::Node *eliminateArrayIndexes(HSIndexFinder &aiFinder, IR::Statement *statement,
                                    const IR::Expression *expr);
};

class HSIndexSimplifier : public PassManager {
 public:
    explicit HSIndexSimplifier(TypeMap *typeMap, size_t maxExpansion = 1000) {
        // remove block statements
        passes.push_back(new TypeChecking(nullptr, typeMap, true));
        passes.push_back(new HSIndexContretizer(typeMap, maxExpansion));
        setName("HSIndexSimplifier");
    }
};

}  // namespace P4

#endif /* MIDEND_HSINDEXSIMPLIFY_H_ */
