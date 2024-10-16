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

#include "bf-p4c/mau/mau_spec.h"

#define MAU_SPEC_UNSUPPORTED                                     \
    BUG("Unsupported: a base class was used in a context/place " \
        "where only a derived one is permitted.");

int TofinoIMemSpec::rows() const { return 32; }

int TofinoIMemSpec::colors() const { return 2; }

int TofinoIMemSpec::color_bits() const { return 1; }

int TofinoIMemSpec::address_bits() const { return 6; }

int TofinoIMemSpec::map_table_entries() const { return 8; }


int IXBarSpec::byteGroups() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::exactBytesPerGroup() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::exactGroups() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::fairModeHashBits() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::gatewaySearchBytes() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::hashDistBits() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::hashDistExpandBits() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::hashDistMaxMaskBits() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::hashDistSlices() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::hashDistUnits() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::hashGroups() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::hashIndexGroups() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::hashMatrixSize() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::hashParityBit() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::hashSingleBits() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::hashTables() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::lpfInputBytes() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::maxHashBits() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::meterAluHashBits() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::meterAluHashParityByteStart() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::meterPrecolorSize() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::ramLineSelectBits() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::ramSelectBitStart() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::repeatingConstraintSect() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::resilientModeHashBits() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::ternaryBytesPerBigGroup() const { MAU_SPEC_UNSUPPORTED }

int IXBarSpec::tofinoMeterAluByteOffset() const { MAU_SPEC_UNSUPPORTED }


IR::Node *MauSpec::postTransformTables(IR::MAU::Table *const table) const {
    return table;
}

int MauSpec::tcam_rows() const { return Tofino_tcam_rows; }

int MauSpec::tcam_columns() const { return Tofino_tcam_columns; }

int MauSpec::tcam_width() const { return 44; }

int MauSpec::tcam_depth() const { return 512; }

TofinoIXBarSpec::TofinoIXBarSpec() = default;

int TofinoIXBarSpec::byteGroups() const { return StageUse::MAX_TERNARY_GROUPS / 2; }

int TofinoIXBarSpec::hashDistMaxMaskBits() const { return hashDistBits() + hashDistExpandBits(); }

int TofinoIXBarSpec::hashMatrixSize() const { return ramSelectBitStart() + hashSingleBits(); }

int TofinoIXBarSpec::ternaryGroups() const { return StageUse::MAX_TERNARY_GROUPS; }

int TofinoIXBarSpec::exactBytesPerGroup() const { return 16; }

int TofinoIXBarSpec::exactGroups() const { return 8; }

int TofinoIXBarSpec::fairModeHashBits() const { return 14; }

int TofinoIXBarSpec::gatewaySearchBytes() const { return 4; }

int TofinoIXBarSpec::hashDistBits() const { return 16; }

int TofinoIXBarSpec::hashDistExpandBits() const { return 7; }

int TofinoIXBarSpec::hashDistSlices() const { return 3; }

int TofinoIXBarSpec::hashDistUnits() const { return 2; }

int TofinoIXBarSpec::hashGroups() const { return 8; }

int TofinoIXBarSpec::hashIndexGroups() const { return 4; }

int TofinoIXBarSpec::hashParityBit() const { return 51; }

int TofinoIXBarSpec::hashSingleBits() const { return 12; }

int TofinoIXBarSpec::hashTables() const { return 16; }

int TofinoIXBarSpec::lpfInputBytes() const { return 4; }

int TofinoIXBarSpec::maxHashBits() const { return 52; }

int TofinoIXBarSpec::meterAluHashBits() const { return 52; }

int TofinoIXBarSpec::meterAluHashParityByteStart() const { return 48; }

int TofinoIXBarSpec::meterPrecolorSize() const { return 2; }

int TofinoIXBarSpec::ramLineSelectBits() const { return 10; }

int TofinoIXBarSpec::ramSelectBitStart() const { return 40; }

int TofinoIXBarSpec::repeatingConstraintSect() const { return 4; }

int TofinoIXBarSpec::resilientModeHashBits() const { return 51; }

int TofinoIXBarSpec::ternaryBytesPerBigGroup() const { return 11; }

int TofinoIXBarSpec::ternaryBytesPerGroup() const { return 5; }

int TofinoIXBarSpec::tofinoMeterAluByteOffset() const { return 8; }

int TofinoIXBarSpec::tcam_rows() const { return Tofino_tcam_rows; }

int TofinoIXBarSpec::tcam_columns() const { return Tofino_tcam_columns; }

int TofinoIXBarSpec::exactMatchTotalBytes() const { return 8 * 16; }

int TofinoIXBarSpec::ternaryMatchTotalBytes() const { return 6 * 11; }

int TofinoIXBarSpec::xcmpMatchTotalBytes() const { return 0; }

const IXBarSpec &TofinoMauSpec::getIXBarSpec() const { return ixbar_; }

const IMemSpec &TofinoMauSpec::getIMemSpec() const { return imem_; }

const IMemSpec &JBayMauSpec::getIMemSpec() const { return imem_; }

const IXBarSpec &JBayMauSpec::getIXBarSpec() const { return ixbar_; }


