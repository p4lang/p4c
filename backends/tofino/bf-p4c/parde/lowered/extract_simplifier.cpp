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

#include "extract_simplifier.h"

#include <boost/range/adaptor/reversed.hpp>

namespace Parde::Lowered {

void ExtractSimplifier::add(const IR::BFN::Extract *extract) {
    if (auto ec = extract->to<IR::BFN::ExtractClot>())
        add(ec);
    else if (auto ep = extract->to<IR::BFN::ExtractPhv>())
        add(ep);
    else
        BUG("Unexpected unclassified extract encountered while lowering parser IR: %1%", extract);
}

void ExtractSimplifier::add(const IR::BFN::ExtractClot *extract) {
    le_bitrange extracted_range;
    auto field = phv.field(extract->dest->field, &extracted_range);
    auto extracted_slice = new PHV::FieldSlice(field, extracted_range);

    // Populate clotExtracts with entries for the current extract.
    auto slice_clots = clot.slice_clots(extracted_slice);
    BUG_CHECK(slice_clots, "Parser extract didn't receive a CLOT allocation: %1%", extract);
    BUG_CHECK(slice_clots->size() == 1,
              "Expected a single CLOT for a parser extract, but it was allocated across %1% "
              "CLOTs: %2%",
              slice_clots->size(), extract);

    auto entry = *slice_clots->begin();
    auto slice = entry.first;
    auto clot = entry.second;

    BUG_CHECK(slice->range().contains(extracted_range),
              "Parser extract received an incomplete CLOT allocation: %1%", extract);
    clotExtracts[clot].push_back(extract);
}

void ExtractSimplifier::add(const IR::BFN::ExtractPhv *extract) {
    PHV::FieldUse use(PHV::FieldUse::WRITE);
    std::vector<PHV::AllocSlice> slices =
        phv.get_alloc(extract->dest->field, PHV::AllocContext::PARSER, &use);
    if (slices.empty()) {
        BUG("Parser extract didn't receive a PHV allocation: %1%", extract);
        return;
    }

    // TODO we should have single slice at this point

    for (const auto &slice : slices)
        BUG_CHECK(bool(slice.container()), "Parser extracts into invalid PHV container: %1%",
                  extract);

    if (auto *bufferSource = extract->source->to<IR::BFN::InputBufferRVal>()) {
        for (const auto &slice : slices) {
            if (!slice.isUsedParser()) continue;
            // Shift the slice to its proper place in the input buffer.
            auto bufferRange = bufferSource->range;

            // Mask to round bit positions down to the nearest byte boundary
            const int byteMask = ~(8 - 1);

            // Expand the buffer slice so that it will write to the entire
            // destination container, with this slice in the proper place.
            // If the slice didn't fit the container exactly, this will
            // write to more bits than were included in the slice, but if
            // PHV allocation did its job then those bit are either unused,
            // or are occupied by other extracts that we'll merge with this
            // one.

            const nw_bitrange containerRange =
                slice.container_slice().toOrder<Endian::Network>(slice.container().size());

            const nw_bitrange finalBufferRange =
                Device::pardeSpec().parserAllExtractorsSupportSingleByte()
                    ? bufferRange.shiftedByBits(-(containerRange.lo % 8))
                          .resizedToBits(((containerRange.hi & byteMask) + 7) -
                                         (containerRange.lo & byteMask) + 1)
                    : bufferRange.shiftedByBits(-containerRange.lo)
                          .resizedToBits(slice.container().size());

            LOG4("mapping input buffer field slice "
                 << bufferRange << " into " << slice.container() << " " << containerRange
                 << " named " << extract->dest << ". Final buffer range: " << finalBufferRange);

            const auto byteFinalBufferRange = finalBufferRange.toUnit<RangeUnit::Byte>();

            // Generate the lowered extract.
            const IR::BFN::LoweredParserRVal *newSource;
            if (bufferSource->is<IR::BFN::PacketRVal>())
                newSource = new IR::BFN::LoweredPacketRVal(byteFinalBufferRange);
            else
                newSource = new IR::BFN::LoweredMetadataRVal(byteFinalBufferRange);

            auto containerRef = new IR::BFN::ContainerRef(slice.container());
            if (Device::pardeSpec().parserAllExtractorsSupportSingleByte()) {
                nw_bitrange newRange;
                newRange.lo = containerRange.lo & byteMask;
                newRange.hi = (containerRange.hi & byteMask) + 7;
                containerRef->range = newRange;
            }
            auto *newExtract = new IR::BFN::LoweredExtractPhv(newSource, containerRef);

            newExtract->write_mode = extract->write_mode;
            newExtract->debug.info.push_back(debugInfoFor(extract, slice, bufferRange));

            if (bufferSource->is<IR::BFN::PacketRVal>())
                extractFromPacketByContainer[slice.container()].push_back(newExtract);
            else
                extractFromBufferByContainer[slice.container()].push_back(newExtract);
        }
    } else if (auto *constantSource = extract->source->to<IR::BFN::ConstantRVal>()) {
        for (auto &slice : boost::adaptors::reverse(slices)) {
            // Large constant may be extracted across multiple containers, therefore we
            // need to slice the containt into multiple slices and align each slice
            // within each container.

            auto constSlice = *(constantSource->constant);
            constSlice = constSlice & IR::Constant::GetMask(slice.width());

            // Place those bits at their offset within the container.
            constSlice = constSlice << slice.container_slice().lo;

            BUG_CHECK(constSlice.fitsUint(), "Constant slice larger than 32-bit?");

            // Create an extract that writes just those bits.
            LOG4("extract " << constSlice << " into " << slice.container());

            auto *newSource = new IR::BFN::LoweredConstantRVal(constSlice.asUnsigned());
            auto *newExtract = new IR::BFN::LoweredExtractPhv(slice.container(), newSource);

            newExtract->write_mode = extract->write_mode;
            newExtract->debug.info.push_back(debugInfoFor(extract, slice));
            extractConstantByContainer[slice.container()].push_back(newExtract);
        }
    } else {
        BUG("Unexpected parser primitive (most likely something that should "
            "have been eliminated by an earlier pass): %1%",
            extract);
    }
}

template <typename InputBufferRValType>
const IR::BFN::LoweredExtractPhv *ExtractSimplifier::createMergedExtract(
    PHV::Container container, const ExtractSimplifier::ExtractSequence &extracts,
    nw_byteinterval bufferRange) {
    // Create a single combined extract that implements all of the
    // component extracts. Each merged extract writes to an entire container.
    const auto *finalBufferValue = new InputBufferRValType(*toClosedRange(bufferRange));

    auto containerRef = new IR::BFN::ContainerRef(container);

    auto extractedSizeBits = bufferRange.toUnit<RangeUnit::Bit>().size();
    if (size_t(extractedSizeBits) != container.size()) {
        nw_bitrange newRange;
        newRange.lo = extracts[0]->dest->range->lo;
        newRange.hi = newRange.lo + extractedSizeBits - 1;
        containerRef->range = newRange;
    }

    auto *mergedExtract = new IR::BFN::LoweredExtractPhv(finalBufferValue, containerRef);

    mergedExtract->write_mode = extracts[0]->write_mode;

    for (auto *extract : extracts) mergedExtract->debug.mergeWith(extract->debug);

    return mergedExtract;
}

template <typename InputBufferRValType>
const ExtractSimplifier::ExtractSequence ExtractSimplifier::mergeExtractsFor(
    PHV::Container container, const ExtractSequence &extracts) {
    BUG_CHECK(!extracts.empty(), "Trying merge an empty sequence of extracts?");

    ExtractSequence rv;

    if (extracts.size() == 1) {
        rv.push_back(extracts[0]);
        return rv;
    }

    // Merge the input buffer range for every extract that writes to
    // this container. They should all be the same, but if they aren't
    // we want to know about it.
    nw_byteinterval bufferRange;
    std::map<int, std::map<int, ordered_set<const IR::BFN::LoweredExtractPhv *>>> extractDstSrcs;

    const IR::BFN::LoweredExtractPhv *prev = nullptr;

    for (auto *extract : extracts) {
        auto *bufferSource = extract->source->to<InputBufferRValType>();

        BUG_CHECK(bufferSource, "Unexpected non-buffer source");

        if (std::is_same<InputBufferRValType, IR::BFN::LoweredMetadataRVal>::value &&
            !Device::pardeSpec().parserAllExtractorsSupportSingleByte())
            BUG_CHECK(toHalfOpenRange(Device::pardeSpec().byteInputBufferMetadataRange())
                          .contains(bufferSource->byteInterval()),
                      "Extract from out of the input buffer range: %1%",
                      bufferSource->byteInterval());

        if (prev && extract->write_mode != prev->write_mode)
            BUG("Inconsistent parser write semantic on %1%", container);

        bufferRange = bufferSource->byteInterval().unionWith(bufferRange);
        unsigned int lo = extract->dest->range ? (unsigned int)(extract->dest->range->lo / 8) : 0;
        unsigned int hi = extract->dest->range ? (unsigned int)(extract->dest->range->hi / 8)
                                               : (unsigned int)(container.size() / 8 - 1);
        for (unsigned int i = 0; i < hi - lo + 1; i++) {
            extractDstSrcs[lo + i][bufferSource->byteInterval().lo + i].emplace(extract);
        }

        prev = extract;
    }

    BUG_CHECK(!bufferRange.empty(), "Extracting zero bytes?");

    if (Device::pardeSpec().parserAllExtractorsSupportSingleByte()) {
        // The device supports single-byte extracts from all extractors, so merge byte extracts
        // where possible.
        while (extractDstSrcs.size()) {
            ExtractSequence currExtracts;
            std::set<const IR::BFN::LoweredExtractPhv *> currExtractsSet;
            int dest = extractDstSrcs.begin()->first;
            int src = extractDstSrcs.at(dest).begin()->first;

            nw_byteinterval newBufferRange(src, src);
            while (extractDstSrcs.count(dest) && extractDstSrcs.at(dest).count(src)) {
                for (const auto *extract : extractDstSrcs.at(dest).at(src)) {
                    if (!currExtractsSet.count(extract)) {
                        currExtracts.push_back(extract);
                        currExtractsSet.emplace(extract);
                    }
                }
                newBufferRange.hi += 1;
                extractDstSrcs.at(dest).erase(src);
                if (!extractDstSrcs.at(dest).size()) extractDstSrcs.erase(dest);
                dest += 1;
                src += 1;
            }

            // Create a single combined extract that implements all of the
            // component extracts. Each merged extract writes to an entire container.
            rv.push_back(
                createMergedExtract<InputBufferRValType>(container, currExtracts, newBufferRange));
        }
    } else {
        // The device only supports full-container extractions. Veryfy that the extracts
        // consume the full container and merge.
        auto extractedSizeBits = bufferRange.toUnit<RangeUnit::Bit>().size();

        BUG_CHECK(size_t(extractedSizeBits) == container.size(),
                  "PHV allocation is invalid for container %1%"
                  " (number of extracted bits does not match container size).",
                  container);

        // Create a single combined extract that implements all of the
        // component extracts. Each merged extract writes to an entire container.
        rv.push_back(createMergedExtract<InputBufferRValType>(container, extracts, bufferRange));
    }

    return rv;
}

void ExtractSimplifier::sortExtractPhvs(
    IR::Vector<IR::BFN::LoweredParserPrimitive> &loweredExtracts) {
    std::stable_sort(
        loweredExtracts.begin(), loweredExtracts.end(),
        [&](const IR::BFN::LoweredParserPrimitive *a, const IR::BFN::LoweredParserPrimitive *b) {
            auto ea = a->to<IR::BFN::LoweredExtractPhv>();
            auto eb = b->to<IR::BFN::LoweredExtractPhv>();

            if (ea && eb) {
                auto va = ea->source->to<IR::BFN::LoweredPacketRVal>();
                auto vb = eb->source->to<IR::BFN::LoweredPacketRVal>();

                return (va && vb) ? (va->range < vb->range) : !!va;
            }

            return !!ea;
        });
}

const IR::BFN::LoweredExtractPhv *ExtractSimplifier::mergeExtractsForConstants(
    PHV::Container container, const ExtractSequence &extracts) {
    BUG_CHECK(!extracts.empty(), "Trying merge an empty sequence of extracts?");

    if (extracts.size() == 1) return extracts[0];

    // Merge all of the constant extracts for this container into a
    // single operation by ORing the constant sources together.
    auto *mergedValue = new IR::BFN::LoweredConstantRVal(0);
    auto *mergedExtract = new IR::BFN::LoweredExtractPhv(container, mergedValue);

    for (auto *extract : extracts) {
        auto *constantSource = extract->source->to<IR::BFN::LoweredConstantRVal>();

        BUG_CHECK(constantSource, "Unexpected non-constant source");

        mergedValue->constant |= constantSource->constant;
        mergedExtract->write_mode = extract->write_mode;
        mergedExtract->debug.mergeWith(extract->debug);
    }

    return mergedExtract;
}

/// Convert the sequence of Extract operations that have been passed to
/// `add()` so far into a sequence of LoweredExtract operations. Extracts
/// that write to the same container are merged together.
IR::Vector<IR::BFN::LoweredParserPrimitive> ExtractSimplifier::lowerExtracts(
    std::map<gress_t, std::map<unsigned, unsigned>> clotTagToCsumUnit) {
    IR::Vector<IR::BFN::LoweredParserPrimitive> loweredExtracts;

    for (const auto &item : extractFromPacketByContainer) {
        auto container = item.first;
        auto &extracts = item.second;
        auto &merged = mergeExtractsFor<IR::BFN::LoweredPacketRVal>(container, extracts);
        loweredExtracts.insert(loweredExtracts.end(), merged.begin(), merged.end());
    }

    sortExtractPhvs(loweredExtracts);

    for (const auto &item : extractFromBufferByContainer) {
        auto container = item.first;
        auto &extracts = item.second;
        auto &merged = mergeExtractsFor<IR::BFN::LoweredMetadataRVal>(container, extracts);
        loweredExtracts.insert(loweredExtracts.end(), merged.begin(), merged.end());
    }

    for (const auto &item : extractConstantByContainer) {
        auto container = item.first;
        auto &extracts = item.second;
        auto *merged = mergeExtractsForConstants(container, extracts);
        loweredExtracts.push_back(merged);
    }

    for (const auto &cx : clotExtracts) {
        auto *clot = cx.first;
        const auto *first_slice = clot->parser_state_to_slices().begin()->second.front();
        const auto *first_field = first_slice->field();

        bool is_start = false;
        nw_bitinterval bitInterval;

        for (const auto extract : cx.second) {
            auto rval = extract->source->to<IR::BFN::PacketRVal>();
            bitInterval = bitInterval.unionWith(rval->interval());

            // Figure out if the current extract includes the first bit in the CLOT.
            if (is_start) continue;

            // Make sure we're extracting the first field.
            auto dest = phv.field(extract->dest->field);
            if (dest != first_field) continue;

            if (extract->dest->field->is<IR::Member>()) {
                // Extracting the whole field.
                is_start = true;
                continue;
            }

            if (auto sl = extract->dest->field->to<IR::Slice>()) {
                if (int(sl->getH()) == first_slice->range().hi) {
                    // Extracted slice includes the first bit of the slice.
                    is_start = true;
                    continue;
                }
            }
        }

        nw_bitrange bitrange = *toClosedRange(bitInterval);
        nw_byterange byterange = bitrange.toUnit<RangeUnit::Byte>();

        auto *rval = new IR::BFN::LoweredPacketRVal(byterange);
        auto *extractClot = new IR::BFN::LoweredExtractClot(is_start, rval, clot);
        extractClot->higher_parser_state = cx.second.front()->original_state;
        if (clotTagToCsumUnit[clot->gress].count(clot->tag)) {
            clot->csum_unit = clotTagToCsumUnit[clot->gress][clot->tag];
        }
        loweredExtracts.push_back(extractClot);
    }

    return loweredExtracts;
}

}  // namespace Parde::Lowered
