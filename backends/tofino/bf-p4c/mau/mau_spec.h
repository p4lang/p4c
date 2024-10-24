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

#ifndef BACKENDS_TOFINO_BF_P4C_MAU_MAU_SPEC_H_
#define BACKENDS_TOFINO_BF_P4C_MAU_MAU_SPEC_H_

#include "ir/ir.h"

// device-specific parameters for MAU/PPU.
using namespace P4;

class IMemSpec {
 public:
    virtual int rows() const = 0;        //  pure virtual
    virtual int colors() const = 0;      //  pure virtual
    virtual int color_bits() const = 0;  //  pure virtual
    virtual int address_bits() const = 0;
    virtual int map_table_entries() const = 0;
};

class TofinoIMemSpec : public IMemSpec {
    int rows() const override;
    int colors() const override;
    int color_bits() const override;
    int address_bits() const override;
    int map_table_entries() const override;
};

class IXBarSpec {
 public:
    /* --- common --- */
    virtual int ternaryBytesPerGroup() const = 0;  //  pure virtual
    virtual int ternaryGroups() const = 0;         //  pure virtual

    // the next two: support for "legacy code" [as of Nov. 10 2022] that gets these via IXBarSpec
    virtual int tcam_rows() const = 0;     //  pure virtual
    virtual int tcam_columns() const = 0;  //  pure virtual

    /* --- Tofino[1, 2] --- */
    virtual int byteGroups() const;
    virtual int exactBytesPerGroup() const;
    virtual int exactGroups() const;
    virtual int fairModeHashBits() const;
    virtual int gatewaySearchBytes() const;
    virtual int hashDistBits() const;
    virtual int hashDistExpandBits() const;
    virtual int hashDistMaxMaskBits() const;
    virtual int hashDistSlices() const;
    virtual int hashDistUnits() const;
    virtual int hashGroups() const;
    virtual int hashIndexGroups() const;
    virtual int hashMatrixSize() const;
    virtual int hashParityBit() const;
    virtual int hashSingleBits() const;
    virtual int hashTables() const;
    virtual int lpfInputBytes() const;
    virtual int maxHashBits() const;
    virtual int meterAluHashBits() const;
    virtual int meterAluHashParityByteStart() const;
    virtual int meterPrecolorSize() const;
    virtual int ramLineSelectBits() const;
    virtual int ramSelectBitStart() const;
    virtual int repeatingConstraintSect() const;
    virtual int resilientModeHashBits() const;
    virtual int ternaryBytesPerBigGroup() const;
    virtual int tofinoMeterAluByteOffset() const;

    virtual int getExactOrdBase(int group) const = 0;
    virtual int getTernaryOrdBase(int group) const = 0;
    virtual int exactMatchTotalBytes() const = 0;
    virtual int ternaryMatchTotalBytes() const = 0;
    virtual int xcmpMatchTotalBytes() const = 0;
};

// TU-local constants to avoid circular dependencies between IXBarSpec subclasses
//   and their respective MauSpec subclasses, while still preserving DRY compliance.
static constexpr int Tofino_tcam_rows = 12;
static constexpr int Tofino_tcam_columns = 2;

class MauSpec {
 public:
    virtual const IXBarSpec &getIXBarSpec() const = 0;  //  pure virtual
    virtual const IMemSpec &getIMemSpec() const = 0;    //  pure virtual

    // Called at the end of table rewriting in TablePlacement::TransformTables to do
    // any target-specific fixups needed
    virtual IR::Node *postTransformTables(IR::MAU::Table *const table) const;

    // The next 4 lines: correct data for Tof.1 + Tof.2 + Tof.3; must override elsewhere for Tof.5
    virtual int tcam_rows() const;
    virtual int tcam_columns() const;
    virtual int tcam_width() const;
    virtual int tcam_depth() const;
};

class TofinoIXBarSpec : public IXBarSpec {
 public:
    TofinoIXBarSpec();

    int byteGroups() const override;

    int hashDistMaxMaskBits() const override;

    int hashMatrixSize() const override;

    int ternaryGroups() const override;

    int exactBytesPerGroup() const override;
    int exactGroups() const override;
    int fairModeHashBits() const override;
    int gatewaySearchBytes() const override;
    int hashDistBits() const override;
    int hashDistExpandBits() const override;
    int hashDistSlices() const override;
    int hashDistUnits() const override;
    int hashGroups() const override;

    int hashIndexGroups() const override;
    /* groups of 10 bits for indexing */

    int hashParityBit() const override;
    /* If enabled reserved parity bit position */

    int hashSingleBits() const override;
    /* top 12 bits of hash table individually */

    int hashTables() const override;
    int lpfInputBytes() const override;
    int maxHashBits() const override;
    int meterAluHashBits() const override;
    int meterAluHashParityByteStart() const override;
    int meterPrecolorSize() const override;
    int ramLineSelectBits() const override;
    int ramSelectBitStart() const override;
    int repeatingConstraintSect() const override;
    int resilientModeHashBits() const override;
    int ternaryBytesPerBigGroup() const override;
    int ternaryBytesPerGroup() const override;
    int tofinoMeterAluByteOffset() const override;

    // the next two: support for "legacy code" [as of Nov. 10 2022] that gets these via IXBarSpec
    int tcam_rows() const override;
    int tcam_columns() const override;

    int getExactOrdBase(int group) const override;
    int getTernaryOrdBase(int group) const override;

    int exactMatchTotalBytes() const override;
    int ternaryMatchTotalBytes() const override;
    int xcmpMatchTotalBytes() const override;
};

class TofinoMauSpec : public MauSpec {
    const TofinoIXBarSpec ixbar_;
    const TofinoIMemSpec imem_;

 public:
    TofinoMauSpec() = default;
    const IXBarSpec &getIXBarSpec() const override;
    const IMemSpec &getIMemSpec() const override;
};

class JBayMauSpec : public MauSpec {
    const TofinoIXBarSpec ixbar_;
    const TofinoIMemSpec imem_;

 public:
    JBayMauSpec() = default;
    const IXBarSpec &getIXBarSpec() const override;
    const IMemSpec &getIMemSpec() const override;
};

#endif /* BACKENDS_TOFINO_BF_P4C_MAU_MAU_SPEC_H_ */
