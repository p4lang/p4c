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
