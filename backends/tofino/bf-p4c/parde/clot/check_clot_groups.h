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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_CHECK_CLOT_GROUPS_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_CHECK_CLOT_GROUPS_H_

#include <iostream>

#include "clot_info.h"
#include "parde/parde_visitor.h"

class CheckClotGroups : public DeparserInspector {
    const PhvInfo &phv;
    const ClotInfo &clot;
    bool preorder(const IR::BFN::AbstractDeparser *);
    bool preorder(const IR::BFN::EmitField *);
    void postorder(const IR::BFN::AbstractDeparser *);

    unsigned DEPARSER_CHUNK_SIZE;
    unsigned DEPARSER_CHUNKS_PER_GROUP;
    unsigned DEPARSER_CHUNK_GROUPS;
    unsigned DEPARSER_CLOTS_PER_GROUP;

    struct fde_span {
        const Clot *clot = nullptr;  // nullptr for no clot/data from PHV
        unsigned size = 0;           // in bits
        unsigned chunk = 0;
        fde_span() {}
        explicit fde_span(const Clot *cl) : clot(cl) {}
    };
    std::vector<fde_span> field_dictionary;

 public:
    CheckClotGroups(const PhvInfo &phv, const ClotInfo &clot) : phv(phv), clot(clot) {
        auto &spec = Device::pardeSpec();
        DEPARSER_CHUNK_SIZE = spec.deparserChunkSize();
        DEPARSER_CHUNKS_PER_GROUP = spec.deparserChunkGroupSize();
        DEPARSER_CHUNK_GROUPS = spec.numDeparserChunkGroups();
        DEPARSER_CLOTS_PER_GROUP = spec.numClotsPerDeparserGroup();
    }
};

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_CHECK_CLOT_GROUPS_H_ */
