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
#include "lib/error.h"
#include "frontends/p4-14/p4-14-parse.h"
#include "frontends/p4/fromv1.0/converters.h"
#include "frontends/p4/frontend.h"
#include "frontends/p4/p4-parse.h"

const IR::P4Program* parseP4File(CompilerOptions& options) {
    FILE* in = options.preprocess();
    if (::errorCount() > 0 || in == nullptr)
        return nullptr;

    const IR::P4Program* result = nullptr;
    bool compiling10 = options.isv1();
    if (compiling10) {
        P4V1::Converter converter;
        converter.loadModel();
        // Model is loaded before parsing the input file.
        // In this way the SourceInfo in the model comes first.
        const IR::Node* v1 = parse_p4v1_file(options, in);
        if (verbose)
            std::cerr << "Converting to P4 v1.2" << std::endl;
        converter.visit(v1);
        if (v1 != nullptr) {
            result = v1->to<IR::P4Program>();
            if (result == nullptr) {
                BUG("Conversion returned %1%", v1);
                return result;
            }
        }
    } else {
        result = parse_p4v1_2_file(options.file, in);
    }
    options.closeInput(in);
    if (::errorCount() > 0) {
        ::error("%1% errors encountered, aborting compilation", ::errorCount());
        return nullptr;
    }
    return result;
}
