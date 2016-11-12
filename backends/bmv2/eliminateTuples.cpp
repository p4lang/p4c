#include "eliminateTuples.h"

namespace BMV2 {

const IR::Type* ReplacementMap::convertType(const IR::Type* type) {
    if (type->is<IR::Type_StructLike>()) {
        auto st = type->to<IR::Type_StructLike>();
        bool changes = false;
        auto fields = new IR::IndexedVector<IR::StructField>();
        for (auto f : *st->fields) {
            auto ftype = typeMap->getType(f);
            auto cftype = convertType(ftype);
            if (cftype != ftype)
                changes = true;
            auto field = new IR::StructField(Util::SourceInfo(), f->name, f->annotations, cftype->getP4Type());
            fields->push_back(field);
        }
        if (changes)
            return new IR::Type_Struct(st->srcInfo, st->name, st->annotations, fields);
        else
            return type;
    } else if (type->is<IR::Type_Tuple>()) {
        cstring name = ng->newName("tuple");
        auto fields = new IR::IndexedVector<IR::StructField>();
        for (auto t : *type->to<IR::Type_Tuple>()->components) {
            auto ftype = convertType(t);
            auto fname = ng->newName("field");
            auto field = new IR::StructField(Util::SourceInfo(), IR::ID(fname), IR::Annotations::empty, ftype->getP4Type());
            fields->push_back(field);
        }
        auto result = new IR::Type_Struct(Util::SourceInfo(), name, IR::Annotations::empty, fields);
        return result;
    } else {
        return type;
    }
}

const IR::Type_Struct* ReplacementMap::getReplacement(const IR::Type_Tuple* tt) {
    auto it = replacement.find(tt);
    if (it != replacement.end())
        return it->second;
    auto st = convertType(tt)->to<IR::Type_Struct>();
    CHECK_NULL(st);
    replacement.emplace(tt, st);
    return st;
}

IR::IndexedVector<IR::Node>* ReplacementMap::getAllReplacements() {
    if (replacement.empty())
        return nullptr;
    auto retval = new IR::IndexedVector<IR::Node>();
    for (auto t : replacement)
        retval->push_back(t.second);
    replacement.clear();
    return retval;
}

const IR::Node* DoReplaceTuples::postorder(IR::Type_Tuple* type) {
    auto st = repl->getReplacement(type)->getP4Type();
    return st;
}

const IR::Node* DoReplaceTuples::insertReplacements(const IR::Node* before) {
    auto result = repl->getAllReplacements();
    if (result == nullptr)
        return before;
    result->push_back(before);
    return result;
}

}  // namespace BMV2
