/*
    Copyright 2018 MNK Consulting, LLC.
    http://mnkcg.com

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

#include "flattenHeaders.h"

namespace P4 {

void HeaderTypeReplacement::flatten(const P4::TypeMap* typeMap,
                                    cstring prefix,
                                    const IR::Type* type,
                                    IR::IndexedVector<IR::StructField> *fields) {
    if (auto st = type->to<IR::Type_Struct>()) {
        for (auto f : st->fields) {
            auto ft = typeMap->getType(f, true);
            flatten(typeMap, prefix + "." + f->name, ft, fields);
        }
        return;
    } else if (auto st = type->to<IR::Type_Header>()) {
        for (auto f : st->fields) {
            auto ft = typeMap->getType(f, true);
            flatten(typeMap, prefix + "." + f->name, ft, fields);
        }
        return;
    }
    //  cstring originalName = prefix; TODO once behavioral model is fixed.
    cstring fieldName = prefix.replace(".", "_") +
                        cstring::to_cstring(fieldNameRemap.size());
    fieldNameRemap.emplace(prefix, fieldName);
#if 0  // TODO once behavioral model is fixed.
    const IR::Annotations* annos = IR::Annotations::empty;
    auto ann = annos->addAnnotation(IR::Annotation::nameAnnotation,
                                    new IR::StringLiteral(originalName));
#endif
    fields->push_back(new IR::StructField(IR::ID(fieldName), /* ann, */ type->getP4Type()));
    LOG3("FH Flatten: " << type << " | " << prefix);
}

HeaderTypeReplacement::HeaderTypeReplacement(
    const P4::TypeMap* typeMap, const IR::Type_Header* type) {
    auto vec = new IR::IndexedVector<IR::StructField>();
    flatten(typeMap, "", type, vec);
    replacementType = new IR::Type_Header(type->name, IR::Annotations::empty, *vec);
}

void NestedHeaderMap::createReplacement(const IR::Type_Header* type) {
    auto repl = ::get(replacement, type->name);
    if (repl != nullptr)
        return;
    repl = new HeaderTypeReplacement(typeMap, type);
    LOG3("FH Replacement for " << type << " is " << repl);
    replacement.emplace(type->name, repl);
}

bool FindHeaderTypesToReplace::preorder(const IR::Type_Header* type) {
    // check if header has structs in it and create flat replacement if it does
    for (auto f : type->fields) {
        auto ft = map->typeMap->getType(f, true);
        if (ft->is<IR::Type_Struct>()) {
            map->createReplacement(type);
            break;
        }
    }
    return false;
}

/////////////////////////////////

const IR::Node* ReplaceHeaders::preorder(IR::P4Program* program) {
    if (replacementMap->empty()) {
        // nothing to do
        prune();
    }
    return program;
}

const IR::Node* ReplaceHeaders::postorder(IR::Type_Header* type) {
    auto canon = replacementMap->typeMap->getTypeType(getOriginal(), true);
    auto name = canon->to<IR::Type_Header>()->name;
    auto repl = replacementMap->getReplacement(name);
    if (repl != nullptr)
        return repl->replacementType;
    LOG3("FH NO REPLACEMENT FOUND FOR " << type);
    return type;
}

const IR::Node* ReplaceHeaders::postorder(IR::Member* expression) {
    // Find out if this applies to one of the parameters that are being replaced.
    LOG3("FH Starting replacing " << expression);
    const IR::Expression* e = expression;
    cstring prefix = "";
    const IR::Type_Header* h = nullptr;
    while (auto mem = e->to<IR::Member>()) {
        e = mem->expr;
        prefix = cstring(".") + mem->member + prefix;
        auto type = replacementMap->typeMap->getType(e, true);
        if ((h = type->to<IR::Type_Header>())) break;
    }
    if (h == nullptr)
        return expression;

    auto repl = replacementMap->getReplacement(h->name);
    if (repl == nullptr) {
        LOG3("FH Replacement not found for header " << h);
        return expression;
    }
    LOG3("FH Found replacement type " << h);
    // At this point we know that e is an expression of the form
    // param.field1.etc.hdr, where hdr needs to be replaced.
    auto newFieldName = ::get(repl->fieldNameRemap, prefix);
    const IR::Expression* result;
    if (newFieldName.isNullOrEmpty()) {
        auto type = replacementMap->typeMap->getType(getOriginal(), true);
        // This could be, for example, a method like setValid.
        if (!type->is<IR::Type_Struct>())
            return expression;
        if (getParent<IR::Member>() != nullptr)
            // We only want to process the outermost Member
            return expression;
        BUG_CHECK(!newFieldName.isNullOrEmpty(),
                  "FH cannot find replacement for %s in type %s", prefix, h);
    }

    result = new IR::Member(e, newFieldName);
    LOG3("FH Replacing " << expression << " with " << result);
    return result;
}

}  // namespace P4
