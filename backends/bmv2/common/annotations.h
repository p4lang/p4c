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

#ifndef BACKENDS_BMV2_COMMON_ANNOTATIONS_H_
#define BACKENDS_BMV2_COMMON_ANNOTATIONS_H_

#include "frontends/p4/parseAnnotations.h"
#include "ir/ir.h"

namespace p4c::BMV2 {

using namespace ::p4c::P4::literals;

/// Parses BMV2-specific annotations.
class ParseAnnotations : public P4::ParseAnnotations {
 public:
    ParseAnnotations()
        : P4::ParseAnnotations("BMV2"_cs, false,
                               {
                                   PARSE_EMPTY("metadata"_cs),
                                   PARSE_EXPRESSION_LIST("field_list"_cs),
                                   PARSE("alias"_cs, StringLiteral),
                                   PARSE("priority"_cs, Constant),
                                   PARSE_EXPRESSION_LIST("p4runtime_translation_mappings"_cs),
                                   PARSE_P4RUNTIME_TRANSLATION("p4runtime_translation"_cs),
                               }) {}
};

}  // namespace p4c::BMV2

#endif /* BACKENDS_BMV2_COMMON_ANNOTATIONS_H_ */
