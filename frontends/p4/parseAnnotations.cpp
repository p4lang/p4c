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

#include "parseAnnotations.h"

namespace P4 {

bool ParseAnnotations::needsParsing(IR::Annotation* annotation) {
    if (!annotation->expr.empty() || !annotation->kv.empty()) {
        // We already have an expression or a key-value list. This happens when
        // we have a P4₁₄ program, in which case the annotation body had better
        // be empty.
        if (!annotation->body.empty()) {
            BUG("Annotation has been parsed already");
        }

        return false;
    }

    return true;
}

void ParseAnnotations::postorder(IR::Annotation* annotation) {
    // @tableonly, @defaultonly, @hidden, @atomic, and @optional have no
    // arguments.
    PARSE_NO_ARGS(IR::Annotation::tableOnlyAnnotation)
    PARSE_NO_ARGS(IR::Annotation::defaultOnlyAnnotation)
    PARSE_NO_ARGS(IR::Annotation::hiddenAnnotation)
    PARSE_NO_ARGS(IR::Annotation::atomicAnnotation)
    PARSE_NO_ARGS(IR::Annotation::optionalAnnotation)

    // @name and @deprecated have a string literal argument.
    PARSE(IR::Annotation::nameAnnotation, StringLiteral)
    PARSE(IR::Annotation::deprecatedAnnotation, StringLiteral)

    // @length has an expression argument.
    PARSE(IR::Annotation::lengthAnnotation, Expression)

    // @pkginfo has a key-value list argument.
    PARSE_KV_LIST(IR::Annotation::pkginfoAnnotation)

    // Unknown annotation. Leave as is.
}

}  // namespace P4
