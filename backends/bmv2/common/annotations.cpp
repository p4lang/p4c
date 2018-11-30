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

#include "annotations.h"
#include "frontends/parsers/parserDriver.h"

namespace BMV2 {

using P4::P4ParserDriver;

void ParseAnnotations::postorder(IR::Annotation* annotation) {
    // @metadata has no arguments.
    if (annotation->name == "metadata") {
        if (!annotation->body.empty()) {
            ::error("%1% should not have any arguments", annotation);
        }

        return;
    }

    // @alias has a string literal argument.
    if (annotation->name == "alias") {
        if (!needsParsing(annotation)) return;

        const IR::StringLiteral* parsed =
                P4ParserDriver::parseStringLiteral(annotation->srcInfo,
                                                   &annotation->body);
        if (parsed != nullptr) {
            annotation->expr.push_back(parsed);
        }
        return;
    }

    // @priority has an int constant argument.
    if (annotation->name == "priority") {
        if (!needsParsing(annotation)) return;

        const IR::Constant* parsed =
                P4ParserDriver::parseInteger(annotation->srcInfo,
                                             &annotation->body);
        if (parsed != nullptr) {
            annotation->expr.push_back(parsed);
        }
        return;
    }

    // Unknown annotation. Leave as is.
}

}  // namespace BMV2
