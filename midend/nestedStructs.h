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

#ifndef _MIDEND_NESTEDSTRUCTS_H_
#define _MIDEND_NESTEDSTRUCTS_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

class ComplexValues final {
 public:
    /**
     * Represents a field or a collection of fields of a value that
     * has a struct type.
     */
    struct Component : public IHasDbPrint {
        virtual const IR::Expression* convertToExpression() = 0;
        virtual Component* getComponent(cstring name) = 0;
        virtual void dbprint(std::ostream& out) const = 0;
    };

    struct FinalName : public Component {
        cstring newName;
        explicit FinalName(cstring name) : newName(name) {}
        const IR::Expression* convertToExpression() override
        { return new IR::PathExpression(IR::ID(newName)); }
        Component* getComponent(cstring) override
        { return nullptr; }
        void dbprint(std::ostream& out) const override
        { out << newName << IndentCtl::endl; }
    };

    struct FieldsMap : public Component {
        ordered_map<cstring, Component*> members;
        FieldsMap() = default;
        const IR::Expression* convertToExpression() override {
            auto vec = new IR::ListExpression({});
            for (auto m : members) {
                auto e = m.second->convertToExpression();
                vec->push_back(e);
            }
            return vec;
        }
        Component* getComponent(cstring name) override
        { return ::get(members, name); }
        void dbprint(std::ostream& out) const override {
            out << IndentCtl::indent;
            for (auto m : members)
                out << m.first << "=>" << m.second;
            out << IndentCtl::unindent;
        }
    };

    std::map<const IR::Declaration_Variable*, Component*> values;
    std::map<const IR::Expression*, Component*> translation;

    ReferenceMap* refMap;
    TypeMap* typeMap;

    ComplexValues(ReferenceMap* refMap, TypeMap* typeMap)  : refMap(refMap), typeMap(typeMap)
    { CHECK_NULL(refMap); CHECK_NULL(typeMap); }
    /// Helper function that test if a struct is nested
    bool isNestedStruct(const IR::Type* type);
    /// Flatten a nested struct to only contain field declaration or non-nested struct
    void explode(cstring prefix, const IR::Type_Struct* type,
                 FieldsMap* map, IR::Vector<IR::Declaration>* result);
    Component* getTranslation(const IR::IDeclaration* decl) {
        auto dv = decl->to<IR::Declaration_Variable>();
        if (dv == nullptr)
            return nullptr;
        return ::get(values, dv);
    }
    Component* getTranslation(const IR::Expression* expression)
    {  LOG2("Check translation " << dbp(expression)); return ::get(translation, expression); }
    void setTranslation(const IR::Expression* expression, Component* comp) {
        translation.emplace(expression, comp);
        LOG2("Translated " << dbp(expression) << " to " << comp); }
};

/**
 * Implements a pass that removes nested structs.
 *
 * Specifically, it converts only local variables, but does not change
 * the arguments of controls, parsers, packages, and methods.
 *
 * Example:
 *  struct T { bit b; }
 *  struct S { T t1; T t2; }
 *  S v;
 *  f(v.t1, v);
 *
 *  is replaced by
 *
 *  T v_t1;
 *  T v_t2;
 *  f(v_t1, { v_t1, v_t2 });
 *
 *  This does not work if the second argument of f is out or inout,
 *  since the list expression is not a l-value.  This pass cannot be
 *  used in this case.  This can arise only if there are extern functions
 *  that can return nested structs.
 *
 *  @pre: This pass should be run after CopyStructures, EliminateTuples, and
 *        MoveInitializers.
 *
 *  @post: Ensure that
 *    - all variables whose types are nested structs are flattened.
 */
class RemoveNestedStructs final : public Transform {
    ComplexValues* values;
 public:
    explicit RemoveNestedStructs(ComplexValues* values) : values(values)
    { CHECK_NULL(values); setName("RemoveNestedStructs"); }

    /// rewrite nested structs to non-nested structs
    const IR::Node* postorder(IR::Declaration_Variable* decl) override;
    /// replace reference to nested structs with the corresponding non-nested version
    const IR::Node* postorder(IR::Member* expression) override;
    /// replace reference to nested structs with the corresponding non-nested version
    const IR::Node* postorder(IR::PathExpression* expression) override;
};

class NestedStructs final : public PassManager {
 public:
    NestedStructs(ReferenceMap* refMap, TypeMap* typeMap,
            TypeChecking* typeChecking = nullptr) {
        auto values = new ComplexValues(refMap, typeMap);
        if (!typeChecking)
            typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new RemoveNestedStructs(values));
        passes.push_back(new ClearTypeMap(typeMap));
        setName("NestedStructs");
    }
};

}  // namespace P4

#endif /* _MIDEND_NESTEDSTRUCTS_H_ */
