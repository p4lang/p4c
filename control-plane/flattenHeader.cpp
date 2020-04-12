/*
Copyright 2019-present Barefoot Networks, Inc.

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

#include "flattenHeader.h"
#include "frontends/p4/typeMap.h"

namespace P4 {

namespace ControlPlaneAPI {

FlattenHeader::FlattenHeader(const P4::TypeMap* typeMap, IR::Type_Header* flattenedHeader)
    : typeMap(typeMap), flattenedHeader(flattenedHeader) { }

void FlattenHeader::doFlatten(const IR::Type* type) {
    if (type->is<IR::Type_Struct>()) needsFlattening = true;
    if (auto st = type->to<IR::Type_StructLike>()) {
        for (auto f : st->fields) {
            nameSegments.push_back(f->name);
            allAnnotations.push_back(f->annotations);
            doFlatten(typeMap->getType(f, true));
            allAnnotations.pop_back();
            nameSegments.pop_back();
        }
    } else {  // primitive field types
        auto& newFields = flattenedHeader->fields;
        auto newName = makeName("_");
        // preserve the original name using an annotation
        auto originalName = makeName(".");
        auto annotations = mergeAnnotations();
        annotations = annotations->addOrReplace(
            IR::Annotation::nameAnnotation, new IR::StringLiteral(originalName));
        newFields.push_back(new IR::StructField(IR::ID(newName), annotations, type));
    }
}

cstring FlattenHeader::makeName(cstring sep) const {
    cstring name = "";
    for (auto n : nameSegments) name += sep + n;
    return name;
}

/// Merge all the annotation vectors in allAnnotations into a single
/// one. Duplicates are resolved, with preference given to the ones towards the
/// end of allAnnotations, which correspond to the most "nested" ones.
const IR::Annotations* FlattenHeader::mergeAnnotations() const {
    auto mergedAnnotations = new IR::Annotations();
    for (auto annosIt = allAnnotations.rbegin(); annosIt != allAnnotations.rend(); annosIt++) {
        for (auto anno : (*annosIt)->annotations) {
            // if an annotation with the same name was added previously, skip
            if (mergedAnnotations->getSingle(anno->name)) continue;
            mergedAnnotations->add(anno);
        }
    }
    return mergedAnnotations;
}

/* static */
const IR::Type_Header* FlattenHeader::flatten(
    const P4::TypeMap* typeMap, const IR::Type_Header* headerType) {
    auto flattenedHeader = headerType->clone();
    flattenedHeader->fields.clear();
    FlattenHeader flattener(typeMap, flattenedHeader);
    flattener.doFlatten(headerType);
    return flattener.needsFlattening ? flattenedHeader : headerType;
}

}  // namespace ControlPlaneAPI

}  // namespace P4
