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

#include "absl/strings/str_replace.h"
#include "frontends/p4/typeChecking/typeSubstitutionVisitor.h"

namespace P4 {

const IR::Type_Declaration *TypeSpecializationMap::nextAvailable() {
    for (auto &[_, specialization] : map) {
        if (specialization.inserted) continue;
        auto &insReq = specialization.insertion;
        if (insReq.empty()) {
            specialization.inserted = true;
            return specialization.replacement;
        }
    }
    return nullptr;
}

void TypeSpecializationMap::markDefined(const IR::Type_Declaration *tdec) {
    const auto name = tdec->name.name;
    for (auto &[_, specialization] : map) {
        if (specialization.inserted) continue;
        specialization.insertion.erase(name);
    }
}

void TypeSpecializationMap::fillInsertionSet(const IR::Type_StructLike *decl,
                                             InsertionSet &insertion) {
    auto handleName = [&](cstring name) {
        LOG4("TSM: " << decl->toString() << " to be inserted after " << name);
        insertion.insert(name);
    };
    // - not using type map, the struct type could be a fresh one, and even if it is not, struct
    //   fields will always have known type (unlike e.g. expressions)
    // - using visitor to handle type names and type specialization inside types like `list` and
    //   `tuple`.
    forAllMatching<IR::Type_Name>(
        decl, [&](const IR::Type_Name *tn) { handleName(tn->path->name.name); });
    forAllMatching<IR::Type_Specialized>(decl, [&](const IR::Type_Specialized *ts) {
        if (const auto *specialization = get(ts)) {
            handleName(specialization->name);
        }
    });
}

void TypeSpecializationMap::add(const IR::Type_Specialized *t, const IR::Type_StructLike *decl,
                                NameGenerator *nameGen) {
    const auto sig = SpecSignature::get(t);
    if (!sig) {
        return;
    }
    if (map.count(*sig)) return;

    cstring name = nameGen->newName(sig->name());
    LOG3("Found to specialize: " << dbp(t) << " with name " << name << " insert after "
                                 << dbp(decl));
    map.emplace(*sig, TypeSpecialization{name, t, decl, {decl->name.name}, t->arguments});
}

namespace {

// depending on constness of Map returns a const or non-const pointer
template <typename Map>
auto _get(Map &map, const IR::Type_Specialized *type)
    -> std::conditional_t<std::is_const_v<Map>, const TypeSpecialization *, TypeSpecialization *> {
    if (const auto sig = SpecSignature::get(type)) {
        return getref(map, *sig);
    }
    return nullptr;
}
}  // namespace

const TypeSpecialization *TypeSpecializationMap::get(const IR::Type_Specialized *type) const {
    return _get(map, type);
}

TypeSpecialization *TypeSpecializationMap::get(const IR::Type_Specialized *type) {
    return _get(map, type);
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

Visitor::profile_t FindTypeSpecializations::init_apply(const IR::Node *node) {
    auto rv = Inspector::init_apply(node);
    node->apply(nameGen);
    return rv;
}

void FindTypeSpecializations::postorder(const IR::Type_Specialized *type) {
    const auto *baseType = getDeclaration(type->baseType->path, true);
    const auto *st = baseType->to<IR::Type_StructLike>();
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
    specMap->add(type, st, &nameGen);
}

///////////////////////////////////////////////////////////////////////////////////////

void CreateSpecializedTypes::postorder(IR::Type_Specialized *spec) {
    if (auto *specialization = specMap->get(spec)) {
        const auto *declT = getDeclaration(spec->baseType->path)->to<IR::Type_Declaration>();
        BUG_CHECK(declT, "Could not get declaration for %1%", spec);
        auto genDecl = declT->to<IR::IMayBeGenericType>();
        BUG_CHECK(genDecl, "Not a generic declaration: %1%", declT);
        TypeVariableSubstitution ts;
        ts.setBindings(declT, genDecl->getTypeParameters(), specialization->argumentTypes);
        TypeSubstitutionVisitor tsv(specMap->typeMap, &ts);
        tsv.setCalledBy(this);
        auto renamed = declT->apply(tsv)->to<IR::Type_StructLike>()->clone();
        cstring name = specialization->name;
        renamed->name = name;
        renamed->typeParameters = new IR::TypeParameters();
        specialization->replacement = renamed;
        // add additional insertion constraints
        specMap->fillInsertionSet(renamed, specialization->insertion);
        LOG3("CST: Specializing " << dbp(declT) << " with [" << ts << "] as " << dbp(renamed));
    }
}

void CreateSpecializedTypes::postorder(IR::P4Program *prog) {
    IR::Vector<IR::Node> newObjects;
    for (const auto *obj : prog->objects) {
        newObjects.push_back(obj);
        if (const auto *tdec = obj->to<IR::Type_Declaration>()) {
            specMap->markDefined(tdec);
            while (const auto *addTDec = specMap->nextAvailable()) {
                newObjects.push_back(addTDec);
                specMap->markDefined(addTDec);
                LOG2("CST: Will insert " << dbp(addTDec) << " after " << dbp(newObjects.back()));
            }
        }
    }
    prog->objects = newObjects;
}

const IR::Node *ReplaceTypeUses::postorder(IR::Type_Specialized *type) {
    auto t = specMap->get(type);
    if (!t) return type;
    BUG_CHECK(t->replacement, "Missing replacement %1% -> %2%", type, t->name);
    LOG3("RTU Replacing " << dbp(type) << " with " << dbp(t->replacement));
    return t->replacement->getP4Type();
}

const IR::Node *ReplaceTypeUses::postorder(IR::StructExpression *expression) {
    const IR::Annotation *anNode = findContext<IR::Annotation>();
    if (anNode != nullptr && !anNode->structured) return expression;

    auto st = getOriginal<IR::StructExpression>()->structType;
    if (!st) {
        ::P4::error(ErrorType::ERR_TYPE_ERROR,
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

std::string SpecSignature::name() const {
    std::stringstream ss;
    ss << baseType;
    for (const auto &t : arguments) {
        ss << "_"
           << absl::StrReplaceAll(t, {{"<", ""}, {">", ""}, {" ", ""}, {",", "_"}, {".", "_"}});
    }
    return ss.str();
}

std::string toString(const SpecSignature &sig) {
    return absl::StrCat(sig.baseType, "<", absl::StrJoin(sig.arguments, ","), ">");
}

std::optional<SpecSignature> SpecSignature::get(const IR::Type_Specialized *spec) {
    SpecSignature out;
    out.baseType = spec->baseType->path->name;
    for (const auto *arg : *spec->arguments) {
        if (ContainsTypeVariable::inspect(arg)) {
            return {};
        }
        out.arguments.push_back(arg->toString());
    }
    return out;
}

}  // namespace P4
