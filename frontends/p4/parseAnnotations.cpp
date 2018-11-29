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
#include "ir/ir.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/parsers/parserDriver.h"

namespace P4 {
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
            return;
        }

        return;
    }

    if (!annotation->expr.empty() || !annotation->kv.empty()) {
        BUG("Annotation has been parsed already");
    }
    
    // @name has a string literal argument.
    if (annotation->name == IR::Annotation::nameAnnotation) {
        annotation->expr.push_back(
                P4ParserDriver::parseStringLiteral(annotation->srcInfo,
                                                   &annotation->body));
        return;
    }

    // @length has an int literal argument.
    if (annotation->name == IR::Annotation::lengthAnnotation) {
        annotation->expr.push_back(
                P4ParserDriver::parseInteger(annotation->srcInfo,
                                             &annotation->body));
        return;
    }

    // @pkginfo has a key-value list argument.
    if (annotation->name == IR::Annotation::pkginfoAnnotation) {
        annotation->kv.append(
                *P4ParserDriver::parseKvList(annotation->srcInfo,
                                             &annotation->body));
        return;
    }

    // Unknown annotation. Leave as is.
}
}
