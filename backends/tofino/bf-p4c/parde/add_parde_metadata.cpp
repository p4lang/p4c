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

#include "bf-p4c/parde/add_parde_metadata.h"

#include "bf-p4c/arch/bridge_metadata.h"
#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/common/pragma/all_pragmas.h"
#include "bf-p4c/common/pragma/collect_global_pragma.h"
#include "bf-p4c/common/pragma/pragma.h"
#include "bf-p4c/device.h"
#include "lib/exceptions.h"

bool AddParserMetadata::preorder(IR::BFN::Parser *parser) {
    switch (parser->gress) {
        case INGRESS:
            addIngressMetadata(parser);
            break;
        case EGRESS:
            addEgressMetadata(parser);
            break;
        default:
            break;  // Nothing for Ghost
    }

    return true;
}

void AddParserMetadata::addTofinoIngressParserEntryPoint(IR::BFN::Parser *parser) {
    // This state initializes some special metadata and serves as an entry
    // point.
    auto *igParserMeta = getMetadataType(pipe, "ingress_intrinsic_metadata_from_parser"_cs);
    auto *alwaysDeparseBit =
        new IR::TempVar(IR::Type::Bits::get(1), true, "ingress$always_deparse"_cs);
    auto *bridgedMetadataIndicator =
        new IR::TempVar(IR::Type::Bits::get(8), false, BFN::BRIDGED_MD_INDICATOR);

    auto prim = new IR::Vector<IR::BFN::ParserPrimitive>();
    if (isV1) {
        prim->push_back(new IR::BFN::Extract(alwaysDeparseBit, new IR::BFN::ConstantRVal(1)));
        prim->push_back(
            new IR::BFN::Extract(bridgedMetadataIndicator, new IR::BFN::ConstantRVal(0)));
    }

    if (igParserMeta) {
        auto *globalTimestamp = gen_fieldref(igParserMeta, "global_tstamp"_cs);
        auto *globalVersion = gen_fieldref(igParserMeta, "global_ver"_cs);
        prim->push_back(new IR::BFN::Extract(
            globalTimestamp,
            new IR::BFN::MetadataRVal(
                StartLen(Device::metaGlobalTimestampStart(), Device::metaGlobalTimestampLen()))));
        prim->push_back(new IR::BFN::Extract(
            globalVersion, new IR::BFN::MetadataRVal(StartLen(Device::metaGlobalVersionStart(),
                                                              Device::metaGlobalVersionLen()))));
    } else {
        warning("ingress_intrinsic_metadata_from_parser not defined in parser %1%", parser->name);
    }

    // Initialize mirror_type.$valid to 1 to workaround ingress drop issue in tofino2.
    if (Device::currentDevice() == Device::JBAY) {
        // can be disabled with a pragma
        if (!pipe->has_pragma(PragmaDisableI2EReservedDropImplementation::name)) {
            auto *igDeparserMeta =
                getMetadataType(pipe, "ingress_intrinsic_metadata_for_deparser"_cs);
            if (igDeparserMeta) {
                auto *mirrorType = gen_fieldref(igDeparserMeta, "mirror_type"_cs);
                auto povBit = new IR::BFN::FieldLVal(new IR::TempVar(
                    IR::Type::Bits::get(1), true, mirrorType->toString() + ".$valid"));
                prim->push_back(new IR::BFN::Extract(povBit, new IR::BFN::ConstantRVal(1)));
            } else {
                warning("ingress_intrinsic_metadata_for_deparser not defined in parser %1%",
                        parser->name);
            }
        }
    }

    parser->start =
        new IR::BFN::ParserState(createThreadName(parser->gress, "$entry_point"_cs), parser->gress,
                                 *prim, {}, {new IR::BFN::Transition(match_t(), 0, parser->start)});
}

void AddParserMetadata::addIngressMetadata(IR::BFN::Parser *parser) {
    if (Device::currentDevice() == Device::TOFINO || Device::currentDevice() == Device::JBAY) {
        addTofinoIngressParserEntryPoint(parser);
    }
}

void AddParserMetadata::addTofinoEgressParserEntryPoint(IR::BFN::Parser *parser) {
    auto *egParserMeta = getMetadataType(pipe, "egress_intrinsic_metadata_from_parser"_cs);

    auto *alwaysDeparseBit =
        new IR::TempVar(IR::Type::Bits::get(1), true, "egress$always_deparse"_cs);

    auto prim = new IR::Vector<IR::BFN::ParserPrimitive>();
    if (isV1) prim->push_back(new IR::BFN::Extract(alwaysDeparseBit, new IR::BFN::ConstantRVal(1)));

    if (egParserMeta) {
        auto *globalTimestamp = gen_fieldref(egParserMeta, "global_tstamp"_cs);
        auto *globalVersion = gen_fieldref(egParserMeta, "global_ver"_cs);
        prim->push_back(new IR::BFN::Extract(
            globalTimestamp,
            new IR::BFN::MetadataRVal(
                StartLen(Device::metaGlobalTimestampStart(), Device::metaGlobalTimestampLen()))));
        prim->push_back(new IR::BFN::Extract(
            globalVersion, new IR::BFN::MetadataRVal(StartLen(Device::metaGlobalVersionStart(),
                                                              Device::metaGlobalVersionLen()))));
    } else {
        warning("egress_intrinsic_metadata_from_parser not defined in parser %1%", parser->name);
    }

    parser->start =
        new IR::BFN::ParserState(createThreadName(parser->gress, "$entry_point"_cs), parser->gress,
                                 *prim, {}, {new IR::BFN::Transition(match_t(), 0, parser->start)});
}

void AddParserMetadata::addEgressMetadata(IR::BFN::Parser *parser) {
    if (Device::currentDevice() == Device::TOFINO || Device::currentDevice() == Device::JBAY) {
        addTofinoEgressParserEntryPoint(parser);
    }
}

bool AddDeparserMetadata::preorder(IR::BFN::Deparser *d) {
    switch (d->gress) {
        case INGRESS:
            addIngressMetadata(d);
            break;
        case EGRESS:
            addEgressMetadata(d);
            break;
        default:
            break;  // Nothing for Ghost
    }
    return false;
}

void addDeparserParamRename(IR::BFN::Deparser *deparser, const IR::HeaderOrMetadata *meta,
                            cstring field, cstring paramName) {
    auto *param = new IR::BFN::DeparserParameter(paramName, gen_fieldref(meta, field));

    deparser->params.push_back(param);
}

void AddDeparserMetadata::addIngressMetadata(IR::BFN::Deparser *d) {
    for (auto f : Device::archSpec().getIngressInstrinicMetadataForTM()) {
        auto *tmMeta = getMetadataType(pipe, "ingress_intrinsic_metadata_for_tm"_cs);
        if (!tmMeta) {
            warning("ig_intr_md_for_tm not defined in ingress control block");
            continue;
        }
        addDeparserParamRename(d, tmMeta, f.name, f.asm_name);
    }

    for (auto f : Device::archSpec().getIngressInstrinicMetadataForDeparser()) {
        auto *dpMeta = getMetadataType(pipe, "ingress_intrinsic_metadata_for_deparser"_cs);
        if (!dpMeta) {
            warning("ig_intr_md_for_dprsr not defined in ingress control block");
            continue;
        }
        addDeparserParamRename(d, dpMeta, f.name, f.asm_name);
    }
}

void AddDeparserMetadata::addEgressMetadata(IR::BFN::Deparser *d) {
    for (auto f : Device::archSpec().getEgressIntrinsicMetadataForOutputPort()) {
        auto *outputMeta = getMetadataType(pipe, "egress_intrinsic_metadata_for_output_port"_cs);
        if (!outputMeta) {
            warning("eg_intr_md_for_oport not defined in egress control block");
            continue;
        }
        addDeparserParamRename(d, outputMeta, f.name, f.asm_name);
    }

    for (auto f : Device::archSpec().getEgressIntrinsicMetadataForDeparser()) {
        auto *dpMeta = getMetadataType(pipe, "egress_intrinsic_metadata_for_deparser"_cs);
        if (!dpMeta) {
            warning("eg_intr_md_for_dprsr not defined in egress control block");
            continue;
        }
        addDeparserParamRename(d, dpMeta, f.name, f.asm_name);
    }
    /* egress_port is how the egress deparser knows where to push
     * the reassembled header and is absolutely necessary
     */
    for (auto f : Device::archSpec().getEgressIntrinsicMetadata()) {
        auto *egMeta = getMetadataType(pipe, "egress_intrinsic_metadata"_cs);
        if (!egMeta) {
            warning("eg_intr_md not defined in egress control block");
            continue;
        }
        addDeparserParamRename(d, egMeta, f.name, f.asm_name);
    }
}
