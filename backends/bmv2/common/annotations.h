/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_BMV2_COMMON_ANNOTATIONS_H_
#define BACKENDS_BMV2_COMMON_ANNOTATIONS_H_

#include "frontends/p4/parseAnnotations.h"
#include "ir/ir.h"

namespace P4::BMV2 {

using namespace P4::literals;

/// Parses BMV2-specific annotations.
class ParseAnnotations : public P4::ParseAnnotations {
 public:
    ParseAnnotations()
        : P4::ParseAnnotations("BMV2", false,
                               {
                                   PARSE_EMPTY("metadata"_cs),
                                   PARSE_EXPRESSION_LIST("field_list"_cs),
                                   PARSE("alias"_cs, StringLiteral),
                                   PARSE("priority"_cs, Constant),
                                   PARSE_EXPRESSION_LIST("p4runtime_translation_mappings"_cs),
                                   PARSE_P4RUNTIME_TRANSLATION("p4runtime_translation"_cs),
                               }) {}
};

}  // namespace P4::BMV2

#endif /* BACKENDS_BMV2_COMMON_ANNOTATIONS_H_ */
