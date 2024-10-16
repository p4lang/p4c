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

#ifndef EXTENSIONS_BF_P4C_PARDE_CLOT_PSEUDOHEADER_H_
#define EXTENSIONS_BF_P4C_PARDE_CLOT_PSEUDOHEADER_H_

#include "field_slice_set.h"
#include "bf-p4c/phv/phv_fields.h"

/// Represents a sequence of fields that are always contiguously emitted by the deparser. A
/// pseudoheader may be emitted multiple times by the deparser, each time with a different POV bit.
class Pseudoheader : public LiftLess<Pseudoheader> {
    friend class ClotInfo;

 public:
    const int id;

    /// The set of all POV bits under which this pseudoheader is emitted.
    const PovBitSet pov_bits;

    /// The sequence of fields that constitute this pseudoheader.
    const std::vector<const PHV::Field*> fields;

 private:
    static int nextId;

 public:
    explicit Pseudoheader(const PovBitSet pov_bits,
                          const std::vector<const PHV::Field*> fields)
        : id(nextId++), pov_bits(pov_bits), fields(fields) { }

    /// Ordering on id.
    bool operator<(const Pseudoheader& other) const {
        return id < other.id;
    }
};

#endif /* EXTENSIONS_BF_P4C_PARDE_CLOT_PSEUDOHEADER_H_ */
