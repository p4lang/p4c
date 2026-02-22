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

#include <cstdio>
#include <cstdlib>
#include <sstream>

#include "frontends/common/constantFolding.h"
#include "frontends/common/options.h"
#include "frontends/parsers/v1/v1lexer.hpp"
#include "frontends/parsers/v1/v1parser.hpp"
#include "parserDriver.h"

#ifdef SUPPORT_P4_14
namespace P4::V1 {

V1ParserDriver::V1ParserDriver() : global(new IR::V1Program) {}

/* static */ const IR::V1Program *V1ParserDriver::parse(std::istream &in,
                                                        std::string_view sourceFile,
                                                        unsigned sourceLine /* = 1 */) {
    LOG1("Parsing P4-14 program " << sourceFile);

    // Create and configure the parser and lexer.
    V1ParserDriver driver;
    V1Lexer lexer(in);
    V1Parser parser(driver, lexer);

#ifdef YYDEBUG
    if (const char *p = getenv("YYDEBUG")) parser.set_debug_level(atoi(p));
#endif

    // Provide an initial source location.
    driver.sources->mapLine(sourceFile, sourceLine);

    // Parse.
    if (parser.parse() != 0) return nullptr;
    return driver.global;
}

/* static */ const IR::V1Program *V1ParserDriver::parse(FILE *in, std::string_view sourceFile,
                                                        unsigned sourceLine /* = 1 */) {
    std::stringstream stream;
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), in)) stream << buffer;
    return parse(stream, sourceFile, sourceLine);
}

IR::Constant *V1ParserDriver::constantFold(IR::Expression *expr) {
    IR::Node *node(expr);
    auto rv = node->apply(P4::DoConstantFolding())->to<IR::Constant>();
    return rv ? new IR::Constant(rv->srcInfo, rv->type, rv->value, rv->base) : nullptr;
}

IR::Vector<IR::Expression> V1ParserDriver::makeExpressionList(const IR::NameList *list) {
    IR::Vector<IR::Expression> rv;
    for (auto &name : list->names) rv.push_back(new IR::StringLiteral(name));
    return rv;
}

void V1ParserDriver::clearPragmas() { currentPragmas.clear(); }

void V1ParserDriver::addPragma(IR::Annotation *pragma) {
    if (!P4CContext::get().options().isAnnotationDisabled(pragma)) currentPragmas.push_back(pragma);
}

IR::Vector<IR::Annotation> V1ParserDriver::takePragmasAsVector() {
    IR::Vector<IR::Annotation> pragmas;
    std::swap(pragmas, currentPragmas);
    return pragmas;
}

}  // namespace P4::V1
#endif
