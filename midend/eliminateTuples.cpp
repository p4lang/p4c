// SPDX-FileCopyrightText: 2016 VMware, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "eliminateTuples.h"

#include <string>
#include <vector>

namespace P4 {

using namespace literals;

namespace {

/// @returns true if @p type contains a type variable. Type parameter declarations do not count.
bool containsTypeVariable(const IR::Type *type) {
    struct FindTypeVariable : public Inspector {
        bool found = false;
        bool preorder(const IR::Type_Var *) override {
            found = true;
            return false;
        }
        bool preorder(const IR::TypeParameters *) override { return false; }
    } finder;
    type->apply(finder);
    return finder.found;
}

/// Collects the tuples that contain a type variable.
///
/// A type variable can only occur in a declaration that has type parameters, or in a method of an
/// extern. The visitor visits only these top-level declarations, so it does not visit the parsers
/// and controls of the program.
class FindGenericTuples : public Inspector {
    const TypeMap *typeMap;

    static bool mayContainTypeVariables(const IR::Node *declaration) {
        // A method of an extern can have type parameters of its own.
        if (declaration->is<IR::Type_Extern>()) return true;
        if (const auto *method = declaration->to<IR::Method>()) {
            return !method->type->typeParameters->empty();
        }
        if (const auto *function = declaration->to<IR::Function>()) {
            return !function->type->typeParameters->empty();
        }
        if (const auto *generic = declaration->to<IR::IMayBeGenericType>()) {
            return !generic->getTypeParameters()->empty();
        }
        return false;
    }

 public:
    std::vector<const IR::Type_BaseList *> tuples;

    explicit FindGenericTuples(const TypeMap *typeMap) : typeMap(typeMap) {
        setName("FindGenericTuples");
    }
    bool preorder(const IR::P4Program *program) override {
        for (const auto *declaration : program->objects) {
            if (mayContainTypeVariables(declaration)) visit(declaration);
        }
        return false;
    }
    bool preorder(const IR::Type_BaseList *tuple) override {
        for (const auto *component : tuple->components) {
            if (containsTypeVariable(typeMap->getTypeType(component, true))) {
                tuples.push_back(tuple);
                break;
            }
        }
        return true;
    }
    // DoReplaceTuples does not replace tuples in these nodes.
    bool preorder(const IR::P4ValueSet *) override { return false; }
    bool preorder(const IR::P4ListExpression *) override { return false; }
    bool preorder(const IR::Type_P4List *) override { return false; }
};

}  // namespace

void ReplacementMap::findGenericShapes(const IR::Node *program) {
    genericShapes.clear();
    FindGenericTuples findGenericTuples(typeMap);
    program->apply(findGenericTuples);
    for (const auto *tuple : findGenericTuples.tuples) addGenericShape(tuple);
}

void ReplacementMap::addGenericShape(const IR::Type_BaseList *tuple) {
    genericShapes.insert(shapeOf(tuple));
    for (const auto *component : tuple->components) {
        if (const auto *nested = canonical(component)->to<IR::Type_BaseList>()) {
            addGenericShape(nested);
        }
    }
}

cstring ReplacementMap::shapeOf(const IR::Type_BaseList *tuple) const {
    std::string shape = "(";
    const char *separator = "";
    for (const auto *component : tuple->components) {
        shape += separator;
        separator = ",";
        if (const auto *nested = canonical(component)->to<IR::Type_BaseList>()) {
            shape += shapeOf(nested).string_view();
        } else {
            shape += "_";
        }
    }
    return cstring(shape + ")");
}

void ReplacementMap::flattenComponents(const IR::Type_BaseList *tuple,
                                       std::vector<const IR::Type *> &components) const {
    for (const auto *component : tuple->components) {
        if (const auto *nested = canonical(component)->to<IR::Type_BaseList>()) {
            flattenComponents(nested, components);
        } else {
            components.push_back(component);
        }
    }
}

const IR::Type *ReplacementMap::canonical(const IR::Type *type) const {
    const auto *result = typeMap->getTypeType(type, false);
    return result != nullptr ? result : type;
}

const IR::Type_Struct *ReplacementMap::getGenericStruct(const IR::Type_BaseList *tuple,
                                                        cstring shape) {
    auto it = genericStructs.find(shape);
    if (it != genericStructs.end()) return it->second;
    IR::IndexedVector<IR::Type_Var> typeParameters;
    auto newTypeParameter = [&typeParameters]() {
        auto *typeParameter =
            new IR::Type_Var(IR::ID("T"_cs + Util::toString(typeParameters.size())));
        typeParameters.push_back(typeParameter);
        return new IR::Type_Name(typeParameter->name);
    };
    IR::IndexedVector<IR::StructField> fields;
    size_t index = 0;
    for (const auto *component : tuple->components) {
        const IR::Type *fieldType = nullptr;
        if (const auto *nested = canonical(component)->to<IR::Type_BaseList>()) {
            // The field specializes the generic struct of the nested shape with new type
            // parameters of this struct.
            const auto *nestedStruct = getGenericStruct(nested, shapeOf(nested));
            auto *arguments = new IR::Vector<IR::Type>();
            for (size_t i = 0; i < nestedStruct->typeParameters->size(); i++) {
                arguments->push_back(newTypeParameter());
            }
            fieldType = new IR::Type_Specialized(new IR::Type_Name(nestedStruct->name), arguments);
        } else {
            fieldType = newTypeParameter();
        }
        fields.push_back(new IR::StructField(IR::ID("f"_cs + Util::toString(index)), fieldType));
        index++;
    }
    auto *result = new IR::Type_Struct(IR::ID(ng.newName("tuple")),
                                       new IR::TypeParameters(typeParameters), fields);
    LOG3("Created " << dbp(result) << " for the tuples of shape " << shape);
    genericStructs.emplace(shape, result);
    declarations.push_back(result);
    return result;
}

const IR::Type_Specialized *ReplacementMap::specializeGenericStruct(const IR::Type_BaseList *tuple,
                                                                    cstring shape) {
    std::vector<const IR::Type *> components;
    flattenComponents(tuple, components);
    auto *arguments = new IR::Vector<IR::Type>();
    for (const auto *component : components) {
        arguments->push_back(convertType(component)->getP4Type());
    }
    const auto *generic = getGenericStruct(tuple, shape);
    return new IR::Type_Specialized(new IR::Type_Name(generic->name), arguments);
}

const IR::Type *ReplacementMap::convertTuple(const IR::Type_BaseList *tuple) {
    if (!genericShapes.empty()) {
        auto shape = shapeOf(tuple);
        if (genericShapes.count(shape) != 0) return specializeGenericStruct(tuple, shape);
    }
    cstring name = ng.newName("tuple");
    IR::IndexedVector<IR::StructField> fields;
    size_t index = 0;
    for (auto t : tuple->components) {
        auto ftype = convertType(t);
        auto fname = "f"_cs + Util::toString(index);
        auto field = new IR::StructField(IR::ID(fname), ftype->getP4Type());
        fields.push_back(field);
        index++;
    }
    auto *result = new IR::Type_Struct(name, fields);
    declarations.push_back(result);
    return result;
}

const IR::Type *ReplacementMap::convertType(const IR::Type *type) {
    auto it = replacement.find(type);
    if (it != replacement.end()) return it->second;
    if (auto canon = typeMap->getTypeType(type, false)) {
        if (auto it = replacement.find(canon); it != replacement.end()) {
            replacement.emplace(type, it->second);
            return it->second;
        }
    }
    if (auto st = type->to<IR::Type_Struct>()) {
        bool changes = false;
        IR::IndexedVector<IR::StructField> fields;
        for (auto f : st->fields) {
            auto ftype = typeMap->getType(f);
            auto cftype = convertType(ftype);
            if (cftype != ftype) changes = true;
            auto field = new IR::StructField(f->name, f->annotations, cftype->getP4Type());
            fields.push_back(field);
        }
        if (changes) {
            auto result = new IR::Type_Struct(st->srcInfo, st->name, st->annotations, fields);
            LOG3("Converted " << dbp(type) << " to " << dbp(result));
            replacement.emplace(type, result);
            declarations.push_back(result);
            return result;
        } else {
            return type;
        }
    } else if (auto bl = type->to<IR::Type_BaseList>()) {
        const auto *result = convertTuple(bl);
        LOG3("Converted " << dbp(type) << " to " << dbp(result));
        replacement.emplace(type, result);
        if (auto canon = typeMap->getTypeType(type, false)) replacement.emplace(canon, result);
        return result;
    }
    return type;
}

const IR::Type *ReplacementMap::getReplacement(const IR::Type_BaseList *tt) {
    const auto *result = convertType(tt);
    BUG_CHECK(result->is<IR::Type_Struct>() || result->is<IR::Type_Specialized>(),
              "%1%: the replacement %2% is neither a struct nor a specialized struct", tt, result);
    replacement.emplace(tt, result);
    return result;
}

IR::IndexedVector<IR::Node> *ReplacementMap::getNewReplacements() {
    auto retval = new IR::IndexedVector<IR::Node>();
    for (const auto *declaration : declarations) {
        if (inserted.find(declaration) == inserted.end()) {
            retval->push_back(declaration);
            inserted.emplace(declaration);
        }
    }
    if (retval->empty()) return nullptr;
    return retval;
}

Visitor::profile_t DoReplaceTuples::init_apply(const IR::Node *node) {
    auto rv = Transform::init_apply(node);
    node->apply(ng);
    repl.findGenericShapes(node);
    return rv;
}

const IR::Node *DoReplaceTuples::postorder(IR::Type_BaseList * /*bl*/) {
    auto type = getOriginal<IR::Type_BaseList>();
    return repl.getReplacement(type)->getP4Type();
}

const IR::Node *DoReplaceTuples::insertReplacements(const IR::Node *before) {
    // Check that we are in the top-level P4Program list of declarations.
    if (!getParent<IR::P4Program>()) return before;

    auto result = repl.getNewReplacements();
    if (result == nullptr) return before;
    LOG3("Inserting replacements before " << dbp(before));
    result->push_back(before);
    return result;
}

const IR::Node *DoReplaceTuples::postorder(IR::ArrayIndex *expression) {
    auto original = getOriginal<IR::ArrayIndex>();
    auto type = repl.typeMap->getType(original->left);
    if (type->is<IR::Type_Tuple>()) {
        auto cst = expression->right->to<IR::Constant>();
        BUG_CHECK(cst, "%1%: Expected a constant", expression->right);
        cstring field = absl::StrCat("f", cst->asInt());
        auto src = expression->right->srcInfo;
        return new IR::Member(src, expression->left, IR::ID(src, field));
    }
    return expression;
}

}  // namespace P4
