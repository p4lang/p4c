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

#include <cstdio>
#include <iostream>
#include <sstream>

#include "frontends/parsers/parserDriver.h"
#include "frontends/p4/fromv1.0/converters.h"
#include "frontends/p4/frontend.h"
#include "lib/error.h"
#include "lib/source_file.h"

namespace P4 {

const IR::P4Program* parseP4File(CompilerOptions& options) {
    clearProgramState();
    FILE* in = nullptr;
    if (options.doNotPreprocess) {
        in = fopen(options.file, "r");
        if (in == nullptr) {
            ::error("%s: No such file or directory.", options.file);
            return nullptr;
        }
    } else {
        in = options.preprocess();
        if (::errorCount() > 0 || in == nullptr)
            return nullptr;
    }
    const IR::P4Program* result = nullptr;
    bool compiling10 = options.isv1();
    if (compiling10) {
        // We load the model before parsing the input file, so that the
        // SourceInfo in the model comes first.
        P4V1::Converter converter;
        converter.addDebugHook(options.getDebugHook());
        converter.loadModel();

        // Parse.
        const IR::Node* v1 = V1::V1ParserDriver::parse(options.file, in);
        if (::errorCount() > 0 || v1 == nullptr)
            return nullptr;

        // Convert to P4-16.
        if (Log::verbose())
            std::cerr << "Converting to P4-16" << std::endl;
        v1 = v1->apply(converter);
        if (v1 != nullptr) {
            BUG_CHECK(v1->is<IR::P4Program>(), "Conversion returned %1%", v1);
            result = v1->to<IR::P4Program>();
        }
    } else {
        result = P4::P4ParserDriver::parse(options.file, in);
    }
    options.closeInput(in);
    if (::errorCount() > 0) {
        ::error("%1% errors encountered, aborting compilation", ::errorCount());
        return nullptr;
    }
    return result;
}

const IR::P4Program* parseP4String(const std::string& input,
                                   CompilerOptions::FrontendVersion version) {
    clearProgramState();

    std::istringstream stream(input);
    const IR::P4Program* result = nullptr;

    switch (version) {
      case CompilerOptions::FrontendVersion::P4_14: {
        // We load the model before parsing the input file, so that the
        // SourceInfo in the model comes first.
        P4V1::Converter converter;
        converter.loadModel();

        // Parse.
        const IR::Node* v1 = V1::V1ParserDriver::parse("(string)", stream);
        if (::errorCount() > 0 || v1 == nullptr)
            return nullptr;

        // Convert to P4-16.
        if (Log::verbose())
            std::cerr << "Converting to P4-16" << std::endl;
        v1 = v1->apply(converter);
        if (v1 != nullptr) {
            BUG_CHECK(v1->is<IR::P4Program>(), "Conversion returned %1%", v1);
            result = v1->to<IR::P4Program>();
        }
        break;
      }

      case CompilerOptions::FrontendVersion::P4_16:
        result = P4::P4ParserDriver::parse("(string)", stream);
        break;
    }

    if (::errorCount() > 0) {
        ::error("%1% errors encountered, aborting compilation", ::errorCount());
        return nullptr;
    }

    return result;
}

void clearProgramState() {
    Util::InputSources::reset();
    clearErrorReporter();
}

}  // namespace P4
