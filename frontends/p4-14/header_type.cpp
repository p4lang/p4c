// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

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
            new IR::Annotation(IR::Annotation::lengthAnnotation, max_length->getExpr()));
    return false;
}

}  // namespace P4
