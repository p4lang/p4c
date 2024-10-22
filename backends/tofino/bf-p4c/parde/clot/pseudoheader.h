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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_PSEUDOHEADER_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_PSEUDOHEADER_H_

#include "bf-p4c/phv/phv_fields.h"
#include "field_slice_set.h"

/// Represents a sequence of fields that are always contiguously emitted by the deparser. A
/// pseudoheader may be emitted multiple times by the deparser, each time with a different POV bit.
class Pseudoheader : public LiftLess<Pseudoheader> {
    friend class ClotInfo;

 public:
    const int id;

    /// The set of all POV bits under which this pseudoheader is emitted.
    const PovBitSet pov_bits;

    /// The sequence of fields that constitute this pseudoheader.
    const std::vector<const PHV::Field *> fields;

 private:
    static int nextId;

 public:
    explicit Pseudoheader(const PovBitSet pov_bits, const std::vector<const PHV::Field *> fields)
        : id(nextId++), pov_bits(pov_bits), fields(fields) {}

    /// Ordering on id.
    bool operator<(const Pseudoheader &other) const { return id < other.id; }
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_PSEUDOHEADER_H_ */
