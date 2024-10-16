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

#include "collect_global_pragma.h"
#include <algorithm>
#include "bf-p4c/common/pragma/all_pragmas.h"
#include "bf-p4c/parde/clot/pragma/do_not_use_clot.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"

const std::vector<std::string>*
CollectGlobalPragma::g_global_pragma_names = new std::vector<std::string>{
    PragmaAlias::name,
    PragmaAtomic::name,
    PragmaAutoInitMetadata::name,
    PragmaBackwardCompatible::name,
    PragmaBytePack::name,
    PragmaContainerSize::name,
    PragmaContainerType::name,
    PragmaDisableI2EReservedDropImplementation::name,
    PragmaDisableEgressMirrorIOSelectInitialization::name,
    PragmaDoNotUseClot::name,
    PragmaEgressIntrinsicMetadataOpt::name,
    PragmaGFMParityEnable::name,
    PragmaMutuallyExclusive::name,
    PragmaNoInit::name,
    PragmaNoOverlay::name,
    PragmaNoPack::name,
    PragmaParserBandwidthOpt::name,
    PragmaParserGroupMonogress::name,
    PragmaAllowPOVonMocha::name,
    PragmaPhvLimit::name,
    PragmaPrioritizeARAinits::name,
    PragmaQuickPhvAlloc::name,
    PragmaSolitary::name,
    PHV::pragma::DISABLE_DEPARSE_ZERO
};

cstring CollectGlobalPragma::getStructFieldName(const IR::StructField* s) const {
    const auto nameAnnotation = s->annotations->getSingle("name"_cs);
    if (!nameAnnotation || nameAnnotation->expr.size() != 1)
        return cstring();
    auto structName = nameAnnotation->expr.at(0)->to<IR::StringLiteral>();
    if (!structName) return cstring();
    return structName->value;
}

bool CollectGlobalPragma::preorder(const IR::StructField* s) {
    if (!s->annotations) return true;
    cstring structFieldName = getStructFieldName(s);
    // The header names are prefixed with a '.'. For the purposes of PHV allocation, we do not need
    // the period.
    cstring structFieldNameWithoutDot = (structFieldName != cstring()) ? structFieldName.substr(1)
                                        : structFieldName;
    for (auto ann : s->annotations->annotations) {
        // Ignore annotations that are not NOT_PARSED or NOT_DEPARSED
        bool notParsedDeparsedAnnotation = (ann->name.name == PHV::pragma::NOT_PARSED ||
                ann->name.name == PHV::pragma::NOT_DEPARSED);
        if (!notParsedDeparsedAnnotation)
            continue;
        if (structFieldName == cstring())
            BUG("Pragma %1% on Struct Field %2% without a name", ann->name.name, s);
        // For the notParsedDeparsedAnnotation, create a new annotation that includes
        // structFieldName.
        if (!ann->expr.size())
            continue;

        auto& exprs = ann->expr;

        const unsigned min_required_arguments = 1;  // gress
        unsigned required_arguments = min_required_arguments;
        unsigned expr_index = 0;
        const IR::StringLiteral *pipe_arg = nullptr;
        const IR::StringLiteral *gress_arg = nullptr;

        if (!PHV::Pragmas::determinePipeGressArgs(exprs, expr_index,
                required_arguments, pipe_arg, gress_arg)) {
            continue;
        }

        if (!PHV::Pragmas::checkNumberArgs(ann, required_arguments,
                min_required_arguments, true, ann->name.name,
                "`gress'"_cs)) {
            continue;
        }

        // Create a new annotation that includes the pragma not_parsed or not_deparsed and also adds
        // the name of the header with which the pragma is associated. The format is equivalent to:
        // @pragma not_parsed <gress> <hdr_name>. PHV allocation can then consume this added
        // annotation when implementing the deparsed zero optimization.
        auto* name = new IR::StringLiteral(IR::ID(structFieldNameWithoutDot));
        IR::Annotation* newAnnotation = new IR::Annotation(IR::ID(ann->name.name),
            {pipe_arg, gress_arg, name});
        LOG1("      Adding global annotation: " << newAnnotation);
        global_pragmas_.push_back(newAnnotation);
    }
    return true;
}

bool CollectGlobalPragma::preorder(const IR::Annotation *annotation) {
    auto pragma_name = annotation->name.name;
    bool is_global_pragma = (std::find(g_global_pragma_names->begin(),
                                       g_global_pragma_names->end(), pragma_name)
                             != g_global_pragma_names->end());
    if (is_global_pragma) {
        global_pragmas_.push_back(annotation);
        LOG1("      Adding global annotation: " << annotation);
    }
    return false;
}

const IR::Annotation *CollectGlobalPragma::exists(const char *pragma_name) const {
    const IR::Annotation * pragma = nullptr;
    for (auto p : global_pragmas()) {
        if (p->name == pragma_name) {
            pragma = p;
            break;
        }
    }
    return pragma;
}
