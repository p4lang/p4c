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

#include "bf-p4c/control-plane/runtime.h"

#include <filesystem>  // If not already included
#include <fstream>     // Ensure this header is included

#include "bf-p4c/arch/arch.h"
#include "bf-p4c/arch/rewrite_action_selector.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/control-plane/bfruntime_arch_handler.h"
#include "bf-p4c/control-plane/bfruntime_ext.h"
#include "bf-p4c/control-plane/p4runtime_force_std.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "midend/eliminateNewtype.h"

namespace BFN {

bool CheckReservedNames::preorder(const IR::Type_ArchBlock *b) {
    LOG3(" Checking block " << b);
    auto name = b->name.toString();
    if (reservedNames.count(name) > 0)
        error("Block name in p4 cannot contain BF-RT reserved name (%s) : %s", name, b->toString());
    return false;
}

bool SetDefaultSize::preorder(IR::P4Table *table) {
    LOG2("SetDefaultSize on table : " << table->name);
    auto sizeProperty = table->getSizeProperty();
    if (sizeProperty == nullptr) {
        int defaultSize = 512;
        if (auto k = table->getConstantProperty("min_size"_cs)) defaultSize = k->asInt();
        auto properties = new IR::TableProperties(table->properties->properties);
        auto sizeProp = new IR::Property(
            IR::ID("size"), new IR::ExpressionValue(new IR::Constant(defaultSize)), true);
        properties->properties.push_back(sizeProp);
        if (warn) {
            auto hidden = table->getAnnotation("hidden"_cs);
            if (!hidden)
                warning("No size defined for table '%s', setting default size to %d", table->name,
                        defaultSize);
        }
        table->properties = properties;
    }
    return false;
}

void generateRuntime(const IR::P4Program *program, const BFN_Options &options) {
    // If the user didn't ask for us to generate P4Runtime, skip the analysis.
    bool doNotGenerateP4Info =
        options.p4RuntimeFile.isNullOrEmpty() && options.p4RuntimeFiles.isNullOrEmpty() &&
        options.p4RuntimeEntriesFile.isNullOrEmpty() &&
        options.p4RuntimeEntriesFiles.isNullOrEmpty() && options.bfRtSchema.isNullOrEmpty();
    bool generateP4Info = !doNotGenerateP4Info;

    bool doNotGenerateBFRT = options.bfRtSchema.isNullOrEmpty();
    bool generateBFRT = !doNotGenerateBFRT;

    if (doNotGenerateP4Info && doNotGenerateBFRT) return;

    auto p4RuntimeSerializer = P4::P4RuntimeSerializer::get();

    // By design we can use the same architecture handler implementation for
    // both TNA and T2NA.
    p4RuntimeSerializer->registerArch("psa"_cs, new PSAArchHandlerBuilder());
    p4RuntimeSerializer->registerArch("tna"_cs, new TofinoArchHandlerBuilder());
    p4RuntimeSerializer->registerArch("t2na"_cs, new TofinoArchHandlerBuilder());

    auto arch = P4::P4RuntimeSerializer::resolveArch(options);

    // Typedefs in P4 source are replaced in the midend. However since bf
    // runtime runs before midend, we run the pass to eliminate typedefs here to
    // facilitate bf-rt json generation.
    // Note: This can be removed if typedef elimination pass moves to frontend.
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;
    refMap.setIsV1(true);
    program = program->apply(P4::EliminateTypedef(&typeMap));
    program = program->apply(P4::TypeChecking(&refMap, &typeMap));
    program = program->apply(SetDefaultSize(false /* warn */));

    if (arch != "psa") {
        // Following ActionSelector API has been retired, we convert them
        // to the new syntax before generating BFRT json.
        // ActionSelector(bit<32> size, Hash<_> hash, SelectorMode_t mode);
        // ActionSelector(bit<32> size, Hash<_> hash, SelectorMode_t mode, Register<bit<1>, _> reg);
        // NOTE: this can be removed when we remove old syntax from tofino.p4.
        LOG1("Rewriting Action Selector for non PSA architectures" << Log::indent);
        program = program->apply(RewriteActionSelector(&refMap, &typeMap));
        LOG1_UNINDENT;
    }

    // Generate P4Info o/p
    if (generateP4Info) {
        LOG1("Populating P4Runtime Info for architecture " << arch);
        auto p4Runtime = p4RuntimeSerializer->generateP4Runtime(program, arch);

        if (options.p4RuntimeForceStdExterns) {
            auto p4RuntimeStd = convertToStdP4Runtime(p4Runtime);
            LOG1("Generating P4Runtime Output with standardized externs for architecture "
                 << arch << Log::indent);
            p4RuntimeSerializer->serializeP4RuntimeIfRequired(p4RuntimeStd, options);
            LOG1_UNINDENT;
        } else {
            LOG1("Generating P4Runtime Output for architecture " << arch << Log::indent);
            p4RuntimeSerializer->serializeP4RuntimeIfRequired(p4Runtime, options);
            LOG1_UNINDENT;
        }
        if (errorCount() > 0) return;
    }

    // Generate BFRT json o/p
    if (generateBFRT) {
        std::filesystem::path schemaFilePath = options.bfRtSchema.string();
        std::ofstream outFile(schemaFilePath);

        if (!outFile.is_open()) {
            error("Couldn't open BF-RT schema file: %1%", schemaFilePath.string());
            return;
        }
        std::ostream &out = outFile;

        // New types must be eliminated to resolve their bitwidths to be
        // generated in BF-RT json.
        auto typeChecking = new BFN::TypeChecking(&refMap, &typeMap);
        program = program->apply(P4::EliminateNewtype(&typeMap, typeChecking));
        program->apply(CheckReservedNames());

        LOG1("Populating BFRuntime Info for architecture " << arch << Log::indent);
        auto p4Runtime = p4RuntimeSerializer->generateP4Runtime(program, arch);
        LOG1_UNINDENT;

        LOG1("Generating BFRuntime JSON for architecture " << arch << Log::indent);
        auto *bfrt = new BFRT::BFRuntimeSchemaGenerator(*p4Runtime.p4Info);
        bfrt->serializeBFRuntimeSchema(&out);
        LOG1_UNINDENT;

        if (errorCount() > 0) return;
    }
}

}  // namespace BFN
