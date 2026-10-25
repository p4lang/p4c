/*
 * SPDX-FileCopyrightText: 2018 Barefoot Networks, Inc.
 * Copyright 2018-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONTROL_PLANE_P4RUNTIMEANNOTATIONS_H_
#define CONTROL_PLANE_P4RUNTIMEANNOTATIONS_H_

#include "frontends/p4/parseAnnotations.h"
#include "ir/ir.h"

namespace P4 {

namespace ControlPlaneAPI {

/// Parses P4Runtime-specific annotations.
class ParseP4RuntimeAnnotations : public ParseAnnotations {
 public:
    ParseP4RuntimeAnnotations()
        : ParseAnnotations(
              "P4Runtime", false,
              {
                  PARSE("controller_header"_cs, StringLiteral),
                  PARSE_EMPTY("hidden"_cs),
                  PARSE("id"_cs, Constant),
                  PARSE("brief"_cs, StringLiteral),
                  PARSE("description"_cs, StringLiteral),
                  PARSE_KV_LIST("platform_property"_cs),
                  // These annotations are architecture-specific in theory, but
                  // given that they are "reserved" by the P4Runtime
                  // specification, I don't really have any qualms about adding
                  // them here. I don't think it is possible to just run a
                  // different ParseAnnotations pass in the constructor of the
                  // architecture-specific P4RuntimeArchHandlerIface
                  // implementation, since ParseAnnotations modifies the
                  // program. I don't really like the possible alternatives
                  // either: 1) modify the P4RuntimeArchHandlerIface interface
                  // so that each implementation can provide a custom
                  // ParseAnnotations instance, or 2) run a ParseAnnotations
                  // pass "locally" (in this case on action profile instances
                  // since these annotations are for them).
                  PARSE("max_group_size"_cs, Constant),
                  PARSE("selector_size_semantics"_cs, StringLiteral),
                  PARSE("max_member_weight"_cs, Constant),
                  {"p4runtime_translation"_cs, &ParseAnnotations::parseP4rtTranslationAnnotation},
              }) {}
};

}  // namespace ControlPlaneAPI

}  // namespace P4

#endif  // CONTROL_PLANE_P4RUNTIMEANNOTATIONS_H_
