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

#include "parseInput.h"

#include <iostream>

#include "frontends/p4/fromv1.0/converters.h"
#include "frontends/parsers/parserDriver.h"
#include "lib/error.h"

namespace P4 {

const IR::P4Program *parseP4String(const char *sourceFile, unsigned sourceLine,
                                   const std::string &input,
                                   CompilerOptions::FrontendVersion version) {
    std::istringstream stream(input);
    auto result =
        version == CompilerOptions::FrontendVersion::P4_14
            ? parseV1Program<std::istringstream, P4V1::Converter>(stream, sourceFile, sourceLine)
            : P4ParserDriver::parse(stream, sourceFile, sourceLine);

    if (::errorCount() > 0) {
        ::error(ErrorType::ERR_OVERLIMIT, "%1% errors encountered, aborting compilation",
                ::errorCount());
        return nullptr;
    }
    BUG_CHECK(result != nullptr, "Parsing failed, but we didn't report an error");
    return result;
}

const IR::P4Program *parseP4String(const std::string &input,
                                   CompilerOptions::FrontendVersion version) {
    return parseP4String("(string)", 1, input, version);
}

}  // namespace P4
