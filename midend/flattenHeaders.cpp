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

void FindHeaderTypesToReplace::HeaderTypeReplacement::flatten(cstring prefix,
        const IR::Type* type, const IR::Annotations* annos,
        IR::IndexedVector<IR::StructField> *fields,
        AnnotationSelectionPolicy *policy) {
    if (auto st = type->to<IR::Type_StructLike>()) {
        std::function<bool(const IR::Annotation *)> selector =
                [&policy](const IR::Annotation *annot) {
                    if (!policy)
                        return false;
                    return policy->keep(annot);
                };
        auto annotations = st->annotations->where(selector);
        for (auto f : st->fields) {
            auto ft = typeMap->getType(f, true);
            flatten(prefix + "." + f->name, ft, annotations, fields, policy);
        }
        return;
    }
    //  cstring originalName = prefix; TODO once behavioral model is fixed.
    cstring fieldName = prefix.replace(".", "_") +
                        cstring::to_cstring(fieldNameRemap.size());
    fieldNameRemap.emplace(prefix, fieldName);
    fields->push_back(new IR::StructField(IR::ID(fieldName), annos, type->getP4Type()));
    LOG3("Flatten: " << type << " | " << prefix);
}

FindHeaderTypesToReplace::HeaderTypeReplacement::HeaderTypeReplacement(
    const P4::TypeMap* typeMap, const IR::Type_Header* type, AnnotationSelectionPolicy *policy) :
    typeMap(typeMap) {
    auto vec = new IR::IndexedVector<IR::StructField>();
    flatten("", type, IR::Annotations::empty, vec, policy);
    replacementType = new IR::Type_Header(type->name, type->annotations, *vec);
}

void FindHeaderTypesToReplace::createReplacement(const IR::Type_Header* type,
        AnnotationSelectionPolicy *policy) {
    if (replacement.count(type->name))
        return;
    replacement.emplace(type->name, new HeaderTypeReplacement(typeMap, type, policy));
}

bool FindHeaderTypesToReplace::preorder(const IR::Type_Header* type) {
    // check if header has structs in it and create flat replacement if it does
    for (auto f : type->fields) {
        auto ft = typeMap->getType(f, true);
        if (ft->is<IR::Type_Struct>()) {
            createReplacement(type, policy);
            break;
        }
    }
    return false;
}

/////////////////////////////////

const IR::Node* ReplaceHeaders::preorder(IR::P4Program* program) {
    if (findHeaderTypesToReplace->empty()) {
        // nothing to do
        prune();
    }
    return program;
}

const IR::Node* ReplaceHeaders::postorder(IR::Type_Header* type) {
    auto canon = typeMap->getTypeType(getOriginal(), true);
    auto name = canon->to<IR::Type_Header>()->name;
    auto repl = findHeaderTypesToReplace->getReplacement(name);
    if (repl != nullptr) {
        LOG3("Replace " << type << " with " << repl->replacementType);
        return repl->replacementType;
    }
    return type;
}

const IR::Node* ReplaceHeaders::postorder(IR::Member* expression) {
    // Find out if this applies to one of the parameters that are being replaced.
    const IR::Expression* e = expression;
    cstring prefix = "";
    const IR::Type_Header* h = nullptr;
    while (auto mem = e->to<IR::Member>()) {
        e = mem->expr;
        prefix = cstring(".") + mem->member + prefix;
        auto type = typeMap->getType(e, true);
        if ((h = type->to<IR::Type_Header>())) break;
    }
    if (h == nullptr)
        return expression;

    auto repl = findHeaderTypesToReplace->getReplacement(h->name);
    if (repl == nullptr) {
        return expression;
    }
    // At this point we know that e is an expression of the form
    // param.field1.etc.hdr, where hdr needs to be replaced.
    auto newFieldName = ::get(repl->fieldNameRemap, prefix);
    const IR::Expression* result;
    if (newFieldName.isNullOrEmpty()) {
        auto type = typeMap->getType(getOriginal(), true);
        // This could be, for example, a method like setValid.
        if (!type->is<IR::Type_Struct>())
            return expression;
        if (getParent<IR::Member>() != nullptr)
            // We only want to process the outermost Member
            return expression;
        BUG_CHECK(!newFieldName.isNullOrEmpty(),
                  "cannot find replacement for %s in type %s", prefix, h);
    }

    result = new IR::Member(e, newFieldName);
    LOG3("Replacing " << expression << " with " << result);
    return result;
}

}  // namespace P4
