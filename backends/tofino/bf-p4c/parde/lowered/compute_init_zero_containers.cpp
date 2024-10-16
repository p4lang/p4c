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

#include "compute_init_zero_containers.h"

namespace Parde::Lowered {

void ComputeInitZeroContainers::postorder(IR::BFN::LoweredParser* parser) {
    ordered_set<PHV::Container> zero_init_containers;
    ordered_set<PHV::Container> intrinsic_invalidate_containers;

    auto ctxt = PHV::AllocContext::PARSER;
    for (const auto& f : phv) {
        if (f.gress != parser->gress) continue;

        // POV bits are treated as metadata
        if (f.pov || f.metadata) {
            f.foreach_alloc(ctxt, nullptr, [&](const PHV::AllocSlice& alloc) {
                bool hasHeaderField = false;

                for (auto fc : phv.fields_in_container(alloc.container())) {
                    if (!fc->metadata && !fc->pov) {
                        hasHeaderField = true;
                        break;
                    }
                }

                if (!hasHeaderField) zero_init_containers.insert(alloc.container());
            });
        }

        if (f.is_invalidate_from_arch()) {
            // Track the allocated containers for fields that are invalidate_from_arch
            f.foreach_alloc(ctxt, nullptr, [&](const PHV::AllocSlice& alloc) {
                intrinsic_invalidate_containers.insert(alloc.container());
                LOG3(alloc.container() << " contains intrinsic invalidate fields");
            });
            continue;
        }

        if (defuse.hasUninitializedRead(f.id)) {
            // If pa_no_init specified, then the field does not have to rely on parser zero
            // initialization.
            if (no_init_fields.count(&f)) continue;
            f.foreach_alloc(ctxt, nullptr, [&](const PHV::AllocSlice& alloc) {
                zero_init_containers.insert(alloc.container());
            });
        }
    }

    if (origParserZeroInitContainers.count(parser->gress))
        for (auto& c : origParserZeroInitContainers.at(parser->gress))
            zero_init_containers.insert(c);

    for (auto& c : zero_init_containers) {
        // Containers for intrinsic invalidate_from_arch metadata should be left
        // uninitialized, therefore skip zero-initialization
        if (!intrinsic_invalidate_containers.count(c)) {
            parser->initZeroContainers.push_back(new IR::BFN::ContainerRef(c));
            LOG3("parser init " << c);
        }
    }

    // Also initialize the container validity bits for the zero-ed containers (as part of
    // deparsed zero optimization) to 1.
    for (auto& c : phv.getZeroContainers(parser->gress)) {
        if (intrinsic_invalidate_containers.count(c))
            BUG("%1% used for both init-zero and intrinsic invalidate field?", c);

        parser->initZeroContainers.push_back(new IR::BFN::ContainerRef(c));
    }
}

}  // namespace Parde::Lowered
