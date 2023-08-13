/*
Copyright 2020 VMware, Inc.

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

#include "specializeGenericTypes.h"

#include <stddef.h>

#include "frontends/p4/typeChecking/typeSubstitution.h"
#include "frontends/p4/typeChecking/typeSubstitutionVisitor.h"
#include "ir/id.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"

namespace P4 {

bool TypeSpecializationMap::same(const TypeSpecialization *spec,
                                 const IR::Type_Specialized *right) const {
    if (!spec->specialized->baseType->equiv(*right->baseType)) return false;
    BUG_CHECK(spec->argumentTypes->size() == right->arguments->size(),
              "Type %1% and %2% specialized with different number of arguments?", spec->specialized,
              right);
    for (size_t i = 0; i < spec->argumentTypes->size(); i++) {
        auto argl = spec->argumentTypes->at(i);
        auto argr = typeMap->getType(right->arguments->at(i), true);
        if (!typeMap->equivalent(argl, argr)) return false;
    }
    return true;
}

void TypeSpecializationMap::add(const IR::Type_Specialized *t, const IR::Type_StructLike *decl,
                                const IR::Node *insertion) {
    auto it = map.find(t);
    if (it != map.end()) return;

    // First check if we have another specialization with the same
    // type arguments, in that case reuse it
    for (auto it : map) {
        if (same(it.second, t)) {
            map.emplace(t, it.second);
            LOG3("Found to specialize: " << t << " as previous " << it.second->name);
            return;
        }
    }

    cstring name = refMap->newName(decl->getName());
    LOG3("Found to specialize: " << dbp(t) << "(" << t << ") with name " << name
                                 << " insert before " << dbp(insertion));
    auto argTypes = new IR::Vector<IR::Type>();
    for (auto a : *t->arguments) argTypes->push_back(typeMap->getType(a, true));
    TypeSpecialization *s = new TypeSpecialization(name, t, decl, insertion, argTypes);
    map.emplace(t, s);
}

TypeSpecialization *TypeSpecializationMap::get(const IR::Type_Specialized *type) const {
    for (auto it : map) {
        if (same(it.second, type)) return it.second;
    }
    return nullptr;
}

namespace {

class ContainsTypeVariable : public Inspector {
    bool contains = false;

 public:
    bool preorder(const IR::TypeParameters *) override { return false; }
    bool preorder(const IR::Type_Var *) override {
        contains = true;
        return false;
    }
    bool preorder(const IR::Type_Specialized *) override {
        contains = true;
        return false;
    }
    static bool inspect(const IR::Node *node) {
        ContainsTypeVariable ctv;
        node->apply(ctv);
        return ctv.contains;
    }
};

}  // namespace

void FindTypeSpecializations::postorder(const IR::Type_Specialized *type) {
    auto baseType = specMap->typeMap->getTypeType(type->baseType, true);
    auto st = baseType->to<IR::Type_StructLike>();
    if (st == nullptr || st->typeParameters->size() == 0)
        // nothing to specialize
        return;
    for (auto tp : *type->arguments) {
        auto argType = specMap->typeMap->getTypeType(tp, true);
        if (ContainsTypeVariable::inspect(argType))
            // If type argument contains a type variable, for example, in
            // the field f:
            // struct G<T> { T field; }
            // struct S<T> { G<T> f; }
            // then we won't specialize this now, but only when encountered in
            // specialized instances of G, e.g., G<bit<32>>.
            return;
    }
    // Find location where the specialization is to be inserted.
    // This can be before a Parser, Control, or a toplevel instance declaration
    const IR::Node *insert = findContext<IR::P4Parser>();
    if (!insert) insert = findContext<IR::Function>();
    if (!insert) insert = findContext<IR::P4Control>();
    if (!insert) insert = findContext<IR::Type_Declaration>();
    if (!insert) insert = findContext<IR::Declaration_Constant>();
    if (!insert) insert = findContext<IR::Declaration_Variable>();
    if (!insert) insert = findContext<IR::Declaration_Instance>();
    if (!insert) insert = findContext<IR::P4Action>();
    CHECK_NULL(insert);
    specMap->add(type, st, insert);
}

///////////////////////////////////////////////////////////////////////////////////////

const IR::Node *CreateSpecializedTypes::postorder(IR::Type_Declaration *type) {
    for (auto it : specMap->map) {
        if (it.second->declaration->name == type->name) {
            auto specialized = it.first;
            auto genDecl = type->to<IR::IMayBeGenericType>();
            TypeVariableSubstitution ts;
            ts.setBindings(type, genDecl->getTypeParameters(), specialized->arguments);
            TypeSubstitutionVisitor tsv(specMap->typeMap, &ts);
            tsv.setCalledBy(this);
            auto renamed = type->apply(tsv)->to<IR::Type_StructLike>()->clone();
            cstring name = it.second->name;
            auto empty = new IR::TypeParameters();
            renamed->name = name;
            renamed->typeParameters = empty;
            it.second->replacement = postorder(renamed)->to<IR::Type_StructLike>();
            LOG3("CST Specializing " << dbp(type) << " with " << ts << " as " << dbp(renamed));
        }
    }
    return insert(type);
}

const IR::Node *CreateSpecializedTypes::insert(const IR::Node *before) {
    auto specs = specMap->getSpecializations(getOriginal());
    if (specs == nullptr) return before;
    LOG2(specs->size() << " instantiations before " << dbp(before));
    specs->push_back(before);
    return specs;
}

const IR::Node *ReplaceTypeUses::postorder(IR::Type_Specialized *type) {
    auto t = specMap->get(getOriginal<IR::Type_Specialized>());
    if (!t) return type;
    CHECK_NULL(t->replacement);
    LOG3("RTU Replacing " << dbp(type) << " with " << dbp(t->replacement));
    return t->replacement->getP4Type();
}

const IR::Node *ReplaceTypeUses::postorder(IR::StructExpression *expression) {
    auto st = getOriginal<IR::StructExpression>()->structType;
    if (!st) {
        ::error(ErrorType::ERR_TYPE_ERROR,
                "%1%: could not infer a type for expression; "
                "please specify it explicitly",
                expression);
        return expression;
    }
    auto spec = st->to<IR::Type_Specialized>();
    if (spec == nullptr) return expression;
    auto replacement = specMap->get(spec);
    if (replacement == nullptr) return expression;
    auto replType = replacement->replacement;
    LOG3("RTU Replacing " << dbp(expression->structType) << " with "
                          << dbp(replacement->replacement));
    expression->structType = replType->getP4Type();
    return expression;
}

}  // namespace P4
