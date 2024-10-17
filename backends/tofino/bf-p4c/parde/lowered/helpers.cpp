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

#include "helpers.h"

#include <utility>

#include <boost/range/adaptor/reversed.hpp>

#include "bf-p4c/arch/bridge_metadata.h"

namespace Parde::Lowered {

cstring sanitizeName(StringRef name) {
    // Drop any thread-specific prefix from the name.
    if (auto prefix = name.findstr("::"))
        name = name.after(prefix) += 2;
    return name;
}

cstring debugInfoFor(const cstring fieldName, const PHV::AllocSlice& slice,
                     bool includeContainerInfo) {
    std::stringstream info;
    // Describe the range of bits assigned to this field slice in the container.
    // (In some cases we break this down in more detail elsewhere, so we don't
    // need to repeat it.)
    if (includeContainerInfo) {
        const le_bitrange sourceBits = slice.container_slice();
        if (sourceBits.size() != ssize_t(slice.container().size()))
            info << sourceBits << ": ";
    }

    // Identify the P4 field that we're writing to.
    info << fieldName;

    // Although it's confusing given that e.g. input buffer ranges are printed
    // in network order, consistency with the rest of the output of the
    // assembler requires that we describe partial writes to a field in little
    // endian order.
    const le_bitrange destFieldBits = slice.field_slice();
    if (slice.field()->size != destFieldBits.size())
        info << "." << destFieldBits.lo << "-" << destFieldBits.hi;

    return cstring(info);
}

/**
 * Construct debugging information describing a slice of a field.
 *
 * @param lval  A reference to a field.
 * @param slice  An `alloc_slice` mapping a range of bits in the field to a
 *               range of bits in a container.
 * @param includeContainerInfo  If true, the result will include information
 *                              about which bits in the container the field
 *                              slice was mapped to.
 * @return a string describing which bits in the field are included in the
 * slice, and describing the corresponding bits in the container.
 */
cstring debugInfoFor(const IR::BFN::ParserLVal* lval,
                     const PHV::AllocSlice& slice,
                     bool includeContainerInfo) {
    auto fieldRef = lval->to<IR::BFN::FieldLVal>();
    if (!fieldRef) return ""_cs;

    return debugInfoFor(fieldRef->field->toString(), slice, includeContainerInfo);
}

/**
 * Construct a string describing how an `Extract` primitive was mapped to a
 * hardware extract operation.
 *
 * @param extract  The original `Extract` primitive, with a field as the
 *                 destination.
 * @param slice  An `alloc_slice` mapping a range of bits in the field to a
 *               range of bits in a container.
 * @param bufferRange  For extracts that read from the input buffer, an input
 *                     buffer range corresponding to the range of bits in the
 *                     `alloc_slice`.
 * @param includeRangeInfo  If true, the range of bits being extracted into
 *                          the destination container is printed even if it
 *                          matches the size of the container exactly.
 * @return a string containing debugging info describing the mapping between the
 * field, the container, and the constant or input buffer region read by the
 * `Extract`.
 */
cstring debugInfoFor(const IR::BFN::Extract* extract,
                     const PHV::AllocSlice& slice,
                     const nw_bitrange& bufferRange,
                     bool includeRangeInfo) {
    std::stringstream info;

    // Describe the value that's being written into the destination container.
    if (auto* constantSource = extract->source->to<IR::BFN::ConstantRVal>()) {
        info << "value " << constantSource->constant << " -> "
             << slice.container() << " " << slice.container_slice() << ": ";
    } else if (extract->source->is<IR::BFN::PacketRVal>()) {
        // In the interest of brevity, don't print the range of bits being
        // extracted into the destination container if it matches the size of
        // the container exactly.
        // This behaviour can be overridden by explicit true value of
        // includeRangeInfo parameter in case we desire to print the range always.
        if (includeRangeInfo || slice.container().size() != size_t(bufferRange.size()))
            info << bufferRange << " -> " << slice.container() << " "
                 << slice.container_slice() << ": ";
    } else if (extract->source->is<IR::BFN::MetadataRVal>()) {
        info << "buffer mapped I/O: " << bufferRange << " -> "
             << slice.container() << " " << slice.container_slice() << ": ";
    }

    // Describe the field slice that we're writing to.
    info << debugInfoFor(extract->dest, slice, /* includeContainerInfo = */ false);

    return cstring(info);
}

/**
 * Construct a string describing how an `Extract` primitive was mapped to a
 * hardware extract operation (in case of extract from POV (state/flags) bits).
 *
 * @param extract  The original `Extract` primitive, with a field as the
 *                 destination.
 * @param slice  An `alloc_slice` mapping a range of bits in the field to a
 *               range of bits in a container.
 * @param pov_range  The range corresponding to the bits from POV state/flags bits.
 * @param pov_type_string  Optional string with info about type of POV (state/flags).
 * @return a string containing debugging info describing the mapping between the
 * field, the container, and POV state/flags bits read by the `Extract`.
 */
cstring debugInfoFor(const IR::BFN::Extract* extract, const PHV::AllocSlice& slice,
        const le_bitrange& pov_range, const cstring pov_type_string) {
    std::stringstream info;
    /// Describe the value that's being written into the destination container.
    info << "POV " << pov_type_string << ((pov_type_string != "") ? " " : "") <<
            pov_range << " -> " << slice.container() << " " <<
            slice.container_slice() << ": " <<
    /// Describe the field slice that we're writing to.
            debugInfoFor(extract->dest, slice, /* includeContainerInfo = */ false);
    return cstring(info);
}

/// Maps a POV bit field to a single bit within a container, represented as a
/// ContainerBitRef. Checks that the allocation for the POV bit field is sane.
const IR::BFN::ContainerBitRef* lowerSingleBit(const PhvInfo& phv,
                                               const IR::BFN::FieldLVal* fieldRef,
                                               const PHV::AllocContext* ctxt) {
    le_bitrange range;
    auto* field = phv.field(fieldRef->field, &range);

    std::vector<PHV::AllocSlice> slices;
    field->foreach_alloc(&range, ctxt, nullptr,
                         [&](const PHV::AllocSlice& alloc) { slices.push_back(alloc); });

    BUG_CHECK(!slices.empty(), "bit %1% didn't receive a PHV allocation", fieldRef->field);
    BUG_CHECK(slices.size() == 1,
              "bit %1% is somehow split across "
              "multiple containers?",
              fieldRef->field);

    auto container = new IR::BFN::ContainerRef(slices.back().container());
    auto containerRange = slices.back().container_slice();
    BUG_CHECK(containerRange.size() == 1, "bit %1% is multiple bits?", fieldRef->field);

    auto* bit = new IR::BFN::ContainerBitRef(container, containerRange.lo);
    LOG5("Mapping bit field " << fieldRef->field << " to " << bit);
    bit->debug.info.push_back(debugInfoFor(fieldRef, slices.back(),
                                           /* includeContainerInfo = */ false));
    return bit;
}

std::pair<IR::Vector<IR::BFN::ContainerRef>, std::vector<Clot*>> lowerFields(
    const PhvInfo& phv, const ClotInfo& clotInfo, const IR::Vector<IR::BFN::FieldLVal>& fields,
    bool is_checksum) {
    IR::Vector<IR::BFN::ContainerRef> containers;
    std::vector<Clot*> clots;

    IR::BFN::ContainerRef* last = nullptr;
    // Perform a left fold over the field sequence and merge contiguous fields
    // which have been placed in the same container into a single container
    // reference.
    for (auto* fieldRef : fields) {
        auto field = phv.field(fieldRef->field);
        assoc::map<const PHV::FieldSlice*, Clot*, PHV::FieldSlice::Greater>* slice_clots = nullptr;
        if (clotInfo.is_readonly(field) && is_checksum) {
            slice_clots = clotInfo.slice_clots(field);
        } else {
            slice_clots = clotInfo.allocated_slices(field);
        }
        if (slice_clots) {
            for (const auto &entry : *slice_clots) {
                auto clot = entry.second;
                if (clots.empty() || clots.back() != clot) clots.push_back(clot);
            }

            if (clotInfo.fully_allocated(field) ||
                (clotInfo.is_readonly(field) && clotInfo.whole_field_clot(field)))
                continue;
        }

        // padding in digest list does not need phv allocation
        if (field->is_ignore_alloc()) continue;

        PHV::FieldUse use(PHV::FieldUse::READ);
        std::vector<PHV::AllocSlice> slices =
            phv.get_alloc(fieldRef->field, PHV::AllocContext::DEPARSER, &use);

        BUG_CHECK(!slices.empty(), "Emitted field didn't receive a PHV allocation: %1%",
                  fieldRef->field);

        for (auto& slice : boost::adaptors::reverse(slices)) {
            BUG_CHECK(bool(slice.container()),
                      "Emitted field was allocated to "
                      "an invalid PHV container: %1%",
                      fieldRef->field);

            const nw_bitrange containerRange =
                slice.container_slice().toOrder<Endian::Network>(slice.container().size());

            if (last && last->container == slice.container()) {
                auto lastRange = *(last->range);
                if (lastRange.hi < containerRange.lo) {
                    LOG5(" - Merging in " << fieldRef->field);
                    last->debug.info.push_back(debugInfoFor(fieldRef, slice));
                    last->range = lastRange.unionWith(containerRange);
                    continue;
                }
            }

            LOG5("Deparser: lowering field " << fieldRef->field << " to " << slice.container());

            last = new IR::BFN::ContainerRef(slice.container());
            last->range = containerRange;
            last->debug.info.push_back(debugInfoFor(fieldRef, slice));
            containers.push_back(last);

            if (slice.field()->is_checksummed() && slice.field()->is_solitary()) {
                // Since the field has a solitary constraint, its is safe to
                // extend the range till the end of container
                last->range =
                    containerRange.unionWith(nw_bitrange(StartLen(0, slice.container().size())));
            }
        }
    }

    return {containers, clots};
}

const IR::BFN::ContainerRef* lowerUnsplittableField(const PhvInfo& phv, const ClotInfo& clotInfo,
                                                    const IR::BFN::FieldLVal* fieldRef,
                                                    const char* unsplittableReason) {
    IR::Vector<IR::BFN::ContainerRef> containers;
    std::tie(containers, std::ignore) = lowerFields(phv, clotInfo, {fieldRef});
    BUG_CHECK(containers.size() == 1,
              "Field %1% must be placed in a single container because it's "
              "used in the deparser as a %2%, but it was sliced across %3% "
              "containers",
              fieldRef->field, unsplittableReason, containers.size());
    return containers.back();
}

std::map<PHV::Container, unsigned> getChecksumPhvSwap(const PhvInfo& phv,
                                                      const IR::BFN::EmitChecksum* emitChecksum) {
    std::map<PHV::Container, unsigned> containerToSwap;
    for (auto source : emitChecksum->sources) {
        auto* phv_field = phv.field(source->field->field);
        PHV::FieldUse use(PHV::FieldUse::READ);
        std::vector<PHV::AllocSlice> slices = phv.get_alloc(phv_field, nullptr,
                PHV::AllocContext::DEPARSER, &use);
        int offset = source->offset;
        for (auto& slice : boost::adaptors::reverse(slices)) {
            unsigned swap = 0;
            bool isResidualChecksum = false;
            std::string f_name(phv_field->name.c_str());
            if (f_name.find(BFN::COMPILER_META) != std::string::npos
             && f_name.find("residual_checksum_") != std::string::npos)
                isResidualChecksum = true;
            // If a field slice is on an even byte in the checksum operation field list
            // and even byte in the container and vice-versa then swap is true
            // Offset : offset of the field slice is offset of the field + difference between
            // field.hi and slice.hi
            if (!isResidualChecksum &&
                ((offset + phv_field->size - slice.field_slice().hi -1)/8) % 2 ==
                 (slice.container_slice().hi/8) % 2) {
                swap = (1 << slice.container_slice().hi/16U) |
                             (1 << slice.container_slice().lo/16U);
            }
            containerToSwap[slice.container()] |= swap;
        }
    }
    return containerToSwap;
}

/// Given a sequence of fields, construct a packing format describing how the
/// fields will be laid out once they're lowered to containers.
const safe_vector<IR::BFN::DigestField>*
computeControlPlaneFormat(const PhvInfo& phv,
                          const IR::Vector<IR::BFN::FieldLVal>& fields) {
    struct LastContainerInfo {
        /// The container into which the last field was placed.
        PHV::Container container;
        /// The number of unused bits which remain on the LSB side of the
        /// container after the last field was placed.
        int remainingBitsInContainer = 0;
    };

    std::optional<LastContainerInfo> last = std::nullopt;
    unsigned totalWidth = 0;
    auto *packing = new safe_vector<IR::BFN::DigestField>();

    // Walk over the field sequence in network order and construct a
    // FieldPacking that reflects its structure, with padding added where
    // necessary to reflect gaps between the fields.
    for (auto* fieldRef : fields) {
        LOG5("Computing digest packing for field : " << fieldRef);
        PHV::FieldUse use(PHV::FieldUse::READ);
        std::vector<PHV::AllocSlice> slices = phv.get_alloc(fieldRef->field,
                PHV::AllocContext::DEPARSER, &use);

        // padding in digest list does not need phv allocation
        auto field = phv.field(fieldRef->field);
        if (field->is_ignore_alloc())
            continue;

        BUG_CHECK(!slices.empty(),
                  "Emitted field didn't receive a PHV allocation: %1%",
                  fieldRef->field);

        // Confusingly, the first slice in network order is the *last* one in
        // `slices` because `foreach_alloc()` (and hence `get_alloc()`)
        // enumerates the slices in increasing order of their little endian
        // offset, which means that in terms of network order it walks the
        // slices backwards.
        for (std::vector<PHV::AllocSlice>::reverse_iterator slice = slices.rbegin();
                slice != slices.rend(); slice++) {
            const nw_bitrange sliceContainerRange = slice->container_slice()
                        .toOrder<Endian::Network>(slice->container().size());

            unsigned packStartByte = 0;

            // If we switched containers (or if this is the very first field),
            // appending padding equivalent to the bits at the end of the previous
            // container and the beginning of the new container that aren't
            // occupied.
            if (last && last->container != slice->container()) {
                totalWidth += last->remainingBitsInContainer;
                totalWidth += sliceContainerRange.lo;
            } else if (!last) {
                totalWidth += sliceContainerRange.lo;
            }
            // The actual start byte on all packings are incremented by 1 during
            // assembly generation to factor in the select byte
            packStartByte = totalWidth / 8;
            totalWidth += slice->width();

            // Place the field slice in the packing format. The field name is
            // used in assembly generation; hence, we use its external name.
            auto packFieldName = slice->field()->externalName();
            // The pack start bit refers to the start bit within the container
            // in network order. This is the hi bit in byte of the container slice
            //  e.g.
            //  - W3(0..3)  # bit[3..0]: ingress::md.y2
            // The start bit in this example is bit 3
            auto packStartBit = slice->container_slice().hi % 8;
            auto packWidth = slice->width();
            auto packFieldStartBit = slice->field_slice().lo;
            packing->emplace_back(packFieldName, packStartByte, packStartBit,
                                                    packWidth, packFieldStartBit);
            LOG5("  Packing digest field slice : " << *slice
                    << " with startByte : " << packStartByte
                    << ", startBit: " << packStartBit << ", width: " << packWidth
                    << ", fields start bit (phv_offset) : " << packFieldStartBit);

            // Remember information about the container placement of the last slice
            // in network order (the first one in `slices`) so we can add any
            // necessary padding on the next pass around the loop.
            last = LastContainerInfo{ slice->container(), slice->container_slice().lo };
        }
    }

    return packing;
}

}  // namespace Parde::Lowered
