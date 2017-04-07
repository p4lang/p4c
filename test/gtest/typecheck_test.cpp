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

#include "gtest/gtest.h"

#include "ir/ir.h"
#include "p4/p4-parse.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/typeChecking/bindVariables.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

#include "p4/validateParsedProgram.h"
#include "p4/createBuiltins.h"
#include "p4/unusedDeclarations.h"
#include "p4/typeChecking/typeChecker.h"

#include "helper.h"

using namespace P4;

TEST(UNITTEST, helloworld) {
    std::string program = with_core_p4(
        "parser Parser<H, M> (packet_in p){ state start{} };\n"
        "control empty() { apply {} };\n"
        "package top(empty e);\n"
        "top(empty()) main;\n");

    const IR::P4Program* pgm = parse_string(program);
    ASSERT_NE(nullptr, pgm);

    ReferenceMap  refMap;
    TypeMap       typeMap;

    PassManager passes = {
      new ValidateParsedProgram(false),
      new CreateBuiltins(),
      new ResolveReferences(&refMap, true),
      new ConstantFolding(&refMap, nullptr),
      new ResolveReferences(&refMap),
//      new TypeInference(&refMap, &typeMap),
    };
    pgm = pgm->apply(passes);

    ASSERT_NE(nullptr, pgm);
}

TEST(UNITTEST, package) {
    std::string program = with_core_p4(
        "parser Parser<H, M> (packet_in p){ state start{} };\n"
        "control empty() { apply {} };\n"
        "package top(empty e);\n");

    const IR::P4Program* pgm = parse_string(program);

    ASSERT_NE(nullptr, pgm);

    PassManager passes = {
        new CreateBuiltins(),
    };
    pgm = pgm->apply(passes);
}
