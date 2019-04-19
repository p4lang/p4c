#include "nestedStructs.h"

namespace P4 {

bool ComplexValues::isNestedStruct(const IR::Type* type) {
    if (!type->is<IR::Type_Struct>())
        return false;
    auto st = type->to<IR::Type_Struct>();
    for (auto f : st->fields) {
        auto ftype = typeMap->getType(f, true);
        if (ftype->is<IR::Type_StructLike>() ||
            ftype->is<IR::Type_Tuple>() ||
            ftype->is<IR::Type_Stack>()) {
            LOG3("Type " << dbp(type) << " is nested");
            return true;
        }
    }
    return false;
}

void ComplexValues::explode(cstring prefix, const IR::Type_Struct* type,
                            FieldsMap* map, IR::Vector<IR::Declaration>* result) {
    CHECK_NULL(type);
    for (auto f : type->fields) {
        cstring fname = prefix + "_" + f->name;
        auto ftype = typeMap->getType(f, true);
        if (isNestedStruct(ftype)) {
            auto submap = new FieldsMap();
            map->members.emplace(f->name.name, submap);
            explode(fname, ftype->to<IR::Type_Struct>(), submap, result);
        } else {
            cstring newName = refMap->newName(fname);
            auto comp = new FinalName(newName);
            map->members.emplace(f->name.name, comp);
            auto clone = new IR::Declaration_Variable(IR::ID(newName), ftype->getP4Type());
            LOG3("Created " << clone);
            result->push_back(clone);
        }
    }
}

const IR::Node* RemoveNestedStructs::postorder(IR::Declaration_Variable* decl) {
    auto type = values->typeMap->getType(getOriginal(), true);
    if (!values->isNestedStruct(type))
        return decl;

    BUG_CHECK(decl->initializer == nullptr, "%1%: did not expect an initializer", decl);
    BUG_CHECK(decl->annotations->size() == 0,
              "%1%: don't know how to handle variable annotations", decl);
    auto result = new IR::Vector<IR::Declaration>();
    auto map = new ComplexValues::FieldsMap();
    values->values.emplace(getOriginal<IR::Declaration_Variable>(), map);
    values->explode(decl->getName().name, type->to<IR::Type_Struct>(), map, result);
    return result;
}

const IR::Node* RemoveNestedStructs::postorder(IR::Member* expression) {
    LOG3("Visiting " << dbp(getOriginal()));
    auto parent = getContext()->node;
    auto left = values->getTranslation(expression->expr);
    if (left == nullptr)
        return expression;
    auto comp = left->getComponent(expression->member.name);
    if (comp == nullptr) {
        auto l = left->convertToExpression();
        auto e = new IR::Member(expression->srcInfo, l, expression->member);
        return e;
    }
    values->setTranslation(getOriginal<IR::Member>(), comp);
    if (parent->is<IR::Member>())
        // Translation done by parent (if necessary)
        return expression;
    auto e = comp->convertToExpression();
    return e;
}

const IR::Node* RemoveNestedStructs::postorder(IR::PathExpression* expression) {
    LOG3("Visiting " << dbp(getOriginal()));
    auto decl = values->refMap->getDeclaration(expression->path, true);
    auto comp = values->getTranslation(decl);
    if (comp == nullptr)
        return expression;
    values->setTranslation(getOriginal<IR::PathExpression>(), comp);
    auto parent = getContext()->node;
    if (parent->is<IR::Member>())
        // translation done by parent
        return expression;
    auto list = comp->convertToExpression();
    return list;
}

}  // namespace P4
