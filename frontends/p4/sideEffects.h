/*
Copyright 2016 VMware, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _FRONTENDS_P4_SIDEEFFECTS_H_
#define _FRONTENDS_P4_SIDEEFFECTS_H_

/* makes explicit side effect ordering */

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/methodInstance.h"

namespace P4 {

/// Checks to see whether an IR node includes a table.apply() sub-expression
class HasTableApply : public Inspector {
    ReferenceMap* refMap;
    TypeMap*      typeMap;
 public:
    const IR::P4Table*  table;
    const IR::MethodCallExpression* call;
    HasTableApply(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap), table(nullptr), call(nullptr)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); setName("HasTableApply"); }

    void postorder(const IR::MethodCallExpression* expression) override {
        auto mi = MethodInstance::resolve(expression, refMap, typeMap);
        if (!mi->isApply()) return;
        auto am = mi->to<P4::ApplyMethod>();
        if (!am->object->is<IR::P4Table>()) return;
        BUG_CHECK(table == nullptr, "%1% and %2%: multiple table applications in one expression",
                  table, am->object);
        table = am->object->to<IR::P4Table>();
        call = expression;
        LOG3("Invoked table is " << dbp(table));
    }
};

/** @brief Determines whether an expression may have method or constructor
 * invocations.
 *
 * The TypeMap and ReferenceMap arguments may be null, in which case every
 * method call expression is counted.  With type information, invocations
 * of ```isValid()``` are ignored.
 */
class SideEffects : public Inspector {
 private:
    ReferenceMap* refMap;
    TypeMap*      typeMap;

 public:
    /// Last visited side-effecting node.  Null if no node has side effects.
    const IR::Node* nodeWithSideEffect = nullptr;

    /// Number of side effects in this expression.
    unsigned sideEffectCount = 0;

    void postorder(const IR::MethodCallExpression* mce) override {
        if (refMap == nullptr || typeMap == nullptr) {
            // conservative
            sideEffectCount++;
            nodeWithSideEffect = mce;
            return;
        }
        auto mi = MethodInstance::resolve(mce, refMap, typeMap);
        if (!mi->is<BuiltInMethod>()) {
            sideEffectCount++;
            nodeWithSideEffect = mce;
            return;
        }
        auto bim = mi->to<BuiltInMethod>();
        if (bim->name.name != IR::Type_Header::isValid) {
            sideEffectCount++;
            nodeWithSideEffect = mce;
        }
    }

    void postorder(const IR::ConstructorCallExpression* cce) override {
        sideEffectCount++;
        nodeWithSideEffect = cce;
    }

    /// The @refMap and @typeMap arguments can be null, in which case the check
    /// will be more conservative.
    SideEffects(ReferenceMap* refMap, TypeMap* typeMap) :
            refMap(refMap), typeMap(typeMap) { setName("SideEffects"); }

    /// @return true if the expression may have side-effects.
    static bool check(const IR::Expression* expression, const Visitor* calledBy,
                      ReferenceMap* refMap, TypeMap* typeMap) {
        SideEffects se(refMap, typeMap);
        se.setCalledBy(calledBy);
        expression->apply(se);
        return se.nodeWithSideEffect != nullptr;
    }
    /// @return true if the method call expression may have side-effects.
    static bool hasSideEffect(const IR::MethodCallExpression* mce,
                              ReferenceMap* refMap,
                              TypeMap* typeMap) {
        // mce does not produce a side effect in few cases:
        //  * isValid()
        //  * function, extern function, or extern method with noSideEffects annotation
        auto mi = MethodInstance::resolve(mce, refMap, typeMap);
        if (auto em = mi->to<P4::ExternMethod>()) {
            if (em->method->getAnnotation(IR::Annotation::noSideEffectsAnnotation)) {
                return false;
            }
            return true;
        }
        if (auto ef = mi->to<P4::ExternFunction>()) {
            if (ef->method->getAnnotation(IR::Annotation::noSideEffectsAnnotation)) {
                return false;
            }
            return true;
        }
        if (auto ef = mi->to<P4::FunctionCall>()) {
            if (ef->function->getAnnotation(IR::Annotation::noSideEffectsAnnotation)) {
                return false;
            }
            return true;
        }
        if (auto bim = mi->to<BuiltInMethod>()) {
            if (bim->name.name == IR::Type_Header::isValid) {
                return false;
            }
        }
        return true;
    }
};

/** @brief Convert expressions so that each expression contains at most one
 * side effect.
 *
 * Left-values are converted to contain no side-effects.  An important consequence
 * of this pass is that it converts function calls so that no two parameters
 * (one of which is output) can alias.  This makes the job of the inliner simpler.
 * For example:
 * - The expression ```a[f(x)] = b;``` is converted to ```tmp = f(x); a[tmp] = b;```
 * - And ```m(n())``` is converted to ```tmp = n(); m(tmp)```
 *
 * This is especially tricky for handling out and inout arguments.  Consider
 * two function prototypes, ```bit f(inout T a, in T b); T g(inout T c);```.
 * The expression ```a[g(w)].x = f(a[1].x, g(a[1].x));``` is translated as
 *
```
T tmp1 = g(w);           // modifies w
T tmp2 = a[1].x;         // save a[1].x
T tmp3 = g(a[1].x);      // modifies a[1].x
T tmp4 = f(tmp2, tmp3);  // modifies tmp2
a[1].x = tmp2;           // copy tmp2 to a[1].x - out argument
a[tmp1].x = tmp4;        // assign result of call of f to actual left value
```
 *
 * Translating function calls proceeds as follows:
 * - arguments are evaluated in order
 * - inout and out arguments produce left-values---these are "saved"
 * - function is called with temporaries for all arguments
 * - out and inout temporaries are copied to the saved left-values in order
 *
 * For assignment statements ```e = e1;``` the left hand side is evaluated
 * first.
 */
class DoSimplifyExpressions : public Transform, P4WriteContext {
    ReferenceMap*        refMap;
    TypeMap*             typeMap;
    // Expressions holding temporaries that are already added.
    std::set<const IR::Expression*>* added;

    IR::IndexedVector<IR::Declaration> toInsert;  // temporaries
    IR::IndexedVector<IR::StatOrDecl> statements;
    /// Set of temporaries introduced for method call results during
    /// this pass.
    std::set<const IR::Expression*> temporaries;

    cstring createTemporary(const IR::Type* type);
    const IR::Expression* addAssignment(Util::SourceInfo srcInfo, cstring varName,
                                        const IR::Expression* expression);
    bool mayAlias(const IR::Expression* left, const IR::Expression* right) const;
    bool containsHeaderType(const IR::Type* type);

 public:
    DoSimplifyExpressions(ReferenceMap* refMap, TypeMap* typeMap,
                          std::set<const IR::Expression*>* added)
            : refMap(refMap), typeMap(typeMap), added(added) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        setName("DoSimplifyExpressions");
    }

    const IR::Node* postorder(IR::Expression* expression) override;
    const IR::Node* preorder(IR::StructExpression * expression) override;
    const IR::Node* preorder(IR::ListExpression * expression) override;
    const IR::Node* preorder(IR::Literal* expression) override;
    const IR::Node* preorder(IR::ArrayIndex* expression) override;
    const IR::Node* preorder(IR::Member* expression) override;
    const IR::Node* preorder(IR::SelectExpression* expression) override;
    const IR::Node* preorder(IR::Operation_Unary* expression) override;
    const IR::Node* preorder(IR::Operation_Binary* expression) override;
    const IR::Node* shortCircuit(IR::Operation_Binary* expression);
    const IR::Node* preorder(IR::Mux* expression) override;
    const IR::Node* preorder(IR::LAnd* expression) override;
    const IR::Node* preorder(IR::LOr* expression) override;
    const IR::Node* preorder(IR::MethodCallExpression* mce) override;

    const IR::Node* preorder(IR::ConstructorCallExpression* cce) override { prune(); return cce; }
    const IR::Node* preorder(IR::Property* prop) override { prune(); return prop; }
    const IR::Node* preorder(IR::Annotation* anno) override { prune(); return anno; }

    const IR::Node* postorder(IR::P4Parser* parser) override;
    const IR::Node* postorder(IR::Function* function) override;
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::P4Action* action) override;
    const IR::Node* postorder(IR::ParserState* state) override;
    const IR::Node* postorder(IR::AssignmentStatement* statement) override;
    const IR::Node* postorder(IR::MethodCallStatement* statement) override;
    const IR::Node* postorder(IR::ReturnStatement* statement) override;
    const IR::Node* preorder(IR::SwitchStatement* statement) override;
    const IR::Node* preorder(IR::IfStatement* statement) override;

    void end_apply(const IR::Node *) override;
};

class TableInsertions {
 public:
    std::vector<const IR::Declaration_Variable*> declarations;
    std::vector<const IR::AssignmentStatement*> statements;
};

/**
 * This pass is an adaptation of the midend code SimplifyKey.
 * If a key computation involves side effects then all key field
 * computations are lifted prior to the table application.
 * We need to lift all key field computations since the order
 * of side-effects needs to be preserved.
 *
 * \code{.cpp}
 *  table t {
 *    key = { a: exact,
 *            f() : exact; }
 *    ...
 *  }
 *  apply {
 *    t.apply();
 *  }
 * \endcode
 *
 * is transformed to
 *
 * \code{.cpp}
 *  bit<32> key_0;
 *  bit<32> key_1;
 *  table t {
 *    key = { key_0 : exact;
 *            key_1 : exact; }
 *    ...
 *  }
 *  apply {
 *    key_0 = a;
 *    key_1 = f();
 *    t.apply();
 *  }
 * \endcode
 *
 * @pre none
 * @post all complex table key expressions are replaced with a simpler expression.
 */
class KeySideEffect : public Transform {
 protected:
    ReferenceMap* refMap;
    TypeMap*      typeMap;
    std::map<const IR::P4Table*, TableInsertions*> toInsert;
    std::set<const IR::P4Table*>* invokedInKey;

 public:
    KeySideEffect(ReferenceMap* refMap, TypeMap* typeMap,
                  std::set<const IR::P4Table*>* invokedInKey)
            : refMap(refMap), typeMap(typeMap), invokedInKey(invokedInKey)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(invokedInKey); setName("KeySideEffect"); }
    virtual const IR::Node* doStatement(const IR::Statement* statement,
                                        const IR::Expression* expression);

    const IR::Node* preorder(IR::Key* key) override;

    // These should be all kinds of statements that may contain a table apply
    // after the program has been simplified
    const IR::Node* postorder(IR::MethodCallStatement* statement) override
    { return doStatement(statement, statement->methodCall); }
    const IR::Node* postorder(IR::IfStatement* statement) override
    { return doStatement(statement, statement->condition); }
    const IR::Node* postorder(IR::SwitchStatement* statement) override
    { return doStatement(statement, statement->expression); }
    const IR::Node* postorder(IR::AssignmentStatement* statement) override
    { return doStatement(statement, statement->right); }
    const IR::Node* postorder(IR::KeyElement* element) override;
    const IR::Node* postorder(IR::P4Table* table) override;
    const IR::Node* preorder(IR::P4Table* table) override;
};

/// Discovers tables that are invoked within key computations
/// for other tables: e.g.
/// table Y { ... }
/// table X { key = { ... Y.apply.hit() ... } }
/// This inserts Y into the map invokedInKey;
class TablesInKeys : public Inspector {
    ReferenceMap* refMap;
    TypeMap*      typeMap;
    std::set<const IR::P4Table*>* invokedInKey;

 public:
    TablesInKeys(ReferenceMap* refMap, TypeMap* typeMap, std::set<const IR::P4Table*>* invokedInKey)
            : refMap(refMap), typeMap(typeMap), invokedInKey(invokedInKey)
    { CHECK_NULL(invokedInKey); setName("TableInKeys"); }
    Visitor::profile_t init_apply(const IR::Node* node) override {
        invokedInKey->clear();
        return Inspector::init_apply(node);
    }
    void postorder(const IR::MethodCallExpression* mce) override {
        if (!findContext<IR::Key>())
            return;
        HasTableApply hta(refMap, typeMap);
        hta.setCalledBy(this);
        (void)mce->apply(hta);
        if (hta.table != nullptr) {
            LOG2("Table " << hta.table << " invoked in key of another table");
            invokedInKey->emplace(hta.table);
        }
    }
};

/// Discovers action arguments that may involve table applications.
/// These idioms are currently not supported:
/// table s { ... }
/// table t {
///    actions { a(s.apply().hit ? ... ); }
class TablesInActions : public Inspector {
    ReferenceMap* refMap;
    TypeMap*      typeMap;

 public:
    TablesInActions(ReferenceMap* refMap, TypeMap* typeMap)
            : refMap(refMap), typeMap(typeMap) {}
    void postorder(const IR::MethodCallExpression* expression) override {
        if (findContext<IR::ActionList>()) {
            HasTableApply hta(refMap, typeMap);
            hta.setCalledBy(this);
            (void)expression->apply(hta);
            if (hta.table != nullptr) {
                ::error(ErrorType::ERR_UNSUPPORTED,
                        "%1%: table invocation in action argument", expression);
            }
        }
    }
};

class SideEffectOrdering : public PassRepeated {
    // Contains all tables that are invoked within key
    // computations for other tables.  The keys for these
    // inner tables cannot be expanded until the keys of
    // the caller tables have been expanded.
    std::set<const IR::P4Table*> invokedInKey;
    // Temporaries that were added
    std::set<const IR::Expression*> added;

 public:
    SideEffectOrdering(ReferenceMap* refMap, TypeMap* typeMap, bool skipSideEffectOrdering,
                       TypeChecking* typeChecking = nullptr) {
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        if (!skipSideEffectOrdering) {
            passes.push_back(new TypeChecking(refMap, typeMap));
            passes.push_back(new DoSimplifyExpressions(refMap, typeMap, &added));
            passes.push_back(typeChecking);
            passes.push_back(new TablesInActions(refMap, typeMap));
            passes.push_back(new TablesInKeys(refMap, typeMap, &invokedInKey));
            passes.push_back(new KeySideEffect(refMap, typeMap, &invokedInKey));
        }
        setName("SideEffectOrdering");
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_SIDEEFFECTS_H_ */
