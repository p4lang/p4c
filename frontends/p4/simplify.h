#ifndef _FRONTENDS_P4_SIMPLIFY_H_
#define _FRONTENDS_P4_SIMPLIFY_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

// Determines whether an expression may have side-effects
class SideEffects : public Inspector {
 private:
    ReferenceMap* refMap;
    TypeMap*      typeMap;
    bool          hasSideEffects;
    void postorder(const IR::MethodCallExpression* mce) override {
        if (refMap == nullptr || typeMap == nullptr) {
            // conservative
            hasSideEffects = true;
            return;
        }
        // TODO: add an annotation to indicate lack of side-effects
        auto mi = MethodInstance::resolve(mce, refMap, typeMap);
        if (!mi->is<BuiltInMethod>()) {
            hasSideEffects = true;
            return;
        }
        auto bim = mi->to<BuiltInMethod>();
        if (bim->name.name != IR::Type_Header::isValid)
            hasSideEffects = true;
    }
    void postorder(const IR::ConstructorCallExpression*) override {
        hasSideEffects = true;
    }
    // If you pass nullptr for these arguments the check will be more conservative
    SideEffects(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap), hasSideEffects(false) {}

 public:
    // Returns true if the expression may have side-effects.
    static bool check(const IR::Expression* expression, ReferenceMap* refMap, TypeMap* typeMap) {
        SideEffects se(refMap, typeMap);
        expression->apply(se);
        return se.hasSideEffects;
    }
};

class SimplifyControlFlow : public Transform {
    ReferenceMap* refMap;
    TypeMap*      typeMap;
 public:
    SimplifyControlFlow(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); }
    const IR::Node* postorder(IR::BlockStatement* statement) override;
    const IR::Node* postorder(IR::IfStatement* statement) override;
    const IR::Node* postorder(IR::EmptyStatement* statement) override;
    const IR::Node* postorder(IR::SwitchStatement* statement) override;
};

}  // namespace P4

#endif /* _FRONTENDS_P4_SIMPLIFY_H_ */
