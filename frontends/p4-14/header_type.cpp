/*
Copyright 2013-present Barefoot Networks, Inc. 

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

#include "header_type.h"

bool
HeaderTypeMaxLengthCalculator::preorder(IR::Type_StructLike *hdr_type) {
    IR::Annotations *annot = nullptr;
    auto *max_length = hdr_type->getAnnotation("max_length");
    if (!max_length) {
        unsigned len = 0;
        for (auto field : hdr_type->fields)
            len += field->type->width_bits();
        max_length = new IR::Annotation("max_length", len);
        if (!annot) annot = hdr_type->annotations->clone();
        annot->annotations.push_back(max_length); }
    auto *length = hdr_type->getAnnotation("length");
    if (!length) {
        if (!annot) annot = hdr_type->annotations->clone();
        length = new IR::Annotation("length", max_length->expr);
        annot->annotations.push_back(length); }
    if (annot) hdr_type->annotations = annot;
    return false;
}
