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

ParseAnnotations::HandlerMap ParseAnnotations::standardHandlers() {
    return {
            // @tableonly, @defaultonly, @hidden, @atomic, and @optional have
            // empty bodies.
            PARSE_EMPTY(IR::Annotation::tableOnlyAnnotation),
            PARSE_EMPTY(IR::Annotation::defaultOnlyAnnotation),
            PARSE_EMPTY(IR::Annotation::hiddenAnnotation),
            PARSE_EMPTY(IR::Annotation::atomicAnnotation),
            PARSE_EMPTY(IR::Annotation::optionalAnnotation),

            // @name and @deprecated have a string literal argument.
            PARSE(IR::Annotation::nameAnnotation, StringLiteral),
            PARSE(IR::Annotation::deprecatedAnnotation, StringLiteral),

            // @length has an expression argument.
            PARSE(IR::Annotation::lengthAnnotation, Expression),

            // @pkginfo has a key-value list argument.
            PARSE_KV_LIST(IR::Annotation::pkginfoAnnotation),

            // @synchronous has a list of method names
            PARSE_EXPRESSION_LIST(IR::Annotation::synchronousAnnotation)
        };
}

bool ParseAnnotations::parseSkip(IR::Annotation*) {
    return false;
}

bool ParseAnnotations::parseEmpty(IR::Annotation* annotation) {
    if (!annotation->body.empty()) {
        ::error("%1% should not have any arguments", annotation);
        return false;
    }

    return true;
}

bool ParseAnnotations::parseExpressionList(IR::Annotation* annotation) {
    const IR::Vector<IR::Expression>* parsed =
        P4::P4ParserDriver::parseExpressionList(annotation->srcInfo,
                                                annotation->body);
    if (parsed != nullptr) {
        annotation->expr.append(*parsed);
    }

    return parsed != nullptr;
}

bool ParseAnnotations::parseKvList(IR::Annotation* annotation) {
    const IR::IndexedVector<IR::NamedExpression>* parsed =
        P4::P4ParserDriver::parseKvList(annotation->srcInfo,
                                        annotation->body);
    if (parsed != nullptr) {
        annotation->kv.append(*parsed);
    }

    return parsed != nullptr;
}

void ParseAnnotations::postorder(IR::Annotation* annotation) {
    if (!annotation->needsParsing) {
        return;
    }

    if (!annotation->expr.empty() || !annotation->kv.empty()) {
        BUG("Unparsed annotation with non-empty expr or kv");
        return;
    }

    cstring name = annotation->name.name;
    if (!handlers.count(name)) {
        // Unknown annotation. Leave as is, but warn if desired.
        if (warnUnknown && warned.count(name) == 0) {
            warned.insert(name);
            ::warning(ErrorType::WARN_UNKNOWN, "Unknown annotation: %1%", annotation->name);
        }
        return;
    }

    annotation->needsParsing = !handlers[name](annotation);
}

}  // namespace P4
