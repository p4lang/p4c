/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "program_structure.h"

#include <algorithm>
#include <set>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

#include "ir/ir.h"
// #include "lib/path.h"
#include "bf-p4c/common/parse_annotations.h"
#include "frontends/common/constantFolding.h"
#include "frontends/common/options.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4-14/fromv1.0/converters.h"
#include "frontends/p4-14/header_type.h"
#include "frontends/p4-14/typecheck.h"
#include "frontends/p4/cloner.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/reservedWords.h"
#include "frontends/p4/tableKeyNames.h"
#include "frontends/parsers/parserDriver.h"
#include "lib/big_int_util.h"

namespace BFN {

const cstring ProgramStructure::INGRESS_PARSER = "ingress_parser"_cs;
const cstring ProgramStructure::INGRESS = "ingress"_cs;
const cstring ProgramStructure::INGRESS_DEPARSER = "ingress_deparser"_cs;
const cstring ProgramStructure::EGRESS_PARSER = "egress_parser"_cs;
const cstring ProgramStructure::EGRESS = "egress"_cs;
const cstring ProgramStructure::EGRESS_DEPARSER = "egress_deparser"_cs;

// append target architecture to declaration
void ProgramStructure::include(cstring filename, IR::Vector<IR::Node> *vector) {
    // the p4c driver sets environment variables for include
    // paths.  check the environment and add these to the command
    // line for the preprocessor
    char *drvP4IncludePath = getenv("P4C_16_INCLUDE_PATH");
    std::filesystem::path path(drvP4IncludePath ? drvP4IncludePath : p4includePath);

    CompilerOptions options;
    if (filename.startsWith("/"))
        options.file = filename.string();
    else
        options.file = path / filename.string();

    options.langVersion = CompilerOptions::FrontendVersion::P4_16;

    if (filename == "tna.p4") options.preprocessor_options += " -D__TARGET_TOFINO__=1";
    if (auto preprocessorResult = options.preprocess(); preprocessorResult.has_value()) {
        FILE *file = preprocessorResult.value().get();

        if (errorCount() > 0) {
            error("Failed to preprocess architecture file %1%", options.file);
            return;
        }

        auto code = P4::P4ParserDriver::parse(file, options.file.string());
        if (code == nullptr || errorCount() > 0) {
            error("Failed to load architecture file %1%", options.file);
            return;
        }

        // Apply the ParseAnnotations
        code = code->apply(BFN::ParseAnnotations());

        // Add parsed declarations to vector
        for (auto decl : code->objects) {
            vector->push_back(decl);
        }
    } else {
        error("Preprocessing failed for architecture file %1%", options.file);
    }
}

cstring ProgramStructure::getBlockName(cstring name) {
    BUG_CHECK(blockNames.find(name) != blockNames.end(), "Cannot find block %1% in package", name);
    auto blockname = blockNames.at(name);
    return blockname;
}

void ProgramStructure::createTofinoArch() {
    for (auto decl : targetTypes) {
        if (decl->is<IR::Type_Error>()) continue;
        if (auto td = decl->to<IR::Type_Typedef>()) {
            if (auto def = declarations.getDeclaration(td->name)) {
                LOG3("Type " << td->name << " is already defined to " << def->getName()
                             << ", ignored.");
                continue;
            }
        }
        // There are 2 overloaded declarations of static_assert() extern functions
        // in latest core.p4 version.
        // As 'declarations' is IR::IndexedVector, it can not store 2 objects
        // with the same name.
        // We have 2 options:
        // - either omit static_assert() declarations here
        // - or change 'declarations' to IR::Vector and add additional structure to
        //   track the types everywhere where declarations.getDeclaration() is used
        //
        // As static_assert() invocations are constant-folded in Frontend, there
        // should not be any of those invocations in the IR at this point so
        // the declaration is not needed.
        // The first option of options listed above is chosen here.
        if (decl->is<IR::Method>() && decl->to<IR::Method>()->name == "static_assert") {
            LOG3("Skipping: " << decl);
            continue;
        }
        declarations.push_back(decl);
    }
}

void ProgramStructure::createErrors() {
    auto allErrors = new IR::IndexedVector<IR::Declaration_ID>();
    for (auto e : errors) {
        allErrors->push_back(new IR::Declaration_ID(e));
    }
    declarations.push_back(new IR::Type_Error("error", *allErrors));
}

void ProgramStructure::createTypes() {
    for (auto h : type_declarations) {
        if (auto def = declarations.getDeclaration(h.first)) {
            LOG3("Type " << h.first << " is already defined to " << def->getName() << ", ignored.");
            continue;
        }
        declarations.push_back(h.second);
    }
}

void ProgramStructure::createActions() {
    for (auto a : action_types) {
        declarations.push_back(a);
    }
}

std::ostream &operator<<(std::ostream &out, const BFN::MetadataField &m) {
    out << "Metadata Field { struct : " << m.structName << ", field : " << m.fieldName
        << ", width : " << m.width << ", offset : " << m.offset << ", isCG : " << m.isCG << " }"
        << std::endl;
    return out;
}

}  // namespace BFN
