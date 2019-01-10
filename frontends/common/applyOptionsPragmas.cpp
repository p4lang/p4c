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

#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/options.h"
#include "frontends/parsers/parserDriver.h"
#include "lib/error.h"
#include "lib/log.h"

namespace P4 {

ApplyOptionsPragmas::ApplyOptionsPragmas(IOptionPragmaParser& parser)
        : parser(parser) {
    // Add an initial "option" which will ultimately be ignored.
    // Util::Options::process() expects its arguments to be `argc` and `argv`,
    // so it skips over `argv[0]`, which would ordinarily be the program name.
    options.push_back("(from pragmas)");
}

bool ApplyOptionsPragmas::preorder(const IR::Annotation* annotation) {
    auto newOptions = parser.tryToParse(annotation);
    if (!newOptions) return false;

    LOG1("Adding options from pragma: " << annotation);
    options.insert(options.end(), newOptions->begin(), newOptions->end());
    return false;
}

void ApplyOptionsPragmas::end_apply() {
    // Process the options just as if they were specified at the command line.
    // XXX(seth): It'd be nice if the user's command line options took
    // precedence; currently, pragmas override the command line.
    auto& compilerOptionsInstance = P4CContext::get().options();
    compilerOptionsInstance.process(options.size(),
                                    const_cast<char* const*>(options.data()));
}

boost::optional<IOptionPragmaParser::CommandLineOptions>
P4COptionPragmaParser::tryToParse(const IR::Annotation* annotation) {
    auto pragmaName = annotation->name.name;
    if (pragmaName == "diagnostic")
        return parseDiagnostic(annotation);
    return boost::none;
}

boost::optional<IOptionPragmaParser::CommandLineOptions>
P4COptionPragmaParser::parseDiagnostic(const IR::Annotation* annotation) {
    CommandLineOptions newOptions;

    auto pragmaArgs = &annotation->expr;

    // Parsing of option pragmas is done early in the compiler, before P4₁₆
    // annotations are parsed, so we are responsible for doing our own parsing
    // here.
    if (pragmaArgs->empty()) {
        auto parseResult =
            P4ParserDriver::parseExpressionList(annotation->srcInfo,
                                                annotation->body);
        if (parseResult != nullptr) {
            pragmaArgs = parseResult;
        }
    }

    if (pragmaArgs->size() != 2) {
        ::warning(ErrorType::WARN_MISSING, "@diagnostic takes two arguments: %1%", annotation);
        return boost::none;
    }

    auto* diagnosticName = pragmaArgs->at(0)->to<IR::StringLiteral>();
    auto* diagnosticAction = pragmaArgs->at(1)->to<IR::StringLiteral>();
    if (!diagnosticName || !diagnosticAction) {
        ::warning(ErrorType::WARN_MISSING, "@diagnostic arguments must be strings: %1%",
                  annotation);
        return boost::none;
    }

    cstring diagnosticOption;
    if (diagnosticAction->value == "disable") {
        diagnosticOption = "--Wdisable=";
    } else if (diagnosticAction->value == "warn") {
        diagnosticOption = "--Wwarn=";
    } else if (diagnosticAction->value == "error") {
        diagnosticOption = "--Werror=";
    } else {
        ::warning(ErrorType::WARN_MISMATCH, "@diagnostic's second argument must be 'disable', "
                  "'warn', or 'error': %1%", annotation);
        return boost::none;
    }

    diagnosticOption += diagnosticName->value;
    newOptions.push_back(diagnosticOption.c_str());
    return newOptions;
}

}  // namespace P4
