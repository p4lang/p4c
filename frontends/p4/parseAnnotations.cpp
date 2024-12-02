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
        // These annotations have empty bodies.
        PARSE_EMPTY(IR::Annotation::tableOnlyAnnotation),
        PARSE_EMPTY(IR::Annotation::defaultOnlyAnnotation),
        PARSE_EMPTY(IR::Annotation::hiddenAnnotation),
        PARSE_EMPTY(IR::Annotation::atomicAnnotation),
        PARSE_EMPTY(IR::Annotation::optionalAnnotation),
        PARSE_EMPTY(IR::Annotation::pureAnnotation),
        PARSE_EMPTY(IR::Annotation::noSideEffectsAnnotation),
        PARSE_EMPTY("disable_optimization"_cs),
        PARSE_EMPTY("unroll"_cs),
        PARSE_EMPTY("nounroll"_cs),

        // String arguments. These are allowed to contain concatenation which will be
        // constant-folded, so just parse them as expressions.
        PARSE(IR::Annotation::nameAnnotation, Expression),
        PARSE(IR::Annotation::deprecatedAnnotation, Expression),
        PARSE(IR::Annotation::noWarnAnnotation, Expression),

        // @length has an expression argument.
        PARSE(IR::Annotation::lengthAnnotation, Expression),

        // @pkginfo has a key-value list argument.
        PARSE_KV_LIST(IR::Annotation::pkginfoAnnotation),

        // @synchronous has a list of method names
        PARSE_EXPRESSION_LIST(IR::Annotation::synchronousAnnotation),
        // @field_list also has a list of expressions
        PARSE_EXPRESSION_LIST(IR::Annotation::fieldListAnnotation),

        // @match has an expression argument
        PARSE(IR::Annotation::matchAnnotation, Expression),
    };
}

bool ParseAnnotations::parseSkip(IR::Annotation *) { return false; }

bool ParseAnnotations::parseEmpty(IR::Annotation *annotation) {
    if (!annotation->getUnparsed().empty()) {
        ::P4::error(ErrorType::ERR_OVERLIMIT, "%1% should not have any arguments", annotation);
        return false;
    }

    annotation->body.emplace<IR::Annotation::ExpressionAnnotation>();

    return true;
}

bool ParseAnnotations::parseExpressionList(IR::Annotation *annotation) {
    const IR::Vector<IR::Expression> *parsed =
        P4::P4ParserDriver::parseExpressionList(annotation->srcInfo, annotation->getUnparsed());
    if (parsed != nullptr) {
        annotation->body = *parsed;
    }

    return parsed != nullptr;
}

bool ParseAnnotations::parseKvList(IR::Annotation *annotation) {
    const IR::IndexedVector<IR::NamedExpression> *parsed =
        P4::P4ParserDriver::parseKvList(annotation->srcInfo, annotation->getUnparsed());
    if (parsed != nullptr) {
        annotation->body = *parsed;
    }

    return parsed != nullptr;
}

bool ParseAnnotations::parseConstantList(IR::Annotation *annotation) {
    const IR::Vector<IR::Expression> *parsed =
        P4::P4ParserDriver::parseConstantList(annotation->srcInfo, annotation->getUnparsed());
    if (parsed != nullptr) {
        annotation->body = *parsed;
    }

    return parsed != nullptr;
}

bool ParseAnnotations::parseConstantOrStringLiteralList(IR::Annotation *annotation) {
    const IR::Vector<IR::Expression> *parsed = P4::P4ParserDriver::parseConstantOrStringLiteralList(
        annotation->srcInfo, annotation->getUnparsed());
    if (parsed != nullptr) {
        annotation->body = *parsed;
    }

    return parsed != nullptr;
}

bool ParseAnnotations::parseStringLiteralList(IR::Annotation *annotation) {
    const IR::Vector<IR::Expression> *parsed =
        P4::P4ParserDriver::parseStringLiteralList(annotation->srcInfo, annotation->getUnparsed());
    if (parsed != nullptr) {
        annotation->body = *parsed;
    }

    return parsed != nullptr;
}

bool ParseAnnotations::parseP4rtTranslationAnnotation(IR::Annotation *annotation) {
    const IR::Vector<IR::Expression> *parsed = P4::P4ParserDriver::parseP4rtTranslationAnnotation(
        annotation->srcInfo, annotation->getUnparsed());
    if (parsed != nullptr) {
        annotation->body = *parsed;
    }
    return parsed != nullptr;
}

void ParseAnnotations::postorder(IR::Annotation *annotation) {
    if (!annotation->needsParsing()) return;

    cstring name = annotation->name.name;
    auto handler = handlers.find(name);
    if (handler == handlers.end()) {
        // Unknown annotation. Leave as is, but warn if desired.
        if (warnUnknown && warned.insert(name).second) {
            warn(ErrorType::WARN_UNKNOWN, "Unknown annotation: %1%", annotation->name);
        }
        return;
    }

    handler->second(annotation);
}

}  // namespace P4
