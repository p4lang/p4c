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

#include "compute_lowered_deparser_ir.h"

#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/device.h"
#include "helpers.h"

namespace Parde::Lowered {

IR::Vector<IR::BFN::FieldLVal> ComputeLoweredDeparserIR::removeDeparserZeroFields(
    IR::Vector<IR::BFN::FieldLVal> checksumFields) {
    IR::Vector<IR::BFN::FieldLVal> newChecksumFields;
    for (auto cfield : checksumFields) {
        auto *phv_field = phv.field(cfield->field);
        if (!phv_field->is_deparser_zero_candidate()) {
            newChecksumFields.push_back(cfield);
        }
    }
    return newChecksumFields;
}

std::pair<IR::BFN::PartialChecksumUnitConfig *, std::vector<IR::BFN::ChecksumClotInput *>>
ComputeLoweredDeparserIR::getPartialUnit(const IR::BFN::EmitChecksum *emitChecksum, gress_t gress) {
    unsigned checksumUnit;
    // Find if the checksum is already allocated a checksum units
    if (checksumInfo.count(gress) && checksumInfo[gress].count(emitChecksum)) {
        checksumUnit = checksumInfo[gress][emitChecksum];
    } else {
        checksumUnit = getChecksumUnit(emitChecksum->nested_checksum.size() > 0);
    }
    if (checksumUnit >= Device::pardeSpec().numDeparserChecksumUnits()) {
        ::fatal_error(
            "Number of deparser checksum updates exceeds the number of checksum"
            " engines available. Checksum engine not allocated for destination %1%",
            emitChecksum->dest);
    }
    LOG3("Assigning deparser partial checksum unit " << checksumUnit << " to "
                                                     << emitChecksum->dest);
    auto *unitConfig = new IR::BFN::PartialChecksumUnitConfig(checksumUnit);
    auto containerToSwap = getChecksumPhvSwap(phv, emitChecksum);
    checksumInfo[gress][emitChecksum] = unitConfig->unit;
    IR::Vector<IR::BFN::ContainerRef> phvSources;
    std::vector<Clot *> clotSources;
    std::vector<IR::BFN::ChecksumClotInput *> clots;

    if (Device::currentDevice() == Device::TOFINO) {
        IR::Vector<IR::BFN::FieldLVal> checksumFields;
        for (auto f : emitChecksum->sources) {
            checksumFields.push_back(f->field);
        }
        checksumFields = removeDeparserZeroFields(checksumFields);
        std::tie(phvSources, clotSources) = lowerFields(phv, clotInfo, checksumFields, true);
        for (auto *source : phvSources) {
            auto *input = new IR::BFN::ChecksumPhvInput(source);
            input->swap = containerToSwap[source->container];
            unitConfig->phvs.push_back(input);
        }
    } else if (Device::currentDevice() == Device::JBAY) {
        std::vector<const IR::BFN::FieldLVal *> groupPov;
        ordered_map<IR::Vector<IR::BFN::FieldLVal> *, const IR::BFN::FieldLVal *> groups;
        const IR::BFN::FieldLVal *lastPov = nullptr;
        // Since the information about mapping of field to pov is lost after lowering
        // the fields due to container merging, we will lower the fields in groups.
        // Each group of fields will have same pov bit
        IR::Vector<IR::BFN::FieldLVal> *newGroup = nullptr;
        for (auto f : emitChecksum->sources) {
            if (lastPov && lastPov->equiv(*f->povBit)) {
                newGroup->push_back(f->field);
            } else {
                // Since a new group will be created, nothing will be added in old group.
                // Adding it in a map with corresponding POV
                if (newGroup) {
                    groups[newGroup] = lastPov;
                }
                newGroup = new IR::Vector<IR::BFN::FieldLVal>();
                newGroup->push_back(f->field);
                lastPov = f->povBit;
            }
        }
        if (newGroup) groups[newGroup] = lastPov;
        int groupidx = 0;
        for (auto &group : groups) {
            phvSources.clear();
            clotSources.clear();
            *group.first = removeDeparserZeroFields(*group.first);
            std::tie(phvSources, clotSources) = lowerFields(phv, clotInfo, *group.first, true);
            auto povBit = lowerSingleBit(phv, group.second, PHV::AllocContext::DEPARSER);
            for (auto *source : phvSources) {
                auto *input = new IR::BFN::ChecksumPhvInput(source);
                input->swap = containerToSwap[source->container];
                input->povBit = povBit;
                unitConfig->phvs.push_back(input);
            }
            for (auto source : clotSources) {
                if (clots.empty() || clots.back()->clot != source) {
                    auto povBit = lowerSingleBit(phv, source->pov_bit, PHV::AllocContext::DEPARSER);
                    auto *input = new IR::BFN::ChecksumClotInput(source, povBit);
                    clots.push_back(input);
                }
            }
            groupidx++;
        }
        unitConfig->povBit = lowerSingleBit(phv, emitChecksum->povBit, PHV::AllocContext::DEPARSER);
    }
    return {unitConfig, clots};
}

IR::BFN::FullChecksumUnitConfig *ComputeLoweredDeparserIR::lowerChecksum(
    const IR::BFN::EmitChecksum *emitChecksum, gress_t gress) {
    auto fullChecksumUnit = new IR::BFN::FullChecksumUnitConfig();
    IR::BFN::PartialChecksumUnitConfig *checksumUnit = nullptr;
    std::vector<IR::BFN::ChecksumClotInput *> clots;
    std::tie(checksumUnit, clots) = getPartialUnit(emitChecksum, gress);
    fullChecksumUnit->partialUnits.push_back(checksumUnit);
    for (auto clot : clots) {
        fullChecksumUnit->clots.push_back(clot);
    }
    fullChecksumUnit->unit = checksumUnit->unit;
    // Only for JbayB0
    if (Device::pardeSpec().numDeparserInvertChecksumUnits()) {
        for (auto nestedCsum : emitChecksum->nested_checksum) {
            IR::BFN::PartialChecksumUnitConfig *nestedUnit = nullptr;
            std::vector<IR::BFN::ChecksumClotInput *> nestedClots;
            std::tie(nestedUnit, nestedClots) = getPartialUnit(nestedCsum, gress);
            // For more information about why inversion is needed
            // check ticket JBAY-2979
            nestedUnit->invert = true;
            fullChecksumUnit->partialUnits.push_back(nestedUnit);
            for (auto clot : nestedClots) {
                clot->invert = true;
                fullChecksumUnit->clots.push_back(clot);
            }
        }
    }
    fullChecksumUnit->zeros_as_ones = emitChecksum->zeros_as_ones;
    return fullChecksumUnit;
}

unsigned int ComputeLoweredDeparserIR::getChecksumUnit(bool nested) {
    if (Device::pardeSpec().numDeparserInvertChecksumUnits() == 4) {
        if (nested) {
            if (nested_unit == Device::pardeSpec().numDeparserInvertChecksumUnits()) {
                error("Too many nested checksums");
            }
            return nested_unit++;
        } else {
            if (normal_unit < Device::pardeSpec().numDeparserChecksumUnits()) {
                return normal_unit++;
            } else if (normal_unit == Device::pardeSpec().numDeparserChecksumUnits() &&
                       nested_unit < Device::pardeSpec().numDeparserInvertChecksumUnits()) {
                return nested_unit++;
            } else {
                ::fatal_error(
                    "Number of deparser checksum updates exceeds the number of"
                    "checksum engines available.");
                return 0;
            }
        }
    } else {
        if (nextChecksumUnit > 1 && Device::currentDevice() == Device::TOFINO) {
            return (nextChecksumUnit++ + lastSharedUnit);
        }
        return nextChecksumUnit++;
    }
}

bool ComputeLoweredDeparserIR::preorder(const IR::BFN::Deparser *deparser) {
    auto *loweredDeparser = deparser->gress == INGRESS ? igLoweredDeparser : egLoweredDeparser;

    // Reset the next checksum unit if needed. On Tofino, each thread has
    // its own checksum units. On JBay they're shared, and their ids are
    // global, so on that device we don't reset the next checksum unit for
    // each deparser.
    if (Device::currentDevice() == Device::TOFINO) {
        if (nextChecksumUnit > 2) lastSharedUnit = nextChecksumUnit - 2;
        nextChecksumUnit = 0;
    }

    struct LastSimpleEmitInfo {
        // The `PHV::Field::id` of the POV bit for the last simple emit.
        int povFieldId;
        // The actual range of bits (of size 1) corresponding to the POV
        // bit for the last simple emit.
        le_bitrange povFieldBits;
    };

    std::optional<LastSimpleEmitInfo> lastSimpleEmit = std::nullopt;
    std::vector<std::vector<const IR::BFN::DeparserPrimitive *>> groupedEmits;

    // The deparser contains a sequence of emit-like primitives which we'd
    // like to lower to operate on containers. Each container may contain
    // several fields, so a number of emit primitives at the field level may
    // boil down to a single emit of a container. We need to be sure,
    // however, that we don't merge together emits for fields which are
    // controlled by different POV bits or are part of different CLOTs;
    // such fields are independent entities and we can't introduce a
    // dependency between them. For that reason, we start out by grouping
    // emit-like primitives by POV bit and CLOT tag.
    LOG5("Grouping deparser primitives:");
    for (auto *prim : deparser->emits) {
        if (!prim->is<IR::BFN::EmitField>()) {
            groupedEmits.emplace_back(1, prim);
            lastSimpleEmit = std::nullopt;
            continue;
        }

        // Gather the POV bit and CLOT tag associated with this emit.
        auto *emit = prim->to<IR::BFN::EmitField>();
        auto *field = phv.field(emit->source->field);
        BUG_CHECK(field, "No allocation for emitted field: %1%", emit);
        le_bitrange povFieldBits;
        auto *povField = phv.field(emit->povBit->field, &povFieldBits);
        BUG_CHECK(povField, "No allocation for POV bit: %1%", emit);

        // Compare the POV bit and CLOT tag with the previous emit and
        // decide whether to place this emit in the same group or to start a
        // new group.
        if (!lastSimpleEmit || groupedEmits.empty()) {
            LOG5(" - Starting new emit group: " << emit);
            groupedEmits.emplace_back(1, emit);
        } else if (lastSimpleEmit->povFieldId == povField->id &&
                   lastSimpleEmit->povFieldBits == povFieldBits) {
            LOG5(" - Adding emit to group: " << emit);
            groupedEmits.back().push_back(emit);
        } else {
            LOG5(" - Starting new emit group: " << emit);
            groupedEmits.emplace_back(1, emit);
        }

        lastSimpleEmit = LastSimpleEmitInfo{povField->id, povFieldBits};
    }

    // Now we've partitioned the emit primitives into groups which can be
    // lowered independently. Walk over the groups and lower each one.
    for (auto &group : groupedEmits) {
        BUG_CHECK(!group.empty(), "Generated an empty emit group?");

        if (auto *emitClot = group.back()->to<IR::BFN::EmitClot>()) {
            auto *loweredEmitClot = new IR::BFN::LoweredEmitClot(emitClot);
            loweredDeparser->emits.push_back(loweredEmitClot);

            auto cl = emitClot->clot;
            if (clotInfo.clot_to_emit_checksum().count(cl)) {
                // this emit checksum is part of a clot
                auto emitChecksumVec = clotInfo.clot_to_emit_checksum().at(cl);
                for (auto emitChecksum : emitChecksumVec) {
                    auto f = phv.field(emitChecksum->dest->field);
                    auto unitConfig = lowerChecksum(emitChecksum, deparser->gress);
                    cl->checksum_field_to_checksum_id[f] = unitConfig->unit;
                    loweredDeparser->checksums.push_back(unitConfig);
                }
            }

            continue;
        }

        // If this is a checksum emit primitive, lower it.
        if (auto *emitChecksum = group.back()->to<IR::BFN::EmitChecksum>()) {
            BUG_CHECK(group.size() == 1, "Checksum primitives should be in a singleton group");
            auto unitConfig = lowerChecksum(emitChecksum, deparser->gress);
            loweredDeparser->checksums.push_back(unitConfig);

            // Generate the lowered checksum emit.
            auto *loweredPovBit =
                lowerSingleBit(phv, emitChecksum->povBit, PHV::AllocContext::DEPARSER);
            auto *loweredEmit = new IR::BFN::LoweredEmitChecksum(loweredPovBit, unitConfig->unit);

            loweredDeparser->emits.push_back(loweredEmit);

            continue;
        }

        if (auto *emitConstant = group.back()->to<IR::BFN::EmitConstant>()) {
            auto *loweredPovBit =
                lowerSingleBit(phv, emitConstant->povBit, PHV::AllocContext::DEPARSER);

            BUG_CHECK(emitConstant->constant->fitsUint(), "Emit constant too large");

            // TODO cut large constant into bytes

            auto value = emitConstant->constant->asUnsigned();

            BUG_CHECK(value >> 8 == 0, "Deparser constants must in bytes");

            auto *loweredEmit = new IR::BFN::LoweredEmitConstant(loweredPovBit, value);

            loweredDeparser->emits.push_back(loweredEmit);

            continue;
        }

        // This is a group of simple emit primitives. Pull out a
        // representative; all emits in the group will have the same POV bit
        // and CLOT tag.
        auto *emit = group.back()->to<IR::BFN::EmitField>();
        BUG_CHECK(emit, "Unexpected deparser primitive: %1%", group.back());

        // Gather the source fields for all of the emits.
        IR::Vector<IR::BFN::FieldLVal> sources;
        for (auto *memberEmit : group)
            sources.push_back(memberEmit->to<IR::BFN::EmitField>()->source);

        // Lower the source fields to containers and generate the new,
        // lowered emit primitives.
        IR::Vector<IR::BFN::ContainerRef> emitSources;
        std::tie(emitSources, std::ignore) = lowerFields(phv, clotInfo, sources);
        auto *loweredPovBit = lowerSingleBit(phv, emit->povBit, PHV::AllocContext::DEPARSER);
        for (auto *source : emitSources) {
            auto *loweredEmit = new IR::BFN::LoweredEmitPhv(loweredPovBit, source);
            loweredDeparser->emits.push_back(loweredEmit);
        }
    }

    // Lower deparser parameters from fields to containers.
    for (auto *param : deparser->params) {
        bool skipPOV = false;
        if (!param->source) continue;
        auto *loweredSource =
            lowerUnsplittableField(phv, clotInfo, param->source, "deparser parameter");
        auto *lowered = new IR::BFN::LoweredDeparserParameter(param->name, {loweredSource});
        if (param->povBit && !skipPOV)
            lowered->povBit = lowerSingleBit(phv, param->povBit, PHV::AllocContext::DEPARSER);
        loweredDeparser->params.push_back(lowered);
    }

    // Filter padding field out of digest field list
    auto filterPaddingField =
        [&](const IR::BFN::DigestFieldList *fl) -> IR::BFN::DigestFieldList * {
        auto sources = new IR::Vector<IR::BFN::FieldLVal>();
        for (auto src : fl->sources) {
            // do not emit padding field in digest field list.
            if (src->is<IR::Padding>()) continue;
            sources->push_back(src);
        }
        return new IR::BFN::DigestFieldList(fl->idx, *sources, fl->type, fl->controlPlaneName);
    };

    // Lower digests from fields to containers.
    for (auto &item : deparser->digests) {
        auto *digest = item.second;

        auto *lowered = new IR::BFN::LoweredDigest(digest->name);

        if (digest->selector) {
            auto *loweredSelector =
                lowerUnsplittableField(phv, clotInfo, digest->selector, "digest selector");
            lowered->selector = loweredSelector;
        }

        if (digest->povBit)
            lowered->povBit = lowerSingleBit(phv, digest->povBit, PHV::AllocContext::DEPARSER);

        // Each field list, when lowered, becomes a digest table entry.
        // Learning field lists are used to generate the format for learn
        // quanta, which are exposed to the control plane, so they have a
        // bit more metadata than other kinds of digests.
        for (auto fieldList : digest->fieldLists) {
            IR::Vector<IR::BFN::ContainerRef> phvSources;
            std::vector<Clot *> clotSources;

            LOG3("\temit fieldlist " << fieldList);
            // TODO: filter out padding fields inside the field list which
            // exist for alignment purpose, they should not be deparsed as it
            // would causes the same container to be emitted twice.
            auto fieldListNoPad = filterPaddingField(fieldList);

            std::tie(phvSources, clotSources) = lowerFields(phv, clotInfo, fieldListNoPad->sources);

            BUG_CHECK(clotSources.empty(), "digest data cannot be sourced from CLOT");

            IR::BFN::DigestTableEntry *entry = nullptr;

            if (digest->name == "learning") {
                auto *controlPlaneFormat = computeControlPlaneFormat(phv, fieldListNoPad->sources);
                entry = new IR::BFN::LearningTableEntry(
                    fieldList->idx, phvSources, fieldList->controlPlaneName, controlPlaneFormat);
            } else {
                entry = new IR::BFN::DigestTableEntry(fieldList->idx, phvSources);
            }
            lowered->entries.push_back(entry);
        }

        loweredDeparser->digests.push_back(lowered);
    }

    return false;
}

}  // namespace Parde::Lowered
