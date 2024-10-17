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

#ifndef EXTENSIONS_BF_P4C_PARDE_LOWERED_HELPERS_H_
#define EXTENSIONS_BF_P4C_PARDE_LOWERED_HELPERS_H_

#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/phv/utils/slice_alloc.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/stringref.h"

namespace Parde::Lowered {

/// @return a version of the provided state name which is safe to use in the
/// generated assembly.
cstring sanitizeName(StringRef name);

/**
 * Construct debugging information describing a slice of a field.
 *
 * @param fieldName  Field name.
 * @param slice  An `alloc_slice` mapping a range of bits in the field to a
 *               range of bits in a container.
 * @param includeContainerInfo  If true, the result will include information
 *                              about which bits in the container the field
 *                              slice was mapped to.
 * @return a string describing which bits in the field are included in the
 * slice, and describing the corresponding bits in the container.
 */
cstring debugInfoFor(const cstring fieldName, const PHV::AllocSlice& slice,
                     bool includeContainerInfo = true);

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
cstring debugInfoFor(const IR::BFN::ParserLVal* lval, const PHV::AllocSlice& slice,
                     bool includeContainerInfo = true);

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
cstring debugInfoFor(const IR::BFN::Extract* extract, const PHV::AllocSlice& slice,
                     const nw_bitrange& bufferRange = nw_bitrange(), bool includeRangeInfo = false);

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
        const le_bitrange& pov_range, const cstring pov_type_string = ""_cs);

/// Maps a POV bit field to a single bit within a container, represented as a
/// ContainerBitRef. Checks that the allocation for the POV bit field is sane.
const IR::BFN::ContainerBitRef* lowerSingleBit(const PhvInfo& phv,
                                               const IR::BFN::FieldLVal* fieldRef,
                                               const PHV::AllocContext* ctxt);

/// Maps a sequence of fields to a sequence of PHV containers. The sequence of
/// fields is treated as ordered and non-overlapping; the resulting container
/// sequence is the shortest one which maintains these properties.
std::pair<IR::Vector<IR::BFN::ContainerRef>, std::vector<Clot*>> lowerFields(
    const PhvInfo& phv, const ClotInfo& clotInfo, const IR::Vector<IR::BFN::FieldLVal>& fields,
    bool is_checksum = false);

/// Maps a field which cannot be split between multiple containers to a single
/// container, represented as a ContainerRef. Checks that the allocation for the
/// field is sane.
const IR::BFN::ContainerRef* lowerUnsplittableField(const PhvInfo& phv, const ClotInfo& clotInfo,
                                                    const IR::BFN::FieldLVal* fieldRef,
                                                    const char* unsplittableReason);

// Deparser checksum engine exposes input entries as 16-bit.
// PHV container entry needs a swap if the field's 2-byte alignment
// in the container is not same as the alignment in the packet layout
// i.e. off by 1 byte. For example, this could happen if "ipv4.ttl" is
// allocated to a 8-bit container.
std::map<PHV::Container, unsigned> getChecksumPhvSwap(const PhvInfo& phv,
                                                      const IR::BFN::EmitChecksum* emitChecksum);

/// Given a sequence of fields, construct a packing format describing how the
/// fields will be laid out once they're lowered to containers.
const safe_vector<IR::BFN::DigestField>* computeControlPlaneFormat(
    const PhvInfo& phv, const IR::Vector<IR::BFN::FieldLVal>& fields);

}  // namespace Parde::Lowered

#endif /* EXTENSIONS_BF_P4C_PARDE_LOWERED_HELPERS_H_ */
