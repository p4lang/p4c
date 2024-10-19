/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "frontend.h"

#include "bf-p4c/arch/fromv1.0/programStructure.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/front_end_policy.h"
#include "bf-p4c/common/parse_annotations.h"
#include "bf-p4c/lib/error_type.h"
#include "bf-p4c/logging/event_logger.h"
#include "bf-p4c/midend/desugar_varbit_extract.h"
#include "frontends/common/applyOptionsPragmas.h"
#include "frontends/common/options.h"
#include "frontends/common/parseInput.h"
#include "frontends/p4-14/fromv1.0/converters.h"
#include "frontends/p4/frontend.h"
#include "lib/error.h"

// This is a simple update of the original convertor that just allows
// recirculate to have parameter of action param/68, that we allow.
// In the original p4c converter the pass FindRecirculated does not allow this.
// All we do here is replace FindRecirculated pass with a updated pass that allows this
class FindRecirculatedAllowingPort : public Inspector {
    P4V1::ProgramStructure *structure;

    void add(const IR::Primitive *primitive, unsigned operand) {
        if (primitive->operands.size() <= operand) {
            // not enough arguments, do nothing.
            // resubmit and recirculate have optional arguments
            return;
        }
        auto expression = primitive->operands.at(operand);
        if (!expression->is<IR::PathExpression>()) {
            ::error("%1%: expected a field list", expression);
            return;
        }
        auto nr = expression->to<IR::PathExpression>();
        auto fl = structure->field_lists.get(nr->path->name);
        if (fl == nullptr) {
            ::error("%1%: Expected a field list", expression);
            return;
        }
        LOG3("Recirculated " << nr->path->name);
        structure->allFieldLists.emplace(fl);
    }

 public:
    explicit FindRecirculatedAllowingPort(P4V1::ProgramStructure *structure)
        : structure(structure) {
        CHECK_NULL(structure);
        setName("FindRecirculatedAllowingPort");
    }

    void postorder(const IR::Primitive *primitive) override {
        if (primitive->name == "recirculate") {
            // exclude recirculate(68) and recirculate(local_port)
            auto operand = primitive->operands.at(0);
            if (operand->is<IR::Constant>() || operand->is<IR::ActionArg>()) {
                LOG3("Ignoring field list extraction for " << primitive);
            } else {
                add(primitive, 0);
            }
        } else if (primitive->name == "resubmit") {
            add(primitive, 0);
        } else if (primitive->name.startsWith("clone") && primitive->operands.size() == 2) {
            add(primitive, 1);
        }
    }
};
class ConverterAllowingRecirculate : public P4V1::Converter {
 public:
    ConverterAllowingRecirculate() : P4V1::Converter() {
        bool found = false;
        for (unsigned i = 0; i < passes.size(); i++) {
            // Replace FindRecirculated with our version
            if (dynamic_cast<P4V1::FindRecirculated *>(passes[i])) {
                found = true;
                LOG3("Replacing FindRecirculated (" << i << ") with AllowingPort version.");
                passes[i] = new FindRecirculatedAllowingPort(structure);
            }
        }
        BUG_CHECK(found == true, "Expected different passes in P4V1::Converter");
    }
};

const IR::P4Program *run_frontend() {
    // Initialize the Barefoot-specific error types, in case they aren't already initialized.
    BFN::ErrorType::getErrorTypes();

    auto &options = BackendOptions();
    auto hook = options.getDebugHook();

    const IR::P4Program *program = nullptr;
    if (options.arch == "tna" && options.langVersion == CompilerOptions::FrontendVersion::P4_14) {
        program = P4::parseP4File<P4V1::TnaConverter>(options);
    } else {
        // TODO: used by 14-to-v1model path, to be removed
        P4V1::Converter::createProgramStructure = P4V1::TNA_ProgramStructure::create;
        program = P4::parseP4File<ConverterAllowingRecirculate>(options);
    }
    if (!program || ::errorCount() > 0) return program;

    BFNOptionPragmaParser optionsPragmaParser;
    program->apply(P4::ApplyOptionsPragmas(optionsPragmaParser));
    program = program->apply(BFN::AnnotateVarbitExtractStates());

    auto parse_annotations = BFN::ParseAnnotations();
    auto policy = BFN::FrontEndPolicy(&parse_annotations, options.skip_seo);
    auto frontend = P4::FrontEnd(&policy);
    frontend.addDebugHook(hook);
    frontend.addDebugHook(EventLogger::getDebugHook());
    return frontend.run(options, program);
}
