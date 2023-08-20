#include "eliminateTuples.h"

#include <string>

namespace P4 {

const IR::Type *ReplacementMap::convertType(const IR::Type *type) {
    auto it = replacement.find(type);
    if (it != replacement.end()) return it->second;
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
            return result;
        } else {
            return type;
        }
    } else if (auto bl = type->to<IR::Type_BaseList>()) {
        cstring name = ng->newName("tuple");
        IR::IndexedVector<IR::StructField> fields;
        size_t index = 0;
        for (auto t : bl->components) {
            auto ftype = convertType(t);
            auto fname = cstring("f") + cstring(std::to_string(index));
            auto field = new IR::StructField(IR::ID(fname), ftype->getP4Type());
            fields.push_back(field);
            index++;
        }
        auto result = new IR::Type_Struct(name, fields);
        LOG3("Converted " << dbp(type) << " to " << dbp(result));
        replacement.emplace(type, result);
        return result;
    }
    return type;
}

const IR::Type_Struct *ReplacementMap::getReplacement(const IR::Type_BaseList *tt) {
    auto st = convertType(tt)->to<IR::Type_Struct>();
    CHECK_NULL(st);
    replacement.emplace(tt, st);
    return st;
}

IR::IndexedVector<IR::Node> *ReplacementMap::getNewReplacements() {
    auto retval = new IR::IndexedVector<IR::Node>();
    for (auto t : replacement) {
        if (inserted.find(t.second) == inserted.end()) {
            retval->push_back(t.second);
            inserted.emplace(t.second);
        }
    }
    if (retval->empty()) return nullptr;
    return retval;
}

const IR::Node *DoReplaceTuples::postorder(IR::Type_BaseList *bl) {
    auto type = getOriginal<IR::Type_BaseList>();
    // If any of the arguments is a type variable leave the type unchanged
    for (auto arg : type->components) {
        auto targ = repl->typeMap->getTypeType(arg, true);
        if (targ->is<IR::Type_Var>()) return bl;
    }
    auto canon = repl->typeMap->getTypeType(type, true)->to<IR::Type_BaseList>();
    auto st = repl->getReplacement(canon)->getP4Type();
    return st;
}

const IR::Node *DoReplaceTuples::insertReplacements(const IR::Node *before) {
    // Check that we are in the top-level P4Program list of declarations.
    if (!getParent<IR::P4Program>()) return before;

    auto result = repl->getNewReplacements();
    if (result == nullptr) return before;
    LOG3("Inserting replacements before " << dbp(before));
    result->push_back(before);
    return result;
}

const IR::Node *DoReplaceTuples::postorder(IR::ArrayIndex *expression) {
    auto original = getOriginal<IR::ArrayIndex>();
    auto type = repl->typeMap->getType(original->left);
    if (type->is<IR::Type_Tuple>()) {
        auto cst = expression->right->to<IR::Constant>();
        BUG_CHECK(cst, "%1%: Expected a constant", expression->right);
        cstring field = cstring("f") + Util::toString(cst->asInt());
        auto src = expression->right->srcInfo;
        return new IR::Member(src, expression->left, IR::ID(src, field));
    }
    return expression;
}

}  // namespace P4
