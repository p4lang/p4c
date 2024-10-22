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

#ifndef BF_P4C_PHV_PARDE_PHV_CONSTRAINTS_H_
#define BF_P4C_PHV_PARDE_PHV_CONSTRAINTS_H_

#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/pa_container_size.h"
#include "ir/ir.h"

/** This class is meant to gather parser/deparser constraints related to digests. Here is a running
 * list of constraints gathered by this pass:
 * - The total size of containers containing fields that are involved in a learning quanta is
 *   Device::maxDigestSizeInBytes().
 *
 * In both Tofino and Tofino2, the maximum size of learning quanta is 384b (48B). This class
 * introduces a learning quanta threshold (set currently to 90% of the maximum learning quanta
 * size). If the learning quanta exceeds this threshold, then it imposes the following constraints
 * on PHV fields used in that learning quanta:
 * - If the field is byte aligned, the exact containers property for the field is set to true and
 *   the field is set to solitary (to prevent other fields in the same header from being tacked on
 *   and forcing allocation into a larger container size).
 * - If the field is less than 32b, the field is made no_split.
 * - Depending on the size of field (for fields less than 32b in size), pa_container_size pragmas
 *   are inserted to occupy the smallest size of containers possible.
 */
class PardePhvConstraints : public Inspector {
 private:
    static constexpr int MAX_CONSTANT_WINDOW = 3;

    /// Maximum size of learning quanta in bytes.
    unsigned DIGEST_BYTES_THRESHOLD;

    PhvInfo &phv;
    PragmaContainerSize &sizePragmas;

    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::BFN::Digest *digest) override;

 public:
    explicit PardePhvConstraints(PhvInfo &p, PragmaContainerSize &pa) : phv(p), sizePragmas(pa) {
        // Set the threshold for setting restrictions of unused bits in digest fields to 90% of the
        // maximum.
        DIGEST_BYTES_THRESHOLD = Device::maxDigestSizeInBytes() * 9 / 10;
    }
};

class TofinoParserConstantExtract : public Inspector {
 private:
    PhvInfo &phv;

    ordered_map<const IR::BFN::ParserState *, ordered_set<PHV::Field *>> stateToPOVMap;

    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::BFN::Extract *extract) override;
    void end_apply() override;

 public:
    explicit TofinoParserConstantExtract(PhvInfo &p) : phv(p) {}
};

#endif /* BF_P4C_PHV_PARDE_PHV_CONSTRAINTS_H_ */
