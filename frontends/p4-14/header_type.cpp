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

namespace P4 {

using namespace P4::literals;

bool HeaderTypeMaxLengthCalculator::preorder(IR::Type_StructLike *hdr_type) {
    auto *max_length = hdr_type->getAnnotation(IR::Annotation::maxLengthAnnotation);
    if (!max_length) {
        unsigned len = 0;
        for (auto field : hdr_type->fields) len += field->type->width_bits();
        max_length = new IR::Annotation(IR::Annotation::maxLengthAnnotation, len);
        hdr_type->addAnnotation(max_length);
    }
    if (!hdr_type->hasAnnotation(IR::Annotation::lengthAnnotation))
        hdr_type->addAnnotation(
            new IR::Annotation(IR::Annotation::lengthAnnotation, max_length->expr));
    return false;
}

}  // namespace P4
