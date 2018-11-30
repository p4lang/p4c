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
#include "frontends/parsers/parserDriver.h"

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
    if (annotation->name == IR::Annotation::tableOnlyAnnotation
            || annotation->name == IR::Annotation::defaultOnlyAnnotation
            || annotation->name == IR::Annotation::hiddenAnnotation
            || annotation->name == IR::Annotation::atomicAnnotation
            || annotation->name == IR::Annotation::optionalAnnotation) {
        if (!annotation->body.empty()) {
            ::error("%1% should not have any arguments", annotation);
        }

        return;
    }

    if (!needsParsing(annotation)) return;

    // @name and @deprecated have a string literal argument.
    if (annotation->name == IR::Annotation::nameAnnotation
            || annotation->name == IR::Annotation::deprecatedAnnotation) {
        const IR::StringLiteral* parsed =
                P4ParserDriver::parseStringLiteral(annotation->srcInfo,
                                                   &annotation->body);
        if (parsed != nullptr) {
            annotation->expr.push_back(parsed);
        }
        return;
    }

    // @length has an expression argument.
    if (annotation->name == IR::Annotation::lengthAnnotation) {
        const IR::Expression* parsed =
                P4ParserDriver::parseExpression(annotation->srcInfo,
                                                &annotation->body);
        if (parsed != nullptr) {
            annotation->expr.push_back(parsed);
        }
        return;
    }

    // @pkginfo has a key-value list argument.
    if (annotation->name == IR::Annotation::pkginfoAnnotation) {
        const IR::IndexedVector<IR::NamedExpression>* parsed =
                P4ParserDriver::parseKvList(annotation->srcInfo,
                                            &annotation->body);
        if (parsed != nullptr) {
            annotation->kv.append(*parsed);
        }
        return;
    }

    // Unknown annotation. Leave as is.
}

}  // namespace P4
